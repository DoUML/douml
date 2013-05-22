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
//Added by qt3to4:
#include <QTextStream>

#include "DiagramCanvas.h"
#include "BrowserDiagram.h"
#include "ArrowCanvas.h"
#include "LabelCanvas.h"
#include "SimpleRelationCanvas.h"
#include "SimpleRelationData.h"
#include "FlowCanvas.h"
#include "FlowData.h"
#include "TransitionCanvas.h"
#include "TransitionData.h"
#include "StereotypePropertiesCanvas.h"
#include "UmlWindow.h"
#include "Settings.h"
#include "UmlGlobal.h"
#include "myio.h"

DiagramCanvas::DiagramCanvas(BrowserNode * bn, UmlCanvas * canvas,
                             int x, int y, int wi, int he, int id)
    : Q3CanvasRectangle(canvas), DiagramItem(id, canvas),
      browser_node(bn), label(0), stereotypeproperties(0)
{
    double zoom = canvas->zoom();

    if (wi >= 0) {
        width_scale100 = wi;
        wi  = (int)(wi * zoom + 0.5);
    }
    else {
        wi = -wi;
        width_scale100 = wi;
    }

    if (he >= 0) {
        height_scale100 = he;
        he = (int)(he * zoom + 0.5);
    }
    else {
        he = -he;
        height_scale100 = he;
    }

    setSize(wi, he);
    setX(x);
    setY(y);

    QPoint c = center();

    center_x_scale100 = (int)(c.x() / zoom + 0.5);
    center_y_scale100 = (int)(c.y() / zoom + 0.5);

    if (bn != 0) {
        // must be created after setX/Y whose call moveBy
        QFontMetrics fm(canvas->get_font(UmlNormalFont));

        label = new LabelCanvas(bn->get_name(), canvas,
                                x + (wi - fm.width(bn->get_name())) / 2,
                                y + he);
    }

    setZ(DIAGRAMCANVAS_Z);

    if (canvas->paste())
        canvas->select(this);
}

DiagramCanvas::DiagramCanvas(UmlCanvas * canvas, int id)
    : Q3CanvasRectangle(canvas), DiagramItem(id, canvas),
      browser_node(0), label(0), stereotypeproperties(0)
{

    if (canvas->paste())
        canvas->select(this);
}

DiagramCanvas::~DiagramCanvas()
{
}

void DiagramCanvas::delete_it()
{
    while (! lines.isEmpty())
        lines.first()->delete_it();	// will remove the line

    if (label != 0)
        ((UmlCanvas *) canvas())->del(label);

    if (stereotypeproperties != 0)
        ((UmlCanvas *) canvas())->del(stereotypeproperties);

    ((UmlCanvas *) canvas())->del(this);
}

BrowserNode * DiagramCanvas::get_bn() const
{
    return browser_node;
}

UmlCanvas * DiagramCanvas::the_canvas() const
{
    return (UmlCanvas *) canvas();
}

void DiagramCanvas::setVisible(bool yes)
{
    Q3CanvasRectangle::setVisible(yes);

    if (label)
        label->setVisible(yes);

    if (stereotypeproperties)
        stereotypeproperties->setVisible(yes);
}

void DiagramCanvas::change_scale()
{
    double scale = the_canvas()->zoom();

    Q3CanvasRectangle::setVisible(FALSE);
    setSize((int)(width_scale100 * scale + 0.5), (int)(height_scale100 * scale + 0.5));
    recenter();
    Q3CanvasRectangle::setVisible(TRUE);
}

void DiagramCanvas::recenter()
{
    double scale = the_canvas()->zoom();
    QPoint c = center();

    Q3CanvasRectangle::moveBy(center_x_scale100 * scale - c.x(),
                              center_y_scale100 * scale - c.y());
}

void DiagramCanvas::set_center100()
{
    QPoint c = center();
    double scale = the_canvas()->zoom();

    center_x_scale100  = (int)(c.x() / scale + 0.5);
    center_y_scale100  = (int)(c.y() / scale + 0.5);
}

void DiagramCanvas::moveBy(double dx, double dy)
{
    hide_lines();

    Q3CanvasRectangle::moveBy(dx, dy);

    if (!((UmlCanvas *) canvas())->do_zoom())
        set_center100();

    if ((label != 0) && !label->selected())
        label->moveBy(dx, dy);

    if ((stereotypeproperties != 0) && !stereotypeproperties->selected())
        stereotypeproperties->moveBy(dx, dy);

    update_show_lines();
}

void DiagramCanvas::moveSelfRelsBy(double dx, double dy)
{
    foreach (ArrowCanvas *canvas, lines)
        if (canvas->get_start() == canvas->get_end())
            // self relation
            canvas->move_self_points(dx, dy); // move only if first segment => not done 2 times
}

void DiagramCanvas::setSelected(bool yes)
{
    if (browser_node) {
        if (yes)
            UmlWindow::set_commented(browser_node);
        else
            UmlWindow::set_commented(0);
    }

    Q3CanvasRectangle::setSelected(yes);
}

void DiagramCanvas::select_associated()
{
    if (!selected()) {
        the_canvas()->select(this);

        if ((label != 0) && !label->selected())
            the_canvas()->select(label);

        if ((stereotypeproperties != 0) && !stereotypeproperties->selected())
            the_canvas()->select(stereotypeproperties);

        DiagramItem::select_associated();
    }
}

QPoint DiagramCanvas::center() const
{
    return QPoint((int)(x() + (width() - 1) / 2.0),
                  (int)(y() + (height() - 1) / 2.0));
}

QRect DiagramCanvas::rect() const
{
    return Q3CanvasRectangle::rect();
}

bool DiagramCanvas::isSelected() const
{
    return selected();
}

bool DiagramCanvas::contains(int x, int y) const
{
    return rect().contains(x, y);
}

void DiagramCanvas::connexion(UmlCode action, DiagramItem * dest,
                              const QPoint &, const QPoint &)
{
    ArrowCanvas * a =
        new ArrowCanvas(the_canvas(), this, dest, action, 0, FALSE, -1.0, -1.0);

    a->show();
    the_canvas()->select(a);
}

void DiagramCanvas::resize(int wi, int he)
{
    setSize(wi, he);

    if (!((UmlCanvas *) canvas())->do_zoom()) {
        double zoom = the_canvas()->zoom();

        width_scale100 = (int)(wi / zoom + 0.5);
        height_scale100 = (int)(he / zoom + 0.5);

        QPoint c = center();

        center_x_scale100  = (int)(c.x() / zoom + 0.5);
        center_y_scale100  = (int)(c.y() / zoom + 0.5);
    }
}

// min_width and min_height must take into account the current zoom
// some element like parameter stay centered to their old position
void DiagramCanvas::resize(aCorner c, int dx, int dy, QPoint & o,
                           int min_width, int min_height,
                           bool odd, bool stay_centered)
{
    hide_lines();

    switch (c) {
    case UmlTopLeft:
        Q3CanvasRectangle::moveBy(dx, dy);
        moveSelfRelsBy(dx, dy);
        dx = -dx;
        dy = -dy;
        break;

    case UmlTopRight:
        if (stay_centered) {
            Q3CanvasRectangle::moveBy(-dx, dy);
            moveSelfRelsBy(-dx, dy);
        }
        else {
            Q3CanvasRectangle::moveBy(0, dy);
            moveSelfRelsBy(0, dy);
        }

        dy = -dy;
        break;

    case UmlBottomLeft:
        if (stay_centered) {
            Q3CanvasRectangle::moveBy(dx, -dy);
            moveSelfRelsBy(dx, -dy);
        }
        else {
            Q3CanvasRectangle::moveBy(dx, 0);
            moveSelfRelsBy(dx, 0);
        }

        dx = -dx;
        break;

    default:
        if (stay_centered) {
            Q3CanvasRectangle::moveBy(-dx, -dy);
            moveSelfRelsBy(-dx, -dy);
        }

        break;
    }

    if (stay_centered) {
        dx += dx;
        dy += dy;
    }

    int wi = width() + dx;
    int he = height() + dy;

    if (wi < min_width)
        wi = min_width;

    if (he < min_height)
        he = min_height;

    if (odd) {
        if (!(wi & 1)) {
            if (o.x() != 0) {
                wi -= 1;
                o.setX(0);
            }
            else {
                wi += 1;
                o.setX(1);
            }
        }

        if (!(he & 1)) {
            if (o.y() != 0) {
                he -= 1;
                o.setY(0);
            }
            else {
                he += 1;
                o.setY(1);
            }
        }
    }

    resize(wi, he);
    update_show_lines();
    force_self_rel_visible();

    canvas()->update();
}

// min_width and min_height must take into account the current zoom
// some element like parameter stay centered to their old position
bool DiagramCanvas::resize(const QSize & sz, bool w, bool h,
                           int min_width, int min_height,
                           bool odd, bool stay_centered)
{
    int wi = (w) ? sz.width() : width();
    int he = (h) ? sz.height() : height();

    if (odd) {
        wi |= 1;
        he |= 1;
    }

    if (wi < min_width)
        wi = min_width;

    if (he < min_height)
        he = min_height;

    if ((wi == width()) && (he == height()))
        return FALSE;

    hide_lines();

    if (stay_centered) {
        int xc = center_x_scale100;
        int yc = center_y_scale100;

        resize(wi, he);
        center_x_scale100 = xc;
        center_y_scale100 = yc;
        recenter();
    }
    else
        resize(wi, he);

    update_show_lines();
    force_self_rel_visible();

    canvas()->update();

    return TRUE;
}

void DiagramCanvas::prepare_for_move(bool on_resize)
{
    if (! on_resize) {
        // select self relations
        foreach (ArrowCanvas *canvas, lines) {
            if (canvas->get_start() == canvas->get_end())
                // note : selected 2 times (from the start and the end)
                canvas->select_associated();
        }
    }
}

bool DiagramCanvas::move_with(UmlCode k) const
{
    switch (k) {
    case UmlSubject:
    case UmlFragment:
        return (the_canvas()->browser_diagram()->get_type() == UmlUseCaseDiagram);

    default:
        return FALSE;
    }
}

void DiagramCanvas::force_self_rel_visible()
{
    QRect r = rect();

    // add a marging
    r.setLeft(r.left() - 5);
    r.setRight(r.right() + 5);
    r.setTop(r.top() - 5);
    r.setBottom(r.bottom() + 5);

    foreach (ArrowCanvas *canvas, lines) {
        if (canvas->get_start() == canvas->get_end())
            // self relation
            canvas->move_outside(r);
    }
}

void DiagramCanvas::upper()
{
    double max_z = 0;
    Q3CanvasItemList l = collisions(FALSE);
    Q3CanvasItemList::Iterator it;

    for (it = l.begin(); it != l.end(); it++)
        if (!isa_alien(*it) &&
            (*it != this) &&
            (*it)->visible() &&
            ((*it)->z() > max_z))
            max_z = (*it)->z();

    if ((z() - max_z) < 5) {
        double nz = ((int) max_z) + 5;	// (int) to manage Template & label

        set_z(nz);

        nz += 1;

        foreach (ArrowCanvas *canvas, lines) {
            canvas->go_up(nz);
        }
    }
}

void DiagramCanvas::lower()
{
    Q3CanvasItemList l = collisions(FALSE);
    Q3CanvasItemList::Iterator it;
    double min_z = 1e100;

    for (it = l.begin(); it != l.end(); it++)
        if ((QCanvasItemToDiagramItem(*it) != 0) &&
            (*it != this) &&
            (*it)->visible() &&
            ((*it)->z() < min_z))
            min_z = (*it)->z();

    if (z() < min_z)
        // already ok
        ;
    else if (min_z > 15)
        set_z(((int) min_z) - 5);	// (int) to manage Template & labels
    else {
        // other canvas go up
        double incr = ((int) z()) + 5;

        for (it = l.begin(); it != l.end(); it++) {
            DiagramItem * di = QCanvasItemToDiagramItem(*it);

            if ((di != 0) && (*it != this))
                di->set_z((*it)->z() + incr);
        }
    }
}

void DiagramCanvas::z_up()
{
    double next_z = 2e100;
    Q3CanvasItemList l = collisions(FALSE);
    Q3CanvasItemList::Iterator it;

    // search for the lowest z > this->z
    for (it = l.begin(); it != l.end(); it++) {
        if (!isa_alien(*it) &&
            (*it)->visible() &&
            ((*it)->z() > z()) &&
            ((*it)->z() < next_z)) {
            DiagramCanvas * e = QCanvasItemToDiagramCanvas(*it);

            if ((e != 0) && e->primaryItem())
                next_z = (*it)->z();
        }
    }

    if (next_z != 2e100) {
        // not at top, echange z with elements at next_z
        for (it = l.begin(); it != l.end(); it++) {
            if (!isa_alien(*it) &&
                (*it)->visible() &&
                ((*it)->z() == next_z)) {
                DiagramCanvas * e = QCanvasItemToDiagramCanvas(*it);

                if ((e != 0) && e->primaryItem())
                    e->set_z(z());
            }
        }

        set_z(next_z);

        foreach (ArrowCanvas *canvas, lines)
            canvas->go_up(next_z);
    }
}

void DiagramCanvas::z_down()
{
    double next_z = 0;
    Q3CanvasItemList l = collisions(FALSE);
    Q3CanvasItemList::Iterator it;

    // search for the highest z < this->z
    for (it = l.begin(); it != l.end(); it++) {
        if (!isa_alien(*it) &&
            (*it)->visible() &&
            ((*it)->z() < z()) &&
            ((*it)->z() > next_z)) {
            DiagramCanvas * e = QCanvasItemToDiagramCanvas(*it);

            if ((e != 0) && e->primaryItem())
                next_z = (*it)->z();
        }
    }

    if (next_z != 2e100) {
        // not at bottom, echange z with elements at next_z
        for (it = l.begin(); it != l.end(); it++) {
            if (!isa_alien(*it) &&
                (*it)->visible() &&
                ((*it)->z() == next_z)) {
                DiagramCanvas * e = QCanvasItemToDiagramCanvas(*it);

                if ((e != 0) && e->primaryItem())
                    e->set_z(z());
            }
        }

        set_z(next_z);
    }
}

bool DiagramCanvas::primaryItem() const
{
    return TRUE;
}

double DiagramCanvas::get_z() const
{
    return z();
}

void DiagramCanvas::set_z(double z)
{
    setZ(z);

    if (label != 0)
        label->setZ(z + 0.5);
}

void DiagramCanvas::package_modified() const
{
    the_canvas()->browser_diagram()->package_modified();
}

void DiagramCanvas::draw_actor(QPainter * p, QRect r)
{
    // the 'original' size is 40 x 40
    double scale = r.width() / 40.0;
    int i, j, k, l;

    p->setBackgroundMode(::Qt::TransparentMode);

    // head
    i = (int)(13 * scale);
    p->drawEllipse(r.left() + (int)(13 * scale + 0.5),
                   r.top(), i, i);

    // arm
    j = r.top() + (int)(18 * scale);
    p->drawLine(r.left() + (int)(8 * scale), j,
                r.left() + (int)(30 * scale), j);

    // body
    k = r.left() + (int)(19 * scale);
    l = r.top() + (int)(28 * scale);
    p->drawLine(k, r.top() + (int)(13 * scale),
                k, l);

    // legs
    p->drawLine(k, l, r.left() + (int)(9 * scale), r.bottom());
    p->drawLine(k, l, r.left() + (int)(29 * scale), r.bottom());

    FILE * fp = svg();

    if (fp != 0) {
        int radius = (int)(6.5 * scale);

        // head
        fprintf(fp, "\t<ellipse fill=\"none\" stroke=\"black\" stroke-width=\"1\" stroke-opacity=\"1\""
                " cx=\"%d\" cy=\"%d\" rx=\"%d\" ry=\"%d\" />\n",
                k, r.top() + radius, radius, radius);

        // arm
        fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                r.left() + (int)(8 * scale), j,
                r.left() + (int)(30 * scale), j);

        // body
        fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                k, r.top() + (int)(13 * scale),
                k, l);

        // legs
        fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                k, l, r.left() + (int)(9 * scale), r.bottom());
        fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                k, l, r.left() + (int)(29 * scale), r.bottom());
    }
}

void DiagramCanvas::draw_control_icon(QPainter & p, QRect & r,
                                      UmlColor used_color, double zoom)
{
    int sz = (int)(CONTROL_WIDTH * zoom);
    int cx = (r.left() + r.right()) / 2;
    int dv = (int)((CONTROL_HEIGHT - CONTROL_WIDTH) * zoom);
    int cy = r.top() + dv;
    QBrush brsh = p.brush();
    QColor co = color(used_color);

    p.setBrush(co);

    p.drawEllipse(cx - sz / 2, cy, sz, sz);
    p.drawLine(cx, cy, cx + dv, cy - dv);
    p.drawLine(cx, cy, cx + dv, cy + dv);

    p.setBrush(brsh);

    FILE * fp = svg();

    if (fp != 0) {
        sz /= 2;
        fprintf(fp, "\t<ellipse fill=\"%s\" stroke=\"black\" stroke-width=\"1\" stroke-opacity=\"1\""
                " cx=\"%d\" cy=\"%d\" rx=\"%d\" ry=\"%d\" />\n",
                svg_color(used_color),
                cx, cy + sz, sz, sz);
        fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                cx, cy, cx + dv, cy - dv);
        fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                cx, cy, cx + dv, cy + dv);
    }
}

void DiagramCanvas::draw_boundary_icon(QPainter & p, QRect & r,
                                       UmlColor used_color, double zoom)
{
    int he = (int)(BOUNDARY_HEIGHT * zoom);
    int cx = (r.left() + r.right() - he) / 2;
    int cy = r.top() + he / 2;
    QBrush brsh = p.brush();
    QColor co = color(used_color);

    p.setBrush(co);

    p.drawEllipse(cx, r.top(), he, he);

    int wi = he / 2 - 2;

    p.drawLine(cx - wi, cy, cx, cy);
    p.drawLine(cx - wi, r.top(), cx - wi, r.top() + he);

    p.setBrush(brsh);

    FILE * fp = svg();

    if (fp != 0) {
        fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                cx - wi, cy, cx, cy);
        fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                cx - wi, r.top(), cx - wi, r.top() + he);
        he /= 2;
        fprintf(fp, "\t<ellipse fill=\"%s\" stroke=\"black\" stroke-width=\"1\" stroke-opacity=\"1\""
                " cx=\"%d\" cy=\"%d\" rx=\"%d\" ry=\"%d\" />\n",
                svg_color(used_color),
                (r.left() + r.right()) / 2, r.top() + he, he, he);
    }
}

void DiagramCanvas::draw_entity_icon(QPainter & p, QRect & r,
                                     UmlColor used_color, double zoom)
{
    int sz = (int)(ENTITY_SIZE * zoom);
    int left = (r.left() + r.right() - sz) / 2;
    QBrush brsh = p.brush();
    QColor co = color(used_color);

    p.setBrush(co);

    p.drawEllipse(left, r.top(), sz, sz);
    p.drawLine(left, r.top() + sz, left + sz, r.top() + sz);

    p.setBrush(brsh);

    FILE * fp = svg();

    if (fp != 0) {
        fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                left, r.top() + sz, left + sz, r.top() + sz);
        sz /= 2;
        fprintf(fp, "\t<ellipse fill=\"%s\" stroke=\"black\" stroke-width=\"1\" stroke-opacity=\"1\""
                " cx=\"%d\" cy=\"%d\" rx=\"%d\" ry=\"%d\" />\n",
                svg_color(used_color),
                (r.left() + r.right()) / 2, r.top() + sz, sz, sz);
    }
}

void DiagramCanvas::draw_interface_icon(QPainter & p, QRect & r,
                                        UmlColor used_color, double zoom)
{
    int sz = (int)(INTERFACE_SIZE * zoom);
    int left = (r.left() + r.right() - sz) / 2;
    QBrush brsh = p.brush();
    QColor co = color(used_color);

    p.setBrush(co);

    p.drawEllipse(left, r.top(), sz, sz);

    p.setBrush(brsh);

    FILE * fp = svg();

    if (fp != 0) {
        sz /= 2;
        fprintf(fp, "\t<ellipse fill=\"%s\" stroke=\"black\" stroke-width=\"1\" stroke-opacity=\"1\""
                " cx=\"%d\" cy=\"%d\" rx=\"%d\" ry=\"%d\" />\n",
                svg_color(used_color),
                (r.left() + r.right()) / 2, r.top() + sz, sz, sz);
    }
}

double DiagramCanvas::compute_angle(double delta_x, double delta_y)
{
    if (delta_x == 0)
        return (delta_y > 0) ? 90 : 270;
    else {
        double angle = atan(delta_y / delta_x) * (180 / 3.1415927);	// -90 .. 90

        if (delta_x < 0)
            angle += 180;
        else if (angle < 0)
            angle += 360;

        return angle;
    }
}

bool DiagramCanvas::has_simple_relation(BasicData * def) const
{
    foreach (ArrowCanvas *canvas, lines) {
        if (IsaSimpleRelation(canvas->type()) &&
            (((SimpleRelationCanvas *) canvas)->get_data() == def))
            return TRUE;
    }

    return FALSE;
}

void DiagramCanvas::draw_all_simple_relations(DiagramCanvas * end)
{
    Q3ListViewItem * child;
    Q3CanvasItemList all = canvas()->allItems();
    Q3CanvasItemList::Iterator cit;

    for (child = browser_node->firstChild(); child; child = child->nextSibling()) {
        if (IsaSimpleRelation(((BrowserNode *) child)->get_type()) &&
            !((BrowserNode *) child)->deletedp()) {
            SimpleRelationData * def =
                ((SimpleRelationData *)((BrowserNode *) child)->get_data());

            if ((def->get_start_node() == browser_node) && 	// rel begins by this
                ((end == 0) || (def->get_end_node() == end->browser_node)) &&
                !has_simple_relation(def)) {
                // adds it in case the target is drawn
                BrowserNode * end_node = def->get_end_node();
                DiagramItem * di;

                if (end_node == browser_node)
                    di = this;
                else {
                    di = 0;

                    for (cit = all.begin(); cit != all.end(); ++cit) {
                        DiagramCanvas * dc = QCanvasItemToDiagramCanvas(*cit);

                        if ((dc != 0) &&
                            (dc->browser_node == end_node) &&
                            ((dc == end) || (*cit)->visible())) {
                            // other diagram canvas find
                            di = dc;
                            break;
                        }
                    }
                }

                if (di != 0)
                    (new SimpleRelationCanvas(the_canvas(), this, di,
                                              (BrowserNode *) child,
                                              def->get_type(), 0,
                                              -1.0, -1.0, def))->show();
            }
        }
    }

    if ((end == 0) &&
        !DrawingSettings::just_modified() &&
        !on_load_diagram()) {
        for (cit = all.begin(); cit != all.end(); ++cit) {
            DiagramCanvas * dc = QCanvasItemToDiagramCanvas(*cit);

            if ((dc != 0) &&
                (dc != this) &&
                (dc->browser_node != 0) &&
                !dc->browser_node->deletedp() &&
                dc->visible())
                dc->draw_all_simple_relations(this);
        }
    }
}

bool DiagramCanvas::has_flow(BasicData * def) const
{
    foreach (ArrowCanvas *canvas, lines) {
        if ((canvas->type() == UmlFlow) &&
            (((FlowCanvas *) canvas)->get_data() == def))
            return TRUE;
    }

    return FALSE;
}

void DiagramCanvas::draw_all_flows(DiagramCanvas * end)
{
    Q3ListViewItem * child;
    Q3CanvasItemList all = canvas()->allItems();
    Q3CanvasItemList::Iterator cit;

    for (child = browser_node->firstChild(); child; child = child->nextSibling()) {
        if ((((BrowserNode *) child)->get_type() == UmlFlow) &&
            !((BrowserNode *) child)->deletedp()) {
            FlowData * def =
                ((FlowData *)((BrowserNode *) child)->get_data());

            if ((def->get_start_node() == browser_node) && 	// rel begins by this
                ((end == 0) || (def->get_end_node() == end->browser_node)) &&
                !has_flow(def)) {
                // adds it in case the target is drawn
                BrowserNode * end_node = def->get_end_node();
                DiagramItem * di;

                if (end_node == browser_node)
                    di = this;
                else {
                    di = 0;

                    for (cit = all.begin(); cit != all.end(); ++cit) {
                        DiagramCanvas * dc = QCanvasItemToDiagramCanvas(*cit);

                        if ((dc != 0) &&
                            (dc->browser_node == end_node) &&
                            ((dc == end) || (*cit)->visible())) {
                            // other diagram canvas find
                            di = dc;
                            break;
                        }
                    }
                }

                if (di != 0)
                    (new FlowCanvas(the_canvas(), this, di,
                                    (BrowserNode *) child,
                                    0, -1.0, -1.0, def))->show();
            }
        }
    }

    if ((end == 0) &&
        !DrawingSettings::just_modified() &&
        !on_load_diagram()) {
        for (cit = all.begin(); cit != all.end(); ++cit) {
            DiagramCanvas * dc = QCanvasItemToDiagramCanvas(*cit);

            if ((dc != 0) &&
                (dc != this) &&
                (dc->browser_node != 0) &&
                !dc->browser_node->deletedp() &&
                dc->visible())
                dc->draw_all_flows(this);
        }
    }
}

bool DiagramCanvas::has_transition(BasicData * def) const
{
    foreach (ArrowCanvas *canvas, lines) {
        if ((canvas->type() == UmlTransition) &&
            (((TransitionCanvas *) canvas)->get_data() == def))
            return TRUE;
    }

    return FALSE;
}

void DiagramCanvas::draw_all_transitions(DiagramCanvas * end)
{
    Q3ListViewItem * child;
    Q3CanvasItemList all = canvas()->allItems();
    Q3CanvasItemList::Iterator cit;

    for (child = browser_node->firstChild(); child; child = child->nextSibling()) {
        if ((((BrowserNode *) child)->get_type() == UmlTransition) &&
            !((BrowserNode *) child)->deletedp()) {
            TransitionData * def =
                ((TransitionData *)((BrowserNode *) child)->get_data());

            if ((def->get_start_node() == browser_node) && 	// rel begins by this
                ((end == 0) || (def->get_end_node() == end->browser_node)) &&
                !has_transition(def)) {
                // adds it in case the target is drawn
                BrowserNode * end_node = def->get_end_node();
                DiagramItem * di;

                if (end_node == browser_node)
                    di = this;
                else {
                    di = 0;

                    for (cit = all.begin(); cit != all.end(); ++cit) {
                        DiagramCanvas * dc = QCanvasItemToDiagramCanvas(*cit);

                        if ((dc != 0) &&
                            (dc->browser_node == end_node) &&
                            ((dc == end) || (*cit)->visible())) {
                            // other diagram canvas find
                            di = dc;
                            break;
                        }
                    }
                }

                if (di != 0)
                    (new TransitionCanvas(the_canvas(), this, di,
                                          (BrowserNode *) child,
                                          0, -1.0, -1.0, def))->show();
            }
        }
    }

    if ((end == 0) &&
        !DrawingSettings::just_modified() &&
        !on_load_diagram()) {
        for (cit = all.begin(); cit != all.end(); ++cit) {
            DiagramCanvas * dc = QCanvasItemToDiagramCanvas(*cit);

            if ((dc != 0) &&
                (dc != this) &&
                (dc->browser_node != 0) &&
                !dc->browser_node->deletedp() &&
                dc->visible())
                dc->draw_all_transitions(this);
        }
    }
}


//

// redefined on element having drawing setting for stereotype properties
bool DiagramCanvas::get_show_stereotype_properties() const
{
    return the_canvas()->browser_diagram()->get_show_stereotype_properties();
}

void DiagramCanvas::check_stereotypeproperties()
{
    if (browser_node != 0) {
        QString s = browser_node->stereotypes_properties();

        if (!s.isEmpty() && get_show_stereotype_properties())
            StereotypePropertiesCanvas::needed(the_canvas(), this, s,
                                               stereotypeproperties, center());
        else if (stereotypeproperties != 0) {
            stereotypeproperties->delete_it();
            stereotypeproperties = 0;
        }
    }
}

void DiagramCanvas::save_stereotype_property(QTextStream & st, QString & warning) const
{
    if (stereotypeproperties != 0)
        stereotypeproperties->save(st, FALSE, warning);
}

void DiagramCanvas::read_stereotype_property(char *& st, char *& k)
{
    stereotypeproperties =
        StereotypePropertiesCanvas::read(st, the_canvas(), k, this);

    if (stereotypeproperties != 0)
        k = read_keyword(st);
}

//

void DiagramCanvas::history_save(QBuffer & b) const
{
    ::save(this, b);
    ::save(center_x_scale100, b);
    ::save(center_y_scale100, b);
    ::save(x(), b);
    ::save(y(), b);
    ::save(z(), b);
}

void DiagramCanvas::history_load(QBuffer & b)
{
    ::load(center_x_scale100, b);
    ::load(center_y_scale100, b);

    double dx = load_double(b) - x();

    Q3CanvasRectangle::moveBy(dx, load_double(b) - y());
    Q3CanvasRectangle::setZ(load_double(b));

    Q3CanvasItem::setSelected(FALSE);
    Q3CanvasItem::setVisible(TRUE);
}

void DiagramCanvas::history_hide()
{
    Q3CanvasItem::setVisible(FALSE);
}
