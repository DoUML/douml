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

#ifndef CLASSDATA_H
#define CLASSDATA_H





#include "AType.h"
#include "UmlEnum.h"
#include "BasicData.h"
//Added by qt3to4:
#include <QTextStream>
//Added by qt3to4:
#include <Q3PtrList>

class ActualParamData;
class FormalParamData;

class ClassData : public BasicData
{
    Q_OBJECT

    friend class ClassDialog;
    friend class ActualParamsTable;
    //friend class QuickEdit;

protected:
    WrapperStr constraint;
    FormalParamData * formals;		// remark : do NOT use QArray
    QList<ActualParamData *> actuals;
    AType base_type;			// typedef
    unsigned char nformals;
    bool is_deleted : 1;
    bool is_abstract : 1;
    bool is_active : 1;
    bool bodies_read : 1;
    bool bodies_modified : 1;
    bool cpp_external : 1;		// C++
    bool java_external : 1;		// Java
    bool java_final : 1;		// Java
    bool php_external : 1;		// Php
    bool php_final : 1;			// Php
    bool python_external : 1;		// Python
    bool python_2_2 : 1;		// Python 2.2 ..2.6
    bool idl_external : 1;		// Idl
    bool idl_local : 1;			// Idl (interface)
    bool idl_custom : 1;		// Idl (valuetype)
    UmlVisibility uml_visibility : 4;

    // C++
    UmlVisibility cpp_visibility : 4;
    WrapperStr cpp_decl;

    // Java
    WrapperStr java_decl;
    WrapperStr java_annotation;

    // Php
    WrapperStr php_decl;

    // Python
    WrapperStr python_decl;

    // IDL
    AType switch_type;			// union
    WrapperStr idl_decl;

    static bool DontUpdateActuals;

    //

    virtual void send_uml_def(ToolCom * com, BrowserNode * bn,
                              const QString & comment);
    virtual void send_cpp_def(ToolCom * com);
    virtual void send_java_def(ToolCom * com);
    virtual void send_php_def(ToolCom * com);
    virtual void send_python_def(ToolCom * com);
    virtual void send_idl_def(ToolCom * com);

    void update_actuals(BrowserClass *,
                        QList<ActualParamData *> & new_actuals,
                        QList<ActualParamData *> & managed);
    void get_actuals(QList<ActualParamData *> & l, BrowserClass * parent);

public:
    ClassData();
    ClassData(const ClassData * model, BrowserNode * bn);
    virtual ~ClassData();

    virtual bool deletedp() const;
    virtual void set_deletedp(bool y);

    void edit();

    const char * get_constraint() const {
        return constraint;
    }

    virtual bool decldefbody_contain(const QString & s, bool cs, BrowserNode *);

    bool get_is_abstract() const {
        return is_abstract;
    }
    void set_is_abstract(bool yes);

    bool get_is_active() const {
        return is_active;
    }
    void set_is_active(bool yes);

    const AType & get_base_type() const {
        return base_type;
    }
    QString get_full_base_type_def() const;
    void set_base_type(const AType & t);

    void inherit_or_instantiate(BrowserClass * other);

    UmlVisibility get_uml_visibility() const {
        return uml_visibility;
    }
    UmlVisibility get_cpp_visibility() const {
        return cpp_visibility;
    }
    void set_uml_visibility(UmlVisibility v);
    void set_uml_visibility(int v);
    void set_cpp_visibility(UmlVisibility v);

    int get_n_formalparams() const {
        return nformals;
    }
    const char * get_formalparam_type(int) const;
    const char * get_formalparam_name(int) const;
    QString get_formalparam_default_value(int, bool) const;
    QString get_formalparam_extends(int, bool) const;
    void set_n_formalparams(int);
    void set_formalparam_type(int, const char *);
    void set_formalparam_name(int, const char *);
    void set_formalparam_default_value(int, BrowserClass * t, const char * e);
    void set_formalparam_extends(int, BrowserClass * t, const char * e);
    void get_template_prefixes(QString & template1, QString & template2) const;
    void get_class_spec(QString & templates, QString & names,
                        QString & templates_tmplop, QString & names_tmplop) const;
    void addFormals(QStringList &);

    int get_n_actualparams() const {
        return actuals.count();
    }
    QString get_actualparam_name(int);	// cannot be const, computed QString
    QString get_actualparam_value(int);	// cannot be const, computed QString
    void set_actualparam_value(int, const AType & t);
    QString get_actuals(BrowserClass * parent);

    const char * get_cppdecl() const {
        return cpp_decl;
    }
    bool cpp_is_external() const {
        return cpp_external;
    }
    void set_cpp_is_external(bool value) {
        cpp_external = value;
    }

    const char * get_javadecl() const {
        return java_decl;
    }
    bool java_is_external() const {
        return java_external;
    }
    bool java_is_final() const {
        return java_final;
    }

    const char * get_phpdecl() const {
        return php_decl;
    }
    bool php_is_external() const {
        return php_external;
    }
    bool php_is_final() const {
        return php_final;
    }

    const char * get_pythondecl() const {
        return python_decl;
    }
    bool python_is_external() const {
        return python_external;
    }
    bool python_is_2_2() const {
        return python_2_2;
    }

    const char * get_idldecl() const {
        return idl_decl;
    }
    const AType & get_switch_type() const {
        return switch_type;
    }
    void set_switch_type(const AType & t);
    bool idl_is_external() const {
        return idl_external;
    }
    bool idl_is_local() const {
        return idl_local;
    }
    bool idl_is_custom() const {
        return idl_custom;
    }

    bool reference(BrowserClass *) const;

    virtual bool tool_cmd(ToolCom * com, const char * args,
                          BrowserNode * bn, const QString & comment);

    void save(QTextStream &, QString & warning) const;
    void read(char *& st, char *& k);

    bool get_bodies_read() {
        return bodies_read;
    }
    void set_bodies_read(bool y) {
        bodies_read = y;
    }
    bool get_bodies_modified() {
        return bodies_modified;
    }
    void set_bodies_modified(bool y) {
        bodies_modified = y;
    }
    void manage_deleted();

    static void setDontUpdateActuals(bool y) {
        DontUpdateActuals = y;
    }

    class ToolCmd
    {
    public:
        ToolCmd() {
            ClassData::setDontUpdateActuals(TRUE);
        }
        ~ToolCmd() {
            ClassData::setDontUpdateActuals(FALSE);
        }
    };

private slots:
    void update_actuals();
    void on_delete();

signals:
    void actuals_changed();
};

#endif
