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


#include "ColDiagramWindow.h"
#include "ColDiagramView.h"
#include "BrowserColDiagram.h"
#include "BrowserClassInstance.h"
#include "BrowserClass.h"
#include "DiagramCanvas.h"
#include "CodClassInstCanvas.h"
#include "CodDirsCanvas.h"
#include "CodLinkCanvas.h"
#include "CodSelfLinkCanvas.h"
#include "ArrowCanvas.h"
#include "IconCanvas.h"
#include "NoteCanvas.h"
#include "PackageCanvas.h"
#include "FragmentCanvas.h"
#include "TextCanvas.h"
#include "ImageCanvas.h"
#include "UmlPixmap.h"
#include "UmlDrag.h"
#include "myio.h"
#include "ui/menufactory.h"
#include "DialogUtil.h"
#include "CodEditMsgDialog.h"
#include "translate.h"

ColDiagramView::ColDiagramView(QWidget * parent, UmlCanvas * canvas, int id)
    : DiagramView(parent, canvas, id)
{
    load("Communication");
    update_msgs();
}

void ColDiagramView::update_msgs()
{
    ColMsg::update_ranks(get_msgs());
    update_msg_supports();
}

void ColDiagramView::update_msg_supports()
{
    Q3CanvasItemList l = canvas()->allItems();
    Q3CanvasItemList::Iterator it;

    for (it = l.begin(); it != l.end(); ++it) {
        if ((*it)->visible()) { // at least not deleted
            DiagramItem * di = QCanvasItemToDiagramItem(*it);

            if (di != 0) {
                switch (di->type()) {
                case UmlSelfLink:
                    ((CodSelfLinkCanvas *) di)->update_msgs();
                    break;

                case UmlLinkDirs:
                    ((CodDirsCanvas *) di)->update_msgs();
                    break;

                default:	// to avoid compiler warning
                    break;
                }
            }
        }
    }
}

void ColDiagramView::menu(const QPoint &)
{
    Q3PopupMenu m(0);

    MenuFactory::createTitle(m, TR("Communication diagram menu"));

    if ((((UmlCanvas *) canvas())->browser_diagram())->is_writable()) {
        m.insertSeparator();
        m.insertItem(TR("Edit all the messages"), 29);
    }

    switch (default_menu(m, 30)) {
    case EDIT_DRAWING_SETTING_CMD:
        ((BrowserColDiagram *) the_canvas()->browser_diagram())->edit_settings();
        break;

    case RELOAD_CMD:
        // pure drawing modifications are lost
        // mark the diagram modified because the undid modifications
        // may be saved in the file are not saved in memory
        load("Communication");
        update_msgs();
        window()->package_modified();
        break;

    case 29:
        CodEditMsgDialog::exec(this, msgs);
        break;
    }
}

void ColDiagramView::contentsMousePressEvent(QMouseEvent * e)
{
    if (!window()->frozen()) {
        if (e->button() == ::Qt::RightButton)
            DiagramView::contentsMousePressEvent(e);
        else {
            UmlCode c = window()->buttonOn();

            switch (c) {
            case UmlClass: {
                history_protected = FALSE;
                unselect_all();
                window()->selectOn();
                history_save();

                BrowserNode * parent =
                    ((BrowserNode *) window()->browser_diagram()->parent());
                BrowserClass * b =
                    BrowserClass::get_class(parent);

                if (b != 0) {
                    CodClassInstCanvas * cl =
                        new CodClassInstCanvas(b, the_canvas(),
                                               e->x(), e->y(), 0);

                    cl->show();
                    cl->upper();
                    window()->package_modified();
                }
            }

            canvas()->update();
            break;

            case UmlClassInstance: {
                history_protected = TRUE;
                unselect_all();
                window()->selectOn();
                history_save();

                BrowserNode * parent =
                    ((BrowserNode *) window()->browser_diagram()->parent());
                BrowserClassInstance * b =
                    BrowserClassInstance::get_classinstance(parent);

                if (b != 0) {
                    CodClassInstCanvas * cl =
                        new CodClassInstCanvas(b, the_canvas(),
                                               e->x(), e->y(), 0);

                    cl->show();
                    cl->upper();
                    window()->package_modified();
                }
            }

            canvas()->update();
            history_protected = FALSE;
            break;

            case UmlSelfLink: {
                history_protected = TRUE;
                window()->selectOn();
                history_save();

                Q3CanvasItem * ci = the_canvas()->collision(e->pos());

                if (ci != 0) {
                    DiagramItem * i = QCanvasItemToDiagramItem(ci);

                    if (i != 0) {
                        QString err = i->may_start(c);

                        if (!err.isEmpty())
                            msg_critical("Douml", err);
                        else {
                            i->connexion(c, i, e->pos(), e->pos());
                            window()->package_modified();
                        }
                    }
                }
            }

            canvas()->update();
            history_protected = FALSE;
            break;

            default:
                DiagramView::contentsMousePressEvent(e);
                break;
            }
        }
    }
    else
        DiagramView::contentsMousePressEvent(e);
}

void ColDiagramView::dragEnterEvent(QDragEnterEvent * e)
{
    if (!window()->frozen() &&
        (UmlDrag::canDecode(e, UmlClass, TRUE, TRUE) ||
         UmlDrag::canDecode(e, UmlClassInstance, TRUE, TRUE) ||
         UmlDrag::canDecode(e, UmlPackage, FALSE, TRUE) ||
         UmlDrag::canDecode(e, UmlClassDiagram, FALSE, TRUE) ||
         UmlDrag::canDecode(e, UmlUseCaseDiagram, FALSE, TRUE) ||
         UmlDrag::canDecode(e, UmlSeqDiagram, FALSE, TRUE) ||
         UmlDrag::canDecode(e, UmlColDiagram, FALSE, TRUE) ||
         UmlDrag::canDecode(e, UmlObjectDiagram, FALSE, TRUE) ||
         UmlDrag::canDecode(e, UmlComponentDiagram, FALSE, TRUE) ||
         UmlDrag::canDecode(e, UmlDeploymentDiagram, FALSE, TRUE) ||
         UmlDrag::canDecode(e, UmlActivityDiagram, TRUE, TRUE) ||
         UmlDrag::canDecode(e, UmlStateDiagram, TRUE, TRUE)))
        e->accept();
    else
        e->ignore();
}

void ColDiagramView::dropEvent(QDropEvent * e)
{
    BrowserNode * bn;
    QPoint p = viewportToContents(e->pos());

    if ((bn = UmlDrag::decode(e, UmlClassInstance)) != 0) {
        history_save();

        CodClassInstCanvas * i =
            new CodClassInstCanvas((BrowserClassInstance *) bn,
                                   the_canvas(), p.x(), p.y(), 0);

        history_protected = TRUE;
        i->show();
        i->upper();
        canvas()->update();
        history_protected = FALSE;
        window()->package_modified();
    }
    else if ((bn = UmlDrag::decode(e, UmlClass)) != 0) {
        history_save();

        CodClassInstCanvas * cli =
            new CodClassInstCanvas(bn, the_canvas(), p.x(), p.y(), 0);

        history_protected = TRUE;
        cli->show();
        cli->upper();
        canvas()->update();
        history_protected = FALSE;
        window()->package_modified();
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
    else if (((bn = UmlDrag::decode(e, UmlClassDiagram)) != 0) ||
             ((bn = UmlDrag::decode(e, UmlUseCaseDiagram)) != 0) ||
             ((bn = UmlDrag::decode(e, UmlSeqDiagram)) != 0) ||
             ((bn = UmlDrag::decode(e, UmlColDiagram)) != 0) ||
             ((bn = UmlDrag::decode(e, UmlObjectDiagram)) != 0) ||
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

void ColDiagramView::save(QTextStream & st, QString & warning,
                          bool copy) const
{
    DiagramItemList items(canvas()->allItems());

    if (!copy)
        // sort is useless for a copy
        items.sort();

    st << "format " << api_format() << "\n";

    // save first the packages fragment, classes instances, notes, icons and text

    foreach (DiagramItem *di, items) {
        switch (di->type()) {
        case UmlPackage:
        case UmlFragment:
        case UmlClass:
        case UmlClassInstance:
        case UmlNote:
        case UmlText:
        case UmlImage:
        case UmlIcon:
            if (!copy || di->copyable())
                di->save(st, FALSE, warning);

            // no break
        default:
            break;
        }
    }

    // then save links selflink and dirs (without messages)

    foreach (DiagramItem *di, items) {
        switch (di->type()) {
        case UmlLink:	// saves associated dirs
        case UmlSelfLink:
            if (!copy || di->copyable())
                di->save(st, FALSE, warning);

        default:	// to avoid warning
            break;
        }
    }

    // then save messages
    ColMsg::save(st, msgs, copy, warning,
                 window()->browser_diagram()->full_name());

    // then save anchors

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

void ColDiagramView::read(char * st, char * k)
{
    UmlCanvas * canvas = the_canvas();

    // reads first the instances, package, fragment notes, icons text and images
    while (CodClassInstCanvas::read(st, canvas, k) ||
           NoteCanvas::read(st, canvas, k) ||
           TextCanvas::read(st, canvas, k) ||
           IconCanvas::read(st, canvas, k) ||
           PackageCanvas::read(st, canvas, k) ||
           FragmentCanvas::read(st, canvas, k) ||
           ImageCanvas::read(st, canvas, k))
        k = read_keyword(st);

    // then reads links selflink and dirs (without messages)
    while (CodLinkCanvas::read(st, canvas, k) ||	// updates k
           CodSelfLinkCanvas::read(st, canvas, k) ||	// updates k
           CodDirsCanvas::read(st, canvas, k))		// updates k
        ;

    // then read messages
    if (!strcmp(k, "msgs"))
        ColMsg::read(st, msgs, canvas);
    else if (strcmp(k, "no_msg"))
        wrong_keyword(k, "msgs/no_msg");

    k = read_keyword(st);

    // then show/update msgs
    Q3CanvasItemList items = canvas->allItems();
    Q3CanvasItemList::Iterator it;

    for (it = items.begin(); it != items.end(); ++it) {
        DiagramItem * di = QCanvasItemToDiagramItem(*it);

        if (di != 0) {
            switch (di->type()) {
            case UmlSelfLink:
                ((CodSelfLinkCanvas *) di)->update_msgs();
                break;

            case UmlLinkDirs:
                ((CodDirsCanvas *) di)->update_msgs();
                break;

            default:	// to avoid warning
                break;
            }
        }
    }

    // then reads anchors
    while (ArrowCanvas::read(st, canvas, k))
        k = read_keyword(st);

    if (!strcmp(k, "preferred_whz") || !strcmp(k, "prefered_whz")) {
        preferred_size.setWidth(read_unsigned(st));
        preferred_size.setHeight(read_unsigned(st));
        preferred_zoom = read_double(st);
        k = read_keyword(st);
    }

    if (strcmp(k, "end"))
        wrong_keyword(k, "end");
}

// for plug-out

void ColDiagramView::send(ToolCom * com)
{
    Q3CanvasItemList l = canvas()->allItems();

    //FragmentCanvas::send(com, l);
    CodClassInstCanvas::send(com, l);
    ColMsg::send(com, msgs);
}
