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

#ifndef ACTIVITYPARTITIONDIALOG_H
#define ACTIVITYPARTITIONDIALOG_H





#include <q3ptrlist.h>
#include <q3tabdialog.h>
#include <q3ptrlist.h>

class Q3ComboBox;
class QCheckBox;

class LineEdit;
class MultiLineEdit;
class ActivityPartitionData;
class KeyValuesTable;
class BodyDialog;
class BrowserNode;

class ActivityPartitionDialog : public Q3TabDialog
{
    Q_OBJECT

protected:
    QWidget * umltab;
    ActivityPartitionData * data;
    LineEdit * edname;
    Q3ComboBox * edstereotype;
    Q3ComboBox * edrepresents;
    BrowserNode * represented;
    QCheckBox * dimension_cb;
    QCheckBox * external_cb;
    MultiLineEdit * comment;
    KeyValuesTable * kvtable;
    QList<BodyDialog *> edits;

    static QSize previous_size;

    static void post_edit_description(ActivityPartitionDialog * d, QString s);

public:
    ActivityPartitionDialog(ActivityPartitionData * nd);
    virtual ~ActivityPartitionDialog();

protected slots:
    virtual void polish();
    virtual void accept();
    void edit_description();
    void menu_represents();

    void change_tabs(QWidget *);
};

#endif
