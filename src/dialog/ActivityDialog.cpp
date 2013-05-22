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
#include <qcheckbox.h>
#include <q3buttongroup.h>
#include <q3popupmenu.h>
#include <qcursor.h>

#include "ActivityDialog.h"
#include "ActivityData.h"
#include "BrowserActivity.h"
#include "BrowserOperation.h"
#include "BrowserView.h"
#include "KeyValueTable.h"
#include "UmlWindow.h"
#include "DialogUtil.h"
#include "strutil.h"
#include "UmlDesktop.h"
#include "BodyDialog.h"
#include "GenerationSettings.h"
#include "ProfiledStereotypes.h"
#include "translate.h"

QSize ActivityDialog::previous_size;

ActivityDialog::ActivityDialog(ActivityData * d)
    : Q3TabDialog(0, 0, FALSE, Qt::WDestructiveClose), activity(d)
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

    setCaption(TR("Activity dialog"));
    visit = !hasOkButton();

    BrowserNode * bn = activity->browser_node;
    Q3Grid * grid;

    //
    // general tab
    //

    grid = new Q3Grid(2, this);
    grid->setMargin(5);
    grid->setSpacing(5);

    new QLabel(TR("name : "), grid);
    edname = new LineEdit(bn->get_name(), grid);
    edname->setReadOnly(visit);

    new QLabel(TR("stereotype : "), grid);
    edstereotype = new Q3ComboBox(!visit, grid);
    edstereotype->insertItem(toUnicode(activity->get_stereotype()));

    if (!visit) {
        edstereotype->insertStringList(BrowserActivity::default_stereotypes());
        edstereotype->insertStringList(ProfiledStereotypes::defaults(UmlActivity));
        edstereotype->setAutoCompletion(completion());
    }

    edstereotype->setCurrentItem(0);
    QSizePolicy sp = edstereotype->sizePolicy();
    sp.setHorData(QSizePolicy::Expanding);
    edstereotype->setSizePolicy(sp);

    connect(new SmallPushButton(TR("specification :"), grid), SIGNAL(clicked()),
            this, SLOT(menu_specification()));
    edspecification = new Q3ComboBox(FALSE, grid);
    edspecification->setSizePolicy(sp);

    if (visit) {
        if (activity->get_specification() == 0)
            edspecification->insertItem("");
        else
            edspecification->insertItem(activity->get_specification()->full_name(TRUE));
    }
    else {
        edspecification->insertItem("");
        edspecification->setAutoCompletion(completion());
        BrowserOperation::instances(opers);
        opers.full_names(list);
        edspecification->insertStringList(list);
        edspecification->setCurrentItem((activity->get_specification() == 0)
                                        ? 0
                                        : opers.indexOf(activity->get_specification()) + 1);
    }

    new QLabel(grid);
    Q3ButtonGroup * bg =
        new Q3ButtonGroup(3, Qt::Horizontal, QString(), grid);

    readonly_cb = new QCheckBox(TR("read only"), bg);

    if (activity->read_only)
        readonly_cb->setChecked(TRUE);

    readonly_cb->setDisabled(visit);

    singlexec_cb = new QCheckBox(TR("single execution"), bg);

    if (activity->single_execution)
        singlexec_cb->setChecked(TRUE);

    singlexec_cb->setDisabled(visit);

    active_cb = new QCheckBox(TR("active"), bg);

    if (activity->is_active)
        active_cb->setChecked(TRUE);

    active_cb->setDisabled(visit);

    Q3VBox * vtab = new Q3VBox(grid);
    new QLabel(TR("description :"), vtab);

    if (! visit)
        connect(new SmallPushButton(TR("Editor"), vtab), SIGNAL(clicked()),
                this, SLOT(edit_description()));

    comment = new MultiLineEdit(grid);
    comment->setReadOnly(visit);
    comment->setText(bn->get_comment());
    //comment->setFont(font);

    vtab = new Q3VBox(grid);
    new QLabel(TR("constraint :"), vtab);

    if (! visit) {
        connect(new SmallPushButton(TR("Editor"), vtab), SIGNAL(clicked()),
                this, SLOT(edit_constraint()));
    }

    constraint = new MultiLineEdit(grid);
    constraint->setReadOnly(visit);
    constraint->setText(activity->constraint);
    //constraint->setFont(font);

    addTab(grid, "Uml");

    // UML / OCL
    init_tab(uml, activity->uml_condition, "Ocl", TRUE);

    // CPP
    init_tab(cpp, activity->cpp_condition, "C++",
             GenerationSettings::cpp_get_default_defs());

    // Java
    init_tab(java, activity->java_condition, "Java",
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

void ActivityDialog::polish()
{
    Q3TabDialog::polish();
    UmlDesktop::limitsize_center(this, previous_size, 0.8, 0.8);
}

void ActivityDialog::menu_specification()
{
    Q3PopupMenu m(0);

    m.insertItem(TR("Choose"), -1);
    m.insertSeparator();

    int index = list.findIndex(edspecification->currentText().stripWhiteSpace());

    if (index != -1)
        m.insertItem(TR("Select in browser"), 0);

    BrowserNode * bn = 0;

    if (! visit) {
        bn = BrowserView::selected_item();

        if ((bn != 0) && (bn->get_type() == UmlOperation) && !bn->deletedp())
            m.insertItem(TR("Choose operation selected in browser"), 1);
        else
            bn = 0;
    }

    if ((index != -1) || (bn != 0)) {
        switch (m.exec(QCursor::pos())) {
        case 0:
            opers.at(index)->select_in_browser();
            break;

        case 1: {
            QString s = bn->full_name(TRUE);

            if ((index = list.findIndex(s)) == -1) {
                // new operation, may be created through an other dialog
                index = 0;
                QStringList::Iterator iter = list.begin();
                QStringList::Iterator iter_end = list.end();

                while ((iter != iter_end) && (*iter < s)) {
                    ++iter;
                    index += 1;
                }

                opers.insert((unsigned) index, bn);
                list.insert(iter, s);
                edspecification->insertItem(s, index + 1);
            }
        }

        edspecification->setCurrentItem(index + 1);
        break;

        default:
            break;
        }
    }
}

ActivityDialog::~ActivityDialog()
{
    activity->browser_node->edit_end();
    previous_size = size();

    foreach (BodyDialog *dialog, edits)
        dialog->close();
    edits.clear();

    close_dialog(this);
}

void ActivityDialog::change_tabs(QWidget *)
{
    if (!visit)
        edname->setFocus();
}

void ActivityDialog::init_tab(CondDialog & d, InfoData & cd,
                              const char * lbl, bool enabled)
{
    Q3Grid * grid = new Q3Grid(2, this);
    grid->setMargin(5);
    grid->setSpacing(5);

    new QLabel(TR("Pre\ncondition : "), grid);
    d.edpre = new MultiLineEdit(grid);
    d.edpre->setText(cd.first);

    if (visit)
        d.edpre->setReadOnly(TRUE);

    new QLabel(TR("Post\ncondition : "), grid);
    d.edpost = new MultiLineEdit(grid);
    d.edpost->setText(cd.second);

    if (visit)
        d.edpost->setReadOnly(TRUE);

    addTab(grid, lbl);

    if (! enabled)
        removePage(grid);
}

void ActivityDialog::edit_description()
{
    edit(comment->text(), "activity_description", activity, TxtEdit,
         this, (post_edit) post_edit_description, edits);
}

void ActivityDialog::post_edit_description(ActivityDialog * d, QString s)
{
    d->comment->setText(s);
}

void ActivityDialog::edit_constraint()
{
    edit(constraint->text(), edname->text().stripWhiteSpace() + "_constraint",
         activity, TxtEdit, this, (post_edit) post_edit_constraint, edits);
}

void ActivityDialog::post_edit_constraint(ActivityDialog * d, QString s)
{
    d->constraint->setText(s);
}

void ActivityDialog::accept()
{
    if (!check_edits(edits) || !kvtable->check_unique())
        return;

    BrowserNode * bn = activity->browser_node;
    QString s;

    s = edname->text().stripWhiteSpace();

    if ((s != bn->get_name()) &&
        ((BrowserNode *) bn->parent())->wrong_child_name(s, UmlActivity,
                bn->allow_spaces(),
                bn->allow_empty()))
        msg_critical(TR("Error"), s + TR("\n\nillegal name or already used"));
    else {
        bn->set_name(s);

        bool newst = activity->set_stereotype(fromUnicode(edstereotype->currentText().stripWhiteSpace()));
        int index = list.findIndex(edspecification->currentText().stripWhiteSpace());

        activity->set_specification((index != -1)
                                    ? (BrowserOperation *) opers.at(index)
                                    : 0);

        activity->read_only = readonly_cb->isChecked();
        activity->single_execution = singlexec_cb->isChecked();
        activity->is_active = active_cb->isChecked();

        uml.accept(activity->uml_condition);
        cpp.accept(activity->cpp_condition);
        java.accept(activity->java_condition);

        bn->set_comment(comment->text());
        UmlWindow::update_comment_if_needed(bn);

        activity->constraint = constraint->stripWhiteSpaceText();

        kvtable->updateNodeFromThis(bn);

        ProfiledStereotypes::modified(bn, newst);

        bn->modified();
        bn->package_modified();
        activity->modified();

        Q3TabDialog::accept();
    }
}

void CondDialog::accept(InfoData & cond)
{
    cond.first = edpre->text().stripWhiteSpace();
    cond.second = edpost->text().stripWhiteSpace();
}
