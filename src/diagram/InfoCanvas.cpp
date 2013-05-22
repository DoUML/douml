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





#include <qcursor.h>
#include <q3painter.h>
#include <q3popupmenu.h>
//Added by qt3to4:
#include <QTextStream>

#include "InfoCanvas.h"
#include "DiagramCanvas.h"
#include "UmlGlobal.h"
#include "UmlCanvas.h"
#include "BrowserDiagram.h"
#include "Settings.h"
#include "SettingsDialog.h"
#include "myio.h"
#include "ui/menufactory.h"
#include "strutil.h"
#include "DialogUtil.h"
#include "translate.h"

InfoCanvas::InfoCanvas(UmlCanvas * canvas, DiagramItem * a,
                       int x, int y, int id)
    : NoteCanvas(canvas, x, y, id), who(a)
{
    // for read
}

InfoCanvas::InfoCanvas(UmlCanvas * canvas, DiagramItem * a, QString s)
    : NoteCanvas(canvas, 0, 0, 0), who(a)
{
    note = s;

    QFontMetrics fm(the_canvas()->get_font(itsfont));
    QSize sz = fm.size(0, s);
    const int margin = (int)(2 * NOTE_MARGIN * the_canvas()->zoom());
    int w = sz.width() + margin;
    int h = sz.height() + margin;

    if (w < width())
        w = width();

    if (h < height())
        h = height();

    setSize(w, h);
    setZ(a->get_z());

    width_scale100 = w;
    height_scale100 = h;
    set_center100();
}

InfoCanvas::~InfoCanvas()
{
}

UmlCode InfoCanvas::type() const
{
    return UmlInfo;
}

void InfoCanvas::delete_available(BooL & in_model, BooL & out_model) const
{
    if (who->isSelected())
        who->delete_available(in_model, out_model);
}

bool InfoCanvas::copyable() const
{
    return who->isSelected();
}

void InfoCanvas::open()
{
    who->open();
}

void InfoCanvas::set(QString s)
{
    s = toUnicode(s);

    if (s != note) {
        note = s;
        modified();
    }
}

void InfoCanvas::menu(const QPoint &)
{
    Q3PopupMenu m(0);
    Q3PopupMenu fontsubm(0);

    MenuFactory::createTitle(m, TR("Information"));
    m.insertSeparator();
    m.insertItem(TR("Upper"), 0);
    m.insertItem(TR("Lower"), 1);
    m.insertItem(TR("Go up"), 5);
    m.insertItem(TR("Go down"), 6);
    m.insertSeparator();
    m.insertItem(TR("Edit"), 2);
    m.insertSeparator();
    m.insertItem(TR("Font"), &fontsubm);
    init_font_menu(fontsubm, the_canvas(), 10);
    m.insertItem(TR("Edit drawing settings"), 3);

    if (linked()) {
        m.insertSeparator();
        m.insertItem(TR("Select linked items"), 4);
    }

    m.insertSeparator();

    int index = m.exec(QCursor::pos());

    switch (index) {
    case 0:
        upper();
        modified();	// call package_modified()
        return;

    case 1:
        lower();
        modified();	// call package_modified()
        return;

    case 5:
        z_up();
        modified();	// call package_modified()
        return;

    case 6:
        z_down();
        modified();	// call package_modified()
        return;

    case 2:
        who->open();
        return;

    case 3:
        for (;;) {
            ColorSpecVector co(1);

            co[0].set(TR("note color"), &itscolor);

            SettingsDialog dialog(0, &co, FALSE);

            dialog.raise();

            if (dialog.exec() == QDialog::Accepted)
                modified();

            if (!dialog.redo())
                return;
        }

        break;

    case 4:
        the_canvas()->unselect_all();
        select_associated();
        return;

    default:
        if (index >= 10) {
            itsfont = (UmlFont)(index - 10);
            modified();
        }

        return;
    }

    package_modified();
}

void InfoCanvas::apply_shortcut(QString s)
{
    if (s == "Upper")
        upper();
    else if (s == "Lower")
        lower();
    else if (s == "Go up")
        z_up();
    else if (s == "Go down")
        z_down();
    else
        return;

    modified();
    package_modified();
}

bool InfoCanvas::has_drawing_settings() const
{
    return TRUE;
}

void InfoCanvas::edit_drawing_settings(QList<DiagramItem *> & l)
{
    for (;;) {
        ColorSpecVector co(1);
        UmlColor itscolor;

        co[0].set(TR("note color"), &itscolor);

        SettingsDialog dialog(0, &co, FALSE, TRUE);

        dialog.raise();

        if ((dialog.exec() == QDialog::Accepted) && !co[0].name.isEmpty()) {
            foreach (DiagramItem *item, l) {
                InfoCanvas *canvas = (InfoCanvas *)item;
                canvas->itscolor = itscolor;
                canvas->modified();	// call package_modified()
            }
        }

        if (!dialog.redo())
            break;
    }
}

void InfoCanvas::clone_drawing_settings(const DiagramItem *src)
{
    const InfoCanvas * x = (const InfoCanvas *) src;
    itscolor = x->itscolor;
    modified();
}

void InfoCanvas::save(QTextStream & st, bool ref, QString &) const
{
    if (ref) {
        st << "information_ref " << get_ident();
    }
    else {
        nl_indent(st);

        st << "information " << get_ident() << " ";
        save_internal(st);
    }
}


InfoCanvas * InfoCanvas::read(char *& st, UmlCanvas * canvas,
                              char * k, DiagramItem * who)
{
    if (!strcmp(k, "information_ref"))
        return (InfoCanvas *) dict_get(read_id(st), "information", canvas);
    else if (!strcmp(k, "information")) {
        int id = read_id(st);
        InfoCanvas * result = new InfoCanvas(canvas, who, 0, 0, id);

        result->read_internal(st);
        return result;
    }
    else
        return 0;
}
