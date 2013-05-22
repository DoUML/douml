//Added by qt3to4:
#include <QPixmap>
#include <QDropEvent>
#include <QTextStream>
#include <QDragMoveEvent>
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

#ifndef BROWSER_STATE_H
#define BROWSER_STATE_H

class QPixmap;
class StateData;

#include "BrowserNode.h"
#include "Labeled.h"

class BrowserStateDiagram;
class BrowserTransition;
class BrowserOperation;

class BrowserState : public BrowserNode, public Labeled<BrowserState>
{
    friend class StereotypesDialog;

protected:
    static IdDict<BrowserState> all;
    static QStringList its_default_stereotypes;

    StateData * def;
    BrowserStateDiagram * associated_diagram;

protected:
    BrowserState(int id);

    void exec_menu_choice(int rank, bool mach);

public:
    BrowserState(QString s, BrowserNode * p, StateData * d, int id = 0);
    BrowserState(const BrowserState * model, BrowserNode * p);
    virtual ~BrowserState();

    bool may_contains(UmlCode k) const;
    virtual bool may_contains_them(const QList<BrowserNode *> & l,
                                   BooL & duplicable) const;
    virtual BrowserNode * duplicate(BrowserNode * p,
                                    QString name = QString());
    static BrowserNodeList & instances(BrowserNodeList &, bool sort);
    static BrowserState * add_state(BrowserNode * future_parent, bool machine);
    static BrowserState * add_state(BrowserNode * future_parent, QString name);
    static BrowserState * get_state(BrowserNode * parent);
    BrowserTransition * add_transition(BrowserNode * end);
    virtual BasicData * add_relation(UmlCode, BrowserNode * end);

    virtual const QPixmap * pixmap(int) const;

    virtual void menu();
    virtual void apply_shortcut(QString s);
    virtual void open(bool);
    virtual UmlCode get_type() const;
    virtual QString get_stype() const;
    virtual int get_identifier() const;
    virtual const char * help_topic() const;
    virtual void modified();
    virtual void on_delete();
    virtual BasicData * get_data() const;
    virtual QString full_name(bool rev = FALSE, bool itself = TRUE) const;

    virtual BrowserNode * get_associated() const;
    void set_associated_diagram(BrowserStateDiagram *, bool on_read = FALSE);

    virtual void save(QTextStream &, bool ref, QString & warning);
    static BrowserState * read(char *& , char *, BrowserNode *);
    static BrowserState * read_ref(char *& st);
    static BrowserNode * read_any_ref(char *& , char *);
    static BrowserNode * get_it(const char * k, int id);

    static void clear(bool old);
    static void update_idmax_for_root();
    virtual void renumber(int phase);
    virtual void prepare_update_lib() const;

    virtual bool tool_cmd(ToolCom * com, const char * args);
    virtual bool api_compatible(unsigned v) const;

    virtual void referenced_by(QList<BrowserNode *> &, bool ondelete = FALSE);
    static void compute_referenced_by(QList<BrowserNode *> &, BrowserOperation *);

    bool is_ref() const;
    bool can_reference() const;
    bool can_reference(BrowserState *) const;

    static void init();
    static const QStringList & default_stereotypes();
    static void read_stereotypes(char *& , char *& k);
    static void save_stereotypes(QTextStream &);

    static QString drag_key(BrowserNode * p);
    virtual QString drag_key() const;
    virtual QString drag_postfix() const;
    virtual void DragMoveEvent(QDragMoveEvent * e);
    virtual void DropEvent(QDropEvent * e);
    virtual void DragMoveInsideEvent(QDragMoveEvent * e);
    virtual void DropAfterEvent(QDropEvent * e, BrowserNode * after);

    static BrowserState * get_machine(const BrowserNode * bn);
};

#endif

