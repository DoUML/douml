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





#include <qlabel.h>
#include <qlayout.h>
#include <q3combobox.h>
#include <qpushbutton.h>
#include <qapplication.h>
//Added by qt3to4:
#include <Q3HBoxLayout>
#include <Q3VBoxLayout>
//Added by qt3to4:
#include <Q3PtrList>

#include "ReferenceDialog.h"
#include "BrowserView.h"
#include "UmlDesktop.h"
#include "DialogUtil.h"
#include "translate.h"

ReferenceDialog * ReferenceDialog::the;
QSize ReferenceDialog::previous_size;

ReferenceDialog::ReferenceDialog(BrowserNode * bn)
    : QDialog(0, "Referenced By dialog", FALSE, Qt::WDestructiveClose)
{
    the = this;
    target = bn;

    setCaption(TR("Referenced By dialog"));

    Q3VBoxLayout * vbox = new Q3VBoxLayout(this);

    vbox->setMargin(5);

    QString s = target->get_name();

    s += TR(" is referenced by :");

    vbox->addWidget(new QLabel(s, this));

    results = new Q3ComboBox(FALSE, this);
    vbox->addWidget(results);

    Q3HBoxLayout * hbox = new Q3HBoxLayout(vbox);
    QPushButton * search_b = new QPushButton(TR("Recompute"), this);
    QPushButton * close_b = new QPushButton(TR("Close"), this);

    hbox->setMargin(5);
    hbox->addWidget(search_b);
    hbox->addWidget(select_b = new QPushButton(TR("Select"), this));
    hbox->addWidget(mark_unmark_b = new QPushButton(TR("Unmark"), this));
    hbox->addWidget(mark_them_b = new QPushButton(TR("Mark them"), this));
    hbox->addWidget(unmark_all_b = new QPushButton(TR("Unmark all"), this));
    hbox->addWidget(close_b);

    search_b->setDefault(TRUE);

    connect(search_b, SIGNAL(clicked()), this, SLOT(compute()));
    connect(select_b, SIGNAL(clicked()), this, SLOT(select()));
    connect(close_b, SIGNAL(clicked()), this, SLOT(reject()));
    connect(mark_unmark_b, SIGNAL(clicked()), this, SLOT(mark_unmark()));
    connect(mark_them_b, SIGNAL(clicked()), this, SLOT(mark_them()));
    connect(unmark_all_b, SIGNAL(clicked()), this, SLOT(unmark_all()));
    connect(results, SIGNAL(activated(int)), this, SLOT(selected(int)));

    compute();

    open_dialog(this);
}

ReferenceDialog::~ReferenceDialog()
{
    the = 0;
    previous_size = size();

    close_dialog(this);
}

void ReferenceDialog::polish()
{
    QDialog::polish();
    UmlDesktop::limitsize_center(this, previous_size, 0.8, 0.8);
}

void ReferenceDialog::compute()
{
    QApplication::setOverrideCursor(Qt::waitCursor);

    QList<BrowserNode *> l;

    nodes.clear();
    results->clear();
    target->referenced_by(l);

    foreach (BrowserNode * bn, l)
        nodes.append(bn);

    nodes.sort();

    // remove duplicats
    BrowserNode *prevNode = 0;
    foreach (BrowserNode * bn, l) {
        if (prevNode == bn)
            nodes.removeOne(bn);
    }

    QStringList names;

    nodes.full_names(names);

    QStringList::Iterator it = names.begin();
    foreach (BrowserNode * bn, nodes) {
        results->insertItem(*(bn->pixmap(0)), *it);
        ++it;
    }

    selected((nodes.isEmpty()) ? -1 : 0);

    QApplication::restoreOverrideCursor();
}

void ReferenceDialog::selected(int index)
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

void ReferenceDialog::mark_unmark()
{
    BrowserNode * bn = nodes.at(results->currentItem());

    bn->toggle_mark();  	// call update
    BrowserView::force_visible(bn);
}

void ReferenceDialog::mark_them()
{
    foreach (BrowserNode *bn, nodes) {
        if (! bn->markedp()) {
            bn->toggle_mark();  	// call update
            BrowserView::force_visible(bn);
        }
    }
}

void ReferenceDialog::unmark_all()
{
    BrowserNode::unmark_all();  	// call update
}

void ReferenceDialog::select()
{
    if (!nodes.isEmpty())
        nodes.at(results->currentItem())->select_in_browser();
}

void ReferenceDialog::update()
{
    selected((results->count() != 0) ? results->currentItem() : -1);
}

void ReferenceDialog::show(BrowserNode * target)
{
    if (the != 0)
        the->close(TRUE);

    (new ReferenceDialog(target))->QDialog::show();
}
