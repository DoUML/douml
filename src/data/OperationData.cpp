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





#include <qcursor.h>
#include <qfile.h>
//Added by qt3to4:
#include <QTextStream>


#include "OperationData.h"
#include "ClassData.h"
#include "ParamData.h"
#include "ExceptionData.h"
#include "KeyValueData.h"
#include "BrowserClass.h"
#include "OperationDialog.h"
#include "BrowserOperation.h"
#include "ClassDialog.h"
#include "GenerationSettings.h"
#include "myio.h"
#include "translate.h"
#include "ToolCom.h"
#include "strutil.h"
#include "BrowserView.h"
#include "mu.h"
#include "err.h"
#include "DialogUtil.h"
#include "ProfiledStereotypes.h"
#include "QSettings"

IdDict<OperationData> OperationData::all(1023, __FILE__);

OperationData::OperationData(int id)
    : Labeled<OperationData>(all, id),
      uml_visibility(UmlDefaultVisibility),
      cpp_visibility(UmlDefaultVisibility),
      is_deleted(FALSE), is_get_or_set(FALSE),
      isa_class_operation(FALSE), is_abstract(FALSE),
      force_body_gen(FALSE), is_volatile(FALSE),
      cpp_const(FALSE), cpp_friend(FALSE), cpp_virtual(FALSE),
      cpp_inline(FALSE), cpp_get_set_frozen(FALSE), cpp_indent_body(TRUE),
      java_final(FALSE), java_synchronized(FALSE),
      java_get_set_frozen(FALSE), java_indent_body(TRUE),
      php_final(FALSE), php_get_set_frozen(FALSE), php_indent_body(TRUE),
      python_get_set_frozen(FALSE), python_indent_body(TRUE),
      idl_oneway(FALSE), idl_get_set_frozen(FALSE),
      nparams(0), nexceptions(0), exceptions(0)
{
}

OperationData::OperationData(OperationData * model, BrowserNode * bn)
    : ClassMemberData(model), Labeled<OperationData>(all, 0),
      uml_visibility(model->uml_visibility),
      cpp_visibility(UmlDefaultVisibility),
      is_deleted(FALSE), is_get_or_set(FALSE),
      isa_class_operation(model->isa_class_operation),
      is_abstract(model->is_abstract),
      force_body_gen(model->force_body_gen),
      is_volatile(model->is_volatile),
      cpp_const(model->cpp_const),
      cpp_friend(model->cpp_friend),
      cpp_virtual(model->cpp_virtual),
      cpp_inline(model->cpp_inline),
      cpp_get_set_frozen(FALSE),
      cpp_indent_body(model->cpp_indent_body),
      java_final(model->java_final),
      java_synchronized(model->java_synchronized),
      java_get_set_frozen(FALSE),
      java_indent_body(model->java_indent_body),
      php_final(model->php_final),
      php_get_set_frozen(FALSE),
      php_indent_body(model->php_indent_body),
      python_get_set_frozen(FALSE),
      python_indent_body(model->python_indent_body),
      idl_oneway(model->idl_oneway),
      idl_get_set_frozen(FALSE),
      nparams(model->nparams),
      nexceptions(model->nexceptions),
      constraint(model->constraint),
      cpp_decl(model->cpp_decl),
      java_annotation(model->java_annotation),
      python_decorator(model->python_decorator),
      idl_decl(model->idl_decl)
{
    browser_node = bn;

    cpp_def.assign((const char *) model->cpp_def, FALSE);
    java_def.assign((const char *) model->java_def, FALSE);
    php_def.assign((const char *) model->php_def, FALSE);
    python_def.assign((const char *) model->python_def, FALSE);
    return_type = model->return_type;
    depend_on(return_type.type);


    if(nparams > 0)
    {
        for(int i(0); i < nparams; ++i)
        {
            params << std::shared_ptr<ParamData>(new ParamData());
            *params[i] = *model->params[i];
            depend_on(params[i]->get_type().type);
        }
    }



    if (nexceptions == 0)
        exceptions = 0;
    else {
        exceptions = new ExceptionData[nexceptions];

        for (unsigned i = 0; i != nexceptions; i += 1) {
            exceptions[i] = model->exceptions[i];
            depend_on(exceptions[i].get_type().type);
        }
    }
}

OperationData::~OperationData()
{

    if (exceptions)
        delete [] exceptions;

    // do not disconnect : deletions may be already made
    unsubscribe_all();

    all.remove(get_ident());
}

void OperationData::PropagateFrom(const OperationData * model, bool goBack, QList<const OperationData*> passed)
{

    passed.append(this);
    BrowserClass* containingClass = static_cast<BrowserClass*>(this->browser_node->get_container(UmlClass));
    QList<BrowserNode *>  passedNodes;
    QList<OperationData*> inheritanceSiblings;
    inheritanceSiblings = containingClass->CollectSameThroughInheritance(this, passedNodes, goBack);

    uml_visibility=model->uml_visibility;
    cpp_visibility=UmlDefaultVisibility;
    //is_abstract = model->is_abstract;
    //force_body_gen = model->force_body_gen;
    is_volatile = model->is_volatile;
    cpp_const = model->cpp_const;
    cpp_friend = model->cpp_friend;
    cpp_virtual = model->cpp_virtual;
    //cpp_inline = model->cpp_inline;
    cpp_get_set_frozen = model->cpp_get_set_frozen;
    //cpp_indent_body = model->cpp_indent_body;
    //java_final = model->java_final;
    //java_synchronized = model->java_synchronized;
    java_get_set_frozen = model->java_get_set_frozen;
    java_indent_body = model->java_indent_body;
    //php_final = model->php_final;
    php_get_set_frozen = model->php_get_set_frozen;
    php_indent_body = model->php_indent_body;
    python_get_set_frozen = model->python_get_set_frozen;

    //python_indent_body = model->python_indent_body;
    idl_oneway = model->idl_oneway;
    idl_get_set_frozen = model->idl_get_set_frozen;

    if (nparams != 0)
    {
        for (unsigned i = 0; i != nparams; i += 1)
        {
            no_longer_depend_on(params[i]->get_type().type);
        }
    }
    if (nexceptions != 0)
    {
        for (unsigned i = 0; i != nexceptions; i += 1)
        {
            no_longer_depend_on(exceptions[i].get_type().type);
        }
    }
    nparams = model->nparams;
    nexceptions = model->nexceptions;
    constraint = model->constraint;
    cpp_decl = model->cpp_decl;
    //java_annotation = model->java_annotation; todo
    python_decorator = model->python_decorator;
    idl_decl = model->idl_decl;

    cpp_def =  model->cpp_def;
    java_def = model->java_def;
    php_def = model->php_def;
    python_def =model->python_def;
    return_type = model->return_type;
    depend_on(return_type.type);

    if(nparams > 0)
    {
        for(int i(0); i < nparams; ++i)
        {
            params << std::shared_ptr<ParamData>(new ParamData());
            *params[i] = *model->params[i];
            depend_on(params[i]->get_type().type);
        }
    }

    if (nexceptions == 0)
        exceptions = 0;
    else
    {
        exceptions = new ExceptionData[nexceptions];

        for (unsigned i = 0; i != nexceptions; i += 1)
        {
            exceptions[i] = model->exceptions[i];
            depend_on(exceptions[i].get_type().type);
        }
    }


    for(OperationData* siblingOper : inheritanceSiblings)
    {
        if(passed.contains(siblingOper))
            continue;
        siblingOper->PropagateFrom(this, goBack, passed);
        ProfiledStereotypes::modified(siblingOper->browser_node, true);
        siblingOper->browser_node->modified();
        siblingOper->browser_node->package_modified();
        siblingOper->modified();
    }

}

bool OperationData::deletedp() const
{
    return is_deleted;
}

void OperationData::set_deletedp(bool y)
{
    is_deleted = y;

    if (y)
        create_modified_body_file();
}

void OperationData::remove_param(std::shared_ptr<ParamData> param)
{
    nparams = nparams - 1;
    params.removeAll(param);
}

void OperationData::insert_param(int position, std::shared_ptr<ParamData> param)
{
    nparams = nparams + 1;
    params.insert(position, param);
}

void OperationData::clear(bool old)
{
    all.clear(old);
}

void OperationData::update_idmax_for_root()
{
    all.update_idmax_for_root();
}

void OperationData::renumber(int phase)
{
    new_ident(phase, all);
}

bool OperationData::is_template_operation() const
{
    return cpp_decl.isEmpty() && is_template_operation(cpp_def);
}

bool OperationData::is_template_operation(QString def) const
{
    int index1 = def.find("${class}");

    if (index1 == -1)
        return FALSE;

    int index2 = def.find("${name}", index1 + 8);

    if (index2 == -1)
        return FALSE;

    def = def.mid(index1 + 8, index2 - index1 - 8);

    if ((index1 = def.find('<')) == -1)
        return FALSE;

    return (def.find('>') > index1);
}

void OperationData::depend_on(BrowserClass * cl)
{
    if (cl != 0) {
        BasicData * def = cl->get_data();

        if (subscribe(def)) {
            connect(def, SIGNAL(deleted()), this, SLOT(on_delete()));
        }
    }
}
void OperationData::no_longer_depend_on(BrowserClass * cl)
{
    if (cl != 0) {
        BasicData * def = cl->get_data();

        if (unsubscribe(def))
        {
            disconnect(def, SIGNAL(deleted()), this, SLOT(on_delete()));
        }
    }
}

void OperationData::on_delete()
{
    if (return_type.type && return_type.type->deletedp()) {
        return_type.explicit_type = return_type.type->get_name();
        return_type.type = 0;
    }

    unsigned short i;

    for (i = 0; i != nparams; i += 1) {
        AType t = params[i]->get_type();

        if (t.type && t.type->deletedp()) {
            t.explicit_type = t.type->get_name();
            t.type = 0;

            params[i]->set_type(t);
        }
    }

    for (i = 0; i != nexceptions; i += 1) {
        AType t = exceptions[i].get_type();

        if (t.type && t.type->deletedp()) {
            t.explicit_type = t.type->get_name();
            t.type = 0;

            exceptions[i].set_type(t);
        }
    }

    modified();
}

QString OperationData::default_cpp_decl(const QString & name)
{
    QString s = GenerationSettings::cpp_default_oper_decl();
    QString parent_name = ((BrowserNode *) browser_node->parent())->get_name();

    if ((name == parent_name) || (name == QString("~") + parent_name)) {
        // constructor or destructor
        int index;

        if ((index = s.find("${friend}")) != -1)
            s.remove(index, 9);

        if ((index = s.find("${static}")) != -1)
            s.remove(index, 9);

        if ((index = s.find("${type}")) != -1)
            s.remove(index, (s.at(index + 7) == QChar(' ')) ? 8 : 7);

        if ((index = s.find("${const}")) != -1)
            s.remove(index, 8);

        if ((index = s.find("${abstract}")) != -1)
            s.remove(index, 11);

        if ((name.at(0) != QChar('~')) && ((index = s.find("${virtual}")) != -1))
            s.remove(index, 10);
    }

    return s;
}

QString OperationData::default_cpp_def(const QString & name)
{
    QString s = GenerationSettings::cpp_default_oper_def();
    QString parent_name = ((BrowserNode *) browser_node->parent())->get_name();

    if ((name == parent_name) || (name == QString("~") + parent_name)) {
        // constructor or destructor
        int index;

        if ((index = s.find("${type}")) != -1)
            s.remove(index, (s.at(index + 7) == QChar(' ')) ? 8 : 7);

        if ((index = s.find("${const}")) != -1)
            s.remove(index, 8);

        if ((index = s.find("${staticnl}")) != -1)
            s.replace(index, 11, " ");
    }

    return s;
}

QString OperationData::default_java_def(const QString & name)
{
    QString s = GenerationSettings::java_default_oper_def();
    QString parent_name = ((BrowserNode *) browser_node->parent())->get_name();

    if (name == parent_name) {
        // constructor
        int index;

        if ((index = s.find("${static}")) != -1)
            s.remove(index, 9);

        if ((index = s.find("${abstract}")) != -1)
            s.remove(index, 11);

        if ((index = s.find("${type}")) != -1)
            s.remove(index, (s.at(index + 7) == QChar(' ')) ? 8 : 7);

        if ((index = s.find("${staticnl}")) != -1)
            s.replace(index, 11, " ");
    }
    else {
        for (const char * cpponly = "()&^[]%|!+-*/=<>~";
             *cpponly;
             cpponly += 1) {
            if (name.find(*cpponly) != -1) {
                // operator or destructor
                return QString();
            }
        }
    }

    return s;
}

QString OperationData::default_php_def(const QString & name, bool nobody)
{
    QString s = GenerationSettings::php_default_oper_def();
    QString parent_name = ((BrowserNode *) browser_node->parent())->get_name();

    if ((name == parent_name) ||
        (name == "__construct") || (name == "__destruct")) {
        // constructor destructor
        int index;

        if ((index = s.find("${static}")) != -1)
            s.remove(index, 9);
    }

    if (nobody) {
        int index = s.find("${)}");

        if (index != -1)
            s = s.left(index + 4) + ";";
    }

    return s;
}

QString OperationData::default_python_def(const QString & name)
{
    if (name == "__init__") {
        Q3ListViewItem * child;

        for (child = ((BrowserNode *) browser_node->parent())->firstChild();
             child != 0;
             child = child->nextSibling()) {
            switch (((BrowserNode *) child)->get_type()) {
            case UmlGeneralisation:
            case UmlRealize:
                return GenerationSettings::python_default_initoper_def();

            default:
                break;
            }
        }

        QString s = GenerationSettings::python_default_oper_def();
        int index = s.find("${(}");

        if (index != -1) {
            s.insert(index + 4, "${p0}${v0}");
            return s;
        }
    }

    return GenerationSettings::python_default_oper_def();
}

QString OperationData::default_idl_decl(const QString & name)
{
    QString s = GenerationSettings::idl_default_oper_decl();
    QString parent_name = ((BrowserNode *) browser_node->parent())->get_name();

    if (name == parent_name) {
        // constructor
        int index = s.find("${name}");

        if ((index != -1) &&
            (s.find("factory") == -1) &&
            (s.find("finder") == -1))
            s.insert(index, "factory init_");

        if ((index = s.find("${type}")) != -1)
            s.remove(index, (s.at(index + 7) == QChar(' ')) ? 8 : 7);
    }
    else {
        for (const char * cpponly = "()&^[]%|!+-*/=<>~";
             *cpponly;
             cpponly += 1) {
            if (name.find(*cpponly) != -1) {
                // operator or destructor
                return QString();
            }
        }
    }

    return s;
}

void OperationData::set_browser_node(BrowserOperation * o, bool update)
{
    BasicData::set_browser_node(o);

    if (update) {
        if (uml_visibility == UmlDefaultVisibility)
            uml_visibility =
                ((BrowserNode *) browser_node->parent())->get_visibility(UmlOperation);

        QString st = ((BrowserClass *) o->parent())->get_stereotype();

        if (GenerationSettings::cpp_get_default_defs()) {
            if (ClassDialog::cpp_stereotype(st) != "enum") {
                cpp_decl = default_cpp_decl(browser_node->get_name());
                cpp_def.assign(default_cpp_def(browser_node->get_name()), TRUE);
            }
            else {
                cpp_decl = "";
                cpp_def.assign(0, FALSE);
            }
        }

        if (GenerationSettings::java_get_default_defs())
            java_def.assign(default_java_def(browser_node->get_name()), TRUE);

        if (GenerationSettings::php_get_default_defs())
            php_def.assign(default_php_def(browser_node->get_name(),
                                           ClassDialog::php_stereotype(st) == "interface"),
                           TRUE);

        if (GenerationSettings::python_get_default_defs()) {
            python_def.assign(default_python_def(browser_node->get_name()),
                              TRUE);

            if (!strcmp(browser_node->get_name(), "__init__")) {
                nparams = 1;
                //params = new ParamData[1];
                params.clear();
                params << std::shared_ptr<ParamData>(new ParamData());
                params[0]->set_name("self");
            }
        }

        if (GenerationSettings::idl_get_default_defs()) {
            if (ClassDialog::idl_stereotype(st) != "enum")
                idl_decl = default_idl_decl(browser_node->get_name());
            else
                idl_decl = "";
        }
    }
}

QString OperationData::definition(bool full, bool with_kind) const
{
    QString stereot = browser_node->get_stype();
    QString definit1 =  definition(full, TRUE, TRUE, DefaultShowContextMode);
    QString definit2 =  definition(full, TRUE, TRUE, DefaultShowContextMode);
    return (with_kind) ? ("[" + stereotype + "] " + definit1).operator QString() : definit2;
}

QString OperationData::definition(bool full, bool withdir,
                                  bool withname, ShowContextMode mode) const
{
    QString result;

    if (full) {
        if (nparams == 0) {

            QString namePart = browser_node->get_name();
            QString bracePart = QString("()");
            result = namePart + bracePart;
        }
        else {
            QString namePart = browser_node->get_name();
            result = namePart;
            const char * sep = "(";
            unsigned index;

            for (index = 0; index != nparams; index += 1) {
                result += sep;
                result += params[index]->definition(withdir, withname, mode);
                sep = ", ";
            }

            result += ")";
        }

        QString rt = return_type.get_type(mode);

        return (rt.isEmpty()) ? result : result + " : " + rt;
    }
    else {
        QString namePart = browser_node->get_name();
        QString bracePart = QString("()");
        return  namePart + bracePart;
    }
}

QString OperationData::definition(bool full, DrawingLanguage language,
                                  bool withdir, bool withname,
                                  ShowContextMode mode) const
{
    switch (language) {
    case UmlView:
        return definition(full, withdir, withname, mode);

    case CppView:
        if (full)
            return OperationDialog::cpp_decl((BrowserOperation *) browser_node,
                                             withname, mode);
        else if (!cpp_decl.isEmpty())
            return definition(FALSE, FALSE);
        else
            return QString();

    case JavaView:
        if (full)
            return OperationDialog::java_decl((BrowserOperation *) browser_node,
                                              withname, mode);
        else if (!java_def.isEmpty())
            return definition(FALSE, FALSE);
        else
            return QString();

    case PhpView:
        if (full)
            return OperationDialog::php_decl((BrowserOperation *) browser_node,
                                             withname, mode);
        else if (!php_def.isEmpty())
            return definition(FALSE, FALSE);
        else
            return QString();

    case PythonView:
        if (full)
            return OperationDialog::python_decl((BrowserOperation *) browser_node,
                                                withname, mode);
        else if (!python_def.isEmpty())
            return definition(FALSE, FALSE);
        else
            return QString();

    default:
        if (full)
            return OperationDialog::idl_decl((BrowserOperation *) browser_node,
                                             withdir, withname, mode);
        else if (!idl_decl.isEmpty())
            return definition(FALSE, FALSE);
        else
            return QString();
    }
}

bool OperationData::decldefbody_contain(const QString & s, bool cs,
                                        BrowserNode *)
{
    return ((QString(get_cppdecl()).find(s, 0, cs) != -1) ||
            (QString(get_cppdef()).find(s, 0, cs) != -1) ||
            (QString(get_javadef()).find(s, 0, cs) != -1) ||
            (QString(get_phpdef()).find(s, 0, cs) != -1) ||
            (QString(get_pythondef()).find(s, 0, cs) != -1) ||
            (QString(get_idldecl()).find(s, 0, cs) != -1) ||
            (QString(get_body('c')).find(s, 0, cs) != -1) ||
            (QString(get_body('j')).find(s, 0, cs) != -1) ||
            (QString(get_body('y')).find(s, 0, cs) != -1) ||
            (QString(get_body('p')).find(s, 0, cs) != -1));
}

UmlVisibility OperationData::get_visibility(BrowserNode *)
{
    return uml_visibility;
}

void OperationData::set_uml_visibility(UmlVisibility v)
{
    uml_visibility = v;
}
//UmlVisibility OperationData::get_visibility()
//{
//    return uml_visibility;
//}

void OperationData::set_is_abstract(bool yes)
{
    is_abstract = yes;

    if (yes) {
        ClassData * d = (ClassData *)((BrowserNode *) browser_node->parent())->get_data();

        if (strcmp(d->get_stereotype(), "enum"))
            d->set_is_abstract(TRUE);
    }
    else {
        QString d = get_phpdef();
        int index;

        if (!d.isEmpty() && ((index = d.find("${)};")) != -1)) {
            QString dd = GenerationSettings::php_default_oper_def();
            int index2 = dd.find("${)}");

            if (index2 != -1) {
                d = d.left(index) + dd.mid(index2);
                php_def.assign(d, TRUE);
            }
        }
    }
}

void OperationData::set_return_type(const AType & t)
{
    if (return_type.type != t.type) {
        if ((return_type.type != 0) && unsubscribe(return_type.type->get_data()))
            disconnect(return_type.type->get_data(), 0, this, 0);

        depend_on(return_type.type = t.type);
    }

    return_type.explicit_type = t.explicit_type;
}

const char * OperationData::get_param_name(int rank) const
{
    return params[rank]->get_name();
}

QStringList OperationData::get_param_names() const
{
    QStringList result;
    for(int i(0); i < nparams; i++)
    {
        result.append(params[i]->get_name());
    }
    return result;
}

QStringList OperationData::get_param_types() const
{
    QStringList result;
    for(int i(0); i < nparams; i++)
    {
        result.append(params[i]->get_type().get_type());
    }
    return result;
}

QStringList OperationData::get_param_default_values() const
{
    QStringList result;
    for(int i(0); i < nparams; i++)
    {
        result.append(params[i]->get_default_value());
    }
    return result;
}

void OperationData::set_param_name(int rank, const char * str)
{
    params[rank]->set_name(str);
}

UmlParamDirection OperationData::get_param_dir(int rank) const
{
    return params[rank]->get_dir();
}

void OperationData::set_param_dir(int rank, UmlParamDirection dir)
{
    params[rank]->set_dir(dir);
}

const AType & OperationData::get_param_type(int rank) const
{
    return params[rank]->get_type();
}

const char * OperationData::get_param_default_value(int rank) const
{
    return params[rank]->get_default_value();
}

void OperationData::set_param_default_value(int rank, const char * str)
{
    params[rank]->set_default_value(str);
}

void OperationData::set_param_type(int rank, const AType & t)
{
    const AType & old = params[rank]->get_type();

    if (old.type != t.type) {
        if ((old.type != 0) && unsubscribe(old.type->get_data()))
            disconnect(old.type->get_data(), 0, this, 0);

        depend_on(t.type);
    }

    params[rank]->set_type(t);
}

void OperationData::set_n_params(unsigned n)
{
    int add_count = 0;
    add_count = n - params.count();
    if(add_count > 0)
    {
        for(int i(0); i < add_count; ++i)
            params << std::shared_ptr<ParamData>(new ParamData());
    }
    else if (add_count < 0)
        for(int i(0); i < qAbs(add_count); ++i)
            params.pop_back();
    nparams = params.count();
}

const AType & OperationData::get_exception(int rank) const
{
    return exceptions[rank].get_type();
}

void OperationData::set_exception(int rank, const AType & t)
{
    const AType & old = exceptions[rank].get_type();

    if (old.type != t.type) {
        if ((old.type != 0) && unsubscribe(old.type->get_data()))
            disconnect(old.type->get_data(), 0, this, 0);

        depend_on(t.type);
    }

    exceptions[rank].set_type(t);
}

void OperationData::set_n_exceptions(unsigned n)
{
    if (n > nexceptions) {
        if (exceptions)
            delete [] exceptions;

        exceptions = new ExceptionData[n];
    }

    nexceptions = n;
}

void OperationData::edit(DrawingLanguage l)
{
    setName(browser_node->get_name());

    //(new OperationDialog(this, l))->show();
    OperationDialog::Instance(this, l)->show();
}

//

void OperationData::copy_getset(const OperationData * model)
{
    cpp_get_set_frozen = model->cpp_get_set_frozen;
    java_get_set_frozen = model->java_get_set_frozen;
    php_get_set_frozen = model->php_get_set_frozen;
    python_get_set_frozen = model->python_get_set_frozen;
    idl_get_set_frozen = model->idl_get_set_frozen;

    cpp_name_spec = model->cpp_name_spec;
    java_name_spec = model->java_name_spec;
    php_name_spec = model->php_name_spec;
    python_name_spec = model->python_name_spec;
    idl_name_spec = model->idl_name_spec;
}

void OperationData::update_cpp_get_of(WrapperStr & decl, WrapperStr & def,
                                      const QString & attr_name,
                                      QString attcpp_decl, bool attis_const,
                                      QString multiplicity, bool isStatic)
{
    remove_comments(attcpp_decl);
    attcpp_decl = attcpp_decl.stripWhiteSpace();

    int index;

    if ((index = attcpp_decl.find("${comment}")) != -1)
        attcpp_decl.remove(index, 10);

    if ((index = attcpp_decl.find("${description}")) != -1)
        attcpp_decl.remove(index, 14);

    if ((index = attcpp_decl.find("${visibility}")) != -1)
        attcpp_decl.remove(index, 13);

    if ((index = attcpp_decl.find("${static}")) != -1)
        attcpp_decl.remove(index, 9);

    if ((index = attcpp_decl.find("${mutable}")) != -1)
        attcpp_decl.remove(index, 10);

    if ((index = attcpp_decl.find("${volatile}")) != -1)
        attcpp_decl.remove(index, 11);

    if ((index = attcpp_decl.find("${value}")) != -1)
        attcpp_decl.truncate(index);

    if ((index = attcpp_decl.find("${h_value}")) != -1)
        attcpp_decl.truncate(index);

    if ((index = attcpp_decl.find(";")) != -1)
        attcpp_decl.truncate(index);

    QString attr_name_spec = extract_name(attcpp_decl);

    if (attr_name_spec.isEmpty()) {
        decl = 0;
        def = 0;
    }
    else {
        int index2;
        bool has_multiplicity = FALSE;
        bool const_mult_value = FALSE;
        QString next_mult;
        QString type_spec_name_spec = attcpp_decl;
        QString type_spec_name;
        bool has_stereotype = (attcpp_decl.find("${stereotype}") != -1);

        type_spec_name_spec.replace(type_spec_name_spec.find(attr_name_spec),
                                    attr_name_spec.length(), "${name}");

        if ((index = type_spec_name_spec.find("${multiplicity}")) != -1) {
            type_spec_name_spec.remove(index, 15);

            if (! multiplicity.isEmpty()) {
                next_mult = "${const})";

                if (*((const char *) multiplicity) != '[')
                    multiplicity = "[" + multiplicity + "]";
                else if (((index = multiplicity.find(']')) != -1) &&
                         (index != (int)(multiplicity.length() - 1)))
                    // several dims
                    next_mult += multiplicity.mid(index + 1);

                has_multiplicity = TRUE;
            }
        }

        if ((index = type_spec_name_spec.find("${const}")) != -1) {
            if (attis_const || GenerationSettings::cpp_default_get_value_const()) {
                if (has_multiplicity) {
                    const_mult_value = TRUE;
                    type_spec_name_spec.remove(index, 8);
                }
                else
                    type_spec_name_spec.replace(index, 8, "const ");
            }
            else
                type_spec_name_spec.remove(index, 8);
        }

        // manage the declaration

        QString type_spec_oper_name = type_spec_name_spec;

        if ((index = type_spec_oper_name.find("${name}")) != -1) {
            if (const_mult_value) {
                if (type_spec_oper_name[index].isLetterOrNumber()) {
                    type_spec_oper_name = type_spec_name_spec.insert(index, " const ");
                    index += 7;
                }
                else {
                    type_spec_oper_name = type_spec_name_spec.insert(index, "const ");
                    index += 6;
                }
            }

            type_spec_oper_name.replace(index, 7, (has_multiplicity) ? "(* $$" : "$$");
        }

        WrapperStr d = (const char *) GenerationSettings::cpp_default_oper_decl();

        if (((index = d.find("${type}")) != -1) &&
            ((index2 = d.find("${name}", index + 1)) != -1)) {
            d.replace(index, index2 - index + 7, type_spec_oper_name);

            if ((index = d.find("${abstract}")) != -1)
                d.remove(index, 11);

            if ((index = d.find("$$")) != -1)
                d.replace(index, 2, (has_stereotype) ? "& ${name}" : "${name}");

            if (has_multiplicity || isStatic)
            {
                if ((index = d.find("${const}")) != -1)
                    d.remove(index, 8);

                if ((index = d.find("${)}")) != -1) {
                    d.insert(index + 4, (const char *) next_mult);
                }
                else
                    d = 0;
            }

            decl = d;
        }
        else
            decl = 0;

        // manage the definition

        type_spec_name = type_spec_name_spec;

        if ((index = type_spec_name.find("${name}")) != -1)
            type_spec_name.replace(index, 7,
                                   (has_multiplicity)
                                   ? "(* ${class}::$$"
                                   : (has_stereotype) ? "& ${class}::$$"
                                   : "${class}::$$");

        d = GenerationSettings::cpp_default_oper_def();

        if (((index = d.find("${type}")) != -1) &&
            ((index2 = d.find("${name}", index + 1)) != -1)) {
            d.replace(index, index2 - index + 7, type_spec_name);

            QString attr_full_name = attr_name_spec;

            if ((index = attr_full_name.find("${name}")) != -1)
                attr_full_name.replace(index, 7, attr_name);

            if ((index = d.find("$$")) != -1)
                d.replace(index, 2, "${name}");

            if (has_multiplicity || isStatic) {
                if ((index = d.find("${const}")) != -1)
                    d.remove(index, 8);

                if ((index = d.find("${)}")) != -1)
                    d.insert(index + 4, (const char *) next_mult);
                else
                    d = 0;
            }

            if ((index = d.find("${body}")) != -1) {
                QString indent;
                QString end = (d[index + 7] == '}') ? ";\n" : ";";

                if (d[index - 1] == '\n')
                    indent = "  ";

                d.replace(index, 7, indent + "return " + attr_full_name + end);
            }

            def = d;
        }
        else
            def = 0;
    }
}

void OperationData::update_java_get_of(WrapperStr & def, const QString & attr_name,
                                       QString attjava_decl, QString multiplicity)
{
    remove_comments(attjava_decl);
    attjava_decl = attjava_decl.stripWhiteSpace();

    int index;

    if ((index = attjava_decl.find("${comment}")) != -1)
        attjava_decl.remove(index, 10);

    if ((index = attjava_decl.find("${@}")) != -1)
        attjava_decl.remove(index, 4);

    if ((index = attjava_decl.find("${description}")) != -1)
        attjava_decl.remove(index, 14);

    if ((index = attjava_decl.find("${visibility}")) != -1)
        attjava_decl.remove(index, 13);

    if ((index = attjava_decl.find("${static}")) != -1)
        attjava_decl.remove(index, 9);

    if ((index = attjava_decl.find("${final}")) != -1)
        attjava_decl.remove(index, 8);

    if ((index = attjava_decl.find("${transient}")) != -1)
        attjava_decl.remove(index, 12);

    if ((index = attjava_decl.find("${volatile}")) != -1)
        attjava_decl.remove(index, 11);

    if ((index = attjava_decl.find("=")) != -1)
        attjava_decl.truncate(index);

    if ((index = attjava_decl.find("${value}")) != -1)
        attjava_decl.truncate(index);

    if ((index = attjava_decl.find(";")) != -1)
        attjava_decl.truncate(index);

    QString attr_name_spec = extract_name(attjava_decl);

    if (attr_name_spec.isEmpty())
        def = "";
    else {
        int index2;
        WrapperStr d = (const char *) GenerationSettings::java_default_oper_def();
        QString type_spec_name = attjava_decl;

        type_spec_name.replace(type_spec_name.find(attr_name_spec),
                               attr_name_spec.length(), "$$");

        if (((index = d.find("${type}")) != -1) &&
            ((index2 = d.find("${name}", index + 1)) != -1)) {
            d.replace(index, index2 - index + 7, type_spec_name);

            QString attr_full_name = attr_name_spec;

            if ((index = attr_full_name.find("${name}")) != -1)
                attr_full_name.replace(index, 7, attr_name);

            if ((index = d.find("$$")) != -1)
                d.replace(index, 2, "${name}");

            if ((index = d.find("${multiplicity}")) != -1)
                d.replace(index, 15, java_multiplicity(multiplicity));

            if ((index = d.find("${body}")) != -1) {
                QString indent;
                QString end = (d[index + 7] == '}') ? ";\n" : ";";

                if (d[index - 1] == '\n')
                    indent = "  ";

                d.replace(index, 7, indent + "return " + attr_full_name + end);
            }

            def = d;
        }
        else
            def = 0;
    }
}

void OperationData::update_php_get_of(WrapperStr & def, const QString & attr_name,
                                      QString attphp_decl)
{
    remove_comments(attphp_decl);
    attphp_decl = attphp_decl.stripWhiteSpace();

    int index;

    if ((index = attphp_decl.find("${comment}")) != -1)
        attphp_decl.remove(index, 10);

    if ((index = attphp_decl.find("${description}")) != -1)
        attphp_decl.remove(index, 14);

    if ((index = attphp_decl.find("${visibility}")) != -1)
        attphp_decl.remove(index, 13);

    if ((index = attphp_decl.find("${static}")) != -1)
        attphp_decl.remove(index, 9);

    if ((index = attphp_decl.find("${var}")) != -1)
        attphp_decl.remove(index, 6);

    if ((index = attphp_decl.find("${const}")) != -1)
        attphp_decl.remove(index, 8);

    if ((index = attphp_decl.find("${value}")) != -1)
        attphp_decl.truncate(index);

    if ((index = attphp_decl.find(";")) != -1)
        attphp_decl.truncate(index);

    QString attr_name_spec = extract_name(attphp_decl);

    if (attr_name_spec.isEmpty())
        def = "";
    else {
        WrapperStr d = (const char *) GenerationSettings::php_default_oper_def();
        QString type_spec_name = attphp_decl;

        type_spec_name.replace(type_spec_name.find(attr_name_spec),
                               attr_name_spec.length(), "$$");

        if ((index = d.find("${name}")) != -1) {
            d.replace(index, 7, type_spec_name);

            QString attr_full_name = attr_name_spec;

            if ((index = attr_full_name.find("${name}")) != -1)
                attr_full_name.replace(index, 7, attr_name);

            if ((index = d.find("$$")) != -1)
                d.replace(index, 2, "${name}");

            if ((index = d.find("${body}")) != -1) {
                QString indent;
                QString end = (d[index + 7] == '}') ? ";\n" : ";";

                if (d[index - 1] == '\n')
                    indent = "  ";

                d.replace(index, 7, indent + "return $this->" + attr_full_name + end);
            }

            def = d;
        }
        else
            def = 0;
    }
}

void OperationData::update_python_get_of(WrapperStr & def, const QString & attr_name,
        QString attpython_decl, bool attis_class_member)
{
    remove_python_comments(attpython_decl);
    attpython_decl = attpython_decl.stripWhiteSpace();

    int index;

    if ((index = attpython_decl.find("${comment}")) != -1)
        attpython_decl.remove(index, 10);

    if ((index = attpython_decl.find("${description}")) != -1)
        attpython_decl.remove(index, 14);

    if ((index = attpython_decl.find("${self}")) != -1)
        attpython_decl.remove(index, 7);

    if ((index = attpython_decl.find("${stereotype}")) != -1)
        attpython_decl.remove(index, 13);

    if ((index = attpython_decl.find("${multiplicity}")) != -1)
        attpython_decl.remove(index, 15);

    if ((index = attpython_decl.find("${value}")) != -1)
        attpython_decl.remove(index, 8);

    if ((index = attpython_decl.find("${type}")) != -1)
        attpython_decl.truncate(index);

    if ((index = attpython_decl.find(" =")) != -1)
        attpython_decl.truncate(index);
    else if ((index = attpython_decl.find("=")) != -1)
        attpython_decl.truncate(index);

    QString attr_name_spec = extract_name(attpython_decl);

    if (attr_name_spec.isEmpty())
        def = "";
    else {
        WrapperStr d = (const char *) GenerationSettings::python_default_oper_def();
        QString type_spec_name = attpython_decl;

        type_spec_name.replace(type_spec_name.find(attr_name_spec),
                               attr_name_spec.length(), "$$");

        if ((index = d.find("${name}")) != -1) {
            d.replace(index, 7, type_spec_name);

            QString attr_full_name = attr_name_spec;

            if ((index = attr_full_name.find("${name}")) != -1)
                attr_full_name.replace(index, 7, attr_name);

            if ((index = d.find("$$")) != -1)
                d.replace(index, 2, "${name}");

            if ((index = d.find("${body}")) != -1) {
                d.replace(index, 7,
                          ((attis_class_member) ? "return " : "return self.")
                          + attr_full_name);
            }

            if (!attis_class_member && (index = d.find("${(}")) != -1)
                d.insert(index + 4, "self");

            def = d;
        }
        else
            def = 0;
    }
}

void OperationData::update_idl_get_of(WrapperStr & decl, QString attidl_decl,
                                      QString multiplicity)
{
    remove_comments(attidl_decl);
    attidl_decl = attidl_decl.stripWhiteSpace();

    int index;

    if ((index = attidl_decl.find("${comment}")) != -1)
        attidl_decl.remove(index, 10);

    if ((index = attidl_decl.find("${description}")) != -1)
        attidl_decl.remove(index, 14);

    if ((index = attidl_decl.find("${visibility}")) != -1)
        attidl_decl.remove(index, 13);

    if ((index = attidl_decl.find("${readonly}")) != -1)
        attidl_decl.remove(index, 11);

    if ((index = attidl_decl.find("${attribut}")) != -1)
        // old version
        attidl_decl.remove(index, 11);

    if ((index = attidl_decl.find("${attribute}")) != -1)
        attidl_decl.remove(index, 12);

    if ((index = attidl_decl.find("const ")) != -1)
        attidl_decl.remove(index, 6);

    if ((index = attidl_decl.find("${value}")) != -1)
        attidl_decl.truncate(index);

    if ((index = attidl_decl.find(";")) != -1)
        attidl_decl.truncate(index);

    QString attr_name_spec = extract_name(attidl_decl);

    if (attr_name_spec.isEmpty())
        decl = 0;
    else {
        WrapperStr d = (const char *) GenerationSettings::idl_default_oper_decl();
        int index2;
        QString mult;
        QString type_spec_name = attidl_decl;

        type_spec_name.replace(type_spec_name.find(attr_name_spec),
                               attr_name_spec.length(), "$$");

        if (((index = d.find("${type}")) != -1) &&
            ((index2 = d.find("${name}", index + 1)) != -1)) {
            d.replace(index, index2 - index + 7, type_spec_name);

            if ((index = d.find("$$")) != -1)
                d.replace(index, 2, "${name}");

            if ((index = d.find("${multiplicity}")) != -1)
                d.replace(index, 15, multiplicity);

            decl = d;
        }
        else
            decl = 0;
    }
}

void OperationData::update_get_of(const QString & attr_name,
                                  QString attcpp_decl, QString attjava_decl,
                                  QString attphp_decl, QString attpython_decl,
                                  QString attidl_decl,
                                  bool attis_const, bool attis_class_member,
                                  const AType & cl, QString multiplicity,
                                  QString relstereotype, bool create)
{
    set_return_type(cl);
    isa_class_operation = attis_class_member;
    stereotype = relstereotype.stripWhiteSpace();
    is_get_or_set = TRUE;

    if (create)
        force_body_gen = TRUE;

    QString st = ((BrowserClass *) browser_node->parent())->get_stereotype();
    WrapperStr decl;
    WrapperStr def;

    // C++

    if (!cpp_get_set_frozen) {
        if (ClassDialog::cpp_stereotype(st) != "enum") {
            if (create) {
                if (cpp_name_spec.isEmpty())
                    cpp_name_spec = GenerationSettings::cpp_default_get_name();

                cpp_const = GenerationSettings::cpp_default_get_const();
                cpp_inline = GenerationSettings::cpp_default_get_inline();

                if (GenerationSettings::cpp_default_get_visibility() !=
                    GenerationSettings::noncpp_default_get_visibility())
                    cpp_visibility = GenerationSettings::cpp_default_get_visibility();
            }

            update_cpp_get_of(decl, def, attr_name, attcpp_decl,
                              attis_const, multiplicity, this->isa_class_operation);
            cpp_decl = decl;

            if (def.isEmpty())
                cpp_def.assign(0, TRUE);
            else
                cpp_def.assign(def, FALSE);
        }
        else {
            cpp_decl = "";
            cpp_def.assign(0, TRUE);
        }
    }

    // Java
    if (create) {
        if (java_name_spec.isEmpty())
            java_name_spec = GenerationSettings::java_default_get_name();

        java_final = GenerationSettings::java_default_get_final();
        uml_visibility = GenerationSettings::noncpp_default_get_visibility();
    }

    if (!java_get_set_frozen) {
        update_java_get_of(def, attr_name, attjava_decl, multiplicity);

        if (def.isEmpty())
            java_def.assign("", TRUE);
        else
            java_def.assign(def, FALSE);
    }

    // Php
    if (create) {
        if (php_name_spec.isEmpty())
            php_name_spec = GenerationSettings::php_default_get_name();

        php_final = GenerationSettings::php_default_get_final();
    }

    if (!php_get_set_frozen) {
        update_php_get_of(def, attr_name, attphp_decl);

        if (def.isEmpty())
            php_def.assign("", TRUE);
        else
            php_def.assign(def, FALSE);
    }

    // Python
    if (create) {
        if (python_name_spec.isEmpty())
            python_name_spec = GenerationSettings::python_default_get_name();
    }

    if (!python_get_set_frozen) {
        update_python_get_of(def, attr_name, attpython_decl, attis_class_member);

        if (def.isEmpty())
            python_def.assign("", TRUE);
        else
            python_def.assign(def, FALSE);
    }

    // Idl
    if (!idl_get_set_frozen) {
        if (ClassDialog::idl_stereotype(st) != "enum") {
            if (create && idl_name_spec.isEmpty())
                idl_name_spec = GenerationSettings::idl_default_get_name();

            update_idl_get_of(decl, attidl_decl, multiplicity);
            idl_decl = decl;
        }
        else {
            idl_decl = "";
        }
    }
}

static WrapperStr cpp_copy(const WrapperStr attr_full_name, const WrapperStr param_name,
                          WrapperStr multiplicity, const WrapperStr elt_type,
                          WrapperStr indent)
{
    int var_index = elt_type.find("$$");

    // note : param name is value or new_value
    WrapperStr src_pfix;
    WrapperStr tgt_pfix;
    WrapperStr index_pfix;

    WrapperStr s4 = attr_full_name.left(4);

    if ((s4 == "src_") || (s4 == "tgt_") || (s4 == "pos_")) {
        src_pfix = "src";
        tgt_pfix = "tgt";
        index_pfix = "pos";
    }
    else {
        src_pfix = "src_";
        tgt_pfix = "tgt_";
        index_pfix = "pos_";
    }

    WrapperStr r;
    WrapperStr srank;
    WrapperStr close;
    WrapperStr src_prev = param_name;
    WrapperStr tgt_prev = attr_full_name;
    int rank = 0;

    do {
        int index;

        if ((*((const char *) multiplicity) != '[') ||
            ((index = multiplicity.find("]")) == -1))
            return "#error invalid multiplicity part \"" + WrapperStr(multiplicity) + "\"\n";

        WrapperStr n = "(" + multiplicity.mid(1, index - 1) + ")";

        multiplicity = multiplicity.mid(index + 1).stripWhiteSpace();

        WrapperStr s;

        srank.setNum(rank);

        s = elt_type;
        s.replace(var_index, 2, src_pfix + srank);
        s.replace(s.find("%%"), 2, multiplicity);
        r += indent + s + " = " + src_prev + ";\n";
        src_prev = "*" + src_pfix + srank;

        s = elt_type;
        s.replace(var_index, 2, tgt_pfix + srank);
        s.replace(s.find("%%"), 2, multiplicity);
        r += indent + s + " = " + tgt_prev + ";\n";
        tgt_prev = "*" + tgt_pfix + srank;

        r += "\n" + indent + "for (int " + index_pfix + srank + " = " + n +
             "; " + index_pfix + srank + "--; " + src_pfix + srank + " += 1, "
             + tgt_pfix + srank + " += 1) {\n";

        close = indent + "}\n" + close;
        indent += "  ";
        rank += 1;
    }
    while (!multiplicity.isEmpty());

    r += indent + "*" + tgt_pfix + srank + " = *" +
         src_pfix + srank + ";\n" + close;

    return r;
}

void OperationData::update_cpp_set_of(WrapperStr & decl, WrapperStr & def,
                                      const QString & attr_name,
                                      QString attcpp_decl, bool attis_const,
                                      QString multiplicity)
{
    remove_comments(attcpp_decl);
    attcpp_decl = attcpp_decl.stripWhiteSpace();

    int index;

    if ((index = attcpp_decl.find("${comment}")) != -1)
        attcpp_decl.remove(index, 10);

    if ((index = attcpp_decl.find("${description}")) != -1)
        attcpp_decl.remove(index, 14);

    if ((index = attcpp_decl.find("${static}")) != -1)
        attcpp_decl.remove(index, 9);

    if ((index = attcpp_decl.find("${mutable}")) != -1)
        attcpp_decl.remove(index, 10);

    if ((index = attcpp_decl.find("${volatile}")) != -1)
        attcpp_decl.remove(index, 11);

    if ((index = attcpp_decl.find("${value}")) != -1)
        attcpp_decl.truncate(index);

    if ((index = attcpp_decl.find("${h_value}")) != -1)
        attcpp_decl.truncate(index);

    if ((index = attcpp_decl.find(";")) != -1)
        attcpp_decl.truncate(index);

    QString attr_name_spec = extract_name(attcpp_decl);

    if (attr_name_spec.isEmpty()) {
        decl = 0;
        def = 0;
    }
    else {
        bool has_stereotype = (attcpp_decl.find("${stereotype}") != -1);
        bool byref = GenerationSettings::cpp_default_set_param_ref();
        QString arg_spec = attcpp_decl;
        QString elt_type;
        bool has_multiplicity = FALSE;

        if ((index = arg_spec.find("${multiplicity}")) != -1) {
            if (! multiplicity.isEmpty()) {
                if (*((const char *) multiplicity) != '[')
                    multiplicity = "[" + multiplicity + "]";

                elt_type = arg_spec;
                elt_type.replace(index, 15, "%%");
                arg_spec.replace(index, 15, multiplicity);
                has_multiplicity = TRUE;
            }
        }

        if (byref && ((index = arg_spec.find("${type}")) != -1)) {
            // don't add ref for a pointer or a ref
            int index2 = arg_spec.find("${name}", index + 7);

            if (index2 != -1) {
                QString modifiers = arg_spec.mid(index + 7, index2 - index - 7);

                byref = (modifiers.find("*") == -1) && (modifiers.find("&") == -1);
            }
        }

        arg_spec.replace(arg_spec.find(attr_name_spec),
                         attr_name_spec.length(),
                         (has_stereotype || byref) ? "& ${p0}" : "${p0}");

        if ((index = arg_spec.find("${type}")) != -1)
            arg_spec.replace(index, 7, "${t0}");

        if ((index = arg_spec.find("${const}")) != -1) {
            if (attis_const || GenerationSettings::cpp_default_set_param_const())
                arg_spec.replace(index, 8, "const ");
            else
                arg_spec.remove(index, 8);
        }

        if (has_multiplicity) {
            elt_type.replace(elt_type.find(attr_name_spec),
                             attr_name_spec.length(), "(* $$)");

            if ((index = elt_type.find("${type}")) != -1)
                elt_type.replace(index, 7, "${t0}");

            if ((index = elt_type.find("${const}")) != -1)
                elt_type.remove(index, 8);
        }

        WrapperStr d = (const char *) GenerationSettings::cpp_default_oper_decl();

        if ((index = d.find("${)}")) != -1) {
            d.insert(index, (const char *) arg_spec);

            if ((index = d.find("${abstract}")) != -1)
                d.remove(index, 11);

            decl = d;
        }
        else
            decl = 0;

        // definition

        d = (const char *) GenerationSettings::cpp_default_oper_def();

        if ((index = d.find("${)}")) != -1) {
            d.insert(index, (const char *) arg_spec);

            QString attr_full_name = attr_name_spec;

            if ((index = attr_full_name.find("${name}")) != -1)
                attr_full_name.replace(index, 7, attr_name);

            if ((index = d.find("${body}")) != -1) {
                WrapperStr indent;

                if (has_multiplicity) {
                    if (d[index - 1] == '\n')
                        indent = "  ";
                    else {
                        int index2 = index;

                        while (index2 != 0) {
                            char c = d[index2 - 1].toAscii();

                            if ((c != ' ') && (c != '\t'))
                                break;
                            else
                                index2 -= 1;
                        }

                        if (index != index2) {
                            indent = d.mid(index2, index - index2);
                            d.remove(index2, index - index2);
                            index = index2;
                        }
                    }

                    QByteArray attr_full_name_array;
                    attr_full_name_array = attr_full_name.toLatin1();
                    const char * c_attr_full_name = attr_full_name_array.data();

                    QByteArray multiplicity_array;
                    multiplicity_array = multiplicity.toLatin1();
                    const char * c_multiplicity = multiplicity_array.data();

                    QByteArray elt_type_array;
                    elt_type_array = elt_type.toLatin1();
                    const char * c_elt_type = elt_type_array.data();


                    QByteArray indent_array;
                    indent_array = indent.operator QString().toLatin1();
                    const char * c_indent = indent_array.data();

                    d.replace(index, 7,
                              cpp_copy(
                                  WrapperStr(c_attr_full_name),
                                  params[0]->get_name(),
                                  WrapperStr(c_multiplicity),
                                  WrapperStr(c_elt_type),
                                  WrapperStr(c_indent)));
                }
                else {
                    if (d[index - 1] == '\n')
                        indent = "  ";

                    d.replace(index, 7,
                              indent + attr_full_name + " = ${p0};\n");
                }

                def = d;
            }
            else
                def = 0;
        }
        else
            def = 0;
    }
}

void OperationData::update_java_set_of(WrapperStr & def, const QString & attr_name,
                                       QString attjava_decl, QString multiplicity)
{
    remove_comments(attjava_decl);
    attjava_decl = attjava_decl.stripWhiteSpace();

    int index;

    if ((index = attjava_decl.find("${comment}")) != -1)
        attjava_decl.remove(index, 10);

    if ((index = attjava_decl.find("${@}")) != -1)
        attjava_decl.remove(index, 4);

    if ((index = attjava_decl.find("${description}")) != -1)
        attjava_decl.remove(index, 14);

    if ((index = attjava_decl.find("${visibility}")) != -1)
        attjava_decl.remove(index, 13);

    if ((index = attjava_decl.find("${static}")) != -1)
        attjava_decl.remove(index, 9);

    if ((index = attjava_decl.find("${final}")) != -1)
        attjava_decl.remove(index, 8);

    if ((index = attjava_decl.find("${transient}")) != -1)
        attjava_decl.remove(index, 12);

    if ((index = attjava_decl.find("${volatile}")) != -1)
        attjava_decl.remove(index, 11);

    if ((index = attjava_decl.find("=")) != -1)
        attjava_decl.truncate(index);

    if ((index = attjava_decl.find("${value}")) != -1)
        attjava_decl.truncate(index);

    if ((index = attjava_decl.find(";")) != -1)
        attjava_decl.truncate(index);

    QString attr_name_spec = extract_name(attjava_decl);

    if (attr_name_spec.isEmpty())
        def = 0;
    else {
        QString arg_spec = attjava_decl;

        arg_spec.replace(arg_spec.find(attr_name_spec),
                         attr_name_spec.length(), "${p0}");

        if ((index = arg_spec.find("${type}")) != -1)
            arg_spec.replace(index, 7, "${t0}");

        if ((index = arg_spec.find("${final}")) != -1) {
            if (GenerationSettings::java_default_set_param_final())
                arg_spec.replace(index, 8, "final ");
            else
                arg_spec.remove(index, 8);
        }

        WrapperStr d = (const char *) GenerationSettings::java_default_oper_def();

        if ((index = d.find("${)}")) != -1) {
            d.insert(index, (const char *) arg_spec);

            QString attr_full_name = attr_name_spec;

            if ((index = attr_full_name.find("${name}")) != -1)
                attr_full_name.replace(index, 7, attr_name);

            if ((index = d.find("${multiplicity}")) != -1)
                d.replace(index, 15, java_multiplicity(multiplicity));

            if ((index = d.find("${body}")) != -1) {
                QString indent;
                QString end = (d[index + 7] == '}') ? ";\n" : ";";

                if (d[index - 1] == '\n')
                    indent = "  ";

                d.replace(index, 7, indent + attr_full_name + " = ${p0}" + end);

                def = d;
            }
            else
                def = 0;
        }
        else
            def = 0;
    }
}

void OperationData::update_php_set_of(WrapperStr & def,
                                      const QString & attr_name,
                                      QString attphp_decl)
{
    remove_comments(attphp_decl);
    attphp_decl = attphp_decl.stripWhiteSpace();

    int index;

    if ((index = attphp_decl.find("${comment}")) != -1)
        attphp_decl.remove(index, 10);

    if ((index = attphp_decl.find("${description}")) != -1)
        attphp_decl.remove(index, 14);

    if ((index = attphp_decl.find("${visibility}")) != -1)
        attphp_decl.remove(index, 13);

    if ((index = attphp_decl.find("${static}")) != -1)
        attphp_decl.remove(index, 9);

    if ((index = attphp_decl.find("${var}")) != -1)
        attphp_decl.remove(index, 6);

    if ((index = attphp_decl.find("${const}")) != -1)
        attphp_decl.remove(index, 8);

    if ((index = attphp_decl.find("${value}")) != -1)
        attphp_decl.truncate(index);

    if ((index = attphp_decl.find(";")) != -1)
        attphp_decl.truncate(index);

    QString attr_name_spec = extract_name(attphp_decl);

    if (attr_name_spec.isEmpty())
        def = 0;
    else {
        QString arg_spec = attphp_decl;

        arg_spec.replace(arg_spec.find(attr_name_spec),
                         attr_name_spec.length(), "${p0}");

        if ((index = arg_spec.find("${final}")) != -1) {
            arg_spec.remove(index, 8);
        }

        WrapperStr d = (const char *) GenerationSettings::php_default_oper_def();

        if ((index = d.find("${)}")) != -1) {
            d.insert(index, (const char *) arg_spec);

            QString attr_full_name = attr_name_spec;

            if ((index = attr_full_name.find("${name}")) != -1)
                attr_full_name.replace(index, 7, attr_name);

            if ((index = d.find("${body}")) != -1) {
                QString indent;
                QString end = (d[index + 7] == '}') ? ";\n" : ";";

                if (d[index - 1] == '\n')
                    indent = "  ";

                d.replace(index, 7, indent + "$this->" + attr_full_name + " = ${p0}" + end);

                def = d;
            }
            else
                def = 0;
        }
        else
            def = 0;
    }
}

void OperationData::update_python_set_of(WrapperStr & def,
        const QString & attr_name,
        QString attpython_decl,
        bool attis_class_member)
{
    remove_python_comments(attpython_decl);
    attpython_decl = attpython_decl.stripWhiteSpace();

    int index;

    if ((index = attpython_decl.find("${comment}")) != -1)
        attpython_decl.remove(index, 10);

    if ((index = attpython_decl.find("${description}")) != -1)
        attpython_decl.remove(index, 14);

    if ((index = attpython_decl.find("${self}")) != -1)
        attpython_decl.remove(index, 7);

    if ((index = attpython_decl.find("${stereotype}")) != -1)
        attpython_decl.remove(index, 13);

    if ((index = attpython_decl.find("${multiplicity}")) != -1)
        attpython_decl.remove(index, 15);

    if ((index = attpython_decl.find("${value}")) != -1)
        attpython_decl.truncate(index);

    if ((index = attpython_decl.find("${type}")) != -1)
        attpython_decl.truncate(index);

    if ((index = attpython_decl.find(" =")) != -1)
        attpython_decl.truncate(index);
    else if ((index = attpython_decl.find("=")) != -1)
        attpython_decl.truncate(index);

    QString attr_name_spec = extract_name(attpython_decl);

    if (attr_name_spec.isEmpty())
        def = 0;
    else {
        QString arg_spec = attpython_decl;

        arg_spec.replace(arg_spec.find(attr_name_spec),
                         attr_name_spec.length(), "${p0}");

        WrapperStr d = (const char *) GenerationSettings::python_default_oper_def();

        if ((index = d.find("${)}")) != -1) {
            if (! attis_class_member) {
                d.insert(index, "self, ");
                index += 6;
            }

            d.insert(index, (const char *) arg_spec);

            QString attr_full_name = attr_name_spec;

            if ((index = attr_full_name.find("${name}")) != -1)
                attr_full_name.replace(index, 7, attr_name);

            if ((index = d.find("${body}")) != -1) {
                QString s;

                if (!attis_class_member)
                    s = "self.";

                d.replace(index, 7, s + attr_full_name + " = ${p0}");
                def = d;
            }
            else
                def = 0;
        }
        else
            def = 0;
    }
}

void OperationData::update_idl_set_of(WrapperStr & decl, QString attidl_decl,
                                      QString multiplicity)
{
    remove_comments(attidl_decl);
    attidl_decl = attidl_decl.stripWhiteSpace();

    int index;
    QString arg_spec;

    if ((index = attidl_decl.find("${comment}")) != -1)
        attidl_decl.remove(index, 10);

    if ((index = attidl_decl.find("${description}")) != -1)
        attidl_decl.remove(index, 14);

    if ((index = attidl_decl.find("${visibility}")) != -1)
        attidl_decl.remove(index, 13);

    if ((index = attidl_decl.find("${readonly}")) != -1)
        attidl_decl.remove(index, 11);

    if ((index = attidl_decl.find("${attribut}")) != -1)
        // old version
        attidl_decl.remove(index, 11);

    if ((index = attidl_decl.find("${attribute}")) != -1)
        attidl_decl.remove(index, 12);

    if ((index = attidl_decl.find("const ")) != -1)
        attidl_decl.remove(index, 6);

    if ((index = attidl_decl.find("${value}")) != -1)
        attidl_decl.truncate(index);

    if ((index = attidl_decl.find(";")) != -1)
        attidl_decl.truncate(index);

    QString attr_name_spec = extract_name(attidl_decl);

    if (attr_name_spec.isEmpty())
        decl = 0;
    else {
        QString arg_spec = attidl_decl;

        arg_spec.replace(arg_spec.find(attr_name_spec),
                         attr_name_spec.length(), "${p0}");

        if ((index = arg_spec.find("${type}")) != -1)
            arg_spec.replace(index, 7, "${t0}");

        WrapperStr d = (const char *) GenerationSettings::idl_default_oper_decl();

        if ((index = d.find("${)}")) == -1)
            decl = 0;
        else {
            d.insert(index, (const char *) arg_spec);

            if ((index = d.find("${multiplicity}")) != -1)
                d.replace(index, 15, multiplicity);

            decl = d;
        }
    }
}

void OperationData::update_set_of(const QString & attr_name,
                                  QString attcpp_decl, QString attjava_decl,
                                  QString attphp_decl, QString attpython_decl,
                                  QString attidl_decl,
                                  bool attis_const, bool attis_class_member,
                                  const AType & cl, QString multiplicity,
                                  QString relstereotype, bool create)
{
//#warning warning si attis_const;

    stereotype = relstereotype.stripWhiteSpace();
    return_type.type = 0;
    return_type.explicit_type = "void";
    isa_class_operation = attis_class_member;
    is_get_or_set = TRUE;

    if (nparams == 0) {
        set_n_params(1);
        set_param_dir(0, UmlIn);
    }

    set_param_name(0, (attr_name == "value") ? "new_value" : "value");
    set_param_type(0, cl);

    if (create)
        force_body_gen = TRUE;

    QString st = ((BrowserClass *) browser_node->parent())->get_stereotype();
    WrapperStr decl;
    WrapperStr def;

    // C++
    if (!cpp_get_set_frozen) {
        if (ClassDialog::cpp_stereotype(st) != "enum") {
            if (create) {
                if (cpp_name_spec.isEmpty())
                    cpp_name_spec = GenerationSettings::cpp_default_set_name();

                cpp_inline = GenerationSettings::cpp_default_set_inline();

                if (GenerationSettings::cpp_default_set_visibility() !=
                    GenerationSettings::noncpp_default_set_visibility())
                    cpp_visibility = GenerationSettings::cpp_default_set_visibility();
            }

            update_cpp_set_of(decl, def, attr_name, attcpp_decl,
                              attis_const, multiplicity);
            cpp_decl = decl;

            if (def.isEmpty())
                cpp_def.assign("", TRUE);
            else
                cpp_def.assign(def, FALSE);
        }
        else {
            cpp_decl = "";
            cpp_def.assign(0, TRUE);
        }
    }

    // Java
    if (create) {
        if (java_name_spec.isEmpty())
            java_name_spec = GenerationSettings::java_default_set_name();

        java_final = GenerationSettings::java_default_set_final();
        uml_visibility = GenerationSettings::noncpp_default_set_visibility();
    }

    if (!java_get_set_frozen) {
        update_java_set_of(def, attr_name, attjava_decl, multiplicity);

        if (def.isEmpty())
            java_def.assign("", TRUE);
        else
            java_def.assign(def, FALSE);
    }

    // Php
    if (create) {
        if (php_name_spec.isEmpty())
            php_name_spec = GenerationSettings::php_default_set_name();

        php_final = GenerationSettings::php_default_set_final();
    }

    if (!php_get_set_frozen) {
        update_php_set_of(def, attr_name, attphp_decl);

        if (def.isEmpty())
            php_def.assign("", TRUE);
        else
            php_def.assign(def, FALSE);
    }

    // Python
    if (create) {
        if (python_name_spec.isEmpty())
            python_name_spec = GenerationSettings::python_default_set_name();
    }

    if (!python_get_set_frozen) {
        update_python_set_of(def, attr_name, attpython_decl, attis_class_member);

        if (def.isEmpty())
            python_def.assign("", TRUE);
        else
            python_def.assign(def, FALSE);
    }

    // Idl
    if (!idl_get_set_frozen) {
        if (ClassDialog::idl_stereotype(st) != "enum") {
            if (create) {
                if (idl_name_spec.isEmpty())
                    idl_name_spec = GenerationSettings::idl_default_set_name();

                idl_oneway = GenerationSettings::idl_default_set_oneway();
            }

            update_idl_set_of(decl, attidl_decl, multiplicity);
            idl_decl = decl;
        }
        else {
            idl_decl = "";
        }
    }
}

//

bool OperationData::reference(BrowserClass * target) const
{
    if (return_type.type == target)
        return TRUE;

    unsigned i;

    for (i = 0; i != nparams; i += 1)
        if (params[i]->get_type().type == target)
            return TRUE;

    for (i = 0; i != nexceptions; i += 1)
        if (exceptions[i].get_type().type == target)
            return TRUE;

    return FALSE;
}

//

void OperationData::replace(BrowserClass * old, BrowserClass * nw)
{
    AType t;

    t.type = nw;

    if (return_type.type == old)
        set_return_type(t);

    unsigned i;

    for (i = 0; i != nparams; i += 1)
        if (params[i]->get_type().type == old)
            params[i]->set_type(t);

    for (i = 0; i != nexceptions; i += 1)
        if (exceptions[i].get_type().type == old)
            // almost no chance to be true ...
            exceptions[i].set_type(t);
}

//

void OperationData::send_uml_def(ToolCom * com, BrowserNode * bn,
                                 const QString & comment)
{
    int api = com->api_format(true);

    BasicData::send_uml_def(com, bn, comment);
    com->write_bool(isa_class_operation);

    if (api >= 13)
        com->write_bool(is_volatile);

    com->write_char(((api >= 23) ||
                     (uml_visibility != UmlPackageVisibility))
                    ? uml_visibility : UmlPublic);

    if (api >= 30)
        com->write_string(constraint);

    return_type.send_def(com);
    com->write_bool(is_abstract);

    if (api >= 35)
        com->write_bool(force_body_gen);

    unsigned n;

    com->write_unsigned(params.count());
    n = nparams;
    for(auto& param : params)
    {
        param->send_uml_def(com);
        n -= 1;
    }

    ExceptionData * ep;

    com->write_unsigned(nexceptions);

    for (ep = exceptions, n = nexceptions; n; ep += 1, n -= 1)
        ep->send_uml_def(com);

    if (((BrowserOperation *) browser_node)->get_get_of() != 0)
        ((BrowserOperation *) browser_node)->get_get_of()->write_id(com);
    else
        com->write_id(0);

    if (((BrowserOperation *) browser_node)->get_set_of() != 0)
        ((BrowserOperation *) browser_node)->get_set_of()->write_id(com);
    else
        com->write_id(0);
}

void OperationData::send_cpp_def(ToolCom * com)
{
    com->write_string(cpp_decl);

    if (com->api_format() >= 23)
        com->write_char(cpp_visibility);
    else {
        switch (cpp_visibility) {
        case UmlPackageVisibility:
            com->write_char(UmlPublic);
            break;

        case UmlDefaultVisibility:
            com->write_char(UmlDefaultVisibility - 1);
            break;

        default:
            com->write_char(cpp_visibility);
        }
    }

    if (com->api_format() < 13)
        com->write_bool(is_volatile);

    com->write_bool(cpp_const);
    com->write_bool(cpp_friend);
    com->write_bool(cpp_virtual);
    com->write_bool(cpp_inline);
    QLOG_INFO() << "writing defaulted";
    if(api_format(true) > 76)
    {
        com->write_bool(cpp_default);
        com->write_bool(cpp_delete);
        com->write_bool(cpp_override);
        com->write_bool(cpp_final);
    }
    com->write_string(cpp_def);
    com->write_string(cpp_name_spec);

    if (com->api_format() >= 26)
        com->write_bool(cpp_get_set_frozen);

    if (com->api_format() >= 37)
        com->write_bool(cpp_indent_body);
}

void OperationData::send_java_def(ToolCom * com)
{
    com->write_string(java_def);

    if (com->api_format() >= 21)
        com->write_string(java_annotation);

    com->write_bool(java_final);
    com->write_bool(java_synchronized);
    com->write_string(java_name_spec);

    if ((com->api_format() >= 19) && (com->api_format() < 21))
        com->write_string(java_annotation);

    if (com->api_format() >= 26)
        com->write_bool(java_get_set_frozen);

    if (com->api_format() >= 37)
        com->write_bool(java_indent_body);
}

void OperationData::send_php_def(ToolCom * com)
{
    com->write_string(php_def);
    com->write_bool(php_final);
    com->write_string(php_name_spec);
    com->write_bool(php_get_set_frozen);

    if (com->api_format() >= 37)
        com->write_bool(php_indent_body);
}

void OperationData::send_python_def(ToolCom * com)
{
    com->write_string(python_def);
    com->write_string(python_decorator);
    com->write_string(python_name_spec);
    com->write_bool(python_get_set_frozen);
    com->write_bool(python_indent_body);
}

void OperationData::send_idl_def(ToolCom * com)
{
    com->write_string(idl_decl);
    com->write_bool(idl_oneway);
    com->write_string(idl_name_spec);

    if (com->api_format() >= 26)
        com->write_bool(idl_get_set_frozen);
}

//

// translate plug out release < 2.0
void OperationData::convert(OperationData * comp, OperationData * art)
{
    // UmlComponent constructor definition
    comp->cpp_decl =
        "    ${comment}${name}${(}${t0} ${p0}, const ${t1} & ${p1}${)} : UmlBaseComponent(id, n) {};\n";
    comp->cpp_def.assign("", TRUE);
    comp->java_def.assign("  ${comment}${@}${visibility}${final}${name}${(}${t0} ${p0}, ${t1} ${p1}${)}${throws}{ super(id, n); }\n",
                          FALSE);

    // replace call to UmlBaseComponent by a call to UmlBaseArtifact
    // in the UmlArtifact contructor

    int index;
    WrapperStr s = art->cpp_decl;

    if ((index = s.find("UmlBaseComponent")) != -1) {
        s.replace(index + 7, 9, "Artifact");
        art->cpp_decl = s;
    }
    else if (strstr((const char *) art->cpp_def, "${body}") == 0) {
        s = (const char *) art->cpp_def;

        if ((index = s.find("UmlBaseComponent")) != -1) {
            s.replace(index + 7, 9, "UmlArtifact");
            art->cpp_def.assign(s, FALSE);
        }
    }
    else {
        char * b = art->get_body('c');

        if (b != 0) {
            s = b;

            if ((index = s.find("UmlBaseComponent")) != -1) {
                s.replace(index + 7, 9, "Artifact");
                art->new_body(s, 'c');
            }

            delete [] b;
        }
    }
}

//

bool OperationData::tool_cmd(ToolCom * com, const char * args,
                             BrowserNode * bn,
                             const QString & comment)
{
    if (((unsigned char) args[-1]) >= firstSetCmd) {
        if (!bn->is_writable() && !root_permission())
            com->write_ack(FALSE);
        else {
            switch ((unsigned char) args[-1]) {
            case setIsForceBodyGenCmd:
                force_body_gen = (*args != 0);
                bn->package_modified();
                com->write_ack(TRUE);
                // useless to say the operation is modified
                QLOG_DEBUG() << "not modified";
                return TRUE;

            case setIsClassMemberCmd:
                isa_class_operation = (*args != 0);
                break;

            case setIsVolatileCmd:
                is_volatile = (*args != 0);
                break;

            case setVisibilityCmd: {
                UmlVisibility v;

                if (! com->get_visibility(v, args)) {
                    com->write_ack(FALSE);
                    return TRUE;
                }
                else
                    uml_visibility = v;
            }
            break;

            case setIsAbstractCmd:
                is_abstract = (*args != 0);
                break;

            case setReturnTypeCmd: {
                AType t;

                com->get_type(t, args);
                set_return_type(t);
            }
            break;

            case setConstraintCmd:
                constraint = args;
                break;

            case setCppVisibilityCmd: {
                UmlVisibility v;

                if (! com->get_extended_visibility(v, args)) {
                    com->write_ack(FALSE);
                    return TRUE;
                }
                else
                    cpp_visibility = v;
            }
            break;

            case setCppDeclCmd:
                cpp_decl = args;
                break;

            case setIsCppConstCmd:
                cpp_const = (*args != 0);
                break;

            case setIsCppFriendCmd:
                cpp_friend = (*args != 0);
                break;

            case setIsCppDefaultCmd:
                cpp_default = (*args != 0);
                break;

            case setIsCppDeleteCmd:
                cpp_delete = (*args != 0);
                break;

            case setIsCppOverrideCmd:
                cpp_override = (*args != 0);
                break;

            case setIsCppFinalCmd:
                cpp_final = (*args != 0);
                break;

            case setIsCppVirtualCmd:
                cpp_virtual = (*args != 0);
                break;

            case setIsCppInlineCmd:
                cpp_inline = (*args != 0);
                break;

            case setCppDefCmd:
                cpp_def.assign(args, is_abstract || (strstr(args, "${body}") != 0));
                break;

            case setCppBodyCmd: {
                char * b = get_body('c');

                if (b != 0) {
                    if (strcmp(b, args))
                        new_body(args, 'c');

                    delete [] b;
                }
                else if (*args)
                    new_body(args, 'c');
            }
            break;

            case setCppNameSpecCmd:
                cpp_name_spec = args;
                break;

            case setCppFrozenCmd:
                cpp_get_set_frozen = (*args != 0);
                break;

            case setJavaDeclCmd: {
                QString ste = GenerationSettings::java_class_stereotype(stereotype);

                java_def.assign(args,
                                is_abstract ||
                                (ste == "interface") ||
                                (ste == "@interface") ||
                                (strstr(args, "${body}") != 0));
            }
            break;

            case setJavaAnnotationCmd: {
                WrapperStr s = args;

                s = s.stripWhiteSpace();

                if (! s.isEmpty())
                    s += '\n';

                java_annotation = s;
            }
            break;

            case setJavaFinalCmd:
                java_final = (*args != 0);
                break;

            case setJavaSynchronizedCmd:
                java_synchronized = (*args != 0);
                break;

            case setJavaBodyCmd: {
                char * b = get_body('j');

                if (b != 0) {
                    if (strcmp(b, args))
                        new_body(args, 'j');

                    delete [] b;
                }
                else if (*args)
                    new_body(args, 'j');
            }
            break;

            case setJavaNameSpecCmd:
                java_name_spec = args;
                break;

            case setJavaFrozenCmd:
                java_get_set_frozen = (*args != 0);
                break;

            case setPhpDeclCmd:
                php_def.assign(args, (strstr(args, "${body}") != 0));
                break;

            case setPhpBodyCmd: {
                char * b = get_body('p');

                if (b != 0) {
                    if (strcmp(b, args))
                        new_body(args, 'p');

                    delete [] b;
                }
                else if (*args)
                    new_body(args, 'p');
            }
            break;

            case setPhpNameSpecCmd:
                php_name_spec = args;
                break;

            case setPhpFrozenCmd:
                php_get_set_frozen = (*args != 0);
                break;

            case setPhpFinalCmd:
                php_final = (*args != 0);
                break;

            case setPythonDeclCmd:
                python_def.assign(args, (strstr(args, "${body}") != 0));
                break;

            case setPythonDecoratorsCmd:
                python_decorator = args;
                break;

            case setPythonBodyCmd: {
                char * b = get_body('y');

                if (b != 0) {
                    if (strcmp(b, args))
                        new_body(args, 'y');

                    delete [] b;
                }
                else if (*args)
                    new_body(args, 'y');
            }
            break;

            case setPythonNameSpecCmd:
                python_name_spec = args;
                break;

            case setPythonFrozenCmd:
                python_get_set_frozen = (*args != 0);
                break;

            case setIsIdlOnewayCmd:
                idl_oneway = (*args != 0);
                break;

            case setIdlDeclCmd:
                idl_decl = args;
                break;

            case setIdlNameSpecCmd:
                idl_name_spec = args;
                break;

            case setIdlFrozenCmd:
                idl_get_set_frozen = (*args != 0);
                break;

            case addParameterCmd: {
                unsigned rank = com->get_unsigned(args);
                UmlParamDirection dir;
                bool valid_dir = com->get_direction(dir, args);

                if ((rank > nparams) || !valid_dir || (dir == UmlReturn)) {
                    com->write_ack(FALSE);
                    return TRUE;
                }

                WrapperStr name = com->get_string(args);
                WrapperStr dflt = com->get_string(args);
                AType t;

                com->get_type(t, args);

                params.insert(rank, std::shared_ptr<ParamData> (new ParamData()));
                params[rank]->set_name(name);
                params[rank]->set_dir(dir);
                params[rank]->set_default_value((dflt.isEmpty()) ? "" : (const char *) dflt);
                params[rank]->set_type(t);
                depend_on(t.type);
                nparams += 1;
            }
            break;

            case replaceParameterCmd: {
                unsigned rank = com->get_unsigned(args);
                UmlParamDirection dir;
                bool valid_dir = com->get_direction(dir, args);

                if ((rank >= nparams) || !valid_dir || (dir == UmlReturn)) {
                    com->write_ack(FALSE);
                    return TRUE;
                }

                WrapperStr name = com->get_string(args);
                WrapperStr dflt = com->get_string(args);
                AType t;

                com->get_type(t, args);

                params[rank]->set_name(name);
                params[rank]->set_dir(dir);
                params[rank]->set_default_value((dflt.isEmpty()) ? "" : (const char *) dflt);
                set_param_type(rank, t);
            }
            break;

            case removeParameterCmd: {
                unsigned rank = com->get_unsigned(args);

                if (rank >= nparams) {
                    com->write_ack(FALSE);
                    return TRUE;
                }

                while (++rank != nparams)
                    *params[rank - 1] = *params[rank];

                nparams -= 1;
            }
            break;

            case addExceptionCmd: {
                unsigned rank = com->get_unsigned(args);
                AType t;

                com->get_type(t, args);

                if (rank > nexceptions) {
                    com->write_ack(FALSE);
                    return TRUE;
                }

                ExceptionData * new_exceptions = new ExceptionData[nexceptions + 1];
                unsigned index;

                for (index = 0; index != rank; index += 1)
                    new_exceptions[index] = exceptions[index];

                new_exceptions[index].set_type(t);
                depend_on(t.type);

                while (index != nexceptions) {
                    new_exceptions[index + 1] = exceptions[index];
                    index += 1;
                }

                delete [] exceptions;
                exceptions = new_exceptions;
                nexceptions += 1;
            }
            break;

            case replaceExceptionCmd: {
                unsigned rank = com->get_unsigned(args);
                AType t;

                com->get_type(t, args);

                if (rank >= nexceptions) {
                    com->write_ack(FALSE);
                    return TRUE;
                }

                set_exception(rank, t);
            }
            break;

            case removeExceptionCmd: {
                unsigned rank = com->get_unsigned(args);

                if (rank >= nexceptions) {
                    com->write_ack(FALSE);
                    return TRUE;
                }

                while (++rank != nexceptions)
                    exceptions[rank - 1] = exceptions[rank];

                nexceptions -= 1;
            }
            break;

            case setCppContextualBodyIndentCmd:
                cpp_indent_body = (*args != 0);
                break;

            case setJavaContextualBodyIndentCmd:
                java_indent_body = (*args != 0);
                break;

            case setPhpContextualBodyIndentCmd:
                php_indent_body = (*args != 0);
                break;

            case setPythonContextualBodyIndentCmd:
                python_indent_body = (*args != 0);
                break;

            default:
                return BasicData::tool_cmd(com, args, bn, comment);
            }

            // ok case
            bn->modified();
            bn->package_modified();
            modified();
            com->write_ack(TRUE);
        }
    }
    else {
        switch ((unsigned char) args[-1]) {
        case cppBodyCmd: {
            char * body = get_body('c');

            if (body != 0) {
                com->write_string(body);
                delete [] body;
            }
            else
                com->write_string("");
        }
        break;

        case javaBodyCmd: {
            char * body = get_body('j');

            if (body != 0) {
                com->write_string(body);
                delete [] body;
            }
            else
                com->write_string("");

            return TRUE;
        }
        break;

        case phpBodyCmd: {
            char * body = get_body('p');

            if (body != 0) {
                com->write_string(body);
                delete [] body;
            }
            else
                com->write_string("");

            return TRUE;
        }
        break;

        case pythonBodyCmd: {
            char * body = get_body('y');

            if (body != 0) {
                com->write_string(body);
                delete [] body;
            }
            else
                com->write_string("");

            return TRUE;
        }
        break;

        default:
            return BasicData::tool_cmd(com, args, bn, comment);
        }
    }

    return TRUE;
}

//

void OperationData::set_bodies_info()
{
    BrowserClass * cl = (BrowserClass *) browser_node->parent();
    char * s = set_bodies_info(cl, cl->get_ident());

    if (s != 0)
        delete [] s;
}

// id may be the old cl's ident in case of an import
char * OperationData::set_bodies_info(BrowserClass * cl, int id)
{
    cl->set_bodies_read(TRUE);

    // no bodies by default
    for (Q3ListViewItem * child = cl->firstChild(); child; child = child->nextSibling()) {
        if (((BrowserNode *) child)->get_type() == UmlOperation) {
            OperationData * d = (OperationData *)((BrowserNode *) child)->get_data();

            d->cpp_body.length =
                d->java_body.length =
                    d->python_body.length = 0;
            d->php_body.length = 0;
        }
    }

    char * s = read_file(QString::number(id) + ".bodies");

    if (s == 0)
        return 0;

    char * p = strstr(s, "\n!!!");

    if (p == 0) {
        // no body
        delete [] s;
        return 0;
    }

    // some operations have a body
    OperationBody * b = 0;
    char * start = 0;

    for (;;) {
        p += 4;

        char * p2;

        if (sscanf(p, "%d", &id) == 1) {
            p2 = p + QString::number(id).length();

            bool find = FALSE;
            OperationData * d;

            if (strncmp(p2, ".cpp!!!\t", 8) == 0) {
                if (b != 0)
                    b->length = p - start - 3;

                if (((d = all[id]) != 0) && (d->browser_node->parent() == cl)) {
                    b = &d->cpp_body;
                    find = TRUE;
                }
                else
                    // !! wrong file
                    b = 0;
            }

            if (strncmp(p2, ".java!!!\t", 9) == 0) {
                if (b != 0)
                    b->length = p - start - 3;

                if (((d = all[id]) != 0) && (d->browser_node->parent() == cl)) {
                    b = &d->java_body;
                    find = TRUE;
                }
                else
                    // !! wrong file
                    b = 0;
            }

            if (strncmp(p2, ".php!!!\t", 8) == 0) {
                if (b != 0)
                    b->length = p - start - 3;

                if (((d = all[id]) != 0) && (d->browser_node->parent() == cl)) {
                    b = &d->php_body;
                    find = TRUE;
                }
                else
                    // !! wrong file
                    b = 0;
            }

            if (strncmp(p2, ".python!!!\t", 11) == 0) {
                if (b != 0)
                    b->length = p - start - 3;

                if (((d = all[id]) != 0) && (d->browser_node->parent() == cl)) {
                    b = &d->python_body;
                    find = TRUE;
                }
                else
                    // !! wrong file
                    b = 0;
            }

            if (find) {
                if ((p = strchr(p2, '\n')) != 0) {
                    start = p + 1;
                    b->offset = start - s;
                }
                else
                    // wrong file
                    break;
            }
        }

        p2 = strstr(p, "\n!!!");

        if (p2 == 0) {
            if (b != 0)
                b->length = p + strlen(p) - start;

            break;
        }

        p = p2;
    }

    return s;
}

void OperationData::create_modified_body_file()
{
    if (browser_node == 0)
        return;

    BrowserClass * cl = (BrowserClass *) browser_node->parent();

    if (! cl->get_bodies_modified()) {
        for (;;) {
            QString fn = abs_file_path(cl->get_ident(), "b");
            QFile fp(fn);

            while (!fp.open(QIODevice::WriteOnly))
                (void) msg_critical("Error", QString("Cannot create file\n") + fn,
                                    QMessageBox::Retry);

            cl->set_bodies_modified(TRUE);

            if (! cl->get_bodies_read())
                set_bodies_info();

            char * old = read_file(QString::number(cl->get_ident()) + ".bodies");

            if (old != 0) {
                fp.writeBlock(old, strlen(old));
                delete [] old;
            }
            else {
                QString header =
                    QString("class ") + cl->get_data()->definition(TRUE, FALSE) + '\n';

                fp.writeBlock(header, header.length());
            }

            fp.close();

            if (static_cast<uint>(fp.status()) == IO_Ok)
                // all is ok
                return;

            (void) msg_critical("Error", QString("Error while writting in\n") + fn +
                                "\nmay be your disk is full",
                                QMessageBox::Retry);
        }
    }
}

char * OperationData::get_body(int who)
{
    OperationBody * body_info;

    switch (who) {
    case 'c':
        body_info = &cpp_body;
        break;

    case 'j':
        body_info = &java_body;
        break;

    case 'p':
        body_info = &php_body;
        break;

    default: // 'y'
        body_info = &python_body;
    }

    switch (body_info->length) {
    case -1:
        set_bodies_info();
        break;

    case 0:
        return 0;
    }

    return read_definition(((BrowserClass *) browser_node->parent())->get_ident(),
                           "bodies", body_info->offset, body_info->length);
}

void OperationData::new_body(QString s, int who)
{
    OperationBody * body_info;
    QString key;

    switch (who) {
    case 'c':
        body_info = &cpp_body;
        key = ".cpp!!!\t";
        break;

    case 'j':
        body_info = &java_body;
        key = ".java!!!\t";
        break;

    case 'p':
        body_info = &php_body;
        key = ".php!!!\t";
        break;

    default: // 'y'
        body_info = &python_body;
        key = ".python!!!\t";
    }

    if (!((BrowserClass *) browser_node->parent())->get_bodies_read())
        set_bodies_info();

    BrowserClass * cl = (BrowserClass *) browser_node->parent();
    QString fn = abs_file_path(cl->get_ident(), "b");

    for (;;) {
        QFile fp(fn);

        while (!fp.open(QIODevice::WriteOnly | QIODevice::Append))
            (void) msg_critical("Error", QString("Cannot create file\n") + fn,
                                QMessageBox::Retry);

        cl->set_bodies_modified(TRUE);

        if (fp.at() == 0) {
            char * old = read_file(QString::number(cl->get_ident()) + ".bodies");

            if (old != 0) {
                fp.writeBlock(old, strlen(old));
                delete [] old;
            }
            else {
                QString header =
                    QString("class ") + cl->get_data()->definition(TRUE, FALSE) + '\n';

                fp.writeBlock(header, header.length());
            }
        }

        if (!s.isEmpty()) {
            if (s.at(s.length() - 1) != QChar('\n'))
                s += '\n';

            QString op_header = QString("!!!") + QString::number(get_ident()) +
                                key + definition(TRUE, FALSE) + "\n";

            fp.writeBlock(op_header, op_header.length());

            body_info->offset = fp.at();
            body_info->length = s.length();

            fp.writeBlock(s, s.length());
        }
        else
            body_info->length = 0;

        if (static_cast<uint>(fp.status()) == IO_Ok)
            // all is ok
            return;

        (void) msg_critical("Error", QString("Error while writting in\n") + fn +
                            "\nmay be your disk is full",
                            QMessageBox::Retry);
    }
}

void OperationData::save_body(QFile & qf, QString & filename,
                              bool dobackup, char * modified_bodies,
                              int who)
{
    OperationBody * body_info;
    QString key;

    switch (who) {
    case 'c':
        body_info = &cpp_body;
        key = ".cpp!!!\t";
        break;

    case 'j':
        body_info = &java_body;
        key = ".java!!!\t";
        break;

    case 'p':
        body_info = &php_body;
        key = ".php!!!\t";
        break;

    default: // 'y'
        body_info = &python_body;
        key = ".python!!!\t";
    }

    if (body_info->length > 0) {
        if (!qf.isOpen()) {
            BrowserClass * cl = (BrowserClass *) browser_node->parent();
            QDir d = BrowserView::get_dir();

            filename = abs_file_path(cl->get_ident(),
                                     (in_import()) ? "b" : "bodies");

            if (dobackup)
                backup(d, filename);

            qf.setName(filename);

            while (!qf.open(QIODevice::WriteOnly))
                (void) msg_critical("Error", QString("Cannot create file\n") + filename,
                                    QMessageBox::Retry);

            QString header =
                QString("class ") + cl->get_data()->definition(TRUE, FALSE) + '\n';

            qf.writeBlock(header, header.length());
        }

        QString op_header = QString("!!!") + QString::number(get_ident()) +
                            key + definition(TRUE, FALSE) + "\n";

        qf.writeBlock(op_header, op_header.length());

        int new_offset = qf.at();

        qf.writeBlock(modified_bodies + body_info->offset,
                      body_info->length);
        body_info->offset = new_offset;
    }
}

void OperationData::raz_body()
{
    cpp_body.length =
        java_body.length =
            python_body.length = 0;
    php_body.length = 0;
}

// save all the operations's body of cl, id is its old ident
void OperationData::import(BrowserClass * cl, int id)
{
    char * s = set_bodies_info(cl, id);

    if (s == 0)
        return;

    bool dobackup = TRUE;

    for (;;) {
        QFile qf;
        QString filename;

        for (Q3ListViewItem * child = cl->firstChild(); child; child = child->nextSibling()) {
            if (((BrowserNode *) child)->get_type() == UmlOperation) {
                OperationData * d = (OperationData *)((BrowserNode *) child)->get_data();

                d->save_body(qf, filename, dobackup, s, 'c');
                d->save_body(qf, filename, dobackup, s, 'j');
                d->save_body(qf, filename, dobackup, s, 'p');
                d->save_body(qf, filename, dobackup, s, 'y');
            }
        }

        if (filename.isEmpty())
            // no body
            break;

        qf.close();

        if (static_cast<uint>(qf.status()) == IO_Ok)
            // all is ok
            break;

        (void) msg_critical("Error", QString("Error while writting in\n") + filename +
                            "\nmay be your disk is full",
                            QMessageBox::Retry);
        dobackup = FALSE;
    }

    delete [] s;
}

void OperationData::save(QTextStream & st, bool ref, QString & warning) const
{
    if (ref) {
        st << "operation_ref " << get_ident() << " // ";
        save_string(definition(TRUE, FALSE), st);
    }
    else {
        BasicData::save(st, warning);

        nl_indent(st);

        if (isa_class_operation)
            st << "class_operation ";
        else if (is_abstract)
            st << "abstract ";

        if (force_body_gen)
            st << "force_body_gen ";

        if (is_volatile)
            st << "volatile ";

        if (! cpp_indent_body)
            st << "preserve_cpp_body_indent ";

        if (cpp_get_set_frozen)
            st << "cpp_frozen ";

        if (cpp_const)
            st << "const ";

        QSettings settings(QSettings::IniFormat, QSettings::UserScope, "DoUML", "settings");
        settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
        if(settings.value("Main/compatibility_save").toInt() != 1)
        {
            if (cpp_default)
                st << "default ";

            if (cpp_delete)
                st << "delete ";

            if (cpp_override)
                st << "override ";

            if (cpp_final)
                st << "final ";
        }

        if (cpp_friend)
            st << "friend ";

        if (cpp_virtual)
            st << "cpp_virtual ";

        if (cpp_inline)
            st << "cpp_inline ";

        st << stringify(uml_visibility);

        return_type.save(st, warning, " return_type ", " explicit_return_type ");
        nl_indent(st);

        st << "nparams " << nparams;
        nl_indent(st);

        if(settings.value("Main/compatibility_save").toInt() != 1)
        {

            st << "origin ";
            save_string(originClass, st);
        }


        for (unsigned i = 0; i != nparams; i += 1)
            params[i]->save(st, warning);

        if (nexceptions != 0) {
            nl_indent(st);
            st << "nexceptions " << nexceptions;

            for (unsigned i = 0; i != nexceptions; i += 1)
                exceptions[i].save(st, warning);
        }

        if (!constraint.isEmpty()) {
            nl_indent(st);
            st << "constraint ";
            save_string(constraint, st);
        }

        nl_indent(st);

        if (cpp_visibility != UmlDefaultVisibility)
            st << "cpp_visibility " << stringify(cpp_visibility) << " ";

        if (! cpp_decl.isEmpty()) {
            st << "cpp_decl ";
            save_string(cpp_decl, st);
        }

        if (/*!is_abstract &&*/ !cpp_def.isEmpty()) {
            nl_indent(st);
            st << "cpp_def ";
            save_string(cpp_def, st);
        }

        if (!cpp_name_spec.isEmpty()) {
            nl_indent(st);
            st << "cpp_name_spec ";
            save_string(cpp_name_spec, st);
        }

        nl_indent(st);

        if (! java_indent_body)
            st << "preserve_java_body_indent ";

        if (java_get_set_frozen)
            st << "java_frozen ";

        if (java_final)
            st << "final ";

        if (java_synchronized)
            st << "synchronized ";

        if (! java_def.isEmpty()) {
            st << "java_def ";
            save_string(java_def, st);
        }

        if (! java_annotation.isEmpty()) {
            nl_indent(st);
            st << "java_annotation ";
            save_string(java_annotation, st);
        }

        if (!java_name_spec.isEmpty()) {
            nl_indent(st);
            st << "java_name_spec ";
            save_string(java_name_spec, st);
        }

        nl_indent(st);

        if (! php_indent_body)
            st << "preserve_php_body_indent ";

        if (php_get_set_frozen)
            st << "php_frozen ";

        if (php_final)
            st << "php_final ";

        if (! php_def.isEmpty()) {
            st << "php_def ";
            save_string(php_def, st);
        }

        if (!php_name_spec.isEmpty()) {
            nl_indent(st);
            st << "php_name_spec ";
            save_string(php_name_spec, st);
        }

        nl_indent(st);

        if (! python_indent_body)
            st << "preserve_python_body_indent ";

        if (python_get_set_frozen)
            st << "python_frozen ";

        if (! python_decorator.isEmpty()) {
            st << "python_decorator ";
            save_string(python_decorator, st);
        }

        if (! python_def.isEmpty()) {
            st << "python_def ";
            save_string(python_def, st);
        }

        if (!python_name_spec.isEmpty()) {
            nl_indent(st);
            st << "python_name_spec ";
            save_string(python_name_spec, st);
        }

        nl_indent(st);

        if (idl_get_set_frozen)
            st << "idl_frozen ";

        if (idl_oneway)
            st << "oneway ";

        if (! idl_decl.isEmpty()) {
            st << "idl_decl ";
            save_string(idl_decl, st);
        }

        if (!idl_name_spec.isEmpty()) {
            nl_indent(st);
            st << "idl_name_spec ";
            save_string(idl_name_spec, st);
        }
    }
}

OperationData * OperationData::read_ref(char *& st)
{
    read_keyword(st, "operation_ref");

    return all[read_id(st)];
}

void OperationData::read(char *& st, char *& k)
{
    cpp_body.length = 0;
    java_body.length = 0;
    python_body.length = 0;
    php_body.length = 0;

    k = read_keyword(st);
    BasicData::read(st, k);	// updates k

    isa_class_operation = is_abstract = FALSE;

    if (!strcmp(k, "class_operation")) {
        isa_class_operation = TRUE;
        k = read_keyword(st);
    }
    else if (!strcmp(k, "abstract")) {
        is_abstract = TRUE;
        k = read_keyword(st);
    }

    if (!strcmp(k, "force_body_gen")) {
        force_body_gen = TRUE;
        k = read_keyword(st);
    }
    else
        force_body_gen = FALSE;

    if (!strcmp(k, "volatile")) {
        is_volatile = TRUE;
        k = read_keyword(st);
    }
    else
        is_volatile = FALSE;

    if (!strcmp(k, "preserve_cpp_body_indent")) {
        cpp_indent_body = FALSE;
        k = read_keyword(st);
    }
    else
        cpp_indent_body = TRUE;

    if (!strcmp(k, "cpp_frozen")) {
        cpp_get_set_frozen = TRUE;
        k = read_keyword(st);
    }
    else
        cpp_get_set_frozen = FALSE;

    if (!strcmp(k, "const")) {
        cpp_const = TRUE;
        k = read_keyword(st);
    }
    else
        cpp_const = FALSE;

    if(api_format(true) > 76)
    {
        if (!strcmp(k, "default")) {
            cpp_default = TRUE;
            k = read_keyword(st);
        }
        else
            cpp_default = FALSE;

        if (!strcmp(k, "delete")) {
            cpp_delete = TRUE;
            k = read_keyword(st);
        }
        else
            cpp_delete = FALSE;

        if (!strcmp(k, "override")) {
            cpp_override = TRUE;
            k = read_keyword(st);
        }
        else
            cpp_override = FALSE;

        if (!strcmp(k, "final")) {
            cpp_final = TRUE;
            k = read_keyword(st);
        }
        else
            cpp_final = FALSE;
    }

    if (!strcmp(k, "friend")) {
        cpp_friend = TRUE;
        k = read_keyword(st);
    }
    else
        cpp_friend = FALSE;

    if (!strcmp(k, "virtual") || !strcmp(k, "cpp_virtual")) {
        cpp_virtual = TRUE;
        k = read_keyword(st);
    }
    else
        cpp_virtual = FALSE;

    if (!strcmp(k, "inline") || !strcmp(k, "cpp_inline")) {
        cpp_inline = TRUE;
        k = read_keyword(st);
    }
    else
        cpp_inline = FALSE;

    if (!strcmp(k, "cpp_volatile")) {
        // old version
        is_volatile = TRUE;
        k = read_keyword(st);
    }

    uml_visibility = ::visibility(k);

    k = read_keyword(st);

    AType t;

    t.read(st, "return_type", "explicit_return_type", k);
    set_return_type(t);


    k = read_keyword(st);
    if (read_file_format() == 76 && !strcmp(k, "multiplicity"))
    {
        read_string(st);
        k = read_keyword(st);
    }

    if(strcmp(k, "nparams"))
    {
        msg_critical(TR("Error"),  TR("Expected nparams instead of " + QString(k)));
        THROW_ERROR 0;
    }

    unsigned n = read_unsigned(st);
    set_n_params(n);

    if(read_file_format() > 76)
    {
        read_keyword(st, "origin");
        set_origin_class(WrapperStr(read_string(st)));
    }

    k = read_keyword(st);

    for (unsigned i = 0; i != n; i += 1) {
        params[i]->read(st, k);	// updates k
        depend_on(params[i]->get_type().type);
    }

    if (!strcmp(k, "nexceptions")) {
        n = read_unsigned(st);

        set_n_exceptions(n);
        k = read_keyword(st);

        for (unsigned i = 0; i != n; i += 1) {
            exceptions[i].read(st, k);	// updates k
            depend_on(exceptions[i].get_type().type);
        }
    }
    else
        set_n_exceptions(0);

    if (!strcmp(k, "constraint")) {
        constraint = read_string(st);
        k = read_keyword(st);
    }
    else
        constraint = "";

    if (!strcmp(k, "cpp_visibility")) {
        cpp_visibility = ::visibility(read_keyword(st));
        k = read_keyword(st);
    }
    else
        cpp_visibility = UmlDefaultVisibility;

    if (!strcmp(k, "cpp_decl")) {
        cpp_decl = read_string(st);
        k = read_keyword(st);
    }
    else
        cpp_decl = "";

    if (!strcmp(k, "cpp_def")) {
        char * d = read_string(st);

        cpp_def.assign(d, (is_abstract || (strstr(d, "${body}") != 0)));
        k = read_keyword(st);
    }
    else
        cpp_def.assign("", TRUE);

    if (!strcmp(k, "cpp_name_spec")) {
        is_get_or_set = TRUE;
        cpp_name_spec = read_string(st);
        k = read_keyword(st);
    }
    else {
        is_get_or_set = FALSE;
        cpp_name_spec = "";
    }

    if (!strcmp(k, "preserve_java_body_indent")) {
        java_indent_body = FALSE;
        k = read_keyword(st);
    }
    else
        java_indent_body = TRUE;

    if (!strcmp(k, "java_frozen")) {
        java_get_set_frozen = TRUE;
        k = read_keyword(st);
    }
    else
        java_get_set_frozen = FALSE;

    if (!strcmp(k, "final")) {
        java_final = TRUE;
        k = read_keyword(st);
    }
    else
        java_final = FALSE;

    if (!strcmp(k, "synchronized")) {
        java_synchronized = TRUE;
        k = read_keyword(st);
    }
    else
        java_synchronized = FALSE;

    if (!strcmp(k, "java_def")) {
        char * d = read_string(st);
        QString ste = GenerationSettings::java_class_stereotype(stereotype);

        java_def.assign(d,
                        is_abstract ||
                        (strstr(d, "${body}") != 0) ||
                        (ste == "interface") ||
                        (ste == "@interface"));
        k = read_keyword(st);
    }
    else
        java_def.assign("", TRUE);

    if (!strcmp(k, "java_annotation")) {
        java_annotation = read_string(st);
        k = read_keyword(st);
    }
    else
        java_annotation = "";

    if (!strcmp(k, "java_name_spec")) {
        is_get_or_set = TRUE;
        java_name_spec = read_string(st);
        k = read_keyword(st);
    }
    else {
        is_get_or_set = FALSE;
        java_name_spec = "";
    }

    if (!strcmp(k, "preserve_php_body_indent")) {
        php_indent_body = FALSE;
        k = read_keyword(st);
    }
    else
        php_indent_body = TRUE;

    if (!strcmp(k, "php_frozen")) {
        php_get_set_frozen = TRUE;
        k = read_keyword(st);
    }
    else
        php_get_set_frozen = FALSE;

    if (!strcmp(k, "php_final")) {
        php_final = TRUE;
        k = read_keyword(st);
    }
    else
        php_final = FALSE;

    if (!strcmp(k, "php_def")) {
        char * d = read_string(st);
        QString ste = GenerationSettings::php_class_stereotype(stereotype);

        php_def.assign(d,
                       is_abstract ||
                       (strstr(d, "${body}") != 0) ||
                       (ste == "interface"));
        k = read_keyword(st);
    }
    else
        php_def.assign("", TRUE);

    if (!strcmp(k, "php_name_spec")) {
        is_get_or_set = TRUE;
        php_name_spec = read_string(st);
        k = read_keyword(st);
    }
    else {
        is_get_or_set = FALSE;
        php_name_spec = "";
    }

    if (!strcmp(k, "preserve_python_body_indent")) {
        python_indent_body = FALSE;
        k = read_keyword(st);
    }
    else
        python_indent_body = TRUE;

    if (!strcmp(k, "python_frozen")) {
        python_get_set_frozen = TRUE;
        k = read_keyword(st);
    }
    else
        python_get_set_frozen = FALSE;

    if (!strcmp(k, "python_decorator")) {
        python_decorator = read_string(st);
        k = read_keyword(st);
    }
    else
        python_decorator = "";

    if (!strcmp(k, "python_def")) {
        char * d = read_string(st);

        python_def.assign(d, (strstr(d, "${body}") != 0));
        k = read_keyword(st);
    }
    else
        python_def.assign("", TRUE);

    if (!strcmp(k, "python_name_spec")) {
        is_get_or_set = TRUE;
        python_name_spec = read_string(st);
        k = read_keyword(st);
    }
    else {
        is_get_or_set = FALSE;
        python_name_spec = "";
    }

    if (!strcmp(k, "idl_frozen")) {
        idl_get_set_frozen = TRUE;
        k = read_keyword(st);
    }
    else
        idl_get_set_frozen = FALSE;

    if (!strcmp(k, "oneway")) {
        idl_oneway = TRUE;
        k = read_keyword(st);
    }
    else
        idl_oneway = FALSE;

    if (!strcmp(k, "idl_decl")) {
        idl_decl = read_string(st);
        k = read_keyword(st);
    }
    else
        idl_decl = "";

    if (!strcmp(k, "idl_name_spec")) {
        is_get_or_set = TRUE;
        idl_name_spec = read_string(st);
        k = read_keyword(st);
    }
    else {
        is_get_or_set = FALSE;
        idl_name_spec = "";
    }
}
bool operator==(const OperationData & origin, const OperationData & another)
{
    bool paramsResult = false;
//    if(origin.params.isEmpty() && another.params.isEmpty())
//        paramsResult = true;
//    else if(origin.params != another.params)
//    {
//        if(origin.params.isEmpty())
//            return false;
//        else if(another.params.isEmpty() == nullptr)
//            return false;
//         paramsResult = *origin.params == *another.params;
//    }
    paramsResult = origin.params == another.params;
    if(!paramsResult)
        return false;
    if(origin.uml_visibility != another.uml_visibility ||
     origin.cpp_visibility!= another.cpp_visibility ||
     origin.is_deleted != another.is_deleted ||
     origin.is_get_or_set != another.is_get_or_set ||
     origin.isa_class_operation!= another.isa_class_operation ||
     origin.is_abstract!= another.is_abstract ||
     origin.force_body_gen != another.force_body_gen ||
     origin.is_volatile != another.is_volatile ||
     origin.cpp_const != another.cpp_const ||
     origin.cpp_friend!= another. cpp_friend||
     origin.cpp_virtual!= another.cpp_virtual ||
     origin.cpp_inline!= another.cpp_inline ||
     origin.cpp_default!= another.cpp_default ||
     origin.cpp_delete!= another.cpp_delete ||
     origin.cpp_override != another. cpp_override||
     origin.cpp_final != another.cpp_final ||
     origin.cpp_get_set_frozen != another.cpp_get_set_frozen ||
     origin.cpp_indent_body != another.cpp_indent_body ||
     origin.java_final != another. java_final||
     origin.java_synchronized != another. java_synchronized||
     origin.java_get_set_frozen!= another. java_get_set_frozen||
     origin.java_indent_body != another.java_indent_body ||
     origin.php_final != another.php_final ||
     origin.php_get_set_frozen != another. php_get_set_frozen||
     origin.php_indent_body != another. php_indent_body||
     origin.python_get_set_frozen != another. python_get_set_frozen||
     origin.python_indent_body != another.python_indent_body ||
     origin.idl_oneway != another.idl_oneway ||
     origin.idl_get_set_frozen != another.idl_get_set_frozen ||

     origin.nexceptions!= another.nexceptions ||
     origin.return_type!= another.return_type ||
     origin.originClass!= another.originClass ||
     origin.exceptions!= another.exceptions ||
     origin.constraint!= another. constraint||


    origin.cpp_body.offset!= another.cpp_body.offset ||
    origin.cpp_body.length!= another.cpp_body.length ||
    origin.cpp_decl!= another.cpp_decl ||
    origin.cpp_def!= another.cpp_def ||
    origin.cpp_name_spec!= another.cpp_name_spec||


    origin.java_body.offset!= another.java_body.offset ||
    origin.java_body.length!= another.java_body.length ||
    origin.java_def!= another.java_def ||
    origin.java_name_spec!= another.java_name_spec ||
    //origin.java_annotation!= another. java_annotation||


    origin.php_body.offset!= another.php_body.offset||
    origin.php_body.length!= another.php_body.length||
    origin.php_def!= another.php_def ||
    origin.php_name_spec!= another.php_name_spec||


    origin.python_body.offset!= another. python_body.offset||
    origin.python_body.length!= another. python_body.length||
    origin.python_def!= another.python_def ||
    origin.python_name_spec!= another. python_name_spec||
    origin.python_decorator!= another.python_decorator ||


    origin.idl_decl!= another.idl_decl ||
    origin.idl_name_spec != another. idl_name_spec)
         return false;
     return true;


}
#include <algorithm>
bool PropagationEquality(const OperationData & origin, const OperationData & another)
{
    bool paramsResult = true;

    if(origin.params.size() != another.params.size())
        paramsResult = false;
    else
    {
        int i = 0;
        auto it = origin.params.begin();
        while(it != origin.params.end())
        {
            it++;i++;
            if(*origin.params[i].get() == *another.params[i].get() )
                continue;
            paramsResult = false;
            break;
        }
    }
    //origin.params != another.params;

    if(!paramsResult)
        return false;
    if(origin.uml_visibility != another.uml_visibility ||
     origin.cpp_visibility!= another.cpp_visibility ||
     origin.is_deleted != another.is_deleted ||
     origin.is_get_or_set != another.is_get_or_set ||
     origin.isa_class_operation!= another.isa_class_operation ||
     origin.is_volatile != another.is_volatile ||
     origin.cpp_const != another.cpp_const ||
     origin.cpp_friend!= another. cpp_friend||
     origin.cpp_virtual!= another.cpp_virtual ||
     //origin.cpp_inline!= another.cpp_inline ||
     origin.cpp_get_set_frozen != another.cpp_get_set_frozen ||
     origin.java_synchronized != another. java_synchronized||
     origin.java_get_set_frozen!= another. java_get_set_frozen||
     origin.php_get_set_frozen != another. php_get_set_frozen||

     origin.python_get_set_frozen != another. python_get_set_frozen||
     origin.idl_oneway != another.idl_oneway ||
     origin.idl_get_set_frozen != another.idl_get_set_frozen ||

     origin.nexceptions!= another.nexceptions ||
     origin.return_type!= another.return_type ||
     origin.originClass!= another.originClass ||
     origin.exceptions!= another.exceptions ||
     origin.constraint!= another.constraint||


    origin.cpp_body.offset!= another.cpp_body.offset ||
    origin.cpp_body.length!= another.cpp_body.length ||
    origin.cpp_decl!= another.cpp_decl ||
    //origin.cpp_def!= another.cpp_def ||
    origin.cpp_name_spec!= another.cpp_name_spec||


    origin.java_body.offset!= another.java_body.offset ||
    origin.java_body.length!= another.java_body.length ||
    origin.java_def!= another.java_def ||
    origin.java_name_spec!= another.java_name_spec ||


    origin.php_body.offset!= another.php_body.offset||
    origin.php_body.length!= another.php_body.length||
    origin.php_def!= another.php_def ||
    origin.php_name_spec!= another.php_name_spec||


    origin.python_body.offset!= another. python_body.offset||
    origin.python_body.length!= another. python_body.length||
    origin.python_def!= another.python_def ||
    origin.python_name_spec!= another. python_name_spec||
    origin.python_decorator!= another.python_decorator ||
    origin.idl_decl!= another.idl_decl ||
    origin.idl_name_spec != another. idl_name_spec)
         return false;
     return true;



     //origin.java_annotation!= another. java_annotation||
     //     origin.cpp_default!= another.cpp_default ||
     //     origin.cpp_delete!= another.cpp_delete ||
     //     origin.cpp_override != another. cpp_override||
     //     origin.cpp_final != another.cpp_final ||

     //origin.cpp_indent_body != another.cpp_indent_body ||
     //origin.java_final != another. java_final||

     //origin.java_indent_body != another.java_indent_body ||
     //origin.php_final != another.php_final ||

     //origin.php_indent_body != another. php_indent_body||


     //origin.python_indent_body != another.python_indent_body ||

//     bool constraint = origin.constraint != another.constraint;
//     bool uml_visibility = origin.uml_visibility != another.uml_visibility;
//     bool cpp_visibility = origin.cpp_visibility != another.cpp_visibility;
//     bool is_deleted = origin.is_deleted != another.is_deleted;
//     bool is_get_or_set = origin.is_get_or_set != another.is_get_or_set;
//     bool isa_class_operation = origin.isa_class_operation != another.isa_class_operation;
//     bool is_volatile = origin.is_volatile != another.is_volatile;
//     bool cpp_const = origin.cpp_const != another.cpp_const;
//     bool cpp_friend = origin.cpp_friend != another.cpp_friend;
//     bool cpp_virtual = origin.cpp_virtual != another.cpp_virtual;
//     bool cpp_inline = origin.cpp_inline != another.cpp_inline;
//     bool cpp_get_set_frozen = origin.cpp_get_set_frozen != another.cpp_get_set_frozen;
//     bool nexceptions = origin.nexceptions != another.nexceptions;
//     bool return_type = origin.return_type != another.return_type;
//     bool originClass = origin.originClass != another.originClass;
//     bool exceptions = origin.exceptions != another.exceptions;
//     bool java_synchronized = origin.java_synchronized != another.java_synchronized;
//     bool java_get_set_frozen = origin.java_get_set_frozen != another.java_get_set_frozen;
//     bool php_get_set_frozen = origin.php_get_set_frozen != another.php_get_set_frozen;
//     bool python_get_set_frozen = origin.python_get_set_frozen != another.python_get_set_frozen;
//     bool idl_oneway = origin.idl_oneway != another.idl_oneway;
//     bool idl_get_set_frozen = origin.idl_get_set_frozen != another.idl_get_set_frozen;
//     bool cpp_bodyoffset = origin.cpp_body.offset != another.cpp_body.offset;
//     bool cpp_bodylength = origin.cpp_body.length != another.cpp_body.length;
//     bool cpp_decl = origin.cpp_decl != another.cpp_decl;
//     bool cpp_def = origin.cpp_def != another.cpp_def;
}

void OperationData::set_cpp_visibility(int v)
{
    cpp_visibility = static_cast<UmlVisibility>(v);
}
