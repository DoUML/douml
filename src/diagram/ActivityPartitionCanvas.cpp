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

#include "ActivityPartitionCanvas.h"
#include "ActivityPartitionData.h"
#include "BrowserActivityPartition.h"
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

ActivityPartitionCanvas::ActivityPartitionCanvas(BrowserNode * bn, UmlCanvas * canvas,
        int x, int y)
    : ActivityContainerCanvas(0, canvas, x, y,
                              ACTIVITY_PARTITION_CANVAS_MIN_SIZE,
                              ACTIVITY_PARTITION_CANVAS_MIN_SIZE, 0)
{
    browser_node = bn;
    itscolor = UmlDefaultColor;

    BrowserNode * parent = (BrowserNode *) browser_node->parent();

    if (parent->get_type() == UmlActivityPartition) {
        Q3CanvasItemList all = the_canvas()->allItems();
        Q3CanvasItemList::Iterator cit;

        for (cit = all.begin(); cit != all.end(); ++cit) {
            if ((*cit)->visible()) {
                DiagramItem * di = QCanvasItemToDiagramItem(*cit);

                if ((di != 0) && (di->get_bn() == parent)) {
                    horiz = ((ActivityPartitionCanvas *) di)->horiz;
                    break;
                }
            }
        }

        if (cit == all.end())
            horiz = FALSE;
    }
    else
        horiz = FALSE;

    check_size();
    check_stereotypeproperties();

    ActivityPartitionData * data =
        (ActivityPartitionData *) browser_node->get_data();

    connect(data, SIGNAL(changed()), this, SLOT(modified()));
    connect(data, SIGNAL(deleted()), this, SLOT(deleted()));
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
}

ActivityPartitionCanvas::ActivityPartitionCanvas(UmlCanvas * canvas, int id)
    : ActivityContainerCanvas(canvas, id)
{
    // for read operation
    browser_node = 0;
    itscolor = UmlDefaultColor;
    horiz = FALSE;
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
}

ActivityPartitionCanvas::~ActivityPartitionCanvas()
{
}

void ActivityPartitionCanvas::delete_it()
{
    disconnect(DrawingSettings::instance(), SIGNAL(changed()),
               this, SLOT(modified()));
    disconnect(browser_node->get_data(), 0, this, 0);

    DiagramCanvas::delete_it();
}

void ActivityPartitionCanvas::deleted()
{
    delete_it();
    canvas()->update();
}

void ActivityPartitionCanvas::remove(bool from_model)
{
    if (! from_model)
        delete_it();
    else
        browser_node->delete_it();	// will remove canvas
}

void ActivityPartitionCanvas::check_size()
{
    ActivityPartitionData * data =
        (ActivityPartitionData *) browser_node->get_data();

    if (data->get_represents() != 0) {
        BasicData * d = data->get_represents()->get_data();

        connect(d, SIGNAL(changed()), this, SLOT(modified()));
        connect(d, SIGNAL(deleted()), this, SLOT(modified()));
    }

    QFontMetrics fm(the_canvas()->get_font(UmlNormalFont));

    str = data->get_stereotype();

    if (!str.isEmpty())
        str = "<<" + str + ">>";
    else if (data->get_is_external())
        str = "<<external>>";

    int w1 = fm.width(str);
    int w2;
    QString s;

    if (!(s = browser_node->get_name()).isEmpty() ||
        ((data->get_represents() != 0) &&
         !data->get_represents()->deletedp() &&
         !(s = data->get_represents()->get_name()).isEmpty())) {
        w2 = fm.width(s);

        if (w2 > w1)
            w1 = w2;

        str += "\n" + s;
    }

    w1 += fm.height();

    w2 = (int)(ACTIVITY_PARTITION_CANVAS_MIN_SIZE * the_canvas()->zoom());

    if (w2 > w1)
        w1 = w2;

    if (horiz) {
        min_height = w1;
        min_width = w2;
    }
    else {
        min_width = w1;
        min_height = w2;
    }

    // force odd width and height for line alignment
    min_width |= 1;
    min_height |= 1;

    // warning : do NOT check if ((width() < min_width) || (height() < min_height))
    // because te resize must be done to set data on scale change
    DiagramCanvas::resize((width() > min_width) ? width() : min_width,
                          (height() > min_height) ? height() : min_height);

    used_color = (itscolor == UmlDefaultColor)
                 ? the_canvas()->browser_diagram()->get_color(UmlActivityPartition)
                 : itscolor;
}

void ActivityPartitionCanvas::change_scale()
{
    double scale = the_canvas()->zoom();

    Q3CanvasRectangle::setVisible(FALSE);
    setSize((int)(width_scale100 * scale), (int)(height_scale100 * scale));
    check_size();
    recenter();
    Q3CanvasRectangle::setVisible(TRUE);
}

void ActivityPartitionCanvas::modified()
{
    hide();
    hide_lines();
    check_size();
    show();
    update_show_lines();
    check_stereotypeproperties();
    canvas()->update();
    force_sub_inside(FALSE);
    package_modified();
}

aCorner ActivityPartitionCanvas::on_resize_point(const QPoint & p)
{
    return ::on_resize_point(p, rect());
}

void ActivityPartitionCanvas::resize(aCorner c, int dx, int dy, QPoint & o)
{
    double zoom = the_canvas()->zoom();

    DiagramCanvas::resize(c, dx, dy, o,
                          (int)(ACTIVITY_PARTITION_CANVAS_MIN_SIZE * zoom),
                          (int)(ACTIVITY_PARTITION_CANVAS_MIN_SIZE * zoom));

    force_sub_inside(FALSE);
}

void ActivityPartitionCanvas::resize(const QSize & sz, bool w, bool h)
{
    double zoom = the_canvas()->zoom();

    if (DiagramCanvas::resize(sz, w, h,
                              (int)(ACTIVITY_PARTITION_CANVAS_MIN_SIZE * zoom),
                              (int)(ACTIVITY_PARTITION_CANVAS_MIN_SIZE * zoom)))
        force_sub_inside(FALSE);
}

bool ActivityPartitionCanvas::move_with_its_package() const
{
    return TRUE;
}

void ActivityPartitionCanvas::force_sub_inside(bool resize_it)
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

void ActivityPartitionCanvas::draw(QPainter & p)
{
    if (! visible()) return;

    p.setRenderHint(QPainter::Antialiasing, true);

    QRect r = rect();
    FILE * fp = svg();
    QColor bckgrnd = p.backgroundColor();
    QColor co = color(used_color);

    p.setBackgroundMode((used_color == UmlTransparent)
                        ? ::Qt::TransparentMode
                        : ::Qt::OpaqueMode);


    p.setBackgroundColor(co);

    if (used_color != UmlTransparent)
        p.fillRect(r, co);

    p.setFont(the_canvas()->get_font(UmlNormalFont));

    QFontMetrics fm(p.font());
    int h = 3 * fm.height();

    if (fp != 0) {
        fprintf(fp, "<g>\n\t<rect fill=\"%s\" stroke=\"black\" stroke-width=\"1\" stroke-opacity=\"1\""
                " x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" />\n",
                svg_color(used_color),
                r.x(), r.y(), r.width() - 1, r.height() - 1);
    }

    p.drawRect(r);

    if (horiz) {
        if (! str.isEmpty()) {
            p.save();
            p.rotate(-90);
            p.drawText(-r.y(), r.x(), -r.height(), h,
                       ::Qt::AlignCenter, str);
            p.restore();

            if (fp != 0) {
                int index = str.find('\n');

                if (index == -1)
                    draw_rotate_text(r.x() + h / 2, r.y() + r.height() / 2,
                                     270, str, p.font(), fp);
                else {
                    int vc = r.y() + r.height() / 2;

                    draw_rotate_text(r.x() + h / 4, vc,
                                     270, str.left(index), p.font(), fp);
                    draw_rotate_text(r.x() + (h * 3) / 4, vc,
                                     270, str.mid(index + 1), p.font(), fp);
                }
            }
        }

        r.setLeft(r.left() + h);
        p.drawLine(r.topLeft(), r.bottomLeft());

        if (fp) {
            fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                    " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                    r.left(), r.top(), r.left(), r.bottom());
            fputs("</g>\n", fp);
        }
    }
    else {
        if (! str.isEmpty()) {
            p.drawText(r.x(), r.y(), r.width(), h,
                       ::Qt::AlignCenter, str);

            if (fp != 0)
                draw_text(r.x(), r.y(), r.width(), h,
                          ::Qt::AlignCenter, str, p.font(), fp);
        }

        r.setTop(r.top() + h);
        p.drawLine(r.topLeft(), r.topRight());

        if (fp) {
            fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                    " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                    r.left(), r.top(), r.right(), r.top());
            fputs("</g>\n", fp);
        }
    }

    p.setBackgroundColor(bckgrnd);

    if (selected())
        show_mark(p, rect());
}

UmlCode ActivityPartitionCanvas::type() const
{
    return UmlActivityPartition;
}

void ActivityPartitionCanvas::delete_available(BooL & in_model, BooL & out_model) const
{
    out_model |= TRUE;
    in_model |= browser_node->is_writable();
}

bool ActivityPartitionCanvas::alignable() const
{
    return TRUE;
}

bool ActivityPartitionCanvas::copyable() const
{
    return selected();
}

void ActivityPartitionCanvas::open()
{
    browser_node->open(FALSE);
}

void ActivityPartitionCanvas::menu(const QPoint &)
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
    m.insertItem((horiz) ? TR("draw vertically") : TR("draw horizontally"), 10);
    m.insertSeparator();
    m.insertItem(TR("Edit drawing settings"), 2);
    m.insertSeparator();
    m.insertItem(TR("Edit activity partition"), 3);
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

    if (Tool::menu_insert(&toolm, UmlActivityPartition, 20))
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
        ((BrowserActivityPartition *) browser_node)
        ->set_associated_diagram((BrowserActivityDiagram *)
                                 the_canvas()->browser_diagram());
        return;

    case 7:
        //remove from diagram
        delete_it();
        break;

    case 8:
        //delete from model
        browser_node->delete_it();	// will delete the canvas
        break;

    case 9:
        ((BrowserActivityPartition *) browser_node)
        ->set_associated_diagram(0);
        return;

    case 10:
        turn(-1000, -1000);
        break;

    default:
        if (index >= 20)
            ToolCom::run(Tool::command(index - 20), browser_node);

        return;
    }

    package_modified();
}

void ActivityPartitionCanvas::turn(int cx100, int cy100)
{
    horiz ^= TRUE;
    setSize(height(), width());

    if (cx100 == -1000) {
        // first turn partition
        cx100 = center_x_scale100;
        cy100 = center_y_scale100;
    }
    else {
        int dx = center_x_scale100 - cx100;
        int dy = center_y_scale100 - cy100;

        if (horiz) {
            center_x_scale100 = cx100 + dy;
            center_y_scale100 = cy100 - dx;
        }
        else {
            center_x_scale100 = cx100 - dy;
            center_y_scale100 = cy100 + dx;
        }
    }

    recenter();
    DiagramCanvas::resize(width(), height());

    Q3CanvasItemList all = the_canvas()->allItems();
    Q3CanvasItemList::Iterator cit;

    for (cit = all.begin(); cit != all.end(); ++cit) {
        if ((*cit)->visible()) {
            ActivityPartitionCanvas * p = dynamic_cast<ActivityPartitionCanvas *>(*cit);

            if ((p != 0) &&
                (p->get_bn()->parent() == browser_node) &&
                (p->horiz != horiz)) {
                p->turn(cx100, cy100);
            }
        }
    }
}

void ActivityPartitionCanvas::apply_shortcut(QString s)
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

void ActivityPartitionCanvas::edit_drawing_settings()
{
    for (;;) {
        ColorSpecVector co(1);

        co[0].set(TR("activity partition color"), &itscolor);

        SettingsDialog dialog(0, &co, FALSE);

        dialog.raise();

        if (dialog.exec() == QDialog::Accepted)
            modified();	// call package_modified()

        if (!dialog.redo())
            break;
    }
}

bool ActivityPartitionCanvas::has_drawing_settings() const
{
    return TRUE;
}

void ActivityPartitionCanvas::edit_drawing_settings(QList<DiagramItem *> & l)
{
    for (;;) {
        ColorSpecVector co(1);
        UmlColor itscolor;

        co[0].set(TR("activity partition color"), &itscolor);

        SettingsDialog dialog(0, &co, FALSE, TRUE);

        dialog.raise();

        if ((dialog.exec() == QDialog::Accepted) && !co[0].name.isEmpty()) {
            foreach (DiagramItem *item, l) {
                ActivityPartitionCanvas *canvas = (ActivityPartitionCanvas *)item;
                canvas->itscolor = itscolor;
                canvas->modified();
            }
        }

        if (!dialog.redo())
            break;
    }
}

void ActivityPartitionCanvas::clone_drawing_settings(const DiagramItem *src)
{
    const ActivityPartitionCanvas * x = (const ActivityPartitionCanvas *) src;
    itscolor = x->itscolor;
    modified();
}

QString ActivityPartitionCanvas::may_start(UmlCode & l) const
{
    return (l == UmlAnchor) ? QString() : TR("illegal");
}

QString ActivityPartitionCanvas::may_connect(UmlCode & l, const DiagramItem * dest) const
{
    return (l == UmlAnchor) ? dest->may_start(l) : TR("illegal");
}

void ActivityPartitionCanvas::connexion(UmlCode action, DiagramItem * dest,
                                        const QPoint &, const QPoint &)
{
    ArrowCanvas * a =
        new ArrowCanvas(the_canvas(), this, dest, action, 0, FALSE, -1.0, -1.0);

    a->show();
    the_canvas()->select(a);
}

void ActivityPartitionCanvas::save(QTextStream & st, bool ref, QString & warning) const
{
    if (ref) {
        st << "activitypartitioncanvas_ref " << get_ident() << " // "
           << browser_node->full_name();
    }
    else {
        nl_indent(st);
        st << "activitypartitioncanvas " << get_ident() << " ";
        browser_node->save(st, TRUE, warning);
        indent(+1);

        if (itscolor != UmlDefaultColor) {
            nl_indent(st);
            st << "color " << stringify(itscolor);

            if (horiz)
                st << " horiz ";
        }
        else if (horiz) {
            nl_indent(st);
            st << "horiz ";
        }

        save_stereotype_property(st, warning);
        nl_indent(st);
        save_xyzwh(st, this, "xyzwh");

        indent(-1);
        nl_indent(st);
        st << "end";
    }
}

ActivityPartitionCanvas *
ActivityPartitionCanvas::read(char *& st, UmlCanvas * canvas,
                              char * k)
{
    if (!strcmp(k, "activitypartitioncanvas_ref"))
        return ((ActivityPartitionCanvas *) dict_get(read_id(st), "activitypartitioncanvas", canvas));
    else if (!strcmp(k, "activitypartitioncanvas")) {
        int id = read_id(st);
        BrowserActivityPartition * br =
            BrowserActivityPartition::read_ref(st);
        ActivityPartitionCanvas * result =
            new ActivityPartitionCanvas(canvas, id);

        result->browser_node = br;
        connect(br->get_data(), SIGNAL(changed()), result, SLOT(modified()));
        connect(br->get_data(), SIGNAL(deleted()), result, SLOT(deleted()));

        k = read_keyword(st);

        read_color(st, "color", result->itscolor, k);	// updates k

        if (! strcmp(k, "horiz")) {
            result->horiz = TRUE;
            k = read_keyword(st);
        }

        result->read_stereotype_property(st, k);	// updates k

        if (!strcmp(k, "xyzwh"))
            read_xyzwh(st, result);
        else
            wrong_keyword(k, "xyzwh");

        result->check_size();
        result->width_scale100 = result->width();
        result->height_scale100 = result->height();
        result->set_center100();
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

void ActivityPartitionCanvas::history_save(QBuffer & b) const
{
    DiagramCanvas::history_save(b);
    ::save(width_scale100, b);
    ::save(height_scale100, b);
    ::save(width(), b);
    ::save(height(), b);
    ::save(horiz, b);
}

void ActivityPartitionCanvas::history_load(QBuffer & b)
{
    DiagramCanvas::history_load(b);
    ::load(width_scale100, b);
    ::load(height_scale100, b);

    int w, h;

    ::load(w, b);
    ::load(h, b);
    Q3CanvasRectangle::setSize(w, h);

    ::load(h, b);
    horiz = (h != 0);

    ActivityPartitionData * data =
        (ActivityPartitionData *) browser_node->get_data();

    connect(data, SIGNAL(changed()), this, SLOT(modified()));
    connect(data, SIGNAL(deleted()), this, SLOT(deleted()));
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));

    if (data->get_represents() != 0) {
        BasicData * d = data->get_represents()->get_data();

        connect(d, SIGNAL(changed()), this, SLOT(modified()));
        connect(d, SIGNAL(deleted()), this, SLOT(modified()));
    }
}

void ActivityPartitionCanvas::history_hide()
{
    Q3CanvasItem::setVisible(FALSE);

    disconnect(DrawingSettings::instance(), SIGNAL(changed()),
               this, SLOT(modified()));

    ActivityPartitionData * data =
        (ActivityPartitionData *) browser_node->get_data();

    disconnect(data, 0, this, 0);

    if (data->get_represents() != 0)
        disconnect(data->get_represents()->get_data(), 0, this, 0);
}
