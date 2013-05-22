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

#ifndef RELATIONDATA_H
#define RELATIONDATA_H

#include "ClassMemberData.h"
#include "Labeled.h"
#include "AType.h"
//Added by qt3to4:
#include <QTextStream>

//Added by qt3to4:
#include <Q3PtrList>
#include <QHash>

class QTextStream;

class BrowserRelation;
class BrowserAttribute;
class BrowserNode;
class BrowserClass;

struct RoleData {
    // UML (except some cases to save memory)
    WrapperStr role;
    WrapperStr multiplicity;
    WrapperStr init_value;
    WrapperStr comment;
    WrapperStr constraint;	//
    UmlVisibility uml_visibility : 8;	// : 4 useless here, : 8 faster than : 4 ?
    UmlVisibility cpp_visibility : 8;	// : 4 useless here, : 8 faster than : 4
    bool isa_class_relation : 1;
    bool isa_volatile_relation: 1;
    bool isa_const_relation : 1;
    bool is_derived : 1;
    bool is_derivedunion : 1;
    bool is_ordered : 1;
    bool is_unique : 1;
    bool java_transient : 1;		// java
    bool idl_truncatable_inheritance : 1;	// idl inheritance

    // C++
    bool cpp_virtual_inheritance : 1;
    bool cpp_mutable: 1;
    WrapperStr cpp_decl;

    // Java
    WrapperStr java_decl;
    WrapperStr java_annotation;

    // Php
    WrapperStr php_decl;

    // Python
    WrapperStr python_decl;

    // Idl
    BrowserAttribute * idl_case;	// exclusive with idl_explicit_case
    WrapperStr idl_explicit_case;
    WrapperStr idl_decl;
};

class RelationData : public ClassMemberData, public Labeled<RelationData>
{
    Q_OBJECT

    friend class RelationDialog;
    friend class QuickEdit;

protected:
    static IdDict<RelationData> all;
    static QList<RelationData *> Unconsistent;

    // Uml
    bool is_deleted : 8;	// 1 useless here, 8 faster than 1 ?
    bool is_unconsistent : 8;	// 1 useless here, 8 faster than 1 ?
    UmlCode type: 8;		// < UmlRelations
    WrapperStr name;
    RoleData a;
    RoleData b;
    BrowserRelation * start  = nullptr;
    BrowserRelation * end = nullptr;
    BrowserNode * end_removed_from;
    AType association;		// class association
    int original_id;	// from project library

    QHash<QString, RoleData*> dataForClass;

    virtual void send_uml_def(ToolCom * com, BrowserRelation * rel);
    virtual void send_cpp_def(ToolCom * com, BrowserRelation * rel);
    virtual void send_java_def(ToolCom * com, BrowserRelation * rel);
    virtual void send_php_def(ToolCom * com, BrowserRelation * rel);
    virtual void send_python_def(ToolCom * com, BrowserRelation * rel);
    virtual void send_idl_def(ToolCom * com, BrowserRelation * rel);

public:
    RelationData(UmlCode e, int id = 0);
    RelationData(const BrowserRelation * model, BrowserRelation * r);
    virtual ~RelationData();
    void IndexStartEnd();

    void SetStart(BrowserRelation*);
    void SetEnd(BrowserRelation*);
    RoleData* GetDataForClass(QString node)
    {
        if(dataForClass.contains(node))
                return dataForClass[node];
    }

    void garbage(BrowserRelation * r);
    void copy(RelationData * model);

    virtual bool deletedp() const;
    virtual void set_deletedp(bool y);
    virtual void delete_it();
    virtual void undelete(QString & warning, QString & renamed);
    bool undelete(QString & warning, QString & renamed,
                  BrowserRelation * rel, BooL & br_deleted);

    BrowserRelation * set_start_end(BrowserRelation * s, BrowserClass * e);
    void edit();
    bool check_end_visibility();
    bool navigable(BrowserRelation *) const;
    BrowserClass * get_start_class() const;
    BrowserClass * get_end_class() const;
    BrowserRelation * get_start() const {
        return start;
    }
    BrowserRelation * get_end() const {
        return end;
    }
    void unidir_change_dest(BrowserNode * e);

    UmlCode get_type() const {
        return type;
    }
    void set_type(UmlCode e);

    const char * get_name() const {
        return name;
    }
    QString get_name(BrowserRelation *) const;
    void set_name(const QString &);
    virtual QString definition(bool full, bool with_kind) const;
    virtual bool set_stereotype(const QString &);
    virtual bool set_stereotype(const WrapperStr &);
    virtual bool set_stereotype(const char *);

    virtual bool decldefbody_contain(const QString & s, bool cs, BrowserNode *);

    bool is_a(const BrowserRelation * br) const {
        return br == start;
    }

    const char * get_role_a() const {
        return a.role;
    }
    const char * get_role_b() const {
        return b.role;
    }
    bool wrong_role_a_name(const QString &);
    bool wrong_role_b_name(const QString &);

    AType get_association() const {
        return association;
    }
    void set_association(const AType & t);

    const char * get_multiplicity_a() const {
        return a.multiplicity;
    }
    const char * get_multiplicity_b() const {
        return b.multiplicity;
    }

    bool get_isa_class_relation_a() const {
        return a.isa_class_relation;
    }
    bool get_isa_class_relation_b() const {
        return b.isa_class_relation;
    }

    bool get_isa_const_relation_a() const {
        return a.isa_const_relation;
    }
    bool get_isa_const_relation_b() const {
        return b.isa_const_relation;
    }

    bool get_is_derived_a() const {
        return a.is_derived;
    }
    bool get_is_derived_b() const {
        return b.is_derived;
    }

    bool get_is_derivedunion_a() const {
        return a.is_derivedunion;
    }
    bool get_is_derivedunion_b() const {
        return b.is_derivedunion;
    }

    bool get_is_ordered_a() const {
        return a.is_ordered;
    }
    bool get_is_ordered_b() const {
        return b.is_ordered;
    }

    bool get_is_unique_a() const {
        return a.is_unique;
    }
    bool get_is_unique_b() const {
        return b.is_unique;
    }

    UmlVisibility get_uml_visibility_a() const {
        return a.uml_visibility;
    }
    UmlVisibility get_uml_visibility_b() const {
        return b.uml_visibility;
    }
    virtual UmlVisibility get_visibility(BrowserNode *);

    const char * get_comment_a() const {
        return a.comment;
    }
    const char * get_comment_b() const {
        return b.comment;
    }
    void  set_comment_a(const char * s) {
        a.comment = s;
    }
    void set_comment_b(const char * s) {
        b.comment = s;
    }

    const char * get_constraint_a() const {
        return a.constraint;
    }
    const char * get_constraint_b() const {
        return b.constraint;
    }
    void  set_constraint_a(const char * s) {
        a.constraint = s;
    }
    void set_constraint_b(const char * s) {
        b.constraint = s;
    }

    bool get_cpp_virtual_inheritance() const {
        return a.cpp_virtual_inheritance;
    }

    UmlVisibility get_cpp_visibility_a() const {
        return a.cpp_visibility;
    }
    UmlVisibility get_cpp_visibility_b() const {
        return b.cpp_visibility;
    }

    const char * get_cppdecl_a() const {
        return a.cpp_decl;
    }
    const char * get_cppdecl_b() const {
        return b.cpp_decl;
    }

    const char * get_javadecl_a() const {
        return a.java_decl;
    }
    const char * get_javadecl_b() const {
        return b.java_decl;
    }

    const char * get_phpdecl_a() const {
        return a.php_decl;
    }
    const char * get_phpdecl_b() const {
        return b.php_decl;
    }

    const char * get_pythondecl_a() const {
        return a.python_decl;
    }
    const char * get_pythondecl_b() const {
        return b.python_decl;
    }

    const char * get_idldecl_a() const {
        return a.idl_decl;
    }
    const char * get_idldecl_b() const {
        return b.idl_decl;
    }
    static const char * get_idlcase(const RoleData & role);
    void set_idlcase(RoleData & role, BrowserAttribute * a, const char * e);
    bool get_idl_truncatable() const {
        return a.idl_truncatable_inheritance;
    }

    void select_in_browser(bool prefer_start) const;

    void save(QTextStream &, bool ref, QString & warning) const;
    static RelationData * read_ref(char *& , bool complete = FALSE,
                                   const char * k = 0);
    static RelationData * read(char *& , char *& , BrowserRelation *& unconsistent);

    static bool uni_directional(UmlCode);
    static const QString & default_name(UmlCode e);
    static bool isa_association(UmlCode e);
    static bool isa_inherit(UmlCode e);

    bool tool_cmd(ToolCom * com, BrowserRelation * rel, const char * args);

    static void clear(bool old);
    static void update_idmax_for_root();
    void prepare_update_lib() const;
    void renumber(int phase, BrowserRelation * br);
    bool is_writable(const BrowserRelation * br) const;

    bool unconsistentp() const {
        return is_unconsistent;
    }
    void set_unconsistent();
    static bool has_unconsistencies();
    static void delete_unconsistent();
    static void post_load();

protected slots:
    void end_deleted();
    void on_delete();
};

#endif
