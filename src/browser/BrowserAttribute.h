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

#ifndef BROWSER_ATTRIBUTE_H
#define BROWSER_ATTRIBUTE_H

class QPixmap;
class AttributeData;
class BrowserOperation;
class BrowserClass;

#include "BrowserNode.h"
#include "Labeled.h"

class BrowserAttribute : public BrowserNode, public Labeled<BrowserAttribute>
{
    friend class StereotypesDialog;
    friend class QuickEdit;

protected:
    static IdDict<BrowserAttribute> all;
    static QStringList its_default_stereotypes;

    AttributeData * def;
    BrowserOperation * get_oper;
    BrowserOperation * set_oper;

protected:
    BrowserAttribute(int id);

    void exec_menu_choice(int rank);

public:
    BrowserAttribute(QString s, BrowserNode * p, AttributeData * d, int id = 0);
    BrowserAttribute(const BrowserAttribute * model, BrowserNode * p);
    virtual ~BrowserAttribute();
    uint TypeID() override;
    virtual void delete_it();
    virtual BrowserNode * duplicate(BrowserNode * p,
                                    QString name = QString());

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

    virtual void menu();
    virtual void apply_shortcut(QString s);
    virtual void open(bool);
    void open_new_ste_attr();
    virtual UmlCode get_type() const;
    virtual QString get_stype() const;
    virtual int get_identifier() const;
    virtual const char * help_topic() const;
    virtual void modified();
    virtual BasicData * get_data() const;
    virtual bool allow_spaces() const;
    virtual bool same_name(const QString & s, UmlCode type) const;
    virtual QString full_name(bool rev = FALSE, bool itself = TRUE) const;
    virtual void member_cpp_def(const QString & prefix, const QString &,
                                QString & s, bool templ) const;
    virtual const char * constraint() const;

    virtual void save(QTextStream &, bool ref, QString & warning);
    static BrowserAttribute * read_ref(char *&);
    static BrowserAttribute * read(char *& , char *, BrowserNode *, bool force = TRUE);
    static BrowserNode * get_it(const char * k, int id);
    static void post_load();

    static BrowserAttribute * new_one(QString s, BrowserNode * p, bool enum_item);

    static void init();
    static void clear(bool old);
    static void update_idmax_for_root();
    virtual void renumber(int phase);
    virtual void prepare_update_lib() const;

    virtual void referenced_by(QList<BrowserNode *> &, bool ondelete = FALSE);
    static void compute_referenced_by(QList<BrowserNode *> &, BrowserNode *);

    virtual bool tool_cmd(ToolCom * com, const char * args);

    virtual void DropAfterEvent(QDropEvent * e, BrowserNode * after);

    static const QStringList & default_stereotypes();
    static void read_stereotypes(char *& , char *& k);
    static void save_stereotypes(QTextStream &);
};

#endif

