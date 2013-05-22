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

#include "CodSelfLinkCanvas.h"
#include "BrowserDiagram.h"
#include "Settings.h"
#include "CodObjCanvas.h"
#include "ColMsg.h"
#include "UmlCanvas.h"
#include "LabelCanvas.h"
#include "ColDiagramView.h"
#include "CodAddMsgDialog.h"
#include "CodEditMsgDialog.h"
#include "SettingsDialog.h"
#include "OperationData.h"
#include "myio.h"
#include "ui/menufactory.h"
#include "translate.h"

CodSelfLinkCanvas::CodSelfLinkCanvas(UmlCanvas * canvas, CodObjCanvas * o,
                                     const QPoint & p, int id)
    : DiagramCanvas(0, canvas, 0, 0, SELF_LINK_RADIUS * 2,
                    SELF_LINK_RADIUS * 2, id),
    obj(o)
{
    setZ(o->z() - 0.5);
    browser_node = canvas->browser_diagram();
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
    compute_pos(p);

    if (canvas->paste())
        // must not be selected else the move done after if wrong
        canvas->unselect(this);
}

CodSelfLinkCanvas::~CodSelfLinkCanvas()
{
}

void CodSelfLinkCanvas::delete_it()
{
    disconnect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
    CodMsgSupport::delete_it(((ColDiagramView *) the_canvas()->get_view())->get_msgs());
    DiagramCanvas::delete_it();
}

void CodSelfLinkCanvas::remove_it(ColMsg * msg)
{
    const BasicData * oper_data = msg->get_operation();

    if (unsubscribe(oper_data))
        disconnect(oper_data, 0, this, 0);

    msgs.remove(msg);
}

void CodSelfLinkCanvas::delete_available(BooL &, BooL & out_model) const
{
    out_model |= TRUE;
}

void CodSelfLinkCanvas::get_from_to(CodObjCanvas *& from,
                                    CodObjCanvas *& to,
                                    bool)
{
    from = to = obj;
}

void CodSelfLinkCanvas::modified()
{
    hide();
    update_pos();
    show();
    canvas()->update();
    update_msgs();
    package_modified();
}

void CodSelfLinkCanvas::update_pos()
{
    QPoint oc = obj->center();
    QPoint c(oc.x() + delta_x, oc.y() + delta_y);

    compute_pos(c);
}

void CodSelfLinkCanvas::compute_pos(QPoint p)
{
    QPoint obj_center = obj->center();

    delta_x = p.x() - obj_center.x();
    delta_y = p.y() - obj_center.y();

    // goes outside object;
    while (obj->contains(p.x(), p.y())) {
        p.setX(p.x() + delta_x);
        p.setY(p.y() + delta_y);
    }

    // move on the border of object
    QPoint center;
    QRect r = rect();

    obj->shift(center, p, FALSE);
    r.moveCenter(center);
    moveBy(r.x() - x() + 100000, r.y() - y());

    angle = compute_angle(delta_x, delta_y);

    // messages
    if (label != 0)
        update_label_pos();
}

void CodSelfLinkCanvas::update_label_pos()
{
    if (!label->selected())
        label->move_outside(rect(), angle);
}

void CodSelfLinkCanvas::update_msgs()
{
    // does not unsubscribe & disconnect signals because compute_size may
    // be called during a signal management, and the signal connection list
    // cannot be modified in this case
    QString s;

    if (! msgs.isEmpty()) {
        msgs.sort();

        CollaborationDiagramSettings dflt = settings;

        the_canvas()->browser_diagram()->get_collaborationdiagramsettings(dflt);

        QString nl = "\n";
        QString null;
        const QString * pfix = &null;

        foreach (ColMsg *msg, msgs) {
            const BasicData *oper_data = msg->get_operation();

            if ((oper_data != 0) && subscribe(oper_data)) {
                connect(oper_data, SIGNAL(changed()), this, SLOT(modified()));
                connect(oper_data, SIGNAL(deleted()), this, SLOT(modified()));
            }

            QString m = msg->def(dflt.show_hierarchical_rank == UmlYes,
                                 dflt.show_full_operations_definition == UmlYes,
                                 dflt.drawing_language,
                                 dflt.show_msg_context_mode);

            if (!m.isEmpty()) {
                s += *pfix + m;
                pfix = &nl;
            }
        }
    }

    if (s.isEmpty()) {
        if (label != 0) {
            label->delete_it();
            label = 0;
            package_modified();
        }
    }
    else {
        if (label == 0)
            (label = new LabelCanvas(s, the_canvas(), 0, 0))->show();
        else if (label->get_name() == s)
            return;
        else
            label->set_name(s);

        update_label_pos();
        canvas()->update();
        package_modified();
    }
}

void CodSelfLinkCanvas::moveBy(double dx, double dy)
{
    if (dx > 80000)
        // compute_pos
        DiagramCanvas::moveBy(dx - 100000, dy);
    else if (!selected() || !obj->selected()) {
        // manual move & not moved by selection
        QPoint p((int)(center().x() + dx), (int)(center().y() + dy));

        compute_pos(p);
    }
}

void CodSelfLinkCanvas::draw(QPainter & p)
{
    if (! visible()) return;

    p.setRenderHint(QPainter::Antialiasing, true);
    QPoint ce = center();

    p.save();
    p.translate(ce.x(), ce.y());
    p.rotate(angle);

    QRect r(-width() / 2, -width() / 4, width() - 1, width() / 2);

    p.drawEllipse(r);

    FILE * fp = svg();

    if (fp != 0)
        fprintf(fp, "<ellipse fill=\"none\" stroke=\"black\" stroke-opacity=\"1\" transform=\"translate(%d %d) rotate(%d)\" rx=\"%d\" ry=\"%d\" />\n",
                (int) ce.x(), (int) ce.y(), (int) angle, width() / 2, width() / 4);

    if (selected())
        show_mark(p, r);

    p.restore();
}

UmlCode CodSelfLinkCanvas::type() const
{
    return UmlSelfLink;
}

void CodSelfLinkCanvas::open()
{
    QPoint dummy;

    menu(dummy);
}

void CodSelfLinkCanvas::menu(const QPoint &)
{
    Q3PopupMenu m;

    MenuFactory::createTitle(m, TR("Self link"));
    m.insertSeparator();
    m.insertItem(TR("Add messages"), 1);
    m.insertSeparator();
    m.insertItem(TR("Edit its messages"), 2);
    m.insertItem(TR("Edit all the messages"), 3);
    m.insertSeparator();
    m.insertItem(TR("Edit drawing settings"), 4);
    m.insertSeparator();
    m.insertItem(TR("Remove from diagram"), 5);

    switch (m.exec(QCursor::pos())) {
    case 1: {
        CodAddMsgDialog d(obj, obj, this, (ColDiagramView *) the_canvas()->get_view(),
                          TRUE);

        d.exec();
    }

    return;

    case 2:
        CodEditMsgDialog::exec((ColDiagramView *) the_canvas()->get_view(), msgs);
        return;

    case 3:
        CodEditMsgDialog::exec((ColDiagramView *) the_canvas()->get_view(),
                               ((ColDiagramView *) the_canvas()->get_view())->get_msgs());
        return;

    case 4:
        for (;;) {
            StateSpecVector st;

            settings.complete_msg(st);

            SettingsDialog dialog(&st, 0, FALSE);

            if (dialog.exec() == QDialog::Accepted)
                modified();

            if (!dialog.redo())
                return;
        }

        break;

    case 5:
        delete_it();
        ((ColDiagramView *) the_canvas()->get_view())->update_msgs();
        break;

    default:
        return;
    }

    package_modified();
}

QString CodSelfLinkCanvas::may_start(UmlCode & l) const
{
    return (l == UmlAnchor) ? QString() : TR("illegal");
}

QString CodSelfLinkCanvas::may_connect(UmlCode & l, const DiagramItem * dest) const
{
    return (l == UmlAnchor) ? dest->may_start(l) : TR("illegal");
}

bool CodSelfLinkCanvas::copyable() const
{
    return selected() && obj->copyable();
}

bool CodSelfLinkCanvas::represents(BrowserNode * bn)
{
    return supports(bn);
}

void CodSelfLinkCanvas::save(QTextStream & st, bool ref, QString & warning) const
{
    if (ref)
        st << "selflinkcanvas_ref " << get_ident();
    else {
        nl_indent(st);
        st << "selflinkcanvas " << get_ident() << " ";
        obj->save(st, TRUE, warning);

        indent(+1);
        nl_indent(st);

        QPoint obj_center = obj->center();

#ifdef FORCE_INT_COORD
        // note : << float bugged in Qt 3.3.3
        st << "xy " << (int)(delta_x + obj_center.x())
           << " " << (int)(delta_y + obj_center.y());
#else
        QString sx, sy;

        st << "xy " << sx.setNum(delta_x + obj_center.x())
           << " " << sy.setNum(delta_y + obj_center.y());
#endif

        settings.save_msg(st);

        if (label != 0) {
            nl_indent(st);
            st << "forward_label ";
            save_string(label->get_name(), st);
            save_xyz(st, label, " xyz");
        }

        indent(-1);
    }
}

CodSelfLinkCanvas * CodSelfLinkCanvas::read(char *& st, UmlCanvas * canvas,
        char *& k)
{
    if (!strcmp(k, "selflinkcanvas_ref"))
        return ((CodSelfLinkCanvas *) dict_get(read_id(st), "self link canvas", canvas));
    else if (!strcmp(k, "selflinkcanvas")) {
        int id = read_id(st);

        CodObjCanvas * o = CodObjCanvas::read(st, canvas);

        read_keyword(st, "xy");
        int x = read_unsigned(st);
        QPoint p(x, read_unsigned(st));

        CodSelfLinkCanvas * result =
            new CodSelfLinkCanvas(canvas, o, p, id);

        o->set_self_link(result);
        result->setZ(o->z() - 0.5);

        k = read_keyword(st);
        result->settings.read(st, k);		// updates k

        if (!strcmp(k, "forward_label")) {
            result->label = new LabelCanvas(read_string(st), canvas, 0, 0);

            if (read_file_format() < 5) {
                read_keyword(st, "xy");
                read_xy(st, result->label);
                //result->label->setZ(OLD_LABEL_Z);
            }
            else {
                read_keyword(st, "xyz");
                read_xyz(st, result->label);
            }

            result->label->set_center100();
            k = read_keyword(st);
        }

        result->show();

        return result;
    }
    else
        return 0;
}


void CodSelfLinkCanvas::history_save(QBuffer & b) const
{
    DiagramCanvas::history_save(b);
    ::save(delta_x, b);
    ::save(delta_y, b);
    ::save(angle, b);
}

void CodSelfLinkCanvas::history_load(QBuffer & b)
{
    DiagramCanvas::history_load(b);
    ::load(delta_x, b);
    ::load(delta_y, b);
    ::load(angle, b);

    foreach (ColMsg *msg, msgs) {
        const BasicData *oper_data = msg->get_operation();

        if ((oper_data != 0) && subscribe(oper_data)) {
            connect(oper_data, SIGNAL(changed()), this, SLOT(modified()));
            connect(oper_data, SIGNAL(deleted()), this, SLOT(modified()));
        }
    }
}

void CodSelfLinkCanvas::history_hide()
{
    DiagramCanvas::setVisible(FALSE);
    disconnect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));

    foreach (ColMsg *msg, msgs) {
        const BasicData * oper_data = msg->get_operation();

        if ((oper_data != 0) && unsubscribe(oper_data))
            disconnect(oper_data, 0, this, 0);
    }
}
