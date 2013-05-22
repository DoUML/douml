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

#ifndef FLOWCANVAS_H
#define FLOWCANVAS_H

#include "ArrowCanvas.h"
#include "Settings.h"
//Added by qt3to4:
#include <QTextStream>

class BrowserNode;
class FlowData;
class InfoCanvas;
class DiagramItem;
class StereotypePropertiesCanvas;

class FlowCanvas : public ArrowCanvas
{
    Q_OBJECT

protected:
    BrowserNode * br_begin;
    FlowData * data;
    InfoCanvas * selection;
    InfoCanvas * transformation;
    StereotypePropertiesCanvas * stereotypeproperties; // in the first segment
    ActivityDrawingSettings settings;
    ActivityDrawingSettings used_settings;
    Uml3States write_horizontally;

    void check_sel_trans();

public:
    FlowCanvas(UmlCanvas * canvas, DiagramItem * b, DiagramItem * e,
               BrowserNode * bb, int id, float d_start, float d_end,
               FlowData * d = 0);
    virtual ~FlowCanvas();

    virtual void delete_it();
    virtual void delete_available(BooL & in_model, BooL & out_model) const;
    virtual void remove(bool from_model);
    virtual void unconnect();

    virtual ArrowPointCanvas * brk(const QPoint &);
    virtual ArrowCanvas * join(ArrowCanvas * other, ArrowPointCanvas * ap);

    virtual void default_label_position() const;
    void stereotype_default_position() const;

    virtual void save(QTextStream & st, bool ref, QString & warning) const;

    virtual void history_load(QBuffer &);
    virtual void history_hide();

    virtual bool has_drawing_settings() const;
    virtual void edit_drawing_settings(QList<DiagramItem *> &);
    virtual void clone_drawing_settings(const DiagramItem *src);
    void edit_drawing_settings();

    virtual void apply_shortcut(QString s);

    static FlowCanvas * read(char *& st, UmlCanvas * canvas, char *);
    virtual BasicData * get_data() const;

    virtual void open();
    virtual void menu(const QPoint &);

    virtual void setVisible(bool yes);
    virtual void moveBy(double dx, double dy);
    virtual void select_associated();
    virtual void check_stereotypeproperties();

    static void drop(BrowserNode *, UmlCanvas *);

    virtual bool represents(BrowserNode *);

protected:
    BrowserNode * update_begin(DiagramItem * cnend);
    void update(bool updatepos);
    void propagate_drawing_settings();

private slots:
    void modified();	// canvas must be updated
    void deleted();	// the relation is deleted
};

#endif
