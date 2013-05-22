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

#include "UseCaseDiagramWindow.h"
#include "UseCaseDiagramView.h"
#include "BrowserUseCaseDiagram.h"
#include "DiagramCanvas.h"
#include "UcUseCaseCanvas.h"
#include "PackageCanvas.h"
#include "FragmentCanvas.h"
#include "SubjectCanvas.h"
#include "UcClassCanvas.h"
#include "IconCanvas.h"
#include "RelationCanvas.h"
#include "SimpleRelationCanvas.h"
#include "NoteCanvas.h"
#include "TextCanvas.h"
#include "ImageCanvas.h"
#include "UmlPixmap.h"
#include "UmlDrag.h"
#include "BrowserClass.h"
#include "BrowserUseCase.h"
#include "ui/menufactory.h"
#include "myio.h"
#include "BrowserView.h"
#include "RelatedElementsDialog.h"
#include "translate.h"

UseCaseDiagramView::UseCaseDiagramView(QWidget * parent, UmlCanvas * canvas, int id)
    : DiagramView(parent, canvas, id)
{
    load("Use Case");
}

// have marked elements not yet drawn ?
static bool marked_not_yet_drawn(Q3PtrDict<DiagramItem> & drawn)
{
    const QList<BrowserNode *> & l = BrowserNode::marked_nodes();
    foreach (BrowserNode *bn, l) {
        UmlCode k = bn->get_type();

        switch (k) {
        default:
            if (!IsaRelation(k) && !IsaSimpleRelation(k))
                break;

            // no break
        case UmlClass:
        case UmlPackage:
        case UmlUseCase:
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
        case UmlClass:
        case UmlPackage:
        case UmlUseCase:
            drawn.replace(di->get_bn()->get_data(), di);
            break;

        default:
            if (IsaRelation(k) || IsaSimpleRelation(k))
                drawn.replace(((ArrowCanvas *) di)->get_data(), di);
        }
    }
}

void UseCaseDiagramView::menu(const QPoint & p)
{
    Q3PopupMenu m(0);

    MenuFactory::createTitle(m, TR("Use case diagram menu"));

    if ((((UmlCanvas *) canvas())->browser_diagram())->is_writable()) {
        DiagramItemList items(canvas()->allItems());
        Q3PtrDict<DiagramItem> drawn;

        get_drawn(items, drawn);

        if (marked_not_yet_drawn(drawn))
            m.insertItem(TR("Add marked elements"), 28);

        switch (default_menu(m, 30)) {
        case EDIT_DRAWING_SETTING_CMD:
            ((BrowserUseCaseDiagram *) the_canvas()->browser_diagram())->edit_settings();
            break;

        case RELOAD_CMD:
            // pure drawing modifications are lost
            // mark the diagram modified because the undid modifications
            // may be saved in the file are not saved in memory
            load("Use Case");
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

const int Diagram_Margin = 20;

void UseCaseDiagramView::add_marked_elements(const QPoint & p,
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
    const QList<BrowserNode *> & l = BrowserNode::marked_nodes();

    foreach (BrowserNode *bn, l) {
        if (drawn[bn->get_data()] == 0) {
            DiagramCanvas * dc;

            switch (bn->get_type()) {
            case UmlClass:
                dc = new UcClassCanvas(bn, the_canvas(), x, y, 0);
                break;

            case UmlPackage:
                dc = new PackageCanvas(bn, the_canvas(), x, y, 0);
                break;

            case UmlUseCase:
                dc = new UcUseCaseCanvas(bn, the_canvas(), x, y, 0);
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
        foreach (BrowserNode *bn, l) {
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

void UseCaseDiagramView::add_related_elements(DiagramItem  * di, QString what,
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
                case UmlClass:
                    dc = new UcClassCanvas(bn, the_canvas(), x, y, 0);
                    break;

                case UmlPackage:
                    dc = new PackageCanvas(bn, the_canvas(), x, y, 0);
                    break;

                case UmlUseCase:
                    dc = new UcUseCaseCanvas(bn, the_canvas(), x, y, 0);
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

void UseCaseDiagramView::contentsMousePressEvent(QMouseEvent * e)
{
    if (!window()->frozen()) {
        UmlCode action = window()->buttonOn();

        switch (action) {
        case UmlClass: {
            history_protected = TRUE;
            unselect_all();
            window()->selectOn();
            history_save();

            BrowserNode * b =
                BrowserClass::get_class(((BrowserNode *) window()->browser_diagram()->parent()),
                                        "actor");

            if (b != 0) {
                UcClassCanvas * uc =
                    new UcClassCanvas(b, the_canvas(), e->x(), e->y(), 0);

                uc->show();
                uc->upper();
                window()->package_modified();
            }
        }
        break;

        case UmlUseCase: {
            history_protected = TRUE;
            unselect_all();
            window()->selectOn();
            history_save();

            BrowserNode * b =
                BrowserUseCase::get_use_case(((BrowserNode *) window()->browser_diagram()->parent()));

            if (b != 0) {
                UcUseCaseCanvas * uc =
                    new UcUseCaseCanvas(b, the_canvas(), e->x(), e->y(), 0);
                uc->show();
                uc->upper();
                window()->package_modified();
            }
        }
        break;

        case UmlSubject: {
            unselect_all();
            window()->selectOn();
            history_save();

            SubjectCanvas * f =
                new SubjectCanvas(the_canvas(), e->x(), e->y(), 0);

            f->show();
            f->upper();
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

void UseCaseDiagramView::dragEnterEvent(QDragEnterEvent * e)
{
    abort_line_construction();

    if (!window()->frozen() &&
        (UmlDrag::canDecode(e, UmlClass, TRUE, TRUE) ||
         UmlDrag::canDecode(e, UmlUseCase, FALSE, TRUE) ||
         UmlDrag::canDecode(e, UmlPackage, FALSE, TRUE) ||
         UmlDrag::canDecode(e, UmlRelations, TRUE, TRUE) ||
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

void UseCaseDiagramView::dropEvent(QDropEvent * e)
{
    BrowserNode * bn;
    QPoint p = viewportToContents(e->pos());

    if ((bn = UmlDrag::decode(e, UmlClass)) != 0) {
        history_save();

        UcClassCanvas * uc =
            new UcClassCanvas(bn, the_canvas(), p.x(), p.y(), 0);

        history_protected = TRUE;
        uc->show();
        uc->upper();
        canvas()->update();
        history_protected = FALSE;
        window()->package_modified();
    }
    else if ((bn = UmlDrag::decode(e, UmlUseCase)) != 0) {
        history_save();

        UcUseCaseCanvas * uc =
            new UcUseCaseCanvas(bn, the_canvas(), p.x(), p.y(), 0);

        history_protected = TRUE;
        uc->show();
        uc->upper();
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

void UseCaseDiagramView::save(QTextStream & st, QString & warning,
                              bool copy) const
{
    DiagramItemList items(canvas()->allItems());

    if (!copy)
        // sort is useless for a copy
        items.sort();

    st << "format " << api_format() << "\n";

    // save first class use_cases packages fragment subject notes and icons

    foreach (DiagramItem *di, items) {
        switch (di->type()) {
        case UmlClass:
        case UmlUseCase:
        case UmlNote:
        case UmlText:
        case UmlImage:
        case UmlPackage:
        case UmlFragment:
        case UmlSubject:
        case UmlIcon:
            if (!copy || di->copyable())
                di->save(st, FALSE, warning);

            // no break
        default:
            break;
        }
    }

    // then saves relations

    foreach (DiagramItem *di, items)
        if ((!copy || di->copyable()) &&
            (IsaRelation(di->type()) || IsaSimpleRelation(di->type())))
            di->save(st, FALSE, warning);

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

void UseCaseDiagramView::read(char * st, char * k)
{
    UmlCanvas * canvas = the_canvas();

    // reads first the class use_cases package icons, notes text and image
    while (UcClassCanvas::read(st, canvas, k) ||
           UcUseCaseCanvas::read(st, canvas, k) ||
           NoteCanvas::read(st, canvas, k) ||
           TextCanvas::read(st, canvas, k) ||
           IconCanvas::read(st, canvas, k) ||
           PackageCanvas::read(st, canvas, k) ||
           FragmentCanvas::read(st, canvas, k) ||
           SubjectCanvas::read(st, canvas, k) ||
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

// for plug-out

void UseCaseDiagramView::send(ToolCom * com)
{
    Q3CanvasItemList l = canvas()->allItems();
    QList<FragmentCanvas *> fragments;
    QList<FragmentCanvas *> refs;

    FragmentCanvas::send(com, l, fragments, refs);
    SubjectCanvas::send(com, l);
    UcUseCaseCanvas::send(com, l);
}

