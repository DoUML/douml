//Added by qt3to4:
#include <QPixmap>
#include <QDropEvent>
#include <QTextStream>
#include <Q3ValueList>
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

#ifndef BROWSER_COMPONENT_H
#define BROWSER_COMPONENT_H

class QPixmap;

#include "BrowserNode.h"
#include "Labeled.h"

class SimpleData;
class BrowserClass;
class BrowserComponentDiagram;

class BrowserComponent : public BrowserNode, public Labeled<BrowserComponent>
{
    friend class StereotypesDialog;

protected:
    static IdDict<BrowserComponent> all;
    static QStringList its_default_stereotypes;

    SimpleData * def;
    BrowserNode * associated_diagram;
    Q3ValueList<BrowserClass *> realizing_classes;
    Q3ValueList<BrowserClass *> provided_classes;
    Q3ValueList<BrowserClass *> required_classes;

    void associate_class(BrowserClass * c);
    void exec_menu_choice(int rank);

    void get_all_provided_classes(Q3ValueList<BrowserClass *> &) const;
    void get_all_required_classes(Q3ValueList<BrowserClass *> &) const;

public:
    BrowserComponent(QString s, BrowserNode * p, int id = 0);
    BrowserComponent(const BrowserComponent * model, BrowserNode * p);
    BrowserComponent(int id);

    virtual ~BrowserComponent();

    virtual BrowserNode * get_associated() const;
    void set_associated_diagram(BrowserNode *, bool on_read = FALSE);

    void set_associated_classes(const Q3ValueList<BrowserClass *> & rz,
                                const Q3ValueList<BrowserClass *> & pr,
                                const Q3ValueList<BrowserClass *> & rq,
                                bool on_read = FALSE);
    void remove_associated_class(BrowserClass *);
    const Q3ValueList<BrowserClass *> & get_provided_classes() const {
        return provided_classes;
    }
    const Q3ValueList<BrowserClass *> & get_required_classes() const {
        return required_classes;
    }
    const Q3ValueList<BrowserClass *> & get_realizing_classes() const {
        return realizing_classes;
    }
    void get_all_provided_classes(Q3ValueList<BrowserClass *> &, bool sorted) const;
    void get_all_required_classes(Q3ValueList<BrowserClass *> &, bool sorted) const;

    virtual BrowserNode * duplicate(BrowserNode * p,
                                    QString name = QString());
    virtual QString full_name(bool rev = FALSE, bool itself = TRUE) const;
    virtual void menu();
    virtual void apply_shortcut(QString s);
    virtual void open(bool force_edit);
    virtual UmlCode get_type() const;
    virtual QString get_stype() const;
    virtual int get_identifier() const;
    virtual void modified();
    virtual const char * help_topic() const;
    virtual BasicData * get_data() const;
    virtual void on_delete();
    virtual QString check_inherit(const BrowserNode * parent) const;
    virtual void save(QTextStream &, bool ref, QString & warning);
    static BrowserComponent * read_ref(char *& , const char *);
    static BrowserComponent * read(char *& , char *, BrowserNode *);
    static BrowserNode * get_it(const char * k, int id);

    virtual bool tool_cmd(ToolCom * com, const char * args);

    static BrowserNodeList & instances(BrowserNodeList &, const char * st = 0, bool non_nested = FALSE);
    static BrowserComponent * get_component(BrowserNode * future_parent,
                                            bool existing = FALSE);
    static BrowserComponent * add_component(BrowserNode * future_parent);

    static void clear(bool old);
    static void update_idmax_for_root();
    virtual void renumber(int phase);
    virtual void prepare_update_lib() const;

    virtual const QPixmap * pixmap(int) const;
    virtual void iconChanged();

    virtual void referenced_by(QList<BrowserNode *> & l, bool ondelete);
    static void compute_referenced_by(QList<BrowserNode *> &, BrowserClass *);

    static void init();
    static const QStringList & default_stereotypes();
    static void read_stereotypes(char *& , char *& k);
    static void save_stereotypes(QTextStream &);

    virtual void DragMoveEvent(QDragMoveEvent * e);
    virtual void DropEvent(QDropEvent * e);
    virtual void DragMoveInsideEvent(QDragMoveEvent * e);
    virtual void DropAfterEvent(QDropEvent * e, BrowserNode * after);
};

#endif

