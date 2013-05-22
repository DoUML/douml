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
#include <QPixmap>

#include "ComponentCanvas.h"
#include "ArrowJunctionCanvas.h"
#include "SimpleRelationCanvas.h"
#include "DiagramView.h"
#include "SimpleData.h"
#include "BrowserComponent.h"
#include "BrowserComponentDiagram.h"
#include "BrowserClass.h"
#include "UmlPixmap.h"
#include "UmlGlobal.h"
#include "SettingsDialog.h"
#include "myio.h"
#include "ToolCom.h"
#include "Tool.h"
#include "ui/menufactory.h"
#include "Settings.h"
#include "ClassListDialog.h"
#include "strutil.h"
#include "ProfiledStereotypes.h"
#include "translate.h"

ComponentCanvas::ComponentCanvas(BrowserNode * bn, UmlCanvas * canvas,
                                 int x, int y)
    : DiagramCanvas(0, canvas, x, y, COMPONENT_CANVAS_MIN_SIZE, 1, 0)
{
    browser_node = bn;
    itscolor = UmlDefaultColor;

    compute_size();
    check_stereotypeproperties();

    connect(bn->get_data(), SIGNAL(changed()), this, SLOT(modified()));
    connect(bn->get_data(), SIGNAL(deleted()), this, SLOT(deleted()));
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));

    if (canvas->must_draw_all_relations()) {
        draw_all_simple_relations();
    }
}

ComponentCanvas::ComponentCanvas(UmlCanvas * canvas, int id)
    : DiagramCanvas(canvas, id)
{
    // for read operation
    browser_node = 0;
    itscolor = UmlDefaultColor;
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
}

ComponentCanvas::~ComponentCanvas()
{
}

void ComponentCanvas::delete_it()
{
    disconnect(DrawingSettings::instance(), SIGNAL(changed()),
               this, SLOT(modified()));
    disconnect(browser_node->get_data(), 0, this, 0);

    MultipleDependencyIterator<BasicData> it(this);

    while (it.current()) {
        disconnect(it.current(), 0, this, 0);
        ++it;
    }

    unsubscribe_all();

    DiagramCanvas::delete_it();
}

void ComponentCanvas::deleted()
{
    delete_it();
    canvas()->update();
}

void ComponentCanvas::remove(bool from_model)
{
    if (! from_model)
        delete_it();
    else
        browser_node->delete_it();	// will remove canvas
}

void ComponentCanvas::compute_size()
{
    // does not unsubscribe & disconnect signals because compute_size may
    // be called during a signal management, and the signal connection list
    // cannot be modified in this case
    const BasicData * data = browser_node->get_data();
    double zoom = the_canvas()->zoom();
    const QPixmap * px = 0;
    QFontMetrics fm(the_canvas()->get_font(UmlNormalBoldFont));
    ComponentDrawingSettings st = settings;
    int eight = (int)(8 * zoom);

    the_canvas()->browser_diagram()->get_componentdrawingsettings(st);

    as_icon = (st.draw_component_as_icon == UmlYes);
    req_prov = (st.show_component_req_prov == UmlYes);
    rea = (st.show_component_rea == UmlYes);

    used_color = (itscolor == UmlDefaultColor)
                 ? the_canvas()->browser_diagram()->get_color(UmlComponent)
                 : itscolor;

    if (!as_icon) {
        // <<component>>/stereotype on 2*font_height with the icon on the right
        // the icon height = 2*font_height
        // the icon width = 4*height/5
        // name on font_height+4 points
        const int four = (int)(4 * zoom);
        const int sixteen = (int)(16 * zoom);

        min_height = 3 * fm.height() + four;
        min_width = fm.width(browser_node->get_name());

        int w = fm.width((data->get_stereotype()[0])
                         ? (QString("<<") + toUnicode(data->get_short_stereotype()) + ">>")
                         : QString("<<component>>"))
                + 16 * fm.height() / 5;

        if (min_width < w)
            min_width = w;

        BrowserComponent * comp = (BrowserComponent *) browser_node;
        const Q3ValueList<BrowserClass *> & pr = comp->get_provided_classes();
        const Q3ValueList<BrowserClass *> & rq = comp->get_required_classes();
        const Q3ValueList<BrowserClass *> & rz = comp->get_realizing_classes();
        int lh = fm.height() + four;	// line height

        if (req_prov && (!pr.isEmpty() || !rq.isEmpty())) {
            // a line on 8 points
            min_height += eight;

            if (!pr.isEmpty()) {
                min_height += (lh * (1 + pr.count()));
                w = fm.width("<<provided interfaces>>") + four;

                if (min_width < w)
                    min_width = w;

                for (Q3ValueList<BrowserClass *>::ConstIterator it = pr.begin();
                     it != pr.end();
                     it++) {
                    // add a left margin of 14, a right of 2
                    w = fm.width((*it)->get_name()) + sixteen;

                    if (min_width < w)
                        min_width = w;

                    if (subscribe((*it)->get_data()))
                        connect((*it)->get_data(), SIGNAL(changed()),
                                this, SLOT(modified()));
                }
            }

            if (!rq.isEmpty()) {
                min_height += (lh * (1 + rq.count()));
                w = fm.width("<<required interfaces>>") + four;

                if (min_width < w)
                    min_width = w;

                for (Q3ValueList<BrowserClass *>::ConstIterator it = rq.begin();
                     it != rq.end();
                     it++) {
                    // add a left margin of 14, a right of 2
                    w = fm.width((*it)->get_name()) + sixteen;

                    if (min_width < w)
                        min_width = w;

                    if (subscribe((*it)->get_data()))
                        connect((*it)->get_data(), SIGNAL(changed()),
                                this, SLOT(modified()));
                }
            }
        }

        if (rea && !rz.isEmpty()) {
            // a line on 8 points
            min_height += eight;

            min_height += (lh * (1 + rz.count()));
            w = fm.width("<<realizations>>") + four;

            if (min_width < w)
                min_width = w;

            for (Q3ValueList<BrowserClass *>::ConstIterator it = rz.begin();
                 it != rz.end();
                 it++) {
                // add a left margin of 12
                w = fm.width((*it)->get_name()) + sixteen;

                if (min_width < w)
                    min_width = w;

                if (subscribe((*it)->get_data()))
                    connect((*it)->get_data(), SIGNAL(changed()),
                            this, SLOT(modified()));
            }
        }

        min_width += four * 2;

        if (used_color != UmlTransparent) {
            const int shadow = the_canvas()->shadow();

            min_width += shadow;
            min_height += shadow;
        }

        // force odd width and height for line alignment
        min_width |= 1;
        min_height |= 1;

        DiagramCanvas::resize((width() > min_width) ? width() : min_width,
                              (height() > min_height) ? height() : min_height);
    }
    else if ((px = ProfiledStereotypes::diagramPixmap(data->get_stereotype(), zoom))
             != 0) {
        min_width = fm.width(browser_node->get_name());

        if (min_width < px->width())
            min_width = px->width();

        // force odd width and height for line alignment
        min_width |= 1;
        min_height = (px->height() + fm.height()) | 1;

        DiagramCanvas::resize(min_width, min_height);
    }
    else {
        min_height = fm.height() * 3;
        min_width = fm.width(browser_node->get_name());

        if (data->get_stereotype()[0]) {
            int stw = fm.width(QString("<<") + toUnicode(data->get_short_stereotype()) + ">>");

            if (min_width < stw)
                min_width = stw;
        }

        min_width += eight + (min_height * 5) / 13;

        if (min_width < ((int)(COMPONENT_CANVAS_MIN_SIZE * zoom)))
            min_width = (int)(COMPONENT_CANVAS_MIN_SIZE * zoom);

        // force odd width and height for line alignment
        min_width |= 1;
        min_height |= 1;

        DiagramCanvas::resize(min_width, min_height);
    }
}

void ComponentCanvas::change_scale()
{
    Q3CanvasRectangle::setVisible(FALSE);

    if (!as_icon) {
        double scale = the_canvas()->zoom();

        setSize((int)(width_scale100 * scale), (int)(height_scale100 * scale));
    }

    compute_size();
    recenter();
    Q3CanvasRectangle::setVisible(TRUE);
}

bool ComponentCanvas::valid(ArrowCanvas * a) const
{
    Q3ValueList<BrowserClass *> l;

    switch (a->type()) {
    case UmlRequired:
        ((BrowserComponent *) browser_node)->get_all_required_classes(l, FALSE);
        break;

    case UmlProvided:
        ((BrowserComponent *) browser_node)->get_all_provided_classes(l, FALSE);
        break;

    default:
        return TRUE;
    }

    return (l.findIndex(((ArrowJunctionCanvas *) a->get_end())->get_interface()) != -1);
}

void ComponentCanvas::modified()
{
    hide();
    hide_lines();
    compute_size();
    show();
    update_show_lines();
    force_self_rel_visible();
    check_stereotypeproperties();

    // remove required/provided arrow if needed
    foreach (ArrowCanvas *a, lines)
        if (! valid(a))
            a->delete_it(); // class removed from required/provided

    if (the_canvas()->must_draw_all_relations())
        draw_all_simple_relations();

    canvas()->update();
    package_modified();
}

void ComponentCanvas::post_loaded()
{
    force_self_rel_visible();

    if (the_canvas()->must_draw_all_relations())
        draw_all_simple_relations();
}

void ComponentCanvas::check_line(ArrowCanvas * l)
{
    if (!valid(l))
        Undefined.append(l);
}

bool ComponentCanvas::connexion(UmlCode action, const QPoint &, const QPoint & p)
{
    BrowserComponent * comp = (BrowserComponent *) browser_node;
    Q3ValueList<BrowserClass *> l;

    if (action == UmlProvided)
        comp->get_all_provided_classes(l, TRUE);
    else
        comp->get_all_required_classes(l, TRUE);

    ClassListDialog dialog(TR("Choose class"), l);

    if (dialog.exec() != QDialog::Accepted)
        return FALSE;

    Q3ValueList<BrowserClass *>::ConstIterator it = l.at(dialog.choosen());
    ArrowJunctionCanvas * aj =
        new ArrowJunctionCanvas(the_canvas(), p.x(), p.y(), *it, 0);

    aj->show();
    aj->upper();

    ArrowCanvas * a =
        new ArrowCanvas(the_canvas(), this, aj, action, 0, FALSE, -1.0, -1.0);

    a->show();
    the_canvas()->select(a);

    return TRUE;
}

void ComponentCanvas::connexion(UmlCode action, DiagramItem * dest,
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

aCorner ComponentCanvas::on_resize_point(const QPoint & p)
{
    return (as_icon)
           ? NoCorner
           : ::on_resize_point(p, rect());
}

void ComponentCanvas::resize(aCorner c, int dx, int dy, QPoint & o)
{
    if (! as_icon)
        DiagramCanvas::resize(c, dx, dy, o, min_width, min_height, TRUE);
}

void ComponentCanvas::resize(const QSize & sz, bool w, bool h)
{
    if (! as_icon)
        DiagramCanvas::resize(sz, w, h, min_width, min_height, TRUE);
}

void ComponentCanvas::prepare_for_move(bool on_resize)
{
    if (! on_resize) {
        DiagramCanvas::prepare_for_move(on_resize);

        Q3CanvasItemList l = collisions(TRUE);
        Q3CanvasItemList::ConstIterator it;
        Q3CanvasItemList::ConstIterator end = l.end();
        DiagramItem * di;
        BrowserNode * p = get_bn();

        for (it = l.begin(); it != end; ++it) {
            if ((*it)->visible() && // at least not deleted
                !(*it)->selected() &&
                ((di = QCanvasItemToDiagramItem(*it)) != 0) &&
                (di->type() == UmlComponent) &&
                (di->get_bn()->parent() == p)) {
                the_canvas()->select(*it);
                di->prepare_for_move(FALSE);
            }
        }
    }
}

bool ComponentCanvas::move_with_its_package() const
{
    return TRUE;
}

void ComponentCanvas::draw(QPainter & p)
{
    if (! visible()) return;

    p.setRenderHint(QPainter::Antialiasing, true);
    const BasicData * data = browser_node->get_data();
    QRect r = rect();
    QColor bckgrnd = p.backgroundColor();
    QColor co = color(used_color);
    const QPixmap * px = 0;
    double zoom = the_canvas()->zoom();
    FILE * fp = svg();

    if (fp != 0)
        fputs("<g>\n", fp);

    p.setBackgroundMode((used_color == UmlTransparent) ? ::Qt::TransparentMode : ::Qt::OpaqueMode);

    if (! as_icon) {
        // <<component>>/stereotype on 2*font_height with the icon on the right
        // the icon height = 2*font_height
        // the icon width = 4*height/5
        // name on font_height+4 points
        const int four = (int)(4 * zoom);
        QFontMetrics fm(the_canvas()->get_font(UmlNormalBoldFont));
        const int he = fm.height();

        p.setBackgroundColor(co);

        if (used_color != UmlTransparent) {
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

        QRect re = r;

        if (used_color != UmlTransparent)
            p.fillRect(r, co);

        if (fp != 0)
            fprintf(fp, "\t<rect fill=\"%s\" stroke=\"black\" stroke-width=\"1\" stroke-opacity=\"1\""
                    " x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" />\n",
                    svg_color(used_color),
                    r.x(), r.y(), r.width() - 1, r.height() - 1);

        p.drawRect(r);

        r.setHeight(he * 2);
        p.setFont(the_canvas()->get_font(UmlNormalFont));

        if (data->get_stereotype()[0]) {
            p.drawText(r, ::Qt::AlignCenter,
                       QString("<<") + toUnicode(data->get_short_stereotype()) + ">>");

            if (fp != 0)
                draw_text(r, ::Qt::AlignCenter,
                          QString("<<") + toUnicode(data->get_short_stereotype()) + ">>",
                          p.font(), fp);
        }
        else {
            p.drawText(r, ::Qt::AlignCenter, "<<component>>");

            if (fp != 0)
                draw_text(r, ::Qt::AlignCenter, "<<component>>",
                          p.font(), fp);
        }

        r.moveBy(0, r.height());
        r.setHeight(he + four);
        p.setFont(the_canvas()->get_font(UmlNormalBoldFont));
        p.drawText(r, ::Qt::AlignCenter, browser_node->get_name());

        if (fp != 0)
            draw_text(r, ::Qt::AlignCenter, browser_node->get_name(),
                      p.font(), fp);

        p.setFont(the_canvas()->get_font(UmlNormalFont));

        // draw icon
        r = re;
        r.setTop(r.top() + four);
        r.setHeight(2 * (he - four));
        r.setRight(r.right() - four);
        r.setLeft(r.right() - 4 * r.height() / 5);

        QRect r2 = r;
        int l = r.width() >> 2;

        r2.setLeft(r.left() + l);
        p.drawRect(r2);

        if (fp != 0)
            fprintf(fp, "\t<rect fill=\"none\" stroke=\"black\" stroke-width=\"1\" stroke-opacity=\"1\""
                    " x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" />\n",
                    r2.x(), r2.y(), r2.width() - 1, r2.height() - 1);

        r2.setLeft(r.left());
        r2.setTop(r.top() + l);
        r2.setHeight(l);
        r2.setWidth(r.width() >> 1);
        p.fillRect(r2, co);
        p.drawRect(r2);

        if (fp != 0)
            fprintf(fp, "\t<rect fill=\"%s\" stroke=\"black\" stroke-width=\"1\" stroke-opacity=\"1\""
                    " x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" />\n",
                    svg_color(used_color),
                    r2.x(), r2.y(), r2.width() - 1, r2.height() - 1);

        r2.moveBy(0, r.width() >> 1);
        p.fillRect(r2, co);
        p.drawRect(r2);

        if (fp != 0)
            fprintf(fp, "\t<rect fill=\"%s\" stroke=\"black\" stroke-width=\"1\" stroke-opacity=\"1\""
                    " x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" />\n",
                    svg_color(used_color),
                    r2.x(), r2.y(), r2.width() - 1, r2.height() - 1);

        // compartments

        BrowserComponent * comp = (BrowserComponent *) browser_node;
        const Q3ValueList<BrowserClass *> & pr = comp->get_provided_classes();
        const Q3ValueList<BrowserClass *> & rq = comp->get_required_classes();
        const Q3ValueList<BrowserClass *> & rz = comp->get_realizing_classes();
        int lh = fm.height() + four;	// line height

        r = re;
        r.setTop(r.top() + 3 * he + four + four);

        int left0 = r.left();
        int left1 = left0 + four;
        int left2 = left0 + (int)(16 * zoom);

        if (req_prov && (!pr.isEmpty() || !rq.isEmpty())) {
            p.drawLine(r.topLeft(), r.topRight());

            if (fp != 0)
                fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                        " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                        r.left(), r.top(), r.right(), r.top());

            r.setTop(r.top() + four);

            if (!pr.isEmpty()) {
                r.setLeft(left1);
                p.drawText(r, ::Qt::AlignLeft + ::Qt::AlignTop, "<<provided interfaces>>");

                if (fp != 0)
                    draw_text(r, ::Qt::AlignLeft + ::Qt::AlignTop, "<<provided interfaces>>",
                              p.font(), fp);

                r.setLeft(left2);
                r.setTop(r.top() + lh);

                for (Q3ValueList<BrowserClass *>::ConstIterator it = pr.begin();
                     it != pr.end();
                     it++) {
                    p.drawText(r, ::Qt::AlignLeft + ::Qt::AlignTop, (*it)->get_name());

                    if (fp != 0)
                        draw_text(r, ::Qt::AlignLeft + ::Qt::AlignTop, (*it)->get_name(),
                                  p.font(), fp);

                    r.setTop(r.top() + lh);
                }
            }

            if (!rq.isEmpty()) {
                r.setLeft(left1);
                p.drawText(r, ::Qt::AlignLeft + ::Qt::AlignTop, "<<required interfaces>>");

                if (fp != 0)
                    draw_text(r, ::Qt::AlignLeft + ::Qt::AlignTop, "<<required interfaces>>",
                              p.font(), fp);

                r.setLeft(left2);
                r.setTop(r.top() + lh);

                for (Q3ValueList<BrowserClass *>::ConstIterator it = rq.begin();
                     it != rq.end();
                     it++) {
                    p.drawText(r, ::Qt::AlignLeft + ::Qt::AlignTop, (*it)->get_name());

                    if (fp != 0)
                        draw_text(r, ::Qt::AlignLeft + ::Qt::AlignTop, (*it)->get_name(),
                                  p.font(), fp);

                    r.setTop(r.top() + lh);
                }
            }
        }

        if (rea && !rz.isEmpty()) {
            r.setLeft(left0);
            r.setTop(r.top() + four);
            p.drawLine(r.topLeft(), r.topRight());

            if (fp != 0)
                fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                        " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                        r.left(), r.top(), r.right(), r.top());

            r.setTop(r.top() + four);
            r.setLeft(left1);
            p.drawText(r, ::Qt::AlignLeft + ::Qt::AlignTop, "<<realizations>>");

            if (fp != 0)
                draw_text(r, ::Qt::AlignLeft + ::Qt::AlignTop, "<<realizations>>",
                          p.font(), fp);

            r.setLeft(left2);
            r.setTop(r.top() + lh);

            for (Q3ValueList<BrowserClass *>::ConstIterator it = rz.begin();
                 it != rz.end();
                 it++) {
                p.drawText(r, ::Qt::AlignLeft + ::Qt::AlignTop, (*it)->get_name());

                if (fp != 0)
                    draw_text(r, ::Qt::AlignLeft + ::Qt::AlignTop, (*it)->get_name(),
                              p.font(), fp);

                r.setTop(r.top() + lh);
            }
        }
    }
    else if ((px = ProfiledStereotypes::diagramPixmap(data->get_stereotype(), zoom))
             != 0) {
        p.setBackgroundMode(::Qt::TransparentMode);

        int lft = (px->width() < width()) ? r.x() + (width() - px->width()) / 2 : r.x();

        p.drawPixmap(lft, r.y(), *px);

        if (fp != 0)
            // pixmap not really exported in SVG
            fprintf(fp, "\t<rect fill=\"%s\" stroke=\"black\" stroke-width=\"1\" stroke-opacity=\"1\""
                    " x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" />\n",
                    svg_color(UmlBlack), lft, r.y(), px->width() - 1, px->height() - 1);

        r.moveBy(0, px->height());
        p.setFont(the_canvas()->get_font(UmlNormalBoldFont));
        p.drawText(r, ::Qt::AlignHCenter, browser_node->get_name());

        if (fp != 0)
            draw_text(r, ::Qt::AlignHCenter, browser_node->get_name(),
                      p.font(), fp);

        p.setFont(the_canvas()->get_font(UmlNormalFont));
    }
    else {
        p.setBackgroundColor(co);

        int he = r.height();

        r.setLeft(r.left() + (he * 5) / (13 * 2));

        if (used_color != UmlTransparent)
            p.fillRect(r, co);

        if (fp != 0)
            fprintf(fp, "\t<rect fill=\"%s\" stroke=\"black\" stroke-width=\"1\" stroke-opacity=\"1\""
                    " x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" />\n",
                    svg_color(used_color),
                    r.x(), r.y(), r.width() - 1, r.height() - 1);

        p.drawRect(r);

        r.setLeft(r.left() + (he * 5) / (13 * 2));

        if (data->get_stereotype()[0]) {
            r.setHeight(he / 2);
            p.setFont(the_canvas()->get_font(UmlNormalFont));
            p.drawText(r, ::Qt::AlignCenter,
                       QString("<<") + toUnicode(data->get_short_stereotype()) + ">>");

            if (fp != 0)
                draw_text(r, ::Qt::AlignCenter,
                          QString("<<") + toUnicode(data->get_short_stereotype()) + ">>",
                          p.font(), fp);

            r.moveBy(0, he / 2);
        }

        p.setFont(the_canvas()->get_font(UmlNormalBoldFont));
        p.drawText(r, ::Qt::AlignCenter, browser_node->get_name());

        if (fp != 0)
            draw_text(r, ::Qt::AlignCenter, browser_node->get_name(),
                      p.font(), fp);

        p.setFont(the_canvas()->get_font(UmlNormalFont));

        r = rect();
        r.setTop(r.top() + (he * 3) / 13);
        r.setHeight((he * 3) / 13);
        r.setWidth((he * 5) / 13);
        p.fillRect(r, co);
        p.drawRect(r);

        if (fp != 0)
            fprintf(fp, "\t<rect fill=\"%s\" stroke=\"black\" stroke-width=\"1\" stroke-opacity=\"1\""
                    " x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" />\n",
                    svg_color((used_color != UmlTransparent) ? used_color : UmlWhite),
                    r.x(), r.y(), r.width() - 1, r.height() - 1);

        r.moveBy(0, (he * 4) / 13);
        p.fillRect(r, co);
        p.drawRect(r);

        if (fp != 0)
            fprintf(fp, "\t<rect fill=\"%s\" stroke=\"black\" stroke-width=\"1\" stroke-opacity=\"1\""
                    " x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" />\n",
                    svg_color((used_color != UmlTransparent) ? used_color : UmlWhite),
                    r.x(), r.y(), r.width() - 1, r.height() - 1);
    }

    if (fp != 0)
        fputs("</g>\n", fp);

    p.setBackgroundColor(bckgrnd);

    if (selected())
        show_mark(p, rect());
}

UmlCode ComponentCanvas::type() const
{
    return UmlComponent;
}

void ComponentCanvas::delete_available(BooL & in_model, BooL & out_model) const
{
    out_model |= TRUE;
    in_model |= browser_node->is_writable();
}

bool ComponentCanvas::alignable() const
{
    return TRUE;
}

bool ComponentCanvas::copyable() const
{
    return selected();
}

void ComponentCanvas::open()
{
    browser_node->open(FALSE);
}

void ComponentCanvas::menu(const QPoint &)
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
    m.insertItem(TR("Add related elements"), 10);
    m.insertSeparator();
    m.insertItem(TR("Edit drawing settings"), 2);
    m.insertSeparator();
    m.insertItem(TR("Edit component"), 3);
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

    if (Tool::menu_insert(&toolm, UmlComponent, 20))
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

    case 2:
        edit_drawing_settings();
        return;

    case 13:
        z_up();
        modified();	// call package_modified()
        return;

    case 14:
        z_down();
        modified();	// call package_modified()
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
        ((BrowserComponent *) browser_node)
        ->set_associated_diagram((BrowserComponentDiagram *)
                                 the_canvas()->browser_diagram());
        return;

    case 9:
        ((BrowserComponent *) browser_node)
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

    case 10:
        ((UmlCanvas *) canvas())->get_view()
        ->add_related_elements(this, TR("component"), TRUE, FALSE);
        return;

    default:
        if (index >= 20)
            ToolCom::run(Tool::command(index - 20), browser_node);

        return;
    }

    package_modified();
}

void ComponentCanvas::apply_shortcut(QString s)
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
    else if (s == "Add related elements") {
        ((UmlCanvas *) canvas())->get_view()
        ->add_related_elements(this, TR("component"), TRUE, FALSE);
        return;
    }
    else {
        browser_node->apply_shortcut(s);
        return;
    }

    modified();
    package_modified();
}

void ComponentCanvas::edit_drawing_settings()
{
    for (;;) {
        StateSpecVector st(4);
        ColorSpecVector co(1);

        st[0].set(TR("drawn as icon"), &settings.draw_component_as_icon);
        st[1].set(TR("show required and provided interfaces"), &settings.show_component_req_prov);
        st[2].set(TR("show realizations"), &settings.show_component_rea);
        st[3].set(TR("show stereotype properties"), &settings.show_stereotype_properties);
        co[0].set(TR("component color"), &itscolor);

        SettingsDialog dialog(&st, &co, FALSE);

        if (dialog.exec() == QDialog::Accepted)
            modified();	// call package_modified()

        if (!dialog.redo())
            break;
    }
}

bool ComponentCanvas::has_drawing_settings() const
{
    return TRUE;
}

void ComponentCanvas::edit_drawing_settings(QList<DiagramItem *> & l)
{
    for (;;) {
        StateSpecVector st(4);
        ColorSpecVector co(1);
        Uml3States draw_component_as_icon;
        Uml3States show_component_req_prov;
        Uml3States show_component_rea;
        Uml3States show_stereotype_properties;
        UmlColor itscolor;

        st[0].set(TR("drawn as icon"), &draw_component_as_icon);
        st[1].set(TR("show required and provided interfaces"), &show_component_req_prov);
        st[2].set(TR("show realizations"), &show_component_rea);
        st[3].set(TR("show stereotype properties"), &show_stereotype_properties);
        co[0].set(TR("component color"), &itscolor);

        SettingsDialog dialog(&st, &co, FALSE, TRUE);

        if (dialog.exec() == QDialog::Accepted) {

            foreach (DiagramItem *item, l) {
                ComponentCanvas *canvas = (ComponentCanvas *)item;
                if (!st[0].name.isEmpty())
                    canvas->settings.draw_component_as_icon = draw_component_as_icon;

                if (!st[1].name.isEmpty())
                    canvas->settings.show_component_req_prov = show_component_req_prov;

                if (!st[2].name.isEmpty())
                    canvas->settings.show_component_rea = show_component_rea;

                if (!st[3].name.isEmpty())
                    canvas->settings.show_stereotype_properties = show_stereotype_properties;

                if (!co[0].name.isEmpty())
                    canvas->itscolor = itscolor;

                canvas->modified();
            }
        }

        if (!dialog.redo())
            break;
    }
}

void ComponentCanvas::clone_drawing_settings(const DiagramItem *src)
{
    const ComponentCanvas * x = (const ComponentCanvas *) src;
    settings = x->settings;
    itscolor = x->itscolor;
    modified();
}

bool ComponentCanvas::get_show_stereotype_properties() const
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

QString ComponentCanvas::may_start(UmlCode & l) const
{
    switch (l) {
    case UmlDependency:
        l = UmlDependOn;
        return (browser_node->is_writable()) ? QString() : TR("read only");

    case UmlGeneralisation:
        l = UmlInherit;
        return (browser_node->is_writable()) ? QString() : TR("read only");

    case UmlProvided: {
        Q3ValueList<BrowserClass *> ll;

        ((BrowserComponent *) browser_node)->get_all_provided_classes(ll, FALSE);
        return (!ll.isEmpty()) ? QString() : TR("no provided interfaces");
    }
    break;

    case UmlRequired: {
        Q3ValueList<BrowserClass *> ll;

        ((BrowserComponent *) browser_node)->get_all_required_classes(ll, FALSE);
        return (!ll.isEmpty()) ? QString() : TR("no required interfaces");
    }
    break;

    case UmlAnchor:
        return 0;

    default:
        return TR("illegal");
    }
}

bool ComponentCanvas::may_connect(UmlCode l) const
{
    switch (l) {
    case UmlRequired:
    case UmlProvided:
        return TRUE;

    default:
        return FALSE;
    }
}

QString ComponentCanvas::may_connect(UmlCode & l, const DiagramItem * dest) const
{
    if (l == UmlAnchor)
        return dest->may_start(l);

    switch (dest->type()) {
    case UmlComponent:
        switch (l) {
        case UmlDependOn:
            return 0;

        case UmlInherit:
            return browser_node->check_inherit(dest->get_bn());

        default:
            return TR("illegal");
        }

    case UmlArrowJunction:
        switch (l) {
        case UmlRequired: {
            Q3ValueList<BrowserClass *> ll;

            ((BrowserComponent *) browser_node)->get_all_required_classes(ll, FALSE);
            return (ll.findIndex(((ArrowJunctionCanvas *) dest)->get_interface()) != -1)
                   ? QString() : TR("not required");
        }

        case UmlProvided: {
            Q3ValueList<BrowserClass *> ll;

            ((BrowserComponent *) browser_node)->get_all_provided_classes(ll, FALSE);
            return (ll.findIndex(((ArrowJunctionCanvas *) dest)->get_interface()) != -1)
                   ? QString() : TR("not provided");
        }

        default:
            return TR("illegal");
        }

    default:
        return TR("illegal");
    }
}

void ComponentCanvas::save(QTextStream & st, bool ref, QString & warning) const
{
    if (ref) {
        st << "componentcanvas_ref " << get_ident() << " // "
           << browser_node->full_name();
    }
    else {
        nl_indent(st);
        st << "componentcanvas " << get_ident() << " ";
        browser_node->save(st, TRUE, warning);
        indent(+1);
        settings.save(st);
        nl_indent(st);

        if (itscolor != UmlDefaultColor)
            st << "color " << stringify(itscolor) << " ";

        save_xyzwh(st, this, "xyzwh");
        save_stereotype_property(st, warning);
        indent(-1);
        nl_indent(st);
        st << "end";
    }
}

ComponentCanvas * ComponentCanvas::read(char *& st, UmlCanvas * canvas,
                                        char * k)
{
    if (!strcmp(k, "componentcanvas_ref"))
        return ((ComponentCanvas *) dict_get(read_id(st), "componentcanvas", canvas));
    else if (!strcmp(k, "componentcanvas")) {
        int id = read_id(st);
        BrowserComponent * br = BrowserComponent::read_ref(st, read_keyword(st));
        ComponentCanvas * result = new ComponentCanvas(canvas, id);

        result->browser_node = br;
        connect(br->get_data(), SIGNAL(changed()), result, SLOT(modified()));
        connect(br->get_data(), SIGNAL(deleted()), result, SLOT(deleted()));

        k = read_keyword(st);

        result->settings.read(st, k);	// updates k
        read_color(st, "color", result->itscolor, k);	// updates k

        if (!strcmp(k, "xyz"))
            read_xyz(st, result);
        else if (!strcmp(k, "xyzwh"))
            read_xyzwh(st, result);
        else
            wrong_keyword(k, "xyzwh");

        if (read_file_format() >= 58) {
            k = read_keyword(st);
            result->read_stereotype_property(st, k);	// updates k

            if (strcmp(k, "end"))
                wrong_keyword(k, "end");
        }

        result->compute_size();
        result->set_center100();
        result->show();
        result->check_stereotypeproperties();

        return result;
    }
    else
        return 0;
}

void ComponentCanvas::history_save(QBuffer & b) const
{
    DiagramCanvas::history_save(b);
    ::save(min_width, b);
    ::save(min_height, b);
    ::save(width_scale100, b);
    ::save(height_scale100, b);
    ::save(width(), b);
    ::save(height(), b);
}

void ComponentCanvas::history_load(QBuffer & b)
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
    compute_size();
}

void ComponentCanvas::history_hide()
{
    DiagramCanvas::setVisible(FALSE);

    disconnect(DrawingSettings::instance(), SIGNAL(changed()),
               this, SLOT(modified()));
    disconnect(browser_node->get_data(), 0, this, 0);

    MultipleDependencyIterator<BasicData> it(this);

    while (it.current()) {
        disconnect(it.current(), 0, this, 0);
        ++it;
    }

    unsubscribe_all();
}
