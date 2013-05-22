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

#ifndef UMLATTRIBUTE_H
#define UMLATTRIBUTE_H

#include "UmlBaseAttribute.h"
//Added by qt3to4:
#include "misc/mystr.h"
#include <Q3PtrList>

class Class;

class UmlAttribute : public UmlBaseAttribute
{
public:
    UmlAttribute(void * id, const WrapperStr & n)
        :  UmlBaseAttribute(id, n) {};

    static bool new_one(Class * cl, const WrapperStr & name, const WrapperStr & type,
                        const WrapperStr & modifier, const WrapperStr & pretype,
                        const WrapperStr & array,	aVisibility visibility,
                        bool staticp, bool constp, bool typenamep, bool mutablep,
                        bool volatilep,	const WrapperStr & bitfield,
                        const WrapperStr & value,	WrapperStr comment,
                        WrapperStr description
#ifdef ROUNDTRIP
                        , bool roundtrip, QList<UmlItem *> & expected_order
#endif
                       );
#ifdef ROUNDTRIP
    static UmlAttribute * search_attr(UmlClass * cl, const WrapperStr & name);
#endif
};

#endif
