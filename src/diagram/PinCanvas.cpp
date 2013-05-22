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

#include "PinCanvas.h"
#include "BrowserPin.h"
#include "ActivityActionCanvas.h"
#include "PinData.h"
#include "LabelCanvas.h"
#include "FlowCanvas.h"
#include "UmlCanvas.h"
#include "BrowserClass.h"
#include "BrowserDiagram.h"
#include "UmlGlobal.h"
#include "SettingsDialog.h"
#include "myio.h"
#include "ToolCom.h"
#include "Tool.h"
#include "ui/menufactory.h"
#include "Settings.h"
#include "strutil.h"
#include "translate.h"

PinCanvas::PinCanvas(BrowserNode * bn, UmlCanvas * canvas,
                     int x, int y, int id, ActivityActionCanvas * a)
    : DiagramCanvas(0, canvas, x, y, PIN_SIZE, PIN_SIZE, id), act(a)
{
    browser_node = bn;
    itscolor = UmlDefaultColor;

    connect(bn->get_data(), SIGNAL(changed()), this, SLOT(modified()));
    connect(bn->get_data(), SIGNAL(deleted()), this, SLOT(deleted()));
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));

    if (id == 0) {
        // not on read
        update();
        setZ(a->z() + 2);  // z+2 to be upper the parameter sets with their lines

        if (canvas->must_draw_all_relations())
            draw_all_flows();

        check_stereotypeproperties();
    }
}

PinCanvas::~PinCanvas()
{
}

void PinCanvas::delete_it()
{
    // don't remove it from action's pins to manage undo
    disconnect(browser_node->get_data(), 0, this, 0);
    disconnect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));

    DiagramCanvas::delete_it();
}

void PinCanvas::deleted()
{
    delete_it();
    act->deleted(this);
    canvas()->update();
}

void PinCanvas::remove(bool from_model)
{
    if (! from_model)
        delete_it();
    else
        browser_node->delete_it();	// will remove canvas
}

void PinCanvas::delete_available(BooL & in_model, BooL & out_model) const
{
    out_model |= TRUE;
    in_model |= browser_node->is_writable();
}

void PinCanvas::update()
{
    used_color = (itscolor == UmlDefaultColor)
                 ? the_canvas()->browser_diagram()->get_color(UmlActivityPin)
                 : itscolor;

    PinData * d = (PinData *) browser_node->get_data();
    QString s = browser_node->get_name();
    QString effect;
    bool multilines = FALSE;

    if (d->get_effect() != UmlNoEffect)
        effect = QString("effect=") + stringify(d->get_effect());

    if (s.isEmpty())
        s = d->get_type().get_type();

    if (d->get_multiplicity()[0])
        s += QString("[") + d->get_multiplicity() + QString("]");

    if (d->get_is_stream()) {
        if (s.isEmpty()) {
            if (effect.isEmpty())
                s = "{stream}";
            else
                s = "{" + effect + ", stream}";
        }
        else {
            multilines = TRUE;

            if (effect.isEmpty())
                s += "\n{stream}";
            else
                s += "\n{" + effect + ", stream}";
        }
    }
    else if (d->get_is_exception()) {
        if (s.isEmpty()) {
            if (effect.isEmpty())
                s = LabelCanvas::Triangle;
            else {
                multilines = TRUE;
                s = "{" + effect + "}\n" + LabelCanvas::Triangle;
            }
        }
        else {
            multilines = TRUE;

            if (effect.isEmpty())
                s += QString("\n") + LabelCanvas::Triangle;
            else
                s += " {" + effect + "}\n" + LabelCanvas::Triangle;
        }
    }

    if (!s.isEmpty()) {
        if (label == 0) {
            QFontMetrics fm(the_canvas()->get_font(UmlNormalFont));
            QRect r = fm.boundingRect(0, 0, 1000, 1000, ::Qt::AlignCenter, s);
            QPoint pp = center();
            QPoint pa = act->center();
            int x, y;

            if (pp.x() < pa.x()) {
                x = (int) this->x() - r.width() - 5;
                y = pp.y() - ((multilines) ? r.height() / 2 : r.height());
            }
            else if (pp.x() > pa.x()) {
                x = this->rect().right() + 5;
                y = pp.y() - ((multilines) ? r.height() / 2 : r.height());
            }
            else if (pp.y() < pa.y()) {
                x = pp.x() - r.width() - 5;
                y = (int) this->y() - r.height() - 5;
            }
            else {
                x = pp.x() + r.width() + 5;
                y = this->rect().bottom() + 5;
            }

            label = new LabelCanvas(s, the_canvas(), x, y);
        }
        else
            label->set_name(s);
    }
    else if (label != 0) {
        label->delete_it();
        label = 0;
    }

    check_position();
}

// check position, note : size is odd

void PinCanvas::check_position()
{
    QRect r = rect();
    QRect action_r = act->rect();
    int shadow = act->shadow_margin();

    if (shadow != 0) {
        action_r.setRight(action_r.right() - shadow);
        action_r.setBottom(action_r.bottom() - shadow);
    }

    double dxl = fabs(action_r.left() - r.right());
    double dxr = fabs(action_r.right() - r.left());
    double dyt = fabs(action_r.top() - r.bottom());
    double dyb = fabs(action_r.bottom() - r.top());

    if ((dxr == 0) || (dxl == 0)) {
        if (r.bottom() < action_r.top())
            DiagramCanvas::moveBy(0, action_r.top() - r.bottom());
        else if (r.top() > action_r.bottom())
            DiagramCanvas::moveBy(0, action_r.bottom() - r.top());
    }
    else if ((dyt == 0) || (dyb == 0)) {
        if (r.right() < action_r.left())
            DiagramCanvas::moveBy(action_r.left() - r.right(), 0);
        else if (r.left() > action_r.right())
            DiagramCanvas::moveBy(action_r.right() - r.left(), 0);
    }
    else {
        double dx = (dxl < dxr) ? dxl : dxr;
        double dy = (dyt < dyb) ? dyt : dyb;

        if (dx < dy) {
            dx = (dxl < dxr) ? action_r.left() - r.right()
                 : action_r.right() - r.left();

            if (r.bottom() < action_r.top())
                DiagramCanvas::moveBy(dx, action_r.top() - r.bottom());
            else if (r.top() > action_r.bottom())
                DiagramCanvas::moveBy(dx, action_r.bottom() - r.top());
            else
                DiagramCanvas::moveBy(dx, 0);
        }
        else {
            dy = (dyt < dyb) ? action_r.top() - r.bottom()
                 : action_r.bottom() - r.top();

            if (r.right() < action_r.left())
                DiagramCanvas::moveBy(action_r.left() - r.right(), dy);
            else if (r.left() > action_r.right())
                DiagramCanvas::moveBy(action_r.right() - r.left(), dy);
            else
                DiagramCanvas::moveBy(0, dy);
        }
    }
}

void PinCanvas::change_scale()
{
    // does nothing, made by do_change_scale called by
    // ActivityActionCanvas::change_scale()
}

void PinCanvas::do_change_scale()
{
    Q3CanvasRectangle::setVisible(FALSE);
    int sz = (int)(PIN_SIZE * the_canvas()->zoom());

    setSize(sz, sz);
    recenter();
    // action already in position, can check
    check_position();
    Q3CanvasRectangle::setVisible(TRUE);
}

void PinCanvas::moveBy(double dx, double dy)
{
    if (!act->selected() || the_canvas()->paste()) {
        do_moveBy(dx, dy);
        act->check_parameter_sets_position();
    }

    // else do_moveBy called by ActivityActionCanvas::moveBy()
}

void PinCanvas::do_moveBy(double dx, double dy)
{
    DiagramCanvas::moveBy(dx, dy);

    // action already in position, can check
    check_position();

    if (!the_canvas()->do_zoom())
        set_center100();
}

bool PinCanvas::primaryItem() const
{
    return FALSE;
}

void PinCanvas::draw(QPainter & p)
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

    p.setBackgroundColor(co);

    if (used_color != UmlTransparent)
        p.setBrush(co);

    if (fp != 0)
        fprintf(fp, "<g>\n"
                "\t<rect fill=\"%s\" stroke=\"black\" stroke-width=\"1\" stroke-opacity=\"1\""
                " x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" />\n"
                "</g>\n",
                svg_color(used_color),
                r.x(), r.y(), r.width() - 1, r.height() - 1);

    p.drawRect(r);

    p.setBackgroundColor(bckgrnd);
    p.setBrush(brsh);

    if (selected())
        show_mark(p, r);
}

UmlCode PinCanvas::type() const
{
    return UmlActivityPin;
}

bool PinCanvas::copyable() const
{
    return act->selected();
}

bool PinCanvas::action_selected() const
{
    return act->selected();
}

void PinCanvas::open()
{
    browser_node->open(FALSE);
}

void PinCanvas::menu(const QPoint &)
{
    Q3PopupMenu m(0);
    Q3PopupMenu toolm(0);
    int index;
    BrowserClass * cl =
        ((PinData *) browser_node->get_data())->get_type().type;

    MenuFactory::createTitle(m, browser_node->get_data()->definition(FALSE, TRUE));
    m.insertSeparator();
    m.insertItem(TR("Upper"), 0);
    m.insertItem(TR("Lower"), 1);
    m.insertItem(TR("Go up"), 13);
    m.insertItem(TR("Go down"), 14);
    m.insertSeparator();
    m.insertItem(TR("Edit drawing settings"), 2);
    m.insertSeparator();
    m.insertItem(TR("Edit pin"), 3);
    m.insertSeparator();
    m.insertItem(TR("Select in browser"), 4);

    if (cl != 0)
        m.insertItem(TR("Select class in browser"), 9);

    if (linked())
        m.insertItem(TR("Select linked items"), 5);

    m.insertSeparator();

    if (browser_node->is_writable()) {
        m.insertItem(TR("Delete from model"), 8);
        m.insertSeparator();
    }

    if (Tool::menu_insert(&toolm, UmlActivityPin, 20))
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

void PinCanvas::apply_shortcut(QString s)
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

void PinCanvas::edit_drawing_settings()
{
    for (;;) {
        ColorSpecVector co(1);

        co[0].set(TR("pin color"), &itscolor);

        SettingsDialog dialog(0, &co, FALSE);

        dialog.raise();

        if (dialog.exec() == QDialog::Accepted)
            modified();	// call package_modified()

        if (!dialog.redo())
            break;
    }
}

bool PinCanvas::has_drawing_settings() const
{
    return TRUE;
}

void PinCanvas::edit_drawing_settings(QList<DiagramItem *> & l)
{
    for (;;) {
        ColorSpecVector co(1);
        UmlColor itscolor;

        co[0].set(TR("pin color"), &itscolor);

        SettingsDialog dialog(0, &co, FALSE, TRUE);

        dialog.raise();

        if ((dialog.exec() == QDialog::Accepted) && !co[0].name.isEmpty()) {
            foreach (DiagramItem *item, l) {
                PinCanvas *canvas = (PinCanvas *)item;
                canvas->itscolor = itscolor;
                canvas->modified();
            }
        }

        if (!dialog.redo())
            break;
    }
}

void PinCanvas::clone_drawing_settings(const DiagramItem *src)
{
    const PinCanvas * x = (const PinCanvas *) src;
    itscolor = x->itscolor;
    modified();
}

QString PinCanvas::may_start(UmlCode & l) const
{
    switch (l) {
    case UmlFlow:
        return ((BrowserPin *) browser_node)->may_start();

    case UmlDependOn:
        return TR("illegal");

    default: // anchor
        return 0;
    }
}

QString PinCanvas::may_connect(UmlCode & l, const DiagramItem * dest) const
{
    if (l == UmlAnchor)
        return dest->may_start(l);
    else if (dest->get_bn() == 0)
        return TR("illegal");
    else
        return ((BrowserPin *) browser_node)->may_connect(dest->get_bn());
}

void PinCanvas::modified()
{
    hide();
    hide_lines();
    update();
    show();
    update_show_lines();

    if (the_canvas()->must_draw_all_relations())
        draw_all_flows();

    check_stereotypeproperties();
    canvas()->update();
    package_modified();
}

void PinCanvas::post_loaded()
{
    if (the_canvas()->must_draw_all_relations())
        draw_all_flows();
}

void PinCanvas::connexion(UmlCode action, DiagramItem * dest,
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

void PinCanvas::save(QTextStream & st, bool ref, QString & warning) const
{
    if (ref) {
        st << "pincanvas_ref " << get_ident() << " // "
           << browser_node->full_name();
    }
    else {
        nl_indent(st);
        st << "pincanvas " << get_ident() << " ";
        browser_node->save(st, TRUE, warning);
        nl_indent(st);

        if (itscolor != UmlDefaultColor)
            st << "  color " << stringify(itscolor);

        save_xyzwh(st, this, "  xyzwh");

        if (label != 0)
            save_xy(st, label, " label_xy");

        save_stereotype_property(st, warning);
        nl_indent(st);
        st << "end";
    }
}

PinCanvas * PinCanvas::read(char *& st, UmlCanvas * canvas,
                            char * k, ActivityActionCanvas * a)
{
    if (!strcmp(k, "pincanvas_ref"))
        return ((PinCanvas *) dict_get(read_id(st), "pincanvas", canvas));
    else if (!strcmp(k, "pincanvas")) {
        int id = read_id(st);
        BrowserPin * br = BrowserPin::read_ref(st);
        PinCanvas * result = new PinCanvas(br, canvas, 0, 0, id, a);

        k = read_keyword(st);

        read_color(st, "color", result->itscolor, k);	// updates k

        if (!strcmp(k, "xyzwh"))
            read_xyzwh(st, result);
        else
            wrong_keyword(k, "xyzwh");

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

void PinCanvas::history_hide()
{
    Q3CanvasItem::setVisible(FALSE);
    disconnect(browser_node->get_data(), 0, this, 0);
    disconnect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
}

void PinCanvas::history_save(QBuffer & b) const
{
    DiagramCanvas::history_save(b);
    ::save(width_scale100, b);
    ::save(height_scale100, b);
    ::save(width(), b);
    ::save(height(), b);
}

void PinCanvas::history_load(QBuffer & b)
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

