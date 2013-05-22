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

#ifndef DIALOGUTIL_H
#define DIALOGUTIL_H

#include <qpushbutton.h>
#include <qlineedit.h>
#include <Q3TextEdit>
#include <qmessagebox.h>
//Added by qt3to4:
#include <Q3PopupMenu>
//Added by qt3to4:
#include <Q3PtrList>

#include "AType.h"

class QWidget;
class Q3PopupMenu;
class Q3TabDialog;
class BrowserNodeList;
class UmlCanvas;
class BodyDialog;
class BrowserNode;
class KeyValuesTable;
class HaveKeyValueData;

// a push button without magin
class SmallPushButton : public QPushButton
{
public:
    SmallPushButton(const QString & text, QWidget * parent);

    virtual QSize sizeHint() const;
};

// redefine text() to remove non latin1 characters
class MultiLineEdit : public Q3TextEdit
{
public:
    MultiLineEdit(QWidget * w, const char * name = 0);

    virtual void setText(const QString &);
    virtual QString text() const;

    QString stripWhiteSpaceText() const;

    void setTheText(const QString & s);

    QString theText() const;
};

// redefine text() to remove non latin1 characters
class LineEdit : public QLineEdit
{
public:
    LineEdit(QWidget * w);
    LineEdit(const QString & s, QWidget * parent, const char * name = 0);

    virtual void setText(const QString &);
    virtual QString text() const;

    void setTheText(const QString & s);

    QString theText() const;
};

extern void init_font_menu(Q3PopupMenu & fontsubm, UmlCanvas * the_canvas,
                           int index);
extern void same_width(QWidget *, QWidget *);
extern void same_width(QWidget *, QWidget *, QWidget *);
extern void same_width(QWidget *, QWidget *, QWidget *, QWidget *);
extern void same_width(QWidget *, QWidget *, QWidget *, QWidget *, QWidget *);

enum EditType { CppEdit, JavaEdit, PhpEdit, PythonEdit, TxtEdit };

typedef void (* post_edit)(Q3TabDialog *, QString);

extern void edit(const QString &, QString name, void * id, EditType k,
                 Q3TabDialog * tbl, post_edit pf, QList<BodyDialog *> & edits);
extern bool check_edits(QList<BodyDialog *> & edits);

extern AType the_type(const QString & t, const QStringList & types,
                      BrowserNodeList & nodes);

extern QString type(const QString & t, const QStringList & types,
                    BrowserNodeList & nodes);

// 'true' name or external name
extern QString get_cpp_name(const BrowserClass * cl,
                            ShowContextMode mode = noContext);
extern QString get_java_name(const BrowserClass * cl,
                             ShowContextMode mode = noContext);
extern QString get_php_name(const BrowserClass * cl,
                            ShowContextMode mode = noContext);
extern QString get_python_name(const BrowserClass * cl,
                               ShowContextMode mode = noContext);
extern QString get_idl_name(const BrowserClass * cl,
                            ShowContextMode mode = noContext);
extern QString get_cpp_name(const AType,
                            ShowContextMode mode = noContext);
extern QString get_java_name(const AType,
                             ShowContextMode mode = noContext);
extern QString get_php_name(const AType,
                            ShowContextMode mode = noContext);
extern QString get_python_name(const AType,
                               ShowContextMode mode = noContext);
extern QString get_idl_name(const AType,
                            ShowContextMode mode = noContext);

extern void manage_alias(const BrowserNode * node, const char *& p,
                         QString & s, KeyValuesTable *);
extern void manage_alias(const BrowserNode * node, const char *& p,
                         QString & s, HaveKeyValueData *);

extern int msg_warning(QString caption, QString text,
                       int button0 = QMessageBox::Ok,
                       int button1 = 0, int button2 = 0);
extern int msg_critical(QString caption, QString text,
                        int button0 = QMessageBox::Ok,
                        int button1 = 0, int button2 = 0);
extern int msg_information(QString caption, QString text,
                           int button0 = QMessageBox::Ok,
                           int button1 = 0, int button2 = 0);

extern QString editor();
extern void set_editor(QString);

extern void set_completion(bool y);
extern bool completion();

extern void open_dialog(QWidget *);
extern void close_dialog(QWidget *);
extern QWidgetList dialogs();

#endif
