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





#include <q3grid.h>
#include <q3vbox.h>
#include <qlabel.h>
#include <q3combobox.h>
#include <q3buttongroup.h>
#include <qcheckbox.h>
#include <qradiobutton.h>
#include <qpushbutton.h>
#include <q3popupmenu.h>
#include <qcursor.h>
//Added by qt3to4:


#include "AttributeDialog.h"
#include "AttributeData.h"
#include "ClassData.h"
#include "BrowserView.h"
#include "BrowserClass.h"
#include "BrowserAttribute.h"
#include "BrowserOperation.h"
#include "KeyValueTable.h"
#include "UmlWindow.h"
#include "UmlDesktop.h"
#include "DialogUtil.h"
#include "GenerationSettings.h"
#include "strutil.h"
#include "BodyDialog.h"
#include "AnnotationDialog.h"
#include "ProfiledStereotypes.h"
#include "translate.h"

QSize AttributeDialog::previous_size;

AttributeDialog::AttributeDialog(AttributeData * a, bool new_st_attr)
    : Q3TabDialog(0, 0, FALSE, Qt::WDestructiveClose),
      new_in_st(new_st_attr), att(a)
{
    a->browser_node->edit_start();

    if (a->browser_node->is_writable()) {
        setOkButton(TR("OK"));
        setCancelButton(TR("Cancel"));
    }
    else {
        setOkButton(QString());
        setCancelButton(TR("Close"));
    }

    visit = !hasOkButton();
    ClassData * cld = (ClassData *)
                      ((BrowserNode *) a->browser_node->parent())->get_data();
    QString stereotype = cld->get_stereotype();
    QString lang_st;

    in_enum = (stereotype == "enum");

    lang_st = GenerationSettings::cpp_class_stereotype(stereotype);
    cpp_in_enum = in_enum || (lang_st == "enum");
    cpp_ignored = !cpp_in_enum && ((lang_st == "typedef") || (lang_st == "ignored"));

    lang_st = GenerationSettings::java_class_stereotype(stereotype);
    java_in_enum = in_enum || (lang_st == "enum");
    java_in_enum_pattern = !java_in_enum && (lang_st == "enum_pattern");
    java_ignored = (lang_st == "ignored");

    lang_st = GenerationSettings::php_class_stereotype(stereotype);
    php_in_enum = in_enum || (lang_st == "enum");
    php_ignored = !php_in_enum && (lang_st == "ignored");

    lang_st = GenerationSettings::python_class_stereotype(stereotype);
    python_in_enum = in_enum || (lang_st == "enum");
    python_ignored = !python_in_enum && (lang_st == "ignored");

    lang_st = GenerationSettings::idl_class_stereotype(stereotype);
    idl_in_enum = in_enum || (lang_st == "enum");
    idl_in_typedef = !idl_in_enum && (lang_st == "typedef");
    idl_in_struct = !idl_in_enum && ((lang_st == "struct") || (lang_st == "exception"));
    idl_in_union = !idl_in_enum && (lang_st == "union");

    setCaption((in_enum || java_in_enum_pattern) ? TR("Enum item dialog") : TR("Attribute dialog"));

    Q3Grid * grid;
    Q3HBox * htab;
    QString s;

    // general tab

    grid = new Q3Grid(2, this);
    umltab = grid;
    grid->setMargin(5);
    grid->setSpacing(5);

    new QLabel(TR("class : "), grid);
    new QLabel(((BrowserNode *) a->get_browser_node()->parent())->full_name(TRUE),
               grid);

    new QLabel(TR("name :"), grid);
    edname = new LineEdit(a->name(), grid);
    edname->setReadOnly(visit);

    QFont font = edname->font();

    if (! hasCodec())
        font.setFamily("Courier");

    font.setFixedPitch(TRUE);

    if (!java_in_enum_pattern) {
        new QLabel(TR("stereotype :"), grid);
        htab = new Q3HBox(grid);
        edstereotype = new Q3ComboBox(!visit, htab);
        edstereotype->insertItem(toUnicode(a->get_stereotype()));

        if (!visit) {
            edstereotype->insertStringList(BrowserAttribute::default_stereotypes());
            edstereotype->insertStringList(ProfiledStereotypes::defaults(UmlAttribute));

            if (java_in_enum) {
                int n = edstereotype->count();

                for (attribute_st_rank = 0; attribute_st_rank != n; attribute_st_rank += 1)
                    if (edstereotype->text(attribute_st_rank) == "attribute")
                        break;

                if (attribute_st_rank == n) {
                    edstereotype->insertItem("attribute");
                    n += 1;
                }

                for (empty_st_rank = 0; empty_st_rank != n; empty_st_rank += 1)
                    if (edstereotype->text(empty_st_rank).isEmpty())
                        break;

                if (empty_st_rank == n)
                    edstereotype->insertItem("");
            }

            edstereotype->setAutoCompletion(completion());
        }

        edstereotype->setCurrentItem(0);

        QSizePolicy sp = edstereotype->sizePolicy();

        sp.setHorData(QSizePolicy::Expanding);
        edstereotype->setSizePolicy(sp);

        new QLabel(TR("    multiplicity :  "), htab);
        multiplicity = new Q3ComboBox(!visit, htab);
        multiplicity->setSizePolicy(sp);
        previous_multiplicity = a->get_multiplicity();
        multiplicity->insertItem(previous_multiplicity);

        if (!visit) {
            multiplicity->insertItem("1");
            multiplicity->insertItem("0..1");
            multiplicity->insertItem("*");
            multiplicity->insertItem("1..*");
        }

        connect(new SmallPushButton(TR("type :"), grid), SIGNAL(clicked()),
                this, SLOT(menu_type()));
        edtype = new Q3ComboBox(!visit, grid);
        edtype->insertItem(a->get_type().get_full_type());
        BrowserClass::instances(nodes);
        nodes.full_names(list);

        if (!visit) {
            QStringList l = GenerationSettings::basic_types();

            cld->addFormals(l);
            edtype->insertStringList(l);
            offset = edtype->count();
            edtype->insertStringList(list);
            edtype->setAutoCompletion(completion());
            view = a->browser_node->container(UmlClass);
        }

        edtype->setCurrentItem(0);
        edtype->setSizePolicy(sp);

        new QLabel(TR("initial value :"), grid);
    }
    else {
        multiplicity = 0;
        new QLabel(TR("value :"), grid);
    }

    htab = new Q3HBox(grid);
    edinit = new LineEdit(a->get_init_value_qstring(), htab);

    if (visit)
        edinit->setReadOnly(TRUE);
    else
        connect(new SmallPushButton(TR("Editor"), htab), SIGNAL(clicked()),
                this, SLOT(edit_init()));

    Q3ButtonGroup * bg;

    if (!java_in_enum_pattern) {
        new QLabel(grid);

        htab = new Q3HBox(grid);
        bg = uml_visibility.init(htab, a->get_uml_visibility(), TRUE);

        if (visit)
            bg->setEnabled(FALSE);

        bg = new Q3ButtonGroup(7, Qt::Horizontal, QString(), htab);
        bg->setExclusive(FALSE);
        classattribute_cb = new QCheckBox("static", bg);

        if (a->get_isa_class_attribute())
            classattribute_cb->setChecked(TRUE);

        classattribute_cb->setDisabled(visit);

        volatile_cb = new QCheckBox("volatile", bg);

        if (a->isa_volatile_attribute)
            volatile_cb->setChecked(TRUE);

        volatile_cb->setDisabled(visit);

        constattribute_cb = new QCheckBox(TR("read-only"), bg);

        if (a->get_isa_const_attribute())
            constattribute_cb->setChecked(TRUE);

        constattribute_cb->setDisabled(visit);

        derived_cb = new QCheckBox(TR("derived"), bg);

        if (a->get_is_derived())
            derived_cb->setChecked(TRUE);

        derived_cb->setDisabled(visit);
        connect(derived_cb, SIGNAL(toggled(bool)), SLOT(derived_changed(bool)));

        derivedunion_cb = new QCheckBox("union", bg);

        if (a->get_is_derivedunion())
            derivedunion_cb->setChecked(TRUE);

        derivedunion_cb->setDisabled(visit || !derived_cb->isChecked());

        ordered_cb = new QCheckBox(TR("ordered"), bg);

        if (a->get_is_ordered())
            ordered_cb->setChecked(TRUE);

        ordered_cb->setDisabled(visit);

        unique_cb = new QCheckBox("unique", bg);

        if (a->get_is_unique())
            unique_cb->setChecked(TRUE);

        unique_cb->setDisabled(visit);
    }

    Q3VBox * vtab = new Q3VBox(grid);

    new QLabel(TR("description :"), vtab);

    if (! visit) {
        connect(new SmallPushButton(TR("Editor"), vtab), SIGNAL(clicked()),
                this, SLOT(edit_description()));
        connect(new SmallPushButton(TR("Default"), vtab), SIGNAL(clicked()),
                this, SLOT(default_description()));
    }

    comment = new MultiLineEdit(grid);
    comment->setReadOnly(visit);
    QString temp =  a->browser_node->get_comment();
    comment->setText(temp);
    comment->setFont(font);

    vtab = new Q3VBox(grid);
    new QLabel(TR("constraint :"), vtab);

    if (! visit) {
        connect(new SmallPushButton(TR("Editor"), vtab), SIGNAL(clicked()),
                this, SLOT(edit_constraint()));
    }

    constraint = new MultiLineEdit(grid);
    constraint->setReadOnly(visit);
    constraint->setText(a->constraint);
    constraint->setFont(font);

    addTab(grid, "Uml");

    // C++

    if (! cpp_ignored) {
        grid = new Q3Grid(2, this);
        cppTab = grid;
        grid->setMargin(5);
        grid->setSpacing(5);

        if (!cpp_in_enum) {
            new QLabel(TR("Visibility :"), grid);
            htab = new Q3HBox(grid);

            Q3ButtonGroup * bg =
                cpp_visibility.init(htab, a->get_cpp_visibility(), FALSE, 0, TR("follow uml"));

            if (visit)
                bg->setEnabled(FALSE);

            new QLabel(" ", htab);

            mutable_cb = new QCheckBox("mutable", htab);

            if (a->cpp_mutable)
                mutable_cb->setChecked(TRUE);

            if (visit)
                mutable_cb->setDisabled(TRUE);
            else
                connect(mutable_cb, SIGNAL(toggled(bool)), this, SLOT(cpp_update()));
        }

        new QLabel(TR("Declaration :"), grid);
        edcppdecl = new MultiLineEdit(grid);
        edcppdecl->setText(a->get_cppdecl());
        edcppdecl->setFont(font);

        if (visit)
            edcppdecl->setReadOnly(TRUE);
        else
            connect(edcppdecl, SIGNAL(textChanged()), this, SLOT(cpp_update()));

        new QLabel(TR("Result after\nsubstitution :"), grid);
        showcppdecl = new MultiLineEdit(grid);
        showcppdecl->setReadOnly(TRUE);
        showcppdecl->setFont(font);

        if (! visit) {
            new QLabel(grid);
            htab = new Q3HBox(grid);
            connect(new QPushButton(TR("Default declaration"), htab), SIGNAL(clicked()),
                    this, SLOT(cpp_default()));
            connect(new QPushButton(TR("Not generated in C++"), htab), SIGNAL(clicked()),
                    this, SLOT(cpp_unmapped()));
        }

        addTab(grid, "C++");

        if (!GenerationSettings::cpp_get_default_defs())
            removePage(grid);
    }
    else
        cppTab = 0;

    // Java

    if (! java_ignored) {
        grid = new Q3Grid(2, this);
        javatab = grid;
        grid->setMargin(5);
        grid->setSpacing(5);

        if (!java_in_enum && !java_in_enum_pattern) {
            new QLabel("", grid);
            transient_cb = new QCheckBox("transient", grid);

            if (a->java_transient)
                transient_cb->setChecked(TRUE);

            if (visit)
                transient_cb->setDisabled(TRUE);
            else
                connect(transient_cb, SIGNAL(toggled(bool)), this, SLOT(java_update()));
        }

        new QLabel(TR("Declaration :"), grid);
        edjavadecl = new MultiLineEdit(grid);
        edjavadecl->setText(a->get_javadecl());
        edjavadecl->setFont(font);

        if (visit)
            edjavadecl->setReadOnly(TRUE);
        else
            connect(edjavadecl, SIGNAL(textChanged()), this, SLOT(java_update()));

        new QLabel(TR("Result after\nsubstitution :"), grid);
        showjavadecl = new MultiLineEdit(grid);
        showjavadecl->setReadOnly(TRUE);
        showjavadecl->setFont(font);

        new QLabel(grid);
        htab = new Q3HBox(grid);

        if (! visit) {
            if (java_in_enum) {
                connect(new QPushButton(TR("Default item declaration"), htab), SIGNAL(clicked()),
                        this, SLOT(java_default_item()));
                connect(new QPushButton(TR("Default attribute declaration"), htab), SIGNAL(clicked()),
                        this, SLOT(java_default()));
            }
            else
                connect(new QPushButton(TR("Default declaration"), htab), SIGNAL(clicked()),
                        this, SLOT(java_default()));

            connect(new QPushButton(TR("Not generated in Java"), htab), SIGNAL(clicked()),
                    this, SLOT(java_unmapped()));

        }

        javaannotation = (const char *) a->java_annotation;
        editjavaannotation =
            new QPushButton((visit) ? TR("Show annotation") : TR("Edit annotation"),
                            htab);
        connect(editjavaannotation, SIGNAL(clicked()),
                this, SLOT(java_edit_annotation()));

        addTab(grid, "Java");

        if (!GenerationSettings::java_get_default_defs())
            removePage(grid);
    }
    else
        javatab = 0;

    // Php

    if (! php_ignored) {
        python_self =
            BrowserOperation::python_init_self((BrowserNode *) a->browser_node->parent())
            + ".";

        grid = new Q3Grid(2, this);
        phptab = grid;
        grid->setMargin(5);
        grid->setSpacing(5);

        new QLabel(TR("Declaration :"), grid);
        edphpdecl = new MultiLineEdit(grid);
        edphpdecl->setText(a->get_phpdecl());
        edphpdecl->setFont(font);

        if (visit)
            edphpdecl->setReadOnly(TRUE);
        else
            connect(edphpdecl, SIGNAL(textChanged()), this, SLOT(php_update()));

        new QLabel(TR("Result after\nsubstitution :"), grid);
        showphpdecl = new MultiLineEdit(grid);
        showphpdecl->setReadOnly(TRUE);
        showphpdecl->setFont(font);

        new QLabel(grid);
        htab = new Q3HBox(grid);

        if (! visit) {
            connect(new QPushButton(TR("Default declaration"), htab), SIGNAL(clicked()),
                    this, SLOT(php_default()));
            connect(new QPushButton(TR("Not generated in Php"), htab), SIGNAL(clicked()),
                    this, SLOT(php_unmapped()));
        }

        addTab(grid, "Php");

        if (!GenerationSettings::php_get_default_defs())
            removePage(grid);
    }
    else
        phptab = 0;

    // Python

    if (! python_ignored) {
        grid = new Q3Grid(2, this);
        pythontab = grid;
        grid->setMargin(5);
        grid->setSpacing(5);

        new QLabel(TR("Declaration :"), grid);
        edpythondecl = new MultiLineEdit(grid);
        edpythondecl->setText(a->get_pythondecl());
        edpythondecl->setFont(font);

        if (visit)
            edpythondecl->setReadOnly(TRUE);
        else
            connect(edpythondecl, SIGNAL(textChanged()), this, SLOT(python_update()));

        new QLabel(TR("Result after\nsubstitution :"), grid);
        showpythondecl = new MultiLineEdit(grid);
        showpythondecl->setReadOnly(TRUE);
        showpythondecl->setFont(font);

        new QLabel(grid);
        htab = new Q3HBox(grid);

        if (! visit) {
            connect(new QPushButton(TR("Default declaration"), htab), SIGNAL(clicked()),
                    this, SLOT(python_default()));
            connect(new QPushButton(TR("Not generated in Python"), htab), SIGNAL(clicked()),
                    this, SLOT(python_unmapped()));
        }

        addTab(grid, "Python");

        if (!GenerationSettings::python_get_default_defs())
            removePage(grid);
    }
    else
        pythontab = 0;

    // IDL

    if (! idl_in_typedef) {
        grid = new Q3Grid(2, this);
        idltab = grid;
        grid->setMargin(5);
        grid->setSpacing(5);

        if (idl_in_union) {
            new QLabel("Case :", grid);
            edcase = new Q3ComboBox(TRUE, grid);
            edcase->insertItem(a->get_idlcase());

            if (! visit) {
                AType switch_type = cld->get_switch_type();

                if (switch_type.type != 0) {
                    switch_type.type->children(enums, UmlAttribute);
                    enums.names(enum_names);
                    edcase->insertStringList(enum_names);
                    edcase->setAutoCompletion(completion());
                }
            }

            edcase->setCurrentItem(0);

            QSizePolicy sp = edcase->sizePolicy();

            sp.setHorData(QSizePolicy::Expanding);
            edcase->setSizePolicy(sp);

            if (! visit)
                connect(edcase, SIGNAL(activated(int)), this, SLOT(idl_update()));
        }

        new QLabel(TR("Declaration :"), grid);
        edidldecl = new MultiLineEdit(grid);
        edidldecl->setText(a->get_idldecl());
        edidldecl->setFont(font);

        if (visit)
            edidldecl->setReadOnly(TRUE);
        else
            connect(edidldecl, SIGNAL(textChanged()), this, SLOT(idl_update()));

        new QLabel(TR("Result after\nsubstitution :"), grid);
        showidldecl = new MultiLineEdit(grid);
        showidldecl->setReadOnly(TRUE);
        showidldecl->setFont(font);

        if (! visit) {
            new QLabel(grid);
            htab = new Q3HBox(grid);
            connect(new QPushButton((idl_in_enum) ? TR("Default declaration")
                                    : TR("Default attribute declaration"),
                                    htab),
                    SIGNAL(clicked()), this, SLOT(idl_default()));

            if (!idl_in_enum && !idl_in_union) {
                if (!idl_in_struct)
                    connect(new QPushButton(TR("Default state declaration"), htab), SIGNAL(clicked()),
                            this, SLOT(idl_default_state()));

                connect(new QPushButton(TR("Default constant declaration"), htab), SIGNAL(clicked()),
                        this, SLOT(idl_default_constant()));
            }

            connect(new QPushButton(TR("Not generated in Idl"), htab), SIGNAL(clicked()),
                    this, SLOT(idl_unmapped()));
        }

        addTab(grid, "Idl");

        if (!GenerationSettings::idl_get_default_defs())
            removePage(grid);
    }
    else
        idltab = 0;

    // USER : list key - value

    grid = new Q3Grid(2, this);
    grid->setMargin(5);
    grid->setSpacing(5);

    kvtable = new KeyValuesTable(a->browser_node, grid, visit);
    addTab(grid, TR("Properties"));

    //

    connect(this, SIGNAL(currentChanged(QWidget *)),
            this, SLOT(update_all_tabs(QWidget *)));

    open_dialog(this);
}

AttributeDialog::~AttributeDialog()
{
    att->browser_node->edit_end();
    previous_size = size();

    foreach (BodyDialog *dialog, edits)
        dialog->close();
    edits.clear();

    if (new_in_st)
        // new_in_st cleared by accept
        ProfiledStereotypes::added((BrowserAttribute *) att->browser_node);

    close_dialog(this);
}

void AttributeDialog::polish()
{
    Q3TabDialog::polish();
    UmlDesktop::limitsize_center(this, previous_size, 0.8, 0.8);
}

void AttributeDialog::default_defs_if_needed()
{
    QString m = multiplicity->currentText().stripWhiteSpace();

    if (m != previous_multiplicity) {
        default_cpp_if_needed(previous_multiplicity, m);
        default_java_if_needed(previous_multiplicity, m);
        default_idl_if_needed(previous_multiplicity, m);
        default_php_if_needed(previous_multiplicity, m);
        default_python_if_needed(previous_multiplicity, m);

        previous_multiplicity = m;
    }
}

void AttributeDialog::menu_type()
{
    Q3PopupMenu m(0);

    m.insertItem(TR("Choose"), -1);
    m.insertSeparator();

    int index = list.findIndex(edtype->currentText().stripWhiteSpace());

    if (index != -1)
        m.insertItem(TR("Select in browser"), 0);

    BrowserNode * bn = 0;

    if (! visit) {
        bn = BrowserView::selected_item();

        if ((bn != 0) && (bn->get_type() == UmlClass) && !bn->deletedp())
            m.insertItem(TR("Choose class selected in browser"), 1);
        else
            bn = 0;

        m.insertItem(TR("Create class and choose it"), 2);
    }

    if (!visit || (index != -1) || (bn != 0)) {
        switch (m.exec(QCursor::pos())) {
        case 0:
            nodes.at(index)->select_in_browser();
            break;

        case 2:
            bn = BrowserClass::add_class(FALSE, view);

            if (bn == 0)
                return;

            bn->select_in_browser();

            // no break
        case 1: {
            QString s = bn->full_name(TRUE);

            if ((index = list.findIndex(s)) == -1) {
                // new class, may be created through an other dialog
                index = 0;
                QStringList::Iterator iter = list.begin();
                QStringList::Iterator iter_end = list.end();

                while ((iter != iter_end) && (*iter < s)) {
                    ++iter;
                    index += 1;
                }

                nodes.insert((unsigned) index, bn);
                list.insert(iter, s);
                edtype->insertItem(s, index + offset);
            }
        }

        edtype->setCurrentItem(index + offset);
        break;

        default:
            break;
        }
    }
}

void AttributeDialog::derived_changed(bool on)
{
    derivedunion_cb->setEnabled(on);
}

void AttributeDialog::accept()
{
    if (!check_edits(edits) || !kvtable->check_unique())
        return;

    BrowserNode * bn = att->browser_node;
    QString oldname = att->name();
    QString s;

    s = edname->text().stripWhiteSpace();

    if ((s != oldname) &&
        ((BrowserNode *) bn->parent())->wrong_child_name(s, UmlAttribute,
                bn->allow_spaces(),
                bn->allow_empty()))
        msg_critical(TR("Error"), s + TR("\n\nillegal name or already used"));
    else {
        default_defs_if_needed();

        bn->set_name(s);

        bool newst = FALSE;

        if (!java_in_enum_pattern) {
            AType t;

            s = edtype->currentText().stripWhiteSpace();

            if (!s.isEmpty()) {
                int index = list.findIndex(s);

                if (index >= 0)
                    t.type = (BrowserClass *) nodes.at(index);
                else
                    t.explicit_type = s;
            }

            att->set_type(t);

            att->uml_visibility = uml_visibility.value();

            att->isa_class_attribute = classattribute_cb->isChecked();
            att->isa_volatile_attribute = volatile_cb->isChecked();
            att->isa_const_attribute = constattribute_cb->isChecked();
            att->is_derived = derived_cb->isChecked();
            att->is_derivedunion = derivedunion_cb->isChecked();
            att->is_ordered = ordered_cb->isChecked();
            att->is_unique = unique_cb->isChecked();

            att->multiplicity = multiplicity->currentText().stripWhiteSpace();

            WrapperStr tempStereo = fromUnicode(edstereotype->currentText().stripWhiteSpace());
            newst = att->set_stereotype(tempStereo.operator const char *());
        }

        att->init_value = edinit->text();

        if (cpp_ignored)
            att->cpp_decl = QString();
        else {
            att->cpp_decl = edcppdecl->text();

            if (!cpp_in_enum)
                att->cpp_visibility = cpp_visibility.value();
        }

        att->cpp_mutable = (cpp_in_enum || cpp_ignored)
                           ? FALSE
                           : mutable_cb->isChecked();

        att->java_transient = (java_in_enum || java_ignored || java_in_enum_pattern)
                              ? FALSE
                              : transient_cb->isChecked();
        att->java_decl = (java_ignored) ? QString() : edjavadecl->text();
        att->java_annotation = javaannotation;

        att->php_decl = (php_ignored) ? QString() : edphpdecl->text();

        att->python_decl = (python_ignored) ? QString() : edpythondecl->text();

        att->idl_decl = (idl_in_typedef) ? QString() : edidldecl->text();

        if (idl_in_union) {
            int index;
            s = edcase->currentText().stripWhiteSpace();

            if (!s.isEmpty() && ((index = enum_names.findIndex(s)) != -1))
                att->set_idlcase((BrowserAttribute *) enums.at(index), "");
            else
                att->set_idlcase(0, s);
        }

        bn->set_comment(comment->text());
        UmlWindow::update_comment_if_needed(bn);

        att->constraint = constraint->stripWhiteSpaceText();

        kvtable->updateNodeFromThis(bn);

        ProfiledStereotypes::modified(bn, newst);

        if (new_in_st) {
            ProfiledStereotypes::added((BrowserAttribute *) bn);
            new_in_st = FALSE;
        }
        else if (!strcmp(((BrowserNode *) bn->parent())->get_data()->get_stereotype(), "stereotype"))
            ProfiledStereotypes::changed((BrowserAttribute *) bn, oldname);

        bn->modified();
        bn->package_modified();
        att->modified();

        QDialog::accept();
    }
}

void AttributeDialog::update_all_tabs(QWidget * w)
{
    if (!visit)
        default_defs_if_needed();

    edname->setText(edname->text().stripWhiteSpace());

    if (w == umltab) {
        if (!visit)
            edname->setFocus();
    }
    else if (w == cppTab) {
        cpp_update();

        if (!visit)
            edcppdecl->setFocus();
    }
    else if (w == javatab) {
        java_update();

        if (!visit)
            edjavadecl->setFocus();
    }
    else if (w == phptab) {
        php_update();

        if (!visit)
            edphpdecl->setFocus();
    }
    else if (w == pythontab) {
        python_update();

        if (!visit)
            edpythondecl->setFocus();
    }
    else if (w == idltab) {
        idl_update();

        if (!visit)
            edidldecl->setFocus();
    }
}

void AttributeDialog::default_description()
{
    comment->setText(GenerationSettings::default_attribute_description());
}

void AttributeDialog::edit_description()
{
    edit(comment->text(), edname->text().stripWhiteSpace() + "_description",
         att, TxtEdit, this, (post_edit) post_edit_description, edits);
}

void AttributeDialog::post_edit_description(AttributeDialog * d, QString s)
{
    d->comment->setText(s);
}

void AttributeDialog::edit_constraint()
{
    edit(constraint->text(), edname->text().stripWhiteSpace() + "_constraint",
         att, TxtEdit, this, (post_edit) post_edit_constraint, edits);
}

void AttributeDialog::post_edit_constraint(AttributeDialog * d, QString s)
{
    d->constraint->setText(s);
}

void AttributeDialog::edit_init()
{
    edit(edinit->text(), edname->text().stripWhiteSpace() + "_initialization",
         att, TxtEdit, this, (post_edit) post_edit_init, edits);
}

void AttributeDialog::post_edit_init(AttributeDialog * d, QString s)
{
    d->edinit->setText(s);
}

void AttributeDialog::cpp_default()
{
    edcppdecl->setText((cpp_in_enum)
                       ? GenerationSettings::cpp_default_enum_item_decl()
                       : GenerationSettings::cpp_default_attr_decl(multiplicity->currentText().stripWhiteSpace()));
    cpp_update();
}

void AttributeDialog::default_cpp_if_needed(QString prev_mult, QString new_mult)
{
    if (!cpp_in_enum && !edcppdecl->text().isEmpty()) {
        QString n = GenerationSettings::cpp_default_attr_decl(new_mult);

        if (n != GenerationSettings::cpp_default_attr_decl(prev_mult))
            edcppdecl->setText(n);
    }
}

void AttributeDialog::cpp_unmapped()
{
    edcppdecl->setText(QString());
    showcppdecl->setText(QString());
}

void AttributeDialog::cpp_update()
{
    if (!cpp_in_enum)
        cpp_visibility.update_default(uml_visibility);

    // do NOT write
    //	const char * p = edcppdecl->text();
    // because the QString is immediatly destroyed !
    QString def = edcppdecl->text();
    const char * p = def;
    const char * pp = 0;
    QString indent = "";
    QString s;

    while ((*p == ' ') || (*p == '\t'))
        indent += *p++;

    if (*p != '#')
        s = indent;

    for (;;) {
        if (*p == 0) {
            if (pp == 0)
                break;

            // comment management done
            p = pp;
            pp = 0;

            if (*p == 0)
                break;

            if (*p != '#')
                s += indent;
        }

        if (!strncmp(p, "${comment}", 10))
            manage_comment(comment->text(), p, pp,
                           GenerationSettings::cpp_javadoc_style());
        else if (!strncmp(p, "${description}", 14))
            manage_description(comment->text(), p, pp);
        else if (!strncmp(p, "${name}", 7)) {
            p += 7;
            s += edname->text();
        }
        else if ((cpp_in_enum && !strncmp(p, "${value}", 8)) ||
                 !strncmp(p, "${h_value}", 10)) {
            QString i = edinit->text().stripWhiteSpace();

            if (!i.isEmpty() && (cpp_in_enum || classattribute_cb->isChecked())) {
                if (need_equal(p, i, TRUE))
                    s += " = ";

                s += edinit->text();
            }

            p += (p[2] == 'h') ? 10 : 8;
        }
        else if (!strncmp(p, "${value}", 8)) {
            QString i = edinit->text().stripWhiteSpace();

            if (!i.isEmpty()) {
                if (need_equal(p, i, TRUE))
                    s += " = ";

                s += edinit->text();
                p += (p[2] == 'h') ? 10 : 8;
            }
            else
                p += 8;
        }
        //p += 8;
        else if (*p == '\n') {
            s += *p++;

            if (*p && (*p != '#'))
                s += indent;
        }
        else if (cpp_in_enum)
            s += *p++;
        else if (!strncmp(p, "${static}", 9)) {
            p += 9;

            if (classattribute_cb->isChecked())
                s += "static ";
        }
        else if (!strncmp(p, "${const}", 8)) {
            p += 8;

            if (constattribute_cb->isChecked())
                s += "const ";
        }
        else if (!strncmp(p, "${mutable}", 10)) {
            p += 10;

            if (mutable_cb->isChecked())
                s += "mutable ";
        }
        else if (!strncmp(p, "${volatile}", 11)) {
            p += 11;

            if (volatile_cb->isChecked())
                s += "volatile ";
        }
        else if (!strncmp(p, "${type}", 7)) {
            p += 7;
            s += get_cpp_name(the_type(edtype->currentText().stripWhiteSpace(),
                                       list, nodes));
        }
        else if (!strncmp(p, "${multiplicity}", 15)) {
            p += 15;

            QString m = multiplicity->currentText().stripWhiteSpace();

            s += (*m == '[') ? m : QString("[") + m + "]";
        }
        else if (!strncmp(p, "${stereotype}", 13)) {
            p += 13;
            s += GenerationSettings::cpp_relationattribute_stereotype(fromUnicode(edstereotype->currentText().stripWhiteSpace()));
        }
        else if (*p == '@')
            manage_alias(att->browser_node, p, s, kvtable);
        else
            s += *p++;
    }

    showcppdecl->setText(s);
}

QString AttributeDialog::cpp_decl(const BrowserAttribute * at, bool init,
                                  ShowContextMode mode)
{
    QString s;
    AttributeData * d = (AttributeData *) at->get_data();
    WrapperStr decl = d->cpp_decl;

    remove_comments(decl);
    remove_preprocessor(decl);

    const char * p = decl;

    while ((*p == ' ') || (*p == '\t'))
        p += 1;

    while (*p) {
        if (!strncmp(p, "${comment}", 10))
            p += 10;
        else if (!strncmp(p, "${description}", 14))
            p += 14;
        else if (!strncmp(p, "${name}", 7)) {
            p += 7;
            s += at->get_name();
        }
        else if (!strncmp(p, "${value}", 8) || !strncmp(p, "${h_value}", 10)) {
            if (init) {
                const char * v = d->get_init_value();

                if (*v) {
                    if (need_equal(p, v, TRUE))
                        s += " = ";

                    s += v;
                }
            }

            break;
        }
        else if (!strncmp(p, "${static}", 9))
            p += 9;
        else if (!strncmp(p, "${const}", 8))
            p += 8;
        else if (!strncmp(p, "${mutable}", 10))
            p += 10;
        else if (!strncmp(p, "${volatile}", 11))
            p += 11;
        else if (!strncmp(p, "${type}", 7)) {
            p += 7;
            s += get_cpp_name(d->type, mode);
        }
        else if (!strncmp(p, "${multiplicity}", 15)) {
            p += 15;

            QString m = d->get_multiplicity();

            if (!m.isEmpty())
                s += (*m == '[') ? m : QString("[") + m + "]";
        }
        else if (!strncmp(p, "${stereotype}", 13)) {
            p += 13;
            s += GenerationSettings::cpp_relationattribute_stereotype(d->stereotype);
        }
        else if (*p == '\n') {
            s += " ";

            do
                p += 1;

            while ((*p == ' ') || (*p == '\t'));
        }
        else if (*p == ';')
            break;
        else if (*p == '@')
            manage_alias(at, p, s, static_cast<KeyValuesTable*>(0));
        else
            s += *p++;
    }

    return s;
}

void AttributeDialog::java_default_item()
{
    edjavadecl->setText(GenerationSettings::java_default_enum_item_decl());

    if (edstereotype->currentText().stripWhiteSpace() == "attribute")
        edstereotype->setCurrentItem(empty_st_rank);

    java_update();
}

void AttributeDialog::java_default()
{
    if (java_in_enum_pattern)
        edjavadecl->setText(GenerationSettings::java_default_enum_pattern_item_decl());
    else {
        if (java_in_enum)
            edstereotype->setCurrentItem(attribute_st_rank);

        edjavadecl->setText(GenerationSettings::java_default_attr_decl(multiplicity->currentText().stripWhiteSpace()));
    }

    java_update();
}

void AttributeDialog::default_java_if_needed(QString prev_mult, QString new_mult)
{
    if (!java_in_enum_pattern && !edjavadecl->text().isEmpty()) {
        QString n = GenerationSettings::java_default_attr_decl(new_mult);

        if (n != GenerationSettings::java_default_attr_decl(prev_mult))
            edjavadecl->setText(n);
    }
}

void AttributeDialog::java_unmapped()
{
    edjavadecl->setText(QString());
    showjavadecl->setText(QString());
}

void AttributeDialog::java_update()
{
    // do NOT write
    //	const char * p = edjavadecl->text();
    // because the QString is immediatly destroyed !
    QString def = edjavadecl->text();
    const char * p = def;
    const char * pp = 0;
    QString indent = "";
    QString s;

    while ((*p == ' ') || (*p == '\t'))
        indent += *p++;

    s = indent;

    for (;;) {
        if (*p == 0) {
            if (pp == 0)
                break;

            // comment management done
            p = pp;
            pp = 0;

            if (*p == 0)
                break;

            s += indent;
        }

        if (!strncmp(p, "${comment}", 10))
            manage_comment(comment->text(), p, pp,
                           GenerationSettings::java_javadoc_style());
        else if (!strncmp(p, "${description}", 14))
            manage_description(comment->text(), p, pp);
        else if (!strncmp(p, "${name}", 7)) {
            p += 7;
            s += edname->text();
        }
        else if (!strncmp(p, "${value}", 8)) {
            QString i = edinit->text().stripWhiteSpace();

            if (!i.isEmpty()) {
                if (java_in_enum &&
                    (edstereotype->currentText().stripWhiteSpace() != "attribute"))
                    s += '(' + edinit->text() + ')';
                else {
                    if (need_equal(p, i, FALSE))
                        s += " = ";

                    s += edinit->text();
                }
            }
            else if (java_in_enum_pattern)
                s += (need_equal(p, "", FALSE)) ? " = ..." : "...";

            p += 8;
        }
        else if (*p == '\n') {
            s += *p++;

            if (*p)
                s += indent;
        }
        else if (!strncmp(p, "${@}", 4)) {
            p += 4;

            if (pp != 0)
                s += "${@}";
            else if (!javaannotation.isEmpty()) {
                pp = p;
                p = javaannotation;
            }
        }
        else if (*p == '@')
            manage_alias(att->browser_node, p, s, kvtable);
        else if (java_in_enum_pattern) {
            if (!strncmp(p, "${class}", 8)) {
                p += 8;
                s += ((BrowserNode *) att->browser_node->parent())->get_name();
            }
            else
                s += *p++;
        }
        else if (!strncmp(p, "${visibility}", 13)) {
            p += 13;

            if (uml_visibility.value() != UmlPackageVisibility)
                s += uml_visibility.state() + " ";
        }
        else if (!strncmp(p, "${static}", 9)) {
            p += 9;

            if (classattribute_cb->isChecked())
                s += "static ";
        }
        else if (!strncmp(p, "${transient}", 12)) {
            p += 12;

            if (!java_in_enum && !java_in_enum_pattern && !java_ignored && transient_cb->isChecked())
                s += "transient ";
        }
        else if (!strncmp(p, "${volatile}", 11)) {
            p += 11;

            if (volatile_cb->isChecked())
                s += "volatile ";
        }
        else if (!strncmp(p, "${final}", 8)) {
            p += 8;

            if (constattribute_cb->isChecked())
                s += "final ";
        }
        else if (!strncmp(p, "${type}", 7)) {
            p += 7;
            s += get_java_name(the_type(edtype->currentText().stripWhiteSpace(),
                                        list, nodes));
        }
        else if (!strncmp(p, "${multiplicity}", 15)) {
            p += 15;
            s += java_multiplicity(multiplicity->currentText().stripWhiteSpace());
        }
        else if (!strncmp(p, "${stereotype}", 13)) {
            p += 13;
            s += GenerationSettings::java_relationattribute_stereotype(fromUnicode(edstereotype->currentText().stripWhiteSpace()));
        }
        else
            s += *p++;
    }

    editjavaannotation->setEnabled(def.find("${@}") != -1);

    showjavadecl->setText(s);
}

QString AttributeDialog::java_decl(const BrowserAttribute * at, bool init,
                                   ShowContextMode mode)
{
    QString s;
    AttributeData * d = (AttributeData *) at->get_data();
    WrapperStr decl = d->java_decl;

    remove_comments(decl);

    const char * p = decl;

    while ((*p == ' ') || (*p == '\t'))
        p += 1;

    while (*p) {
        if (!strncmp(p, "${comment}", 10))
            p += 10;
        else if (!strncmp(p, "${description}", 14))
            p += 14;
        else if (!strncmp(p, "${name}", 7)) {
            p += 7;
            s += at->get_name();
        }
        else if (!strncmp(p, "${value}", 8)) {
            if (init) {
                const char * v = d->get_init_value();

                if (*v) {
                    if (need_equal(p, v, FALSE))
                        s += " = ";

                    s += v;
                }
            }

            break;
        }
        else if (!strncmp(p, "${class}", 8)) {
            p += 8;
            s += ((BrowserNode *) at->parent())->get_name();
        }
        else if (!strncmp(p, "${visibility}", 13))
            p += 13;
        else if (!strncmp(p, "${static}", 9))
            p += 9;
        else if (!strncmp(p, "${transient}", 12))
            p += 12;
        else if (!strncmp(p, "${volatile}", 11))
            p += 11;
        else if (!strncmp(p, "${final}", 8))
            p += 8;
        else if (!strncmp(p, "${type}", 7)) {
            p += 7;
            s += get_java_name(d->type, mode);
        }
        else if (!strncmp(p, "${multiplicity}", 15)) {
            p += 15;

            QString m = d->get_multiplicity();

            if (*m != '[')
                s += "[]";
            else {
                for (int index = 0; index != m.length(); index += 1) {
                    switch (m.at(index).latin1()) {
                    case '[':
                        s += '[';
                        break;

                    case ']':
                        s += ']';

                    default:
                        break;
                    }
                }
            }
        }
        else if (!strncmp(p, "${stereotype}", 13)) {
            p += 13;
            s += GenerationSettings::java_relationattribute_stereotype(d->get_multiplicity());
        }
        else if (!strncmp(p, "${@}", 4))
            p += 4;
        else if (*p == '\n') {
            s += " ";

            do
                p += 1;

            while ((*p == ' ') || (*p == '\t'));
        }
        else if (*p == ';')
            break;
        else if (*p == '@')
            manage_alias(at, p, s, static_cast<KeyValuesTable*>(0));
        else
            s += *p++;
    }

    return s;
}

void AttributeDialog::java_edit_annotation()
{
    AnnotationDialog dialog(this, javaannotation, !hasOkButton());

    if (dialog.exec() == QDialog::Accepted)
        java_update();
}

void AttributeDialog::php_default()
{
    if (php_in_enum)
        edphpdecl->setText(GenerationSettings::php_default_enum_item_decl());
    else
        edphpdecl->setText(GenerationSettings::php_default_attr_decl());

    php_update();
}

void AttributeDialog::default_php_if_needed(QString, QString)
{
    // do nothing : no link with multiplicity
}

void AttributeDialog::php_unmapped()
{
    edphpdecl->setText(QString());
    showphpdecl->setText(QString());
}

void AttributeDialog::php_update()
{
    // do NOT write
    //	const char * p = edphpdecl->text();
    // because the QString is immediatly destroyed !
    QString def = edphpdecl->text();
    const char * p = def;
    const char * pp = 0;
    QString indent = "";
    QString s;

    while ((*p == ' ') || (*p == '\t'))
        indent += *p++;

    s = indent;

    for (;;) {
        if (*p == 0) {
            if (pp == 0)
                break;

            // comment management done
            p = pp;
            pp = 0;

            if (*p == 0)
                break;

            s += indent;
        }

        if (!strncmp(p, "${comment}", 10))
            manage_comment(comment->text(), p, pp,
                           GenerationSettings::php_javadoc_style());
        else if (!strncmp(p, "${description}", 14))
            manage_description(comment->text(), p, pp);
        else if (!strncmp(p, "${name}", 7)) {
            p += 7;

            if (!php_in_enum && !constattribute_cb->isChecked())
                s += "$";

            s += edname->text();
        }
        else if (!strncmp(p, "${value}", 8)) {
            QString i = edinit->text().stripWhiteSpace();

            if (!i.isEmpty()) {
                if (need_equal(p, i, FALSE))
                    s += " = ";

                s += edinit->text();
            }
            else if (php_in_enum)
                s += (need_equal(p, "", FALSE)) ? " = ..." : "...";

            p += 8;
        }
        else if (!strncmp(p, "${const}", 8)) {
            p += 8;

            if (constattribute_cb->isChecked())
                s += "const ";
        }
        else if (!strncmp(p, "${var}", 6)) {
            p += 6;

            if (!php_in_enum &&
                !constattribute_cb->isChecked() &&
                !classattribute_cb->isChecked() &&
                (uml_visibility.value() == UmlPackageVisibility))
                s += "var ";
        }
        else if (*p == '\n') {
            s += *p++;

            if (*p)
                s += indent;
        }
        else if (*p == '@')
            manage_alias(att->browser_node, p, s, kvtable);
        else if (!strncmp(p, "${visibility}", 13)) {
            p += 13;

            if (uml_visibility.value() != UmlPackageVisibility)
                s += uml_visibility.state() + " ";
        }
        else if (!strncmp(p, "${static}", 9)) {
            p += 9;

            if (classattribute_cb->isChecked())
                s += "static ";
        }
        else if (!strncmp(p, "${type}", 7)) {
            // for comment
            p += 7;
            s += get_php_name(the_type(edtype->currentText().stripWhiteSpace(),
                                       list, nodes));
        }
        else
            s += *p++;
    }

    showphpdecl->setText(s);
}

QString AttributeDialog::php_decl(const BrowserAttribute * at, bool init,
                                  ShowContextMode mode)
{
    QString s;
    AttributeData * d = (AttributeData *) at->get_data();
    WrapperStr decl = d->php_decl;

    remove_comments(decl);

    const char * p = decl;

    while ((*p == ' ') || (*p == '\t'))
        p += 1;

    while (*p) {
        if (!strncmp(p, "${comment}", 10))
            p += 10;
        else if (!strncmp(p, "${description}", 14))
            p += 14;
        else if (!strncmp(p, "${name}", 7)) {
            p += 7;

            ClassData * cld = (ClassData *)
                              ((BrowserNode *) at->parent())->get_data();
            QString stereotype = cld->get_stereotype();
            bool in_enum = (stereotype == "enum");
            QString lang_st = GenerationSettings::php_class_stereotype(stereotype);

            in_enum |= (lang_st == "enum");

            if (!in_enum &&
                !((AttributeData *) at->get_data())->get_isa_const_attribute())
                s += "$";

            s += at->get_name();
        }
        else if (!strncmp(p, "${value}", 8)) {
            if (init) {
                const char * v = d->get_init_value();

                if (*v) {
                    if (need_equal(p, v, FALSE))
                        s += " = ";

                    s += v;
                }
            }

            break;
        }
        else if (!strncmp(p, "${const}", 8))
            p += 8;
        else if (!strncmp(p, "${visibility}", 13))
            p += 13;
        else if (!strncmp(p, "${static}", 9))
            p += 9;
        else if (!strncmp(p, "${var}", 6))
            p += 6;
        else if (*p == '\n') {
            s += " ";

            do
                p += 1;

            while ((*p == ' ') || (*p == '\t'));
        }
        else if (*p == ';')
            break;
        else if (*p == '@')
            manage_alias(at, p, s, static_cast<KeyValuesTable*>(0));
        else if (!strncmp(p, "${type}", 7)) {
            // for comment
            p += 7;
            s += get_php_name(d->type, mode);
        }
        else
            s += *p++;
    }

    return s;
}

void AttributeDialog::python_default()
{
    if (python_in_enum)
        edpythondecl->setText(GenerationSettings::python_default_enum_item_decl());
    else if (!java_in_enum_pattern)
        edpythondecl->setText(GenerationSettings::python_default_attr_decl(""));
    else
        edpythondecl->setText(GenerationSettings::python_default_attr_decl(multiplicity->currentText().stripWhiteSpace()));

    python_update();
}

void AttributeDialog::default_python_if_needed(QString prev_mult, QString new_mult)
{
    if (!python_in_enum && !java_in_enum_pattern && !edpythondecl->text().isEmpty()) {
        QString n = GenerationSettings::python_default_attr_decl(new_mult);

        if (n != GenerationSettings::python_default_attr_decl(prev_mult))
            edpythondecl->setText(n);
    }
}

void AttributeDialog::python_unmapped()
{
    edpythondecl->setText(QString());
    showpythondecl->setText(QString());
}

void AttributeDialog::python_update()
{
    // do NOT write
    //	const char * p = edpythondecl->text();
    // because the QString is immediatly destroyed !
    QString def = edpythondecl->text();
    const char * p = def;
    const char * pp = 0;
    QString s;

    for (;;) {
        if (*p == 0) {
            if (pp == 0)
                break;

            // comment management done
            p = pp;
            pp = 0;

            if (*p == 0)
                break;
        }

        if (!strncmp(p, "${comment}", 10))
            manage_python_comment(comment->text(), p, pp);
        else if (!strncmp(p, "${description}", 14))
            manage_python_description(comment->text(), p, pp);
        else if (!strncmp(p, "${name}", 7)) {
            p += 7;
            s += edname->text();
        }
        else if (!strncmp(p, "${value}", 8)) {
            QString i = edinit->text().stripWhiteSpace();

            if (!i.isEmpty()) {
                if (need_equal(p, i, FALSE))
                    s += " = ";

                s += edinit->text();
            }
            else if (python_in_enum)
                s += (need_equal(p, "", FALSE)) ? " = ..." : "...";
            else if (need_equal(p, "None", FALSE))
                s += " = None";
            else
                s += "None";

            p += 8;
        }
        else if (!strncmp(p, "${self}", 7)) {
            p += 7;

            if (java_in_enum_pattern || !classattribute_cb->isChecked())
                s += python_self;
        }
        else if (!strncmp(p, "${stereotype}", 13)) {
            p += 13;

            if (! java_in_enum_pattern)
                s += GenerationSettings::python_relationattribute_stereotype(fromUnicode(edstereotype->currentText().stripWhiteSpace()));
        }
        else if (!strncmp(p, "${type}", 7)) {
            p += 7;
            s += get_python_name(the_type(edtype->currentText().stripWhiteSpace(),
                                          list, nodes));
        }
        else if (*p == '@')
            manage_alias(att->browser_node, p, s, kvtable);
        else
            s += *p++;
    }

    showpythondecl->setText(s);
}

// produced out of __init__
QString AttributeDialog::python_decl(const BrowserAttribute * at,
                                     bool init, ShowContextMode)
{
    QString s;
    AttributeData * d = (AttributeData *) at->get_data();
    WrapperStr decl = d->python_decl;

    remove_comments(decl);

    const char * p = decl;

    while (*p) {
        if (!strncmp(p, "${comment}", 10)) {
            p += 10;
        }
        else if (!strncmp(p, "${description}", 14)) {
            p += 14;
        }
        else if (!strncmp(p, "${name}", 7)) {
            p += 7;
            s += at->get_name();
        }
        else if (!strncmp(p, "${value}", 8)) {
            if (init) {
                const char * v = d->get_init_value();

                if (*v) {
                    if (need_equal(p, v, FALSE))
                        s += " = ";

                    s += v;
                }
            }

            break;
        }
        else if (!strncmp(p, "${self}", 7)) {
            p += 7;
        }
        else if (!strncmp(p, "${stereotype}", 13)) {
            p += 13;
        }
        else if (!strncmp(p, "${type}", 7)) {
            p += 7;
        }
        else if (*p == '=') {
            if (!init)
                break;
            else
                s += *p++;
        }
        else if (*p == '\n') {
            s += " ";

            do
                p += 1;

            while ((*p == ' ') || (*p == '\t'));
        }
        else if (*p == '@')
            manage_alias(at, p, s, static_cast<KeyValuesTable*>(0));
        else
            s += *p++;
    }

    return s;
}

void AttributeDialog::idl_default()
{
    if (idl_in_enum)
        edidldecl->setText(GenerationSettings::idl_default_enum_item_decl());
    else if (idl_in_union)
        edidldecl->setText(GenerationSettings::idl_default_union_item_decl(multiplicity->currentText().stripWhiteSpace()));
    else
        edidldecl->setText((constattribute_cb->isChecked() &&
                            !edinit->text().stripWhiteSpace().isEmpty())
                           ? GenerationSettings::idl_default_const_decl(multiplicity->currentText().stripWhiteSpace())
                           : GenerationSettings::idl_default_attr_decl(multiplicity->currentText().stripWhiteSpace()));

    idl_update();
}

void AttributeDialog::default_idl_if_needed(QString prev_mult, QString new_mult)
{
    if (!idl_in_enum && !idl_in_union && !edidldecl->text().isEmpty()) {
        QString p;
        QString n;

        if (constattribute_cb->isChecked() && !edinit->text().stripWhiteSpace().isEmpty()) {
            p = GenerationSettings::idl_default_const_decl(prev_mult);
            n = GenerationSettings::idl_default_const_decl(new_mult);
        }
        else {
            p = GenerationSettings::idl_default_attr_decl(prev_mult);
            n = GenerationSettings::idl_default_attr_decl(new_mult);
        }

        if (n != p)
            edidldecl->setText(n);
    }
}

void AttributeDialog::idl_default_state()
{
    edidldecl->setText(GenerationSettings::idl_default_valuetype_attr_decl(multiplicity->currentText().stripWhiteSpace()));
    idl_update();
}

void AttributeDialog::idl_default_constant()
{
    edidldecl->setText(GenerationSettings::idl_default_const_decl(multiplicity->currentText().stripWhiteSpace()));
    idl_update();
}

void AttributeDialog::idl_unmapped()
{
    edidldecl->setText(QString());
    showidldecl->setText(QString());
}

void AttributeDialog::idl_update()
{
    // do NOT write
    //	const char * p = edidldecl->text();
    // because the QString is immediatly destroyed !
    QString def = edidldecl->text();
    const char * p = def;
    const char * pp = 0;
    QString indent = "";
    QString s;

    while ((*p == ' ') || (*p == '\t'))
        indent += *p++;

    if (*p != '#')
        s = indent;

    for (;;) {
        if (*p == 0) {
            if (pp == 0)
                break;

            // comment management done
            p = pp;
            pp = 0;

            if (*p == 0)
                break;

            s += indent;
        }

        if (!strncmp(p, "${comment}", 10))
            manage_comment(comment->text(), p, pp, FALSE);
        else if (!strncmp(p, "${description}", 14))
            manage_description(comment->text(), p, pp);
        else if (!strncmp(p, "${name}", 7)) {
            p += 7;
            s += edname->text();
        }
        else if (!strncmp(p, "${value}", 8)) {
            QString i = edinit->text().stripWhiteSpace();

            if (need_equal(p, i, FALSE))
                s += " = ";

            s += edinit->text();
            p += 8;
        }
        else if (*p == '\n') {
            s += *p++;

            if (*p && (*p != '#'))
                s += indent;
        }
        else if (idl_in_enum)
            s += *p++;
        else if (idl_in_union && !strncmp(p, "${case}", 7)) {
            p += 7;
            s += edcase->currentText().stripWhiteSpace();
        }
        else if (!strncmp(p, "${attribut}", 11)) {
            // old version
            p += 11;

            if (!idl_in_struct)
                s += "attribute ";
        }
        else if (!strncmp(p, "${attribute}", 12)) {
            p += 12;

            if (!idl_in_struct)
                s += "attribute ";
        }
        else if (!strncmp(p, "${visibility}", 13)) {
            p += 13;

            switch (uml_visibility.value()) {
            case UmlPublic:
            case UmlPackageVisibility:
                s += "public ";

            default:
                s += "private ";
            }
        }
        else if (!strncmp(p, "${readonly}", 11)) {
            p += 11;

            if (constattribute_cb->isChecked())
                s += "readonly ";
        }
        else if (!strncmp(p, "${type}", 7)) {
            p += 7;
            s += get_idl_name(the_type(edtype->currentText().stripWhiteSpace(),
                                       list, nodes));
        }
        else if (!strncmp(p, "${stereotype}", 13)) {
            p += 13;
            s += GenerationSettings::idl_relationattribute_stereotype(fromUnicode(edstereotype->currentText().stripWhiteSpace()));
        }
        else if (!strncmp(p, "${multiplicity}", 15)) {
            p += 15;
            s += multiplicity->currentText().stripWhiteSpace();
        }
        else if (*p == '@')
            manage_alias(att->browser_node, p, s, kvtable);
        else
            s += *p++;
    }

    showidldecl->setText(s);
}

QString AttributeDialog::idl_decl(const BrowserAttribute * at,
                                  ShowContextMode mode)
{
    QString s;
    AttributeData * d = (AttributeData *) at->get_data();
    WrapperStr decl = d->idl_decl;
    QString stereotype = ((BrowserNode *) at->parent())->get_data()->get_stereotype();
    bool in_enum = (stereotype == "enum") ||
                   (GenerationSettings::idl_class_stereotype(stereotype) == "enum");

    remove_comments(decl);

    const char * p = decl;

    while ((*p == ' ') || (*p == '\t'))
        p += 1;

    while (*p) {
        if (!strncmp(p, "${comment}", 10))
            p += 10;
        else if (!strncmp(p, "${description}", 14))
            p += 14;
        else if (!strncmp(p, "${name}", 7)) {
            p += 7;
            s += at->get_name();
        }
        else if (!strncmp(p, "${value}", 8))
            break;
        else if (!strncmp(p, "${case}", 7)) {
            p += 7;
            s += d->get_idlcase();
        }
        else if (!strncmp(p, "${attribut}", 11))
            // old version
            p += 11;
        else if (!strncmp(p, "${attribute}", 12))
            p += 12;
        else if (!strncmp(p, "${visibility}", 13))
            p += 13;
        else if (!strncmp(p, "${readonly}", 11))
            p += 11;
        else if (!strncmp(p, "${type}", 7)) {
            p += 7;
            s += get_idl_name(d->type, mode);
        }
        else if (!strncmp(p, "${stereotype}", 13)) {
            p += 13;
            s += GenerationSettings::idl_relationattribute_stereotype(d->get_stereotype());
        }
        else if (!strncmp(p, "${multiplicity}", 15)) {
            p += 15;
            s += d->get_multiplicity();
        }
        else if (*p == '\n') {
            s += " ";

            do
                p += 1;

            while ((*p == ' ') || (*p == '\t'));
        }
        else if ((*p == ';') || (in_enum && (*p == ',')))
            break;
        else if (*p == '@')
            manage_alias(at, p, s, static_cast<KeyValuesTable*>(0));
        else
            s += *p++;
    }

    return s;
}
