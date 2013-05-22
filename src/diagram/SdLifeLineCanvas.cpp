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
#include <q3ptrdict.h>
//Added by qt3to4:
#include <QTextStream>

#include "SdLifeLineCanvas.h"
#include "SdObjCanvas.h"
#include "SdDurationCanvas.h"
#include "SdMsgBaseCanvas.h"
#include "FragmentCanvas.h"
#include "ui/menufactory.h"
#include "ToolCom.h"
#include "myio.h"
#include "translate.h"

#define LIFE_LINE_WIDTH 7
#define LIFE_LINE_HEIGHT 10000

SdLifeLineCanvas::SdLifeLineCanvas(UmlCanvas * canvas, SdObjCanvas * o)
    : DiagramCanvas(0, canvas, 0, 0, LIFE_LINE_WIDTH, LIFE_LINE_HEIGHT, -1),
      obj(o), end(LIFE_LINE_HEIGHT)
{
    update_pos();
    set_center100();
    setZ(DIAGRAMCANVAS_Z);

    if (obj->is_mortal())
        update_instance_dead();	// call show
    else
        show();

    if (canvas->paste())
        // must not be selected else the move done after if wrong
        canvas->unselect(this);
}

SdLifeLineCanvas::~SdLifeLineCanvas()
{
}

void SdLifeLineCanvas::delete_it()
{
    while (!durations.isEmpty())
        durations.first()->delete_it();	// will remove the msgs

    DiagramCanvas::delete_it();
}

void SdLifeLineCanvas::update_pos()
{
    move(obj->x() + (obj->width() - width()) / 2,
         obj->y() + LIFE_LINE_TOPOFFSET * the_canvas()->zoom() + 100000);
}

bool SdLifeLineCanvas::is_decenter(const QPoint &, BooL &) const
{
    return FALSE;
}

void SdLifeLineCanvas::change_scale()
{
    double scale = the_canvas()->zoom();
    int w = (int)(width_scale100 * scale);
    // force odd width
    setSize(((w & 1) == 0) ? w + 1 : w, LIFE_LINE_HEIGHT);
    // don't move, the position is updated with its obj
}

void SdLifeLineCanvas::drawShape(QPainter & p)
{
    if (end == 0)
        // masked by user
        return;

    p.setBackgroundMode(::Qt::OpaqueMode);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(::Qt::DashLine);

    int m = (int)(x() + width() / 2);
    FILE * fp = svg();

    p.drawLine(m, (int) y(), m, end);

    p.setPen(::Qt::SolidLine);

    if (end != LIFE_LINE_HEIGHT) {
        int b = end + (int) width();

        p.drawLine((int) x(), end,
                   (int)(x() + width()) - 1, b);
        p.drawLine((int)(x() + width() - 1), end,
                   (int) x(), b);

        if (fp != 0) {
            fprintf(fp, "<g>\n"
                    "\t<line stroke=\"black\" stroke-dasharray=\"18,6\"  stroke-opacity=\"1\""
                    " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n"
                    "</g>\n",
                    m, (int) y(), m, end);

            fprintf(fp, "<g>\n"
                    "\t<line stroke=\"black\" stroke-opacity=\"1\""
                    " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                    (int) x(), end, (int)(x() + width()) - 1, b);
            fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                    " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n"
                    "</g>\n",
                    (int)(x() + width() - 1), end, (int) x(), b);
        }
    }
    else if (fp != 0)
        fprintf(fp, "<g>\n"
                "\t<line stroke=\"black\" stroke-dasharray=\"18,6\"  stroke-opacity=\"1\""
                " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n"
                "</g>\n",
                m, (int) y(), m, svg_height());
}

void SdLifeLineCanvas::moveBy(double dx, double dy)
{
    DiagramCanvas::moveBy(dx, (dy > 80000) ? dy - 100000 : /*dy*/0);

    if ((dy < 80000) && !obj->selected())
        // not called by update_pos() which is only called by obj->moveBy()
        obj->moveBy(dx, 0);

    foreach (SdDurationCanvas *canvas, durations)
        canvas->update_hpos();
}

UmlCode SdLifeLineCanvas::type() const
{
    return UmlLifeLine;
}

double SdLifeLineCanvas::instance_max_y() const
{
    double miny = 100000;

    foreach (SdDurationCanvas *canvas, durations)
        if (canvas->y() < miny)
            miny = canvas->y();

    return miny - LIFE_LINE_TOPOFFSET * the_canvas()->zoom() - 5;
}

DiagramItem::LineDirection SdLifeLineCanvas::allowed_direction(UmlCode)
{
    return DiagramItem::Horizontal;
}

QString SdLifeLineCanvas::may_start(UmlCode & l) const
{
    return (l != UmlAnchor) ? QString() : TR("illegal");
}

QString SdLifeLineCanvas::may_connect(UmlCode & l, const DiagramItem * dest) const
{
    switch (dest->type()) {
    case UmlActivityDuration:
    case UmlLifeLine:
        return (l != UmlAnchor) ? QString() : TR("illegal");

    default:
        return TR("illegal");
    }
}

bool SdLifeLineCanvas::may_connect(UmlCode l) const
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

void SdLifeLineCanvas::connexion(UmlCode l, DiagramItem * dest,
                                 const QPoint & s, const QPoint & e)
{
    switch (l) {
    default:
        if (dest == this)
            break;

        // no break
    case UmlSyncSelfMsg:
    case UmlAsyncSelfMsg:
    case UmlSelfReturnMsg:
        (new SdDurationCanvas(the_canvas(), this, s.y(), FALSE))->connexion(l, dest, s, e);
    }
}

bool SdLifeLineCanvas::connexion(UmlCode l, const QPoint & s, const QPoint & e)
{
    // for lost/found msg
    // this starts or ends msg depending on l
    switch (l) {
    case UmlFoundSyncMsg:
    case UmlFoundAsyncMsg:
        return (new SdDurationCanvas(the_canvas(), this, s.y(), TRUE))->connexion(l, s, e);

    case UmlLostSyncMsg:
    case UmlLostAsyncMsg:
        return (new SdDurationCanvas(the_canvas(), this, s.y(), FALSE))->connexion(l, s, e);

    default:
        return FALSE;
    }
}

void SdLifeLineCanvas::add(SdDurationCanvas * d)
{
    durations.append(d);
    update_instance_dead();
}

void SdLifeLineCanvas::remove(SdDurationCanvas * d)
{
    durations.remove(d);
    update_instance_dead();
}

void SdLifeLineCanvas::toFlat()
{
    foreach (SdDurationCanvas *canvas, durations)
        canvas->toFlat();
}

void SdLifeLineCanvas::toOverlapping()
{
    foreach (SdDurationCanvas *canvas, durations)
        canvas->toOverlapping();
}

void SdLifeLineCanvas::set_masked(bool y)
{
    end = (y) ? 0 : LIFE_LINE_HEIGHT;
    hide();
    show();
}

void SdLifeLineCanvas::update_instance_dead()
{
    if (obj->is_mortal() && !durations.isEmpty()) {
        int new_end = 0;

        foreach (SdDurationCanvas *canvas, durations) {
            int b = (int)(canvas->y() + canvas->height());

            if (b > new_end)
                new_end = b;
        }

        if (new_end != end) {
            end = new_end;
            hide();
            show();
        }
    }
    else if ((end != 0) && (end != LIFE_LINE_HEIGHT)) {
        end = LIFE_LINE_HEIGHT;
        hide();
        show();
    }
}

void SdLifeLineCanvas::update_v_to_contain(SdDurationCanvas *, bool)
{
    update_instance_dead();
}

int SdLifeLineCanvas::sub_x(int sub_w) const
{
    QRect r = rect();

    return (r.left() + r.right() - sub_w) / 2 + 1;
}

double SdLifeLineCanvas::min_y() const
{
    return y() + 6 * the_canvas()->zoom();
}

SdLifeLineCanvas * SdLifeLineCanvas::get_line() const
{
    return (SdLifeLineCanvas *) this;
}

bool SdLifeLineCanvas::isaDuration() const
{
    return false;
}

double SdLifeLineCanvas::getZ() const
{
    return z();
}

void SdLifeLineCanvas::open()
{
}

void SdLifeLineCanvas::exec_menu(int rank)
{
    double old_z = z();

    switch (rank) {
    case 0:
        SdDurationCanvas::propag_visible(durations, FALSE);
        upper();
        SdDurationCanvas::propag_visible(durations, TRUE);
        break;

    case 1:
        lower();
        break;

    case 2:
        SdDurationCanvas::propag_visible(durations, FALSE);
        z_up();
        SdDurationCanvas::propag_visible(durations, TRUE);
        break;

    case 3:
        z_down();
        break;

    default:
        return;
    }

    double dz = z() - old_z;

    if (dz != 0) {
        SdDurationCanvas::propag_dz(durations, dz);

        // force son reaffichage
        hide();
        show();
        canvas()->update();
        package_modified();
    }
}

void SdLifeLineCanvas::menu(const QPoint &)
{
    // delete must call SdObjCanvas->delete_it() NOT the own delete_it !
    Q3PopupMenu m(0);

    MenuFactory::createTitle(m, TR("Life line"));
    m.insertSeparator();
    m.insertItem(TR("Upper"), 0);
    m.insertItem(TR("Lower"), 1);
    m.insertItem(TR("Go up"), 2);
    m.insertItem(TR("Go down"), 3);

    exec_menu(m.exec(QCursor::pos()));
}

void SdLifeLineCanvas::apply_shortcut(QString s)
{
    if (s == "Upper")
        exec_menu(0);
    else if (s == "Lower")
        exec_menu(1);
    else if (s == "Go up")
        exec_menu(2);
    else if (s == "Go down")
        exec_menu(3);
    else
        return;
}

bool SdLifeLineCanvas::copyable() const
{
    foreach (SdDurationCanvas *canvas, durations)
        if (canvas->copyable())
            return TRUE;

    return FALSE;
}

void SdLifeLineCanvas::save(QTextStream & st, bool, QString &) const
{
    st << "\nERROR SdLifeLineCanvas::save must not be called\n";
}

void SdLifeLineCanvas::history_save(QBuffer & b) const
{
    DiagramCanvas::history_save(b);
    ::save(end, b);
    ::save((int) durations.count(), b);

    foreach (SdDurationCanvas *canvas, durations)
        ::save(canvas, b);
}

void SdLifeLineCanvas::history_load(QBuffer & b)
{
    DiagramCanvas::history_load(b);
    ::load(end, b);

    int n;

    ::load(n, b);
    durations.clear();

    while (n--)
        durations.append((SdDurationCanvas *) ::load_item(b));
}

void SdLifeLineCanvas::send(ToolCom * com, const Q3CanvasItemList & l,
                            QList<FragmentCanvas *> & fragments,
                            QList<FragmentCanvas *> & refs)
{
    int api_format = com->api_format();
    Q3PtrDict<SdLifeLineCanvas> used_refs; // the key is the fragment ref
    QList<SdLifeLineCanvas *> lls;
    Q3CanvasItemList::ConstIterator cit;
    unsigned n = 0;

    // count msgs

    for (cit = l.begin(); cit != l.end(); ++cit) {
        DiagramItem * it = QCanvasItemToDiagramItem(*cit);

        if ((it != 0) && // an uml canvas item
            (*cit)->visible() &&
            (it->type() == UmlLifeLine)) {
            SdLifeLineCanvas * ll = (SdLifeLineCanvas *) it;

            lls.append(ll);

            // standard msgs
            foreach (SdDurationCanvas *canvas, ll->durations)
                n += canvas->count_msg(api_format);

            if (com->api_format() >= 41) {
                if (ll->end != LIFE_LINE_HEIGHT)
                    // deletion message, ll can't masked by user
                    n += 1;

                foreach (FragmentCanvas *f, refs) {
                    if (f->collidesWith(ll)) {
                        // interaction use message
                        if (used_refs.find((void *) f) == 0) {
                            n += 1;
                            used_refs.insert((void *) f, ll);
                        }
                    }
                }
            }
        }
    }

    // write messages

    com->write_unsigned(n);

    foreach (SdLifeLineCanvas *ll, lls) {
        int id = ll->obj->get_ident();

        // write standard messages
        foreach (SdDurationCanvas *d, ll->durations)
            d->send(com, id);

        if ((ll->end != LIFE_LINE_HEIGHT) && (com->api_format() >= 41)) {
            // deletion message, lf can't masked by user
            int m = ll->width() / 2;

            SdMsgBaseCanvas::send(com, id, (unsigned) ll->x() + m,
                                  (unsigned) ll->end + m, aDestruction, "", "", "");
        }
    }

    if (com->api_format() >= 41) {
        // interaction use messages
        Q3PtrDictIterator<SdLifeLineCanvas>itref(used_refs);
        SdLifeLineCanvas *ll;

        while ((ll = itref.current()) != 0) {
            FragmentCanvas * f = (FragmentCanvas *) itref.currentKey();
            int m = ll->width() / 2;

            SdMsgBaseCanvas::send(com, ll->obj->get_ident(),
                                  (unsigned) ll->x() + m, (unsigned) f->center().y(),
                                  anInteractionUse, "", "", f->arguments());

            ++itref;
        }

        // send life lines covered by fragment
        foreach (FragmentCanvas *f, fragments) {
            QList<SdLifeLineCanvas *> covered;

            foreach (SdLifeLineCanvas *ll, lls)
                if (f->collidesWith(ll))
                    covered.append(ll);

            com->write_unsigned(covered.count());

            foreach (SdLifeLineCanvas *ll, covered)
                com->write_unsigned((unsigned) ll->obj->get_ident());
        }
    }
}
