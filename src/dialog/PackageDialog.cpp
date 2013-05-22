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





#include <stdio.h>

#include <q3grid.h>
#include <q3vbox.h>
#include <qlabel.h>
#include <q3combobox.h>
#include <qpushbutton.h>
#include <q3filedialog.h>

#include "PackageDialog.h"
#include "PackageData.h"
#include "GenerationSettings.h"
#include "BrowserPackage.h"
#include "KeyValueTable.h"
#include "UmlWindow.h"
#include "DialogUtil.h"
#include "BrowserView.h"
#include "UmlDesktop.h"
#include "BodyDialog.h"
#include "strutil.h"
#include "ProfiledStereotypes.h"
#include "translate.h"

static QString Relative;
static QString Absolute;

QSize PackageDialog::previous_size;

PackageDialog::PackageDialog(PackageData * da)
    : Q3TabDialog(0, 0, FALSE, Qt::WDestructiveClose), pa(da)
{
    Relative = TR("Set it relative");
    Absolute = TR("Set it absolute");

    da->browser_node->edit_start();

    bool visit = !da->browser_node->is_writable();

    if (!visit) {
        setOkButton(TR("OK"));
        setCancelButton(TR("Cancel"));
    }
    else {
        setOkButton(QString());
        setCancelButton(TR("Close"));
    }

    setCaption(TR("Package dialog"));

    Q3Grid * grid;
    Q3VBox * vtab;
    Q3HBox * htab;
    QPushButton * button;

    // general tab

    grid = new Q3Grid(2, this);
    umltab = grid;
    grid->setSpacing(5);
    grid->setMargin(5);

    new QLabel(TR("name : "), grid);
    edname = new LineEdit(pa->name(), grid);
    edname->setReadOnly(!da->browser_node->is_writable() ||
                        (da->browser_node == BrowserView::get_project()));

    new QLabel(TR("stereotype : "), grid);
    edstereotype = new Q3ComboBox(!visit, grid);
    edstereotype->insertItem(toUnicode(pa->stereotype));

    if (! visit) {
        edstereotype->insertStringList(BrowserPackage::default_stereotypes());
        edstereotype->insertStringList(ProfiledStereotypes::defaults(UmlPackage));
        edstereotype->setAutoCompletion(completion());
        connect(edstereotype, SIGNAL(activated(const QString &)),
                this, SLOT(edStereotypeActivated(const QString &)));
    }

    edstereotype->setCurrentItem(0);
    QSizePolicy sp = edstereotype->sizePolicy();
    sp.setHorData(QSizePolicy::Expanding);
    edstereotype->setSizePolicy(sp);

    vtab = new Q3VBox(grid);
    new QLabel(TR("description :"), vtab);

    if (! visit)
        connect(new SmallPushButton(TR("Editor"), vtab), SIGNAL(clicked()),
                this, SLOT(edit_description()));

    comment = new MultiLineEdit(grid);
    comment->setReadOnly(visit);
    comment->setText(da->browser_node->get_comment());
    QFont font = comment->font();

    if (! hasCodec())
        font.setFamily("Courier");

    font.setFixedPitch(TRUE);
    comment->setFont(font);

    addTab(grid, "Uml");

    // C++

    vtab = new Q3VBox(this);
    cppTab = vtab;
    vtab->setMargin(5);

    htab = new Q3HBox(vtab);
    htab->setMargin(5);
    new QLabel(TR("The generation directories may be relative in case the root directory\n\
is specified (through the project menu entry 'edit generation settings')\n\n"), htab);

    htab = new Q3HBox(vtab);
    htab->setMargin(5);
    QLabel * lbl1 = new QLabel(TR("headers directory : "), htab);
    edcpphdir = new LineEdit(pa->cpp_h_dir, htab);

    if (visit)
        edcpphdir->setReadOnly(TRUE);
    else {
        htab = new Q3HBox(vtab);
        new QLabel("", htab);
        button = new QPushButton(TR("Browse"), htab);
        connect(button, SIGNAL(clicked()), this, SLOT(cpph_browse()));
        new QLabel("", htab);
        cpphbutton = new QPushButton((pa->cpp_h_dir.isEmpty() ||
                                      QDir::isRelativePath(pa->cpp_h_dir))
                                     ? Absolute : Relative, htab);

        if (GenerationSettings::get_cpp_root_dir().isEmpty())
            cpphbutton->setEnabled(FALSE);

        connect(cpphbutton, SIGNAL(clicked()), this, SLOT(cpph_relative()));
        new QLabel("", htab);
    }

    htab = new Q3HBox(vtab);
    htab->setMargin(5);
    new QLabel("", htab);

    htab = new Q3HBox(vtab);
    htab->setMargin(5);
    QLabel * lbl2 = new QLabel(TR("sources directory : "), htab);
    edcppsrcdir = new LineEdit(pa->cpp_src_dir, htab);

    if (visit)
        edcppsrcdir->setReadOnly(TRUE);
    else {
        htab = new Q3HBox(vtab);
        new QLabel("", htab);
        button = new QPushButton(TR("Browse"), htab);
        connect(button, SIGNAL(clicked()), this, SLOT(cppsrc_browse()));
        new QLabel("", htab);
        cppsrcbutton =
            new QPushButton((pa->cpp_src_dir.isEmpty() ||
                             QDir::isRelativePath(pa->cpp_src_dir))
                            ? Absolute : Relative, htab);

        if (GenerationSettings::get_cpp_root_dir().isEmpty())
            cppsrcbutton->setEnabled(FALSE);

        connect(cppsrcbutton, SIGNAL(clicked()), this, SLOT(cppsrc_relative()));
        new QLabel("", htab);
    }

    htab = new Q3HBox(vtab);
    htab->setMargin(5);
    new QLabel("", htab);

    htab = new Q3HBox(vtab);
    htab->setMargin(5);
    QLabel * lbl3 = new QLabel("namespace : ", htab);
    edcppnamespace = new LineEdit(pa->cpp_namespace, htab);
    edcppnamespace->setReadOnly(visit);

    same_width(lbl1, lbl2, lbl3);

    vtab->setStretchFactor(new Q3HBox(vtab), 1000);

    addTab(vtab, "C++");

    if (!GenerationSettings::cpp_get_default_defs())
        removePage(vtab);

    // Java

    vtab = new Q3VBox(this);
    javatab = vtab;
    vtab->setMargin(5);

    htab = new Q3HBox(vtab);
    htab->setMargin(5);
    new QLabel(TR("The generation directory may be relative in case the root directory\n\
is specified (through the project menu entry 'edit generation settings')\n\n"), htab);

    htab = new Q3HBox(vtab);
    htab->setMargin(5);
    lbl1 = new QLabel(TR("directory : "), htab);
    edjavadir = new LineEdit(pa->java_dir, htab);

    if (visit)
        edjavadir->setReadOnly(TRUE);
    else {
        htab = new Q3HBox(vtab);
        new QLabel("", htab);
        button = new QPushButton(TR("Browse"), htab);
        connect(button, SIGNAL(clicked()), this, SLOT(java_browse()));
        new QLabel("", htab);
        javabutton = new QPushButton((pa->java_dir.isEmpty() ||
                                      QDir::isRelativePath(pa->java_dir))
                                     ? Absolute : Relative, htab);

        if (GenerationSettings::get_java_root_dir().isEmpty())
            javabutton->setEnabled(FALSE);

        connect(javabutton, SIGNAL(clicked()), this, SLOT(java_relative()));
        new QLabel("", htab);
    }

    htab = new Q3HBox(vtab);
    htab->setMargin(5);
    new QLabel("", htab);

    htab = new Q3HBox(vtab);
    htab->setMargin(5);
    lbl2 = new QLabel("package : ", htab);
    edjavapackage = new LineEdit(pa->java_package, htab);
    edjavapackage->setReadOnly(visit);

    same_width(lbl1, lbl2);

    vtab->setStretchFactor(new Q3HBox(vtab), 1000);

    addTab(vtab, "Java");

    if (!GenerationSettings::java_get_default_defs())
        removePage(vtab);

    // Php

    vtab = new Q3VBox(this);
    phptab = vtab;
    vtab->setMargin(5);

    htab = new Q3HBox(vtab);
    htab->setMargin(5);
    new QLabel(TR("The generation directory may be relative in case the root directory\n\
is specified (through the project menu entry 'edit generation settings')\n\n"), htab);

    htab = new Q3HBox(vtab);
    htab->setMargin(5);
    lbl1 = new QLabel(TR("directory : "), htab);
    edphpdir = new LineEdit(pa->php_dir, htab);

    if (visit)
        edphpdir->setReadOnly(TRUE);
    else {
        htab = new Q3HBox(vtab);
        new QLabel("", htab);
        button = new QPushButton(TR("Browse"), htab);
        connect(button, SIGNAL(clicked()), this, SLOT(php_browse()));
        new QLabel("", htab);
        phpbutton = new QPushButton((pa->php_dir.isEmpty() ||
                                     QDir::isRelativePath(pa->php_dir))
                                    ? Absolute : Relative, htab);

        if (GenerationSettings::get_php_root_dir().isEmpty())
            phpbutton->setEnabled(FALSE);

        connect(phpbutton, SIGNAL(clicked()), this, SLOT(php_relative()));
        new QLabel("", htab);
    }

    htab = new Q3HBox(vtab);
    htab->setMargin(5);
    new QLabel("", htab);

    htab = new Q3HBox(vtab);
    htab->setMargin(5);
    lbl2 = new QLabel("namespace : ", htab);
    edphpnamespace = new LineEdit(pa->php_namespace, htab);
    edphpnamespace->setReadOnly(visit);

    same_width(lbl1, lbl2);

    vtab->setStretchFactor(new Q3HBox(vtab), 1000);

    addTab(vtab, "Php");

    if (!GenerationSettings::php_get_default_defs())
        removePage(vtab);

    // Python

    vtab = new Q3VBox(this);
    pythontab = vtab;
    vtab->setMargin(5);

    htab = new Q3HBox(vtab);
    htab->setMargin(5);
    new QLabel(TR("The generation directory may be relative in case the root directory\n\
is specified (through the project menu entry 'edit generation settings')\n\n"), htab);

    htab = new Q3HBox(vtab);
    htab->setMargin(5);
    lbl1 = new QLabel(TR("directory : "), htab);
    edpythondir = new LineEdit(pa->python_dir, htab);

    if (visit)
        edpythondir->setReadOnly(TRUE);
    else {
        htab = new Q3HBox(vtab);
        new QLabel("", htab);
        button = new QPushButton(TR("Browse"), htab);
        connect(button, SIGNAL(clicked()), this, SLOT(python_browse()));
        new QLabel("", htab);
        pythonbutton = new QPushButton((pa->python_dir.isEmpty() ||
                                        QDir::isRelativePath(pa->python_dir))
                                       ? Absolute : Relative, htab);

        if (GenerationSettings::get_python_root_dir().isEmpty())
            pythonbutton->setEnabled(FALSE);

        connect(pythonbutton, SIGNAL(clicked()), this, SLOT(python_relative()));
        new QLabel("", htab);
    }

    htab = new Q3HBox(vtab);
    htab->setMargin(5);
    new QLabel("", htab);

    htab = new Q3HBox(vtab);
    htab->setMargin(5);
    lbl2 = new QLabel("package : ", htab);
    edpythonpackage = new LineEdit(pa->python_package, htab);
    edpythonpackage->setReadOnly(visit);

    same_width(lbl1, lbl2);

    vtab->setStretchFactor(new Q3HBox(vtab), 1000);

    addTab(vtab, "Python");

    if (!GenerationSettings::python_get_default_defs())
        removePage(vtab);

    // IDL

    vtab = new Q3VBox(this);
    idltab = vtab;
    vtab->setMargin(5);

    htab = new Q3HBox(vtab);
    htab->setMargin(5);
    new QLabel(TR("The generation directory may be relative in case the root directory\n\
is specified (through the project menu entry 'edit generation settings')\n\n"), htab);

    htab = new Q3HBox(vtab);
    htab->setMargin(5);
    lbl1 = new QLabel(TR("directory : "), htab);
    edidldir = new LineEdit(pa->idl_dir, htab);

    if (visit)
        edidldir->setReadOnly(TRUE);
    else {
        htab = new Q3HBox(vtab);
        new QLabel("", htab);
        button = new QPushButton(TR("Browse"), htab);
        connect(button, SIGNAL(clicked()), this, SLOT(idl_browse()));
        new QLabel("", htab);
        idlbutton = new QPushButton((pa->idl_dir.isEmpty() ||
                                     QDir::isRelativePath(pa->idl_dir))
                                    ? Absolute : Relative, htab);

        if (GenerationSettings::get_idl_root_dir().isEmpty())
            idlbutton->setEnabled(FALSE);

        connect(idlbutton, SIGNAL(clicked()), this, SLOT(idl_relative()));
        new QLabel("", htab);
    }

    htab = new Q3HBox(vtab);
    htab->setMargin(5);
    new QLabel("", htab);

    htab = new Q3HBox(vtab);
    htab->setMargin(5);
    lbl2 = new QLabel("module : ", htab);
    edidlmodule = new LineEdit(pa->idl_module, htab);
    edidlmodule->setReadOnly(visit);

    same_width(lbl1, lbl2);

    vtab->setStretchFactor(new Q3HBox(vtab), 1000);

    addTab(vtab, "IDL");

    if (!GenerationSettings::idl_get_default_defs())
        removePage(vtab);

    // Profile

    vtab = new Q3VBox(this);
    profiletab = vtab;
    vtab->setMargin(5);

    htab = new Q3HBox(vtab);
    htab->setMargin(5);
    new QLabel("", htab);

    htab = new Q3HBox(vtab);
    htab->setMargin(5);
    lbl1 = new QLabel(TR("meta model : \nreference"), htab);
    edmetamodelReference =
        new LineEdit(pa->browser_node->get_value("metamodelReference"), htab);
    edmetamodelReference->setReadOnly(visit);

    htab = new Q3HBox(vtab);
    htab->setMargin(5);
    new QLabel("", htab);

    htab = new Q3HBox(vtab);
    htab->setMargin(5);
    lbl2 = new QLabel(TR("meta class : \nreference"), htab);
    edmetaclassreference =
        new LineEdit(pa->browser_node->get_value("metaclassreference"), htab);
    edidlmodule->setReadOnly(visit);

    same_width(lbl1, lbl2);

    vtab->setStretchFactor(new Q3HBox(vtab), 1000);

    addTab(vtab, TR("Profile"));

    // USER : list key - value

    vtab = new Q3VBox(this);
    kvtable = new KeyValuesTable(da->browser_node, vtab, visit);
    kvtable->remove("metamodelReference");
    kvtable->remove("metaclassreference");
    addTab(vtab, TR("Properties"));

    //

    edStereotypeActivated(edstereotype->currentText());

    connect(this, SIGNAL(currentChanged(QWidget *)),
            this, SLOT(change_tabs(QWidget *)));

    open_dialog(this);
}

void PackageDialog::polish()
{
    Q3TabDialog::polish();
    UmlDesktop::limitsize_move(this, previous_size, 0.8, 0.8);
}

PackageDialog::~PackageDialog()
{
    pa->browser_node->edit_end();
    previous_size = size();

    foreach (BodyDialog *dialog, edits)
        dialog->close();
    edits.clear();

    close_dialog(this);
}

void PackageDialog::edStereotypeActivated(const QString & s)
{
    bool np = (s.stripWhiteSpace() != "profile");

    if (GenerationSettings::cpp_get_default_defs())
        setTabEnabled(cppTab, np);

    if (GenerationSettings::java_get_default_defs())
        setTabEnabled(javatab, np);

    if (GenerationSettings::idl_get_default_defs())
        setTabEnabled(idltab, np);

    if (GenerationSettings::php_get_default_defs())
        setTabEnabled(phptab, np);

    if (GenerationSettings::python_get_default_defs())
        setTabEnabled(pythontab, np);

    setTabEnabled(profiletab, !np);

    if (!np &&
        edmetamodelReference->text().simplifyWhiteSpace().isEmpty() &&
        edmetaclassreference->text().simplifyWhiteSpace().isEmpty())
        edmetamodelReference->setText("http://schema.omg.org/spec/UML/2.1/uml.xml");
}

void PackageDialog::change_tabs(QWidget * w)
{
    if (hasOkButton()) {
        if (w == umltab)
            edname->setFocus();
        else if (w == cppTab)
            edcpphdir->setFocus();
        else if (w == javatab)
            edjavadir->setFocus();
        else if (w == phptab)
            edphpdir->setFocus();
        else if (w == pythontab)
            edpythondir->setFocus();
        else if (w == idltab)
            edidldir->setFocus();
    }
}

void PackageDialog::edit_description()
{
    edit(comment->text(), edname->text().stripWhiteSpace() + "_description",
         pa, TxtEdit, this, (post_edit) post_edit_description, edits);
}

void PackageDialog::post_edit_description(PackageDialog * d, QString s)
{
    d->comment->setText(s);
}

void PackageDialog::accept()
{
    if (!check_edits(edits) || !kvtable->check_unique())
        return;

    BrowserPackage * bn = (BrowserPackage *) pa->browser_node;
    QString oldname = pa->name();
    bool was_pr = !strcmp(pa->get_stereotype(), "profile");
    QString st = fromUnicode(edstereotype->currentText().stripWhiteSpace());
    QString s;

    s = edname->text().stripWhiteSpace();

    if (s != oldname) {
        if (((BrowserNode *) bn->parent())->wrong_child_name(s, UmlPackage,
                bn->allow_spaces(),
                bn->allow_empty())) {
            msg_critical(TR("Error"), s + TR("\n\nillegal name or already used"));
            return;
        }

        if ((st == "profile") && !ProfiledStereotypes::canAddPackage(bn, s)) {
            msg_critical(TR("Error"), TR("conflict on stereotypes"));
            return;
        }

        bn->set_name(s);
    }
    else if ((st == "profile") &&
             !was_pr &&
             !ProfiledStereotypes::canAddPackage(bn, s)) {
        msg_critical(TR("Error"), TR("conflict on stereotypes"));
        return;
    }

    bool newst = pa->set_stereotype(st);

    pa->cpp_h_dir = edcpphdir->text().simplifyWhiteSpace();
    pa->cpp_src_dir = edcppsrcdir->text().simplifyWhiteSpace();
    pa->cpp_namespace = edcppnamespace->text().simplifyWhiteSpace();
    pa->java_dir = edjavadir->text().simplifyWhiteSpace();
    pa->java_package = edjavapackage->text().simplifyWhiteSpace();
    pa->php_dir = edphpdir->text().simplifyWhiteSpace();
    pa->php_namespace = edphpnamespace->text().simplifyWhiteSpace();
    pa->python_dir = edpythondir->text().simplifyWhiteSpace();
    pa->python_package = edpythonpackage->text().simplifyWhiteSpace();
    pa->idl_dir = edidldir->text().simplifyWhiteSpace();
    pa->idl_module = edidlmodule->text().simplifyWhiteSpace();

    bn->set_comment(comment->text());
    UmlWindow::update_comment_if_needed(bn);

    kvtable->updateNodeFromThis(bn);

    if (st == "profile") {
        unsigned n = bn->get_n_keys();

        bn->set_n_keys(n + 2);
        bn->set_key(n, "metamodelReference");
        bn->set_value(n, fromUnicode(edmetamodelReference->text().simplifyWhiteSpace()));
        bn->set_key(n + 1, "metaclassreference");
        bn->set_value(n + 1, fromUnicode(edmetaclassreference->text().simplifyWhiteSpace()));

        if (was_pr && (oldname != bn->get_name()))
            ProfiledStereotypes::renamed(bn, oldname);
    }
    else if (was_pr) {
        bool propag = (msg_warning(TR("Question"),
                                   TR("Propagate the removal of the profile ?"),
                                   1, 2)
                       == 1);

        ProfiledStereotypes::deleted(bn, propag);
    }

    ProfiledStereotypes::modified(bn, newst);

    bn->package_modified();
    pa->modified();

    Q3TabDialog::accept();
}

void PackageDialog::browse(LineEdit * ed, QPushButton * button,
                           const char * caption, QString root)
{
    QString s = ed->text();
    bool rel = FALSE;

    if (s.isNull())
        s = root;
    else if (!root.isEmpty() && QDir::isRelativePath(s)) {
        rel = TRUE;
        s = root + s;
    }

    const QString ns = Q3FileDialog::getExistingDirectory(s, this, 0, caption);


    if (! ns.isNull()) {
        int len = root.length();

        if (rel &&
            (ns.find(root) == 0) &&
            (ns.length() >= len))
            ed->setText(ns.mid(len));
        else {
            ed->setText(ns);
            button->setText(Relative);
        }
    }
}

void PackageDialog::cpph_browse()
{
    browse(edcpphdir, cpphbutton, TR("C++ header directory"),
           GenerationSettings::get_cpp_root_dir());
}

void PackageDialog::cppsrc_browse()
{
    browse(edcppsrcdir, cppsrcbutton, TR("C++ source directory"),
           GenerationSettings::get_cpp_root_dir());
}

void PackageDialog::java_browse()
{
    browse(edjavadir, javabutton, TR("Java directory"),
           GenerationSettings::get_java_root_dir());
}

void PackageDialog::php_browse()
{
    browse(edphpdir, phpbutton, TR("Php directory"),
           GenerationSettings::get_php_root_dir());
}

void PackageDialog::python_browse()
{
    browse(edpythondir, pythonbutton, TR("Python directory"),
           GenerationSettings::get_python_root_dir());
}

void PackageDialog::idl_browse()
{
    browse(edidldir, idlbutton, TR("Idl directory"),
           GenerationSettings::get_idl_root_dir());
}

void PackageDialog::relative(LineEdit * ed, QPushButton * button,
                             QString root)
{
    const QString s = ed->text();

    if (QDir::isRelativePath(root))
        root = BrowserView::get_dir().filePath(root);

    if (button->text() == Relative) {
        int len = root.length();

        if (



            (s.find(root) == 0) &&

            (s.length() >= len)) {
            ed->setText(s.mid(len));
            button->setText(Absolute);
        }
    }
    else {
        ed->setText(root + s);
        button->setText(Relative);
    }
}

void PackageDialog::cpph_relative()
{
    relative(edcpphdir, cpphbutton, GenerationSettings::get_cpp_root_dir());
}

void PackageDialog::cppsrc_relative()
{
    relative(edcppsrcdir, cppsrcbutton, GenerationSettings::get_cpp_root_dir());
}

void PackageDialog::java_relative()
{
    relative(edjavadir, javabutton, GenerationSettings::get_java_root_dir());
}

void PackageDialog::php_relative()
{
    relative(edphpdir, phpbutton, GenerationSettings::get_php_root_dir());
}

void PackageDialog::python_relative()
{
    relative(edpythondir, pythonbutton, GenerationSettings::get_python_root_dir());
}

void PackageDialog::idl_relative()
{
    relative(edidldir, idlbutton, GenerationSettings::get_idl_root_dir());
}
