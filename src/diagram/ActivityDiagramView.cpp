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
#include <qfont.h>
#include <q3popupmenu.h>
//Added by qt3to4:
#include <QTextStream>
#include <QDropEvent>
#include <QMouseEvent>
#include <QDragEnterEvent>

#include "ActivityDiagramWindow.h"
#include "ActivityDiagramView.h"
#include "ActivityCanvas.h"
#include "InterruptibleActivityRegionCanvas.h"
#include "ExpansionRegionCanvas.h"
#include "ActivityNodeCanvas.h"
#include "ActivityActionCanvas.h"
#include "ActivityObjectCanvas.h"
#include "ActivityPartitionCanvas.h"
#include "SimpleRelationCanvas.h"
#include "BrowserActivityDiagram.h"
#include "PackageCanvas.h"
#include "FragmentCanvas.h"
#include "FlowCanvas.h"
#include "IconCanvas.h"
#include "NoteCanvas.h"
#include "TextCanvas.h"
#include "ImageCanvas.h"
#include "UmlPixmap.h"
#include "UmlDrag.h"
#include "BrowserActivity.h"
#include "BrowserInterruptibleActivityRegion.h"
#include "BrowserExpansionRegion.h"
#include "BrowserActivityNode.h"
#include "BrowserActivityObject.h"
#include "BrowserActivityAction.h"
#include "BrowserActivityPartition.h"
#include "ActivityActionData.h"
#include "ActivityObjectData.h"
#include "ClassInstanceData.h"
#include "BrowserPin.h"
#include "PinCanvas.h"
#include "ui/menufactory.h"
#include "DialogUtil.h"
#include "myio.h"
#include "translate.h"

ActivityDiagramView::ActivityDiagramView(QWidget * parent, UmlCanvas * canvas, int id)
    : DiagramView(parent, canvas, id)
{
    load("Activity");
}

void ActivityDiagramView::menu(const QPoint &)
{
    Q3PopupMenu m(0);

    MenuFactory::createTitle(m, TR("Activity diagram menu"));

    switch (default_menu(m, 30)) {
    case EDIT_DRAWING_SETTING_CMD:
        ((BrowserActivityDiagram *) the_canvas()->browser_diagram())->edit_settings();
        break;

    case RELOAD_CMD:
        // pure drawing modifications are lost
        // mark the diagram modified because the undid modifications
        // may be saved in the file are not saved in memory
        load("Activity");
        window()->package_modified();
        break;
    }
}

BrowserNode * ActivityDiagramView::container(const QPoint & p, bool part)
{
    Q3CanvasItem * ci = the_canvas()->collision(p);
    DiagramItem * di;

    return ((ci != 0) &&
            ((di = QCanvasItemToDiagramItem(ci)) != 0) &&
            IsaActivityContainer(di->type(), part))
           ? ((ActivityContainerCanvas *) di)->get_bn()
           : (BrowserNode *) window()->browser_diagram()->parent();
}

void ActivityDiagramView::contentsMousePressEvent(QMouseEvent * e)
{
    if (!window()->frozen()) {
        UmlCode action = window()->buttonOn();

        switch (action) {
        case UmlActivity: {
            history_protected = TRUE;
            unselect_all();
            window()->selectOn();
            history_save();

            BrowserNode * b =
                BrowserActivity::get_activity((BrowserNode *) window()->browser_diagram()->parent()->parent());

            if (b != 0) {
                if (the_canvas()->already_drawn(b)) {
                    msg_information("Douml", TR("already drawn"));
                    history_protected = FALSE;
                    return;
                }

                ActivityCanvas * c =
                    new ActivityCanvas(b, the_canvas(), e->x(), e->y());

                c->show();
                c->upper();
                history_protected = TRUE;
                c->force_sub_inside(TRUE);
                window()->package_modified();
            }
        }
        break;

        case UmlInterruptibleActivityRegion: {
            history_protected = TRUE;
            unselect_all();
            window()->selectOn();
            history_save();

            BrowserNode * parent = container(e->pos());
            BrowserNode * b =
                BrowserInterruptibleActivityRegion::get_interruptibleactivityregion(parent);

            if (b != 0) {
                if (the_canvas()->already_drawn(b)) {
                    msg_information("Douml", TR("already drawn"));
                    history_protected = FALSE;
                    return;
                }

                InterruptibleActivityRegionCanvas * c =
                    new InterruptibleActivityRegionCanvas(b, the_canvas(), e->x(), e->y());
                bool rz;

                c->show();
                history_protected = TRUE;
                rz = !ActivityContainerCanvas::force_inside(c, FALSE);
                history_protected = TRUE;
                c->force_sub_inside(rz);
                window()->package_modified();
            }
        }
        break;

        case UmlExpansionRegion: {
            history_protected = TRUE;
            unselect_all();
            window()->selectOn();
            history_save();

            BrowserNode * parent = container(e->pos());
            BrowserNode * b =
                BrowserExpansionRegion::get_expansionregion(parent);

            if (b != 0) {
                if (the_canvas()->already_drawn(b)) {
                    msg_information("Douml", TR("already drawn"));
                    history_protected = FALSE;
                    return;
                }

                ExpansionRegionCanvas * c =
                    new ExpansionRegionCanvas(b, the_canvas(), e->x(), e->y());
                bool rz;

                c->show();
                history_protected = TRUE;
                rz = !ActivityContainerCanvas::force_inside(c, FALSE);
                history_protected = TRUE;
                c->force_sub_inside(rz);
                window()->package_modified();
            }
        }
        break;

        case InitialAN:
        case ActivityFinalAN:
        case FlowFinalAN:
        case DecisionAN:
        case MergeAN:
        case ForkAN:
        case JoinAN: {
            history_protected = TRUE;
            unselect_all();
            window()->selectOn();
            history_save();

            BrowserNode * parent = container(e->pos());
            BrowserNode * b = BrowserActivityNode::get_activitynode(parent, action);

            if (b != 0) {
                // a new element, don't check already drawn
                ActivityNodeCanvas * c =
                    new ActivityNodeCanvas(b, the_canvas(), e->x(), e->y());

                c->show();
                (void) ActivityContainerCanvas::force_inside(c, FALSE);
                history_protected = TRUE;
                window()->package_modified();
            }
        }
        break;

        case UmlActivityAction: {
            history_protected = TRUE;
            unselect_all();
            window()->selectOn();
            history_save();

            BrowserNode * parent = container(e->pos());
            BrowserNode * b = BrowserActivityAction::get_activityaction(parent);

            if (b != 0) {
                history_protected = TRUE;

                if (the_canvas()->already_drawn(b)) {
                    msg_information("Douml", TR("already drawn"));
                    history_protected = FALSE;
                    return;
                }

                ActivityActionCanvas * c =
                    new ActivityActionCanvas(b, the_canvas(), e->x(), e->y());

                c->show();
                (void) ActivityContainerCanvas::force_inside(c, FALSE);
                history_protected = TRUE;
                c->upper();
                window()->package_modified();
            }
        }
        break;

        case UmlActivityObject: {
            history_protected = TRUE;
            unselect_all();
            window()->selectOn();
            history_save();

            BrowserNode * parent = container(e->pos());
            BrowserNode * b = BrowserActivityObject::get_activityobject(parent);

            if (b != 0) {
                if (the_canvas()->already_drawn(b)) {
                    msg_information("Douml", TR("already drawn"));
                    history_protected = FALSE;
                    return;
                }

                history_protected = TRUE;

                ActivityObjectCanvas * c =
                    new ActivityObjectCanvas(b, the_canvas(), e->x(), e->y());

                c->show();
                (void) ActivityContainerCanvas::force_inside(c, FALSE);
                history_protected = TRUE;
                c->upper();
                window()->package_modified();
            }
        }
        break;

        case UmlActivityPartition: {
            history_protected = TRUE;
            unselect_all();
            window()->selectOn();
            history_save();

            BrowserNode * parent = container(e->pos(), TRUE);
            BrowserNode * b =
                BrowserActivityPartition::get_activitypartition(parent);

            if (b != 0) {
                if (the_canvas()->already_drawn(b)) {
                    msg_information("Douml", TR("already drawn"));
                    history_protected = FALSE;
                    return;
                }

                ActivityPartitionCanvas * c =
                    new ActivityPartitionCanvas(b, the_canvas(), e->x(), e->y());
                bool rz;

                c->show();
                history_protected = TRUE;
                rz = !ActivityContainerCanvas::force_inside(c, TRUE);
                history_protected = TRUE;
                c->force_sub_inside(rz);
                window()->package_modified();
            }
        }
        break;

        default:
            DiagramView::contentsMousePressEvent(e);
            return;
        }

        canvas()->update();
        history_protected = FALSE;
    }
    else
        DiagramView::contentsMousePressEvent(e);
}

void ActivityDiagramView::dragEnterEvent(QDragEnterEvent * e)
{
    if (!window()->frozen() &&
        (UmlDrag::canDecode(e, UmlActivity, TRUE, TRUE) ||
         UmlDrag::canDecode(e, UmlInterruptibleActivityRegion, TRUE, TRUE) ||
         UmlDrag::canDecode(e, UmlExpansionRegion, TRUE, TRUE) ||
         UmlDrag::canDecode(e, UmlActivityPartition, TRUE, TRUE) ||
         UmlDrag::canDecode(e, UmlPackage, FALSE, TRUE) ||
         UmlDrag::canDecode(e, UmlFlow, TRUE, TRUE) ||
         UmlDrag::canDecode(e, UmlSimpleRelations, TRUE, TRUE) ||
         UmlDrag::canDecode(e, UmlClassDiagram, FALSE, TRUE) ||
         UmlDrag::canDecode(e, UmlUseCaseDiagram, FALSE, TRUE) ||
         UmlDrag::canDecode(e, UmlSeqDiagram, FALSE, TRUE) ||
         UmlDrag::canDecode(e, UmlColDiagram, FALSE, TRUE) ||
         UmlDrag::canDecode(e, UmlObjectDiagram, FALSE, TRUE) ||
         UmlDrag::canDecode(e, UmlComponentDiagram, FALSE, TRUE) ||
         UmlDrag::canDecode(e, UmlDeploymentDiagram, FALSE, TRUE) ||
         UmlDrag::canDecode(e, UmlActivityDiagram, TRUE, TRUE) ||
         UmlDrag::canDecode(e, UmlStateDiagram, TRUE, TRUE) ||
         UmlDrag::canDecode(e, UmlActivityNode, TRUE, TRUE) ||
         UmlDrag::canDecode(e, UmlActivityAction, TRUE, TRUE) ||
         UmlDrag::canDecode(e, UmlActivityObject, TRUE, TRUE) ||
         UmlDrag::canDecode(e, UmlOperation, FALSE, TRUE) ||
         UmlDrag::canDecode(e, UmlAttribute, FALSE, TRUE) ||
         UmlDrag::canDecode(e, UmlRelations, TRUE, TRUE) ||
         UmlDrag::canDecode(e, UmlClass, FALSE, TRUE) ||
         UmlDrag::canDecode(e, UmlClassInstance, FALSE, TRUE) ||
         UmlDrag::canDecode(e, UmlState, FALSE, TRUE)))
        e->accept();
    else
        // don't accept Parameter & ParameterSet
        e->ignore();
}

void ActivityDiagramView::dropEvent(QDropEvent * e)
{
    BrowserNode * bn;
    QPoint p = viewportToContents(e->pos());

    if ((bn = UmlDrag::decode(e, UmlActivity, TRUE)) != 0) {
        Q3PopupMenu m(0);

        MenuFactory::createTitle(m, TR("Choose"));
        m.insertSeparator();

        if (!the_canvas()->already_drawn(bn))
            m.insertItem(TR("Draw activity"), 0);

        m.insertItem(TR("Add a call behavior"), 1);

        switch (m.exec(QCursor::pos())) {
        case 0: {
            history_save();
            history_protected = TRUE;

            ActivityCanvas * c =
                new ActivityCanvas(bn, the_canvas(), p.x(), p.y());

            history_protected = TRUE;
            c->show();
            c->upper();
            history_protected = TRUE;
            c->force_sub_inside(TRUE);

            canvas()->update();
            history_protected = FALSE;
            window()->package_modified();
        }
        break;

        case 1: {
            bn = BrowserActivityAction::add_call_behavior(container(p), bn);

            if (bn != 0) {
                history_save();
                history_protected = TRUE;

                ActivityActionCanvas * c =
                    new ActivityActionCanvas(bn, the_canvas(), p.x(), p.y());

                history_protected = TRUE;
                c->show();
                (void) ActivityContainerCanvas::force_inside(c, FALSE);
                history_protected = TRUE;

                canvas()->update();
                history_protected = FALSE;
                window()->package_modified();
            }
        }
        break;

        default:
            break;
        }
    }
    else if ((bn = UmlDrag::decode(e, UmlPackage)) != 0) {
        history_save();

        PackageCanvas * pk =
            new PackageCanvas(bn, the_canvas(), p.x(), p.y(), 0);

        history_protected = TRUE;
        pk->show();
        pk->upper();
        canvas()->update();
        history_protected = FALSE;
        window()->package_modified();
    }
    else if ((bn = UmlDrag::decode(e, UmlActivityNode, TRUE)) != 0) {
        if (the_canvas()->already_drawn(bn))
            msg_information("Douml", TR("already drawn"));
        else {
            history_save();

            ActivityNodeCanvas * c =
                new ActivityNodeCanvas(bn, the_canvas(), p.x(), p.y());

            history_protected = TRUE;
            c->show();
            (void) ActivityContainerCanvas::force_inside(c, FALSE);
            history_protected = TRUE;

            canvas()->update();
            history_protected = FALSE;
            window()->package_modified();
        }
    }
    else if ((bn = UmlDrag::decode(e, UmlInterruptibleActivityRegion, TRUE)) != 0) {
        if (the_canvas()->already_drawn(bn))
            msg_information("Douml", TR("already drawn"));
        else {
            history_save();

            history_protected = TRUE;

            InterruptibleActivityRegionCanvas * c =
                new InterruptibleActivityRegionCanvas(bn, the_canvas(), p.x(), p.y());
            bool sz;

            history_protected = TRUE;
            c->show();
            sz = !ActivityContainerCanvas::force_inside(c, FALSE);
            history_protected = TRUE;
            c->force_sub_inside(sz);
            history_protected = TRUE;

            canvas()->update();
            history_protected = FALSE;
            window()->package_modified();
        }
    }
    else if ((bn = UmlDrag::decode(e, UmlExpansionRegion, TRUE)) != 0) {
        if (the_canvas()->already_drawn(bn))
            msg_information("Douml", TR("already drawn"));
        else {
            history_save();

            history_protected = TRUE;

            ExpansionRegionCanvas * c =
                new ExpansionRegionCanvas(bn, the_canvas(), p.x(), p.y());
            bool sz;

            history_protected = TRUE;
            c->show();
            sz = !ActivityContainerCanvas::force_inside(c, FALSE);
            history_protected = TRUE;
            c->force_sub_inside(sz);
            history_protected = TRUE;

            canvas()->update();
            history_protected = FALSE;
            window()->package_modified();
        }
    }
    else if ((bn = UmlDrag::decode(e, UmlActivityPartition, TRUE)) != 0) {
        if (the_canvas()->already_drawn(bn))
            msg_information("Douml", TR("already drawn"));
        else {
            history_save();

            history_protected = TRUE;

            ActivityPartitionCanvas * c =
                new ActivityPartitionCanvas(bn, the_canvas(), p.x(), p.y());
            bool sz;

            history_protected = TRUE;
            c->show();
            sz = !ActivityContainerCanvas::force_inside(c, TRUE);
            history_protected = TRUE;
            c->force_sub_inside(sz);
            history_protected = TRUE;

            canvas()->update();
            history_protected = FALSE;
            window()->package_modified();
        }
    }
    else if ((bn = UmlDrag::decode(e, UmlActivityAction, TRUE)) != 0) {
        if (the_canvas()->already_drawn(bn))
            msg_information("Douml", TR("already drawn"));
        else {
            history_save();
            history_protected = TRUE;

            ActivityActionCanvas * c =
                new ActivityActionCanvas(bn, the_canvas(), p.x(), p.y());

            history_protected = TRUE;
            c->show();
            (void) ActivityContainerCanvas::force_inside(c, FALSE);
            history_protected = TRUE;

            canvas()->update();
            history_protected = FALSE;
            window()->package_modified();
        }
    }
    else if ((bn = UmlDrag::decode(e, UmlActivityObject, TRUE)) != 0) {
        if (the_canvas()->already_drawn(bn))
            msg_information("Douml", TR("already drawn"));
        else {
            history_save();
            history_protected = TRUE;

            ActivityObjectCanvas * c =
                new ActivityObjectCanvas(bn, the_canvas(), p.x(), p.y());

            history_protected = TRUE;
            c->show();
            (void) ActivityContainerCanvas::force_inside(c, FALSE);
            history_protected = TRUE;

            canvas()->update();
            history_protected = FALSE;
            window()->package_modified();
        }
    }
    else if ((bn = UmlDrag::decode(e, UmlFlow, TRUE)) != 0) {
        history_save();

        history_protected = TRUE;
        FlowCanvas::drop(bn, the_canvas());
        canvas()->update();
        history_protected = FALSE;
    }
    else if ((bn = UmlDrag::decode(e, UmlSimpleRelations, TRUE)) != 0) {
        history_save();

        history_protected = TRUE;
        SimpleRelationCanvas::drop(bn, the_canvas());
        canvas()->update();
        history_protected = FALSE;
    }
    else if (((bn = UmlDrag::decode(e, UmlClassDiagram)) != 0) ||
             ((bn = UmlDrag::decode(e, UmlUseCaseDiagram)) != 0) ||
             ((bn = UmlDrag::decode(e, UmlSeqDiagram)) != 0) ||
             ((bn = UmlDrag::decode(e, UmlColDiagram)) != 0) ||
             ((bn = UmlDrag::decode(e, UmlObjectDiagram)) != 0) ||
             ((bn = UmlDrag::decode(e, UmlActivityDiagram)) != 0) ||
             ((bn = UmlDrag::decode(e, UmlComponentDiagram)) != 0) ||
             ((bn = UmlDrag::decode(e, UmlDeploymentDiagram)) != 0) ||
             ((bn = UmlDrag::decode(e, UmlStateDiagram, TRUE)) != 0) ||
             ((bn = UmlDrag::decode(e, UmlActivityDiagram, TRUE)) != 0)) {
        history_save();

        IconCanvas * ic = new IconCanvas(bn, the_canvas(), p.x(), p.y(), 0);

        history_protected = TRUE;
        ic->show();
        ic->upper();

        canvas()->update();
        history_protected = FALSE;
        window()->package_modified();
    }
    else if ((bn = UmlDrag::decode(e, UmlOperation)) != 0) {
        bn = BrowserActivityAction::add_call_operation(container(p), (BrowserOperation *) bn);

        if (bn != 0) {
            history_save();
            history_protected = TRUE;

            ActivityActionCanvas * c =
                new ActivityActionCanvas(bn, the_canvas(), p.x(), p.y());

            history_protected = TRUE;
            c->show();
            (void) ActivityContainerCanvas::force_inside(c, FALSE);
            history_protected = TRUE;

            canvas()->update();
            history_protected = FALSE;
            window()->package_modified();
        }
    }
    else if (((bn = UmlDrag::decode(e, UmlAttribute)) != 0) ||
             ((bn = UmlDrag::decode(e, UmlRelations, TRUE)) != 0)) {
        Q3PopupMenu m(0);

        MenuFactory::createTitle(m, TR("Choose"));
        m.insertSeparator();
        m.insertItem(TR("Add a read variable value action"), 0);
        m.insertItem(TR("Add a clear variable value action"), 1);
        m.insertItem(TR("Add a write variable value action"), 2);
        m.insertItem(TR("Add a add variable value action"), 3);
        m.insertItem(TR("Add a remove variable value action"), 4);

        switch (m.exec(QCursor::pos())) {
        case 0:
            bn = BrowserActivityAction::add_read_variable_value(container(p), bn);
            break;

        case 1:
            bn = BrowserActivityAction::add_clear_variable_value(container(p), bn);
            break;

        case 2:
            bn = BrowserActivityAction::add_write_variable_value(container(p), bn);
            break;

        case 3:
            bn = BrowserActivityAction::add_add_variable_value(container(p), bn);
            break;

        case 4:
            bn = BrowserActivityAction::add_remove_variable_value(container(p), bn);
            break;

        default:
            return;
        }

        if (bn != 0) {
            history_save();
            history_protected = TRUE;

            ActivityActionCanvas * c =
                new ActivityActionCanvas(bn, the_canvas(), p.x(), p.y());

            history_protected = TRUE;
            c->show();
            (void) ActivityContainerCanvas::force_inside(c, FALSE);
            history_protected = TRUE;

            canvas()->update();
            history_protected = FALSE;
            window()->package_modified();
        }
    }
    else if ((bn = UmlDrag::decode(e, UmlClass)) != 0) {
        history_protected = FALSE;

        BrowserActivityObject * obj =
            BrowserActivityObject::add_activityobject(container(p), 0);

        if (obj != 0) {
            ((ActivityObjectData *) obj->get_data())->set_type((BrowserClass *) bn);

            history_save();
            history_protected = TRUE;

            ActivityObjectCanvas * c =
                new ActivityObjectCanvas(obj, the_canvas(), p.x(), p.y());

            history_protected = TRUE;
            c->show();
            (void) ActivityContainerCanvas::force_inside(c, FALSE);
            history_protected = TRUE;

            canvas()->update();
            history_protected = FALSE;
            window()->package_modified();
        }
    }
    else if ((bn = UmlDrag::decode(e, UmlClassInstance)) != 0) {
        history_protected = FALSE;

        BrowserActivityObject * obj =
            BrowserActivityObject::add_activityobject(container(p), bn->get_name());

        if (obj != 0) {
            ((ActivityObjectData *) obj->get_data())
            ->set_type(((ClassInstanceData *) bn->get_data())->get_class());

            history_save();
            history_protected = TRUE;

            ActivityObjectCanvas * c =
                new ActivityObjectCanvas(obj, the_canvas(), p.x(), p.y());

            history_protected = TRUE;
            c->show();
            (void) ActivityContainerCanvas::force_inside(c, FALSE);
            history_protected = TRUE;

            canvas()->update();
            history_protected = FALSE;
            window()->package_modified();
        }
    }
    else if ((bn = UmlDrag::decode(e, UmlState, TRUE)) != 0) {
        history_protected = FALSE;

        bn = BrowserActivityAction::add_call_behavior(container(p), bn);

        if (bn != 0) {
            history_save();
            history_protected = TRUE;

            ActivityActionCanvas * c =
                new ActivityActionCanvas(bn, the_canvas(), p.x(), p.y());

            history_protected = TRUE;
            c->show();
            (void) ActivityContainerCanvas::force_inside(c, FALSE);
            history_protected = TRUE;
            canvas()->update();
            history_protected = FALSE;
            window()->package_modified();
        }
    }
}

void ActivityDiagramView::save(QTextStream & st, QString & warning,
                               bool copy) const
{
    DiagramItemList items(canvas()->allItems());

    if (!copy)
        // sort is useless for a copy
        items.sort();

    st << "format " << api_format() << "\n";

    // save first activity, activity nodes, actions, objects, packages, fragments, notes and icons

    foreach (DiagramItem *di, items) {
        switch (di->type()) {
        case UmlActivity:
        case UmlInterruptibleActivityRegion:
        case UmlExpansionRegion:
        case UmlActivityPartition:
        case InitialAN:
        case ActivityFinalAN:
        case FlowFinalAN:
        case DecisionAN:
        case MergeAN:
        case ForkAN:
        case JoinAN:
        case UmlActivityAction:
        case UmlActivityObject:
        case UmlNote:
        case UmlText:
        case UmlImage:
        case UmlPackage:
        case UmlFragment:
        case UmlIcon:
            if (!copy || di->copyable())
                di->save(st, FALSE, warning);

            // no break
        default:
            // Parameters, parameter sets and pins are saved by their parents
            // pre & post conditions are saved by activity action
            break;
        }
    }

    // then saves relations

    foreach (DiagramItem *di, items) {
        switch (di->type()) {
        case UmlFlow:
        case UmlDependOn:
            if (!copy || di->copyable())
                di->save(st, FALSE, warning);

        default:
            break;
        }
    }

    // then saves anchors

    foreach (DiagramItem *di, items)
        if ((!copy || di->copyable()) && (di->type() == UmlAnchor))
            di->save(st, FALSE, warning);

    if (!copy && (preferred_zoom != 0)) {
        nl_indent(st);
        st << "preferred_whz " << preferred_size.width() << " "
           << preferred_size.height() << " " << preferred_zoom;
    }

    nl_indent(st);
    st << "end\n";
}

void ActivityDiagramView::read(char * st, char * k)
{
    // reads first state package icons notes text and image
    UmlCanvas * canvas = the_canvas();

    while (ActivityCanvas::read(st, canvas, k) ||
           InterruptibleActivityRegionCanvas::read(st, canvas, k) ||
           ExpansionRegionCanvas::read(st, canvas, k) ||
           ActivityPartitionCanvas::read(st, canvas, k) ||
           ActivityActionCanvas::read(st, canvas, k) ||
           ActivityObjectCanvas::read(st, canvas, k) ||
           ActivityNodeCanvas::read(st, canvas, k) ||
           NoteCanvas::read(st, canvas, k) ||
           TextCanvas::read(st, canvas, k) ||
           IconCanvas::read(st, canvas, k) ||
           PackageCanvas::read(st, canvas, k) ||
           FragmentCanvas::read(st, canvas, k) ||
           ImageCanvas::read(st, canvas, k))
        k = read_keyword(st);

    // then reads relations and anchors
    ArrowCanvas * a;

    while (((a = ArrowCanvas::read(st, canvas, k)) != 0) ||
           ((a = FlowCanvas::read(st, canvas, k)) != 0) ||
           ((a = SimpleRelationCanvas::read(st, canvas, k)) != 0)) {
        a->get_start()->check_line(a);
        k = read_keyword(st);
    }

    if (!strcmp(k, "preferred_whz") || !strcmp(k, "prefered_whz")) {
        preferred_size.setWidth(read_unsigned(st));
        preferred_size.setHeight(read_unsigned(st));
        preferred_zoom = read_double(st);
        k = read_keyword(st);
    }

    if (strcmp(k, "end"))
        wrong_keyword(k, "end");
}


