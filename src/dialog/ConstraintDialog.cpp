// *************************************************************************
//
// Copyright 2004-2010 Bruno PAGES  .
// Copyright 2012-2013 Nikolai Marchenko.
// Copyright 2012-2013 Leonardo Guilherme.
//
// This file is part of the DOUML Uml Contraintkit.
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
#include <qlabel.h>
#include <qpushbutton.h>
#include <qcheckbox.h>
//Added by qt3to4:
#include <Q3HBoxLayout>
#include <Q3VBoxLayout>
#include <Q3ValueList>

#include "ConstraintDialog.h"
#include "ConstraintCanvas.h"
#include "CdClassCanvas.h"
#include "BrowserNode.h"
#include "UmlPixmap.h"
#include "UmlDesktop.h"
#include "strutil.h"
#include "MyTable.h"
#include "translate.h"

QSize ConstraintDialog::previous_size;

ConstraintDialog::ConstraintDialog(ConstraintCanvas * c)
    : QDialog(0, "ConstraintVisibilityDialog", TRUE, 0), constraint(c)
{
    setCaption(TR("Constraints visibility dialog"));

    Q3VBoxLayout * vbox = new Q3VBoxLayout(this);

    vbox->setMargin(5);

    table = new ConstraintTable(this, constraint);
    vbox->addWidget(table);
    vbox->addWidget(new QLabel(this));

    Q3HBoxLayout * hbox;

    hbox = new Q3HBoxLayout(vbox);

    cb_visible = new QCheckBox(TR("Specify visible elements rather than hidden ones"), this);
    cb_visible->setChecked(constraint->indicate_visible);
    hbox->addWidget(cb_visible);

    QPushButton * showall = new QPushButton(TR("Show all"), this);
    QPushButton * hideall = new QPushButton(TR("Hide all"), this);
    QPushButton * hideinherited = new QPushButton(TR("Hide inherited"), this);
    QSize bs = hideinherited->sizeHint();

    showall->setFixedSize(bs);
    hideall->setFixedSize(bs);
    hideinherited->setFixedSize(bs);

    hbox->addWidget(new QLabel(this));
    hbox->addWidget(showall);
    hbox->addWidget(new QLabel(this));
    hbox->addWidget(hideall);
    hbox->addWidget(new QLabel(this));
    hbox->addWidget(hideinherited);
    hbox->addWidget(new QLabel(this));

    connect(showall, SIGNAL(clicked()), this, SLOT(show_all()));
    connect(hideall, SIGNAL(clicked()), this, SLOT(hide_all()));
    connect(hideinherited, SIGNAL(clicked()), this, SLOT(hide_inherited()));

    vbox->addWidget(new QLabel(this));
    hbox = new Q3HBoxLayout(vbox);

    hbox->setMargin(5);
    QPushButton * ok = new QPushButton(TR("&OK"), this);
    QPushButton * cancel = new QPushButton(TR("&Cancel"), this);

    ok->setDefault(TRUE);
    bs = cancel->sizeHint();
    ok->setFixedSize(bs);
    cancel->setFixedSize(bs);

    hbox->addWidget(ok);
    hbox->addWidget(cancel);

    connect(ok, SIGNAL(clicked()), this, SLOT(accept()));
    connect(cancel, SIGNAL(clicked()), this, SLOT(reject()));
}

ConstraintDialog::~ConstraintDialog()
{
    previous_size = size();
}

void ConstraintDialog::polish()
{
    static bool first = TRUE;

    QDialog::polish();

    if (first) {
        first = FALSE;

        QSize sz(width() * 2, height());

        resize(sz);
    }

    UmlDesktop::limitsize_center(this, previous_size, 0.9, 0.9);
}


void ConstraintDialog::show_all()
{
    table->show_all();
}

void ConstraintDialog::hide_all()
{
    table->hide_all();
}

void ConstraintDialog::hide_inherited()
{
    table->hide_inherited(constraint);
}

void ConstraintDialog::accept()
{
    constraint->indicate_visible = cb_visible->isChecked();
    table->update(constraint);

    QDialog::accept();
}

// ConstraintTable
//
// v/h, icon, name, constraint

ConstraintTable::ConstraintTable(QWidget * parent, ConstraintCanvas * c)
    : Q3Table(c->elements.count(), 4, parent)
{
    setSorting(true);
    setSelectionMode(NoSelection);
    setRowMovingEnabled(FALSE);

    horizontalHeader()->setLabel(0, TR("visible"));
    horizontalHeader()->setLabel(1, " ");
    horizontalHeader()->setLabel(2, TR("element"));
    horizontalHeader()->setLabel(3, TR("constraint"));
    setColumnStretchable(0, FALSE);
    setColumnStretchable(1, FALSE);

    bool v = c->indicate_visible;
    Q3ValueList<BrowserNode *> & hv = c->hidden_visible;
    BrowserNodeList & elts = c->elements;
    QString yes = TR("  yes");

    elts.sort();

    int row = 0;
    foreach (BrowserNode *bn, elts) {
        if ((v) ? hv.findIndex(bn) != -1 : hv.findIndex(bn) == -1)
            setText(row, 0, yes);

        setPixmap(row, 1, *(bn->pixmap(0)));

        TableItem * ti;

        ti = new TableItem(this, Q3TableItem::Never, bn->full_name(TRUE));
        ti->setReplaceable(FALSE);
        setItem(row, 2, ti);

        QString s = toUnicode(bn->constraint());
        int n = s.count('\n');

        ti = new TableItem(this, Q3TableItem::Never, s);
        ti->setReplaceable(FALSE);
        setItem(row, 3, ti);

        if (n != 0) {
            // note : adjustRow(row) does nothing
            setRowHeight(row, rowHeight(row) * (n + 1));
        }
        ++row;
    }

    adjustColumn(0);
    adjustColumn(1);
    setColumnStretchable(2, TRUE);
    setColumnStretchable(3, TRUE);

    connect(this, SIGNAL(pressed(int, int, int, const QPoint &)),
            this, SLOT(button_pressed(int, int, int, const QPoint &)));
}

void ConstraintTable::button_pressed(int row, int col, int, const QPoint &)
{
    if (col == 0)
        setText(row, col, (text(row, col).isEmpty()) ? TR("  yes") : QString());
}

void ConstraintTable::show_all()
{
    int row;

    for (row = 0; row != numRows(); row += 1)
        setText(row, 0, TR("  yes"));
}

void ConstraintTable::hide_all()
{
    QString empty;
    int row;

    for (row = 0; row != numRows(); row += 1)
        setText(row, 0, empty);
}

void ConstraintTable::hide_inherited(ConstraintCanvas * c)
{
    BrowserNode * cl = c->cl->get_bn();
    BrowserNodeList & elts = c->elements;
    QString yes = TR("  yes");
    QString empty;

    int row = 0;
    foreach (BrowserNode *bn, elts) {
        setText(row, 0,
                ((bn == cl) || (bn->parent() == cl)) ? yes : empty);
        ++row;
    }
}

void ConstraintTable::update(ConstraintCanvas * c)
{
    Q3ValueList<BrowserNode *> & list = c->hidden_visible;
    bool empty_if_visible = !c->indicate_visible;
    BrowserNodeList & elts = c->elements;

    list.clear();

    int row = 0;
    foreach (BrowserNode *bn, elts) {
        if (text(row, 0).isEmpty() == empty_if_visible)
            list.append(bn);
        ++row;
    }
}
