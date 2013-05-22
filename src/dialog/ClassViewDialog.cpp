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
#include <qlabel.h>
#include <qpushbutton.h>
#include <q3combobox.h>
#include <q3vbox.h>

#include "ClassViewDialog.h"
#include "BasicData.h"
#include "BrowserClassView.h"
#include "BrowserDeploymentView.h"
#include "UmlWindow.h"
#include "DialogUtil.h"
#include "KeyValueTable.h"
#include "UmlDesktop.h"
#include "BodyDialog.h"
#include "strutil.h"
#include "ProfiledStereotypes.h"
#include "translate.h"

QSize ClassViewDialog::previous_size;

ClassViewDialog::ClassViewDialog(BasicData * nd)
    : Q3TabDialog(0, 0, FALSE, Qt::WDestructiveClose), data(nd)
{
    nd->get_browser_node()->edit_start();

    if (nd->get_browser_node()->is_writable()) {
        setOkButton(TR("OK"));
        setCancelButton(TR("Cancel"));
    }
    else {
        setOkButton(QString());
        setCancelButton(TR("Close"));
    }

    setCaption(TR("class view dialog"));

    bool visit = !hasOkButton();

    // general tab

    BrowserClassView * bn = (BrowserClassView *) data->get_browser_node();
    Q3Grid * grid = new Q3Grid(2, this);

    umltab = grid;
    grid->setMargin(5);
    grid->setSpacing(5);

    new QLabel(TR("name : "), grid);
    edname = new LineEdit(bn->get_name(), grid);
    edname->setReadOnly(visit);

    new QLabel(TR("stereotype : "), grid);
    edstereotype = new Q3ComboBox(!visit, grid);
    edstereotype->insertItem(toUnicode(data->get_stereotype()));

    if (!visit) {
        edstereotype->insertStringList(BrowserClassView::default_stereotypes());
        edstereotype->insertStringList(ProfiledStereotypes::defaults(UmlClassView));
        edstereotype->setAutoCompletion(completion());
    }

    edstereotype->setCurrentItem(0);
    QSizePolicy sp = edstereotype->sizePolicy();
    sp.setHorData(QSizePolicy::Expanding);
    edstereotype->setSizePolicy(sp);

    BrowserNode * bcv = bn->get_associated();

    if (visit) {
        if ((bcv != 0) && !bcv->deletedp()) {
            new QLabel(TR("deployment\nview : "), grid);
            deploymentview = new Q3ComboBox(FALSE, grid);

            BrowserNode * bcv = bn->get_associated();

            deploymentview->insertItem(bcv->full_name(TRUE));
            deploymentview->setCurrentItem(0);
        }
        else
            deploymentview = 0;
    }
    else {
        BrowserDeploymentView::instances(deploymentviews);

        if (!deploymentviews.isEmpty()) {
            QStringList deploymentview_names;

            deploymentviews.full_names(deploymentview_names);
            new QLabel(TR("deployment\nview : "), grid);
            deploymentview = new Q3ComboBox(FALSE, grid);
            deploymentview->insertItem("");
            deploymentview->insertStringList(deploymentview_names);

            if (bcv != 0) {
                if (bcv->deletedp()) {
                    QString fn = bcv->full_name(TRUE);

                    deploymentview->insertItem(fn);
                    deploymentviews.append(bcv);
                    deploymentview->setCurrentItem(deploymentviews.count());
                }
                else
                    deploymentview->setCurrentItem(deploymentviews.indexOf(bcv) + 1);
            }
            else
                deploymentview->setCurrentItem(0);
        }
        else
            deploymentview = 0;
    }

    Q3VBox * vtab = new Q3VBox(grid);
    new QLabel(TR("description :"), vtab);

    if (!visit)
        connect(new SmallPushButton(TR("Editor"), vtab), SIGNAL(clicked()),
                this, SLOT(edit_description()));

    comment = new MultiLineEdit(grid);
    comment->setReadOnly(visit);
    comment->setText(bn->get_comment());
    QFont font = comment->font();

    if (! hasCodec())
        font.setFamily("Courier");

    font.setFixedPitch(TRUE);
    comment->setFont(font);

    addTab(grid, "Uml");

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

void ClassViewDialog::polish()
{
    Q3TabDialog::polish();
    UmlDesktop::limitsize_move(this, previous_size, 0.8, 0.8);
}

ClassViewDialog::~ClassViewDialog()
{
    data->get_browser_node()->edit_end();
    previous_size = size();

    foreach (BodyDialog *dialog, edits)
        dialog->close();
    edits.clear();

    close_dialog(this);
}

void ClassViewDialog::change_tabs(QWidget * w)
{
    if (hasOkButton() && (w == umltab))
        edname->setFocus();
}

void ClassViewDialog::edit_description()
{
    edit(comment->text(), edname->text().stripWhiteSpace() + "_description",
         data, TxtEdit, this, (post_edit) post_edit_description, edits);
}

void ClassViewDialog::post_edit_description(ClassViewDialog * d, QString s)
{
    d->comment->setText(s);
}

void ClassViewDialog::accept()
{
    if (!check_edits(edits) || !kvtable->check_unique())
        return;

    QString s = edname->text().stripWhiteSpace();
    BrowserClassView * bn = (BrowserClassView *) data->get_browser_node();

    if ((s != bn->get_name()) &&
        ((BrowserNode *) bn->parent())->wrong_child_name(s, bn->get_type(),
                bn->allow_spaces(),
                bn->allow_empty()))
        msg_critical(TR("Error"), edname->text() + TR("\n\nillegal name or already used"));
    else {
        bn->set_name(s);

        bool newst = data->set_stereotype(fromUnicode(edstereotype->currentText().stripWhiteSpace()));

        if (deploymentview != 0) {
            int index = deploymentview->currentItem();

            bn->set_associated_deploymentview((index == 0)
                                              ? 0 // "" : no deploymentview
                                              : ((BrowserDeploymentView *) deploymentviews.at(index - 1)));
        }

        bn->set_comment(comment->text());
        UmlWindow::update_comment_if_needed(bn);

        kvtable->updateNodeFromThis(bn);

        ProfiledStereotypes::modified(bn, newst);

        data->modified();
        bn->package_modified();

        Q3TabDialog::accept();
    }
}
