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
#include <Q3ValueList>

#include "ConstraintCanvas.h"
#include "ConstraintDialog.h"
#include "BasicData.h"
#include "CdClassCanvas.h"
#include "UmlGlobal.h"
#include "UmlCanvas.h"
#include "BrowserDiagram.h"
#include "BrowserClass.h"
#include "BrowserAttribute.h"
#include "BrowserRelation.h"
#include "BrowserOperation.h"
#include "Settings.h"
#include "SettingsDialog.h"
#include "myio.h"
#include "ui/menufactory.h"
#include "strutil.h"
#include "DialogUtil.h"
#include "translate.h"

ConstraintCanvas::ConstraintCanvas(UmlCanvas * canvas, CdClassCanvas * a,
                                   int x, int y, int id)
    : NoteCanvas(canvas, x, y, id), cl(a)
{
    // for read
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(update()));
}

ConstraintCanvas::ConstraintCanvas(UmlCanvas * canvas, CdClassCanvas * a, QString s)
    : NoteCanvas(canvas, 0, 0, 0),
      indicate_visible(FALSE), cl(a)
{
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(update()));

    note = s;	// already unicode

    QFontMetrics fm(the_canvas()->get_font(itsfont));
    QSize sz = fm.size(0, s);
    const int margin = (int)(2 * NOTE_MARGIN * the_canvas()->zoom());
    int w = sz.width() + margin;
    int h = sz.height() + margin;

    if (w < width())
        w = width();

    if (h < height())
        h = height();

    DiagramCanvas::resize(w, h);
    setZ(a->get_z());

    width_scale100 = w;
    height_scale100 = h;
    set_center100();
}

ConstraintCanvas::~ConstraintCanvas()
{
}

void ConstraintCanvas::delete_it()
{
    disconnect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(update()));

    Q3ValueList<BasicData *>::Iterator iter;

    for (iter = connect_list.begin(); iter != connect_list.end(); ++iter)
        disconnect(*iter, 0, this, 0);

    NoteCanvas::delete_it();
}

UmlCode ConstraintCanvas::type() const
{
    return UmlConstraint;
}

void ConstraintCanvas::delete_available(BooL & in_model, BooL & out_model) const
{
    if (cl->isSelected())
        cl->delete_available(in_model, out_model);
}

bool ConstraintCanvas::copyable() const
{
    return cl->isSelected();
}

void ConstraintCanvas::open()
{
    ConstraintDialog dialog(this);

    dialog.raise();

    if (dialog.exec() == QDialog::Accepted) {
        update();
        package_modified();
    }
}

void ConstraintCanvas::menu(const QPoint &)
{
    Q3PopupMenu m(0);
    Q3PopupMenu fontsubm(0);

    MenuFactory::createTitle(m, TR("Constraint"));
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
        open();
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
            modified();	// call package_modified()
        }

        return;
    }

    package_modified();
}

void ConstraintCanvas::apply_shortcut(QString s)
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

bool ConstraintCanvas::has_drawing_settings() const
{
    return TRUE;
}

void ConstraintCanvas::edit_drawing_settings(QList<DiagramItem *> & l)
{
    for (;;) {
        ColorSpecVector co(1);
        UmlColor itscolor;

        co[0].set(TR("note color"), &itscolor);

        SettingsDialog dialog(0, &co, FALSE, TRUE);

        dialog.raise();

        if ((dialog.exec() == QDialog::Accepted) && !co[0].name.isEmpty()) {
            foreach (DiagramItem *item, l) {
                ConstraintCanvas *canvas = (ConstraintCanvas *)item;
                canvas->itscolor = itscolor;
                canvas->modified();
            }
        }

        if (!dialog.redo())
            break;
    }
}

void ConstraintCanvas::clone_drawing_settings(const DiagramItem *src)
{
    const ConstraintCanvas * x = (const ConstraintCanvas *) src;
    itscolor = x->itscolor;
    modified();
}

// warning : don't remove connect because may be called
// during connexion list use
ConstraintCanvas *
ConstraintCanvas::compute(UmlCanvas * canvas,
                          CdClassCanvas * cl,
                          ConstraintCanvas * current)
{
    Q3ValueList<BasicData *> list;
    BrowserNodeList elts;

    ((BrowserClass *) cl->get_bn())->get_tree(elts);

    QString s;
    QString constraint;

    if (current == 0) {
        // get all
        foreach (BrowserNode *bn, elts) {
            if (bn->get_type() == UmlClass)
                // change on class's members modify class => memorize classes only
                list.append(bn->get_data());

            constraint = toUnicode(bn->constraint());

            if (!constraint.isEmpty())
                s += constraint + '\n';
        }
    }
    else if (current->indicate_visible) {
        Q3ValueList<BrowserNode *> & visible = current->hidden_visible;

        foreach (BrowserNode *bn, elts) {
            if (visible.findIndex(bn) != -1) {
                if (bn->get_type() == UmlClass)
                    // change on class's members modify class => memorize classes only
                    list.append(bn->get_data());

                constraint = toUnicode(bn->constraint());

                if (!constraint.isEmpty())
                    s += constraint + '\n';
            }
        }
    }
    else {
        Q3ValueList<BrowserNode *> & hidden = current->hidden_visible;

        foreach (BrowserNode *bn, elts) {
            if (hidden.findIndex(bn) == -1) {
                if (bn->get_type() == UmlClass)
                    // change on class's members modify class => memorize classes only
                    list.append(bn->get_data());

                constraint = toUnicode(bn->constraint());

                if (!constraint.isEmpty())
                    s += constraint + '\n';
            }
        }
    }

    if (current == 0) {
        current = new ConstraintCanvas(canvas, cl, s);
        current->package_modified();
    }
    else if (s != current->note) {
        current->note = s;
        current->modified();	// call package_modified()
    }

    current->elements = elts;

    Q3ValueList<BasicData *> & old_list = current->connect_list;

    for (Q3ValueList<BasicData *>::Iterator iterd = list.begin(); iterd != list.end(); ++iterd) {
        if (old_list.findIndex(*iterd) == -1) {
            old_list.append(*iterd);
            connect(*iterd, SIGNAL(changed()), current, SLOT(update()));
            connect(*iterd, SIGNAL(deleted()), current, SLOT(update()));
        }
    }

    return current;
}

void ConstraintCanvas::update()
{
    cl->check_constraint();
}

void ConstraintCanvas::save(QTextStream & st, bool ref, QString & warning) const
{
    if (ref) {
        st << "constraint_ref " << get_ident();
    }
    else {
        nl_indent(st);

        st << "constraint " << get_ident();
        indent(+1);
        nl_indent(st);
        st << ((indicate_visible)  ? "visible" : "hidden");
        indent(+1);

        Q3ValueList<BrowserNode *>::ConstIterator iter;

        for (iter = hidden_visible.begin(); iter != hidden_visible.end(); iter++) {
            nl_indent(st);
            (*iter)->save(st, TRUE, warning);
        }

        indent(-1);
        nl_indent(st);
        st << "end";
        nl_indent(st);
        save_internal(st);
        indent(-1);
    }
}

ConstraintCanvas * ConstraintCanvas::read(char *& st, UmlCanvas * canvas,
        char * k, CdClassCanvas * cl)
{
    if (!strcmp(k, "constraint_ref"))
        return (ConstraintCanvas *) dict_get(read_id(st), "constraint", canvas);
    else if (!strcmp(k, "constraint")) {
        int id = read_id(st);
        ConstraintCanvas * result = new ConstraintCanvas(canvas, cl, 0, 0, id);

        k = read_keyword(st);
        result->indicate_visible = (*k == 'v');

        while (strcmp(k = read_keyword(st), "end")) {
            BrowserNode * bn;

            if (((bn = BrowserClass::read(st, k, 0, FALSE)) != 0) ||
                ((bn = BrowserOperation::read(st, k, 0, FALSE)) != 0) ||
                ((bn = BrowserAttribute::read(st, k, 0, FALSE)) != 0) ||
                ((bn = BrowserRelation::read(st, k, 0, FALSE)) != 0))
                result->hidden_visible.append(bn);
        }

        result->read_internal(st);
        // update done by cl
        return result;
    }
    else
        return 0;
}

void ConstraintCanvas::history_hide()
{
    Q3CanvasItem::setVisible(FALSE);
    disconnect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(update()));

    Q3ValueList<BasicData *>::Iterator iter;

    for (iter = connect_list.begin(); iter != connect_list.end(); ++iter)
        disconnect(*iter, 0, this, 0);
}

void ConstraintCanvas::history_load(QBuffer & b)
{
    NoteCanvas::history_load(b);
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(update()));

    Q3ValueList<BasicData *>::Iterator iter;

    for (iter = connect_list.begin(); iter != connect_list.end(); ++iter) {
        connect(*iter, SIGNAL(changed()), this, SLOT(update()));
        connect(*iter, SIGNAL(deleted()), this, SLOT(update()));
    }
}
