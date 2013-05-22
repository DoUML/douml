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

#ifndef ACTIVITYACTIONCANVAS_H
#define ACTIVITYACTIONCANVAS_H





#include <qobject.h>
#include <q3valuelist.h>
//Added by qt3to4:
#include <QTextStream>

#include "DiagramCanvas.h"
#include "Settings.h"

#define ACTIVITYACTION_MIN_SIZE 40

class PinCanvas;
class ParameterSetCanvas;
class InfoCanvas;

class ActivityActionCanvas : public QObject, public DiagramCanvas
{
    Q_OBJECT

protected:
    Q3ValueList<PinCanvas *> pins;
    Q3ValueList<ParameterSetCanvas *> paramsets;
    InfoCanvas * pre;
    InfoCanvas * post;
    InfoCanvas * constraint;
    ActivityDrawingSettings settings;
    ActivityDrawingSettings used_settings;
    Uml3States show_opaque_action_definition;
    UmlColor itscolor;
    UmlColor used_color;
    int align;
    QString s;

protected:
    ActivityActionCanvas(UmlCanvas * canvas, int id);

    void check_pins();
    void check_parametersets();
    void check_conditions_constraint();

public:
    ActivityActionCanvas(BrowserNode * bn, UmlCanvas * canvas, int x, int y);
    virtual ~ActivityActionCanvas();

    Q3ValueList<PinCanvas *> get_pins() {
        return pins;
    }
    void add(PinCanvas * p) {
        pins.append(p);
    }
    void force_pins_arround();

    bool force_inside();
    void check_parameter_sets_position();

    int shadow_margin() const;

    virtual void delete_it();

    void update();
    void deleted(PinCanvas *);
    void deleted(ParameterSetCanvas *);

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
    virtual void set_z(double z);
    virtual aCorner on_resize_point(const QPoint & p);
    virtual void resize(aCorner c, int dx, int dy, QPoint &);
    virtual void resize(const QSize & sz, bool w, bool h);
    virtual bool move_with_its_package() const;
    virtual void moveBy(double dx, double dy);

    virtual bool has_drawing_settings() const;
    virtual void edit_drawing_settings(QList<DiagramItem *> &);
    virtual void clone_drawing_settings(const DiagramItem *src);
    void edit_drawing_settings();
    virtual bool get_show_stereotype_properties() const;

    virtual void apply_shortcut(QString s);

    virtual void save(QTextStream  & st, bool ref, QString & warning) const;
    static ActivityActionCanvas * read(char *& , UmlCanvas *, char *);
    virtual void post_loaded();

    virtual void history_save(QBuffer &) const;
    virtual void history_load(QBuffer &);
    virtual void history_hide();

private slots:
    void modified();	// canvas must be updated
    void deleted();
};

#endif
