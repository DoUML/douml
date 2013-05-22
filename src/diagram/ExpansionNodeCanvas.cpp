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

#include "ExpansionNodeCanvas.h"
#include "BrowserExpansionNode.h"
#include "ExpansionRegionCanvas.h"
#include "ArrowCanvas.h"
#include "FlowCanvas.h"
#include "ActivityObjectData.h"
#include "LabelCanvas.h"
#include "UmlCanvas.h"
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

ExpansionNodeCanvas::ExpansionNodeCanvas(BrowserNode * bn, UmlCanvas * canvas,
        int x, int y, int id, ExpansionRegionCanvas * r)
    : DiagramCanvas(0, canvas, x, y, EXPANSIONNODE_WIDTH,
                    EXPANSIONNODE_HEIGHT, id),
    region(r)
{
    browser_node = bn;
    itscolor = UmlDefaultColor;

    if (id == 0) {
        // not on read
        update();
        setZ(r->z() + 2);	// 2 to have lines upper region's z

        if (canvas->must_draw_all_relations())
            draw_all_flows();

        check_stereotypeproperties();
    }

    connect(bn->get_data(), SIGNAL(changed()), this, SLOT(modified()));
    connect(bn->get_data(), SIGNAL(deleted()), this, SLOT(deleted()));
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
}

ExpansionNodeCanvas::~ExpansionNodeCanvas()
{
}

void ExpansionNodeCanvas::delete_it()
{
    // don't remove it from region's nodes to manage undo
    disconnect(browser_node->get_data(), 0, this, 0);
    disconnect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));

    DiagramCanvas::delete_it();
}

void ExpansionNodeCanvas::deleted()
{
    delete_it();
    region->deleted(this);
    canvas()->update();
}

void ExpansionNodeCanvas::remove(bool from_model)
{
    if (! from_model)
        delete_it();
    else
        browser_node->delete_it();	// will remove canvas
}

void ExpansionNodeCanvas::update()
{
    used_color = (itscolor == UmlDefaultColor)
                 ? the_canvas()->browser_diagram()->get_color(UmlExpansionNode)
                 : itscolor;

    QString s = browser_node->get_name();

    if (!s.isEmpty()) {
        if (label == 0) {
            QFontMetrics fm(the_canvas()->get_font(UmlNormalFont));
            int ly = (y() < region->y())
                     ? (int) y() - fm.height() - 4
                     : (int) y() + height() + 4;

            label = new LabelCanvas(s, the_canvas(), (int) x(), ly);
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

void ExpansionNodeCanvas::check_position()
{
    QRect act_rect = region->rect();
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

void ExpansionNodeCanvas::change_scale()
{
    // does nothing, made by do_change_scale called by
    // ExpansionRegionCanvas::change_scale()
}

void ExpansionNodeCanvas::do_change_scale()
{
    Q3CanvasRectangle::setVisible(FALSE);
    double scale = the_canvas()->zoom();

    setSize((int)(width_scale100 * scale) | 1,
            (int)(height_scale100 * scale) | 1);
    recenter();
    // expansion region already in position, can check
    check_position();
    Q3CanvasRectangle::setVisible(TRUE);
}

void ExpansionNodeCanvas::moveBy(double dx, double dy)
{
    if (!region->selected() || the_canvas()->paste())
        do_moveBy(dx, dy);

    // else do_moveBy called by ExpansionRegionCanvas::moveBy()
}

void ExpansionNodeCanvas::do_moveBy(double dx, double dy)
{
    DiagramCanvas::moveBy(dx, dy);

    // expansion region already in position, can check
    check_position();

    if (!the_canvas()->do_zoom())
        set_center100();
}

void ExpansionNodeCanvas::draw(QPainter & p)
{
    if (! visible()) return;

    p.setRenderHint(QPainter::Antialiasing, true);
    QBrush brsh = p.brush();
    QColor bckgrnd = p.backgroundColor();

    p.setBackgroundMode((used_color == UmlTransparent)
                        ? ::Qt::TransparentMode
                        : ::Qt::OpaqueMode);

    QColor co = color(used_color);
    FILE * fp = svg();

    p.setBackgroundColor(co);

    if (used_color != UmlTransparent)
        p.setBrush(co);

    QRect r = rect();

    p.drawRect(r);

    if (fp != 0) {
        fputs("<g>\n", fp);
        fprintf(fp, "\t<rect fill=\"%s\" stroke=\"black\" stroke-width=\"1\" stroke-opacity=\"1\""
                " x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" />\n",
                svg_color(used_color),
                r.x(), r.y(), r.width() - 1, r.height() - 1);
    }

    int dx = width() / 3;
    int b = r.bottom();

    p.drawLine((int) x() + dx, (int) y(), (int) x() + dx, b);

    if (fp != 0)
        fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                (int) x() + dx, (int) y(), (int) x() + dx, b);

    dx += dx;
    p.drawLine((int) x() + dx, (int) y(), (int) x() + dx, b);

    if (fp != 0) {
        fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                (int) x() + dx, (int) y(), (int) x() + dx, b);
        fputs("</g>\n", fp);
    }

    p.setBackgroundColor(bckgrnd);
    p.setBrush(brsh);

    if (selected())
        show_mark(p, r);
}

UmlCode ExpansionNodeCanvas::type() const
{
    return UmlExpansionNode;
}

bool ExpansionNodeCanvas::copyable() const
{
    return region->selected();
}

bool ExpansionNodeCanvas::region_selected() const
{
    return region->selected();
}

void ExpansionNodeCanvas::open()
{
    browser_node->open(FALSE);
}

void ExpansionNodeCanvas::menu(const QPoint &)
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
    m.insertItem(TR("Edit expansion node"), 3);
    m.insertSeparator();
    m.insertItem(TR("Select in browser"), 4);

    if (linked())
        m.insertItem(TR("Select linked items"), 5);

    m.insertSeparator();

    if (browser_node->is_writable())
        m.insertItem(TR("Delete from model"), 8);

    m.insertSeparator();

    if (Tool::menu_insert(&toolm, UmlExpansionNode, 20))
        m.insertItem(TR("Tool"), &toolm);

    switch (index = m.exec(QCursor::pos())) {
    case 0:
        region->upper();
        modified();	// call package_modified()
        return;

    case 1:
        region->lower();
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

void ExpansionNodeCanvas::apply_shortcut(QString s)
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

void ExpansionNodeCanvas::edit_drawing_settings()
{
    for (;;) {
        ColorSpecVector co(1);

        co[0].set(TR("expansion node color"), &itscolor);

        SettingsDialog dialog(0, &co, FALSE);

        dialog.raise();

        if (dialog.exec() == QDialog::Accepted)
            modified();	// call package_modified()

        if (!dialog.redo())
            break;
    }
}

bool ExpansionNodeCanvas::has_drawing_settings() const
{
    return TRUE;
}

void ExpansionNodeCanvas::edit_drawing_settings(QList<DiagramItem *> & l)
{
    for (;;) {
        ColorSpecVector co(1);
        UmlColor itscolor;

        co[0].set(TR("expansion node color"), &itscolor);

        SettingsDialog dialog(0, &co, FALSE, TRUE);

        dialog.raise();

        if ((dialog.exec() == QDialog::Accepted) && !co[0].name.isEmpty()) {
            foreach (DiagramItem *item, l) {
                ExpansionNodeCanvas *canvas = (ExpansionNodeCanvas *)item;
                canvas->itscolor = itscolor;
                canvas->modified();	// call package_modified()
            }
        }

        if (!dialog.redo())
            break;
    }
}

void ExpansionNodeCanvas::clone_drawing_settings(const DiagramItem *src)
{
    const ExpansionNodeCanvas * x = (const ExpansionNodeCanvas *) src;
    itscolor = x->itscolor;
    modified();
}

QString ExpansionNodeCanvas::may_start(UmlCode & l) const
{
    switch (l) {
    case UmlAnchor:
        return 0;

    case UmlFlow:
        return ((BrowserExpansionNode *) browser_node)->may_start();

    default:
        // dependency
        return TR("illegal");
    }
}

QString ExpansionNodeCanvas::may_connect(UmlCode & l, const DiagramItem * dest) const
{
    if (l == UmlAnchor)
        return dest->may_start(l);
    else if (dest->get_bn() == 0)
        return TR("illegal");
    else
        return ((BrowserExpansionNode *) browser_node)->may_connect(dest->get_bn());
}

void ExpansionNodeCanvas::modified()
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

void ExpansionNodeCanvas::post_loaded()
{
    if (the_canvas()->must_draw_all_relations())
        draw_all_flows();
}

void ExpansionNodeCanvas::connexion(UmlCode action, DiagramItem * dest,
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

void ExpansionNodeCanvas::save(QTextStream & st, bool ref, QString & warning) const
{
    if (ref) {
        st << "expansionnodecanvas_ref " << get_ident() << " // "
           << browser_node->full_name();
    }
    else {
        nl_indent(st);
        st << "expansionnodecanvas " << get_ident() << " ";
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

ExpansionNodeCanvas * ExpansionNodeCanvas::read(char *& st, UmlCanvas * canvas,
        char * k, ExpansionRegionCanvas * a)
{
    if (!strcmp(k, "expansionnodecanvas_ref"))
        return ((ExpansionNodeCanvas *) dict_get(read_id(st), "expansionnodecanvas", canvas));
    else if (!strcmp(k, "expansionnodecanvas")) {
        int id = read_id(st);
        BrowserExpansionNode * br = BrowserExpansionNode::read_ref(st);
        ExpansionNodeCanvas * result =
            new ExpansionNodeCanvas(br, canvas, 0, 0, id, a);

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

void ExpansionNodeCanvas::history_hide()
{
    Q3CanvasItem::setVisible(FALSE);
    disconnect(browser_node->get_data(), SIGNAL(changed()), this, SLOT(modified()));
    disconnect(browser_node->get_data(), SIGNAL(deleted()), this, SLOT(deleted()));
    disconnect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
}

void ExpansionNodeCanvas::history_save(QBuffer & b) const
{
    DiagramCanvas::history_save(b);
    ::save(width_scale100, b);
    ::save(height_scale100, b);
    ::save(width(), b);
    ::save(height(), b);
}

void ExpansionNodeCanvas::history_load(QBuffer & b)
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
