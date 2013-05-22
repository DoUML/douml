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
#include <qlabel.h>
#include <qcheckbox.h>
#include <qpushbutton.h>
#include <qapplication.h>
//Added by qt3to4:
#include <Q3VBoxLayout>
#include <Q3HBoxLayout>
//Added by qt3to4:
#include <Q3PtrList>

#include "SynchroDialog.h"
#include "BrowserView.h"

SynchroDialog::SynchroDialog(QList<BrowserView *> & b)
    : QDialog(0, "Synchronize", TRUE), browsers(b)
{
    setCaption("Synchronize");

    Q3VBoxLayout * vbox = new Q3VBoxLayout(this);
    Q3HBoxLayout * hbox;

    vbox->setMargin(5);

    bool has_ro = FALSE;
    bool has_ro_need_update = FALSE;
    bool has_need_update = FALSE;
    Q3PtrListIterator<BrowserView> it(browsers);

    for (; it.current(); ++it) {
        if (it.current()->is_need_update()) {
            if (it.current()->is_cant_update()) {
                has_ro = TRUE;
                has_ro_need_update = TRUE;
            }
            else
                has_need_update = TRUE;
        }
    }

    QLabel * lbl;

    if (has_need_update) {
        lbl =
            new QLabel("\nChoose the projects to synchronize with the others\n",
                       this);

        lbl->setAlignment(::Qt::AlignCenter);
        vbox->addWidget(lbl);

        if (has_ro_need_update) {
            lbl =
                new QLabel("\nWarning : some projects can't be synchronized because of read-only files\n",
                           this);

            lbl->setAlignment(::Qt::AlignCenter);
            vbox->addWidget(lbl);
        }
    }
    else if (has_ro_need_update) {
        lbl =
            new QLabel("\nSynchronisation not possible because of read-only files\n",
                       this);

        lbl->setAlignment(::Qt::AlignCenter);
        vbox->addWidget(lbl);
    }
    else {
        lbl =
            new QLabel("\nAll the projects are synchronized\n",
                       this);

        lbl->setAlignment(::Qt::AlignCenter);
        vbox->addWidget(lbl);
    }

    //

    for (it.toFirst() ; it.current(); ++it) {
        if (it.current()->is_need_update()) {
            QCheckBox * cb = new QCheckBox(it.current()->get_dir().path(), this);

            vbox->addWidget(cb);

            if (it.current()->is_cant_update())
                cb->setEnabled(FALSE);
            else
                checks.append(cb);
        }
    }

    //

    hbox = new Q3HBoxLayout(vbox);
    hbox->setMargin(5);

    if (has_need_update) {
        QPushButton * ok = new QPushButton("Synchronize", this);

        hbox->addWidget(new QLabel(this));
        hbox->addWidget(ok);
        connect(ok, SIGNAL(clicked()), this, SLOT(accept()));
    }

    QPushButton * cancel = new QPushButton("Cancel", this);

    hbox->addWidget(new QLabel(this));
    hbox->addWidget(cancel);
    hbox->addWidget(new QLabel(this));
    connect(cancel, SIGNAL(clicked()), this, SLOT(reject()));
}

SynchroDialog::~SynchroDialog()
{
}

void SynchroDialog::accept()
{
    Q3PtrListIterator<BrowserView> itprj(browsers);
    Q3PtrListIterator<QCheckBox> itcb(checks);
    bool work = FALSE;

    QApplication::setOverrideCursor(Qt::waitCursor);

    for (; itprj.current(); ++itprj) {
        if (itprj.current()->is_need_update() &&
            !itprj.current()->is_cant_update()) {
            if (itcb.current()->isChecked()) {
                itprj.current()->synchronize();
                work = TRUE;
            }

            ++itcb;
        }
    }

    QApplication::restoreOverrideCursor();

    if (work)
        QDialog::accept();
}

