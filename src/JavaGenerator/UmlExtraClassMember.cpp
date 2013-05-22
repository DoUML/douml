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

#include <QTextStream>
//Added by qt3to4:
#include "misc/mystr.h"
#include <QTextStream>

#include "UmlExtraClassMember.h"

/*
void UmlExtraClassMember::compute_dependency(QList<JavaRefType *> &, WrapperStr) {
  // does nothing
}
*/

void UmlExtraClassMember::generate(QTextStream & f, const WrapperStr &,
                                   WrapperStr)
{
    f << javaDecl();
}

void UmlExtraClassMember::generate_enum_pattern_item(QTextStream & f, int &,
        const WrapperStr &,
        WrapperStr)
{
    f << javaDecl();
}

void UmlExtraClassMember::generate_enum_pattern_case(QTextStream & f,
        WrapperStr)
{
    f << javaDecl();
}

void UmlExtraClassMember::generate_enum_member(QTextStream & f, WrapperStr)
{
    f << javaDecl();
}
