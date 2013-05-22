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





#include <qcursor.h>
#include <qpainter.h>
#include <q3popupmenu.h>
//Added by qt3to4:
#include <QTextStream>
#include <Q3PointArray>

#include "SdMsgCanvas.h"
#include "SdDurationCanvas.h"
#include "SdLostFoundMsgSupportCanvas.h"
#include "OperationData.h"
#include "UmlCanvas.h"
#include "LabelCanvas.h"
#include "Settings.h"
#include "SettingsDialog.h"
#include "BrowserSeqDiagram.h"
#include "myio.h"
#include "ui/menufactory.h"
#include "translate.h"

SdMsgCanvas::SdMsgCanvas(UmlCanvas * canvas, SdMsgSupport * s,
                         SdMsgSupport * d, UmlCode l, int v, int id)
    : SdMsgBaseCanvas(canvas, d, l, v, id), start(s)
{
    start->add(this);
    dest->add(this);

    update_hpos();
    set_center100();
    show();
}

SdMsgCanvas::~SdMsgCanvas()
{
}

void SdMsgCanvas::delete_it()
{
    start->remove(this);
    SdMsgBaseCanvas::delete_it();
}

void SdMsgCanvas::update_hpos()
{
    const QRect sr = start->rect();
    const QRect dr = dest->rect();
    LabelCanvas * lbl = label;
    int cy = center_y_scale100;

    if (the_canvas()->do_zoom())
        // the label is moved independently
        label = 0;

    if (sr.left() < dr.left()) {
        double dx = sr.right() + 1 - x();

        DiagramCanvas::moveBy(dx, 0);
        setSize(dr.left() - sr.right(), MSG_HEIGHT);

        if (!the_canvas()->do_zoom() &&
            (stereotype != 0) &&
            !stereotype->selected())
            stereotype->moveBy(dx, 0);
    }
    else {
        double dx = dr.right() + 1 - x();

        DiagramCanvas::moveBy(dx, 0);
        setSize(sr.left() - dr.right(), MSG_HEIGHT);

        if (!the_canvas()->do_zoom() &&
            (stereotype != 0) &&
            !stereotype->selected())
            stereotype->moveBy(dx, 0);
    }

    center_y_scale100 = cy;	// updated later

    label = lbl;
}

double SdMsgCanvas::min_y() const
{
    return (start->min_y() < dest->min_y())
           ? start->min_y()
           : dest->min_y();
}

void SdMsgCanvas::set_z(double newz)
{
    DiagramCanvas::set_z(newz);

    if (! start->isaDuration())
        start->set_z(newz);
    else if (! dest->isaDuration())
        dest->set_z(newz);
}

void SdMsgCanvas::draw(QPainter & p)
{
    const QRect r = rect();
    const int v = r.center().y();
    Q3PointArray poly(3);
    FILE * fp = svg();
    p.setRenderHint(QPainter::Antialiasing, true);

    if (itsType == UmlReturnMsg)
        p.setPen(::Qt::DotLine);

    p.drawLine(r.left(), v, r.right(), v);

    if (fp != 0) {
        fputs("<g>\n\t<line stroke=\"black\" stroke-opacity=\"1\"", fp);

        if (itsType == UmlReturnMsg)
            fputs(" stroke-dasharray=\"4,4\"", fp);

        fprintf(fp, " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                r.left(), v, r.right(), v);
    }

    if (start->rect().left() < dest->rect().left()) {
        poly.setPoint(0, r.right(), v);
        poly.setPoint(1, r.right() - MSG_HEIGHT / 2 + 1, v - MSG_HEIGHT / 2 + 1);
        poly.setPoint(2, r.right() - MSG_HEIGHT / 2 + 1, v + MSG_HEIGHT / 2 - 1);
    }
    else {
        poly.setPoint(0, r.left(), v);
        poly.setPoint(1, r.left() + MSG_HEIGHT / 2 - 1, v - MSG_HEIGHT / 2 + 1);
        poly.setPoint(2, r.left() + MSG_HEIGHT / 2 - 1, v + MSG_HEIGHT / 2 - 1);
    }

    switch (itsType) {
    case UmlSyncMsg:
    case UmlFoundSyncMsg:
    case UmlLostSyncMsg: {
        QBrush brsh = p.brush();

        p.setBrush(Qt::black);
        p.drawPolygon(poly/*, TRUE*/);
        p.setBrush(brsh);

        if (fp != 0) {
            draw_poly(fp, poly, UmlBlack, FALSE);
            fputs("</g>\n", fp);
        }
    }
    break;

    case UmlReturnMsg:
        p.setPen(::Qt::SolidLine);

        // no break !
    default: {
        QPoint p0 = poly.point(0);
        QPoint p1 = poly.point(1);
        QPoint p2 = poly.point(2);

        p.drawLine(p0, p1);
        p.drawLine(p0, p2);

        if (fp != 0)
            fprintf(fp, "\t<path fill=\"none\" stroke=\"black\" stroke-opacity=\"1\""
                    " d=\"M %d %d L %d %d L %d %d\" />\n"
                    "</g>\n",
                    p1.x(), p1.y(), p0.x(), p0.y(), p2.x(), p2.y());
    }
    }

    if (selected())
        show_mark(p, r);
}

void SdMsgCanvas::update()
{
    SdMsgBaseCanvas::update_after_move(start);
    SdMsgBaseCanvas::update_after_move(dest);
}

void SdMsgCanvas::check_vpos(const QRect &)
{
    // do nothing, concern self msg
}

void SdMsgCanvas::change_duration(SdDurationCanvas * oldone,
                                  SdDurationCanvas * newone)
{
    if (oldone == start)
        start = newone;
    else
        dest = newone;
}

int SdMsgCanvas::overlap_dir(SdDurationCanvas * d) const
{
    switch (itsType) {
    case UmlReturnMsg:
        return -1;

    case UmlSyncMsg:
        return (d != start) ? 1 : 0;

    default:
        // async
        return (d != start) ? 2 : 0;
    }
}

void SdMsgCanvas::menu(const QPoint &)
{
    Q3PopupMenu m(0);

    MenuFactory::createTitle(m, TR("Message"));
    m.insertSeparator();
    m.insertItem(TR("Upper"), 0);
    m.insertItem(TR("Lower"), 1);
    m.insertItem(TR("Go up"), 13);
    m.insertItem(TR("Go down"), 14);
    m.insertSeparator();
    m.insertItem(TR("Edit"), 2);
    m.insertItem(TR("Edit drawing settings"), 3);
    m.insertSeparator();

    if (msg != 0)
        m.insertItem(TR("Select operation in browser"), 8);

    m.insertItem(TR("Select linked items"), 4);

    if (label || stereotype) {
        m.insertSeparator();
        m.insertItem(TR("Select stereotype and label"), 5);
        m.insertItem(TR("Default stereotype and label position"), 6);
    }

    if (((BrowserSeqDiagram *) the_canvas()->browser_diagram())
        ->is_overlapping_bars()) {
        m.insertSeparator();

        if (start->isaDuration()) {
            m.insertItem(TR("Start from new overlapping bar"), 9);

            if (start->isOverlappingDuration())
                m.insertItem(TR("Start from parent bar"), 10);
        }

        if (dest->isaDuration()) {
            m.insertItem(TR("Go to new overlapping bar"), 11);

            if (dest->isOverlappingDuration())
                m.insertItem(TR("Go to parent bar"), 12);
        }
    }

    m.insertSeparator();
    m.insertItem(TR("Remove from diagram"), 7);

    switch (m.exec(QCursor::pos())) {
    case 0:
        upper();
        // force son reaffichage
        hide();
        show();
        break;

    case 1:
        lower();
        // force son reaffichage
        hide();
        show();
        break;

    case 13:
        z_up();
        // force son reaffichage
        hide();
        show();
        break;

    case 14:
        z_down();
        // force son reaffichage
        hide();
        show();
        break;

    case 2:
        open();
        break;

    case 3:
        edit_drawing_settings();
        return;

    case 4:
        select_associated();
        return;

    case 5:
        the_canvas()->unselect_all();

        if (label)
            the_canvas()->select(label);

        if (stereotype)
            the_canvas()->select(stereotype);

        return;

    case 6:
        if (label)
            default_label_position();

        if (stereotype)
            default_stereotype_position();

        break;

    case 7:
        delete_it();
        return;

    case 8:
        msg->get_browser_node()->select_in_browser();
        return;

    case 9:
        ((SdDurationCanvas *) start)->go_up(this, FALSE);
        break;

    case 10:
        ((SdDurationCanvas *) start)->go_down(this);
        break;

    case 11:
        ((SdDurationCanvas *) dest)->go_up(this, TRUE);
        break;

    case 12:
        ((SdDurationCanvas *) dest)->go_down(this);
        break;

    default:
        return;
    }

    package_modified();
    canvas()->update();
}

void SdMsgCanvas::apply_shortcut(QString s)
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
    else if (s == "Edit")
        open();
    else
        return;

    // force son reaffichage
    hide();
    show();
    package_modified();
    canvas()->update();
}

void SdMsgCanvas::edit_drawing_settings()
{
    for (;;) {
        StateSpecVector st(3);

        st[0].set(TR("operation drawing language"), &drawing_language);
        st[1].set(TR("show full operation definition"), &show_full_oper);
        st[2].set(TR("show context mode"), &show_context_mode);

        SettingsDialog dialog(&st, 0, FALSE);

        dialog.raise();

        if (dialog.exec() == QDialog::Accepted)
            modified();

        if (!dialog.redo())
            break;
    }
}

bool SdMsgCanvas::has_drawing_settings() const
{
    return TRUE;
}

void SdMsgCanvas::edit_drawing_settings(QList<DiagramItem *> & l)
{
    for (;;) {
        StateSpecVector st(3);
        DrawingLanguage drawing_language;
        Uml3States show_full_oper;
        ShowContextMode show_context_mode;

        st[0].set(TR("operation drawing language"), &drawing_language);
        st[1].set(TR("show full operation definition"), &show_full_oper);
        st[2].set(TR("show context mode"), &show_context_mode);

        SettingsDialog dialog(&st, 0, FALSE, TRUE);

        dialog.raise();

        if (dialog.exec() == QDialog::Accepted) {
            foreach (DiagramItem *item, l) {
                SdMsgCanvas *canvas = (SdMsgCanvas *)item;
                if (!st[0].name.isEmpty())
                    canvas->drawing_language =
                        drawing_language;

                if (!st[1].name.isEmpty())
                    canvas->show_full_oper =
                        show_full_oper;

                if (!st[2].name.isEmpty())
                    canvas->show_context_mode =
                        show_context_mode;

                canvas->modified();
            }
        }

        if (!dialog.redo())
            break;
    }
}

void SdMsgCanvas::clone_drawing_settings(const DiagramItem *src)
{
    const SdMsgCanvas * x = (const SdMsgCanvas *) src;
    drawing_language = x->drawing_language;
    show_full_oper = x->show_full_oper;
    show_context_mode = x->show_context_mode;
    modified();
}

void SdMsgCanvas::select_associated()
{
    the_canvas()->select(this);

    if (!start->selected())
        start->select_associated();

    if (!dest->selected())
        dest->select_associated();
}

bool SdMsgCanvas::copyable() const
{
    // must not call start->copyable() else infinite loop
    return start->selected() && SdMsgBaseCanvas::copyable();
}

void SdMsgCanvas::save(QTextStream & st, bool ref, QString & warning) const
{
    if (ref) {
        st << "msg_ref " << get_ident()
           << " // " << get_msg(FALSE);
    }
    else {
        nl_indent(st);
        st << "msg " << get_ident();

        switch (itsType) {
        case UmlSyncMsg:
            st << " synchronous";
            break;

        case UmlAsyncMsg:
            st << " asynchronous";
            break;

        case UmlFoundSyncMsg:
            st << " found_synchronous";
            break;

        case UmlFoundAsyncMsg:
            st << " found_asynchronous";
            break;

        case UmlLostSyncMsg:
            st << " lost_synchronous";
            break;

        case UmlLostAsyncMsg:
            st << " lost_asynchronous";
            break;

        default:
            st << " return";
        }

        indent(+1);
        nl_indent(st);
        st << "from ";
        start->save(st, TRUE, warning);
        SdMsgBaseCanvas::save(st, warning);
        indent(-1);
    }
}

SdMsgCanvas * SdMsgCanvas::read(char *& st, UmlCanvas * canvas, char * k)
{
    if (!strcmp(k, "msg_ref"))
        return ((SdMsgCanvas *) dict_get(read_id(st), "msg", canvas));
    else if (!strcmp(k, "msg")) {
        int id = read_id(st);
        UmlCode c;

        k = read_keyword(st);

        if (!strcmp(k, "synchronous"))
            c = UmlSyncMsg;
        else if (!strcmp(k, "asynchronous"))
            c = UmlAsyncMsg;
        else if (!strcmp(k, "found_synchronous"))
            c = UmlFoundSyncMsg;
        else if (!strcmp(k, "found_asynchronous"))
            c = UmlFoundAsyncMsg;
        else if (!strcmp(k, "lost_synchronous"))
            c = UmlLostSyncMsg;
        else if (!strcmp(k, "lost_asynchronous"))
            c = UmlLostAsyncMsg;
        else if (!strcmp(k, "return"))
            c = UmlReturnMsg;
        else {
            wrong_keyword(k, "synchronous/asynchronous/return");
            return 0; 	// to avoid warning
        }

        SdMsgSupport * start;
        SdMsgSupport * dest;

        read_keyword(st, "from");

        switch (c) {
        case UmlFoundSyncMsg:
        case UmlFoundAsyncMsg:
            start = SdLostFoundMsgSupportCanvas::read(st, canvas, read_keyword(st));
            break;

        default:
            start = SdDurationCanvas::read(st, canvas, TRUE);
        }

        read_keyword(st, "to");

        switch (c) {
        case UmlLostSyncMsg:
        case UmlLostAsyncMsg:
            dest = SdLostFoundMsgSupportCanvas::read(st, canvas, read_keyword(st));
            break;

        default:
            dest = SdDurationCanvas::read(st, canvas, TRUE);
        }

        k = read_keyword(st);

        SdMsgCanvas * result =
            new SdMsgCanvas(canvas, start, dest, c, (int) read_double(st), id);

        if (!strcmp(k, "yz"))
            // new version
            result->setZ(read_double(st));
        else if (strcmp(k, "y"))
            wrong_keyword(k, "y/yz");

        result->SdMsgBaseCanvas::read(st);

        return result;
    }
    else
        return 0;
}
