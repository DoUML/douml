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

#include <qcursor.h>
#include <q3grid.h>
#include <q3vbox.h>
#include <qlabel.h>
#include <q3combobox.h>
#include <q3buttongroup.h>
#include <qcheckbox.h>
#include <qradiobutton.h>
#include <q3popupmenu.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qtimer.h>
//Added by qt3to4:
#include <Q3HBoxLayout>
#include <QDesktopWidget>
#include <QHBoxLayout>
#include <Q3VBoxLayout>

#include <QRegExp>
#include <functional>

#include "OperationDialog.h"
#include "OperationData.h"
#include "ParamData.h"
#include "BrowserOperation.h"
#include "ClassData.h"
#include "ClassDialog.h"
#include "BrowserAttribute.h"
#include "AttributeData.h"
#include "BrowserRelation.h"
#include "RelationData.h"
#include "BrowserView.h"
#include "UmlDesktop.h"
#include "BrowserClass.h"
#include "KeyValueTable.h"
#include "ComboItem.h"
#include "myio.h"
#include "UmlWindow.h"
#include "DialogUtil.h"
#include "GenerationSettings.h"
#include "BrowserView.h"
#include "strutil.h"
#include "BodyDialog.h"
#include "AnnotationDialog.h"
#include "DecoratorDialog.h"
#include "Tool.h"
#include "ProfiledStereotypes.h"
#include "translate.h"
#include "misc/TypeIdentifier.h"
#include "ui/operationwidgetcpp.h"
#include "ui_operationwidgetcpp.h"
#include "Tools/SignalBlockerWrapper.h"
#include "CustomWidgets/EdgeMenuToolBar.h"
#include "browserfunctions/operationfuncs.h"

QSize OperationDialog::previous_size;
QSharedPointer<OperationDialog> OperationDialog::instance;
OperationDialog::OperationDialog(OperationData * o, DrawingLanguage )
    : EdgeMenuDialog(0, 0, FALSE), oper(o),
      cl((ClassData *)((BrowserClass *) o->browser_node->parent())->get_data())
{
    //o->browser_node->edit_start();

    SetDialogMode(o->browser_node->is_writable());
    //isWritable = hasOkButton();
    setCaption(TR("Operation dialog"));
    SetCurrentNode(o->browser_node);
    //setWindowFlags(!Qt::WindowTitleHint);
    InitGui();
    FillGuiElements(oper);



    connect(this, SIGNAL(currentChanged(QWidget *)),
            this, SLOT(update_all_tabs(QWidget *)));
    open_dialog(this);
}

OperationDialog::~OperationDialog()
{
//    if(oper->browser_node)
//        oper->browser_node->edit_end();
    previous_size = size();

    while (!edits.isEmpty())
        edits.take(0)->close();

    if(toolbar)
    {
        toolbar->setParent(0);
        toolbar->hide();
    }

    //close_dialog(this);
}

void OperationDialog::polish()
{
    Q3TabDialog::polish();
    UmlDesktop::limitsize_center(this, previous_size, 0.8, 0.8);
}

// to manage get_ set_, search corresp attr/rel
void OperationDialog::init_get_set()
{

    nodes.clear();
    list.clear();

    BrowserClass::instances(nodes);
    nodes.full_names(list);

    get_of_attr = 0;
    set_of_attr = 0;
    get_of_rel = 0;
    set_of_rel = 0;

    if (oper->is_get_or_set)
    {
        BrowserOperation * br_op = (BrowserOperation *) oper->browser_node;
        Q3ListViewItem * child;

        for (child = br_op->parent()->firstChild();
             child != 0;
             child = child->nextSibling()) {
            BrowserNode * br = (BrowserNode *) child;

            if (!br->deletedp()) {
                if (br->get_type() == UmlAttribute)
                {
                    if (((BrowserAttribute *) br)->get_get_oper() == br_op) {
                        get_of_attr = (AttributeData *) br->get_data();
                        break;
                    }

                    if (((BrowserAttribute *) br)->get_set_oper() == br_op) {
                        set_of_attr = (AttributeData *) br->get_data();
                        break;
                    }
                }
                else if (IsaRelation(br->get_type())) {
                    if (((BrowserRelation *) br)->get_get_oper() == br_op) {
                        get_of_rel = (RelationData *) br->get_data();
                        is_rel_a = get_of_rel->is_a((BrowserRelation *) br);
                        break;
                    }

                    if (((BrowserRelation *) br)->get_set_oper() == br_op) {
                        set_of_rel = (RelationData *) br->get_data();
                        is_rel_a = set_of_rel->is_a((BrowserRelation *) br);
                        break;
                    }
                }
            }
        }
    }
}

// general tab
void OperationDialog::init_uml()
{
    umlGrid = new Q3Grid(2, this);
    umltab = umlGrid;
    RegisterTab("Uml", umltab);
    umlGrid->setSpacing(5);
    umlGrid->setMargin(5);

    new QLabel(TR("class : "), umlGrid);
    lblFullClassName = new QLabel(((BrowserNode *) oper->get_browser_node()->parent())->full_name(TRUE),
                                  umlGrid);
    new QLabel(TR("name : "), umlGrid);
    edname = new LineEdit(oper->name(), umlGrid);


    new QLabel(TR("stereotype : "), umlGrid);
    edstereotype = new Q3ComboBox(isWritable, umlGrid);

    pbValueType = new SmallPushButton(TR("value type :"), umlGrid);
    connect(pbValueType, SIGNAL(clicked()), this, SLOT(menu_returntype()));


    edreturn_type = new Q3ComboBox(isWritable, umlGrid);

    new QLabel(umlGrid);

    htabBgUml = new Q3HBox(umlGrid);
    bgUml1 = uml_visibility.init(htabBgUml, oper->get_uml_visibility(), TRUE);

    htabBgUml->setStretchFactor(bgUml1, 1000);

    htabBgUml->setStretchFactor(new QLabel("  ", htabBgUml), 0);

    bgUml2 = new Q3ButtonGroup(2, Qt::Horizontal, QString(), htabBgUml);

    htabBgUml->setStretchFactor(bgUml2, 1000);
    classoperation_cb = new QCheckBox(TR("static"), bgUml2);
    abstract_cb = new QCheckBox(TR("abstract"), bgUml2);

    connect(classoperation_cb, SIGNAL(toggled(bool)), SLOT(classoper_toggled(bool)));
    connect(abstract_cb, SIGNAL(toggled(bool)), SLOT(abstract_toggled(bool)));

    htabBgUml->setStretchFactor(new QLabel("  ", htabBgUml), 0);

    bgUml3 = new Q3ButtonGroup(1, Qt::Horizontal, QString(), htabBgUml);
    htabBgUml->setStretchFactor(bgUml3, 1000);
    forcegenbody_cb = new QCheckBox(TR("force body generation"), bgUml3);


    connect(forcegenbody_cb, SIGNAL(toggled(bool)), SLOT(forcegenbody_toggled(bool)));

    new QLabel(TR("parameters : "), umlGrid);
    table = new ParamsTable(oper, umlGrid, list, this, !isWritable);

    new QLabel(TR("exceptions : "), umlGrid);
    etable = new ExceptionsTable(oper, umlGrid, list, !isWritable); //todo update the table

    Q3VBox * vtab = new Q3VBox(umlGrid);

    new QLabel(TR("description :"), vtab);
    pbEditor = new SmallPushButton(TR("Editor"), vtab);
    pbDefault = new SmallPushButton(TR("Default"), vtab);

    connect(pbEditor, SIGNAL(clicked()), this, SLOT(edit_description()));
    connect(pbDefault, SIGNAL(clicked()), this, SLOT(default_description()));

    comment = new MultiLineEdit(umlGrid);

    vtab = new Q3VBox(umlGrid);
    new QLabel(TR("constraint :"), vtab);
    pbConstraintEditor = new SmallPushButton(TR("Editor"), vtab);
    connect(pbConstraintEditor, SIGNAL(clicked()), this, SLOT(edit_constraint()));

    constraint = new MultiLineEdit(umlGrid);

    addTab(umlGrid, "Uml");
}


void OperationDialog::FillUmlTab(OperationData * oper)
{

    BrowserNode * node = (BrowserNode *) oper->get_browser_node();
    lblFullClassName->setText(((BrowserNode *) oper->get_browser_node()->parent())->full_name(TRUE));
    QString operationName = node->get_name();
    edname->setText(operationName);
    edname->setReadOnly(!isWritable);
    edstereotype->clear();
    edstereotype->insertItem(toUnicode(oper->stereotype));

    if (oper->is_get_or_set) {
        edstereotype->setEnabled(FALSE);
    }
    else if (isWritable) {
        edstereotype->setEnabled(true);
        edstereotype->insertStringList(BrowserOperation::default_stereotypes());
        edstereotype->insertStringList(ProfiledStereotypes::defaults(UmlOperation));
        edstereotype->setAutoCompletion(completion());
    }

    edstereotype->setCurrentItem(0);
    QSizePolicy sp = edstereotype->sizePolicy();
    sp.setHorData(QSizePolicy::Expanding);
    edstereotype->setSizePolicy(sp);

    if (oper->is_get_or_set)
        pbValueType->setEnabled(false);
    else
        pbValueType->setEnabled(true);


    edreturn_type->clear();
    edreturn_type->insertItem(oper->get_return_type().get_full_type());

    if (oper->is_get_or_set)
        edreturn_type->setEnabled(FALSE);
    else if (isWritable) {
        edreturn_type->setEnabled(true);
        QStringList l = GenerationSettings::basic_types();

        cl->addFormals(l);
        edreturn_type->insertStringList(l);
        edreturn_type_offset = edreturn_type->count();
        edreturn_type->insertStringList(list);
        edreturn_type->setAutoCompletion(completion());
        view = oper->browser_node->container(UmlClass);
    }

    edreturn_type->setCurrentItem(0);

    edreturn_type->setSizePolicy(sp);

    bgUml1 = uml_visibility.init(htabBgUml, oper->get_uml_visibility(), TRUE);
    bgUml1->setEnabled(isWritable);
    bgUml2->setEnabled(isWritable);

    classoperation_cb->setDisabled(!isWritable);
    classoperation_cb->setChecked(false);
    abstract_cb->setDisabled(!isWritable);
    abstract_cb->setChecked(false);


    if (oper->get_isa_class_operation())
        classoperation_cb->setChecked(TRUE);
    else
        classoperation_cb->setChecked(false);

    if (oper->get_is_abstract())
        abstract_cb->setChecked(TRUE);
    else
        abstract_cb->setChecked(false);

    if (oper->is_get_or_set)
        bgUml2->setEnabled(FALSE);
    else
        bgUml2->setEnabled(true);

    forcegenbody_cb->setDisabled(!isWritable);

    if (oper->body_generation_forced())
        forcegenbody_cb->setChecked(TRUE);
    else
        forcegenbody_cb->setChecked(false);

    init_get_set();
    table->update(oper, list, this, !isWritable);
    if (oper->is_get_or_set)
        table->setEnabled(FALSE);
    else
        table->setEnabled(true);

    etable->Reinitialize(oper,list, isWritable);

//    BrowserNodeList inh;
//    cl->get_browser_node()->children(inh, UmlGeneralisation, UmlRealize);
//    etable->update(oper,inh);
    pbEditor->setEnabled(isWritable);
    pbDefault->setEnabled(isWritable);
    comment->setText(oper->get_browser_node()->get_comment());
    QFont font = comment->font();

    if (! hasCodec())
        font.setFamily("Courier");

    font.setFixedPitch(TRUE);
    comment->setFont(font);
    comment->setReadOnly(!isWritable);

    pbConstraintEditor->setEnabled(isWritable);
    constraint->setReadOnly(!isWritable);
    constraint->setText(oper->constraint);
    constraint->setFont(font);

}


// C++
void OperationDialog::init_cpp()
{

    cppTab = new OperationWidgetCpp();

    RegisterTab("C++", cppTab);

    tabBgCppModifiers = new Q3HBox();
    tabBgCppModifiers->setStretchFactor(new QLabel("      ", tabBgCppModifiers), 0);


    visibilityBg = cpp_visibility.init(tabBgCppModifiers, oper->get_cpp_visibility(), FALSE, 0, TR("follow uml")); // update this
    bgCppModifiers = new Q3ButtonGroup(5, Qt::Horizontal, QString(), tabBgCppModifiers);
    cbCppConst = new QCheckBox("const", bgCppModifiers);
    cbCppVolatile = new QCheckBox("volatile", bgCppModifiers);
    cbCppFriend = new QCheckBox("friend", bgCppModifiers);
    cbCppVirtual = new QCheckBox("virtual", bgCppModifiers);
    cbCppInline = new QCheckBox("inline", bgCppModifiers);
    QHBoxLayout * layout = new QHBoxLayout();
    layout->addWidget(tabBgCppModifiers);
    layout->setContentsMargins(0, 0, 0, 0);
    cppTab->ui->wdgVisibilityGroup->setLayout(layout);


    connect(cbCppConst, SIGNAL(toggled(bool)),     SLOT(const_volatile_toggled(bool)));
    connect(cbCppVolatile, SIGNAL(toggled(bool)),  SLOT(const_volatile_toggled(bool)));
    connect(cbCppFriend, SIGNAL(toggled(bool)),    SLOT(friend_toggled(bool)));
    connect(cbCppVirtual, SIGNAL(toggled(bool)),   SLOT(virtual_toggled(bool)));
    connect(cbCppInline, SIGNAL(toggled(bool)),    SLOT(inline_toggled(bool)));
    connect(cppTab->ui->cbCppDefaulted, SIGNAL(toggled(bool)),    SLOT(default_toggled(bool)));
    connect(cppTab->ui->cbCppDeleted, SIGNAL(toggled(bool)),    SLOT(delete_toggled(bool)));
    connect(cppTab->ui->cbCppFinal, SIGNAL(toggled(bool)),    SLOT(final_toggled(bool)));
    connect(cppTab->ui->cbCppOverride, SIGNAL(toggled(bool)),    SLOT(override_toggled(bool)));
    connect(cppTab->ui->leCppNamespec, SIGNAL(textChanged(const QString &)), this, SLOT(cpp_update_decl()));
    connect(cppTab->ui->leCppNamespec, SIGNAL(textChanged(const QString &)), this, SLOT(cpp_update_def()));
    connect(cppTab->ui->pbDefaultDeclaration, SIGNAL(clicked()), this, SLOT(cpp_default_decl()));
    connect(cppTab->ui->pbFromDefinition, SIGNAL(clicked()), this, SLOT(cpp_decl_from_def()));
    connect(cppTab->ui->pbNotGeneratedInCpp, SIGNAL(clicked()), this, SLOT(cpp_unmapped_decl()));
    connect(cppTab->ui->pbEditParameters, SIGNAL(clicked()), this, SLOT(cpp_edit_param_decl()));
    connect(cppTab->ui->editcppbody, SIGNAL(clicked()), this, SLOT(cpp_edit_body()));
    connect(cppTab->ui->pbDefaultDefinition, SIGNAL(clicked()), this, SLOT(cpp_default_def()));
    connect(cppTab->ui->pbFromDeclaration, SIGNAL(clicked()), this, SLOT(cpp_def_from_decl()));
    connect(cppTab->ui->pb2NotGeneratedInCpp, SIGNAL(clicked()), this, SLOT(cpp_unmapped_def()));
    connect(cppTab->ui->pb2EditParameters, SIGNAL(clicked()), this, SLOT(cpp_edit_param_def()));

    char * b = oper->get_body('c');

    if (b != 0) {
        cppbody = oldcppbody = b;
        delete [] b;
    }

    insertTab(cppTab, "C++", 1);
}


void OperationDialog::FillcppTab(OperationData * oper)
{
    cppTab->ui->cbCppFrozen->setChecked(false);

    if (!isWritable || !oper->is_get_or_set)
        cppTab->ui->cbCppFrozen->hide();
    else {
        if (oper->cpp_get_set_frozen)
            cppTab->ui->cbCppFrozen->setChecked(TRUE);
    }

    visibilityBg = cpp_visibility.init(tabBgCppModifiers, oper->get_cpp_visibility(), FALSE, 0, TR("follow uml")); // update this
    visibilityBg->setEnabled(isWritable);

    SilentCall(cbCppConst)->setChecked(oper->get_cpp_const());
    SilentCall(cbCppVolatile)->setChecked(oper->get_is_volatile());
    SilentCall(cbCppFriend)->setChecked(oper->get_cpp_friend());
    SilentCall(cbCppVirtual)->setChecked(oper->get_cpp_virtual());
    SilentCall(cbCppInline)->setChecked(oper->get_cpp_inline());
    SilentCall(cppTab->ui->cbCppDefaulted)->setChecked(oper->get_cpp_default());
    SilentCall(cppTab->ui->cbCppDeleted)->setChecked(oper->get_cpp_delete());
    SilentCall(cppTab->ui->cbCppOverride)->setChecked(oper->get_cpp_override());
    SilentCall(cppTab->ui->cbCppFinal)->setChecked(oper->get_cpp_final());


    cbCppConst->setDisabled(!isWritable);
    cbCppVolatile->setDisabled(!isWritable);
    cbCppVirtual->setDisabled(!isWritable);
    cbCppFriend->setDisabled(!isWritable);
    cbCppInline->setDisabled(!isWritable);


    if (oper->is_get_or_set) {
        cppTab->ui->leCppNamespec->setText(oper->cpp_name_spec);
        cppTab->ui->leCppNamespec->setReadOnly(!isWritable);
    }
    else {
        cppTab->ui->lblNameFormCpp->hide();
        cppTab->ui->leCppNamespec->hide();
    }

    if (isWritable) {
        connect(cppTab->ui->edCppDeclProto, SIGNAL(textChanged()), this, SLOT(cpp_update_decl()));
        connect(cppTab->ui->edCppDefProto, SIGNAL(textChanged()), this, SLOT(cpp_update_def()));
    }
    else {
        disconnect(cppTab->ui->edCppDeclProto, SIGNAL(textChanged()), this, SLOT(cpp_update_decl()));
        disconnect(cppTab->ui->edCppDefProto, SIGNAL(textChanged()), this, SLOT(cpp_update_def()));
    }

    cppTab->ui->edCppDeclProto->setText(oper->get_cppdecl());
    cppTab->ui->edCppDeclProto->setFont(comment->font());
    cppTab->ui->edCppDeclProto->setReadOnly(!isWritable);

    cppTab->ui->edCppDeclActual->setReadOnly(TRUE);
    cppTab->ui->edCppDeclActual->setFont(comment->font());

    cppTab->ui->pbDefaultDeclaration->setEnabled(isWritable);
    cppTab->ui->pbFromDefinition->setEnabled(isWritable);
    cppTab->ui->pbNotGeneratedInCpp->setEnabled(isWritable && !oper->is_get_or_set);
    cppTab->ui->pbEditParameters->setEnabled(isWritable);

//    if(oper->get_is_abstract())
//        cpp_default_def();
//    else
    cppTab->ui->edCppDefProto->setText(oper->get_cppdef());
    cppTab->ui->edCppDefProto->setFont(comment->font());
    cppTab->ui->edCppDefProto->setReadOnly(!isWritable);

    cppTab->ui->cbIndentCppBody->setVisible(isWritable && !oper->is_get_or_set);
    cppTab->ui->cbIndentCppBody->setEnabled(!(preserve_bodies() && !forcegenbody_cb->isChecked()));
    cppTab->ui->cbIndentCppBody->setChecked((preserve_bodies() && !forcegenbody_cb->isChecked()) && oper->cpp_indent_body);

    cppTab->ui->edCppDefActual->setReadOnly(TRUE);
    cppTab->ui->edCppDefActual->setFont(comment->font());

    bool bodyEditable = (!isWritable || (preserve_bodies() && !forcegenbody_cb->isChecked()));
    QString buttonText = bodyEditable ? TR("Show body") : TR("Edit body");
    cppTab->ui->editcppbody->setText(buttonText);

    char * b = oper->get_body('c');

    if (b != 0) {
        cppbody = oldcppbody = b;
        delete [] b;
    }

    //cppTab->ui->edCppDefProto
    cppTab->ui->pbDefaultDefinition->setEnabled(isWritable);
    cppTab->ui->pbFromDeclaration->setEnabled(isWritable);
    cppTab->ui->pb2NotGeneratedInCpp->setEnabled(isWritable && !oper->is_get_or_set);
    cppTab->ui->pb2EditParameters->setEnabled(isWritable);

    if (!GenerationSettings::cpp_get_default_defs())
        HideTab("Cpp");
    else
        ShowTab("Cpp");

    cl->get_class_spec(templates, cl_names, templates_tmplop, cl_names_tmplop);

    cbCppInline->setDisabled(!templates.isEmpty());


    if (cpp_undef)
        HideTab("Cpp");
}

// Java
void OperationDialog::init_java()
{

    Q3Grid * grid;
    Q3HBox * htab = 0;
    Q3ButtonGroup * bg;

    grid = new Q3Grid(2, this);
    javatab = grid;
    RegisterTab("Java", javatab);
    grid->setMargin(5);
    grid->setSpacing(5);

    javafrozen_cb = new QCheckBox(TR("frozen"), grid);

    bg = new Q3ButtonGroup(2, Qt::Horizontal, QString(), grid);

    javafinal_cb = new QCheckBox("final", bg);

    connect(javafinal_cb, SIGNAL(toggled(bool)), SLOT(java_finalsynchronized_toggled(bool)));

    synchronized_cb = new QCheckBox("synchronized", bg);

    connect(synchronized_cb, SIGNAL(toggled(bool)), SLOT(java_finalsynchronized_toggled(bool)));

    lblNameFormJava = new QLabel(TR("Name form : "), grid);
    namespecTab = new Q3HBox(grid);
    edjavanamespec = new LineEdit(htab);


    new QLabel(TR("Definition :"), grid);
    edjavadef = new MultiLineEdit(grid);
    connect(edjavadef, SIGNAL(textChanged()), this, SLOT(java_update_def()));

    Q3VBox * vtab = new Q3VBox(grid);

    new QLabel(TR("Result after\nsubstitution : "), vtab);

    indentjavabody_cb = new QCheckBox(TR("contextual\nbody indent"), vtab);

    showjavadef = new MultiLineEdit(grid);

    editjavabody = new QPushButton("",  grid);
    connect(editjavabody, SIGNAL(clicked()), this, SLOT(java_edit_body()));

    char * b = oper->get_body('j');

    if (b != 0) {
        javabody = oldjavabody = b;
        delete [] b;
    }

    htab = new Q3HBox(grid);

    pbDefaultDeclarationJava = new QPushButton(TR("Default declaration"), htab);
    pbNotGeneratedInJava = new QPushButton(TR("Not generated in Java"), htab);
    connect(pbDefaultDeclarationJava, SIGNAL(clicked()), this, SLOT(java_default_def()));
    connect(pbNotGeneratedInJava, SIGNAL(clicked()), this, SLOT(java_unmapped_def()));

    editjavaannotation = new QPushButton(" ", htab);
    connect(editjavaannotation, SIGNAL(clicked()), this, SLOT(java_edit_annotation()));

    addTab(grid, "Java");

}

void OperationDialog::FillJavaTab(OperationData * oper)
{
    //lblNameFormJava->setVisible(isWritable);
    javafrozen_cb->setChecked(oper->java_get_set_frozen);
    javafrozen_cb->setDisabled(!isWritable || !oper->is_get_or_set);


    javafinal_cb->setChecked(oper->get_java_final());
    javafinal_cb->setDisabled(!isWritable);

    synchronized_cb->setChecked(oper->get_java_synchronized());
    synchronized_cb->setDisabled(!isWritable);

    lblNameFormJava->setVisible(oper->is_get_or_set);
    namespecTab->setVisible(oper->is_get_or_set);

    if (!isWritable)
        disconnect(edjavanamespec, SIGNAL(textChanged(const QString &)), this, SLOT(java_update_def()));
    else
        connect(edjavanamespec, SIGNAL(textChanged(const QString &)), this, SLOT(java_update_def()));

    edjavanamespec->setDisabled(!isWritable);
    edjavanamespec->setText(oper->java_name_spec);


    edjavadef->setText(oper->get_javadef());
    edjavadef->setFont(comment->font());
    edjavadef->setReadOnly(!isWritable);

    indentjavabody_cb->setChecked(false);
    indentjavabody_cb->setVisible(isWritable && !oper->is_get_or_set);
    indentjavabody_cb->setEnabled(!(preserve_bodies() && !forcegenbody_cb->isChecked()));
    indentjavabody_cb->setChecked((preserve_bodies() && !forcegenbody_cb->isChecked()) && oper->java_indent_body);

    showjavadef->setReadOnly(TRUE);
    showjavadef->setFont(comment->font());

    bool bodyEditable = (!isWritable || (preserve_bodies() && !forcegenbody_cb->isChecked()));
    QString buttonText = bodyEditable ? TR("Show body") : TR("Edit body");
    editjavabody->setText(buttonText);

    char * b = oper->get_body('j');

    if (b != 0) {
        javabody = oldjavabody = b;
        delete [] b;
    }

    cppTab->ui->pbDefaultDeclaration->setEnabled(isWritable);
    pbNotGeneratedInJava->setEnabled(isWritable);

    javaannotation = (const char *) oper->java_annotation;

    editjavaannotation->setText((!isWritable) ? TR("Show annotation") : TR("Edit annotation"));

    if (!GenerationSettings::java_get_default_defs())
        HideTab("Java");
    else
        ShowTab("Java");

    if (java_undef)
        HideTab("Java");
}

// Php
void OperationDialog::init_php()
{
    Q3Grid * grid;
    Q3HBox * htab = 0;
    Q3ButtonGroup * bgPhp;

    grid = new Q3Grid(2, this);
    phptab = grid;
    RegisterTab("Php", phptab);
    grid->setMargin(5);
    grid->setSpacing(5);

    phpfrozen_cb = new QCheckBox(TR("frozen"), grid);

    bgPhp = new Q3ButtonGroup(2, Qt::Horizontal, QString(), grid);

    phpfinal_cb = new QCheckBox("final", bgPhp);

    connect(phpfinal_cb, SIGNAL(toggled(bool)), SLOT(php_final_toggled(bool)));

    lblNameFormPhp = new QLabel(TR("Name form : "), grid);
    namespecTab = new Q3HBox(grid);
    edphpnamespec = new LineEdit(htab);


    new QLabel(TR("Definition :"), grid);
    edphpdef = new MultiLineEdit(grid);
    connect(edphpdef, SIGNAL(textChanged()), this, SLOT(php_update_def()));

    Q3VBox * vtab = new Q3VBox(grid);

    new QLabel(TR("Result after\nsubstitution : "), vtab);

    indentphpbody_cb = new QCheckBox(TR("contextual\nbody indent"), vtab);

    showphpdef = new MultiLineEdit(grid);

    editphpbody = new QPushButton("",  grid);
    connect(editphpbody, SIGNAL(clicked()), this, SLOT(php_edit_body()));

    char * b = oper->get_body('p');

    if (b != 0) {
        phpbody = oldphpbody = b;
        delete [] b;
    }

    htab = new Q3HBox(grid);

    pbDefaultDeclarationPhp = new QPushButton(TR("Default declaration"), htab);
    pbNotGeneratedInPhp = new QPushButton(TR("Not generated in Php"), htab);
    pbEditParametersPhp = new QPushButton(TR("Edit parameters"), htab);
    connect(pbDefaultDeclarationPhp, SIGNAL(clicked()), this, SLOT(php_default_def()));
    connect(pbNotGeneratedInPhp, SIGNAL(clicked()), this, SLOT(php_unmapped_def()));
    connect(pbEditParametersPhp, SIGNAL(clicked()), this, SLOT(php_edit_param()));
    addTab(grid, "Php");
}

void OperationDialog::FillPhpTab(OperationData *)
{
    phpfrozen_cb->setChecked(false);

    if (!isWritable || !oper->is_get_or_set)
        phpfrozen_cb->hide();
    else {
        if (oper->php_get_set_frozen)
            phpfrozen_cb->setChecked(TRUE);
    }

    //lblPhpPlaceholder1->setVisible();
    phpfrozen_cb->setChecked(oper->php_get_set_frozen);
    phpfrozen_cb->setDisabled(!isWritable || !oper->is_get_or_set);

    phpfinal_cb->setChecked(oper->get_php_final());
    phpfinal_cb->setDisabled(!isWritable);

    lblNameFormPhp->setVisible(oper->is_get_or_set);
    namespecTab->setVisible(oper->is_get_or_set);

    if (!isWritable)
        disconnect(edphpnamespec, SIGNAL(textChanged(const QString &)), this, SLOT(php_update_def()));
    else
        connect(edphpnamespec, SIGNAL(textChanged(const QString &)), this, SLOT(php_update_def()));

    edphpnamespec->setDisabled(!isWritable);
    edphpnamespec->setText(oper->php_name_spec);


    edphpdef->setText(oper->get_phpdef());
    edphpdef->setFont(comment->font());
    edphpdef->setReadOnly(!isWritable);

    indentphpbody_cb->setChecked(false);
    indentphpbody_cb->setVisible(isWritable && !oper->is_get_or_set);
    indentphpbody_cb->setEnabled(!(preserve_bodies() && !forcegenbody_cb->isChecked()));
    indentphpbody_cb->setChecked((preserve_bodies() && !forcegenbody_cb->isChecked()) && oper->php_indent_body);

    showphpdef->setReadOnly(TRUE);
    showphpdef->setFont(comment->font());

    bool bodyEditable = (!isWritable || (preserve_bodies() && !forcegenbody_cb->isChecked()));
    QString buttonText = bodyEditable ? TR("Show body") : TR("Edit body");
    editphpbody->setText(buttonText);

    char * b = oper->get_body('p');

    if (b != 0) {
        phpbody = oldphpbody = b;
        delete [] b;
    }

    cppTab->ui->pbDefaultDeclaration->setEnabled(isWritable);
    pbNotGeneratedInPhp->setEnabled(isWritable);

    ///phpannotation = (const char *) oper->php_annotation; todo

    pbEditParametersPhp->setText((!isWritable) ? TR("Show parameters") : TR("Edit parameters"));

    if (!GenerationSettings::php_get_default_defs())
        HideTab("Php");
    else
        ShowTab("Php");

    if (php_undef)
        HideTab("Php");
}


// Python_1
void OperationDialog::init_python()
{
    Q3Grid * grid;
    Q3HBox * htab = 0;
    grid = new Q3Grid(2, this);
    pythontab = grid;
    RegisterTab("Python", pythontab);
    grid->setMargin(5);
    grid->setSpacing(5);

    pythonfrozen_cb = new QCheckBox(TR("frozen"), grid);

    bgPython = new Q3ButtonGroup(2, Qt::Horizontal, QString(), grid);

    lblNameFormPython = new QLabel(TR("Name form : "), grid);
    pythonNamespecTab = new Q3HBox(grid);
    edpythonnamespec = new LineEdit(htab);


    new QLabel(TR("Definition :"), grid);
    edpythondef = new MultiLineEdit(grid);
    connect(edpythondef, SIGNAL(textChanged()), this, SLOT(python_update_def()));

    Q3VBox * vtab = new Q3VBox(grid);

    new QLabel(TR("Result after\nsubstitution : "), vtab);

    indentpythonbody_cb = new QCheckBox(TR("contextual\nbody indent"), vtab);

    showpythondef = new MultiLineEdit(grid);

    editpythonbody = new QPushButton("",  grid);
    connect(editpythonbody, SIGNAL(clicked()), this, SLOT(python_edit_body()));

    char * b = oper->get_body('y');

    if (b != 0) {
        pythonbody = oldpythonbody = b;
        delete [] b;
    }

    htab = new Q3HBox(grid);

    pbDefaultDeclarationPython = new QPushButton(TR("Default declaration"), htab);
    pbNotGeneratedInPython = new QPushButton(TR("Not generated in Python"), htab);
    pbEditParamsPython = new QPushButton(TR("Edit parameters"), htab);
    connect(pbDefaultDeclarationPython, SIGNAL(clicked()), this, SLOT(python_default_def()));
    connect(pbNotGeneratedInPython, SIGNAL(clicked()), this, SLOT(python_unmapped_def()));
    connect(pbEditParamsPython, SIGNAL(clicked()), this, SLOT(python_edit_param()));

    pythondecorator = (const char *) oper->python_decorator;
    editpythondecorator = new QPushButton("",  htab);
    connect(editpythondecorator, SIGNAL(clicked()), this, SLOT(python_edit_decorator()));

    addTab(pythontab, "Python");
}
void OperationDialog::FillPythonTab(OperationData *)
{
    pythonfrozen_cb->setChecked(false);

    if (!isWritable || !oper->is_get_or_set)
        pythonfrozen_cb->hide();
    else {
        if (oper->python_get_set_frozen)
            pythonfrozen_cb->setChecked(TRUE);
    }

    //lblPythonPlaceholder1->setVisible();
    pythonfrozen_cb->setChecked(oper->python_get_set_frozen);
    pythonfrozen_cb->setEnabled(!(!isWritable || !oper->is_get_or_set));


    // pythonfinal_cb->setChecked(oper->get_python_final());
    // pythonfinal_cb->setDisabled(!isWritable);

    lblNameFormPython->setVisible(oper->is_get_or_set);
    pythonNamespecTab->setVisible(oper->is_get_or_set);

    if (!isWritable) {
        disconnect(edpythonnamespec, SIGNAL(textChanged(const QString &)), this, SLOT(python_update_def()));
        disconnect(edpythondef, SIGNAL(textChanged()), this, SLOT(python_update_def()));
    }
    else {
        connect(edpythonnamespec, SIGNAL(textChanged(const QString &)), this, SLOT(python_update_def()));
        connect(edpythondef, SIGNAL(textChanged()), this, SLOT(python_update_def()));
    }

    edpythonnamespec->setDisabled(!isWritable);
    edpythonnamespec->setText(oper->python_name_spec);


    edpythondef->setText(oper->get_pythondef());
    edpythondef->setFont(comment->font());
    edpythondef->setReadOnly(!isWritable);

    indentpythonbody_cb->setChecked(false);
    indentpythonbody_cb->setVisible(isWritable && !oper->is_get_or_set);
    indentpythonbody_cb->setEnabled(!(preserve_bodies() && !forcegenbody_cb->isChecked()));
    indentpythonbody_cb->setChecked((preserve_bodies() && !forcegenbody_cb->isChecked()) && oper->python_indent_body);

    showpythondef->setReadOnly(TRUE);
    showpythondef->setFont(comment->font());

    bool bodyEditable = (!isWritable || (preserve_bodies() && !forcegenbody_cb->isChecked()));
    QString buttonText = bodyEditable ? TR("Show body") : TR("Edit body");
    editpythonbody->setText(buttonText);

    char * b = oper->get_body('y');

    if (b != 0) {
        pythonbody = oldpythonbody = b;
        delete [] b;
    }

    cppTab->ui->pbDefaultDeclaration->setEnabled(isWritable);
    pbNotGeneratedInPython->setEnabled(isWritable);
    pbEditParamsPython->setEnabled(isWritable);
    pythondecorator = (const char *) oper->python_decorator;
    editpythondecorator->setText((!isWritable) ? TR("Show decorators") : TR("Edit decorators"));

    if (!GenerationSettings::python_get_default_defs())
        HideTab("Python");
    else
        ShowTab("Python");

    if (python_undef)
        HideTab("Python");
}

// idl
void OperationDialog::init_idl()
{
    Q3Grid * grid;
    Q3HBox * htab = 0;

    grid = new Q3Grid(2, this);
    idltab = grid;
    RegisterTab("Idl", idltab);
    grid->setMargin(5);
    grid->setSpacing(5);

    idlfrozen_cb = new QCheckBox(TR("frozen"), grid);

    bgIdl = new Q3ButtonGroup(2, Qt::Horizontal, QString(), grid);

    oneway_cb = new QCheckBox("final", bgIdl);

    connect(oneway_cb, SIGNAL(toggled(bool)), SLOT(oneway_toggled(bool)));

    lblNameFormIdl = new QLabel(TR("Name form : "), grid);
    namespecTabIdl = new Q3HBox(grid);
    edidlnamespec = new LineEdit(htab);


    new QLabel(TR("Definition :"), grid);
    edidldecl = new MultiLineEdit(grid);
    connect(edidldecl, SIGNAL(textChanged()), this, SLOT(idl_update_decl()));

    Q3VBox * vtab = new Q3VBox(grid);

    new QLabel(TR("Result after\nsubstitution : "), vtab);

    showidldecl = new MultiLineEdit(grid);

//    editidlbody = new QPushButton("",  grid);
//    connect(editidlbody, SIGNAL(clicked()), this, SLOT(idl_edit_body()));

    htab = new Q3HBox(grid);

    pbDefaultDeclarationIdl = new QPushButton(TR("Default declaration"), htab);
    pbNotGeneratedInIdl = new QPushButton(TR("Not generated in Idl"), htab);
    connect(pbDefaultDeclarationIdl, SIGNAL(clicked()), this, SLOT(idl_default_def()));
    connect(pbNotGeneratedInIdl, SIGNAL(clicked()), this, SLOT(idl_unmapped_def()));
    addTab(grid, "Idl");
}


void OperationDialog::FillIdlTab(OperationData *)
{
    idlfrozen_cb->setChecked(false);

    if (!isWritable || !oper->is_get_or_set)
        idlfrozen_cb->hide();
    else {
        if (oper->idl_get_set_frozen)
            idlfrozen_cb->setChecked(TRUE);
    }

    //lblIdlPlaceholder1->setVisible();
    idlfrozen_cb->setChecked(oper->idl_get_set_frozen);
    idlfrozen_cb->setDisabled(!isWritable || !oper->is_get_or_set);

    oneway_cb->setChecked(oper->get_idl_oneway());
    oneway_cb->setDisabled(!isWritable);

    lblNameFormIdl->setVisible(oper->is_get_or_set);
    namespecTabIdl->setVisible(oper->is_get_or_set);

    if (!isWritable)
        disconnect(edidlnamespec, SIGNAL(textChanged(const QString &)), this, SLOT(idl_update_decl()));
    else
        connect(edidlnamespec, SIGNAL(textChanged(const QString &)), this, SLOT(idl_update_decl()));

    edidlnamespec->setDisabled(!isWritable);
    edidlnamespec->setText(oper->idl_name_spec);


    edidldecl->setText(oper->get_idldecl());
    edidldecl->setFont(comment->font());
    edidldecl->setReadOnly(!isWritable);

    showidldecl->setReadOnly(TRUE);
    showidldecl->setFont(comment->font());


    pbDefaultDeclarationIdl->setEnabled(isWritable);
    pbNotGeneratedInIdl->setEnabled(isWritable);

    if (!GenerationSettings::idl_get_default_defs())
        HideTab("Idl");
    else
        ShowTab("Idl");

    if (idl_undef)
        HideTab("Idl");
}

void OperationDialog::FillGeneric(/*OperationData * oper*/)
{
    QString stereotype = cl->get_stereotype();
    QString st;

    st = GenerationSettings::cpp_class_stereotype(stereotype);
    cpp_undef = (st == "enum") || (st == "typedef") || (st == "ignored");

    st = GenerationSettings::java_class_stereotype(stereotype);
    java_undef = (st == "enum_pattern") || (st == "ignored");

    st = GenerationSettings::php_class_stereotype(stereotype);
    php_undef = (st == "enum") || (st == "ignored");

    st = GenerationSettings::python_class_stereotype(stereotype);
    python_undef = (st == "enum") || (st == "ignored");

    st = GenerationSettings::idl_class_stereotype(stereotype);
    idl_undef = (st == "enum") || (st == "typedef") || (st == "ignored");


    unique = (GenerationSettings::cpp_get_default_defs())
             ? ((cppTab != 0) ? CppView : DefaultDrawingLanguage)
                 : UmlView;

    if (GenerationSettings::java_get_default_defs())
        unique = ((unique == UmlView) && (javatab != 0))
                 ? JavaView
                 : DefaultDrawingLanguage;

    if (GenerationSettings::php_get_default_defs())
        unique = ((unique == UmlView) && (phptab != 0))
                 ? PhpView
                 : DefaultDrawingLanguage;

    if (GenerationSettings::python_get_default_defs())
        unique = ((unique == UmlView) && (pythontab != 0))
                 ? PythonView
                 : DefaultDrawingLanguage;

    if (GenerationSettings::idl_get_default_defs())
        unique = ((unique == UmlView) && (idltab != 0))
                 ? IdlView
                 : DefaultDrawingLanguage;

    if (unique == DefaultDrawingLanguage)
        unique = UmlView;

    //

    switch (drawingLanguage)
    {
    case CppView:
        if (! cpp_undef) {
            cpp_update_def();
            QTimer::singleShot(100, this, SLOT(cpp_edit_body()));
        }

        break;

    case JavaView:
        if (! java_undef) {
            java_update_def();
            QTimer::singleShot(100, this, SLOT(java_edit_body()));
        }

        break;

    case PhpView:
        if (! php_undef) {
            php_update_def();
            QTimer::singleShot(100, this, SLOT(php_edit_body()));
        }

        break;

    case PythonView:
        if (! python_undef) {
            python_update_def();
            QTimer::singleShot(100, this, SLOT(python_edit_body()));
        }

        break;

    default:
        break;
    }
}


void OperationDialog::menu_returntype()
{
    Q3PopupMenu m(0);

    m.insertItem(TR("Choose"), -1);
    m.insertSeparator();

    int index = list.findIndex(edreturn_type->currentText().stripWhiteSpace());

    if (index != -1)
        m.insertItem(TR("Select in browser"), 0);

    BrowserNode * bn = 0;

    if (isWritable) {
        bn = BrowserView::selected_item();

        if ((bn != 0) && (bn->get_type() == UmlClass) && !bn->deletedp())
            m.insertItem(TR("Choose class selected in browser"), 1);
        else
            bn = 0;

        m.insertItem(TR("Create class and choose it"), 2);
    }

    if (isWritable || (index != -1) || (bn != 0)) {
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
                edreturn_type->insertItem(s, index + edreturn_type_offset);
            }
        }

        edreturn_type->setCurrentItem(index + edreturn_type_offset);
        }
    }
}

void OperationDialog::default_description()
{
    comment->setText(GenerationSettings::default_operation_description());
}

void OperationDialog::edit_description()
{
    edit(comment->text(), edname->text().stripWhiteSpace() + "_description",
         oper, TxtEdit, this, (post_edit) post_edit_description, edits);
}

void OperationDialog::post_edit_description(OperationDialog * d, QString s)
{
    d->comment->setText(s);
}

void OperationDialog::edit_constraint()
{
    edit(constraint->text(), edname->text().stripWhiteSpace() + "_constraint",
         oper, TxtEdit, this, (post_edit) post_edit_constraint, edits);
}

void OperationDialog::post_edit_constraint(OperationDialog * d, QString s)
{
    d->constraint->setText(s);
}

void OperationDialog::accept()
{

    if(SaveData())
        Q3TabDialog::accept();
}

void OperationDialog::classoper_toggled(bool on)
{
    if (on) {
        abstract_cb->setChecked(FALSE);

        if (! cpp_undef)
            cbCppVirtual->setChecked(FALSE);
    }
}

void OperationDialog::abstract_toggled(bool on)
{
    if (on) {
        classoperation_cb->setChecked(FALSE);

        if (! cpp_undef)
            cbCppVirtual->setChecked(TRUE);
    }
}

void OperationDialog::forcegenbody_toggled(bool on)
{
    bool ro = (!isWritable || (preserve_bodies() && !on));
    QString lbl = (ro) ? TR("Show body") : TR("Edit body");

    if (!cpp_undef)
        cppTab->ui->editcppbody->setText(lbl);

    if (!java_undef)
        editjavabody->setText(lbl);

    if (!php_undef)
        editphpbody->setText(lbl);

    if (!python_undef)
        editpythonbody->setText(lbl);

    if (isWritable && !oper->is_get_or_set) {
        if (!cpp_undef)
            cppTab->ui->cbIndentCppBody->setEnabled(cppTab->ui->editcppbody->isEnabled() && !ro);

        if (!java_undef)
            indentjavabody_cb->setEnabled(editjavabody->isEnabled() && !ro);

        if (!php_undef)
            indentphpbody_cb->setEnabled(editphpbody->isEnabled() && !ro);

        if (!python_undef)
            indentpythonbody_cb->setEnabled(editpythonbody->isEnabled() && !ro);
    }
}

void OperationDialog::update_all_tabs(QWidget * w)
{
    table->forceUpdateCells();
    etable->forceUpdateCells();

    if (w == umltab) {
        if (isWritable)
            edname->setFocus();
    }
    else if (w == cppTab)
    {
        cpp_update_decl();
        cpp_update_def();

        if (isWritable)
            cppTab->ui->edCppDeclProto->setFocus();
    }
    else if (w == javatab) {
        java_update_def();

        if (isWritable)
            edjavadef->setFocus();
    }
    else if (w == phptab) {
        php_update_def();

        if (isWritable)
            edphpdef->setFocus();
    }
    else if (w == pythontab) {
        python_update_def();

        if (isWritable)
            edpythondef->setFocus();
    }
    else if (w == idltab) {
        idl_update_decl();

        if (isWritable)
            edidldecl->setFocus();
    }
}

// C++

void OperationDialog::const_volatile_toggled(bool)
{
    cpp_update_decl();
    cpp_update_def();
}

void OperationDialog::virtual_toggled(bool on)
{
    if (on)
        cbCppFriend->setChecked(FALSE);

    cpp_update_decl();
    cpp_update_def();
}

void OperationDialog::friend_toggled(bool on)
{
    if (on) {
        cbCppInline->setChecked(FALSE);
        cbCppVirtual->setChecked(FALSE);
    }

    cpp_update_decl();
    cpp_update_def();
}

void OperationDialog::inline_toggled(bool on)
{
    if (on)
        cbCppFriend->setChecked(FALSE);

    cpp_update_decl();
    cpp_update_def();
}

void OperationDialog::final_toggled(bool on)
{
    if (on) {
        cppTab->ui->cbCppDefaulted->setChecked(FALSE);
        cppTab->ui->cbCppDeleted->setChecked(FALSE);
    }

    cpp_update_decl();
    cpp_update_def();
}

void OperationDialog::delete_toggled(bool on)
{
    if (on) {
        cppTab->ui->cbCppFinal->setChecked(FALSE);
        cppTab->ui->cbCppOverride->setChecked(FALSE);
        cppTab->ui->cbCppDefaulted->setChecked(FALSE);
    }

    cpp_update_decl();
    cpp_update_def();
}

void OperationDialog::override_toggled(bool on)
{
    if (on) {
        cppTab->ui->cbCppDefaulted->setChecked(FALSE);
        cppTab->ui->cbCppDeleted->setChecked(FALSE);
    }

    cpp_update_decl();
    cpp_update_def();
}

void OperationDialog::default_toggled(bool on)
{
    if (on) {
        cppTab->ui->cbCppFinal->setChecked(FALSE);
        cppTab->ui->cbCppOverride->setChecked(FALSE);
        cppTab->ui->cbCppDeleted->setChecked(FALSE);
    }

    cpp_update_decl();
    cpp_update_def();
}

void OperationDialog::cpp_default_decl()
{
    if (oper->is_get_or_set)
    {
        WrapperStr decl;
        WrapperStr def;

        if (get_of_attr != 0)
            oper->update_cpp_get_of(decl, def, get_of_attr->get_browser_node()->get_name(),
                                    get_of_attr->get_cppdecl(),
                                    get_of_attr->get_isa_const_attribute(),
                                    get_of_attr->get_multiplicity(), oper->get_isa_class_operation());
        else if (set_of_attr != 0)
            oper->update_cpp_set_of(decl, def, set_of_attr->get_browser_node()->get_name(),
                                    set_of_attr->get_cppdecl(),
                                    set_of_attr->get_isa_const_attribute(),
                                    set_of_attr->get_multiplicity());
        else if (get_of_rel != 0)
        {
            if (is_rel_a)
                oper->update_cpp_get_of(decl, def, get_of_rel->get_role_a(),
                                        get_of_rel->get_cppdecl_a(),
                                        get_of_rel->get_isa_const_relation_a(),
                                        get_of_rel->get_multiplicity_a(), oper->get_isa_class_operation());
            else
                oper->update_cpp_get_of(decl, def, get_of_rel->get_role_b(),
                                        get_of_rel->get_cppdecl_b(),
                                        get_of_rel->get_isa_const_relation_b(),
                                        get_of_rel->get_multiplicity_b(), oper->get_isa_class_operation());
        }
        else {
            // set_of_rel != 0
            if (is_rel_a)
                oper->update_cpp_set_of(decl, def, set_of_rel->get_role_a(),
                                        set_of_rel->get_cppdecl_a(),
                                        set_of_rel->get_isa_const_relation_a(),
                                        set_of_rel->get_multiplicity_a());
            else
                oper->update_cpp_set_of(decl, def, set_of_rel->get_role_b(),
                                        set_of_rel->get_cppdecl_b(),
                                        set_of_rel->get_isa_const_relation_b(),
                                        set_of_rel->get_multiplicity_b());
        }

        cppTab->ui->edCppDeclProto->setText(decl);
    }
    else
    {
        QString s = oper->default_cpp_decl(edname->text().stripWhiteSpace());

        GenerationSettings::set_cpp_return_type(the_type(edreturn_type->currentText()
                                                .stripWhiteSpace(),
                                                list, nodes),
                                                s);

        int index;
        QString params;
        int nparams = (int) table->nparams();
        QString sep;

        for (index = 0; index != nparams; index += 1, sep = ", ")
            params += sep + GenerationSettings::cpp(the_type(table->type(index),
                                                    list, nodes),
                                                    table->dir(index), index)
                      + "${v" + QString().setNum(index) + "}";

        if ((index = s.find("${)}")) != -1)
            s.insert(index, params);

        cppTab->ui->edCppDeclProto->setText(s);
    }

    cpp_update_decl();
}

void OperationDialog::cpp_unmapped_decl()
{
    cppTab->ui->edCppDeclProto->setText(QString());
    cppTab->ui->edCppDeclActual->setText(QString());

    cpp_unmapped_def();
}

void OperationDialog::cpp_decl_from_def()
{
    QString dcl = oper->default_cpp_decl(edname->text().stripWhiteSpace());
    QString def = cppTab->ui->edCppDefProto->text();
    int index1;
    int index2;
    int index3;
    int index4;

    // copy args
    if (((index1 = def.find("${(}") + 4) == (-1 + 4)) ||
        ((index2 = def.find("${)}", index1)) == -1) ||
        ((index3 = dcl.find("${(}") + 4) == (-1 + 4)) ||
        ((index4 = dcl.find("${)}", index3)) == -1))
        // wrong specification(s)
        return;

    if (index4 != index3)
        dcl.remove(index3, index4 - index3);

    if (index2 != index1)
        dcl.insert(index3, def.mid(index1, index2 - index1));

    // copy return type modifications
    if (((index1 = def.find("${type}")) != -1) &&
        ((index2 = def.find("${class}", index1 + 7)) != -1) &&
        ((index3 = dcl.find("${type}")) != -1) &&
        ((index4 = dcl.find("${name}", index3 + 7)) != -1)) {
        // valid specification
        while ((index1 != 0) && (def.at(index1 - 1) != QChar('}')))
            index1 -= 1;

        while ((index3 != 0) && (dcl.at(index3 - 1) != QChar('}')))
            index3 -= 1;

        dcl.remove(index3, index4 - index3);
        dcl.insert(index3, def.mid(index1, index2 - index1));
    }

    // manage const
    if ((def.find("${const}") == -1) && // const removed
        ((index1 = dcl.find("${const}")) != -1))
        // remove const
        dcl.remove(index1, 8);

    // manage throw
    if ((def.find("${throw}") == -1) && // throw removed
        ((index1 = dcl.find("${throw}")) != -1))
        // remove throw
        dcl.remove(index1, 8);

    //add default param value
    index1 = 0;

    while (((index2 = dcl.find("${p", index1)) != -1) &&
           ((index3 = dcl.find('}', index2 + 3)) != -1)) {
        dcl.insert(index3 + 1, "${v" + dcl.mid(index2 + 3, index3 - index2 - 2));
        index1 = index3 + index3 - index2 + 2;
    }

    // update decl
    cppTab->ui->edCppDeclProto->setText(dcl);
    cpp_update_decl();
}

void OperationDialog::cpp_edit_param_decl()
{
    QString form = cppTab->ui->edCppDeclProto->text();
    int index;

    if (((index = form.find("${(}")) != 0) &&
        (form.find("${)}", index + 4) != 0))
    {
        CppParamsDialog d(this, table, cppTab->ui->edCppDeclProto, TRUE);

        if (d.exec() == QDialog::Accepted)
        {
            cpp_update_decl();
            cpp_def_from_decl();
            cpp_update_def();
        }
    }
    else
        msg_warning("Douml", TR("wrong specification"));
}


void OperationDialog::manage_cpp_type(unsigned rank, QString & s)
{
    if (rank < table->nparams())
        s += get_cpp_name(the_type(table->type(rank), list, nodes));
    else {
        s += "${t";
        s += QString::number(rank);
        s += '}';
    }
}

void OperationDialog::manage_var(unsigned rank, QString & s)
{
    if (rank < table->nparams())
        s += table->name(rank);
    else {
        s += "${p";
        s += QString::number(rank);
        s += '}';
    }
}

QString OperationDialog::compute_name(LineEdit * spec)
{
    if (spec->isVisible())
    {
        QString s = spec->text().stripWhiteSpace();
        int index;

        if ((index = s.find("${name}")) != -1)
            s.replace(index, 7, ((BrowserOperation *) oper->browser_node)->get_of_name());
        else if ((index = s.find("${Name}")) != -1)
            s.replace(index, 7,
                      capitalize(((BrowserOperation *) oper->browser_node)->get_of_name()));

        return s;
    }
    else
        return edname->text();
}

void OperationDialog::manage_init(unsigned rank, QString & s)
{
    if (rank < table->nparams()) {
        QString v = table->value(rank).stripWhiteSpace();

        if (! v.isEmpty())
            s += " = " + v;
    }
    else {
        s += "${v";
        s += QString::number(rank);
        s += '}';
    }
}



void OperationDialog::manage_cpp_exceptions(QString & s)
{
    int index;
    int ne = etable->nexceptions();
    const char * sep = " throw (";

    for (index = 0; index != ne; index += 1)
    {
        s += sep;
        s += GenerationSettings::cpp_type(type(etable->type(index), list, nodes));
        sep = ", ";
    }

    if (index != 0)
        s += ')';
    else if (GenerationSettings::cpp_oper_force_throw())
        s += " throw ()";
}

bool CompareAgainstTag(QString & currentTag, QString tagToCompare, const char * p)
{
    currentTag = tagToCompare;
    tagToCompare = "${" + tagToCompare + "}";

    if (!strncmp(p, tagToCompare, tagToCompare.length()))
        return true;

    return false;
}

void OperationDialog::cpp_update_decl()
{
    cpp_visibility.update_default(uml_visibility);

    // do NOT write
    //	const char * p = edcppdecl->text();
    // because the QString is immediatly destroyed !
    QString def = cppTab->ui->edCppDeclProto->text();
    const char * p = def;
    const char * pp = 0;
    QString indent = "";
    QString s;
    unsigned rank;

    while ((*p == ' ') || (*p == '\t'))
        indent += *p++;

    if (*p != '#')
        s = indent;

    QString currentTag;

    for (;;) {
        if (*p == 0)
        {
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

        std::function<bool(QString)> compareTagToBuffer = std::bind(CompareAgainstTag, std::ref(currentTag), std::placeholders::_1, p);

        if (!strncmp(p, "${comment}", 10))
            manage_comment(comment->text(), p, pp,
                           GenerationSettings::cpp_javadoc_style());
        else if (!strncmp(p, "${description}", 14))
            manage_description(comment->text(), p, pp);
        else if (!strncmp(p, "${friend}", 9)) {
            p += 9;

            if (cbCppFriend->isChecked())
                s += "friend ";
        }
        else if (!strncmp(p, "${static}", 9)) {
            p += 9;

            if (classoperation_cb->isChecked())
                s += "static ";
        }
        else if (!strncmp(p, "${inline}", 9)) {
            p += 9;

            if (cbCppInline->isChecked())
                s += "inline ";
        }
        else if (!strncmp(p, "${virtual}", 10)) {
            p += 10;

            if (cbCppVirtual->isChecked())
                s += "virtual ";
        }
        else if (!strncmp(p, "${type}", 7)) {
            p += 7;
            s += get_cpp_name(the_type(edreturn_type->currentText().stripWhiteSpace(),
                                       list, nodes));
        }
        else if (!strncmp(p, "${name}", 7)) {
            p += 7;
            s += compute_name(cppTab->ui->leCppNamespec);
        }
        else if (!strncmp(p, "${class}", 8)) {
            // to be placed in the description
            p += 8;
            s += cl->get_browser_node()->get_name();
        }
        else if (!strncmp(p, "${(}", 4)) {
            p += 4;
            s += '(';
        }
        else if (!strncmp(p, "${)}", 4)) {
            p += 4;
            s += ')';
        }
        else if (!strncmp(p, "${const}", 8)) {
            p += 8;

            if (cbCppConst->isChecked())
                s += " const";
        }
        else if (!strncmp(p, "${volatile}", 11)) {
            p += 11;

            if (cbCppVolatile->isChecked())
                s += " volatile";
        }
        else if (!strncmp(p, "${throw}", 8)) {
            p += 8;
            manage_cpp_exceptions(s);
        }
        else if (!strncmp(p, "${abstract}", 11)) {
            p += 11;

            if (abstract_cb->isChecked())
                s += " = 0";
        }
        else if (sscanf(p, "${t%u}", &rank) == 1) {
            manage_cpp_type(rank, s);
            p = strchr(p, '}') + 1;
        }
        else if (sscanf(p, "${p%u}", &rank) == 1) {
            manage_var(rank, s);
            p = strchr(p, '}') + 1;
        }
        else if (sscanf(p, "${v%u}", &rank) == 1) {
            manage_init(rank, s);
            p = strchr(p, '}') + 1;
        }
        else if (oper->is_get_or_set && !strncmp(p, "${stereotype}", 13)) {
            p += 13;
            // get/set with multiplicity > 1
            s += GenerationSettings::cpp_relationattribute_stereotype(oper->stereotype);
        }
        else if (oper->is_get_or_set && !strncmp(p, "${association}", 14)) {
            p += 14;
            // get/set with multiplicity > 1
            s += GenerationSettings::cpp_type(((BrowserOperation *) oper->browser_node)
                                              ->get_of_association().get_type());
        }
        else if (*p == '\n') {
            s += *p++;

            if (*p && (*p != '#'))
                s += indent;
        }
        else if (*p == '@')
            manage_alias(oper->browser_node, p, s, kvtable);
        else if (compareTagToBuffer("final")) {
            p += currentTag.length() + 3;

            if (cppTab->ui->cbCppFinal->isChecked())
                s += " " + currentTag;
        }
        else if (compareTagToBuffer("default")) {
            p += currentTag.length() + 3;

            if (cppTab->ui->cbCppDefaulted->isChecked())
                s += " = " + currentTag;
        }
        else if (compareTagToBuffer("delete")) {
            p += currentTag.length() + 3;

            if (cppTab->ui->cbCppDeleted->isChecked())
                s += " = " + currentTag;
        }
        else if (compareTagToBuffer("override")) {
            p += currentTag.length() + 3;

            if (cppTab->ui->cbCppOverride->isChecked())
                s += " " + currentTag;
        }
        else
            s += *p++;
    }

    cppTab->ui->edCppDeclActual->setText(s);
}

QString OperationDialog::cpp_decl(const BrowserOperation * op, bool withname,
                                  ShowContextMode mode)
{
    OperationData * d = (OperationData *) op->get_data();
    WrapperStr decl = d->cpp_decl;

    remove_comments(decl);
    remove_preprocessor(decl);

    const char * p = decl;
    bool afterparams = FALSE;
    WrapperStr s;
    unsigned rank;

    while ((*p == ' ') || (*p == '\t'))
        p += 1;

    while (*p) {
        if (!strncmp(p, "${comment}", 10))
            p += 10;
        else if (!strncmp(p, "${description}", 14))
            p += 14;
        else if (!strncmp(p, "${friend}", 9))
            p += 9;
        else if (!strncmp(p, "${static}", 9))
            p += 9;
        else if (!strncmp(p, "${inline}", 9))
            p += 9;
        else if (!strncmp(p, "${virtual}", 10))
            p += 10;
        else if (!strncmp(p, "${type}", 7)) {
            p += 7;
            s += get_cpp_name(d->return_type, mode);
        }
        else if (!strncmp(p, "${name}", 7)) {
            p += 7;
            s += op->compute_name(d->cpp_name_spec);
        }
        else if (!strncmp(p, "${(}", 4)) {
            p += 4;
            s += '(';
        }
        else if (!strncmp(p, "${)}", 4)) {
            p += 4;
            s += ')';
            afterparams = TRUE;
        }
        else if (!strncmp(p, "${const}", 8))
            p += 8;
        else if (!strncmp(p, "${volatile}", 11))
            p += 11;
        else if (!strncmp(p, "${throw}", 8))
            p += 8;
        else if (!strncmp(p, "${abstract}", 11))
            break;
        else if (sscanf(p, "${t%u}", &rank) == 1) {
            if (rank < d->nparams)
                s += GenerationSettings::cpp_type(d->params[rank]->get_type().get_type(mode));
            else {
                s += "${t";
                s += QString::number(rank);
                s += '}';
            }

            p = strchr(p, '}') + 1;
        }
        else if (sscanf(p, "${p%u}", &rank) == 1) {
            if (withname) {
                if (rank < d->nparams)
                    s += d->params[rank]->get_name();
                else {
                    s += "${p";
                    s += QString::number(rank);
                    s += '}';
                }
            }

            p = strchr(p, '}') + 1;
        }
        else if (sscanf(p, "${v%u}", &rank) == 1) {
            if (rank < d->nparams) {
                WrapperStr v = d->params[rank]->get_default_value();

                if (!v.isEmpty())
                    s += " = " + v;
            }
            else {
                s += "${v";
                s += QString::number(rank);
                s += '}';
            }

            p = strchr(p, '}') + 1;
        }
        else if (d->is_get_or_set && !strncmp(p, "${stereotype}", 13)) {
            p += 13;
            // get/set with multiplicity > 1
            s += GenerationSettings::cpp_relationattribute_stereotype(d->stereotype);
        }
        else if (d->is_get_or_set && !strncmp(p, "${association}", 14)) {
            p += 14;
            // get/set with multiplicity > 1
            s += GenerationSettings::cpp_type(op->get_of_association().get_type());
        }
        else if (*p == '\n') {
            s += " ";

            do
                p += 1;

            while ((*p == ' ') || (*p == '\t'));
        }
        else if ((*p == '{') ||
                 (*p == ';') ||
                 ((*p == ':') && afterparams))
            break;
        else if (*p == '@')
            manage_alias(op, p, s.GetInternalRef(), static_cast<KeyValuesTable*>(0));
        else
            s += *p++;
    }

    return s;
}

void OperationDialog::cpp_default_def()
{
    if (oper->is_get_or_set) {
        WrapperStr decl;
        WrapperStr def;

        if (get_of_attr != 0)
            oper->update_cpp_get_of(decl, def, get_of_attr->get_browser_node()->get_name(),
                                    get_of_attr->get_cppdecl(),
                                    get_of_attr->get_isa_const_attribute(),
                                    get_of_attr->get_multiplicity(), oper->get_isa_class_operation());
        else if (set_of_attr != 0)
            oper->update_cpp_set_of(decl, def, set_of_attr->get_browser_node()->get_name(),
                                    set_of_attr->get_cppdecl(),
                                    set_of_attr->get_isa_const_attribute(),
                                    set_of_attr->get_multiplicity());
        else if (get_of_rel != 0) {
            if (is_rel_a)
                oper->update_cpp_get_of(decl, def, get_of_rel->get_role_a(),
                                        get_of_rel->get_cppdecl_a(),
                                        get_of_rel->get_isa_const_relation_a(),
                                        get_of_rel->get_multiplicity_a(), oper->get_isa_class_operation());
            else
                oper->update_cpp_get_of(decl, def, get_of_rel->get_role_b(),
                                        get_of_rel->get_cppdecl_b(),
                                        get_of_rel->get_isa_const_relation_b(),
                                        get_of_rel->get_multiplicity_b(), oper->get_isa_class_operation());
        }
        else {
            // set_of_rel != 0
            if (is_rel_a)
                oper->update_cpp_set_of(decl, def, set_of_rel->get_role_a(),
                                        set_of_rel->get_cppdecl_a(),
                                        set_of_rel->get_isa_const_relation_a(),
                                        set_of_rel->get_multiplicity_a());
            else
                oper->update_cpp_set_of(decl, def, set_of_rel->get_role_b(),
                                        set_of_rel->get_cppdecl_b(),
                                        set_of_rel->get_isa_const_relation_b(),
                                        set_of_rel->get_multiplicity_b());
        }

        cppTab->ui->edCppDefProto->setText(def);
    }
    else {
        QString s = oper->default_cpp_def(edname->text().stripWhiteSpace());

        GenerationSettings::set_cpp_return_type(the_type(edreturn_type->currentText()
                                                .stripWhiteSpace(),
                                                list, nodes),
                                                s);

        QString params;
        int index;
        int nparams = (int) table->nparams();
        QString sep;

        for (index = 0; index != nparams; index += 1, sep = ", ")
            params += sep + GenerationSettings::cpp(the_type(table->type(index),
                                                    list, nodes),
                                                    table->dir(index), index);

        if ((index = s.find("${)}")) != -1)
            s.insert(index, params);

        cppTab->ui->edCppDefProto->setText(s);
    }

    cpp_update_def();
}

void OperationDialog::cpp_unmapped_def()
{
    cppTab->ui->edCppDefProto->setText(QString());
    cppTab->ui->edCppDefActual->setText(QString());
}
struct UserTag
{
    UserTag(QString _tag, QString _leftContext,QString _rightContext):
         leftContext(_leftContext),rightContext(_rightContext),tag(_tag)
    {}
    QString leftContext;
    QString rightContext;
    QString tag;
};

void OperationDialog::cpp_def_from_decl()
{

    QString dcl = cppTab->ui->edCppDeclProto->text();
    QString def;
    QString currentActualText = cppTab->ui->edCppDefProto->text();
    QRegExp rx(tr("[@][{][A-Za-z0-9]+[}]"));
    int shift = 0;
    QList<UserTag> tags;
    while(rx.indexIn(currentActualText, shift) != -1)
    {
        int matchEnd = rx.pos()+rx.matchedLength();
        QString leftContext = currentActualText.mid(rx.pos() - 10, rx.pos() < 10 ? rx.pos() : 10);
        QString rightContext = currentActualText.mid(matchEnd, currentActualText.length() - matchEnd < 10 ? currentActualText.length() - matchEnd : 10);
        tags.append(UserTag(rx.cap(),leftContext,rightContext));
        shift = matchEnd;
    }


    // manage abstract
    if (/*(dcl.find("${abstract}") == -1) || !abstract_cb->isChecked()*/true)
    {
        // probably not abstract
        int index1;
        int index2;
        int index3;
        int index4;

        def = oper->default_cpp_def(edname->text().stripWhiteSpace());

        // copy args
        if (((index1 = dcl.find("${(}") + 4) == (-1 + 4)) ||
            ((index2 = dcl.find("${)}", index1)) == -1) ||
            ((index3 = def.find("${(}") + 4) == (-1 + 4)) ||
            ((index4 = def.find("${)}", index3)) == -1))
            // wrong specification(s)
            return;

        if (index4 != index3)
            def.remove(index3, index4 - index3);

        if (index2 != index1)
            def.insert(index3, dcl.mid(index1, index2 - index1));

        // copy return type modifications
        if (((index1 = dcl.find("${type}")) != -1) &&
            ((index2 = dcl.find("${name}", index1 + 7)) != -1) &&
            ((index3 = def.find("${type}")) != -1) &&
            ((index4 = def.find("${class}", index3 + 7)) != -1)) {
            // valid specifications
            while ((index1 != 0) && (dcl.at(index1 - 1) != QChar('}')))
                index1 -= 1;

            while ((index3 != 0) && (def.at(index3 - 1) != QChar('}')))
                index3 -= 1;

            def.remove(index3, index4 - index3);
            def.insert(index3, dcl.mid(index1, index2 - index1));
        }

        // manage const
        if ((dcl.find("${const}") == -1) && // const removed
            ((index1 = def.find("${const}")) != -1))
            // remove const
            def.remove(index1, 8);

        // manage throw
        if ((dcl.find("${throw}") == -1) && // throws removed
            ((index1 = def.find("${throw}")) != -1))
            // remove throw
            def.remove(index1, 8);

        // remove inits
        index1 = def.find("${(}");
        index4 = def.find("${)}", index1 + 4);

        while (((index2 = def.find("${v", index1)) != -1) &&
               (index2 < index4) &&
               ((index3 = def.find('}', index2 + 3)) != -1)) {
            def.remove(index2, index3 - index2 + 1);
            index1 = index2;
        }
    }

        QList<UserTag> failedTags;
        for(auto tag : tags)
        {
            //we remove all instances of the tag that might have been left
            def = def.replace(QString("@{") + tag.tag + QString("}"), "");

            //first we try to find left context in the new definition
            QRegExp rx(QRegExp::escape(tag.leftContext));
            int index = rx.indexIn(def);
            if(index != -1)
            {
                def.replace(tag.leftContext, tag.leftContext + tag.tag);
                continue;
            }
            // next we try right context
            rx.setPattern(QRegExp::escape(tag.leftContext));
            index = rx.indexIn(def);
            if(index != -1)
            {
                def.replace(tag.rightContext, tag.tag + tag.leftContext);
                continue;
            }
            failedTags.append(tag);
        }
        if(failedTags.size() > 0)
        {
            QString tagString;
            for(auto tag : failedTags)
                tagString.append(tag.tag + tr(","));
            if(tagString.length() > 0)
                tagString.chop(1);
            QMessageBox::critical(0, tr("Warning!"), tr("Could not find correct place for these tags: " + tagString), QMessageBox::Ok);
        }


    // update def
    cppTab->ui->edCppDefProto->setText(def);
    if(oper->is_abstract)
        return;
    cpp_update_def();
}

void OperationDialog::cpp_edit_param_def()
{
    QString form = cppTab->ui->edCppDefProto->text();
    int index;

    if (((index = form.find("${(}")) != 0) &&
        (form.find("${)}", index + 4) != 0)) {
        CppParamsDialog d(this, table, cppTab->ui->edCppDefProto, FALSE);

        d.raise();

        if (d.exec() == QDialog::Accepted)
            cpp_update_def();
    }
    else
        msg_warning("Douml", TR("wrong specification"));
}

// return TRUE if stop on ${commnt} or ${description}
bool insert_template(const QString & tm, const char *& p,
                     QString & s, const QString & indent)
{
    // search the beginning of the definition/declaration in p;
    for (;;) {
        if (*p == 0)
            return FALSE;

        if (*p == '\n') {
            s += *p++;

            if (*p != '#')
                s += indent;
        }

        if (*p <= ' ')
            s += *p++;
        else if (*p == '/') {
            if (p[1] == '/') {
                // comment
                s += *p++;

                do s += *p++;

                while (*p && (*p != '\n'));
            }
            else if (p[1] == '*') {
                /* comment */
                s += *p++;

                do s += *p++;

                while (*p && ((*p != '*') || (p[1] != '/')));

                s += "*/";
                p += 2;
            }
            else
                break;
        }
        else if (*p == '#') {
            do {
                s += *p++;

                if (*p == '\\') {
                    s += *p++;
                    s += *p++;
                }
                else if ((*p == '/') && (p[1] == '*')) {
                    /* comment */
                    s += *p++;

                    do s += *p++;

                    while (*p && ((*p != '*') || (p[1] != '/')));

                    s += "*/";
                    p += 2;
                }
            }
            while (*p && (*p != '\n'));
        }
        else if ((strncmp(p, "${comment}", 10) == 0) ||
                 (strncmp(p, "${description}", 14) == 0))
            return TRUE;
        else
            break;
    }

    s += tm;
    return FALSE;
}

void OperationDialog::cpp_update_def()
{

    QString def = cppTab->ui->edCppDefProto->text();

    if (cppTab->ui->cbCppDefaulted->isChecked() ||
        cppTab->ui->cbCppDeleted->isChecked()) {
        cppTab->ui->edCppDefActual->setText("");
        forcegenbody_toggled(forcegenbody_cb->isChecked());	// update indent*body_cb
        return;
    }

    bool template_oper = cppTab->ui->edCppDeclProto->text().isEmpty()
                         && oper->is_template_operation(def);
    QString s;

    if (!abstract_cb->isChecked()) {
        // do NOT write
        //	const char * p = edcppdef->text()
        // because the QString is immediatly destroyed !
        const char * p = def;
        const char * pp = 0;
        QString indent = "";
        unsigned rank;

        while ((*p == ' ') || (*p == '\t'))
            indent += *p++;

        bool re_template = !templates.isEmpty() &&
                           insert_template((template_oper) ? templates_tmplop : templates,
                                           p, s, indent);

        if (*p != '#')
            s += indent;

        for (;;) {

            if (*p == 0) {
                if (pp == 0)
                    break;

                // comment management done
                p = pp;
                pp = 0;

                if (re_template)
                    s += templates;

                if (*p == 0)
                    break;

                if (*p != '#')
                    s += indent;
            }

            if (!strncmp(p, "${comment}", 10)) {
                if (!manage_comment(comment->text(), p, pp,
                                    GenerationSettings::cpp_javadoc_style())
                    && re_template)
                    s += templates;
            }
            else if (!strncmp(p, "${description}", 14)) {
                if (!manage_description(comment->text(), p, pp) && re_template)
                    s += templates;
            }
            else if (!strncmp(p, "${inline}", 9)) {
                p += 9;

                if (cbCppInline->isChecked())
                    s += "inline ";
            }
            else if (!strncmp(p, "${type}", 7)) {
                p += 7;
                s += get_cpp_name(the_type(edreturn_type->currentText().stripWhiteSpace(),
                                           list, nodes));
            }
            else if (!strncmp(p, "${class}", 8)) {
                p += 8;

                if (cbCppFriend->isChecked() && !strncmp(p, "::", 2))
                    p += 2;
                else
                    s += ((template_oper) ? cl_names_tmplop : cl_names);
            }
            else if (!strncmp(p, "${name}", 7)) {
                p += 7;
                s += compute_name(cppTab->ui->leCppNamespec);
            }
            else if (!strncmp(p, "${(}", 4)) {
                p += 4;
                s += '(';
            }
            else if (!strncmp(p, "${)}", 4)) {
                p += 4;
                s += ')';
            }
            else if (!strncmp(p, "${const}", 8)) {
                p += 8;

                if (cbCppConst->isChecked())
                    s += " const";
            }
            else if (!strncmp(p, "${volatile}", 11)) {
                p += 11;

                if (cbCppVolatile->isChecked())
                    s += " volatile";
            }
            else if (!strncmp(p, "${throw}", 8)) {
                p += 8;
                manage_cpp_exceptions(s);
            }
            else if (!strncmp(p, "${staticnl}", 11)) {
                p += 11;

                if (classoperation_cb->isChecked()) {
                    s += '\n';
                    s += indent;
                }
                else
                    s += " ";
            }
            else if (sscanf(p, "${t%u}", &rank) == 1) {
                manage_cpp_type(rank, s);
                p = strchr(p, '}') + 1;
            }
            else if (sscanf(p, "${p%u}", &rank) == 1) {
                manage_var(rank, s);
                p = strchr(p, '}') + 1;
            }
            else if (oper->is_get_or_set && !strncmp(p, "${stereotype}", 13)) {
                p += 13;
                // get/set with multiplicity > 1
                s += GenerationSettings::cpp_relationattribute_stereotype(oper->stereotype);
            }
            else if (oper->is_get_or_set && !strncmp(p, "${association}", 14)) {
                p += 14;
                // get/set with multiplicity > 1
                s += GenerationSettings::cpp_type(((BrowserOperation *) oper->browser_node)
                                                  ->get_of_association().get_type());
            }
            else if (*p == '\n') {
                s += *p++;

                if (*p && (*p != '#'))
                    s += indent;
            }
            else if (*p == '@')
            {
                manage_alias(oper->browser_node, p, s, kvtable);
            }
            else
                s += *p++;
        }

        cppTab->ui->editcppbody->setEnabled(def.find("${body}") != -1);
    }
    else
        cppTab->ui->editcppbody->setEnabled(FALSE);

    cppTab->ui->edCppDefActual->setText(s);

    forcegenbody_toggled(forcegenbody_cb->isChecked());	// update indent*body_cb
}

static QString Marker = " ---- header including this line will be automatically removed ----\n";

static QString add_profile(QString b, const char * comment = "//")
{
    return b.left(b.find("${body}")) + comment + Marker;
}

static QString remove_profile(QString b)
{
    int index = b.find(Marker);

    return (index == -1)
           ? b
           : b.mid(index + Marker.length());
}

void OperationDialog::cpp_edit_body()
{
    QString b;

    if (add_operation_profile())
        b = add_profile(cppTab->ui->edCppDefActual->text()) + cppbody;
    else
        b = cppbody;

    edit(b, edname->text().stripWhiteSpace() + "_body",
         oper, CppEdit, this,
         (preserve_bodies() && !forcegenbody_cb->isChecked())
         ? (post_edit) 0
         : (post_edit) post_cpp_edit_body,
         edits);
}

void OperationDialog::post_cpp_edit_body(OperationDialog * d, QString s)
{
    d->cppbody = (add_operation_profile()) ? remove_profile(s) : s;
}

// Java

void OperationDialog::java_finalsynchronized_toggled(bool)
{
    java_update_def();
}

void OperationDialog::manage_java_type(unsigned rank, QString & s)
{
    if (rank < table->nparams())
        s += get_java_name(the_type(table->type(rank), list, nodes));
    else {
        s += "${t";
        s += QString::number(rank);
        s += '}';
    }
}

void OperationDialog::manage_java_exceptions(QString & s)
{
    int index;
    int ne = etable->nexceptions();
    const char * sep = " throws ";

    for (index = 0; index != ne; index += 1) {
        s += sep;
        s += GenerationSettings::java_type(type(etable->type(index), list, nodes));
        sep = ", ";
    }

    if (index != 0)
        s += " ";
}

void OperationDialog::java_default_def()
{
    if (oper->is_get_or_set) {
        WrapperStr def;

        if (get_of_attr != 0)
            oper->update_java_get_of(def, get_of_attr->get_browser_node()->get_name(),
                                     get_of_attr->get_javadecl(),
                                     get_of_attr->get_multiplicity());
        else if (set_of_attr != 0)
            oper->update_java_set_of(def, set_of_attr->get_browser_node()->get_name(),
                                     set_of_attr->get_javadecl(),
                                     set_of_attr->get_multiplicity());
        else if (get_of_rel != 0) {
            if (is_rel_a)
                oper->update_java_get_of(def, get_of_rel->get_role_a(),
                                         get_of_rel->get_javadecl_a(),
                                         get_of_rel->get_multiplicity_a());
            else
                oper->update_java_get_of(def, get_of_rel->get_role_b(),
                                         get_of_rel->get_javadecl_b(),
                                         get_of_rel->get_multiplicity_b());
        }
        else {
            // set_of_rel != 0
            if (is_rel_a)
                oper->update_java_set_of(def, set_of_rel->get_role_a(),
                                         set_of_rel->get_javadecl_a(),
                                         set_of_rel->get_multiplicity_a());
            else
                oper->update_java_set_of(def, set_of_rel->get_role_b(),
                                         set_of_rel->get_javadecl_b(),
                                         set_of_rel->get_multiplicity_b());
        }

        edjavadef->setText(def);
    }
    else {
        QString s = oper->default_java_def(edname->text().stripWhiteSpace());
        QString params;
        int index;
        int nparams = (int) table->nparams();
        const char * sep;

        for (index = 0, sep = ""; index != nparams; index += 1, sep = ", ") {
            QString p;

            p.sprintf("%s${t%d} ${p%d}", sep, index, index);
            params += p;
        }

        if ((index = s.find("${)}")) != -1)
            s.insert(index, params);

        edjavadef->setText(s);
    }

    java_update_def();
}

void OperationDialog::java_unmapped_def()
{
    edjavadef->setText(QString());
    showjavadef->setText(QString());
}

static const char * bypass_body(const char * p)
{
    // p point to '{'

    p += 1;

    const char * pb = strstr(p, "${body}");

    if (pb != 0) {
        pb += 7;

        while (*pb)
            if (*pb++ == '}')
                return pb;
    }

    return p;
}

void OperationDialog::java_update_def()
{
    // do NOT write
    //	const char * p = edjavadef->text();
    // because the QString is immediatly destroyed !
    QString def = edjavadef->text();
    QString ste = GenerationSettings::java_class_stereotype(cl->get_stereotype());
    bool interf = (ste == "interface") || (ste == "@interface");
    bool nobody = (abstract_cb->isChecked() || interf);
    const char * p = def;
    const char * pp = 0;
    const char * afterparam = 0;
    QString indent = "";
    QString s;
    unsigned rank;

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
        else if (!strncmp(p, "${final}", 8)) {
            p += 8;

            if (javafinal_cb->isChecked())
                s += "final ";
        }
        else if (!strncmp(p, "${visibility}", 13)) {
            p += 13;

            if (uml_visibility.value() != UmlPackageVisibility)
                s += uml_visibility.state() + " ";
        }
        else if (!strncmp(p, "${static}", 9)) {
            p += 9;

            if (classoperation_cb->isChecked())
                s += "static ";
        }
        else if (!strncmp(p, "${abstract}", 11)) {
            p += 11;

            if (abstract_cb->isChecked() && !interf)
                s += "abstract ";
        }
        else if (!strncmp(p, "${synchronized}", 15)) {
            p += 15;

            if (synchronized_cb->isChecked())
                s += "synchronized ";
        }
        else if (!strncmp(p, "${type}", 7)) {
            p += 7;
            s += get_java_name(the_type(edreturn_type->currentText().stripWhiteSpace(),
                                        list, nodes));
        }
        else if (!strncmp(p, "${name}", 7)) {
            p += 7;
            s += compute_name(edjavanamespec);
        }
        else if (!strncmp(p, "${(}", 4)) {
            p += 4;
            s += '(';
        }
        else if (!strncmp(p, "${)}", 4)) {
            p += 4;
            afterparam = p;
            s += ')';
        }
        else if (!strncmp(p, "${throws}", 9)) {
            manage_java_exceptions(s);
            p += 9;
        }
        else if (!strncmp(p, "${staticnl}", 11)) {
            p += 11;

            if (classoperation_cb->isChecked()) {
                s += '\n';
                s += indent;
            }
            else
                s += " ";
        }
        else if (sscanf(p, "${t%u}", &rank) == 1) {
            manage_java_type(rank, s);
            p = strchr(p, '}') + 1;
        }
        else if (sscanf(p, "${p%u}", &rank) == 1) {
            manage_var(rank, s);
            p = strchr(p, '}') + 1;
        }
        else if (oper->is_get_or_set && !strncmp(p, "${stereotype}", 13)) {
            p += 13;
            // get/set with multiplicity > 1
            s += GenerationSettings::java_relationattribute_stereotype(oper->stereotype);
        }
        else if (oper->is_get_or_set && !strncmp(p, "${association}", 14)) {
            p += 14;
            // get/set with multiplicity > 1
            s += GenerationSettings::java_type(((BrowserOperation *) oper->browser_node)
                                               ->get_of_association().get_type());
        }
        else if (*p == '\n') {
            s += *p++;

            if (*p)
                s += indent;
        }
        else if ((*p == '{') && nobody) {
            if (afterparam != 0) {
                if ((ste == "@interface") &&
                    (strstr(afterparam, "default") != 0))
                    afterparam = 0;
                else {
                    s += ';';
                    p = bypass_body(p);
                    continue;
                }
            }

            s += *p++;
        }
        else if (!strncmp(p, "${@}", 4)) {
            p += 4;

            if (pp != 0)
                s += "${@}";
            else if (! javaannotation.isEmpty()) {
                pp = p;
                p = javaannotation;
            }
        }
        else if (*p == '@')
            manage_alias(oper->browser_node, p, s, kvtable);
        else
            s += *p++;
    }

    editjavabody->setEnabled(!nobody && (def.find("${body}") != -1));
    editjavaannotation->setEnabled(def.find("${@}") != -1);

    showjavadef->setText(s);

    forcegenbody_toggled(forcegenbody_cb->isChecked());	// update indent*body_cb
}

QString OperationDialog::java_decl(const BrowserOperation * op, bool withname,
                                   ShowContextMode mode)
{
    OperationData * d = (OperationData *) op->get_data();
    WrapperStr decl = d->java_def;

    remove_comments(decl);

    const char * p = decl;
    QString s;
    unsigned rank;

    while ((*p == ' ') || (*p == '\t'))
        p += 1;

    while (*p) {
        if (!strncmp(p, "${comment}", 10))
            p += 10;
        else if (!strncmp(p, "${description}", 14))
            p += 14;
        else if (!strncmp(p, "${final}", 8))
            p += 8;
        else if (!strncmp(p, "${visibility}", 13))
            p += 13;
        else if (!strncmp(p, "${static}", 9))
            p += 9;
        else if (!strncmp(p, "${abstract}", 11))
            p += 11;
        else if (!strncmp(p, "${synchronized}", 15))
            p += 15;
        else if (!strncmp(p, "${type}", 7)) {
            p += 7;
            s += get_java_name(d->return_type, mode);
        }
        else if (!strncmp(p, "${name}", 7)) {
            p += 7;
            s += op->compute_name(d->java_name_spec);
        }
        else if (!strncmp(p, "${(}", 4)) {
            p += 4;
            s += '(';
        }
        else if (!strncmp(p, "${)}", 4)) {
            p += 4;
            s += ')';
        }
        else if (!strncmp(p, "${throws}", 9))
            p += 9;
        else if (!strncmp(p, "${staticnl}", 11)) {
            p += 11;
            s += " ";
        }
        else if (sscanf(p, "${t%u}", &rank) == 1) {
            if (rank < d->nparams)
                s += GenerationSettings::java_type(d->params[rank]->get_type().get_type(mode));
            else {
                s += "${t";
                s += QString::number(rank);
                s += '}';
            }

            p = strchr(p, '}') + 1;
        }
        else if (sscanf(p, "${p%u}", &rank) == 1) {
            if (withname) {
                if (rank < d->nparams)
                    s += d->params[rank]->get_name();
                else {
                    s += "${p";
                    s += QString::number(rank);
                    s += '}';
                }
            }

            p = strchr(p, '}') + 1;
        }
        else if (!strncmp(p, "${@}", 4))
            p += 4;
        else if (d->is_get_or_set && !strncmp(p, "${stereotype}", 13)) {
            p += 13;
            // get/set with multiplicity > 1
            s += GenerationSettings::java_relationattribute_stereotype(d->stereotype);
        }
        else if (d->is_get_or_set && !strncmp(p, "${association}", 14)) {
            p += 14;
            // get/set with multiplicity > 1
            s += GenerationSettings::java_type(op->get_of_association().get_type());
        }
        else if (*p == '\n') {
            s += " ";

            do
                p += 1;

            while ((*p == ' ') || (*p == '\t'));
        }
        else if ((*p == '{') || (*p == ';'))
            break;
        else if (*p == '@')
            manage_alias(op, p, s,static_cast<KeyValuesTable*>(0));
        else
            s += *p++;
    }

    return s;
}

void OperationDialog::java_edit_body()
{
    QString b;

    if (add_operation_profile())
        b = add_profile(showjavadef->text()) + javabody;
    else
        b = javabody;

    edit(b, edname->text().stripWhiteSpace() + "_body",
         oper, JavaEdit, this,
         (preserve_bodies() && !forcegenbody_cb->isChecked())
         ? (post_edit) 0
         : (post_edit) post_java_edit_body,
         edits);
}

void OperationDialog::post_java_edit_body(OperationDialog * d, QString s)
{
    d->javabody = (add_operation_profile()) ? remove_profile(s) : s;
}

void OperationDialog::java_edit_annotation()
{
    AnnotationDialog dialog(this, javaannotation, !hasOkButton());

    if (dialog.exec() == QDialog::Accepted)
        java_update_def();
}

// Php

void OperationDialog::php_final_toggled(bool)
{
    php_update_def();
}

void OperationDialog::manage_php_type(unsigned rank, QString & s)
{
    if (rank < table->nparams())
        s += get_php_name(the_type(table->type(rank), list, nodes));
    else {
        s += "${t";
        s += QString::number(rank);
        s += '}';
    }
}

void OperationDialog::php_default_def()
{
    if (oper->is_get_or_set) {
        WrapperStr def;

        if (get_of_attr != 0)
            oper->update_php_get_of(def, get_of_attr->get_browser_node()->get_name(),
                                    get_of_attr->get_phpdecl());
        else if (set_of_attr != 0)
            oper->update_php_set_of(def, set_of_attr->get_browser_node()->get_name(),
                                    set_of_attr->get_phpdecl());
        else if (get_of_rel != 0) {
            if (is_rel_a)
                oper->update_php_get_of(def, get_of_rel->get_role_a(),
                                        get_of_rel->get_phpdecl_a());
            else
                oper->update_php_get_of(def, get_of_rel->get_role_b(),
                                        get_of_rel->get_phpdecl_b());
        }
        else {
            // set_of_rel != 0
            if (is_rel_a)
                oper->update_php_set_of(def, set_of_rel->get_role_a(),
                                        set_of_rel->get_phpdecl_a());
            else
                oper->update_php_set_of(def, set_of_rel->get_role_b(),
                                        set_of_rel->get_phpdecl_b());
        }

        edphpdef->setText(def);
    }
    else {
        QString ste = GenerationSettings::php_class_stereotype(cl->get_stereotype());
        QString s = oper->default_php_def(edname->text().stripWhiteSpace(),
                                          abstract_cb->isChecked() || (ste == "interface"));
        QString params;
        int index;
        int nparams = (int) table->nparams();
        const char * sep;

        for (index = 0, sep = ""; index != nparams; index += 1, sep = ", ") {
            QString p;

            p.sprintf("%s${p%d}${v%d}", sep, index, index);
            params += p;
        }

        if ((index = s.find("${)}")) != -1)
            s.insert(index, params);

        edphpdef->setText(s);
    }

    php_update_def();
}

void OperationDialog::php_unmapped_def()
{
    edphpdef->setText(QString());
    showphpdef->setText(QString());
}

void OperationDialog::php_update_def()
{
    // do NOT write
    //	const char * p = edphpdef->text();
    // because the QString is immediatly destroyed !
    QString def = edphpdef->text();
    QString ste = GenerationSettings::php_class_stereotype(cl->get_stereotype());
    bool interf = (ste == "interface");
    bool nobody = (abstract_cb->isChecked() || interf);
    const char * p = def;
    const char * pp = 0;
    QString indent = "";
    QString s;
    unsigned rank;

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
        else if (!strncmp(p, "${final}", 8)) {
            p += 8;

            if (phpfinal_cb->isChecked())
                s += "final ";
        }
        else if (!strncmp(p, "${visibility}", 13)) {
            p += 13;

            if (uml_visibility.value() != UmlPackageVisibility)
                s += uml_visibility.state() + " ";
        }
        else if (!strncmp(p, "${static}", 9)) {
            p += 9;

            if (classoperation_cb->isChecked())
                s += "static ";
        }
        else if (!strncmp(p, "${abstract}", 11)) {
            p += 11;

            if (abstract_cb->isChecked() && !interf)
                s += "abstract ";
        }
        else if (!strncmp(p, "${name}", 7)) {
            p += 7;
            s += compute_name(edphpnamespec);
        }
        else if (!strncmp(p, "${(}", 4)) {
            p += 4;
            s += '(';
        }
        else if (!strncmp(p, "${)}", 4)) {
            p += 4;
            // afterparam = p;
            s += ')';
        }
        else if (!strncmp(p, "${staticnl}", 11)) {
            p += 11;

            if (classoperation_cb->isChecked()) {
                s += '\n';
                s += indent;
            }
            else
                s += " ";
        }
        else if (sscanf(p, "${t%u}", &rank) == 1) {
            manage_php_type(rank, s);
            p = strchr(p, '}') + 1;
        }
        else if (sscanf(p, "${p%u}", &rank) == 1) {
            s += "$";
            manage_var(rank, s);
            p = strchr(p, '}') + 1;
        }
        else if (sscanf(p, "${v%u}", &rank) == 1) {
            manage_init(rank, s);
            p = strchr(p, '}') + 1;
        }
        else if (*p == '\n') {
            s += *p++;

            if (*p)
                s += indent;
        }
        else if ((*p == '{') && nobody) {
            s += ";";
            break;
        }
        else if (*p == '@')
            manage_alias(oper->browser_node, p, s, kvtable);
        else if (!strncmp(p, "${type}", 7)) {
            // for comment
            p += 7;
            s += get_php_name(the_type(edreturn_type->currentText().stripWhiteSpace(),
                                       list, nodes));
        }
        else
            s += *p++;
    }

    editphpbody->setEnabled(!nobody && (def.find("${body}") != -1));

    showphpdef->setText(s);

    forcegenbody_toggled(forcegenbody_cb->isChecked());	// update indent*body_cb
}

QString OperationDialog::php_decl(const BrowserOperation * op, bool withname,
                                  ShowContextMode mode)
{
    OperationData * d = (OperationData *) op->get_data();
    WrapperStr decl = d->php_def;

    remove_comments(decl);

    int index = decl.find("function ");

    if (index != -1)
        decl.remove(index, 9);

    const char * p = decl;
    WrapperStr s;
    unsigned rank;

    while ((*p == ' ') || (*p == '\t'))
        p += 1;

    while (*p) {
        if (!strncmp(p, "${comment}", 10))
            p += 10;
        else if (!strncmp(p, "${description}", 14))
            p += 14;
        else if (!strncmp(p, "${final}", 8))
            p += 8;
        else if (!strncmp(p, "${visibility}", 13))
            p += 13;
        else if (!strncmp(p, "${static}", 9))
            p += 9;
        else if (!strncmp(p, "${abstract}", 11))
            p += 11;
        else if (!strncmp(p, "${name}", 7)) {
            p += 7;
            s += op->compute_name(d->php_name_spec);
        }
        else if (!strncmp(p, "${(}", 4)) {
            p += 4;
            s += '(';
        }
        else if (!strncmp(p, "${)}", 4)) {
            p += 4;
            s += ')';
        }
        else if (!strncmp(p, "${staticnl}", 11)) {
            p += 11;
            s += " ";
        }
        else if (sscanf(p, "${t%u}", &rank) == 1) {
            if (rank < d->nparams)
                s += d->params[rank]->get_type().get_type(mode);
            else {
                s += "${t";
                s += QString::number(rank);
                s += '}';
            }

            p = strchr(p, '}') + 1;
        }
        else if (sscanf(p, "${p%u}", &rank) == 1) {
            if (withname) {
                if (rank < d->nparams)
                    s += WrapperStr("$") + d->params[rank]->get_name();
                else {
                    s += "${p";
                    s += QString::number(rank);
                    s += '}';
                }
            }

            p = strchr(p, '}') + 1;
        }
        else if (sscanf(p, "${v%u}", &rank) == 1) {
            if (rank < d->nparams) {
                WrapperStr v = d->params[rank]->get_default_value();

                if (!v.isEmpty())
                    s += " = " + v;
            }
            else {
                s += "${v";
                s += QString::number(rank);
                s += '}';
            }

            p = strchr(p, '}') + 1;
        }
        else if (*p == '\n') {
            s += " ";

            do
                p += 1;

            while ((*p == ' ') || (*p == '\t'));
        }
        else if ((*p == '{') || (*p == ';'))
            break;
        else if (*p == '@')
            manage_alias(op, p, s.GetInternalRef(), static_cast<KeyValuesTable*>(0));
        else if (!strncmp(p, "${type}", 7)) {
            p += 7;
            s += get_php_name(d->return_type, mode);
        }
        else
            s += *p++;
    }

    return s;
}

void OperationDialog::php_edit_body()
{
    QString b;

    if (add_operation_profile())
        b = add_profile(showphpdef->text()) + phpbody;
    else
        b = phpbody;

    edit(b, edname->text().stripWhiteSpace() + "_body",
         oper, PhpEdit, this,
         (preserve_bodies() && !forcegenbody_cb->isChecked())
         ? (post_edit) 0
         : (post_edit) post_php_edit_body,
         edits);
}

void OperationDialog::post_php_edit_body(OperationDialog * d, QString s)
{
    d->phpbody = (add_operation_profile()) ? remove_profile(s) : s;
}

bool OperationDialog::SaveData()
{
    BrowserClass* containingClass = static_cast<BrowserClass*>(oper->browser_node->get_container(UmlClass));
    QList<BrowserNode *>  passedNodes;
    bool goBack = true;
    QList<OperationData*> inheritanceSiblings;
    inheritanceSiblings = containingClass->CollectSameThroughInheritance(oper, passedNodes, goBack);
    bool propagateThroughInheritance = false;
    OperationData* operCopy = new OperationData(oper, (BrowserOperation*)oper->get_browser_node());

    SaveData(operCopy);
    bool equals = *oper == *operCopy;
    bool newst = operCopy->set_stereotype(fromUnicode(edstereotype->currentText().stripWhiteSpace()));
    delete operCopy;

    if(!inheritanceSiblings.isEmpty() && !equals)
    {
        // call messagebox to confirm propagation
        QMessageBox msg;
        msg.setWindowTitle(tr("Warning!"));
        msg.setText(tr("The Operation you are trying to change is\ninhertied from/to other classes.\n"
                       "Do you want to propagate the changes to it\n through the inheritance tree?"));
        QPushButton* above = msg.addButton(tr("Above"), QMessageBox::ActionRole);
        QPushButton* whole = msg.addButton(tr("Everywhere"), QMessageBox::ActionRole);
        Q_UNUSED(whole);
        QPushButton* dont = msg.addButton(tr("Do not propagate"), QMessageBox::ActionRole);
        QPushButton* cancel = msg.addButton(tr("Cancel"), QMessageBox::ActionRole);
        msg.show();


        int yPos = QCursor::pos().y();
        if((QApplication::desktop()->height() - QCursor::pos().y()) < msg.height())
            yPos=(QApplication::desktop()->height() - msg.height() - 120);
        msg.move(QCursor::pos().x() + 15, yPos);

        //msg.move(QCursor::pos());
        msg.exec();
        if(msg.clickedButton() == cancel)
            return false;
        if(msg.clickedButton() != dont)
            propagateThroughInheritance = true;
        if(msg.clickedButton() == above)
        {
            goBack = false;
            passedNodes.clear();
            inheritanceSiblings.clear();
            inheritanceSiblings = containingClass->CollectSameThroughInheritance(oper,passedNodes, goBack);
        }
    }
    SaveData(oper);
    // user
    kvtable->updateNodeFromThis(oper->get_browser_node());

    ProfiledStereotypes::modified(oper->get_browser_node(), newst);

    oper->get_browser_node()->modified();
    oper->get_browser_node()->package_modified();
    oper->modified();

    if(propagateThroughInheritance && !equals)
    {
        QList<const OperationData*> passed;
        for(OperationData* siblingOper : inheritanceSiblings)
        {
            passed.append(oper);
            siblingOper->PropagateFrom(oper, goBack, passed);
            ProfiledStereotypes::modified(siblingOper->browser_node, newst);
            siblingOper->browser_node->modified();
            siblingOper->browser_node->package_modified();
            siblingOper->modified();
        }
    }
    return true;
}

bool OperationDialog::SaveData(OperationData *oper)
{

    if (!check_edits(edits) || !kvtable->check_unique())
        return true;

    BrowserNode * bn = oper->browser_node;
    QString s = edname->text().stripWhiteSpace();

    if ((s != oper->name()) &&
        ((BrowserNode *) bn->parent())->wrong_child_name(s, UmlOperation,
                bn->allow_spaces(),
                bn->allow_empty()))
        msg_critical(TR("Error"), s + TR("\n\nillegal name or already used"));
    else
    {
        bn->set_name(s);
        if (!oper->is_get_or_set)
        {
            AType t;

            s = edreturn_type->currentText().stripWhiteSpace();

            if (! s.isEmpty()) {
                int index = list.findIndex(edreturn_type->currentText());

                if (index >= 0)
                    t.type = (BrowserClass *) nodes.at(index);
                else
                    t.explicit_type = s;
            }

            oper->set_return_type(t);

            bool newst = oper->set_stereotype(fromUnicode(edstereotype->currentText().stripWhiteSpace()));
            Q_UNUSED(newst);
        }

        oper->uml_visibility = uml_visibility.value();

        oper->isa_class_operation = classoperation_cb->isChecked();
        oper->set_is_abstract(abstract_cb->isChecked());

        oper->force_body_gen = forcegenbody_cb->isChecked();

        table->update(oper, nodes);
        etable->update(oper, nodes);

        bn->set_comment(comment->text());
        UmlWindow::update_comment_if_needed(bn);

        oper->constraint = constraint->stripWhiteSpaceText();

        // C++

        if (cpp_undef)
        {
            oper->cpp_decl = QString();
            oper->cpp_def.assign(QString(), TRUE);

            if (!oldcppbody.isEmpty())
                oper->new_body(QString(), 'c');
        }
        else {
            if (oper->is_get_or_set)
            {
                oper->cpp_name_spec = cppTab->ui->leCppNamespec->text().stripWhiteSpace();
                oper->cpp_body.length = 0;
                oper->cpp_get_set_frozen = cppTab->ui->cbCppFrozen->isChecked();
            }
            else
                oper->cpp_indent_body = cppTab->ui->cbIndentCppBody->isChecked();

            if (!abstract_cb->isChecked() &&
                (cppTab->ui->edCppDefProto->text().find("${body}") != -1))
            {
                if (cppbody != oldcppbody)
                    oper->new_body(cppbody, 'c');
            }
            else if (!oldcppbody.isEmpty())
                oper->new_body(QString(), 'c');


            oper->cpp_visibility = cpp_visibility.value();

            oper->cpp_const = cbCppConst->isChecked();
            oper->is_volatile = cbCppVolatile->isChecked();
            oper->cpp_friend = cbCppFriend->isChecked();
            oper->cpp_virtual = cbCppVirtual->isChecked();
            oper->cpp_inline = cbCppInline->isChecked();
            oper->cpp_default = cppTab->ui->cbCppDefaulted->isChecked();
            oper->cpp_delete = cppTab->ui->cbCppDeleted->isChecked();
            oper->cpp_override = cppTab->ui->cbCppOverride->isChecked();
            oper->cpp_final = cppTab->ui->cbCppFinal->isChecked();
            oper->cpp_decl = cppTab->ui->edCppDeclProto->text();
            oper->cpp_def.assign(cppTab->ui->edCppDefProto->text(),
                                 abstract_cb->isChecked() ||
                                 (cppTab->ui->edCppDefProto->text().find("${body}") != -1));
        }

        // java

        if (java_undef) {
            oper->java_def.assign(QString(), TRUE);

            if (!oldjavabody.isEmpty())
                oper->new_body(QString(), 'j');
        }
        else {
            if (oper->is_get_or_set) {
                oper->java_name_spec = edjavanamespec->text().stripWhiteSpace();
                oper->java_get_set_frozen = javafrozen_cb->isChecked();
            }
            else
                oper->java_indent_body = indentjavabody_cb->isChecked();

            QString ste = GenerationSettings::java_class_stereotype(cl->get_stereotype());
            bool interf = (ste == "interface") || (ste == "@interface");

            if (!abstract_cb->isChecked() && !interf &&
                (edjavadef->text().find("${body}") != -1)) {
                if (javabody != oldjavabody)
                    oper->new_body(javabody, 'j');
            }
            else if (!oldjavabody.isEmpty())
                oper->new_body(QString(), 'j');

            oper->java_final = javafinal_cb->isChecked();
            oper->java_synchronized = synchronized_cb->isChecked();
            oper->java_def.assign(edjavadef->text(),
                                  abstract_cb->isChecked() || interf ||
                                  (edjavadef->text().find("${body}") != -1));

            oper->java_annotation = javaannotation;
        }

        // php

        if (php_undef) {
            oper->php_def.assign(QString(), TRUE);

            if (!oldphpbody.isEmpty())
                oper->new_body(QString(), 'p');
        }
        else {
            if (oper->is_get_or_set) {
                oper->php_name_spec = edphpnamespec->text().stripWhiteSpace();
                oper->php_get_set_frozen = phpfrozen_cb->isChecked();
            }
            else
                oper->php_indent_body = indentphpbody_cb->isChecked();

            QString ste = GenerationSettings::php_class_stereotype(cl->get_stereotype());
            bool interf = (ste == "interface");

            if (!abstract_cb->isChecked() && !interf &&
                (edphpdef->text().find("${body}") != -1)) {
                if (phpbody != oldphpbody)
                    oper->new_body(phpbody, 'p');
            }
            else if (!oldphpbody.isEmpty())
                oper->new_body(QString(), 'p');

            oper->php_final = phpfinal_cb->isChecked();
            oper->php_def.assign(edphpdef->text(),
                                 abstract_cb->isChecked() || interf ||
                                 (edphpdef->text().find("${body}") != -1));
        }

        // python

        if (python_undef) {
            oper->python_def.assign(QString(), TRUE);

            if (!oldpythonbody.isEmpty())
                oper->new_body(QString(), 'y');
        }
        else {
            if (oper->is_get_or_set) {
                oper->python_name_spec = edpythonnamespec->text().stripWhiteSpace();
                oper->python_get_set_frozen = pythonfrozen_cb->isChecked();
            }
            else
                oper->python_indent_body = indentpythonbody_cb->isChecked();

            // rmq : abstractmethod have body !
            if (edpythondef->text().find("${body}") != -1) {
                if (pythonbody != oldpythonbody)
                    oper->new_body(pythonbody, 'y');
            }
            else if (!oldpythonbody.isEmpty())
                oper->new_body(QString(), 'y');

            oper->python_def.assign(edpythondef->text(),
                                    // rmq : abstractmethod have body !
                                    (edpythondef->text().find("${body}") != -1));

            oper->python_decorator = pythondecorator;
        }

        // idl

        if (idl_undef)
            oper->idl_decl = QString();
        else {
            if (oper->is_get_or_set) {
                oper->idl_name_spec = edidlnamespec->text().stripWhiteSpace();
                oper->idl_get_set_frozen = idlfrozen_cb->isChecked();
            }

            oper->idl_oneway = oneway_cb->isChecked();
            oper->idl_decl = edidldecl->text();
        }


    }
    return true;
}

void OperationDialog::php_edit_param()
{
    QString form = edphpdef->text();
    int index;

    if (((index = form.find("${(}")) != 0) &&
        (form.find("${)}", index + 4) != 0)) {
        PhpParamsDialog d(this, table, edphpdef);

        if (d.exec() == QDialog::Accepted)
            php_update_def();
    }
    else
        msg_warning("Douml", TR("wrong specification"));
}

// Python

void OperationDialog::manage_python_type(unsigned rank, QString & s, bool in_param)
{
    if (rank < table->nparams()) {
        QString t = get_python_name(the_type(table->type(rank), list, nodes));

        if (!t.isEmpty()) {
            if (in_param)
                s += ": " + t;
            else
                s += t;
        }
    }
    else {
        s += "${t";
        s += QString::number(rank);
        s += '}';
    }
}

void OperationDialog::python_default_def()
{
    if (oper->is_get_or_set) {
        WrapperStr def;

        if (get_of_attr != 0)
            oper->update_python_get_of(def, get_of_attr->get_browser_node()->get_name(),
                                       get_of_attr->get_pythondecl(),
                                       get_of_attr->get_isa_class_attribute());
        else if (set_of_attr != 0)
            oper->update_python_set_of(def, set_of_attr->get_browser_node()->get_name(),
                                       set_of_attr->get_pythondecl(),
                                       set_of_attr->get_isa_class_attribute());
        else if (get_of_rel != 0) {
            if (is_rel_a)
                oper->update_python_get_of(def, get_of_rel->get_role_a(),
                                           get_of_rel->get_pythondecl_a(),
                                           get_of_rel->get_isa_class_relation_a());
            else
                oper->update_python_get_of(def, get_of_rel->get_role_b(),
                                           get_of_rel->get_pythondecl_b(),
                                           get_of_rel->get_isa_class_relation_b());
        }
        else {
            // set_of_rel != 0
            if (is_rel_a)
                oper->update_python_set_of(def, set_of_rel->get_role_a(),
                                           set_of_rel->get_pythondecl_a(),
                                           set_of_rel->get_isa_class_relation_a());
            else
                oper->update_python_set_of(def, set_of_rel->get_role_b(),
                                           set_of_rel->get_pythondecl_b(),
                                           set_of_rel->get_isa_class_relation_b());
        }

        edpythondef->setText(def);
    }
    else {
        // rmq : abstractmethod have body !
        QString s = oper->default_python_def(edname->text().stripWhiteSpace());
        QString params;
        int index;
        int nparams = (int) table->nparams();
        const char * sep;

        if (GenerationSettings::python_default_operation3()) {
            for (index = 0, sep = ""; index != nparams; index += 1, sep = ", ") {
                QString p;

                p.sprintf("%s${p%d}${t%d}${v%d}", sep, index, index, index);
                params += p;
            }
        }
        else {
            for (index = 0, sep = ""; index != nparams; index += 1, sep = ", ") {
                QString p;

                p.sprintf("%s${p%d}${v%d}", sep, index, index);
                params += p;
            }
        }

        int index2 = -1;

        if (((index = s.find("${(}") + 4) != (-1 + 4)) &&
            ((index2 = s.find("${)}", index)) != -1)) {
            int index3 = s.find("${type}", index2 + 4);
            int index4 = s.find(":", index2 + 4);

            if (GenerationSettings::python_default_operation3()) {
                if ((index3 == -1) || (index3 > index4))
                    s.insert(index2 + 4, "${type}");
            }
            else if ((index3 != -1) && (index3 < index4))
                s.remove(index3, 7);

            if (index2 != index)
                s.remove(index, index2 - index);

            s.insert(index, params);
        }

        edpythondef->setText(s);
    }

    python_update_def();
}

void OperationDialog::python_unmapped_def()
{
    edpythondef->setText(QString());
    showpythondef->setText(QString());
}

void OperationDialog::manage_decorators(QString & s, QString indent,
                                        BooL & indent_needed)
{
    if (! pythondecorator.isEmpty()) {
        int index = 0;
        int index2;

        while ((index2 = pythondecorator.find("\n", index)) != -1) {
            if (indent_needed)
                s += indent;
            else
                indent_needed = TRUE;

            s += pythondecorator.mid(index, index2 + 1 - index);
            index = index2 + 1;
        }

        if (index != (int) pythondecorator.length()) {
            if (indent_needed) {
                s += indent;
                indent_needed = FALSE;
            }

            s += pythondecorator.mid(index);
        }
    }
}

void OperationDialog::python_update_def()
{
    // do NOT write
    //	const char * p = edpythondef->text();
    // because the QString is immediatly destroyed !
    QString def = edpythondef->text();
    const char * p = def;

    if (p == 0)
        return;

    bool nobody = abstract_cb->isChecked();
    const char * pp = 0;
    BooL indent_needed = FALSE;
    bool in_params = FALSE;
    QString indent;
    QString saved_indent = indent;
    QString indent_step =
        GenerationSettings::python_get_indent_step();
    QString s;
    unsigned rank;

    for (;;) {
        if (*p == 0) {
            if (pp == 0)
                break;

            // comment management done
            p = pp;

            if (*p == 0)
                break;

            pp = 0;
            indent = saved_indent;
        }

        if (!strncmp(p, "${comment}", 10))
            manage_python_comment(comment->text(), p, pp);
        else if (!strncmp(p, "${description}", 14))
            manage_python_description(comment->text(), p, pp);
        else if (!strncmp(p, "${docstring}", 12))
            manage_python_docstring(comment->text(), p, pp, indent_needed, indent, saved_indent);
        else if (!strncmp(p, "${static}", 9)) {
            p += 9;

            if (classoperation_cb->isChecked()) {
                if (indent_needed)
                    s += "\n";
                else
                    indent_needed = TRUE;

                s += "@staticmethod\n";
            }
        }
        else if (!strncmp(p, "${abstract}", 11)) {
            p += 11;

            if (abstract_cb->isChecked()) {
                if (indent_needed)
                    s += "\n";
                else
                    indent_needed = TRUE;

                s += "@abstractmethod\n";
            }
        }
        else if (!strncmp(p, "${name}", 7)) {
            if (indent_needed) {
                indent_needed = FALSE;
                s += indent;
            }

            p += 7;
            s += compute_name(edpythonnamespec);
        }
        else if (!strncmp(p, "${class}", 8)) {
            if (indent_needed) {
                indent_needed = FALSE;
                s += indent;
            }

            p += 8;
            s += cl->get_browser_node()->get_name();
        }
        else if (!strncmp(p, "${(}", 4)) {
            p += 4;
            s += '(';
            in_params = TRUE;
        }
        else if (!strncmp(p, "${)}", 4)) {
            p += 4;
            // afterparam = p;
            s += ')';
            in_params = FALSE;
        }
        else if (!strncmp(p, "${@}", 4)) {
            manage_decorators(s, indent, indent_needed);
            p += 4;
        }
        else if (sscanf(p, "${t%u}", &rank) == 1) {
            manage_python_type(rank, s, in_params);
            p = strchr(p, '}') + 1;
        }
        else if (sscanf(p, "${p%u}", &rank) == 1) {
            manage_var(rank, s);
            p = strchr(p, '}') + 1;
        }
        else if (sscanf(p, "${v%u}", &rank) == 1) {
            manage_init(rank, s);
            p = strchr(p, '}') + 1;
        }
        else if (*p == '@')
            manage_alias(oper->browser_node, p, s, kvtable);
        else if (!strncmp(p, "${body}", 7)) {
            if (edname->text().stripWhiteSpace() == "__init__")
                s += ClassDialog::python_instance_att_rel(cl->get_browser_node());

            s += "${body}";
            p += 7;
            indent_needed = FALSE;
        }
        else if (!strncmp(p, "${type}", 7)) {
            p += 7;

            QString t = get_python_name(the_type(edreturn_type->currentText().stripWhiteSpace(),
                                                 list, nodes));

            if (!t.isEmpty())
                s += " -> " + t;
        }
        else if (oper->is_get_or_set && !strncmp(p, "${association}", 14)) {
            p += 14;

            if (indent_needed) {
                indent_needed = FALSE;
                s += indent;
            }

            // get/set with multiplicity > 1
            s += ((BrowserOperation *) oper->browser_node)
                 ->get_of_association().get_type();
        }
        else {
            if (indent_needed) {
                indent_needed = FALSE;
                s += indent;
            }

            switch (*p) {
            case ':':
                if (pp == 0) {
                    indent += indent_step;
                    saved_indent = indent;
                    indent_step = "";
                }

                break;

            case '\n':
                indent_needed = TRUE;
                break;
            }

            s += *p++;
        }
    }

    editpythonbody->setEnabled(!nobody && (def.find("${body}") != -1));
    editpythondecorator->setEnabled(def.find("${@}") != -1);

    showpythondef->setText(s);

    forcegenbody_toggled(forcegenbody_cb->isChecked());	// update indent*body_cb
}

QString OperationDialog::python_decl(const BrowserOperation * op, bool withname,
                                     ShowContextMode mode)
{
    OperationData * d = (OperationData *) op->get_data();
    WrapperStr decl = d->python_def;
    bool in_params = FALSE;

    remove_python_comments(decl.GetInternalRef());

    int index = decl.find("def ");
    WrapperStr s;

    if (index == -1)
        return s;

    decl.remove(0, index + 4);

    const char * p = decl;
    unsigned rank;

    while (*p) {
        if (!strncmp(p, "${comment}", 10))
            p += 10;
        else if (!strncmp(p, "${description}", 14))
            p += 14;
        else if (!strncmp(p, "${docstring}", 12))
            p += 12;
        else if (!strncmp(p, "${static}", 9))
            p += 9;
        else if (!strncmp(p, "${abstract}", 11))
            p += 11;
        else if (!strncmp(p, "${name}", 7)) {
            p += 7;
            s += op->compute_name(d->python_name_spec);
        }
        else if (!strncmp(p, "${(}", 4)) {
            p += 4;
            s += '(';
            in_params = TRUE;
        }
        else if (!strncmp(p, "${)}", 4)) {
            p += 4;
            s += ')';

            if (!strncmp(p, "${type}", 7)) {
                QString t = get_python_name(d->return_type, mode);

                if (!t.isEmpty())
                    s += " -> " + t;
            }

            break;
        }
        else if (sscanf(p, "${t%u}", &rank) == 1) {
            if (rank < d->nparams) {
                QString t = d->params[rank]->get_type().get_type(mode);

                if (!t.isEmpty()) {
                    if (in_params)
                        s += ": " + t;
                    else
                        s += t;
                }
            }
            else {
                s += "${t";
                s += QString::number(rank);
                s += '}';
            }

            p = strchr(p, '}') + 1;
        }
        else if (sscanf(p, "${p%u}", &rank) == 1) {
            if (withname) {
                if (rank < d->nparams)
                    s += d->params[rank]->get_name();
                else {
                    s += "${p";
                    s += QString::number(rank);
                    s += '}';
                }
            }

            p = strchr(p, '}') + 1;
        }
        else if (sscanf(p, "${v%u}", &rank) == 1) {
            if (rank < d->nparams) {
                WrapperStr v = d->params[rank]->get_default_value();

                if (!v.isEmpty())
                    s += " = " + v;
            }
            else {
                s += "${v";
                s += QString::number(rank);
                s += '}';
            }

            p = strchr(p, '}') + 1;
        }
        else if (d->is_get_or_set && !strncmp(p, "${association}", 14)) {
            p += 14;
            // get/set with multiplicity > 1
            s += op->get_of_association().get_type();
        }
        else if (*p == '@')
            manage_alias(op, p, s.GetInternalRef(), static_cast<KeyValuesTable*>(0));
        else if (!strncmp(p, "${type}", 7)) {
            // probably in comment
            p += 7;
        }
        else
            s += *p++;
    }

    return s;
}

void OperationDialog::python_edit_body()
{
    QString b;

    if (add_operation_profile())
        b = add_profile(showpythondef->text(), "#") + pythonbody;
    else
        b = pythonbody;

    edit(b, edname->text().stripWhiteSpace() + "_body",
         oper, PythonEdit, this,
         (preserve_bodies() && !forcegenbody_cb->isChecked())
         ? (post_edit) 0
         : (post_edit) post_python_edit_body,
         edits);
}

void OperationDialog::post_python_edit_body(OperationDialog * d, QString s)
{
    d->pythonbody = (add_operation_profile()) ? remove_profile(s) : s;
}

void OperationDialog::python_edit_param()
{
    QString form = edpythondef->text();
    int index;

    if (((index = form.find("${(}")) != 0) &&
        (form.find("${)}", index + 4) != 0)) {
        PythonParamsDialog d(this, table, edpythondef);

        if (d.exec() == QDialog::Accepted)
            python_update_def();
    }
    else
        msg_warning("Douml", TR("wrong specification"));
}

void OperationDialog::python_edit_decorator()
{
    DecoratorDialog dialog(this, pythondecorator, !hasOkButton());

    if (dialog.exec() == QDialog::Accepted)
        python_update_def();
}

// Idl

void OperationDialog::oneway_toggled(bool)
{
    idl_update_decl();
}

void OperationDialog::manage_idl_type(unsigned rank, QString & s)
{
    if (rank < table->nparams())
        s += get_idl_name(the_type(table->type(rank), list, nodes));
    else {
        s += "${t";
        s += QString::number(rank);
        s += '}';
    }
}

void OperationDialog::manage_dir(unsigned rank, QString & s)
{
    if (rank < table->nparams()) {
        switch (table->dir(rank)) {
        case UmlIn:
            s += "in";
            break;

        case UmlOut:
            s += "out";
            break;

        default:
            // can't be return
            s += "inout";
        }
    }
    else {
        s += "${d";
        s += QString::number(rank);
        s += '}';
    }
}

void OperationDialog::manage_idl_exceptions(QString & s)
{
    int index;
    int ne = etable->nexceptions();
    const char * sep = " raises (";

    for (index = 0; index != ne; index += 1) {
        s += sep;
        s += GenerationSettings::idl_type(type(etable->type(index), list, nodes));
        sep = ", ";
    }

    if (index != 0)
        s += ')';
}

void OperationDialog::idl_default_def()
{
    if (oper->is_get_or_set) {
        WrapperStr decl;

        if (get_of_attr != 0)
            oper->update_idl_get_of(decl, get_of_attr->get_idldecl(),
                                    get_of_attr->get_multiplicity());
        else if (set_of_attr != 0)
            oper->update_idl_set_of(decl, set_of_attr->get_idldecl(),
                                    set_of_attr->get_multiplicity());
        else if (get_of_rel != 0) {
            if (is_rel_a)
                oper->update_idl_get_of(decl, get_of_rel->get_idldecl_a(),
                                        get_of_rel->get_multiplicity_a());
            else
                oper->update_idl_get_of(decl, get_of_rel->get_idldecl_b(),
                                        get_of_rel->get_multiplicity_b());
        }
        else {
            // set_of_rel != 0
            if (is_rel_a)
                oper->update_idl_set_of(decl, set_of_rel->get_idldecl_a(),
                                        set_of_rel->get_multiplicity_a());
            else
                oper->update_idl_set_of(decl, set_of_rel->get_idldecl_b(),
                                        set_of_rel->get_multiplicity_b());
        }

        edidldecl->setText(decl);
    }
    else {
        QString s = oper->default_idl_decl(edname->text().stripWhiteSpace());
        QString params;
        int index;
        int nparams = (int) table->nparams();
        const char * sep;

        for (index = 0, sep = ""; index != nparams; index += 1, sep = ", ") {
            QString p;

            p.sprintf("%s${d%d} ${t%d} ${p%d}", sep, index, index, index);
            params += p;
        }

        if ((index = s.find("${)}")) != -1)
            s.insert(index, params);

        edidldecl->setText(s);
    }

    idl_update_decl();
}

void OperationDialog::idl_unmapped_def()
{
    edidldecl->setText(QString());
    showidldecl->setText(QString());
}

void OperationDialog::idl_update_decl()
{
    // do NOT write
    //	const char * p = edidldecl->text();
    // because the QString is immediatly destroyed !
    QString def = edidldecl->text();
    const char * p = def;
    const char * pp = 0;
    QString indent = "";
    QString s;
    unsigned rank;

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
            manage_comment(comment->text(), p, pp, FALSE);
        else if (!strncmp(p, "${description}", 14))
            manage_description(comment->text(), p, pp);
        else if (!strncmp(p, "${oneway}", 9)) {
            p += 9;

            if (oneway_cb->isChecked())
                s += "oneway ";
        }
        else if (!strncmp(p, "${type}", 7)) {
            p += 7;
            s += get_idl_name(the_type(edreturn_type->currentText().stripWhiteSpace(),
                                       list, nodes));
        }
        else if (!strncmp(p, "${name}", 7)) {
            p += 7;
            s += compute_name(edidlnamespec);
        }
        else if (!strncmp(p, "${(}", 4)) {
            p += 4;
            s += '(';
        }
        else if (!strncmp(p, "${)}", 4)) {
            p += 4;
            s += ')';
        }
        else if (sscanf(p, "${d%u}", &rank) == 1) {
            manage_dir(rank, s);
            p = strchr(p, '}') + 1;
        }
        else if (sscanf(p, "${t%u}", &rank) == 1) {
            manage_idl_type(rank, s);
            p = strchr(p, '}') + 1;
        }
        else if (sscanf(p, "${p%u}", &rank) == 1) {
            manage_var(rank, s);
            p = strchr(p, '}') + 1;
        }
        else if (oper->is_get_or_set && !strncmp(p, "${stereotype}", 13)) {
            p += 13;
            // get/set with multiplicity > 1
            s += GenerationSettings::idl_relationattribute_stereotype(oper->stereotype);
        }
        else if (oper->is_get_or_set && !strncmp(p, "${association}", 14)) {
            p += 14;
            // get/set with multiplicity > 1
            s += GenerationSettings::idl_type(((BrowserOperation *) oper->browser_node)
                                              ->get_of_association().get_type());
        }
        else if (!strncmp(p, "${raisesnl}", 11)) {
            if (etable->nexceptions() != 0) {
                s += '\n';
                s += indent;
                s += "  ";
            }

            p += 11;
        }
        else if (!strncmp(p, "${raises}", 9)) {
            manage_idl_exceptions(s);
            p += 9;
        }
        else if (*p == '\n') {
            s += *p++;

            if (*p)
                s += indent;
        }
        else if (*p == '@')
            manage_alias(oper->browser_node, p, s, kvtable);
        else
            s += *p++;
    }

    showidldecl->setText(s);
}

QString OperationDialog::idl_decl(const BrowserOperation * op, bool withdir,
                                  bool withname, ShowContextMode mode)
{
    OperationData * d = (OperationData *) op->get_data();
    WrapperStr decl = d->idl_decl;

    remove_comments(decl);

    const char * p = decl;
    QString s;
    unsigned rank;

    while ((*p == ' ') || (*p == '\t'))
        p += 1;

    while (*p) {
        if (!strncmp(p, "${comment}", 10))
            p += 10;
        else if (!strncmp(p, "${description}", 14))
            p += 14;
        else if (!strncmp(p, "${oneway}", 9))
            p += 9;
        else if (!strncmp(p, "${type}", 7)) {
            p += 7;
            s += get_idl_name(d->return_type, mode);
        }
        else if (!strncmp(p, "${name}", 7)) {
            p += 7;
            s += op->compute_name(d->idl_name_spec);
        }
        else if (!strncmp(p, "${(}", 4)) {
            p += 4;
            s += '(';
        }
        else if (!strncmp(p, "${)}", 4)) {
            p += 4;
            s += ')';
        }
        else if (sscanf(p, "${d%u}", &rank) == 1) {
            if (withdir) {
                if (rank < d->nparams)
                    switch (d->params[rank]->get_dir()) {
                    case UmlIn:
                        s += "in";
                        break;

                    case UmlOut:
                        s += "out";
                        break;

                    default:
                        // can't be return
                        s += "inout";
                    }
                else {
                    s += "${d";
                    s += QString::number(rank);
                    s += '}';
                }
            }

            p = strchr(p, '}') + 1;
        }
        else if (sscanf(p, "${t%u}", &rank) == 1) {
            if (withname) {
                if (rank < d->nparams)
                    s += GenerationSettings::idl_type(d->params[rank]->get_type().get_type(mode));
                else {
                    s += "${t";
                    s += QString::number(rank);
                    s += '}';
                }
            }

            p = strchr(p, '}') + 1;
        }
        else if (sscanf(p, "${p%u}", &rank) == 1) {
            if (rank < d->nparams)
                s += d->params[rank]->get_name();
            else {
                s += "${p";
                s += QString::number(rank);
                s += '}';
            }

            p = strchr(p, '}') + 1;
        }
        else if (d->is_get_or_set && !strncmp(p, "${stereotype}", 13)) {
            p += 13;
            // get/set with multiplicity > 1
            s += GenerationSettings::idl_relationattribute_stereotype(d->stereotype);
        }
        else if (d->is_get_or_set && !strncmp(p, "${association}", 14)) {
            p += 14;
            // get/set with multiplicity > 1
            s += GenerationSettings::idl_type(op->get_of_association().get_type());
        }
        else if (!strncmp(p, "${raisesnl}", 11)) {
            p += 11;
            s += " ";
        }
        else if (!strncmp(p, "${raises}", 9))
            p += 9;
        else if (*p == '\n') {
            s += " ";

            do
                p += 1;

            while ((*p == ' ') || (*p == '\t'));
        }
        else if (*p == ';')
            break;
        else if (*p == '@')
            manage_alias(op, p, s, static_cast<KeyValuesTable*>(0));
        else
            s += *p++;
    }

    return s;
}


void OperationDialog::force_param(int rank, bool recompute)
{
    char t[16];
    char p[16];
    char v[16];
    QString s;

    sprintf(t, "${t%d}", rank);
    sprintf(p, "${p%d}", rank);
    sprintf(v, "${v%d}", rank);

    switch (unique) {
    case CppView: {
        QString theo =
            GenerationSettings::cpp(the_type(table->type(rank), list, nodes),
                                    table->dir(rank), rank);

        s = cppTab->ui->edCppDeclProto->text();

        if ((s.find(t) == -1) && (s.find(p) == -1)) {
            add_param(s, rank, theo + QString(v));
            cppTab->ui->edCppDeclProto->setText(s);
        }
        else if (recompute) {
            replace_param(s, rank, theo + QString(v));
            cppTab->ui->edCppDeclProto->setText(s);
        }

        s = cppTab->ui->edCppDefProto->text();

        if ((s.find(t) == -1) && (s.find(p) == -1)) {
            add_param(s, rank, theo);
            cppTab->ui->edCppDefProto->setText(s);
        }
        else if (recompute) {
            replace_param(s, rank, theo);
            cppTab->ui->edCppDefProto->setText(s);
        }
    }
    break;

    case JavaView:
        s = edjavadef->text();

        if ((s.find(t) == -1) && (s.find(p) == -1)) {
            add_param(s, rank, QString(t) + QString(" ") + QString(p));
            edjavadef->setText(s);
        }

        break;

    case PhpView:
        s = edphpdef->text();

        if (s.find(p) == -1) {
            add_param(s, rank, QString(p) + QString(v));
            edphpdef->setText(s);
        }

        break;

    case PythonView:
        s = edpythondef->text();

        if (s.find(p) == -1) {
            add_param(s, rank, QString(p) + QString(v));
            edpythondef->setText(s);
        }

        break;

    case IdlView:
        s = edidldecl->text();

        if ((s.find(t) == -1) && (s.find(p) == -1)) {
            char d[16];

            sprintf(d, "${d%d}", rank);
            add_param(s, rank, QString(d) + QString(" ") + QString(t) + QString(" ") + QString(p));
            edidldecl->setText(s);
        }

        break;

    default:
        break;
    }
}

void OperationDialog::add_param(QString & form, int rank, QString s)
{
    int index = OperationFuncs::param_begin(form, rank);

    if (index == -1)
        return;

    if (rank == 0) {
        // first param, index point to '}' ending ${(}
        if (table->nparams() != 1)
            s += ", ";

        index += 1;
    }
    else
        // index point to ',' or '$' starting ${)}
        s = ", " + s;

    form.insert(index, s);
}

void OperationDialog::replace_param(QString & form, int rank, QString s)
{
    int index_start = OperationFuncs::param_begin(form, rank);

    if (index_start == -1)
        return;

    const char * p = form;

    switch (p[index_start]) {
    case '$':
        // '$' starting ${)}, param not yet present, not first one,
        // not possible except if user remove it in language form
        form.insert(index_start, ", " + s);
        return;

    case ',':
        index_start += (p[index_start + 1] == ' ') ? 2 : 1;
        break;

    default:
        // first param, index point to '}' ending ${(}
        index_start += 1;
    }

    int index_sup = OperationFuncs::supOf(p, index_start);

    switch (p[index_sup]) {
    case ',':
    case '$': // ${)}
        break;

    default:
        // error
        return;
    }

    form.replace(index_start, index_sup - index_start, s);
}

void OperationDialog::insert_param(int rank)
{
    // just renumber
    switch (unique) {
    case CppView:
        insert_param(rank, cppTab->ui->edCppDeclProto);
        insert_param(rank, cppTab->ui->edCppDefProto);
        break;

    case JavaView:
        insert_param(rank, edjavadef);
        break;

    case PhpView:
        insert_param(rank, edphpdef);
        break;

    case PythonView:
        insert_param(rank, edpythondef);
        break;

    case IdlView:
        insert_param(rank, edidldecl);
        break;

    default:
        break;
    }
}

void OperationDialog::insert_param(int rank, MultiLineEdit * ed)
{
    // just renumber
    QString form = ed->text();

    OperationFuncs::renumber(form, rank, 1);
    ed->setText(form);
}

void OperationDialog::delete_param(int rank)
{
    // remove and renumber
    switch (unique) {
    case CppView:
        delete_param(rank, cppTab->ui->edCppDeclProto);
        delete_param(rank, cppTab->ui->edCppDefProto);
        break;

    case JavaView:
        delete_param(rank, edjavadef);
        break;

    case PhpView:
        delete_param(rank, edphpdef);
        break;

    case PythonView:
        delete_param(rank, edpythondef);
        break;

    case IdlView:
        delete_param(rank, edidldecl);
        break;

    default:
        break;
    }
}

QString OperationDialog::delete_param(int rank, MultiLineEdit * ed)
{
    // remove
    QString form = ed->text();
    int index = OperationFuncs::param_begin(form, rank);

    if (index == -1)
        return "";

    const char * p = form;
    int index_sup;

    switch (p[index]) {
    case '$':
        // '$' starting ${)}, param not yet present, not first one,
        // not possible except if user remove it in language form
        return "";

    case ',':
        index_sup = OperationFuncs::supOf(p, index + 1);
        break;

    default: // first param, index point to '}' ending ${(}
        index += 1;
        index_sup = OperationFuncs::supOf(p, index);
        break;
    }

    QString result;

    switch (p[index_sup]) {
    case ',':
        if (p[index] == ',') {
            if (p[index + 1] == ' ')
                result = form.mid(index + 2, index_sup - index - 2);
            else
                result = form.mid(index + 1, index_sup - index - 1);
        }
        else {
            // first param
            result = form.mid(index, index_sup - index);
            index_sup += (p[index_sup + 1] == ' ') ? 2 : 1;
        }

        break;

    case '$': // ${)}
        if (p[index] == ',') {
            if (p[index + 1] == ' ')
                result = form.mid(index + 2, index_sup - index - 2);
            else
                result = form.mid(index + 1, index_sup - index - 1);
        }
        else {
            // alone param
            result = form.mid(index, index_sup - index);
        }

        break;

    default:
        // error
        return "";
    }

    form.remove(index, index_sup - index);

    // renumber
    OperationFuncs::renumber(form, rank, -1);

    ed->setText(form);
    return result;
}

void OperationDialog::move_param(int old_rank, int new_rank)
{
    switch (unique) {
    case CppView:
        move_param(old_rank, new_rank, cppTab->ui->edCppDeclProto);
        move_param(old_rank, new_rank, cppTab->ui->edCppDefProto);
        break;

    case JavaView:
        move_param(old_rank, new_rank, edjavadef);
        break;

    case PhpView:
        move_param(old_rank, new_rank, edphpdef);
        break;

    case PythonView:
        move_param(old_rank, new_rank, edpythondef);
        break;

    case IdlView:
        move_param(old_rank, new_rank, edidldecl);
        break;

    default:
        break;
    }
}

void OperationDialog::move_param(int old_rank, int new_rank,
                                 MultiLineEdit * ed)
{
    QString s = delete_param(old_rank, ed);

    if (s.isEmpty())
        return;

    s = "${(}" + s + "${)}";
    OperationFuncs::renumber(s, old_rank, new_rank - old_rank, TRUE);

    QString form = ed->text();

    OperationFuncs::renumber(form, new_rank, 1);
    add_param(form, new_rank, s.mid(4, s.length() - 8));

    ed->setText(form);
}

//
// ParamTable
//

// copy/cut/paste
QString ParamsTable::dir_copy;
QString ParamsTable::name_copy;
QString ParamsTable::type_copy;
QString ParamsTable::default_value_copy;

static QStringList DirList;

ParamsTable::ParamsTable(OperationData * o, QWidget * parent,
                         const QStringList & list,
                         OperationDialog * d, bool isReadOnly)
    : MyTable(o->get_n_params() + 1, (isReadOnly) ? 4 : 5, parent),
      dialog(d), types(list)
{
    int index;
    int sup = o->get_n_params();

    setSorting(-1);
    setSelectionMode(NoSelection);	// single does not work
    setRowMovingEnabled(TRUE);
    horizontalHeader()->setLabel(0, TR("Direction"));
    horizontalHeader()->setLabel(1, TR("Name"));
    horizontalHeader()->setLabel(2, TR("Type"));
    horizontalHeader()->setLabel(3, TR("Default value"));

    if (isReadOnly) {
        for (index = 0; index < sup; index += 1) {
            setItem(index, 0, new TableItem(this, Q3TableItem::Never, stringify(o->get_param_dir(index))));
            setItem(index, 1, new TableItem(this, Q3TableItem::Never, o->get_param_name(index)));
            setItem(index, 2, new TableItem(this, Q3TableItem::Never, o->get_param_type(index).get_full_type()));
            setItem(index, 3, new TableItem(this, Q3TableItem::Never, o->get_param_default_value(index)));
        }
    }
    else {
        horizontalHeader()->setLabel(4, TR("do"));

        alltypes = GenerationSettings::basic_types();
        ((ClassData *)((BrowserNode *) o->get_browser_node()->parent())->get_data())
        ->addFormals(alltypes);

        for (QStringList::ConstIterator it = list.begin(); it != list.end(); ++it)
            alltypes.append(*it);

        if (DirList.count() == 0) {
            // Made DirList without be linked with the items order
            DirList.append(QString());
            DirList.append(QString());
            DirList.append(QString());
            DirList[UmlInOut] = stringify(UmlInOut);
            DirList[UmlIn] = stringify(UmlIn);
            DirList[UmlOut] = stringify(UmlOut);
            // can't be return
        }

        for (index = 0; index < sup; index += 1) {
            setItem(index, 0, new ComboItem(this, stringify(o->get_param_dir(index)), DirList, FALSE));
            setText(index, 1, o->get_param_name(index));
            setItem(index, 2, new ComboItem(this, o->get_param_type(index).get_full_type(), alltypes));
            setText(index, 3, o->get_param_default_value(index));
            setText(index, 4, QString());
        }

        setItem(index, 0, new ComboItem(this, stringify(UmlIn), DirList, FALSE));
        setText(index, 1, QString());
        setItem(index, 2, new ComboItem(this, QString(), alltypes));
        setText(index, 3, QString());
        setText(index, 4, QString());
    }

    adjustColumn(0);
    //setColumnStretchable (1, TRUE);
    horizontalHeader()->setResizeEnabled(TRUE, 1);
    setColumnStretchable(2, TRUE);
    //setColumnStretchable (3, TRUE);
    horizontalHeader()->setResizeEnabled(TRUE, 3);

    if (!isReadOnly) {
        adjustColumn(4);
        setColumnStretchable(4, FALSE);

        connect(this, SIGNAL(pressed(int, int, int, const QPoint &)),
                this, SLOT(button_pressed(int, int, int, const QPoint &)));
        connect(this, SIGNAL(valueChanged(int, int)),
                this, SLOT(value_changed(int, int)));
    }
}

void ParamsTable::update(OperationData *o, const QStringList &list, OperationDialog *, bool isReadOnly)
{

    if (!isReadOnly)
    {
        if(numCols() == 4)
            insertColumns(4);
        horizontalHeader()->setLabel(4, TR("do"));
    }
    else
    {
        if(numCols() == 5)
            removeColumn(4);
    }


    int index;
    int sup = o->get_n_params();

    setSorting(-1);
    setSelectionMode(NoSelection);	// single does not work
    setRowMovingEnabled(TRUE);

    disconnect(this, SIGNAL(pressed(int, int, int, const QPoint &)),
            this, SLOT(button_pressed(int, int, int, const QPoint &)));
    disconnect(this, SIGNAL(valueChanged(int, int)),
            this, SLOT(value_changed(int, int)));

    int rowCount = numRows();
    for (index = rowCount - 1; index > sup; index -= 1)
        removeRow(index);

    if (isReadOnly)
    {
        for (index = 0; index < sup; index += 1)
        {
            if(index == numRows())
                insertRows(numRows());

            setItem(index, 0, new TableItem(this, Q3TableItem::Never, stringify(o->get_param_dir(index))));
            setItem(index, 1, new TableItem(this, Q3TableItem::Never, o->get_param_name(index)));
            setItem(index, 2, new TableItem(this, Q3TableItem::Never, o->get_param_type(index).get_full_type()));
            setItem(index, 3, new TableItem(this, Q3TableItem::Never, o->get_param_default_value(index)));
        }
    }
    else
    {
        horizontalHeader()->setLabel(4, TR("do"));

        alltypes = GenerationSettings::basic_types();
        ((ClassData *)((BrowserNode *) o->get_browser_node()->parent())->get_data())
        ->addFormals(alltypes);

        for (QStringList::ConstIterator it = list.begin(); it != list.end(); ++it)
            alltypes.append(*it);

        if (DirList.count() == 0) {
            // Made DirList without be linked with the items order
            DirList.append(QString());
            DirList.append(QString());
            DirList.append(QString());
            DirList[UmlInOut] = stringify(UmlInOut);
            DirList[UmlIn] = stringify(UmlIn);
            DirList[UmlOut] = stringify(UmlOut);
            // can't be return
        }


        for (index = 0; index < sup; index += 1)
        {
            if(index == numRows())
                insertRows(numRows());

            setItem(index, 0, new ComboItem(this, stringify(o->get_param_dir(index)), DirList, FALSE));
            setText(index, 1, o->get_param_name(index));
            setItem(index, 2, new ComboItem(this, o->get_param_type(index).get_full_type(), alltypes));
            setText(index, 3, o->get_param_default_value(index));
            setText(index, 4, QString());
        }

        //int columnCount = numCols();
        if(index == numRows())
            insertRows(numRows());
        setItem(index, 0, new ComboItem(this, stringify(UmlIn), DirList, FALSE));
        setText(index, 1, QString());
        setItem(index, 2, new ComboItem(this, QString(), alltypes));
        setText(index, 3, QString());
        setText(index, 4, QString());



        connect(this, SIGNAL(pressed(int, int, int, const QPoint &)),
                this, SLOT(button_pressed(int, int, int, const QPoint &)));
        connect(this, SIGNAL(valueChanged(int, int)),
                this, SLOT(value_changed(int, int)));
    }

    adjustColumn(0);
    //setColumnStretchable (1, TRUE);
    horizontalHeader()->setResizeEnabled(TRUE, 1);
    setColumnStretchable(2, TRUE);
    //setColumnStretchable (3, TRUE);
    horizontalHeader()->setResizeEnabled(TRUE, 3);

    if (!isReadOnly) {
        adjustColumn(4);
        setColumnStretchable(4, FALSE);
    }
}

void ParamsTable::activateNextCell()
{
    int row = currentRow();
    int col = currentColumn();

    if (col == 3) {
        // goto next line
        if (++row == numRows()) {
            // adds a new line
            setNumRows(row + 1);
            setItem(row, 0, new ComboItem(this, stringify(UmlIn), DirList, FALSE));
            setText(row, 1, QString());
            setItem(row, 2, new ComboItem(this, QString(), alltypes));
            setText(row, 3, QString());
            setText(row, 4, QString());
        }

        setCurrentCell(row, 0);
    }
    else
        setCurrentCell(row, col + 1);
}

void ParamsTable::value_changed(int row, int col)
{
    if ((row == (numRows() - 1)) && (col != 0) && !text(row, col).isEmpty())
        insert_row_after(row);

    if ((col <= 3) && (!text(row, 1).isEmpty() || !text(row, 2).isEmpty()))
        dialog->force_param(row, col != 1);
}

void ParamsTable::button_pressed(int row, int col, int, const QPoint &)
{
    if (col == 4) {
        int n = nparams();
        char s[16];
        Q3PopupMenu m;

        sprintf(s, "%d", row + 1);
        m.insertItem(TR("param %1", s), -1);
        m.insertSeparator();

        if (row < n) {
            m.insertItem(TR("Insert param before"), 0);
            m.insertItem(TR("Insert param after"), 1);
            m.insertSeparator();
            m.insertItem(TR("Delete param"), 2);
            m.insertSeparator();
            m.insertItem(TR("Copy param"), 3);
            m.insertItem(TR("Cut param"), 4);
        }

        if (!name_copy.isEmpty() || !type_copy.isEmpty())
            m.insertItem(TR("Paste param"), 5);

        Q3PopupMenu mv;
        int rank;

        if (row < n) {
            for (rank = 0; rank != n; rank += 1)
                if (rank != row)
                    mv.insertItem(QString::number(rank + 1), 10 + rank);

            if (mv.count() != 0) {
                m.insertSeparator();
                m.insertItem(TR("Move param"), &mv);
            }
        }

        switch (rank = m.exec(QCursor::pos())) {
        case 0:
            insert_row_before(row);
            dialog->insert_param(row);
            dialog->force_param(row, TRUE);
            break;

        case 1:
            insert_row_after(row);
            dialog->insert_param(row + 1);
            dialog->force_param(row + 1, TRUE);
            break;

        case 2:
            delete_row(row);
            dialog->delete_param(row);
            break;

        case 3:
            copy_row(row);
            break;

        case 4:
            cut_row(row);
            dialog->delete_param(row);
            break;

        case 5:
            paste_row(row);
            dialog->force_param(row, TRUE);
            break;

        default:
            if (rank >= 10) {
                move_row(row, rank - 10);
                dialog->move_param(row, rank - 10);
            }

            break;
        }
    }
}

void ParamsTable::insert_row_before(int row)
{
    int n = numRows();
    int index;

    setNumRows(n + 1);

    for (index = n; index != row; index -= 1) {
        Q3TableItem * it;

        it = item(index - 1, 0);
        takeItem(it);
        setItem(index, 0, it);
        setText(index, 1, text(index - 1, 1));
        it = item(index - 1, 2);
        takeItem(it);
        setItem(index, 2, it);
        setText(index, 3, text(index - 1, 3));
    }

    setItem(row, 0, new ComboItem(this, stringify(UmlIn), DirList, FALSE));
    setText(row, 1, QString());
    setItem(row, 2, new ComboItem(this, QString(), alltypes));
    setText(row, 3, QString());
    setText(row, 4, QString());
}

void ParamsTable::insert_row_after(int row)
{
    int n = numRows();
    int index;

    setNumRows(n + 1);

    for (index = n; index > row + 1; index -= 1) {
        Q3TableItem * it;

        it = item(index - 1, 0);
        takeItem(it);
        setItem(index, 0, it);
        setText(index, 1, text(index - 1, 1));
        it = item(index - 1, 2);
        takeItem(it);
        setItem(index, 2, it);
        setText(index, 3, text(index - 1, 3));
    }

    setItem(row + 1, 0, new ComboItem(this, stringify(UmlIn), DirList, FALSE));
    setText(row + 1, 1, QString());
    setItem(row + 1, 2, new ComboItem(this, QString(), alltypes));
    setText(row + 1, 3, QString());
    setText(row + 1, 4, QString());
}

void ParamsTable::delete_row(int row)
{
    int n = numRows();
    int index;

    clearCellWidget(row, 1);

    if (row == (n - 1)) {
        // the last line : empty it
        setItem(row, 0, new ComboItem(this, stringify(UmlIn), DirList, FALSE));
        setText(row, 1, QString());
        setItem(row, 2, new ComboItem(this, QString(), alltypes));
        setText(row, 3, QString());
    }
    else {
        for (index = row; index != n - 1; index += 1) {
            Q3TableItem * it;

            it = item(index + 1, 0);
            takeItem(it);
            setItem(index, 0, it);
            setText(index, 1, text(index + 1, 1));
            it = item(index + 1, 2);
            takeItem(it);
            setItem(index, 2, it);
            setText(index, 3, text(index + 1, 3));
        }

        setNumRows(n - 1);
    }
}

void ParamsTable::copy_row(int row)
{
    dir_copy = text(row, 0);
    name_copy = text(row, 1);
    type_copy = text(row, 2);
    default_value_copy = text(row, 3);
}

void ParamsTable::cut_row(int row)
{
    copy_row(row);
    delete_row(row);
}

void ParamsTable::paste_row(int row)
{
    setText(row, 0, dir_copy);
    setText(row, 1, name_copy);
    setText(row, 2, type_copy);
    setText(row, 3, default_value_copy);

    if ((row == (numRows() - 1)) &&
        (!dir_copy.isEmpty() ||
         !name_copy.isEmpty() ||
         !type_copy.isEmpty()))
        insert_row_after(row);
}

void ParamsTable::move_row(int from, int to)
{
    QString save_dir_copy = dir_copy;
    QString save_name_copy = name_copy;
    QString save_type_copy = type_copy;
    QString save_default_value_copy = default_value_copy;

    cut_row(from);

    if (to > from)
        insert_row_after(to - 1);
    else
        insert_row_before(to);

    paste_row(to);

    dir_copy = save_dir_copy;
    name_copy = save_name_copy;
    type_copy = save_type_copy;
    default_value_copy = save_default_value_copy;
}

void ParamsTable::update(OperationData * oper,
                         BrowserNodeList & nodes)
{
    forceUpdateCells();

    int n = nparams();
    int index;

    oper->set_n_params(n);

    for (index = 0; index != n; index += 1) {
        oper->set_param_dir(index, (UmlParamDirection) DirList.findIndex(text(index, 0)));

        oper->set_param_name(index, text(index, 1).stripWhiteSpace());

        AType t;
        QString s = text(index, 2).stripWhiteSpace();

        if (! s.isEmpty()) {
            int rank = types.findIndex(s);

            if (rank != -1)
                t.type = (BrowserClass *) nodes.at(rank);
            else
                t.explicit_type = s;
        }

        oper->set_param_type(index, t);

        oper->set_param_default_value(index, text(index, 3).stripWhiteSpace());
    }
}

unsigned ParamsTable::nparams() const
{
    int n = numRows();

    while (n && text(n - 1, 1).isEmpty() && text(n - 1, 2).isEmpty())
        n -= 1;

    return n;
}

QString ParamsTable::name(unsigned rank) const
{
    return text(rank, 1);
}

QString ParamsTable::type(unsigned rank) const
{
    return text(rank, 2);
}

UmlParamDirection ParamsTable::dir(unsigned rank) const
{
    return (UmlParamDirection) DirList.findIndex(text(rank, 0));
}

QString ParamsTable::value(unsigned rank) const
{
    return text(rank, 3);
}

QStringList ParamsTable::all_types()
{
    QStringList result;
    for(unsigned i(1); i < nparams()+1; i++)
    {
        result.append(text(i, 1));
    }
    return result;
}

QStringList ParamsTable::all_names()
{
    QStringList result;
    for(unsigned i(1); i < nparams()+1; i++)
    {
        result.append(text(i, 2));
    }
    return result;
}

QStringList ParamsTable::all_values()
{
    QStringList result;
    for(unsigned i(1); i < nparams()+1; i++)
    {
        result.append(text(i, 3));
    }
    return result;
}

//
// ExceptionTable
//

// copy/cut/paste
QString ExceptionsTable::type_copy;

ExceptionsTable::ExceptionsTable(OperationData * o, QWidget * parent,
                                 QStringList & list, bool visit)
    : MyTable(o->get_n_exceptions() + 1, (visit) ? 1 : 2, parent), types(list)
{
    int index;
    int sup = o->get_n_exceptions();

    setSorting(-1);
    setSelectionMode(NoSelection);	// single does not work
    setRowMovingEnabled(TRUE);
    horizontalHeader()->setLabel(0, TR("Type"));

    if (visit)
    {
        for (index = 0; index < sup; index += 1)
            setItem(index, 0, new TableItem(this, Q3TableItem::Never, o->get_exception(index).get_full_type()));
    }
    else {
        horizontalHeader()->setLabel(1, TR("do"));

        for (index = 0; index < sup; index += 1) {
            setItem(index, 0, new ComboItem(this, o->get_exception(index).get_full_type(), types));
            setText(index, 1, QString());
        }

        setItem(index, 0, new ComboItem(this, QString(), types));
        setText(index, 1, QString());
    }

    setColumnStretchable(0, TRUE);

    if (!visit) {
        adjustColumn(1);
        setColumnStretchable(1, FALSE);

        connect(this, SIGNAL(pressed(int, int, int, const QPoint &)),
                this, SLOT(button_pressed(int, int, int, const QPoint &)));
        connect(this, SIGNAL(valueChanged(int, int)),
                this, SLOT(value_changed(int, int)));
    }
}

void ExceptionsTable::Reinitialize(OperationData *o, QStringList &list, bool isWritable)
{
    types = list;
    int oldRowCount = numRows()-1;
    for (int index =  oldRowCount; index > -1; index -= 1) {
        removeRow(index);
    }
    int sup = o->get_n_exceptions();
    setNumRows(sup+1);

    if (!isWritable)
    {
        disconnect(this, SIGNAL(pressed(int, int, int, const QPoint &)),
                this, SLOT(button_pressed(int, int, int, const QPoint &)));
        disconnect(this, SIGNAL(valueChanged(int, int)),
                this, SLOT(value_changed(int, int)));
        setNumCols(1);
        for (int index = 0; index < sup; index += 1)
            setItem(index, 0, new TableItem(this, Q3TableItem::Never, o->get_exception(index).get_full_type()));
    }
    else
    {
        setNumCols(2);
        horizontalHeader()->setLabel(1, TR("do"));
        setColumnStretchable(1, FALSE);
        adjustColumn(1);

        int index;
        for (index = 0; index < sup; index += 1) {
            setItem(index, 0, new ComboItem(this, o->get_exception(index).get_full_type(), types));
            setText(index, 1, QString());
        }

        setItem(index, 0, new ComboItem(this, QString(), types));
        setText(index, 1, QString());
        connect(this, SIGNAL(pressed(int, int, int, const QPoint &)),
                this, SLOT(button_pressed(int, int, int, const QPoint &)));
        connect(this, SIGNAL(valueChanged(int, int)),
                this, SLOT(value_changed(int, int)));

    }

}


void ExceptionsTable::update(OperationData * oper,
                             BrowserNodeList & nodes)
{
    forceUpdateCells();

    int n = nexceptions();
    int index;

    oper->set_n_exceptions(n);

    for (index = 0; index != n; index += 1) {
        AType t;
        QString s = text(index, 0).stripWhiteSpace();

        if (!s.isEmpty()) {
            int rank = types.findIndex(s);

            if (rank != -1)
                t.type = (BrowserClass *) nodes.at(rank);
            else
                t.explicit_type = s;
        }

        oper->set_exception(index, t);
    }
}

void ExceptionsTable::activateNextCell()
{
    int row = currentRow();

    // goto next line
    if (++row == numRows()) {
        // adds a new line
        setNumRows(row + 1);
        setItem(row, 0, new ComboItem(this, QString(), types));
        setText(row, 1, QString());
    }

    setCurrentCell(row, 0);
}

void ExceptionsTable::value_changed(int row, int col)
{
    if ((row == (numRows() - 1)) && !text(row, col).isEmpty())
        insert_row_after(row);
}

void ExceptionsTable::button_pressed(int row, int col, int, const QPoint &)
{
    if (col == 1) {
        int n = nexceptions();
        char s[16];
        Q3PopupMenu m;

        sprintf(s, "%d", row + 1);
        m.insertItem(TR("exception %1", s), -1);
        m.insertSeparator();

        if (row < n) {
            m.insertItem(TR("Insert exception before"), 0);
            m.insertItem(TR("Insert exception after"), 1);
            m.insertSeparator();
            m.insertItem(TR("Delete exception"), 2);
            m.insertSeparator();
            m.insertItem(TR("Copy exception"), 3);
            m.insertItem(TR("Cut exception"), 4);
        }

        if (!type_copy.isEmpty())
            m.insertItem(TR("Paste exception"), 5);

        Q3PopupMenu mv;
        int rank;

        if (row < n) {
            for (rank = 0; rank != n; rank += 1)
                if (rank != row)
                    mv.insertItem(QString::number(rank + 1), 10 + rank);

            if (mv.count() != 0) {
                m.insertSeparator();
                m.insertItem(TR("Move exception"), &mv);
            }
        }

        switch (rank = m.exec(QCursor::pos())) {
        case 0:
            insert_row_before(row);
            break;

        case 1:
            insert_row_after(row);
            break;

        case 2:
            delete_row(row);
            break;

        case 3:
            copy_row(row);
            break;

        case 4:
            cut_row(row);
            break;

        case 5:
            paste_row(row);
            break;

        default:
            if (rank >= 10)
                move_row(row, rank - 10);

            break;
        }
    }
}

void ExceptionsTable::insert_row_before(int row)
{
    int n = numRows();
    int index;

    setNumRows(n + 1);

    for (index = n; index != row; index -= 1) {
        Q3TableItem * it = item(index - 1, 0);

        takeItem(it);
        setItem(index, 0, it);
    }

    setItem(row, 0, new ComboItem(this, QString(), types));
    setText(row, 1, QString());
}

void ExceptionsTable::insert_row_after(int row)
{
    int n = numRows();
    int index;

    setNumRows(n + 1);

    for (index = n; index > row + 1; index -= 1) {
        Q3TableItem * it = item(index - 1, 0);

        takeItem(it);
        setItem(index, 0, it);
    }

    setItem(row + 1, 0, new ComboItem(this, QString(), types));
    setText(row + 1, 1, QString());
}

void ExceptionsTable::delete_row(int row)
{
    int n = numRows();
    int index;

    clearCellWidget(row, 1);

    if (row == (n - 1)) {
        // the last line : empty it
        setItem(0, 0, new ComboItem(this, QString(), types));
        setText(0, 1, QString());
    }
    else {
        for (index = row; index != n - 1; index += 1) {
            Q3TableItem * it = item(index + 1, 0);
            takeItem(it);
            setItem(index, 0, it);
        }

        setNumRows(n - 1);
    }
}

void ExceptionsTable::copy_row(int row)
{
    type_copy = text(row, 0);
}

void ExceptionsTable::cut_row(int row)
{
    copy_row(row);
    delete_row(row);
}

void ExceptionsTable::paste_row(int row)
{
    setText(row, 0, type_copy);

    if ((row == (numRows() - 1)) && !type_copy.isEmpty())
        insert_row_after(row);
}

void ExceptionsTable::move_row(int from, int to)
{
    QString save_type_copy = type_copy;

    cut_row(from);

    if (to > from)
        insert_row_after(to - 1);
    else
        insert_row_before(to);

    paste_row(to);

    type_copy = save_type_copy;
}


unsigned ExceptionsTable::nexceptions() const
{
    int n = numRows();

    while (n && text(n - 1, 0).isEmpty())
        n -= 1;

    return n;
}

QString ExceptionsTable::type(unsigned rank) const
{
    return text(rank, 0);
}

//
// CppParamTable
//

// copy/cut/paste
QString CppParamsTable::copied[8];	// copy/cut/paste

static QStringList SpecifierList;
static QStringList TypeRankList;
static QStringList PtrList;
static QStringList ParamRankList;
static QStringList ValueRankList;

CppParamsTable::CppParamsTable(ParamsTable * p, MultiLineEdit * f,
                               QWidget * parent, bool decl)
    : MyTable(0, (decl) ? 8 : 7, parent), params(p), edform(f), dcl(decl)
{

    setSorting(-1);
    setSelectionMode(NoSelection);	// single does not work
    setRowMovingEnabled(TRUE);
    horizontalHeader()->setLabel(0, TR("Name"));
    horizontalHeader()->setLabel(1, TR("Specifier"));
    horizontalHeader()->setLabel(2, "${t<i>}");
    horizontalHeader()->setLabel(3, TR("Pointer"));
    horizontalHeader()->setLabel(4, "${p<i>}");

    if (decl) {
        horizontalHeader()->setLabel(5, "${v<i>}");
        horizontalHeader()->setLabel(6, TR("Modifier"));
        horizontalHeader()->setLabel(7, TR("do"));
    }
    else {
        horizontalHeader()->setLabel(5, TR("Modifier"));
        horizontalHeader()->setLabel(6, TR("do"));
    }

    setColumnStretchable(0, TRUE);
    setColumnStretchable(1, TRUE);
    setColumnStretchable(2, TRUE);
    adjustColumn(3);
    setColumnStretchable(4, TRUE);
    setColumnStretchable(5, TRUE);

    if (decl) {
        setColumnStretchable(6, TRUE);
        adjustColumn(7);
        setColumnStretchable(7, FALSE);
    }
    else {
        adjustColumn(6);
        setColumnStretchable(6, FALSE);
    }

    QString form = edform->text();
    //the presence of ${(} and ${)} was checked
    int form_index = form.find("${(}") + 4;
    int tbl_index = 0;

    while (extract(tbl_index, form_index, form)) {
        setText(tbl_index, (decl) ? 7 : 6, QString());
        tbl_index += 1;
    }

    if (tbl_index == 0)
        insert_row_before(0);

    connect(this, SIGNAL(pressed(int, int, int, const QPoint &)),
            this, SLOT(button_pressed(int, int, int, const QPoint &)));

    if (SpecifierList.isEmpty()) {
        SpecifierList.append("");
        SpecifierList.append("const");
        SpecifierList.append("volatile");
    }

    if (PtrList.isEmpty()) {
        PtrList.append("");
        PtrList.append("*");
        PtrList.append("&");
        PtrList.append("*&");
    }

    TypeRankList.clear();
    ParamRankList.clear();
    ValueRankList.clear();

    for (int rank = 0; rank != params->numRows(); rank += 1) {
        if (!params->name(rank).isEmpty() || !params->type(rank).isEmpty()) {
            QString s;

            s.sprintf("${t%u}", rank);
            TypeRankList.append(s);
            s.sprintf("${p%u}", rank);
            ParamRankList.append(s);
            s.sprintf("${v%u}", rank);
            ValueRankList.append(s);
        }
    }
}

void CppParamsTable::init_row(int row)
{
    setItem(row, 0, new Q3TableItem(this, Q3TableItem::Never, QString()));
    setItem(row, 1, new ComboItem(this, QString(), SpecifierList));
    setItem(row, 2, new ComboItem(this, QString(), TypeRankList));
    setItem(row, 3, new ComboItem(this, QString(), PtrList));
    setItem(row, 4, new ComboItem(this, QString(), ParamRankList));

    if (dcl) {
        setItem(row, 5, new ComboItem(this, QString(), ValueRankList));
        setText(row, 6, QString());
        setText(row, 7, QString());
    }
    else {
        setText(row, 5, QString());
        setText(row, 6, QString());
    }
}

bool CppParamsTable::extract(int tblindex, int & strindex, QString s)
{
    // s at least contains ${)}
    while (s.at(strindex).isSpace())
        strindex += 1;

    int sup = OperationFuncs::supOf(s, strindex);

    if (s.mid(strindex, sup - strindex).stripWhiteSpace().isEmpty())
        return FALSE;

    QString specifier;
    QString t_i;
    QString ptr;
    QString p_i;
    QString modifier;
    int index = s.find("${t", strindex);

    if ((index == -1) || (index >= sup)) {
        // no $type
        index = s.find("${p", strindex);

        if ((index == -1) || (index >= sup))
            // no $type, no $name
            specifier = s.mid(strindex, sup - strindex).stripWhiteSpace();
        else {
            // no $type
            t_i = s.mid(strindex, index - strindex).stripWhiteSpace();

            // extract ptr
            int ip = t_i.find('*');
            int ir = t_i.find('&');

            if (ip != -1) {
                if ((ir != -1) && (ir < ip)) {
                    ptr = t_i.mid(ir).stripWhiteSpace();
                    t_i = t_i.left(ir).stripWhiteSpace();
                }
                else  {
                    ptr = t_i.mid(ip).stripWhiteSpace();
                    t_i = t_i.left(ip).stripWhiteSpace();
                }
            }
            else if (ir != -1) {
                ptr = t_i.mid(ir).stripWhiteSpace();
                t_i = t_i.left(ir).stripWhiteSpace();
            }

            if (((strindex = s.find('}', index + 2)) == -1) || (strindex >= sup))
                return FALSE;
            else {
                strindex += 1;
                p_i = s.mid(index, strindex - index);
                modifier = s.mid(strindex, sup - strindex).stripWhiteSpace();
            }
        }
    }
    else {
        specifier = s.mid(strindex, index - strindex).stripWhiteSpace();

        if (((strindex = s.find('}', index + 2)) == -1) || (strindex >= sup))
            return FALSE;

        strindex += 1;
        t_i = s.mid(index, strindex - index);

        index = s.find("${p", strindex);

        if ((index == -1) || (index >= sup))
            // unnamed
            modifier = s.mid(strindex, sup - strindex).stripWhiteSpace();
        else {
            ptr = s.mid(strindex, index - strindex).stripWhiteSpace();

            if (((strindex = s.find('}', index + 2)) == -1) || (strindex >= sup))
                return FALSE;
            else {
                strindex += 1;
                p_i = s.mid(index, strindex - index);
                modifier = s.mid(strindex, sup - strindex).stripWhiteSpace();
            }
        }
    }

    setNumRows(tblindex + 1);

    setItem(tblindex, 1, new ComboItem(this, specifier, SpecifierList));
    setItem(tblindex, 2, new ComboItem(this, t_i, TypeRankList));
    setItem(tblindex, 3, new ComboItem(this, ptr, PtrList));
    setItem(tblindex, 4, new ComboItem(this, p_i, ParamRankList));

    if (dcl) {
        QString v_i;

        if ((modifier.length() >= 5) && (modifier.left(3) == "${v")) {
            if ((strindex = modifier.find('}', 3)) == -1)
                return FALSE;
            else {
                strindex += 1;
                v_i = modifier.left(strindex);
                modifier = modifier.mid(strindex);
            }
        }

        setItem(tblindex, 5, new ComboItem(this, v_i, ValueRankList));
        setText(tblindex, 6, modifier);
    }
    else
        setText(tblindex, 5, modifier);

    strindex = (s.at(sup) == QChar(',')) ? sup + 1 : sup;

    return TRUE;
}

void CppParamsTable::setItem(int row, int col, Q3TableItem * item)
{
    Q3Table::setItem(row, col, item);

    if ((col == 2) || (col == 4))
        update_name(row);
}

void CppParamsTable::setCurrentCell(int row, int col)
{
    Q3Table::setCurrentCell(row, col);
    update_names();
}

void CppParamsTable::update_names()
{
    int n = numRows();
    int row;

    for (row = 0; row != n; row += 1)
        update_name(row);
}

void CppParamsTable::update_name(int row)
{
    bool t_set;
    bool p_set;
    unsigned t_i;
    unsigned p_i;

    if (!text(row, 2).isEmpty() &&
        (sscanf((const char *) text(row, 2), "${t%u}", &t_i) == 1))
        t_set = TRUE;
    else
        t_set = FALSE;

    if (!text(row, 4).isEmpty() &&
        (sscanf((const char *) text(row, 4), "${p%u}", &p_i) == 1))
        p_set = TRUE;
    else
        p_set = FALSE;

    if (t_set) {
        if (p_set)
            Q3Table::setItem(row, 0,
                             new Q3TableItem(this, Q3TableItem::Never,
                                             ((t_i == p_i) && (t_i < params->nparams()))
                                             ? params->name(t_i) : QString()));
        else
            Q3Table::setItem(row, 0,
                             new Q3TableItem(this, Q3TableItem::Never,
                                             (t_i < params->nparams())
                                             ? params->name(t_i) : QString()));
    }
    else
        Q3Table::setItem(row, 0,
                         new Q3TableItem(this, Q3TableItem::Never,
                                         (p_set && (p_i < params->nparams()))
                                         ? params->name(p_i) : QString()));
}

void CppParamsTable::button_pressed(int row, int col, int, const QPoint &)
{
    if (col == ((dcl) ? 7 : 6)) {
        char s[16];
        Q3PopupMenu m;

        sprintf(s, "%d", row + 1);
        m.insertItem(TR("param %1", s), -1);
        m.insertSeparator();
        m.insertItem(TR("Insert param before"), 0);
        m.insertItem(TR("Insert param after"), 1);
        m.insertSeparator();
        m.insertItem(TR("Delete param"), 2);
        m.insertSeparator();
        m.insertItem(TR("Copy param"), 3);
        m.insertItem(TR("Cut param"), 4);
        m.insertItem(TR("Paste param"), 5);
        m.insertSeparator();

        Q3PopupMenu mv;
        int rank;

        for (rank = 0; rank != numRows(); rank += 1)
            if (rank != row)
                mv.insertItem(QString::number(rank + 1), 10 + rank);

        m.insertItem(TR("Move param"), &mv);
        m.insertSeparator();

        Q3PopupMenu rk;
        int t_i;
        int p_i;
        int v_i;

        if (text(row, 2).isEmpty() ||
            (sscanf((const char *) text(row, 2), "${t%d}", &t_i) != 1))
            t_i = -1;

        if (text(row, 4).isEmpty() ||
            (sscanf((const char *) text(row, 4), "${p%d}", &p_i) != 1))
            p_i = -1;

        if (!dcl ||
            text(row, 5).isEmpty() ||
            (sscanf((const char *) text(row, 5), "${v%d}", &v_i) != 1))
            v_i = -1;

        for (rank = 0; rank != params->numRows(); rank += 1)
            if ((!params->name(rank).isEmpty() || !params->type(rank).isEmpty()) &&
                ((rank != t_i) || (rank != p_i)))
                rk.insertItem(QString::number(rank), 100 + rank);

        m.insertItem(TR("Set rank <i>"), &rk);

        switch (rank = m.exec(QCursor::pos())) {
        case 0:
            insert_row_before(row);
            break;

        case 1:
            insert_row_after(row);
            break;

        case 2:
            delete_row(row);
            break;

        case 3:
            copy_row(row);
            break;

        case 4:
            cut_row(row);
            break;

        case 5:
            paste_row(row);
            break;

        default:
            if (rank >= 100) {
                char s[32];

                if (t_i != -1) {
                    sprintf(s, "${t%d}", rank - 100);
                    setItem(row, 2, new ComboItem(this, s, TypeRankList));
                }

                if (p_i != -1) {
                    sprintf(s, "${p%d}", rank - 100);
                    setItem(row, 4, new ComboItem(this, s, ParamRankList));
                }

                if (v_i != -1) {
                    sprintf(s, "${v%d}", rank - 100);
                    setItem(row, 5, new ComboItem(this, s, ParamRankList));
                }
            }
            else if (rank >= 10)
                move_row(row, rank - 10);

            break;
        }
    }
}

void CppParamsTable::insert_row_before(int row)
{
    int n = numRows();
    int index;
    int col;
    int mcol = ((dcl) ? 6 : 5);

    setNumRows(n + 1);

    for (index = n; index != row; index -= 1) {
        for (col = 0; col != mcol; col += 1) {
            Q3TableItem * it = item(index - 1, col);

            takeItem(it);
            setItem(index, col, it);
        }

        setText(index, mcol, text(index - 1, mcol));
        setText(index, mcol + 1, text(index - 1, mcol + 1));
    }

    init_row(row);
}

void CppParamsTable::insert_row_after(int row)
{
    int n = numRows();
    int index;
    int col;
    int mcol = ((dcl) ? 6 : 5);

    setNumRows(n + 1);

    for (index = n; index > row + 1; index -= 1) {
        for (col = 0; col != mcol; col += 1) {
            Q3TableItem * it = item(index - 1, col);

            takeItem(it);
            setItem(index, col, it);
        }

        setText(index, mcol, text(index - 1, col));
        setText(index, mcol + 1, text(index - 1, col));
    }

    init_row(row + 1);
}

void CppParamsTable::delete_row(int row)
{
    int n = numRows();
    int index;
    int col;

    clearCellWidget(row, 1);

    if (row == (n - 1)) {
        // the last line : empty it
        init_row(row);
    }
    else {
        int mcol = ((dcl) ? 6 : 5);

        for (index = row; index != n - 1; index += 1) {
            for (col = 0; col != mcol; col += 1) {
                Q3TableItem * it = item(index + 1, col);

                takeItem(it);
                setItem(index, col, it);
            }

            setText(index, mcol, text(index + 1, col));
            setText(index, mcol + 1, text(index + 1, col));
        }

        setNumRows(n - 1);
    }
}

void CppParamsTable::copy_row(int row)
{
    int col;
    int mcol = ((dcl) ? 8 : 7);

    for (col = 0; col != mcol; col += 1)
        copied[col] = text(row, col);
}

void CppParamsTable::cut_row(int row)
{
    copy_row(row);
    delete_row(row);
}

void CppParamsTable::paste_row(int row)
{
    int col;
    int mcol = ((dcl) ? 8 : 7);

    for (col = 0; col != mcol; col += 1)
        setText(row, col, copied[col]);
}

void CppParamsTable::move_row(int from, int to)
{
    int col;
    int mcol = ((dcl) ? 8 : 7);
    QString save_copied[8];

    for (col = 0; col != mcol; col += 1)
        save_copied[col] = copied[col];

    cut_row(from);

    if (to > from)
        insert_row_after(to - 1);
    else
        insert_row_before(to);

    paste_row(to);

    for (col = 0; col != mcol; col += 1)
        copied[col] = save_copied[col];
}

void CppParamsTable::update_edform()
{
    forceUpdateCells();

    QString s;
    const char * sep = "";

    int n = numRows();
    int index;
    int mcol = ((dcl) ? 6 : 5);

    for (index = 0; index != n; index += 1) {
        QString p;
        int col;

        for (col = 1; col != mcol; col += 1) {
            if (!text(index, col).isEmpty()) {
                switch (col) {
                case 1:
                case 5:
                    p += text(index, col);
                    break;

                default:
                    p += " " + text(index, col);
                }
            }
        }

        if (!text(index, mcol).isEmpty()) {
            if (p.isEmpty() || (text(index, mcol).at(0) == QChar('[')))
                p += text(index, mcol);
            else
                p += " " + text(index, mcol);
        }

        p = p.stripWhiteSpace();

        if (! p.isEmpty()) {
            s += sep + p;
            sep = ", ";
        }
    }

    QString form = edform->text();

    index = form.find("${(}");

    form.replace(index + 4, form.find("${)}") - index - 4, s);
    edform->setText(form);
}

//
// CppParamsDialog
//

QSize CppParamsDialog::previous_size;

CppParamsDialog::CppParamsDialog(QWidget * parent, ParamsTable * params,
                                 MultiLineEdit * form, bool decl)
    : QDialog(parent, "C++ parameters dialog", TRUE)
{
    setCaption(TR("C++ parameters dialog"));

    Q3VBoxLayout * vbox = new Q3VBoxLayout(this);

    vbox->setMargin(5);

    tbl = new CppParamsTable(params, form, this, decl);
    vbox->addWidget(tbl);

    Q3HBoxLayout * hbox = new Q3HBoxLayout(vbox);
    hbox->setMargin(5);
    QPushButton * accept = new QPushButton(TR("&OK"), this);
    QPushButton * cancel = new QPushButton(TR("&Cancel"), this);
    QSize bs(cancel->sizeHint());

    accept->setDefault(TRUE);
    accept->setFixedSize(bs);
    cancel->setFixedSize(bs);

    hbox->addWidget(accept);
    hbox->addWidget(cancel);

    connect(accept, SIGNAL(clicked()), this, SLOT(accept()));
    connect(cancel, SIGNAL(clicked()), this, SLOT(reject()));
}

CppParamsDialog::~CppParamsDialog()
{
    previous_size = size();
}

void CppParamsDialog::polish()
{
    QDialog::polish();
    UmlDesktop::limitsize_center(this, previous_size, 0.8, 0.8);
}


void CppParamsDialog::accept()
{
    tbl->update_edform();
    QDialog::accept();
}

//
// PhpParamTable
//

// copy/cut/paste
QString PhpParamsTable::copied[6];	// copy/cut/paste

static QStringList PhpTypeRankList;
static QStringList PhpRefList;
static QStringList PhpParamRankList;
static QStringList PhpValueRankList;

PhpParamsTable::PhpParamsTable(QWidget * parent, ParamsTable * p, MultiLineEdit * f)
    : MyTable(0, 6, parent), params(p), edform(f)
{

    setSorting(-1);
    setSelectionMode(NoSelection);	// single does not work
    setRowMovingEnabled(TRUE);
    horizontalHeader()->setLabel(0, TR("Name"));
    horizontalHeader()->setLabel(1, "${t<i>}/array");
    horizontalHeader()->setLabel(2, TR("Ref."));
    horizontalHeader()->setLabel(3, "${p<i>}");
    horizontalHeader()->setLabel(4, "${v<i>}");
    horizontalHeader()->setLabel(5, TR("do"));
    setColumnStretchable(0, TRUE);
    setColumnStretchable(1, TRUE);
    adjustColumn(2);
    setColumnStretchable(3, TRUE);
    setColumnStretchable(4, TRUE);
    adjustColumn(5);


    QString form = edform->text();
    //the presence of ${(} and ${)} was checked
    int form_index = form.find("${(}") + 4;
    int tbl_index = 0;

    while (extract(tbl_index, form_index, form)) {
        setText(tbl_index, 5, QString());
        tbl_index += 1;
    }

    if (tbl_index == 0)
        insert_row_before(0);

    connect(this, SIGNAL(pressed(int, int, int, const QPoint &)),
            this, SLOT(button_pressed(int, int, int, const QPoint &)));

    if (PhpRefList.isEmpty()) {
        PhpRefList.append("");
        PhpRefList.append("&");
    }

    PhpTypeRankList.clear();
    PhpParamRankList.clear();
    PhpValueRankList.clear();

    for (int rank = 0; rank != params->numRows(); rank += 1) {
        if (!params->name(rank).isEmpty() || !params->type(rank).isEmpty()) {
            QString s;

            s.sprintf("${t%u}", rank);
            PhpTypeRankList.append(s);
            s.sprintf("${p%u}", rank);
            PhpParamRankList.append(s);
            s.sprintf("${v%u}", rank);
            PhpValueRankList.append(s);
        }
    }

    PhpTypeRankList.append(TR("array"));
}

void PhpParamsTable::init_row(int row)
{
    setItem(row, 0, new Q3TableItem(this, Q3TableItem::Never, QString()));
    setItem(row, 1, new ComboItem(this, QString(), PhpTypeRankList));
    setItem(row, 2, new ComboItem(this, QString(), PhpRefList));
    setItem(row, 3, new ComboItem(this, QString(), PhpParamRankList));
    setItem(row, 4, new ComboItem(this, QString(), PhpValueRankList));
    setText(row, 5, QString());
}

bool PhpParamsTable::extract(int tblindex, int & strindex, QString s)
{
    // s at least contains ${)}
    while (s.at(strindex).isSpace())
        strindex += 1;

    int sup = OperationFuncs::supOf(s, strindex);

    if (s.mid(strindex, sup - strindex).stripWhiteSpace().isEmpty())
        return FALSE;

    QString t_i;
    QString ptr;
    QString p_i;
    QString v_i;
    int index = s.find("${t", strindex);

    if ((index == -1) || (index >= sup)) {
        // no $type
        index = s.find("${p", strindex);

        if ((index != -1) && (index < sup)) {
            // no $type, have name
            t_i = s.mid(strindex, index - strindex).stripWhiteSpace();

            // extract ptr
            int ir = t_i.find('&');

            if (ir != -1) {
                ptr = t_i.mid(ir).stripWhiteSpace();
                t_i = t_i.left(ir).stripWhiteSpace();
            }

            if (((strindex = s.find('}', index + 2)) == -1) || (strindex >= sup))
                return FALSE;
            else {
                strindex += 1;
                p_i = s.mid(index, strindex - index);
                v_i = s.mid(strindex);
            }
        }
    }
    else {
        if (((strindex = s.find('}', index + 2)) == -1) || (strindex >= sup))
            return FALSE;

        strindex += 1;
        t_i = s.mid(index, strindex - index);

        index = s.find("${p", strindex);

        if ((index != -1) && (index < sup)) {
            // named
            ptr = s.mid(strindex, index - strindex).stripWhiteSpace();

            if (((strindex = s.find('}', index + 2)) == -1) || (strindex >= sup))
                return FALSE;
            else {
                strindex += 1;
                p_i = s.mid(index, strindex - index);
                v_i = s.mid(strindex);
            }
        }
    }

    if ((v_i.length() >= 5) && (v_i.left(3) == "${v")) {
        if ((strindex = v_i.find('}', 3)) == -1)
            return FALSE;
        else
            v_i = v_i.left(strindex + 1);
    }
    else
        v_i = "";

    setNumRows(tblindex + 1);

    setItem(tblindex, 1, new ComboItem(this, t_i, PhpTypeRankList));
    setItem(tblindex, 2, new ComboItem(this, ptr, PhpRefList));
    setItem(tblindex, 3, new ComboItem(this, p_i, PhpParamRankList));
    setItem(tblindex, 4, new ComboItem(this, v_i, PhpValueRankList));

    strindex = (s.at(sup) == QChar(',')) ? sup + 1 : sup;

    return TRUE;
}

void PhpParamsTable::setItem(int row, int col, Q3TableItem * item)
{
    Q3Table::setItem(row, col, item);

    if ((col == 1) || (col == 3))
        update_name(row);
}

void PhpParamsTable::setCurrentCell(int row, int col)
{
    Q3Table::setCurrentCell(row, col);
    update_names();
}

void PhpParamsTable::update_names()
{
    int n = numRows();
    int row;

    for (row = 0; row != n; row += 1)
        update_name(row);
}

void PhpParamsTable::update_name(int row)
{
    bool t_set;
    bool p_set;
    unsigned t_i;
    unsigned p_i;

    if (!text(row, 1).isEmpty() &&
        (sscanf((const char *) text(row, 1), "${t%u}", &t_i) == 1))
        t_set = TRUE;
    else
        t_set = FALSE;

    if (!text(row, 3).isEmpty() &&
        (sscanf((const char *) text(row, 3), "${p%u}", &p_i) == 1))
        p_set = TRUE;
    else
        p_set = FALSE;

    if (t_set) {
        if (p_set)
            Q3Table::setItem(row, 0,
                             new Q3TableItem(this, Q3TableItem::Never,
                                             ((t_i == p_i) && (t_i < params->nparams()))
                                             ? params->name(t_i) : QString()));
        else
            Q3Table::setItem(row, 0,
                             new Q3TableItem(this, Q3TableItem::Never,
                                             (t_i < params->nparams())
                                             ? params->name(t_i) : QString()));
    }
    else
        Q3Table::setItem(row, 0,
                         new Q3TableItem(this, Q3TableItem::Never,
                                         (p_set && (p_i < params->nparams()))
                                         ? params->name(p_i) : QString()));
}

void PhpParamsTable::button_pressed(int row, int col, int, const QPoint &)
{
    if (col == 5) {
        char s[16];
        Q3PopupMenu m;

        sprintf(s, "%d", row + 1);
        m.insertItem(TR("param %1", s), -1);
        m.insertSeparator();
        m.insertItem(TR("Insert param before"), 0);
        m.insertItem(TR("Insert param after"), 1);
        m.insertSeparator();
        m.insertItem(TR("Delete param"), 2);
        m.insertSeparator();
        m.insertItem(TR("Copy param"), 3);
        m.insertItem(TR("Cut param"), 4);
        m.insertItem(TR("Paste param"), 5);
        m.insertSeparator();

        Q3PopupMenu mv;
        int rank;

        for (rank = 0; rank != numRows(); rank += 1)
            if (rank != row)
                mv.insertItem(QString::number(rank + 1), 10 + rank);

        m.insertItem(TR("Move param"), &mv);
        m.insertSeparator();

        Q3PopupMenu rk;
        int t_i;
        int p_i;
        int v_i;

        if (text(row, 1).isEmpty() ||
            (sscanf((const char *) text(row, 1), "${t%d}", &t_i) != 1))
            t_i = -1;

        if (text(row, 3).isEmpty() ||
            (sscanf((const char *) text(row, 3), "${p%d}", &p_i) != 1))
            p_i = -1;

        if (text(row, 4).isEmpty() ||
            (sscanf((const char *) text(row, 4), "${v%d}", &v_i) != 1))
            v_i = -1;

        for (rank = 0; rank != params->numRows(); rank += 1)
            if ((!params->name(rank).isEmpty() || !params->type(rank).isEmpty()) &&
                ((rank != t_i) || (rank != p_i)))
                rk.insertItem(QString::number(rank), 100 + rank);

        m.insertItem(TR("Set rank <i>"), &rk);

        switch (rank = m.exec(QCursor::pos())) {
        case 0:
            insert_row_before(row);
            break;

        case 1:
            insert_row_after(row);
            break;

        case 2:
            delete_row(row);
            break;

        case 3:
            copy_row(row);
            break;

        case 4:
            cut_row(row);
            break;

        case 5:
            paste_row(row);
            break;

        default:
            if (rank >= 100) {
                char s[32];

                if (t_i != -1) {
                    sprintf(s, "${t%d}", rank - 100);
                    setItem(row, 1, new ComboItem(this, s, PhpTypeRankList));
                }

                if (p_i != -1) {
                    sprintf(s, "${p%d}", rank - 100);
                    setItem(row, 3, new ComboItem(this, s, PhpParamRankList));
                }

                if (v_i != -1) {
                    sprintf(s, "${v%d}", rank - 100);
                    setItem(row, 4, new ComboItem(this, s, PhpValueRankList));
                }
            }
            else if (rank >= 10)
                move_row(row, rank - 10);

            break;
        }
    }
}

void PhpParamsTable::insert_row_before(int row)
{
    int n = numRows();
    int index;
    int col;

    setNumRows(n + 1);

    for (index = n; index != row; index -= 1) {
        for (col = 0; col != 5; col += 1) {
            Q3TableItem * it = item(index - 1, col);

            takeItem(it);
            setItem(index, col, it);
        }

        setText(index, 5, text(index - 1, 5));
    }

    init_row(row);
}

void PhpParamsTable::insert_row_after(int row)
{
    int n = numRows();
    int index;
    int col;

    setNumRows(n + 1);

    for (index = n; index > row + 1; index -= 1) {
        for (col = 0; col != 5; col += 1) {
            Q3TableItem * it = item(index - 1, col);

            takeItem(it);
            setItem(index, col, it);
        }

        setText(index, 5, text(index - 1, col));
    }

    init_row(row + 1);
}

void PhpParamsTable::delete_row(int row)
{
    int n = numRows();
    int index;
    int col;

    clearCellWidget(row, 1);

    if (row == (n - 1)) {
        // the last line : empty it
        init_row(row);
    }
    else {
        for (index = row; index != n - 1; index += 1) {
            for (col = 0; col != 5; col += 1) {
                Q3TableItem * it = item(index + 1, col);

                takeItem(it);
                setItem(index, col, it);
            }

            setText(index, 5, text(index + 1, col));
        }

        setNumRows(n - 1);
    }
}

void PhpParamsTable::copy_row(int row)
{
    int col;

    for (col = 0; col != 6; col += 1)
        copied[col] = text(row, col);
}

void PhpParamsTable::cut_row(int row)
{
    copy_row(row);
    delete_row(row);
}

void PhpParamsTable::paste_row(int row)
{
    int col;

    for (col = 0; col != 6; col += 1)
        setText(row, col, copied[col]);
}

void PhpParamsTable::move_row(int from, int to)
{
    int col;
    QString save_copied[6];

    for (col = 0; col != 6; col += 1)
        save_copied[col] = copied[col];

    cut_row(from);

    if (to > from)
        insert_row_after(to - 1);
    else
        insert_row_before(to);

    paste_row(to);

    for (col = 0; col != 6; col += 1)
        copied[col] = save_copied[col];
}

void PhpParamsTable::update_edform()
{
    forceUpdateCells();

    QString s;
    const char * sep = "";

    int n = numRows();
    int index;

    for (index = 0; index != n; index += 1) {
        QString p;
        int col;

        for (col = 1; col != 5; col += 1) {
            if (! text(index, col).isEmpty()) {
                switch (col) {
                case 1:
                case 4:
                    p += text(index, col);
                    break;

                default:
                    p += " " + text(index, col);
                }
            }
        }

        p = p.stripWhiteSpace();

        if (! p.isEmpty()) {
            s += sep + p;
            sep = ", ";
        }
    }

    QString form = edform->text();

    index = form.find("${(}");

    form.replace(index + 4, form.find("${)}") - index - 4, s);
    edform->setText(form);
}

//
// PhpParamsDialog
//

QSize PhpParamsDialog::previous_size;

PhpParamsDialog::PhpParamsDialog(QWidget * parent, ParamsTable * params, MultiLineEdit * form)
    : QDialog(parent, "Php parameters dialog", TRUE)
{
    setCaption(TR("Php parameters dialog"));

    Q3VBoxLayout * vbox = new Q3VBoxLayout(this);

    vbox->setMargin(5);

    tbl = new PhpParamsTable(this, params, form);
    vbox->addWidget(tbl);

    Q3HBoxLayout * hbox = new Q3HBoxLayout(vbox);
    hbox->setMargin(5);
    QPushButton * accept = new QPushButton(TR("&OK"), this);
    QPushButton * cancel = new QPushButton(TR("&Cancel"), this);
    QSize bs(cancel->sizeHint());

    accept->setDefault(TRUE);
    accept->setFixedSize(bs);
    cancel->setFixedSize(bs);

    hbox->addWidget(accept);
    hbox->addWidget(cancel);

    connect(accept, SIGNAL(clicked()), this, SLOT(accept()));
    connect(cancel, SIGNAL(clicked()), this, SLOT(reject()));
}

PhpParamsDialog::~PhpParamsDialog()
{
    previous_size = size();
}

void PhpParamsDialog::polish()
{
    QDialog::polish();
    UmlDesktop::limitsize_center(this, previous_size, 0.8, 0.8);
}


void PhpParamsDialog::accept()
{
    tbl->update_edform();

    QDialog::accept();
}

//
// PythonParamTable
//

// copy/cut/paste
QString PythonParamsTable::copied[6];	// copy/cut/paste

static QStringList PythonTypeRankList;
static QStringList PythonModList;
static QStringList PythonParamRankList;
static QStringList PythonValueRankList;

PythonParamsTable::PythonParamsTable(QWidget * parent, ParamsTable * p, MultiLineEdit * f)
    : MyTable(0, 6, parent), params(p), edform(f)
{

    setSorting(-1);
    setSelectionMode(NoSelection);	// single does not work
    setRowMovingEnabled(TRUE);
    horizontalHeader()->setLabel(0, TR("Name"));
    horizontalHeader()->setLabel(1, TR("Modifier"));
    horizontalHeader()->setLabel(2, "${p<i>}");
    horizontalHeader()->setLabel(3, "${t<i>}");
    horizontalHeader()->setLabel(4, "${v<i>}");
    horizontalHeader()->setLabel(5, TR("do"));
    setColumnStretchable(0, TRUE);
    adjustColumn(1);
    setColumnStretchable(2, TRUE);
    setColumnStretchable(3, TRUE);
    setColumnStretchable(4, TRUE);
    adjustColumn(5);


    QString form = edform->text();
    //the presence of ${(} and ${)} was checked
    int form_index = form.find("${(}") + 4;
    int tbl_index = 0;

    while (extract(tbl_index, form_index, form)) {
        setText(tbl_index, 5, QString());
        tbl_index += 1;
    }

    if (tbl_index == 0)
        insert_row_before(0);

    connect(this, SIGNAL(pressed(int, int, int, const QPoint &)),
            this, SLOT(button_pressed(int, int, int, const QPoint &)));

    if (PythonModList.isEmpty()) {
        PythonModList.append("");
        PythonModList.append("*");
        PythonModList.append("**");
    }

    PythonParamRankList.clear();
    PythonTypeRankList.clear();
    PythonValueRankList.clear();

    for (int rank = 0; rank != params->numRows(); rank += 1) {
        if (!params->name(rank).isEmpty()) {
            QString s;

            s.sprintf("${p%u}", rank);
            PythonParamRankList.append(s);

            s.sprintf("${t%u}", rank);
            PythonTypeRankList.append(s);

            s.sprintf("${v%u}", rank);
            PythonValueRankList.append(s);
        }
    }
}

void PythonParamsTable::init_row(int row)
{
    setItem(row, 0, new Q3TableItem(this, Q3TableItem::Never, QString()));
    setItem(row, 1, new ComboItem(this, QString(), PythonModList));
    setItem(row, 2, new ComboItem(this, QString(), PythonParamRankList));
    setItem(row, 3, new ComboItem(this, QString(), PythonTypeRankList));
    setItem(row, 4, new ComboItem(this, QString(), PythonValueRankList));
    setText(row, 5, QString());
}

bool PythonParamsTable::extract(int tblindex, int & strindex, QString s)
{
    // s at least contains ${)}
    while (s.at(strindex).isSpace())
        strindex += 1;

    int sup = OperationFuncs::supOf(s, strindex);

    if (s.mid(strindex, sup - strindex).stripWhiteSpace().isEmpty())
        return FALSE;

    QString p_i;
    QString t_i;
    QString v_i;
    QString ptr;
    QString m_i;
    int index = s.find("${p", strindex);

    if ((index != -1) && (index < sup)) {
        // have name
        m_i = s.mid(strindex, index - strindex).stripWhiteSpace();

        // extract modifier
        if (m_i.find("**") != -1)
            m_i = "**";
        else if (m_i.find("*") != -1)
            m_i = "*";

        if (((strindex = s.find('}', index + 2)) == -1) || (strindex >= sup))
            return FALSE;
        else {
            strindex += 1;
            p_i = s.mid(index, strindex - index);
            t_i = s.mid(strindex);
        }
    }
    else
        return FALSE;

    if ((t_i.length() >= 5) && (t_i.left(3) == "${t")) {
        if ((strindex = t_i.find('}', 3)) == -1)
            return FALSE;
        else {
            strindex += 1;
            v_i = t_i.mid(strindex);
            t_i = t_i.left(strindex);
        }
    }
    else {
        v_i = t_i;
        t_i = "";
    }

    if ((v_i.length() >= 5) && (v_i.left(3) == "${v")) {
        if ((strindex = v_i.find('}', 3)) == -1)
            return FALSE;
        else
            v_i = v_i.left(strindex + 1);
    }
    else
        v_i = "";

    setNumRows(tblindex + 1);

    setItem(tblindex, 1, new ComboItem(this, m_i, PythonModList));
    setItem(tblindex, 2, new ComboItem(this, p_i, PythonParamRankList));
    setItem(tblindex, 3, new ComboItem(this, t_i, PythonTypeRankList));
    setItem(tblindex, 4, new ComboItem(this, v_i, PythonValueRankList));

    strindex = (s.at(sup) == QChar(',')) ? sup + 1 : sup;

    return TRUE;
}

void PythonParamsTable::setItem(int row, int col, Q3TableItem * item)
{
    Q3Table::setItem(row, col, item);

    if (col == 2)
        update_name(row);
}

void PythonParamsTable::setCurrentCell(int row, int col)
{
    Q3Table::setCurrentCell(row, col);
    update_names();
}

void PythonParamsTable::update_names()
{
    int n = numRows();
    int row;

    for (row = 0; row != n; row += 1)
        update_name(row);
}

void PythonParamsTable::update_name(int row)
{
    bool p_set;
    unsigned p_i;

    if (!text(row, 2).isEmpty() &&
        (sscanf((const char *) text(row, 2), "${p%u}", &p_i) == 1))
        p_set = TRUE;
    else
        p_set = FALSE;

    Q3Table::setItem(row, 0,
                     new Q3TableItem(this, Q3TableItem::Never,
                                     (p_set && (p_i < params->nparams()))
                                     ? params->name(p_i) : QString()));
}

void PythonParamsTable::button_pressed(int row, int col, int, const QPoint &)
{
    if (col == 5) {
        char s[16];
        Q3PopupMenu m;

        sprintf(s, "%d", row + 1);
        m.insertItem(TR("param %1", s), -1);
        m.insertSeparator();
        m.insertItem(TR("Insert param before"), 0);
        m.insertItem(TR("Insert param after"), 1);
        m.insertSeparator();
        m.insertItem(TR("Delete param"), 2);
        m.insertSeparator();
        m.insertItem(TR("Copy param"), 3);
        m.insertItem(TR("Cut param"), 4);
        m.insertItem(TR("Paste param"), 5);
        m.insertSeparator();

        Q3PopupMenu mv;
        int rank;

        for (rank = 0; rank != numRows(); rank += 1)
            if (rank != row)
                mv.insertItem(QString::number(rank + 1), 10 + rank);

        m.insertItem(TR("Move param"), &mv);
        m.insertSeparator();

        Q3PopupMenu rk;
        int p_i;
        int v_i;
        int t_i;

        if (text(row, 2).isEmpty() ||
            (sscanf((const char *) text(row, 2), "${p%d}", &p_i) != 1))
            p_i = -1;

        if (text(row, 3).isEmpty() ||
            (sscanf((const char *) text(row, 3), "${t%d}", &t_i) != 1))
            t_i = -1;

        if (text(row, 4).isEmpty() ||
            (sscanf((const char *) text(row, 4), "${v%d}", &v_i) != 1))
            v_i = -1;

        for (rank = 0; rank != params->numRows(); rank += 1)
            if (!params->name(rank).isEmpty() && (rank != p_i))
                rk.insertItem(QString::number(rank), 100 + rank);

        m.insertItem(TR("Set rank <i>"), &rk);

        switch (rank = m.exec(QCursor::pos())) {
        case 0:
            insert_row_before(row);
            break;

        case 1:
            insert_row_after(row);
            break;

        case 2:
            delete_row(row);
            break;

        case 3:
            copy_row(row);
            break;

        case 4:
            cut_row(row);
            break;

        case 5:
            paste_row(row);
            break;

        default:
            if (rank >= 100) {
                char s[32];

                if (p_i != -1) {
                    sprintf(s, "${p%d}", rank - 100);
                    setItem(row, 2, new ComboItem(this, s, PythonParamRankList));
                }

                if (v_i != -1) {
                    sprintf(s, "${t%d}", rank - 100);
                    setItem(row, 3, new ComboItem(this, s, PythonTypeRankList));
                }

                if (v_i != -1) {
                    sprintf(s, "${v%d}", rank - 100);
                    setItem(row, 4, new ComboItem(this, s, PythonValueRankList));
                }
            }
            else if (rank >= 10)
                move_row(row, rank - 10);

            break;
        }
    }
}

void PythonParamsTable::insert_row_before(int row)
{
    int n = numRows();
    int index;
    int col;

    setNumRows(n + 1);

    for (index = n; index != row; index -= 1) {
        for (col = 0; col != 5; col += 1) {
            Q3TableItem * it = item(index - 1, col);

            takeItem(it);
            setItem(index, col, it);
        }

        setText(index, col, text(index - 1, col));
    }

    init_row(row);
}

void PythonParamsTable::insert_row_after(int row)
{
    int n = numRows();
    int index;
    int col;

    setNumRows(n + 1);

    for (index = n; index > row + 1; index -= 1) {
        for (col = 0; col != 5; col += 1) {
            Q3TableItem * it = item(index - 1, col);

            takeItem(it);
            setItem(index, col, it);
        }

        setText(index, col, text(index - 1, col));
    }

    init_row(row + 1);
}

void PythonParamsTable::delete_row(int row)
{
    int n = numRows();
    int index;
    int col;

    clearCellWidget(row, 1);

    if (row == (n - 1)) {
        // the last line : empty it
        init_row(row);
    }
    else {
        for (index = row; index != n - 1; index += 1) {
            for (col = 0; col != 5; col += 1) {
                Q3TableItem * it = item(index + 1, col);

                takeItem(it);
                setItem(index, col, it);
            }

            setText(index, col, text(index + 1, col));
        }

        setNumRows(n - 1);
    }
}

void PythonParamsTable::copy_row(int row)
{
    int col;

    for (col = 0; col != 6; col += 1)
        copied[col] = text(row, col);
}

void PythonParamsTable::cut_row(int row)
{
    copy_row(row);
    delete_row(row);
}

void PythonParamsTable::paste_row(int row)
{
    int col;

    for (col = 0; col != 6; col += 1)
        setText(row, col, copied[col]);
}

void PythonParamsTable::move_row(int from, int to)
{
    int col;
    QString save_copied[6];

    for (col = 0; col != 6; col += 1)
        save_copied[col] = copied[col];

    cut_row(from);

    if (to > from)
        insert_row_after(to - 1);
    else
        insert_row_before(to);

    paste_row(to);

    for (col = 0; col != 6; col += 1)
        copied[col] = save_copied[col];
}

void PythonParamsTable::update_edform()
{
    forceUpdateCells();

    QString s;
    const char * sep = "";

    int n = numRows();
    int index;

    for (index = 0; index != n; index += 1) {
        QString p;
        int col;

        for (col = 1; col != 5; col += 1)
            if (!text(index, col).isEmpty())
                p += text(index, col);

        p = p.stripWhiteSpace();

        if (! p.isEmpty()) {
            s += sep + p;
            sep = ", ";
        }
    }

    QString form = edform->text();

    index = form.find("${(}");

    form.replace(index + 4, form.find("${)}") - index - 4, s);
    edform->setText(form);
}

//
// PythonParamsDialog
//

QSize PythonParamsDialog::previous_size;

PythonParamsDialog::PythonParamsDialog(QWidget * parent, ParamsTable * params, MultiLineEdit * form)
    : QDialog(parent, "Python parameters dialog", TRUE)
{
    setCaption(TR("Python parameters dialog"));

    Q3VBoxLayout * vbox = new Q3VBoxLayout(this);

    vbox->setMargin(5);

    tbl = new PythonParamsTable(this, params, form);
    vbox->addWidget(tbl);

    Q3HBoxLayout * hbox = new Q3HBoxLayout(vbox);
    hbox->setMargin(5);
    QPushButton * accept = new QPushButton(TR("&OK"), this);
    QPushButton * cancel = new QPushButton(TR("&Cancel"), this);
    QSize bs(cancel->sizeHint());

    accept->setDefault(TRUE);
    accept->setFixedSize(bs);
    cancel->setFixedSize(bs);

    hbox->addWidget(accept);
    hbox->addWidget(cancel);

    connect(accept, SIGNAL(clicked()), this, SLOT(accept()));
    connect(cancel, SIGNAL(clicked()), this, SLOT(reject()));
}

PythonParamsDialog::~PythonParamsDialog()
{
    previous_size = size();
}

void PythonParamsDialog::polish()
{
    QDialog::polish();
    UmlDesktop::limitsize_center(this, previous_size, 0.8, 0.8);
}


void PythonParamsDialog::accept()
{
    tbl->update_edform();

    QDialog::accept();
}

void OperationDialog::InitGui()
{
    init_uml();

    InitPropertiesTab();
    init_cpp();
    init_java();
    init_php();
    init_python();
    init_idl();

}

void OperationDialog::FillGuiElements(BrowserNode * bn)
{
    if (dynamic_cast<OperationData *>(bn->get_data()))
        FillGuiElements(dynamic_cast<OperationData *>(bn->get_data()));
}

void OperationDialog::FillGuiElements(OperationData * _oper)
{
    SetDialogMode(_oper->browser_node->is_writable());
    cppbody = QString();
    oldcppbody = QString();
    templates = QString();
    cl_names = QString();
    templates_tmplop = QString();
    cl_names_tmplop = QString();
    list.clear();
    //nodes.clear();
    //edits.clear();
    get_of_attr = 0;
    set_of_attr = 0;
    get_of_rel = 0;
    set_of_rel = 0;
    is_rel_a = false;
    javaannotation = QString();
    javabody = QString();
    oldjavabody = QString();
    phpbody = QString();
    oldphpbody = QString();
    pythondecorator = QString();
    pythonbody = QString();
    oldpythonbody = QString();
    currentNode = (BrowserNode *) _oper->get_browser_node();
    oper = _oper;
    cl = ((ClassData *)((BrowserClass *) _oper->browser_node->parent())->get_data());
    init_get_set();
    FillGeneric();
    FillUmlTab(oper);
    FillcppTab(oper);
    FillJavaTab(oper);
    FillPhpTab(oper);
    FillPythonTab(oper);
    FillIdlTab(oper);
    FillPropertiesTab(oper);
}

uint OperationDialog::TypeID()
{
    return TypeIdentifier<OperationDialog>::id();
}

QSharedPointer<OperationDialog> OperationDialog::Instance(OperationData * o, DrawingLanguage l)
{
    if (instance.isNull())
        instance = QSharedPointer<OperationDialog>(new OperationDialog(o, l));
    else {
        instance->drawingLanguage = l;
        instance->ChangeTab(0);
        instance->FillGuiElements(o);

    }

    return instance;
}

void OperationDialog::InitPropertiesTab()
{
    // USER : list key - value

    Q3Grid  *  grid = new Q3Grid(2, this);

    grid->setMargin(5);
    grid->setSpacing(5);

    kvtable = new KeyValuesTable(oper->get_browser_node(), grid, !isWritable); //update this table
    RegisterTab("Properties", grid);
    addTab(grid, TR("Properties"));

}

void OperationDialog::FillPropertiesTab(OperationData * o)
{
    kvtable->updateThisFromNode(o->browser_node, !isWritable);
}


void OperationDialog::closeEvent(QCloseEvent *)
{
    int k = 0;
    k++;
}
