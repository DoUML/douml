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
#include <Q3PointArray>

#include "SdSelfMsgCanvas.h"
#include "SdDurationCanvas.h"
#include "OperationData.h"
#include "UmlCanvas.h"
#include "LabelCanvas.h"
#include "Settings.h"
#include "SettingsDialog.h"
#include "BrowserSeqDiagram.h"
#include "myio.h"
#include "ui/menufactory.h"
#include "translate.h"

#define SELF_MSG_WIDTH  20
#define SELF_MSG_HEIGHT 14

SdSelfMsgCanvas::SdSelfMsgCanvas(UmlCanvas * canvas, SdMsgSupport * d,
                                 UmlCode l, int v, int id)
    : SdMsgBaseCanvas(canvas, d, l, v + 1, id)
{
    dest->add(this);
    update_hpos();
    setSize(SELF_MSG_WIDTH, SELF_MSG_HEIGHT);
    check_vpos(d->rect());
    set_center100();
    show();
}

SdSelfMsgCanvas::~SdSelfMsgCanvas()
{
}

void SdSelfMsgCanvas::change_scale()
{
    if (!((BrowserSeqDiagram *) the_canvas()->browser_diagram())
        ->is_overlapping_bars()) {
        // else done by check_vpos called by SdDurationCanvas::change_scale()
        SdMsgBaseCanvas::change_scale();
    }
}

void SdSelfMsgCanvas::update_hpos()
{
    LabelCanvas * lbl = label;
    int cy = center_y_scale100;
    double dx = dest->rect().right() + 1 - x();

    if (the_canvas()->do_zoom())
        // the label and stereotype are moved independently
        label = 0;
    else if ((stereotype != 0) && !stereotype->selected())
        stereotype->moveBy(dx, 0);

    DiagramCanvas::moveBy(dx, 0);

    label = lbl;
    center_y_scale100 = cy;	// updated later
}

void SdSelfMsgCanvas::check_vpos(const QRect & r)
{
    if (((BrowserSeqDiagram *) the_canvas()->browser_diagram())
        ->is_overlapping_bars()) {
        double v;

        switch (itsType) {
        case UmlSelfReturnMsg:
            v = r.bottom() - height() / 2;
            break;

        case UmlSyncSelfMsg:
            v = r.top();
            break;

        default:
            return;
        }

        if (y() != v) {
            double dy = v - y();

            DiagramCanvas::moveBy(0, dy);

            if ((stereotype != 0) && !stereotype->selected())
                stereotype->moveBy(0, dy);
        }
    }
}

double SdSelfMsgCanvas::min_y() const
{
    return dest->min_y();
}

void SdSelfMsgCanvas::draw(QPainter & p)
{
    const QRect r = rect();
    int ah = (r.height() - 1 - 1 - 2 - 1 - 1) / 2;
    int he = r.top() + 1 + 2 + ah + 1;
    FILE * fp = svg();
    p.setRenderHint(QPainter::Antialiasing, true);

    if (itsType == UmlSelfReturnMsg)
        p.setPen(::Qt::DotLine);

    p.drawLine(r.left() + 1, r.top() + 1, r.right() - 1, r.top() + 1);



    //p.lineTo(r.right() - 1, he);
    p.drawLine(r.right() - 1, r.top() + 1, r.right() - 1, he);
    //p.lineTo(r.left() + 1, he);
    p.drawLine(r.right() - 1, he, r.left() + 1, he);

    if (fp != 0) {
        fputs("<g>\n\t<path fill=\"none\" stroke=\"black\" stroke-opacity=\"1\"", fp);

        if (itsType == UmlSelfReturnMsg)
            fputs(" stroke-dasharray=\"4,4\"", fp);

        fprintf(fp, " d=\"M %d %d L %d %d L %d %d L %d %d\" />\n",
                r.left() + 1, r.top() + 1, r.right() - 1, r.top() + 1,
                r.right() - 1, he,
                r.left() + 1, he);
    }

    if (itsType == UmlSyncSelfMsg) {
        Q3PointArray poly(3);
        QBrush brsh = p.brush();

        p.setBrush(Qt::black);
        poly.setPoint(0, r.left() + 1, he);
        poly.setPoint(1, r.left() + 1 + ah, he + ah);
        poly.setPoint(2, r.left() + 1 + ah, he - ah);
        p.drawPolygon(poly/*, TRUE*/);
        p.setBrush(brsh);

        if (fp != 0) {
            draw_poly(fp, poly, UmlBlack, FALSE);
            fputs("</g>\n", fp);
        }
    }
    else {
        if (itsType == UmlSelfReturnMsg)
            p.setPen(::Qt::SolidLine);

        //p.lineTo(r.left() + 1 + ah, he + ah);
        p.drawLine(r.left() + 1 + ah, he - ah, r.left() + 1 + ah, he + ah); // assuming last call poly.setPoint
        p.drawLine(r.left() + 1, he, r.left() + 1 + ah, he - ah);

        if (fp != 0)
            fprintf(fp, "\t<path fill=\"none\" stroke=\"black\" stroke-opacity=\"1\""
                    " d=\"M %d %d L %d %d L %d %d\" />\n"
                    "</g>\n",
                    r.left() + 1 + ah, he + ah,
                    r.left() + 1, he,
                    r.left() + 1 + ah, he - ah);
    }

    if (selected())
        show_mark(p, r);
}

void SdSelfMsgCanvas::update()
{
    if (((BrowserSeqDiagram *) the_canvas()->browser_diagram())
        ->is_overlapping_bars()) {
        switch (itsType) {
        case UmlSelfReturnMsg:
        case UmlSyncSelfMsg:
            check_vpos(dest->rect());
            return;

        default:
            break;
        }
    }

    SdMsgBaseCanvas::update_after_move(dest);
}

void SdSelfMsgCanvas::change_duration(SdDurationCanvas *,
                                      SdDurationCanvas * newone)
{
    dest = newone;
}

int SdSelfMsgCanvas::overlap_dir(SdDurationCanvas *) const
{
    switch (itsType) {
    case UmlSelfReturnMsg:
        return -1;

    case UmlSyncSelfMsg:
        return 1;

    default:
        // async
        return 2;
    }
}

void SdSelfMsgCanvas::menu(const QPoint &)
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
        m.insertItem(TR("Go to new overlapping bar"), 9);

        if (dest->isOverlappingDuration())
            m.insertItem(TR("Go to parent bar"), 10);
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
        break;

    case 5:
        the_canvas()->unselect_all();

        if (label)
            the_canvas()->select(label);

        if (stereotype)
            the_canvas()->select(stereotype);

        break;

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
        ((SdDurationCanvas *) dest)->go_up(this, TRUE);
        break;

    case 10:
        ((SdDurationCanvas *) dest)->go_down(this);

    default:
        return;
    }

    package_modified();
    canvas()->update();
}

void SdSelfMsgCanvas::apply_shortcut(QString s)
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

void SdSelfMsgCanvas::edit_drawing_settings()
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

bool SdSelfMsgCanvas::has_drawing_settings() const
{
    return TRUE;
}

void SdSelfMsgCanvas::edit_drawing_settings(QList<DiagramItem *> & l)
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
                SdSelfMsgCanvas *canvas = (SdSelfMsgCanvas *)item;
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

void SdSelfMsgCanvas::clone_drawing_settings(const DiagramItem *src)
{
    const SdSelfMsgCanvas * x = (const SdSelfMsgCanvas *) src;
    drawing_language = x->drawing_language;
    show_full_oper = x->show_full_oper;
    show_context_mode = x->show_context_mode;
    modified();
}

void SdSelfMsgCanvas::select_associated()
{
    the_canvas()->select(this);

    if (!dest->selected())
        dest->select_associated();
}

void SdSelfMsgCanvas::save(QTextStream & st, bool ref, QString & warning) const
{
    if (ref) {
        st << ((itsType == UmlSelfReturnMsg)
               ? "selfreflexivemsg_ref "
               : "reflexivemsg_ref ")
           << get_ident() << " // " << get_msg(FALSE);
    }
    else {
        nl_indent(st);

        switch (itsType) {
        case UmlSelfReturnMsg:
            st << "selfreflexivemsg " << get_ident();
            break;

        case UmlSyncSelfMsg:
            st << "reflexivemsg " << get_ident() << " synchronous";
            break;

        default:
            st << "reflexivemsg " << get_ident() << " asynchronous";
        }

        indent(+1);
        SdMsgBaseCanvas::save(st, warning);
        indent(-1);
    }
}

SdSelfMsgCanvas * SdSelfMsgCanvas::read(char *& st, UmlCanvas * canvas, char * k)
{
    if (!strcmp(k, "reflexivemsg_ref"))
        return ((SdSelfMsgCanvas *) dict_get(read_id(st), "reflexivemsg", canvas));

    if (!strcmp(k, "selfreflexivemsg_ref"))
        return ((SdSelfMsgCanvas *) dict_get(read_id(st), "selfreflexivemsg", canvas));
    else if (!strcmp(k, "reflexivemsg") || !strcmp(k, "selfreflexivemsg")) {
        int id = read_id(st);
        UmlCode c;

        if (*k == 's')
            c = UmlSelfReturnMsg;
        else {
            k = read_keyword(st);

            if (!strcmp(k, "synchronous"))
                c = UmlSyncSelfMsg;
            else if (!strcmp(k, "asynchronous"))
                c = UmlAsyncSelfMsg;
            else {
                wrong_keyword(k, "synchronous/asynchronous");
                return 0; 	// to avoid warning
            }
        }

        read_keyword(st, "to");

        SdDurationCanvas * d = SdDurationCanvas::read(st, canvas, TRUE);

        k = read_keyword(st);

        SdSelfMsgCanvas * result =
            new SdSelfMsgCanvas(canvas, d, c, (int) read_double(st) - 1, id);

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
