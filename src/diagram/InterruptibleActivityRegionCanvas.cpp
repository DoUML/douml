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





#include <q3popupmenu.h>
#include <qcursor.h>
#include <qpainter.h>
//Added by qt3to4:
#include <QTextStream>

#include "InterruptibleActivityRegionCanvas.h"
#include "SimpleData.h"
#include "BrowserInterruptibleActivityRegion.h"
#include "BrowserActivityDiagram.h"
#include "ArrowCanvas.h"
#include "UmlPixmap.h"
#include "UmlGlobal.h"
#include "SettingsDialog.h"
#include "myio.h"
#include "ToolCom.h"
#include "Tool.h"
#include "ui/menufactory.h"
#include "Settings.h"
#include "strutil.h"
#include "translate.h"

InterruptibleActivityRegionCanvas::InterruptibleActivityRegionCanvas(BrowserNode * bn, UmlCanvas * canvas,
        int x, int y)
    : ActivityContainerCanvas(0, canvas, x, y,
                              INTERRUPTIBLE_ACTIVITY_REGION_CANVAS_MIN_SIZE,
                              INTERRUPTIBLE_ACTIVITY_REGION_CANVAS_MIN_SIZE, 0)
{
    browser_node = bn;
    itscolor = UmlDefaultColor;

    update();
    check_stereotypeproperties();

    connect(bn->get_data(), SIGNAL(changed()), this, SLOT(modified()));
    connect(bn->get_data(), SIGNAL(deleted()), this, SLOT(deleted()));
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
}

InterruptibleActivityRegionCanvas::InterruptibleActivityRegionCanvas(UmlCanvas * canvas, int id)
    : ActivityContainerCanvas(canvas, id)
{
    // for read operation
    browser_node = 0;
    itscolor = UmlDefaultColor;
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
}

InterruptibleActivityRegionCanvas::~InterruptibleActivityRegionCanvas()
{
}

void InterruptibleActivityRegionCanvas::delete_it()
{
    disconnect(DrawingSettings::instance(), SIGNAL(changed()),
               this, SLOT(modified()));
    disconnect(browser_node->get_data(), 0, this, 0);

    DiagramCanvas::delete_it();
}

void InterruptibleActivityRegionCanvas::deleted()
{
    delete_it();
    canvas()->update();
}

void InterruptibleActivityRegionCanvas::remove(bool from_model)
{
    if (! from_model)
        delete_it();
    else
        browser_node->delete_it();	// will remove canvas
}

void InterruptibleActivityRegionCanvas::update()
{
    used_color = (itscolor == UmlDefaultColor)
                 ? the_canvas()->browser_diagram()->get_color(UmlInterruptibleActivityRegion)
                 : itscolor;
}

void InterruptibleActivityRegionCanvas::modified()
{
    hide();
    hide_lines();
    update();
    show();
    update_show_lines();
    check_stereotypeproperties();
    canvas()->update();
    force_sub_inside(FALSE);
    package_modified();
}

aCorner InterruptibleActivityRegionCanvas::on_resize_point(const QPoint & p)
{
    return ::on_resize_point(p, rect());
}

void InterruptibleActivityRegionCanvas::resize(aCorner c, int dx, int dy, QPoint & o)
{
    double zoom = the_canvas()->zoom();

    DiagramCanvas::resize(c, dx, dy, o,
                          (int)(INTERRUPTIBLE_ACTIVITY_REGION_CANVAS_MIN_SIZE * zoom),
                          (int)(INTERRUPTIBLE_ACTIVITY_REGION_CANVAS_MIN_SIZE * zoom));

    force_sub_inside(FALSE);
}

void InterruptibleActivityRegionCanvas::resize(const QSize & sz, bool w, bool h)
{
    double zoom = the_canvas()->zoom();

    if (DiagramCanvas::resize(sz, w, h,
                              (int)(INTERRUPTIBLE_ACTIVITY_REGION_CANVAS_MIN_SIZE * zoom),
                              (int)(INTERRUPTIBLE_ACTIVITY_REGION_CANVAS_MIN_SIZE * zoom)))
        force_sub_inside(FALSE);
}

bool InterruptibleActivityRegionCanvas::move_with_its_package() const
{
    return TRUE;
}

void InterruptibleActivityRegionCanvas::force_sub_inside(bool resize_it)
{
    // update sub nodes position to be inside of the activity region
    // or resize it to contains sub elts if resize_it
    Q3CanvasItemList all = canvas()->allItems();
    BooL need_sub_upper = FALSE;

    if (resize_it)
        resize_to_contain(all, need_sub_upper);
    else
        ActivityContainerCanvas::force_sub_inside(all, need_sub_upper);

    if (need_sub_upper)
        force_sub_upper(all);
}

void InterruptibleActivityRegionCanvas::draw(QPainter & p)
{
    if (! visible()) return;

    p.setRenderHint(QPainter::Antialiasing, true);
    QRect r = rect();
    QBrush brsh = p.brush();
    QColor bckgrnd = p.backgroundColor();

    p.setBackgroundMode((used_color == UmlTransparent)
                        ? ::Qt::TransparentMode
                        : ::Qt::OpaqueMode);

    QColor co = color(used_color);

    p.setBackgroundColor(co);

    if (used_color != UmlTransparent)
        p.setBrush(co);

    p.setPen(::Qt::DotLine);
    p.drawRoundRect(r, 8, 8);

    FILE * fp = svg();

    if (fp != 0)
        fprintf(fp,
                "\t<rect fill=\"%s\" stroke=\"black\" stroke-dasharray=\"4,4\" stroke-opacity=\"1\""
                " x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" rx=\"10\" />\n",
                svg_color(used_color),
                r.left(), r.top(), r.width() - 1, r.height() - 1);

    p.setPen(::Qt::SolidLine);
    p.setBackgroundColor(bckgrnd);
    p.setBrush(brsh);

    if (selected())
        show_mark(p, r);
}

UmlCode InterruptibleActivityRegionCanvas::type() const
{
    return UmlInterruptibleActivityRegion;
}

void InterruptibleActivityRegionCanvas::delete_available(BooL & in_model, BooL & out_model) const
{
    out_model |= TRUE;
    in_model |= browser_node->is_writable();
}

bool InterruptibleActivityRegionCanvas::alignable() const
{
    return TRUE;
}

bool InterruptibleActivityRegionCanvas::copyable() const
{
    return selected();
}

void InterruptibleActivityRegionCanvas::open()
{
    browser_node->open(FALSE);
}

void InterruptibleActivityRegionCanvas::menu(const QPoint &)
{
    Q3PopupMenu m(0);
    Q3PopupMenu toolm(0);
    int index;

    MenuFactory::createTitle(m, browser_node->get_data()->definition(FALSE, TRUE));
    m.insertSeparator();
    m.insertItem(TR("Upper"), 0);
    m.insertItem(TR("Lower"), 1);
    m.insertItem(TR("Go up"), 13);
    m.insertItem(TR("Go down"), 14);
    m.insertSeparator();
    m.insertItem(TR("Edit drawing settings"), 2);
    m.insertSeparator();
    m.insertItem(TR("Edit interruptible activity region"), 3);
    m.insertSeparator();
    m.insertItem(TR("Select in browser"), 4);

    if (linked())
        m.insertItem(TR("Select linked items"), 5);

    m.insertSeparator();

    if (browser_node->is_writable()) {
        if (browser_node->get_associated() !=
            (BrowserNode *) the_canvas()->browser_diagram())
            m.insertItem(TR("Set associated diagram"), 6);

        if (browser_node->get_associated())
            m.insertItem(TR("Remove diagram association"), 9);
    }

    m.insertSeparator();
    m.insertItem(TR("Remove from diagram"), 7);

    if (browser_node->is_writable())
        m.insertItem(TR("Delete from model"), 8);

    m.insertSeparator();

    if (Tool::menu_insert(&toolm, UmlInterruptibleActivityRegion, 20))
        m.insertItem(TR("Tool"), &toolm);

    switch (index = m.exec(QCursor::pos())) {
    case 0:
        upper();
        modified();	// call package_modified()
        return;

    case 1:
        lower();
        modified();	// call package_modified()
        return;

    case 13:
        z_up();
        modified();	// call package_modified()
        return;

    case 14:
        z_down();
        modified();	// call package_modified()
        return;

    case 2:
        edit_drawing_settings();
        return;

    case 3:
        browser_node->open(TRUE);
        return;

    case 4:
        browser_node->select_in_browser();
        return;

    case 5:
        the_canvas()->unselect_all();
        select_associated();
        return;

    case 6:
        ((BrowserInterruptibleActivityRegion *) browser_node)
        ->set_associated_diagram((BrowserActivityDiagram *)
                                 the_canvas()->browser_diagram());
        return;

    case 9:
        ((BrowserInterruptibleActivityRegion *) browser_node)
        ->set_associated_diagram(0);
        return;

    case 7:
        //remove from diagram
        delete_it();
        break;

    case 8:
        //delete from model
        browser_node->delete_it();	// will delete the canvas
        break;

    default:
        if (index >= 20)
            ToolCom::run(Tool::command(index - 20), browser_node);

        return;
    }

    package_modified();
}

void InterruptibleActivityRegionCanvas::apply_shortcut(QString s)
{
    if (s == "Select in browser") {
        browser_node->select_in_browser();
        return;
    }
    else if (s == "Upper")
        upper();
    else if (s == "Lower")
        lower();
    else if (s == "Go up")
        z_up();
    else if (s == "Go down")
        z_down();
    else if (s == "Edit drawing settings") {
        edit_drawing_settings();
        return;
    }
    else {
        browser_node->apply_shortcut(s);
        return;
    }

    modified();
    package_modified();
}

void InterruptibleActivityRegionCanvas::edit_drawing_settings()
{
    for (;;) {
        ColorSpecVector co(1);

        co[0].set(TR("interruptible activity region color"), &itscolor);

        SettingsDialog dialog(0, &co, FALSE);

        dialog.raise();

        if (dialog.exec() == QDialog::Accepted)
            modified();	// call package_modified()

        if (!dialog.redo())
            break;
    }
}

bool InterruptibleActivityRegionCanvas::has_drawing_settings() const
{
    return TRUE;
}

void InterruptibleActivityRegionCanvas::edit_drawing_settings(QList<DiagramItem *> & l)
{
    for (;;) {
        ColorSpecVector co(1);
        UmlColor itscolor;

        co[0].set(TR("interruptible activity region color"), &itscolor);

        SettingsDialog dialog(0, &co, FALSE, TRUE);

        dialog.raise();

        if ((dialog.exec() == QDialog::Accepted) && !co[0].name.isEmpty()) {
            foreach (DiagramItem *item, l) {
                InterruptibleActivityRegionCanvas *canvas = (InterruptibleActivityRegionCanvas *)item;
                canvas->itscolor = itscolor;
                canvas->modified();
            }
        }

        if (!dialog.redo())
            break;
    }
}

void InterruptibleActivityRegionCanvas::clone_drawing_settings(const DiagramItem *src)
{
    const InterruptibleActivityRegionCanvas * x = (const InterruptibleActivityRegionCanvas *) src;
    itscolor = x->itscolor;
    modified();
}

QString InterruptibleActivityRegionCanvas::may_start(UmlCode & l) const
{
    return (l == UmlAnchor) ? QString() : TR("illegal");
}

QString InterruptibleActivityRegionCanvas::may_connect(UmlCode & l, const DiagramItem * dest) const
{
    return (l == UmlAnchor) ? dest->may_start(l) : TR("illegal");
}

void InterruptibleActivityRegionCanvas::connexion(UmlCode action, DiagramItem * dest,
        const QPoint &, const QPoint &)
{
    ArrowCanvas * a =
        new ArrowCanvas(the_canvas(), this, dest, action, 0, FALSE, -1.0, -1.0);

    a->show();
    the_canvas()->select(a);
}

void InterruptibleActivityRegionCanvas::save(QTextStream & st, bool ref, QString & warning) const
{
    if (ref) {
        st << "interruptibleactivityregioncanvas_ref " << get_ident() << " // "
           << browser_node->full_name();
    }
    else {
        nl_indent(st);
        st << "interruptibleactivityregioncanvas " << get_ident() << " ";
        browser_node->save(st, TRUE, warning);
        indent(+1);

        if (itscolor != UmlDefaultColor) {
            nl_indent(st);
            st << "color " << stringify(itscolor);
        }

        save_stereotype_property(st, warning);
        nl_indent(st);
        save_xyzwh(st, this, "xyzwh");

        indent(-1);
        nl_indent(st);
        st << "end";
    }
}

InterruptibleActivityRegionCanvas *
InterruptibleActivityRegionCanvas::read(char *& st, UmlCanvas * canvas,
                                        char * k)
{
    if (!strcmp(k, "interruptibleactivityregioncanvas_ref"))
        return ((InterruptibleActivityRegionCanvas *) dict_get(read_id(st), "interruptibleactivityregioncanvas", canvas));
    else if (!strcmp(k, "interruptibleactivityregioncanvas")) {
        int id = read_id(st);
        BrowserInterruptibleActivityRegion * br =
            BrowserInterruptibleActivityRegion::read_ref(st);
        InterruptibleActivityRegionCanvas * result =
            new InterruptibleActivityRegionCanvas(canvas, id);

        result->browser_node = br;
        connect(br->get_data(), SIGNAL(changed()), result, SLOT(modified()));
        connect(br->get_data(), SIGNAL(deleted()), result, SLOT(deleted()));

        k = read_keyword(st);

        read_color(st, "color", result->itscolor, k);	// updates k
        result->read_stereotype_property(st, k);	// updates k

        if (!strcmp(k, "xyzwh"))
            read_xyzwh(st, result);
        else
            wrong_keyword(k, "xyzwh");

        result->width_scale100 = result->width();
        result->height_scale100 = result->height();
        result->set_center100();

        result->update();
        result->show();
        result->check_stereotypeproperties();

        read_keyword(st, "end");

        // result->force_sub_inside() useless

        if (canvas->paste())
            result->remove_if_already_present();

        return result;
    }
    else
        return 0;
}

void InterruptibleActivityRegionCanvas::history_save(QBuffer & b) const
{
    DiagramCanvas::history_save(b);
    ::save(width_scale100, b);
    ::save(height_scale100, b);
    ::save(width(), b);
    ::save(height(), b);
}

void InterruptibleActivityRegionCanvas::history_load(QBuffer & b)
{
    DiagramCanvas::history_load(b);
    ::load(width_scale100, b);
    ::load(height_scale100, b);

    int w, h;

    ::load(w, b);
    ::load(h, b);
    Q3CanvasRectangle::setSize(w, h);

    connect(browser_node->get_data(), SIGNAL(changed()), this, SLOT(modified()));
    connect(browser_node->get_data(), SIGNAL(deleted()), this, SLOT(deleted()));
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
}

void InterruptibleActivityRegionCanvas::history_hide()
{
    Q3CanvasItem::setVisible(FALSE);

    disconnect(DrawingSettings::instance(), SIGNAL(changed()),
               this, SLOT(modified()));
    disconnect(browser_node->get_data(), 0, this, 0);
}
