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

#include "UmlBaseClass.h"
#include "UmlFormalParameter.h"
//Added by qt3to4:
#include "misc/mystr.h"
#include <Q3ValueList>
#include <Q3PtrList>

class ClassContainer;
#ifdef ROUNDTRIP
class Class;
class UmlRelation;
class UmlExtraClassMember;
#endif

typedef Q3ValueList<UmlFormalParameter> FormalParameterList;

class UmlClass : public UmlBaseClass
{
#ifdef ROUNDTRIP
private:
    bool created;
    Class * the_class;
#endif

public:
    UmlClass(void * id, const WrapperStr & n);

    bool manage_inherit(ClassContainer * pack,
                        const Q3ValueList<FormalParameterList> & tmplt
#ifdef REVERSE
                        , bool libp
# ifdef ROUNDTRIP
                        , bool roundtrip, QList<UmlItem *> & expected_order
                        , bool container_roundtrip, QList<UmlItem *> & container_expected_order
# endif
#endif
                       );
    bool get_actuals(UmlClass * mother, ClassContainer * container,
                     const Q3ValueList<FormalParameterList> & formals
#ifdef ROUNDTRIP
                     , bool roundtrip
#endif
                    );

    void set_under_construction(bool y, bool rec = FALSE);
    bool inside_its_definition();
    bool is_itself(WrapperStr t);

    static void clear_usings() {
        Usings.clear();
    }
    void using_it() {
        Usings.replace(name(), this);
    }
    static UmlClass * used(const char * n) {
        return Usings[n];
    }
    static void save_using_scope() {
        UsingScope.prepend(Usings);
    }
    static void restore_using_scope();

#ifdef REVERSE
    void need_artifact(const WrapperStr & nmsp);
    virtual bool need_source();

# ifdef ROUNDTRIP
    virtual void upload(ClassContainer * cnt);
    virtual bool set_roundtrip_expected();
    virtual void mark_useless(QList<UmlItem *> & l);
    virtual void scan_it(int & n);
    virtual void send_it(int n);
    bool is_created() const {
        return created;
    }
    void set_created() {
        created = TRUE;
    }
    Class * get_class() const {
        return the_class;
    }
    UmlItem * search_for_att_rel(const WrapperStr & name);
    UmlExtraClassMember * search_for_extra(const WrapperStr & name, const WrapperStr & decl);
    UmlRelation * search_for_inherit(UmlClass * mother);
    void reorder(QList<UmlItem *> & expected_order);
# endif
#endif

private:
    UmlClass * auxilarily_typedef(const WrapperStr & base
#ifdef REVERSE
                                  , bool libp
# ifdef ROUNDTRIP
                                  , bool container_roundtrip
                                  , QList<UmlItem *> & container_expected_order
# endif
#endif
                                 );

    static QList<UmlClass *> UnderConstruction;
    static Q3Dict<UmlClass> Usings;
    static Q3ValueList<Q3Dict<UmlClass> > UsingScope;
};

#endif
