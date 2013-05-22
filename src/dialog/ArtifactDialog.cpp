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





#include <qsplitter.h>
#include <q3grid.h>
#include <q3vbox.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <q3combobox.h>
//Added by qt3to4:
#include <Q3ValueList>

#include "ArtifactDialog.h"
#include "UmlDesktop.h"
#include "BrowserArtifact.h"
#include "ArtifactData.h"
#include "PackageData.h"
#include "GenerationSettings.h"
#include "CustomWidgets/EdgeMenuToolBar.h"
#include "BrowserClass.h"
#include "UmlWindow.h"
#include "DialogUtil.h"
#include "KeyValueTable.h"
#include "ClassDialog.h"
#include "ClassData.h"
#include "ListBoxBrowserNode.h"
#include "strutil.h"
#include "UmlPixmap.h"
#include "BodyDialog.h"
#include "ProfiledStereotypes.h"
#include "translate.h"
#include "misc/TypeIdentifier.h"

QSize ArtifactDialog::previous_size;
static QScopedPointer<ArtifactDialog> instance;
ArtifactDialog::ArtifactDialog(ArtifactData * nd)
    : EdgeMenuDialog(0, 0, FALSE), data(nd)
{
    nd->browser_node->edit_start();

    if (nd->browser_node->is_writable()) {
        setOkButton(TR("OK"));
        setCancelButton(TR("Cancel"));
    }
    else {
        setOkButton(QString());
        setCancelButton(TR("Close"));
    }

    setCaption(TR("artifact dialog"));

    init_uml_tab();
    init_cpp_tab();
    init_java_tab();
    init_php_tab();
    init_python_tab();
    init_idl_tab();
    init_assoc_classes_tab();
    init_assoc_artifacts_tab();

    // USER : list key - value

    Q3Grid * grid = new Q3Grid(2, this);
    grid->setMargin(5);
    grid->setSpacing(5);

    kvtable = new KeyValuesTable((BrowserArtifact *) data->get_browser_node(),
                                 grid, !hasOkButton());
    addTab(grid, TR("Properties"));

    //

    edStereotypeActivated(data->get_stereotype());

    connect(this, SIGNAL(currentChanged(QWidget *)),
            this, SLOT(update_tab(QWidget *)));

    //open_dialog(this);
}

void ArtifactDialog::polish()
{
    Q3TabDialog::polish();
    UmlDesktop::setsize_center(this, previous_size, 0.8, 0.8);
}

ArtifactDialog::~ArtifactDialog()
{
    //data->browser_node->edit_end();
    previous_size = size();

    foreach (BodyDialog *dialog, edits)
        dialog->close();
    edits.clear();

    if(toolbar)
    {
        toolbar->setParent(0);
        toolbar->hide();
    }
    close_dialog(this);

}

void ArtifactDialog::init_uml_tab()
{
    bool visit = !hasOkButton();

    BrowserArtifact * bn = (BrowserArtifact *) data->get_browser_node();
    Q3VBox * vbox;
    Q3Grid * grid = new Q3Grid(2, this);

    uml_page = grid;
    grid->setMargin(5);
    grid->setSpacing(5);

    new QLabel(TR("name : "), grid);
    edname = new LineEdit(bn->get_name(), grid);
    edname->setReadOnly(visit);

    new QLabel(TR("stereotype : "), grid);
    edstereotype = new Q3ComboBox(TRUE, grid);
    edstereotype->insertItem(toUnicode(data->get_stereotype()));

    if (! visit) {
        edstereotype->insertStringList(BrowserArtifact::default_stereotypes());
        edstereotype->insertStringList(ProfiledStereotypes::defaults(UmlArtifact));
        edstereotype->setAutoCompletion(completion());
    }

    edstereotype->setCurrentItem(0);
    QSizePolicy sp = edstereotype->sizePolicy();
    sp.setHorData(QSizePolicy::Expanding);
    edstereotype->setSizePolicy(sp);

    if (! visit)
        connect(edstereotype, SIGNAL(activated(const QString &)),
                this, SLOT(edStereotypeActivated(const QString &)));

    vbox = new Q3VBox(grid);
    new QLabel(TR("description :"), vbox);

    if (! visit) {
        connect(new SmallPushButton(TR("Editor"), vbox), SIGNAL(clicked()),
                this, SLOT(edit_description()));
        connect(new SmallPushButton(TR("Default"), vbox), SIGNAL(clicked()),
                this, SLOT(default_description()));
    }

    comment = new MultiLineEdit(grid);
    comment->setReadOnly(visit);
    comment->setText(bn->get_comment());
    QFont font = comment->font();

    if (! hasCodec())
        font.setFamily("Courier");

    font.setFixedPitch(TRUE);
    comment->setFont(font);

    addTab(grid, "Uml");
}

void ArtifactDialog::init_cpp_tab()
{
    bool visit = !hasOkButton();
    Q3HBox * hbox;
    Q3VBox * vbox;
    QLabel * lbl1;
    QLabel * lbl2;
    QPushButton * edit = 0;

    // C++ h

    cpp_h_content_page = new QSplitter(Qt::Vertical, this);
    cpp_h_content_page->setOpaqueResize(TRUE);

    vbox = new Q3VBox(cpp_h_content_page);

    hbox = new Q3HBox(vbox);
    hbox->setMargin(5);
    lbl1 = new QLabel(TR("Header file \ndefinition : "), hbox);
    edcpp_h_content = new MultiLineEdit(hbox);
    edcpp_h_content->setText(data->cpp_h);
    QFont font = comment->font();

    if (! hasCodec())
        font.setFamily("Courier");

    font.setFixedPitch(TRUE);
    edcpp_h_content->setFont(font);

    if (visit)
        edcpp_h_content->setReadOnly(TRUE);
    else {
        connect(edcpp_h_content, SIGNAL(textChanged()), this, SLOT(cpp_update_h()));

        hbox = new Q3HBox(vbox);
        edit = new SmallPushButton(TR("Editor"), hbox);
        connect(edit, SIGNAL(clicked()), this, SLOT(cpp_edit_h()));
        connect(new QPushButton(TR("Default definition"), hbox), SIGNAL(clicked()),
                this, SLOT(cpp_default_h()));
        connect(new QPushButton(TR("Not generated in C++"), hbox), SIGNAL(clicked()),
                this, SLOT(cpp_unmapped_h()));
    }

    hbox = new Q3HBox(cpp_h_content_page, "result");
    hbox->setMargin(5);
    lbl2 = new QLabel(TR("Result after\nsubstitution : "), hbox);
    showcpp_h_content = new MultiLineEdit(hbox);
    showcpp_h_content->setReadOnly(TRUE);
    showcpp_h_content->setFont(font);

    if (visit)
        same_width(lbl1, lbl2);
    else
        same_width(lbl1, lbl2, edit);

    addTab(cpp_h_content_page, TR("C++ header"));

    if (!GenerationSettings::cpp_get_default_defs())
        removePage(cpp_h_content_page);

    // C++ source

    cpp_src_content_page = new QSplitter(Qt::Vertical, this);
    cpp_src_content_page->setOpaqueResize(TRUE);

    vbox = new Q3VBox(cpp_src_content_page);

    hbox = new Q3HBox(vbox);
    hbox->setMargin(5);
    lbl1 = new QLabel(TR("Source file \ndefinition : "), hbox);
    edcpp_src_content = new MultiLineEdit(hbox);
    edcpp_src_content->setText(data->cpp_src);
    edcpp_src_content->setFont(font);

    if (visit)
        edcpp_src_content->setReadOnly(TRUE);
    else {
        connect(edcpp_src_content, SIGNAL(textChanged()), this, SLOT(cpp_update_src()));

        hbox = new Q3HBox(vbox);
        edit = new SmallPushButton(TR("Editor"), hbox);
        connect(edit, SIGNAL(clicked()), this, SLOT(cpp_edit_src()));
        connect(new QPushButton(TR("Default definition"), hbox), SIGNAL(clicked()),
                this, SLOT(cpp_default_src()));
        connect(new QPushButton(TR("Not generated in C++"), hbox), SIGNAL(clicked()),
                this, SLOT(cpp_unmapped_src()));
    }

    hbox = new Q3HBox(cpp_src_content_page, "result");
    hbox->setMargin(5);
    lbl2 = new QLabel(TR("Result after\nsubstitution : "), hbox);
    showcpp_src_content = new MultiLineEdit(hbox);
    showcpp_src_content->setReadOnly(TRUE);
    showcpp_src_content->setFont(font);

    if (visit)
        same_width(lbl1, lbl2);
    else
        same_width(lbl1, lbl2, edit);

    addTab(cpp_src_content_page, TR("C++ source"));

    if (!GenerationSettings::cpp_get_default_defs())
        removePage(cpp_src_content_page);
}

void ArtifactDialog::init_java_tab()
{
    bool visit = !hasOkButton();
    Q3HBox * hbox;
    Q3VBox * vbox;
    QLabel * lbl1;
    QLabel * lbl2;
    QPushButton * edit = 0;

    java_content_page = new QSplitter(Qt::Vertical, this);
    java_content_page->setOpaqueResize(TRUE);

    vbox = new Q3VBox(java_content_page);

    hbox = new Q3HBox(vbox);
    hbox->setMargin(5);
    lbl1 = new QLabel(TR("File \ndefinition : "), hbox);
    edjava_content = new MultiLineEdit(hbox);
    edjava_content->setText(data->java_src);
    QFont font = comment->font();

    if (! hasCodec())
        font.setFamily("Courier");

    font.setFixedPitch(TRUE);
    edjava_content->setFont(font);

    if (visit)
        edjava_content->setReadOnly(TRUE);
    else {
        connect(edjava_content, SIGNAL(textChanged()), this, SLOT(java_update_src()));

        hbox = new Q3HBox(vbox);
        edit = new SmallPushButton(TR("Editor"), hbox);
        connect(edit, SIGNAL(clicked()), this, SLOT(java_edit()));
        connect(new QPushButton(TR("Default definition"), hbox), SIGNAL(clicked()),
                this, SLOT(java_default_src()));
        connect(new QPushButton(TR("Not generated in Java"), hbox), SIGNAL(clicked()),
                this, SLOT(java_unmapped_src()));
    }

    hbox = new Q3HBox(java_content_page, "result");
    hbox->setMargin(5);
    lbl2 = new QLabel(TR("Result after\nsubstitution : "), hbox);
    showjava_content = new MultiLineEdit(hbox);
    showjava_content->setReadOnly(TRUE);
    showjava_content->setFont(font);

    if (visit)
        same_width(lbl1, lbl2);
    else
        same_width(lbl1, lbl2, edit);

    addTab(java_content_page, TR("Java source"));

    if (!GenerationSettings::java_get_default_defs())
        removePage(java_content_page);
}

void ArtifactDialog::init_php_tab()
{
    bool visit = !hasOkButton();
    Q3HBox * hbox;
    Q3VBox * vbox;
    QLabel * lbl1;
    QLabel * lbl2;
    QPushButton * edit = 0;

    php_content_page = new QSplitter(Qt::Vertical, this);
    php_content_page->setOpaqueResize(TRUE);

    vbox = new Q3VBox(php_content_page);

    hbox = new Q3HBox(vbox);
    hbox->setMargin(5);
    lbl1 = new QLabel(TR("File \ndefinition : "), hbox);
    edphp_content = new MultiLineEdit(hbox);
    edphp_content->setText(data->php_src);
    QFont font = comment->font();

    if (! hasCodec())
        font.setFamily("Courier");

    font.setFixedPitch(TRUE);
    edphp_content->setFont(font);

    if (visit)
        edphp_content->setReadOnly(TRUE);
    else {
        connect(edphp_content, SIGNAL(textChanged()), this, SLOT(php_update_src()));

        hbox = new Q3HBox(vbox);
        edit = new SmallPushButton(TR("Editor"), hbox);
        connect(edit, SIGNAL(clicked()), this, SLOT(php_edit()));
        connect(new QPushButton(TR("Default definition"), hbox), SIGNAL(clicked()),
                this, SLOT(php_default_src()));
        connect(new QPushButton(TR("Not generated in Php"), hbox), SIGNAL(clicked()),
                this, SLOT(php_unmapped_src()));
    }

    hbox = new Q3HBox(php_content_page, "result");
    hbox->setMargin(5);
    lbl2 = new QLabel(TR("Result after\nsubstitution : "), hbox);
    showphp_content = new MultiLineEdit(hbox);
    showphp_content->setReadOnly(TRUE);
    showphp_content->setFont(font);

    if (visit)
        same_width(lbl1, lbl2);
    else
        same_width(lbl1, lbl2, edit);

    addTab(php_content_page, TR("Php source"));

    if (!GenerationSettings::php_get_default_defs())
        removePage(php_content_page);
}

void ArtifactDialog::init_python_tab()
{
    bool visit = !hasOkButton();
    Q3HBox * hbox;
    Q3VBox * vbox;
    QLabel * lbl1;
    QLabel * lbl2;
    QPushButton * edit = 0;

    python_content_page = new QSplitter(Qt::Vertical, this);
    python_content_page->setOpaqueResize(TRUE);

    vbox = new Q3VBox(python_content_page);

    hbox = new Q3HBox(vbox);
    hbox->setMargin(5);
    lbl1 = new QLabel(TR("File \ndefinition : "), hbox);
    edpython_content = new MultiLineEdit(hbox);
    edpython_content->setText(data->python_src);
    QFont font = comment->font();

    if (! hasCodec())
        font.setFamily("Courier");

    font.setFixedPitch(TRUE);
    edpython_content->setFont(font);

    if (visit)
        edpython_content->setReadOnly(TRUE);
    else {
        connect(edpython_content, SIGNAL(textChanged()), this, SLOT(python_update_src()));

        hbox = new Q3HBox(vbox);
        edit = new SmallPushButton(TR("Editor"), hbox);
        connect(edit, SIGNAL(clicked()), this, SLOT(python_edit()));
        connect(new QPushButton(TR("Default definition"), hbox), SIGNAL(clicked()),
                this, SLOT(python_default_src()));
        connect(new QPushButton(TR("Not generated in Python"), hbox), SIGNAL(clicked()),
                this, SLOT(python_unmapped_src()));
    }

    hbox = new Q3HBox(python_content_page, "result");
    hbox->setMargin(5);
    lbl2 = new QLabel(TR("Result after\nsubstitution : "), hbox);
    showpython_content = new MultiLineEdit(hbox);
    showpython_content->setReadOnly(TRUE);
    showpython_content->setFont(font);

    if (visit)
        same_width(lbl1, lbl2);
    else
        same_width(lbl1, lbl2, edit);

    addTab(python_content_page, TR("Python source"));

    if (!GenerationSettings::python_get_default_defs())
        removePage(python_content_page);
}

void ArtifactDialog::init_idl_tab()
{
    bool visit = !hasOkButton();
    Q3HBox * hbox;
    Q3VBox * vbox;
    QLabel * lbl1;
    QLabel * lbl2;
    QPushButton * edit = 0;

    idl_content_page = new QSplitter(Qt::Vertical, this);
    idl_content_page->setOpaqueResize(TRUE);

    vbox = new Q3VBox(idl_content_page);

    hbox = new Q3HBox(vbox);
    hbox->setMargin(5);
    lbl1 = new QLabel(TR("File \ndefinition : "), hbox);
    edidl_content = new MultiLineEdit(hbox);
    edidl_content->setText(data->idl_src);
    QFont font = comment->font();

    if (! hasCodec())
        font.setFamily("Courier");

    font.setFixedPitch(TRUE);
    edidl_content->setFont(font);

    if (visit)
        edidl_content->setReadOnly(TRUE);
    else {
        connect(edidl_content, SIGNAL(textChanged()), this, SLOT(idl_update_src()));

        hbox = new Q3HBox(vbox);
        edit = new SmallPushButton(TR("Editor"), hbox);
        connect(edit, SIGNAL(clicked()), this, SLOT(idl_edit()));
        connect(new QPushButton(TR("Default definition"), hbox), SIGNAL(clicked()),
                this, SLOT(idl_default_src()));
        connect(new QPushButton(TR("Not generated in Idl"), hbox), SIGNAL(clicked()),
                this, SLOT(idl_unmapped_src()));
    }

    hbox = new Q3HBox(idl_content_page, "result");
    hbox->setMargin(5);
    lbl2 = new QLabel(TR("Result after\nsubstitution : "), hbox);
    showidl_content = new MultiLineEdit(hbox);
    showidl_content->setReadOnly(TRUE);
    showidl_content->setFont(font);

    if (visit)
        same_width(lbl1, lbl2);
    else
        same_width(lbl1, lbl2, edit);

    addTab(idl_content_page, TR("Idl source"));

    if (!GenerationSettings::idl_get_default_defs())
        removePage(idl_content_page);
}

void ArtifactDialog::init_assoc_classes_tab()
{
    bool visit = !hasOkButton();
    Q3HBox * hbox;
    Q3VBox * vbox;
    QPushButton * button;
    BrowserArtifact * bn = (BrowserArtifact *) data->get_browser_node();
    const Q3ValueList<BrowserClass *> & l = bn->get_associated_classes();
    Q3ValueList<BrowserClass *>::ConstIterator it;
    Q3ValueList<BrowserClass *>::ConstIterator end = l.end();

    cl_assoc_page = new Q3VBox(this);

    if (!visit) {
        hbox = new Q3HBox(cl_assoc_page);
        vbox = new Q3VBox(hbox);
        vbox->setMargin(5);
        (new QLabel(TR("Classes without artifact"), vbox))->setAlignment(Qt::AlignCenter);
        lb_cl_available = new Q3ListBox(vbox);
        lb_cl_available->setSelectionMode(Q3ListBox::Multi);

        Q3PtrDict<BrowserNode> d(97);

        for (it = l.begin(); it != end; ++it)
            d.insert(*it, *it);

        BrowserNodeList classes;

        BrowserClass::instances(classes, 0, TRUE);

        foreach (BrowserNode *cl, classes)
            if ((((BrowserClass *) cl)->get_associated_artifact() == 0) &&
                (d.find(cl) == 0))
                lb_cl_available->insertItem(new ListBoxBrowserNode(cl, cl->full_name(TRUE)));

        lb_cl_available->sort();

        vbox = new Q3VBox(hbox);
        vbox->setMargin(5);
        (new QLabel("", vbox))->setScaledContents(TRUE);
        button = new QPushButton(vbox);
        button->setPixmap(*rightPixmap);
        connect(button, SIGNAL(clicked()), this, SLOT(associate_cls()));
        (new QLabel("", vbox))->setScaledContents(TRUE);
        button = new QPushButton(vbox);
        button->setPixmap(*leftPixmap);
        connect(button, SIGNAL(clicked()), this, SLOT(unassociate_cls()));
        (new QLabel("", vbox))->setScaledContents(TRUE);
        vbox = new Q3VBox(hbox);
    }
    else
        vbox = cl_assoc_page;

    vbox->setMargin(5);
    (new QLabel(TR("Associated classes"), vbox))->setAlignment(Qt::AlignCenter);
    lb_cl_associated = new Q3ListBox(vbox);
    lb_cl_associated->setSelectionMode((visit) ? Q3ListBox::NoSelection
                                       : Q3ListBox::Multi);

    n_cpp = n_java = n_php = n_python = n_idl = 0;

    for (it = l.begin(); it != end; ++it) {
        if (!(*it)->deletedp() && !(*it)->nestedp()) {
            lb_cl_associated->insertItem(new ListBoxBrowserNode(*it, (*it)->full_name(TRUE)));

            ClassData * c = (ClassData *)(*it)->get_data();

            if (c->get_cppdecl()[0])
                n_cpp += 1;

            if (c->get_javadecl()[0])
                n_java += 1;

            if (c->get_phpdecl()[0])
                n_php += 1;

            if (c->get_pythondecl()[0])
                n_python += 1;

            if (c->get_idldecl()[0])
                n_idl += 1;
        }
    }

    if (!visit) {
        hbox = new Q3HBox(vbox);
        hbox->setMargin(5);
        (new QLabel("", hbox))->setScaledContents(TRUE);
        button = new QPushButton(TR("go up"), hbox);
        connect(button, SIGNAL(clicked()), this, SLOT(move_cls_up()));
        (new QLabel("", hbox))->setScaledContents(TRUE);
        button = new QPushButton(TR("go down"), hbox);
        connect(button, SIGNAL(clicked()), this, SLOT(move_cls_down()));
        (new QLabel("", hbox))->setScaledContents(TRUE);
    }

    addTab(cl_assoc_page, TR("Associated classes"));
}

void ArtifactDialog::init_assoc_artifacts_tab()
{
    bool visit = !hasOkButton();
    Q3HBox * hbox;
    Q3VBox * vbox;
    QPushButton * button;

    art_assoc_page = new Q3VBox(this);

    if (!visit) {
        hbox = new Q3HBox(art_assoc_page);
        hbox->setMargin(5);
        new QLabel(TR("Stereotype filtering  "), hbox);
        stereotypefilter = new Q3ComboBox(TRUE, hbox);
        stereotypefilter->setAutoCompletion(completion());
        stereotypefilter->insertItem("");
        stereotypefilter->insertStringList(BrowserArtifact::default_stereotypes());
        stereotypefilter->insertStringList(ProfiledStereotypes::defaults(UmlArtifact));
        stereotypefilter->setCurrentItem(0);
        QSizePolicy sp = stereotypefilter->sizePolicy();
        sp.setHorData(QSizePolicy::Expanding);
        stereotypefilter->setSizePolicy(sp);
        connect(stereotypefilter, SIGNAL(activated(const QString &)),
                this, SLOT(stereotypeFilterActivated(const QString &)));

        hbox = new Q3HBox(art_assoc_page);
        vbox = new Q3VBox(hbox);
        vbox->setMargin(5);
        (new QLabel(TR("Unassociated artifacts"), vbox))->setAlignment(Qt::AlignCenter);
        lb_art_available = new Q3ListBox(vbox);
        lb_art_available->setSelectionMode(Q3ListBox::Multi);

        vbox = new Q3VBox(hbox);
        vbox->setMargin(5);
        (new QLabel("", vbox))->setScaledContents(TRUE);
        button = new QPushButton(vbox);
        button->setPixmap(*rightPixmap);
        connect(button, SIGNAL(clicked()), this, SLOT(associate_arts()));
        (new QLabel("", vbox))->setScaledContents(TRUE);
        button = new QPushButton(vbox);
        button->setPixmap(*leftPixmap);
        connect(button, SIGNAL(clicked()), this, SLOT(unassociate_arts()));
        (new QLabel("", vbox))->setScaledContents(TRUE);
        vbox = new Q3VBox(hbox);
    }
    else
        vbox = art_assoc_page;

    vbox->setMargin(5);
    (new QLabel(TR("Associated artifacts"), vbox))->setAlignment(Qt::AlignCenter);
    lb_art_associated = new Q3ListBox(vbox);
    lb_art_associated->setSelectionMode((visit) ? Q3ListBox::NoSelection
                                        : Q3ListBox::Multi);
    lb_art_initialized = FALSE;

    addTab(art_assoc_page, TR("Associated artifacts"));
}

void ArtifactDialog::default_description()
{
    comment->setText(GenerationSettings::default_artifact_description());
}

void ArtifactDialog::edit_description()
{
    edit(comment->text(), edname->text().stripWhiteSpace() + "_description",
         data, TxtEdit, this, (post_edit) post_edit_description, edits);
}

void ArtifactDialog::post_edit_description(ArtifactDialog * d, QString s)
{
    d->comment->setText(s);
}

void ArtifactDialog::edStereotypeActivated(const QString & ste)
{
    bool a_source = (ste.stripWhiteSpace() == "source");
    bool a_text = (ste.stripWhiteSpace() == "text");

    setTabEnabled(cpp_h_content_page, a_source && !a_text);
    setTabEnabled(cpp_src_content_page, a_source || a_text);
    setTabEnabled(java_content_page, a_source || a_text);
    setTabEnabled(php_content_page, a_source || a_text);
    setTabEnabled(python_content_page, a_source || a_text);
    setTabEnabled(idl_content_page, a_source || a_text);
    setTabEnabled(cl_assoc_page, a_source && !a_text);
    setTabEnabled(art_assoc_page, !a_source && !a_text);

    if (!a_source && !a_text) {
        if (!lb_art_initialized) {
            lb_art_initialized = TRUE;

            if (data->associated != 0)
                art_associated = *(data->associated);

            stereotypeFilterActivated(QString());
        }
    }
    else if (lb_cl_associated->count() != 0) {
        if (n_cpp == 0) {
            setTabEnabled(cpp_h_content_page, FALSE);
            setTabEnabled(cpp_src_content_page, FALSE);
        }

        if (n_java == 0)
            setTabEnabled(java_content_page, FALSE);

        if (n_php == 0)
            setTabEnabled(php_content_page, FALSE);

        if (n_python == 0)
            setTabEnabled(python_content_page, FALSE);

        if (n_idl == 0)
            setTabEnabled(idl_content_page, FALSE);
    }
}

void ArtifactDialog::update_tab(QWidget * w)
{
    bool visit = !hasOkButton();

    if (w == uml_page) {
        if (! visit)
            edname->setFocus();
    }
    else if (w == cpp_h_content_page) {
        cpp_update_h();

        if (! visit)
            edcpp_h_content->setFocus();
    }
    else if (w == cpp_src_content_page) {
        cpp_update_src();

        if (! visit)
            edcpp_src_content->setFocus();
    }
    else if (w == java_content_page) {
        java_update_src();

        if (! visit)
            edjava_content->setFocus();
    }
    else if (w == php_content_page) {
        php_update_src();

        if (! visit)
            edphp_content->setFocus();
    }
    else if (w == python_content_page) {
        python_update_src();

        if (! visit)
            edpython_content->setFocus();
    }
    else if (w == idl_content_page) {
        idl_update_src();

        if (! visit)
            edidl_content->setFocus();
    }
}


void ArtifactDialog::cpp_edit_h()
{
    edit(edcpp_h_content->text(),
         edname->text().stripWhiteSpace() + "_header", data,
         CppEdit, this, (post_edit) post_cpp_edit_h, edits);
}

void ArtifactDialog::post_cpp_edit_h(ArtifactDialog * d, QString s)
{
    d->edcpp_h_content->setText(s);
}

void ArtifactDialog::cpp_edit_src()
{
    edit(edcpp_src_content->text(),
         edname->text().stripWhiteSpace() + "_source", data,
         (edstereotype->currentText().stripWhiteSpace() == "text") ? TxtEdit : CppEdit,
         this, (post_edit) post_cpp_edit_src, edits);
}

void ArtifactDialog::post_cpp_edit_src(ArtifactDialog * d, QString s)
{
    d->edcpp_src_content->setText(s);
}

void ArtifactDialog::java_edit()
{
    edit(edjava_content->text(),
         edname->text().stripWhiteSpace() + "_source", data,
         (edstereotype->currentText().stripWhiteSpace() == "text") ? TxtEdit : JavaEdit,
         this, (post_edit) post_java_edit, edits);
}

void ArtifactDialog::post_java_edit(ArtifactDialog * d, QString s)
{
    d->edjava_content->setText(s);
}

void ArtifactDialog::php_edit()
{
    edit(edphp_content->text(),
         edname->text().stripWhiteSpace() + "_source", data,
         (edstereotype->currentText().stripWhiteSpace() == "text") ? TxtEdit : PhpEdit,
         this, (post_edit) post_php_edit, edits);
}

void ArtifactDialog::post_php_edit(ArtifactDialog * d, QString s)
{
    d->edphp_content->setText(s);
}

void ArtifactDialog::python_edit()
{
    edit(edpython_content->text(),
         edname->text().stripWhiteSpace() + "_source", data,
         (edstereotype->currentText().stripWhiteSpace() == "text") ? TxtEdit : PythonEdit,
         this, (post_edit) post_python_edit, edits);
}

void ArtifactDialog::post_python_edit(ArtifactDialog * d, QString s)
{
    d->edpython_content->setText(s);
}

void ArtifactDialog::idl_edit()
{
    edit(edidl_content->text(),
         edname->text().stripWhiteSpace() + "_source",
         data, TxtEdit, this, (post_edit) post_idl_edit, edits);
}

void ArtifactDialog::post_idl_edit(ArtifactDialog * d, QString s)
{
    d->edidl_content->setText(s);
}

uint ArtifactDialog::TypeID()
{
    return TypeIdentifier<ArtifactDialog>::id();
}

void ArtifactDialog::cpp_default_h()
{
    edcpp_h_content->setText((n_cpp != 0)
                             ? GenerationSettings::cpp_default_header_content()
                             : "");
    cpp_update_h();
}

void ArtifactDialog::cpp_default_src()
{
    edcpp_src_content->setText(((n_cpp != 0) &&
                                (edstereotype->currentText().stripWhiteSpace() != "text"))
                               ? GenerationSettings::cpp_default_source_content()
                               : "");
    cpp_update_src();
}

void ArtifactDialog::java_default_src()
{
    edjava_content->setText(((n_java != 0) &&
                             (edstereotype->currentText().stripWhiteSpace() != "text"))
                            ? GenerationSettings::java_default_source_content()
                            : "");
    java_update_src();
}

void ArtifactDialog::php_default_src()
{
    edphp_content->setText(((n_php != 0) &&
                            (edstereotype->currentText().stripWhiteSpace() != "text"))
                           ? GenerationSettings::php_default_source_content()
                           : "");
    php_update_src();
}

void ArtifactDialog::python_default_src()
{
    edpython_content->setText(((n_python != 0) &&
                               (edstereotype->currentText().stripWhiteSpace() != "text"))
                              ? GenerationSettings::python_default_source_content()
                              : "");
    python_update_src();
}

void ArtifactDialog::idl_default_src()
{
    edidl_content->setText(((n_idl != 0) &&
                            (edstereotype->currentText().stripWhiteSpace() != "text"))
                           ? GenerationSettings::idl_default_source_content()
                           : "");
    idl_update_src();
}

void ArtifactDialog::cpp_unmapped_h()
{
    edcpp_h_content->setText(QString());
    cpp_update_h();
}

void ArtifactDialog::cpp_unmapped_src()
{
    edcpp_src_content->setText(QString());
    cpp_update_src();
}

void ArtifactDialog::java_unmapped_src()
{
    edjava_content->setText(QString());
    java_update_src();
}

void ArtifactDialog::php_unmapped_src()
{
    edphp_content->setText(QString());
    php_update_src();
}

void ArtifactDialog::python_unmapped_src()
{
    edpython_content->setText(QString());
    python_update_src();
}

void ArtifactDialog::idl_unmapped_src()
{
    edidl_content->setText(QString());
    idl_update_src();
}

static void hide_result(QSplitter * spl)
{
    Q3HBox * vb = (Q3HBox *) spl->child("result");

    if (vb->isVisible()) {
        vb->hide();

        Q3ValueList<int> sz = spl->sizes();

        sz[0] = sz[0] + sz[1];
        sz[1] = 0;
        spl->setSizes(sz);
    }
}

static void show_result(QSplitter * spl)
{
    Q3HBox * vb = (Q3HBox *) spl->child("result");

    if (vb->isHidden()) {
        Q3ValueList<int> sz = spl->sizes();

        sz[1] = sz[0] = sz[0] / 2;
        spl->setSizes(sz);

        vb->show();
    }
}

void ArtifactDialog::compute_cpp_namespace(QString & nasp,
        QString & nasp_start,
        QString & nasp_end)
{
    const char * cnasp = (const char *)
                         ((PackageData *)
                          ((BrowserNode *) data->browser_node->parent()->parent())->get_data())
                         ->get_cpp_namespace();

    nasp = ((cnasp[0] == ':') && (cnasp[1] == ':'))
           ? cnasp + 2 : cnasp;

    if (! nasp.isEmpty()) {
        int index = 0;
        int index2;

        while ((index2 = nasp.find(':', index)) != -1) {
            nasp_start += QString("namespace ") +
                          nasp.mid(index, index2 - index) + " {\n\n";
            nasp_end += "\n}\n";
            nasp.replace(index2, 2, "_");
            index = index2 + 1;
        }

        nasp_start += QString("namespace ") + nasp.mid(index) + " {\n\n";
        nasp_end += "\n}\n";
    }
}

QString ArtifactDialog::compute_java_package()
{
    const QString pack = (const char *)
                         ((PackageData *)
                          ((BrowserNode *) data->browser_node->parent()->parent())->get_data())
                         ->get_java_package();

    return (pack.isEmpty())
           ? pack
           : QString("package ") + pack + ";\n";
}

QString ArtifactDialog::compute_php_namespace()
{
    const QString nasp = (const char *)
                         ((PackageData *)
                          ((BrowserNode *) data->browser_node->parent()->parent())->get_data())
                         ->get_php_namespace();

    return (nasp.isEmpty())
           ? nasp
           : QString("namespace ") + nasp + ";\n";
}

void ArtifactDialog::compute_idl_module(QString & mod,
                                        QString & mod_start,
                                        QString & mod_end)
{
    mod = (const char *)
          ((PackageData *)
           ((BrowserNode *) data->browser_node->parent()->parent())->get_data())
          ->get_idl_module();

    if (! mod.isEmpty()) {
        int index = 0;
        int index2;

        while ((index2 = mod.find(':', index)) != -1) {
            mod_start += QString("module ") +
                         mod.mid(index, index2 - index) + " {\n\n";
            mod_end += "\n};\n";
            mod.replace(index2, 2, "_");
            index = index2 + 1;
        }

        mod_start += QString("module ") + mod.mid(index) + " {\n\n";
        mod_end += "\n};\n";
    }
}

void ArtifactDialog::cpp_update_h()
{
    if (lb_cl_associated->count() == 0)
        hide_result(cpp_h_content_page);
    else {
        show_result(cpp_h_content_page);

        QString def = edcpp_h_content->text();
        const char * p = def;
        const char * pp = 0;
        QString s;
        QString nasp;
        QString nasp_start;
        QString nasp_end;

        compute_cpp_namespace(nasp, nasp_start, nasp_end);

        bool all_incl = (def.find("${all_includes}") != -1);

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


            if (*p == '@')
                manage_alias(data->browser_node, p, s, kvtable);
            else if (*p != '$')
                s += *p++;
            else if (!strncmp(p, "${comment}", 10))
                manage_comment(comment->text(), p, pp,
                               GenerationSettings::cpp_javadoc_style());
            else if (!strncmp(p, "${description}", 14))
                manage_description(comment->text(), p, pp);
            else if (!strncmp(p, "${name}", 7)) {
                p += 7;
                s += edname->text().stripWhiteSpace();
            }
            else if (!strncmp(p, "${Name}", 7)) {
                p += 7;
                s += capitalize(edname->text().stripWhiteSpace());
            }
            else if (!strncmp(p, "${NAME}", 7)) {
                p += 7;
                s += edname->text().stripWhiteSpace().upper();
            }
            else if (!strncmp(p, "${nAME}", 7)) {
                p += 7;
                s += edname->text().stripWhiteSpace().lower();
            }
            else if (!strncmp(p, "${namespace}", 12)) {
                p += 12;
                s += nasp;
            }
            else if (!strncmp(p, "${NAMESPACE}", 12)) {
                p += 12;
                s += nasp.upper();
            }
            else if (!strncmp(p, "${includes}", 11)) {
                p += 11;
                s += (all_incl)
                     ? "ERROR : ${includes} exclusive with ${all_includes}\n"
                     : "...#include \"a_needed_file.h\"   // produced by the C++ generator\n";
            }
            else if (!strncmp(p, "${all_includes}", 15)) {
                p += 15;
                s += "...#include \"a_needed_file.h\"   // produced by the C++ generator\n";
            }
            else if (!strncmp(p, "${declarations}", 15)) {
                p += 15;
                s += (all_incl)
                     ? "WARNING : ${declarations} is useless because of ${all_includes}\n"
                     : "...class a_referenced_class;    // produced by the C++ generator\n";
            }
            else if (!strncmp(p, "${namespace_start}", 18)) {
                p += 18;
                s += nasp_start;
            }
            else if (!strncmp(p, "${namespace_end}", 16)) {
                p += 16;
                s += nasp_end;
            }
            else if (!strncmp(p, "${definition}", 13)) {
                p += 13;

                if (*p == '\n')
                    p += 1;

                static QStringList nodes;	// must be static for FormalParamsTable
                QStringList node_names;
                unsigned n = lb_cl_associated->count();

                for (unsigned i = 0; i != n; i += 1) {
                    BrowserNode * bn =
                        ((ListBoxBrowserNode *) lb_cl_associated->item(i))->browser_node;
                    ClassData * c = (ClassData *) bn->get_data();

                    if (c->get_cppdecl()[0]) {
                        FormalParamsTable * f = formals[(BrowserClass *) bn];

                        if (f == 0) {
                            f = new FormalParamsTable(c, currentPage(), nodes, TRUE);
                            f->hide();
                            formals.insert((BrowserClass *) bn, f);
                        }

                        ActualParamsTable * a;

                        if (c->get_n_actualparams() == 0)
                            a = 0;
                        else if ((a = actuals[(BrowserClass *) bn]) == 0) {
                            a = new ActualParamsTable(c, currentPage(), nodes, TRUE);
                            a->hide();
                            actuals.insert((BrowserClass *) bn, a);
                        }

                        const AType & t = c->get_base_type();
                        BrowserNodeList nds;

                        ClassDialog::cpp_generate_decl(s, c, c->get_cppdecl(),
                                                       true_name(bn->get_name(),
                                                               c->get_cppdecl()),
                                                       c->get_stereotype(),
                                                       (t.type != 0) ? t.type->full_name(TRUE)
                                                       : QString((const char *) t.explicit_type),
                                                       bn->get_comment(), a, f,
                                                       nds, node_names, 0);
                        s += '\n';
                    }
                }
            }
            else
                // strange
                s += *p++;
        }

        showcpp_h_content->setText(s);
    }
}

void ArtifactDialog::cpp_update_src()
{
    if (lb_cl_associated->count() == 0)
        hide_result(cpp_src_content_page);
    else {
        show_result(cpp_src_content_page);

        QString def = edcpp_src_content->text();
        const char * p = def;
        const char * pp = 0;
        QString s;
        QString nasp;
        QString nasp_start;
        QString nasp_end;

        compute_cpp_namespace(nasp, nasp_start, nasp_end);

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

            if (*p == '@')
                manage_alias(data->browser_node, p, s, kvtable);
            else if (*p != '$')
                s += *p++;
            else if (!strncmp(p, "${comment}", 10))
                manage_comment(comment->text(), p, pp,
                               GenerationSettings::cpp_javadoc_style());
            else if (!strncmp(p, "${description}", 14))
                manage_description(comment->text(), p, pp);
            else if (!strncmp(p, "${includes}", 11)) {
                p += 11;
                s += "...#include \"a_needed_file.h\"   // produced by the C++ generator\n";
            }
            else if (!strncmp(p, "${name}", 7)) {
                p += 7;
                s += edname->text().stripWhiteSpace();
            }
            else if (!strncmp(p, "${Name}", 7)) {
                p += 7;
                s += capitalize(edname->text().stripWhiteSpace());
            }
            else if (!strncmp(p, "${NAME}", 7)) {
                p += 7;
                s += edname->text().stripWhiteSpace().upper();
            }
            else if (!strncmp(p, "${nAME}", 7)) {
                p += 7;
                s += edname->text().stripWhiteSpace().lower();
            }
            else if (!strncmp(p, "${namespace}", 12)) {
                p += 12;
                s += nasp;
            }
            else if (!strncmp(p, "${NAMESPACE}", 12)) {
                p += 12;
                s += nasp.upper();
            }
            else if (!strncmp(p, "${namespace_start}", 18)) {
                p += 18;
                s += nasp_start;
            }
            else if (!strncmp(p, "${members}", 10)) {
                p += 10;

                if (*p == '\n')
                    p += 1;

                unsigned n = lb_cl_associated->count();

                for (unsigned i = 0; i != n; i += 1)
                    ClassDialog::cpp_generate_members_def(((ListBoxBrowserNode *) lb_cl_associated->item(i))->browser_node,
                                                          s);
            }
            else if (!strncmp(p, "${namespace_end}", 16)) {
                p += 16;
                s += nasp_end;
            }
            else
                // strange
                s += *p++;
        }

        showcpp_src_content->setText(s);
    }
}

void ArtifactDialog::java_update_src()
{
    if (lb_cl_associated->count() == 0)
        hide_result(java_content_page);
    else {
        show_result(java_content_page);

        QString def = edjava_content->text();
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

            if (*p == '@')
                manage_alias(data->browser_node, p, s, kvtable);
            else if (*p != '$')
                s += *p++;
            else if (!strncmp(p, "${comment}", 10))
                manage_comment(comment->text(), p, pp,
                               GenerationSettings::java_javadoc_style());
            else if (!strncmp(p, "${description}", 14))
                manage_description(comment->text(), p, pp);
            else if (!strncmp(p, "${name}", 7)) {
                p += 7;
                s += edname->text().stripWhiteSpace();
            }
            else if (!strncmp(p, "${imports}", 10)) {
                p += 10;
                s += "...import a_needed_package   // produced by the Java generator\n";
            }
            else if (!strncmp(p, "${Name}", 7)) {
                p += 7;
                s += capitalize(edname->text().stripWhiteSpace());
            }
            else if (!strncmp(p, "${NAME}", 7)) {
                p += 7;
                s += edname->text().stripWhiteSpace().upper();
            }
            else if (!strncmp(p, "${nAME}", 7)) {
                p += 7;
                s += edname->text().stripWhiteSpace().lower();
            }
            else if (!strncmp(p, "${package}", 10)) {
                p += 10;
                s += compute_java_package();
            }
            else if (!strncmp(p, "${definition}", 13)) {
                p += 13;

                if (*p == '\n')
                    p += 1;

                static QStringList nodes;	// must be static for FormalParamsTable
                QStringList node_names;
                unsigned n = lb_cl_associated->count();

                for (unsigned i = 0; i != n; i += 1) {
                    BrowserNode * bn =
                        ((ListBoxBrowserNode *) lb_cl_associated->item(i))->browser_node;
                    ClassData * c = (ClassData *) bn->get_data();
                    FormalParamsTable * f = formals[(BrowserClass *) bn];

                    if (f == 0) {
                        f = new FormalParamsTable(c, currentPage(), nodes, TRUE);
                        f->hide();
                        formals.insert((BrowserClass *) bn, f);
                    }

                    ActualParamsTable * a;

                    if (c->get_n_actualparams() == 0)
                        a = 0;
                    else if ((a = actuals[(BrowserClass *) bn]) == 0) {
                        a = new ActualParamsTable(c, currentPage(), nodes, TRUE);
                        a->hide();
                        actuals.insert((BrowserClass *) bn, a);
                    }

                    BrowserNodeList nds;

                    ClassDialog::java_generate_decl(s, c, c->get_javadecl(), "", bn->get_name(),
                                                    c->get_stereotype(), bn->get_comment(),
                                                    c->get_uml_visibility(), c->java_is_final(),
                                                    c->get_is_abstract(), a, f, nds, node_names, 0);
                    s += '\n';
                }
            }
            else
                // strange
                s += *p++;
        }

        showjava_content->setText(s);
    }
}

void ArtifactDialog::php_update_src()
{
    if (lb_cl_associated->count() == 0)
        hide_result(php_content_page);
    else {
        show_result(php_content_page);

        QString def = edphp_content->text();
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

            if (*p == '@')
                manage_alias(data->browser_node, p, s, kvtable);
            else if (*p != '$')
                s += *p++;
            else if (!strncmp(p, "${comment}", 10))
                manage_comment(comment->text(), p, pp,
                               GenerationSettings::php_javadoc_style());
            else if (!strncmp(p, "${description}", 14))
                manage_description(comment->text(), p, pp);
            else if (!strncmp(p, "${name}", 7)) {
                p += 7;
                s += edname->text().stripWhiteSpace();
            }
            else if (!strncmp(p, "${Name}", 7)) {
                p += 7;
                s += capitalize(edname->text().stripWhiteSpace());
            }
            else if (!strncmp(p, "${NAME}", 7)) {
                p += 7;
                s += edname->text().stripWhiteSpace().upper();
            }
            else if (!strncmp(p, "${nAME}", 7)) {
                p += 7;
                s += edname->text().stripWhiteSpace().lower();
            }
            else if (!strncmp(p, "${namespace}", 12)) {
                p += 12;
                s += compute_php_namespace();
            }
            else if (!strncmp(p, "${require_once}", 15)) {
                p += 15;
                s += "...require_once 'a_needed_file.php'   // produced by the Php generator\n";
            }
            else if (!strncmp(p, "${definition}", 13)) {
                p += 13;

                if (*p == '\n')
                    p += 1;

                static QStringList nodes;	// must be static for FormalParamsTable
                QStringList node_names;
                unsigned n = lb_cl_associated->count();

                for (unsigned i = 0; i != n; i += 1) {
                    BrowserNode * bn =
                        ((ListBoxBrowserNode *) lb_cl_associated->item(i))->browser_node;
                    ClassData * c = (ClassData *) bn->get_data();

                    ClassDialog::php_generate_decl(s, c, c->get_phpdecl(), bn->get_name(),
                                                   c->get_stereotype(), bn->get_comment(),
                                                   c->php_is_final(), c->get_is_abstract(), 0);
                    s += '\n';
                }
            }
            else
                s += *p++;
        }

        showphp_content->setText(s);
    }
}

void ArtifactDialog::python_update_src()
{
    if (lb_cl_associated->count() == 0)
        hide_result(python_content_page);
    else {
        show_result(python_content_page);

        QString def = edpython_content->text();
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

            if (*p == '@')
                manage_alias(data->browser_node, p, s, kvtable);
            else if (*p != '$')
                s += *p++;
            else if (!strncmp(p, "${comment}", 10))
                manage_python_comment(comment->text(), p, pp);
            else if (!strncmp(p, "${description}", 14))
                manage_python_description(comment->text(), p, pp);
            else if (!strncmp(p, "${name}", 7)) {
                p += 7;
                s += edname->text().stripWhiteSpace();
            }
            else if (!strncmp(p, "${Name}", 7)) {
                p += 7;
                s += capitalize(edname->text().stripWhiteSpace());
            }
            else if (!strncmp(p, "${NAME}", 7)) {
                p += 7;
                s += edname->text().stripWhiteSpace().upper();
            }
            else if (!strncmp(p, "${nAME}", 7)) {
                p += 7;
                s += edname->text().stripWhiteSpace().lower();
            }
            else if (!strncmp(p, "${definition}", 13)) {
                p += 13;

                if (*p == '\n')
                    p += 1;

                BrowserNodeList nds;
                QStringList node_names;
                unsigned n = lb_cl_associated->count();

                for (unsigned i = 0; i != n; i += 1) {
                    BrowserNode * bn =
                        ((ListBoxBrowserNode *) lb_cl_associated->item(i))->browser_node;
                    ClassData * c = (ClassData *) bn->get_data();

                    ClassDialog::python_generate_decl(s, c, c->get_pythondecl(), bn->get_name(),
                                                      c->get_stereotype(), bn->get_comment(),
                                                      c->python_is_2_2(), nds, node_names, 0);
                    s += '\n';
                }
            }
            else if (!strncmp(p, "${import}", 9)) {
                p += 9;
                s += "...import    // produced by the Python generator\n";
            }
            else
                s += *p++;
        }

        showpython_content->setText(s);
    }
}

void ArtifactDialog::idl_update_src()
{
    if (lb_cl_associated->count() == 0)
        hide_result(idl_content_page);
    else {
        show_result(idl_content_page);

        QString def = edidl_content->text();
        const char * p = def;
        const char * pp = 0;
        QString s;
        QString mod;
        QString mod_start;
        QString mod_end;

        compute_idl_module(mod, mod_start, mod_end);

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


            if (*p == '@')
                manage_alias(data->browser_node, p, s, kvtable);
            else if (*p != '$')
                s += *p++;
            else if (!strncmp(p, "${comment}", 10))
                manage_comment(comment->text(), p, pp, FALSE);
            else if (!strncmp(p, "${description}", 14))
                manage_description(comment->text(), p, pp);
            else if (!strncmp(p, "${name}", 7)) {
                p += 7;
                s += edname->text().stripWhiteSpace();
            }
            else if (!strncmp(p, "${Name}", 7)) {
                p += 7;
                s += capitalize(edname->text().stripWhiteSpace());
            }
            else if (!strncmp(p, "${NAME}", 7)) {
                p += 7;
                s += edname->text().stripWhiteSpace().upper();
            }
            else if (!strncmp(p, "${nAME}", 7)) {
                p += 7;
                s += edname->text().stripWhiteSpace().lower();
            }
            else if (!strncmp(p, "${module}", 9)) {
                p += 9;
                s += mod;
            }
            else if (!strncmp(p, "${MODULE}", 9)) {
                p += 9;
                s += mod.upper();
            }
            else if (!strncmp(p, "${includes}", 11)) {
                p += 11;
                s += "...#include \"a_needed_file.h\"   // produced by the IDL generator\n";
            }
            else if (!strncmp(p, "${module_start}", 15)) {
                p += 15;
                s += mod_start;
            }
            else if (!strncmp(p, "${definition}", 13)) {
                p += 13;

                if (*p == '\n')
                    p += 1;

                unsigned n = lb_cl_associated->count();

                for (unsigned i = 0; i != n; i += 1) {
                    BrowserNode * bn =
                        ((ListBoxBrowserNode *) lb_cl_associated->item(i))->browser_node;
                    ClassData * c = (ClassData *) bn->get_data();

                    ClassDialog::idl_generate_decl(s, c, c->get_idldecl(), bn->get_name(),
                                                   c->get_stereotype(),
                                                   c->get_base_type().get_type(),
                                                   bn->get_comment(), c->get_switch_type().get_type(),
                                                   c->idl_is_local(), c->idl_is_custom(),
                                                   c->get_is_abstract(), 0);
                    s += '\n';
                }
            }
            else if (!strncmp(p, "${module_end}", 13)) {
                p += 13;
                s += mod_end;

                if (*p == '\n')
                    p += 1;
            }
            else
                // strange
                s += *p++;
        }

        showidl_content->setText(s);
    }
}

void ArtifactDialog::associate_cls()
{
    unsigned int i = 0;

    while (i < lb_cl_available->count()) {
        Q3ListBoxItem * item = lb_cl_available->item(i);

        if (item->selected()) {
            lb_cl_available->takeItem(item);
            lb_cl_associated->insertItem(item);

            ClassData * c = (ClassData *)
                            ((ListBoxBrowserNode *) item)->browser_node->get_data();

            if (c->get_cppdecl()[0])
                n_cpp += 1;

            if (c->get_javadecl()[0])
                n_java += 1;

            if (c->get_phpdecl()[0])
                n_php += 1;

            if (c->get_pythondecl()[0])
                n_python += 1;

            if (c->get_idldecl()[0])
                n_idl += 1;
        }
        else
            i += 1;
    }
}

void ArtifactDialog::unassociate_cls()
{
    unsigned int i = 0;

    while (i < lb_cl_associated->count()) {
        Q3ListBoxItem * item = lb_cl_associated->item(i);

        if (item->selected()) {
            lb_cl_associated->takeItem(item);
            lb_cl_available->insertItem(item);

            ClassData * c = (ClassData *)
                            ((ListBoxBrowserNode *) item)->browser_node->get_data();

            if (c->get_cppdecl()[0])
                n_cpp -= 1;

            if (c->get_javadecl()[0])
                n_java -= 1;

            if (c->get_phpdecl()[0])
                n_php -= 1;

            if (c->get_pythondecl()[0])
                n_python -= 1;

            if (c->get_idldecl()[0])
                n_idl -= 1;
        }
        else
            i += 1;
    }

    lb_cl_available->sort();
}

void ArtifactDialog::move_cls_up()
{
    unsigned n = lb_cl_associated->count();

    if ((n > 1) && !lb_cl_associated->item(0)->selected()) {
        for (unsigned i = 1; i != n; i += 1) {
            Q3ListBoxItem * item = lb_cl_associated->item(i);

            if (item->selected()) {
                lb_cl_associated->takeItem(item);
                lb_cl_associated->insertItem(item, i - 1);
            }
        }
    }
}

void ArtifactDialog::move_cls_down()
{
    int n = (int) lb_cl_associated->count();

    if ((n > 1) && !lb_cl_associated->item(n - 1)->selected()) {
        for (int i = n - 2; i >= 0; i -= 1) {
            if (lb_cl_associated->item(i)->selected()) {
                Q3ListBoxItem * item = lb_cl_associated->item(i + 1);

                lb_cl_associated->takeItem(item);
                lb_cl_associated->insertItem(item, i);
            }
        }
    }
}

void ArtifactDialog::associate_arts()
{
    unsigned int i = 0;

    while (i < lb_art_available->count()) {
        Q3ListBoxItem * item = lb_art_available->item(i);

        if (item->selected()) {
            lb_art_available->takeItem(item);
            lb_art_associated->insertItem(item);

            BrowserArtifact * c = (BrowserArtifact *)
                                  ((ListBoxBrowserNode *) item)->browser_node;

            art_associated.insert(c, c);
        }
        else
            i += 1;
    }

    lb_art_associated->sort();
}

void ArtifactDialog::unassociate_arts()
{
    unsigned int i = 0;

    while (i < lb_art_associated->count()) {
        Q3ListBoxItem * item = lb_art_associated->item(i);

        if (item->selected()) {
            lb_art_associated->takeItem(item);
            lb_art_available->insertItem(item);
            art_associated.remove(((ListBoxBrowserNode *) item)->browser_node);
        }
        else
            i += 1;
    }

    lb_art_available->sort();
}

void ArtifactDialog::stereotypeFilterActivated(const QString & st)
{
    if (!hasOkButton()) {
        Q3PtrDictIterator<BrowserArtifact> it(art_associated);

        while (it.current()) {
            lb_art_associated->insertItem(it.current()->full_name(TRUE));
            ++it;
        }

        lb_art_associated->sort();
    }
    else {
        QString s = st.stripWhiteSpace();
        BrowserNodeList artifacts;
        BrowserNode * itself = data->browser_node;

        BrowserArtifact::instances(artifacts, s);
        lb_art_available->clear();
        lb_art_associated->clear();

        foreach (BrowserNode *cp, artifacts) {
            if (cp != itself) {
                if (art_associated.find(cp))
                    lb_art_associated->insertItem(new ListBoxBrowserNode(cp, cp->full_name(TRUE)));
                else
                    lb_art_available->insertItem(new ListBoxBrowserNode(cp, cp->full_name(TRUE)));
            }
        }

        lb_art_available->sort();
        lb_art_associated->sort();
    }
}

void ArtifactDialog::accept()
{
    if (!check_edits(edits) || !kvtable->check_unique())
        return;

    QString s = edname->text().stripWhiteSpace();
    BrowserArtifact * bn = (BrowserArtifact *) data->get_browser_node();

    if ((s != bn->get_name()) &&
        ((BrowserNode *) bn->parent())->wrong_child_name(s, UmlArtifact,
                bn->allow_spaces(),
                bn->allow_empty()))
        msg_critical(TR("Error"), edname->text() + TR("\n\nillegal name or already used"));
    else {
        bn->set_name(s);
        bn->set_comment(comment->text());
        UmlWindow::update_comment_if_needed(bn);

        QString stereotype =
            fromUnicode(edstereotype->currentText().stripWhiteSpace());
        bool a_text = (stereotype == "text");
        bool newst = data->set_stereotype(stereotype);

        if (a_text || (stereotype == "source")) {
            Q3ValueList<BrowserClass *> l;

            if (! a_text) {
                unsigned n = lb_cl_associated->count();

                for (unsigned i = 0; i != n; i += 1)
                    l.append((BrowserClass *)
                             (((ListBoxBrowserNode *) lb_cl_associated->item(i))
                              ->browser_node));
            }

            bn->set_associated_classes(l);

            if (isTabEnabled(cpp_h_content_page)) {
                data->cpp_h = edcpp_h_content->text();
            }
            else
                data->cpp_h = QString();

            if (isTabEnabled(cpp_src_content_page)) {
                data->cpp_src = edcpp_src_content->text();
            }
            else
                data->cpp_src = QString();

            if (isTabEnabled(java_content_page)) {
                data->java_src = edjava_content->text();
            }
            else
                data->java_src = QString();

            if (isTabEnabled(php_content_page)) {
                data->php_src = edphp_content->text();
            }
            else
                data->php_src = QString();

            if (isTabEnabled(python_content_page)) {
                data->python_src = edpython_content->text();
            }
            else
                data->python_src = QString();

            if (isTabEnabled(idl_content_page)) {
                data->idl_src = edidl_content->text();
            }
            else
                data->idl_src = QString();
        }
        else
            data->update_associated(art_associated);

        kvtable->updateNodeFromThis(bn);

        ProfiledStereotypes::modified(bn, newst);

        bn->modified();
        bn->package_modified();
        data->modified();

        BrowserArtifact * bn = (BrowserArtifact *) data->get_browser_node();
        bn->edit_end();
        toolbar->setParent(0);
        toolbar->hide();
        Q3TabDialog::accept();
    }
}

void ArtifactDialog::reject()
{
    BrowserArtifact * bn = (BrowserArtifact *) data->get_browser_node();
    bn->edit_end();
    toolbar->setParent(0);
    toolbar->hide();
    Q3TabDialog::accept();
}
ArtifactDialog* ArtifactDialog::Instance(ArtifactData * nd)
{
    //since the dialog is not yet fixed to be called multiple times we do recreation
    //if(instance.isNull())
    instance.reset(new ArtifactDialog(nd));
    //else
    //instance->FillGuiElements(cl);
    return instance.data();
}
