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

#ifndef INFOCANVAS_H
#define INFOCANVAS_H

#include "NoteCanvas.h"
//Added by qt3to4:
#include <QTextStream>

class DiagramItem;

class InfoCanvas : public NoteCanvas
{
protected:
    DiagramItem * who;

public:
    InfoCanvas(UmlCanvas * canvas, DiagramItem *, int x, int y, int id);
    InfoCanvas(UmlCanvas * canvas, DiagramItem *, QString);
    virtual ~InfoCanvas();

    virtual UmlCode type() const;
    virtual void delete_available(BooL & in_model, BooL & out_model) const;
    virtual bool copyable() const;
    virtual void open();
    virtual void menu(const QPoint &);

    virtual void apply_shortcut(QString s);
    virtual bool has_drawing_settings() const;
    virtual void edit_drawing_settings(QList<DiagramItem *> &);
    virtual void clone_drawing_settings(const DiagramItem *src);

    virtual void save(QTextStream  & st, bool ref, QString & warning) const;
    static InfoCanvas * read(char *& , UmlCanvas *, char *, DiagramItem *);

    void set(QString s);
};

#endif
