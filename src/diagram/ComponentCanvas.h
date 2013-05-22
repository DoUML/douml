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

#ifndef COMPONENTCANVAS_H
#define COMPONENTCANVAS_H





#include <qobject.h>
//Added by qt3to4:
#include <QTextStream>

#include "DiagramCanvas.h"
#include "MultipleDependency.h"
#include "Settings.h"

#define COMPONENT_CANVAS_MIN_SIZE 40

class ComponentCanvas : public QObject, public DiagramCanvas,
    public MultipleDependency<BasicData>
{
    Q_OBJECT

protected:
    ComponentDrawingSettings settings;
    UmlColor itscolor;
    UmlColor used_color;
    bool as_icon;
    bool req_prov;
    bool rea;
    int min_width;
    int min_height;

protected:
    ComponentCanvas(UmlCanvas * canvas, int id);
    bool valid(ArrowCanvas * a) const;

public:
    ComponentCanvas(BrowserNode * bn, UmlCanvas * canvas, int x, int y);
    virtual ~ComponentCanvas();

    virtual void delete_it();

    void compute_size();

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
    virtual bool may_connect(UmlCode l) const;
    virtual QString may_connect(UmlCode & l, const DiagramItem * dest) const;
    virtual bool connexion(UmlCode action, const QPoint &, const QPoint &);
    virtual void connexion(UmlCode, DiagramItem *, const QPoint &, const QPoint &);
    virtual void check_line(ArrowCanvas * l);
    virtual aCorner on_resize_point(const QPoint & p);
    virtual void resize(aCorner c, int dx, int dy, QPoint &);
    virtual void resize(const QSize & sz, bool w, bool h);
    virtual void prepare_for_move(bool on_resize);
    virtual bool move_with_its_package() const;

    virtual void save(QTextStream  & st, bool ref, QString & warning) const;
    static ComponentCanvas * read(char *& , UmlCanvas *, char *);
    virtual void post_loaded();

    virtual void history_save(QBuffer &) const;
    virtual void history_load(QBuffer &);
    virtual void history_hide();

    virtual bool has_drawing_settings() const;
    virtual void edit_drawing_settings(QList<DiagramItem *> &);
    virtual void clone_drawing_settings(const DiagramItem *src);
    void edit_drawing_settings();
    virtual bool get_show_stereotype_properties() const;

    virtual void apply_shortcut(QString s);

private slots:
    void modified();	// canvas must be updated
    void deleted();	// the class is deleted
};

#endif
