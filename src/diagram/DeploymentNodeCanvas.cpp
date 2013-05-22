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
#include <qpainter.h>
#include <q3popupmenu.h>
//Added by qt3to4:
#include <QTextStream>
#include <Q3PointArray>
#include <QPixmap>

#include "DeploymentNodeCanvas.h"
#include "SimpleRelationCanvas.h"
#include "SimpleData.h"
#include "BrowserDeploymentNode.h"
#include "BrowserDiagram.h"
#include "DiagramView.h"
#include "UmlGlobal.h"
#include "Settings.h"
#include "SettingsDialog.h"
#include "InstanceDialog.h"
#include "myio.h"
#include "ToolCom.h"
#include "Tool.h"
#include "ui/menufactory.h"
#include "strutil.h"
#include "ProfiledStereotypes.h"
#include "translate.h"

DeploymentNodeCanvas::DeploymentNodeCanvas(BrowserNode * bn, UmlCanvas * canvas,
        int x, int y, int id)
    : DiagramCanvas(0, canvas, x, y, DEPLOYMENTNODE_CANVAS_MIN_SIZE,
                    DEPLOYMENTNODE_CANVAS_MIN_SIZE, id),
    itscolor(UmlDefaultColor), used_color(UmlDefaultColor)
{
    browser_node = bn;
    write_horizontally = UmlDefaultState;
    show_stereotype_properties = UmlDefaultState;
    check_size();

    if (id == 0) {
        // not on read
        check_stereotypeproperties();
    }

    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
    connect(bn->get_data(), SIGNAL(changed()), this, SLOT(modified()));
    connect(bn->get_data(), SIGNAL(deleted()), this, SLOT(deleted()));
}

DeploymentNodeCanvas::~DeploymentNodeCanvas()
{
}

void DeploymentNodeCanvas::delete_it()
{
    disconnect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
    disconnect(browser_node->get_data(), 0, this, 0);
    DiagramCanvas::delete_it();
}

void DeploymentNodeCanvas::deleted()
{
    delete_it();
    canvas()->update();
}

void DeploymentNodeCanvas::remove(bool from_model)
{
    if (! from_model)
        delete_it();
    else
        browser_node->delete_it();	// will remove canvas
}

QString DeploymentNodeCanvas::get_name() const
{
    return iname;
}

void DeploymentNodeCanvas::set_name(const QString & s)
{
    iname = s;
}

BrowserNode * DeploymentNodeCanvas::get_type() const
{
    return browser_node;
}

BrowserNodeList & DeploymentNodeCanvas::get_types(BrowserNodeList & r) const
{
    return BrowserDeploymentNode::instances(r);
}

void DeploymentNodeCanvas::set_type(BrowserNode * t)
{
    if (browser_node != t) {
        disconnect(browser_node->get_data(), SIGNAL(changed()), this, SLOT(modified()));
        disconnect(browser_node->get_data(), SIGNAL(deleted()), this, SLOT(deleted()));
        browser_node = t;
        connect(browser_node->get_data(), SIGNAL(changed()), this, SLOT(modified()));
        connect(browser_node->get_data(), SIGNAL(deleted()), this, SLOT(deleted()));
    }
}

BrowserNode * DeploymentNodeCanvas::new_type()
{
    BrowserNode * container = (BrowserNode *)
                              the_canvas()->browser_diagram()->parent();

    return (container->is_writable())
           ? BrowserDeploymentNode::add_deploymentnode(container)
           : 0;
}

bool DeploymentNodeCanvas::new_type_available()
{
    BrowserNode * container = (BrowserNode *)
                              the_canvas()->browser_diagram()->parent();

    return container->is_writable();
}

int DeploymentNodeCanvas::min_width()
{
    QFontMetrics fm(the_canvas()->get_font(UmlNormalFont));

    int wi;

    if (horiz)
        wi = fm.width(iname + ":" + browser_node->get_name());
    else {
        wi = fm.width(browser_node->get_name());

        int iw = fm.width(iname + ":");

        if (iw > wi)
            wi = iw;
    }

    const char * st = browser_node->get_data()->get_short_stereotype();

    if (st[0]) {
        int stw = fm.width(QString("<<") + toUnicode(st) + ">>");

        if (wi < stw)
            wi = stw;
    }

    wi += (int)((DEPLOYMENTNODE_CANVAS_ADDED + 8) * the_canvas()->zoom());

    int min_w = (int)(DEPLOYMENTNODE_CANVAS_MIN_SIZE * the_canvas()->zoom());

    return (wi < min_w)
           ? min_w
           : wi;
}

int DeploymentNodeCanvas::min_height()
{
    QFontMetrics fm(the_canvas()->get_font(UmlNormalFont));
    int three = (int)(3 * the_canvas()->zoom());
    int h = fm.height() + three;
    const char * st = browser_node->get_data()->get_short_stereotype();
    int he = h;

    if (st[0])
        he += h;

    if (! horiz)
        he += h;

    he += (int)((DEPLOYMENTNODE_CANVAS_ADDED + 4) * the_canvas()->zoom());

    int min_h = (int)(DEPLOYMENTNODE_CANVAS_MIN_SIZE * the_canvas()->zoom());

    return (he < min_h)
           ? min_h
           : he;
}

void DeploymentNodeCanvas::check_size()
{
    DeploymentDiagramSettings dflt;

    dflt.write_horizontally = write_horizontally;
    the_canvas()->browser_diagram()->get_deploymentdiagramsettings(dflt);
    horiz = (dflt.write_horizontally == UmlYes);
    show_properties = (dflt.componentdrawingsettings.show_stereotype_properties == UmlYes);

    const QPixmap * px =
        ProfiledStereotypes::diagramPixmap(browser_node->get_data()->get_stereotype(),
                                           the_canvas()->zoom());

    if (px != 0) {
        QFontMetrics fm(the_canvas()->get_font(UmlNormalBoldFont));
        int w;
        int h = px->height() + fm.height();

        if (horiz)
            w = fm.width(iname + ":" + browser_node->get_name());
        else {
            w = fm.width(browser_node->get_name());

            int iw = fm.width(iname + ":");

            if (iw > w)
                w = iw;

            h += fm.height() + (int)(3 * the_canvas()->zoom());
        }

        if (w < px->width())
            w = px->width();

        // force odd width and height for line alignment
        DiagramCanvas::resize(w | 1, h | 1);
    }
    else {
        int wi = min_width();
        int he = min_height();

        // warning : do NOT check if ((width() < wi) || (height() < he))
        // because te resize must be done to set data on scale change
        DiagramCanvas::resize((width() < wi) ? wi : width(),
                              (height() < he) ? he : height());
    }
}

void DeploymentNodeCanvas::change_scale()
{
    double scale = the_canvas()->zoom();

    Q3CanvasRectangle::setVisible(FALSE);
    setSize((int)(width_scale100 * scale), (int)(height_scale100 * scale));
    check_size();
    recenter();
    Q3CanvasRectangle::setVisible(TRUE);
}

void DeploymentNodeCanvas::draw(QPainter & p)
{
    if (! visible()) return;

    p.setRenderHint(QPainter::Antialiasing, true);
    QRect r = rect();
    FILE * fp = svg();

    if (fp != 0)
        fputs("<g>\n", fp);

    const int three = (int)(3 * the_canvas()->zoom());
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
        UmlColor c;

        if (itscolor != UmlDefaultColor)
            c = itscolor;
        else {
            if (used_color == UmlDefaultColor)
                used_color = the_canvas()->browser_diagram()->get_color(UmlDeploymentNode);

            c = used_color;
        }

        QColor co = color(c);
        const int added = (int)(DEPLOYMENTNODE_CANVAS_ADDED * the_canvas()->zoom());
        Q3PointArray a(7);

        r.setTop(r.top() + added);
        r.setRight(r.right() - added);

        a.setPoint(0, r.left(), r.top());
        a.setPoint(1, r.left() + added, r.top() - added);
        a.setPoint(2, r.right() + added, r.top() - added);
        a.setPoint(3, r.right() + added, r.bottom() - added);
        a.setPoint(4, r.right(), r.bottom());
        a.setPoint(5, r.right(), r.top());
        a.setPoint(6, r.left(), r.top());

        if (c == UmlTransparent) {
            p.setBackgroundMode(::Qt::TransparentMode);
            p.setBackgroundColor(co);
            p.drawPolyline(a);

            if (fp != 0) {
                fprintf(fp, "\t<rect fill=\"none\" stroke=\"black\" stroke-width=\"1\" stroke-opacity=\"1\""
                        " x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" />\n",
                        r.x(), r.y(), r.width() - 1, r.height() - 1);
                draw_poly(fp, a, UmlTransparent);
            }
        }
        else {
            QBrush brsh = p.brush();

            p.setBackgroundMode(::Qt::OpaqueMode);
            p.fillRect(r, co);
            p.setBrush(co);
            p.drawPolygon(a, TRUE, 0, 6);
            p.setBrush(brsh);
            p.setBackgroundColor(co);

            if (fp != 0) {
                fprintf(fp, "\t<rect fill=\"%s\" stroke=\"black\" stroke-width=\"1\" stroke-opacity=\"1\""
                        " x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" />\n",
                        svg_color(c),
                        r.x(), r.y(), r.width() - 1, r.height() - 1);
                draw_poly(fp, a, c);
            }
        }

        p.drawRect(r);
        p.drawLine(r.topRight(), a.point(2));

        if (fp != 0)
            fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                    " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                    r.right(), r.top(), a.point(2).x(), a.point(2).y());

        QFontMetrics fm(the_canvas()->get_font(UmlNormalFont));
        const char * st = browser_node->get_data()->get_short_stereotype();
        QString s;

        r.setTop(r.top() + three);
        p.setFont(the_canvas()->get_font(UmlNormalFont));

        if (st[0]) {
            s = QString("<<") + toUnicode(st) + ">>";
            p.drawText(r, ::Qt::AlignHCenter + ::Qt::AlignTop, s);

            if (fp != 0)
                draw_text(r, ::Qt::AlignHCenter + ::Qt::AlignTop, s, p.font(), fp);

            r.setTop(r.top() + fm.height() + three);
        }
    }

    QFontMetrics fm(p.font());

    if (horiz) {
        QString s = iname + ":" + browser_node->get_name();

        p.drawText(r, ::Qt::AlignHCenter + ::Qt::AlignTop, s);

        if (fp != 0)
            draw_text(r, ::Qt::AlignHCenter + ::Qt::AlignTop, s, p.font(), fp);
    }
    else {
        QString s = iname + ":";

        p.drawText(r, ::Qt::AlignHCenter + ::Qt::AlignTop, s);

        if (fp != 0)
            draw_text(r, ::Qt::AlignHCenter + ::Qt::AlignTop, s, p.font(), fp);

        r.setTop(r.top() + fm.height() + three);
        s = browser_node->get_name();
        p.drawText(r, ::Qt::AlignHCenter + ::Qt::AlignTop, s);

        if (fp != 0)
            draw_text(r, ::Qt::AlignHCenter + ::Qt::AlignTop, s, p.font(), fp);
    }

    p.setFont(the_canvas()->get_font(UmlNormalFont));
    p.setBackgroundColor(bckgrnd);

    if (fp != 0)
        fputs("</g>\n", fp);

    if (selected())
        show_mark(p, rect());
}

UmlCode DeploymentNodeCanvas::type() const
{
    return UmlDeploymentNode;
}

void DeploymentNodeCanvas::delete_available(BooL & in_model, BooL & out_model) const
{
    out_model |= TRUE;
    in_model |= browser_node->is_writable();
}

bool DeploymentNodeCanvas::alignable() const
{
    return TRUE;
}

bool DeploymentNodeCanvas::copyable() const
{
    return selected();
}

void DeploymentNodeCanvas::open()
{
    InstanceDialog d(this, "node", UmlDeploymentNode);

    if (d.exec() == QDialog::Accepted)
        modified();
}

void DeploymentNodeCanvas::modified()
{
    used_color = (itscolor == UmlDefaultColor)
                 ? the_canvas()->browser_diagram()->get_color(UmlDeploymentNode)
                 : itscolor;
    // force son reaffichage
    hide();
    check_size();
    show();
    force_self_rel_visible();
    check_stereotypeproperties();
    canvas()->update();
    package_modified();
}

void DeploymentNodeCanvas::menu(const QPoint &)
{
    Q3PopupMenu m(0);
    Q3PopupMenu toolm(0);

    MenuFactory::createTitle(m, browser_node->get_data()->definition(FALSE, TRUE));
    m.insertSeparator();
    m.insertItem(TR("Upper"), 0);
    m.insertItem(TR("Lower"), 1);
    m.insertItem(TR("Go up"), 13);
    m.insertItem(TR("Go down"), 14);
    m.insertSeparator();
    m.insertItem(TR("Add related elements"), 10);
    m.insertSeparator();
    m.insertItem(TR("Edit"), 2);
    m.insertSeparator();
    m.insertItem(TR("Edit drawing settings"), 3);
    m.insertSeparator();
    m.insertItem(TR("Select node in browser"), 4);

    if (linked())
        m.insertItem(TR("Select linked items"), 5);

    m.insertSeparator();
    m.insertItem(TR("Set node associated diagram"), 6);
    m.insertSeparator();
    m.insertItem(TR("Remove from diagram"), 7);
    m.insertSeparator();

    if (Tool::menu_insert(&toolm, UmlDeploymentNode, 20))
        m.insertItem(TR("Tool"), &toolm);

    int rank = m.exec(QCursor::pos());

    switch (rank) {
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
        open();
        return;

    case 3:
        edit_drawing_settings();
        return;

    case 4:
        browser_node->select_in_browser();
        return;

    case 5:
        the_canvas()->unselect_all();
        select_associated();
        return;

    case 6:
        ((BrowserDeploymentNode *) browser_node)
        ->set_associated_diagram((BrowserDeploymentDiagram *)
                                 the_canvas()->browser_diagram());
        return;

    case 7:
        // remove from diagram
        delete_it();
        break;

    case 10:
        ((UmlCanvas *) canvas())->get_view()
        ->add_related_elements(this, TR("node"), FALSE, FALSE);
        return;

    default:
        if (rank >= 20)
            ToolCom::run(Tool::command(rank - 20), browser_node);

        return;
    }

    package_modified();
}

void DeploymentNodeCanvas::apply_shortcut(QString s)
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
        ->add_related_elements(this, TR("node"), FALSE, FALSE);
        return;
    }
    else {
        browser_node->apply_shortcut(s);
        return;
    }

    modified();
}

void DeploymentNodeCanvas::edit_drawing_settings()
{
    for (;;) {
        StateSpecVector st(2);
        ColorSpecVector co(1);

        st[0].set(TR("write node instance \nhorizontally"), &write_horizontally);
        st[1].set(TR("show stereotype \nproperties"), &show_stereotype_properties);
        co[0].set(TR("Node color"), &itscolor);

        SettingsDialog dialog(&st, &co, FALSE);

        dialog.raise();

        if (dialog.exec() == QDialog::Accepted)
            modified();

        if (!dialog.redo())
            break;
    }
}

bool DeploymentNodeCanvas::has_drawing_settings() const
{
    return TRUE;
}

void DeploymentNodeCanvas::edit_drawing_settings(QList<DiagramItem *> & l)
{
    for (;;) {
        StateSpecVector st(2);
        ColorSpecVector co(1);
        Uml3States write_horizontally;
        Uml3States show_stereotype_properties;
        UmlColor itscolor;

        st[0].set(TR("write node instance \nhorizontally"), &write_horizontally);
        st[1].set(TR("show stereotype \nproperties"), &show_stereotype_properties);
        co[0].set(TR("Node color"), &itscolor);

        SettingsDialog dialog(&st, &co, FALSE, TRUE);

        dialog.raise();

        if (dialog.exec() == QDialog::Accepted) {
            foreach (DiagramItem *item, l) {
                DeploymentNodeCanvas *canvas = (DeploymentNodeCanvas *)item;
                if (!st[0].name.isEmpty())
                    canvas->write_horizontally =
                        write_horizontally;

                if (!st[1].name.isEmpty())
                    canvas->show_stereotype_properties =
                        show_stereotype_properties;

                if (!co[0].name.isEmpty())
                    canvas->itscolor = itscolor;

                canvas->modified();
            }
        }

        if (!dialog.redo())
            break;
    }
}

void DeploymentNodeCanvas::clone_drawing_settings(const DiagramItem *src)
{
    const DeploymentNodeCanvas * x = (const DeploymentNodeCanvas *) src;
    write_horizontally = x->write_horizontally;
    show_stereotype_properties = x->show_stereotype_properties;
    itscolor = x->itscolor;
    modified();
}

bool DeploymentNodeCanvas::get_show_stereotype_properties() const
{
    switch (show_stereotype_properties) {
    case UmlYes:
        return TRUE;

    case UmlNo:
        return FALSE;

    default:
        return the_canvas()->browser_diagram()->get_show_stereotype_properties();
    }
}

QString DeploymentNodeCanvas::may_start(UmlCode & l) const
{
    switch (l) {
    case UmlDependency:
        l = UmlDependOn;
        return (browser_node->is_writable()) ? QString() : TR("read only");

    case UmlAssociation:
    case UmlAnchor:
        return 0;

    default:
        return TR("illegal");
    }
}

QString DeploymentNodeCanvas::may_connect(UmlCode & l, const DiagramItem * dest) const
{
    if (l == UmlAnchor)
        return dest->may_start(l);

    switch (dest->type()) {
    case UmlDeploymentNode:
        return 0;

    case UmlHub:
        return (l == UmlAssociation) ? QString() : TR("illegal");

    default:
        return TR("illegal");
    }
}

void DeploymentNodeCanvas::connexion(UmlCode action, DiagramItem * dest,
                                     const QPoint &, const QPoint &)
{
    ArrowCanvas * a;

    if (action == UmlDependOn)
        a = new SimpleRelationCanvas(the_canvas(), this, dest, 0, action, 0, -1.0, -1.0);
    else
        a = new ArrowCanvas(the_canvas(), this, dest, action, 0, FALSE, -1.0, -1.0);

    a->show();
    the_canvas()->select(a);
}

aCorner DeploymentNodeCanvas::on_resize_point(const QPoint & p)
{
    return (ProfiledStereotypes::diagramPixmap(browser_node->get_data()->get_stereotype(),
            the_canvas()->zoom())
            != 0)
           ? NoCorner
           : ::on_resize_point(p, rect());
}

void DeploymentNodeCanvas::resize(aCorner c, int dx, int dy, QPoint & o)
{
    DiagramCanvas::resize(c, dx, dy, o, min_width(), min_height(), TRUE);
}

void DeploymentNodeCanvas::resize(const QSize & sz, bool w, bool h)
{
    DiagramCanvas::resize(sz, w, h, min_width(), min_height(), TRUE);
}

bool DeploymentNodeCanvas::move_with_its_package() const
{
    return TRUE;
}

void DeploymentNodeCanvas::save(QTextStream & st, bool ref, QString & warning) const
{
    if (ref)
        st << "deploymentnodecanvas_ref " << get_ident() << " // " << browser_node->get_name();
    else {
        nl_indent(st);
        st << "deploymentnodecanvas " << get_ident() << " ";
        indent(+1);
        browser_node->save(st, TRUE, warning);

        if (!iname.isEmpty()) {
            nl_indent(st);
            st << "name ";
            save_string(iname, st);
        }

        nl_indent(st);

        if (write_horizontally != UmlDefaultState)
            st << "write_horizontally " << stringify(write_horizontally) << " ";

        if (show_stereotype_properties != UmlDefaultState)
            st << "show_stereotype_properties " << stringify(show_stereotype_properties) << " ";

        if (itscolor != UmlDefaultColor)
            st << "color " << stringify(itscolor) << " ";

        save_xyzwh(st, this, "xyzwh");
        save_stereotype_property(st, warning);
        indent(-1);
        nl_indent(st);
        st << "end";
    }
}

DeploymentNodeCanvas * DeploymentNodeCanvas::read(char *& st, UmlCanvas * canvas, char * k)
{
    if (!strcmp(k, "deploymentnodecanvas_ref"))
        return ((DeploymentNodeCanvas *) dict_get(read_id(st), "deploymentnodecanvas", canvas));
    else if (!strcmp(k, "deploymentnodecanvas")) {
        int id = read_id(st);
        BrowserNode * br = BrowserDeploymentNode::read_ref(st);
        QString iname;
        Uml3States write_horizontally = UmlDefaultState;
        Uml3States show_stereotype_properties = UmlDefaultState;

        k = read_keyword(st);

        if (!strcmp(k, "name")) {
            iname = read_string(st);
            k = read_keyword(st);
        }

        if (!strcmp(k, "write_horizontally") ||
            !strcmp(k, "write_horizontaly")) {
            write_horizontally = state(read_keyword(st));
            k = read_keyword(st);
        }

        if (!strcmp(k, "show_stereotype_properties")) {
            show_stereotype_properties = state(read_keyword(st));
            k = read_keyword(st);
        }

        UmlColor color = UmlDefaultColor;

        read_color(st, "color", color, k);

        if (strcmp(k, "xyzwh"))
            wrong_keyword(k, "xyzwh");

        int x = (int) read_double(st);
        DeploymentNodeCanvas * result =
            new DeploymentNodeCanvas(br, canvas, x, (int) read_double(st), id);

        read_zwh(st, result);

        if (read_file_format() >= 58) {
            k = read_keyword(st);
            result->read_stereotype_property(st, k);	// updates k

            if (strcmp(k, "end"))
                wrong_keyword(k, "end");
        }

        result->iname = iname;
        result->write_horizontally = write_horizontally;
        result->show_stereotype_properties = show_stereotype_properties;
        result->itscolor = color;
        result->check_size();
        result->set_center100();
        result->show();
        result->check_stereotypeproperties();

        return result;
    }
    else
        return 0;
}

void DeploymentNodeCanvas::history_hide()
{
    Q3CanvasItem::setVisible(FALSE);
    disconnect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
    disconnect(browser_node->get_data(), 0, this, 0);
}

void DeploymentNodeCanvas::history_save(QBuffer & b) const
{
    DiagramCanvas::history_save(b);
    ::save(width_scale100, b);
    ::save(height_scale100, b);
    ::save(width(), b);
    ::save(height(), b);
}

void DeploymentNodeCanvas::history_load(QBuffer & b)
{
    DiagramCanvas::history_load(b);
    ::load(width_scale100, b);
    ::load(height_scale100, b);

    int w, h;

    ::load(w, b);
    ::load(h, b);
    Q3CanvasRectangle::setSize(w, h);

    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
    connect(browser_node->get_data(), SIGNAL(changed()), this, SLOT(modified()));
    connect(browser_node->get_data(), SIGNAL(deleted()), this, SLOT(deleted()));
}
