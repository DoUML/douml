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
//Added by qt3to4:
#include <QTextStream>

#include "CodDirsCanvas.h"
#include "BrowserDiagram.h"
#include "LabelCanvas.h"
#include "ColMsg.h"
#include "Settings.h"
#include "OperationData.h"
#include "CodLinkCanvas.h"
#include "CodObjCanvas.h"
#include "SettingsDialog.h"
#include "ColDiagramView.h"
#include "myio.h"
#include "translate.h"

CodDirsCanvas::CodDirsCanvas(UmlCanvas * canvas, CodLinkCanvas * l, int id)
    : DiagramCanvas(0, canvas, 0, 0, COL_DIRS_SIZE, COL_DIRS_SIZE, id),
      angle(0), backward_label(0), link(l)
{
    setZ(COL_MSG_Z);
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
}

CodDirsCanvas::~CodDirsCanvas()
{
}

void CodDirsCanvas::delete_it()
{
    disconnect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));

    CodMsgSupport::delete_it(((ColDiagramView *) the_canvas()->get_view())->get_msgs());
    DiagramCanvas::delete_it();

    if (backward_label)
        backward_label->delete_it();
}

void CodDirsCanvas::remove_it(ColMsg * msg)
{
    const BasicData * oper_data = msg->get_operation();

    if (unsubscribe(oper_data))
        disconnect(oper_data, 0, this, 0);

    msgs.remove(msg);
}


void CodDirsCanvas::get_from_to(CodObjCanvas *& from,
                                CodObjCanvas *& to,
                                bool forward)
{
    link->get_start_end(from, to);

    if (! forward) {
        CodObjCanvas * x = from;

        from = to;
        to = x;
    }
}

void CodDirsCanvas::update_pos(const QPoint & link_start,
                               const QPoint & link_end)
{
    hide();
    angle = compute_angle(link_end.x() - link_start.x(),
                          link_end.y() - link_start.y());

    QRect r = rect();

    r.moveCenter((link_start + link_end) / 2);

    move(r.x(), r.y());

    if (label != 0)
        update_label_pos(label, TRUE);

    if (backward_label != 0)
        update_label_pos(backward_label, FALSE);

    show();
    canvas()->update();
}

void CodDirsCanvas::update_label_pos(LabelCanvas * lbl, bool forward)
{
    if (!lbl->selected()) {
        double a = angle + ((forward) ? 270 : 90);
        QRect r = rect();
        QPoint c = r.center();

        r.setWidth(r.width() * 2 / 3);
        r.setHeight(r.height() * 2 / 3);
        r.moveCenter(c);

        lbl->move_outside(r, (a  > 360) ? a - 360 : a);
    }
}

void CodDirsCanvas::update_msgs()
{
    // does not unsubscribe & disconnect signals because update_msgs may
    // be called during a signal management, and the signal connection list
    // cannot be modified in this case
    msgs.sort();

    QString nl = "\n";
    QString null;
    const QString * forward_pfix = &null;
    const QString * backward_pfix = &null;
    QString forward;
    QString backward;
    CollaborationDiagramSettings  dflt = settings;

    the_canvas()->browser_diagram()->get_collaborationdiagramsettings(dflt);

    foreach (ColMsg *msg, msgs) {
        const BasicData * oper_data = msg->get_operation();

        if ((oper_data != 0) && subscribe(oper_data)) {
            connect(oper_data, SIGNAL(changed()), this, SLOT(modified()));
            connect(oper_data, SIGNAL(deleted()), this, SLOT(modified()));
        }

        if (msg->get_is_forward()) {
            QString s = msg->def(dflt.show_hierarchical_rank == UmlYes,
                                 dflt.show_full_operations_definition == UmlYes,
                                 dflt.drawing_language,
                                 dflt.show_msg_context_mode);

            if (!s.isEmpty()) {
                forward += *forward_pfix + s;
                forward_pfix = &nl;
            }
        }
        else {
            QString s = msg->def(dflt.show_hierarchical_rank == UmlYes,
                                 dflt.show_full_operations_definition == UmlYes,
                                 dflt.drawing_language,
                                 dflt.show_msg_context_mode);

            if (!s.isEmpty()) {
                backward += *backward_pfix + s;
                backward_pfix = &nl;
            }
        }
    }

    if (forward.isEmpty()) {
        if (label != 0) {
            label->delete_it();
            label = 0;
            link->package_modified();
        }
    }
    else {
        if (label == 0) {
            label = new LabelCanvas(forward, the_canvas(), 0, 0);
            label->show();
            update_label_pos(label, TRUE);
            link->package_modified();
        }
        else if (label->get_name() != forward) {
            label->set_name(forward);
            update_label_pos(label, TRUE);
            link->package_modified();
        }
    }

    if (backward.isEmpty()) {
        if (backward_label != 0) {
            backward_label->delete_it();
            backward_label = 0;
            link->package_modified();
        }
    }
    else {
        if (backward_label == 0) {
            backward_label = new LabelCanvas(backward, the_canvas(), 0, 0);
            update_label_pos(backward_label, FALSE);
            backward_label->show();
            link->package_modified();
        }
        else if (backward_label->get_name() != backward) {
            backward_label->set_name(backward);
            update_label_pos(backward_label, FALSE);
            link->package_modified();
        }
    }

    hide();
    show();
    canvas()->update();
}

void CodDirsCanvas::modified()
{
    if (visible()) {
        update_msgs();
        package_modified();
    }
}

void CodDirsCanvas::setVisible(bool yes)
{
    DiagramCanvas::setVisible(yes);

    if (backward_label != 0)
        backward_label->setVisible(yes);
}

void CodDirsCanvas::draw(QPainter & p)
{
    if ((! visible()) || ((label == 0) && (backward_label == 0)))
        return;

    //p.drawRect(rect()); // to check that the arrows doesnt goes outside the rect

    p.save();
    p.translate(center().x(), center().y());
    p.rotate(angle);

    double zoom = the_canvas()->zoom();
    const double seven = 7 * zoom;
    const double five = 5 * zoom;
    const double half_width = width() / 2.0;
    FILE * fp = svg();

    QPoint p1((int)(-half_width + seven), (int) - seven);
    QPoint p2((int)(half_width - seven), p1.y());

    if (label != 0) {
        p.drawLine(p1, p2);
        //p.lineTo((int) (p2.x() - five), (int) (p2.y() - five));
        p.drawLine(p2.x(), p2.y(), (int)(p2.x() - five), (int)(p2.y() - five));
        p.drawLine(p2.x(), p2.y(), (int)(p2.x() - five), (int)(p2.y() + five));

        if (fp != 0) {
            fprintf(fp, "<g transform=\"translate(%d %d) rotate(%d)\">\n"
                    "\t<path stroke=\"black\" fill=\"none\" d=\"M %d %d L %d %d L %d %d M %d %d L %d %d\" />\n",
                    center().x(), center().y(), (int) angle,
                    p1.x(), p1.y(), p2.x(), p2.y(),
                    (int)(p2.x() - five), (int)(p2.y() - five),
                    p2.x(), p2.y(), (int)(p2.x() - five), (int)(p2.y() + five));

            if (backward_label == 0)
                fputs("</g>\n", fp);
        }
    }

    if (backward_label != 0) {
        p1.setY((int) seven);
        p2.setY((int) seven);
        p.drawLine(p2, p1);
        //p.lineTo((int) (p1.x() + five), (int) (p1.y() - five));
        p.drawLine(p1.x(), p1.y(), (int)(p1.x() + five), (int)(p1.y() - five));
        p.drawLine(p1.x(), p1.y(), (int)(p1.x() + five), (int)(p1.y() + five));

        if (fp != 0) {
            if (label == 0)
                fprintf(fp, "<g transform=\"translate(%d %d) rotate(%d)\">\n",
                        center().x(), center().y(), (int) angle);

            fprintf(fp, "\t<path stroke=\"black\" fill=\"none\" d=\"M %d %d L %d %d L %d %d M %d %d L %d %d\" />\n"
                    "</g>\n",
                    p2.x(), p2.y(), p1.x(), p1.y(),
                    (int)(p1.x() + five), (int)(p1.y() - five),
                    p1.x(), p1.y(), (int)(p1.x() + five), (int)(p1.y() + five));
        }
    }

    p.restore();
}

bool CodDirsCanvas::edit_drawing_settings()
{
    for (;;) {
        StateSpecVector st;

        settings.complete_msg(st);

        SettingsDialog dialog(&st, 0, FALSE);

        dialog.raise();

        if (dialog.exec() != QDialog::Accepted)
            return FALSE;

        update_msgs();

        if (!dialog.redo())
            return TRUE;
    }
}

UmlCode CodDirsCanvas::type() const
{
    return UmlLinkDirs;
}

int CodDirsCanvas::rtti() const
{
    return RTTI_COL_MSG;
}

void CodDirsCanvas::open()
{
    // cannot be called
}

void CodDirsCanvas::menu(const QPoint &)
{
    // cannot be called
}

QString CodDirsCanvas::may_start(UmlCode &) const
{
    // cannot be called
    return TR("illegal");
}

QString CodDirsCanvas::may_connect(UmlCode &, const DiagramItem *) const
{
    // cannot be called
    return TR("illegal");
}

bool CodDirsCanvas::copyable() const
{
    CodObjCanvas * from;
    CodObjCanvas * to;

    link->get_start_end(from, to);

    return link->selected() && from->copyable() && to->copyable();
}

bool CodDirsCanvas::represents(BrowserNode * bn)
{
    return supports(bn);
}

void CodDirsCanvas::save(QTextStream & st, bool ref, QString & warning) const
{
    if (ref)
        st << "dirscanvas_ref " << get_ident();
    else {
        nl_indent(st);
        st << "dirscanvas " << get_ident() << " z " << z() << " ";
        link->save(st, TRUE, warning);

        indent(+1);

        settings.save_msg(st);

        if (label != 0) {
            nl_indent(st);
            st << "forward_label ";
            save_string(label->get_name(), st);
            save_xyz(st, label, " xyz");
        }

        if (backward_label != 0) {
            nl_indent(st);
            st << "backward_label ";
            save_string(backward_label->get_name(), st);
            save_xyz(st, backward_label, " xyz");
        }

        indent(-1);
    }
}

CodDirsCanvas * CodDirsCanvas::read(char *& st, UmlCanvas * canvas, char *& k)
{
    if (!strcmp(k, "dirscanvas_ref"))
        return ((CodDirsCanvas *) dict_get(read_id(st), "dirs canvas", canvas));
    else if (!strcmp(k, "dirscanvas")) {
        int id = read_id(st);
        double z;

        if (read_file_format() < 5)
            z = OLD_COL_MSG_Z;
        else {
            read_keyword(st, "z");
            z = read_double(st);
        }

        k = read_keyword(st);

        CodDirsCanvas * result =
            new CodDirsCanvas(canvas, CodLinkCanvas::read(st, canvas, k), id);

        result->setZ(z);
        result->link->set_dirs(result);
        result->link->update_pos();		// to place result

        k = read_keyword(st);
        result->settings.read(st, k);		// updates k

        if (!strcmp(k, "forward_label")) {
            result->label = new LabelCanvas(read_string(st), canvas, 0, 0);

            if (read_file_format() < 5) {
                read_keyword(st, "xy");
                read_xy(st, result->label);
            }
            else {
                read_keyword(st, "xyz");
                read_xyz(st, result->label);
            }

            result->label->set_center100();
            k = read_keyword(st);
        }

        if (!strcmp(k, "backward_label")) {
            result->backward_label = new LabelCanvas(read_string(st), canvas, 0, 0);

            if (read_file_format() < 5) {
                read_keyword(st, "xy");
                read_xy(st, result->backward_label);
            }
            else {
                read_keyword(st, "xyz");
                read_xyz(st, result->backward_label);
            }

            result->backward_label->set_center100();
            k = read_keyword(st);
        }

        return result;
    }
    else
        return 0;
}

void CodDirsCanvas::history_save(QBuffer & b) const
{
    DiagramCanvas::history_save(b);
    ::save(angle, b);
}

void CodDirsCanvas::history_load(QBuffer & b)
{
    DiagramCanvas::history_load(b);
    ::load(angle, b);
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));

    foreach (ColMsg *msg, msgs) {
        const BasicData * oper_data = msg->get_operation();

        if ((oper_data != 0) && subscribe(oper_data)) {
            connect(oper_data, SIGNAL(changed()), this, SLOT(modified()));
            connect(oper_data, SIGNAL(deleted()), this, SLOT(modified()));
        }
    }
}

void CodDirsCanvas::history_hide()
{
    Q3CanvasItem::setVisible(FALSE);
    disconnect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));

    foreach (ColMsg *msg, msgs) {
        const BasicData * oper_data = msg->get_operation();

        if ((oper_data != 0) && unsubscribe(oper_data)) {
            disconnect(oper_data, SIGNAL(changed()), this, SLOT(modified()));
            disconnect(oper_data, SIGNAL(deleted()), this, SLOT(modified()));
        }
    }
}
