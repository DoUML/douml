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

#ifndef STATECANVAS_H
#define STATECANVAS_H





#include <qobject.h>
//Added by qt3to4:
#include <QTextStream>
#include <Q3MemArray>
#include "DiagramCanvas.h"
#include "MultipleDependency.h"
#include "Settings.h"


#define STATE_CANVAS_MIN_SIZE 40

class BrowserRegion;

class StateCanvas : public QObject, public DiagramCanvas, public MultipleDependency<BasicData>
{
    Q_OBJECT

protected:
    StateDrawingSettings settings;
    UmlColor itscolor;
    UmlColor used_color;
    bool region_horizontally : 2;
    bool show_decomp_icon : 2;
    bool was_drawn : 2;
    WrapperStr activities;
    int min_width;
    int min_height;
    Q3MemArray<QRect> regions_rect;	// from bottom to top / right to left
    Q3MemArray<BrowserRegion *> regions;

protected:
    StateCanvas(UmlCanvas * canvas, int id);
    void compute_regions();
    int resize_to_show_regions();

public:
    StateCanvas(BrowserNode * bn, UmlCanvas * canvas, int x, int y);
    virtual ~StateCanvas();

    virtual void delete_it();

    void compute_size();
    BrowserRegion * pointed_region(const QPoint & p) const;
    QRect region_rect(BrowserRegion *);

    virtual void draw(QPainter & p);
    virtual void change_scale();

    virtual UmlCode type() const;
    virtual void delete_available(BooL & in_model, BooL & out_model) const;
    virtual bool alignable() const;
    virtual bool copyable() const;
    virtual void remove(bool from_model);
    virtual void open();
    virtual void menu(const QPoint &);
    virtual QString may_start(UmlCode &) const;
    virtual QString may_connect(UmlCode & l, const DiagramItem * dest) const;
    virtual void connexion(UmlCode, DiagramItem *, const QPoint &, const QPoint &);
    virtual aCorner on_resize_point(const QPoint & p);
    virtual void resize(aCorner c, int dx, int dy, QPoint &);
    virtual void resize(const QSize & sz, bool w, bool h);
    virtual void prepare_for_move(bool on_resize);
    void force_sub_inside(bool resize_it);
    static void force_inside(DiagramCanvas *, bool resize_it);
    void force_sub_upper();


    virtual bool has_drawing_settings() const;
    virtual void edit_drawing_settings(QList<DiagramItem *> &);
    virtual void clone_drawing_settings(const DiagramItem *src);
    void edit_drawing_settings();
    virtual bool get_show_stereotype_properties() const;

    virtual void apply_shortcut(QString s);

    virtual void save(QTextStream  & st, bool ref, QString & warning) const;
    static StateCanvas * read(char *& , UmlCanvas *, char *);
    virtual void post_loaded();

    virtual void history_save(QBuffer &) const;
    virtual void history_load(QBuffer &);
    virtual void history_hide();

private slots:
    void modified();	// canvas must be updated
    void deleted();
};

#endif
