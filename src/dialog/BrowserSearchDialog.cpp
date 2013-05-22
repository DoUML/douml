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





#include <qlayout.h>
#include <q3hbox.h>
#include <qlabel.h>
#include <q3combobox.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qcheckbox.h>
#include <q3groupbox.h>
#include <q3buttongroup.h>
#include <qapplication.h>
//Added by qt3to4:
#include <Q3HBoxLayout>
#include <Q3GridLayout>
#include <Q3VBoxLayout>

#include "BrowserSearchDialog.h"
#include "BrowserView.h"
#include "UmlDesktop.h"
#include "BrowserPackage.h"
#include "UmlEnum.h"
#include "DialogUtil.h"
#include "translate.h"

BrowserSearchDialog * BrowserSearchDialog::the;
int BrowserSearchDialog::saved_kind;
QString BrowserSearchDialog::saved_ed;
bool BrowserSearchDialog::saved_case_sensitive;
bool BrowserSearchDialog::saved_even_deleted;
bool BrowserSearchDialog::saved_name = TRUE;
bool BrowserSearchDialog::saved_stereotype;
bool BrowserSearchDialog::saved_comment;
bool BrowserSearchDialog::saved_decldefbody;
QSize BrowserSearchDialog::previous_size;

static const struct {
    const char * lbl;
    UmlCode k;
} Kinds[] = {
    { "<Any>", UmlCodeSup },
    { "activity", UmlActivity },
    { "activity action", UmlActivityAction },
    { "activity diagram", UmlActivityDiagram },
    { "activity expansion region", UmlExpansionRegion },
    { "activity interruptible region", UmlInterruptibleActivityRegion },
    { "activity node", UmlActivityNode },
    { "activity object", UmlActivityObject },
    { "activity parameter", UmlParameter },
    { "activity partition", UmlActivityPartition },
    { "attribute", UmlAttribute },
    { "artifact ", UmlArtifact },
    { "class ", UmlClass },
    { "class instance", UmlClassInstance },
    { "class diagram", UmlClassDiagram },
    { "class view", UmlClassView },
    { "communication diagram", UmlColDiagram },
    { "component", UmlComponent },
    { "component diagram", UmlComponentDiagram },
    { "component view", UmlComponentView },
    { "deployment diagram", UmlDeploymentDiagram },
    { "deployment view", UmlDeploymentView },
    { "extra class member", UmlExtraMember },
    { "flow", UmlFlow },
    { "interruptible activity region", UmlInterruptibleActivityRegion },
    { "expansion region", UmlExpansionRegion },
    { "node", UmlDeploymentNode },
    { "object diagram", UmlObjectDiagram },
    { "operation", UmlOperation },
    { "package", UmlPackage },
    { "parameter set", UmlParameterSet },
    { "pin", UmlActivityPin },
    { "relation", UmlRelations },
    { "sequence diagram", UmlSeqDiagram },
    { "state", UmlState },
    { "state action", UmlStateAction, },
    { "state diagram", UmlStateDiagram },
    { "state region", UmlRegion },
    { "transition", UmlTransition },
    { "use case", UmlUseCase },
    { "use case diagram", UmlUseCaseDiagram },
    { "use case view", UmlUseCaseView }
};

BrowserSearchDialog::BrowserSearchDialog()
    : QDialog(0, "Browser search", FALSE, Qt::WDestructiveClose)
{
    the = this;
    setCaption(TR("Browser search"));

    Q3VBoxLayout * vbox = new Q3VBoxLayout(this);

    vbox->setMargin(5);

    Q3GridLayout * gl = new Q3GridLayout(vbox, 4, 2, 5/*space*/);

    gl->addWidget(new QLabel(TR("Kind"), this), 0, 0, Qt::AlignLeft);
    kind = new Q3ComboBox(FALSE, this);

    for (int index = 0; index != sizeof(Kinds) / sizeof(*Kinds); index += 1)
        kind->insertItem(TR(Kinds[index].lbl));

    kind->setCurrentItem(saved_kind);
    gl->addWidget(kind, 0, 1);

    ed = new LineEdit(this);
    ed->setText(saved_ed);
    gl->addWidget(new QLabel(TR("Containing"), this), 1, 0, Qt::AlignLeft);
    gl->addWidget(ed, 1, 1);
    ed->setFocus();

    Q3HBox * hb = new Q3HBox(this);

    gl->addWidget(hb, 2, 1);

    Q3GroupBox * gb = new Q3GroupBox(2, Qt::Horizontal, hb);

    case_sensitive = new QCheckBox(TR("case sensitive"), gb);
    case_sensitive->setChecked(saved_case_sensitive);
    even_deleted = new QCheckBox(TR("even deleted"), gb);
    even_deleted->setChecked(saved_even_deleted);

    Q3ButtonGroup * bg = new Q3ButtonGroup(4, Qt::Horizontal, hb);

    bg->setExclusive(TRUE);
    for_name = new QRadioButton(TR("name"), bg);
    for_name->setChecked(saved_name);
    for_stereotype = new QRadioButton(TR("stereotype"), bg);
    for_stereotype->setChecked(saved_stereotype);
    for_comment = new QRadioButton(TR("description"), bg);
    for_comment->setChecked(saved_comment);
    for_decldefbody = new QRadioButton(TR("declaration/definition/body"), bg);
    for_decldefbody->setChecked(saved_decldefbody);

    gl->addWidget(new QLabel(TR("Result"), this), 3, 0, Qt::AlignLeft);
    results = new Q3ComboBox(FALSE, this);
    gl->addWidget(results, 3, 1);

    Q3HBoxLayout * hbox = new Q3HBoxLayout(vbox);
    QPushButton * search_b = new QPushButton(TR("Search"), this);
    QPushButton * close_b = new QPushButton(TR("Close"), this);

    hbox->setMargin(5);
    hbox->addWidget(search_b);
    hbox->addWidget(select_b = new QPushButton(TR("Select"), this));
    hbox->addWidget(mark_unmark_b = new QPushButton(TR("Unmark"), this));
    hbox->addWidget(mark_them_b = new QPushButton(TR("Mark them"), this));
    hbox->addWidget(unmark_all_b = new QPushButton(TR("Unmark all"), this));
    hbox->addWidget(close_b);

    search_b->setDefault(TRUE);

    connect(search_b, SIGNAL(clicked()), this, SLOT(search()));
    connect(select_b, SIGNAL(clicked()), this, SLOT(select()));
    connect(close_b, SIGNAL(clicked()), this, SLOT(reject()));
    connect(mark_unmark_b, SIGNAL(clicked()), this, SLOT(mark_unmark()));
    connect(mark_them_b, SIGNAL(clicked()), this, SLOT(mark_them()));
    connect(unmark_all_b, SIGNAL(clicked()), this, SLOT(unmark_all()));
    connect(results, SIGNAL(activated(int)), this, SLOT(selected(int)));

    if ((saved_kind != 0) || !saved_ed.isEmpty())
        search();
    else
        selected(-1);

    open_dialog(this);
}

void BrowserSearchDialog::polish()
{
    QDialog::polish();
    //setMaximumHeight(sizeHint().height());
    //setMaximumWidth(29*UmlDesktop::width()/30)
    UmlDesktop::limitsize_move(this, previous_size, 29.0 / 30, 0.9);
}

BrowserSearchDialog::~BrowserSearchDialog()
{
    the = 0;
    saved_kind = kind->currentItem();
    saved_ed = ed->text();
    saved_case_sensitive = case_sensitive->isChecked();
    saved_even_deleted = even_deleted->isChecked();
    saved_name = for_name->isChecked();
    saved_stereotype = for_stereotype->isChecked();
    saved_comment = for_comment->isChecked();
    saved_decldefbody = for_decldefbody->isChecked();
    previous_size = size();

    close_dialog(this);
}

void BrowserSearchDialog::search()
{
    QApplication::setOverrideCursor(Qt::waitCursor);

    results->clear();

    nodes.clear();

    if (for_name->isChecked())
        nodes.search(BrowserView::get_project(), Kinds[kind->currentItem()].k,
                     ed->text(), case_sensitive->isChecked(),
                     even_deleted->isChecked(), TRUE, FALSE);
    else if (for_stereotype->isChecked())
        nodes.search(BrowserView::get_project(), Kinds[kind->currentItem()].k,
                     ed->text(), case_sensitive->isChecked(),
                     even_deleted->isChecked(), FALSE, TRUE);
    else if (for_comment->isChecked())
        nodes.search(BrowserView::get_project(), Kinds[kind->currentItem()].k,
                     ed->text(), case_sensitive->isChecked(),
                     even_deleted->isChecked(), FALSE, FALSE);
    else
        nodes.search_ddb(BrowserView::get_project(), Kinds[kind->currentItem()].k,
                         ed->text(), case_sensitive->isChecked(),
                         even_deleted->isChecked());

    if (! nodes.isEmpty()) {
        nodes.sort();

        foreach (BrowserNode *bn, nodes)
            results->insertItem(*(bn->pixmap(0)), bn->full_name(TRUE));

        selected(0);
    }
    else
        selected(-1);

    QApplication::restoreOverrideCursor();
}

void BrowserSearchDialog::selected(int index)
{
    if (index == -1) {
        select_b->setEnabled(FALSE);
        mark_unmark_b->setEnabled(FALSE);
        mark_them_b->setEnabled(FALSE);
    }
    else {
        select_b->setEnabled(TRUE);
        mark_unmark_b->setEnabled(TRUE);
        mark_unmark_b->setText((nodes.at(index)->markedp())
                               ? TR("Unmark") : TR("Mark"));
        mark_them_b->setEnabled(TRUE);
    }

    unmark_all_b->setEnabled(!BrowserNode::marked_nodes().isEmpty());
}

void BrowserSearchDialog::mark_unmark()
{
    BrowserNode * bn = nodes.at(results->currentItem());

    bn->toggle_mark();  	// call update
    BrowserView::force_visible(bn);
}

void BrowserSearchDialog::mark_them()
{
    foreach (BrowserNode *bn, nodes) {
        if (! bn->markedp()) {
            bn->toggle_mark();  	// call update
            BrowserView::force_visible(bn);
        }
    }
}

void BrowserSearchDialog::unmark_all()
{
    BrowserNode::unmark_all();  	// call update
}

void BrowserSearchDialog::select()
{
    if (results->count() != 0)
        nodes.at(results->currentItem())->select_in_browser();
}

void BrowserSearchDialog::update()
{
    selected((results->count() != 0) ? results->currentItem() : -1);
}
