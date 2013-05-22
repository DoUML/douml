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
#include <q3popupmenu.h>
#include <qcursor.h>
#include <qpainter.h>
//Added by qt3to4:
#include <QTextStream>

#include <QPixmap>

#include "ArrowCanvas.h"
#include "ArrowPointCanvas.h"
#include "ArrowJunctionCanvas.h"
#include "UmlCanvas.h"
#include "LabelCanvas.h"
#include "CdClassCanvas.h"
#include "TransitionData.h"
#include "BrowserDiagram.h"
#include "StereotypeDialog.h"
#include "DialogUtil.h"
#include "myio.h"
#include "ui/menufactory.h"
#include "DiagramView.h"
#include "UmlPixmap.h"
#include "ToolCom.h"
#include "strutil.h"
#include "translate.h"

#include "geometry_hv.xpm"
#include "geometry_vh.xpm"
#include "geometry_hvh.xpm"
#include "geometry_vhv.xpm"

ArrowCanvas::ArrowCanvas(UmlCanvas * canvas, DiagramItem * b,
                         DiagramItem * e, UmlCode t,
                         int id, bool own_brk, float dbegin, float dend)
    : Q3CanvasPolygon(canvas), DiagramItem(id, canvas), begin(b), end(e),
      itstype(t), geometry(NoGeometry), fixed_geometry(FALSE),
      label(0), stereotype(0), decenter_begin(dbegin), decenter_end(dend)
{
    boundings.resize(4);

    double bz = begin->get_z();
    double ez = end->get_z();

    setZ(((bz > ez) ? bz : ez) + 1);
    b->add_line(this);
    e->add_line(this);

    // manages the case start == end
    if ((b == e) && !own_brk)
        cut_self();

    // the first time the canvas must not have a label nor stereotype
    auto_pos = canvas->browser_diagram()->get_auto_label_position();
    update_pos();

    connect(DrawingSettings::instance(), SIGNAL(changed()),
            this, SLOT(drawing_settings_modified()));
}

ArrowCanvas::~ArrowCanvas()
{
}

void ArrowCanvas::cut_self()
{
    // cut the line adding two points
    QRect r = ((DiagramCanvas *) begin)->rect();
    QPoint pt(r.left() - ARROW_LENGTH * 3, r.top());

    ArrowPointCanvas * ap = brk(pt);

    pt.setY(pt.y() + ARROW_LENGTH * 3);
    ap->get_other(this)->brk(pt)->update_show_lines();
    ap->update_show_lines();
}

void ArrowCanvas::delete_it()
{
    unconnect();

    if (begin != 0) {
        begin->remove_line(this);

        if (begin->type() == UmlArrowPoint)
            begin->delete_it();
    }

    if (end != 0) {
        end->remove_line(this);

        if (end->type() == UmlArrowPoint)
            end->delete_it();
    }

    if (label != 0)
        the_canvas()->del(label);

    if (stereotype != 0)
        the_canvas()->del(stereotype);

    while (!lines.isEmpty())
        lines.first()->delete_it();	// will remove the line

    the_canvas()->del(this);
}

void ArrowCanvas::unconnect()
{
    disconnect(DrawingSettings::instance(), SIGNAL(changed()),
               this, SLOT(drawing_settings_modified()));
}

UmlCanvas * ArrowCanvas::the_canvas() const
{
    return ((UmlCanvas *) canvas());
}

static int iabs(int v)
{
    return (v >= 0) ? v : -v;
}

void ArrowCanvas::update_pos()
{
    hide_lines();

    QPoint old_beginp = beginp;
    QPoint old_endp = endp;
    QPoint end_computed;
    bool need_update_geometry = FALSE;

    // calcul beginp & endp pour ne plus etre dans b_rct & e_rct

    if (begin->type() == UmlArrowPoint)
        beginp = begin->center();
    else {
        QRect r = begin->rect();

        if (r.contains(end->rect()))
            begin->shift(beginp, end->center(), TRUE);
        else {
            switch (end->type()) {
            case UmlArrowPoint:
            case UmlArrowJunction:
                begin->shift(beginp, end->center(), FALSE);
                break;

            default: {
                QRect endr = end->rect();

                if (endr.contains(r)) {
                    end->shift(end_computed, r.center(), TRUE);
                    begin->shift(beginp, end_computed, FALSE);
                }
                else
                    begin->shift(beginp, endr.center(), FALSE);
            }
            }
        }

        if (decenter_begin >= 0) {
            if ((iabs(beginp.y() - r.top()) < 2) ||
                (iabs(beginp.y() - r.bottom()) < 2)) {
                // on top or botton
                beginp.setX((int)(r.left() + r.width()*decenter_begin));
                need_update_geometry = TRUE;
            }
            else if ((iabs(beginp.x() - r.left()) < 2) ||
                     (iabs(beginp.x() - r.right()) < 2)) {
                // on left or right
                beginp.setY((int)(r.top() + r.height()*decenter_begin));
                need_update_geometry = TRUE;
            }

            // else end point inside 'start'
        }
    }

    switch (end->type()) {
    case UmlArrowPoint:
    case UmlArrowJunction:
        endp = end->center();
        break;

    default: {
        QRect r = end->rect();

        if (end_computed.isNull())
            end->shift(endp, beginp, r.contains(begin->rect()));
        else
            endp = end_computed;

        if (decenter_end >= 0) {
            if ((iabs(endp.y() - r.top()) < 2) ||
                (iabs(endp.y() - r.bottom()) < 2)) {
                // on top or botton
                endp.setX((int)(r.left() + r.width()*decenter_end));
                need_update_geometry = TRUE;
            }
            else if ((iabs(endp.x() - r.left()) < 2) ||
                     (iabs(endp.x() - r.right()) < 2)) {
                // on left or right
                endp.setY((int)(r.top() + r.height()*decenter_end));
                need_update_geometry = TRUE;
            }

            // else start point inside 'end'
        }
    }
    }

    // calcul rectangle englobant

    const int dx = endp.x() - beginp.x();
    const int dy = beginp.y() - endp.y();

    if ((dx == 0) && (dy == 0)) {
        boundings.setPoint(0, beginp.x(), beginp.y());
        boundings.setPoint(1, beginp.x() + 1, beginp.y());
        boundings.setPoint(2, beginp.x() + 1, beginp.y() + 1);
        boundings.setPoint(3, beginp.x(), beginp.y() + 1);
        setPoints(boundings);
        return;
    }

    const double m = ARROW_LENGTH / sqrt(double(dx * dx + dy * dy));

    double deltax = dy * m;
    double deltay = dx * m;

    // modif de delta?/1 pour placer une marge de l/1 a chaque extremite
    boundings.setPoint(0, (int)(beginp.x() - deltax - deltay / 1),
                       (int)(beginp.y() - deltay + deltax / 1));
    boundings.setPoint(1, (int)(beginp.x() + deltax - deltay / 1),
                       (int)(beginp.y() + deltay + deltax / 1));
    boundings.setPoint(2, (int)(endp.x() + deltax + deltay / 1),
                       (int)(endp.y() + deltay - deltax / 1));
    boundings.setPoint(3, (int)(endp.x() - deltax + deltay / 1),
                       (int)(endp.y() - deltay - deltax / 1));

    setPoints(boundings);

    // points de la fleche
    // *3/5 pour diminuer la fleche et ne pas risquer de sortir de boundings
    deltax *= 3.0 / 5.0;
    deltay *= 3.0 / 5.0;

    switch (end->type()) {
    case UmlArrowPoint:
        break;

    case UmlArrowJunction:

        // note : the circle or arc is drawn by the arrow junction
        if (itstype == UmlRequired) {
            // end of line
            arrow[0].setX((int)(endp.x() - deltay * (5.0 / 3.0 / ARROW_LENGTH * ARROW_JUNCTION_SIZE / 2)));
            arrow[0].setY((int)(endp.y() + deltax * (5.0 / 3.0 / ARROW_LENGTH * ARROW_JUNCTION_SIZE / 2)));
            // start angle*16
            arrow[1].setX((dx == 0)
                          ? ((dy > 0) ? 180 * 16 : 0 * 16)
                              : ((((int)(atan(float(dy) / dx) * (180 / 3.1415927 * 16))) +
                                  ((dx > 0) ? 90 * 16 : 270 * 16))
                                 % (360 * 16)));
        }
        else {
            // end of line
            arrow[0].setX((int)(endp.x() - deltay * (5.0 / 3.0 / ARROW_LENGTH * PROVIDED_RADIUS)));
            arrow[0].setY((int)(endp.y() + deltax * (5.0 / 3.0 / ARROW_LENGTH * PROVIDED_RADIUS)));
        }

        break;

    default:
        switch (itstype) {
        case UmlInner:
            arrow[0].setX((int)(endp.x() - deltay));
            arrow[0].setY((int)(endp.y() + deltax));
            arrow[1].setX((int)(endp.x() - deltay - deltay));
            arrow[1].setY((int)(endp.y() + deltax + deltax));
            break;

        case UmlDirectionalAssociation:
            if (!strcmp(end->get_bn()->get_stereotype(), "metaclass") &&
                !strcmp(get_start()->get_bn()->get_stereotype(), "stereotype")) {
                // extend polygone
                poly.resize(3);
                poly.setPoint(0, (int)(endp.x() - deltax - deltay), (int)(endp.y() - deltay + deltax));
                poly.setPoint(1, (int)(endp.x() + deltax - deltay), (int)(endp.y() + deltay + deltax));
                poly.setPoint(2, (int) endp.x(), (int) endp.y());
                break;
            }

            // no break
        default:
            arrow[0].setX((int)(endp.x() - deltax - deltay));
            arrow[0].setY((int)(endp.y() - deltay + deltax));
            arrow[1].setX((int)(endp.x() + deltax - deltay));
            arrow[1].setY((int)(endp.y() + deltay + deltax));
            arrow[2].setX((int)(endp.x() - deltay));
            arrow[2].setY((int)(endp.y() + deltax));
        }
    }

    if (begin->type() != UmlArrowPoint) {
        // aggregation's polygone
        switch (itstype) {
        case UmlAggregation:
        case UmlDirectionalAggregation:
        case UmlAggregationByValue:
        case UmlDirectionalAggregationByValue: {
            double d_x = (deltax + deltay); // *2.0/3.0;
            double d_y = (deltay - deltax); // *2.0/3.0;

            poly.resize(4);
            poly.setPoint(0, beginp.x(), beginp.y());
            poly.setPoint(1, (int)(beginp.x() + d_y), (int)(beginp.y() - d_x));
            poly.setPoint(2, (int)(beginp.x() + d_y + d_x), (int)(beginp.y() - d_x + d_y));
            poly.setPoint(3, (int)(beginp.x() + d_x), (int)(beginp.y() + d_y));
        }
        break;

        default:	// to avoid compiler warning
            break;
        }
    }

    if (auto_pos) {
        // moves label dependent on old position
        if ((label != 0) && !label->selected())
            label->moveBy((beginp.x() - old_beginp.x() + endp.x() - old_endp.x()) / 2.0,
                          (beginp.y() - old_beginp.y() + endp.y() - old_endp.y()) / 2.0);

        // moves stereotype dependent on old position
        if ((stereotype != 0) && !stereotype->selected())
            stereotype->moveBy((beginp.x() - old_beginp.x() + endp.x() - old_endp.x()) / 2.0,
                               (beginp.y() - old_beginp.y() + endp.y() - old_endp.y()) / 2.0);
    }

    update_show_lines();

    if (need_update_geometry)
        update_geometry();
}

// force all the arrow points to be ouside r
void ArrowCanvas::move_outside(QRect r)
{
    // check it is the first segment
    if ((begin != 0) && (begin->type() != UmlArrowPoint)) {
        QPoint c = r.center();
        ArrowCanvas * ar = this;

        while ((ar->end != 0) && (ar->end->type() == UmlArrowPoint)) {
            ArrowPointCanvas * p = (ArrowPointCanvas *) ar->end;
            double px = p->x();
            double py = p->y();

            double dx = (px < c.x()) ? -10.0 : 10.0;
            double dy = (py < c.y()) ? -10.0 : 10.0;

            while (r.contains((int) px, (int) py)) {
                px += dx;
                py += dy;
            }

            p->move(px, py);

            ar = p->get_other(ar);

            if (ar == 0)
                // something wrong
                return;
        }
    }
}

void ArrowCanvas::move_self_points(double dx, double dy)
{
    // check it is the first segment
    if ((begin != 0) && (begin->type() != UmlArrowPoint)) {
        ArrowCanvas * a = this;

        while ((a->end != 0) && (a->end->type() == UmlArrowPoint)) {
            ((ArrowPointCanvas *) a->end)->moveBy(dx, dy);
            a = ((ArrowPointCanvas *) a->end)->get_other(a);

            if (a == 0)
                // something wrong
                return;
        }
    }
}

void ArrowCanvas::change_scale()
{
    // all the extremities and the label&stereotype are already moved
    LabelCanvas * lab = label;
    LabelCanvas * ste = stereotype;

    label = 0;
    stereotype = 0;
    Q3CanvasPolygon::setVisible(FALSE);
    update_pos();
    Q3CanvasPolygon::setVisible(TRUE);
    label = lab;
    stereotype = ste;
}

void ArrowCanvas::setVisible(bool yes)
{
    Q3CanvasPolygon::setVisible(yes);

    if (label != 0)
        label->setVisible(yes);

    if (stereotype != 0)
        stereotype->setVisible(yes);
}

void ArrowCanvas::moveBy(double dx, double dy)
{
    if ((label != 0) && !label->selected())
        label->moveBy(dx, dy);

    if ((stereotype != 0) && !stereotype->selected())
        stereotype->moveBy(dx, dy);
}

double ArrowCanvas::get_z() const
{
    return z();
}

void ArrowCanvas::set_z(double z)
{
    setZ(z);

    if (stereotype != 0)
        stereotype->setZ(z);

    if (label != 0)
        label->setZ(z);

    if ((begin != 0) &&
        (begin->type() == UmlArrowPoint) &&
        (((ArrowPointCanvas *) begin)->z() < (z + 1)))
        ((ArrowPointCanvas *) begin)->setZ(z + 1);

    if ((end != 0) &&
        (end->type() == UmlArrowPoint) &&
        (((ArrowPointCanvas *) end)->z() < (z + 1)))
        ((ArrowPointCanvas *) end)->setZ(z + 1);
}

void ArrowCanvas::go_up(double nz)
{
    ArrowCanvas * a = this;

    while ((a->begin != 0) && (a->begin->type() == UmlArrowPoint)) {
        a = ((ArrowPointCanvas *) a->begin)->get_other(a);

        if (a == 0)
            // something wrong
            return;
    }

    for (;;) {
        if (a->z() < nz) {
            a->setZ(nz);

            if (a->stereotype != 0)
                a->stereotype->setZ(nz);

            if (a->label != 0)
                a->label->setZ(nz);
        }

        foreach (ArrowCanvas *canvas, a->lines)
            canvas->go_up(nz);

        if ((a->end != 0) && (a->end->type() == UmlArrowPoint)) {
            if (((ArrowPointCanvas *) a->end)->z() < (nz + 1))
                ((ArrowPointCanvas *) a->end)->set_z(nz + 1);

            a = ((ArrowPointCanvas *) a->end)->get_other(a);

            if (a == 0)
                // something wrong
                return;
        }
        else
            return;
    }
}

bool ArrowCanvas::copyable() const
{
    return selected() && get_start()->copyable() && get_end()->copyable();
}

void ArrowCanvas::drawShape(QPainter & p)
{
    if (! visible() || (beginp == endp))
        return;

    p.setRenderHint(QPainter::Antialiasing, true);
    p.setBackgroundMode(::Qt::OpaqueMode);

    FILE * fp = svg();
    const char * dash = "";

    if (fp != 0)
        fputs("<g>\n", fp);

    switch (((itstype == UmlTransition) &&
             (get_data() != 0) && // not under construction
             ((TransitionData *) get_data())->internal())
            ? UmlDependency : itstype) {
    case UmlDirectionalAssociation:
        if (end->type() != UmlArrowPoint) {
            if (!strcmp(end->get_bn()->get_stereotype(), "metaclass") &&
                !strcmp(get_start()->get_bn()->get_stereotype(), "stereotype")) {
                // extend
                QBrush brsh = p.brush();

                p.setBrush(::Qt::black);
                p.drawPolygon(poly/*, TRUE*/);
                p.setBrush(brsh);

                if (fp != 0)
                    draw_poly(fp, poly, UmlBlack);
            }
            else {
                p.drawLine(endp, arrow[0]);
                p.drawLine(endp, arrow[1]);

                if (fp != 0) {
                    fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                            " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                            endp.x(), endp.y(), arrow[0].x(), arrow[0].y());
                    fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                            " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                            endp.x(), endp.y(), arrow[1].x(), arrow[1].y());
                }
            }
        }

        p.drawLine(beginp, endp);

        if (fp != 0)
            fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                    " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                    beginp.x(), beginp.y(), endp.x(), endp.y());

        break;

    case UmlDirectionalAggregation:
    case UmlDirectionalAggregationByValue:
    case UmlContain:
    case UmlTransition:
    case UmlFlow:
        if (end->type() != UmlArrowPoint) {
            p.drawLine(endp, arrow[0]);
            p.drawLine(endp, arrow[1]);

            if (fp != 0) {
                fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                        " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                        endp.x(), endp.y(), arrow[0].x(), arrow[0].y());
                fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                        " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                        endp.x(), endp.y(), arrow[1].x(), arrow[1].y());
            }
        }

        // no break
    case UmlAssociation:
    case UmlAggregation:
    case UmlAggregationByValue:
    case UmlLink:
    case UmlObjectLink:
        p.drawLine(beginp, endp);

        if (fp != 0)
            fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                    " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                    beginp.x(), beginp.y(), endp.x(), endp.y());

        if (begin->type() != UmlArrowPoint) {
            switch (itstype) {
            case UmlAggregation:
            case UmlDirectionalAggregation: {
                QBrush brsh = p.brush();

                p.setBrush(::Qt::white);
                p.drawPolygon(poly/*, TRUE*/);
                p.setBrush(brsh);

                if (fp != 0)
                    draw_poly(fp, poly, UmlWhite);
            }
            break;

            case UmlAggregationByValue:
            case UmlDirectionalAggregationByValue: {
                QBrush brsh = p.brush();

                p.setBrush(::Qt::black);
                p.drawPolygon(poly/*, TRUE*/);
                p.setBrush(brsh);

                if (fp != 0)
                    draw_poly(fp, poly, UmlBlack);
            }
            break;

            default:	// to avoid compiler warning
                break;
            }
        }

        break;

    case UmlDependency:
    case UmlDependOn:
        if (end->type() != UmlArrowPoint) {
            p.drawLine(endp, arrow[0]);
            p.drawLine(endp, arrow[1]);

            if (fp != 0) {
                fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                        " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                        endp.x(), endp.y(), arrow[0].x(), arrow[0].y());
                fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                        " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                        endp.x(), endp.y(), arrow[1].x(), arrow[1].y());
            }
        }

        p.setPen(::Qt::DotLine);
        p.drawLine(beginp, endp);
        p.setPen(::Qt::SolidLine);

        if (fp != 0)
            fprintf(fp, "\t<line stroke-dasharray=\"4,4\" stroke=\"black\" stroke-opacity=\"1\""
                    " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                    beginp.x(), beginp.y(), endp.x(), endp.y());

        break;

    case UmlRealize:
        p.setPen(::Qt::DotLine);
        dash = "stroke-dasharray=\"4,4\" ";

        // no break
    case UmlGeneralisation:
    case UmlInherit:
        if (end->type() != UmlArrowPoint) {
            p.drawLine(beginp, arrow[2]);
            p.setPen(::Qt::SolidLine);
            p.drawLine(endp, arrow[0]);
            p.drawLine(endp, arrow[1]);
            p.drawLine(arrow[0], arrow[1]);

            if (fp != 0) {
                fprintf(fp, "\t<line %sstroke=\"black\" stroke-opacity=\"1\""
                        " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                        dash, beginp.x(), beginp.y(), arrow[2].x(), arrow[2].y());
                fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                        " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                        endp.x(), endp.y(), arrow[0].x(), arrow[0].y());
                fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                        " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                        endp.x(), endp.y(), arrow[1].x(), arrow[1].y());
                fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                        " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                        arrow[0].x(), arrow[0].y(), arrow[1].x(), arrow[1].y());
            }
        }
        else {
            p.drawLine(beginp, endp);
            p.setPen(::Qt::SolidLine);

            if (fp != 0)
                fprintf(fp, "\t<line %sstroke=\"black\" stroke-opacity=\"1\""
                        " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                        dash, beginp.x(), beginp.y(), endp.x(), endp.y());
        }

        break;

    case UmlAnchor:
        p.setPen(::Qt::DotLine);
        p.drawLine(beginp, endp);
        p.setPen(::Qt::SolidLine);

        if (fp != 0)
            fprintf(fp, "\t<line stroke-dasharray=\"4,4\" stroke=\"black\" stroke-opacity=\"1\""
                    " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                    beginp.x(), beginp.y(), endp.x(), endp.y());

        break;

    case UmlRequired:
    case UmlProvided:
        if (end->type() != UmlArrowPoint) {
            p.drawLine(beginp, arrow[0]);

            if (fp != 0)
                fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                        " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                        beginp.x(), beginp.y(), arrow[0].x(), arrow[0].y());
        }
        else {
            p.drawLine(beginp, endp);

            if (fp != 0)
                fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                        " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                        beginp.x(), beginp.y(), endp.x(), endp.y());
        }

        break;

    case UmlInner:
        if (end->type() != UmlArrowPoint) {
            p.drawLine(beginp, arrow[1]);
            p.drawPixmap(QPoint(arrow[0].x() - ARROW_LENGTH / 2,
                                arrow[0].y() - ARROW_LENGTH / 2),
                         *innerPixmap);

            if (fp != 0) {
                fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                        " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                        beginp.x(), beginp.y(), arrow[1].x(), arrow[1].y());
                fprintf(fp, "<ellipse fill=\"none\" stroke=\"black\" stroke-width=\"1\" stroke-opacity=\"1\" cx=\"%d\" cy=\"%d\" rx=\"%d\" ry=\"%d\" />\n",
                        arrow[0].x(), arrow[0].y(),
                        ARROW_LENGTH / 2, ARROW_LENGTH / 2);
                fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                        " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                        arrow[0].x() - ARROW_LENGTH / 2, arrow[0].y(),
                        arrow[0].x() + ARROW_LENGTH / 2, arrow[0].y());
                fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                        " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                        arrow[0].x(), arrow[0].y() - ARROW_LENGTH / 2,
                        arrow[0].x(), arrow[0].y() + ARROW_LENGTH / 2);
            }
        }
        else {
            p.drawLine(beginp, endp);

            if (fp != 0)
                fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                        " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                        beginp.x(), beginp.y(), endp.x(), endp.y());
        }

    default:	// to avoid compiler warning
        break;
    }

    if (fp != 0)
        fputs("</g>\n", fp);

    if (selected()) {
        p.fillRect(beginp.x() - SELECT_SQUARE_SIZE / 2,
                   beginp.y() - SELECT_SQUARE_SIZE / 2,
                   SELECT_SQUARE_SIZE, SELECT_SQUARE_SIZE, ::Qt::black);
        p.fillRect(endp.x() - SELECT_SQUARE_SIZE / 2,
                   endp.y() - SELECT_SQUARE_SIZE / 2,
                   SELECT_SQUARE_SIZE, SELECT_SQUARE_SIZE, ::Qt::black);
    }
}

UmlCode ArrowCanvas::type() const
{
    return itstype;
}

int ArrowCanvas::rtti() const
{
    return RTTI_ARROW;
}

BasicData * ArrowCanvas::get_data() const
{
    // must not be used at this level
    return 0;
}

void ArrowCanvas::delete_available(BooL &, BooL & out_model) const
{
    out_model |= TRUE;
}

void ArrowCanvas::remove(bool)
{
    if (itstype == UmlInner) {
        if (the_canvas()->must_draw_all_relations()) {
            const ArrowCanvas * a = this;

            while (a->begin->type() == UmlArrowPoint) {
                a = ((ArrowPointCanvas *) a->begin)->get_other(a);

                if (a == 0)
                    break;
            }

            if (a && !a->begin->isSelected() && !a->begin->get_bn()->deletedp()) {
                a = this;

                while (a->end->type() == UmlArrowPoint) {
                    a = ((ArrowPointCanvas *) a->end)->get_other(a);

                    if (a == 0)
                        break;
                }

                if (a && !a->end->isSelected() && !a->end->get_bn()->deletedp()) {
                    msg_warning("Douml", TR("<i>Draw all relations</i> forced to <i>no</i>"));
                    the_canvas()->dont_draw_all_relations();
                }
            }
        }
    }

    delete_it();
}

QRect ArrowCanvas::rect() const
{
    QRect r(beginp, endp);

    return r.normalize();
}

QPoint ArrowCanvas::center() const
{
    return QPoint((beginp.x() + endp.x()) / 2,
                  (beginp.y() + endp.y()) / 2);
}

bool ArrowCanvas::isSelected() const
{
    return selected();
}

bool ArrowCanvas::contains(int, int) const
{
    return FALSE;
}

QString ArrowCanvas::may_start(UmlCode & l) const
{
    return (l == UmlAnchor) ? QString() : TR("a relation can't have a relation");
}

QString ArrowCanvas::may_connect(UmlCode & l, const DiagramItem * dest) const
{
    return (l == UmlAnchor) ? dest->may_start(l) : TR("illegal");
}

void ArrowCanvas::connexion(UmlCode action, DiagramItem * dest,
                            const QPoint &, const QPoint &)
{
    ArrowCanvas * a =
        new ArrowCanvas(the_canvas(), this, dest, action, 0, FALSE, -1.0, -1.0);

    a->show();
    the_canvas()->select(a);
    a->modified();
}

bool ArrowCanvas::edit(const QStringList & defaults,
                       ArrowCanvas * plabel, ArrowCanvas * pstereotype)
{
    QString la;
    QString st;

    if (plabel)
        la = plabel->label->get_name();

    if (pstereotype)
        st = pstereotype->stereotype->get_name();

    StereotypeDialog d(defaults, st, la,
                       TR("stereotype/label dialog"),
                       TR("label : "));

    d.raise();

    if (d.exec() == QDialog::Accepted) {
        bool result = FALSE;

        if (la.isEmpty()) {
            if (plabel != 0) {
                // removes it
                the_canvas()->del(plabel->label);
                plabel->label = 0;
                result = TRUE;
            }
        }
        else {
            if (plabel == 0) {
                // adds relation's label
                label = new LabelCanvas(la, the_canvas(), 0, 0);
                default_label_position();
                label->show();
                result = TRUE;
            }
            else if ((plabel != 0) &&
                     (plabel->label->get_name() != la)) {
                // update name
                plabel->label->set_name(la);
                plabel->default_label_position();
                result = TRUE;
            }
        }

        if (st.isEmpty()) {
            if (pstereotype != 0) {
                // removes it
                the_canvas()->del(pstereotype->stereotype);
                pstereotype->stereotype = 0;
                result = TRUE;
            }
        }
        else {
            if (pstereotype == 0) {
                // adds relation's stereotype
                stereotype = new LabelCanvas(st, the_canvas(), 0, 0);
                default_stereotype_position();
                stereotype->show();
                result = TRUE;
            }
            else if ((pstereotype != 0) &&
                     (pstereotype->stereotype->get_name() != st)) {
                // update name
                pstereotype->stereotype->set_name(st);
                pstereotype->default_stereotype_position();
                result = TRUE;
            }
        }

        return result;
    }
    else
        return FALSE;
}

void ArrowCanvas::open()
{
    if (IsaRelation(itstype)) {
        ArrowCanvas * plabel;
        ArrowCanvas * pstereotype;

        search_supports(plabel, pstereotype);

        BrowserNode * bn = get_start()->get_bn();

        if ((bn != 0) &&
            edit(bn->default_stereotypes(itstype, get_end()->get_bn()),
                 plabel, pstereotype)) {
            canvas()->update();
            package_modified();
        }
    }
}

void ArrowCanvas::default_label_position() const
{
    QPoint c = center();
    QFontMetrics fm(the_canvas()->get_font(UmlNormalFont));

    label->move(c.x() - fm.width(label->get_name()) / 2, c.y() - fm.height());
}

void ArrowCanvas::default_stereotype_position() const
{
    QPoint c = center();
    QFontMetrics fm(the_canvas()->get_font(UmlNormalFont));

    stereotype->move(c.x() - fm.width(stereotype->get_name()) / 2, c.y());
}

void ArrowCanvas::menu(const QPoint &)
{
    Q3PopupMenu m(0);
    Q3PopupMenu geo(0);
    ArrowCanvas * plabel;
    ArrowCanvas * pstereotype;

    search_supports(plabel, pstereotype);

    MenuFactory::createTitle(m, ((plabel == 0) ||
                                 plabel->label->get_name().isEmpty())
                             ? QString(TR("line")) : plabel->label->get_name());

    if (IsaRelation(itstype)) {
        m.insertSeparator();
        m.insertItem(TR("Edit"), 1);
    }

    if (pstereotype || plabel) {
        m.insertSeparator();
        m.insertItem(TR("Select stereotype and label"), 2);
        m.insertItem(TR("Default stereotype and label position"), 3);

        if (plabel && (label == 0))
            m.insertItem(TR("Attach label to this segment"), 4);

        if (pstereotype && (stereotype == 0))
            m.insertItem(TR("Attach stereotype to this segment"), 5);
    }

    if (get_start() != get_end()) {
        m.insertSeparator();
        init_geometry_menu(geo, 10);
        m.insertItem(TR("Geometry (Ctrl+l)"), &geo);
    }

    m.insertSeparator();
    m.insertItem(TR("Remove from diagram"), 6);

    int choice = m.exec(QCursor::pos());

    switch (choice) {
    case 1:
        open();
        return;

    case 2:
        the_canvas()->unselect_all();

        if (plabel != 0)
            the_canvas()->select(plabel->label);

        if (pstereotype)
            the_canvas()->select(pstereotype->stereotype);

        return;

    case 3:
        if (plabel != 0)
            plabel->default_label_position();

        if (pstereotype)
            pstereotype->default_stereotype_position();

        break;

    case 4:
        label = plabel->label;
        plabel->label = 0;
        default_label_position();
        break;

    case 5:
        stereotype = pstereotype->stereotype;
        pstereotype->stereotype = 0;
        default_stereotype_position();
        break;

    case 6:
        remove(FALSE);
        break;

    default:
        if (choice >= 10) {
            choice -= 10;

            if (choice == RecenterBegin)
                set_decenter(-1.0, decenter_end);
            else if (choice == RecenterEnd)
                set_decenter(decenter_begin, -1.0);
            else if (choice != (int) geometry)
                set_geometry((LineGeometry) choice, TRUE);
            else
                return;
        }
        else
            return;

        break;
    }

    package_modified();
}

void ArrowCanvas::init_geometry_menu(Q3PopupMenu & m, int first)
{
    QPixmap hv((const char **) geometry_hv);
    QPixmap vh((const char **) geometry_vh);
    QPixmap hvh((const char **) geometry_hvh);
    QPixmap vhv((const char **) geometry_vhv);

    m.setCheckable(TRUE);

    m.insertItem(TR("None"), first);
    m.insertItem(hv, first + HVGeometry);
    m.insertItem(vh, first + VHGeometry);
    m.insertItem(hvh, first + HVHGeometry);
    m.insertItem(vhv, first + VHVGeometry);

    switch (geometry) {
    case NoGeometry:
        m.setItemChecked(first, TRUE);
        break;

    case HVGeometry:
    case HVrGeometry:
        m.setItemChecked(first + HVGeometry, TRUE);
        break;

    case VHGeometry:
    case VHrGeometry:
        m.setItemChecked(first + VHGeometry, TRUE);
        break;

    case HVHGeometry:
        m.setItemChecked(first + HVHGeometry, TRUE);
        break;

    default:
        // VHVGeometry
        m.setItemChecked(first + VHVGeometry, TRUE);
    }

    if (decenter_begin >= 0)
        m.insertItem(TR("Recenter begin"), first + RecenterBegin);

    if (decenter_end >= 0)
        m.insertItem(TR("Recenter end"), first + RecenterEnd);
}

DiagramItem * ArrowCanvas::get_start() const
{
    const ArrowCanvas * a = this;

    while (a->begin->type() == UmlArrowPoint)
        a = ((ArrowPointCanvas *) a->begin)->get_other(a);

    return a->begin;
}

DiagramItem * ArrowCanvas::get_end() const
{
    const ArrowCanvas * a = this;

    while (a->end->type() == UmlArrowPoint)
        a = ((ArrowPointCanvas *) a->end)->get_other(a);

    return a->end;
}

void ArrowCanvas::extremities(DiagramItem *& b, DiagramItem *& e) const
{
    b = begin;
    e = end;
}

// search label & stereotype supports
void ArrowCanvas::search_supports(ArrowCanvas *& plabel,
                                  ArrowCanvas *& pstereotype) const
{
    plabel = 0;
    pstereotype = 0;

    const ArrowCanvas * p;

    p = this;

    while (p->end->type() == UmlArrowPoint) {
        if (p->label != 0) plabel = (ArrowCanvas *) p;

        if (p->stereotype != 0) pstereotype = (ArrowCanvas *) p;

        p = ((ArrowPointCanvas *) p->end)->get_other(p);
    }

    if (p->label != 0) plabel = (ArrowCanvas *) p;

    if (p->stereotype != 0) pstereotype = (ArrowCanvas *) p;

    p = this;

    while (p->begin->type() == UmlArrowPoint) {
        if (p->label != 0) plabel = (ArrowCanvas *) p;

        if (p->stereotype != 0) pstereotype = (ArrowCanvas *) p;

        p = ((ArrowPointCanvas *) p->begin)->get_other(p);
    }

    if (p->label != 0) plabel = (ArrowCanvas *) p;

    if (p->stereotype != 0) pstereotype = (ArrowCanvas *) p;
}

// reverse line, warning caller must call update_pos() after
void ArrowCanvas::reverse()
{
    ArrowCanvas * a = this;

    while (a->begin->type() == UmlArrowPoint)
        a = (ArrowCanvas *)((ArrowPointCanvas *) a->begin)->get_other(a);

    for (;;) {
        DiagramItem * di = a->begin;

        a->begin = a->end;
        a->end = di;

        float decenter = a->decenter_begin;

        a->decenter_begin = a->decenter_end;
        a->decenter_end = decenter;

        if (a->begin->type() != UmlArrowPoint)
            break;

        a = (ArrowCanvas *)((ArrowPointCanvas *) a->begin)->get_other(a);
    }
}

ArrowPointCanvas * ArrowCanvas::brk(const QPoint & p)
{
    ArrowPointCanvas * ap =
        new ArrowPointCanvas(the_canvas(), p.x(), p.y());

    ap->setZ(z() + 1);

    DiagramItem * e = end;

    ap->add_line(this);
    end->remove_line(this, TRUE);
    end = ap;

    ArrowCanvas * other =
        new ArrowCanvas(the_canvas(), ap, e, itstype, 0, FALSE,
                        decenter_begin, decenter_end);

    if ((p - beginp).manhattanLength() < (p - endp).manhattanLength()) {
        if (label != 0) {
            other->label = label;
            label = 0;
        }

        if (stereotype != 0) {
            other->stereotype = stereotype;
            stereotype = 0;
        }
    }

    // no geometry, must be set before calling update_pos()
    // whose may call update_geometry
    propag_geometry(NoGeometry, FALSE);

    ap->show();
    other->show();

    modified();
    other->modified();

    return ap;
}

// return false if a point can't removed
bool ArrowCanvas::may_join() const
{
    if (get_start() == get_end()) {
        // self relation
        int npoint = 0;
        const ArrowCanvas * a = this;

        a = this;

        while (a->begin->type() == UmlArrowPoint) {
            npoint += 1;
            a = ((ArrowCanvas *)((ArrowPointCanvas *) a->begin)->get_other(a));
        }

        a = this;

        while (a->end->type() == UmlArrowPoint) {
            npoint += 1;
            a = ((ArrowCanvas *)((ArrowPointCanvas *) a->end)->get_other(a));
        }

        return (npoint > 2);
    }
    else
        return TRUE;
}

ArrowCanvas * ArrowCanvas::join(ArrowCanvas * other, ArrowPointCanvas * ap)
{
    // has already check is join is possible (self relation must have two points)
    // ap is the removed arrow point
    if (end == ap) {
        // no geometry, must be set before calling update_pos()
        // whose may call update_geometry
        propag_geometry(NoGeometry, FALSE);

        end = other->end;
        end->add_line(this);	// add before remove in case end is
        end->remove_line(other, TRUE);	// an arrow junction and not del it

        QList<ArrowCanvas *> olines = other->lines;
        LabelCanvas * olabel = other->label;
        LabelCanvas * ostereotype = other->stereotype;

        other->lines.clear();
        other->begin = other->end = 0;
        other->label = 0;
        other->stereotype = 0;
        ap->remove_line(this, TRUE);
        ap->remove_line(other, TRUE);
        other->unconnect();
        the_canvas()->del(other);
        the_canvas()->del(ap);
        hide();
        update_pos();
        show();

        //gets other's lines (anchor to note)
        foreach (ArrowCanvas *l, olines) {
            l->hide();

            if (l->begin == other)
                l->begin = this;
            else
                l->end = this;

            add_line(l);
            l->update_pos();
            l->show();
        }
        olines.clear();

        //gets label and stereotype
        QFontMetrics fm(the_canvas()->get_font(UmlNormalFont));
        int h = fm.height();
        QPoint c = center();

        if (olabel) {
            ///tant que gere pas label sur n'importe qu el segment
            if (label != 0)
                label->hide();

            //tq

            label = olabel;
            label->move(c.x() - fm.width(label->get_name()) / 2, c.y() - h);
        }

        if (ostereotype) {
            stereotype = ostereotype;
            stereotype->move(c.x() - fm.width(stereotype->get_name()) / 2, c.y());
        }

        return this;
    }
    else
        return other->ArrowCanvas::join(this, ap);
}

// the arrow is the alone selected element and there is
// a mouse move event, what is must be done ?

bool ArrowCanvas::cut_on_move(ArrowPointCanvas *& ap) const
{
    ap = 0;

    switch (geometry) {
    case NoGeometry:
        // cut the line
        return TRUE;

    case HVGeometry:
    case VHrGeometry:
    case HVrGeometry:
    case VHGeometry:
        // nothing must be done (ap == 0)
        return FALSE;

    default:
        // get any point, this will be moved
        ap = (ArrowPointCanvas *)
             ((begin->type() == UmlArrowPoint) ? begin : end);
        return FALSE;
    }
}

bool ArrowCanvas::is_decenter(QPoint mousePressPos,
                              BooL & start, BooL & horiz) const
{
    switch (end->type()) {
    case UmlArrowPoint:
    case UmlArrowJunction:
        break;

    default:
        if ((iabs(endp.x() - mousePressPos.x()) < 10) &&
            (iabs(endp.y() - mousePressPos.y()) < 10)) {
            // near end of line
            start = FALSE;

            QRect r = end->rect();
            QPoint p = endp;

            end->shift(p, beginp, r.contains(begin->rect()));
            horiz = (iabs(endp.y() - r.top()) < 2) ||
                    (iabs(endp.y() - r.bottom()) < 2);

            return TRUE;
        }
    }

    if ((begin->type() != UmlArrowPoint) &&
        (iabs(beginp.x() - mousePressPos.x()) < 10) &&
        (iabs(beginp.y() - mousePressPos.y()) < 10)) {
        // near start of line
        start = TRUE;

        QRect r = begin->rect();
        QPoint p = beginp;

        begin->shift(p, end->center(), r.contains(end->rect()));

        horiz = (iabs(beginp.y() - r.top()) < 2) ||
                (iabs(beginp.y() - r.bottom()) < 2);

        return TRUE;
    }
    else
        return FALSE;
}

void ArrowCanvas::decenter(QPoint p, bool start, bool horiz)
{
    float & decenter_it = (start) ? decenter_begin : decenter_end;
    QRect r = (start) ? begin->rect() : end->rect();
    int margin =
        // force the position to have more than 3 point to the edge
        // to not be < 2 and change the edge
        (int)(9 * the_canvas()->zoom());

    if (horiz) {
        // on top or botton
        int nx = p.x();

        if (nx < (r.left() + margin))
            decenter_it = margin / ((float) r.width());
        else if (nx > (r.right() - margin))
            decenter_it = (r.width() - margin) / ((float) r.width());
        else
            decenter_it = (nx - r.left()) / ((float) r.width());
    }
    else  {
        // on left or right
        int ny = p.y();

        if (ny < (r.top() + margin))
            decenter_it = margin / ((float) r.height());
        else if (ny > (r.bottom() - margin))
            decenter_it = (r.height() - margin) / ((float) r.height());
        else
            decenter_it = (ny - r.top()) / ((float) r.height());
    }

    decenter_it =
        // to have the same error after reloading
        ((int)(decenter_it * 1000)) / 1000.0;

    propag_decenter(decenter_begin, decenter_end);

    Q3CanvasPolygon::setVisible(FALSE);
    update_pos();
    Q3CanvasPolygon::setVisible(TRUE);
}

void ArrowCanvas::propag_decenter(float db, float de)
{
    decenter_begin = db;
    decenter_end = de;

    ArrowCanvas * a = this;

    while (a->begin->type() == UmlArrowPoint) {
        a = ((ArrowPointCanvas *) a->begin)->get_other(a);
        a->decenter_begin = db;
        a->decenter_end = de;
    }

    a = this;

    while (a->end->type() == UmlArrowPoint) {
        a = ((ArrowPointCanvas *) a->end)->get_other(a);
        a->decenter_begin = db;
        a->decenter_end = de;
    }
}

void ArrowCanvas::set_decenter(float db, float de)
{
    propag_decenter(db, de);

    Q3CanvasPolygon::setVisible(FALSE);

    ArrowCanvas * a = this;

    while (a->begin->type() == UmlArrowPoint) {
        a = ((ArrowPointCanvas *) a->begin)->get_other(a);
        a->update_pos();
    }

    a = this;

    while (a->end->type() == UmlArrowPoint) {
        a = ((ArrowPointCanvas *) a->end)->get_other(a);
        a->update_pos();
    }

    update_pos();
    update_geometry();

    Q3CanvasPolygon::setVisible(TRUE);

    canvas()->update();
}

void ArrowCanvas::propag_geometry(LineGeometry geo, bool fixed)
{
    geometry = geo;
    fixed_geometry = fixed;

    ArrowCanvas * a = this;

    while (a->begin->type() == UmlArrowPoint) {
        a = ((ArrowPointCanvas *) a->begin)->get_other(a);
        a->geometry = geo;
        a->fixed_geometry = fixed;
    }

    a = this;

    while (a->end->type() == UmlArrowPoint) {
        a = ((ArrowPointCanvas *) a->end)->get_other(a);
        a->geometry = geo;
        a->fixed_geometry = fixed;
    }
}

// (get_start() != get_end()) check before

ArrowCanvas * ArrowCanvas::set_geometry(LineGeometry geo, bool fixed)
{
    DiagramView * view = the_canvas()->get_view();

    view->history_save();
    view->freeze_history(TRUE);

    // first remove arrow points
    // warning : after join() 'this' can't be used
    ArrowCanvas * ar = this;

    while (ar->begin->type() == UmlArrowPoint)
        ar = ar->join(((ArrowPointCanvas *) ar->begin)->get_other(ar),
                      (ArrowPointCanvas *) ar->begin);

    while (ar->end->type() == UmlArrowPoint)
        ar = ar->join(((ArrowPointCanvas *) ar->end)->get_other(ar),
                      (ArrowPointCanvas *) ar->end);

    DiagramItem * a = ar->get_start();
    DiagramItem * b = ar->get_end();
    QPoint ca = a->center();
    QPoint cb = b->center();

    switch (geo) {
    case NoGeometry:
        // note : new geometry already propaged
        view->freeze_history(FALSE);
        view->protect_history(TRUE);
        return ar;

    case HVGeometry:
    case VHGeometry:
        if (ca.x() > cb.x())
            geo = (LineGeometry)(geo + 1);	// Xr

        break;

    default:
        break;
    }

    ar->propag_decenter(-1.0, -1.0);

    switch (geo) {
    case HVGeometry:
    case VHrGeometry: {
        QPoint p(cb.x() - ARROW_POINT_SIZE / 2,
                 ca.y() - ARROW_POINT_SIZE / 2);

        ar->brk(p);
    }
    break;

    case HVrGeometry:
    case VHGeometry: {
        QPoint p(ca.x() - ARROW_POINT_SIZE / 2,
                 cb.y() - ARROW_POINT_SIZE / 2);

        ar->brk(p);
    }
    break;

    case HVHGeometry: {
        QRect arect = a->rect();
        QRect brect = b->rect();
        QPoint p(((ca.x() < cb.x()) ? (arect.right() + brect.left())
                  : (arect.left() + brect.right())) / 2
                 - ARROW_POINT_SIZE / 2,
                 ca.y() - ARROW_POINT_SIZE / 2);
        ArrowPointCanvas * ap = ar->brk(p);

        p.setY(cb.y() -  ARROW_POINT_SIZE / 2);
        ap->get_other(ar)->brk(p);
    }
    break;

    default:
        // VHVGeometry
    {
        QRect arect = a->rect();
        QRect brect = b->rect();
        QPoint p(ca.x() - ARROW_POINT_SIZE / 2,
                 ((ca.y() < cb.y()) ? (arect.bottom() + brect.top())
                  : (arect.top() + brect.bottom())) / 2
                 - ARROW_POINT_SIZE / 2);
        ArrowPointCanvas * ap = ar->brk(p);

        p.setX(cb.x() -  ARROW_POINT_SIZE / 2);
        ap->get_other(ar)->brk(p);
    }
    break;
    }

    ar->propag_geometry(geo, fixed);
    view->freeze_history(FALSE);
    view->protect_history(TRUE);
    return ar;
}

// To move the arrow point to nx,ny but 'may be not exactly'
// to not go in an infinite loop between ArrowCanvas::update_geometry()
// and DiagramItem::update_show_lines() because when a extremity
// is moved this imply an update of the line attached to this point
// then the update of the geometry of the line, => a float computation
// problem may be dramatic

static void move_to(ArrowPointCanvas * ap, double nx, double ny)
{
    nx = nx - ap->x();
    ny = ny - ap->y();

    if ((fabs(nx) > 0.01) || (fabs(ny) > 0.01))
        ap->moveBy(nx, ny);
}

void ArrowCanvas::update_geometry()
{
    if (geometry == NoGeometry)
        return;

    static int done = 0;

    if (done == 3)
        // to avoid infinite loop in decenter case
        return;

    done += 1;

    const int half_arrow_size = ARROW_POINT_SIZE / 2;
    ArrowCanvas * first_segment = this;
    ArrowCanvas * last_segment = this;

    while (first_segment->begin->type() == UmlArrowPoint)
        first_segment = ((ArrowPointCanvas *) first_segment->begin)->get_other(first_segment);

    while (last_segment->end->type() == UmlArrowPoint)
        last_segment = ((ArrowPointCanvas *) last_segment->end)->get_other(last_segment);

    DiagramItem * a = first_segment->begin;
    DiagramItem * b = last_segment->end;

    if (a == b)
        return;

    QPoint ca = first_segment->beginp;
    QPoint cb = last_segment->endp;

    switch (geometry) {
    case HVGeometry:
    case VHrGeometry:
        move_to((ArrowPointCanvas *) first_segment->end,
                cb.x() - half_arrow_size,
                ca.y() - half_arrow_size);
        break;

    case HVrGeometry:
    case VHGeometry:
        move_to((ArrowPointCanvas *) first_segment->end,
                ca.x() - half_arrow_size,
                cb.y() - half_arrow_size);
        break;

    case HVHGeometry: {
        ArrowPointCanvas * ap1 = (ArrowPointCanvas *) first_segment->end;
        ArrowPointCanvas * ap2 = (ArrowPointCanvas *) last_segment->begin;

        if (fixed_geometry &&
            !a->isSelected() &&
            !b->isSelected() &&
            (ap1->selected() || ap2->selected()))
            // user moves just the middle line for the first time
            propag_geometry(geometry, FALSE);

        QRect arect = a->rect();
        QRect brect = b->rect();
        double nx;

        if (fixed_geometry)
            nx = ((ca.x() < cb.x()) ? (arect.right() + brect.left())
                  : (arect.left() + brect.right())) / 2
                 - half_arrow_size;
        else if (ap1->selected())
            // may be ap1 moved
            nx = ap1->x();
        else
            // may be ap2 moved
            nx = ap2->x();

        move_to(ap1, nx, ca.y() - half_arrow_size);

        move_to(ap2, ap1->x(), cb.y() - half_arrow_size);
    }
    break;

    default:
        // VHVGeometry
    {
        ArrowPointCanvas * ap1 = (ArrowPointCanvas *) first_segment->end;
        ArrowPointCanvas * ap2 = (ArrowPointCanvas *) last_segment->begin;

        if (fixed_geometry &&
            !a->isSelected() &&
            !b->isSelected() &&
            (ap1->selected() || ap2->selected()))
            // user moves just the middle line for the first time
            propag_geometry(geometry, FALSE);

        QRect arect = a->rect();
        QRect brect = b->rect();
        double ny;

        if (fixed_geometry)
            ny = ((ca.y() < cb.y()) ? (arect.bottom() + brect.top())
                  : (arect.top() + brect.bottom())) / 2
                 - half_arrow_size;
        else if (ap1->selected())
            // may be ap1 moved
            ny =  ap1->y();
        else
            // may be ap2 moved
            ny = ap2->y();

        move_to(ap1, ca.x() - half_arrow_size, ny);

        move_to(ap2, cb.x() - half_arrow_size, ap1->y());
    }
    break;
    }

    done -= 1;
}

ArrowCanvas * ArrowCanvas::next_geometry()
{
    if (get_start() != get_end()) {
        int geo = geometry;

        for (;;) {
            geo = (geo + 1) % GeometrySup;

            switch (geo) {
            case HVrGeometry:
            case VHrGeometry:
                break;

            default:
                return set_geometry((LineGeometry) geo, TRUE);
            }
        }
    }
    else
        return this;
}

void ArrowCanvas::select_associated()
{
    if (! selected()) {
        the_canvas()->select(this);

        if ((label != 0) && !label->selected())
            the_canvas()->select(label);

        if ((stereotype != 0) && !stereotype->selected())
            the_canvas()->select(stereotype);

        DiagramItem::select_associated();
        begin->select_associated();
        end->select_associated();
    }
}

void ArrowCanvas::prepare_for_move(bool)
{
    // the arrow is selected, select its labels to not
    // move them twice
    if ((label != 0) && !label->selected())
        the_canvas()->select(label);

    if ((stereotype != 0) && !stereotype->selected())
        the_canvas()->select(stereotype);
}

void ArrowCanvas::modified()
{
    if (visible()) {
        hide();
        update_pos();
        show();
        canvas()->update();
        package_modified();
    }
}

void ArrowCanvas::drawing_settings_modified()
{
    auto_pos = the_canvas()->browser_diagram()->get_auto_label_position();
}

void ArrowCanvas::save(QTextStream & st, bool ref, QString & warning) const
{
    if (ref)
        st << "line_ref " << get_ident();
    else if (begin->type() != UmlArrowPoint) {
        nl_indent(st);
        st << "line " << get_ident() << " " << stringify(itstype);

        if (geometry != NoGeometry) {
            st << " geometry " << stringify(geometry);

            if (!fixed_geometry)
                st << " unfixed";
        }

        if (decenter_begin >= 0) {
            // float output/input bugged
            st << " decenter_begin " << ((int)(decenter_begin * 1000));
        }

        if (decenter_end >= 0) {
            // float output/input bugged
            st << " decenter_end " << ((int)(decenter_end * 1000));
        }

        indent(+1);
        save_lines(st, TRUE, TRUE, warning);
        indent(-1);
    }
}

const ArrowCanvas * ArrowCanvas::save_lines(QTextStream & st, bool with_label,
        bool with_stereotype,
        QString & warning) const
{
    nl_indent(st);
    st << "from ref " << begin->get_ident();

    const ArrowCanvas * ar = this;
    QString zs;

    while (ar->end->type() == UmlArrowPoint) {
        st << " z " << zs.setNum(z());

        if (with_label && ar->label) {
            st << " ";
            ar->label->save(st, FALSE, warning);
        }

        if (with_stereotype && ar->stereotype) {
            st << " stereotype ";
            save_string(ar->stereotype->get_name(), st);
            save_xyz(st, ar->stereotype, " xyz");
        }

        st << " to ";
        ar->end->save(st, FALSE, warning);
        ar = ((ArrowPointCanvas *) ar->end)->get_other(ar);
        nl_indent(st);
        st << "line " << ar->get_ident();
    }

    st << " z " << zs.setNum(z());

    if (with_label && ar->label) {
        st << " ";
        ar->label->save(st, FALSE, warning);
    }

    if (with_stereotype && ar->stereotype) {
        st << " stereotype ";
        save_string(ar->stereotype->get_name(), st);
        save_xyz(st, ar->stereotype, " xyz");
    }

    st << " to ref " << ar->end->get_ident();

    return ar;
}

static ArrowCanvas * make(UmlCanvas * canvas, DiagramItem * b,
                          DiagramItem * e, UmlCode t, float dbegin, float dend, int id)
{
    return new ArrowCanvas(canvas, b, e, t, id, FALSE, dbegin, dend);
}

ArrowCanvas * ArrowCanvas::read_list(char *& st, UmlCanvas * canvas,
                                     UmlCode t, LineGeometry geo,
                                     bool fixed, float dbegin, float dend, int id,
                                     ArrowCanvas * (*pf)(UmlCanvas * canvas, DiagramItem * b,
                                             DiagramItem * e, UmlCode t,
                                             float dbegin, float dend, int id))
{
    char * k;

    read_keyword(st, "from");
    read_keyword(st, "ref");

    DiagramItem * bi = dict_get(read_id(st), "", canvas);
    ArrowCanvas * result;
    LabelCanvas * label;
    LabelCanvas * stereotype;

    for (;;) {
        double z;

        if (read_file_format() < 5)
            z = OLD_ARROW_Z;
        else {
            read_keyword(st, "z");
            z = read_double(st);
        }

        k = read_keyword(st);

        if ((label = LabelCanvas::read(st, canvas, k)) != 0) {
            label->setZ(z);
            k = read_keyword(st);
        }

        if (!strcmp(k, "stereotype")) {
            k = read_string(st);

            if (read_file_format() < 5)
                read_keyword(st, "xy");
            else
                read_keyword(st, "xyz");

            int x = (int) read_double(st);

            stereotype =
                new LabelCanvas(k, canvas, x, (int) read_double(st));

            stereotype->setZ((read_file_format() < 5) ? OLD_ARROW_Z
                             : read_double(st));
            k = read_keyword(st);
        }
        else
            stereotype = 0;

        if (strcmp(k, "to"))
            wrong_keyword(k, "to");

        DiagramItem * di;

        if (!strcmp(k = read_keyword(st), "ref"))
            di = dict_get(read_id(st), "", canvas);
        else {
            di = ArrowPointCanvas::read(st, canvas, k);

            if (di == 0)
                unknown_keyword(k);
        }

        result = pf(canvas, bi, di, t, dbegin, dend, id);
        result->geometry = geo;
        result->fixed_geometry = fixed;
        result->set_z(z);
        result->show();

        if (label != 0)
            (result->label = label)->show();

        if (stereotype != 0)
            (result->stereotype = stereotype)->show();

        if (di->type() != UmlArrowPoint)
            return result;

        bi = di;

        read_keyword(st, "line");
        id = read_id(st);
    }
}

ArrowCanvas * ArrowCanvas::read(char *& st, UmlCanvas * canvas, char * k)
{
    if (!strcmp(k, "line_ref"))
        return ((ArrowCanvas *) dict_get(read_id(st), "arrow canvas", canvas));
    else if (!strcmp(k, "line")) {
        int id = read_id(st);
        UmlCode t = arrow_type(read_keyword(st));
        LineGeometry geo;
        bool fixed;

        k = read_keyword(st);

        if (! strcmp(k, "geometry")) {
            geo = line_geometry(read_keyword(st));
            k = read_keyword(st);

            if (! strcmp(k, "unfixed")) {
                k = read_keyword(st);
                fixed = FALSE;
            }
            else
                fixed = TRUE;
        }
        else {
            geo = NoGeometry;
            fixed = FALSE;
        }

        float dbegin;
        float dend;

        if (! strcmp(k, "decenter_begin")) {
            dbegin = read_double(st) / 1000;
            k = read_keyword(st);
        }
        else
            dbegin = -1;

        if (! strcmp(k, "decenter_end")) {
            dend = read_double(st) / 1000;
            k = read_keyword(st);
        }
        else
            dend = -1;

        unread_keyword(k, st);

        ArrowCanvas * result = read_list(st, canvas, t, geo, fixed, dbegin, dend, id, &make);

        result->update_geometry();
        return result;
    }
    else
        return 0;
}

void ArrowCanvas::package_modified() const
{
    the_canvas()->browser_diagram()->package_modified();
}

void ArrowCanvas::history_save(QBuffer & b) const
{
    ::save(this, b);
    // must save begin etc .. because they may change
    // when an arrow point is added/removed
    ::save(begin, b);
    ::save(end, b);

    int v = geometry;

    ::save((fixed_geometry) ? v : -v, b);
    ::save(decenter_begin, b);
    ::save(decenter_end, b);
    ::save(label, b);
    ::save(stereotype, b);
    ::save(beginp, b);
    ::save(endp, b);
    ::save(boundings, b);
    ::save(arrow[0], b);
    ::save(arrow[1], b);
    ::save(arrow[2], b);
    ::save(poly, b);
    ::save(z(), b);
}

void ArrowCanvas::history_load(QBuffer & b)
{
    begin = load_item(b);
    end = load_item(b);

    int geo;

    ::load(geo, b);

    if (geo <= 0) {
        fixed_geometry = FALSE;
        geometry = (LineGeometry) - geo;
    }
    else {
        fixed_geometry = TRUE;
        geometry = (LineGeometry) geo;
    }

    ::load(decenter_begin, b);
    ::load(decenter_end, b);

    label = (LabelCanvas *) load_item(b);
    stereotype = (LabelCanvas *) load_item(b);
    ::load(beginp, b);
    ::load(endp, b);
    ::load(boundings, b);
    ::load(arrow[0], b);
    ::load(arrow[1], b);
    ::load(arrow[2], b);
    ::load(poly, b);
    setPoints(boundings);
    Q3CanvasPolygon::setZ(load_double(b));
    Q3CanvasItem::setSelected(FALSE);
    Q3CanvasItem::setVisible(TRUE);

    begin->add_line(this);
    end->add_line(this);
    connect(DrawingSettings::instance(), SIGNAL(changed()),
            this, SLOT(drawing_settings_modified()));
}

void ArrowCanvas::history_hide()
{
    Q3CanvasPolygon::setVisible(FALSE);
    unconnect();
}

//

QList<ArrowCanvas *> ArrowCanvas::RelsToDel;

void ArrowCanvas::post_load()
{
    foreach (ArrowCanvas *ar, RelsToDel)
        if (ar->visible())
            ar->delete_it();
    RelsToDel.clear();
}

// to remove redondant relation made by release 2.22
QList<ArrowCanvas *> ArrowCanvas::RelsToCheck;

void ArrowCanvas::remove_redondant_rels()
{
    // the key is <source address>_<browser node address>_<target address>
    QMap<QString, ArrowCanvas *> arrows;
    char s[128];

    foreach (ArrowCanvas *r, RelsToCheck) {
        if (r->visible()) {
            sprintf(s, "%lx_%lx_%lx",
                    (long) r->get_start(),
                    (long) r->get_bn(),
                    (long) r->get_end());

            if (arrows.find(s) != arrows.end())
                r->delete_it();
            else
                arrows.insert(s, r);
        }
    }
    RelsToCheck.clear();
}

// for plug-out

void ArrowCanvas::write_uc_rel(ToolCom * com) const
{
    // between use case an actor
    DiagramItem * from = get_start();
    DiagramItem * to = get_end();

    if (from->type() == UmlUseCase) {
        com->write_unsigned((unsigned) from->get_ident());
        to->get_bn()->write_id(com);
        com->write_bool((itstype == UmlDirectionalAssociation)
                        ? FALSE
                        : (to->rect().center().x() < from->rect().center().x()));
    }
    else {
        com->write_unsigned((unsigned) to->get_ident());
        from->get_bn()->write_id(com);
        com->write_bool((itstype == UmlDirectionalAssociation)
                        ? TRUE
                        : (to->rect().center().x() > from->rect().center().x()));
    }

    ArrowCanvas * plabel;
    ArrowCanvas * pstereotype;

    search_supports(plabel, pstereotype);

    WrapperStr s;

    if (plabel == 0)
        com->write_string("");
    else {
        s = fromUnicode(plabel->label->text());
        com->write_string(s);
    }

    if (pstereotype == 0)
        com->write_string("");
    else {
        s = fromUnicode(pstereotype->stereotype->text());
        com->write_string(s);
    }
}

//

void ArrowCanvas::check_stereotypeproperties()
{
    // does nothing
}
