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

#include "ComponentDiagramWindow.h"
#include "ComponentDiagramView.h"
#include "ComponentCanvas.h"
#include "BrowserComponentDiagram.h"
#include "PackageCanvas.h"
#include "FragmentCanvas.h"
#include "SimpleRelationCanvas.h"
#include "IconCanvas.h"
#include "NoteCanvas.h"
#include "TextCanvas.h"
#include "ImageCanvas.h"
#include "ArrowJunctionCanvas.h"
#include "UmlPixmap.h"
#include "UmlDrag.h"
#include "BrowserComponent.h"
#include "ui/menufactory.h"
#include "myio.h"
#include "BrowserView.h"
#include "RelatedElementsDialog.h"
#include "translate.h"

ComponentDiagramView::ComponentDiagramView(QWidget * parent, UmlCanvas * canvas, int id)
    : DiagramView(parent, canvas, id)
{
    load("Component");
}

// have marked elements not yet drawn ?
static bool marked_not_yet_drawn(Q3PtrDict<DiagramItem> & drawn)
{
    foreach (BrowserNode *bn, BrowserNode::marked_nodes()) {
        UmlCode k = bn->get_type();

        switch (k) {
        default:
            if (!IsaSimpleRelation(k))
                break;

            // no break
        case UmlComponent:
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
    foreach (DiagramItem *di, items) {
        UmlCode k = di->type();

        switch (k) {
        case UmlComponent:
        case UmlPackage:
            drawn.replace(di->get_bn()->get_data(), di);
            break;

        default:
            if (IsaSimpleRelation(k))
                drawn.replace(((ArrowCanvas *) di)->get_data(), di);
        }
    }
}

void ComponentDiagramView::menu(const QPoint & p)
{
    Q3PopupMenu m(0);

    MenuFactory::createTitle(m, TR("Component diagram menu"));

    if ((((UmlCanvas *) canvas())->browser_diagram())->is_writable()) {
        DiagramItemList items(canvas()->allItems());
        Q3PtrDict<DiagramItem> drawn;

        get_drawn(items, drawn);

        if (marked_not_yet_drawn(drawn))
            m.insertItem(TR("Add marked elements"), 28);

        switch (default_menu(m, 30)) {
        case EDIT_DRAWING_SETTING_CMD:
            ((BrowserComponentDiagram *) the_canvas()->browser_diagram())->edit_settings();
            break;

        case RELOAD_CMD:
            // pure drawing modifications are lost
            // mark the diagram modified because the undid modifications
            // may be saved in the file are not saved in memory
            load("Component");
            window()->package_modified();
            break;

        case 28:
            add_marked_elements(p, drawn);
            break;
        }
    }
    else
        (void) default_menu(m, 30);
}

static const int Diagram_Margin = 20;

void ComponentDiagramView::add_marked_elements(const QPoint & p,
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

    foreach (BrowserNode *bn, BrowserNode::marked_nodes()) {
        if (drawn[bn->get_data()] == 0) {
            DiagramCanvas * dc;

            switch (bn->get_type()) {
            case UmlComponent:
                dc = new ComponentCanvas(bn, the_canvas(), x, y);
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
        foreach (BrowserNode *bn, BrowserNode::marked_nodes()) {
            if ((drawn[bn->get_data()] == 0) &&
                IsaSimpleRelation(bn->get_type()))
                SimpleRelationCanvas::drop(bn, cnv, drawn);
        }
    }

    canvas()->update();
    history_protected = FALSE;
    window()->package_modified();

    QApplication::restoreOverrideCursor();
}

void ComponentDiagramView::add_related_elements(DiagramItem  * di, QString what,
        bool inh, bool assoc)
{
    BrowserNodeList l;
    RelatedElementsDialog dialog(di->get_bn(), what, inh, assoc, l);

    dialog.raise();

    if (dialog.exec() == QDialog::Accepted) {
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
        foreach (BrowserNode *bn, l) {
            if (drawn[bn->get_data()] == 0) {
                DiagramCanvas * dc;

                switch (bn->get_type()) {
                case UmlComponent:
                    dc = new ComponentCanvas(bn, the_canvas(), x, y);
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

        canvas()->update();
        history_protected = FALSE;
        window()->package_modified();

        QApplication::restoreOverrideCursor();
    }
}

void ComponentDiagramView::contentsMousePressEvent(QMouseEvent * e)
{
    if (!window()->frozen()) {
        UmlCode action = window()->buttonOn();

        switch (action) {
        case UmlComponent: {
            history_protected = TRUE;
            unselect_all();
            window()->selectOn();
            history_save();

            BrowserNode * b =
                BrowserComponent::get_component(((BrowserNode *) window()->browser_diagram()->parent()));

            if (b != 0) {
                ComponentCanvas * c =
                    new ComponentCanvas(b, the_canvas(), e->x(), e->y());

                c->show();
                c->upper();
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

void ComponentDiagramView::dragEnterEvent(QDragEnterEvent * e)
{
    if (!window()->frozen() &&
        (UmlDrag::canDecode(e, UmlComponent, FALSE, TRUE) ||
         UmlDrag::canDecode(e, UmlPackage, FALSE, TRUE) ||
         UmlDrag::canDecode(e, UmlSimpleRelations, TRUE, TRUE) ||
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

void ComponentDiagramView::dropEvent(QDropEvent * e)
{
    BrowserNode * bn;
    QPoint p = viewportToContents(e->pos());

    if ((bn = UmlDrag::decode(e, UmlComponent)) != 0) {
        history_save();

        ComponentCanvas * c =
            new ComponentCanvas(bn, the_canvas(), p.x(), p.y());

        history_protected = TRUE;
        c->show();
        c->upper();
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

void ComponentDiagramView::save(QTextStream & st, QString & warning,
                                bool copy) const
{
    DiagramItemList items(canvas()->allItems());

    if (!copy)
        // sort is useless for a copy
        items.sort();

    st << "format " << api_format() << "\n";

    // save first component packages fragment notes junctions and icons

    foreach (DiagramItem *di, items) {
        switch (di->type()) {
        case UmlComponent:
        case UmlNote:
        case UmlText:
        case UmlImage:
        case UmlPackage:
        case UmlFragment:
        case UmlArrowJunction:
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
        switch (di->type()) {
        case UmlInherit:
        case UmlDependency:
        case UmlDependOn:
        case UmlRequired:
        case UmlProvided:
            if (!copy || di->copyable())
                di->save(st, FALSE, warning);

        default:	// to avoid compiler warning
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

void ComponentDiagramView::read(char * st, char * k)
{
    UmlCanvas * canvas = the_canvas();

    // reads first component package icons notes junctions text and images
    while (ComponentCanvas::read(st, canvas, k) ||
           NoteCanvas::read(st, canvas, k) ||
           TextCanvas::read(st, canvas, k) ||
           IconCanvas::read(st, canvas, k) ||
           PackageCanvas::read(st, canvas, k) ||
           FragmentCanvas::read(st, canvas, k) ||
           ArrowJunctionCanvas::read(st, canvas, k) ||
           ImageCanvas::read(st, canvas, k))
        k = read_keyword(st);

    // then reads relations and anchors
    ArrowCanvas * a;

    while (((a = ArrowCanvas::read(st, canvas, k)) != 0) ||
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


