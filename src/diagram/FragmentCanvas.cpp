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
#include <Q3PointArray>


#include "FragmentCanvas.h"
#include "FragmentDialog.h"
#include "FragmentSeparatorCanvas.h"
#include "UmlGlobal.h"
#include "UmlCanvas.h"
#include "BrowserDiagram.h"
#include "BasicData.h"
#include "Settings.h"
#include "SettingsDialog.h"
#include "myio.h"
#include "ui/menufactory.h"
#include "DialogUtil.h"
#include "ToolCom.h"
#include "strutil.h"
#include "translate.h"

FragmentCanvas::FragmentCanvas(UmlCanvas * canvas, int x, int y, int id)
    : DiagramCanvas(0, canvas, x, y, FRAGMENT_CANVAS_MIN_SIZE,
                    FRAGMENT_CANVAS_MIN_SIZE, id)
{
    browser_node = canvas->browser_diagram();
    itscolor = UmlDefaultColor;
    refer = 0;
    check_size();
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
}

FragmentCanvas::~FragmentCanvas()
{
}

void FragmentCanvas::delete_it()
{
    while (! separators.isEmpty())
        separators.first()->delete_it();	// call remove_it

    disconnect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
    DiagramCanvas::delete_it();
}

void FragmentCanvas::remove_it(FragmentSeparatorCanvas * sp)
{
    separators.remove(sp);
}

void FragmentCanvas::draw(QPainter & p)
{
    if (! visible()) return;

    p.setRenderHint(QPainter::Antialiasing, true);
    QFontMetrics fm(the_canvas()->get_font(UmlNormalFont));
    int w = fm.width((name.isEmpty()) ? QString("X") : name);
    int h = fm.height() / 2;
    QRect r = rect();
    QRect rname = r;

    rname.setWidth(w + h);
    rname.setHeight(fm.height() + h);

    int h1 = (fm.height() + h) / 2;
    int x1 = rname.right() + h1;
    int y1 = rname.bottom() - h1;

    QColor bckgrnd = p.backgroundColor();

    p.setBackgroundMode((used_color == UmlTransparent) ? ::Qt::TransparentMode : ::Qt::OpaqueMode);

    QColor co = color(used_color);
    FILE * fp = svg();

    if (fp != 0)
        fputs("<g>\n", fp);

    p.setBackgroundColor(co);
    p.setFont(the_canvas()->get_font(UmlNormalFont));

    if (used_color != UmlTransparent)
        p.fillRect(r, co);
    else if (!name.isEmpty()) {
        Q3PointArray a(6);
        QBrush brsh = p.brush();

        a.setPoint(0, rname.left(), rname.top());
        a.setPoint(1, x1, rname.top());
        a.setPoint(2, x1, y1);
        a.setPoint(3, rname.right(), rname.bottom());
        a.setPoint(4, rname.left(), rname.bottom());
        a.setPoint(5, rname.left(), rname.top());

        p.setBrush(UmlWhiteColor);
        p.drawPolygon(a, TRUE, 0, 5);
        p.setBrush(brsh);

        if (fp != 0)
            draw_poly(fp, a, UmlWhite);
    }

    if (fp != 0)
        fprintf(fp, "\t<rect fill=\"%s\" stroke=\"black\" stroke-width=\"1\" stroke-opacity=\"1\""
                " x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" />\n",
                svg_color(used_color),
                r.x(), r.y(), r.width() - 1, r.height() - 1);

    p.drawRect(r);

    if (refer != 0) {
        QString s = refer->get_name() + form;
        QRect r2(r.left(), r.top() + 2 * fm.height(),
                 r.width(), fm.height());

        p.drawText(r2, ::Qt::AlignCenter, s);

        if (fp != 0)
            draw_text(r2, ::Qt::AlignCenter, s,
                      p.font(), fp);
    }

    if (!name.isEmpty())
        p.drawText(rname, ::Qt::AlignCenter, name);

    if (fp != 0)
        draw_text(rname, ::Qt::AlignCenter, name,
                  p.font(), fp);

    p.drawLine(rname.left(), rname.bottom(), rname.right(), rname.bottom());
    p.drawLine(rname.right(), rname.bottom(), x1, y1);
    p.drawLine(x1, y1, x1, rname.top());

    if (fp != 0) {
        fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                rname.left(), rname.bottom(), rname.right(), rname.bottom());
        fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                rname.right(), rname.bottom(), x1, y1);
        fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                x1, y1, x1, rname.top());
        fputs("</g>\n", fp);
    }

    p.setBackgroundColor(bckgrnd);

    if (selected())
        show_mark(p, r);
}

UmlCode FragmentCanvas::type() const
{
    return UmlFragment;
}

void FragmentCanvas::delete_available(BooL &, BooL & out_model) const
{
    out_model |= TRUE;
}

bool FragmentCanvas::alignable() const
{
    return TRUE;
}

bool FragmentCanvas::copyable() const
{
    return selected();
}

void FragmentCanvas::moveBy(double dx, double dy)
{
    DiagramCanvas::moveBy(dx, dy);

    foreach (FragmentSeparatorCanvas *canvas, separators)
        canvas->update();
}

void FragmentCanvas::set_z(double z)
{
    setZ(z);

    foreach (FragmentSeparatorCanvas *canvas, separators)
        canvas->update();
}

void FragmentCanvas::open()
{
    static QStringList dflt;

    if (dflt.isEmpty()) {
        dflt.append("alt");
        dflt.append("assert");
        dflt.append("break");
        dflt.append("consider");
        dflt.append("critical");
        dflt.append("ignore");
        dflt.append("loop");
        dflt.append("neg");
        dflt.append("opt");
        dflt.append("par");
        dflt.append("ref");
        dflt.append("seq");
        dflt.append("strict");
    }

    BrowserNode * refered = refer;
    FragmentDialog d(dflt, name, form, refer);

    d.raise();

    if (d.exec() == QDialog::Accepted) {
        if (refer != refered) {
            if (refered != 0) {
                disconnect(refered->get_data(), SIGNAL(changed()), this, SLOT(modified()));
                disconnect(refered->get_data(), SIGNAL(deleted()), this, SLOT(modified()));
            }

            if (refer != 0) {
                connect(refer->get_data(), SIGNAL(changed()), this, SLOT(modified()));
                connect(refer->get_data(), SIGNAL(deleted()), this, SLOT(modified()));
            }
        }

        modified();
    }
}

void FragmentCanvas::check_size()
{
    QFontMetrics fm(the_canvas()->get_font(UmlNormalFont));

    min_width = fm.width((name.isEmpty()) ? QString("X") : name);
    min_height = fm.height() * 2;

    min_width += min_height;
    min_height += fm.height();

    int min = (int)(FRAGMENT_CANVAS_MIN_SIZE * the_canvas()->zoom());

    if (min_width < min)
        min_width = min;

    if (min_height < min)
        min_height = min;


    if (refer != 0) {
        QString s = refer->get_name() + form + "__";
        int w = fm.width(s);

        if (min_width < w)
            min_width = w;

        min_height += fm.height() / 2;
    }

    // force odd width and height for line alignment
    min_width |= 1;
    min_height |= 1;

    // warning : do NOT check if ((width() < min_width) || (height() < min_height))
    // because te resize must be done to set data on scale change
    DiagramCanvas::resize((width() > min_width) ? width() : min_width,
                          (height() > min_height) ? height() : min_height);

    used_color = (itscolor == UmlDefaultColor)
                 ? the_canvas()->browser_diagram()->get_color(UmlFragment)
                 : itscolor;
}

void FragmentCanvas::change_scale()
{
    double scale = the_canvas()->zoom();

    Q3CanvasRectangle::setVisible(FALSE);
    setSize((int)(width_scale100 * scale), (int)(height_scale100 * scale));
    check_size();
    recenter();
    Q3CanvasRectangle::setVisible(TRUE);

    foreach (FragmentSeparatorCanvas *canvas, separators)
        canvas->update();
}

void FragmentCanvas::modified()
{
    // force son reaffichage
    if ((refer != 0) && refer->deletedp()) {
        disconnect(refer->get_data(), SIGNAL(changed()), this, SLOT(modified()));
        disconnect(refer->get_data(), SIGNAL(deleted()), this, SLOT(modified()));
        refer = 0;
    }

    hide();
    check_size();
    show();
    canvas()->update();
    package_modified();
}

void FragmentCanvas::menu(const QPoint &)
{
    Q3PopupMenu m(0);

    MenuFactory::createTitle(m, TR("Fragment"));
    m.insertSeparator();
    m.insertItem(TR("Upper"), 0);
    m.insertItem(TR("Lower"), 1);
    m.insertItem(TR("Go up"), 7);
    m.insertItem(TR("Go down"), 8);
    m.insertSeparator();
    m.insertItem(TR("Edit"), 2);
    m.insertItem(TR("Add separator"), 6);
    m.insertSeparator();
    m.insertItem(TR("Edit drawing settings"), 3);

    if (linked()) {
        m.insertSeparator();
        m.insertItem(TR("Select linked items"), 4);
    }

    m.insertSeparator();
    m.insertItem(TR("Remove from diagram"), 5);

    if ((refer != 0) && !refer->deletedp()) {
        m.insertSeparator();
        m.insertItem(TR("Show referenced diagram"), 9);
    }

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

    case 7:
        z_up();
        modified();	// call package_modified()
        return;

    case 8:
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

    case 6: {
        FragmentSeparatorCanvas * sp =
            new FragmentSeparatorCanvas(the_canvas(), this);

        separators.append(sp);
        the_canvas()->unselect_all();
        the_canvas()->select(sp);
    }
    break;

    case 9:
        refer->open(FALSE);
        return;

    default:
        return;
    }

    package_modified();
}

void FragmentCanvas::apply_shortcut(QString s)
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

void FragmentCanvas::edit_drawing_settings()
{
    for (;;) {
        ColorSpecVector co(1);

        co[0].set(TR("fragment color"), &itscolor);

        SettingsDialog dialog(0, &co, FALSE);

        dialog.raise();

        if (dialog.exec() == QDialog::Accepted)
            modified();

        if (!dialog.redo())
            break;
    }
}

bool FragmentCanvas::has_drawing_settings() const
{
    return TRUE;
}

void FragmentCanvas::edit_drawing_settings(QList<DiagramItem *> & l)
{
    for (;;) {
        ColorSpecVector co(1);
        UmlColor itscolor;

        co[0].set(TR("fragment color"), &itscolor);

        SettingsDialog dialog(0, &co, FALSE, TRUE);

        dialog.raise();

        if ((dialog.exec() == QDialog::Accepted) && !co[0].name.isEmpty()) {
            foreach (DiagramItem *item, l) {
                ((FragmentCanvas *) item)->itscolor = itscolor;
                ((FragmentCanvas *) item)->modified(); // call package_modified()
            }
        }

        if (!dialog.redo())
            break;
    }
}

void FragmentCanvas::clone_drawing_settings(const DiagramItem *src)
{
    const FragmentCanvas * x = (const FragmentCanvas *) src;
    itscolor = x->itscolor;
    modified();
}

QString FragmentCanvas::may_start(UmlCode & l) const
{
    return (l == UmlAnchor) ? QString() : TR("illegal");
}

QString FragmentCanvas::may_connect(UmlCode & l, const DiagramItem * dest) const
{
    return (l == UmlAnchor) ? dest->may_start(l) : TR("illegal");
}

aCorner FragmentCanvas::on_resize_point(const QPoint & p)
{
    return ::on_resize_point(p, rect());
}

void FragmentCanvas::resize(aCorner c, int dx, int dy, QPoint & o)
{
    DiagramCanvas::resize(c, dx, dy, o, min_width, min_height);

    foreach (FragmentSeparatorCanvas *canvas, separators)
        canvas->update();
}

void FragmentCanvas::resize(const QSize & sz, bool w, bool h)
{
    if (DiagramCanvas::resize(sz, w, h, min_width, min_height)) {
        foreach (FragmentSeparatorCanvas *canvas, separators)
            canvas->update();
    }
}

void FragmentCanvas::prepare_for_move(bool on_resize)
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
                di->move_with(UmlFragment)) {
                the_canvas()->select(*it);
                di->prepare_for_move(FALSE);
            }
        }
    }
}

void FragmentCanvas::save(QTextStream & st, bool ref, QString & warning) const
{
    if (ref) {
        st << "fragment_ref " << get_ident();
    }
    else {
        nl_indent(st);

        st << "fragment " << get_ident() << " ";
        save_string(name, st);
        indent(+1);
        nl_indent(st);

        if (itscolor != UmlDefaultColor)
            st << "color " << stringify(itscolor) << " ";

        if (refer != 0) {
            st << "refer ";
            refer->save(st, TRUE, warning);
            nl_indent(st);
        }

        if (! form.isEmpty()) {
            st << "form ";
            save_string(form, st);
            st << " ";
        }

        save_xyzwh(st, this, "xyzwh");

        foreach (FragmentSeparatorCanvas *canvas, separators)
            canvas->save(st, false, warning);

        indent(-1);
        nl_indent(st);
        st << "end";
    }
}

FragmentCanvas * FragmentCanvas::read(char *& st, UmlCanvas * canvas, char * k)
{
    if (!strcmp(k, "fragment_ref"))
        return (FragmentCanvas *) dict_get(read_id(st), "fragment", canvas);
    else if (!strcmp(k, "fragment")) {
        int id = read_id(st);
        FragmentCanvas * result = new FragmentCanvas(canvas, 0, 0, id);

        result->name = read_string(st);

        k = read_keyword(st);
        read_color(st, "color", result->itscolor, k);

        if (!strcmp(k, "refer")) {
            result->refer = BrowserDiagram::read_diagram_ref(st);
            connect(result->refer->get_data(), SIGNAL(changed()), result, SLOT(modified()));
            connect(result->refer->get_data(), SIGNAL(deleted()), result, SLOT(modified()));
            k = read_keyword(st);
        }

        if (! strcmp(k, "form")) {
            result->form = read_string(st);
            k = read_keyword(st);
        }

        if (strcmp(k, "xyzwh"))
            wrong_keyword(k, "xyzwh");

        read_xyzwh(st, result);
        result->check_size();
        result->width_scale100 = result->width();
        result->height_scale100 = result->height();
        result->set_center100();
        result->show();

        if (read_file_format() >= 28) {
            while (strcmp(k = read_keyword(st), "end"))
                result->separators.append(FragmentSeparatorCanvas::read(st, canvas, k, result));
        }

        return result;
    }
    else
        return 0;
}


void FragmentCanvas::history_save(QBuffer & b) const
{
    DiagramCanvas::history_save(b);
    ::save(min_width, b);
    ::save(min_height, b);
    ::save(width_scale100, b);
    ::save(height_scale100, b);
    ::save(width(), b);
    ::save(height(), b);

    ::save((int) separators.count(), b);


    foreach (FragmentSeparatorCanvas *canvas, separators)
        ::save(canvas, b);
}

void FragmentCanvas::history_load(QBuffer & b)
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

    int l;

    ::load(l, b);

    separators.clear();

    while (l--)
        separators.append((FragmentSeparatorCanvas *) ::load_item(b));

    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));

    if (refer != 0) {
        connect(refer->get_data(), SIGNAL(changed()), this, SLOT(modified()));
        connect(refer->get_data(), SIGNAL(deleted()), this, SLOT(modified()));
    }
}

void FragmentCanvas::history_hide()
{
    DiagramCanvas::setVisible(FALSE);
    disconnect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));

    if (refer != 0) {
        disconnect(refer->get_data(), SIGNAL(changed()), this, SLOT(modified()));
        disconnect(refer->get_data(), SIGNAL(deleted()), this, SLOT(modified()));
    }
}

// for plug outs

void FragmentCanvas::send(ToolCom * com, Q3CanvasItemList & all,
                          QList<FragmentCanvas *> & fragments,
                          QList<FragmentCanvas *> & refs)
{
    Q3CanvasItemList::Iterator cit;

    for (cit = all.begin(); cit != all.end(); ++cit) {
        DiagramItem * di = QCanvasItemToDiagramItem(*cit);

        if ((di != 0) && (*cit)->visible() && (di->type() == UmlFragment))
            fragments.append((FragmentCanvas *) di);
    }

    com->write_unsigned(fragments.count());

    foreach (FragmentCanvas * f, fragments) {
        WrapperStr s = fromUnicode(f->name);

        com->write_string((const char *) s);
        com->write(f->rect());

        // few separators, use bubble sort
        unsigned sz = f->separators.count();

        if (sz == 0)
            com->write_unsigned(1);
        else {
            FragmentSeparatorCanvas ** v = new FragmentSeparatorCanvas *[sz];
            unsigned index = 0;
            foreach (FragmentSeparatorCanvas *canvas, f->separators) {
                v[index] = canvas;
                ++index;
            }

            bool modified;

            do {
                modified = FALSE;

                for (index = 1; index < sz; index += 1) {
                    if (v[index - 1]->y() > v[index]->y()) {
                        modified = TRUE;

                        FragmentSeparatorCanvas * fs = v[index - 1];

                        v[index - 1] = v[index];
                        v[index] = fs;
                    }
                }
            }
            while (modified);

            com->write_unsigned(sz + 1);

            for (index = 0; index != sz; index += 1)
                com->write_unsigned((unsigned) v[index]->y());

            delete [] v;
        }

        com->write_unsigned((unsigned)(f->y() + f->height() - 1));

        if (com->api_format() >= 41) {
            if (f->refer != 0)
                f->refer->write_id(com);
            else
                com->write_id(0);

            s = fromUnicode(f->form);
            com->write_string((const char *) s);

            if (f->name == "ref")
                refs.append(f);
        }
    }
}
