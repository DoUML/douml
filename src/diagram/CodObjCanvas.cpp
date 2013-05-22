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





#include "CodObjCanvas.h"
#include "CodLinkCanvas.h"
#include "CodSelfLinkCanvas.h"
#include "CodDirsCanvas.h"
#include "ColMsg.h"
#include "CodClassInstCanvas.h"
#include "myio.h"
#include "translate.h"

CodObjCanvas::CodObjCanvas(BrowserNode * bn, UmlCanvas * canvas,
                           int x, int y, int we, int he, int id)
    : DiagramCanvas(bn, canvas, x, y, we, he, id), self_link(0)
{
}

CodObjCanvas::~CodObjCanvas()
{
}

void CodObjCanvas::delete_it()
{
    if (self_link)
        self_link->delete_it();

    DiagramCanvas::delete_it();
}

void CodObjCanvas::delete_available(BooL &, BooL & out_model) const
{
    out_model |= TRUE;
}

BrowserClass * CodObjCanvas::get_class() const
{
    return 0;
}

void CodObjCanvas::change_scale()
{
    DiagramCanvas::change_scale();
}

void CodObjCanvas::moveBy(double dx, double dy)
{
    DiagramCanvas::moveBy(dx, dy);

    if (self_link) {
        if (self_link->selected())
            the_canvas()->unselect(self_link);

        self_link->update_pos();
    }
}

void CodObjCanvas::set_z(double z)
{
    setZ(z);

    if (self_link)
        self_link->setZ(z - 0.5);
}

QString CodObjCanvas::may_start(UmlCode & l) const
{
    return ((l != UmlSelfLink) || (self_link == 0)) ? QString() : TR("illegal");
}

QString CodObjCanvas::may_connect(UmlCode & l, const DiagramItem * dest) const
{
    if (l == UmlAnchor)
        return dest->may_start(l);

    switch (dest->type()) {
    case UmlClass:
    case UmlClassInstance:
        return (l == UmlLink) ? QString() : TR("illegal");

    default:
        return TR("illegal");
    }
}

void CodObjCanvas::connexion(UmlCode l, DiagramItem * dest,
                             const QPoint & s, const QPoint & e)
{
    switch (l) {
    case UmlSelfLink:
        self_link = new CodSelfLinkCanvas(the_canvas(), this, s, 0);
        self_link->show();
        the_canvas()->select(self_link);
        break;

    case UmlAnchor:
        DiagramCanvas::connexion(l, dest, s, e);
        break;

    default:
        if (dest != this) {
            ArrowCanvas * a =
                new CodLinkCanvas(the_canvas(), this, dest, 0, -1.0, -1.0);

            a->show();
            the_canvas()->select(a);
        }
    }
}

void CodObjCanvas::get_all_in_all_out(ColMsgList & all_in, ColMsgList & all_out) const
{
    if (self_link) {
        ColMsg::get_all_in_all_out(all_in, all_out, self_link->get_msgs());
        // all_out still empty equals all_in
        all_out = all_in;
    }

    foreach (ArrowCanvas *canvas, lines) {
        CodDirsCanvas * dirs;

        if ((canvas->type() == UmlLink) &&
            ((dirs = ((CodLinkCanvas *) canvas)->find_dirs()) != 0)) {
            CodObjCanvas * from;
            CodObjCanvas * to;

            ((CodLinkCanvas *) canvas)->get_start_end(from, to);

            if (this == to)
                ColMsg::get_all_in_all_out(all_in, all_out, dirs->get_msgs());
            else
                ColMsg::get_all_in_all_out(all_out, all_in, dirs->get_msgs());
        }
    }
}

CodObjCanvas * CodObjCanvas::read(char *& st, UmlCanvas * canvas)
{
    char * k = read_keyword(st);

    return CodClassInstCanvas::read(st, canvas, k);
}
