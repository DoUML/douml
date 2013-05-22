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





#include <math.h>

#include <qpainter.h>
#include <q3popupmenu.h>
#include <qcursor.h>
//Added by qt3to4:
#include <QTextStream>
#include <Q3ValueList>

#include "ParameterCanvas.h"
#include "BrowserParameter.h"
#include "ActivityCanvas.h"
#include "ParameterData.h"
#include "LabelCanvas.h"
#include "FlowCanvas.h"
#include "UmlCanvas.h"
#include "BrowserDiagram.h"
#include "BrowserClass.h"
#include "UmlGlobal.h"
#include "SettingsDialog.h"
#include "myio.h"
#include "ToolCom.h"
#include "Tool.h"
#include "ui/menufactory.h"
#include "Settings.h"
#include "strutil.h"
#include "translate.h"

ParameterCanvas::ParameterCanvas(BrowserNode * bn, UmlCanvas * canvas,
                                 int x, int y, int id, ActivityCanvas * a)
    : DiagramCanvas(0, canvas, x, y, PARAMETER_CANVAS_MIN_WIDTH,
                    PARAMETER_CANVAS_MIN_HEIGHT, id),
    act(a)
{
    browser_node = bn;
    itscolor = UmlDefaultColor;

    if (id == 0) {
        // not on read
        update();
        setZ(a->z() + 2);	// 2 to have lines upper activity's z

        if (canvas->must_draw_all_relations())
            draw_all_flows();

        check_stereotypeproperties();
    }

    connect(bn->get_data(), SIGNAL(changed()), this, SLOT(modified()));
    connect(bn->get_data(), SIGNAL(deleted()), this, SLOT(deleted()));
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
}

ParameterCanvas::~ParameterCanvas()
{
}

void ParameterCanvas::delete_it()
{
    // don't remove it from activity's params to manage undo
    disconnect(browser_node->get_data(), 0, this, 0);
    disconnect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));

    DiagramCanvas::delete_it();
}

void ParameterCanvas::deleted()
{
    delete_it();
    act->deleted(this);
    canvas()->update();
}

void ParameterCanvas::remove(bool from_model)
{
    if (! from_model)
        delete_it();
    else
        browser_node->delete_it();	// will remove canvas
}

void ParameterCanvas::update()
{
    used_color = (itscolor == UmlDefaultColor)
                 ? the_canvas()->browser_diagram()->get_color(UmlParameter)
                 : itscolor;

    ParameterData * d = (ParameterData *) browser_node->get_data();

    if (d->get_is_stream()) {
        if (label == 0) {
            QFontMetrics fm(the_canvas()->get_font(UmlNormalFont));

            label = new LabelCanvas("{stream}", the_canvas(),
                                    (int) x(), (int) y() - fm.height() - 4);
        }
        else
            label->set_name("{stream}");
    }
    else if (d->get_is_exception()) {
        if (label == 0) {
            QFontMetrics fm(the_canvas()->get_font(UmlNormalFont));

            label = new LabelCanvas(LabelCanvas::Triangle, the_canvas(),
                                    (int) x(), (int) y() - fm.height() - 4);
        }
        else
            label->set_name(LabelCanvas::Triangle);
    }
    else if (label != 0) {
        label->delete_it();
        label = 0;
    }

    check_position();
}

// check position, note : size is odd

void ParameterCanvas::check_position()
{
    QRect act_rect = act->rect();
    QPoint ce = center();

    double dxl = fabs(act_rect.left() - ce.x());
    double dxr = fabs(act_rect.right() - ce.x());
    double dyt = fabs(act_rect.top() - ce.y());
    double dyb = fabs(act_rect.bottom() - ce.y());

    if ((dxr == 0) || (dxl == 0)) {
        if (ce.y() < act_rect.top())
            DiagramCanvas::moveBy(0, act_rect.top() - ce.y());
        else if (ce.y() > act_rect.bottom())
            DiagramCanvas::moveBy(0, act_rect.bottom() - ce.y());
    }
    else if ((dyt == 0) || (dyb == 0)) {
        if (ce.x() < act_rect.left())
            DiagramCanvas::moveBy(act_rect.left() - ce.x(), 0);
        else if (ce.x() > act_rect.right())
            DiagramCanvas::moveBy(act_rect.right() - ce.x(), 0);
    }
    else {
        double dx = (dxl < dxr) ? dxl : dxr;
        double dy = (dyt < dyb) ? dyt : dyb;

        if (dx < dy) {
            dx = (dxl < dxr) ? act_rect.left() - ce.x()
                 : act_rect.right() - ce.x();

            if (ce.y() < act_rect.top())
                DiagramCanvas::moveBy(dx, act_rect.top() - ce.y());
            else if (ce.y() > act_rect.bottom())
                DiagramCanvas::moveBy(dx, act_rect.bottom() - ce.y());
            else
                DiagramCanvas::moveBy(dx, 0);
        }
        else {
            dy = (dyt < dyb) ? act_rect.top() - ce.y()
                 : act_rect.bottom() - ce.y();

            if (ce.x() < act_rect.left())
                DiagramCanvas::moveBy(act_rect.left() - ce.x(), dy);
            else if (ce.x() > act_rect.right())
                DiagramCanvas::moveBy(act_rect.right() - ce.x(), dy);
            else
                DiagramCanvas::moveBy(0, dy);
        }
    }
}

aCorner ParameterCanvas::on_resize_point(const QPoint & p)
{
    return ::on_resize_point(p, rect());
}

void ParameterCanvas::resize(aCorner c, int dx, int dy, QPoint & o)
{
    double zoom = the_canvas()->zoom();

    DiagramCanvas::resize(c, dx, dy, o,
                          (int)(PARAMETER_CANVAS_MIN_WIDTH * zoom),
                          (int)(PARAMETER_CANVAS_MIN_HEIGHT * zoom),
                          TRUE, TRUE);

    check_position();	// theorically useless
}

void ParameterCanvas::resize(const QSize & sz, bool w, bool h)
{
    double zoom = the_canvas()->zoom();

    if (DiagramCanvas::resize(sz, w, h,
                              (int)(PARAMETER_CANVAS_MIN_WIDTH * zoom),
                              (int)(PARAMETER_CANVAS_MIN_HEIGHT * zoom),
                              TRUE, TRUE))
        check_position();	// theorically useless
}

void ParameterCanvas::change_scale()
{
    // does nothing, made by do_change_scale called by
    // ActivityCanvas::change_scale()
}

void ParameterCanvas::do_change_scale()
{
    Q3CanvasRectangle::setVisible(FALSE);
    double scale = the_canvas()->zoom();

    setSize((int)(width_scale100 * scale) | 1,
            (int)(height_scale100 * scale) | 1);
    recenter();
    // activity already in position, can check
    check_position();
    Q3CanvasRectangle::setVisible(TRUE);
}

void ParameterCanvas::moveBy(double dx, double dy)
{
    if (!act->selected() || the_canvas()->paste())
        do_moveBy(dx, dy);

    // else do_moveBy called by ActivityCanvas::moveBy()
}

void ParameterCanvas::do_moveBy(double dx, double dy)
{
    DiagramCanvas::moveBy(dx, dy);

    // activity already in position, can check
    check_position();

    if (!the_canvas()->do_zoom())
        set_center100();
}

bool ParameterCanvas::primaryItem() const
{
    return FALSE;
}

void ParameterCanvas::draw(QPainter & p)
{
    if (! visible()) return;

    p.setRenderHint(QPainter::Antialiasing, true);
    QBrush brsh = p.brush();
    QColor bckgrnd = p.backgroundColor();

    p.setBackgroundMode((used_color == UmlTransparent)
                        ? ::Qt::TransparentMode
                        : ::Qt::OpaqueMode);

    QColor co = color(used_color);
    QRect r = rect();
    FILE * fp = svg();

    if (fp != 0)
        fputs("<g>\n", fp);

    p.setBackgroundColor(co);

    if (used_color != UmlTransparent)
        p.setBrush(co);

    if (fp != 0)
        fprintf(fp, "\t<rect fill=\"%s\" stroke=\"black\" stroke-width=\"1\" stroke-opacity=\"1\""
                " x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" />\n",
                svg_color(used_color),
                r.x(), r.y(), r.width() - 1, r.height() - 1);

    p.drawRect(r);

    p.setFont(the_canvas()->get_font(UmlNormalFont));
    p.drawText(r.left() + 2, r.top() + 2,
               r.width() - 4, r.height() - 4,
               ::Qt::AlignCenter + ::Qt::TextWordWrap,
               browser_node->get_name());

    if (fp != 0)
        draw_text(r.left() + 2, r.top() + 2,
                  r.width() - 4, r.height() - 4,
                  ::Qt::AlignCenter + ::Qt::TextWordWrap,
                  browser_node->get_name(),
                  p.font(), fp);

    if (fp != 0)
        fputs("</g>\n", fp);

    p.setBackgroundColor(bckgrnd);
    p.setBrush(brsh);

    if (selected())
        show_mark(p, r);
}

UmlCode ParameterCanvas::type() const
{
    return UmlParameter;
}

bool ParameterCanvas::copyable() const
{
    return act->selected();
}

bool ParameterCanvas::activity_selected() const
{
    return act->selected();
}

void ParameterCanvas::open()
{
    browser_node->open(FALSE);
}

void ParameterCanvas::menu(const QPoint &)
{
    Q3PopupMenu m(0);
    Q3PopupMenu toolm(0);
    int index;
    Q3ValueList<ParameterCanvas *> params = act->get_params();
    BrowserClass * cl =
        ((ParameterData *) browser_node->get_data())->get_type().type;

    MenuFactory::createTitle(m, browser_node->get_data()->definition(FALSE, TRUE));
    m.insertSeparator();
    m.insertItem(TR("Upper"), 0);
    m.insertItem(TR("Lower"), 1);
    m.insertItem(TR("Go up"), 13);
    m.insertItem(TR("Go down"), 14);
    m.insertSeparator();
    m.insertItem(TR("Edit drawing settings"), 2);

    if (params.count() > 1)
        m.insertItem(TR("Resize other like it"), 6);

    m.insertSeparator();
    m.insertItem(TR("Edit parameter"), 3);
    m.insertSeparator();
    m.insertItem(TR("Select in browser"), 4);

    if (cl != 0)
        m.insertItem(TR("Select class in browser"), 9);

    if (linked())
        m.insertItem(TR("Select linked items"), 5);

    m.insertSeparator();

    if (browser_node->is_writable())
        m.insertItem(TR("Delete from model"), 8);

    m.insertSeparator();

    if (Tool::menu_insert(&toolm, UmlParameter, 20))
        m.insertItem(TR("Tool"), &toolm);

    switch (index = m.exec(QCursor::pos())) {
    case 0:
        act->upper();
        modified();	// call package_modified()
        return;

    case 1:
        act->lower();
        modified();	// call package_modified()
        return;

    case 13:
        act->z_up();
        modified();	// call package_modified()
        return;

    case 14:
        act->z_down();
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

    case 6: {
        int w = width();
        int h = height();
        Q3ValueList<ParameterCanvas *>::Iterator iter;

        for (iter = params.begin(); iter != params.end(); ++iter) {
            if (*iter != this) {
                (*iter)->setSize(w, h);
                (*iter)->recenter();
                (*iter)->width_scale100 = width_scale100;
                (*iter)->height_scale100 = height_scale100;
            }
        }
    }
    break;

    case 8:
        //delete from model
        browser_node->delete_it();	// will delete the canvas
        break;

    case 9:
        cl->select_in_browser();
        return;

    default:
        if (index >= 20)
            ToolCom::run(Tool::command(index - 20), browser_node);

        return;
    }

    package_modified();
}

void ParameterCanvas::apply_shortcut(QString s)
{
    if (s == "Select in browser") {
        browser_node->select_in_browser();
        return;
    }
    else if (s == "Upper")
        act->upper();
    else if (s == "Lower")
        act->lower();
    else if (s == "Go up")
        act->z_up();
    else if (s == "Go down")
        act->z_down();
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

void ParameterCanvas::edit_drawing_settings()
{
    for (;;) {
        ColorSpecVector co(1);

        co[0].set(TR("parameter node color"), &itscolor);

        SettingsDialog dialog(0, &co, FALSE);

        dialog.raise();

        if (dialog.exec() == QDialog::Accepted)
            modified();	// call package_modified()

        if (!dialog.redo())
            break;
    }
}

bool ParameterCanvas::has_drawing_settings() const
{
    return TRUE;
}

void ParameterCanvas::edit_drawing_settings(QList<DiagramItem *> & l)
{
    for (;;) {
        ColorSpecVector co(1);
        UmlColor itscolor;

        co[0].set(TR("parameter node color"), &itscolor);

        SettingsDialog dialog(0, &co, FALSE, TRUE);

        dialog.raise();

        if ((dialog.exec() == QDialog::Accepted) && !co[0].name.isEmpty()) {
            foreach (DiagramItem *item, l) {
                ParameterCanvas *canvas = (ParameterCanvas *)item;
                canvas->itscolor = itscolor;
                canvas->modified();
            }
        }

        if (!dialog.redo())
            break;
    }
}

void ParameterCanvas::clone_drawing_settings(const DiagramItem *src)
{
    const ParameterCanvas * x = (const ParameterCanvas *) src;
    itscolor = x->itscolor;
    modified();
}

QString ParameterCanvas::may_start(UmlCode & l) const
{
    switch (l) {
    case UmlFlow:
        return ((BrowserParameter *) browser_node)->may_start();

    case UmlDependOn:
        return TR("illegal");

    default: // anchor
        return 0;
    }
}

QString ParameterCanvas::may_connect(UmlCode & l, const DiagramItem * dest) const
{
    if (l == UmlAnchor)
        return dest->may_start(l);
    else if (dest->get_bn() == 0)
        return TR("illegal");
    else
        return ((BrowserParameter *) browser_node)->may_connect(dest->get_bn());
}

void ParameterCanvas::connexion(UmlCode action, DiagramItem * dest,
                                const QPoint &, const QPoint &)
{
    ArrowCanvas * a;

    if (action == UmlFlow)
        a = new FlowCanvas(the_canvas(), this, dest, 0, 0, -1.0, -1.0);
    else
        a = new ArrowCanvas(the_canvas(), this, dest, action, 0, FALSE, -1.0, -1.0);

    a->show();
    the_canvas()->select(a);
}

void ParameterCanvas::modified()
{
    hide();
    hide_lines();
    update();
    show();
    check_stereotypeproperties();
    update_show_lines();

    if (the_canvas()->must_draw_all_relations())
        draw_all_flows();

    canvas()->update();
    package_modified();
}

void ParameterCanvas::post_loaded()
{
    if (the_canvas()->must_draw_all_relations())
        draw_all_flows();
}

void ParameterCanvas::save(QTextStream & st, bool ref, QString & warning) const
{
    if (ref) {
        st << "parametercanvas_ref " << get_ident() << " // "
           << browser_node->full_name();
    }
    else {
        nl_indent(st);
        st << "parametercanvas " << get_ident() << " ";
        browser_node->save(st, TRUE, warning);
        indent(+1);
        nl_indent(st);

        if (itscolor != UmlDefaultColor)
            st << "  color " << stringify(itscolor);

        save_xyzwh(st, this, "  xyzwh");

        if (label != 0)
            save_xy(st, label, " label_xy");

        save_stereotype_property(st, warning);
        indent(-1);
        nl_indent(st);
        st << "end";
    }
}

ParameterCanvas * ParameterCanvas::read(char *& st, UmlCanvas * canvas,
                                        char * k, ActivityCanvas * a)
{
    if (!strcmp(k, "parametercanvas_ref"))
        return ((ParameterCanvas *) dict_get(read_id(st), "parametercanvas", canvas));
    else if (!strcmp(k, "parametercanvas")) {
        int id = read_id(st);
        BrowserParameter * br = BrowserParameter::read_ref(st);
        ParameterCanvas * result =
            new ParameterCanvas(br, canvas, 0, 0, id, a);

        k = read_keyword(st);

        read_color(st, "color", result->itscolor, k);	// updates k

        if (!strcmp(k, "xyzwh"))
            read_xyzwh(st, result);
        else
            wrong_keyword(k, "xyzwh");

        result->width_scale100 = result->width();
        result->height_scale100 = result->height();
        result->set_center100();
        result->update();
        result->show();

        k = read_keyword(st);

        if (!strcmp(k, "label_xy")) {
            if (result->label != 0) {
                read_xy(st, result->label);
                result->label->set_center100();
            }
            else
                bypass_xy(st);

            k = read_keyword(st);
        }

        result->read_stereotype_property(st, k);	// updates k

        if (strcmp(k, "end"))
            wrong_keyword(k, "end");

        result->check_stereotypeproperties();

        return result;
    }
    else
        return 0;
}

void ParameterCanvas::history_hide()
{
    Q3CanvasItem::setVisible(FALSE);
    disconnect(browser_node->get_data(), SIGNAL(changed()), this, SLOT(modified()));
    disconnect(browser_node->get_data(), SIGNAL(deleted()), this, SLOT(deleted()));
    disconnect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
}

void ParameterCanvas::history_save(QBuffer & b) const
{
    DiagramCanvas::history_save(b);
    ::save(width_scale100, b);
    ::save(height_scale100, b);
    ::save(width(), b);
    ::save(height(), b);
}

void ParameterCanvas::history_load(QBuffer & b)
{
    DiagramCanvas::history_load(b);
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
