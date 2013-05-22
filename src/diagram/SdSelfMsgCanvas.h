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

#ifndef SDSELFMSGCANVAS_H
#define SDSELFMSGCANVAS_H

#include "SdMsgBaseCanvas.h"
//Added by qt3to4:
#include <QTextStream>

class SdSelfMsgCanvas : public SdMsgBaseCanvas
{
public:
    SdSelfMsgCanvas(UmlCanvas * canvas, SdMsgSupport * d,
                    UmlCode l, int v, int id);
    virtual ~SdSelfMsgCanvas();

    virtual void draw(QPainter & p);

    virtual void change_scale();
    virtual void update_hpos();
    virtual void check_vpos(const QRect &);
    virtual double min_y() const;
    virtual void update();
    virtual void change_duration(SdDurationCanvas * oldone,
                                 SdDurationCanvas * newone);
    virtual int overlap_dir(SdDurationCanvas *) const;
    virtual void menu(const QPoint &);
    virtual void select_associated();

    virtual bool has_drawing_settings() const;
    virtual void edit_drawing_settings(QList<DiagramItem *> &);
    virtual void clone_drawing_settings(const DiagramItem *src);

    virtual void apply_shortcut(QString s);
    void edit_drawing_settings();

    virtual void save(QTextStream &, bool ref, QString & warning) const;
    static SdSelfMsgCanvas * read(char *& , UmlCanvas *, char *);
};

#endif

