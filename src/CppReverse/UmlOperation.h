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

#ifndef UMLOPERATION_H
#define UMLOPERATION_H

#include "UmlBaseOperation.h"
#include "UmlClass.h"
#include "Namespace.h"
//Added by qt3to4:
#include "misc/mystr.h"
#include <Q3ValueList>
#include <Q3PtrList>

class ClassContainer;
class Package;
class Class;

class UmlOperation : public UmlBaseOperation
{
public:
    UmlOperation(void * id, const WrapperStr & n);
    virtual ~UmlOperation();

    static bool new_one(Class * cl, const WrapperStr & name,
                        const Q3ValueList<FormalParameterList> & tmplt,
                        WrapperStr type, const WrapperStr & modifier,
                        const WrapperStr & pretype, aVisibility visibility,
                        bool inlinep, bool virtualp, bool staticp, bool constp,
                        bool volatilep, bool typenamep,	bool explicitp,
                        bool friendp, WrapperStr friend_template,
                        WrapperStr comment, WrapperStr description, bool pfunc
#ifdef ROUNDTRIP
                        , bool roundtrip, QList<UmlItem *> & expected_order
#endif
                       );
    static void reverse_definition(Package * pack, WrapperStr name, WrapperStr type,
                                   Q3ValueList<FormalParameterList> & tmplt,
                                   bool inlinep, const WrapperStr & comment,
                                   const WrapperStr & description);

#ifdef REVERSE
    virtual bool need_source();
#ifdef ROUNDTRIP
    static void force_defs();
# endif
#endif

    static void skip_body(int level = 0);
    static void skip_expr(WrapperStr end, bool allow_templ = FALSE);
    static bool pfunc(BooL & func, WrapperStr & name, WrapperStr & type,
                      const char * namespec);
private:
#ifdef ROUNDTRIP
    static Q3PtrDict<WrapperStr> DefNotYetSet;
#endif
    static NDict< QList<UmlOperation *> > friends;
    FormalParameterList * formals;
    WrapperStr def0;	// for template operations

    static bool read_param(ClassContainer * container, unsigned rank,
                           UmlParameter & param, WrapperStr & decl,
                           const Q3ValueList<FormalParameterList> & tmplt,
                           BooL & on_error, bool add_defaultvalue);
    static void friend_operations(QList<UmlOperation *> & candidates,
                                  const Q3ValueList<FormalParameterList> & tmplt,
                                  const WrapperStr & name);
    static bool operations(QList<UmlOperation *> & candidates, UmlClass * cl,
                           const Q3ValueList<FormalParameterList> & tmplt,
                           const FormalParameterList *& oper_tmplt,
                           const WrapperStr & name);
    static bool read_throw_elt(ClassContainer * container,
                               UmlTypeSpec & typespec,
                               const Q3ValueList<FormalParameterList> & tmplts);
    bool reverse_if_def(Package * pack,
                        Q3ValueList<FormalParameterList> & tmplts,
                        const FormalParameterList * oper_tmplt,
                        bool inlinep, bool pfct, const WrapperStr & comment,
                        const WrapperStr & description, BooL & on_error,
                        unsigned & nargs, WrapperStr oper_name);

    void update_param_names(Q3ValueList<UmlParameter> & params);
#ifdef ROUNDTRIP
    void update_params(Class * cl, Q3ValueList<UmlParameter> & params);
    void update_exceptions(Class * cl, const Q3ValueList<UmlTypeSpec> & exceptions);

    static void clean_body(WrapperStr & body);
    static UmlOperation * already_exist(Class * container, const WrapperStr & name,
                                        Q3ValueList<UmlParameter> & params,
                                        bool empty_decl);
#endif
};

#endif
