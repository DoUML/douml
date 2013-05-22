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

#ifndef STATEACTIONDIALOG_H
#define STATEACTIONDIALOG_H

#include <qwidget.h>
#include <qstringlist.h>
#include <q3tabdialog.h>
//Added by qt3to4:
#include <Q3PtrList>

#include "BrowserNode.h"

class QPushButton;
class Q3ComboBox;

class LineEdit;
class MultiLineEdit;
class StateActionData;
class BrowserNode;
class KeyValuesTable;
class BodyDialog;

class StateActionDialog : public Q3TabDialog
{
    Q_OBJECT

protected:
    bool visit;
    StateActionData * action;
    Q3ComboBox * edstereotype;
    MultiLineEdit * comment;
    QList<BodyDialog *> edits;

    QWidget * umltab;
    QWidget * cppTab;
    QWidget * javatab;
    MultiLineEdit * uml;
    MultiLineEdit * cpp;
    MultiLineEdit * java;

    // User
    KeyValuesTable * kvtable;

    static QSize previous_size;

    static void post_edit_description(StateActionDialog * d, QString s);

public:
    StateActionDialog(StateActionData * r);
    virtual ~StateActionDialog();

protected slots:
    virtual void polish();
    virtual void accept();

    void edit_description();

    void change_tabs(QWidget *);
};

#endif
