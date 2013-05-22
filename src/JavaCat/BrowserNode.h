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

#ifndef BROWSERNODE_H
#define BROWSERNODE_H

#include "TreeItem.h"
//Added by qt3to4:
#include "misc/mystr.h"
#include <Q3PtrCollection>
#include <Q3PtrList>

#ifndef REVERSE
class BrowserView;
#endif

class WrapperStr;
class QDataStream;
class Class;

class BrowserNode : public TreeItem
{
public:
    BrowserNode(BrowserNode * parent, const char * n);
    virtual ~BrowserNode() {};	// to avoid compiler warning

    virtual bool isa_package() const = 0;
    virtual void declare(const WrapperStr &, Class *) = 0;
#ifdef ROUNDTRIP
    virtual Class * localy_defined(QString name) const = 0;
#endif

#ifndef REVERSE
    BrowserNode(BrowserView * parent, const char * n);

    void select_in_browser();

    virtual void activated();
    virtual void selected();
    virtual void menu() = 0;
    virtual void refer(const QString & name) = 0;
    virtual void backup(QDataStream  & dt) const = 0;
    virtual QString get_path() const = 0;
#endif
};


#ifndef REVERSE

// a sortable list of BrowserNode

#include <q3ptrlist.h>

class BrowserNodeList : public QList<BrowserNode *>
{
public:
    void search(BrowserNode * bn, int k, const QString & s, bool cs);
    static bool lessThan(BrowserNode *a, BrowserNode *b);
    void sort();
};

#endif

#endif
