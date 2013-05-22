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

#ifndef OPERATIONDIALOG_H
#define  OPERATIONDIALOG_H

#include <q3tabdialog.h>
//Added by qt3to4:
#include <Q3PtrList>
#include <Q3Grid>
#include <QVBoxLayout>
#include <QLabel>

#include "MyTable.h"
#include "BrowserNode.h"
#include "VisibilityGroup.h"
#include "dialog/EdgeMenuDialog.h"
#include <Q3HBox>

class Q3ComboBox;
class QCheckBox;
class QRadioButton;
class QPushButton;

class LineEdit;
class MultiLineEdit;
class BrowserOperation;
class OperationData;
class ClassData;
class AttributeData;
class RelationData;
class BrowserNode;
class ParamsTable;
class CppParamsTable;
class ExceptionsTable;
class KeyValuesTable;
class BodyDialog;
class OperationWidgetCpp;

class OperationDialog : public EdgeMenuDialog
{
    Q_OBJECT

protected:
    virtual void closeEvent(QCloseEvent *);
    bool visit;
    OperationData * oper;
    ClassData * cl;
    QStringList list;
    BrowserNodeList nodes;
    QList<BodyDialog *> edits;
    BrowserNode * view;
    AttributeData * get_of_attr;
    AttributeData * set_of_attr;
    RelationData * get_of_rel = nullptr;
    RelationData * set_of_rel = nullptr;
    bool is_rel_a = false;
    DrawingLanguage unique;

    // uml tab
    QWidget * umltab;
    Q3Grid * umlGrid;
    LineEdit * edname;
    Q3ComboBox * edreturn_type;
    int edreturn_type_offset;
    Q3ComboBox * edstereotype;
    VisibilityGroup uml_visibility;
    QCheckBox * classoperation_cb;
    QCheckBox * abstract_cb;
    QCheckBox * forcegenbody_cb;
    ParamsTable * table;
    ExceptionsTable * etable;
    MultiLineEdit * comment;
    MultiLineEdit * constraint;

    // c++ tab
    OperationWidgetCpp * cppTab;
    bool cpp_undef;
    VisibilityGroup cpp_visibility;
    QCheckBox * cbCppFriend;
    QCheckBox * cbCppConst;
    QCheckBox * cbCppVolatile;
    QCheckBox * cbCppVirtual;
    QCheckBox * cbCppInline;
    Q3ButtonGroup * cppBg;
    Q3ButtonGroup * cppBg2;

    Q3HBox * htabBgCpp1;
    Q3HBox * tabBgCppModifiers;


    CppParamsTable * paramcppdecltable;
    CppParamsTable * paramcppdeftable;
    QString cppbody;
    QString oldcppbody;
    QString templates;
    QString cl_names;
    QString templates_tmplop;
    QString cl_names_tmplop;

    // java tab
    QWidget * javatab;
    bool java_undef;
    QCheckBox * javafinal_cb;
    QCheckBox * synchronized_cb;
    LineEdit * edjavanamespec;	// get/set
    QCheckBox * javafrozen_cb;	// get/set
    QCheckBox * indentjavabody_cb;
    MultiLineEdit * edjavadef;
    MultiLineEdit * showjavadef;
    QPushButton * editjavabody;
    QPushButton * editjavaannotation;
    QString javaannotation;
    QString javabody;
    QString oldjavabody;

    // php tab
    QWidget * phptab;
    bool php_undef;
    QCheckBox * phpfinal_cb;
    LineEdit * edphpnamespec;	// get/set
    QCheckBox * phpfrozen_cb;	// get/set
    QCheckBox * indentphpbody_cb;
    MultiLineEdit * edphpdef;
    MultiLineEdit * showphpdef;
    QPushButton * editphpbody;
    QString phpbody;
    QString oldphpbody;

    // python tab
    QWidget * pythontab;
    bool python_undef;
    LineEdit * edpythonnamespec;	// get/set
    QCheckBox * pythonfrozen_cb;	// get/set
    QCheckBox * indentpythonbody_cb;
    MultiLineEdit * edpythondef;
    MultiLineEdit * showpythondef;
    QPushButton * editpythonbody;
    QPushButton * editpythondecorator;
    QString pythondecorator;
    QString pythonbody;
    QString oldpythonbody;

    // idl tab
    QWidget * idltab;
    bool idl_undef;
    QCheckBox * oneway_cb;
    LineEdit * edidlnamespec;	// get/set
    QCheckBox * idlfrozen_cb;	// get/set
    MultiLineEdit * edidldecl;
    MultiLineEdit * showidldecl;

    // user
    KeyValuesTable * kvtable;

    static QSize previous_size;

    QLabel * lblFullClassName;
    //QLabel* lblNameFormCpp;
    QLabel * lblNameFormPhp;
    QLabel * lblNameFormJava;
    QLabel * lblNameFormPython;
    QLabel * lblNameFormIdl;
    QPushButton * pbValueType;
    QPushButton * pbEditor;
    QPushButton * pbDefault;
    QPushButton * pbConstraintEditor;
    Q3ButtonGroup * bgUml1;
    Q3ButtonGroup * bgUml2;
    Q3ButtonGroup * bgUml3;

    Q3ButtonGroup * notableBg;
    Q3ButtonGroup * bgPhp;
    Q3ButtonGroup * bgPython;
    Q3ButtonGroup * bgIdl;
    Q3ButtonGroup * bgCppModifiers;
    Q3ButtonGroup * bgCpp11Modifiers;
    Q3ButtonGroup * visibilityBg;

    Q3HBox * htabBgUml;
    Q3HBox * namespecTab;
    Q3HBox * namespecTabIdl;
    Q3HBox * pythonNamespecTab;





    QPushButton * pbDefaultDeclarationPhp;
    QPushButton * pbNotGeneratedInPhp;
    QPushButton * pbEditParametersPhp;


    QPushButton * pbNotGeneratedInJava;
    QPushButton * pbDefaultDeclarationJava;

    QPushButton * pbDefaultDeclarationPython;
    QPushButton * pbNotGeneratedInPython;
    QPushButton * pbEditParamsPython;

    QPushButton * editidlbody;
    QPushButton * pbDefaultDeclarationIdl;
    QPushButton * pbNotGeneratedInIdl;

    virtual void InitGui();
    virtual void FillGuiElements(OperationData *);
    virtual void FillGuiElements(BrowserNode *);
    virtual uint TypeID();

    DrawingLanguage drawingLanguage;

    static QSharedPointer<OperationDialog> instance;


protected:
    void manage_decorators(QString & s, QString indent,
                           BooL & indent_needed);
    void init_get_set();
    void init_uml();
    void init_cpp();
    void init_java();
    void init_php();
    void init_python();
    void init_idl();
    void InitPropertiesTab();

    void FillUmlTab(OperationData *);
    void FillcppTab(OperationData *);
    void FillJavaTab(OperationData *);
    void FillPhpTab(OperationData *);
    void FillPythonTab(OperationData *);
    void FillIdlTab(OperationData *);
    void FillPropertiesTab(OperationData *);
    void FillGeneric(/*OperationData * oper*/);

    QString compute_name(LineEdit * spec);
    void manage_cpp_type(unsigned rank, QString & s);
    void manage_cpp_exceptions(QString & s);
    void manage_var(unsigned rank, QString & s);
    void manage_init(unsigned rank, QString & s);
    void manage_java_type(unsigned rank, QString & s);
    void manage_java_exceptions(QString & s);
    void manage_php_type(unsigned rank, QString & s);
    void manage_python_type(unsigned rank, QString & s, bool in_params);
    void manage_idl_type(unsigned rank, QString & s);
    void manage_dir(unsigned rank, QString & s);
    void manage_idl_exceptions(QString & s);

    void add_param(QString & form, int rank, QString s);
    void replace_param(QString & form, int rank, QString s);
    void insert_param(int rank, MultiLineEdit * ed);
    QString delete_param(int rank, MultiLineEdit * ed);
    void move_param(int old_rank, int new_rank, MultiLineEdit * ed);

    static void post_edit_description(OperationDialog * d, QString s);
    static void post_edit_constraint(OperationDialog * d, QString s);
    static void post_cpp_edit_body(OperationDialog * d, QString s);
    static void post_java_edit_body(OperationDialog * d, QString s);
    static void post_python_edit_body(OperationDialog * d, QString s);
    static void post_php_edit_body(OperationDialog * d, QString s);

    bool SaveData() override;
    bool SaveData(OperationData* oper);

public:
    OperationDialog(OperationData * a, DrawingLanguage l);
    virtual ~OperationDialog();

    static QString cpp_decl(const BrowserOperation * op, bool withname,
                            ShowContextMode mode);
    static QString java_decl(const BrowserOperation * op, bool withname,
                             ShowContextMode mode);
    static QString php_decl(const BrowserOperation * op, bool withname,
                            ShowContextMode mode);
    static QString python_decl(const BrowserOperation * op, bool withname,
                               ShowContextMode mode);
    static QString idl_decl(const BrowserOperation * op, bool withdir,
                            bool withname, ShowContextMode mode);

    void force_param(int rank, bool recompute);
    void insert_param(int rank);
    void delete_param(int rank);
    void move_param(int old_rank, int new_rank);
    static QSharedPointer<OperationDialog> Instance(OperationData * , DrawingLanguage l);

protected slots:
    virtual void polish();
    virtual void accept();
    void default_description();
    void edit_description();
    void edit_constraint();
    void classoper_toggled(bool on);
    void abstract_toggled(bool on);
    void forcegenbody_toggled(bool on);
    void cpp_default_decl();
    void cpp_decl_from_def();
    void cpp_unmapped_decl();
    void cpp_update_decl();
    void cpp_edit_param_decl();
    void cpp_def_from_decl();
    void cpp_default_def();
    void cpp_unmapped_def();
    void cpp_update_def();
    void cpp_edit_param_def();
    void cpp_edit_body();
    void const_volatile_toggled(bool on);
    void friend_toggled(bool on);
    void virtual_toggled(bool on);
    void inline_toggled(bool on);
    void final_toggled(bool on);
    void delete_toggled(bool on);
    void override_toggled(bool on);
    void default_toggled(bool on);
    void java_default_def();
    void java_unmapped_def();
    void java_update_def();
    void java_edit_body();
    void java_edit_annotation();
    void java_finalsynchronized_toggled(bool on);
    void php_default_def();
    void php_unmapped_def();
    void php_update_def();
    void php_edit_body();
    void php_final_toggled(bool on);
    void php_edit_param();
    void python_default_def();
    void python_unmapped_def();
    void python_update_def();
    void python_edit_body();
    void python_edit_param();
    void python_edit_decorator();
    void idl_default_def();
    void idl_unmapped_def();
    void idl_update_decl();
    void oneway_toggled(bool on);
    void update_all_tabs(QWidget *);
    void menu_returntype();
};

class ParamsTable : public MyTable
{
    Q_OBJECT

protected:
    OperationDialog * dialog;
    const QStringList & types;
    QStringList alltypes;

    static QString dir_copy;		// copy/cut/paste
    static QString name_copy;
    static QString type_copy;
    static QString default_value_copy;

public:
    ParamsTable(OperationData * a, QWidget * parent,
                const QStringList & list,
                OperationDialog * d, bool isReadOnly);

    void update(OperationData * o,
                const QStringList & list,
                OperationDialog * d, bool isReadOnly);

protected:
    virtual void activateNextCell();

    void insert_row_before(int row);
    void insert_row_after(int row);
    void delete_row(int row);
    void copy_row(int row);
    void cut_row(int row);
    void paste_row(int row);
    void move_row(int from, int to);

public:
    void update(OperationData * oper, BrowserNodeList & nodes);
    unsigned nparams() const;
    QString name(unsigned rank) const;
    QString type(unsigned rank) const;
    UmlParamDirection dir(unsigned rank) const;
    QString value(unsigned rank) const;

    QStringList all_types();
    QStringList all_names();
    QStringList all_values();

protected slots:
    void button_pressed(int row, int col, int button, const QPoint & mousePos);
    void value_changed(int row, int col);
};

class ExceptionsTable : public MyTable
{
    Q_OBJECT

protected:
    QStringList & types;

    static QString type_copy;		// copy/cut/paste

public:
    ExceptionsTable(OperationData * a, QWidget * parent,
                    QStringList & list, bool visit);
    void Reinitialize(OperationData * a, QStringList & list, bool visit);

protected:
    virtual void activateNextCell();

    void insert_row_before(int row);
    void insert_row_after(int row);
    void delete_row(int row);
    void copy_row(int row);
    void cut_row(int row);
    void paste_row(int row);
    void move_row(int from, int to);

public:
    void update(OperationData * oper, BrowserNodeList & nodes);
    unsigned nexceptions() const;
    QString type(unsigned rank) const;

protected slots:
    void button_pressed(int row, int col, int button, const QPoint & mousePos);
    void value_changed(int row, int col);
};

class CppParamsTable : public MyTable
{
    Q_OBJECT

protected:
    ParamsTable * params;
    MultiLineEdit * edform;
    bool dcl;

    static QString copied[8];		// copy/cut/paste

public:
    CppParamsTable(ParamsTable * p, MultiLineEdit * f,
                   QWidget * parent, bool dcl);
    void update_edform();

protected:
    bool extract(int tblindex, int & strindex, QString s);

    void insert_row_before(int row);
    void insert_row_after(int row);
    void delete_row(int row);
    void copy_row(int row);
    void cut_row(int row);
    void paste_row(int row);
    void move_row(int from, int to);
    void init_row(int row);
    void update_name(int row);
    void update_names();

    virtual void setItem(int row, int col, Q3TableItem * item);

protected slots:
    void button_pressed(int row, int col, int button, const QPoint & mousePos);
    virtual void setCurrentCell(int row, int col);
};

class CppParamsDialog : public QDialog
{
    Q_OBJECT

protected:
    CppParamsTable * tbl;

    static QSize previous_size;

public:
    CppParamsDialog(QWidget * parent, ParamsTable * params,
                    MultiLineEdit * form, bool decl);
    virtual ~CppParamsDialog();

protected slots:
    virtual void polish();
    virtual void accept();
};

class PhpParamsTable : public MyTable
{
    Q_OBJECT

protected:
    ParamsTable * params;
    MultiLineEdit * edform;

    static QString copied[6];		// copy/cut/paste

public:
    PhpParamsTable(QWidget * parent, ParamsTable * p,
                   MultiLineEdit * f);
    void update_edform();

protected:
    bool extract(int tblindex, int & strindex, QString s);

    void insert_row_before(int row);
    void insert_row_after(int row);
    void delete_row(int row);
    void copy_row(int row);
    void cut_row(int row);
    void paste_row(int row);
    void move_row(int from, int to);
    void init_row(int row);
    void update_name(int row);
    void update_names();

    virtual void setItem(int row, int col, Q3TableItem * item);

protected slots:
    void button_pressed(int row, int col, int button, const QPoint & mousePos);
    virtual void setCurrentCell(int row, int col);
};

class PhpParamsDialog : public QDialog
{
    Q_OBJECT

protected:
    PhpParamsTable * tbl;

    static QSize previous_size;

public:
    PhpParamsDialog(QWidget * parent, ParamsTable * params,
                    MultiLineEdit * form);
    virtual ~PhpParamsDialog();

protected slots:
    virtual void polish();
    virtual void accept();
};

class PythonParamsTable : public MyTable
{
    Q_OBJECT

protected:
    ParamsTable * params;
    MultiLineEdit * edform;

    static QString copied[6];		// copy/cut/paste

public:
    PythonParamsTable(QWidget * parent, ParamsTable * p, MultiLineEdit * f);
    void update_edform();

protected:
    bool extract(int tblindex, int & strindex, QString s);

    void insert_row_before(int row);
    void insert_row_after(int row);
    void delete_row(int row);
    void copy_row(int row);
    void cut_row(int row);
    void paste_row(int row);
    void move_row(int from, int to);
    void init_row(int row);
    void update_name(int row);
    void update_names();

    virtual void setItem(int row, int col, Q3TableItem * item);

protected slots:
    void button_pressed(int row, int col, int button, const QPoint & mousePos);
    virtual void setCurrentCell(int row, int col);
};

class PythonParamsDialog : public QDialog
{
    Q_OBJECT

protected:
    PythonParamsTable * tbl;

    static QSize previous_size;

public:
    PythonParamsDialog(QWidget * parent, ParamsTable * params, MultiLineEdit * form);
    virtual ~PythonParamsDialog();

protected slots:
    virtual void polish();
    virtual void accept();
};

#endif
