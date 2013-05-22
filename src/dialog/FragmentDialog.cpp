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





#include <qcursor.h>
#include <qlayout.h>
#include <qlabel.h>
#include <q3combobox.h>
#include <qpushbutton.h>
#include <q3popupmenu.h>
//Added by qt3to4:
#include <Q3HBoxLayout>
#include <Q3VBoxLayout>

#include "FragmentDialog.h"
#include "BrowserDiagram.h"
#include "DialogUtil.h"
#include "UmlDesktop.h"
#include "BrowserView.h"
#include "translate.h"

QSize FragmentDialog::previous_size;

FragmentDialog::FragmentDialog(const QStringList & defaults, QString & s,
                               QString & fo, BrowserNode *& d)
    : QDialog(0, "Fragment dialog", TRUE), name(s), form(fo), refer(d)
{
    setCaption(TR("Fragment dialog"));

    Q3VBoxLayout * vbox = new Q3VBoxLayout(this);
    Q3HBoxLayout * hbox;
    QLabel * lbl1;
    QLabel * lbl2;
    SmallPushButton * refer_bt;

    vbox->setMargin(5);

    hbox = new Q3HBoxLayout(vbox);
    hbox->setMargin(5);
    hbox->addWidget(lbl1 = new QLabel(TR("name : "), this));
    name_cb = new Q3ComboBox(TRUE, this);
    name_cb->insertItem(name);
    name_cb->setCurrentItem(0);
    name_cb->insertStringList(defaults);
    name_cb->setAutoCompletion(completion());
    hbox->addWidget(name_cb);

    QSizePolicy sp = name_cb->sizePolicy();

    sp.setHorData(QSizePolicy::Expanding);
    name_cb->setSizePolicy(sp);

    hbox = new Q3HBoxLayout(vbox);
    hbox->setMargin(5);
    hbox->addWidget(refer_bt = new SmallPushButton(TR("refer to : "), this));
    connect(refer_bt, SIGNAL(clicked()), this, SLOT(menu_refer()));
    diag_cb = new Q3ComboBox(FALSE, this);
    BrowserDiagram::instances(nodes, TRUE);
    diag_cb->insertItem("");

    foreach (BrowserNode *bn, nodes)
        diag_cb->insertItem(*(bn->pixmap(0)), bn->full_name(TRUE));

    diag_cb->setCurrentItem((refer == 0)
                            ? 0
                            : nodes.indexOf(refer) + 1);
    diag_cb->setSizePolicy(sp);
    hbox->addWidget(diag_cb);

    hbox = new Q3HBoxLayout(vbox);
    hbox->setMargin(5);
    hbox->addWidget(lbl2 = new QLabel(TR("arguments \n/ value : "), this));
    hbox->addWidget(ed_form = new LineEdit(this));
    ed_form->setText(form);

    same_width(lbl1, lbl2, refer_bt);

    hbox = new Q3HBoxLayout(vbox);
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

void FragmentDialog::polish()
{
    QDialog::polish();
    UmlDesktop::limitsize_move(this, previous_size, 0.8, 0.8);
}

FragmentDialog::~FragmentDialog()
{
    previous_size = size();
}

void FragmentDialog::menu_refer()
{
    Q3PopupMenu m(0);
    bool used = FALSE;

    m.insertItem(TR("Choose"), -1);
    m.insertSeparator();

    if (diag_cb->currentItem() != 0) {
        used = TRUE;
        m.insertItem(TR("Select in browser"), 0);
    }

    BrowserNode * bn = BrowserView::selected_item();

    if ((bn != 0) &&
        !bn->deletedp() &&
        (dynamic_cast<BrowserDiagram *>(bn) != 0)) {
        m.insertItem(TR("Choose diagram selected in browser"), 1);
        used = TRUE;
    }
    else
        bn = 0;

    if (used) {
        switch (m.exec(QCursor::pos())) {
        case 0:
            nodes.at(diag_cb->currentItem() - 1)->select_in_browser();
            break;

        case 1:
            diag_cb->setCurrentItem(nodes.indexOf(bn) + 1);
            break;

        default:
            break;
        }
    }
}

void FragmentDialog::accept()
{
    name = name_cb->currentText().stripWhiteSpace();
    form = ed_form->text().stripWhiteSpace();
    refer = (diag_cb->currentItem() == 0)
            ? (BrowserNode *) 0 : nodes.at(diag_cb->currentItem() - 1);

    QDialog::accept();
}
