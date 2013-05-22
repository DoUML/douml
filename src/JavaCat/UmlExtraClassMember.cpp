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

#include "UmlExtraClassMember.h"

#ifdef ROUNDTRIP
#include "Class.h"
#include "UmlClass.h"
//Added by qt3to4:
#include "misc/mystr.h"
//Added by qt3to4:
#include <Q3PtrList>

// an empty string is != a null string in Qt !
//bool inline neq(const WrapperStr & s1, const WrapperStr & s2)
//{
//    return (s1.isEmpty()) ? !s2.isEmpty() : (s1 != s2);
//}

void UmlExtraClassMember::add_init(UmlClass * cl, WrapperStr def, bool roundtrip,
                                   QList<UmlItem *> & expected_order)
{
    if (roundtrip) {
        const Q3PtrVector<UmlItem> & ch = cl->children();
        UmlItem ** v = ch.data();
        UmlItem ** const vsup = v + ch.size();
        UmlItem * x;

        for (; v != vsup; v += 1) {
            if (((x = *v)->kind() == anExtraClassMember) &&
                ((UmlExtraClassMember *) x)->is_useless() &&
                (x->name() == "initialization")) {
                expected_order.append(x);

                if (neq(((UmlExtraClassMember *) x)->javaDecl(), def)) {
                    ((UmlExtraClassMember *) x)->set_JavaDecl(def);
                    cl->get_class()->set_updated();
                }

                ((UmlExtraClassMember *) x)->set_usefull();
                return;
            }
        }
    }

    UmlExtraClassMember * x =
        UmlExtraClassMember::create(cl, "initialization");

    x->set_JavaDecl(def);
    expected_order.append(x);
}

#endif
