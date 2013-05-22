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





#include <q3popupmenu.h>
#include <qcursor.h>
#include <qpainter.h>
//Added by qt3to4:
#include <QTextStream>

#include "ArrowPointCanvas.h"
#include "ArrowCanvas.h"
#include "RelationCanvas.h"
#include "SimpleRelationCanvas.h"
#include "TransitionCanvas.h"
#include "FlowCanvas.h"
#include "CodLinkCanvas.h"
#include "ObjectLinkCanvas.h"
#include "AssocContainCanvas.h"
#include "UmlCanvas.h"
#include "myio.h"
#include "ui/menufactory.h"
#include "BrowserDiagram.h"
#include "DiagramView.h"
#include "translate.h"

ArrowPointCanvas::ArrowPointCanvas(UmlCanvas * canvas, int x, int y)
    : DiagramCanvas(0, canvas, x, y, ARROW_POINT_SIZE, ARROW_POINT_SIZE, -1)
{
    browser_node = canvas->browser_diagram();
}

ArrowPointCanvas::~ArrowPointCanvas()
{
}

void ArrowPointCanvas::delete_it()
{
    if (lines.isEmpty())
        the_canvas()->del(this);
    else
        lines.first()->delete_it();	// will apply del on this
}

void ArrowPointCanvas::delete_available(BooL &,
                                        BooL & out_model) const
{
    out_model |= lines.first()->may_join();
}

// not produced in SVG file

void ArrowPointCanvas::change_scale()
{
    // the size is not modified
    Q3CanvasRectangle::setVisible(FALSE);
    recenter();
    Q3CanvasRectangle::setVisible(TRUE);
}

void ArrowPointCanvas::draw(QPainter & p)
{
    if (! visible()) return;

    QRect r = rect();

    r.setX(r.x() + 1);
    r.setY(r.y() + 1);
    r.setWidth(r.width() - 1);
    r.setHeight(r.height() - 1);

    p.fillRect(r, ::Qt::black);

    if (selected())
        show_mark(p, rect());
}

UmlCode ArrowPointCanvas::type() const
{
    return UmlArrowPoint;
}

void ArrowPointCanvas::connexion(UmlCode action, DiagramItem * dest,
                                 const QPoint &, const QPoint &)
{
    ArrowCanvas * a;

    switch (the_canvas()->browser_diagram()->get_type()) {
    case UmlClassDiagram:
        if (IsaRelation(action)) {
            a = new RelationCanvas(the_canvas(), this, dest, 0, action, 0, -1.0, -1.0);
            a->show();
            the_canvas()->select(a);
            return;
        }

        break;

    case UmlColDiagram:
        if (action == UmlLink) {
            a = new CodLinkCanvas(the_canvas(), this, dest, 0, -1.0, -1.0);
            a->show();
            the_canvas()->select(a);
            return;
        }

        break;

    case UmlObjectDiagram:
        if (action == UmlObjectLink) {
            a = new ObjectLinkCanvas(the_canvas(), this, dest, UmlObjectLink, 0, -1.0, -1.0);
            a->show();
            the_canvas()->select(a);
            return;
        }

        break;

    case UmlDeploymentDiagram:
    case UmlComponentDiagram:
        if (action == UmlContain) {
            a = new AssocContainCanvas(the_canvas(), this, dest, 0, -1.0, -1.0);
            a->show();
            the_canvas()->select(a);
            return;
        }

    case UmlStateDiagram:
        if (action == UmlTransition) {
            a = new TransitionCanvas(the_canvas(), this, dest, 0, 0, -1.0, -1.0);
            a->show();
            the_canvas()->select(a);
            return;
        }

    case UmlActivityDiagram:
        if (action == UmlFlow) {
            a = new FlowCanvas(the_canvas(), this, dest, 0, 0, -1.0, -1.0);
            a->show();
            the_canvas()->select(a);
            return;
        }

    default:	// to avoid warning
        break;
    }

    if (IsaSimpleRelation(action))
        a = new SimpleRelationCanvas(the_canvas(), this, dest, 0, action, 0, -1.0, -1.0);
    else
        a = new ArrowCanvas(the_canvas(), this, dest, action, 0, FALSE, -1.0, -1.0);

    a->show();
    the_canvas()->select(a);
}

void ArrowPointCanvas::open()
{
    if (lines.at(0)->may_join()) {
        DiagramView * view = the_canvas()->get_view();

        view->history_save();
        view->protect_history(TRUE);
        lines.at(0)->join(lines.at(1), this);
        canvas()->update();
        view->protect_history(FALSE);
        package_modified();
    }
}

void ArrowPointCanvas::menu(const QPoint &)
{
    Q3PopupMenu m;

    MenuFactory::createTitle(m, TR("Line break"));
    m.insertSeparator();
    m.insertItem(TR("Remove from diagram"), 0);
    m.setItemEnabled(0, lines.at(0)->may_join());

    switch (m.exec(QCursor::pos())) {
    case 0:
        // removes the point replacing the lines around it by a single line
        lines.at(0)->join(lines.at(1), this);
        break;

    default:
        return;
    }

    package_modified();
}

QString ArrowPointCanvas::may_start(UmlCode &) const
{
    return TR("illegal");
}

QString ArrowPointCanvas::may_connect(UmlCode &, const DiagramItem *) const
{
    return TR("illegal");
}

bool ArrowPointCanvas::alignable() const
{
    return TRUE;
}

void ArrowPointCanvas::prepare_for_move(bool)
{
    // defined to do nothing, else in case the point is
    // part of a self relation this last and the element defining
    // the self relation move with the point
}

ArrowCanvas * ArrowPointCanvas::get_other(const ArrowCanvas * l) const
{
    return (lines.first() == l) ? lines.last() : lines.first();
}

void ArrowPointCanvas::save(QTextStream & st, bool, QString &) const
{
    save_xy(st, this, "point");
}

ArrowPointCanvas * ArrowPointCanvas::read(char *& st, UmlCanvas * canvas,
        char * k)
{
    if (strcmp(k, "point"))
        return 0;

    int x = (int) read_double(st);

    ArrowPointCanvas * result =
        new ArrowPointCanvas(canvas, x, (int) read_double(st));

    result->show();

    return result;
}

