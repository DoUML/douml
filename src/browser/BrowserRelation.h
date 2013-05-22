//Added by qt3to4:
#include <QPixmap>
#include <QDropEvent>
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

#ifndef BROWSER_RELATION_H
#define BROWSER_RELATION_H

template <class K> class Q3PtrDict;

class QPixmap;
class BrowserClass;
class RelationData;
class BrowserOperation;

#include "BrowserNode.h"
#include "Labeled.h"

class BrowserRelation : public BrowserNode, public Labeled<BrowserRelation>
{
friend class QuickEdit;
protected:
    static IdDict<BrowserRelation> all;

    RelationData * def;
    BrowserOperation * get_oper;
    BrowserOperation * set_oper;

    BrowserRelation(int id);

    void exec_menu_choice(int rank);

public:
    BrowserRelation(BrowserNode * p, RelationData * d, int id = 0);
    BrowserRelation(const BrowserRelation * model, BrowserNode * p);
    virtual ~BrowserRelation();

    virtual bool is_undefined() const;

    virtual void delete_it();
    virtual bool undelete(bool rec, QString & warning, QString & renamed);
    virtual BrowserNode * duplicate(BrowserNode * p,
                                    QString name = QString());

    virtual const char * get_comment() const;
    virtual void set_comment(const char * c);

    virtual UmlVisibility get_visibility() const override;

    BrowserOperation * get_get_oper() {
        return get_oper;
    };
    BrowserOperation * get_set_oper() {
        return set_oper;
    };
    void set_get_oper(BrowserOperation *);
    void set_set_oper(BrowserOperation *);
    void update_get_oper();
    void update_set_oper();
    void add_get_oper();
    void add_set_oper();

    virtual const QPixmap * pixmap(int) const;
    virtual void paintCell(QPainter *, const QColorGroup &, int, int, int);
    BrowserNode * extract();
    static BrowserRelation * reinsert(BrowserNode * p, RelationData * d);

    virtual void menu();
    virtual void apply_shortcut(QString s);
    virtual void open(bool);
    virtual bool in_edition() const;
    virtual UmlCode get_type() const;
    virtual QString get_stype() const;
    virtual int get_identifier() const;
    virtual const char * help_topic() const;
    virtual void modified();
    virtual BasicData * get_data() const;
    virtual bool same_name(const QString & s, UmlCode type) const;
    virtual QString full_name(bool rev = FALSE, bool itself = TRUE) const;
    virtual void member_cpp_def(const QString & prefix, const QString &,
                                QString & s, bool templ) const;
    virtual void update_stereotype(bool = FALSE);
    virtual QString stereotypes_properties() const;
    virtual AType class_association() const;
    virtual const char * constraint() const;

    virtual void write_id(ToolCom * com);
    virtual void save(QTextStream &, bool ref, QString & warning);
    static BrowserRelation * read(char *& , char *, BrowserNode *, bool force = TRUE);
    static BrowserRelation * read_ref(char *& st);
    static BrowserRelation * read_ref(char *& , char * k);
    static BrowserRelation * temporary(RelationData * d);
    static BrowserNode * get_it(const char * k, int id);
    static void post_load();
    void invalidate();

    static void clear(bool old);
    static void update_idmax_for_root();
    virtual void renumber(int phase);
    virtual void prepare_update_lib() const;

    virtual void referenced_by(QList<BrowserNode *> &, bool ondelete = FALSE);
    static void compute_referenced_by(QList<BrowserNode *> &, BrowserClass *);

    virtual bool tool_cmd(ToolCom * com, const char * args);

    static QString drag_key(BrowserNode * p);
    virtual QString drag_key() const;
    virtual QString drag_postfix() const;
    virtual void DropAfterEvent(QDropEvent * e, BrowserNode * after);

    static void get_relating(BrowserNode *, Q3PtrDict<BrowserNode> & d,
                             BrowserNodeList & newones,
                             bool inh, bool dep, bool assoc);
};

#endif

