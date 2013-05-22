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
#include <Q3PointArray>

#include "ActivityActionCanvas.h"
#include "ActivityActionData.h"
#include "BrowserActivityAction.h"
#include "BrowserActivityDiagram.h"
#include "BrowserParameterSet.h"
#include "ActivityContainerCanvas.h"
#include "ArrowCanvas.h"
#include "FlowCanvas.h"
#include "SimpleRelationCanvas.h"
#include "BrowserPin.h"
#include "PinCanvas.h"
#include "PinData.h"
#include "InfoCanvas.h"
#include "ParameterSetCanvas.h"
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

ActivityActionCanvas::ActivityActionCanvas(BrowserNode * bn, UmlCanvas * canvas,
        int x, int y)
    : DiagramCanvas(0, canvas, x, y, ACTIVITYACTION_MIN_SIZE * 5 / 2,
                    ACTIVITYACTION_MIN_SIZE * 3 / 2, 0)
{
    browser_node = bn;
    pre = 0;
    post = 0;
    constraint = 0;
    show_opaque_action_definition = UmlDefaultState;
    itscolor = UmlDefaultColor;

    update();

    check_pins();
    check_parametersets();
    check_conditions_constraint();
    check_stereotypeproperties();

    connect(bn->get_data(), SIGNAL(changed()), this, SLOT(modified()));
    connect(bn->get_data(), SIGNAL(deleted()), this, SLOT(deleted()));
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));

    if (canvas->must_draw_all_relations()) {
        draw_all_flows();
        draw_all_simple_relations();
    }
}

ActivityActionCanvas::ActivityActionCanvas(UmlCanvas * canvas, int id)
    : DiagramCanvas(canvas, id)
{
    // for read operation
    browser_node = 0;
    pre = 0;
    post = 0;
    constraint = 0;
    itscolor = UmlDefaultColor;
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
}

ActivityActionCanvas::~ActivityActionCanvas()
{
}

void ActivityActionCanvas::delete_it()
{
    disconnect(DrawingSettings::instance(), SIGNAL(changed()),
               this, SLOT(modified()));
    disconnect(browser_node->get_data(), 0, this, 0);

    DiagramCanvas::delete_it();

    Q3ValueList<PinCanvas *>::Iterator iter;

    for (iter = pins.begin(); iter != pins.end(); ++iter)
        // don't empty pins to manage undo
        (*iter)->delete_it();

    Q3ValueList<ParameterSetCanvas *>::Iterator iterps = paramsets.begin();

    while (! paramsets.isEmpty()) {
        (*iterps)->delete_it();
        iterps = paramsets.remove(iterps);
    }

    if (pre != 0)
        pre->delete_it();

    if (post != 0)
        post->delete_it();

    if (constraint != 0)
        constraint->delete_it();
}

void ActivityActionCanvas::deleted()
{
    delete_it();
    canvas()->update();
}

void ActivityActionCanvas::deleted(PinCanvas * pa)
{
    pins.remove(pa);
}

void ActivityActionCanvas::deleted(ParameterSetCanvas * ps)
{
    paramsets.remove(ps);
}

void ActivityActionCanvas::remove(bool from_model)
{
    if (! from_model)
        delete_it();
    else
        browser_node->delete_it();	// will remove canvas
}

void ActivityActionCanvas::update()
{
    const ActivityActionData * data =
        (ActivityActionData *) browser_node->get_data();

    used_color = (itscolor == UmlDefaultColor)
                 ? the_canvas()->browser_diagram()->get_color(UmlActivityAction)
                 : itscolor;

    BrowserActivityDiagram * diagram =
        (BrowserActivityDiagram *) the_canvas()->browser_diagram();

    used_settings = settings;
    diagram->get_activitydrawingsettings(used_settings);

    bool opaquedef = ((data->get_action_kind() == UmlOpaqueAction) &&
                      ((show_opaque_action_definition == UmlDefaultState)
                       ? diagram->get_show_opaque_action_definition()
                       : (show_opaque_action_definition == UmlYes)));

    s = data->str((opaquedef) ? used_settings.drawing_language
                  : DefaultDrawingLanguage);
    s = s.stripWhiteSpace();

    QString st = data->get_short_stereotype();

    if (!st.isEmpty() && !opaquedef) {
        s += "\n<<" + st + ">>";
        align = (int) ::Qt::AlignCenter | ::Qt::TextWordWrap;
    }
    else
        align = (s.find('\n') != -1)
                ? (int) ::Qt::AlignLeft | ::Qt::AlignVCenter
                : (int) ::Qt::AlignCenter | ::Qt::TextWordWrap;

    force_pins_arround();
    check_parameter_sets_position();
}

bool ActivityActionCanvas::force_inside()
{
    // if its parent is present, force inside it

    Q3CanvasItemList all = the_canvas()->allItems();
    Q3CanvasItemList::Iterator cit;
    BrowserNode * parent = (BrowserNode *) browser_node->parent();

    for (cit = all.begin(); cit != all.end(); ++cit) {
        if ((*cit)->visible()) {
            DiagramItem * di = QCanvasItemToDiagramItem(*cit);

            if ((di != 0) &&
                IsaActivityContainer(di->type()) &&
                (((ActivityContainerCanvas *) di)->get_bn() == parent)) {
                ((ActivityContainerCanvas *) di)->force_inside(this, this);
                return TRUE;
            }
        }
    }

    return FALSE;
}

void ActivityActionCanvas::moveBy(double dx, double dy)
{
    DiagramCanvas::moveBy(dx, dy);

    // update pins position
    Q3ValueList<PinCanvas *>::Iterator iter;

    for (iter = pins.begin(); iter != pins.end(); ++iter)
        (*iter)->do_moveBy(dx, dy);

    // update parameter sets position
    Q3ValueList<ParameterSetCanvas *>::Iterator iterps;

    for (iterps = paramsets.begin(); iterps != paramsets.end(); ++iterps)
        (*iterps)->do_moveBy(dx, dy);

    // update conditions
    if ((pre != 0) && !pre->selected())
        pre->moveBy(dx, dy);

    if ((post != 0) && !post->selected())
        post->moveBy(dx, dy);

    if ((constraint != 0) && !constraint->selected())
        constraint->moveBy(dx, dy);
}

void ActivityActionCanvas::check_parameter_sets_position()
{
    Q3ValueList<ParameterSetCanvas *>::Iterator iterps;

    for (iterps = paramsets.begin(); iterps != paramsets.end(); ++iterps)
        (*iterps)->check_position();
}

void ActivityActionCanvas::change_scale()
{
    Q3CanvasRectangle::setVisible(FALSE);
    double scale = the_canvas()->zoom();

    setSize((int)(width_scale100 * scale) | 1,
            (int)(height_scale100 * scale) | 1);
    recenter();
    Q3CanvasRectangle::setVisible(TRUE);

    // update pins position
    Q3ValueList<PinCanvas *>::Iterator iter;

    for (iter = pins.begin(); iter != pins.end(); ++iter)
        (*iter)->do_change_scale();

    // update paramsets position
    check_parameter_sets_position();
}

void ActivityActionCanvas::modified()
{
    hide();
    hide_lines();
    update();
    show();
    update_show_lines();
    force_pins_arround();
    check_pins();
    check_parametersets();

    if (the_canvas()->must_draw_all_relations()) {
        draw_all_flows();
        draw_all_simple_relations();
    }

    check_conditions_constraint();
    check_stereotypeproperties();
    canvas()->update();
    package_modified();
}

void ActivityActionCanvas::post_loaded()
{
    force_pins_arround();

    if (the_canvas()->must_draw_all_relations()) {
        draw_all_flows();
        draw_all_simple_relations();
    }
}

aCorner ActivityActionCanvas::on_resize_point(const QPoint & p)
{
    return ::on_resize_point(p, rect());
}

void ActivityActionCanvas::resize(aCorner c, int dx, int dy, QPoint & o)
{
    double zoom = the_canvas()->zoom();

    DiagramCanvas::resize(c, dx, dy, o,
                          (int)(ACTIVITYACTION_MIN_SIZE * zoom),
                          (int)(ACTIVITYACTION_MIN_SIZE * zoom));

    force_pins_arround();
}

void ActivityActionCanvas::resize(const QSize & sz, bool w, bool h)
{
    double zoom = the_canvas()->zoom();

    if (DiagramCanvas::resize(sz, w, h,
                              (int)(ACTIVITYACTION_MIN_SIZE * zoom),
                              (int)(ACTIVITYACTION_MIN_SIZE * zoom)))
        force_pins_arround();
}

bool ActivityActionCanvas::move_with_its_package() const
{
    return TRUE;
}

void ActivityActionCanvas::set_z(double z)
{
    setZ(z);

    Q3ValueList<ParameterSetCanvas *>::Iterator iterps;

    for (iterps = paramsets.begin(); iterps != paramsets.end(); ++iterps)
        (*iterps)->set_z(z);

    // pins have z+1 to be upper the parameter sets
    z += 1;

    Q3ValueList<PinCanvas *>::Iterator iter;

    for (iter = pins.begin(); iter != pins.end(); ++iter)
        (*iter)->set_z(z);
}

void ActivityActionCanvas::force_pins_arround()
{
    Q3ValueList<PinCanvas *>::Iterator iter;

    for (iter = pins.begin(); iter != pins.end(); ++iter)
        (*iter)->check_position();
}

void ActivityActionCanvas::check_parametersets()
{
    // add missing parameter sets
    const Q3ValueList<BrowserParameterSet *> brps =
        ((BrowserActivityAction *) browser_node)->get_parametersets();
    Q3ValueList<BrowserParameterSet *>::ConstIterator iter;

    for (iter = brps.begin(); iter != brps.end(); ++iter) {
        Q3ValueList<ParameterSetCanvas *>::ConstIterator itershown;

        for (itershown = paramsets.begin(); itershown != paramsets.end(); ++itershown)
            if ((*itershown)->get_bn() == *iter)
                break;

        if (itershown == paramsets.end()) {
            // add it
            ParameterSetCanvas * psc =
                new ParameterSetCanvas(*iter, the_canvas(), 0, this);

            paramsets.append(psc);
            psc->update();
            psc->show();
        }
    }
}

void ActivityActionCanvas::check_pins()
{
    // add missing pins
    const Q3ValueList<BrowserPin *> brpins =
        ((BrowserActivityAction *) browser_node)->get_pins();
    Q3ValueList<BrowserPin *>::ConstIterator iter;
    double zoom = the_canvas()->zoom();
    int sz = (int)(zoom * PIN_SIZE);
    int six = (int)(zoom * 6);
    int xl = (int) x() - sz;
    int xr = rect().right();
    int ymin = (int) y() + six;
    int ymax = (int) y() + height() - six - sz;
    int yr = ymin;
    int yl = ymin;

    sz += six;

    for (iter = brpins.begin(); iter != brpins.end(); ++iter) {
        Q3ValueList<PinCanvas *>::ConstIterator itershown;

        for (itershown = pins.begin(); itershown != pins.end(); ++itershown)
            if ((*itershown)->get_bn() == *iter)
                break;

        if (itershown == pins.end()) {
            // add it
            PinCanvas * pc;

            if (((PinData *)(*iter)->get_data())->get_dir() ==  UmlIn) {
                pc = new PinCanvas(*iter, the_canvas(), xl, yl, 0, this);
                yl = (yl + sz);

                if (yl > ymax)
                    yl = ymin + 2;
            }
            else {
                pc = new PinCanvas(*iter, the_canvas(), xr, yr, 0, this);
                yr = (yr + sz);

                if (yr > ymax)
                    yr = ymin + 2;
            }

            pins.append(pc);
            pc->update();
            pc->show();
        }
    }
}

void ActivityActionCanvas::check_conditions_constraint()
{
    // update must be called before
    if (used_settings.show_infonote == UmlYes) {
        ActivityActionData * data = (ActivityActionData *) browser_node->get_data();
        int margin = (int)(the_canvas()->zoom() * 15);
        QString s;

        s = data->get_precond(used_settings.drawing_language);

        if (s.isEmpty()) {
            if (pre != 0) {
                pre->delete_it();
                pre = 0;
            }
        }
        else {
            s = "<<localPrecondition>>\n" + s;

            if (pre == 0) {
                pre = new InfoCanvas(the_canvas(), this, s);
                pre->upper();

                int cdx = (int) x() - margin;
                int cdy = (int) y() - pre->height() - margin;

                pre->move((cdx < 0) ? 0 : cdx, (cdy < 0) ? 0 : cdy);
                pre->show();
                (new ArrowCanvas(the_canvas(), this, pre, UmlAnchor, 0, FALSE, -1.0, -1.0))
                ->show();
            }
            else
                pre->set(s);
        }

        s = data->get_postcond(settings.drawing_language);

        if (s.isEmpty()) {
            if (post != 0) {
                post->delete_it();
                post = 0;
            }
        }
        else {
            s = "<<localPostcondition>>\n" + s;

            if (post == 0) {
                post = new InfoCanvas(the_canvas(), this, s);
                post->upper();
                post->move(x() + margin, y() + height() + margin);
                post->show();
                (new ArrowCanvas(the_canvas(), this, post, UmlAnchor, 0, FALSE, -1.0, -1.0))
                ->show();
            }
            else
                post->set(s);
        }

        s = data->get_constraint();

        if (s.isEmpty()) {
            if (constraint != 0) {
                constraint->delete_it();
                constraint = 0;
            }
        }
        else {
            if (constraint == 0) {
                constraint = new InfoCanvas(the_canvas(), this, s);
                constraint->upper();
                constraint->move(x() + width() + margin, y());
                constraint->show();
                (new ArrowCanvas(the_canvas(), this, constraint, UmlAnchor, 0, FALSE, -1.0, -1.0))
                ->show();
            }
            else
                constraint->set(s);
        }
    }
    else {
        if (pre != 0) {
            pre->delete_it();
            pre = 0;
        }

        if (post != 0) {
            post->delete_it();
            post = 0;
        }

        if (constraint != 0) {
            constraint->delete_it();
            constraint = 0;
        }
    }
}

int ActivityActionCanvas::shadow_margin() const
{
    return (used_color == UmlTransparent)
           ? 0
           : the_canvas()->shadow();
}

void ActivityActionCanvas::draw(QPainter & p)
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

    const ActivityActionData * data =
        (ActivityActionData *) browser_node->get_data();
    const int shadow = the_canvas()->shadow();
    int margin;
    FILE * fp = svg();

    if (fp != 0)
        fputs("<g>\n", fp);

    switch (data->get_action_kind()) {
    case UmlAcceptEventAction:
        if (((AcceptEventAction *) data->get_action())->timeevent) {
            // don't draw shadow
            margin = (int)(21 * the_canvas()->zoom());

            int t = (r.y() + r.bottom() - margin) / 2;

            p.setPen(::Qt::SolidLine);
            p.setBackgroundMode(::Qt::TransparentMode);
            p.drawLine(r.right() - margin, t, r.right() - 1, t);
            //p.lineTo(r.right() - margin - 1, t + margin);
            p.drawLine(r.right() - 1, t, r.right() - margin - 1, t + margin);
            //p.lineTo(r.right() - 1, t + margin);
            p.drawLine(r.right() - margin - 1, t + margin, r.right() - 1, t + margin);
            //p.lineTo(r.right() - margin - 1, t);
            p.drawLine(r.right() - 1, t + margin, r.right() - margin - 1, t);

            if (fp != 0) {
                fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                        " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                        r.right() - margin, t, r.right() - 1, t);
                fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                        " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                        r.right() - 1, t, r.right() - margin - 1, t + margin);
                fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                        " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                        r.right() - margin - 1, t + margin, r.right() - 1, t + margin);
                fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                        " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                        r.right() - 1, t + margin, r.right() - margin - 1, t);
            }

            r.setWidth(r.width() - margin - 1);
            margin = (int)(3 * the_canvas()->zoom());
        }
        else {
            if ((used_color != UmlTransparent) && (shadow != 0)) {
                r.setRight(r.right() - shadow);
                r.setBottom(r.bottom() - shadow);
            }

            margin = ((r.height() < r.width()) ? r.height() : r.width()) / 4;

            Q3PointArray a(6);

            a.setPoint(0, r.x(), r.y());
            a.setPoint(1, r.right(), r.y());
            a.setPoint(2, r.right(), r.bottom());
            a.setPoint(3, r.x(), r.bottom());
            a.setPoint(4, r.x() + margin, (r.y() + r.bottom()) / 2);
            a.setPoint(5, r.x(), r.y());

            if (used_color == UmlTransparent) {
                p.drawPolyline(a);

                if (fp != 0)
                    draw_poly(fp, a, UmlTransparent);
            }
            else {
                if (shadow != 0) {
                    Q3PointArray b(6);

                    b.setPoint(0, r.x() + shadow, r.y() + shadow);
                    b.setPoint(1, r.right() + shadow, r.y() + shadow);
                    b.setPoint(2, r.right() + shadow, r.bottom() + shadow);
                    b.setPoint(3, r.x() + shadow, r.bottom() + shadow);
                    b.setPoint(4, r.x() + margin + shadow, (r.y() + r.bottom()) / 2 + shadow);
                    b.setPoint(5, r.x() + shadow, r.y() + shadow);
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

            r.setLeft(r.left() + margin);
            margin = (int)(6 * the_canvas()->zoom());
        }

        break;

    case UmlSendSignalAction:
    case UmlBroadcastSignalAction: {
        if ((used_color != UmlTransparent) && (shadow != 0)) {
            r.setRight(r.right() - shadow);
            r.setBottom(r.bottom() - shadow);
        }

        margin = ((r.height() < r.width()) ? r.height() : r.width()) / 4;

        Q3PointArray a(6);

        a.setPoint(0, r.x(), r.y());
        a.setPoint(1, r.right() - margin, r.y());
        a.setPoint(2, r.right(), (r.y() + r.bottom()) / 2);
        a.setPoint(3, r.right() - margin, r.bottom());
        a.setPoint(4, r.x(), r.bottom());
        a.setPoint(5, r.x(), r.y());

        if (used_color == UmlTransparent) {
            p.drawPolyline(a);

            if (fp != 0)
                draw_poly(fp, a, UmlTransparent);
        }
        else {
            if (shadow != 0) {
                Q3PointArray b(6);

                b.setPoint(0, r.x() + shadow, r.y() + shadow);
                b.setPoint(1, r.right() - margin + shadow, r.y() + shadow);
                b.setPoint(2, r.right() + shadow, (r.y() + r.bottom()) / 2 + shadow);
                b.setPoint(3, r.right() - margin + shadow, r.bottom() + shadow);
                b.setPoint(4, r.x() + shadow, r.bottom() + shadow);
                b.setPoint(5, r.x() + shadow, r.y() + shadow);
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

        r.setWidth(r.width() - margin);
        margin = (int)(6 * the_canvas()->zoom());
    }
    break;

    default:
        margin = (int)(9 * the_canvas()->zoom());

        if ((used_color != UmlTransparent) && (shadow != 0)) {
            r.setRight(r.right() - shadow);
            r.setBottom(r.bottom() - shadow);
            p.setPen(::Qt::NoPen);
            p.setBrush(::Qt::darkGray);
            p.drawRoundRect(r.left() + shadow, r.top() + shadow, r.width(), r.height());

            if (fp != 0)
                fprintf(fp, "\t<rect fill=\"#%06x\" stroke=\"none\" stroke-opacity=\"1\""
                        " x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" rx=\"10\" />\n",
                        QColor(::Qt::darkGray).rgb() & 0xffffff,
                        r.left() + shadow, r.top() + shadow, r.width() - 1, r.height() - 1);

            p.setPen(::Qt::SolidLine);
        }

        p.setBrush(co);
        p.drawRoundRect(r);

        if (fp != 0)
            fprintf(fp, "\t<rect fill=\"%s\" stroke=\"black\" stroke-opacity=\"1\""
                    " x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" rx=\"10\" />\n",
                    svg_color(used_color),
                    r.left(), r.top(), r.width() - 1, r.height() - 1);

        if (data->get_action_kind() == UmlCallBehaviorAction) {
            BrowserNode * behavior =
                ((CallBehaviorAction *) data->get_action())->behavior;

            if ((behavior != 0) && (behavior->get_type() == UmlActivity)) {
                int l = (int)(6 * the_canvas()->zoom());
                int lx = r.right() - margin - l;
                int ty = r.bottom() - margin - l;
                int mx = lx + l;
                int my = ty + l;
                int rx = mx + l;
                int by = my + l;

                p.drawLine(lx, my, rx, my);
                p.drawLine(mx, ty, mx, by);
                p.drawLine(lx, my, lx, by);
                p.drawLine(rx, my, rx, by);

                if (fp != 0) {
                    fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                            " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                            lx, my, rx, my);
                    fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                            " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                            mx, ty, mx, by);
                    fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                            " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                            lx, my, lx, by);
                    fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                            " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                            rx, my, rx, by);
                }
            }
        }

        break;
    }

    p.setFont(the_canvas()->get_font(UmlNormalFont));
    p.drawText(r.x() + margin,
               r.y() + margin,
               r.width() - margin - margin,
               r.height() - margin - margin,
               align, s);

    if (fp != 0) {
        fputs("</g>\n", fp);
        draw_text(r.x() + margin,
                  r.y() + margin,
                  r.width() - margin - margin,
                  r.height() - margin - margin,
                  align, s, p.font(), fp);
    }

    p.setBackgroundColor(bckgrnd);
    p.setBrush(brsh);

    if (selected())
        show_mark(p, rect());
}

UmlCode ActivityActionCanvas::type() const
{
    return UmlActivityAction;
}

void ActivityActionCanvas::delete_available(BooL & in_model, BooL & out_model) const
{
    out_model |= TRUE;
    in_model |= browser_node->is_writable();
}

bool ActivityActionCanvas::alignable() const
{
    return TRUE;
}

bool ActivityActionCanvas::copyable() const
{
    return selected();
}

void ActivityActionCanvas::open()
{
    browser_node->open(FALSE);
}

void ActivityActionCanvas::menu(const QPoint &)
{
    ActivityActionData * data = (ActivityActionData *) browser_node->get_data();
    Q3PopupMenu m(0);
    Q3PopupMenu toolm(0);

    MenuFactory::createTitle(m, data->definition(FALSE, TRUE));
    m.insertSeparator();

    if (browser_node->is_writable() && data->may_add_pin()) {
        m.insertItem(TR("Add pin"), 7);
        m.insertSeparator();
    }

    m.insertItem(TR("Upper"), 0);
    m.insertItem(TR("Lower"), 1);
    m.insertItem(TR("Go up"), 13);
    m.insertItem(TR("Go down"), 14);
    m.insertSeparator();
    m.insertItem(TR("Edit drawing settings"), 2);
    m.insertSeparator();
    m.insertItem(TR("Edit activity action"), 3);
    m.insertSeparator();
    m.insertItem(TR("Select in browser"), 4);

    const char * what;
    BrowserNode * who = data->get_action()->referenced(what);
    BrowserNode * diag = 0;

    if (who != 0)
        m.insertItem(TR("Select %1 in browser", QString(what)), 10);

    if (linked())
        m.insertItem(TR("Select linked items"), 5);

    m.insertSeparator();

    if (browser_node->is_writable()) {
        if (browser_node->get_associated() !=
            (BrowserNode *) the_canvas()->browser_diagram())
            m.insertItem(TR("Set associated diagram"), 6);

        if (data->get_action_kind() == UmlCallBehaviorAction) {
            BasicData * d = data->get_action()->depend_on();

            if (d != 0) {
                diag = d->get_browser_node()->get_associated();

                if ((diag != 0) &&
                    (diag != (BrowserNode *) the_canvas()->browser_diagram()))
                    m.insertItem(TR("Set associated diagram from behavior"), 11);
            }
        }

        if (browser_node->get_associated())
            m.insertItem(TR("Remove diagram association"), 12);
    }

    m.insertSeparator();
    m.insertItem(TR("Remove from diagram"), 8);

    if (browser_node->is_writable())
        m.insertItem(TR("Delete from model"), 9);

    m.insertSeparator();

    if (Tool::menu_insert(&toolm, UmlActivityAction, 20))
        m.insertItem(TR("Tool"), &toolm);

    int index;

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
        ((BrowserActivityAction *) browser_node)
        ->set_associated_diagram(the_canvas()->browser_diagram());
        return;

    case 12:
        ((BrowserActivityAction *) browser_node)
        ->set_associated_diagram(0);
        return;

    case 7:
        if (BrowserPin::add_pin(0, browser_node) != 0)
            modified();	// call package_modified()

        break;

    case 8:
        //remove from diagram
        delete_it();
        break;

    case 9:
        //delete from model
        browser_node->delete_it();	// will delete the canvas
        break;

    case 10:
        who->select_in_browser();
        return;

    case 11:
        ((BrowserActivityAction *) browser_node)->set_associated_diagram(diag);
        return;

    default:
        if (index >= 20)
            ToolCom::run(Tool::command(index - 20), browser_node);

        return;
    }
}

void ActivityActionCanvas::apply_shortcut(QString s)
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

void ActivityActionCanvas::edit_drawing_settings()
{
    for (;;) {
        StateSpecVector st(1);
        ColorSpecVector co(1);

        st[0].set(TR("show opaque definition"), &show_opaque_action_definition);
        settings.complete(st, TRUE);

        co[0].set(TR("action color"), &itscolor);

        SettingsDialog dialog(&st, &co, FALSE);

        dialog.raise();

        if (dialog.exec() == QDialog::Accepted)
            modified();	// call package_modified()

        if (!dialog.redo())
            break;
    }
}

bool ActivityActionCanvas::has_drawing_settings() const
{
    return TRUE;
}

void ActivityActionCanvas::edit_drawing_settings(QList<DiagramItem *> & l)
{
    for (;;) {
        StateSpecVector st(1);
        ColorSpecVector co(1);
        Uml3States show_opaque_action_definition;
        UmlColor itscolor;
        ActivityDrawingSettings settings;

        st[0].set(TR("show opaque definition"), &show_opaque_action_definition);
        settings.complete(st, TRUE);

        co[0].set(TR("action color"), &itscolor);

        SettingsDialog dialog(&st, &co, FALSE, TRUE);

        dialog.raise();

        if (dialog.exec() == QDialog::Accepted) {
            foreach (DiagramItem *item, l) {
                ActivityActionCanvas *canvas = (ActivityActionCanvas *)item;
                if (!st[0].name.isEmpty())
                    canvas->show_opaque_action_definition =
                        show_opaque_action_definition;

                if (!co[0].name.isEmpty())
                    canvas->itscolor = itscolor;

                canvas->settings.set(st, 1);
                canvas->modified();
            }
        }

        if (!dialog.redo())
            break;
    }
}

void ActivityActionCanvas::clone_drawing_settings(const DiagramItem *src)
{
    ActivityActionCanvas *srcCanvas = (ActivityActionCanvas *)src;
    show_opaque_action_definition = srcCanvas->show_opaque_action_definition;
    itscolor = srcCanvas->itscolor;
    settings = srcCanvas->settings;
    modified();
}

bool ActivityActionCanvas::get_show_stereotype_properties() const
{
    switch (settings.show_stereotype_properties) {
    case UmlYes:
        return TRUE;

    case UmlNo:
        return FALSE;

    default:
        return the_canvas()->browser_diagram()->get_show_stereotype_properties();
    }
}

QString ActivityActionCanvas::may_start(UmlCode & l) const
{
    return (l == UmlFlow)
           ? ((BrowserActivityAction *) browser_node)->may_start()
           : QString();
}

QString ActivityActionCanvas::may_connect(UmlCode & l, const DiagramItem * dest) const
{
    if (l == UmlAnchor)
        return dest->may_start(l);
    else if (dest->get_bn() == 0)
        return TR("illegal");
    else
        return ((BrowserActivityAction *) browser_node)->may_connect(l, dest->get_bn());
}

void ActivityActionCanvas::connexion(UmlCode action, DiagramItem * dest,
                                     const QPoint &, const QPoint &)
{
    ArrowCanvas * a;

    if (action == UmlFlow)
        a = new FlowCanvas(the_canvas(), this, dest, 0, 0, -1.0, -1.0);
    else if (IsaSimpleRelation(action))
        a = new SimpleRelationCanvas(the_canvas(), this, dest, 0, action, 0, -1.0, -1.0);
    else
        a = new ArrowCanvas(the_canvas(), this, dest, action, 0, FALSE, -1.0, -1.0);

    a->show();
    the_canvas()->select(a);
}

void ActivityActionCanvas::save(QTextStream & st, bool ref, QString & warning) const
{
    if (ref) {
        st << "activityactioncanvas_ref " << get_ident() << " // "
           << browser_node->full_name();
    }
    else {
        nl_indent(st);
        st << "activityactioncanvas " << get_ident() << " ";
        browser_node->save(st, TRUE, warning);
        indent(+1);
        settings.save(st);
        nl_indent(st);
        st << "show_opaque_action_definition " << stringify(show_opaque_action_definition);

        if (itscolor != UmlDefaultColor) {
            nl_indent(st);
            st << "color " << stringify(itscolor);
        }

        nl_indent(st);
        save_xyzwh(st, this, "xyzwh");

        if (! pins.isEmpty()) {
            nl_indent(st);
            st << "pins";
            indent(+1);

            Q3ValueList<PinCanvas *>::ConstIterator iter;

            for (iter = pins.begin(); iter != pins.end(); ++iter)
                (*iter)->save(st, FALSE, warning);

            indent(-1);
            nl_indent(st);
            st << "end";
        }

        if (! paramsets.isEmpty()) {
            nl_indent(st);
            st << "parameter_sets";
            indent(+1);

            Q3ValueList<ParameterSetCanvas *>::ConstIterator iterps;

            for (iterps = paramsets.begin(); iterps != paramsets.end(); ++iterps)
                (*iterps)->save(st, FALSE, warning);

            indent(-1);
            nl_indent(st);
            st << "end";
        }

        if (pre != 0) {
            nl_indent(st);
            st << "pre ";
            pre->save(st, FALSE, warning);
        }

        if (post != 0) {
            nl_indent(st);
            st << "post ";
            post->save(st, FALSE, warning);
        }

        if (constraint != 0) {
            nl_indent(st);
            st << "constraint ";
            constraint->save(st, FALSE, warning);
        }

        save_stereotype_property(st, warning);

        indent(-1);
        nl_indent(st);
        st << "end";
    }
}

ActivityActionCanvas * ActivityActionCanvas::read(char *& st, UmlCanvas * canvas,
        char * k)
{
    if (!strcmp(k, "activityactioncanvas_ref"))
        return ((ActivityActionCanvas *) dict_get(read_id(st), "activitycanvas", canvas));
    else if (!strcmp(k, "activityactioncanvas")) {
        int id = read_id(st);
        BrowserActivityAction * br = BrowserActivityAction::read_ref(st);
        ActivityActionCanvas * result = new ActivityActionCanvas(canvas, id);

        result->browser_node = br;
        connect(br->get_data(), SIGNAL(changed()), result, SLOT(modified()));
        connect(br->get_data(), SIGNAL(deleted()), result, SLOT(deleted()));

        k = read_keyword(st);

        result->settings.read(st, k);	// updates k

        if (! strcmp(k, "show_opaque_action_definition")) {
            result->show_opaque_action_definition = state(read_keyword(st));
            k = read_keyword(st);
        }

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

        if (! strcmp(k, "pins")) {
            while (strcmp(k = read_keyword(st), "end") != 0)
                result->pins.append(PinCanvas::read(st, canvas, k, result));

            k = read_keyword(st);
        }

        if (! strcmp(k, "parameter_sets")) {
            while (strcmp(k = read_keyword(st), "end") != 0)
                result->paramsets.append(ParameterSetCanvas::read(st, canvas, k, result));

            k = read_keyword(st);
        }

        if (! strcmp(k, "pre")) {
            k = read_keyword(st);
            result->pre = InfoCanvas::read(st, canvas, k, result);
            k = read_keyword(st);
        }

        if (! strcmp(k, "post")) {
            k = read_keyword(st);
            result->post = InfoCanvas::read(st, canvas, k, result);
            k = read_keyword(st);
        }

        if (! strcmp(k, "constraint")) {
            k = read_keyword(st);
            result->constraint = InfoCanvas::read(st, canvas, k, result);
            k = read_keyword(st);
        }

        result->read_stereotype_property(st, k);	// updates k

        if (strcmp(k, "end"))
            wrong_keyword(k, "end");

        result->check_pins();
        result->check_parametersets();
        result->check_conditions_constraint();
        result->check_stereotypeproperties();

        if (canvas->paste())
            result->remove_if_already_present();

        return result;
    }
    else
        return 0;
}

void ActivityActionCanvas::history_save(QBuffer & b) const
{
    DiagramCanvas::history_save(b);
    ::save(width_scale100, b);
    ::save(height_scale100, b);
    ::save(width(), b);
    ::save(height(), b);
}

void ActivityActionCanvas::history_load(QBuffer & b)
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

void ActivityActionCanvas::history_hide()
{
    Q3CanvasItem::setVisible(FALSE);

    disconnect(DrawingSettings::instance(), SIGNAL(changed()),
               this, SLOT(modified()));
    disconnect(browser_node->get_data(), 0, this, 0);
}

