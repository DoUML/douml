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

#ifndef BROWSER_PARAMETER_H
#define BROWSER_PARAMETER_H

class QPixmap;
class ParameterData;

#include "BrowserNode.h"
#include "Labeled.h"
#include "BrowserActivityElement.h"

class BrowserParameter : public BrowserNode, public Labeled<BrowserParameter>, public BrowserActivityElement
{
    friend class StereotypesDialog;

protected:
    static IdDict<BrowserParameter> all;
    static QStringList its_default_stereotypes;

    ParameterData * def;

protected:
    BrowserParameter(int id);

    void exec_menu_choice(int rank);

public:
    BrowserParameter(QString s, BrowserNode * p, int id = 0);
    BrowserParameter(const BrowserParameter * model, BrowserNode * p);
    virtual ~BrowserParameter();

    virtual BrowserNode * duplicate(BrowserNode * p,
                                    QString name = QString());

    virtual BasicData * add_relation(UmlCode, BrowserNode * end);
    QString may_start() const;
    QString may_connect(const BrowserNode * dest) const;
    virtual QString connexion_from(bool control) const;

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
    static BrowserParameter * read_ref(char *&);
    static BrowserParameter * read(char *& , char *, BrowserNode *);
    static BrowserNode * get_it(const char * k, int id);

    static void clear(bool old);
    static void update_idmax_for_root();
    virtual void renumber(int phase);
    virtual void prepare_update_lib() const;

    virtual void referenced_by(QList<BrowserNode *> &, bool ondelete = FALSE);
    static void compute_referenced_by(QList<BrowserNode *> &, BrowserNode *);

    virtual bool tool_cmd(ToolCom * com, const char * args);
    virtual bool api_compatible(unsigned v) const;

    virtual void DragMoveEvent(QDragMoveEvent * e);
    virtual void DropEvent(QDropEvent * e);
    virtual void DragMoveInsideEvent(QDragMoveEvent * e);
    virtual void DropAfterEvent(QDropEvent * e, BrowserNode * after);
    QString drag_key() const;
    QString drag_postfix() const;
    static QString drag_key(BrowserNode * p);

    static void init();
    static const QStringList & default_stereotypes();
    static void read_stereotypes(char *& , char *& k);
    static void save_stereotypes(QTextStream &);
};

#endif

