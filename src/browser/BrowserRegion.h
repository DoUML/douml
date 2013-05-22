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

#ifndef BROWSER_REGION_H
#define BROWSER_REGION_H

class QPixmap;
class BasicData;

#include "BrowserNode.h"
#include "Labeled.h"

class BrowserTransition;

class BrowserRegion : public BrowserNode, public Labeled<BrowserRegion>
{
    friend class StereotypesDialog;

protected:
    static IdDict<BrowserRegion> all;
    static QStringList its_default_stereotypes;

    BasicData * def;

protected:
    BrowserRegion(int id);

    void exec_menu_choice(int rank);

public:
    BrowserRegion(QString s, BrowserNode * p, BasicData * d, int id = 0);
    BrowserRegion(const BrowserRegion * model, BrowserNode * p);
    virtual ~BrowserRegion();

    virtual bool may_contains_them(const QList<BrowserNode *> & l,
                                   BooL & duplicable) const;
    static BrowserRegion * add_region(BrowserNode * future_parent,
                                      const char * s = "region");

    virtual const QPixmap * pixmap(int) const;

    virtual void delete_it();
    virtual void menu();
    virtual void apply_shortcut(QString s);
    virtual void open(bool);
    virtual UmlCode get_type() const;
    virtual QString get_stype() const;
    virtual int get_identifier() const;
    virtual void modified();
    virtual BasicData * get_data() const;
    virtual BrowserNode * duplicate(BrowserNode * p,
                                    QString name = QString());
    virtual QString full_name(bool rev = FALSE, bool itself = TRUE) const;

    virtual void save(QTextStream &, bool ref, QString & warning);
    static BrowserRegion * read(char *& , char *, BrowserNode *);
    static BrowserRegion * read_ref(char *& st);
    static BrowserNode * get_it(const char * k, int id);

    static void clear(bool old);
    static void update_idmax_for_root();
    virtual void renumber(int phase);
    virtual void prepare_update_lib() const;

    virtual bool tool_cmd(ToolCom * com, const char * args);
    virtual bool api_compatible(unsigned v) const;

    virtual bool allow_spaces() const;
    virtual bool allow_empty() const;
    virtual bool same_name(const QString & s, UmlCode type) const;

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

