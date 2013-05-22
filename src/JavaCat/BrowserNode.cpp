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

#include "BrowserNode.h"
#include "UmlClass.h"
#include "Class.h"
#include "aRelationKind.h"
#include "aDirection.h"
#include "aVisibility.h"
#include "anItemKind.h"
#include "BrowserView.h"
//Added by qt3to4:
#include <Q3PtrCollection>

BrowserNode::BrowserNode(BrowserView * parent, const char * n)
    : TreeItem(parent, n)
{
}

BrowserNode::BrowserNode(BrowserNode * parent, const char * n)
    : TreeItem(parent, n)
{
    // move it at end
    TreeItem * child = parent->firstChild();

    while (child->nextSibling())
        child = child->nextSibling();

    if (child != this)
        moveItem(child);
}

void BrowserNode::activated()
{
}

void BrowserNode::selected()
{
}

//

void BrowserNodeList::search(BrowserNode * bn, int k,
                             const QString & s, bool cs)
{
    TreeItem * child;

    for (child = bn->firstChild(); child != 0; child = child->nextSibling()) {
        if ((k == aPackage)
            ? (((BrowserNode *) child)->isa_package() &&
               (child->text(0).find(s, 0, cs) != -1))
            : (!((BrowserNode *) child)->isa_package() &&
               ((k == aClass)
                ? (child->text(0).find(s, 0, cs) != -1)
                : (WrapperStr(((Class *) child)->get_description()).operator QString().find(s, 0, cs) != -1))))
            append((BrowserNode *) child);

        search((BrowserNode *) child, k, s, cs);
    }
}

bool BrowserNodeList::lessThan(BrowserNode *a, BrowserNode *b)
{
    return a->text(0) < b->text(0);
}

void BrowserNodeList::sort()
{
    qSort(begin(), end(), lessThan);
}
