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





#include <qpainter.h>
#include <qcursor.h>
#include <q3popupmenu.h>
//Added by qt3to4:
#include <QTextStream>

#include "SdDurationCanvas.h"
#include "SdLifeLineCanvas.h"
#include "SdClassInstCanvas.h"
#include "SdSelfMsgCanvas.h"
#include "SdMsgCanvas.h"
#include "SdLostFoundMsgSupportCanvas.h"
#include "UmlCanvas.h"
#include "BrowserSeqDiagram.h"
#include "UmlGlobal.h"
#include "Settings.h"
#include "SettingsDialog.h"
#include "myio.h"
#include "ui/menufactory.h"
#include "ToolCom.h"
#include "translate.h"

#define DURATION_MIN_HEIGHT 25
#define DURATION_START_HEIGHT 40

#define DURATION_TOP 70

SdDurationCanvas::SdDurationCanvas(UmlCanvas * canvas, SdDurationSupport * sp,
                                   int v, bool isdest)
    : SdMsgSupport(canvas, 0, v, DURATION_WIDTH,
                   (isdest) ? DURATION_MIN_HEIGHT : DURATION_START_HEIGHT,
                   0),
    support(sp), itscolor(UmlDefaultColor), coregion(FALSE)
{
    browser_node = canvas->browser_diagram();
    update_hpos();
    setZ(sp->getZ() + 10);
    show();

    support->add(this);

    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
}

// called by load
SdDurationCanvas::SdDurationCanvas(UmlCanvas * canvas, SdDurationSupport * sp,
                                   int x, int y, int wi, int he, int id, bool coreg)
    : SdMsgSupport(canvas, x, y, wi, he, id),
      support(sp), itscolor(UmlDefaultColor), coregion(coreg)
{
    browser_node = canvas->browser_diagram();
    support->add(this);
}

SdDurationCanvas::~SdDurationCanvas()
{
}

void SdDurationCanvas::delete_it()
{
    while (!durations.isEmpty())
        durations.first()->delete_it();

    while (!msgs.isEmpty())
        msgs.first()->delete_it();	// will update msgs

    disconnect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
    support->remove(this);
    DiagramCanvas::delete_it();
}

void SdDurationCanvas::delete_available(BooL &, BooL & out_model) const
{
    out_model |= TRUE;
}

void SdDurationCanvas::change_scale()
{
    DiagramCanvas::change_scale();
    update_self();

    hide();
    update_hpos();
    show();
}

void SdDurationCanvas::update_hpos()
{
    move(support->sub_x(width()), 100000);

    foreach (SdDurationCanvas *canvas, durations)
        canvas->update_hpos();
}

void SdDurationCanvas::draw(QPainter & p)
{
    const QRect r = rect();
    p.setRenderHint(QPainter::Antialiasing, true);
    UmlColor used_color = (itscolor != UmlDefaultColor)
                          ? itscolor
                          : browser_node->get_color(UmlActivityDuration);
    QColor co = color(used_color);

    int w = width() - 1;
    int x = r.left();
    int y = r.top();

    p.setBackgroundMode((used_color == UmlTransparent)
                        ? ::Qt::TransparentMode
                        : ::Qt::OpaqueMode);
    p.fillRect(r, co);

    if (coregion) {
        p.drawLine(x, y + w, x, y);



        //p.lineTo(x + w, y);
        p.drawLine(x, y, x + w, y);
        //p.lineTo(x + w, y + w);
        p.drawLine(x + w, y, x + w, y + w);

        int b = r.bottom();

        p.drawLine(x, b - w, x, b);



        //p.lineTo(x + w, b);
        p.drawLine(x, b, x + w, b);
        //p.lineTo(x + w, b - w);
        p.drawLine(x + w, b, x + w, b - w);
    }
    else
        p.drawRect(r);

    FILE * fp = svg();

    if (fp != 0) {
        if (coregion) {
            fprintf(fp, "<g>\n"
                    "\t<rect fill=\"%s\" stroke=\"none\" "
                    " x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" />\n"
                    "\t<path fill = \"none\" stroke=\"black\" stroke-width=\"1\" stroke-opacity=\"1\" "
                    "d=\"M %d %d v %d h %d v %d M %d %d v %d h %d v %d\" />\n"
                    "</g>\n",
                    svg_color(used_color),
                    x, y, w, r.height() - 1,
                    x, y + w, -w, w, w,
                    x, r.bottom() - w, w, w, -w);
        }
        else
            fprintf(fp, "<g>\n"
                    "\t<rect fill=\"%s\" stroke=\"black\" stroke-width=\"1\" stroke-opacity=\"1\""
                    " x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" />\n"
                    "</g>\n",
                    svg_color(used_color),
                    x, y, w, r.height() - 1);
    }

    if (selected())
        show_mark(p, r);
}

void SdDurationCanvas::moveBy(double dx, double dy)
{
    if (dy > 80000) {
        // from update_hpos
        DiagramCanvas::moveBy(dx, 0);

        // update messages position (even selected, this case is managed by the messages)
        foreach (SdMsgBaseCanvas *canvas, msgs)
            canvas->update_hpos();
    }
    else {
        double my = min_y();

        if ((y() + dy) < my)
            dy = my - y();

        // accept only vertical move
        DiagramCanvas::moveBy(0, dy);

        // move messages (even selected, this case is managed by the messages)
        foreach (SdMsgBaseCanvas *canvas, msgs)
            canvas->moveBy(100000, dy);
    }

    support->update_v_to_contain(this, FALSE);
    support->update_instance_dead();
}

void SdDurationCanvas::prepare_for_move(bool on_resize)
{
    DiagramCanvas::prepare_for_move(on_resize);

    if (! on_resize) {
        foreach (SdDurationCanvas *canvas, durations) {
            if (! canvas->selected()) {
                the_canvas()->select(canvas);
                canvas->prepare_for_move(on_resize);
            }
        }
    }
}

double SdDurationCanvas::min_y() const
{
    return support->min_y();
}

UmlCode SdDurationCanvas::type() const
{
    return UmlActivityDuration;
}

DiagramItem::LineDirection SdDurationCanvas::allowed_direction(UmlCode c)
{
    return (c == UmlAnchor) ? DiagramItem::All : DiagramItem::Horizontal;
}

// called by the duration's messages to increase if
// needed the duration to contain its messages
void SdDurationCanvas::update_v_to_contain(const QRect re)
{
    const QRect r = rect();

    if (re.top() < r.top()) {
        int dy = re.top() - r.top();

        Q3CanvasItem::moveBy(0, dy);
        resize(r.width(), r.height() - dy);
        update_self();
    }
    else if (re.bottom() > r.bottom()) {
        resize(r.width(), re.bottom() - r.top() + 1);
        update_self();
    }

    // done in all cases
    support->update_v_to_contain(this, TRUE);
}

void SdDurationCanvas::update_v_to_contain(SdDurationCanvas * d, bool force)
{
    if (force || !selected()) {
        int min_b = (int) d->y() + d->height() + width() / 2;
        int b = (int) y() + height();

        if (d->y() < y()) {
            Q3CanvasItem::moveBy(0, d->y() - y());
            resize(width(),
                   ((min_b > b) ? min_b : b) - (int) y());
            update_self();
        }
        else if (min_b > b) {
            resize(width(), min_b - (int) y() + 1);
            update_self();
        }

        // done in all cases
        support->update_v_to_contain(this, force);
    }
}

void SdDurationCanvas::update_self()
{
    QRect r = rect();

    foreach (SdMsgBaseCanvas *canvas, msgs)
        canvas->check_vpos(r);
}

QString SdDurationCanvas::may_start(UmlCode &) const
{
    return 0;
}

QString SdDurationCanvas::may_connect(UmlCode & l, const DiagramItem * dest) const
{
    if (l == UmlAnchor)
        return dest->may_start(l);
    else {
        switch (dest->type()) {
        case UmlActivityDuration:
        case UmlLifeLine:
            return 0;

        default:
            return TR("illegal");
        }
    }
}

bool SdDurationCanvas::may_connect(UmlCode l) const
{
    switch (l) {
    case UmlFoundSyncMsg:
    case UmlFoundAsyncMsg:
    case UmlLostSyncMsg:
    case UmlLostAsyncMsg:
        return TRUE;

    default:
        return FALSE;
    }
}

void SdDurationCanvas::connexion(UmlCode l, DiagramItem * dest,
                                 const QPoint & s, const QPoint & e)
{
    switch (l) {
    case UmlSyncSelfMsg:
    case UmlAsyncSelfMsg:
        if (dest->type() == UmlLifeLine)
            dest = this;
        else if (((BrowserSeqDiagram *) browser_node)->is_overlapping_bars()) {
            SdDurationCanvas * d = (SdDurationCanvas *) dest;

            dest = new SdDurationCanvas(the_canvas(), d, s.y(), TRUE);
            d->update_v_to_contain((SdDurationCanvas *) dest, FALSE);
        }

        (new SdSelfMsgCanvas(the_canvas(), (SdDurationCanvas *) dest, l, s.y(), 0))->upper();
        break;

    case UmlSelfReturnMsg:
        if (dest->type() == UmlLifeLine)
            dest = this;

        (new SdSelfMsgCanvas(the_canvas(), (SdDurationCanvas *) dest, l, s.y(), 0))->upper();
        break;

    case UmlAnchor:
        DiagramCanvas::connexion(l, dest, s, e);
        break;

    default:
        if (dest != this) {
            if (dest->type() == UmlLifeLine)
                // insert an activity duration canvas
                dest = new SdDurationCanvas(the_canvas(), ((SdLifeLineCanvas *) dest), s.y(), TRUE);
            else if (((BrowserSeqDiagram *) browser_node)->is_overlapping_bars() &&
                     (l != UmlReturnMsg)) {
                SdDurationCanvas * d = (SdDurationCanvas *) dest;

                dest = new SdDurationCanvas(the_canvas(), d, s.y(), TRUE);
                d->update_v_to_contain((SdDurationCanvas *) dest, FALSE);
            }

            (new SdMsgCanvas(the_canvas(), this, (SdDurationCanvas *) dest, l, s.y(), 0))
            ->upper();
        }
    }
}

bool SdDurationCanvas::connexion(UmlCode l, const QPoint & s, const QPoint & e)
{
    // for lost/found msg, 'this' starts or ends msg depending on l
    SdMsgSupport * start;
    SdMsgSupport * dest;
    int v;

    switch (l) {
    case UmlFoundSyncMsg:
    case UmlFoundAsyncMsg:

        // 'this' is dest
        if (((BrowserSeqDiagram *) browser_node)->is_overlapping_bars() &&
            (!msgs.isEmpty() || !durations.isEmpty())) { // 'this' not just created
            dest = new SdDurationCanvas(the_canvas(), this, s.y(), TRUE);
            update_v_to_contain((SdDurationCanvas *) dest, FALSE);
        }
        else
            dest = this;

        v = s.y();
        start = new SdLostFoundMsgSupportCanvas(the_canvas(), s.x() - LOSTFOUND_SIZE / 2,
                                                v - LOSTFOUND_SIZE / 2, 0);
        start->show();
        break;

    case UmlLostSyncMsg:
    case UmlLostAsyncMsg:
        // 'this' is start
        start = this;
        v = e.y();
        dest = new SdLostFoundMsgSupportCanvas(the_canvas(), e.x() - LOSTFOUND_SIZE / 2,
                                               v - LOSTFOUND_SIZE / 2, 0);
        dest->show();
        break;

    default:
        return FALSE;
    }

    (new SdMsgCanvas(the_canvas(), start, dest, l, v - MSG_HEIGHT / 2, 0))->upper();

    return TRUE;
}

void SdDurationCanvas::add(SdMsgBaseCanvas * m)
{
    msgs.append(m);
}

void SdDurationCanvas::remove(SdMsgBaseCanvas * m)
{
    msgs.remove(m);

    if (msgs.isEmpty() && durations.isEmpty())
        // now useless, remove it
        delete_it();
}

void SdDurationCanvas::add(SdDurationCanvas * d)
{
    durations.append(d);
}

void SdDurationCanvas::remove(SdDurationCanvas * d)
{
    durations.remove(d);

    if (msgs.isEmpty() && durations.isEmpty())
        // now useless, remove it
        delete_it();
}

void SdDurationCanvas::cut_internal(int py)
{
    foreach (SdDurationCanvas *canvas, durations)
        canvas->cut_internal(py);

    QRect r = rect();
    resize(width(), py - r.top());
    SdDurationCanvas * newone = 0;

    foreach (SdMsgBaseCanvas *canvas, msgs) {
        if (canvas->rect().center().y() > py) {
            // on the new duration
            if (newone == 0) {
                // width and height to 0 on creation to not have
                // a round error because of the zoom
                newone = new SdDurationCanvas(the_canvas(), support, r.x(), py, 0, 0, 0, coregion);
                newone->resize(width(), r.bottom() - py);
                newone->itscolor = itscolor;
                newone->setZ(z());
                newone->show();
            }

            remove(canvas);
            newone->add(canvas);
            canvas->change_duration(this, newone);
        }
    }

    foreach (SdDurationCanvas *canvas, durations) {
        if (canvas->rect().center().y() > py) {
            // on the new duration
            if (newone == 0) {
                // width and height to 0 on creation to not have
                // a round error because of the zoom
                newone = new SdDurationCanvas(the_canvas(), support, r.x(), py, 0, 0, 0, coregion);
                newone->resize(width(), r.bottom() - py);
                newone->itscolor = itscolor;
                newone->setZ(z());
                newone->show();
            }

            remove(canvas);
            newone->add(canvas);
            canvas->support =  newone;
        }

        canvas->update_v_to_contain(canvas, TRUE);
    }
}

void SdDurationCanvas::cut(const QPoint & p)
{
    cut_internal(p.y());
    package_modified();
}

void SdDurationCanvas::merge(QList<SdDurationCanvas *> & l)
{
    l.remove(this);

    QRect r = rect();
    int vmin = r.top();
    int vmax = r.bottom();

    foreach (SdDurationCanvas *d, l) {
        QRect dr = d->rect();

        if (dr.top() < vmin)
            vmin = dr.top();

        if (dr.bottom() > vmax)
            vmax = dr.bottom();

        collapse(d);
    }

    if (vmin < r.top())
        Q3CanvasItem::moveBy(0, vmin - r.top());

    resize(r.width(), vmax - vmin + 1);
    update_self();
}

void SdDurationCanvas::collapse(SdDurationCanvas * d)
{
    foreach (SdMsgBaseCanvas *m, d->msgs) {
        msgs << m;
        m->change_duration(d, this);
    }
    d->msgs.clear();

    foreach (SdDurationCanvas *dur, d->durations) {
        durations << dur;
        dur->support = this;
    }
    d->durations.clear();

    d->delete_it();
}

void SdDurationCanvas::go_up(SdMsgBaseCanvas * m, bool isdest)
{
    // create a new overlapping bar and move m in it
    int v = (m->type() != UmlSelfReturnMsg)
            ? (int) m->y()
            : (int)(m->y() - DURATION_MIN_HEIGHT * the_canvas()->zoom() + m->rect().height() / 2);
    SdDurationCanvas * d =
        new SdDurationCanvas(the_canvas(), this, v, isdest);
    update_v_to_contain(d, FALSE);

    d->msgs.append(m);
    m->change_duration(this, d);
    d->update_hpos(); // update sub duration and msg hpos
    d->update_self();
    msgs.remove(m);
    m->upper();
}

void SdDurationCanvas::go_down(SdMsgBaseCanvas * m)
{
    // move m in parent bar
    SdDurationCanvas * d = (SdDurationCanvas *) support;

    d->msgs.append(m);
    m->change_duration(this, d);
    d->update_hpos(); // update sub duration and msg hpos
    d->update_self();
    remove(m);
}

void SdDurationCanvas::toFlat()
{
    while (! durations.isEmpty()) {
        SdDurationCanvas * d = durations.first();

        d->toFlat();
        collapse(d);
    }

    if (! isOverlappingDuration())
        update_hpos(); // update msg hpos
}

void SdDurationCanvas::toOverlapping()
{
    // reorder messages, few messages => use bubble sort
    unsigned sz = msgs.count();
    SdMsgBaseCanvas ** v = new SdMsgBaseCanvas *[sz];
    unsigned index = 0;

    while (! msgs.isEmpty())
        v[index++] = msgs.takeFirst();

    unsigned sup = sz;

    do {
        unsigned upper = 0;

        for (index = 1; index < sup; index += 1) {
            if (v[index - 1]->y() > v[index]->y()) {
                SdMsgBaseCanvas * m = v[index - 1];

                v[index - 1] = v[index];
                v[index] = m;
                upper = index;
            }
        }

        sup = upper;
    }
    while (sup != 0);

    index = 0;
    toOverlapping(v, this, index, sz);

    delete [] v;

    postToOverlapping();
}

void SdDurationCanvas::toOverlapping(SdMsgBaseCanvas ** v,
                                     SdDurationCanvas * orig,
                                     unsigned & index, unsigned sz)
{
    bool first = TRUE;

    while (index != sz) {
        SdMsgBaseCanvas * m = v[index++];
        int d = m->overlap_dir(orig);

        switch (d) {
        case -1:
            // return
            msgs.append(m);

            if (this != orig) {
                m->change_duration(orig, this);
                return;
            }

            // redondant return
            break;

        default: // 1 or 2
            if (! first) {
                SdDurationCanvas * sd =
                    new SdDurationCanvas(the_canvas(), this, (int) m->y(), TRUE);

                if (d == 1) {
                    index -= 1;
                    sd->toOverlapping(v, orig, index, sz);
                }
                else {
                    // asynch, consider exec done immediately
                    sd->msgs.append(m);
                    m->change_duration(orig, sd);
                }

                break;
            }

            // no break;
        case 0:
            msgs.append(m);

            if (this != orig)
                m->change_duration(orig, this);

            break;
        }

        first = FALSE;
    }
}

void SdDurationCanvas::postToOverlapping()
{
    foreach (SdDurationCanvas *canvas, durations)
        canvas->postToOverlapping();

    foreach (SdMsgBaseCanvas *canvas, msgs) {
        canvas->upper();
        canvas->update_hpos();
        canvas->DiagramItem::update();	// not QCanvasItem::update
    }
}

void SdDurationCanvas::open()
{
    // package_modified();
}

void SdDurationCanvas::modified()
{
    // force son reaffichage
    hide();
    show();
    canvas()->update();
    package_modified();
}

void SdDurationCanvas::menu(const QPoint & p)
{
    Q3PopupMenu m(0);
    Q3CanvasItemList items = collisions(TRUE);
    QList<SdDurationCanvas *> l;
    Q3CanvasItemList::ConstIterator it;
    Q3CanvasItemList::ConstIterator end = items.end();

    for (it = items.begin(); it != end; ++it) {
        if ((*it)->visible()) {
            DiagramItem * di = QCanvasItemToDiagramItem(*it);

            if ((di != 0) &&
                (di->type() == UmlActivityDuration) &&
                (((SdDurationCanvas *) di)->support == support))
                l.append((SdDurationCanvas *) di);
        }
    }

    MenuFactory::createTitle(m, TR("Activity bar"));
    m.insertSeparator();
    m.insertItem(TR("Upper"), 0);
    m.insertItem(TR("Lower"), 1);
    m.insertItem(TR("Go up"), 9);
    m.insertItem(TR("Go down"), 10);
    m.insertSeparator();
    m.insertItem((coregion) ? TR("Draw as activity bar") :  TR("Draw as a coregion"), 7);
    m.insertItem(TR("Edit drawing settings"), 2);
    m.insertSeparator();
    m.insertItem(TR("Select linked items"), 3);
    m.insertSeparator();
    m.insertItem(TR("Remove from diagram"), 4);
    m.insertSeparator();
    m.insertItem(TR("Cut here"), 5);

    if (!l.isEmpty())
        m.insertItem(TR("Merge juxtaposed activity bars"), 6);

    if (support->isaDuration())
        m.insertItem(TR("Collapse in parent bar"), 8);

    switch (m.exec(QCursor::pos())) {
    case 0:
        upper();
        modified();
        return;

    case 1:
        lower();
        modified();
        return;

    case 9:
        z_up();
        modified();	// call package_modified()
        return;

    case 10:
        z_down();
        modified();	// call package_modified()
        return;

    case 2:
        edit_drawing_settings();
        return;

    case 3:
        select_associated();
        break;

    case 4:
        delete_it();
        package_modified();
        break;

    case 5:
        cut(p);
        package_modified();
        break;

    case 6:
        merge(l);
        package_modified();
        break;

    case 7:
        coregion = !coregion;
        modified();
        return;

    case 8: {
        SdDurationCanvas * d = (SdDurationCanvas *) support;

        d->collapse(this);
        d->update_hpos(); // update sub duration and msg hpos
        d->update_self();
    }

    package_modified();
    break;

    default:
        return;
    }

    canvas()->update();
}

void SdDurationCanvas::apply_shortcut(QString s)
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
    else
        return;

    modified();
}

void SdDurationCanvas::edit_drawing_settings()
{
    for (;;) {
        ColorSpecVector co(1);

        co[0].set(TR("duration color"), &itscolor);

        SettingsDialog dialog(0, &co, FALSE);

        dialog.raise();

        if (dialog.exec() == QDialog::Accepted)
            modified();

        if (!dialog.redo())
            break;
    }
}

bool SdDurationCanvas::has_drawing_settings() const
{
    return TRUE;
}

void SdDurationCanvas::edit_drawing_settings(QList<DiagramItem *> & l)
{
    for (;;) {
        ColorSpecVector co(1);
        UmlColor itscolor;

        co[0].set(TR("duration color"), &itscolor);

        SettingsDialog dialog(0, &co, FALSE, TRUE);

        dialog.raise();

        if ((dialog.exec() == QDialog::Accepted) && !co[0].name.isEmpty()) {
            foreach (DiagramItem *item, l) {
                SdDurationCanvas *canvas = (SdDurationCanvas *)item;
                canvas->itscolor = itscolor;
                canvas->modified();
            }
        }

        if (!dialog.redo())
            break;
    }
}

void SdDurationCanvas::clone_drawing_settings(const DiagramItem *src)
{
    const SdDurationCanvas * x = (const SdDurationCanvas *) src;
    itscolor = x->itscolor;
    modified();
}

aCorner SdDurationCanvas::on_resize_point(const QPoint & p)
{
    return ::on_resize_point(p, rect());
}

// redefined to not change width_scale100 and center_x_scale100
void SdDurationCanvas::resize(int wi, int he)
{
    setSize(wi, he);

    if (!((UmlCanvas *) canvas())->do_zoom()) {
        double zoom = the_canvas()->zoom();

        height_scale100 = (int)(he / zoom + 0.5);

        QPoint c = center();

        center_y_scale100  = (int)(c.y() / zoom + 0.5);
    }
}

// note : don't consider the messages
// size will be rectified if needed by the messages calling
// update_v_to_contain()
void SdDurationCanvas::resize(aCorner c, int, int dy, QPoint & o)
{
    if ((dy < 0) && ((c == UmlTopLeft) || (c == UmlTopRight))) {
        double my = min_y();

        if ((y() + dy) < my)
            dy = (int)(my - y() + 0.5);
    }

    double zoom = the_canvas()->zoom();

    DiagramCanvas::resize(c, 0, dy, o,
                          (int)(DURATION_WIDTH * zoom),
                          (int)(DURATION_MIN_HEIGHT * zoom));

    support->update_v_to_contain(this, FALSE);
    support->update_instance_dead();
}

void SdDurationCanvas::resize(const QSize & sz, bool, bool h)
{
    double zoom = the_canvas()->zoom();

    if (DiagramCanvas::resize(sz, FALSE, h,
                              (int)(DURATION_WIDTH * zoom),
                              (int)(DURATION_MIN_HEIGHT * zoom))) {
        support->update_v_to_contain(this, FALSE);
        support->update_instance_dead();
    }
}

void SdDurationCanvas::select_associated()
{
    if (! selected())
        the_canvas()->select(this);

    foreach (SdMsgBaseCanvas *canvas, msgs)
        if (! canvas->selected())
            canvas->select_associated();
}

bool SdDurationCanvas::copyable() const
{
    if (! selected())
        return FALSE;

    foreach (SdMsgBaseCanvas *canvas, msgs)
        if (canvas->copyable())
            return TRUE;

    return FALSE;
}

void SdDurationCanvas::update_instance_dead()
{
    // do nothing
}

int SdDurationCanvas::sub_x(int sub_w) const
{
    QRect r = rect();

    return r.right() - sub_w / 2 + 1;
}

SdLifeLineCanvas * SdDurationCanvas::get_line() const
{
    return support->get_line();
}

bool SdDurationCanvas::isaDuration() const
{
    return true;
}

bool SdDurationCanvas::isOverlappingDuration() const
{
    return support->isaDuration();
}

double SdDurationCanvas::getZ() const
{
    return z();
}

void SdDurationCanvas::propag_visible(QList<SdDurationCanvas *> & l, bool y)
{
    foreach (SdDurationCanvas *d, l) {
        d->Q3CanvasItem::setVisible(y);

        foreach (SdMsgBaseCanvas *msg, d->msgs)
            msg->Q3CanvasItem::setVisible(y);

        propag_visible(d->durations, y);
    }
}

void SdDurationCanvas::propag_dz(QList<SdDurationCanvas *> & l, double dz)
{
    foreach (SdDurationCanvas *d, l) {
        d->Q3CanvasItem::setZ(d->z() + dz);
        propag_dz(d->durations, dz);
    }
}

void SdDurationCanvas::save_sub(QTextStream & st) const
{
    nl_indent(st);
    st << "overlappingdurationcanvas " << get_ident();
    save_internal(st);
}

void SdDurationCanvas::save_internal(QTextStream & st) const
{
    indent(+1);

    if (coregion) {
        nl_indent(st);
        st << "coregion";
    }

    if (itscolor != UmlDefaultColor) {
        nl_indent(st);
        st << "color " << stringify(itscolor);
    }

    nl_indent(st);
    save_xyzwh(st, this, "xyzwh");

    foreach (SdDurationCanvas *canvas, durations)
        canvas->save_sub(st);

    indent(-1);
    nl_indent(st);
    st << "end";
}

void SdDurationCanvas::save(QTextStream & st, bool ref, QString & warning) const
{
    if (ref)
        st << "durationcanvas_ref " << get_ident();
    else if (!support->isaDuration()) {
        nl_indent(st);
        st << "durationcanvas " << get_ident() << " ";
        ((DiagramCanvas *)((SdLifeLineCanvas *) support)->get_obj())->save(st, TRUE, warning);
        save_internal(st);
    }
}

SdDurationCanvas * SdDurationCanvas::read(char *& st, UmlCanvas * canvas,
        bool ref)
{
    return read(st, canvas,
                read_keyword(st, (ref) ? "durationcanvas_ref"
                             : "durationcanvas"));
}

SdDurationCanvas * SdDurationCanvas::read_internal(char *& st,
        UmlCanvas * canvas, int id,
        SdDurationSupport * sp)
{
    UmlColor color = UmlDefaultColor;
    bool coreg = FALSE;
    char * k = read_keyword(st);

    if (!strcmp(k, "coregion")) {
        coreg = TRUE;
        k = read_keyword(st);
    }

    read_color(st, "color", color, k);

    if (strcmp(k, "xyzwh"))
        wrong_keyword(k, "xyzwh");

    int x = (int) read_double(st);
    int y = (int) read_double(st);
    double z = read_double(st);
    (void) read_double(st);	// width not used to bypass old bug

    SdDurationCanvas * result =
        new SdDurationCanvas(canvas, sp, x, y, DURATION_WIDTH,
                             (int) read_double(st), id, coreg);

    result->itscolor = color;
    result->setZ(z);
    result->update_hpos();	// in case the current font is not the original one
    result->set_center100();
    result->show();

    if (read_file_format() >= 48) {
        k = read_keyword(st);

        while (!strcmp(k, "overlappingdurationcanvas")) {
            (void) read_internal(st, canvas, read_id(st), result);
            k = read_keyword(st);
        }

        if (strcmp(k, "end"))
            wrong_keyword(k, "end");
    }

    return result;
}

SdDurationCanvas * SdDurationCanvas::read(char *& st, UmlCanvas * canvas, char * k)
{
    if (!strcmp(k, "durationcanvas_ref"))
        return ((SdDurationCanvas *) dict_get(read_id(st), "durationcanvas", canvas));
    else if (!strcmp(k, "durationcanvas")) {
        int id = read_id(st);
        SdObjCanvas * o;

        k = read_keyword(st);

        if ((o = SdClassInstCanvas::read(st, canvas, k)) == 0)
            wrong_keyword(st, k);

        return read_internal(st, canvas, id, o->get_life_line());
    }
    else
        return 0;
}

void SdDurationCanvas::history_hide()
{
    Q3CanvasItem::setVisible(FALSE);
    disconnect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
}

void SdDurationCanvas::history_save(QBuffer & b) const
{
    DiagramCanvas::history_save(b);
    ::save(width_scale100, b);
    ::save(height_scale100, b);
    ::save(width(), b);
    ::save(height(), b);
    ::save_ptr(support, b);

    ::save((int) msgs.count(), b);

    foreach (SdMsgBaseCanvas *canvas, msgs)
        ::save(canvas, b);

    ::save((int) durations.count(), b);

    foreach (SdDurationCanvas *canvas, durations)
        ::save(canvas, b);
}

void SdDurationCanvas::history_load(QBuffer & b)
{
    DiagramCanvas::history_load(b);
    ::load(width_scale100, b);
    ::load(height_scale100, b);

    int w, h;

    ::load(w, b);
    ::load(h, b);
    Q3CanvasRectangle::setSize(w, h);

    support = (SdDurationSupport *) ::load_ptr(b);

    int n;

    ::load(n, b);
    msgs.clear();

    while (n--)
        msgs.append((SdMsgBaseCanvas *) ::load_item(b));

    ::load(n, b);
    durations.clear();

    while (n--)
        durations.append((SdDurationCanvas *) ::load_item(b));

    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
}

// for plug out

unsigned SdDurationCanvas::count_msg(int api_format) const
{
    unsigned count = 0;
    double maxy = 0;
    bool isreturn = FALSE;

    foreach (SdMsgBaseCanvas *canvas, msgs) {
        switch (canvas->type()) {
        case UmlSyncMsg:
        case UmlAsyncMsg:
        case UmlReturnMsg:
            if (canvas->get_dest() == (const SdMsgSupport *) this)
                break;

            // no break
        default:
            // msg starts from duration or duration ends a found msg
            count += 1;
        }

        if (canvas->y() > maxy) {
            maxy = canvas->y();

            switch (canvas->type()) {
            case UmlReturnMsg:
            case UmlSelfReturnMsg:
                isreturn = TRUE;
                break;

            default:
                isreturn = FALSE;
            }
        }
    }

    if (!isreturn)
        count += 1;

    foreach (SdDurationCanvas *canvas, durations)
        count += canvas->count_msg(api_format);

    return count;
}

void SdDurationCanvas::send(ToolCom * com, int id) const
{
    double maxy = 0;
    bool isreturn = FALSE;

    foreach (SdMsgBaseCanvas *canvas, msgs) {
        switch (canvas->type()) {
        case UmlSyncMsg:
        case UmlAsyncMsg:
        case UmlReturnMsg:
            if (canvas->get_dest() == (const SdMsgSupport *) this)
                break;

            // no break
        default:
            // msg start from duration, except for found msg
            canvas->send(com, id);
        }

        if (canvas->y() > maxy) {
            maxy = canvas->y();

            switch (canvas->type()) {
            case UmlReturnMsg:
            case UmlSelfReturnMsg:
                isreturn = TRUE;
                break;

            default:
                isreturn = FALSE;
            }
        }
    }

    if (!isreturn)
        SdMsgBaseCanvas::send(com, id, (unsigned) x() + width(),
                              (unsigned) y() + height(),
                              anImplicitReturn, "", "", "");

    foreach (SdDurationCanvas *canvas, durations)
        canvas->send(com, id);
}
