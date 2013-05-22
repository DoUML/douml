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

#ifndef DIAGRAMITEM_H
#define DIAGRAMITEM_H

#include <qlist.h>
#include <qstring.h>
#include <qpoint.h>
#include <QTextStream>
#include <q3ptrdict.h>
//Added by qt3to4:
#include <Q3PtrCollection>

#include "Labeled.h"
#include "UmlEnum.h"

#define SELECT_SQUARE_SIZE 6

class Q3CanvasItem;
class Q3CanvasItemList;
class QBuffer;
class QPoint;

class DiagramCanvas;
class LabelCanvas;
class ArrowCanvas;
class BrowserNode;
class BrowserClass;
class BasicData;
class UmlCanvas;
#include <q3ptrlist.h> // [lgfreitas] added for q3ptrlist

class DiagramItem : public Labeled<DiagramItem>
{
protected:
    QList<ArrowCanvas *> lines;

    static QList<DiagramItem *> Undefined;

public:
    DiagramItem(int id, UmlCanvas * canvas);
    virtual ~DiagramItem();

    virtual void delete_it() = 0;

    void hide_lines();
    void update_show_lines();

    void add_line(ArrowCanvas * l) {
        lines.append(l);
    }
    virtual void remove_line(ArrowCanvas * l, bool onbrkjoin = FALSE);
    virtual void check_line(ArrowCanvas * l);
    bool attached_to(const ArrowCanvas *) const;

    virtual UmlCode type() const = 0;
    virtual void delete_available(BooL & in_model, BooL & out_model) const;
    virtual void remove(bool from_model);
    virtual BrowserNode * get_bn() const;
    virtual UmlCanvas * the_canvas() const = 0;
    virtual QPoint center() const = 0;
    virtual QRect rect() const = 0;
    virtual bool contains(int, int) const = 0;
    virtual void open() = 0;
    virtual void menu(const QPoint &) = 0;
    virtual QString may_start(UmlCode &) const = 0;
    virtual bool may_connect(UmlCode l) const;
    virtual QString may_connect(UmlCode & l, const DiagramItem * dest) const = 0;
    virtual void connexion(UmlCode, DiagramItem *, const QPoint &, const QPoint &) = 0;
    virtual bool connexion(UmlCode, const QPoint &, const QPoint &);
    virtual void post_connexion(UmlCode, DiagramItem *);
    BasicData * add_relation(UmlCode, DiagramItem *);
    virtual bool has_relation(BasicData *) const;
    virtual bool has_relation(UmlCode, BasicData *) const;
    enum LineDirection { Horizontal, Vertical, All };
    virtual LineDirection allowed_direction(UmlCode);
    virtual bool alignable() const;
    virtual aCorner on_resize_point(const QPoint &);
    virtual void resize(aCorner c, int dx, int dy, QPoint &);
    virtual void resize(const QSize & sz, bool w, bool h);
    virtual void update();
    virtual void change_scale() = 0;
    virtual double get_z() const = 0;
    virtual void set_z(double z) = 0;	// only called by upper() & lower()
    virtual void prepare_for_move(bool on_resize);
    virtual bool move_with_its_package() const;
    virtual bool move_with(UmlCode) const;
    virtual bool copyable() const;
    virtual void select_associated();
    virtual bool isSelected() const = 0;
    virtual void unassociate(DiagramItem *);
    virtual void save(QTextStream & st, bool ref, QString & warning) const = 0;
    virtual void post_loaded();

    virtual void history_save(QBuffer &)const = 0;
    virtual void history_load(QBuffer &) = 0;
    virtual void history_hide() = 0;

    static DiagramItem * dict_get(int id, const char *, UmlCanvas *);

    void shift(QPoint & p, QPoint other, bool contains_other) const;

    bool linked() {
        return !lines.isEmpty();
    }
    void post_history_hide() {
        lines.clear();
    }

    virtual bool has_drawing_settings() const;
    virtual void edit_drawing_settings(QList<DiagramItem *> &);
    virtual void clone_drawing_settings(const DiagramItem *src);

    virtual void apply_shortcut(QString);

    virtual void check_stereotypeproperties() = 0;

    static void post_load();
    void remove_if_already_present();

    virtual bool represents(BrowserNode *);
};

class DiagramItemList : public QList<DiagramItem *>
{
public:
    DiagramItemList(Q3CanvasItemList);
    virtual ~DiagramItemList();

    void sort();
    static bool lessThan(DiagramItem *a, DiagramItem *b);
};


extern void show_mark(QPainter & p, const QRect & r);
extern aCorner on_resize_point(const QPoint & p, const QRect & r);

// not a diagram item : a Qt object
#define TOP_Z 1e101
#define isa_alien(x) ((x)->z() == TOP_Z)

extern DiagramItem * QCanvasItemToDiagramItem(Q3CanvasItem * ci);
extern DiagramCanvas * QCanvasItemToDiagramCanvas(Q3CanvasItem * ci);

#endif

