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

#ifndef CODDIRSCANVAS_H
#define CODDIRSCANVAS_H

#include "DiagramCanvas.h"
#include "MultipleDependency.h"
#include "CodMsgSupport.h"
//Added by qt3to4:
#include <QTextStream>

class CodLinkCanvas;
class QTextStream;

#define COL_MSG_Z 1000
#define OLD_COL_MSG_Z 0.2

#define COL_DIRS_SIZE 50

#define isa_col_msg_dirs(x) (((x)->type()-DiagramItemTypeStart) == RTTI_COL_MSG)

class CodDirsCanvas : public QObject, public DiagramCanvas, public CodMsgSupport,
    public MultipleDependency<BasicData>
{
    Q_OBJECT

protected:
    double angle;

    LabelCanvas * backward_label;
    CodLinkCanvas * link;	// to get start & end

    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
public:
    CodDirsCanvas(UmlCanvas * canvas, CodLinkCanvas * l, int id);
    virtual ~CodDirsCanvas();

    virtual void delete_it() override;
    virtual void remove_it(ColMsg * msg) override;

    virtual void get_from_to(CodObjCanvas *& from, CodObjCanvas *& to,
                             bool forward) override;

    virtual UmlCode typeUmlCode() const override;
    virtual int rtti() const;
    virtual void open() override;
    virtual void menu(const QPoint &) override;
    virtual QString may_start(UmlCode &) const override;
    virtual QString may_connect(UmlCode &, const DiagramItem *) const override;
    virtual bool copyable() const override;

    void set_link(CodLinkCanvas * l) {
        link = l;
    };
    void update_pos(const QPoint & link_start, const QPoint & link_end);
    void update_label_pos(LabelCanvas *, bool forward);
    virtual void update_msgs() override;
    using DiagramCanvas::edit_drawing_settings;
    bool edit_drawing_settings();
    virtual void draw(QPainter & p);
    virtual void setVisible(bool yes) override;

    virtual bool represents(BrowserNode *) override;

    virtual void save(QTextStream & st, bool, QString & warning) const override;
    static CodDirsCanvas * read(char *& st, UmlCanvas * canvas, char *& k);
    virtual void history_save(QBuffer &) const override;
    virtual void history_load(QBuffer &) override;
    virtual void history_hide() override;
    virtual int type() const override;

private slots:
    void modified();	// messages must be updated
    //void deleted();	// the relation is deleted
};

#endif
