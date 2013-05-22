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

#ifndef BROWSER_PSEUDOSTATE_H
#define BROWSER_PSEUDOSTATE_H

class QPixmap;
class PseudoStateData;
class BrowserTransition;

#include "BrowserNode.h"
#include "Labeled.h"

class BrowserPseudoState : public BrowserNode, public Labeled<BrowserPseudoState>
{
    friend class StereotypesDialog;

protected:
    static IdDict<BrowserPseudoState> all;
    static QStringList its_default_stereotypes;

    UmlCode kind;
    PseudoStateData * def;

protected:
    BrowserPseudoState(int id);

    void exec_menu_choice(int rank);

public:
    BrowserPseudoState(UmlCode c, QString s, BrowserNode * p, PseudoStateData * d, int id = 0);
    BrowserPseudoState(const BrowserPseudoState * model, BrowserNode * p);
    virtual ~BrowserPseudoState();

    virtual BrowserNode * duplicate(BrowserNode * p,
                                    QString name = QString());
    static BrowserPseudoState * add_pseudostate(BrowserNode * future_parent, UmlCode c);
    static BrowserPseudoState * add_pseudostate(BrowserNode * future_parent,
            UmlCode c, const char * name);
    static BrowserPseudoState * get_pseudostate(BrowserNode * future_parent, UmlCode c);
    virtual BasicData * add_relation(UmlCode, BrowserNode * end);
    BrowserTransition * add_transition(BrowserNode * end);
    QString may_start() const;
    QString may_connect(const BrowserNode * dest) const;

    virtual const QPixmap * pixmap(int) const;

    virtual void menu();
    virtual void apply_shortcut(QString s);
    virtual void open(bool);
    virtual UmlCode get_type() const;
    virtual QString get_stype() const;
    virtual int get_identifier() const;
    virtual void modified();
    virtual BasicData * get_data() const;
    virtual QString full_name(bool rev = FALSE, bool itself = TRUE) const;

    virtual void save(QTextStream &, bool ref, QString & warning);
    static BrowserPseudoState * read(char *& , char *, BrowserNode *);
    static BrowserPseudoState * read_ref(char *& st);
    static BrowserNode * get_it(const char * k, int id);

    static void clear(bool old);
    static void update_idmax_for_root();
    virtual void renumber(int phase);
    virtual void prepare_update_lib() const;

    virtual bool tool_cmd(ToolCom * com, const char * args);

    virtual void referenced_by(QList<BrowserNode *> &, bool ondelete = FALSE);
    static void compute_referenced_by(QList<BrowserNode *> &, BrowserPseudoState *);

    bool allow_empty() const;
    static bool allow_empty(UmlCode c);

    bool can_reference(BrowserNode *) const;

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
};

#endif

