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

#ifndef BROWSER_PIN_H
#define BROWSER_PIN_H

class QPixmap;
class PinData;
struct PinParamData;

#include "BrowserNode.h"
#include "Labeled.h"
#include "BrowserActivityElement.h"

class BrowserPin : public BrowserNode, public Labeled<BrowserPin>, public BrowserActivityElement
{
    friend class StereotypesDialog;

protected:
    static IdDict<BrowserPin> all;
    static QStringList its_default_stereotypes;

    PinData * def;

protected:
    BrowserPin(int id);

    void exec_menu_choice(int rank);

public:
    BrowserPin(QString s, BrowserNode * p, PinData * d, int id = 0);
    BrowserPin(const BrowserPin * model, BrowserNode * p);
    virtual ~BrowserPin();

    virtual BrowserNode * duplicate(BrowserNode * p,
                                    QString name = QString());

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
    virtual bool allow_empty() const;

    virtual void save(QTextStream &, bool ref, QString & warning);
    static BrowserPin * read_ref(char *&);
    static BrowserPin * read(char *& , char *, BrowserNode *);
    static BrowserNode * get_it(const char * k, int id);

    static void new_one(BrowserNode * future_parent, const char * name, const PinParamData & pd);
    static BrowserPin * new_one(BrowserNode * future_parent, const char * name);
    static BrowserPin * add_pin(BrowserPin * pin, BrowserNode * future_parent);
    virtual BasicData * add_relation(UmlCode, BrowserNode * end);
    QString may_start() const;
    QString may_connect(const BrowserNode * dest) const;
    virtual QString connexion_from(bool control) const;

    static void clear(bool old);
    static void update_idmax_for_root();
    virtual void renumber(int phase);
    virtual void prepare_update_lib() const;

    virtual bool tool_cmd(ToolCom * com, const char * args);
    virtual bool api_compatible(unsigned v) const;

    virtual void referenced_by(QList<BrowserNode *> & l, bool ondelete);
    static void compute_referenced_by(QList<BrowserNode *> &, BrowserNode *);

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

