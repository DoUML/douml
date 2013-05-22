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
//Added by qt3to4:
#include <QTextStream>

//Added by qt3to4:
#include <Q3PtrList>

#include "BrowserClass.h"
#include "ClassData.h"
#include "ClassDialog.h"
#include "BrowserClassDiagram.h"
#include "BrowserComponent.h"
#include "ActualParamData.h"
#include "FormalParamData.h"
#include "RelationData.h"
#include "GenerationSettings.h"
#include "myio.h"
#include "ToolCom.h"
#include "strutil.h"
#include "mu.h"
#include "Factories/EdgeMenuFactory.h"
#include "misc/TypeIdentifier.h"

bool ClassData::DontUpdateActuals = FALSE;

ClassData::ClassData()
    : formals(0), nformals(0),
      is_deleted(FALSE), is_abstract(FALSE), is_active(FALSE),
      bodies_read(FALSE), bodies_modified(FALSE),
      cpp_external(FALSE),
      java_external(FALSE), java_final(FALSE),
      php_external(FALSE), php_final(FALSE),
      python_external(FALSE),
      idl_external(FALSE), idl_local(FALSE), idl_custom(FALSE),
      uml_visibility(UmlPackageVisibility), cpp_visibility(UmlDefaultVisibility)
{
    if (GenerationSettings::cpp_get_default_defs())
        cpp_decl = GenerationSettings::cpp_default_class_decl();

    if (GenerationSettings::java_get_default_defs())
        java_decl = GenerationSettings::java_default_class_decl();

    if (GenerationSettings::php_get_default_defs())
        php_decl = GenerationSettings::php_default_class_decl();

    if (GenerationSettings::python_get_default_defs())
        python_decl = GenerationSettings::python_default_class_decl();

    python_2_2 = GenerationSettings::python_default_2_2();

    if (GenerationSettings::idl_get_default_defs())
        idl_decl = GenerationSettings::idl_default_valuetype_decl();

    connect(this, SIGNAL(changed()), this, SLOT(update_actuals()));
}

ClassData::ClassData(const ClassData * model, BrowserNode * bn)
    : BasicData(model), constraint(model->constraint)
{
    browser_node = bn;

    if ((nformals = model->nformals) == 0)
        formals = 0;
    else {
        FormalParamData * mformals = model->formals;
        formals = new FormalParamData[nformals];

        for (int index = 0; index != nformals; index += 1)
            formals[index] = mformals[index];
    }

    BrowserClass * cl = 0;
    foreach (ActualParamData *data, model->actuals) {
        ActualParamData * actual = new ActualParamData(*data);

        if (actual->get_class() != cl) {
            cl = actual->get_class();
            connect(cl->get_data(), SIGNAL(changed()),
                    this, SLOT(update_actuals()));
            connect(cl->get_data(), SIGNAL(deleted()),
                    this, SLOT(update_actuals()));
        }

        actuals.append(actual);
    }

    set_base_type(model->base_type);
    is_deleted = FALSE;
    bodies_read = FALSE;
    bodies_modified = FALSE;
    is_abstract = model->is_abstract;
    is_active = model->is_active;
    cpp_external = model->cpp_external;
    java_external = model->java_external;
    java_final = model->java_final;
    php_external = model->php_external;
    php_final = model->php_final;
    python_2_2 = model->python_2_2;
    python_external = model->python_external;
    idl_external = model->idl_external;
    idl_local = model->idl_local;
    idl_custom = model->idl_custom;
    uml_visibility = model->uml_visibility;
    cpp_visibility = model->cpp_visibility;
    cpp_decl = model->cpp_decl;
    java_decl = model->java_decl;
    java_annotation = model->java_annotation;
    php_decl = model->php_decl;
    python_decl = model->python_decl;
    set_switch_type(model->switch_type);
    idl_decl = model->idl_decl;

    connect(this, SIGNAL(changed()), this, SLOT(update_actuals()));
}

ClassData::~ClassData()
{
    if (formals != 0)
        delete [] formals;

    while (! actuals.isEmpty()) {
        delete actuals.at(0);
        actuals.remove(0u);
    }
}

bool ClassData::deletedp() const
{
    return is_deleted;
}

void ClassData::set_deletedp(bool y)
{
    is_deleted = y;
}

void ClassData::set_is_abstract(bool yes)
{
    is_abstract = yes;
}

void ClassData::set_is_active(bool yes)
{
    is_active = yes;
}

void ClassData::inherit_or_instantiate(BrowserClass *)
{
    update_actuals();
}

void ClassData::update_actuals(BrowserClass * parent,
                               QList<ActualParamData *> & new_actuals,
                               QList<ActualParamData *> & managed)
{
    if (((BrowserNode *) parent->parent())->get_type() == UmlClass)
        update_actuals((BrowserClass *) parent->parent(), new_actuals, managed);

    int n = ((ClassData *) parent->get_data())->nformals;

    if (n != 0) {
        // search the first associated actual
        int i;
        for (i = 0; i < actuals.size(); ++i)
            if ((actuals[i]->get_class() == parent) && !managed.contains(actuals[i]))
                break; // did find;

        int nth = 0;

        // progress on still present formals
        while (i < actuals.size() && (nth < n) && (actuals[i]->get_class() == parent))  {
            // actual ok
            new_actuals.append(actuals[i]);
            managed.append(actuals[i]);

            ++i;
            nth += 1;
        }

        if (nth < n) {
            // adds necessary actuals
            if (nth == 0) {
                // new inheritance
                connect(parent->get_data(), SIGNAL(deleted()),
                        this, SLOT(update_actuals()));
                connect(parent->get_data(), SIGNAL(changed()),
                        this, SLOT(update_actuals()));
            }

            do {
                new_actuals.append(new ActualParamData(parent, nth));
                nth += 1;
            }
            while (nth != n);
        }
    }
}

void ClassData::update_actuals()
{
    if (DontUpdateActuals)
        return;

    QList<ActualParamData *> new_actuals;
    QList<ActualParamData *> managed;

    // an inherited parent was modified/deleted, updates all actuals
    foreach (BrowserNode * parent, browser_node->parents())
        update_actuals((BrowserClass *)parent, new_actuals, managed);

    if (!(actuals == new_actuals)) {
        foreach (ActualParamData * actual, actuals)
            if (!new_actuals.contains(actual))
                delete actual;

        actuals = new_actuals;
        browser_node->package_modified();
    }

    // note : even if actuals == new_actuals to take
    // into account change in already existing formal and actual
    emit actuals_changed();
}

void ClassData::get_actuals(QList<ActualParamData *> & l, BrowserClass * parent)
{
    if (((BrowserNode *) parent->parent())->get_type() == UmlClass)
        get_actuals(l, (BrowserClass *) parent->parent());

    int n = ((ClassData *) parent->get_data())->nformals;

    if (n != 0) {
        // search the first associated actual
        int i;
        for (i = 0; i < actuals.size(); ++i) {
            ActualParamData * actual = actuals[i];
            if ((actual->get_class() == parent) && !l.contains(actual))
                // find;
                break;
        }

        int nth = 0;

        // progress on still present formals
        while (i < actuals.size() && (nth < n) && (actuals[i]->get_class() == parent)) {
            // actual ok
            l.append(actuals[i]);
            ++i;
            nth += 1;
        }
    }
}

// returns the actuals forms associated to inheritence of 'parent'
QString ClassData::get_actuals(BrowserClass * parent)
{
    QList<ActualParamData *> l;

    get_actuals(l, parent);

    foreach (BrowserNode *grandpa, parent->parents())
        get_actuals(l, (BrowserClass *)grandpa);

    QString s;
    const char * sep = "<";
    QString arrow = "->";

    foreach (ActualParamData * actual, l) {
        QString v = actual->get_value().get_type();

        if (!v.isEmpty()) {
            s += sep + actual->get_name(FALSE) + arrow + v;
            sep = ", ";
        }
    }

    return (s.isEmpty()) ? s : s + ">";
}

const char * ClassData::get_formalparam_type(int rank) const
{
    return formals[rank].get_type();
}

const char * ClassData::get_formalparam_name(int rank) const
{
    return formals[rank].get_name();
}

QString ClassData::get_formalparam_default_value(int rank, bool full) const
{
    const AType & t = formals[rank].get_default_value();

    return (full)
           ? t.get_full_type()
           : QString(t.get_type());
}

QString ClassData::get_formalparam_extends(int rank, bool full) const
{
    const AType & t = formals[rank].get_extends();

    return (full)
           ? t.get_full_type()
           : QString(t.get_type());
}

void ClassData::set_n_formalparams(int n)
{
    if (n > nformals) {
        if (formals)
            delete [] formals;

        formals = new FormalParamData[n];
    }

    nformals = n;
}

void ClassData::set_formalparam_type(int rank, const char * s)
{
    formals[rank].set_type(s);
}

void ClassData::set_formalparam_name(int rank, const char * s)
{
    formals[rank].set_name(s);
}

void ClassData::set_formalparam_default_value(int rank, BrowserClass * t,
        const char * e)
{
    formals[rank].set_default_value(t, e);
}

void ClassData::set_formalparam_extends(int rank, BrowserClass * t,
                                        const char * e)
{
    formals[rank].set_extends(t, e);
}

void ClassData::get_template_prefixes(QString & template1,
                                      QString & template2) const
{
    if (nformals != 0) {
        const char * sep1 = "template<";
        const char * sep2 = "<";

        for (unsigned char index = 0; index != nformals; index += 1) {
            FormalParamData & f = formals[index];

            template1 += sep1;
            template1 += f.get_type();
            template1 += " ";
            template1 += f.get_name();

            template2 += sep2;
            template2 += f.get_name();

            sep1 = sep2 = ", ";
        }

        template1 += ">\n";
        template2 += ">";
    }
}

void ClassData::get_class_spec(QString & templates,
                               QString & names,
                               QString & templates_tmplop,
                               QString & names_tmplop) const
{
    bool browserNodeIsUMLClass = ((BrowserNode *) browser_node->parent())->get_type() == UmlClass;

    if (browserNodeIsUMLClass) {
        ClassData * cd = ((ClassData *)((BrowserNode *) browser_node->parent())->get_data());
        cd->get_class_spec(templates, names, templates_tmplop, names_tmplop);
        names += "::";
    }

    QString t1;
    QString t2;

    get_template_prefixes(t1, t2);
    templates_tmplop = templates + "template<>\n";
    templates += t1;
    names_tmplop = names + true_name(browser_node->get_name(), cpp_decl);
    names = names_tmplop + t2;
}

void ClassData::addFormals(QStringList & l)
{
    for (int index = 0; index != nformals; index += 1) {
        const char * s = formals[index].get_name();

        if (l.findIndex(s) == -1)
            l.prepend(s);
    }

    if (((BrowserNode *) browser_node->parent())->get_type() == UmlClass)
        ((ClassData *)((BrowserNode *) browser_node->parent())->get_data())
        ->addFormals(l);
}

//

QString ClassData::get_actualparam_name(int rank)
{
    return actuals.at(rank)->get_name();
}

QString ClassData::get_actualparam_value(int rank)
{
    return actuals.at(rank)->get_value().get_full_type();
}

void ClassData::set_actualparam_value(int rank, const AType & t)
{
    actuals.at(rank)->set_value(t);
}

void ClassData::set_switch_type(const AType & t)
{
    if (switch_type.type != t.type) {
        if (switch_type.type != 0)
            disconnect(switch_type.type->get_data(), SIGNAL(deleted()),
                       this, SLOT(on_delete()));

        if ((switch_type.type = t.type) != 0)
            connect(switch_type.type->get_data(), SIGNAL(deleted()),
                    this, SLOT(on_delete()));
    }

    switch_type.explicit_type = t.explicit_type;
}

void ClassData::set_base_type(const AType & t)
{
    if (base_type.type != t.type) {
        if (base_type.type != 0)
            disconnect(base_type.type->get_data(), SIGNAL(deleted()),
                       this, SLOT(on_delete()));

        if ((base_type.type = t.type) != 0)
            connect(base_type.type->get_data(), SIGNAL(deleted()),
                    this, SLOT(on_delete()));
    }

    base_type.explicit_type = t.explicit_type;
}

void ClassData::on_delete()
{
    if (switch_type.type && switch_type.type->deletedp()) {
        switch_type.explicit_type = switch_type.type->get_name();
        switch_type.type = 0;
    }

    if (base_type.type && base_type.type->deletedp()) {
        base_type.explicit_type = base_type.type->get_name();
        base_type.type = 0;
    }

    ((BrowserClass *) browser_node)->on_delete();

    modified();
}

bool ClassData::reference(BrowserClass * target) const
{
    if ((base_type.type == target) || (switch_type.type == target))
        return TRUE;

    unsigned i;

    for (i = 0; i != nformals; i += 1)
        if ((formals[i].get_default_value().type == target) ||
            (formals[i].get_extends().type == target))
            return TRUE;

    foreach (ActualParamData *data, actuals)
        if (data->get_value().type == target)
            return true;

    return FALSE;
}

//

bool ClassData::decldefbody_contain(const QString & s, bool cs,
                                    BrowserNode *)
{
    return ((QString(get_cppdecl()).find(s, 0, cs) != -1) ||
            (QString(get_javadecl()).find(s, 0, cs) != -1) ||
            (QString(get_phpdecl()).find(s, 0, cs) != -1) ||
            (QString(get_pythondecl()).find(s, 0, cs) != -1) ||
            (QString(get_idldecl()).find(s, 0, cs) != -1));
}

//

void ClassData::set_uml_visibility(UmlVisibility v)
{
    uml_visibility = v;
}

void ClassData::set_uml_visibility(int v)
{
    uml_visibility = static_cast<UmlVisibility>(v);
}

void ClassData::set_cpp_visibility(UmlVisibility v)
{
    cpp_visibility = v;
}

//

bool ClassData::tool_cmd(ToolCom * com, const char * args,
                         BrowserNode * bn,
                         const QString & comment)
{
    if (((unsigned char) args[-1]) >= firstSetCmd) {
        if (!bn->is_writable() && !root_permission())
            com->write_ack(FALSE);
        else {
            switch ((unsigned char) args[-1]) {
            case setIsAbstractCmd:
                if ((*args == 0) &&
                    strcmp(browser_node->get_stereotype(), "enum") &&
                    ((BrowserClass *) browser_node)->have_abstract_operation()) {
                    com->write_ack(FALSE);
                    return TRUE;
                }
                else
                    is_abstract = (*args != 0);

                break;

            case setActiveCmd:
                is_active = (*args != 0);
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

            case setBaseTypeCmd:
                if (stereotype != "typedef") {
                    com->write_ack(FALSE);
                    return TRUE;
                }
                else {
                    AType t;

                    com->get_type(t, args);
                    set_base_type(t);
                }

                break;

            case setConstraintCmd:
                constraint = args;
                break;

            case setIsCppExternalCmd:
                cpp_external = (*args != 0);
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

            case setIsJavaExternalCmd:
                java_external = (*args != 0);
                break;

            case setJavaDeclCmd:
                java_decl = args;
                break;

            case setJavaAnnotationCmd: {
                WrapperStr s = args;

                s = s.stripWhiteSpace();

                if (! s.isEmpty())
                    s += '\n';

                java_annotation = s;
            }
            break;

            case setIsJavaPublicCmd: {
                UmlVisibility v = (*args != 0) ? UmlPublic : UmlPackageVisibility;

                if ((cpp_visibility == UmlDefaultVisibility) &&
                    (uml_visibility != UmlPublic) &&
                    (uml_visibility != UmlPackageVisibility))
                    cpp_visibility = uml_visibility;

                uml_visibility = v;
            }
            break;

            case setIsJavaFinalCmd:
                java_final = (*args != 0);
                break;

            case setIsPhpExternalCmd:
                php_external = (*args != 0);
                break;

            case setPhpDeclCmd:
                php_decl = args;
                break;

            case setIsPhpFinalCmd:
                php_final = (*args != 0);
                break;

            case setIsPythonExternalCmd:
                python_external = (*args != 0);
                break;

            case setPythonDeclCmd:
                python_decl = args;
                break;

            case setIsPython2_2Cmd:
                python_2_2 = (*args != 0);
                break;

            case setIdlDeclCmd:
                idl_decl = args;
                break;

            case setSwitchTypeCmd:
                if (GenerationSettings::idl_class_stereotype(stereotype) != "union") {
                    com->write_ack(FALSE);
                    return TRUE;
                }
                else {
                    AType t;

                    com->get_type(t, args);
                    set_switch_type(t);
                }

                break;

            case setIsIdlExternalCmd:
                idl_external = (*args != 0);
                break;

            case setIsIdlLocalCmd:
                idl_local = (*args != 0);
                break;

            case setIsIdlCustomCmd:
                idl_custom = (*args != 0);
                break;

            case setIsClassMemberCmd:
            case setIsVolatileCmd:
                com->write_ack(FALSE);
                return TRUE;

            case removeFormalCmd: {
                unsigned rank = com->get_unsigned(args);

                if (rank < nformals) {
                    if (nformals == 1) {
                        delete [] formals;
                        formals = 0;
                    }
                    else
                        while (++rank != nformals)
                            formals[rank - 1] = formals[rank];

                    nformals -= 1;
                    DontUpdateActuals = FALSE;
                }
                else {
                    com->write_ack(FALSE);
                    return TRUE;
                }
            }
            break;

            case addFormalCmd: {
                unsigned rank = com->get_unsigned(args);

                if (rank <= nformals) {
                    FormalParamData * new_formals = new FormalParamData[++nformals];
                    unsigned index;

                    for (index = 0; index != rank; index += 1)
                        new_formals[index] = formals[index];

                    new_formals[index].read(com, args);

                    while (++index != nformals)
                        new_formals[index] = formals[index - 1];

                    delete [] formals;
                    formals = new_formals;
                    DontUpdateActuals = FALSE;
                }
                else {
                    FormalParamData::skip(com, args);
                    com->write_ack(FALSE);
                    return TRUE;
                }
            }
            break;

            case replaceFormalCmd: {
                unsigned rank = com->get_unsigned(args);

                if (rank < nformals) {
                    formals[rank].read(com, args);
                    DontUpdateActuals = FALSE;
                }
                else {
                    FormalParamData::skip(com, args);
                    com->write_ack(FALSE);
                    return TRUE;
                }
            }
            break;

            case replaceActualCmd: {
                unsigned rank = com->get_unsigned(args);

                if ((int)rank < actuals.count()) {
                    AType t;

                    com->get_type(t, args);
                    actuals.at(rank)->set_value(t);
                }
                else {
                    com->write_ack(FALSE);
                    return TRUE;
                }
            }
            break;

            default:
                return BasicData::tool_cmd(com, args, bn, comment);
            }

            // ok case
            bn->modified();	// call modified
            bn->package_modified();
            com->write_ack(TRUE);
        }
    }
    else {
        // get only !
        switch ((unsigned char) args[-1]) {
        case formalsCmd: {
            com->write_unsigned((unsigned) nformals);

            for (int index = 0; index != nformals; index += 1)
                formals[index].send_uml_def(com);
        }
        break;

        case actualsCmd: {
            com->write_unsigned((unsigned) actuals.count());

            foreach (ActualParamData *a, actuals)
                a->send_uml_def(com);
        }
        break;

        default:
            return BasicData::tool_cmd(com, args, bn, comment);
        }
    }

    return TRUE;
}

//

void ClassData::edit()
{
    setName(browser_node->get_name());

    // edition must be modal
    //An<EdgeMenuFactory> factory;
    ClassDialog::Instance(this)->show();
    ClassDialog::Instance(this)->raise();
    //dialog->setModal(false);
    //QObject::connect(dialog.data(), SIGNAL(edgeMenuRequested(uint)),factory.getData(), SLOT(OnEdgeMenuRequested(uint)));
    //dialog->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    //dialog->move(QCursor::pos().x() + 15, QCursor::pos().y());
    //factory->SpawnEdgeMenu(TypeIdentifier<ClassDialog>::id());
    //dialog->show();

}

//

void ClassData::send_uml_def(ToolCom * com, BrowserNode * bn,
                             const QString & comment)
{
    int api = com->api_format(true);

    BasicData::send_uml_def(com, bn, comment);
    com->write_bool(FALSE);	// class member

    if (api >= 13)
        com->write_bool(FALSE);	// volatile

    com->write_char(((api >= 23) ||
                     (uml_visibility != UmlPackageVisibility))
                    ? uml_visibility : UmlPublic);

    if (api >= 30)
        com->write_string(constraint);

    com->write_bool(is_abstract);

    if (stereotype == "typedef") {
        BrowserNodeList inh;

        browser_node->children(inh, UmlGeneralisation, UmlRealize);

        if (inh.count() != 0)
            // first inheritance is taken in all cases
            ((RelationData *) inh.at(0)->get_data())->get_end_class()->write_id(com);
        else if (base_type.type != 0)
            base_type.type->write_id(com);
        else if (!base_type.explicit_type.isEmpty()) {
            com->write_id(0);
            com->write_string(base_type.explicit_type);
        }
        else {
            // no base_type, try with a dependency
            browser_node->children(inh, UmlDependency);

            if (inh.count() != 0)
                ((RelationData *) inh.at(0)->get_data())->get_end_class()->write_id(com);
            else {
                com->write_id(0);
                com->write_string("");
            }
        }
    }

    if (api >= 48)
        com->write_bool(is_active);
}

void ClassData::send_cpp_def(ToolCom * com)
{
    int api = com->api_format(true);

    com->write_string(cpp_decl);

    if (api >= 23)
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

    if (api < 13)
        com->write_bool(FALSE);	// volatile

    com->write_bool(cpp_external);
}

void ClassData::send_java_def(ToolCom * com)
{
    int api = com->api_format(true);

    com->write_string(java_decl);

    if (api >= 21)
        com->write_string(java_annotation);

    if (api < 29)
        com->write_bool(uml_visibility == UmlPublic);

    com->write_bool(java_final);
    com->write_bool(java_external);

    if ((api >= 19) && (api < 21))
        com->write_string(java_annotation);
}

void ClassData::send_php_def(ToolCom * com)
{
    com->write_string(php_decl);
    com->write_bool(php_final);
    com->write_bool(php_external);
}

void ClassData::send_python_def(ToolCom * com)
{
    com->write_string(python_decl);
    com->write_bool(python_2_2);
    com->write_bool(python_external);
}

void ClassData::send_idl_def(ToolCom * com)
{
    com->write_string(idl_decl);
    switch_type.send_def(com);
    com->write_bool(idl_local);
    com->write_bool(idl_custom);
    com->write_bool(idl_external);
}

//

void ClassData::save(QTextStream & st, QString & warning) const
{
    nl_indent(st);

    if (is_abstract)
        st << "abstract ";

    if (is_active)
        st << "active ";

    st << "visibility " << stringify(uml_visibility) << " ";

    if (!stereotype.isEmpty()) {
        st << "stereotype ";
        save_string(stereotype, st);

        if (stereotype == "typedef")
            base_type.save(st, warning, " base_type ", " explicit_base_type ");
    }

    nl_indent(st);

    if (nformals != 0) {
        st << "nformals " << nformals;

        for (int i = 0; i != nformals; i += 1)
            formals[i].save(st, warning);

        nl_indent(st);
    }

    if (actuals.count() != 0) {
        st << "nactuals " << actuals.count();

        foreach (ActualParamData *data, actuals)
            data->save(st, warning);

        nl_indent(st);
    }

    if (!constraint.isEmpty()) {
        nl_indent(st);
        st << "constraint ";
        save_string(constraint, st);
    }

    if (cpp_external)
        st << "cpp_external ";

    if (cpp_visibility != UmlDefaultVisibility)
        st << "cpp_visibility " << stringify(cpp_visibility) << " ";

    st << "cpp_decl ";
    save_string(cpp_decl, st);

    nl_indent(st);

    if (java_external)
        st << "java_external ";

    if (java_final)
        st << "final ";

    st << "java_decl ";
    save_string(java_decl, st);

    if (! java_annotation.isEmpty()) {
        nl_indent(st);
        st << "java_annotation ";
        save_string(java_annotation, st);
    }

    nl_indent(st);

    if (php_external)
        st << "php_external ";

    if (php_final)
        st << "php_final ";

    st << "php_decl ";
    save_string(php_decl, st);

    nl_indent(st);

    if (python_external)
        st << "python_external ";

    if (python_2_2)
        st << "python_2_2 ";

    st << "python_decl ";
    save_string(python_decl, st);

    nl_indent(st);

    if (idl_external)
        st << "idl_external ";

    if (idl_local)
        st << "local ";

    if (idl_custom)
        st << "custom ";

    st << "idl_decl ";
    save_string(idl_decl, st);
    nl_indent(st);
    switch_type.save(st, warning, "switch_type ", "explicit_switch_type ");
    nl_indent(st);
}

void ClassData::read(char *& st, char *& k)
{
    if (!strcmp(k, "abstract")) {
        is_abstract = TRUE;
        k = read_keyword(st);
    }
    else
        is_abstract = FALSE;

    if (!strcmp(k, "active")) {
        is_active = TRUE;
        k = read_keyword(st);
    }
    else
        is_active = FALSE;

    if (!strcmp(k, "visibility")) {
        uml_visibility = ::visibility(read_keyword(st));
        k = read_keyword(st);
    }
    else {
        // old non nested class
        uml_visibility = UmlPackageVisibility;
    }

    if (!strcmp(k, "stereotype")) {
        set_stereotype(read_string(st));

        if (!strcmp(stereotype, "typedef")) {
            AType t;

            t.read(st, "base_type", "explicit_base_type");
            set_base_type(t);
        }

        k = read_keyword(st);
    }

    unsigned n, i;

    if (!strcmp(k, "nformals")) {
        n = read_unsigned(st);
        set_n_formalparams(n);

        for (i = 0; i != n; i += 1)
            formals[i].read(st);

        k = read_keyword(st);
    }
    else
        set_n_formalparams(0);

    if (!strcmp(k, "nactuals")) {
        n = read_unsigned(st);

        BrowserClass * cl = 0;

        for (i = 0; i != n; i += 1) {
            ActualParamData * actual = ActualParamData::read(st);

            actuals.append(actual);

            if (actual->get_class() != cl) {
                cl = actual->get_class();
                connect(cl->get_data(), SIGNAL(deleted()),
                        this, SLOT(update_actuals()));
                connect(cl->get_data(), SIGNAL(changed()),
                        this, SLOT(update_actuals()));
            }
        }

        k = read_keyword(st);
    }
    else {
        n = 0;
        actuals.clear();
    }

    if (!strcmp(k, "constraint")) {
        constraint = read_string(st);
        k = read_keyword(st);
    }
    else
        constraint = QString();

    if (!strcmp(k, "cpp_external")) {
        cpp_external = TRUE;
        k = read_keyword(st);
    }
    else
        cpp_external = FALSE;

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
        wrong_keyword(k, "cpp_decl");

    if (!strcmp(k, "java_external")) {
        java_external = TRUE;
        k = read_keyword(st);
    }
    else
        java_external = FALSE;

    if (read_file_format() <= 33) {
        // old file
        if ((cpp_visibility == UmlDefaultVisibility) &&
            (uml_visibility != UmlPublic) &&
            (uml_visibility != UmlPackageVisibility))
            cpp_visibility = uml_visibility;

        if (!strcmp(k, "public")) {
            uml_visibility = UmlPublic;
            k = read_keyword(st);
        }
        else
            uml_visibility = UmlPackageVisibility;
    }

    if (!strcmp(k, "final")) {
        java_final = TRUE;
        k = read_keyword(st);
    }
    else
        java_final = FALSE;

    if (!strcmp(k, "java_decl")) {
        java_decl = read_string(st);
        k = read_keyword(st);
    }
    else
        wrong_keyword(k, "java_decl");

    if (!strcmp(k, "java_annotation")) {
        java_annotation = read_string(st);
        k = read_keyword(st);
    }
    else
        java_annotation = QString();

    if (!strcmp(k, "php_external")) {
        php_external = TRUE;
        k = read_keyword(st);
    }
    else
        php_external = FALSE;

    if (!strcmp(k, "php_final")) {
        php_final = TRUE;
        k = read_keyword(st);
    }
    else
        php_final = FALSE;

    if (!strcmp(k, "php_decl")) {
        php_decl = read_string(st);
        k = read_keyword(st);
    }
    else if (read_file_format() >= 44)
        wrong_keyword(k, "php_decl");
    else
        php_decl = "";

    if (!strcmp(k, "python_external")) {
        python_external = TRUE;
        k = read_keyword(st);
    }
    else
        python_external = FALSE;

    if (!strcmp(k, "python_2_2")) {
        python_2_2 = TRUE;
        k = read_keyword(st);
    }
    else
        python_2_2 = (read_file_format() < 51);

    if (!strcmp(k, "python_decl")) {
        python_decl = read_string(st);
        k = read_keyword(st);
    }
    else if (read_file_format() >= 51)
        wrong_keyword(k, "python_decl");
    else
        python_decl = "";

    if (!strcmp(k, "idl_external")) {
        idl_external = TRUE;
        k = read_keyword(st);
    }
    else
        idl_external = FALSE;

    if (!strcmp(k, "local")) {
        idl_local = TRUE;
        k = read_keyword(st);
    }
    else
        idl_local = FALSE;

    if (!strcmp(k, "custom")) {
        idl_custom = TRUE;
        k = read_keyword(st);
    }
    else
        idl_custom = FALSE;

    if (!strcmp(k, "idl_decl")) {
        idl_decl = read_string(st);
        k = read_keyword(st);
    }
    else
        wrong_keyword(k, "idl_decl");

    AType t;

    t.read(st, "switch_type", "explicit_switch_type", k);
    set_switch_type(t);

    k = read_keyword(st);
}
