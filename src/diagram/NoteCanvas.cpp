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

#include "NoteCanvas.h"
#include "MLEDialog.h"
#include "UmlGlobal.h"
#include "UmlCanvas.h"
#include "BrowserDiagram.h"
#include "Settings.h"
#include "SettingsDialog.h"
#include "myio.h"
#include "ui/menufactory.h"
#include "strutil.h"
#include "DialogUtil.h"
#include "translate.h"

NoteCanvas::NoteCanvas(UmlCanvas * canvas, int x, int y, int id)
    : DiagramCanvas(0, canvas, x, y, NOTE_CANVAS_MIN_SIZE,
                    NOTE_CANVAS_MIN_SIZE, id),
    itsfont(UmlNormalFont), itscolor(UmlDefaultColor),
    used_color(UmlDefaultColor), fg_c(UmlBlack)
{
    browser_node = canvas->browser_diagram();
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
}

NoteCanvas::~NoteCanvas()
{
}

void NoteCanvas::delete_it()
{
    disconnect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
    DiagramCanvas::delete_it();
}

void NoteCanvas::draw(QPainter & p)
{
    if (! visible()) return;

    p.setRenderHint(QPainter::Antialiasing, true);
    QRect r = rect();
    QBrush brsh = p.brush();
    QColor bckgrnd = p.backgroundColor();
    QPen fgcolor = p.pen();
    Q3PointArray a(7);

    used_color = (itscolor == UmlDefaultColor)
                 ? the_canvas()->browser_diagram()->get_color(UmlNote)
                 : itscolor;

    QColor co = color(used_color);

    const int corner_size = (int)(NOTE_MARGIN * the_canvas()->zoom());

    a.setPoint(0, r.left(), r.top());
    a.setPoint(1, r.right() - corner_size, r.top());
    a.setPoint(2, r.right() - corner_size, r.top() + corner_size);
    a.setPoint(3, r.right(), r.top() + corner_size);
    a.setPoint(4, r.right(), r.bottom());
    a.setPoint(5, r.left(), r.bottom());
    a.setPoint(6, r.left(), r.top());

    FILE * fp = svg();

    if (fp != 0)
        fputs("<g>\n", fp);

    if (used_color == UmlTransparent) {
        p.setBackgroundMode(::Qt::TransparentMode);
        p.setBackgroundColor(co);
        p.drawPolyline(a);

        if (fp != 0)
            draw_poly(fp, a, UmlTransparent);
    }
    else {
        p.setBackgroundMode(::Qt::OpaqueMode);
        p.setBrush(co);
        p.drawPolygon(a, TRUE, 0, 6);
        p.setBrush(brsh);
        p.setBackgroundColor(co);

        if (fp != 0)
            draw_poly(fp, a, used_color);
    }

    p.drawLine(r.right() - corner_size, r.top(),
               r.right(), r.top() + corner_size);

    if (fp != 0)
        fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                r.right() - corner_size, r.top(), r.right(), r.top() + corner_size);

    p.setFont(the_canvas()->get_font(itsfont));

    if (fg_c != UmlTransparent)
        p.setPen(color(fg_c));

    p.drawText(r.left() + corner_size, r.top() + corner_size,
               r.width() - 2 * corner_size, r.height() - 2 * corner_size,
               ::Qt::AlignLeft + ::Qt::AlignTop + ::Qt::TextWordWrap,
               note);

    if (fp != 0) {
        draw_text(r.left() + corner_size, r.top() + corner_size,
                  r.width() - 2 * corner_size, r.height() - 2 * corner_size,
                  ::Qt::AlignLeft + ::Qt::AlignTop + ::Qt::TextWordWrap,
                  note, p.font(), fp, fg_c);
        fputs("</g>\n", fp);
    }

    p.setFont(the_canvas()->get_font(UmlNormalFont));
    p.setBackgroundColor(bckgrnd);
    p.setPen(fgcolor);

    if (selected())
        show_mark(p, r);
}

UmlCode NoteCanvas::type() const
{
    return UmlNote;
}

void NoteCanvas::delete_available(BooL &, BooL & out_model) const
{
    out_model |= TRUE;
}

bool NoteCanvas::alignable() const
{
    return TRUE;
}

bool NoteCanvas::copyable() const
{
    return selected();
}

void NoteCanvas::open()
{
    static QSize sz;

    // warning : 'note' is already unicode
    QString s = fromUnicode(note);

    MLEDialog::get(s, QCursor::pos(), sz);
    note = toUnicode(s);

    modified();
}

void NoteCanvas::modified()
{
    used_color = (itscolor == UmlDefaultColor)
                 ? the_canvas()->browser_diagram()->get_color(UmlNote)
                 : itscolor;
    // force son reaffichage
    hide();
    show();
    canvas()->update();
    package_modified();
}

void NoteCanvas::menu(const QPoint &)
{
    Q3PopupMenu m(0);
    Q3PopupMenu fontsubm(0);

    MenuFactory::createTitle(m, TR("Note"));
    m.insertSeparator();
    m.insertItem(TR("Upper"), 0);
    m.insertItem(TR("Lower"), 1);
    m.insertItem(TR("Go up"), 7);
    m.insertItem(TR("Go down"), 8);
    m.insertSeparator();
    m.insertItem(TR("Edit"), 2);
    m.insertSeparator();
    m.insertItem(TR("Color of text"), 6);
    m.insertItem(TR("Font"), &fontsubm);
    init_font_menu(fontsubm, the_canvas(), 10);
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

    case 6:
        for (;;) {
            ColorSpecVector co(1);

            co[0].set(TR("color"), &fg_c);

            SettingsDialog dialog(0, &co, TRUE, FALSE,
                                  TR("Text color dialog"));

            dialog.raise();

            if (dialog.exec() != QDialog::Accepted)
                return;

            // force son reaffichage
            hide();
            show();
            canvas()->update();

            if (!dialog.redo())
                break;
            else
                package_modified();
        }

        break;

    default:
        if (index >= 10) {
            itsfont = (UmlFont)(index - 10);
            modified();
        }

        return;
    }

    package_modified();
}

void NoteCanvas::apply_shortcut(QString s)
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

void NoteCanvas::edit_drawing_settings()
{
    for (;;) {
        ColorSpecVector co(1);

        co[0].set(TR("note color"), &itscolor);

        SettingsDialog dialog(0, &co, FALSE);

        dialog.raise();

        if (dialog.exec() == QDialog::Accepted)
            modified();

        if (!dialog.redo())
            break;
    }
}

bool NoteCanvas::has_drawing_settings() const
{
    return TRUE;
}

void NoteCanvas::edit_drawing_settings(QList<DiagramItem *> & l)
{
    for (;;) {
        ColorSpecVector co(1);
        UmlColor itscolor;

        co[0].set(TR("note color"), &itscolor);

        SettingsDialog dialog(0, &co, FALSE, TRUE);

        dialog.raise();

        if ((dialog.exec() == QDialog::Accepted) && !co[0].name.isEmpty()) {
            foreach (DiagramItem *item, l) {
                NoteCanvas *canvas = (NoteCanvas *)item;
                canvas->itscolor = itscolor;
                canvas->modified();
            }
        }

        if (!dialog.redo())
            break;
    }
}

void NoteCanvas::clone_drawing_settings(const DiagramItem *src)
{
    const NoteCanvas * x = (const NoteCanvas *) src;
    itscolor = x->itscolor;
    modified();
}

QString NoteCanvas::may_start(UmlCode & l) const
{
    return (l == UmlAnchor) ? QString() : TR("illegal");
}

QString NoteCanvas::may_connect(UmlCode & l, const DiagramItem * dest) const
{
    return (l == UmlAnchor) ? dest->may_start(l) : TR("illegal");
}

aCorner NoteCanvas::on_resize_point(const QPoint & p)
{
    return ::on_resize_point(p, rect());
}

void NoteCanvas::resize(aCorner c, int dx, int dy, QPoint & o)
{
    int min = (int)(NOTE_CANVAS_MIN_SIZE * the_canvas()->zoom());

    DiagramCanvas::resize(c, dx, dy, o, min, min, TRUE);
}

void NoteCanvas::resize(const QSize & sz, bool w, bool h)
{
    int min = (int)(NOTE_CANVAS_MIN_SIZE * the_canvas()->zoom());

    DiagramCanvas::resize(sz, w, h, min, min, TRUE);
}

void NoteCanvas::save_internal(QTextStream & st) const
{
    QTextCodec* codec = QTextCodec::codecForLocale();
    QByteArray toWrite = codec->fromUnicode(note);
    save_string(toWrite, st);

    //save_string(temp1, st);
    nl_indent(st);

    if (itscolor != UmlDefaultColor)
        st << "  color " << stringify(itscolor);

    if (itsfont != UmlNormalFont)
        st << "  font " << stringify(itsfont);

    if (fg_c != UmlBlack)
        st << "  fg " << stringify(fg_c);

    save_xyzwh(st, this, "  xyzwh");
}

void NoteCanvas::save(QTextStream & st, bool ref, QString &) const
{
    if (ref) {
        st << "note_ref " << get_ident();
    }
    else {
        nl_indent(st);

        st << "note " << get_ident() << " ";
        save_internal(st);
    }
}

void NoteCanvas::read_internal(char *& st)
{
    const char * p = st;
    QTextCodec* codec = QTextCodec::codecForLocale();
    QTextStream stream(p);
    stream.setCodec(codec);
    QByteArray ba;
    stream   >> ba;
    QString temp = QString::fromLocal8Bit(ba);
    char* test = read_string(st);
    Q_UNUSED(test);
    note = temp;


    char * k = read_keyword(st);
    read_color(st, "color", itscolor, k);
    read_font(st, "font", itsfont, k);
    read_color(st, "fg", fg_c, k);

    if (strcmp(k, "xyzwh"))
        wrong_keyword(k, "xyzwh");

    read_xyzwh(st, this);
    width_scale100 = width();
    height_scale100 = height();
    set_center100();
    show();
}

NoteCanvas * NoteCanvas::read(char *& st, UmlCanvas * canvas, char * k)
{
    if (!strcmp(k, "note_ref"))
        return (NoteCanvas *) dict_get(read_id(st), "note", canvas);
    else if (!strcmp(k, "note")) {
        int id = read_id(st);
        NoteCanvas * result = new NoteCanvas(canvas, 0, 0, id);

        result->read_internal(st);
        return result;
    }
    else
        return 0;
}

void NoteCanvas::history_hide()
{
    Q3CanvasItem::setVisible(FALSE);
    disconnect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
}

void NoteCanvas::history_save(QBuffer & b) const
{
    DiagramCanvas::history_save(b);
    ::save(width_scale100, b);
    ::save(height_scale100, b);
    ::save(width(), b);
    ::save(height(), b);
}

void NoteCanvas::history_load(QBuffer & b)
{
    DiagramCanvas::history_load(b);
    ::load(width_scale100, b);
    ::load(height_scale100, b);

    int w, h;

    ::load(w, b);
    ::load(h, b);
    Q3CanvasRectangle::setSize(w, h);

    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
}
