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

#ifndef FLOWDIALOG_H
#define FLOWDIALOG_H

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
class FlowData;
class BrowserNode;
class KeyValuesTable;
struct FlowDef;
class BodyDialog;

struct FlDialog {
    LineEdit * edweight;
    MultiLineEdit * edguard;
    MultiLineEdit * edselection;
    MultiLineEdit * edtransformation;

    void accept(FlowDef &);
};

class FlowDialog : public Q3TabDialog
{
    Q_OBJECT

protected:
    QWidget * umltab;
    bool visit;
    FlowData * flow;
    LineEdit * edname;
    Q3ComboBox * edstereotype;
    MultiLineEdit * comment;
    QList<BodyDialog *> edits;

    FlDialog uml;
    FlDialog cpp;
    FlDialog java;

    // User
    KeyValuesTable * kvtable;

    static QSize previous_size;

    static void post_edit_description(FlowDialog * d, QString s);
    static void post_edit_uml_guard(FlowDialog * d, QString s);
    static void post_edit_uml_selection(FlowDialog * d, QString s);
    static void post_edit_uml_transformation(FlowDialog * d, QString s);
    static void post_edit_cpp_guard(FlowDialog * d, QString s);
    static void post_edit_cpp_selection(FlowDialog * d, QString s);
    static void post_edit_cpp_transformation(FlowDialog * d, QString s);
    static void post_edit_java_guard(FlowDialog * d, QString s);
    static void post_edit_java_selection(FlowDialog * d, QString s);
    static void post_edit_java_transformation(FlowDialog * d, QString s);

public:
    FlowDialog(FlowData * r);
    virtual ~FlowDialog();

protected:
    void init_tab(FlDialog &, FlowDef & fdef, const char * lbl,
                  const char * sl_guard, const char * sl_selection,
                  const char * sl_transformation, bool enabled);
protected slots:
    virtual void polish();
    virtual void accept();

    void edit_description();
    void edit_uml_guard();
    void edit_uml_selection();
    void edit_uml_transformation();
    void edit_cpp_guard();
    void edit_cpp_selection();
    void edit_cpp_transformation();
    void edit_java_guard();
    void edit_java_selection();
    void edit_java_transformation();

    void change_tabs(QWidget *);
};

#endif
