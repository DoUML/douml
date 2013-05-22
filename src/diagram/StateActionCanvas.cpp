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





#include <q3popupmenu.h>
#include <qcursor.h>
#include <qpainter.h>
//Added by qt3to4:
#include <QTextStream>
#include <Q3PointArray>
#include <QPixmap>

#include "StateActionCanvas.h"
#include "TransitionCanvas.h"
#include "SimpleRelationCanvas.h"
#include "StateActionData.h"
#include "BrowserStateAction.h"
#include "BrowserStateDiagram.h"
#include "UmlPixmap.h"
#include "UmlGlobal.h"
#include "SettingsDialog.h"
#include "myio.h"
#include "ToolCom.h"
#include "Tool.h"
#include "ui/menufactory.h"
#include "Settings.h"
#include "strutil.h"
#include "ProfiledStereotypes.h"
#include "translate.h"

StateActionCanvas::StateActionCanvas(BrowserNode * bn, UmlCanvas * canvas,
                                     int x, int y)
    : DiagramCanvas(0, canvas, x, y, STATE_ACTION_CANVAS_MIN_SIZE, 1, 0)
{
    browser_node = bn;
    itscolor = UmlDefaultColor;
    language = DefaultDrawingLanguage;
    show_stereotype_properties = UmlDefaultState;

    compute_size();
    check_stereotypeproperties();

    connect(bn->get_data(), SIGNAL(changed()), this, SLOT(modified()));
    connect(bn->get_data(), SIGNAL(deleted()), this, SLOT(deleted()));
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));

    if (canvas->must_draw_all_relations()) {
        //draw_all_simple_relations();
        draw_all_transitions();
    }
}

StateActionCanvas::StateActionCanvas(UmlCanvas * canvas, int id)
    : DiagramCanvas(canvas, id)
{
    // for read operation
    browser_node = 0;
    itscolor = UmlDefaultColor;
    language = DefaultDrawingLanguage;
    show_stereotype_properties = UmlDefaultState;
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
}

StateActionCanvas::~StateActionCanvas()
{
}

void StateActionCanvas::delete_it()
{
    disconnect(DrawingSettings::instance(), SIGNAL(changed()),
               this, SLOT(modified()));
    disconnect(browser_node->get_data(), 0, this, 0);

    DiagramCanvas::delete_it();
}

void StateActionCanvas::deleted()
{
    delete_it();
    canvas()->update();
}

void StateActionCanvas::remove(bool from_model)
{
    if (! from_model)
        delete_it();
    else
        browser_node->delete_it();	// will remove canvas
}

void StateActionCanvas::compute_size()
{
    double zoom = the_canvas()->zoom();
    const StateActionData * data =
        (const StateActionData *) browser_node->get_data();
    const QPixmap * px =
        ProfiledStereotypes::diagramPixmap(data->get_stereotype(), zoom);

    if (px != 0) {
        // force odd width and height for line alignment
        min_width = px->width() | 1;
        min_height = px->height() | 1;

        DiagramCanvas::resize(min_width, min_height);
    }
    else {
        QFontMetrics fm(the_canvas()->get_font(UmlNormalFont));

        s = data->get_action((language == DefaultDrawingLanguage)
                             ? ((BrowserStateDiagram *) the_canvas()->browser_diagram())->get_language()
                             : language);
        s = toUnicode(s);

        QSize sz = fm.size(0, s);

        min_height = fm.height() + sz.height();
        min_width = sz.width();

        const char * st = data->get_short_stereotype();
        int mn = (int)(STATE_ACTION_CANVAS_MIN_SIZE * zoom);

        if (!strcmp(st, "receive-signal") ||
            !strcmp(st, "send-signal")) {
            if (min_height < mn)
                min_height = mn;

            min_width += ((height() > min_height) ? height() : min_height) / 2;
        }
        else if (st[0] != 0) {
            int w = fm.width(QString("<<") + toUnicode(st) + ">>");

            if (min_width < w)
                min_width = w;

            min_height += 2 * fm.height();

            if (min_height < mn)
                min_height = mn;
        }

        min_width += (int)(8 * zoom);

        if (min_width < mn)
            min_width = mn;

        min_width += 2;

        used_color = (itscolor == UmlDefaultColor)
                     ? the_canvas()->browser_diagram()->get_color(UmlStateAction)
                     : itscolor;

        if (used_color != UmlTransparent) {
            const char * st = data->get_short_stereotype();

            if (strcmp(st, "send-signal") && strcmp(st, "receive-signal")) {
                const int shadow = the_canvas()->shadow();

                min_width += shadow;
                min_height += shadow;
            }
        }

        DiagramCanvas::resize((width() > min_width) ? width() : min_width,
                              (height() > min_height) ? height() : min_height);
    }
}

void StateActionCanvas::change_scale()
{
    Q3CanvasRectangle::setVisible(FALSE);
    double scale = the_canvas()->zoom();

    setSize((int)(width_scale100 * scale), (int)(height_scale100 * scale));
    compute_size();
    recenter();
    Q3CanvasRectangle::setVisible(TRUE);
}

void StateActionCanvas::modified()
{
    hide();
    hide_lines();
    compute_size();
    show();
    update_show_lines();
    force_self_rel_visible();

    if (the_canvas()->must_draw_all_relations()) {
        //draw_all_simple_relations();
        draw_all_transitions();
    }

    check_stereotypeproperties();
    canvas()->update();
    package_modified();
}

void StateActionCanvas::post_loaded()
{
    force_self_rel_visible();

    if (the_canvas()->must_draw_all_relations()) {
        //draw_all_simple_relations();
        draw_all_transitions();
    }
}

void StateActionCanvas::connexion(UmlCode action, DiagramItem * dest,
                                  const QPoint &, const QPoint &)
{
    ArrowCanvas * a;

    if (action == UmlTransition)
        a = new TransitionCanvas(the_canvas(), this, dest, 0, 0, -1.0, -1.0);
    else if (IsaSimpleRelation(action))
        a = new SimpleRelationCanvas(the_canvas(), this, dest, 0, action, 0, -1.0, -1.0);
    else
        a = new ArrowCanvas(the_canvas(), this, dest, action, 0, FALSE, -1.0, -1.0);

    a->show();
    the_canvas()->select(a);
}

aCorner StateActionCanvas::on_resize_point(const QPoint & p)
{
    return (ProfiledStereotypes::diagramPixmap(browser_node->get_data()->get_stereotype(),
            the_canvas()->zoom())
            != 0)
           ? NoCorner
           : ::on_resize_point(p, rect());
}

void StateActionCanvas::resize(aCorner c, int dx, int dy, QPoint & o)
{
    DiagramCanvas::resize(c, dx, dy, o, min_width, min_height, TRUE);
}

void StateActionCanvas::resize(const QSize & sz, bool w, bool h)
{
    DiagramCanvas::resize(sz, w, h, min_width, min_height, TRUE);
}

void StateActionCanvas::draw(QPainter & p)
{
    if (! visible()) return;

    p.setRenderHint(QPainter::Antialiasing, true);
    QRect r = rect();
    const BasicData * data = browser_node->get_data();
    double zoom = the_canvas()->zoom();
    const QPixmap * px =
        ProfiledStereotypes::diagramPixmap(data->get_stereotype(),
                                           the_canvas()->zoom());
    FILE * fp = svg();

    if (fp != 0)
        fputs("<g>\n", fp);

    if (px != 0) {
        p.setBackgroundMode(::Qt::TransparentMode);

        int lft = (px->width() < width()) ? r.x() + (width() - px->width()) / 2 : r.x();

        p.drawPixmap(lft, r.y(), *px);

        if (fp != 0)
            // pixmap not really exported in SVG
            fprintf(fp, "\t<rect fill=\"%s\" stroke=\"black\" stroke-width=\"1\" stroke-opacity=\"1\""
                    " x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" />\n",
                    svg_color(UmlBlack), lft, r.y(), px->width() - 1, px->height() - 1);
    }
    else {
        QColor bckgrnd = p.backgroundColor();
        int mw = min_width;
        QBrush brsh = p.brush();
        QFontMetrics fm(the_canvas()->get_font(UmlNormalFont));
        const char * st = data->get_short_stereotype();
        const int shadow = the_canvas()->shadow();

        QColor co = color(used_color);

        p.setBackgroundMode((used_color == UmlTransparent)
                            ? ::Qt::TransparentMode
                            : ::Qt::OpaqueMode);

        p.setBackgroundColor(co);

        p.setFont(the_canvas()->get_font(UmlNormalFont));

        if (!strcmp(st, "send-signal")) {
            Q3PointArray a(6);

            if ((used_color != UmlTransparent) && (shadow != 0)) {
                r.setRight(r.right() - shadow);
                r.setBottom(r.bottom() - shadow);
            }

            const int hh = r.height() / 2;

            a.setPoint(0, r.left(), r.top());
            a.setPoint(1, r.right() - hh, r.top());
            a.setPoint(2, r.right(), r.top() + hh);
            a.setPoint(3, r.right() - hh, r.bottom());
            a.setPoint(4, r.left(), r.bottom());
            a.setPoint(5, r.left(), r.top());

            if (used_color == UmlTransparent) {
                p.drawPolyline(a);

                if (fp != 0)
                    draw_poly(fp, a, UmlTransparent);
            }
            else {
                if (shadow != 0) {
                    Q3PointArray b(6);

                    b.setPoint(0, r.left() + shadow, r.top() + shadow);
                    b.setPoint(1, r.right() - hh + shadow, r.top() + shadow);
                    b.setPoint(2, r.right() + shadow, r.top() + hh + shadow);
                    b.setPoint(3, r.right() - hh + shadow, r.bottom() + shadow);
                    b.setPoint(4, r.left() + shadow, r.bottom() + shadow);
                    b.setPoint(5, r.left() + shadow, r.top() + shadow);
                    p.setBrush(::Qt::darkGray);
                    p.setPen(::Qt::NoPen);
                    p.drawPolygon(b, TRUE, 0, 5);
                    p.setPen(::Qt::SolidLine);

                    if (fp != 0)
                        draw_shadow(fp, b);
                }

                p.setBrush(co);
                p.drawPolygon(a, TRUE, 0, 5);

                if (fp != 0)
                    draw_poly(fp, a, used_color);
            }

            r.setRight(r.right() - hh);
            mw -= hh;
        }
        else if (!strcmp(st, "receive-signal")) {
            Q3PointArray a(6);

            if ((used_color != UmlTransparent) && (shadow != 0)) {
                r.setRight(r.right() - shadow);
                r.setBottom(r.bottom() - shadow);
            }

            const int hh = r.height() / 2;

            a.setPoint(0, r.left(), r.top());
            a.setPoint(1, r.right(), r.top());
            a.setPoint(2, r.right() - hh, r.top() + hh);
            a.setPoint(3, r.right(), r.bottom());
            a.setPoint(4, r.left(), r.bottom());
            a.setPoint(5, r.left(), r.top());

            if (used_color == UmlTransparent) {
                p.drawPolyline(a);

                if (fp != 0)
                    draw_poly(fp, a, UmlTransparent);
            }
            else {
                if (shadow != 0) {
                    Q3PointArray b(6);

                    b.setPoint(0, r.left() + shadow, r.top() + shadow);
                    b.setPoint(1, r.right() + shadow, r.top() + shadow);
                    b.setPoint(2, r.right() - hh + shadow, r.top() + hh + shadow);
                    b.setPoint(3, r.right() + shadow, r.bottom() + shadow);
                    b.setPoint(4, r.left() + shadow, r.bottom() + shadow);
                    b.setPoint(5, r.left() + shadow, r.top() + shadow);
                    p.setBrush(::Qt::darkGray);
                    p.setPen(::Qt::NoPen);
                    p.drawPolygon(b, TRUE, 0, 5);
                    p.setPen(::Qt::SolidLine);

                    if (fp != 0)
                        draw_shadow(fp, b);
                }

                p.setBrush(co);
                p.drawPolygon(a, TRUE, 0, 5);

                if (fp != 0)
                    draw_poly(fp, a, used_color);
            }

            r.setRight(r.right() - hh);
            mw -= hh;
        }
        else {
            if (used_color != UmlTransparent) {
                if (shadow != 0) {
                    r.setRight(r.right() - shadow);
                    r.setBottom(r.bottom() - shadow);

                    p.fillRect(r.right(), r.top() + shadow,
                               shadow, r.height() - 1,
                               ::Qt::darkGray);
                    p.fillRect(r.left() + shadow, r.bottom(),
                               r.width() - 1, shadow,
                               ::Qt::darkGray);

                    if (fp != 0) {
                        fprintf(fp, "\t<rect fill=\"#%06x\" stroke=\"none\" stroke-opacity=\"1\""
                                " x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" />\n",
                                QColor(::Qt::darkGray).rgb() & 0xffffff,
                                r.right(), r.top() + shadow, shadow - 1, r.height() - 1 - 1);

                        fprintf(fp, "\t<rect fill=\"#%06x\" stroke=\"none\" stroke-opacity=\"1\""
                                " x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" />\n",
                                QColor(::Qt::darkGray).rgb() & 0xffffff,
                                r.left() + shadow, r.bottom(), r.width() - 1 - 1, shadow - 1);
                    }
                }
            }

            p.setBrush(co);
            p.drawRect(r);

            if (fp != 0)
                fprintf(fp, "\t<rect fill=\"%s\" stroke=\"black\" stroke-width=\"1\" stroke-opacity=\"1\""
                        " x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" />\n",
                        svg_color(used_color),
                        r.x(), r.y(), r.width() - 1, r.height() - 1);

            if (st[0] != 0) {
                r.setTop(r.top() + fm.height() / 2);
                p.drawText(r, ::Qt::AlignHCenter, QString("<<") + toUnicode(st) + ">>");

                if (fp != 0)
                    draw_text(r, ::Qt::AlignHCenter, QString("<<") + toUnicode(st) + ">>",
                              p.font(), fp);

                r.setTop(r.top() + 3 * fm.height() / 2);
            }
        }

        r.setLeft(r.left() + (r.width() + (int)(8 * zoom) - mw) / 2 + 1);
        p.drawText(r, ::Qt::AlignVCenter, s);

        if (fp != 0) {
            draw_text(r, ::Qt::AlignVCenter, s,
                      p.font(), fp);
            fputs("</g>\n", fp);
        }

        p.setBrush(brsh);
        p.setBackgroundColor(bckgrnd);
    }

    if (selected())
        show_mark(p, rect());
}

UmlCode StateActionCanvas::type() const
{
    return UmlStateAction;
}

void StateActionCanvas::delete_available(BooL & in_model, BooL & out_model) const
{
    out_model |= TRUE;
    in_model |= browser_node->is_writable();
}

bool StateActionCanvas::alignable() const
{
    return TRUE;
}

bool StateActionCanvas::copyable() const
{
    return selected();
}

void StateActionCanvas::open()
{
    browser_node->open(FALSE);
}

void StateActionCanvas::menu(const QPoint &)
{
    Q3PopupMenu m(0);
    Q3PopupMenu toolm(0);
    int index;

    MenuFactory::createTitle(m, browser_node->get_data()->definition(FALSE, TRUE));
    m.insertSeparator();
    m.insertItem(TR("Upper"), 0);
    m.insertItem(TR("Lower"), 1);
    m.insertItem(TR("Go up"), 13);
    m.insertItem(TR("Go down"), 14);
    m.insertSeparator();
    m.insertItem(TR("Edit drawing settings"), 2);
    m.insertSeparator();
    m.insertItem(TR("Edit state action"), 3);
    m.insertSeparator();
    m.insertItem(TR("Select in browser"), 4);

    if (linked())
        m.insertItem(TR("Select linked items"), 5);

    m.insertSeparator();
    m.insertItem(TR("Remove from diagram"), 7);

    if (browser_node->is_writable())
        m.insertItem(TR("Delete from model"), 8);

    m.insertSeparator();

    if (Tool::menu_insert(&toolm, UmlStateAction, 20))
        m.insertItem(TR("Tool"), &toolm);

    switch (index = m.exec(QCursor::pos())) {
    case 0:
        upper();
        modified();	// call package_modified()
        return;

    case 1:
        lower();
        modified();	// call package_modified()
        return;

    case 13:
        z_up();
        modified();	// call package_modified()
        return;

    case 14:
        z_down();
        modified();	// call package_modified()
        return;

    case 2:
        edit_drawing_settings();
        return;

    case 3:
        browser_node->open(TRUE);
        return;

    case 4:
        browser_node->select_in_browser();
        return;

    case 5:
        the_canvas()->unselect_all();
        select_associated();
        return;

    case 7:
        //remove from diagram
        delete_it();
        break;

    case 8:
        //delete from model
        browser_node->delete_it();	// will delete the canvas
        break;

    default:
        if (index >= 20)
            ToolCom::run(Tool::command(index - 20), browser_node);

        return;
    }

    package_modified();
}

void StateActionCanvas::apply_shortcut(QString s)
{
    if (s == "Select in browser") {
        browser_node->select_in_browser();
        return;
    }
    else if (s == "Upper")
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
    else {
        browser_node->apply_shortcut(s);
        return;
    }

    modified();
    package_modified();
}

void StateActionCanvas::edit_drawing_settings()
{
    for (;;) {
        StateSpecVector st(2);
        ColorSpecVector co(1);

        st[0].set(TR("drawing language"), &language);
        st[1].set(TR("show stereotype \nproperties"), &show_stereotype_properties);

        co[0].set(TR("state action color"), &itscolor);

        SettingsDialog dialog(&st, &co, FALSE);

        dialog.raise();

        if (dialog.exec() == QDialog::Accepted)
            modified();	// call package_modified()

        if (!dialog.redo())
            break;
    }
}

bool StateActionCanvas::has_drawing_settings() const
{
    return TRUE;
}

void StateActionCanvas::edit_drawing_settings(QList<DiagramItem *> & l)
{
    for (;;) {
        StateSpecVector st(2);
        ColorSpecVector co(1);
        DrawingLanguage language;
        UmlColor itscolor;

        st[0].set(TR("drawing language"), &language);
        st[1].set(TR("show stereotype \nproperties"), &show_stereotype_properties);

        co[0].set(TR("state action color"), &itscolor);

        SettingsDialog dialog(&st, &co, FALSE, TRUE);

        dialog.raise();

        if (dialog.exec() == QDialog::Accepted) {
            foreach (DiagramItem *item, l)  {
                StateActionCanvas *canvas = (StateActionCanvas *)item;
                if (!st[0].name.isEmpty())
                    canvas->language = language;

                if (!st[1].name.isEmpty())
                    canvas->show_stereotype_properties = show_stereotype_properties;

                if (!co[0].name.isEmpty())
                    canvas->itscolor = itscolor;

                canvas->modified();	// call package_modified()
            }
        }

        if (!dialog.redo())
            break;
    }
}

void StateActionCanvas::clone_drawing_settings(const DiagramItem *src)
{
    const StateActionCanvas * x = (const StateActionCanvas *) src;
    language = x->language;
    show_stereotype_properties = x->show_stereotype_properties;
    itscolor = x->itscolor;
    modified();
}

bool StateActionCanvas::get_show_stereotype_properties() const
{
    switch (show_stereotype_properties) {
    case UmlYes:
        return TRUE;

    case UmlNo:
        return FALSE;

    default:
        return the_canvas()->browser_diagram()->get_show_stereotype_properties();
    }
}

QString StateActionCanvas::may_start(UmlCode & l) const
{
    switch (l) {
    case UmlAnchor:
    case UmlTransition:
        return 0;

    default:
        return TR("illegal");
    }
}

QString StateActionCanvas::may_connect(UmlCode & l, const DiagramItem * dest) const
{
    if (l == UmlAnchor)
        return dest->may_start(l);

    switch (dest->type()) {
    case UmlState:
    case UmlStateAction:
    case EntryPointPS:
    case FinalPS:
    case TerminatePS:
    case ExitPointPS:
    case DeepHistoryPS:
    case ShallowHistoryPS:
    case JunctionPS:
    case ChoicePS:
    case ForkPS:
    case JoinPS:
        return (l == UmlTransition) ? QString() : TR("illegal");

    default:
        return TR("illegal");
    }
}

void StateActionCanvas::save(QTextStream & st, bool ref, QString & warning) const
{
    if (ref) {
        st << "stateactioncanvas_ref " << get_ident();
    }
    else {
        nl_indent(st);
        st << "stateactioncanvas " << get_ident() << " ";
        browser_node->save(st, TRUE, warning);
        indent(+1);
        nl_indent(st);

        if (language != DefaultDrawingLanguage)
            st << "language " << stringify(language) << " ";

        if (show_stereotype_properties != UmlDefaultState)
            st << "show_stereotype_properties " << stringify(show_stereotype_properties) << " ";

        if (itscolor != UmlDefaultColor)
            st << "color " << stringify(itscolor) << " ";

        save_xyzwh(st, this, "xyzwh");
        save_stereotype_property(st, warning);
        indent(-1);
        nl_indent(st);
        st << "end";
    }
}

StateActionCanvas * StateActionCanvas::read(char *& st, UmlCanvas * canvas,
        char * k)
{
    if (!strcmp(k, "stateactioncanvas_ref"))
        return ((StateActionCanvas *) dict_get(read_id(st), "stateactioncanvas", canvas));
    else if (!strcmp(k, "stateactioncanvas")) {
        int id = read_id(st);
        BrowserStateAction * br = BrowserStateAction::read_ref(st);
        StateActionCanvas * result = new StateActionCanvas(canvas, id);

        result->browser_node = br;
        connect(br->get_data(), SIGNAL(changed()), result, SLOT(modified()));
        connect(br->get_data(), SIGNAL(deleted()), result, SLOT(deleted()));

        k = read_keyword(st);

        if (!strcmp(k, "language")) {
            result->language = drawing_language(read_keyword(st));
            k = read_keyword(st);
        }

        if (!strcmp(k, "show_stereotype_properties")) {
            result->show_stereotype_properties = state(read_keyword(st));
            k = read_keyword(st);
        }

        read_color(st, "color", result->itscolor, k);	// updates k

        if (!strcmp(k, "xyzwh"))
            read_xyzwh(st, result);
        else
            wrong_keyword(k, "xyzwh");

        if (read_file_format() >= 58) {
            k = read_keyword(st);
            result->read_stereotype_property(st, k);	// updates k

            if (strcmp(k, "end"))
                wrong_keyword(k, "end");
        }

        result->compute_size();
        result->set_center100();
        result->show();
        result->check_stereotypeproperties();
        return result;
    }
    else
        return 0;
}

void StateActionCanvas::history_save(QBuffer & b) const
{
    DiagramCanvas::history_save(b);
    ::save(min_width, b);
    ::save(min_height, b);
    ::save(width_scale100, b);
    ::save(height_scale100, b);
    ::save(width(), b);
    ::save(height(), b);
}

void StateActionCanvas::history_load(QBuffer & b)
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

    connect(browser_node->get_data(), SIGNAL(changed()), this, SLOT(modified()));
    connect(browser_node->get_data(), SIGNAL(deleted()), this, SLOT(deleted()));
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
}

void StateActionCanvas::history_hide()
{
    Q3CanvasItem::setVisible(FALSE);
    disconnect(DrawingSettings::instance(), SIGNAL(changed()),
               this, SLOT(modified()));
    disconnect(browser_node->get_data(), 0, this, 0);
}

