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
#include <Q3ValueList>

#include "ActivityCanvas.h"
#include "ActivityData.h"
#include "BrowserActivity.h"
#include "BrowserActivityDiagram.h"
#include "ArrowCanvas.h"
#include "FlowCanvas.h"
#include "SimpleRelationCanvas.h"
#include "BrowserParameter.h"
#include "ParameterCanvas.h"
#include "InfoCanvas.h"
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

ActivityCanvas::ActivityCanvas(BrowserNode * bn, UmlCanvas * canvas,
                               int x, int y)
    : ActivityContainerCanvas(0, canvas, x, y, ACTIVITY_CANVAS_MIN_SIZE * 10,
                              ACTIVITY_CANVAS_MIN_SIZE * 10, 0)
{
    browser_node = bn;
    itscolor = UmlDefaultColor;
    constraint = 0;

    compute_size();
    check_params();
    check_constraint();
    check_stereotypeproperties();

    connect(bn->get_data(), SIGNAL(changed()), this, SLOT(modified()));
    connect(bn->get_data(), SIGNAL(deleted()), this, SLOT(deleted()));
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
}

ActivityCanvas::ActivityCanvas(UmlCanvas * canvas, int id)
    : ActivityContainerCanvas(canvas, id)
{
    // for read operation
    browser_node = 0;
    constraint = 0;
    itscolor = UmlDefaultColor;
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
}

ActivityCanvas::~ActivityCanvas()
{
}

void ActivityCanvas::delete_it()
{
    disconnect(DrawingSettings::instance(), SIGNAL(changed()),
               this, SLOT(modified()));
    disconnect(browser_node->get_data(), 0, this, 0);

    DiagramCanvas::delete_it();

    Q3ValueList<ParameterCanvas *>::Iterator iter;

    for (iter = params.begin();  iter != params.end(); ++iter)
        // don't empty params to manage undo
        (*iter)->delete_it();

    if (constraint != 0)
        constraint->delete_it();
}

void ActivityCanvas::deleted()
{
    delete_it();
    canvas()->update();
}

void ActivityCanvas::deleted(ParameterCanvas * pa)
{
    params.remove(pa);
}

void ActivityCanvas::remove(bool from_model)
{
    if (! from_model)
        delete_it();
    else
        browser_node->delete_it();	// will remove canvas
}

void ActivityCanvas::compute_size()
{
    double zoom = the_canvas()->zoom();
    int margin = (int)(24 * zoom);
    QFontMetrics fm(the_canvas()->get_font(UmlNormalBoldFont));
    const ActivityData * data = (ActivityData *) browser_node->get_data();
    ActivityDrawingSettings st = settings;

    ((BrowserActivityDiagram *) the_canvas()->browser_diagram())->get_activitydrawingsettings(st);
    min_width = fm.width(browser_node->get_name()) + 2 * margin;

    int x0 = min_width;
    int y0 = margin;
    int addw = 0;
    int w;

    if (data->get_is_single_execution()) {
        singleexec_offset.setX(x0);
        singleexec_offset.setY(y0);
        addw = fm.width("<<singleExecution>>");
        y0 += fm.height() + 2;
    }
    else
        singleexec_offset.setX(0);

    if (data->get_is_read_only()) {
        readonly_offset.setX(x0);
        readonly_offset.setY(y0);

        if (addw == 0)
            addw = fm.width("<<readOnly>>");

        y0 += fm.height() + 2;
    }
    else
        readonly_offset.setX(0);

    show_info_note = (st.show_infonote == UmlYes);

    if (show_info_note) {
        post_width = fm.width("<<postcondition>>##");

        pre = data->get_precond(st.drawing_language);

        if (! pre.isEmpty()) {
            pre = toUnicode(pre);
            pre_offset.setX(x0);
            pre_offset.setY(y0);

            QSize sz = fm.size(0, pre);

            w = post_width + sz.width();

            if (w > addw)
                addw = w;

            y0 += sz.height() + 2;
        }

        post = data->get_postcond(st.drawing_language);

        if (! post.isEmpty()) {
            post = toUnicode(post);
            post_offset.setX(x0);
            post_offset.setY(y0);

            QSize sz = fm.size(0, post);

            w = post_width + sz.width();

            if (w > addw)
                addw = w;

            y0 += sz.height() + 2;
        }
    }
    else
        pre = post = "";

    if (addw != 0)
        min_width += addw + margin;

    min_height = y0 + margin + fm.height();

    used_color = (itscolor == UmlDefaultColor)
                 ? the_canvas()->browser_diagram()->get_color(UmlActivity)
                 : itscolor;

    DiagramCanvas::resize((width() > min_width) ? width() : min_width,
                          (height() > min_height) ? height() : min_height);

    if (! the_canvas()->do_zoom()) {
        // update parameters position
        Q3ValueList<ParameterCanvas *>::Iterator iter;

        for (iter = params.begin(); iter != params.end(); ++iter)
            (*iter)->check_position();
    }
}

void ActivityCanvas::moveBy(double dx, double dy)
{
    DiagramCanvas::moveBy(dx, dy);

    // update parameters position
    Q3ValueList<ParameterCanvas *>::Iterator iter;

    for (iter = params.begin(); iter != params.end(); ++iter)
        (*iter)->do_moveBy(dx, dy);

    if ((constraint != 0) && !constraint->selected())
        constraint->moveBy(dx, dy);
}

void ActivityCanvas::change_scale()
{
    Q3CanvasRectangle::setVisible(FALSE);
    double scale = the_canvas()->zoom();

    setSize((int)(width_scale100 * scale), (int)(height_scale100 * scale));
    compute_size();
    recenter();
    Q3CanvasRectangle::setVisible(TRUE);

    // update parameters position
    Q3ValueList<ParameterCanvas *>::Iterator iter;

    for (iter = params.begin(); iter != params.end(); ++iter)
        (*iter)->do_change_scale();
}

void ActivityCanvas::modified()
{
    hide();
    hide_lines();
    compute_size();
    show();
    update_show_lines();
    check_params();
    check_stereotypeproperties();
    check_constraint();
    canvas()->update();
    force_sub_inside(FALSE);
    package_modified();
}

aCorner ActivityCanvas::on_resize_point(const QPoint & p)
{
    return ::on_resize_point(p, rect());
}

void ActivityCanvas::resize(aCorner c, int dx, int dy, QPoint & o)
{
    DiagramCanvas::resize(c, dx, dy, o, min_width, min_height, TRUE);

    force_sub_inside(FALSE);
}

void ActivityCanvas::resize(const QSize & sz, bool w, bool h)
{
    if (DiagramCanvas::resize(sz, w, h, min_width, min_height, TRUE))
        force_sub_inside(FALSE);
}

bool ActivityCanvas::move_with_its_package() const
{
    return TRUE;
}

void ActivityCanvas::set_z(double z)
{
    setZ(z);

    z += 1;

    Q3ValueList<ParameterCanvas *>::Iterator iter;

    for (iter = params.begin(); iter != params.end(); ++iter)
        (*iter)->set_z(z);
}

void ActivityCanvas::force_sub_inside(bool resize_it)
{
    // update sub nodes position to be inside of the activity
    // except the parameters whose are in the border
    // and the diagram icon
    // or resize it to contains sub elts if resize_it
    Q3CanvasItemList all = canvas()->allItems();
    Q3CanvasItemList::Iterator cit;
    BooL need_sub_upper = FALSE;

    if (resize_it) {
        resize_to_contain(all, need_sub_upper);

        for (cit = all.begin(); cit != all.end(); ++cit) {
            if ((*cit)->visible()/* && !(*cit)->selected()*/) {
                DiagramItem * di = QCanvasItemToDiagramItem(*cit);

                if ((di != 0) &&
                    (di->type() == UmlParameter) &&
                    (di->get_bn() != 0) &&
                    (((BrowserNode *) di->get_bn())->parent() == browser_node)) {
                    ((ParameterCanvas *) di)->check_position();
                }
            }
        }
    }
    else
        ActivityContainerCanvas::force_sub_inside(all, need_sub_upper);

    if (need_sub_upper)
        force_sub_upper(all);
}

void ActivityCanvas::check_params()
{
    // add missing params
    const Q3ValueList<BrowserParameter *> brparams =
        ((BrowserActivity *) browser_node)->get_params();
    Q3ValueList<BrowserParameter *>::ConstIterator iter;
    int dy = (int)(the_canvas()->zoom() * PARAMETER_CANVAS_MIN_HEIGHT);
    int rank;

    for (iter = brparams.begin(), rank = 0; iter != brparams.end(); ++iter, rank += 1) {
        Q3ValueList<ParameterCanvas *>::ConstIterator itershown;

        for (itershown = params.begin(); itershown != params.end(); ++itershown)
            if ((*itershown)->get_bn() == *iter)
                break;

        if (itershown == params.end()) {
            // add it
            int px;
            int py;

            switch (rank % 8) {
            case 0:
                px = (int) x();
                py = (int) y() + dy;
                break;

            case 1:
                px = (int) x();
                py = center().y();
                break;

            case 2:
                px = (int) x();
                py = (int) rect().bottom() - dy - dy;
                break;

            case 3:
                px = center().x();
                py = (int) y() + dy;
                break;

            case 4:
                px = rect().right();
                py = center().y();
                break;

            case 5:
                px = rect().right();
                py = (int) rect().bottom() - dy - dy;
                break;

            case 6:
                px = rect().right();
                py = (int) y() + dy;
                break;

            default:
                px = center().x();
                py = center().y() + dy;
                break;
            }

            ParameterCanvas * pc =
                new ParameterCanvas(*iter, the_canvas(), px, py, 0, this);

            params.append(pc);
            pc->show();
        }
    }
}

void ActivityCanvas::check_constraint()
{
    // compute_size() must be called before
    if (show_info_note) {
        ActivityData * data = (ActivityData *) browser_node->get_data();
        int margin = (int)(the_canvas()->zoom() * 15);
        QString s = data->get_constraint();

        if (s.isEmpty()) {
            if (constraint != 0) {
                constraint->delete_it();
                constraint = 0;
            }
        }
        else {
            if (constraint == 0) {
                constraint = new InfoCanvas(the_canvas(), this, s);
                constraint->upper();
                constraint->move(x() + width() + margin, y());
                constraint->show();
                (new ArrowCanvas(the_canvas(), this, constraint, UmlAnchor, 0, FALSE, -1.0, -1.0))
                ->show();
            }
            else
                constraint->set(s);
        }
    }
    else {
        if (constraint != 0) {
            constraint->delete_it();
            constraint = 0;
        }
    }
}
void ActivityCanvas::draw(QPainter & p)
{
    if (! visible()) return;

    p.setRenderHint(QPainter::Antialiasing, true);
    QRect r = rect();
    QBrush brsh = p.brush();
    QColor bckgrnd = p.backgroundColor();
    int margin = (int)(24 * the_canvas()->zoom());
    FILE * fp = svg();

    if (fp != 0)
        fputs("<g>\n", fp);

    p.setBackgroundMode((used_color == UmlTransparent)
                        ? ::Qt::TransparentMode
                        : ::Qt::OpaqueMode);

    QColor co = color(used_color);

    p.setBackgroundColor(co);

    if (used_color != UmlTransparent)
        p.setBrush(co);

    p.setPen(::Qt::SolidLine);
    p.drawRoundRect(r, 8, 8);

    if (fp != 0) {
        if (used_color != UmlTransparent)
            fprintf(fp, "\t<rect fill=\"%s\" stroke=\"black\" stroke-opacity=\"1\""
                    " x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" rx=\"10\" />\n",
                    svg_color(used_color),
                    r.left(), r.top(), r.width() - 1, r.height() - 1);
        else
            fprintf(fp, "\t<rect fill=\"none\" stroke=\"black\" stroke-opacity=\"1\""
                    " x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" rx=\"10\" />\n",
                    r.left(), r.top(), r.width() - 1, r.height() - 1);
    }

    p.setFont(the_canvas()->get_font(UmlNormalBoldFont));
    p.drawText(r.x() + margin, r.y() + margin,
               r.width(), r.height(), ::Qt::AlignLeft,
               browser_node->get_name());

    if (fp != 0)
        draw_text(r.x() + margin, r.y() + margin,
                  r.width(), r.height(), ::Qt::AlignLeft,
                  browser_node->get_name(), p.font(), fp);

    p.setFont(the_canvas()->get_font(UmlNormalFont));

    if (singleexec_offset.x() != 0) {
        p.drawText(r.x() + singleexec_offset.x(),
                   r.y() + singleexec_offset.y(),
                   r.width(), r.height(), ::Qt::AlignLeft,
                   "<<singleExecution>>");

        if (fp != 0)
            draw_text(r.x() + singleexec_offset.x(),
                      r.y() + singleexec_offset.y(),
                      r.width(), r.height(), ::Qt::AlignLeft,
                      "<<singleExecution>>", p.font(), fp);
    }

    if (readonly_offset.x() != 0) {
        p.drawText(r.x() + readonly_offset.x(),
                   r.y() + readonly_offset.y(),
                   r.width(), r.height(), ::Qt::AlignLeft,
                   "<<readOnly>>");

        if (fp != 0)
            draw_text(r.x() + readonly_offset.x(),
                      r.y() + readonly_offset.y(),
                      r.width(), r.height(), ::Qt::AlignLeft,
                      "<<readOnly>>", p.font(), fp);
    }

    if (! pre.isEmpty()) {
        p.drawText(r.x() + pre_offset.x(),
                   r.y() + pre_offset.y(),
                   r.width(), r.height(), ::Qt::AlignLeft,
                   "<<precondition>>");
        p.drawText(r.x() + pre_offset.x() + post_width,
                   r.y() + pre_offset.y(),
                   r.width(), r.height(),
                   ::Qt::AlignLeft, pre);

        if (fp != 0) {
            draw_text(r.x() + pre_offset.x(),
                      r.y() + pre_offset.y(),
                      r.width(), r.height(), ::Qt::AlignLeft,
                      "<<precondition>>", p.font(), fp);
            draw_text(r.x() + pre_offset.x() + post_width,
                      r.y() + pre_offset.y(),
                      r.width(), r.height(),
                      ::Qt::AlignLeft, pre, p.font(), fp);
        }
    }

    if (! post.isEmpty()) {
        p.drawText(r.x() + post_offset.x(),
                   r.y() + post_offset.y(),
                   r.width(), r.height(), ::Qt::AlignLeft,
                   "<<postcondition>>");
        p.drawText(r.x() + post_offset.x() + post_width,
                   r.y() + post_offset.y(),
                   r.width(), r.height(),
                   ::Qt::AlignLeft, post);

        if (fp != 0) {
            draw_text(r.x() + post_offset.x(),
                      r.y() + post_offset.y(),
                      r.width(), r.height(), ::Qt::AlignLeft,
                      "<<postcondition>>", p.font(), fp);
            draw_text(r.x() + post_offset.x() + post_width,
                      r.y() + post_offset.y(),
                      r.width(), r.height(),
                      ::Qt::AlignLeft, post, p.font(), fp);
        }
    }

    if (fp != 0)
        fputs("</g>\n", fp);

    p.setBackgroundColor(bckgrnd);
    p.setBrush(brsh);

    if (selected())
        show_mark(p, r);
}

UmlCode ActivityCanvas::type() const
{
    return UmlActivity;
}

void ActivityCanvas::delete_available(BooL & in_model, BooL & out_model) const
{
    out_model |= TRUE;
    in_model |= browser_node->is_writable();
}

bool ActivityCanvas::alignable() const
{
    return TRUE;
}

bool ActivityCanvas::copyable() const
{
    return selected();
}

void ActivityCanvas::open()
{
    browser_node->open(FALSE);
}

void ActivityCanvas::menu(const QPoint &)
{
    Q3PopupMenu m(0);
    Q3PopupMenu toolm(0);
    int index;

    MenuFactory::createTitle(m, browser_node->get_data()->definition(FALSE, TRUE));
    m.insertSeparator();

    if (browser_node->is_writable()) {
        m.insertItem(TR("Add parameter"), 9);
        m.insertSeparator();
    }

    m.insertItem(TR("Upper"), 0);
    m.insertItem(TR("Lower"), 1);
    m.insertItem(TR("Go up"), 13);
    m.insertItem(TR("Go down"), 14);
    m.insertSeparator();
    m.insertItem(TR("Edit drawing settings"), 2);
    m.insertSeparator();
    m.insertItem(TR("Edit activity"), 3);
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
            m.insertItem(TR("Remove diagram association"), 10);
    }

    m.insertSeparator();
    m.insertItem(TR("Remove from diagram"), 7);

    if (browser_node->is_writable())
        m.insertItem(TR("Delete from model"), 8);

    m.insertSeparator();

    if (Tool::menu_insert(&toolm, UmlActivity, 20))
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
        ((BrowserActivity *) browser_node)
        ->set_associated_diagram((BrowserActivityDiagram *)
                                 the_canvas()->browser_diagram());
        return;

    case 10:
        ((BrowserActivity *) browser_node)
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

    case 9:
        ((BrowserActivity *) browser_node)->add_parameter(0);
        modified();

    default:
        if (index >= 20)
            ToolCom::run(Tool::command(index - 20), browser_node);

        return;
    }

    package_modified();
}

void ActivityCanvas::apply_shortcut(QString s)
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

void ActivityCanvas::edit_drawing_settings()
{
    for (;;) {
        StateSpecVector st(2);
        ColorSpecVector co(1);

        st[0].set(TR("show conditions"), &settings.show_infonote);
        st[1].set(TR("drawing language"), &settings.drawing_language);

        co[0].set(TR("activity color"), &itscolor);

        SettingsDialog dialog(&st, &co, FALSE);

        dialog.raise();

        if (dialog.exec() == QDialog::Accepted)
            modified();	// call package_modified()

        if (!dialog.redo())
            break;
    }
}

bool ActivityCanvas::has_drawing_settings() const
{
    return TRUE;
}

void ActivityCanvas::edit_drawing_settings(QList<DiagramItem *> & l)
{
    for (;;) {
        StateSpecVector st(2);
        ColorSpecVector co(1);
        Uml3States show_infonote;
        DrawingLanguage drawing_language;
        UmlColor itscolor;

        st[0].set(TR("show conditions"), &show_infonote);
        st[1].set(TR("drawing language"), &drawing_language);

        co[0].set(TR("activity color"), &itscolor);

        SettingsDialog dialog(&st, &co, FALSE, TRUE);

        dialog.raise();

        if (dialog.exec() == QDialog::Accepted) {
            foreach (DiagramItem *item, l) {
                ActivityCanvas *canvas = (ActivityCanvas *)item;
                if (!st[0].name.isEmpty())
                    canvas->settings.show_infonote = show_infonote;

                if (!st[1].name.isEmpty())
                    canvas->settings.drawing_language = drawing_language;

                if (!co[0].name.isEmpty())
                    canvas->itscolor = itscolor;

                canvas->modified();
            }
        }

        if (!dialog.redo())
            break;
    }
}

void ActivityCanvas::clone_drawing_settings(const DiagramItem *src)
{
    const ActivityCanvas * x = (const ActivityCanvas *) src;
    settings = x->settings;
    modified();
}

bool ActivityCanvas::get_show_stereotype_properties() const
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

QString ActivityCanvas::may_start(UmlCode & l) const
{
    return (l == UmlFlow) ? TR("illegal") : QString();
}

QString ActivityCanvas::may_connect(UmlCode & l, const DiagramItem * dest) const
{
    if (l == UmlAnchor)
        return dest->may_start(l);
    else if (dest->get_bn() == 0)
        return TR("illegal");
    else
        // dependency
        return ((BrowserActivity *) browser_node)->may_connect(dest->get_bn());
}

void ActivityCanvas::connexion(UmlCode action, DiagramItem * dest,
                               const QPoint &, const QPoint &)
{
    ArrowCanvas * a;

    if (IsaSimpleRelation(action))
        a = new SimpleRelationCanvas(the_canvas(), this, dest, 0, action, 0, -1.0, -1.0);
    else
        a = new ArrowCanvas(the_canvas(), this, dest, action, 0, FALSE, -1.0, -1.0);

    a->show();
    the_canvas()->select(a);
}

void ActivityCanvas::save(QTextStream & st, bool ref, QString & warning) const
{
    if (ref) {
        st << "activitycanvas_ref " << get_ident() << " // "
           << browser_node->full_name();
    }
    else {
        nl_indent(st);
        st << "activitycanvas " << get_ident() << " ";
        browser_node->save(st, TRUE, warning);
        indent(+1);
        settings.save(st);

        if (itscolor != UmlDefaultColor) {
            nl_indent(st);
            st << "color " << stringify(itscolor);
        }

        nl_indent(st);
        save_xyzwh(st, this, "xyzwh");

        if (! params.isEmpty()) {
            nl_indent(st);
            st << "params";
            indent(+1);

            Q3ValueList<ParameterCanvas *>::ConstIterator iter;

            for (iter = params.begin(); iter != params.end(); ++iter)
                (*iter)->save(st, FALSE, warning);

            indent(-1);
            nl_indent(st);
            st << "end";
        }

        if (constraint != 0) {
            nl_indent(st);
            st << "constraint ";
            constraint->save(st, FALSE, warning);
        }

        save_stereotype_property(st, warning);

        indent(-1);
        nl_indent(st);
        st << "end";
    }
}

ActivityCanvas * ActivityCanvas::read(char *& st, UmlCanvas * canvas,
                                      char * k)
{
    if (!strcmp(k, "activitycanvas_ref"))
        return ((ActivityCanvas *) dict_get(read_id(st), "activitycanvas", canvas));
    else if (!strcmp(k, "activitycanvas")) {
        int id = read_id(st);
        BrowserActivity * br = BrowserActivity::read_ref(st);
        ActivityCanvas * result = new ActivityCanvas(canvas, id);

        result->browser_node = br;
        connect(br->get_data(), SIGNAL(changed()), result, SLOT(modified()));
        connect(br->get_data(), SIGNAL(deleted()), result, SLOT(deleted()));

        k = read_keyword(st);

        result->settings.read(st, k);	// updates k
        read_color(st, "color", result->itscolor, k);	// updates k

        if (!strcmp(k, "xyzwh"))
            read_xyzwh(st, result);
        else
            wrong_keyword(k, "xyzwh");

        result->compute_size();
        result->set_center100();
        result->show();

        k = read_keyword(st);

        if (! strcmp(k, "params")) {
            while (strcmp(k = read_keyword(st), "end") != 0)
                result->params.append(ParameterCanvas::read(st, canvas, k, result));

            k = read_keyword(st);
        }

        if (! strcmp(k, "constraint")) {
            k = read_keyword(st);
            result->constraint = InfoCanvas::read(st, canvas, k, result);
            k = read_keyword(st);
        }

        result->read_stereotype_property(st, k);	// updates k

        if (strcmp(k, "end"))
            wrong_keyword(k, "end");

        result->check_params();
        result->check_constraint();
        result->check_stereotypeproperties();

        // result->force_sub_inside() useless : may only grow

        if (canvas->paste())
            result->remove_if_already_present();

        return result;
    }
    else
        return 0;
}

void ActivityCanvas::history_save(QBuffer & b) const
{
    DiagramCanvas::history_save(b);
    ::save(min_width, b);
    ::save(min_height, b);
    ::save(width_scale100, b);
    ::save(height_scale100, b);
    ::save(width(), b);
    ::save(height(), b);
}

void ActivityCanvas::history_load(QBuffer & b)
{
    DiagramCanvas::history_load(b);
    ::load(min_width, b);
    ::load(min_height, b);
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

void ActivityCanvas::history_hide()
{
    Q3CanvasItem::setVisible(FALSE);

    disconnect(DrawingSettings::instance(), SIGNAL(changed()),
               this, SLOT(modified()));
    disconnect(browser_node->get_data(), 0, this, 0);
}

