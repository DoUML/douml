// *************************************************************************
//
// Copyright 2004-2010 Bruno PAGES  .
// Copyright 2012-2013 Nikolai Marchenko.
// Copyright 2012-2013 Leonardo Guilherme.
//
// This file is part of the DOUML Uml Toolkit.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License Version 3.0 as published by
// the Free Software Foundation and appearing in the file LICENSE.GPL included in the
//  packaging of this file.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License Version 3.0 for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//
// e-mail : doumleditor@gmail.com
// home   : http://sourceforge.net/projects/douml
//
// *************************************************************************





#include <qcursor.h>
#include <qpainter.h>
#include <q3popupmenu.h>
//Added by qt3to4:
#include <QTextStream>


#include "SubjectCanvas.h"
#include "MyInputDialog.h"
#include "UmlGlobal.h"
#include "UmlCanvas.h"
#include "BrowserDiagram.h"
#include "Settings.h"
#include "SettingsDialog.h"
#include "myio.h"
#include "ui/menufactory.h"
#include "strutil.h"
#include "ToolCom.h"
#include "translate.h"

SubjectCanvas::SubjectCanvas(UmlCanvas * canvas, int x, int y, int id)
    : DiagramCanvas(0, canvas, x, y, SUBJECT_CANVAS_MIN_SIZE,
                    SUBJECT_CANVAS_MIN_SIZE, id)
{
    browser_node = canvas->browser_diagram();
    itscolor = UmlDefaultColor;
    check_size();
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
}

SubjectCanvas::~SubjectCanvas()
{
}

void SubjectCanvas::delete_it()
{
    disconnect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
    DiagramCanvas::delete_it();
}


void SubjectCanvas::draw(QPainter & p)
{
    if (! visible()) return;

    p.setRenderHint(QPainter::Antialiasing, true);
    QRect r = rect();
    QColor bckgrnd = p.backgroundColor();

    p.setBackgroundMode((used_color == UmlTransparent) ? ::Qt::TransparentMode : ::Qt::OpaqueMode);

    QColor co = color(used_color);
    FILE * fp = svg();

    if (fp != 0)
        fputs("<g>\n", fp);

    p.setBackgroundColor(co);
    p.setFont(the_canvas()->get_font(UmlNormalBoldFont));

    if (used_color != UmlTransparent)
        p.fillRect(r, co);

    if (fp != 0)
        fprintf(fp, "\t<rect fill=\"%s\" stroke=\"black\" stroke-width=\"1\" stroke-opacity=\"1\""
                " x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" />\n",
                svg_color(used_color),
                r.x(), r.y(), r.width() - 1, r.height() - 1);

    p.drawRect(r);

    r.setTop(r.top() + (int)(2 * the_canvas()->zoom()));
    p.drawText(r, ::Qt::AlignHCenter + ::Qt::AlignTop, name);

    if (fp != 0) {
        draw_text(r, ::Qt::AlignHCenter + ::Qt::AlignTop, name,
                  p.font(), fp);
        fputs("</g>\n", fp);
    }

    p.setBackgroundColor(bckgrnd);

    if (selected())
        show_mark(p, rect());
}

UmlCode SubjectCanvas::type() const
{
    return UmlSubject;
}

void SubjectCanvas::delete_available(BooL &, BooL & out_model) const
{
    out_model |= TRUE;
}

bool SubjectCanvas::alignable() const
{
    return TRUE;
}

bool SubjectCanvas::copyable() const
{
    return selected();
}

void SubjectCanvas::open()
{
    BooL ok;
    QString s = MyInputDialog::getText(TR("Subject dialog"), TR("subject : "),
                                       fromUnicode(name), ok);

    if (ok) {
        name = toUnicode(s);
        modified();
    }
}

void SubjectCanvas::check_size()
{
    QFontMetrics fm(the_canvas()->get_font(UmlNormalBoldFont));

    min_width = fm.width((name.isEmpty()) ? QString("X") : name)
                + (int)(8 * the_canvas()->zoom());
    min_height = fm.height() * 2;

    int min = (int)(SUBJECT_CANVAS_MIN_SIZE * the_canvas()->zoom());

    if (min_width < min)
        min_width = min;

    if (min_height < min)
        min_height = min;

    // force odd width and height for line alignment
    min_width |= 1;
    min_height |= 1;

    // warning : do NOT check if ((width() < min_width) || (height() < min_height))
    // because te resize must be done to set data on scale change
    DiagramCanvas::resize((width() > min_width) ? width() : min_width,
                          (height() > min_height) ? height() : min_height);

    used_color = (itscolor == UmlDefaultColor)
                 ? the_canvas()->browser_diagram()->get_color(UmlSubject)
                 : itscolor;
}

void SubjectCanvas::change_scale()
{
    double scale = the_canvas()->zoom();

    Q3CanvasRectangle::setVisible(FALSE);
    setSize((int)(width_scale100 * scale), (int)(height_scale100 * scale));
    check_size();
    recenter();
    Q3CanvasRectangle::setVisible(TRUE);
}

void SubjectCanvas::modified()
{
    // force son reaffichage
    hide();
    check_size();
    show();
    canvas()->update();
    package_modified();
}

void SubjectCanvas::menu(const QPoint &)
{
    Q3PopupMenu m(0);

    MenuFactory::createTitle(m, TR("Subject"));
    m.insertSeparator();
    m.insertItem(TR("Upper"), 0);
    m.insertItem(TR("Lower"), 1);
    m.insertItem(TR("Go up"), 6);
    m.insertItem(TR("Go down"), 7);
    m.insertSeparator();
    m.insertItem(TR("Edit"), 2);
    m.insertSeparator();
    m.insertItem(TR("Edit drawing settings"), 3);

    if (linked()) {
        m.insertSeparator();
        m.insertItem(TR("Select linked items"), 4);
    }

    m.insertSeparator();
    m.insertItem(TR("Remove from diagram"), 5);

    int index = m.exec(QCursor::pos());

    switch (index) {
    case 0:
        upper();
        modified();	// call package_modified()
        return;

    case 1:
        lower();
        modified();	// call package_modified()
        return;

    case 6:
        z_up();
        modified();	// call package_modified()
        return;

    case 7:
        z_down();
        modified();	// call package_modified()
        return;

    case 2:
        open();
        // modified then package_modified already called
        return;

    case 3:
        edit_drawing_settings();
        return;

    case 4:
        the_canvas()->unselect_all();
        select_associated();
        return;

    case 5:
        delete_it();
        break;

    default:
        return;
    }

    package_modified();
}

void SubjectCanvas::apply_shortcut(QString s)
{
    if (s == "Upper")
        upper();
    else if (s == "Lower")
        lower();
    else if (s == "Go up")
        z_up();
    else if (s == "Go down")
        z_down();
    else if (s == "Edit drawing settings") {
        edit_drawing_settings();
        return;
    }
    else if (s == "Edit") {
        open();  // call modified then package_modified
        return;
    }
    else
        return;

    modified();
    package_modified();
}

void SubjectCanvas::edit_drawing_settings()
{
    for (;;) {
        ColorSpecVector co(1);

        co[0].set(TR("subject color"), &itscolor);

        SettingsDialog dialog(0, &co, FALSE);

        dialog.raise();

        if (dialog.exec() == QDialog::Accepted)
            modified();

        if (!dialog.redo())
            break;
    }
}

bool SubjectCanvas::has_drawing_settings() const
{
    return TRUE;
}

void SubjectCanvas::edit_drawing_settings(QList<DiagramItem *> & l)
{
    for (;;) {
        ColorSpecVector co(1);
        UmlColor itscolor;

        co[0].set(TR("subject color"), &itscolor);

        SettingsDialog dialog(0, &co, FALSE, TRUE);

        dialog.raise();

        if ((dialog.exec() == QDialog::Accepted) && !co[0].name.isEmpty()) {
            foreach (DiagramItem *item, l) {
                ((SubjectCanvas *) item)->itscolor = itscolor;
                ((SubjectCanvas *) item)->modified();	// call package_modified()
            }
        }

        if (!dialog.redo())
            break;
    }
}

void SubjectCanvas::clone_drawing_settings(const DiagramItem *src)
{
    const SubjectCanvas * x = (const SubjectCanvas *) src;
    itscolor = x->itscolor;
    modified();
}

QString SubjectCanvas::may_start(UmlCode & l) const
{
    return (l == UmlAnchor) ? QString() : TR("illegal");
}

QString SubjectCanvas::may_connect(UmlCode & l, const DiagramItem * dest) const
{
    return (l == UmlAnchor) ? dest->may_start(l) : TR("illegal");
}

aCorner SubjectCanvas::on_resize_point(const QPoint & p)
{
    return ::on_resize_point(p, rect());
}

void SubjectCanvas::resize(aCorner c, int dx, int dy, QPoint & o)
{
    DiagramCanvas::resize(c, dx, dy, o, min_width, min_height);
}

void SubjectCanvas::resize(const QSize & sz, bool w, bool h)
{
    DiagramCanvas::resize(sz, w, h, min_width, min_height);
}

void SubjectCanvas::prepare_for_move(bool on_resize)
{
    if (!on_resize) {
        DiagramCanvas::prepare_for_move(on_resize);

        QRect r = rect();
        Q3CanvasItemList l = collisions(TRUE);
        Q3CanvasItemList::ConstIterator it;
        Q3CanvasItemList::ConstIterator end = l.end();
        DiagramItem * di;

        for (it = l.begin(); it != end; ++it) {
            if ((*it)->visible() && // at least not deleted
                !(*it)->selected() &&
                ((di = QCanvasItemToDiagramItem(*it)) != 0) &&
                r.contains(di->rect(), TRUE) &&
                di->move_with(UmlSubject)) {
                the_canvas()->select(*it);
                di->prepare_for_move(FALSE);
            }
        }
    }
}

void SubjectCanvas::save(QTextStream & st, bool ref, QString &) const
{
    if (ref) {
        st << "subject_ref " << get_ident();
    }
    else {
        nl_indent(st);

        st << "subject " << get_ident() << " ";
        save_string(fromUnicode(name), st);
        nl_indent(st);

        if (itscolor != UmlDefaultColor)
            st << "  color " << stringify(itscolor);

        save_xyzwh(st, this, "  xyzwh");
    }
}

SubjectCanvas * SubjectCanvas::read(char *& st, UmlCanvas * canvas, char * k)
{
    if (!strcmp(k, "subject_ref"))
        return (SubjectCanvas *) dict_get(read_id(st), "subject", canvas);
    else if (!strcmp(k, "subject")) {
        int id = read_id(st);
        SubjectCanvas * result = new SubjectCanvas(canvas, 0, 0, id);

        result->name = toUnicode(read_string(st));

        k = read_keyword(st);
        read_color(st, "color", result->itscolor, k);

        if (strcmp(k, "xyzwh"))
            wrong_keyword(k, "xyzwh");

        read_xyzwh(st, result);
        result->check_size();
        result->width_scale100 = result->width();
        result->height_scale100 = result->height();
        result->set_center100();
        result->show();
        return result;
    }
    else
        return 0;
}


void SubjectCanvas::history_save(QBuffer & b) const
{
    DiagramCanvas::history_save(b);
    ::save(min_width, b);
    ::save(min_height, b);
    ::save(width_scale100, b);
    ::save(height_scale100, b);
    ::save(width(), b);
    ::save(height(), b);
}

void SubjectCanvas::history_load(QBuffer & b)
{
    DiagramCanvas::history_load(b);
    ::load(min_width, b);
    ::load(min_height, b);
    ::load(width_scale100, b);
    ::load(height_scale100, b);

    int w, h;

    ::load(w, b);
    ::load(h, b);
    Q3CanvasRectangle::setSize(w, h);

    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
}

void SubjectCanvas::history_hide()
{
    DiagramCanvas::setVisible(FALSE);
    disconnect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
}

// for plug outs

void SubjectCanvas::send(ToolCom * com, Q3CanvasItemList & all)
{
    QList<SubjectCanvas *> subjects;
    Q3CanvasItemList::Iterator cit;

    for (cit = all.begin(); cit != all.end(); ++cit) {
        DiagramItem * di = QCanvasItemToDiagramItem(*cit);

        if ((di != 0) &&
            (*cit)->visible() &&
            (di->type() == UmlSubject))
            subjects.append((SubjectCanvas *) di);
    }

    com->write_unsigned(subjects.count());

    foreach (SubjectCanvas *sc, subjects) {
        WrapperStr s = fromUnicode(sc->name);
        com->write_string((const char *) s);
        com->write(sc->rect());
    }
}
