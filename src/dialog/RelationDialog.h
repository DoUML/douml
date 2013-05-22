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

#ifndef RELATIONDIALOG_H
#define  RELATIONDIALOG_H

#include <qwidget.h>
#include <qstringlist.h>
#include <q3tabdialog.h>
//Added by qt3to4:
#include <Q3PtrList>

#include "BrowserNode.h"
#include "VisibilityGroup.h"

class QWidget;
class Q3ComboBox;
class QCheckBox;
class QPushButton;
class Q3GroupBox;
class Q3ButtonGroup;
class Q3VBox;
class QRadioButton;

class LineEdit;
class MultiLineEdit;
class RelationData;
class BrowserNode;
class KeyValuesTable;
class BrowserClass;
class ClassData;
struct RoleData;
class BodyDialog;

struct RoleDialog {
    // Uml
    LineEdit * edrole;
    Q3ComboBox * multiplicity;
    QString previous_multiplicity;
    LineEdit * edinit;
    VisibilityGroup uml_visibility;
    QCheckBox * classrelation_cb;
    QCheckBox * constrelation_cb;
    QCheckBox * derived_cb;
    QCheckBox * derivedunion_cb;
    QCheckBox * ordered_cb;
    QCheckBox * unique_cb;
    MultiLineEdit * comment;
    MultiLineEdit * constraint;
    QWidgetList opt;

    // C++
    bool cpp_undef;
    QCheckBox * cpp_virtual_inheritance_cb;
    VisibilityGroup cpp_visibility;
    QCheckBox * mutable_cb;
    QCheckBox * volatile_cb;
    MultiLineEdit * edcppdecl;
    MultiLineEdit * showcppdecl;
    QPushButton * cpp_default_decl_bt;
    QPushButton * cpp_include_in_header; // role a only
    QPushButton * cpp_unmapped_decl_bt;

    // Java
    bool java_undef;
    QCheckBox * transient_cb;
    MultiLineEdit * edjavadecl;
    MultiLineEdit * showjavadecl;
    QPushButton * java_default_decl_bt;
    QPushButton * java_unmapped_decl_bt;
    QPushButton * editjavaannotation;
    QString javaannotation;

    // Php
    bool php_undef;
    MultiLineEdit * edphpdecl;
    MultiLineEdit * showphpdecl;
    QPushButton * php_default_decl_bt;
    QPushButton * php_unmapped_decl_bt;

    // Python
    bool python_undef;
    MultiLineEdit * edpythondecl;
    MultiLineEdit * showpythondecl;
    QPushButton * python_default_decl_bt;
    QPushButton * python_unmapped_decl_bt;
    QString python_self;

    // Idl
    bool idl_undef;
    bool idl_in_struct;
    bool idl_in_union;
    bool idl_in_valuetype;
    QCheckBox * idl_truncatable_inheritance_cb;
    Q3ComboBox * edcase;
    BrowserNodeList enums;
    QStringList enum_names;
    MultiLineEdit * edidldecl;
    MultiLineEdit * showidldecl;
    QPushButton * idl_default_decl_bt;
    QPushButton * idl_unmapped_decl_bt;

    // User
    bool ro;
    KeyValuesTable * kvtable;
};

class RelationDialog : public Q3TabDialog
{
    Q_OBJECT

protected:
    bool visit;
    bool onstereotype;
    QWidget * umltab;
    QWidget * cppTab;
    QWidget * javatab;
    QWidget * phptab;
    QWidget * pythontab;
    QWidget * idltab;
    RelationData * rel;
    LineEdit * edname;
    Q3ComboBox * edtype;
    Q3ComboBox * edstereotype;
    RoleDialog a;
    RoleDialog b;
    UmlCode current_type;
    QStringList list;
    BrowserNodeList nodes;
    QString aOldText;
    QString bOldText;
    Q3ComboBox * edassociation;
    QWidgetList groupb;
    Q3GroupBox * cpp_b;
    Q3GroupBox * java_b;
    Q3GroupBox * php_b;
    Q3GroupBox * python_b;
    Q3GroupBox * idl_b;
    QList<BodyDialog *> edits;
    BrowserNode * view;
    int offset;

    static QSize previous_size;

protected:
    void init_uml_role(RoleDialog & role, const RoleData & rel,
                       Q3GroupBox * bg, BrowserClass * cl1);
    void init_cpp_role(RoleDialog & role, const RoleData & rel,
                       Q3GroupBox * bg, const char * cpp_update_slot,
                       const char * cpp_default_slot,
                       const char * cpp_unmapped_slot,
                       const char * cpp_include_in_header_slot);
    void init_java_role(RoleDialog & role, const RoleData & rel,
                        Q3GroupBox * bg, const char * java_update_slot,
                        const char * java_default_slot,
                        const char * java_unmapped_slot,
                        const char * java_edit_annotation);
    void init_php_role(RoleDialog & role, const RoleData & rel,
                       Q3GroupBox * bg, const char * php_update_slot,
                       const char * php_default_slot, const char * php_unmapped_slot);
    void init_python_role(RoleDialog & role, const RoleData & rel, BrowserClass *,
                          Q3GroupBox * bg, const char * python_update_slot,
                          const char * python_default_slot, const char * python_unmapped_slot);
    void init_idl_role(RoleDialog & role, const RoleData & rel, ClassData * cld,
                       Q3GroupBox * bg, const char * idl_update_slot,
                       const char * idl_default_slot, const char * idl_unmapped_slot);
    void cpp_update(RoleDialog & role, BrowserClass * cl, BrowserNode * rl);
    void java_update(RoleDialog & role, BrowserClass * cl, BrowserNode * rl);
    void php_update(RoleDialog & role, BrowserClass * cl, BrowserNode * rl);
    void python_update(RoleDialog & role, BrowserClass * cl, BrowserNode * rl);
    void idl_update(RoleDialog & role, BrowserClass * cl, BrowserNode * rl);
    void set_inherit_or_dependency(UmlCode type);

    void default_defs_if_needed(UmlCode prev_type);
    void default_a_cpp_if_needed(UmlCode prev_type, QString prev_mult, QString new_mult);
    void default_a_java_if_needed(UmlCode prev_type, QString prev_mult, QString new_mult);
    void default_a_idl_if_needed(UmlCode prev_type, QString prev_mult, QString new_mult);
    void default_a_php_if_needed(UmlCode prev_type, QString prev_mult, QString new_mult);
    void default_a_python_if_needed(UmlCode prev_type, QString prev_mult, QString new_mult);
    void default_b_cpp_if_needed(UmlCode prev_type, QString prev_mult, QString new_mult);
    void default_b_java_if_needed(UmlCode prev_type, QString prev_mult, QString new_mult);
    void default_b_idl_if_needed(UmlCode prev_type, QString prev_mult, QString new_mult);
    void default_b_php_if_needed(UmlCode prev_type, QString prev_mult, QString new_mult);
    void default_b_python_if_needed(UmlCode prev_type, QString prev_mult, QString new_mult);

    static void post_edit_description_a(RelationDialog * d, QString s);
    static void post_edit_description_b(RelationDialog * d, QString s);
    static void post_edit_constraint_a(RelationDialog * d, QString s);
    static void post_edit_constraint_b(RelationDialog * d, QString s);
    static void post_edit_init_a(RelationDialog * d, QString s);
    static void post_edit_init_b(RelationDialog * d, QString s);

public:
    RelationDialog(RelationData * r);
    virtual ~RelationDialog();

protected slots:
    virtual void polish();
    virtual void accept();

    void default_description_a();
    void default_description_b();
    void edit_description_a();
    void edit_description_b();
    void edit_constraint_a();
    void edit_constraint_b();
    void edit_init_a();
    void edit_init_b();
    void edTypeActivated(int);
    void menu_assoc();
    void derived_changed_a(bool);
    void derived_changed_b(bool);
    void cpp_update_a();
    void cpp_update_b();
    void cpp_default_a();
    void cpp_include_in_header();
    void cpp_unmapped_a();
    void cpp_default_b();
    void cpp_unmapped_b();
    void java_update_a();
    void java_update_b();
    void java_default_a();
    void java_unmapped_a();
    void java_default_b();
    void java_unmapped_b();
    void java_edit_annotation_a();
    void java_edit_annotation_b();
    void php_update_a();
    void php_update_b();
    void php_default_a();
    void php_unmapped_a();
    void php_default_b();
    void php_unmapped_b();
    void python_update_a();
    void python_update_b();
    void python_default_a();
    void python_unmapped_a();
    void python_default_b();
    void python_unmapped_b();
    void idl_update_a();
    void idl_update_b();
    void idl_default_a();
    void idl_unmapped_a();
    void idl_default_b();
    void idl_unmapped_b();
    void update_all_tabs(QWidget *);
};

#endif
