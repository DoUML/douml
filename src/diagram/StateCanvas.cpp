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

#include "StateCanvas.h"
#include "TransitionCanvas.h"
#include "SimpleRelationCanvas.h"
#include "StateData.h"
#include "BrowserState.h"
#include "BrowserStateDiagram.h"
#include "BrowserRegion.h"
#include "UmlPixmap.h"
#include "UmlGlobal.h"
#include "SettingsDialog.h"
#include "myio.h"
#include "ToolCom.h"
#include "Tool.h"
#include "ui/menufactory.h"
#include "Settings.h"
#include "ArrowPointCanvas.h"
#include "strutil.h"
#include "translate.h"

StateCanvas::StateCanvas(BrowserNode * bn, UmlCanvas * canvas,
                         int x, int y)
    : DiagramCanvas(0, canvas, x, y, STATE_CANVAS_MIN_SIZE, 1, 0)
{
    browser_node = bn;
    itscolor = UmlDefaultColor;
    show_decomp_icon = FALSE;

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

StateCanvas::StateCanvas(UmlCanvas * canvas, int id)
    : DiagramCanvas(canvas, id)
{
    // for read operation
    browser_node = 0;
    itscolor = UmlDefaultColor;
    show_decomp_icon = FALSE;
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
}

StateCanvas::~StateCanvas()
{
}

void StateCanvas::delete_it()
{
    disconnect(DrawingSettings::instance(), SIGNAL(changed()),
               this, SLOT(modified()));
    disconnect(browser_node->get_data(), 0, this, 0);

    MultipleDependencyIterator<BasicData> it(this);

    while (it.current()) {
        disconnect(it.current(), 0, this, 0);
        ++it;
    }

    unsubscribe_all();

    DiagramCanvas::delete_it();
}

void StateCanvas::deleted()
{
    delete_it();
    canvas()->update();
}

void StateCanvas::remove(bool from_model)
{
    if (! from_model)
        delete_it();
    else
        browser_node->delete_it();	// will remove canvas
}

void StateCanvas::compute_size()
{
    double zoom = the_canvas()->zoom();
    QFontMetrics fm(the_canvas()->get_font(UmlNormalBoldFont));
    const StateData * data = (StateData *) browser_node->get_data();
    BrowserState * ref = data->get_reference();
    StateDrawingSettings st = settings;

    // does not unsubscribe & disconnect signals because may be called
    // during a signal management, and the signal connection list
    // cannot be modified in this case
    if ((ref != 0) && subscribe(ref->get_data()))
        connect(ref->get_data(), SIGNAL(changed()), this, SLOT(modified()));

    ((BrowserStateDiagram *) the_canvas()->browser_diagram())->get_statedrawingsettings(st);
    region_horizontally = (st.region_horizontally == UmlYes);

    min_height = 2 * fm.height();

    if (ref != 0) {
        min_height = 3 * fm.height();
        min_width = fm.width(browser_node->get_name() + QString(":"));

        int w = fm.width(ref->get_name());

        if (min_width < w)
            min_width = w;
    }
    else {
        min_height = 2 * fm.height();
        min_width = fm.width(browser_node->get_name());
    }

    if (data->get_stereotype()[0] != 0) {
        int w = fm.width(QString("<<") + toUnicode(data->get_short_stereotype()) + ">>");

        if (min_width < w)
            min_width = w;

        min_height += fm.height();
    }

    activities = QString();

    if ((ref == 0) && (st.show_activities == UmlYes)) {
        const StateBehavior & behavior =
            ((StateData *) browser_node->get_data())
            ->get_behavior(st.drawing_language);
        QString sep;

        if (! behavior.on_entry.isEmpty()) {
            activities = "entry / " + behavior.on_entry;
            sep = "\n";
        }

        if (! behavior.on_exit.isEmpty()) {
            activities += sep + "exit / " + behavior.on_exit;
            sep = "\n";
        }

        if (! behavior.do_activity.isEmpty())
            activities += sep + "do / " + behavior.do_activity;

        if (!activities.isEmpty()) {
            activities = toUnicode(activities);

            QSize sz = fm.size(0, activities);

            min_height += fm.height() + sz.height();

            int w = sz.width() + (int)(16 * zoom);

            if (min_width < w)
                min_width = w;
        }
    }

    min_width += (int)(16 * zoom);

    used_color = (itscolor == UmlDefaultColor)
                 ? the_canvas()->browser_diagram()->get_color(UmlState)
                 : itscolor;

    if (used_color != UmlTransparent) {
        const int shadow = the_canvas()->shadow();

        min_width += shadow;
        min_height += shadow;
    }

    if ((ref != 0) || show_decomp_icon) {
        int mw = ((int)(12 * the_canvas()->zoom())) * 5;

        if (min_width < mw)
            min_width = mw;
    }

    // force odd width and height for line alignment
    min_width |= 1;
    min_height |= 1;

    DiagramCanvas::resize((width() > min_width) ? width() : min_width,
                          (height() > min_height) ? height() : min_height);


    compute_regions();
}

static bool sub_of(BrowserNode * br, BrowserState * st)
{
    for (;;) {
        br = (BrowserNode *) br->parent();

        if (br == st)
            return TRUE;

        switch (br->get_type()) {
        case UmlState:
        case UmlRegion:
            break;

        default:
            return FALSE;
        }
    }
}

void StateCanvas::prepare_for_move(bool on_resize)
{
    if (! on_resize) {
        DiagramCanvas::prepare_for_move(on_resize);

        // select sub nodes
        Q3CanvasItemList all = canvas()->allItems();
        Q3CanvasItemList::Iterator cit;
        UmlCanvas * canvas = the_canvas();

        for (cit = all.begin(); cit != all.end(); ++cit) {
            if ((*cit)->visible() && !(*cit)->selected()) {
                DiagramItem * di = QCanvasItemToDiagramItem(*cit);

                if ((di != 0) && (di->get_bn() != 0)) {
                    switch (di->type()) {
                    case UmlState:
                    case UmlStateAction:
                    case InitialPS:
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
                        if (sub_of(di->get_bn(), (BrowserState *) browser_node))
                            canvas->select(*cit);

                        break;

                    default:
                        break;
                    }
                }
            }
        }

        // select points on lines having the two extremities selected or
        // connecting pin/parameter/expansion node of element selected
        for (cit = all.begin(); cit != all.end(); ++cit) {
            if ((*cit)->visible() && !(*cit)->selected() && isa_arrow(*cit)) {
                ArrowCanvas * ar = (ArrowCanvas *) *cit;
                DiagramItem * b;
                DiagramItem * e;

                ar->extremities(b, e);

                if (!b->isSelected() || !e->isSelected()) {
                    DiagramItem * start = ar->get_start();
                    DiagramItem * dest = ar->get_end();

                    if (start->isSelected() && dest->isSelected()) {
                        while (b->type() == UmlArrowPoint) {
                            canvas->select((ArrowPointCanvas *) b);
                            ar = ((ArrowPointCanvas *) b)->get_other(ar);
                            ar->extremities(b, e);
                        }

                        ar = (ArrowCanvas *) *cit;
                        ar->extremities(b, e);

                        while (e->type() == UmlArrowPoint) {
                            canvas->select((ArrowPointCanvas *) e);
                            ar = ((ArrowPointCanvas *) e)->get_other(ar);
                            ar->extremities(b, e);
                        }
                    }
                }
            }
        }
    }
}

void StateCanvas::change_scale()
{
    Q3CanvasRectangle::setVisible(FALSE);
    double scale = the_canvas()->zoom();

    setSize((int)(width_scale100 * scale), (int)(height_scale100 * scale));
    compute_size();
    recenter();
    Q3CanvasRectangle::setVisible(TRUE);
}

void StateCanvas::modified()
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
    force_sub_inside(FALSE);
    package_modified();
}

void StateCanvas::post_loaded()
{
    force_self_rel_visible();

    if (the_canvas()->must_draw_all_relations()) {
        //draw_all_simple_relations();
        draw_all_transitions();
    }
}

void StateCanvas::connexion(UmlCode action, DiagramItem * dest,
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

aCorner StateCanvas::on_resize_point(const QPoint & p)
{
    return ::on_resize_point(p, rect());
}

void StateCanvas::resize(aCorner c, int dx, int dy, QPoint & o)
{
    DiagramCanvas::resize(c, dx, dy, o, min_width, min_height, TRUE);

    force_sub_inside(FALSE);
}

void StateCanvas::resize(const QSize & sz, bool w, bool h)
{
    if (DiagramCanvas::resize(sz, w, h, min_width, min_height, TRUE))
        force_sub_inside(FALSE);
}

void StateCanvas::force_sub_inside(bool resize_it)
{
    // update sub states (even through region) position to be
    // inside of the state / region
    // or resize state to contains sub elts if resize_it

    Q3CanvasItemList all = canvas()->allItems();
    Q3CanvasItemList::Iterator cit;
    int resize_left = 0;
    int resize_right = 0;
    int resize_top = 0;
    int resize_bottom = 0;
    bool need_sub_upper = FALSE;
    int nr = 0;

    // a for just to place again elements if needed
    for (;;) {
        for (cit = all.begin(); cit != all.end(); ++cit) {
            if ((*cit)->visible()/* && !(*cit)->selected()*/) {
                DiagramItem * di = QCanvasItemToDiagramItem(*cit);

                if (di != 0) {
                    bool border_allowed = FALSE;

                    switch (di->type()) {
                    case EntryPointPS:
                    case ExitPointPS:
                        border_allowed = TRUE;

                        // no break
                    case UmlState:
                    case UmlStateAction:
                    case InitialPS:
                    case FinalPS:
                    case TerminatePS:
                    case DeepHistoryPS:
                    case ShallowHistoryPS:
                    case JunctionPS:
                    case ChoicePS:
                    case ForkPS:
                    case JoinPS: {
                        BrowserNode * parent =
                            (BrowserNode *) di->get_bn()->parent();
                        QRect r;

                        if (parent == browser_node) {
                            nr = 1;

                            r = (border_allowed) ? rect() : regions_rect[0];
                        }
                        else if ((parent->get_type() == UmlRegion) &&
                                 (parent->parent() == browser_node)) {
                            nr = regions.size();

                            if (nr == 0) {
                                // too small to show them
                                if (resize_it)
                                    nr = resize_to_show_regions();
                                else
                                    continue;
                            }

                            r = region_rect((BrowserRegion *) parent);
                        }
                        else
                            continue;

                        need_sub_upper |= ((*cit)->z() <= z());

                        int dx = 0;
                        int dy = 0;
                        QRect di_r = di->rect();

                        if (border_allowed) {
                            QPoint c = di->center();

                            di_r.setLeft(c.x());
                            di_r.setTop(c.y());
                            di_r.setWidth(1);
                            di_r.setHeight(1);
                        }
                        else
                            di_r = di->rect();

                        if (resize_it) {
                            if (di_r.left() < r.left()) {
                                dx = r.left() - di_r.left();

                                if (dx > resize_left)
                                    resize_left = dx;
                            }

                            if (di_r.right() > r.right()) {
                                dx = di_r.right() - r.right();

                                if (dx > resize_right)
                                    resize_right = dx;
                            }

                            if (di_r.top() < r.top()) {
                                dy = r.top() - di_r.top();

                                if (dy > resize_top)
                                    resize_top = dy;
                            }

                            if (di_r.bottom() > r.bottom()) {
                                dy = di_r.bottom() - r.bottom();

                                if (dy > resize_bottom)
                                    resize_bottom = dy;
                            }
                        }
                        else {
                            if (di_r.left() < r.left()) {
                                if (di_r.right() <= r.right())
                                    dx = r.left() - di_r.left();
                            }
                            else if (di_r.right() > r.right())
                                dx = r.right() - di_r.right();

                            if (di_r.top() < r.top()) {
                                if (di_r.bottom() <= r.bottom())
                                    dy = r.top() - di_r.top();
                            }
                            else if (di_r.bottom() > r.bottom())
                                dy = r.bottom() - di_r.bottom();

                            if ((dx != 0) || (dy != 0)) {
                                (*cit)->moveBy(dx, dy);

                                if (di->type() == UmlState) {
                                    ((StateCanvas *) di)->moveSelfRelsBy(dx, dy);
                                    ((StateCanvas *) di)->hide_lines();
                                    ((StateCanvas *) di)->update_show_lines();

                                    // to update regions position
                                    (*cit)->setVisible(FALSE);
                                    ((StateCanvas *) di)->was_drawn = FALSE;
                                    (*cit)->setVisible(TRUE);
                                    canvas()->update();

                                    if (!((StateCanvas *) di)->was_drawn)
                                        ((StateCanvas *) di)->compute_regions();

                                    ((StateCanvas *) di)->force_sub_inside(FALSE);
                                }
                            }
                        }
                    }
                    break;

                    default:
                        break;
                    }
                }
            }
        }

        if (resize_it) {
            int dx = 0;
            int dy = 0;
            int dw = 0;
            int dh = 0;

            if (resize_top != 0) {
                dy = -resize_top;
                dh = resize_top;
            }

            if (resize_bottom != 0)
                dh += resize_bottom;

            if (resize_left != 0) {
                dx = -resize_left;
                dw = resize_left;
            }

            if (resize_right != 0)
                dw += resize_right;

            if ((dx != 0) || (dy != 0)) {
                moveBy(dx, dy);
                moveSelfRelsBy(dx, dy);
            }

            if ((dw != 0) || (dh != 0)) {
                DiagramCanvas::resize(width() + dw, height() + dh);

                if (nr > 1) {
                    // too low probability to have element placed right in regions,
                    // place them again
                    resize_it = FALSE;

                    // to update regions position
                    setVisible(FALSE);
                    was_drawn = FALSE;
                    setVisible(TRUE);
                    canvas()->update();

                    if (! was_drawn)
                        compute_regions();

                    // no break !
                }
                else
                    break;
            }
            else
                break;
        }
        else
            break;
    }

    if (need_sub_upper)
        force_sub_upper();

    hide_lines();
    update_show_lines();
}

int StateCanvas::resize_to_show_regions()
{
    Q3ListViewItem * child = browser_node->firstChild();
    int nr = 0;

    while (child != 0) {
        if (!((BrowserNode *) child)->deletedp() &&
            (((BrowserNode *) child)->get_type() == UmlRegion))
            nr += 1;

        child = child->nextSibling();
    }

    QFontMetrics fm(the_canvas()->get_font(UmlNormalBoldFont));
    int d = fm.height() * nr;

    if (region_horizontally)
        DiagramCanvas::resize(width(), height() + d);
    else
        DiagramCanvas::resize(width() + d, height());

    // to recompute regions rect
    Q3CanvasRectangle::setVisible(FALSE);
    Q3CanvasRectangle::setVisible(TRUE);
    canvas()->update();

    if (! was_drawn)
        compute_regions();

    return nr;
}

void StateCanvas::force_inside(DiagramCanvas * elt, bool resize_it)
{
    BrowserNode * parent = (BrowserNode *) elt->get_bn()->parent();
    BrowserNode * container_state;
    bool container_shown = FALSE;
    bool border_allowed = FALSE;

    switch (parent->get_type()) {
    case UmlState:
        container_state = parent;

        switch (elt->type()) {
        case EntryPointPS:
        case ExitPointPS:
            border_allowed = TRUE;
            break;

        default:
            break;
        }

        break;

    case UmlRegion:
        container_state = (BrowserNode *) parent->parent();
        break;

    default:
        // machine
        container_state = 0;
    }

    if (container_state != 0) {
        Q3CanvasItemList all = elt->the_canvas()->allItems();
        Q3CanvasItemList::Iterator cit;

        for (cit = all.begin(); cit != all.end(); ++cit) {
            if ((*cit)->visible()) {
                DiagramItem * di = QCanvasItemToDiagramItem(*cit);

                if ((di != 0) && (di->get_bn() == container_state)) {
                    StateCanvas * state = (StateCanvas *) di;
                    QRect r;
                    QRect elt_r;

                    if (border_allowed) {
                        QPoint c = elt->center();

                        elt_r.setLeft(c.x());
                        elt_r.setTop(c.y());
                        elt_r.setWidth(1);
                        elt_r.setHeight(1);
                    }
                    else
                        elt_r = elt->rect();

                    if (parent == container_state) {
                        if (border_allowed)
                            r = state->rect();
                        else {
                            r = state->regions_rect[0];

                            // to manage case where the state is too small to show legal areas
                            if ((r.width() <= elt_r.width()) ||
                                (r.height() <= elt_r.height()))
                                r = state->rect();
                        }
                    }
                    else if (state->regions.size() == 0) {
                        // state too small to draw regions
                        r = state->rect();
                    }
                    else {
                        r = state->region_rect((BrowserRegion *) parent);

                        // state too small to draw regions
                        if (r.width() <= elt_r.width())
                            r.setWidth(elt_r.width());

                        if (r.height() <= elt_r.height())
                            r.setHeight(elt_r.height());
                    }

                    int dx = 0;
                    int dy = 0;

                    if (elt_r.left() < r.left()) {
                        if (elt_r.right() <= r.right())
                            dx = r.left() - elt_r.left();
                    }
                    else if (elt_r.right() > r.right())
                        dx = r.right() - elt_r.right();

                    if (elt_r.top() < r.top()) {
                        if (elt_r.bottom() <= r.bottom())
                            dy = r.top() - elt_r.top();
                    }
                    else if (elt_r.bottom() > r.bottom())
                        dy = r.bottom() - elt_r.bottom();

                    if ((dx != 0) || (dy != 0)) {
                        elt->moveBy(dx, dy);
                        elt->moveSelfRelsBy(dx, dy);
                    }

                    if (elt->z() <= state->z())
                        elt->upper();

                    container_shown = TRUE;

                    break;
                }
            }
        }
    }

    if (elt->type() == UmlState) {
        ((StateCanvas *) elt)->hide_lines();
        ((StateCanvas *) elt)->update_show_lines();
        ((StateCanvas *) elt)->force_sub_inside(resize_it && !container_shown);
    }
}

void StateCanvas::force_sub_upper()
{
    Q3CanvasItemList all = canvas()->allItems();
    Q3CanvasItemList::Iterator cit;

    for (cit = all.begin(); cit != all.end(); ++cit) {
        if ((*cit)->visible()) {
            DiagramItem * di = QCanvasItemToDiagramItem(*cit);

            if (di != 0) {
                switch (di->type()) {
                case UmlState:
                case UmlStateAction:
                case InitialPS:
                case EntryPointPS:
                case FinalPS:
                case TerminatePS:
                case ExitPointPS:
                case DeepHistoryPS:
                case ShallowHistoryPS:
                case JunctionPS:
                case ChoicePS:
                case ForkPS:
                case JoinPS: {
                    BrowserNode * parent = (BrowserNode *) di->get_bn()->parent();

                    if ((parent == browser_node) ||
                        ((parent->get_type() == UmlRegion) &&
                         (parent->parent() == browser_node))) {
                        if ((*cit)->z() <= z())
                            ((DiagramCanvas *) di)->upper();

                        if (di->type() == UmlState)
                            ((StateCanvas *) di)->force_sub_upper();
                    }
                }
                break;

                default:
                    break;
                }
            }
        }
    }
}

void StateCanvas::draw(QPainter & p)
{
    // warning : must report any changes of region position & size
    // computation in compute_regions() !

    was_drawn = TRUE; // to know compute_regions() will not have to be called

    if (! visible()) return;

    p.setRenderHint(QPainter::Antialiasing, true);
    QRect r = rect();
    QRect re = r;
    QBrush brsh = p.brush();
    FILE * fp = svg();

    if (fp != 0)
        fputs("<g>\n", fp);

    if (used_color != UmlTransparent) {
        const int shadow = the_canvas()->shadow();

        if (shadow != 0) {
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
    }

    QColor bckgrnd = p.backgroundColor();
    QFontMetrics fm(the_canvas()->get_font(UmlNormalBoldFont));
    const int fnt_height = fm.height();
    const int half_fnt_height = fnt_height / 2;
    const StateData * data = (StateData *) browser_node->get_data();
    BrowserState * ref = data->get_reference();

    p.setBackgroundMode((used_color == UmlTransparent)
                        ? ::Qt::TransparentMode
                        : ::Qt::OpaqueMode);

    QColor co = color(used_color);

    p.setBackgroundColor(co);

    if (used_color != UmlTransparent)
        p.setBrush(co);

    p.drawRoundRect(r);

    if (fp != 0) {
        fprintf(fp, "\t<rect fill=\"%s\" stroke=\"black\" stroke-opacity=\"1\""
                " x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" rx=\"10\" />\n",
                svg_color(used_color),
                r.left(), r.top(), r.width() - 1, r.height() - 1);
    }

    if ((ref != 0) || show_decomp_icon) {
        int ln = (int)(12 * the_canvas()->zoom());
        int to = r.bottom() - 2 * ln;
        int mi = to + ln / 2;
        int le = r.right() - 4 * ln;

        p.drawEllipse(le, to, ln, ln);
        p.drawEllipse(le + 2 * ln, to, ln, ln);
        p.drawLine(le + ln, mi, le + 2 * ln, mi);

        if (fp != 0) {
            int radius = (int)(6 * the_canvas()->zoom());

            fprintf(fp, "\t<ellipse fill=\"none\" stroke=\"black\" stroke-width=\"1\" stroke-opacity=\"1\""
                    " cx=\"%d\" cy=\"%d\" rx=\"%d\" ry=\"%d\" />\n",
                    le + radius, to + radius, radius, radius);

            fprintf(fp, "\t<ellipse fill=\"none\" stroke=\"black\" stroke-width=\"1\" stroke-opacity=\"1\""
                    " cx=\"%d\" cy=\"%d\" rx=\"%d\" ry=\"%d\" />\n",
                    le + radius + 2 * ln, to + radius, radius, radius);

            fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                    " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                    le + ln, mi, le + 2 * ln, mi);
        }
    }

    p.setFont(the_canvas()->get_font(UmlNormalBoldFont));
    r.setTop(r.top() + half_fnt_height);

    QString s = browser_node->get_name();

    if (ref != 0)
        s += ":";

    p.drawText(r, ::Qt::AlignHCenter, s);

    if (fp != 0)
        draw_text(r, ::Qt::AlignHCenter, s, p.font(), fp);

    if (ref != 0) {
        s = ref->get_name();
        r.setTop(r.top() + fnt_height);
        p.drawText(r, ::Qt::AlignHCenter, s);

        if (fp != 0)
            draw_text(r, ::Qt::AlignHCenter, s, p.font(), fp);
    }

    p.setFont(the_canvas()->get_font(UmlNormalFont));
    r.setTop(r.top() + fnt_height);

    if (data->get_stereotype()[0] != 0) {
        p.drawText(r, ::Qt::AlignHCenter,
                   QString("<<") + toUnicode(data->get_short_stereotype()) + ">>");

        if (fp != 0)
            draw_text(r, ::Qt::AlignHCenter,
                      QString("<<") + toUnicode(data->get_short_stereotype()) + ">>",
                      p.font(), fp);

        r.setTop(r.top() + fnt_height);
    }

    const int sixteen = (int)(16 * the_canvas()->zoom());

    r.setLeft(r.left() + sixteen);
    r.setRight(r.right() - sixteen);

    if ((ref == 0) && !activities.isEmpty()) {
        r.setTop(r.top() + half_fnt_height);
        p.drawLine(r.topLeft(), r.topRight());

        if (fp != 0)
            fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                    " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                    r.left(), r.top(), r.right(), r.top());

        r.setTop(r.top() + half_fnt_height);
        p.drawText(r, 0, activities);

        if (fp != 0)
            draw_text(r, 0, activities,
                      p.font(), fp);

        QSize sz = fm.size(0, activities);

        r.setTop(r.top() + sz.height());
    }

    r.setTop(r.top() + half_fnt_height);

    int nregion = 0;

    if (ref == 0) {
        Q3ListViewItem * child = browser_node->firstChild();

        while (child != 0) {
            if (!((BrowserNode *) child)->deletedp() &&
                (((BrowserNode *) child)->get_type() == UmlRegion))
                nregion += 1;

            child = child->nextSibling();
        }
    }

    if (nregion != 0) {
        int szreg = ((region_horizontally) ? r.height() : r.width())
                    / nregion;

        if (szreg > fnt_height) {
            regions.resize(nregion);
            regions_rect.resize(nregion);
            p.drawLine(r.topLeft(), r.topRight());

            if (fp != 0)
                fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                        " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                        r.left(), r.top(), r.right(), r.top());

            p.setPen(::Qt::DashLine);

            Q3ListViewItem * child = browser_node->firstChild();

            for (;;) {
                while (((BrowserNode *) child)->deletedp() ||
                       (((BrowserNode *) child)->get_type() != UmlRegion))
                    child = child->nextSibling();

                // child
                regions[--nregion] = (BrowserRegion *) child;

                QRect & rr = regions_rect[nregion];

                if (region_horizontally) {
                    rr.setTop(r.top());
                    r.setTop(r.top() + szreg);
                    rr.setBottom(r.top());
                    rr.setLeft(re.left());
                    rr.setRight(re.right());
                }
                else {
                    rr.setTop(r.top());
                    rr.setBottom(re.bottom());
                    rr.setLeft(r.left());
                    r.setLeft(r.left() + szreg);
                    rr.setRight(r.left());
                }

                if (nregion == 0)
                    break;

                if (region_horizontally) {
                    p.drawLine(r.topLeft(), r.topRight());

                    if (fp != 0)
                        fprintf(fp, "\t<line stroke=\"black\" stroke-dasharray=\"18,6\" stroke-opacity=\"1\""
                                " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                                r.left(), r.top(), r.right(), r.top());
                }
                else {
                    p.drawLine(r.topLeft(), r.bottomLeft());

                    if (fp != 0)
                        fprintf(fp, "\t<line stroke=\"black\" stroke-dasharray=\"18,6\" stroke-opacity=\"1\""
                                " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                                r.left(), r.top(), r.left(), r.bottom());
                }

                child = child->nextSibling();
            }

            p.setPen(::Qt::SolidLine);
        }
        else {
            regions.resize(0);
            regions_rect.resize(1);
            regions_rect[0] = r;
        }
    }
    else {
        regions.resize(0);
        regions_rect.resize(1);
        regions_rect[0] = r;
    }

    if (fp != 0)
        fputs("</g>\n", fp);

    p.setBackgroundColor(bckgrnd);
    p.setBrush(brsh);

    if (selected())
        show_mark(p, rect());
}

void StateCanvas::compute_regions()
{
    // code of draw() without drawing
    QRect r = rect();
    QRect re = r;

    if (used_color != UmlTransparent) {
        const int shadow = the_canvas()->shadow();

        if (shadow != 0) {
            r.setRight(r.right() - shadow);
            r.setBottom(r.bottom() - shadow);
        }
    }

    QFontMetrics fm(the_canvas()->get_font(UmlNormalBoldFont));
    const int fnt_height = fm.height();
    const int half_fnt_height = fnt_height / 2;
    const StateData * data = (StateData *) browser_node->get_data();
    BrowserState * ref = data->get_reference();

    r.setTop(r.top() + half_fnt_height
             + fnt_height
             + half_fnt_height);

    if (data->get_stereotype()[0] != 0) {
        r.setTop(r.top() + fnt_height);
    }

    const int sixteen = (int)(16 * the_canvas()->zoom());

    r.setLeft(r.left() + sixteen);
    r.setRight(r.right() - sixteen);

    int nregion = 0;

    if (ref != 0)
        r.setTop(r.top() + fnt_height);
    else {
        if (! activities.isEmpty()) {
            QSize sz = fm.size(0, activities);

            r.setTop(r.top() + half_fnt_height
                     + half_fnt_height
                     + sz.height());
        }


        Q3ListViewItem * child = browser_node->firstChild();

        while (child != 0) {
            if (!((BrowserNode *) child)->deletedp() &&
                (((BrowserNode *) child)->get_type() == UmlRegion))
                nregion += 1;

            child = child->nextSibling();
        }
    }

    if (nregion != 0) {
        int szreg = ((region_horizontally) ? r.height() : r.width())
                    / nregion;

        if (szreg > fnt_height) {
            regions.resize(nregion);
            regions_rect.resize(nregion);

            Q3ListViewItem * child = browser_node->firstChild();

            for (;;) {
                while (((BrowserNode *) child)->deletedp() ||
                       (((BrowserNode *) child)->get_type() != UmlRegion))
                    child = child->nextSibling();

                // child
                regions[--nregion] = (BrowserRegion *) child;

                QRect & rr = regions_rect[nregion];

                if (region_horizontally) {
                    rr.setTop(r.top());
                    r.setTop(r.top() + szreg);
                    rr.setBottom(r.top());
                    rr.setLeft(re.left());
                    rr.setRight(re.right());
                }
                else {
                    rr.setTop(r.top());
                    rr.setBottom(re.bottom());
                    rr.setLeft(r.left());
                    r.setLeft(r.left() + szreg);
                    rr.setRight(r.left());
                }

                if (nregion == 0)
                    break;

                child = child->nextSibling();
            }
        }
        else {
            regions.resize(0);
            regions_rect.resize(1);
            regions_rect[0] = r;
        }
    }
    else {
        regions.resize(0);
        regions_rect.resize(1);
        regions_rect[0] = r;
    }
}

UmlCode StateCanvas::type() const
{
    return UmlState;
}

BrowserRegion * StateCanvas::pointed_region(const QPoint & p) const
{
    // warning : without region, region_rect.size() == 1
    for (unsigned i = 0; i != regions.size(); i += 1)
        if (regions_rect.at(i).contains(p))
            return regions.at(i);

    return 0;
}

QRect StateCanvas::region_rect(BrowserRegion * r)
{
    // warning : without region, region_rect.size() == 1
    for (unsigned i = 0; i != regions.size(); i += 1)
        if (regions.at(i) == r)
            return regions_rect.at(i);

    QRect re;

    return re;
}

void StateCanvas::delete_available(BooL & in_model, BooL & out_model) const
{
    out_model |= TRUE;
    in_model |= browser_node->is_writable();
}

bool StateCanvas::alignable() const
{
    return TRUE;
}

bool StateCanvas::copyable() const
{
    return selected();
}

void StateCanvas::open()
{
    browser_node->open(FALSE);
}

void StateCanvas::menu(const QPoint &)
{
    Q3PopupMenu m(0);
    Q3PopupMenu toolm(0);
    int index;
    const StateData * data = (StateData *) browser_node->get_data();
    BrowserState * ref = data->get_reference();

    MenuFactory::createTitle(m, data->definition(FALSE, TRUE));
    m.insertSeparator();
    m.insertItem(TR("Upper"), 0);
    m.insertItem(TR("Lower"), 1);
    m.insertItem(TR("Go up"), 13);
    m.insertItem(TR("Go down"), 14);
    m.insertSeparator();

    if (ref == 0) {
        if (show_decomp_icon)
            m.insertItem(TR("Hide decomposition indicator"), 15);
        else
            m.insertItem(TR("Show decomposition indicator"), 15);
    }

    m.insertItem(TR("Edit drawing settings"), 2);
    m.insertSeparator();
    m.insertItem(TR("Edit state"), 3);
    m.insertSeparator();
    m.insertItem(TR("Select in browser"), 4);

    if (ref != 0)
        m.insertItem(TR("Select referenced state in browser"), 16);

    if (linked())
        m.insertItem(TR("Select linked items"), 5);

    m.insertSeparator();

    if (browser_node->is_writable()) {
        if (browser_node->get_associated() !=
            (BrowserNode *) the_canvas()->browser_diagram())
            m.insertItem(TR("Set associated diagram"), 6);

        if (browser_node->get_associated())
            m.insertItem(TR("Remove diagram association"), 9);
    }

    m.insertSeparator();
    m.insertItem(TR("Remove from diagram"), 7);

    if (browser_node->is_writable())
        m.insertItem(TR("Delete from model"), 8);

    m.insertSeparator();

    if (Tool::menu_insert(&toolm, UmlState, 20))
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
        ((BrowserState *) browser_node)
        ->set_associated_diagram((BrowserStateDiagram *)
                                 the_canvas()->browser_diagram());
        return;

    case 9:
        ((BrowserState *) browser_node)
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

    case 15:
        show_decomp_icon = !show_decomp_icon;
        modified();	// call package_modified()

    case 16:
        ref->select_in_browser();
        return;

    default:
        if (index >= 20)
            ToolCom::run(Tool::command(index - 20), browser_node);

        return;
    }

    package_modified();
}

void StateCanvas::apply_shortcut(QString s)
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

void StateCanvas::edit_drawing_settings()
{
    for (;;) {
        StateSpecVector st(3);
        ColorSpecVector co(1);

        st[0].set(TR("show activities"), &settings.show_activities);
        st[1].set(TR("draw regions horizontally"), &settings.region_horizontally);
        st[2].set(TR("drawing language"), &settings.drawing_language);

        co[0].set(TR("state color"), &itscolor);

        SettingsDialog dialog(&st, &co, FALSE);

        dialog.raise();

        if (dialog.exec() == QDialog::Accepted)
            modified();	// call package_modified()

        if (!dialog.redo())
            break;
    }
}

bool StateCanvas::has_drawing_settings() const
{
    return TRUE;
}

void StateCanvas::edit_drawing_settings(QList<DiagramItem *> & l)
{
    for (;;) {
        StateSpecVector st(3);
        ColorSpecVector co(1);
        Uml3States show_activities;
        Uml3States region_horizontally;
        UmlColor itscolor;
        DrawingLanguage language;

        st[0].set(TR("show activities"), &show_activities);
        st[1].set(TR("draw regions horizontally"), &region_horizontally);
        st[2].set(TR("drawing language"), &language);

        co[0].set(TR("state color"), &itscolor);

        SettingsDialog dialog(&st, &co, FALSE, TRUE);

        dialog.raise();

        if (dialog.exec() == QDialog::Accepted) {
            foreach (DiagramItem *item, l) {
                StateCanvas *canvas = (StateCanvas *)item;
                if (!st[0].name.isEmpty())
                    canvas->settings.show_activities =
                        show_activities;

                if (!st[1].name.isEmpty())
                    canvas->settings.region_horizontally =
                        region_horizontally;

                if (!st[2].name.isEmpty())
                    canvas->settings.drawing_language =
                        language;

                if (!co[0].name.isEmpty())
                    canvas->itscolor = itscolor;

                canvas->modified();
            }
        }

        if (!dialog.redo())
            break;
    }
}

void StateCanvas::clone_drawing_settings(const DiagramItem *src)
{
    const StateCanvas * x = (const StateCanvas *) src;
    settings = x->settings;
    itscolor = x->itscolor;
    modified();
}

bool StateCanvas::get_show_stereotype_properties() const
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

QString StateCanvas::may_start(UmlCode & l) const
{
    switch (l) {
    case UmlAnchor:
    case UmlTransition:
        return 0;

    default:
        return TR("illegal");
    }
}

QString StateCanvas::may_connect(UmlCode & l, const DiagramItem * dest) const
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

void StateCanvas::save(QTextStream & st, bool ref, QString & warning) const
{
    if (ref) {
        st << "statecanvas_ref " << get_ident() << " // "
           << browser_node->full_name();
    }
    else {
        nl_indent(st);
        st << "statecanvas " << get_ident() << " ";
        browser_node->save(st, TRUE, warning);
        indent(+1);
        settings.save(st);
        nl_indent(st);

        if (itscolor != UmlDefaultColor)
            st << "color " << stringify(itscolor) << " " ;

        if (show_decomp_icon)
            st << "show_decomp_icon ";

        save_xyzwh(st, this, "xyzwh");
        save_stereotype_property(st, warning);
        indent(-1);
        nl_indent(st);
        st << "end";
    }
}

StateCanvas * StateCanvas::read(char *& st, UmlCanvas * canvas,
                                char * k)
{
    if (!strcmp(k, "statecanvas_ref"))
        return ((StateCanvas *) dict_get(read_id(st), "statecanvas", canvas));
    else if (!strcmp(k, "statecanvas")) {
        int id = read_id(st);
        BrowserState * br = BrowserState::read_ref(st);
        StateCanvas * result = new StateCanvas(canvas, id);

        result->browser_node = br;
        connect(br->get_data(), SIGNAL(changed()), result, SLOT(modified()));
        connect(br->get_data(), SIGNAL(deleted()), result, SLOT(deleted()));

        k = read_keyword(st);

        result->settings.read(st, k);	// updates k
        read_color(st, "color", result->itscolor, k);	// updates k

        if (!strcmp(k, "show_decomp_icon")) {
            result->show_decomp_icon = TRUE;
            k = read_keyword(st);
        }

        if (!strcmp(k, "xyz"))
            read_xyz(st, result);
        else if (!strcmp(k, "xyzwh"))
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

        if (canvas->paste())
            result->remove_if_already_present();

        return result;
    }
    else
        return 0;
}

void StateCanvas::history_save(QBuffer & b) const
{
    DiagramCanvas::history_save(b);
    ::save(min_width, b);
    ::save(min_height, b);
    ::save(width_scale100, b);
    ::save(height_scale100, b);
    ::save(width(), b);
    ::save(height(), b);

    QRect * r = regions_rect.data();

    if (r != 0) {
        for (unsigned i = 0; i != regions_rect.size(); i++) {
            ::save(r->topLeft(), b);
            ::save(r->bottomRight(), b);
            r += 1;
        }
    }
}

void StateCanvas::history_load(QBuffer & b)
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

    QRect * r = regions_rect.data();

    if (r != 0) {
        for (unsigned i = 0; i != regions_rect.size(); i++) {
            QPoint p;

            ::load(p, b);
            r->moveTopLeft(p);
            ::load(p, b);
            r->moveBottomRight(p);
            r += 1;
        }
    }

    connect(browser_node->get_data(), SIGNAL(changed()), this, SLOT(modified()));
    connect(browser_node->get_data(), SIGNAL(deleted()), this, SLOT(deleted()));
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
}

void StateCanvas::history_hide()
{
    Q3CanvasItem::setVisible(FALSE);
    disconnect(DrawingSettings::instance(), SIGNAL(changed()),
               this, SLOT(modified()));
    disconnect(browser_node->get_data(), 0, this, 0);

    MultipleDependencyIterator<BasicData> it(this);

    while (it.current()) {
        disconnect(it.current(), 0, this, 0);
        ++it;
    }

    unsubscribe_all();
}

