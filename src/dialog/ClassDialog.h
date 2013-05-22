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

#ifndef CLASSDIALOG_H
#define  CLASSDIALOG_H





#include <q3tabdialog.h>
//Added by qt3to4:
#include <QLabel>
//Added by qt3to4:
#include <Q3PtrList>
#include <Q3HBox>
#include <Q3Grid>
#include <QSharedPointer>

#include "MyTable.h"
#include "BrowserNode.h"
#include "VisibilityGroup.h"
#include "dialog/EdgeMenuDialog.h"

class Q3VBox;
class QLabel;
class Q3ComboBox;
class QCheckBox;
class Q3GroupBox;

class LineEdit;
class MultiLineEdit;
class ClassData;
class BrowserNode;
class FormalParamsTable;
class ActualParamsTable;
class ApplicableOnTable;
class KeyValuesTable;
class BodyDialog;

class ClassDialog : public EdgeMenuDialog
{
    Q_OBJECT

protected:
    QWidget * umltab;
    ClassData * cl;
    LineEdit * edname;
    Q3ComboBox * edstereotype;
    VisibilityGroup uml_visibility;
    QLabel * basetypelbl;
    Q3ComboBox * edbasetype;
    QCheckBox * abstract_cb;
    QCheckBox * active_cb;
    Q3GroupBox * opt_bg;
    Q3ComboBox * artifact;
    MultiLineEdit * comment;
    MultiLineEdit * constraint;
    FormalParamsTable * formals_table;
    Q3VBox * parametrized_vtab;
    BrowserNodeList nodes;
    QStringList node_names;
    BrowserNodeList artifacts;
    QList<BodyDialog *> edits;

    ActualParamsTable * actuals_table;
    Q3VBox * instantiate_vtab;

    // C++
    QWidget * cppTab;
    QString current_cpp_stereotype;
    VisibilityGroup cpp_visibility;
    QCheckBox * cpp_external_cb;
    MultiLineEdit * edcppdecl;
    MultiLineEdit * showcppdecl;

    // Java
    QWidget * javatab;
    QString current_java_stereotype;
    QCheckBox * java_external_cb;
    QCheckBox * java_final_cb;
    MultiLineEdit * edjavadecl;
    MultiLineEdit * showjavadecl;
    //QPushButton * editjavaannotation;
    QString javaannotation;

    // Php
    QWidget * phptab;
    QString current_php_stereotype;
    QCheckBox * php_external_cb;
    QCheckBox * php_final_cb;
    MultiLineEdit * edphpdecl;
    MultiLineEdit * showphpdecl;

    // Python
    QWidget * pythontab;
    QString current_python_stereotype;
    QCheckBox * python_external_cb;
    QCheckBox * python_2_2_cb;
    MultiLineEdit * edpythondecl;
    MultiLineEdit * showpythondecl;

    // Idl
    QWidget * idltab;
    QString current_idl_stereotype;
    QCheckBox * idl_external_cb;
    MultiLineEdit * edidldecl;
    MultiLineEdit * showidldecl;
    Q3GroupBox * switch_bg;
    Q3ComboBox * edswitch_type;
    QCheckBox * idl_local_cb;
    QCheckBox * idl_custom_cb;

    // profiled stereotype
    QWidget * stereotypetab;
    Q3ComboBox * stereo_init_cb;
    LineEdit * edinitparam;
    Q3ComboBox * stereo_check_cb;
    LineEdit * edcheckparam;
    LineEdit * ediconpath;
    QPushButton * iconpathrootbutton;
    QPushButton * iconpathprjbutton;

    ApplicableOnTable * applicableon_table;

    // user
    KeyValuesTable * kvtable;

    static QSize previous_size;

    // added buttons
    QPushButton * pbEditorForDescription;
    QPushButton * pbDefaultForDescription;
    QPushButton * pbEditorForConstrant;
    QPushButton * pbCppDefaultDeclaration;
    QPushButton * pbNotGeneratedInCPP;
    QPushButton * pbJavaDefaultDefinition;
    QPushButton * pbNotGeneratedInJava;
    QPushButton * pbJavaAnnotation;
    QPushButton * pbPhpDefaultDefinition;
    QPushButton * pbNotGeneratedInPhp;
    QPushButton * pbPythonDefaultDefinition;
    QPushButton * pbNotGeneratedInPython;
    QPushButton * pbIdlDefaultDeclaration;
    QPushButton * pbINotGeneratedInIdl;
    QLabel * instantiateNotice;
    QLabel * lblProfiledEmpty;
    QPushButton * pbProfiledSteretypeBrowse;
    QLabel * lblProfiledEmpty2;
    QLabel * lblProfiledEmpty3;
    Q3VBox * vtabProfiled;
    Q3ButtonGroup * bgvUml;
    Q3ButtonGroup * bgvCpp;
    Q3ButtonGroup * bgvJava;
    Q3ButtonGroup * bgvPython;
    Q3ButtonGroup * bgvPhp;
    Q3ButtonGroup * bgvIDL;
    Q3HBox * htabcpp;
    Q3HBox * htabidl;
    QLabel * lbl1cpp;
    QLabel * lbl2cpp;
    QLabel * lbl3cpp;
    QLabel * lbl4cpp;


    QLabel * lbl1php;
    QLabel * lbl2php;
    QLabel * lbl3php;
    QLabel * lbl4php;

    QLabel * lbl1python;
    QLabel * lbl2python;
    QLabel * lbl3python;
    QLabel * lbl4python;

    QLabel * lbl1idl;
    QLabel * lbl2idl;
    QLabel * lbl3idl;
    QLabel * lbl4idl;

    QLabel * lbl1java;
    QLabel * lbl2java;
    QLabel * lbl3java;
    QLabel * lbl4java;
    Q3Grid * stereotypeGrid;
    Q3HBox * htabUml;
    QFont font;

    BrowserClass * currentNode;
    BrowserNode * grandParent;

    Q3VBox * keyValueTab;

    virtual uint TypeID();

    virtual void InitGui();
    void FillGuiElements(BrowserNode *);
    void FillGuiElements(ClassData * _cl);
    void NullPointers();
    bool SaveData();

    //refactoring changes

    static QSharedPointer<ClassDialog> instance;


public:
    ClassDialog(ClassData * c);
    virtual ~ClassDialog();

    static QSharedPointer<ClassDialog> Instance(ClassData *);

    static void cpp_generate_decl(QString & s, ClassData * cl,
                                  QString def, QString name,
                                  QString stereotype,
                                  QString basetype, QString comment,
                                  ActualParamsTable * actuals,
                                  FormalParamsTable * formals,
                                  BrowserNodeList & nodes,
                                  QStringList & node_names,
                                  KeyValuesTable * kvt);

    static void java_generate_decl(QString & s, ClassData * cl, QString def,
                                   QString annotation, QString name,
                                   QString stereotype,
                                   QString comment, UmlVisibility visibility,
                                   bool is_final, bool is_abstract,
                                   ActualParamsTable * actuals,
                                   FormalParamsTable * formals,
                                   BrowserNodeList & nodes,
                                   QStringList & node_names,
                                   KeyValuesTable * kvt);

    static void php_generate_decl(QString & s, ClassData * cl, QString def,
                                  QString name, QString stereotype,
                                  QString comment, bool is_final,
                                  bool is_abstract, KeyValuesTable * kvt);

    static void python_generate_decl(QString & s, ClassData * cl, QString def,
                                     QString name, QString stereotype,
                                     QString comment, bool is_python_2_2,
                                     BrowserNodeList & nodes,
                                     QStringList & node_names,
                                     KeyValuesTable * kvt);

    static void idl_generate_decl(QString & s, ClassData * cl, QString def,
                                  QString name, QString stereotype,
                                  QString basetype, QString comment,
                                  QString switch_type, bool is_local,
                                  bool is_custom, bool is_abstract,
                                  KeyValuesTable * kvt);

    static void cpp_generate_members_def(const BrowserNode * cl, QString & s);
    static QString python_instance_att_rel(BrowserNode * cl);

    static void post_edit_description(ClassDialog * d, QString s);
    static void post_edit_constraint(ClassDialog * d, QString s);

    static QString cpp_stereotype(const QString & stereotype);
    static QString java_stereotype(const QString & stereotype);
    static QString php_stereotype(const QString & stereotype);
    static QString python_stereotype(const QString & stereotype);
    static QString idl_stereotype(const QString & stereotype);


protected slots:
    virtual void polish();
    void accept();
    void default_description();
    void edit_description();
    void edit_constraint();
    void edStereotypeActivated(const QString &);
    void icon_browse();
    void icon_root_relative();
    void icon_prj_relative();
    void update_all_tabs(QWidget *);
    void cpp_update_decl();
    void cpp_default_decl();
    void cpp_unmapped_decl();
    void java_update_decl();
    void java_default_decl();
    void java_unmapped_decl();
    void java_edit_annotation();
    void php_update_decl();
    void php_default_decl();
    void php_unmapped_decl();
    void python_update_decl();
    void python_default_decl();
    void python_unmapped_decl();
    void idl_update_decl();
    void idl_default_decl();
    void idl_unmapped_decl();

//    void OnPickNextSibling();
//    void OnPickPreviousSibling();

};

class FormalParamsTable : public MyTable
{
    Q_OBJECT

protected:
    QStringList & types;

    static QString type_copy;
    static QString name_copy;		// copy/cut/paste
    static QString default_value_copy;
    static QString extends_copy;

public:
    FormalParamsTable(ClassData * a, QWidget * parent,
                      QStringList &list, bool visit);
    void Reinitiliaze(ClassData * cl,  QStringList & node_names, bool isWritable);

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
    void update(ClassData * c, BrowserNodeList & nodes);
    void generate_cpp(QString & s, BrowserNodeList & nodes,
                      QStringList & node_names, const QString & cl_name);
    void generate_java(QString & s, BrowserNodeList & nodes,
                       QStringList & node_names);

protected slots:
    void button_pressed(int row, int col, int button, const QPoint & mousePos);
    void value_changed(int row, int col);
};

class ActualParamsTable : public MyTable
{
protected:
    QStringList & types;

public:
    ActualParamsTable(ClassData * a, QWidget * parent,
                       QStringList & list, bool visit);
    void Reinitiliaze(ClassData * cl,  QStringList & node_names, bool isWritable);
    void generate(QString & s, ClassData * cl, BrowserNode * parent, bool cpp,
                  BrowserNodeList & nodes, QStringList & node_names);

protected:
    //virtual void activateNextCell();

public:
    void update(ClassData * c, BrowserNodeList & nodes);
};

class ApplicableOnTable : public MyTable
{
    Q_OBJECT

protected:
    bool ro;

public:
    ApplicableOnTable(QWidget * parent, QString, bool visit);

public:
    QString targets();

protected slots:
    void button_pressed(int row, int col, int, const QPoint &);
};

#endif
