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
#include <Q3ValueList>

#include "ExpansionRegionCanvas.h"
#include "ExpansionRegionData.h"
#include "BrowserExpansionRegion.h"
#include "BrowserActivityDiagram.h"
#include "BrowserExpansionNode.h"
#include "ExpansionNodeCanvas.h"
#include "ArrowCanvas.h"
#include "UmlPixmap.h"
#include "UmlGlobal.h"
#include "SettingsDialog.h"
#include "myio.h"
#include "ToolCom.h"
#include "Tool.h"
#include "ui/menufactory.h"
#include "Settings.h"
#include "strutil.h"
#include "translate.h"

ExpansionRegionCanvas::ExpansionRegionCanvas(BrowserNode * bn, UmlCanvas * canvas,
        int x, int y)
    : ActivityContainerCanvas(0, canvas, x, y,
                              EXPANSION_REGION_CANVAS_MIN_SIZE * 4,
                              EXPANSION_REGION_CANVAS_MIN_SIZE * 4, 0)
{
    browser_node = bn;
    itscolor = UmlDefaultColor;

    compute_size();
    check_nodes();
    check_stereotypeproperties();

    connect(bn->get_data(), SIGNAL(changed()), this, SLOT(modified()));
    connect(bn->get_data(), SIGNAL(deleted()), this, SLOT(deleted()));
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
}

ExpansionRegionCanvas::ExpansionRegionCanvas(UmlCanvas * canvas, int id)
    : ActivityContainerCanvas(canvas, id)
{
    // for read operation
    browser_node = 0;
    itscolor = UmlDefaultColor;
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
}

ExpansionRegionCanvas::~ExpansionRegionCanvas()
{
}

void ExpansionRegionCanvas::delete_it()
{
    disconnect(DrawingSettings::instance(), SIGNAL(changed()),
               this, SLOT(modified()));
    disconnect(browser_node->get_data(), 0, this, 0);

    DiagramCanvas::delete_it();

    Q3ValueList<ExpansionNodeCanvas *>::Iterator iter;

    for (iter = nodes.begin(); iter != nodes.end(); ++iter)
        // don't empty nodes to manage undo
        (*iter)->delete_it();
}

void ExpansionRegionCanvas::deleted()
{
    delete_it();
    canvas()->update();
}

void ExpansionRegionCanvas::deleted(ExpansionNodeCanvas * n)
{
    nodes.remove(n);
}

void ExpansionRegionCanvas::remove(bool from_model)
{
    if (! from_model)
        delete_it();
    else
        browser_node->delete_it();	// will remove canvas
}

void ExpansionRegionCanvas::compute_size()
{
    used_color = (itscolor == UmlDefaultColor)
                 ? the_canvas()->browser_diagram()->get_color(UmlExpansionRegion)
                 : itscolor;

    QString s;

    switch (((ExpansionRegionData *)
             ((BrowserExpansionRegion *) browser_node)->get_data())
            ->get_mode()) {
    case UmlParallel:
        s = "<<parallel>>";
        break;

    case UmlIterative:
        s = "<<iterative>>";
        break;

    default:
        // UmlStream
        s = "<<stream>>";
        break;
    }

    double zoom = the_canvas()->zoom();
    int margin = (int)(12 * zoom);
    QFontMetrics fm(the_canvas()->get_font(UmlNormalFont));

    min_width = fm.width(s) + margin;

    if (min_width < EXPANSION_REGION_CANVAS_MIN_SIZE * zoom)
        min_width = (int)(EXPANSION_REGION_CANVAS_MIN_SIZE * zoom);

    min_height = fm.height() + margin;

    if (min_height < EXPANSION_REGION_CANVAS_MIN_SIZE * zoom)
        min_height = (int)(EXPANSION_REGION_CANVAS_MIN_SIZE * zoom);

    DiagramCanvas::resize((width() > min_width) ? width() : min_width,
                          (height() > min_height) ? height() : min_height);

    force_nodes_arround();
}

void ExpansionRegionCanvas::moveBy(double dx, double dy)
{
    DiagramCanvas::moveBy(dx, dy);

    // update expansion nodes position
    Q3ValueList<ExpansionNodeCanvas *>::Iterator iter;

    for (iter = nodes.begin(); iter != nodes.end(); ++iter)
        (*iter)->do_moveBy(dx, dy);
}

void ExpansionRegionCanvas::change_scale()
{
    Q3CanvasRectangle::setVisible(FALSE);
    double scale = the_canvas()->zoom();

    setSize((int)(width_scale100 * scale), (int)(height_scale100 * scale));
    compute_size();
    recenter();
    Q3CanvasRectangle::setVisible(TRUE);

    // update expansion nodes position
    Q3ValueList<ExpansionNodeCanvas *>::Iterator iter;

    for (iter = nodes.begin(); iter != nodes.end(); ++iter)
        (*iter)->do_change_scale();
}

void ExpansionRegionCanvas::modified()
{
    hide();
    hide_lines();
    compute_size();
    show();
    update_show_lines();
    check_nodes();
    check_stereotypeproperties();
    canvas()->update();
    force_sub_inside(FALSE);
    package_modified();
}

aCorner ExpansionRegionCanvas::on_resize_point(const QPoint & p)
{
    return ::on_resize_point(p, rect());
}

void ExpansionRegionCanvas::resize(aCorner c, int dx, int dy, QPoint & o)
{
    DiagramCanvas::resize(c, dx, dy, o, min_width, min_height);


    force_nodes_arround();
    force_sub_inside(FALSE);
}

void ExpansionRegionCanvas::resize(const QSize & sz, bool w, bool h)
{
    if (DiagramCanvas::resize(sz, w, h, min_width, min_height)) {
        force_nodes_arround();
        force_sub_inside(FALSE);
    }
}

bool ExpansionRegionCanvas::move_with_its_package() const
{
    return TRUE;
}

void ExpansionRegionCanvas::set_z(double z)
{
    setZ(z);

    z += 1;

    Q3ValueList<ExpansionNodeCanvas *>::Iterator iter;

    for (iter = nodes.begin(); iter != nodes.end(); ++iter)
        (*iter)->set_z(z);
}

void ExpansionRegionCanvas::force_sub_inside(bool resize_it)
{
    // update sub nodes position to be inside of the region
    // except the expansion nodes whose are in the border
    // or resize it to contains sub elts if resize_it
    Q3CanvasItemList all = canvas()->allItems();
    BooL need_sub_upper = FALSE;

    if (resize_it)
        resize_to_contain(all, need_sub_upper);
    else
        ActivityContainerCanvas::force_sub_inside(all, need_sub_upper);

    if (need_sub_upper)
        force_sub_upper(all);
}

void ExpansionRegionCanvas::check_nodes()
{
    // add missing nodes
    const Q3ValueList<BrowserExpansionNode *> brnodes =
        ((BrowserExpansionRegion *) browser_node)->get_nodes();
    Q3ValueList<BrowserExpansionNode *>::ConstIterator iter;
    double zoom = the_canvas()->zoom();
    QRect r = rect();
    int cx = (int)(x() + (width() - zoom * EXPANSIONNODE_WIDTH) / 2);
    int rx = r.right() - (int)(zoom * EXPANSIONNODE_WIDTH);
    int ty = (int)(y() - zoom * EXPANSIONNODE_HEIGHT / 2);
    int by = r.bottom() - (int)(zoom * EXPANSIONNODE_HEIGHT / 2);
    int rank;

    // thanks to the min size of an expansion region cx and rx
    // are valid, ie not ouside

    for (iter = brnodes.begin(), rank = 0; iter != brnodes.end(); ++iter, rank += 1) {
        Q3ValueList<ExpansionNodeCanvas *>::ConstIterator itershown;

        for (itershown = nodes.begin(); itershown != nodes.end(); ++itershown)
            if ((*itershown)->get_bn() == *iter)
                break;

        if (itershown == nodes.end()) {
            // add it
            int px;
            int py;

            switch (rank % 6) {
            case 0:
                px = (int) x();
                py = ty;
                break;

            case 1:
                px = (int) x();
                py = by;
                break;

            case 2:
                px = cx;
                py = ty;
                break;

            case 3:
                px = cx;
                py = by;
                break;

            case 4:
                px = rx;
                py = ty;
                break;

            default:
                px = rx;
                py = by;
            }

            ExpansionNodeCanvas * pc =
                new ExpansionNodeCanvas(*iter, the_canvas(), px, py, 0, this);

            nodes.append(pc);
            pc->show();
        }
    }
}

void ExpansionRegionCanvas::force_nodes_arround()
{
    Q3ValueList<ExpansionNodeCanvas *>::Iterator iter;

    for (iter = nodes.begin(); iter != nodes.end(); ++iter)
        (*iter)->check_position();
}

void ExpansionRegionCanvas::draw(QPainter & p)
{
    if (! visible()) return;

    p.setRenderHint(QPainter::Antialiasing, true);
    QRect r = rect();
    QBrush brsh = p.brush();
    QColor bckgrnd = p.backgroundColor();

    p.setBackgroundMode((used_color == UmlTransparent)
                        ? ::Qt::TransparentMode
                        : ::Qt::OpaqueMode);

    QColor co = color(used_color);

    p.setBackgroundColor(co);

    if (used_color != UmlTransparent)
        p.setBrush(co);

    p.setPen(::Qt::DotLine);
    p.drawRoundRect(r, 8, 8);

    FILE * fp = svg();

    if (fp != 0)
        fprintf(fp, "<g>\n"
                "\t<rect fill=\"%s\" stroke=\"black\" stroke-dasharray=\"4,4\" stroke-opacity=\"1\""
                " x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" rx=\"10\" />\n",
                svg_color(used_color),
                r.left(), r.top(), r.width() - 1, r.height() - 1);

    int margin = (int)(6 * the_canvas()->zoom());
    QString s;

    switch (((ExpansionRegionData *)
             ((BrowserExpansionRegion *) browser_node)->get_data())
            ->get_mode()) {
    case UmlParallel:
        s = "<<parallel>>";
        break;

    case UmlIterative:
        s = "<<iterative>>";
        break;

    default:
        // UmlStream
        s = "<<stream>>";
        break;
    }

    r.setTop(r.top() + margin);
    r.setLeft(r.left() + margin);
    p.setFont(the_canvas()->get_font(UmlNormalFont));
    p.drawText(r, ::Qt::AlignLeft, s);

    if (fp != 0) {
        draw_text(r, ::Qt::AlignLeft, s,
                  p.font(), fp);
        fputs("</g>\n", fp);
    }

    p.setPen(::Qt::SolidLine);
    p.setBackgroundColor(bckgrnd);
    p.setBrush(brsh);

    if (selected())
        show_mark(p, rect());
}

UmlCode ExpansionRegionCanvas::type() const
{
    return UmlExpansionRegion;
}

void ExpansionRegionCanvas::delete_available(BooL & in_model, BooL & out_model) const
{
    out_model |= TRUE;
    in_model |= browser_node->is_writable();
}

bool ExpansionRegionCanvas::alignable() const
{
    return TRUE;
}

bool ExpansionRegionCanvas::copyable() const
{
    return selected();
}

void ExpansionRegionCanvas::open()
{
    browser_node->open(FALSE);
}

void ExpansionRegionCanvas::menu(const QPoint &)
{
    Q3PopupMenu m(0);
    Q3PopupMenu toolm(0);
    int index;

    MenuFactory::createTitle(m, browser_node->get_data()->definition(FALSE, TRUE));
    m.insertSeparator();

    if (browser_node->is_writable()) {
        m.insertItem(TR("Add expansion node"), 9);
        m.insertSeparator();
    }

    m.insertItem(TR("Upper"), 0);
    m.insertItem(TR("Lower"), 1);
    m.insertItem(TR("Go up"), 13);
    m.insertItem(TR("Go down"), 14);
    m.insertSeparator();
    m.insertItem(TR("Edit drawing settings"), 2);
    m.insertSeparator();
    m.insertItem(TR("Edit expansion region"), 3);
    m.insertSeparator();
    m.insertItem(TR("Select in browser"), 4);

    if (linked())
        m.insertItem(TR("Select linked items"), 5);

    m.insertSeparator();

    if (browser_node->is_writable()) {
        if (browser_node->get_associated() !=
            (BrowserNode *) the_canvas()->browser_diagram())
            m.insertItem(TR("Set associated diagram"), 6);

        if (browser_node->get_associated())
            m.insertItem(TR("Remove diagram association"), 10);
    }

    m.insertSeparator();
    m.insertItem(TR("Remove from diagram"), 7);

    if (browser_node->is_writable())
        m.insertItem(TR("Delete from model"), 8);

    m.insertSeparator();

    if (Tool::menu_insert(&toolm, UmlExpansionRegion, 20))
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

    case 6:
        ((BrowserExpansionRegion *) browser_node)
        ->set_associated_diagram((BrowserActivityDiagram *)
                                 the_canvas()->browser_diagram());
        return;

    case 10:
        ((BrowserExpansionRegion *) browser_node)
        ->set_associated_diagram(0);
        return;

    case 7:
        //remove from diagram
        delete_it();
        break;

    case 8:
        //delete from model
        browser_node->delete_it();	// will delete the canvas
        break;

    case 9:
        if (((BrowserExpansionRegion *) browser_node)->add_expansionnode() != 0)
            modified();

        break;

    default:
        if (index >= 20)
            ToolCom::run(Tool::command(index - 20), browser_node);

        return;
    }

    package_modified();
}

void ExpansionRegionCanvas::apply_shortcut(QString s)
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

void ExpansionRegionCanvas::edit_drawing_settings()
{
    for (;;) {
        ColorSpecVector co(1);

        co[0].set(TR("expansion region color"), &itscolor);

        SettingsDialog dialog(0, &co, FALSE);

        dialog.raise();

        if (dialog.exec() == QDialog::Accepted)
            modified();	// call package_modified()

        if (!dialog.redo())
            break;
    }
}

bool ExpansionRegionCanvas::has_drawing_settings() const
{
    return TRUE;
}

void ExpansionRegionCanvas::edit_drawing_settings(QList<DiagramItem *> & l)
{
    for (;;) {
        ColorSpecVector co(1);
        UmlColor itscolor;

        co[0].set(TR("expansion region color"), &itscolor);

        SettingsDialog dialog(0, &co, FALSE, TRUE);

        dialog.raise();

        if ((dialog.exec() == QDialog::Accepted) && !co[0].name.isEmpty()) {
            foreach (DiagramItem *item, l) {
                ExpansionRegionCanvas *canvas = (ExpansionRegionCanvas *)item;
                canvas->itscolor = itscolor;
                canvas->modified();
            }
        }

        if (!dialog.redo())
            break;
    }
}

void ExpansionRegionCanvas::clone_drawing_settings(const DiagramItem *src)
{
    const ExpansionRegionCanvas * x = (const ExpansionRegionCanvas *) src;
    itscolor = x->itscolor;
    modified();
}

QString ExpansionRegionCanvas::may_start(UmlCode & l) const
{
    return (l == UmlAnchor) ? QString() : TR("illegal");
}

QString ExpansionRegionCanvas::may_connect(UmlCode & l, const DiagramItem * dest) const
{
    return (l == UmlAnchor) ? dest->may_start(l) : TR("illegal");
}

void ExpansionRegionCanvas::connexion(UmlCode action, DiagramItem * dest,
                                      const QPoint &, const QPoint &)
{
    ArrowCanvas * a =
        new ArrowCanvas(the_canvas(), this, dest, action, 0, FALSE, -1.0, -1.0);

    a->show();
    the_canvas()->select(a);
}

void ExpansionRegionCanvas::save(QTextStream & st, bool ref, QString & warning) const
{
    if (ref) {
        st << "expansionregioncanvas_ref " << get_ident() << " // "
           << browser_node->full_name();
    }
    else {
        nl_indent(st);
        st << "expansionregioncanvas " << get_ident() << " ";
        browser_node->save(st, TRUE, warning);
        indent(+1);

        if (itscolor != UmlDefaultColor) {
            nl_indent(st);
            st << "color " << stringify(itscolor);
        }

        nl_indent(st);
        save_xyzwh(st, this, "xyzwh");

        if (! nodes.isEmpty()) {
            nl_indent(st);
            st << "nodes";
            indent(+1);

            Q3ValueList<ExpansionNodeCanvas *>::ConstIterator iter;

            for (iter = nodes.begin(); iter != nodes.end(); ++iter)
                (*iter)->save(st, FALSE, warning);

            indent(-1);
            nl_indent(st);
            st << "end";
        }

        save_stereotype_property(st, warning);

        indent(-1);
        nl_indent(st);
        st << "end";
    }
}

ExpansionRegionCanvas *
ExpansionRegionCanvas::read(char *& st, UmlCanvas * canvas,
                            char * k)
{
    if (!strcmp(k, "expansionregioncanvas_ref"))
        return ((ExpansionRegionCanvas *) dict_get(read_id(st), "expansionregioncanvas", canvas));
    else if (!strcmp(k, "expansionregioncanvas")) {
        int id = read_id(st);
        BrowserExpansionRegion * br = BrowserExpansionRegion::read_ref(st);
        ExpansionRegionCanvas * result = new ExpansionRegionCanvas(canvas, id);

        result->browser_node = br;
        connect(br->get_data(), SIGNAL(changed()), result, SLOT(modified()));
        connect(br->get_data(), SIGNAL(deleted()), result, SLOT(deleted()));

        k = read_keyword(st);

        read_color(st, "color", result->itscolor, k);	// updates k

        if (!strcmp(k, "xyzwh"))
            read_xyzwh(st, result);
        else
            wrong_keyword(k, "xyzwh");

        result->compute_size();
        result->set_center100();
        result->show();

        k = read_keyword(st);

        if (! strcmp(k, "nodes")) {
            while (strcmp(k = read_keyword(st), "end") != 0)
                result->nodes.append(ExpansionNodeCanvas::read(st, canvas, k, result));

            k = read_keyword(st);
        }

        result->read_stereotype_property(st, k);	// updates k

        if (strcmp(k, "end"))
            wrong_keyword(k, "end");

        result->check_nodes();
        result->check_stereotypeproperties();

        // result->force_sub_inside() useless

        if (canvas->paste())
            result->remove_if_already_present();

        return result;
    }
    else
        return 0;
}

void ExpansionRegionCanvas::history_save(QBuffer & b) const
{
    DiagramCanvas::history_save(b);
    ::save(min_width, b);
    ::save(min_height, b);
    ::save(width_scale100, b);
    ::save(height_scale100, b);
    ::save(width(), b);
    ::save(height(), b);
}

void ExpansionRegionCanvas::history_load(QBuffer & b)
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

void ExpansionRegionCanvas::history_hide()
{
    Q3CanvasItem::setVisible(FALSE);

    disconnect(DrawingSettings::instance(), SIGNAL(changed()),
               this, SLOT(modified()));
    disconnect(browser_node->get_data(), 0, this, 0);
}
