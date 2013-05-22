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

#ifndef BASICDATA_H
#define BASICDATA_H

#include <qobject.h>
//Added by qt3to4:

#include <QTextStream>
//Added by qt3to4:
#include <Q3PtrList>

#include "mystr.h"

class QTextStream;
class BrowserNode;
class HaveKeyValueData;
class ToolCom;

// this class exists to be the parent of all the BasicData, else
// all of them will be placed in the list QObject::object_trees
// and during a close() they are probably deleted in the opposite
// order => removed in O(n*n) !!!!

class BasicParent : public QObject
{
public:
    virtual void insertChild(QObject *);	// does nothing !
    virtual void removeChild(QObject *);	// does nothing !

    static BasicParent the;
};


// This class exists because the BrowserNode and daughters cannot be
// an QObject (allowing to have a signal) because of inheritance problem.
// Notes than the two BrowserNodes corresponding to a relation extremities
// share the same BasicData

class BasicData : public QObject
{
    Q_OBJECT

protected:
    static QList<BasicData *> removed;

    WrapperStr stereotype;
    BrowserNode * browser_node;

    void redelete_it() {
        emit deleted();
    };

public:
    BasicData() : QObject(&BasicParent::the), browser_node(0) {};
    BasicData(const BasicData * model);
    virtual ~BasicData();

    virtual bool deletedp() const = 0;
    virtual void set_deletedp(bool y) = 0;
    virtual void delete_it();
    virtual void undelete(QString & warning, QString & renamed);
    static void resignal_deleted();

    const char * get_stereotype() const {
        return stereotype;
    }
    const char * get_short_stereotype() const;
    virtual bool set_stereotype(const QString &);
    virtual bool set_stereotype(const WrapperStr &);
    virtual bool set_stereotype(const char *);

    virtual void set_browser_node(BrowserNode * bn);
    BrowserNode * get_browser_node() const {
        return browser_node;
    };

    virtual QString definition(bool full, bool with_kind) const;

    virtual bool decldefbody_contain(const QString & s, bool cs,
                                     BrowserNode *);

    void save(QTextStream &, QString & warning) const;
    void read(char *& st, char *& k);

    // tools
    virtual bool tool_cmd(ToolCom * com, const char * args,
                          BrowserNode * bn, const QString & comment);
    virtual void send_uml_def(ToolCom * com, BrowserNode * bn,
                              const QString & comment);
    virtual void send_cpp_def(ToolCom * com);
    virtual void send_java_def(ToolCom * com);
    virtual void send_php_def(ToolCom * com);
    virtual void send_python_def(ToolCom * com);
    virtual void send_idl_def(ToolCom * com);

protected slots:
    void on_delete();
public slots:
    void modified();

signals:
    void changed();	// the object is modified
    void deleted();	// the object is deleted
};

#endif
