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
#include <QPixmap>

#include "PackageCanvas.h"
#include "SimpleRelationCanvas.h"
#include "DiagramView.h"
#include "PackageData.h"
#include "BrowserPackage.h"
#include "BrowserDiagram.h"
#include "UmlGlobal.h"
#include "SettingsDialog.h"
#include "myio.h"
#include "ToolCom.h"
#include "Tool.h"
#include "ui/menufactory.h"
#include "strutil.h"
#include "ProfiledStereotypes.h"
#include "translate.h"

PackageCanvas::PackageCanvas(BrowserNode * bn, UmlCanvas * canvas,
                             int x, int y, int id)
    : DiagramCanvas(0, canvas, x, y, PACKAGE_CANVAS_MIN_WIDTH,
                    PACKAGE_CANVAS_MIN_HEIGHT, id)
{
    browser_node = bn;
    show_context_mode = DefaultShowContextMode;
    name_in_tab = UmlDefaultState;
    show_stereotype_properties = UmlDefaultState;
    itscolor = UmlDefaultColor;
    check_size();
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
    connect(bn->get_data(), SIGNAL(changed()), this, SLOT(modified()));
    connect(bn->get_data(), SIGNAL(deleted()), this, SLOT(deleted()));

    if (id == 0) {
        // not from read
        if (canvas->must_draw_all_relations())
            draw_all_simple_relations();

        check_stereotypeproperties();
    }
}

PackageCanvas::~PackageCanvas()
{
}

void PackageCanvas::delete_it()
{
    disconnect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
    disconnect(browser_node->get_data(), 0, this, 0);
    DiagramCanvas::delete_it();
}

void PackageCanvas::deleted()
{
    delete_it();
    canvas()->update();
}

void PackageCanvas::remove(bool from_model)
{
    if (! from_model)
        delete_it();
    else
        browser_node->delete_it();	// will remove canvas
}

int PackageCanvas::min_width()
{
    BooL intab;
    ShowContextMode showcontext;

    the_canvas()->browser_diagram()->package_settings(intab, showcontext);

    in_tab = (name_in_tab != UmlDefaultState) ? (name_in_tab == UmlYes)
             : intab;

    const PackageData * data = (const PackageData *) browser_node->get_data();

    switch ((show_context_mode != DefaultShowContextMode)
            ? show_context_mode : showcontext) {
    case noContext:
        full_name = browser_node->get_name();
        break;

    case umlContext:
        full_name = browser_node->full_name();
        break;

    case namespaceContext:
        if ((full_name = ((const char *) data->get_cpp_namespace())).isEmpty())
            full_name = browser_node->get_name();

        break;

    case javaPackageContext:
        if ((full_name = ((const char *) data->get_java_package())).isEmpty())
            full_name = browser_node->get_name();

        break;

    case pythonPackageContext:
        if ((full_name = ((const char *) data->get_python_package())).isEmpty())
            full_name = browser_node->get_name();

        break;

    case moduleContext:
        if ((full_name = ((const char *) data->get_idl_module())).isEmpty())
            full_name = browser_node->get_name();

    case DefaultShowContextMode:
        // not possible
        break;
    }

    int eight = (int)(8 * the_canvas()->zoom());
    QFontMetrics fm(the_canvas()->get_font(UmlNormalFont));

    int wi = (in_tab)
             ? (fm.width(browser_node->get_name()) + eight) * 5 / 2
             : fm.width(full_name) + eight;

    if (full_name != browser_node->get_name()) {
        int cntxw = fm.width(full_name) + eight;

        if (wi < cntxw)
            wi = cntxw;
    }

    if (data->get_stereotype()[0]) {
        int stw = fm.width(QString("<<") + toUnicode(data->get_short_stereotype()) + ">>") + eight;

        if (wi < stw)
            wi = stw;
    }

    int min_w = (int)(PACKAGE_CANVAS_MIN_WIDTH * the_canvas()->zoom());

    // note : used_color already updated
    if (used_color != UmlTransparent)
        min_w += the_canvas()->shadow();

    return (wi < min_w) ? min_w : wi;
}

int PackageCanvas::min_height()
{
    QFontMetrics fm(the_canvas()->get_font(UmlNormalFont));

    int he = fm.height() * 5;
    int min_h = (int)(PACKAGE_CANVAS_MIN_HEIGHT * the_canvas()->zoom());

    // note : used_color already updated
    if (used_color != UmlTransparent)
        min_h += the_canvas()->shadow();

    return (he < min_h) ? min_h : he;
}

void PackageCanvas::check_size()
{
    used_color = (itscolor == UmlDefaultColor)
                 ? the_canvas()->browser_diagram()->get_color(UmlPackage)
                 : itscolor;

    const QPixmap * px =
        ProfiledStereotypes::diagramPixmap(browser_node->get_data()->get_stereotype(),
                                           the_canvas()->zoom());

    if (px != 0) {
        QFontMetrics fm(the_canvas()->get_font(UmlNormalBoldFont));
        int wi = fm.width(browser_node->get_name());

        if (wi < px->width())
            wi = px->width();

        // force odd width and height for line alignment
        DiagramCanvas::resize(wi | 1, (px->height() + fm.height()) | 1);

    }
    else {
        int wi = min_width();
        int he = min_height();

        // warning : do NOT check if ((width() < wi) || (height() < he))
        // because te resize must be done to set data on scale change
        // Force odd width and height for line alignment
        DiagramCanvas::resize((width() > wi) ? width() : wi | 1,
                              (height() > he) ? height() : he | 1);
    }
}

void PackageCanvas::change_scale()
{
    double scale = the_canvas()->zoom();

    Q3CanvasRectangle::setVisible(FALSE);
    setSize((int)(width_scale100 * scale), (int)(height_scale100 * scale));
    check_size();
    recenter();
    Q3CanvasRectangle::setVisible(TRUE);
}

void PackageCanvas::draw(QPainter & p)
{
    if (! visible()) return;

    p.setRenderHint(QPainter::Antialiasing, true);
    QRect r = rect();
    const BasicData * data = browser_node->get_data();
    const QPixmap * px =
        ProfiledStereotypes::diagramPixmap(data->get_stereotype(),
                                           the_canvas()->zoom());
    FILE * fp = svg();

    if (fp != 0)
        fputs("<g>\n", fp);

    if (px != 0) {
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

        // pixmap not yet exported in SVG
        if (fp != 0)
            draw_text(r, ::Qt::AlignHCenter, browser_node->get_name(),
                      p.font(), fp);

        p.setFont(the_canvas()->get_font(UmlNormalFont));
    }
    else {
        QColor bckgrnd = p.backgroundColor();
        QColor co = color(used_color);

        p.setBackgroundMode((used_color == UmlTransparent) ? ::Qt::TransparentMode : ::Qt::OpaqueMode);
        p.setBackgroundColor(co);
        p.setFont(the_canvas()->get_font(UmlNormalFont));

        QFontMetrics fm(the_canvas()->get_font(UmlNormalFont));
        const int four = (int)(4 * the_canvas()->zoom());
        const int he = fm.height();
        const int shadow = the_canvas()->shadow();

        if ((used_color != UmlTransparent) && (shadow != 0)) {
            r.setRight(r.right() - shadow);
            r.setBottom(r.bottom() - shadow);
        }

        r.setWidth(r.width() * 2 / 5);
        r.setHeight(he + four);

        if (used_color != UmlTransparent)
            p.fillRect(r, co);

        if (fp != 0)
            fprintf(fp, "\t<rect fill=\"%s\" stroke=\"black\" stroke-width=\"1\" stroke-opacity=\"1\""
                    " x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" />\n",
                    svg_color(used_color),
                    r.x(), r.y(), r.width() - 1, r.height() - 1);

        p.drawRect(r);

        if ((used_color != UmlTransparent) && (shadow != 0)) {
            p.fillRect(r.right(), r.top() + shadow,
                       shadow, r.height() - 1 - shadow,
                       ::Qt::darkGray);

            if (fp != 0)
                fprintf(fp, "\t<rect fill=\"#%06x\" stroke=\"none\" stroke-opacity=\"1\""
                        " x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" />\n",
                        QColor(::Qt::darkGray).rgb() & 0xffffff,
                        r.right(), r.top() + shadow, shadow - 1, r.height() - 1 - shadow - 1);
        }

        const char * name = browser_node->get_name();

        if (in_tab) {
            p.drawText(r, ::Qt::AlignCenter, name);

            if (fp != 0)
                draw_text(r, ::Qt::AlignCenter, name,
                          p.font(), fp);
        }

        r = rect();

        if ((used_color != UmlTransparent) && (shadow != 0)) {
            r.setRight(r.right() - shadow);
            r.setBottom(r.bottom() - shadow);
        }

        r.setTop(r.top() + he + four - 1);

        if (used_color != UmlTransparent)
            p.fillRect(r, co);

        if (fp != 0)
            fprintf(fp, "\t<rect fill=\"%s\" stroke=\"black\" stroke-width=\"1\" stroke-opacity=\"1\""
                    " x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" />\n",
                    svg_color(used_color),
                    r.x(), r.y(), r.width() - 1, r.height() - 1);

        p.drawRect(r);

        if ((used_color != UmlTransparent) && (shadow != 0)) {
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

        const int three = (int)(3 * the_canvas()->zoom());

        r.setTop(r.top() + three);

        if (! in_tab) {
            p.drawText(r, ::Qt::AlignHCenter + ::Qt::AlignTop, name);

            if (fp != 0)
                draw_text(r, ::Qt::AlignHCenter + ::Qt::AlignTop, name,
                          p.font(), fp);

            r.setTop(r.top() + he + three);
        }

        if (data->get_stereotype()[0]) {
            p.drawText(r, ::Qt::AlignHCenter + ::Qt::AlignTop,
                       QString("<<") + toUnicode(data->get_short_stereotype()) + ">>");

            if (fp != 0)
                draw_text(r, ::Qt::AlignHCenter + ::Qt::AlignTop,
                          QString("<<") + toUnicode(data->get_short_stereotype()) + ">>",
                          p.font(), fp);

            r.setTop(r.top() + he + three);
        }

        if (full_name != name) {
            p.setFont(the_canvas()->get_font(UmlNormalItalicFont));
            p.drawText(r, ::Qt::AlignHCenter + ::Qt::AlignTop, full_name);

            if (fp != 0)
                draw_text(r, ::Qt::AlignHCenter + ::Qt::AlignTop, full_name,
                          p.font(), fp);

            p.setFont(the_canvas()->get_font(UmlNormalFont));
        }

        p.setBackgroundColor(bckgrnd);
    }

    if (fp != 0)
        fputs("</g>\n", fp);

    if (selected())
        show_mark(p, rect());
}

UmlCode PackageCanvas::type() const
{
    return UmlPackage;
}

void PackageCanvas::delete_available(BooL & in_model, BooL & out_model) const
{
    out_model |= TRUE;
    in_model |= browser_node->is_writable();
}

bool PackageCanvas::alignable() const
{
    return TRUE;
}

bool PackageCanvas::copyable() const
{
    return selected();
}

void PackageCanvas::open()
{
    browser_node->open(FALSE);
}

void PackageCanvas::modified()
{
    // force son reaffichage
    hide();
    check_size();
    show();
    check_stereotypeproperties();
    force_self_rel_visible();

    if (the_canvas()->must_draw_all_relations())
        draw_all_simple_relations();

    canvas()->update();
    package_modified();
}

void PackageCanvas::post_loaded()
{
    force_self_rel_visible();

    if (the_canvas()->must_draw_all_relations())
        draw_all_simple_relations();
}

void PackageCanvas::menu(const QPoint &)
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

    if (Tool::menu_insert(&toolm, UmlPackage, 20))
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
        browser_node->open(TRUE);
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
        ((BrowserPackage *) browser_node)
        ->set_associated_diagram((BrowserNode *)
                                 the_canvas()->browser_diagram());
        return;

    case 7:
        // remove from diagram
        delete_it();
        break;

    case 8:
        // delete from model
        browser_node->delete_it();	// will remove canvas
        break;

    case 9:
        ((BrowserPackage *) browser_node)
        ->set_associated_diagram(0);
        return;

    case 10:
        ((UmlCanvas *) canvas())->get_view()
        ->add_related_elements(this, TR("package"), TRUE, FALSE);
        return;

    default:
        if (rank >= 20)
            ToolCom::run(Tool::command(rank - 20), browser_node);

        return;
    }

    package_modified();
}

void PackageCanvas::apply_shortcut(QString s)
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
        ->add_related_elements(this, TR("package"), TRUE, FALSE);
        return;
    }
    else {
        browser_node->apply_shortcut(s);
        return;
    }

    modified();
    package_modified();
}

void PackageCanvas::edit_drawing_settings()
{
    for (;;) {
        StateSpecVector st(3);
        ColorSpecVector co(1);

        st[0].set(TR("name in tab"), &name_in_tab);
        st[1].set(TR("show context"), &show_context_mode);
        st[2].set(TR("show stereotype \nproperties"), &show_stereotype_properties);
        co[0].set(TR("Package color"), &itscolor);

        SettingsDialog dialog(&st, &co, FALSE);

        dialog.raise();

        if (dialog.exec() == QDialog::Accepted)
            modified();

        if (!dialog.redo())
            break;
    }
}

bool PackageCanvas::has_drawing_settings() const
{
    return TRUE;
}

void PackageCanvas::edit_drawing_settings(QList<DiagramItem *> & l)
{
    for (;;) {
        StateSpecVector st(3);
        ColorSpecVector co(1);
        Uml3States name_in_tab;
        Uml3States show_stereotype_properties;
        ShowContextMode show_context_mode;
        UmlColor itscolor;

        st[0].set(TR("name in tab"), &name_in_tab);
        st[1].set(TR("show context"), &show_context_mode);
        st[2].set(TR("show stereotype \nproperties"), &show_stereotype_properties);
        co[0].set(TR("Package color"), &itscolor);

        SettingsDialog dialog(&st, &co, FALSE, TRUE);

        dialog.raise();

        if (dialog.exec() == QDialog::Accepted) {
            foreach (DiagramItem *item, l) {
                PackageCanvas *canvas = (PackageCanvas *)item;
                if (!st[0].name.isEmpty())
                    canvas->name_in_tab = name_in_tab;

                if (!st[1].name.isEmpty())
                    canvas->show_context_mode = show_context_mode;

                if (!st[2].name.isEmpty())
                    canvas->show_stereotype_properties = show_stereotype_properties;

                if (!co[0].name.isEmpty())
                    canvas->itscolor = itscolor;

                canvas->modified();
            }
        }

        if (!dialog.redo())
            break;
    }
}

void PackageCanvas::clone_drawing_settings(const DiagramItem *src)
{
    const PackageCanvas * x = (const PackageCanvas *) src;
    name_in_tab = x->name_in_tab;
    show_context_mode = x->show_context_mode;
    show_stereotype_properties = x->show_stereotype_properties;
    itscolor = x->itscolor;
    modified();
}

bool PackageCanvas::get_show_stereotype_properties() const
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

QString PackageCanvas::may_start(UmlCode & l) const
{
    switch (l) {
    case UmlDependency:
        l = UmlDependOn;

    case UmlDependOn:
        return (browser_node->is_writable()) ? QString() : TR("read only");

    case UmlGeneralisation:
        l = UmlInherit;
        return (browser_node->is_writable()) ? QString() : TR("read only");

    case UmlAnchor:
        return 0;

    default:
        return TR("illegal");
    }
}

QString PackageCanvas::may_connect(UmlCode & l, const DiagramItem * dest) const
{
    if (l == UmlAnchor)
        return dest->may_start(l);
    else if (dest->get_bn() == 0)
        return TR("illegal");
    else
        return ((BrowserPackage *) browser_node)->may_connect(l, dest->get_bn());
}

void PackageCanvas::connexion(UmlCode action, DiagramItem * dest,
                              const QPoint &, const QPoint &)
{
    ArrowCanvas * r;

    if (IsaSimpleRelation(action))
        r = new SimpleRelationCanvas(the_canvas(), this, dest, 0, action, 0, -1.0, -1.0);
    else
        r = new ArrowCanvas(the_canvas(), this, dest, action, 0, FALSE, -1.0, -1.0);

    r->show();
    the_canvas()->select(r);
}

aCorner PackageCanvas::on_resize_point(const QPoint & p)
{
    return (ProfiledStereotypes::diagramPixmap(browser_node->get_data()->get_stereotype(),
            the_canvas()->zoom())
            != 0)
           ? NoCorner
           : ::on_resize_point(p, rect());
}

void PackageCanvas::resize(aCorner c, int dx, int dy, QPoint & o)
{
    DiagramCanvas::resize(c, dx, dy, o, min_width(), min_height(), TRUE);
}

void PackageCanvas::resize(const QSize & sz, bool w, bool h)
{
    DiagramCanvas::resize(sz, w, h, min_width(), min_height(), TRUE);
}

void PackageCanvas::prepare_for_move(bool on_resize)
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
                di->move_with_its_package()) {
                BrowserNode * bn = di->get_bn();

                do
                    bn = (BrowserNode *) bn->parent();

                while (bn->get_type() != UmlPackage);

                if (bn == p) {
                    the_canvas()->select(*it);
                    di->prepare_for_move(FALSE);
                }
            }
        }
    }
}

bool PackageCanvas::move_with_its_package() const
{
    return TRUE;
}

void PackageCanvas::save(QTextStream & st, bool ref, QString & warning) const
{
    if (ref)
        st << "packagecanvas_ref " << get_ident() << " // " << browser_node->get_name();
    else {
        nl_indent(st);
        st << "packagecanvas " << get_ident() << " ";
        indent(+1);
        browser_node->save(st, TRUE, warning);
        nl_indent(st);

        if (name_in_tab != UmlDefaultState)
            st << "name_in_tab " << stringify(name_in_tab);

        if (show_context_mode != DefaultShowContextMode)
            st << " show_context_mode " << stringify(show_context_mode);

        if (show_stereotype_properties != UmlDefaultState)
            st << " show_stereotype_properties " << stringify(show_stereotype_properties);

        if (itscolor != UmlDefaultColor)
            st << " color " << stringify(itscolor);

        save_xyzwh(st, this, "  xyzwh");
        save_stereotype_property(st, warning);
        indent(-1);
        nl_indent(st);
        st << "end";
    }
}

PackageCanvas * PackageCanvas::read(char *& st, UmlCanvas * canvas, char * k)
{
    if (!strcmp(k, "packagecanvas_ref"))
        return ((PackageCanvas *) dict_get(read_id(st), "packagecanvas", canvas));
    else if (!strcmp(k, "packagecanvas")) {
        int id = read_id(st);
        BrowserNode * br = BrowserPackage::read_ref(st);
        UmlColor color = UmlDefaultColor;
        Uml3States in_tab = UmlDefaultState;
        ShowContextMode context = DefaultShowContextMode;
        Uml3States show_stereotype_properties = UmlDefaultState;;

        k = read_keyword(st);

        if (!strcmp(k, "name_in_tab")) {
            in_tab = state(read_keyword(st));
            k = read_keyword(st);
        }

        if (!strcmp(k, "show_context_mode")) {
            context = context_mode(read_keyword(st));
            k = read_keyword(st);
        }

        if (!strcmp(k, "show_stereotype_properties")) {
            show_stereotype_properties = state(read_keyword(st));
            k = read_keyword(st);
        }

        read_color(st, "color", color, k);

        if (strcmp(k, "xyzwh"))
            wrong_keyword(k, "xyzwh");

        int x = (int) read_double(st);
        PackageCanvas * result =
            new PackageCanvas(br, canvas, x, (int) read_double(st), id);

        read_zwh(st, result);

        if (read_file_format() >= 58) {
            k = read_keyword(st);
            result->read_stereotype_property(st, k);	// updates k

            if (strcmp(k, "end"))
                wrong_keyword(k, "end");
        }

        result->name_in_tab = in_tab;
        result->show_context_mode = context;
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

void PackageCanvas::history_hide()
{
    Q3CanvasItem::setVisible(FALSE);
    disconnect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
    disconnect(browser_node->get_data(), 0, this, 0);
}

void PackageCanvas::history_save(QBuffer & b) const
{
    DiagramCanvas::history_save(b);
    ::save(width_scale100, b);
    ::save(height_scale100, b);
    ::save(width(), b);
    ::save(height(), b);
}

void PackageCanvas::history_load(QBuffer & b)
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
