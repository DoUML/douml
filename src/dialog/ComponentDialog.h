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

#ifndef COMPONENTDIALOG_H
#define COMPONENTDIALOG_H





#include <q3ptrdict.h>
#include <qmap.h>
#include <q3tabdialog.h>
//Added by qt3to4:
#include <Q3ValueList>
//Added by qt3to4:
#include <Q3PtrList>

#include "BrowserComponent.h"

class Q3ComboBox;
class Q3ListBox;
class Q3VBox;
class LineEdit;
class MultiLineEdit;
class SimpleData;
class KeyValuesTable;
class FormalParamsTable;
class ActualParamsTable;
class BodyDialog;

class ComponentDialog : public Q3TabDialog
{
    Q_OBJECT

protected:
    QWidget * umltab;
    SimpleData * data;
    LineEdit * edname;
    Q3ComboBox * edstereotype;
    MultiLineEdit * comment;
    QList<BodyDialog *> edits;

    // required classes
    Q3VBox * rq_page;
    Q3ComboBox * rq_stereotypefilter;
    Q3ListBox * lb_rq_available;
    Q3ListBox * lb_rq;
    Q3ValueList<BrowserClass *> rqs;

    // provided classes
    Q3VBox * pr_page;
    Q3ComboBox * pr_stereotypefilter;
    Q3ListBox * lb_pr_available;
    Q3ListBox * lb_pr;
    Q3ValueList<BrowserClass *> prs;

    // realization classes
    Q3VBox * rz_page;
    Q3ComboBox * rz_stereotypefilter;
    Q3ListBox * lb_rz_available;
    Q3ListBox * lb_rz;
    Q3ValueList<BrowserClass *> rzs;

    // user
    KeyValuesTable * kvtable;

    static QSize previous_size;

    void init_uml_tab();
    void init_l_tab(Q3VBox *& page, Q3ComboBox *& stereotypefilter,
                    void (ComponentDialog::* filteractivated)(const QString & st),
                    const char * filter_slt,
                    const char * add_slt, const char * remove_slt,
                    Q3ListBox *& lb_available, Q3ListBox *& lb,
                    const Q3ValueList<BrowserClass *> & cls,
                    const char * lbl);

    static void post_edit_description(ComponentDialog * d, QString s);

public:
    ComponentDialog(SimpleData * nd);
    virtual ~ComponentDialog();

protected slots:
    virtual void polish();
    virtual void accept();
    void edit_description();
    void rq_stereotypeFilterActivated(const QString & st);
    void pr_stereotypeFilterActivated(const QString & st);
    void rz_stereotypeFilterActivated(const QString & st);
    void require_cls();
    void unrequire_cls();
    void provide_cls();
    void unprovide_cls();
    void realize_cls();
    void unrealize_cls();

    void change_tabs(QWidget *);
};

#endif
