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





#include <qfont.h>
#include <q3popupmenu.h>
//Added by qt3to4:
#include <QTextStream>
#include <QDropEvent>
#include <QMouseEvent>
#include <QDragEnterEvent>

#include "StateDiagramWindow.h"
#include "StateDiagramView.h"
#include "StateCanvas.h"
#include "PseudoStateCanvas.h"
#include "BrowserStateDiagram.h"
#include "PackageCanvas.h"
#include "FragmentCanvas.h"
#include "TransitionCanvas.h"
#include "StateActionCanvas.h"
#include "IconCanvas.h"
#include "NoteCanvas.h"
#include "TextCanvas.h"
#include "ImageCanvas.h"
#include "UmlPixmap.h"
#include "UmlDrag.h"
#include "BrowserState.h"
#include "BrowserRegion.h"
#include "BrowserPseudoState.h"
#include "BrowserStateAction.h"
#include "StateActionData.h"
#include "ui/menufactory.h"
#include "DialogUtil.h"
#include "myio.h"
#include "translate.h"

StateDiagramView::StateDiagramView(QWidget * parent, UmlCanvas * canvas, int id)
    : DiagramView(parent, canvas, id)
{
    load("State");
}

void StateDiagramView::menu(const QPoint &)
{
    Q3PopupMenu m(0);

    MenuFactory::createTitle(m, TR("State diagram menu"));

    switch (default_menu(m, 30)) {
    case EDIT_DRAWING_SETTING_CMD:
        ((BrowserStateDiagram *) the_canvas()->browser_diagram())->edit_settings();
        break;

    case RELOAD_CMD:
        // pure drawing modifications are lost
        // mark the diagram modified because the undid modifications
        // may be saved in the file are not saved in memory
        load("State");
        window()->package_modified();
        break;
    }
}

void StateDiagramView::contentsMousePressEvent(QMouseEvent * e)
{
    if (!window()->frozen()) {
        UmlCode action = window()->buttonOn();

        switch (action) {
        case UmlState: {
            history_protected = TRUE;
            unselect_all();
            window()->selectOn();
            history_save();

            Q3CanvasItem * ci = the_canvas()->collision(e->pos());
            DiagramItem * di;
            bool specified = ((ci != 0) &&
                              ((di = QCanvasItemToDiagramItem(ci)) != 0) &&
                              (di->type() == UmlState) &&
                              ((BrowserState *)((StateCanvas *) di)->get_bn())->may_contains(action));
            BrowserNode * parent;

            if (specified) {
                if ((parent = ((StateCanvas *) di)->pointed_region(e->pos())) == 0)
                    parent = ((StateCanvas *) di)->get_bn();
            }
            else
                parent = (BrowserNode *) window()->browser_diagram()->parent();

            BrowserNode * b = BrowserState::get_state(parent);

            if (b != 0) {
                if (the_canvas()->already_drawn(b)) {
                    msg_information("Douml", TR("already drawn"));
                    history_protected = FALSE;
                    return;
                }

                // may be added in a the state's region rather than the state
                StateCanvas * c = new StateCanvas(b, the_canvas(), e->x(), e->y());

                c->show();
                c->upper();
                canvas()->update();
                StateCanvas::force_inside(c, TRUE);
                canvas()->update();
                history_protected = FALSE;
                window()->package_modified();
                return; // canvas already updated
            }
        }
        break;

        case UmlRegion: {
            history_protected = TRUE;
            unselect_all();
            window()->selectOn();
            history_save();

            Q3CanvasItem * ci = the_canvas()->collision(e->pos());
            DiagramItem * di;

            if ((ci != 0) &&
                ((di = QCanvasItemToDiagramItem(ci)) != 0) &&
                (di->type() == UmlState) &&
                ((BrowserState *)((StateCanvas *) di)->get_bn())->may_contains(action)) {
                BrowserRegion::add_region(((StateCanvas *) di)->get_bn());
                window()->package_modified();
            }
        }
        break;

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
            history_protected = TRUE;
            unselect_all();
            window()->selectOn();
            history_save();

            Q3CanvasItem * ci = the_canvas()->collision(e->pos());
            DiagramItem * di;
            bool specified = ((ci != 0) &&
                              ((di = QCanvasItemToDiagramItem(ci)) != 0) &&
                              (di->type() == UmlState) &&
                              ((BrowserState *)((StateCanvas *) di)->get_bn())->may_contains(action));
            BrowserNode * parent;

            if (specified) {
                if ((parent = ((StateCanvas *) di)->pointed_region(e->pos())) == 0)
                    parent = ((StateCanvas *) di)->get_bn();
            }
            else
                parent = (BrowserNode *) window()->browser_diagram()->parent();

            BrowserNode * b = BrowserPseudoState::get_pseudostate(parent, action);

            if (b != 0) {
                PseudoStateCanvas * c =
                    // may be added in a the state's region rather than the state
                    new PseudoStateCanvas(b, the_canvas(), e->x(), e->y());

                c->show();
                c->upper();
                StateCanvas::force_inside(c, FALSE);
                window()->package_modified();
            }
        }
        break;

        case UmlStateAction:
        case UmlSignalIn:
        case UmlSignalOut: {
            history_protected = TRUE;
            unselect_all();
            window()->selectOn();
            history_save();

            Q3CanvasItem * ci = the_canvas()->collision(e->pos());
            DiagramItem * di;
            bool specified = ((ci != 0) &&
                              ((di = QCanvasItemToDiagramItem(ci)) != 0) &&
                              (di->type() == UmlState) &&
                              ((BrowserState *)((StateCanvas *) di)->get_bn())->may_contains(UmlStateAction));
            BrowserNode * parent;

            if (specified) {
                if ((parent = ((StateCanvas *) di)->pointed_region(e->pos())) == 0)
                    parent = ((StateCanvas *) di)->get_bn();
            }
            else
                parent = (BrowserNode *) window()->browser_diagram()->parent();

            BrowserNode * b = BrowserStateAction::add_stateaction(parent);

            switch (action) {
            case UmlSignalIn:
                ((StateActionData *) b->get_data())->set_stereotype("receive-signal");
                break;

            case UmlSignalOut:
                ((StateActionData *) b->get_data())->set_stereotype("send-signal");
                break;

            default:	// to avoid warning
                break;
            }

            StateActionCanvas * c =
                // may be added in a the state's region rather than the state
                new StateActionCanvas(b, the_canvas(), e->x(), e->y());

            c->show();
            c->upper();
            StateCanvas::force_inside(c, FALSE);
            window()->package_modified();
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

void StateDiagramView::dragEnterEvent(QDragEnterEvent * e)
{
    if (!window()->frozen() &&
        (UmlDrag::canDecode(e, UmlState, TRUE, TRUE) ||
         UmlDrag::canDecode(e, UmlPackage, FALSE, TRUE) ||
         UmlDrag::canDecode(e, UmlTransition, TRUE, TRUE) ||
         UmlDrag::canDecode(e, UmlClassDiagram, FALSE, TRUE) ||
         UmlDrag::canDecode(e, UmlUseCaseDiagram, FALSE, TRUE) ||
         UmlDrag::canDecode(e, UmlSeqDiagram, FALSE, TRUE) ||
         UmlDrag::canDecode(e, UmlColDiagram, FALSE, TRUE) ||
         UmlDrag::canDecode(e, UmlObjectDiagram, FALSE, TRUE) ||
         UmlDrag::canDecode(e, UmlComponentDiagram, FALSE, TRUE) ||
         UmlDrag::canDecode(e, UmlStateDiagram, TRUE, TRUE) ||
         UmlDrag::canDecode(e, UmlDeploymentDiagram, FALSE, TRUE) ||
         UmlDrag::canDecode(e, UmlPseudoState, TRUE, TRUE) ||
         UmlDrag::canDecode(e, UmlStateAction, TRUE, TRUE) ||
         UmlDrag::canDecode(e, UmlActivityDiagram, TRUE, TRUE)))
        e->accept();
    else
        e->ignore();
}

void StateDiagramView::dropEvent(QDropEvent * e)
{
    BrowserState * mach =
        BrowserState::get_machine(the_canvas()->browser_diagram());
    BrowserNode * bn;
    QPoint p = viewportToContents(e->pos());

    if ((bn = UmlDrag::decode(e, UmlState, TRUE)) != 0) {
        if (the_canvas()->already_drawn(bn))
            msg_information("Douml", TR("already drawn"));
        else if (BrowserState::get_machine(bn) != mach)
            msg_information("Douml", TR("illegal"));
        else {
            history_save();

            // mettre des contraintes sur la validite ?
            StateCanvas * c =
                new StateCanvas(bn, the_canvas(), p.x(), p.y());

            history_protected = TRUE;
            c->show();
            c->upper();
            canvas()->update();
            StateCanvas::force_inside(c, TRUE);
            canvas()->update();
            history_protected = FALSE;
            window()->package_modified();
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
        window()->package_modified();
    }
    else if ((bn = UmlDrag::decode(e, UmlPseudoState, TRUE)) != 0) {
        if (the_canvas()->already_drawn(bn))
            msg_information("Douml", TR("already drawn"));
        else if (BrowserState::get_machine(bn) != mach)
            msg_information("Douml", TR("illegal"));
        else {
            history_save();

            // mettre des contraintes sur la validite ?
            PseudoStateCanvas * c =
                new PseudoStateCanvas(bn, the_canvas(), p.x(), p.y());

            history_protected = TRUE;
            c->show();
            c->upper();
            StateCanvas::force_inside(c, FALSE);
            canvas()->update();
            history_protected = FALSE;
            window()->package_modified();
        }
    }
    else if ((bn = UmlDrag::decode(e, UmlStateAction, TRUE)) != 0) {
        if (the_canvas()->already_drawn(bn))
            msg_information("Douml", TR("already drawn"));
        else if (BrowserState::get_machine(bn) != mach)
            msg_information("Douml", TR("illegal"));
        else {
            history_save();

            // mettre des contraintes sur la validite ?
            StateActionCanvas * c =
                new StateActionCanvas(bn, the_canvas(), p.x(), p.y());

            history_protected = TRUE;
            c->show();
            c->upper();
            StateCanvas::force_inside(c, FALSE);
            canvas()->update();
            history_protected = FALSE;
            window()->package_modified();
        }
    }
    else if ((bn = UmlDrag::decode(e, UmlTransition, TRUE)) != 0) {
        history_save();

        history_protected = TRUE;
        TransitionCanvas::drop(bn, the_canvas());
        canvas()->update();
        history_protected = FALSE;
    }
    else if (((bn = UmlDrag::decode(e, UmlClassDiagram)) != 0) ||
             ((bn = UmlDrag::decode(e, UmlUseCaseDiagram)) != 0) ||
             ((bn = UmlDrag::decode(e, UmlSeqDiagram)) != 0) ||
             ((bn = UmlDrag::decode(e, UmlColDiagram)) != 0) ||
             ((bn = UmlDrag::decode(e, UmlObjectDiagram)) != 0) ||
             ((bn = UmlDrag::decode(e, UmlStateDiagram)) != 0) ||
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
}

void StateDiagramView::save(QTextStream & st, QString & warning,
                            bool copy) const
{
    DiagramItemList items(canvas()->allItems());

    if (!copy)
        // sort is useless for a copy
        items.sort();

    st << "format " << api_format() << "\n";

    // save first states pseudostates actions packages fragments notes and icons

    foreach (DiagramItem *di, items) {
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
            break;
        }
    }

    // then saves relations

    foreach (DiagramItem *di, items) {
        if (di->type() == UmlTransition) {
            if (!copy || di->copyable())
                di->save(st, FALSE, warning);
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

void StateDiagramView::read(char * st, char * k)
{
    UmlCanvas * canvas = the_canvas();

    // reads first state package icons notes text and images
    while (StateCanvas::read(st, canvas, k) ||
           StateActionCanvas::read(st, canvas, k) ||
           PseudoStateCanvas::read(st, canvas, k) ||
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
           ((a = TransitionCanvas::read(st, canvas, k)) != 0)) {
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


