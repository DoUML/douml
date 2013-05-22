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

#ifndef UMLCLASSMEMBER_H
#define UMLCLASSMEMBER_H

#include "UmlBaseClassMember.h"
//Added by qt3to4:
#include "misc/mystr.h"
#include <QTextStream>
#include <Q3PtrList>

// This class globaly manages class's operations, relations, attributes
// and sub-classes
// You can modify it as you want (except the constructor)

class UmlClassMember : public UmlBaseClassMember
{
public:
    UmlClassMember(void * id, const WrapperStr & n)
        : UmlBaseClassMember(id, n) {};

    static void remove_comments(WrapperStr & s);
    static void remove_arrays(WrapperStr & s);
    static void remove_preprocessor(WrapperStr & s);
    static bool compute_dependency(QList<CppRefType *> & dependency,
                                   WrapperStr decl, const UmlTypeSpec &,
                                   bool force_incl = FALSE);

    void generate_visibility(aVisibility & current_visibility,
                             QTextStream & f_h, bool ahead,
                             const WrapperStr & indent);
    bool insert_template(const char *& p, QTextStream & fs,
                         const WrapperStr & indent, const WrapperStr & templ);
};

#endif
