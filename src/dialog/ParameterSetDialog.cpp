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

#include "ParameterSetDialog.h"
#include "UmlDesktop.h"
#include "BrowserParameterSet.h"
#include "BrowserActivityAction.h"
#include "ParameterSetData.h"
#include "BrowserPin.h"
#include "UmlWindow.h"
#include "DialogUtil.h"
#include "KeyValueTable.h"
#include "ListBoxBrowserNode.h"
#include "strutil.h"
#include "UmlPixmap.h"
#include "BodyDialog.h"
#include "ProfiledStereotypes.h"
#include "translate.h"

QSize ParameterSetDialog::previous_size;

ParameterSetDialog::ParameterSetDialog(ParameterSetData * nd)
    : Q3TabDialog(0, 0, FALSE, Qt::WDestructiveClose), data(nd)
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

    setCaption(TR("ParameterSet dialog"));

    init_uml_tab();
    init_pins_tab();

    // USER : list key - value

    Q3Grid * grid = new Q3Grid(2, this);

    grid->setMargin(5);
    grid->setSpacing(5);

    kvtable = new KeyValuesTable((BrowserParameterSet *) data->get_browser_node(),
                                 grid, !hasOkButton());
    addTab(grid, TR("Properties"));

    //

    connect(this, SIGNAL(currentChanged(QWidget *)),
            this, SLOT(change_tabs(QWidget *)));

    open_dialog(this);
}

ParameterSetDialog::~ParameterSetDialog()
{
    data->browser_node->edit_end();
    previous_size = size();

    foreach (BodyDialog *dialog, edits)
        dialog->close();
    edits.clear();

    close_dialog(this);
}

void ParameterSetDialog::change_tabs(QWidget * w)
{
    if (hasOkButton() && (w == umltab))
        edname->setFocus();
}

void ParameterSetDialog::polish()
{
    Q3TabDialog::polish();
    UmlDesktop::limitsize_center(this, previous_size, 0.8, 0.8);
}

void ParameterSetDialog::init_uml_tab()
{
    bool visit = !hasOkButton();

    BrowserParameterSet * bn =
        (BrowserParameterSet *) data->get_browser_node();
    Q3VBox * vbox;
    Q3Grid * grid = new Q3Grid(2, this);

    umltab = grid;
    grid->setMargin(5);
    grid->setSpacing(5);

    new QLabel(TR("name : "), grid);
    edname = new LineEdit(bn->get_name(), grid);
    edname->setReadOnly(visit);

    new QLabel(TR("stereotype : "), grid);
    edstereotype = new Q3ComboBox(TRUE, grid);
    edstereotype->insertItem(toUnicode(data->get_stereotype()));

    if (! visit) {
        edstereotype->insertStringList(BrowserParameterSet::default_stereotypes());
        edstereotype->insertStringList(ProfiledStereotypes::defaults(UmlParameterSet));
        edstereotype->setAutoCompletion(completion());
    }

    edstereotype->setCurrentItem(0);
    QSizePolicy sp = edstereotype->sizePolicy();
    sp.setHorData(QSizePolicy::Expanding);
    edstereotype->setSizePolicy(sp);

    vbox = new Q3VBox(grid);
    new QLabel(TR("description :"), vbox);

    if (! visit) {
        connect(new SmallPushButton(TR("Editor"), vbox), SIGNAL(clicked()),
                this, SLOT(edit_description()));
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

void ParameterSetDialog::init_pins_tab()
{
    bool visit = !hasOkButton();
    Q3HBox * hbox;
    QPushButton * button;
    Q3VBox * vbox = new Q3VBox(this);
    Q3VBox * page = vbox;
    const Q3ValueList<BrowserPin *> & inpins = data->pins;
    Q3ValueList<BrowserPin *>::ConstIterator it;

    if (!visit) {
        hbox = new Q3HBox(vbox);
        vbox = new Q3VBox(hbox);
        vbox->setMargin(5);
        (new QLabel(TR("Parameters out of Parameter Set"), vbox))->setAlignment(Qt::AlignCenter);
        lb_available = new Q3ListBox(vbox);
        lb_available->setSelectionMode(Q3ListBox::Multi);

        BrowserParameterSet * bn =
            (BrowserParameterSet *) data->get_browser_node();
        Q3ValueList<BrowserPin *> allpins =
            ((BrowserActivityAction *) bn->parent())->get_pins();

        for (it = allpins.begin(); it != allpins.end(); it++)
            if (inpins.find(*it) == inpins.end())
                lb_available->insertItem(new ListBoxBrowserNode(*it, (*it)->full_name(TRUE)));

        lb_available->sort();

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

    vbox->setMargin(5);
    (new QLabel(TR("Parameters in Parameter Set"), vbox))->setAlignment(Qt::AlignCenter);
    lb_member = new Q3ListBox(vbox);
    lb_member->setSelectionMode((visit) ? Q3ListBox::NoSelection
                                : Q3ListBox::Multi);

    for (it = inpins.begin(); it != inpins.end(); ++it)
        lb_member->insertItem(new ListBoxBrowserNode(*it, (*it)->full_name(TRUE)));

    addTab(page, TR("Parameters"));
}

void ParameterSetDialog::edit_description()
{
    edit(comment->text(), edname->text().stripWhiteSpace() + "_description",
         data, TxtEdit, this, (post_edit) post_edit_description, edits);
}

void ParameterSetDialog::post_edit_description(ParameterSetDialog * d, QString s)
{
    d->comment->setText(s);
}

void ParameterSetDialog::associate_cls()
{
    unsigned int i = 0;

    while (i < lb_available->count()) {
        Q3ListBoxItem * item = lb_available->item(i);

        if (item->selected()) {
            lb_available->takeItem(item);
            lb_member->insertItem(item);
        }
        else
            i += 1;
    }

    lb_member->sort();
}

void ParameterSetDialog::unassociate_cls()
{
    unsigned int i = 0;

    while (i < lb_member->count()) {
        Q3ListBoxItem * item = lb_member->item(i);

        if (item->selected()) {
            lb_member->takeItem(item);
            lb_available->insertItem(item);
        }
        else
            i += 1;
    }

    lb_available->sort();
}

void ParameterSetDialog::accept()
{
    if (!check_edits(edits) || !kvtable->check_unique())
        return;

    QString s = edname->text().stripWhiteSpace();
    BrowserParameterSet * bn =
        (BrowserParameterSet *) data->get_browser_node();

    if ((s != bn->get_name()) &&
        ((BrowserNode *) bn->parent())->wrong_child_name(s, bn->get_type(),
                bn->allow_spaces(),
                bn->allow_empty()))
        msg_critical(TR("Error"), edname->text() + TR("\n\nillegal name or already used"));
    else {
        bn->set_name(s);
        bn->set_comment(comment->text());
        UmlWindow::update_comment_if_needed(bn);

        QString stereotype =
            fromUnicode(edstereotype->currentText().stripWhiteSpace());
        bool newst = data->set_stereotype(stereotype);
        Q3ValueList<BrowserPin *> l;
        unsigned n = lb_member->count();

        for (unsigned i = 0; i != n; i += 1)
            l.append((BrowserPin *)
                     (((ListBoxBrowserNode *) lb_member->item(i))
                      ->browser_node));

        data->set_pins(l);

        kvtable->updateNodeFromThis(bn);

        ProfiledStereotypes::modified(bn, newst);

        bn->modified();
        bn->package_modified();
        data->modified();

        Q3TabDialog::accept();
    }
}
