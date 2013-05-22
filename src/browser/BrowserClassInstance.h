//Added by qt3to4:
#include <QPixmap>
#include <QTextStream>
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

#ifndef BROWSER_CLASSINSTANCE_H
#define BROWSER_CLASSINSTANCE_H

class QPixmap;

#include "BrowserNode.h"
#include "Labeled.h"

class ClassInstanceData;
class BrowserObjectDiagram;

class BrowserClassInstance : public BrowserNode, public Labeled<BrowserClassInstance>
{
    friend class StereotypesDialog;

protected:
    static IdDict<BrowserClassInstance> all;

    ClassInstanceData * def;
    BrowserObjectDiagram * associated_diagram;

protected:
    void exec_menu_choice(int rank);

public:
    BrowserClassInstance(QString s, BrowserClass * cl,
                         BrowserNode * p, int id = 0);
    BrowserClassInstance(const BrowserClassInstance * model, BrowserNode * p);
    BrowserClassInstance(int id);
    virtual ~BrowserClassInstance();

    virtual BrowserNode * get_associated() const;
    void set_associated_diagram(BrowserObjectDiagram *, bool on_read = FALSE);

    virtual bool undelete(bool, QString & warning, QString & renamed);
    virtual BrowserNode * duplicate(BrowserNode * p,
                                    QString name = QString());
    virtual QString full_name(bool rev = FALSE, bool itself = TRUE) const;
    virtual bool allow_empty() const;
    virtual void update_stereotype(bool rec = FALSE);


    virtual void menu();
    virtual void apply_shortcut(QString s);
    virtual void open(bool force_edit);
    virtual UmlCode get_type() const;
    virtual QString get_stype() const;
    virtual int get_identifier() const;
    virtual const char * help_topic() const;
    virtual BasicData * get_data() const;
    virtual void on_delete();
    static void post_load();

    virtual void save(QTextStream &, bool ref, QString & warning);
    static BrowserClassInstance * read_ref(char *&);
    static BrowserClassInstance * read(char *& , char *, BrowserNode *);
    static BrowserNode * get_it(const char * k, int id);

    static BrowserClassInstance * get_it(QString s, BrowserClass * cl,
                                         BrowserNode * parent);

    virtual bool tool_cmd(ToolCom * com, const char * args);
    virtual bool api_compatible(unsigned v) const;
    static void add_from_tool(BrowserNode * parent, ToolCom * com, const char * args);

    virtual void referenced_by(QList<BrowserNode *> &, bool ondelete = FALSE);
    static void compute_referenced_by(QList<BrowserNode *> &, BrowserNode *);

    static BrowserNodeList & instances(BrowserNodeList &, const char * st = 0);
    static BrowserClassInstance * get_classinstance(BrowserNode * future_parent);
    static BrowserClassInstance * add_classinstance(BrowserNode * future_parent);
    static BrowserClassInstance * get_classinstance(BrowserClass * cl);

    static void clear(bool old);
    static void update_idmax_for_root();
    virtual void renumber(int phase);
    virtual void prepare_update_lib() const;

    virtual const QPixmap * pixmap(int) const;
};

#endif

