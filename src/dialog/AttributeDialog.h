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

#ifndef ATTRIBUTEDIALOG_H
#define  ATTRIBUTEDIALOG_H





#include <qstringlist.h>
#include <q3tabdialog.h>
//Added by qt3to4:
#include <Q3PtrList>

#include "BrowserNode.h"
#include "VisibilityGroup.h"

class Q3ComboBox;
class QCheckBox;
class QRadioButton;

class BrowserAttribute;
class AttributeData;
class BrowserNode;
class LineEdit;
class MultiLineEdit;
class KeyValuesTable;
class BodyDialog;

class AttributeDialog : public Q3TabDialog
{
    Q_OBJECT

private:
    bool visit;
    bool in_enum;
    bool new_in_st;
    AttributeData * att;
    QStringList list;
    BrowserNodeList nodes;
    QList<BodyDialog *> edits;
    BrowserNode * view;
    int offset;

    // uml tab
    QWidget * umltab;
    LineEdit * edname;
    Q3ComboBox * multiplicity;
    QString previous_multiplicity;
    LineEdit * edinit;
    Q3ComboBox * edtype;
    Q3ComboBox * edstereotype;
    int attribute_st_rank;
    int empty_st_rank;
    VisibilityGroup uml_visibility;
    QCheckBox * classattribute_cb;
    QCheckBox * constattribute_cb;
    QCheckBox * derived_cb;
    QCheckBox * derivedunion_cb;
    QCheckBox * ordered_cb;
    QCheckBox * unique_cb;
    MultiLineEdit * comment;
    MultiLineEdit * constraint;

    // C++ tab
    QWidget * cppTab;
    bool cpp_in_enum;
    bool cpp_ignored;
    VisibilityGroup cpp_visibility;
    QCheckBox * mutable_cb;
    QCheckBox * volatile_cb;
    MultiLineEdit * edcppdecl;
    MultiLineEdit * showcppdecl;

    // java tab
    QWidget * javatab;
    bool java_in_enum;
    bool java_in_enum_pattern;
    bool java_ignored;
    QCheckBox * transient_cb;
    MultiLineEdit * edjavadecl;
    MultiLineEdit * showjavadecl;
    QPushButton * editjavaannotation;
    QString javaannotation;

    // php tab
    QWidget * phptab;
    bool php_in_enum;
    bool php_ignored;
    MultiLineEdit * edphpdecl;
    MultiLineEdit * showphpdecl;

    // python tab
    QWidget * pythontab;
    bool python_in_enum;
    bool python_ignored;
    MultiLineEdit * edpythondecl;
    MultiLineEdit * showpythondecl;
    QString python_self;

    // IDL tab
    QWidget * idltab;
    bool idl_in_enum;
    bool idl_in_struct;
    bool idl_in_union;
    bool idl_in_typedef;
    Q3ComboBox * edcase;
    BrowserNodeList enums;
    QStringList enum_names;
    MultiLineEdit * edidldecl;
    MultiLineEdit * showidldecl;

    // User tab
    KeyValuesTable * kvtable;

    static QSize previous_size;

    void default_defs_if_needed();
    void default_cpp_if_needed(QString prev_mult, QString new_mult);
    void default_java_if_needed(QString prev_mult, QString new_mult);
    void default_idl_if_needed(QString prev_mult, QString new_mult);
    void default_php_if_needed(QString prev_mult, QString new_mult);
    void default_python_if_needed(QString prev_mult, QString new_mult);

    static void post_edit_init(AttributeDialog *, QString);
    static void post_edit_description(AttributeDialog *, QString);
    static void post_edit_constraint(AttributeDialog *, QString);

public:
    AttributeDialog(AttributeData * a, bool new_st_attr);
    virtual ~AttributeDialog();

    static QString cpp_decl(const BrowserAttribute * at, bool init = FALSE,
                            ShowContextMode mode = noContext);
    static QString java_decl(const BrowserAttribute * at, bool init = FALSE,
                             ShowContextMode mode = noContext);
    static QString php_decl(const BrowserAttribute * at, bool init = FALSE,
                            ShowContextMode mode = noContext);
    static QString python_decl(const BrowserAttribute * at, bool init = FALSE,
                               ShowContextMode mode = noContext);
    static QString idl_decl(const BrowserAttribute * at,
                            ShowContextMode mode = noContext);

protected slots:
    virtual void polish();
    virtual void accept();

    void edit_init();
    void default_description();
    void edit_description();
    void edit_constraint();
    void menu_type();
    void derived_changed(bool);
    void cpp_default();
    void cpp_unmapped();
    void cpp_update();
    void java_default();
    void java_default_item();
    void java_unmapped();
    void java_update();
    void java_edit_annotation();
    void php_default();
    void php_unmapped();
    void php_update();
    void python_default();
    void python_unmapped();
    void python_update();
    void idl_default();
    void idl_default_state();
    void idl_default_constant();
    void idl_unmapped();
    void idl_update();
    void update_all_tabs(QWidget *);
};

#endif
