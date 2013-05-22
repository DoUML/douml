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
#include <qpushbutton.h>

#include "FlowDialog.h"
#include "FlowData.h"
#include "BrowserFlow.h"
#include "KeyValueTable.h"
#include "UmlWindow.h"
#include "DialogUtil.h"
#include "strutil.h"
#include "UmlDesktop.h"
#include "BodyDialog.h"
#include "GenerationSettings.h"
#include "ProfiledStereotypes.h"
#include "translate.h"

QSize FlowDialog::previous_size;

FlowDialog::FlowDialog(FlowData * d)
    : Q3TabDialog(0, 0, FALSE, Qt::WDestructiveClose), flow(d)
{
    d->browser_node->edit_start();

    if (d->browser_node->is_writable()) {
        setOkButton(TR("OK"));
        setCancelButton(TR("Cancel"));
    }
    else {
        setOkButton(QString());
        setCancelButton(TR("Close"));
    }

    setCaption(TR("Flow dialog"));
    visit = !hasOkButton();

    BrowserNode * bn = flow->browser_node;
    Q3Grid * grid;

    //
    // general tab
    //

    grid = new Q3Grid(2, this);
    umltab = grid;
    grid->setMargin(5);
    grid->setSpacing(5);

    new QLabel(TR("name : "), grid);
    edname = new LineEdit(bn->get_name(), grid);
    edname->setReadOnly(visit);

    new QLabel(TR("stereotype : "), grid);
    edstereotype = new Q3ComboBox(!visit, grid);
    edstereotype->insertItem(toUnicode(flow->get_stereotype()));

    if (!visit) {
        edstereotype->insertStringList(BrowserFlow::default_stereotypes());
        edstereotype->insertStringList(ProfiledStereotypes::defaults(UmlFlow));
        edstereotype->setAutoCompletion(completion());
    }

    edstereotype->setCurrentItem(0);
    QSizePolicy sp = edstereotype->sizePolicy();
    sp.setHorData(QSizePolicy::Expanding);
    edstereotype->setSizePolicy(sp);

    Q3VBox * vtab = new Q3VBox(grid);
    new QLabel(TR("description :"), vtab);

    if (! visit)
        connect(new SmallPushButton(TR("Editor"), vtab), SIGNAL(clicked()),
                this, SLOT(edit_description()));

    comment = new MultiLineEdit(grid);
    comment->setReadOnly(visit);
    comment->setText(bn->get_comment());

    addTab(grid, "Uml");

    // UML / OCL
    init_tab(uml, flow->uml, "Ocl", SLOT(edit_uml_guard()),
             SLOT(edit_uml_selection()), SLOT(edit_uml_transformation()),
             TRUE);

    // CPP
    init_tab(cpp, flow->cpp, "C++", SLOT(edit_cpp_guard()),
             SLOT(edit_cpp_selection()), SLOT(edit_cpp_transformation()),
             GenerationSettings::cpp_get_default_defs());

    // Java
    init_tab(java, flow->java, "Java", SLOT(edit_java_guard()),
             SLOT(edit_java_selection()), SLOT(edit_java_transformation()),
             GenerationSettings::java_get_default_defs());

    // USER : list key - value

    grid = new Q3Grid(2, this);
    grid->setMargin(5);
    grid->setSpacing(5);

    kvtable = new KeyValuesTable(bn, grid, visit);
    addTab(grid, TR("Properties"));

    //

    connect(this, SIGNAL(currentChanged(QWidget *)),
            this, SLOT(change_tabs(QWidget *)));

    open_dialog(this);
}

void FlowDialog::polish()
{
    Q3TabDialog::polish();
    UmlDesktop::limitsize_center(this, previous_size, 0.8, 0.8);
}

FlowDialog::~FlowDialog()
{
    flow->browser_node->edit_end();
    previous_size = size();

    foreach (BodyDialog *dialog, edits)
        dialog->close();
    edits.clear();

    close_dialog(this);
}

void FlowDialog::change_tabs(QWidget * w)
{
    if (hasOkButton() && (w == umltab))
        edname->setFocus();
}

void FlowDialog::init_tab(FlDialog & d, FlowDef & st, const char * lbl,
                          const char * sl_guard, const char * sl_selection,
                          const char * sl_transformation, bool enabled)
{
    Q3Grid * grid = new Q3Grid(2, this);
    Q3VBox * vtab;

    grid->setMargin(5);
    grid->setSpacing(5);

    new QLabel(TR("weight : "), grid);
    d.edweight = new LineEdit(grid);

    QFont font = d.edweight->font();

    if (! hasCodec())
        font.setFamily("Courier");

    font.setFixedPitch(TRUE);
    d.edweight->setFont(font);
    d.edweight->setText(st.weight);

    if (visit)
        d.edweight->setReadOnly(TRUE);

    vtab = new Q3VBox(grid);
    new QLabel(TR("guard : "), vtab);

    if (! visit)
        connect(new SmallPushButton(TR("Editor"), vtab), SIGNAL(clicked()), this, sl_guard);

    d.edguard = new MultiLineEdit(grid);
    d.edguard->setFont(font);
    d.edguard->setText(st.guard);

    if (visit)
        d.edguard->setReadOnly(TRUE);

    vtab = new Q3VBox(grid);
    new QLabel(TR("selection : "), vtab);

    if (! visit)
        connect(new SmallPushButton(TR("Editor"), vtab), SIGNAL(clicked()), this, sl_selection);

    d.edselection = new MultiLineEdit(grid);
    d.edselection->setFont(font);
    d.edselection->setText(st.sel_trans.first);

    if (visit)
        d.edselection->setReadOnly(TRUE);

    vtab = new Q3VBox(grid);
    new QLabel(TR("transformation : "), vtab);

    if (! visit)
        connect(new SmallPushButton(TR("Editor"), vtab), SIGNAL(clicked()), this, sl_transformation);

    d.edtransformation = new MultiLineEdit(grid);
    d.edtransformation->setFont(font);
    d.edtransformation->setText(st.sel_trans.second);

    if (visit)
        d.edtransformation->setReadOnly(TRUE);

    addTab(grid, lbl);

    if (! enabled)
        removePage(grid);
}

void FlowDialog::edit_description()
{
    edit(comment->text(), "flow_description", flow, TxtEdit,
         this, (post_edit) post_edit_description, edits);
}

void FlowDialog::post_edit_description(FlowDialog * d, QString s)
{
    d->comment->setText(s);
}

#define DEF_EDIT(attr, lang, Lang) \
void FlowDialog::edit_##lang##_##attr() {	\
  edit(lang.ed##attr->text(), "state_"#lang"_"#attr, flow, Lang,  \
       this, (post_edit) post_edit_##lang##_##attr, edits);	\
} \
  \
void FlowDialog::post_edit_##lang##_##attr(FlowDialog * d, QString s) \
{ \
  d->lang.ed##attr->setText(s); \
}

DEF_EDIT(guard, uml, TxtEdit)
DEF_EDIT(guard, cpp, CppEdit)
DEF_EDIT(guard, java, JavaEdit)

DEF_EDIT(selection, uml, TxtEdit)
DEF_EDIT(selection, cpp, CppEdit)
DEF_EDIT(selection, java, JavaEdit)

DEF_EDIT(transformation, uml, TxtEdit)
DEF_EDIT(transformation, cpp, CppEdit)
DEF_EDIT(transformation, java, JavaEdit)

void FlowDialog::accept()
{
    if (!check_edits(edits) || !kvtable->check_unique())
        return;

    BrowserNode * bn = flow->browser_node;
    QString s;

    s = edname->text().stripWhiteSpace();

    if ((s != bn->get_name()) &&
        ((BrowserNode *) bn->parent())->wrong_child_name(s, UmlFlow,
                bn->allow_spaces(),
                bn->allow_empty()))
        msg_critical(TR("Error"), s + TR("\n\nillegal name or already used"));
    else {
        bn->set_name(s);

        bool newst = flow->set_stereotype(fromUnicode(edstereotype->currentText().stripWhiteSpace()));

        uml.accept(flow->uml);
        cpp.accept(flow->cpp);
        java.accept(flow->java);

        bn->set_comment(comment->text());
        UmlWindow::update_comment_if_needed(bn);

        kvtable->updateNodeFromThis(bn);

        ProfiledStereotypes::modified(bn, newst);

        bn->modified();
        bn->package_modified();
        flow->modified();

        Q3TabDialog::accept();
    }
}

void FlDialog::accept(FlowDef & st)
{
    st.weight = edweight->text().stripWhiteSpace();
    st.guard = edguard->text();
    st.sel_trans.first = edselection->text();
    st.sel_trans.second = edtransformation->text();
}
