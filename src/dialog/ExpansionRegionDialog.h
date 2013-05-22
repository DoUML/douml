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

#ifndef EXPANSIONREGIONDIALOG_H
#define EXPANSIONREGIONDIALOG_H





#include <q3ptrlist.h>
#include <q3tabdialog.h>
#include <q3ptrlist.h>

class Q3ComboBox;
class QCheckBox;

class LineEdit;
class MultiLineEdit;
class ExpansionRegionData;
class KeyValuesTable;
class BodyDialog;

class ExpansionRegionDialog : public Q3TabDialog
{
    Q_OBJECT

protected:
    QWidget * umltab;
    ExpansionRegionData * data;
    LineEdit * edname;
    Q3ComboBox * edstereotype;
    Q3ComboBox * edmode;
    QCheckBox * must_isolate_cb;
    MultiLineEdit * comment;
    KeyValuesTable * kvtable;
    QList<BodyDialog *> edits;

    static QSize previous_size;

    static void post_edit_description(ExpansionRegionDialog * d, QString s);

public:
    ExpansionRegionDialog(ExpansionRegionData * nd);
    virtual ~ExpansionRegionDialog();

protected slots:
    virtual void polish();
    virtual void accept();
    void edit_description();

    void change_tabs(QWidget *);
};

#endif
