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

#ifndef UMLCLASS_H
#define UMLCLASS_H

#include <q3dict.h>
#include <q3ptrlist.h>
//Added by qt3to4:
#include "misc/mystr.h"
#include <QTextStream>
#include <Q3ValueList>
#include <Q3PtrList>

#include "UmlBaseClass.h"
#include "UmlTypeSpec.h"
#include "UmlFormalParameter.h"
#include "UmlActualParameter.h"

class QTextStream;

class UmlPackage;
class UmlClass;
class CppRefType;

class UmlClass : public UmlBaseClass
{
private:
    bool managed;

    static QList<UmlClass *> context;

    static Q3ValueList<UmlActualParameter> noactuals;

public:
    UmlClass(void * id, const WrapperStr & n)
        : UmlBaseClass(id, n), managed(FALSE) {};

    WrapperStr cpp_stereotype();

    void compute_dependencies(QList<CppRefType *> &, bool all_in_h);

    virtual void compute_dependency(QList<CppRefType *> & dependency,
                                    const WrapperStr & cl_stereotype,
                                    bool all_in_h);
    virtual void generate_decl(aVisibility & current_visibility,
                               QTextStream & f_h,
                               const WrapperStr & cl_stereotype,
                               WrapperStr indent,
                               BooL & first, bool last);
    void generate_def(QTextStream & f, WrapperStr indent, bool h);
    virtual void generate_def(QTextStream & f, WrapperStr indent, bool h,
                              WrapperStr templates, WrapperStr cl_names,
                              WrapperStr templates_tmplop,
                              WrapperStr cl_names_tmplop);

    virtual void generate();
    void generate_decl(QTextStream &, WrapperStr indent);

    void get_template_prefixes(WrapperStr & template1, WrapperStr & template2);
    void spec(WrapperStr & templates, WrapperStr & names,
              WrapperStr & templates_tmplop, WrapperStr & names_tmplop);
    WrapperStr decl();

    void write(QTextStream &, bool with_formals = TRUE,
               BooL * is_template = 0,
               const Q3ValueList<UmlActualParameter> & actuals = noactuals);
    static void write(QTextStream &, const UmlTypeSpec &,
                      bool with_formals = TRUE,
                      BooL * is_template = 0);
};

#endif
