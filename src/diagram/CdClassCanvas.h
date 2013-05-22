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

#ifndef CDCLASSCANVAS_H
#define CDCLASSCANVAS_H





#include <qobject.h>
//Added by qt3to4:
#include <QTextStream>
#include <Q3ValueList>

#include "DiagramCanvas.h"
#include "MultipleDependency.h"
#include "Settings.h"

class TemplateCanvas;
class RelationData;
class ConstraintCanvas;

class CdClassCanvas : public QObject, public DiagramCanvas,
    public MultipleDependency<BasicData>
{
    Q_OBJECT

protected:
    TemplateCanvas * templ;
    ClassDiagramSettings settings;
    ClassDiagramSettings used_settings;
    UmlColor itscolor;
    UmlColor used_color;
    ClassDrawingMode used_view_mode;
    bool indicate_visible_attr : 2;
    bool indicate_visible_oper : 2;
    bool manual_size : 2;
    int width_min;
    int height_min;
    QString full_name;
    Q3ValueList<BrowserNode *> hidden_visible_attributes;
    Q3ValueList<BrowserNode *> hidden_visible_operations;
    ConstraintCanvas * constraint;

protected:
    CdClassCanvas(UmlCanvas * canvas, int id);

public:
    CdClassCanvas(BrowserNode * bn, UmlCanvas * canvas, int x, int y);
    virtual ~CdClassCanvas();

    virtual void delete_it();

    void check_size();
    virtual bool has_relation(BasicData *) const;
    bool has_inner(DiagramItem * end) const;
    void draw_all_relations(CdClassCanvas * end = 0);
    void draw_all_class_assoc();
    void check_constraint();
    void check_inner();
    const ClassDiagramSettings & get_settings() const {
        return settings;
    }

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
    virtual void post_connexion(UmlCode, DiagramItem *);
    virtual void connexion(UmlCode, DiagramItem *, const QPoint &, const QPoint &);
    virtual aCorner on_resize_point(const QPoint & p);
    virtual void resize(aCorner c, int dx, int dy, QPoint &);
    virtual void resize(const QSize & sz, bool w, bool h);
    virtual bool move_with_its_package() const;
    virtual void moveBy(double dx, double dy);
    virtual void set_z(double z);	// only called by upper() & lower()
    virtual void history_save(QBuffer &) const;
    virtual void history_load(QBuffer &);
    virtual void history_hide();

    virtual bool has_drawing_settings() const;
    virtual void edit_drawing_settings(QList<DiagramItem *> &);
    virtual void clone_drawing_settings(const DiagramItem *src);
    void edit_drawing_settings();
    virtual bool get_show_stereotype_properties() const;

    virtual void apply_shortcut(QString s);

    virtual void save(QTextStream  & st, bool ref, QString & warning) const;
    static CdClassCanvas * read(char *& , UmlCanvas *, char *);
    virtual void post_loaded();

private slots:
    void modified();	// canvas must be updated
    void deleted();	// the class is deleted
};

#endif
