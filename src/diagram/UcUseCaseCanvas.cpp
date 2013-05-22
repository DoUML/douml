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





#include <qpainter.h>
#include <qcursor.h>
#include <q3popupmenu.h>
//Added by qt3to4:
#include <QTextStream>

#include "UcUseCaseCanvas.h"
#include "UcClassCanvas.h"
#include "SimpleRelationCanvas.h"
#include "DiagramView.h"
#include "BasicData.h"
#include "BrowserUseCase.h"
#include "UseCaseData.h"
#include "LabelCanvas.h"
#include "BrowserDiagram.h"
#include "UmlGlobal.h"
#include "SettingsDialog.h"
#include "myio.h"
#include "ToolCom.h"
#include "Tool.h"
#include "ui/menufactory.h"
#include "translate.h"

UcUseCaseCanvas::UcUseCaseCanvas(BrowserNode * bn, UmlCanvas * canvas,
                                 int x, int y, int id)
    : DiagramCanvas(bn, canvas, x, y, USECASE_CANVAS_MIN_WIDTH,
                    USECASE_CANVAS_MIN_HEIGHT, id),
    itscolor(UmlDefaultColor), used_color(UmlDefaultColor)
{
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
    connect(bn->get_data(), SIGNAL(changed()), this, SLOT(modified()));
    connect(bn->get_data(), SIGNAL(deleted()), this, SLOT(deleted()));

    if (id == 0) {
        // not on read
        if (canvas->must_draw_all_relations())
            draw_all_simple_relations();

        check_stereotypeproperties();
    }
}

UcUseCaseCanvas::~UcUseCaseCanvas()
{
}

void UcUseCaseCanvas::delete_it()
{
    disconnect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
    disconnect(browser_node->get_data(), 0, this, 0);
    DiagramCanvas::delete_it();
}

void UcUseCaseCanvas::deleted()
{
    delete_it();
}

void UcUseCaseCanvas::remove(bool from_model)
{
    if (! from_model)
        delete_it();
    else
        browser_node->delete_it();	// will remove canvas
}

void UcUseCaseCanvas::draw(QPainter & p)
{
    if (! visible()) return;

    QRect r = rect();
    UseCaseData * data = (UseCaseData *) browser_node->get_data();
    p.setRenderHint(QPainter::Antialiasing, true);
    used_color = (itscolor == UmlDefaultColor)
                 ? the_canvas()->browser_diagram()->get_color(UmlUseCase)
                 : itscolor;

    QColor col = color(used_color);
    QBrush brsh = p.brush();
    bool realizationp =
        !strcmp(data->get_short_stereotype(), "realization");
    FILE * fp = svg();
    int rx = width() / 2 - 1;
    int ry = height() / 2 - 1;

    if (fp != 0)
        fputs("<g>\n", fp);

    if (used_color != UmlTransparent) {
        const int shadow = the_canvas()->shadow() - 1;

        if (shadow != -1) {
            r.setRight(r.right() - shadow);
            r.setBottom(r.bottom() - shadow);
            p.setPen(::Qt::NoPen);
            p.setBrush(::Qt::darkGray);
            p.drawEllipse(r.left() + shadow, r.top() + shadow, r.width(), r.height());

            if (fp != 0) {
                rx = width() / 2 - 1;
                ry = height() / 2 - 1;

                fprintf(fp, "\t<ellipse fill=\"#%06x\" stroke=\"none\""
                        " cx=\"%d\" cy=\"%d\" rx=\"%d\" ry=\"%d\" />\n",
                        QColor(::Qt::darkGray).rgb() & 0xffffff,
                        r.left() + shadow + rx, r.top() + shadow + ry, rx, ry);
            }
        }

        if (fp != 0)
            fprintf(fp, "\t<ellipse fill=\"%s\" stroke=\"black\"%s stroke-width=\"1\" stroke-opacity=\"1\""
                    " cx=\"%d\" cy=\"%d\" rx=\"%d\" ry=\"%d\" />\n",
                    svg_color(used_color),
                    (realizationp) ? " stroke-dasharray=\"4,4\"" : "",
                    r.left() + rx, r.top() + ry, rx, ry);
    }
    else if (fp != 0)
        fprintf(fp, "\t<ellipse fill=\"none\" stroke=\"black\"%s stroke-width=\"1\" stroke-opacity=\"1\""
                " cx=\"%d\" cy=\"%d\" rx=\"%d\" ry=\"%d\" />\n",
                (realizationp) ? " stroke-dasharray=\"4,4\"" : "",
                r.left() + rx, r.top() + ry, rx, ry);

    p.setBackgroundMode((used_color == UmlTransparent) ? ::Qt::TransparentMode : ::Qt::OpaqueMode);
    p.setBrush(col);

    if (realizationp)
        p.setPen(::Qt::DotLine);
    else
        p.setPen(::Qt::SolidLine);

    p.drawEllipse(r.left(), r.top(), r.width(), r.height());

    if (realizationp)
        p.setPen(::Qt::SolidLine);

    QString ep = data->get_extension_points();

    if (!ep.isEmpty()) {
        QFontMetrics fbm(the_canvas()->get_font(UmlNormalBoldFont));
        const int two = (int)(2 * the_canvas()->zoom());
        int he = fbm.height() + two;
        int py = (int) y() + height() / 4;
        int dx = width() / 15;
        int px = (int) x() + dx;

        p.drawLine(px, py, r.right() - dx, py);

        if (fp != 0)
            fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                    " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                    px, py, r.right() - dx, py);

        int h = (height() * 3) / 4 - two;
        int w = width() - dx - dx;
        QColor bckgrnd = p.backgroundColor();

        py += two;
        p.setBackgroundColor(col);
        p.setFont(the_canvas()->get_font(UmlNormalBoldFont));

        p.drawText(px, py, w, h, ::Qt::AlignHCenter + ::Qt::AlignTop,
                   "Extension Points");

        if (fp != 0)
            draw_text(px, py, w, h, ::Qt::AlignHCenter + ::Qt::AlignTop,
                      "Extension Points", p.font(), fp);

        h -= he;

        if (h > he) {
            // at least one line may be written
            py += he;
            p.setFont(the_canvas()->get_font(UmlNormalFont));
            p.drawText(px, py, w, h, ::Qt::AlignCenter, ep);
            p.setBackgroundColor(bckgrnd);

            if (fp != 0)
                draw_text(px, py, w, h, ::Qt::AlignCenter, ep, p.font(), fp);
        }
    }

    if (fp != 0)
        fputs("</g>\n", fp);

    p.setBrush(brsh);

    if (selected())
        show_mark(p, rect());
}

UmlCode UcUseCaseCanvas::type() const
{
    return UmlUseCase;
}

void UcUseCaseCanvas::delete_available(BooL & in_model, BooL & out_model) const
{
    out_model |= TRUE;
    in_model |= browser_node->is_writable();
}

bool UcUseCaseCanvas::alignable() const
{
    return TRUE;
}

bool UcUseCaseCanvas::copyable() const
{
    return selected();
}

void UcUseCaseCanvas::open()
{
    browser_node->open(FALSE);
}

void UcUseCaseCanvas::update_name()
{
    QString oldname = label->get_name();
    const char * st = browser_node->get_data()->get_short_stereotype();
    QString newname;

    if ((*st == 0) || !strcmp(st, "realization"))
        newname = browser_node->get_name();
    else
        newname = QString("<<") + st + QString(">>\n") + browser_node->get_name();

    if (newname != oldname) {
        label->set_name(newname);
        label->recenter();
    }
}

void UcUseCaseCanvas::modified()
{
    used_color = (itscolor == UmlDefaultColor)
                 ? the_canvas()->browser_diagram()->get_color(UmlUseCase)
                 : itscolor;

    update_name();

    // force son reaffichage
    hide();
    show();
    force_self_rel_visible();

    if (the_canvas()->must_draw_all_relations())
        draw_all_simple_relations();

    check_stereotypeproperties();
    canvas()->update();
    package_modified();
}

void UcUseCaseCanvas::post_loaded()
{
    force_self_rel_visible();

    if (the_canvas()->must_draw_all_relations())
        draw_all_simple_relations();
}

void UcUseCaseCanvas::menu(const QPoint &)
{
    Q3PopupMenu m(0);
    Q3PopupMenu toolm(0);

    MenuFactory::createTitle(m, browser_node->get_data()->definition(FALSE, TRUE));
    m.insertSeparator();
    m.insertItem("Upper", 0);
    m.insertItem("Lower", 1);
    m.insertItem("Go up", 13);
    m.insertItem("Go down", 14);
    m.insertSeparator();
    m.insertItem("Add related elements", 10);
    m.insertSeparator();
    m.insertItem("Edit", 2);
    m.insertSeparator();
    m.insertItem("Edit drawing settings", 3);
    m.insertSeparator();
    m.insertItem("Select in browser", 4);

    if (linked())
        m.insertItem("Select linked items", 5);

    m.insertSeparator();

    if (browser_node->is_writable()) {
        if (browser_node->get_associated() !=
            (BrowserNode *) the_canvas()->browser_diagram())
            m.insertItem("Set associated diagram", 6);

        if (browser_node->get_associated())
            m.insertItem("Remove diagram association", 9);
    }

    m.insertSeparator();
    m.insertItem("Remove from diagram", 7);

    if (browser_node->is_writable())
        m.insertItem("Delete from model", 8);

    m.insertSeparator();

    if (Tool::menu_insert(&toolm, UmlUseCase, 20))
        m.insertItem("Tool", &toolm);

    int rank = m.exec(QCursor::pos());

    switch (rank) {
    case 0:
        upper();
        modified();
        return;

    case 1:
        lower();
        modified();
        return;

    case 13:
        z_up();
        modified();
        return;

    case 14:
        z_down();
        modified();
        return;

    case 2:
        browser_node->open(TRUE);
        break;

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
        ((BrowserUseCase *) browser_node)->set_associated_diagram((BrowserUseCaseDiagram *)
                the_canvas()->browser_diagram());
        break;

    case 12:
        ((BrowserUseCase *) browser_node)
        ->set_associated_diagram(0);
        return;

    case 7:
        // remove from diagram
        delete_it();
        break;

    case 8:
        // delete from model
        browser_node->delete_it();	// will remove canvas
        break;

    case 10:
        ((UmlCanvas *) canvas())->get_view()
        ->add_related_elements(this, TR("use case"), TRUE, FALSE);
        return;

    default:
        if (rank >= 20)
            ToolCom::run(Tool::command(rank - 20), browser_node);

        return;
    }

    package_modified();
}

void UcUseCaseCanvas::apply_shortcut(QString s)
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
        ->add_related_elements(this, TR("use case"), TRUE, FALSE);
        return;
    }
    else {
        browser_node->apply_shortcut(s);
        return;
    }

    modified();
    package_modified();
}

void UcUseCaseCanvas::edit_drawing_settings()
{
    for (;;) {
        ColorSpecVector co(1);

        co[0].set("use case color", &itscolor);

        SettingsDialog dialog(0, &co, FALSE);

        dialog.raise();

        if (dialog.exec() == QDialog::Accepted)
            modified();

        if (!dialog.redo())
            break;
    }
}

bool UcUseCaseCanvas::has_drawing_settings() const
{
    return TRUE;
}

void UcUseCaseCanvas::edit_drawing_settings(QList<DiagramItem *> & l)
{
    for (;;) {
        ColorSpecVector co(1);
        UmlColor itscolor;

        co[0].set("use case color", &itscolor);

        SettingsDialog dialog(0, &co, FALSE, TRUE);

        dialog.raise();

        if ((dialog.exec() == QDialog::Accepted) && !co[0].name.isEmpty()) {
            foreach (DiagramItem *item, l) {
                UcUseCaseCanvas *canvas = (UcUseCaseCanvas *)item;
                canvas->itscolor = itscolor;
                canvas->modified();	// call package_modified()
            }
        }

        if (!dialog.redo())
            break;
    }
}

void UcUseCaseCanvas::clone_drawing_settings(const DiagramItem *src)
{
    const UcUseCaseCanvas * x = (const UcUseCaseCanvas *) src;
    itscolor = x->itscolor;
    modified();
}

QString UcUseCaseCanvas::may_start(UmlCode & l) const
{
    switch (l) {
    case UmlDependency:
        l = UmlDependOn;
        return (browser_node->is_writable()) ? QString() : TR("read only");

    case UmlGeneralisation:
        l = UmlInherit;
        return (browser_node->is_writable()) ? QString() : TR("read only");

    case UmlAnchor:
    case UmlAssociation:
    case UmlDirectionalAssociation:
        return 0;

    default:
        return TR("illegal");
    }
}

QString UcUseCaseCanvas::may_connect(UmlCode & l, const DiagramItem * dest) const
{
    if (l == UmlAnchor)
        return dest->may_start(l);

    switch (dest->type()) {
    case UmlUseCase:
        switch (l) {
        case UmlInherit:
            return browser_node->check_inherit(dest->get_bn());

        case UmlDependOn:
        case UmlAssociation:
        case UmlDirectionalAssociation:
            return 0;

        default:
            return TR("illegal");
        }

    case UmlClass:
        return ((l == UmlAssociation) || (l == UmlDirectionalAssociation))
               ? QString() : TR("illegal");

    default:
        return TR("illegal");
    }
}

void UcUseCaseCanvas::connexion(UmlCode action, DiagramItem * dest,
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

aCorner UcUseCaseCanvas::on_resize_point(const QPoint & p)
{
    return ::on_resize_point(p, rect());
}

void UcUseCaseCanvas::resize(aCorner c, int dx, int dy, QPoint & o)
{
    DiagramCanvas::resize(c, dx, dy, o,
                          (int)(USECASE_CANVAS_MIN_WIDTH * the_canvas()->zoom()),
                          (int)(USECASE_CANVAS_MIN_HEIGHT * the_canvas()->zoom()),
                          TRUE);
}

void UcUseCaseCanvas::resize(const QSize & sz, bool w, bool h)
{
    DiagramCanvas::resize(sz, w, h,
                          (int)(USECASE_CANVAS_MIN_WIDTH * the_canvas()->zoom()),
                          (int)(USECASE_CANVAS_MIN_HEIGHT * the_canvas()->zoom()),
                          TRUE);
}

void UcUseCaseCanvas::save(QTextStream & st, bool ref, QString & warning) const
{
    if (ref)
        st << "usecasecanvas_ref " << get_ident() << " // " << browser_node->get_name();
    else {
        nl_indent(st);
        st << "usecasecanvas " << get_ident() << " ";
        browser_node->save(st, TRUE, warning);

        if (itscolor != UmlDefaultColor) {
            nl_indent(st);
            st << "  color " << stringify(itscolor);
        }

        indent(+1);
        nl_indent(st);
        save_xyzwh(st, this, "xyzwh");
        save_xy(st, label, " label_xy");
        save_stereotype_property(st, warning);
        indent(-1);
        nl_indent(st);
        st << "end";
    }
}

bool UcUseCaseCanvas::move_with_its_package() const
{
    return TRUE;
}

UcUseCaseCanvas * UcUseCaseCanvas::read(char *& st, UmlCanvas * canvas, char * k)
{
    if (!strcmp(k, "usecasecanvas_ref"))
        return ((UcUseCaseCanvas *) dict_get(read_id(st), "usecasecanvas", canvas));
    else if (!strcmp(k, "usecasecanvas")) {
        int id = read_id(st);
        BrowserNode * br = BrowserUseCase::read_ref(st);
        UmlColor color = UmlDefaultColor;

        k = read_keyword(st);
        read_color(st, "color", color, k);

        if (strcmp(k, "xyzwh"))
            wrong_keyword(k, "xyzwh");

        int x = (int) read_double(st);
        UcUseCaseCanvas * result =
            new UcUseCaseCanvas(br, canvas, x, (int) read_double(st), id);

        result->itscolor = color;
        read_zwh(st, result);
        result->width_scale100 = result->width();
        result->height_scale100 = result->height();
        result->set_center100();

        k = read_keyword(st);
        result->update_name();
        read_xy(st, result->label);
        result->label->setZ(result->z() + 0.5);
        result->label->set_center100();

        if (!strcmp(k, "label_xyz"))
            // old version
            read_double(st);
        else if (strcmp(k, "label_xy"))
            wrong_keyword(k, "label_xy/label_xyz");

        if (read_file_format() >= 58) {
            k = read_keyword(st);
            result->read_stereotype_property(st, k);	// updates k

            if (strcmp(k, "end"))
                wrong_keyword(k, "end");
        }

        result->show();
        result->check_stereotypeproperties();
        return result;
    }
    else
        return 0;
}

void UcUseCaseCanvas::history_hide()
{
    Q3CanvasItem::setVisible(FALSE);
    disconnect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
    disconnect(browser_node->get_data(), 0, this, 0);
}

void UcUseCaseCanvas::history_save(QBuffer & b) const
{
    DiagramCanvas::history_save(b);
    ::save(width_scale100, b);
    ::save(height_scale100, b);
    ::save(width(), b);
    ::save(height(), b);
}

void UcUseCaseCanvas::history_load(QBuffer & b)
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

// for plug outs

void UcUseCaseCanvas::send(ToolCom * com, Q3CanvasItemList & all)
{
    QList<UcUseCaseCanvas *> lu;
    QList<UcClassCanvas *> la;
    Q3CanvasItemList::Iterator cit;

    for (cit = all.begin(); cit != all.end(); ++cit) {
        DiagramItem * di = QCanvasItemToDiagramItem(*cit);

        if ((di != 0) && (*cit)->visible()) {
            switch (di->type()) {
            case UmlUseCase:
                lu.append((UcUseCaseCanvas *) di);
                break;

            case UmlClass:
                la.append((UcClassCanvas *) di);
                break;

            default:
                break;
            }
        }
    }

    // send UC

    com->write_unsigned(lu.count());

    foreach (UcUseCaseCanvas *canvas, lu) {
        com->write_unsigned((unsigned) canvas->get_ident());
        canvas->get_bn()->write_id(com);
        com->write(canvas->rect());
    }

    // send Actors

    com->write_unsigned(la.count());

    foreach (UcClassCanvas *canvas, la)
        canvas->get_bn()->write_id(com);

    // send rels

    QList<ArrowCanvas *> lr;

    foreach (UcUseCaseCanvas *canvas, lu) {
        foreach (ArrowCanvas *r, canvas->lines) {
            DiagramItem * from = r->get_start();
            DiagramItem * to = r->get_end();

            if ((from->type() == UmlUseCase)
                ? (to->type() == UmlClass)
                : (from->type() == UmlClass))
                lr.append(r);
        }
    }

    com->write_unsigned(lr.count());

    foreach (ArrowCanvas *arrow, lr)
        arrow->write_uc_rel(com);
}
