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

#include "ExtraMemberDialog.h"
#include "ExtraMemberData.h"
#include "BrowserExtraMember.h"
#include "KeyValueTable.h"
#include "UmlWindow.h"
#include "DialogUtil.h"
#include "UmlDesktop.h"
#include "BodyDialog.h"
#include "GenerationSettings.h"
#include "strutil.h"
#include "ProfiledStereotypes.h"
#include "translate.h"

QSize ExtraMemberDialog::previous_size;

ExtraMemberDialog::ExtraMemberDialog(ExtraMemberData * ex)
    : Q3TabDialog(0, 0, FALSE, Qt::WDestructiveClose), emd(ex)
{
    ex->browser_node->edit_start();

    if (ex->browser_node->is_writable()) {
        setOkButton(TR("OK"));
        setCancelButton(TR("Cancel"));
    }
    else {
        setOkButton(QString());
        setCancelButton(TR("Close"));
    }

    bool visit = !hasOkButton();

    setCaption(TR("Extra Class Member dialog"));

    Q3Grid * grid;

    // general tab

    grid = new Q3Grid(2, this);
    grid->setMargin(5);
    grid->setSpacing(5);

    new QLabel(TR("name :"), grid);
    edname = new LineEdit(ex->name(), grid);
    edname->setReadOnly(visit);

    QFont font = edname->font();

    if (! hasCodec())
        font.setFamily("Courier");

    font.setFixedPitch(TRUE);

    new QLabel(TR("stereotype :"), grid);
    edstereotype = new Q3ComboBox(!visit, grid);
    edstereotype->insertItem(toUnicode(ex->get_stereotype()));
    edstereotype->setCurrentItem(0);

    if (! visit) {
        edstereotype->insertStringList(ProfiledStereotypes::defaults(UmlExtraMember));
        edstereotype->setAutoCompletion(completion());
    }

    QSizePolicy sp = edstereotype->sizePolicy();

    edstereotype->setSizePolicy(sp);

    Q3VBox * vtab = new Q3VBox(grid);
    new QLabel(TR("description :"), vtab);

    if (! visit)
        connect(new SmallPushButton(TR("Editor"), vtab), SIGNAL(clicked()),
                this, SLOT(edit_description()));

    comment = new MultiLineEdit(grid);
    comment->setReadOnly(visit);
    comment->setText(ex->get_browser_node()->get_comment());
    comment->setFont(font);

    addTab(grid, "Uml");

    // C++

    grid = new Q3Grid(2, this);
    grid->setMargin(5);
    grid->setSpacing(5);

    new QLabel(grid);
    Q3ButtonGroup * gp =
        new Q3ButtonGroup(1, Qt::Horizontal, QString(), grid);
    inline_cb = new QCheckBox("inline", gp);
    inline_cb->setDisabled(visit);

    if (ex->cpp_inline)
        inline_cb->setChecked(TRUE);

    vtab = new Q3VBox(grid);
    new QLabel(TR("C++ \ndeclaration :"), vtab);

    if (! visit)
        connect(new SmallPushButton(TR("Editor"), vtab), SIGNAL(clicked()),
                this, SLOT(edit_cpp_decl()));

    edcpp_decl = new MultiLineEdit(grid);
    edcpp_decl->setReadOnly(visit);
    edcpp_decl->setText(ex->cpp_decl);
    edcpp_decl->setFont(font);

    vtab = new Q3VBox(grid);
    new QLabel(TR("C++ \ndefinition :"), vtab);

    if (! visit)
        connect(new SmallPushButton(TR("Editor"), vtab), SIGNAL(clicked()),
                this, SLOT(edit_cpp_def()));

    edcpp_def = new MultiLineEdit(grid);
    edcpp_def->setReadOnly(visit);
    edcpp_def->setText(ex->cpp_def);
    edcpp_def->setFont(font);

    addTab(grid, "C++");

    if (!GenerationSettings::cpp_get_default_defs())
        removePage(grid);

    // Java

    grid = new Q3Grid(2, this);
    grid->setMargin(5);
    grid->setSpacing(5);

    vtab = new Q3VBox(grid);
    new QLabel("Java :", vtab);

    if (! visit)
        connect(new SmallPushButton(TR("Editor"), vtab), SIGNAL(clicked()),
                this, SLOT(edit_java_decl()));

    edjava_decl = new MultiLineEdit(grid);
    edjava_decl->setReadOnly(visit);
    edjava_decl->setText(ex->java_decl);
    edjava_decl->setFont(font);

    addTab(grid, "Java");

    if (!GenerationSettings::java_get_default_defs())
        removePage(grid);

    // Php

    grid = new Q3Grid(2, this);
    grid->setMargin(5);
    grid->setSpacing(5);

    vtab = new Q3VBox(grid);
    new QLabel("Php :", vtab);

    if (! visit)
        connect(new SmallPushButton(TR("Editor"), vtab), SIGNAL(clicked()),
                this, SLOT(edit_php_decl()));

    edphp_decl = new MultiLineEdit(grid);
    edphp_decl->setReadOnly(visit);
    edphp_decl->setText(ex->php_decl);
    edphp_decl->setFont(font);

    addTab(grid, "Php");

    if (!GenerationSettings::php_get_default_defs())
        removePage(grid);

    // Python

    grid = new Q3Grid(2, this);
    grid->setMargin(5);
    grid->setSpacing(5);

    vtab = new Q3VBox(grid);
    new QLabel("Python :", vtab);

    if (! visit)
        connect(new SmallPushButton(TR("Editor"), vtab), SIGNAL(clicked()),
                this, SLOT(edit_python_decl()));

    edpython_decl = new MultiLineEdit(grid);
    edpython_decl->setReadOnly(visit);
    edpython_decl->setText(ex->python_decl);
    edpython_decl->setFont(font);

    addTab(grid, "Python");

    if (!GenerationSettings::python_get_default_defs())
        removePage(grid);

    // IDL

    grid = new Q3Grid(2, this);
    grid->setMargin(5);
    grid->setSpacing(5);

    vtab = new Q3VBox(grid);
    new QLabel("Idl :", vtab);

    if (! visit)
        connect(new SmallPushButton(TR("Editor"), vtab), SIGNAL(clicked()),
                this, SLOT(edit_idl_decl()));

    edidl_decl = new MultiLineEdit(grid);
    edidl_decl->setReadOnly(visit);
    edidl_decl->setText(ex->idl_decl);
    edidl_decl->setFont(font);

    addTab(grid, "Idl");

    if (!GenerationSettings::idl_get_default_defs())
        removePage(grid);

    // USER : list key - value

    grid = new Q3Grid(2, this);
    grid->setMargin(5);
    grid->setSpacing(5);

    kvtable = new KeyValuesTable(ex->get_browser_node(), grid, visit);
    addTab(grid, TR("Properties"));

    open_dialog(this);
}

void ExtraMemberDialog::polish()
{
    Q3TabDialog::polish();
    UmlDesktop::limitsize_move(this, previous_size, 0.8, 0.8);
}

ExtraMemberDialog::~ExtraMemberDialog()
{
    emd->browser_node->edit_end();
    previous_size = size();

    foreach (BodyDialog *dialog, edits)
        dialog->close();
    edits.clear();

    close_dialog(this);
}

void ExtraMemberDialog::accept()
{
    if (!check_edits(edits) || !kvtable->check_unique())
        return;

    BrowserNode * bn = emd->browser_node;
    QString s;

    s = edname->text().stripWhiteSpace();
    bn->set_name(s);

    bool newst = emd->set_stereotype(fromUnicode(edstereotype->currentText().stripWhiteSpace()));

    emd->cpp_decl = edcpp_decl->text();
    emd->cpp_def = edcpp_def->text();
    emd->cpp_inline = inline_cb->isChecked();
    emd->java_decl = edjava_decl->text();
    emd->php_decl = edphp_decl->text();
    emd->python_decl = edpython_decl->text();
    emd->idl_decl = edidl_decl->text();

    bn->set_comment(comment->text());
    UmlWindow::update_comment_if_needed(bn);

    kvtable->updateNodeFromThis(bn);

    ProfiledStereotypes::modified(bn, newst);

    bn->modified();
    bn->package_modified();
    emd->modified();

    Q3TabDialog::accept();
}

void ExtraMemberDialog::edit_description()
{
    edit(comment->text(), edname->text().stripWhiteSpace() + "_description",
         emd, TxtEdit, this, (post_edit) post_edit_description, edits);
}

void ExtraMemberDialog::post_edit_description(ExtraMemberDialog * d, QString s)
{
    d->comment->setText(s);
}

void ExtraMemberDialog::edit_cpp_decl()
{
    edit(edcpp_decl->text(), edname->text().stripWhiteSpace() + "_class_extra_member_decl",
         emd, CppEdit, this, (post_edit) post_edit_cpp_decl, edits);
}

void ExtraMemberDialog::post_edit_cpp_decl(ExtraMemberDialog * d, QString s)
{
    d->edcpp_decl->setText(s);
}

void ExtraMemberDialog::edit_cpp_def()
{
    edit(edcpp_def->text(), edname->text().stripWhiteSpace() + "_class_extra_member_def",
         emd, CppEdit, this, (post_edit) post_edit_cpp_def, edits);
}

void ExtraMemberDialog::post_edit_cpp_def(ExtraMemberDialog * d, QString s)
{
    d->edcpp_def->setText(s);
}

void ExtraMemberDialog::edit_java_decl()
{
    edit(edjava_decl->text(), edname->text().stripWhiteSpace() + "_class_extra_member",
         emd, JavaEdit, this, (post_edit) post_edit_java_decl, edits);
}

void ExtraMemberDialog::post_edit_java_decl(ExtraMemberDialog * d, QString s)
{
    d->edjava_decl->setText(s);
}

void ExtraMemberDialog::edit_php_decl()
{
    edit(edphp_decl->text(), edname->text().stripWhiteSpace() + "_class_extra_member",
         emd, PhpEdit, this, (post_edit) post_edit_php_decl, edits);
}

void ExtraMemberDialog::post_edit_php_decl(ExtraMemberDialog * d, QString s)
{
    d->edphp_decl->setText(s);
}

void ExtraMemberDialog::edit_python_decl()
{
    edit(edpython_decl->text(), edname->text().stripWhiteSpace() + "_class_extra_member",
         emd, PythonEdit, this, (post_edit) post_edit_python_decl, edits);
}

void ExtraMemberDialog::post_edit_python_decl(ExtraMemberDialog * d, QString s)
{
    d->edpython_decl->setText(s);
}

void ExtraMemberDialog::edit_idl_decl()
{
    edit(edidl_decl->text(), edname->text().stripWhiteSpace() + "_class_extra_member",
         emd, TxtEdit, this, (post_edit) post_edit_idl_decl, edits);
}

void ExtraMemberDialog::post_edit_idl_decl(ExtraMemberDialog * d, QString s)
{
    d->edidl_decl->setText(s);
}

