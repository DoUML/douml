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
#include <QPixmap>

#include "ActivityObjectCanvas.h"
#include "BrowserActivityObject.h"
#include "ActivityContainerCanvas.h"
#include "ActivityObjectData.h"
#include "ArrowCanvas.h"
#include "FlowCanvas.h"
#include "InfoCanvas.h"
#include "SimpleRelationCanvas.h"
#include "BrowserActivityDiagram.h"
#include "BrowserClass.h"
#include "UmlCanvas.h"
#include "LabelCanvas.h"
#include "SettingsDialog.h"
#include "myio.h"
#include "ui/menufactory.h"
#include "Settings.h"
#include "UmlGlobal.h"
#include "strutil.h"
#include "ToolCom.h"
#include "Tool.h"
#include "ProfiledStereotypes.h"
#include "translate.h"

ActivityObjectCanvas::ActivityObjectCanvas(BrowserNode * bn, UmlCanvas * canvas, int x, int y)
    : DiagramCanvas(0, canvas, x, y, ACTIVITYOBJECT_MIN_SIZE,
                    ACTIVITYOBJECT_MIN_SIZE, 0)
{
    browser_node = bn;
    itscolor = UmlDefaultColor;
    write_horizontally = UmlDefaultState;
    selection = 0;

    compute_size();
    check_selection();
    check_stereotypeproperties();

    connect(bn->get_data(), SIGNAL(changed()), this, SLOT(modified()));
    connect(bn->get_data(), SIGNAL(deleted()), this, SLOT(deleted()));
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));

    if (canvas->must_draw_all_relations()) {
        draw_all_simple_relations();
        draw_all_flows();
    }
}

ActivityObjectCanvas::ActivityObjectCanvas(UmlCanvas * canvas, int id)
    : DiagramCanvas(canvas, id)
{
    // for read operation
    browser_node = 0;
    selection = 0;
    itscolor = UmlDefaultColor;
    write_horizontally = UmlDefaultState;
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
}

ActivityObjectCanvas::~ActivityObjectCanvas()
{
}

void ActivityObjectCanvas::delete_it()
{
    disconnect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
    DiagramCanvas::delete_it();

    if (selection != 0)
        selection->delete_it();
}

void ActivityObjectCanvas::deleted()
{
    delete_it();
    canvas()->update();
}

void ActivityObjectCanvas::remove(bool from_model)
{
    if (! from_model)
        delete_it();
    else
        browser_node->delete_it();	// will remove canvas
}

void ActivityObjectCanvas::compute_size()
{
    UmlCanvas * canvas = the_canvas();

    used_color = (itscolor == UmlDefaultColor)
                 ? canvas->browser_diagram()->get_color(UmlClass)
                 : itscolor;

    ActivityObjectData * data = (ActivityObjectData *) browser_node->get_data();
    QString s = browser_node->get_name();

    str = data->get_type().get_type();
    horiz = TRUE;

    if (! str.isEmpty()) {
        if (!s.isEmpty()) {
            switch (write_horizontally) {
            case UmlYes:
                horiz = TRUE;
                break;

            case UmlNo:
                horiz = FALSE;
                break;

            default:
                // right get_classinstwritehorizontally arg set by the diagram itself
                horiz = canvas->browser_diagram()->get_classinstwritehorizontally();
            }

            if (horiz)
                str = s + ":" + str;
            else
                str = s + ":\n" + str;
        }
    }
    else
        str = s;

    double zoom = canvas->zoom();
    const QPixmap * px =
        ProfiledStereotypes::diagramPixmap(data->get_stereotype(), zoom);

    if (px != 0) {
        QFontMetrics fm(the_canvas()->get_font(UmlNormalBoldFont));
        int w;
        int h;

        if (!str.isEmpty()) {
            QSize sz = fm.size(0, str);

            w = sz.width();

            if (w < px->width())
                w = px->width();

            h = px->height() + sz.height();
        }
        else {
            w = px->width();
            h = px->height();
        }

        // force odd width and height for line alignment
        DiagramCanvas::resize(w | 1, h | 1);
    }
    else {
        s = data->get_in_state();

        if (! s.isEmpty()) {
            if (str.isEmpty())
                str = "[" + s + "]";
            else
                str += "\n[" + s + "]";
        }

        if (data->get_stereotype()[0]) {
            if (str.isEmpty())
                str = QString("<<") + toUnicode(data->get_short_stereotype()) + ">>";
            else
                str = QString("<<") + toUnicode(data->get_short_stereotype()) + ">>\n" + str;
        }

        QFontMetrics fm(the_canvas()->get_font(UmlNormalFont));
        QSize sz = fm.size(0, str);
        int margin = (int)(12 * zoom);
        int wi;
        int he;

        if (used_color != UmlTransparent) {
            const int shadow = canvas->shadow();

            wi = sz.width() + margin + shadow;
            he = sz.height() + margin + shadow;
        }
        else {
            wi = sz.width() + margin;
            he = sz.height() + margin;
        }

        int min = (int)(zoom * ACTIVITYOBJECT_MIN_SIZE);

        if (wi < min)
            wi = min;

        if (he < min)
            he = min;

        // force odd width and height for line alignment
        DiagramCanvas::resize(wi | 1, he | 1);
    }

    if (data->get_multiplicity()[0] != 0) {
        s = QString("{bounds=") + data->get_multiplicity();

        if (data->get_ordering() != UmlUnordered)
            s += QString(", ordering=") + stringify(data->get_ordering()) + "}";
        else
            s += "}";
    }
    else if (data->get_ordering() != UmlUnordered)
        s = QString("{ordering=") + stringify(data->get_ordering()) + "}";
    else
        s = "";

    if (! s.isEmpty()) {
        if (label == 0) {
            label = new LabelCanvas(s, the_canvas(),
                                    (int) x(), (int)(y() + height()));
        }
        else
            label->set_name(s);
    }
    else if (label != 0) {
        label->delete_it();
        label = 0;
    }

    // set settings
    used_settings = settings;
    ((BrowserActivityDiagram *) the_canvas()->browser_diagram())
    ->get_activitydrawingsettings(used_settings);
}

bool ActivityObjectCanvas::force_inside()
{
    // if its parent is present, force inside it

    Q3CanvasItemList all = the_canvas()->allItems();
    Q3CanvasItemList::Iterator cit;
    BrowserNode * parent = (BrowserNode *) browser_node->parent();

    for (cit = all.begin(); cit != all.end(); ++cit) {
        if ((*cit)->visible()) {
            DiagramItem * di = QCanvasItemToDiagramItem(*cit);

            if ((di != 0) &&
                IsaActivityContainer(di->type()) &&
                (((ActivityContainerCanvas *) di)->get_bn() == parent)) {
                ((ActivityContainerCanvas *) di)->force_inside(this, this);
                return TRUE;
            }
        }
    }

    return FALSE;
}

void ActivityObjectCanvas::check_selection()
{
    // compute_size must be called before
    if (used_settings.show_infonote == UmlYes) {
        ActivityObjectData * data = (ActivityObjectData *) browser_node->get_data();
        int margin = (int)(the_canvas()->zoom() * 15);
        QString s = data->get_selection(used_settings.drawing_language);

        if (s.isEmpty()) {
            if (selection != 0) {
                selection->delete_it();
                selection = 0;
            }
        }
        else {
            s = "<<selection>>\n" + s;

            if (selection == 0) {
                selection = new InfoCanvas(the_canvas(), this, s);
                selection->upper();

                int cdx = (int) x() - margin;
                int cdy = (int) y() - selection->height() - margin;

                selection->move((cdx < 0) ? 0 : cdx, (cdy < 0) ? 0 : cdy);
                selection->show();
                (new ArrowCanvas(the_canvas(), this, selection, UmlAnchor,
                                 0, FALSE, -1.0, -1.0))
                ->show();
            }
            else
                selection->set(s);
        }
    }
    else if (selection != 0) {
        selection->delete_it();
        selection = 0;
    }
}

void ActivityObjectCanvas::modified()
{
    hide();
    hide_lines();
    compute_size();
    show();
    update_show_lines();
    check_selection();
    check_stereotypeproperties();
    force_self_rel_visible();

    if (the_canvas()->must_draw_all_relations()) {
        draw_all_simple_relations();
        draw_all_flows();
    }

    canvas()->update();
    package_modified();
}

void ActivityObjectCanvas::post_loaded()
{
    force_self_rel_visible();

    if (the_canvas()->must_draw_all_relations()) {
        draw_all_simple_relations();
        draw_all_flows();
    }
}

void ActivityObjectCanvas::moveBy(double dx, double dy)
{
    DiagramCanvas::moveBy(dx, dy);

    // update selection
    if ((selection != 0) && !selection->selected())
        selection->moveBy(dx, dy);
}

void ActivityObjectCanvas::change_scale()
{
    Q3CanvasRectangle::setVisible(FALSE);
    compute_size();
    recenter();
    Q3CanvasRectangle::setVisible(TRUE);
}

void ActivityObjectCanvas::draw(QPainter & p)
{
    if (visible()) {
        QRect r = rect();
        FILE * fp = svg();
        p.setRenderHint(QPainter::Antialiasing, true);

        if (fp != 0)
            fputs("<g>\n", fp);

        QColor bckgrnd = p.backgroundColor();
        const QPixmap * px =
            ProfiledStereotypes::diagramPixmap(browser_node->get_data()->get_stereotype(), the_canvas()->zoom());

        if (px != 0) {
            p.setBackgroundMode(::Qt::TransparentMode);

            int lft = (px->width() < width()) ? r.x() + (width() - px->width()) / 2 : r.x();

            p.drawPixmap(lft, r.y(), *px);

            if (fp != 0)
                // pixmap not really exported in SVG
                fprintf(fp, "\t<rect fill=\"%s\" stroke=\"black\" stroke-width=\"1\" stroke-opacity=\"1\""
                        " x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" />\n",
                        svg_color(UmlBlack), lft, r.y(), px->width() - 1, px->height() - 1);

            r.setTop(r.top() + px->height());
            p.setFont(the_canvas()->get_font(UmlNormalBoldFont));
        }
        else {
            QFontMetrics fm(the_canvas()->get_font(UmlNormalFont));
            QColor co = color(used_color);

            if (used_color != UmlTransparent) {
                p.setBackgroundMode(::Qt::TransparentMode);

                const int shadow = the_canvas()->shadow();

                if (shadow != 0) {
                    r.setRight(r.right() - shadow);
                    r.setBottom(r.bottom() - shadow);

                    p.fillRect(r.right(), r.top() + shadow,
                               shadow, r.height() - 1,
                               ::Qt::darkGray);
                    p.fillRect(r.left() + shadow, r.bottom(),
                               r.width() - 1, shadow,
                               ::Qt::darkGray);

                    if (fp != 0) {
                        fprintf(fp, "\t<rect fill=\"#%06x\" stroke=\"none\" stroke-opacity=\"1\""
                                " x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" />\n",
                                QColor(::Qt::darkGray).rgb() & 0xffffff,
                                r.right(), r.top() + shadow, shadow - 1, r.height() - 1 - 1);

                        fprintf(fp, "\t<rect fill=\"#%06x\" stroke=\"none\" stroke-opacity=\"1\""
                                " x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" />\n",
                                QColor(::Qt::darkGray).rgb() & 0xffffff,
                                r.left() + shadow, r.bottom(), r.width() - 1 - 1, shadow - 1);
                    }
                }
            }
            else
                p.setBackgroundMode(::Qt::OpaqueMode);

            p.setBackgroundColor(co);

            if (used_color != UmlTransparent) {
                p.fillRect(r, co);

                if (fp != 0)
                    fprintf(fp, "\t<rect fill=\"%s\" stroke=\"black\" stroke-width=\"1\" stroke-opacity=\"1\""
                            " x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" />\n",
                            svg_color(used_color),
                            r.x(), r.y(), r.width() - 1, r.height() - 1);
            }
            else if (fp != 0)
                fprintf(fp, "\t<rect fill=\"none\" stroke=\"black\" stroke-width=\"1\" stroke-opacity=\"1\""
                        " x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" />\n",
                        r.x(), r.y(), r.width() - 1, r.height() - 1);

            p.drawRect(r);

            p.setFont(the_canvas()->get_font(UmlNormalFont));
        }

        if (! str.isEmpty()) {
            p.drawText(r.x(), r.y(), r.width(), r.height(), ::Qt::AlignCenter, str);

            if (fp != 0) {
                draw_text(r.x(), r.y(), r.width(), r.height(),
                          ::Qt::AlignCenter, str, p.font(), fp);
                fputs("</g>\n", fp);
            }
        }
        else if (fp != 0)
            fputs("</g>\n", fp);

        p.setBackgroundColor(bckgrnd);
        p.setFont(the_canvas()->get_font(UmlNormalFont));

        if (selected())
            show_mark(p, rect());
    }
}

UmlCode ActivityObjectCanvas::type() const
{
    return UmlActivityObject;
}

void ActivityObjectCanvas::open()
{
    browser_node->open(FALSE);
}

void ActivityObjectCanvas::menu(const QPoint &)
{
    BrowserClass * cl =
        ((ActivityObjectData *) browser_node->get_data())->get_type().type;

    Q3PopupMenu m(0);
    Q3PopupMenu toolm(0);

    MenuFactory::createTitle(m, browser_node->get_data()->definition(FALSE, TRUE));
    m.insertSeparator();
    m.insertItem(TR("Upper"), 0);
    m.insertItem(TR("Lower"), 1);
    m.insertItem(TR("Go up"), 13);
    m.insertItem(TR("Go down"), 14);
    m.insertSeparator();
    m.insertItem(TR("Edit drawing settings"), 2);
    m.insertSeparator();
    m.insertItem(TR("Edit activity object"), 3);
    m.insertSeparator();
    m.insertItem(TR("Select in browser"), 4);

    if (cl != 0)
        m.insertItem(TR("Select class in browser"), 9);

    if (linked())
        m.insertItem(TR("Select linked items"), 5);

    m.insertSeparator();

    if (browser_node->is_writable()) {
        if (browser_node->get_associated() !=
            (BrowserNode *) the_canvas()->browser_diagram())
            m.insertItem(TR("Set associated diagram"), 6);

        if (browser_node->get_associated())
            m.insertItem(TR("Remove diagram association"), 10);
    }

    m.insertSeparator();
    m.insertItem(TR("Remove from diagram"), 7);

    if (browser_node->is_writable())
        m.insertItem(TR("Delete from model"), 8);

    m.insertSeparator();

    if (Tool::menu_insert(&toolm, UmlActivityObject, 20))
        m.insertItem(TR("Tool"), &toolm);

    int index;

    switch (index = m.exec(QCursor::pos())) {
    case 0:
        upper();
        modified();	// call package_modified
        return;

    case 1:
        lower();
        modified();	// call package_modified
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
        open();	// call package_modified
        return;

    case 4:
        browser_node->select_in_browser();
        return;

    case 5:
        the_canvas()->unselect_all();
        select_associated();
        return;

    case 6:
        ((BrowserActivityObject *) browser_node)
        ->set_associated_diagram((BrowserActivityDiagram *)
                                 the_canvas()->browser_diagram());
        return;

    case 10:
        ((BrowserActivityObject *) browser_node)
        ->set_associated_diagram(0);
        return;

    case 7:
        //remove from diagram
        delete_it();
        package_modified();

    case 8:
        //delete from model
        browser_node->delete_it();	// will delete the canvas
        break;

    case 9:
        cl->select_in_browser();
        return;

    default:
        if (index >= 20)
            ToolCom::run(Tool::command(index - 20), browser_node);

        return;
    }
}

void ActivityObjectCanvas::apply_shortcut(QString s)
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

void ActivityObjectCanvas::edit_drawing_settings()
{
    for (;;) {
        StateSpecVector st(1);
        ColorSpecVector co(1);

        st[0].set(TR("write name:type \nhorizontally"), &write_horizontally);
        settings.complete(st, TRUE);

        co[0].set(TR("activity object color"), &itscolor);

        SettingsDialog dialog(&st, &co, FALSE);

        dialog.raise();

        if (dialog.exec() != QDialog::Accepted)
            return;

        modified();	// call package_modified

        if (!dialog.redo())
            break;
    }
}

bool ActivityObjectCanvas::has_drawing_settings() const
{
    return TRUE;
}

void ActivityObjectCanvas::edit_drawing_settings(QList<DiagramItem *> & l)
{
    for (;;) {
        StateSpecVector st(1);
        ColorSpecVector co(1);
        Uml3States write_horizontally;
        UmlColor itscolor;
        ActivityDrawingSettings settings;

        st[0].set(TR("write name:type \nhorizontally"), &write_horizontally);
        settings.complete(st, TRUE);

        co[0].set(TR("activity object color"), &itscolor);

        SettingsDialog dialog(&st, &co, FALSE, TRUE);

        dialog.raise();

        if (dialog.exec() == QDialog::Accepted) {
            foreach (DiagramItem *item, l) {
                ActivityObjectCanvas *canvas = (ActivityObjectCanvas *)item;
                if (!st[0].name.isEmpty())
                    canvas->write_horizontally =
                        write_horizontally;

                if (!co[0].name.isEmpty())
                    canvas->itscolor = itscolor;

                canvas->settings.set(st, 1);
                canvas->modified();	// call package_modified()
            }
        }

        if (!dialog.redo())
            break;
    }
}

void ActivityObjectCanvas::clone_drawing_settings(const DiagramItem *src)
{
    const ActivityObjectCanvas * x = (const ActivityObjectCanvas *) src;
    write_horizontally = x->write_horizontally;
    itscolor = x->itscolor;
    settings = x->settings;
    modified();
}

bool ActivityObjectCanvas::get_show_stereotype_properties() const
{
    switch (settings.show_stereotype_properties) {
    case UmlYes:
        return TRUE;

    case UmlNo:
        return FALSE;

    default:
        return the_canvas()->browser_diagram()->get_show_stereotype_properties();
    }
}

void ActivityObjectCanvas::delete_available(BooL & in_model, BooL & out_model) const
{
    out_model |= TRUE;
    in_model |= TRUE;
}

bool ActivityObjectCanvas::alignable() const
{
    return TRUE;
}

bool ActivityObjectCanvas::copyable() const
{
    return selected();
}

QString ActivityObjectCanvas::may_start(UmlCode & l) const
{
    return (l == UmlFlow)
           ? ((BrowserActivityObject *) browser_node)->may_start()
           : QString();
}

QString ActivityObjectCanvas::may_connect(UmlCode & l, const DiagramItem * dest) const
{
    if (l == UmlAnchor)
        return dest->may_start(l);
    else if (dest->get_bn() == 0)
        return TR("illegal");
    else
        return ((BrowserActivityObject *) browser_node)->may_connect(l, dest->get_bn());
}

void ActivityObjectCanvas::connexion(UmlCode action, DiagramItem * dest, const QPoint &, const QPoint &)
{
    ArrowCanvas * a;

    if (action == UmlFlow)
        a = new FlowCanvas(the_canvas(), this, dest, 0, 0, -1.0, -1.0);
    else if (IsaSimpleRelation(action))
        a = new SimpleRelationCanvas(the_canvas(), this, dest, 0, action, 0, -1.0, -1.0);
    else
        a = new ArrowCanvas(the_canvas(), this, dest, action, 0, FALSE, -1.0, -1.0);

    a->show();
    the_canvas()->select(a);
}

bool ActivityObjectCanvas::move_with_its_package() const
{
    return TRUE;
}

void ActivityObjectCanvas::save(QTextStream & st, bool ref, QString & warning) const
{
    if (ref)
        st << "activityobjectcanvas_ref " << get_ident() << " // "
           << browser_node->full_name();
    else {
        nl_indent(st);
        st << "activityobjectcanvas " << get_ident() << " ";
        browser_node->save(st, TRUE, warning);
        indent(+1);
        settings.save(st);
        nl_indent(st);

        if (itscolor != UmlDefaultColor)
            st << "color " << stringify(itscolor) << " ";

        if (write_horizontally != UmlDefaultState)
            st << "write_horizontally " << stringify(write_horizontally) << " ";

        save_xyzwh(st, this, "xyzwh");

        if (label != 0)
            save_xy(st, label, " label_xy");

        if (selection != 0) {
            nl_indent(st);
            st << "selection ";
            selection->save(st, FALSE, warning);
        }

        save_stereotype_property(st, warning);

        indent(-1);
        nl_indent(st);
        st << "end";
    }
}

ActivityObjectCanvas *
ActivityObjectCanvas::read(char *& st, UmlCanvas * canvas, char * k)
{
    if (!strcmp(k, "activityobjectcanvas_ref"))
        return ((ActivityObjectCanvas *) dict_get(read_id(st), "activityobjectcanvas", canvas));
    else if (!strcmp(k, "activityobjectcanvas")) {
        int id = read_id(st);
        BrowserActivityObject * br = BrowserActivityObject::read_ref(st);
        ActivityObjectCanvas * result = new ActivityObjectCanvas(canvas, id);

        result->browser_node = br;
        connect(br->get_data(), SIGNAL(changed()), result, SLOT(modified()));
        connect(br->get_data(), SIGNAL(deleted()), result, SLOT(deleted()));

        k = read_keyword(st);

        result->settings.read(st, k);	// updates k
        read_color(st, "color", result->itscolor, k);	// updates k

        if (!strcmp(k, "write_horizontally")) {
            result->write_horizontally = state(read_keyword(st));
            k = read_keyword(st);
        }

        if (!strcmp(k, "xyzwh"))
            read_xyzwh(st, result);
        else
            wrong_keyword(k, "xyzwh");

        result->compute_size();
        result->set_center100();
        result->show();

        k = read_keyword(st);

        if (!strcmp(k, "label_xy")) {
            if (result->label != 0) {
                read_xy(st, result->label);
                result->label->set_center100();
            }
            else
                bypass_xy(st);

            k = read_keyword(st);
        }

        if (! strcmp(k, "selection")) {
            k = read_keyword(st);
            result->selection = InfoCanvas::read(st, canvas, k, result);
            k = read_keyword(st);
        }

        result->read_stereotype_property(st, k);	// updates k

        if (strcmp(k, "end"))
            wrong_keyword(k, "end");

        result->check_selection();
        result->check_stereotypeproperties();

        if (canvas->paste())
            result->remove_if_already_present();

        return result;
    }
    else
        return 0;
}

void ActivityObjectCanvas::history_hide()
{
    Q3CanvasItem::setVisible(FALSE);
    disconnect(browser_node->get_data(), 0, this, 0);
    disconnect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
}

void ActivityObjectCanvas::history_load(QBuffer & b)
{
    DiagramCanvas::history_load(b);
    connect(browser_node->get_data(), SIGNAL(changed()), this, SLOT(modified()));
    connect(browser_node->get_data(), SIGNAL(deleted()), this, SLOT(deleted()));
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
}
