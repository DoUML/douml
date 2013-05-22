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

#ifndef BODYDIALOG_H
#define BODYDIALOG_H

#include <qdialog.h>
//Added by qt3to4:
#include <Q3PtrList>

#include "DialogUtil.h"

class Q3TabDialog;
class MultiLineEdit;

class BodyDialog : public QDialog
{
    Q_OBJECT

protected:
    MultiLineEdit * e;
    Q3TabDialog * dlg;
    post_edit f;
    QList<BodyDialog *> & eds;

    static QSize previous_size;

public:
    BodyDialog(QString t, Q3TabDialog * d, post_edit pf,
               EditType k, QString what, QList<BodyDialog *> & edits);
    virtual ~BodyDialog();
};

#endif

