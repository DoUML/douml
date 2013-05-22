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





#include <qapplication.h>
#include <qfont.h>
#include <q3popupmenu.h>
//Added by qt3to4:
#include <QTextStream>
#include <QDropEvent>
#include <QMouseEvent>
#include <QDragEnterEvent>

#include "ClassDiagramWindow.h"
#include "ClassDiagramView.h"
#include "BrowserClassDiagram.h"
#include "BrowserClass.h"
#include "BrowserRelation.h"
#include "DiagramCanvas.h"
#include "CdClassCanvas.h"
#include "PackageCanvas.h"
#include "FragmentCanvas.h"
#include "RelationCanvas.h"
#include "SimpleRelationCanvas.h"
#include "IconCanvas.h"
#include "NoteCanvas.h"
#include "TextCanvas.h"
#include "ImageCanvas.h"
#include "UmlPixmap.h"
#include "UmlDrag.h"
#include "myio.h"
#include "ui/menufactory.h"
#include "BrowserView.h"
#include "RelatedElementsDialog.h"
#include "translate.h"

ClassDiagramView::ClassDiagramView(QWidget * parent, UmlCanvas * canvas, int id)
    : DiagramView(parent, canvas, id)
{
    load("Class");
}

// class view contains class not already drawn ?
static bool not_yet_drawn(BrowserNode * container,
                          Q3PtrDict<DiagramItem> & drawn)
{
    Q3ListViewItem * child;

    for (child = container->firstChild(); child != 0; child = child->nextSibling()) {
        if (!((BrowserNode *) child)->deletedp() &&
            (((BrowserNode *) child)->get_type() == UmlClass))
            if ((drawn[((BrowserNode *) child)->get_data()] == 0) ||
                not_yet_drawn((BrowserNode *) child, drawn))
                return TRUE;
    }

    return FALSE;
}

// have marked elements not yet drawn ?
static bool marked_not_yet_drawn(Q3PtrDict<DiagramItem> & drawn)
{
    foreach (BrowserNode * bn, BrowserNode::marked_nodes()) {
        UmlCode k = bn->get_type();

        switch (k) {
        default:
            if (!IsaRelation(k) && !IsaSimpleRelation(k))
                break;

            // no break
        case UmlClass:
        case UmlPackage:
            if (drawn[bn->get_data()] == 0)
                return TRUE;
        }
    }

    return FALSE;
}

static void get_drawn(DiagramItemList & items,
                      Q3PtrDict<DiagramItem> & drawn)
{
    foreach (DiagramItem * di, items) {
        UmlCode k = di->type();

        switch (k) {
        case UmlClass:
        case UmlPackage:
            drawn.replace(di->get_bn()->get_data(), di);
            break;

        default:
            if (IsaRelation(k) || IsaSimpleRelation(k))
                drawn.replace(((ArrowCanvas *) di)->get_data(), di);
        }
    }
}

void ClassDiagramView::menu(const QPoint & p)
{
    Q3PopupMenu m(0);

    MenuFactory::createTitle(m, TR("Class diagram menu"));

    if ((((UmlCanvas *) canvas())->browser_diagram())->is_writable()) {
        BrowserNode * bn = BrowserView::selected_item();
        DiagramItemList items(canvas()->allItems());
        Q3PtrDict<DiagramItem> drawn;

        get_drawn(items, drawn);

        if ((bn != 0) && (bn->get_type() == UmlClassView)) {
            if (not_yet_drawn(bn, drawn)) {
                m.insertItem(TR("Add classes of the selected class view"), 29);

                if (marked_not_yet_drawn(drawn))
                    m.insertItem(TR("Add marked elements"), 28);

                m.insertSeparator();
            }
        }
        else if (marked_not_yet_drawn(drawn)) {
            m.insertItem(TR("Add marked elements"), 28);
            m.insertSeparator();
        }

        switch (default_menu(m, 30)) {
        case EDIT_DRAWING_SETTING_CMD:
            ((BrowserClassDiagram *)((UmlCanvas *) canvas())->browser_diagram())->edit_settings();
            break;

        case RELOAD_CMD:
            // pure drawing modifications are lost
            // mark the diagram modified because the undid modifications
            // may be saved in the file are not saved in memory
            load("Class");
            window()->package_modified();
            break;

        case 28:
            add_marked_elements(p, drawn);
            break;

        case 29:
            add_classview_classes(bn, p, drawn);
            break;
        }
    }
    else
        (void) default_menu(m, 30);
}

static const int Diagram_Margin = 20;

void ClassDiagramView::add_classview_classes(BrowserNode * container, const QPoint & p,
        Q3PtrDict<DiagramItem> & drawn)
{
    QApplication::setOverrideCursor(Qt::waitCursor);

    history_save();
    history_protected = TRUE;

    int x = p.x();
    int y = p.y();
    int future_y = Diagram_Margin;

    add_classview_classes(container, drawn, x, y, future_y);

    canvas()->update();
    history_protected = FALSE;
    window()->package_modified();

    QApplication::restoreOverrideCursor();
}

void ClassDiagramView::add_classview_classes(BrowserNode * container,
        Q3PtrDict<DiagramItem> & drawn,
        int & x, int & y, int & future_y)
{
    Q3ListViewItem * child;
    int xmax = canvas()->width() - Diagram_Margin;
    int ymax = canvas()->height() - Diagram_Margin;

    for (child = container->firstChild(); child != 0; child = child->nextSibling()) {
        if (!((BrowserNode *) child)->deletedp() &&
            (((BrowserNode *) child)->get_type() == UmlClass)) {
            if (drawn[((BrowserNode *) child)->get_data()] == 0) {
                CdClassCanvas * cl =
                    new CdClassCanvas((BrowserNode *) child, the_canvas(), x, y);

                if ((x + cl->width()) > xmax)
                    cl->move(x = Diagram_Margin, y = future_y);

                if (y + cl->height() > ymax) {
                    cl->move(x = Diagram_Margin, y = Diagram_Margin);
                    future_y = y + cl->height() + Diagram_Margin;
                }
                else {
                    int bot = y + cl->height() + Diagram_Margin;

                    if (bot > future_y)
                        future_y = bot;
                }

                x = x + cl->width() + Diagram_Margin;

                cl->show();
                cl->upper();
            }

            add_classview_classes((BrowserNode *) child, drawn, x, y, future_y);
        }
    }
}

void ClassDiagramView::add_marked_elements(const QPoint & p,
        Q3PtrDict<DiagramItem> & drawn)
{
    QApplication::setOverrideCursor(Qt::waitCursor);

    history_save();
    history_protected = TRUE;

    int xmax = canvas()->width() - Diagram_Margin;
    int ymax = canvas()->height() - Diagram_Margin;
    int x = p.x();
    int y = p.y();
    int future_y = y;

    foreach (BrowserNode * bn, BrowserNode::marked_nodes()) {
        if (drawn[bn->get_data()] == 0) {
            DiagramCanvas * dc;

            switch (bn->get_type()) {
            case UmlClass:
                dc = new CdClassCanvas(bn, the_canvas(), x, y);
                break;

            case UmlPackage:
                dc = new PackageCanvas(bn, the_canvas(), x, y, 0);
                break;

            default:
                continue;
            }

            drawn.replace(dc->get_bn()->get_data(), dc);

            if ((x + dc->width()) > xmax)
                dc->move(x = Diagram_Margin, y = future_y);

            if (y + dc->height() > ymax) {
                dc->move(x = Diagram_Margin, y = Diagram_Margin);
                future_y = y + dc->height() + Diagram_Margin;
            }
            else {
                int bot = y + dc->height() + Diagram_Margin;

                if (bot > future_y)
                    future_y = bot;
            }

            x = x + dc->width() + Diagram_Margin;

            dc->show();
            dc->upper();
        }
    }

    UmlCanvas * cnv = (UmlCanvas *) canvas();

    if (! cnv->must_draw_all_relations()) {
        foreach (BrowserNode * bn, BrowserNode::marked_nodes()) {
            if (drawn[bn->get_data()] == 0) {
                UmlCode k = bn->get_type();

                if (IsaRelation(k))
                    RelationCanvas::drop(bn, cnv, drawn);
                else if (IsaSimpleRelation(k))
                    SimpleRelationCanvas::drop(bn, cnv, drawn);
            }
        }
    }

    canvas()->update();
    history_protected = FALSE;
    window()->package_modified();

    QApplication::restoreOverrideCursor();
}

void ClassDiagramView::add_related_elements(DiagramItem  * di, QString what,
        bool inh, bool assoc)
{
    BrowserNodeList l;
    RelatedElementsDialog dialog(di->get_bn(), what, inh, assoc, l);

    dialog.raise();

    if ((dialog.exec() == QDialog::Accepted) && !l.isEmpty()) {
        QApplication::setOverrideCursor(Qt::waitCursor);

        DiagramItemList items(canvas()->allItems());
        Q3PtrDict<DiagramItem> drawn;

        get_drawn(items, drawn);

        history_save();
        history_protected = TRUE;

        int xmax = canvas()->width() - Diagram_Margin;
        int ymax = canvas()->height() - Diagram_Margin;
        QRect re = di->rect();
        int x = re.x();
        int y = re.bottom() + Diagram_Margin;
        int future_y = y;

        foreach (BrowserNode *node, l) {
            if (drawn[node->get_data()] == 0) {
                DiagramCanvas * dc;

                switch (node->get_type()) {
                case UmlClass:
                    dc = new CdClassCanvas(node, the_canvas(), x, y);
                    break;

                case UmlPackage:
                    dc = new PackageCanvas(node, the_canvas(), x, y, 0);
                    break;

                default:
                    continue;
                }

                drawn.replace(dc->get_bn()->get_data(), dc);

                if ((x + dc->width()) > xmax)
                    dc->move(x = Diagram_Margin, y = future_y);

                if (y + dc->height() > ymax) {
                    dc->move(x = Diagram_Margin, y = Diagram_Margin);
                    future_y = y + dc->height() + Diagram_Margin;
                }
                else {
                    int bot = y + dc->height() + Diagram_Margin;

                    if (bot > future_y)
                        future_y = bot;
                }

                x = x + dc->width() + Diagram_Margin;

                dc->show();
                dc->upper();
            }
        }

        canvas()->update();
        history_protected = FALSE;
        window()->package_modified();

        QApplication::restoreOverrideCursor();
    }
}

void ClassDiagramView::contentsMousePressEvent(QMouseEvent * e)
{
    if (!window()->frozen()) {
        switch (window()->buttonOn()) {
        case UmlClass: {
            history_protected = TRUE;
            unselect_all();
            window()->selectOn();
            history_save();

            BrowserNode * parent =
                ((BrowserNode *) window()->browser_diagram()->parent());
            BrowserNode * b = BrowserClass::get_class(parent);

            if (b != 0) {
                CdClassCanvas * cl =
                    new CdClassCanvas(b, the_canvas(), e->x(), e->y());

                cl->show();
                cl->upper();
                window()->package_modified();
            }

            canvas()->update();
            history_protected = FALSE;
        }
        break;

        default:
            DiagramView::contentsMousePressEvent(e);
            break;
        }
    }
    else
        DiagramView::contentsMousePressEvent(e);
}

void ClassDiagramView::dragEnterEvent(QDragEnterEvent * e)
{
    if (!window()->frozen() &&
        (UmlDrag::canDecode(e, UmlClass, TRUE, TRUE) ||
         UmlDrag::canDecode(e, UmlRelations, TRUE, TRUE) ||
         UmlDrag::canDecode(e, UmlSimpleRelations, TRUE, TRUE) ||
         UmlDrag::canDecode(e, UmlPackage, FALSE, TRUE) ||
         UmlDrag::canDecode(e, UmlClassDiagram, FALSE, TRUE) ||
         UmlDrag::canDecode(e, UmlUseCaseDiagram, FALSE, TRUE) ||
         UmlDrag::canDecode(e, UmlSeqDiagram, FALSE, TRUE) ||
         UmlDrag::canDecode(e, UmlColDiagram, FALSE, TRUE) ||
         UmlDrag::canDecode(e, UmlObjectDiagram, FALSE, TRUE) ||
         UmlDrag::canDecode(e, UmlComponentDiagram, FALSE, TRUE) ||
         UmlDrag::canDecode(e, UmlDeploymentDiagram, FALSE, TRUE) ||
         UmlDrag::canDecode(e, UmlStateDiagram, TRUE, TRUE) ||
         UmlDrag::canDecode(e, UmlActivityDiagram, TRUE, TRUE)))
        e->accept();
    else
        e->ignore();
}

void ClassDiagramView::dropEvent(QDropEvent * e)
{
    BrowserNode * bn;
    QPoint p = viewportToContents(e->pos());

    if ((bn = UmlDrag::decode(e, UmlClass)) != 0) {
        history_save();

        CdClassCanvas * cl =
            new CdClassCanvas(bn, the_canvas(), p.x(), p.y());

        history_protected = TRUE;
        cl->show();
        cl->upper();
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
        history_protected = FALSE;
        window()->package_modified();
    }
    else if ((bn = UmlDrag::decode(e, UmlRelations, TRUE)) != 0) {
        history_save();

        history_protected = TRUE;
        RelationCanvas::drop(bn, the_canvas());
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

void ClassDiagramView::save(QTextStream & st, QString & warning,
                            bool copy) const
{
    DiagramItemList items(canvas()->allItems());

    if (!copy)
        // sort is useless for a copy
        items.sort();

    st << "format " << api_format() << "\n";

    // save first the classes packages fragment notes and icons

    foreach (DiagramItem * di, items) {
        switch (di->type()) {
        case UmlClass:
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

    foreach (DiagramItem * di, items) {
        if (!copy || di->copyable()) {
            UmlCode k = di->type();

            if (IsaRelation(k) || IsaSimpleRelation(k) || (k == UmlInner))
                di->save(st, FALSE, warning);
        }
    }

    // then saves anchors

    foreach (DiagramItem * di, items)
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

void ClassDiagramView::read(char * st, char * k)
{
    UmlCanvas * canvas = the_canvas();

    // reads first the classes package icons text notes and images
    while (CdClassCanvas::read(st, canvas, k) ||
           NoteCanvas::read(st, canvas, k) ||
           TextCanvas::read(st, canvas, k) ||
           IconCanvas::read(st, canvas, k) ||
           PackageCanvas::read(st, canvas, k) ||
           FragmentCanvas::read(st, canvas, k) ||
           ImageCanvas::read(st, canvas, k))
        k = read_keyword(st);

    // then reads relations and anchors
    while (ArrowCanvas::read(st, canvas, k) ||
           RelationCanvas::read(st, canvas, k) ||
           SimpleRelationCanvas::read(st, canvas, k))
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
