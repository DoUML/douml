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

#ifndef CLASSCONTAINER
#define CLASSCONTAINER

#include "UmlClass.h"
#include "Namespace.h"
//Added by qt3to4:
#include "misc/mystr.h"
#include <Q3ValueList>
#include <Q3PtrList>

class Class;
class UmlTypeSpec;

class ClassContainer
{
public:
    virtual ~ClassContainer();	// just to not have warning

    virtual Class * declare_if_needed(const WrapperStr & name,
                                      WrapperStr stereotype = 0) = 0;
    virtual Class * define(const WrapperStr & name, WrapperStr stereotype = 0) = 0;
    virtual void declare_if_needed(WrapperStr name, Class * cl) = 0;
    virtual void define(WrapperStr name, Class * cl) = 0;
    virtual Class * new_class(const WrapperStr & name,
                              const WrapperStr & stereotype,
                              bool declaration) = 0;
    virtual bool find_type(WrapperStr type, UmlTypeSpec & typespec) = 0;
    virtual void declaration(const WrapperStr & name, const WrapperStr & stereotype,
                             const WrapperStr & decl
#ifdef ROUNDTRIP
                             , bool roundtrip, QList<UmlItem *> & expected_order
#endif
                            ) = 0;

    void compute_type(WrapperStr type, UmlTypeSpec & typespec,
                      WrapperStr & typeform, bool get_first_template_actual = FALSE,
                      const Q3ValueList<FormalParameterList> & tmplts = empty);
    bool find_type(WrapperStr type, UmlTypeSpec & typespec,
                   NDict<Class> & defined);
#ifdef ROUNDTRIP
    virtual Class * upload_define(UmlClass *) = 0;
#endif

    static bool get_template(FormalParameterList & tmplt);

protected:
    Class * declare_if_needed(const WrapperStr & name,
                              const WrapperStr & stereotype,
                              const FormalParameterList & formals,
                              NDict<Class> & declared,
                              NDict<Class> & defined);
    Class * define(const WrapperStr & name, const WrapperStr & stereotype,
                   NDict<Class> & declared, NDict<Class> & defined);

    static Q3ValueList<FormalParameterList> empty;
};

#endif
