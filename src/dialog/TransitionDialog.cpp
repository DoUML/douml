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
#include <qcheckbox.h>
#include <qpushbutton.h>

#include "TransitionDialog.h"
#include "TransitionData.h"
#include "BrowserTransition.h"
#include "KeyValueTable.h"
#include "UmlWindow.h"
#include "DialogUtil.h"
#include "strutil.h"
#include "UmlDesktop.h"
#include "BodyDialog.h"
#include "GenerationSettings.h"
#include "ProfiledStereotypes.h"
#include "translate.h"

QSize TransitionDialog::previous_size;

TransitionDialog::TransitionDialog(TransitionData * r)
    : Q3TabDialog(0, 0, FALSE, Qt::WDestructiveClose), rel(r)
{
    r->browser_node->edit_start();

    if (r->browser_node->is_writable()) {
        setOkButton(TR("OK"));
        setCancelButton(TR("Cancel"));
    }
    else {
        setOkButton(QString());
        setCancelButton(TR("Close"));
    }

    setCaption(TR("Transition dialog"));
    visit = !hasOkButton();

    BrowserNode * bn = rel->browser_node;
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
    edstereotype->insertItem(toUnicode(rel->get_stereotype()));

    if (!visit) {
        //edstereotype->insertStringList(rel->get_start()->default_stereotypes(type));
        edstereotype->setAutoCompletion(completion());
        edstereotype->insertStringList(ProfiledStereotypes::defaults(UmlTransition));
    }

    edstereotype->setCurrentItem(0);
    QSizePolicy sp = edstereotype->sizePolicy();
    sp.setHorData(QSizePolicy::Expanding);
    edstereotype->setSizePolicy(sp);

    if (r->get_start_node() != r->get_end_node())
        internal_cb = 0;
    else {
        new QLabel(grid);
        internal_cb = new QCheckBox(TR("internal"), grid);
        internal_cb->setChecked(r->internal());
    }

    Q3VBox * vtab = new Q3VBox(grid);
    new QLabel(TR("description :"), vtab);

    if (! visit)
        connect(new SmallPushButton(TR("Editor"), vtab), SIGNAL(clicked()),
                this, SLOT(edit_description()));

    comment = new MultiLineEdit(grid);
    comment->setReadOnly(visit);
    comment->setText(bn->get_comment());
    //comment->setFont(font);

    addTab(grid, "Uml");

    // UML / OCL
    init_tab(ocltab, uml, rel->uml, "Ocl", SLOT(edit_uml_trigger()),
             SLOT(edit_uml_guard()), SLOT(edit_uml_expr()), TRUE);

    // CPP
    init_tab(cppTab, cpp, rel->cpp, "C++", SLOT(edit_cpp_trigger()),
             SLOT(edit_cpp_guard()), SLOT(edit_cpp_expr()),
             GenerationSettings::cpp_get_default_defs());

    // Java
    init_tab(javatab, java, rel->java, "Java", SLOT(edit_java_trigger()),
             SLOT(edit_java_guard()), SLOT(edit_java_expr()),
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

TransitionDialog::~TransitionDialog()
{
    rel->browser_node->edit_end();
    previous_size = size();

    foreach (BodyDialog *dialog, edits)
        dialog->close();
    edits.clear();

    close_dialog(this);
}

void TransitionDialog::change_tabs(QWidget * w)
{
    if (!visit) {
        if (w == umltab)
            edname->setFocus();
        else if (w == ocltab)
            uml.edtrigger->setFocus();
        else if (w == cppTab)
            cpp.edtrigger->setFocus();
        else if (w == javatab)
            java.edtrigger->setFocus();
    }
}

void TransitionDialog::polish()
{
    Q3TabDialog::polish();
    UmlDesktop::limitsize_center(this, previous_size, 0.8, 0.8);
}

void TransitionDialog::init_tab(QWidget *& tab, TransDialog & d, TransDef & td,
                                const char * lbl, const char * sl_trigger,
                                const char * sl_guard, const char * sl_expr,
                                bool enabled)
{
    Q3Grid * grid = new Q3Grid(2, this);
    Q3VBox * vtab;

    tab = grid;
    grid->setMargin(5);
    grid->setSpacing(5);

    vtab = new Q3VBox(grid);
    new QLabel(TR("trigger : "), vtab);

    if (! visit)
        connect(new SmallPushButton(TR("Editor"), vtab), SIGNAL(clicked()),
                this, sl_trigger);

    d.edtrigger = new MultiLineEdit(grid);

    QFont font = d.edtrigger->font();

    if (! hasCodec())
        font.setFamily("Courier");

    font.setFixedPitch(TRUE);
    d.edtrigger->setFont(font);
    d.edtrigger->setText(td.trigger);

    if (visit)
        d.edtrigger->setReadOnly(TRUE);

    vtab = new Q3VBox(grid);
    new QLabel(TR("guard\nconstraint : "), vtab);

    if (! visit)
        connect(new SmallPushButton(TR("Editor"), vtab), SIGNAL(clicked()),
                this, sl_guard);

    d.edguard = new MultiLineEdit(grid);
    d.edguard->setFont(font);
    d.edguard->setText(td.guard);

    if (visit)
        d.edguard->setReadOnly(TRUE);

    vtab = new Q3VBox(grid);
    new QLabel(TR("activity\nexpression : "), vtab);

    if (! visit)
        connect(new SmallPushButton(TR("Editor"), vtab), SIGNAL(clicked()),
                this, sl_expr);

    d.edexpr = new MultiLineEdit(grid);
    d.edexpr->setFont(font);
    d.edexpr->setText(td.expr);

    if (visit)
        d.edexpr->setReadOnly(TRUE);

    addTab(grid, lbl);

    if (! enabled)
        removePage(grid);
}

void TransitionDialog::edit_description()
{
    edit(comment->text(), "transition_description", rel, TxtEdit,
         this, (post_edit) post_edit_description, edits);
}

void TransitionDialog::post_edit_description(TransitionDialog * d, QString s)
{
    d->comment->setText(s);
}

#define DEF_EDIT(attr, lang, Lang) \
void TransitionDialog::edit_##lang##_##attr() {	\
  edit(lang.ed##attr->text(), "transition_"#lang"_"#attr, rel, Lang,  \
       this, (post_edit) post_edit_##lang##_##attr, edits);	\
} \
  \
void TransitionDialog::post_edit_##lang##_##attr(TransitionDialog * d, QString s) \
{ \
  d->lang.ed##attr->setText(s); \
}

DEF_EDIT(trigger, uml, TxtEdit)
DEF_EDIT(trigger, cpp, CppEdit)
DEF_EDIT(trigger, java, JavaEdit)

DEF_EDIT(guard, uml, TxtEdit)
DEF_EDIT(guard, cpp, CppEdit)
DEF_EDIT(guard, java, JavaEdit)

DEF_EDIT(expr, uml, TxtEdit)
DEF_EDIT(expr, cpp, CppEdit)
DEF_EDIT(expr, java, JavaEdit)

void TransitionDialog::accept()
{
    if (!check_edits(edits) || !kvtable->check_unique())
        return;

    BrowserNode * bn = rel->browser_node;
    QString s;

    s = edname->text().stripWhiteSpace();

    if (s.isEmpty())
        s = "<transition>";

    if ((s != bn->get_name()) &&
        ((BrowserNode *) bn->parent())->wrong_child_name(s, UmlTransition,
                bn->allow_spaces(),
                bn->allow_empty()))
        msg_critical(TR("Error"), s + TR("\n\nillegal name or already used"));
    else {
        bn->set_name(s);

        bool newst = rel->set_stereotype(fromUnicode(edstereotype->currentText().stripWhiteSpace()));

        if (internal_cb != 0)
            rel->set_internal(internal_cb->isChecked());

        uml.accept(rel->uml);
        cpp.accept(rel->cpp);
        java.accept(rel->java);

        bn->set_comment(comment->text());
        UmlWindow::update_comment_if_needed(bn);

        kvtable->updateNodeFromThis(bn);

        ProfiledStereotypes::modified(bn, newst);

        bn->modified();
        bn->package_modified();
        rel->modified();

        Q3TabDialog::accept();
    }
}

void TransDialog::accept(TransDef & td)
{
    td.trigger = edtrigger->text();
    td.guard = edguard->text();
    td.expr = edexpr->text();
}
