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

#ifndef CPPREFTYPE_H
#define CPPREFTYPE_H

#include <q3ptrlist.h>
//Added by qt3to4:
#include "misc/mystr.h"
#include <QTextStream>
#include <Q3PtrList>

#include "UmlTypeSpec.h"

class QTextStream;
class UmlArtifact;

class CppRefType
{
public:
    enum Weight { Low, Medium, High, Strong };

    static bool add(UmlClass *, QList<CppRefType *> &, bool incl, bool hight = FALSE);
    static bool add(const WrapperStr &, QList<CppRefType *> &, bool incl);
    static bool add(const UmlTypeSpec & t, QList<CppRefType *> & l, bool incl);
    static void remove(UmlClass *, QList<CppRefType *> & l);
    static void remove(const WrapperStr &, QList<CppRefType *> & l);
    static void force_ref(UmlClass * cl, QList<CppRefType *> & l);
    static void compute(QList<CppRefType *> & dependencies,
                        const WrapperStr & hdef, const WrapperStr & srcdef,
                        WrapperStr & h_incl,  WrapperStr & decl, WrapperStr & src_incl,
                        UmlArtifact * who);

protected:
    UmlTypeSpec type;
    bool included;
    Weight weight;

    CppRefType(UmlClass * cl, bool i, Weight w)
        : included(i), weight(w) {
        type.type = cl;
    };
    CppRefType(const WrapperStr & t, bool i, Weight w)
        : included(i), weight(w) {
        type.explicit_type = t;
    };

};

#endif
