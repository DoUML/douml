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
#include <qcursor.h>
//Added by qt3to4:

//Added by qt3to4:
#include <Q3PtrList>

#include "RelationData.h"
#include "BrowserRelation.h"
#include "BrowserAttribute.h"
#include "BrowserOperation.h"
#include "RelationDialog.h"
#include "BrowserClass.h"
#include "ClassDialog.h"
#include "ClassData.h"
#include "myio.h"
#include "ToolCom.h"
#include "GenerationSettings.h"
#include "mu.h"
#include "translate.h"
#include "Logging/QsLog.h"

IdDict<RelationData> RelationData::all(1023, __FILE__);
QList<RelationData *> RelationData::Unconsistent;

// file format < 56
static QList<RelationData *> IncludeToHeaderIfExternal;

RelationData::RelationData(UmlCode e, int id)
    : Labeled<RelationData>(all, id), is_deleted(FALSE), is_unconsistent(FALSE), type(e)
{
    a.isa_class_relation = b.isa_class_relation = FALSE;
    a.isa_volatile_relation = b.isa_volatile_relation = FALSE;
    a.isa_const_relation = b.isa_const_relation = FALSE;
    a.is_derived = b.is_derived =  FALSE;
    a.is_derivedunion = b.is_derivedunion =  FALSE;
    a.is_ordered = b.is_ordered =  FALSE;
    a.is_unique = b.is_unique =  FALSE;
    a.uml_visibility = b.uml_visibility = UmlDefaultVisibility;
    a.cpp_virtual_inheritance = b.cpp_virtual_inheritance = FALSE;
    a.cpp_visibility = b.cpp_visibility = UmlDefaultVisibility;
    a.cpp_mutable = b.cpp_mutable = FALSE;
    a.java_transient = b.java_transient = FALSE;
    a.idl_truncatable_inheritance = b.idl_truncatable_inheritance = FALSE;
    a.idl_case = b.idl_case = 0;
    name = default_name(e);
    SetStart(0);
    SetEnd(0);
    end_removed_from = 0;
    original_id = 0;
}


RelationData::RelationData(const BrowserRelation * model,
                           BrowserRelation * r)
    : ClassMemberData((ClassMemberData *) model->get_data()),
      Labeled<RelationData>(all, 0), is_deleted(FALSE), is_unconsistent(FALSE)
{
    RelationData * md = (RelationData *) model->get_data();

    type = md->type;
    name = md->name;
    a = md->a;
    b = md->b;
    set_association(md->association);
    original_id = 0;

    if (md->start == model) {
        SetStart(r);

        if (md->end_removed_from == 0) {
            // bi dir, self to model became self to r
            SetEnd(new BrowserRelation((md->get_end_class() == (BrowserClass *) model->parent())
                                      ? (BrowserClass *) r->parent() : md->get_end_class(),
                                      this));
            end_removed_from = 0;
        }
        else {
            // uni dir
            SetEnd(0);
            end_removed_from = (md->end_removed_from == (BrowserClass *) model->parent())
                               ? (BrowserClass *) r->parent() : md->end_removed_from;
            connect(end_removed_from->get_data(), SIGNAL(deleted()),
                    this, SLOT(end_deleted()));
        }
    }
    else {
        // bi dir, self to model became self to r
        SetEnd(r);
        SetStart(new BrowserRelation((md->get_start_class() == (BrowserClass *) model->parent())
                                     ? (BrowserClass *) r->parent() : md->get_start_class(),
                                     this));
        end_removed_from = 0;
    }

    if (get_start_class() == md->get_start_class())
        // to not have two times the same role name
        a.role = 0;

    if ((end != 0) && (get_end_class() == md->get_end_class()))
        // to not have two times the same role name
        b.role = 0;
}

RelationData::~RelationData()
{
    all.remove(get_ident());
}

void RelationData::copy(RelationData * model)
{
    // used to convert plug out < 2.0
    a = model->a;
    b = model->b;
    stereotype = model->stereotype;
}

void RelationData::garbage(BrowserRelation * r)
{
    if (r == start) {
        SetStart(0);

        if ((end == 0) ||
            (end->get_data() == 0)) // bi-dir rel, reverse rel wasn't read
            delete this;
    }
    else if (start == 0)
        // end -> start read, but start -> end not read
        delete this;
    else {
        SetEnd(0);

        if (start == 0)
            delete this;
    }
}

void RelationData::clear(bool old)
{
    all.clear(old);
}

void RelationData::update_idmax_for_root()
{
    all.update_idmax_for_root();
}

void RelationData::prepare_update_lib() const
{
    all.memo_id_oid(get_ident(), original_id);
}

void RelationData::renumber(int phase, BrowserRelation * br)
{
    if (br == start)
        new_ident(phase, all);
}

bool RelationData::deletedp() const
{
    return is_deleted;
}

void RelationData::set_deletedp(bool y)
{
    is_deleted = y;
}

void RelationData::delete_it()
{
    if (!deletedp()) {
        if (type != UmlRelations) {
            // a true relation
            if (!start->deletedp())
                start->delete_it();

            if (end && !end->deletedp())
                end->delete_it();
        }

        BasicData::delete_it();
    }
}

bool RelationData::undelete(QString & warning, QString & renamed,
                            BrowserRelation * rel, BooL & br_deleted)
{
    QString s;
    bool changed = FALSE;
    BrowserNode * parent = (BrowserNode *) rel->parent();

    if (is_a(rel)) {
        while (parent->wrong_child_name(a.role, type, FALSE, TRUE)) {
            changed = TRUE;
            a.role = "_" + a.role;
        }
    }
    else {
        while (parent->wrong_child_name(b.role, type, FALSE, TRUE)) {
            changed = TRUE;
            b.role = "_" + b.role;
        }
    }

    br_deleted = FALSE;
    undelete(warning, renamed);

    return changed;
}

void RelationData::undelete(QString & warning, QString & renamed)
{
    if (deletedp()) {
        BasicData::undelete(warning, renamed);

        if (start->deletedp())
            start->undelete(FALSE, warning, renamed);

        if (end && end->deletedp())
            end->undelete(FALSE, warning, renamed);
    }
}

static void default_decls(RoleData & r, UmlCode type, QString cl_stereotype)
{
    if (GenerationSettings::cpp_get_default_defs()) {
        if (ClassDialog::cpp_stereotype(cl_stereotype) != "enum")
            r.cpp_decl =
                GenerationSettings::cpp_default_rel_decl(type, QString());
        else
            r.cpp_decl = "";
    }

    if (GenerationSettings::java_get_default_defs())
        r.java_decl = GenerationSettings::java_default_rel_decl(QString());

    if (GenerationSettings::php_get_default_defs())
        r.php_decl = GenerationSettings::php_default_rel_decl();

    if (GenerationSettings::python_get_default_defs())
        r.python_decl = GenerationSettings::python_default_rel_decl(type, QString());

    if (GenerationSettings::idl_get_default_defs()) {
        QString idl_st = ClassDialog::idl_stereotype(cl_stereotype);

        if (idl_st == "union")
            r.idl_decl = GenerationSettings::idl_default_union_rel_decl(QString());
        else if (idl_st == "valuetype")
            r.idl_decl = GenerationSettings::idl_default_valuetype_rel_decl(QString());
        else if (idl_st == "enum")
            r.idl_decl = "";
        else
            r.idl_decl = GenerationSettings::idl_default_rel_decl(QString());
    }
}

BrowserRelation * RelationData::set_start_end(BrowserRelation * s, BrowserClass * e)
{
    SetStart(s);
    if (! uni_directional(type))
        SetEnd(new BrowserRelation(e, this));
    else {
        end_removed_from = e;
        connect(e->get_data(), SIGNAL(deleted()), this, SLOT(end_deleted()));
    }

    if (a.uml_visibility == UmlDefaultVisibility) {
        QString st = ((BrowserClass *) s->parent())->get_stereotype();

        switch (type) {
        case UmlGeneralisation:
        case UmlRealize:
            a.uml_visibility = UmlPublic;

            if (GenerationSettings::cpp_get_default_defs())
                a.cpp_decl = (ClassDialog::cpp_stereotype(st) != "enum")
                             ? "${type}" : "";

            if (GenerationSettings::java_get_default_defs())
                a.java_decl = "${type}";

            if (GenerationSettings::php_get_default_defs())
                a.php_decl = "${type}";

            if (GenerationSettings::python_get_default_defs())
                a.python_decl = "${type}";

            if (GenerationSettings::idl_get_default_defs())
                a.idl_decl = (ClassDialog::idl_stereotype(st) != "enum")
                             ? "${type}" : "";

            break;

        case UmlDependency:
            if (GenerationSettings::cpp_get_default_defs())
                a.cpp_decl = "#include in source";

            if (GenerationSettings::php_get_default_defs())
                a.php_decl = "${type}";

            if (GenerationSettings::python_get_default_defs())
                a.python_decl = "${type}";

            break;

        default:
            a.uml_visibility =
                ((BrowserNode *) start->parent())->get_visibility(UmlRelations);
            default_decls(a, type,
                          ((BrowserNode *) start->parent())->get_data()->get_stereotype());
        }
    }

    if (end != 0) {
        if (b.uml_visibility == UmlDefaultVisibility)
            b.uml_visibility = e->get_visibility(UmlRelations);

        default_decls(b, type,
                      ((BrowserNode *) end->parent())->get_data()->get_stereotype());
    }
    else
        b.uml_visibility = end_removed_from->get_visibility(UmlRelations);

    return end;
}

QString RelationData::get_name(BrowserRelation * cl) const
{
    static QString result;

    if (cl == start) {
        if (!a.role.isEmpty()) {
            const char * role = a.role;
            const char * tName = get_name();
            result = QString(role + QString(" (") + tName + ")");
            return result;
        }
    }
    else if (cl == end) {
        if (!b.role.isEmpty()) {
        QLOG_INFO() << "Returning name for relation: " << QString((const char *) b.role) + " (" + get_name() + ")";
            const char * role = b.role;
            const char * tName = get_name();
            result = QString(role + QString(" (") + tName + ")");
            return QString(role + QString(" (") + tName + ")");
        }
    }

    bool nameIsNotADefaultType = this->name != default_name(type);

    if (nameIsNotADefaultType)
        result = ("(" + this->name + ")").operator QString();
    else
        result = this->name.operator QString();

    //QLOG_INFO() << "Returning name for relation: " << relationName;
    return result;
}

QString RelationData::definition(bool, bool with_kind) const
{
    return (with_kind)
            ? ("[" + TR("relation") + "] " + this->name).operator QString()
           : this->name.WrapperStr::operator QString();
}

UmlVisibility RelationData::get_visibility(BrowserNode * bn)
{
    return (bn == start)
           ? a.uml_visibility
           : b.uml_visibility;
}

void RelationData::set_association(const AType & t)
{
    if (association.type != t.type) {
        if (association.type != 0)
            disconnect(association.type->get_data(), 0, this, 0);

        if ((association.type = t.type) != 0) {
            connect(association.type->get_data(), SIGNAL(deleted()),
                    this, SLOT(on_delete()));
        }
    }

    association.explicit_type = t.explicit_type;
}

void RelationData::on_delete()
{
    if (association.type && association.type->deletedp()) {
        association.explicit_type = association.type->get_name();
        association.type = 0;
    }

    if ((a.idl_case != 0) && a.idl_case->deletedp()) {
        a.idl_explicit_case = a.idl_case->get_name();
        a.idl_case = 0;
    }

    if ((b.idl_case != 0) && b.idl_case->deletedp()) {
        b.idl_explicit_case = b.idl_case->get_name();
        b.idl_case = 0;
    }

    modified();
}

bool RelationData::set_stereotype(const QString & s)
{
    bool result = ((const char *) stereotype != s);

    stereotype = s;

    if (start != 0)
        start->update_stereotype();

    if (end != 0)
        end->update_stereotype();

    return result;
}

bool RelationData::set_stereotype(const WrapperStr & s)
{
    bool result = ((const char *) stereotype != s);

    stereotype = s;

    if (start != 0)
        start->update_stereotype();

    if (end != 0)
        end->update_stereotype();

    return result;
}

bool RelationData::set_stereotype(const char * s)
{
    bool result = ((const char *) stereotype != s);

    stereotype = s;

    if (start != 0)
        start->update_stereotype();

    if (end != 0)
        end->update_stereotype();

    return result;
}

bool RelationData::decldefbody_contain(const QString & s, bool cs,
                                       BrowserNode * br)
{
    return (is_a((BrowserRelation *) br))
           ? ((QString(get_cppdecl_a()).find(s, 0, cs) != -1) ||
              (QString(get_javadecl_a()).find(s, 0, cs) != -1) ||
              (QString(get_pythondecl_a()).find(s, 0, cs) != -1) ||
              (QString(get_phpdecl_a()).find(s, 0, cs) != -1) ||
              (QString(get_idldecl_a()).find(s, 0, cs) != -1))
           : ((QString(get_cppdecl_b()).find(s, 0, cs) != -1) ||
              (QString(get_javadecl_b()).find(s, 0, cs) != -1) ||
              (QString(get_phpdecl_b()).find(s, 0, cs) != -1) ||
              (QString(get_pythondecl_b()).find(s, 0, cs) != -1) ||
              (QString(get_idldecl_b()).find(s, 0, cs) != -1));
}

bool RelationData::wrong_role_a_name(const QString & s)
{

    QString aRole = a.role;
    return (!s.isEmpty() &&
            (s != a.role) &&
            ((BrowserNode *) start->parent())->wrong_child_name(s, type, FALSE, FALSE));
}

bool RelationData::wrong_role_b_name(const QString & s)
{
    return ((end != 0) &&
            !s.isEmpty() &&
            (s != b.role) &&
            ((BrowserNode *) end->parent())->wrong_child_name(s, type, FALSE, FALSE));
}

void RelationData::edit()
{
    (new RelationDialog(this))->show();
}

bool RelationData::check_end_visibility()
{
    if (!navigable(end)) {
        if (end_removed_from == 0) {
            end_removed_from = end->extract();
            SetEnd(0);
            connect(end_removed_from->get_data(), SIGNAL(deleted()),
                    this, SLOT(end_deleted()));
        }

        return FALSE;
    }

    if (end_removed_from != 0) {
        disconnect(end_removed_from->get_data(), SIGNAL(deleted()),
                   this, SLOT(end_deleted()));
        SetEnd(BrowserRelation::reinsert(end_removed_from, this));
        end_removed_from = 0;
        end->update_stereotype();
    }

    return TRUE;
}

void RelationData::end_deleted()
{
    // the unidirectionnal relation end class is removed
    start->package_modified();
    delete_it();
}

bool RelationData::navigable(BrowserRelation * r) const
{
    return (r == start) ? TRUE : !uni_directional(type);
}

bool RelationData::isa_association(UmlCode type)
{
    switch (type) {
    case UmlGeneralisation:
    case UmlDependency:
    case UmlRealize:
        return FALSE;

    default:
        return TRUE;
    }
}

bool RelationData::isa_inherit(UmlCode type)
{
    switch (type) {
    case UmlGeneralisation:
    case UmlRealize:
        return TRUE;

    default:
        return FALSE;
    }
}

const QString & RelationData::default_name(UmlCode e)
{
    static const QString association("<association>");
    static const QString unidirectionalassociation("<unidirectional association>");
    static const QString generalisation("<generalisation>");
    static const QString aggregation("<aggregation>");
    static const QString aggregationbyvalue("<composition>");
    static const QString directionalaggregation("<directional aggregation>");
    static const QString directionalaggregationbyvalue("<directional composition>");
    static const QString dependency("<dependency>");
    static const QString realization("<realization>");
    static const QString notread("<not yet read>");
    static const QString wrong("<wrong type>");

    switch (e) {
    case UmlAssociation:
        return association;

    case UmlDirectionalAssociation:
        return unidirectionalassociation;

    case UmlGeneralisation:
        return generalisation;

    case UmlAggregation:
        return aggregation;

    case UmlAggregationByValue:
        return aggregationbyvalue;

    case UmlDirectionalAggregation:
        return directionalaggregation;

    case UmlDirectionalAggregationByValue:
        return directionalaggregationbyvalue;

    case UmlDependency:
        return dependency;

    case UmlRealize:
        return realization;

    case UmlRelations:
        return notread;

    default:
        return wrong;
    }
}

bool RelationData::uni_directional(UmlCode type)
{
    switch (type) {
    case UmlGeneralisation:
    case UmlDirectionalAssociation:
    case UmlDirectionalAggregation:
    case UmlDirectionalAggregationByValue:
    case UmlDependency:
    case UmlRealize:
        return TRUE;

    default:
        return FALSE;
    }
}

BrowserClass * RelationData::get_start_class() const
{
    return ((BrowserClass *) start->parent());
}

BrowserClass * RelationData::get_end_class() const
{
    return (end_removed_from != 0)
           ? ((BrowserClass *) end_removed_from)
           : ((BrowserClass *) end->parent());
}

void RelationData::unidir_change_dest(BrowserNode * e)
{
    // called by BrowserClass::plug_out_conversion()
    disconnect(end_removed_from->get_data(), SIGNAL(deleted()),
               this, SLOT(end_deleted()));
    end_removed_from = e;
    connect(end_removed_from->get_data(), SIGNAL(deleted()),
            this, SLOT(end_deleted()));
}

void RelationData::select_in_browser(bool prefer_start) const
{
    if (prefer_start || (end == 0))
        start->select_in_browser();
    else
        end->select_in_browser();
}

//

const char * RelationData::get_idlcase(const RoleData & role)
{
    return (role.idl_case != 0) ? role.idl_case->get_name() : role.idl_explicit_case.operator QString();
}

void RelationData::set_idlcase(RoleData & role, BrowserAttribute * at,
                               const char * e)
{
    if (role.idl_case != at) {
        // do not disconnect because don't check the other side
        role.idl_case = at;

        if (at != 0)
            connect(at->get_data(), SIGNAL(deleted()),
                    this, SLOT(on_delete()));
    }

    role.idl_explicit_case = e;
}

//

void RelationData::send_uml_def(ToolCom * com, BrowserRelation * rel)
{
    int api = com->api_format(true);

    if (rel == start) {
        BasicData::send_uml_def(com, rel, a.comment);
        com->write_bool(a.isa_class_relation);

        if (api >= 13)
            com->write_bool(a.isa_volatile_relation);

        com->write_char(((api >= 23) ||
                         (a.uml_visibility != UmlPackageVisibility))
                        ? a.uml_visibility : UmlPublic);

        if (api >= 30)
            com->write_string(a.constraint);

        com->write_char(type);
        get_end_class()->write_id(com);

        if (api >= 22)
            association.send_def(com);

        com->write_string(a.role);
        com->write_string(a.multiplicity);
        com->write_string(a.init_value);
        com->write_bool(a.isa_const_relation);

        if (api > 41) {
            com->write_bool(a.is_derived);
            com->write_bool(a.is_derivedunion);
            com->write_bool(a.is_ordered);
            com->write_bool(a.is_unique);
        }
    }
    else {
        BasicData::send_uml_def(com, rel, b.comment);
        com->write_bool(b.isa_class_relation);

        if (api >= 13)
            com->write_bool(b.isa_volatile_relation);

        com->write_char(((api >= 23) ||
                         (b.uml_visibility != UmlPackageVisibility))
                        ? b.uml_visibility : UmlPublic);

        if (api >= 30)
            com->write_string(b.constraint);

        com->write_char(type);
        get_start_class()->write_id(com);

        if (api >= 22)
            association.send_def(com);

        com->write_string(b.role);
        com->write_string(b.multiplicity);
        com->write_string(b.init_value);
        com->write_bool(b.isa_const_relation);

        if (api > 41) {
            com->write_bool(b.is_derived);
            com->write_bool(b.is_derivedunion);
            com->write_bool(b.is_ordered);
            com->write_bool(b.is_unique);
        }
    }

    if (rel->get_get_oper() != 0)
        rel->get_get_oper()->write_id(com);
    else
        com->write_id(0);

    if (rel->get_set_oper() != 0)
        rel->get_set_oper()->write_id(com);
    else
        com->write_id(0);
}

void RelationData::send_cpp_def(ToolCom * com, BrowserRelation * rel)
{
    int api = com->api_format(true);

    if (rel == start) {
        com->write_string(a.cpp_decl);

        if (api >= 23)
            com->write_char(a.cpp_visibility);
        else {
            switch (a.cpp_visibility) {
            case UmlPackageVisibility:
                com->write_char(UmlPublic);
                break;

            case UmlDefaultVisibility:
                com->write_char(UmlDefaultVisibility - 1);
                break;

            default:
                com->write_char(a.cpp_visibility);
            }
        }

        if (api < 13)
            com->write_bool(a.isa_volatile_relation);

        com->write_bool(a.cpp_mutable);
        com->write_bool(a.cpp_virtual_inheritance);
    }
    else {
        com->write_string(b.cpp_decl);

        if (api >= 23)
            com->write_char(b.cpp_visibility);
        else {
            switch (b.cpp_visibility) {
            case UmlPackageVisibility:
                com->write_char(UmlPublic);
                break;

            case UmlDefaultVisibility:
                com->write_char(UmlDefaultVisibility - 1);
                break;

            default:
                com->write_char(b.cpp_visibility);
            }
        }

        if (api < 13)
            com->write_bool(b.isa_volatile_relation);

        com->write_bool(b.cpp_mutable);
        com->write_bool(FALSE);
    }
}

void RelationData::send_java_def(ToolCom * com, BrowserRelation * rel)
{
    int api = com->api_format(true);

    if (rel == start) {
        com->write_string(a.java_decl);

        if (api >= 21)
            com->write_string(a.java_annotation);

        com->write_bool(a.java_transient);

        if ((api >= 19) && (api < 21))
            com->write_string(a.java_annotation);
    }
    else {
        com->write_string(b.java_decl);

        if (api >= 21)
            com->write_string(b.java_annotation);

        com->write_bool(b.java_transient);

        if ((api >= 19) && (api < 21))
            com->write_string(b.java_annotation);
    }
}

void RelationData::send_php_def(ToolCom * com, BrowserRelation * rel)
{
    if (rel == start) {
        com->write_string(a.php_decl);
    }
    else {
        com->write_string(b.php_decl);
    }
}

void RelationData::send_python_def(ToolCom * com, BrowserRelation * rel)
{
    if (rel == start) {
        com->write_string(a.python_decl);
    }
    else {
        com->write_string(b.python_decl);
    }
}

void RelationData::send_idl_def(ToolCom * com, BrowserRelation * rel)
{
    if (rel == start) {
        com->write_string(a.idl_decl);
        com->write_bool(a.idl_truncatable_inheritance);

        if (a.idl_case == 0) {
            com->write_id(0);
            com->write_string(a.idl_explicit_case);
        }
        else
            a.idl_case->write_id(com);
    }
    else {
        com->write_string(b.idl_decl);
        com->write_bool(b.idl_truncatable_inheritance);

        if (b.idl_case == 0) {
            com->write_id(0);
            com->write_string(b.idl_explicit_case);
        }
        else
            b.idl_case->write_id(com);
    }
}

bool RelationData::tool_cmd(ToolCom * com, BrowserRelation * rel,
                            const char * args)
{
    if (((unsigned char) args[-1]) >= firstSetCmd) {
        if (!rel->is_writable() && !root_permission())
            com->write_ack(FALSE);
        else {
            RoleData & r = (rel == start) ? a : b;

            switch ((unsigned char) args[-1]) {
            case setDescriptionCmd:
                r.comment = args;
                break;

            case setStereotypeCmd:
                set_stereotype(args);
                modified();
                com->write_ack(TRUE);

                // send the other side to update it
                if (rel == start) {
                    if (end != 0)
                        end->write_id(com);
                    else
                        com->write_id(0);
                }
                else
                    start->write_id(com);

                return TRUE;

            case setRelationAssocClassCmd: {
                AType t;

                com->get_type(t, args);
                set_association(t);
                modified();
                com->write_ack(TRUE);

                // send the other side to update it
                if (rel == start) {
                    if (end != 0)
                        end->write_id(com);
                    else
                        com->write_id(0);
                }
                else
                    start->write_id(com);
            }

            return TRUE;

            case setNameCmd:
                name = args;
                break;

            case setDefaultValueCmd:
                r.init_value = args;
                break;

            case setIsClassMemberCmd:
                r.isa_class_relation = (*args != 0);
                break;

            case setIsVolatileCmd:
                r.isa_volatile_relation = (*args != 0);
                break;

            case setVisibilityCmd: {
                UmlVisibility v;

                if (! com->get_visibility(v, args)) {
                    com->write_ack(FALSE);
                    return TRUE;
                }

                r.uml_visibility = v;
            }
            break;

            case setConstraintCmd:
                r.constraint = args;
                break;

            case setCppDeclCmd:
                r.cpp_decl = args;
                break;

            case setJavaDeclCmd:
                r.java_decl = args;
                break;

            case setJavaAnnotationCmd: {
                WrapperStr s = args;

                s = s.stripWhiteSpace();

                if (! s.isEmpty())
                    s += '\n';

                r.java_annotation = s;
            }
            break;

            case setPhpDeclCmd:
                r.php_decl = args;
                break;

            case setPythonDeclCmd:
                r.python_decl = args;
                break;

            case setIdlDeclCmd:
                r.idl_decl = args;
                break;

            case setCppVisibilityCmd: {
                UmlVisibility v;

                if (! com->get_visibility(v, args)) {
                    com->write_ack(FALSE);
                    return TRUE;
                }

                r.cpp_visibility = v;
            }
            break;

            case setIsReadOnlyCmd:
                r.isa_const_relation = (*args != 0);
                break;

            case setIsCppMutableCmd:
                r.cpp_mutable = (*args != 0);
                break;

            case setRoleNameCmd:
                if ((rel == start) ? wrong_role_a_name(args)
                    : wrong_role_b_name(args)) {
                    com->write_ack(FALSE);
                    return TRUE;
                }

                r.role = args;
                break;

            case setMultiplicityCmd:
                r.multiplicity = args;
                break;

            case setCppVirtualInheritanceCmd:
                r.cpp_virtual_inheritance = (*args != 0);
                break;

            case setIsIdlTruncatableCmd:
                r.idl_truncatable_inheritance = (*args != 0);;
                break;

            case setIsJavaTransientCmd:
                r.java_transient = (*args != 0);
                break;

            case setIdlCaseCmd: {
                BrowserAttribute * at = (BrowserAttribute *) com->get_id(args);

                set_idlcase(r, at, args);
            }
            break;

            case setDerivedCmd:
                switch (*args) {
                case 0:
                    r.is_derived = r.is_derivedunion = FALSE;
                    break;

                case 1:
                    r.is_derived = TRUE;
                    r.is_derivedunion = FALSE;
                    break;

                case 3:
                    r.is_derived = r.is_derivedunion = TRUE;
                    break;

                default:
                    // derived union but non derived
                    com->write_ack(FALSE);
                    return TRUE;
                }

                break;

            case setOrderingCmd:
                r.is_ordered = (*args != 0);
                break;

            case setUniqueCmd:
                r.is_unique = (*args != 0);
                break;

            case setTypeCmd:
                // only inside associations
            {
                UmlCode c;

                if (!com->get_relation_kind(c, args)) {
                    com->write_ack(FALSE);
                    return TRUE;
                }
                else {
                    type = c;

                    if (end)
                        end->package_modified();

                    if (check_end_visibility())
                        end->modified();

                    start->modified();
                    start->package_modified();
                    modified();
                }
            }
            break;

            default:
                return FALSE;
            }

            // ok case
            if (rel == start) {
                start->modified();
                start->package_modified();
            }
            else {
                end->modified();
                end->package_modified();
            }

            modified();
            com->write_ack(TRUE);
        }
    }
    else {
        switch ((unsigned char) args[-1]) {
        case getDefCmd:
            send_uml_def(com, rel);
            send_cpp_def(com, rel);
            send_java_def(com, rel);

            if (com->api_format() >= 34) {
                send_php_def(com, rel);

                if (com->api_format() >= 39)
                    send_python_def(com, rel);
            }

            send_idl_def(com, rel);
            break;

        case getUmlDefCmd:
            send_uml_def(com, rel);
            break;

        case getCppDefCmd:
            send_uml_def(com, rel);
            send_cpp_def(com, rel);
            break;

        case getJavaDefCmd:
            send_uml_def(com, rel);
            send_java_def(com, rel);
            break;

        case getPhpDefCmd:
            send_uml_def(com, rel);
            send_php_def(com, rel);
            break;

        case getPythonDefCmd:
            send_uml_def(com, rel);
            send_python_def(com, rel);
            break;

        case getIdlDefCmd:
            send_uml_def(com, rel);
            send_idl_def(com, rel);
            break;

        case sideCmd:
            if (*args)
                start->write_id(com);
            else if (end != 0)
                end->write_id(com);
            else
                com->write_id(0);

            break;

        default:
            return FALSE;
        }
    }

    return TRUE;
}

bool RelationData::is_writable(const BrowserRelation * br) const
{
    if ((br == end) && !end->is_writable())
        return FALSE;

    // the relation data is save in the package of the relation start
    return start->is_writable();
}

//

static void save_role(const RoleData & role, bool assoc, QTextStream & st,
                      QString & warning)
{
    if (assoc) {
        st << "role_name ";
        save_string(role.role, st);

        if (!role.multiplicity.isEmpty()) {
            st << " multiplicity ";
            save_string(role.multiplicity, st);
        }

        if (!role.init_value.isEmpty()) {
            st << " init_value ";
            save_string(role.init_value, st);
        }

        st << " ";

        if (role.isa_class_relation)
            st << "class_relation ";

        if (role.isa_volatile_relation)
            st << "volatile ";

        if (role.isa_const_relation)
            st << "const_relation ";

        if (role.is_derived)
            st << ((role.is_derivedunion) ? "derivedunion " : "derived ");

        if (role.is_ordered)
            st << "ordered ";

        if (role.is_unique)
            st << "unique ";
    }

    st << stringify(role.uml_visibility);

    if (! role.comment.isEmpty()) {
        nl_indent(st);
        st << "comment ";
        save_string(role.comment, st);
    }

    if (!role.constraint.isEmpty()) {
        nl_indent(st);
        st << "constraint ";
        save_string(role.constraint, st);
    }

    if (!role.cpp_decl.isEmpty()) {
        nl_indent(st);
        st << "cpp ";

        if (role.cpp_virtual_inheritance)
            st << "virtual ";

        if (role.cpp_mutable)
            st << "mutable ";

        st << stringify(role.cpp_visibility) << " ";
        save_string(role.cpp_decl, st);
    }

    if (role.java_transient) {
        nl_indent(st);
        st << "transient";
    }

    if (!role.java_decl.isEmpty()) {
        nl_indent(st);
        st << "java ";
        save_string(role.java_decl, st);
    }

    if (!role.java_annotation.isEmpty()) {
        nl_indent(st);
        st << "java_annotation ";
        save_string(role.java_annotation, st);
    }

    if (!role.php_decl.isEmpty()) {
        nl_indent(st);
        st << "php ";
        save_string(role.php_decl, st);
    }

    if (!role.python_decl.isEmpty()) {
        nl_indent(st);
        st << "python ";
        save_string(role.python_decl, st);
    }

    if (role.idl_case != 0) {
        nl_indent(st);
        st << "idl_case ";
        role.idl_case->save(st, TRUE, warning);
    }
    else if (! role.idl_explicit_case.isEmpty()) {
        nl_indent(st);
        st << "idl_explicit_case ";
        save_string(role.idl_explicit_case, st);
    }

    if (!role.idl_decl.isEmpty()) {
        nl_indent(st);
        st << "idl ";
        save_string(role.idl_decl, st);
    }

    if (role.idl_truncatable_inheritance) {
        nl_indent(st);
        st << "truncatable";
    }

    nl_indent(st);
}

void RelationData::save(QTextStream & st, bool ref, QString & warning) const
{
    if (ref)
        st << "relation_ref " << get_ident() << " // " << (name);
    else {
        nl_indent(st);
        st << "relation " << get_ident() << " " << stringify(type);

        if (name != default_name(type)) {
            st << " name ";
            save_string(name, st);
        }

        indent(+1);

        BasicData::save(st, warning);

        if (original_id != 0) {
            nl_indent(st);
            st << "oid " << original_id;
        }

        bool assoc = isa_association(type);

        nl_indent(st);
        indent(+1);
        st << "a ";
        save_role(a, assoc, st, warning);
        start->save(st, TRUE, warning);
        indent(-1);

        nl_indent(st);
        indent(+1);
        st << "b";

        if (!RelationData::uni_directional(type)) {
            st << " ";
            save_role(b, assoc, st, warning);
            end->save(st, TRUE, warning);
        }
        else {
            if (!b.multiplicity.isEmpty()) {
                st << " multiplicity ";
                save_string(b.multiplicity, st);
            }

            st << " parent ";
            end_removed_from->save(st, TRUE, warning);
        }

        indent(-1);

        if ((association.type != 0) || !association.explicit_type.isEmpty()) {
            nl_indent(st);
            association.save(st, warning, "association_type ", "association_explicit_type ");
        }

        indent(-1);
    }
}

RelationData * RelationData::read_ref(char *& st, bool complete,
                                      const char * k)
{
    if (k == 0)
        read_keyword(st, "relation_ref");
    else if (strcmp(k, "relation_ref"))
        wrong_keyword(k, "relation_ref");

    int id = read_id(st);
    RelationData * result = all[id];

    if (result == 0) {
        result = new RelationData(UmlRelations/*!!*/, id);

        if (complete) {
            // must create a valid temporary relation
            result->SetStart(BrowserRelation::temporary(result));
            result->SetEnd(BrowserRelation::temporary(result));
        }
    }

    return result;
}

static void read_role(RoleData & role, bool assoc,
                      char *& st, char *& k,
                      RelationData * rd)
{
    if (assoc) {
        read_keyword(st, "role_name");
        role.role = read_string(st);

        k = read_keyword(st);

        if (!strcmp(k, "multiplicity")) {
            role.multiplicity = read_string(st);
            k = read_keyword(st);
        }
        else
            role.multiplicity = 0;

        if (!strcmp(k, "init_value")) {
            role.init_value = read_string(st);
            k = read_keyword(st);
        }
        else
            role.init_value = QString();

        if (!strcmp(k, "class_relation")) {
            role.isa_class_relation = TRUE;
            k = read_keyword(st);
        }
        else
            role.isa_class_relation = FALSE;

        if (!strcmp(k, "volatile")) {
            role.isa_volatile_relation = TRUE;
            k = read_keyword(st);
        }
        else
            role.isa_volatile_relation = FALSE;

        if (!strcmp(k, "const_relation")) {
            role.isa_const_relation = TRUE;
            k = read_keyword(st);
        }
        else
            role.isa_const_relation = FALSE;

        if (!strcmp(k, "derivedunion")) {
            role.is_derived = TRUE;
            role.is_derivedunion = TRUE;
            k = read_keyword(st);
        }
        else if (!strcmp(k, "derived")) {
            role.is_derived = TRUE;
            role.is_derivedunion = FALSE;
            k = read_keyword(st);
        }
        else
            role.is_derived = role.is_derivedunion = FALSE;

        if (!strcmp(k, "ordered")) {
            role.is_ordered = TRUE;
            k = read_keyword(st);
        }
        else
            role.is_ordered = FALSE;

        if (!strcmp(k, "unique")) {
            role.is_unique = TRUE;
            k = read_keyword(st);
        }
        else
            role.is_unique = FALSE;
    }
    else
        k = read_keyword(st);

    role.uml_visibility = ::visibility(k);
    k = read_keyword(st);

    if (!strcmp(k, "comment")) {
        role.comment = read_string(st);
        k = read_keyword(st);
    }

    if (!strcmp(k, "constraint")) {
        role.constraint = read_string(st);
        k = read_keyword(st);
    }
    else
        role.constraint = QString();

    if (!strcmp(k, "cpp")) {
        k = read_keyword(st);

        if (!strcmp(k, "virtual")) {
            role.cpp_virtual_inheritance = TRUE;
            k = read_keyword(st);
        }
        else
            role.cpp_virtual_inheritance = FALSE;

        if (!strcmp(k, "volatile")) {
            // old version
            role.isa_volatile_relation = TRUE;
            k = read_keyword(st);
        }

        if (!strcmp(k, "mutable")) {
            role.cpp_mutable = TRUE;
            k = read_keyword(st);
        }
        else
            role.cpp_mutable = FALSE;

        role.cpp_visibility = ::visibility(k);
        role.cpp_decl = read_string(st);
        k = read_keyword(st);
    }
    else
        role.cpp_decl = QString();

    if (!strcmp(k, "transient")) {
        role.java_transient = TRUE;
        k = read_keyword(st);
    }
    else
        role.java_transient = FALSE;

    if (!strcmp(k, "java")) {
        role.java_decl = read_string(st);
        k = read_keyword(st);
    }
    else
        role.java_decl = QString();

    if (!strcmp(k, "java_annotation")) {
        role.java_annotation = read_string(st);
        k = read_keyword(st);
    }
    else
        role.java_annotation = QString();

    if (!strcmp(k, "php")) {
        role.php_decl = read_string(st);
        k = read_keyword(st);
    }
    else
        role.php_decl = QString();

    if (!strcmp(k, "python")) {
        role.python_decl = read_string(st);
        k = read_keyword(st);
    }
    else
        role.python_decl = QString();

    if (!strcmp(k, "idl_case")) {
        rd->set_idlcase(role, BrowserAttribute::read_ref(st), "");
        k = read_keyword(st);
    }
    else if (!strcmp(k, "idl_explicit_case")) {
        rd->set_idlcase(role, 0, read_string(st));
        k = read_keyword(st);
    }
    else
        rd->set_idlcase(role, 0, "");

    if (!strcmp(k, "idl")) {
        role.idl_decl = read_string(st);
        k = read_keyword(st);
    }
    else
        role.idl_decl = QString();

    if (!strcmp(k, "truncatable")) {
        role.idl_truncatable_inheritance = TRUE;
        k = read_keyword(st);
    }
    else
        role.idl_truncatable_inheritance = FALSE;
}

RelationData * RelationData::read(char *& st, char *& k,
                                  BrowserRelation *& unconsistent)
{
    unconsistent = 0;

    RelationData * result;
    int id;

    if (!strcmp(k, "relation_ref")) {
        if ((result = all[id = read_id(st)]) == 0)
            result = new RelationData(UmlRelations/*!!*/, id);

        k = read_keyword(st);
        return result;
    }
    else if (!strcmp(k, "relation")) {
        if ((result = all[id = read_id(st)]) == 0)
            result = new RelationData(relation_type(read_keyword(st)), id);
        else if (result->type != UmlRelations) {
            // shared identifier
            result->set_unconsistent();
            unconsistent = result->start;
            result = new RelationData(relation_type(read_keyword(st)), id);
            result->set_unconsistent();
        }
        else {
            result->type = relation_type(read_keyword(st));
            result->name = default_name(result->type);

            if (result->start != 0) {
                // Created by RelationData::read_ref()
                // invalidate start/end to not delete result
                // when start/end will be deleted
                result->start->invalidate();
                result->end->invalidate();
            }
        }

        k = read_keyword(st);

        if (!strcmp(k, "name")) {
            result->name = read_string(st);
            k = read_keyword(st);
        }

        result->BasicData::read(st, k);	// updates k

        if (in_lib_import()) {
            result->original_id = id;

            if (! strcmp(k, "oid")) {
                // a sub lib is imported as a part of the imported lib
                (void) read_id(st);
                k = read_keyword(st);
            }
        }
        else if (! strcmp(k, "oid")) {
            result->original_id = read_id(st);
            k = read_keyword(st);
        }

        bool assoc = isa_association(result->type);

        if (strcmp(k, "a"))
            wrong_keyword(k, "a");

        read_role(result->a, assoc, st, k, result);		// updates k
        result->SetStart(BrowserRelation::read_ref(st, k));

        read_keyword(st, "b");

        if (!RelationData::uni_directional(result->type)) {
            read_role(result->b, assoc, st, k, result);	// updates k
            result->SetEnd(BrowserRelation::read_ref(st, k));
            // 'end' may be read before 'start' : relation's type was unknown
            result->end->set_name(0);
            result->end->set_name(result->name);
        }
        else {
            k = read_keyword(st);

            if (!strcmp(k, "multiplicity")) {
                result->b.multiplicity = read_string(st);
                k = read_keyword(st);
            }
            else
                result->b.multiplicity = 0;

            if (strcmp(k, "parent"))
                wrong_keyword(k, "parent");

            result->end_removed_from = BrowserClass::read_ref(st);
            result->b.uml_visibility =
                ((BrowserNode *) result->end_removed_from->parent())->get_visibility(UmlRelations);
            connect(result->end_removed_from->get_data(), SIGNAL(deleted()),
                    result, SLOT(end_deleted()));
            result->SetEnd(0);;

            // manage old declarations
            switch (result->type) {
            case UmlRealize:
            case UmlGeneralisation:
                if (result->a.cpp_decl == "Generated")
                    result->a.cpp_decl = "${type}";

                if (result->a.java_decl == "Generated")
                    result->a.java_decl = "${type}";

                if (result->a.idl_decl == "Generated")
                    result->a.idl_decl = "${type}";

                break;

            case UmlDependency:
                if (!result->a.cpp_decl.isEmpty()) {
                    if (result->stereotype == "friend")
                        result->a.cpp_decl = "${type}";
                    else if ((result->a.cpp_decl == "Generated") ||
                             (result->a.cpp_decl == "${type}"))
                        result->a.cpp_decl = "#include in header";
                    else if ((result->a.cpp_decl == "#include in source") &&
                             (read_file_format() < 56))
                        IncludeToHeaderIfExternal.append(result);
                }

                break;

            default:
                break;
            }
        }

        k = read_keyword(st);

        AType t;

        if (!strcmp(k, "association_type") ||
            !strcmp(k, "association_explicit_type")) {
            t.read(st, "association_type", "association_explicit_type", k);
            result->set_association(t);
            k = read_keyword(st);
        }

        return result;
    }
    else
        return 0;
}

//

bool RelationData::has_unconsistencies()
{
    return !Unconsistent.isEmpty();
}

void RelationData::set_unconsistent()
{
    if (! is_unconsistent) {
        is_unconsistent = TRUE;
        Unconsistent.append(this);
    }
}

void RelationData::delete_unconsistent()
{
    foreach (RelationData *d, Unconsistent) {
        d->SetStart(0);
        d->SetEnd(0);
        if (!d->deletedp())
            d->BasicData::delete_it();

        delete d;
    }
    Unconsistent.clear();
}

// for file < 56, modify import from source to header
// if the target class is external

void RelationData::post_load()
{
    foreach (RelationData *rd, IncludeToHeaderIfExternal) {
        if ((rd->end_removed_from != 0) &&
            (rd->end_removed_from->get_type() == UmlClass) &&
            ((ClassData *) rd->end_removed_from->get_data())->cpp_is_external())
            rd->a.cpp_decl = "#include in header";
    }
    IncludeToHeaderIfExternal.clear();
}

void RelationData::IndexStartEnd()
{
    dataForClass.clear();
    if(start && start->parent())
        dataForClass.insert(((BrowserNode*)start->parent())->get_name(), &a);
    if(end && end->parent())
        dataForClass.insert(((BrowserNode*)end->parent())->get_name(), &b);

}

void RelationData::SetStart(BrowserRelation * relStart)
{
    start = relStart;
    IndexStartEnd();
}

void RelationData::SetEnd(BrowserRelation * relEnd)
{
    end = relEnd;
    IndexStartEnd();
}
