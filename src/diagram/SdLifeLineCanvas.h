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

#ifndef SDLIFELINECANVAS_H
#define SDLIFELINECANVAS_H

#include "DiagramCanvas.h"
#include "SdDurationSupport.h"
//Added by qt3to4:
#include <QTextStream>

#define LIFE_LINE_TOPOFFSET 41

class SdDead;
class SdObjCanvas;
class SdDurationCanvas;
class FragmentCanvas;
class ToolCom;

class SdLifeLineCanvas : public DiagramCanvas, public SdDurationSupport
{
protected:
    QList<SdDurationCanvas *> durations;
    SdObjCanvas * obj;
    int end; // 0 if masked by user, LIFE_LINE_HEIGHT if not mortal

    void exec_menu(int rank);

public:
    SdLifeLineCanvas(UmlCanvas * canvas, SdObjCanvas * o);
    virtual ~SdLifeLineCanvas();

    virtual void delete_it();

    SdObjCanvas * get_obj() const {
        return obj;
    };
    virtual void add(SdDurationCanvas *);
    virtual void remove(SdDurationCanvas *);
    void toFlat();
    void toOverlapping();
    double instance_max_y() const;
    void update_pos();
    bool can_be_masked() const {
        return durations.isEmpty();
    }
    bool is_masked() const {
        return end == 0;
    }
    void set_masked(bool y);
    virtual void update_instance_dead();
    virtual void update_v_to_contain(SdDurationCanvas *, bool);
    virtual int sub_x(int sub_w) const;
    virtual double min_y() const;
    virtual SdLifeLineCanvas * get_line() const;
    virtual bool isaDuration() const;
    virtual double getZ() const;

    virtual bool is_decenter(const QPoint &, BooL &) const;

    virtual void drawShape(QPainter & p);
    virtual void moveBy(double dx, double dy);

    virtual UmlCode type() const;
    virtual void open();
    virtual void menu(const QPoint &);
    virtual void change_scale();
    virtual QString may_start(UmlCode &) const;
    virtual QString may_connect(UmlCode & l, const DiagramItem * dest) const;
    virtual bool may_connect(UmlCode l) const;
    virtual void connexion(UmlCode, DiagramItem *, const QPoint &, const QPoint &);
    virtual bool connexion(UmlCode, const QPoint &, const QPoint &);
    virtual LineDirection allowed_direction(UmlCode);
    virtual bool copyable() const;
    virtual void apply_shortcut(QString s);

    virtual void save(QTextStream &, bool ref, QString & warning) const;
    virtual void history_save(QBuffer &) const;
    virtual void history_load(QBuffer &);

    static void send(ToolCom * com, const Q3CanvasItemList & l,
                     QList<FragmentCanvas *> & fragments,
                     QList<FragmentCanvas *> & refs);
};

#endif
