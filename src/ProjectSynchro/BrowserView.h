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

#ifndef BROWSERVIEW_H
#define BROWSERVIEW_H

#include <q3listview.h>
#include <qdir.h>
#include <q3dict.h>
//Added by qt3to4:
#include <QLabel>
//Added by qt3to4:
#include <Q3PtrList>

class BrowserNode;
class QLabel;

class BrowserView : public Q3ListView
{
    Q_OBJECT

protected:
    QDir dir;
    Q3Dict<BrowserNode> nodes;
    QLabel * lbl;
    bool need_update;
    bool cant_update;

public:
    BrowserView(QWidget * parent);
    virtual ~BrowserView();

    void set_project(QDir dir);
    void close();

    BrowserNode * get_project() const;
    QDir & get_dir() {
        return dir;
    }

    void add_node(BrowserNode *);
    BrowserNode * get_node(QString f) const;

    bool is_need_update() const {
        return need_update;
    }
    bool is_cant_update() const {
        return cant_update;
    }
    void set_readonly() {
        cant_update = TRUE;
    }

    void synchronize();

    static void update(const QList<BrowserView *> &);

protected:
    void update_it();

protected slots:
    void select(Q3ListViewItem * b);
};

#endif
