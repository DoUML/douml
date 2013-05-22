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
#include <q3tabdialog.h>
//Added by qt3to4:
#include <Q3VBoxLayout>
//Added by qt3to4:
#include <Q3PtrList>

#include "BodyDialog.h"
#include "UmlDesktop.h"
#include "strutil.h"
#include "Tool.h"
#include "translate.h"

QSize BodyDialog::previous_size;

BodyDialog::BodyDialog(QString t, Q3TabDialog * d, post_edit pf,
                       EditType k, QString what, QList<BodyDialog *> & edits)
    : QDialog(d, what, d->isModal(), Qt::WDestructiveClose), dlg(d), f(pf), eds(edits)
{
    eds.append(this);
    what.replace(what.findRev('_'), 1, " ");

    switch (k) {
    case CppEdit:
        setCaption(what + " (C++)");
        break;

    case JavaEdit:
        setCaption(what + " (Java)");
        break;

    case PhpEdit:
        setCaption(what + " (Php)");
        break;

    case PythonEdit:
        setCaption(what + " (Python)");
        break;

    default:
        setCaption(what);
    }

    Q3VBoxLayout * vbox = new Q3VBoxLayout(this);

    vbox->addWidget(new QLabel((f == 0) ? TR("Note : operation bodies preserved")
                               : TR("You can specify the editor through the environment dialog"),
                               this));

    e = new MultiLineEdit(this);
    e->setText(t);
    e-> setReadOnly(f == 0);

    QFont font = e->font();

    if (! hasCodec())
        font.setFamily("Courier");

    font.setFixedPitch(TRUE);
    e->setFont(font);

    vbox->addWidget(e);
    e->setFocus();

    UmlDesktop::setsize_center(this, previous_size, 0.5, 0.5);

    open_dialog(this);
}

BodyDialog::~BodyDialog()
{
    previous_size = size();

    if (eds.remove(this) && (f != 0))
        // dialog still active
        f(dlg, e->text());

    close_dialog(this);
}
