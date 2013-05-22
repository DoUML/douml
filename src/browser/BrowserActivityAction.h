//Added by qt3to4:
#include <QTextStream>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <Q3ValueList>
#include <QPixmap>
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

#ifndef BROWSER_ACTIVITYACTION_H
#define BROWSER_ACTIVITYACTION_H

class QPixmap;
class ActivityActionData;
class BrowserPin;
class BrowserFlow;
class BrowserParameterSet;
class BrowserActivityDiagram;
class BrowserOperation;

#include "BrowserNode.h"
#include "Labeled.h"
#include "BrowserActivityElement.h"

class BrowserActivityAction : public BrowserNode, public Labeled<BrowserActivityAction>, public BrowserActivityElement
{
    friend class StereotypesDialog;

protected:
    static IdDict<BrowserActivityAction> all;
    static QStringList its_default_stereotypes;

    ActivityActionData * def;
    BrowserNode * associated_diagram;

protected:
    BrowserActivityAction(int id);

    void exec_menu_choice(int rank, BrowserNode * who);

public:
    BrowserActivityAction(QString s, BrowserNode * p, int id = 0);
    BrowserActivityAction(const BrowserActivityAction * model, BrowserNode * p);
    BrowserActivityAction(QString s, BrowserNode * p, UmlActionKind);
    virtual ~BrowserActivityAction();

    virtual BrowserNode * duplicate(BrowserNode * p,
                                    QString name = QString());
    virtual bool may_contains_them(const QList<BrowserNode *> & l,
                                   BooL & duplicable) const;
    static BrowserActivityAction * add_activityaction(BrowserNode * future_parent,
            const char * s);
    static BrowserActivityAction * get_activityaction(BrowserNode * future_parent);
    static BrowserActivityAction * add_call_behavior(BrowserNode * future_parent,
            BrowserNode * beh);
    static BrowserActivityAction * add_call_operation(BrowserNode * future_parent,
            BrowserOperation * oper);
    static BrowserActivityAction * add_read_variable_value(BrowserNode * future_parent,
            BrowserNode * var);
    static BrowserActivityAction * add_clear_variable_value(BrowserNode * future_parent,
            BrowserNode * var);
    static BrowserActivityAction * add_write_variable_value(BrowserNode * future_parent,
            BrowserNode * var);
    static BrowserActivityAction * add_add_variable_value(BrowserNode * future_parent,
            BrowserNode * var);
    static BrowserActivityAction * add_remove_variable_value(BrowserNode * future_parent,
            BrowserNode * var);

    void set_pins();
    Q3ValueList<BrowserPin *> get_pins() const;
    Q3ValueList<BrowserParameterSet *> get_parametersets() const;
    void remove_pins();
    void update_pins();
    BrowserNode * add_parameter_set(BrowserParameterSet *, const char *);
    BasicData * add_relation(UmlCode l, BrowserNode * end);
    QString may_start() const;
    QString may_connect(UmlCode & l, const BrowserNode * dest) const;
    virtual QString connexion_from(bool control) const;

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
    virtual bool allow_empty() const;

    virtual BrowserNode * get_associated() const;
    void set_associated_diagram(BrowserNode *, bool on_read = FALSE);

    virtual void save(QTextStream &, bool ref, QString & warning);
    static BrowserActivityAction * read(char *& , char *, BrowserNode *);
    static BrowserActivityAction * read_ref(char *& st);
    static BrowserNode * get_it(const char * k, int id);

    static void clear(bool old);
    static void update_idmax_for_root();
    virtual void renumber(int phase);
    virtual void prepare_update_lib() const;

    virtual bool tool_cmd(ToolCom * com, const char * args);
    virtual void write_id(ToolCom * com);
    virtual bool api_compatible(unsigned v) const;

    virtual void referenced_by(QList<BrowserNode *> &, bool ondelete = FALSE);
    static void compute_referenced_by(QList<BrowserNode *> &, BrowserNode *);

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
