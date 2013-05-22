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

#ifndef ODCLASSINSTCANVAS_H
#define ODCLASSINSTCANVAS_H





#include <qobject.h>
//Added by qt3to4:
#include <QTextStream>

#include "DiagramCanvas.h"
#include "ClassInstCanvas.h"

class BrowserClassInstance;
class BasicData;
struct SlotRel;
class ObjectLinkCanvas;

class OdClassInstCanvas : public QObject, public DiagramCanvas,
    public ClassInstCanvas
{
    Q_OBJECT

    friend class ClassInstanceDialog;

public:
    OdClassInstCanvas(BrowserClassInstance * t, UmlCanvas * canvas,
                      int x, int y, int id);
    virtual ~OdClassInstCanvas();

    virtual void delete_it();

    virtual void draw(QPainter & p);
    virtual void change_scale();

    void compute_size();

    virtual UmlCode type() const;
    virtual QString get_name() const;	// all cases
    virtual void set_name(const QString & s);	// out of model case
    virtual BrowserNode * get_type() const;	// return class, all cases
    void set_type(BrowserNode * t);	// out of model case
    virtual BrowserNodeList & get_types(BrowserNodeList &) const;
    virtual BrowserNode * container(UmlCode) const;
    virtual BrowserClassInstance * get_instance() const;
    virtual void open();
    virtual void menu(const QPoint &);
    virtual QString may_start(UmlCode &) const;
    virtual QString may_connect(UmlCode & l, const DiagramItem * dest) const;
    virtual void connexion(UmlCode, DiagramItem *, const QPoint &, const QPoint &);
    virtual bool move_with_its_package() const;
    virtual QString get_full_name() const;
    virtual void delete_available(BooL & in_model, BooL & out_model) const;
    virtual bool alignable() const;
    virtual bool copyable() const;
    virtual void remove(bool from_model);
    virtual void history_load(QBuffer &);
    virtual void history_hide();

    virtual bool has_drawing_settings() const;
    virtual void edit_drawing_settings(QList<DiagramItem *> &);
    virtual void clone_drawing_settings(const DiagramItem *src);
    void edit_drawing_settings();
    virtual bool get_show_stereotype_properties() const;

    virtual void apply_shortcut(QString s);
    void draw_all_relations(OdClassInstCanvas * end = 0);
    bool has_relation(const SlotRel &) const;
    bool is_duplicated(ObjectLinkCanvas * lnk,
                       OdClassInstCanvas * other) const;


    virtual void save(QTextStream &, bool ref, QString & warning) const;
    static OdClassInstCanvas * read(char *& , UmlCanvas * canvas, char *);
    virtual void post_loaded();

private slots:
    void modified();	// canvas must be updated
    void deleted();	// the class is deleted
};

#endif
