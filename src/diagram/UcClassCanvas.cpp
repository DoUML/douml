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
#include <QPixmap>

#include "UcClassCanvas.h"
#include "TemplateCanvas.h"
#include "RelationCanvas.h"
#include "DiagramView.h"
#include "RelationData.h"
#include "BasicData.h"
#include "UmlPixmap.h"
#include "UmlGlobal.h"
#include "BrowserClass.h"
#include "BrowserUseCaseDiagram.h"
#include "ClassData.h"
#include "PackageData.h"
#include "SettingsDialog.h"
#include "myio.h"
#include "ToolCom.h"
#include "Tool.h"
#include "ui/menufactory.h"
#include "strutil.h"
#include "ProfiledStereotypes.h"
#include "translate.h"

UcClassCanvas::UcClassCanvas(BrowserNode * bn, UmlCanvas * canvas,
                             int x, int y, int id)
    : DiagramCanvas(0, canvas, x, y, ACTOR_CANVAS_SIZE,
                    ACTOR_CANVAS_SIZE, id)
{
    browser_node = bn;
    templ = 0;	// may be updated by compute_size()

    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
    connect(bn->get_data(), SIGNAL(changed()), this, SLOT(modified()));
    connect(bn->get_data(), SIGNAL(deleted()), this, SLOT(deleted()));

    itscolor = UmlDefaultColor;

    compute_size();	// update used_settings

    if (id == 0) {
        // not on old read
        if (canvas->must_draw_all_relations()) {
            draw_all_depend_gene();
            draw_all_simple_relations();
        }

        check_stereotypeproperties();
    }
}

UcClassCanvas::UcClassCanvas(UmlCanvas * canvas, int id)
    : DiagramCanvas(canvas, id)
{
    // for read operation
    browser_node = 0;
    templ = 0;	// may be updated by compute_size()
    itscolor = UmlDefaultColor;
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
}

UcClassCanvas::~UcClassCanvas()
{
}

void UcClassCanvas::delete_it()
{
    disconnect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
    disconnect(browser_node->get_data(), 0, this, 0);

    if (templ)
        templ->delete_it();

    DiagramCanvas::delete_it();
}

void UcClassCanvas::deleted()
{
    delete_it();
    canvas()->update();
}

void UcClassCanvas::remove(bool from_model)
{
    if (! from_model)
        delete_it();
    else
        browser_node->delete_it();	// will remove canvas
}

void UcClassCanvas::compute_size()
{
    used_settings = settings;
    ((BrowserUseCaseDiagram *) the_canvas()->browser_diagram())
    ->get_simpleclassdiagramsettings(used_settings);

    full_name = browser_node->get_name();

    const WrapperStr & (PackageData::*f)() const = 0;
    const char * sep = 0;	// to avoid warning

    switch (used_settings.show_context_mode) {
    case umlContext:
        full_name = browser_node->full_name();
        break;

    case namespaceContext:
        f = &PackageData::get_cpp_namespace;
        sep = "::";
        break;

    case javaPackageContext:
        f = &PackageData::get_java_package;
        sep = ".";
        break;

    case pythonPackageContext:
        f = &PackageData::get_python_package;
        sep = ".";
        break;

    case moduleContext:
        f = &PackageData::get_idl_module;
        sep = "::";
        break;

    default:
        break;
    }

    if (f != 0) {
        BrowserClass * cl = (BrowserClass *) browser_node;

        while (cl->nestedp())
            cl = (BrowserClass *) cl->parent();

        if (((BrowserNode *) cl->parent())->get_type() == UmlClassView) {
            // not under a use case
            BrowserArtifact * cp = cl->get_associated_artifact();

            QString context =
                (((PackageData *)
                  ((BrowserNode *)
                   (((cp == 0) ? (BrowserNode *) cl : (BrowserNode *) cp)
                    ->parent()->parent()))->get_data())->*f)();

            if (!context.isEmpty())
                full_name = context + sep + full_name;
        }
    }

    QFontMetrics fm(the_canvas()->get_font(UmlNormalFont));
    QFontMetrics fim(the_canvas()->get_font(UmlNormalItalicFont));
    const ClassData * data = ((ClassData *) browser_node->get_data());
    int wi =
        (data->get_is_abstract()) ? fim.width(full_name) : fm.width(full_name);
    double zoom = the_canvas()->zoom();
    const QPixmap * px = 0;

    if (used_settings.class_drawing_mode == Natural) {
        if ((px = ProfiledStereotypes::diagramPixmap(data->get_stereotype(), zoom)) != 0)
            used_view_mode = Natural;
        else {
            const char * st = data->get_short_stereotype();

            if (!strcmp(st, "control"))
                used_view_mode = asControl;
            else if (!strcmp(st, "entity"))
                used_view_mode = asEntity;
            else if (!strcmp(st, "boundary"))
                used_view_mode = asBoundary;
            else if (!strcmp(st, "actor"))
                used_view_mode = asActor;
            else if (!strcmp(st, "interface"))
                used_view_mode = asInterface;
            else
                used_view_mode = asClass;
        }
    }
    else
        used_view_mode = used_settings.class_drawing_mode;

    const int eight = (int)(8 * zoom);

    int he = fm.height() + eight;

    bool tmpl = data->get_n_formalparams() != 0;

    if (tmpl)
        he += fm.height();

    if (used_view_mode == asClass) {
        he += (int)(12 * zoom);

        if (data->get_stereotype()[0]) {
            int stw = fm.width(QString("<<") + toUnicode(data->get_short_stereotype()) + ">>");

            if (wi < stw)
                wi = stw;

            he += fm.height() + (int)(4 * zoom);
        }
    }

    wi += eight;

    int min_w;

    switch (used_view_mode) {
    case asInterface:
        min_w = (int)(INTERFACE_SIZE * zoom);
        he += min_w;
        break;

    case asControl:
        min_w = (int)(CONTROL_WIDTH * zoom);
        he += (int)(CONTROL_HEIGHT * zoom);
        break;

    case asBoundary:
        min_w = (int)(BOUNDARY_WIDTH * zoom);
        he += (int)(BOUNDARY_HEIGHT * zoom);
        break;

    case asEntity:
        min_w = (int)(ENTITY_SIZE * zoom);
        he += min_w;
        break;

    case asActor:
        min_w = (int)(ACTOR_SIZE * zoom);
        he += (int)(ACTOR_SIZE * zoom);
        break;

    case Natural:
        // pixmap
        min_w = px->width();
        he += px->height();
        break;

    default:	// class
        min_w = (int)(CLASS_CANVAS_MIN_SIZE * zoom);
        break;
    }

    if (wi < min_w)
        wi = min_w;

    used_color = (itscolor == UmlDefaultColor)
                 ? the_canvas()->browser_diagram()->get_color(UmlClass)
                 : itscolor;

    if ((used_view_mode == asClass) && (used_color != UmlTransparent)) {
        const int shadow = the_canvas()->shadow();

        wi += shadow;
        he += shadow;
    }

    // force odd width and height for line alignment
    DiagramCanvas::resize(wi | 1, he | 1);

    if (tmpl) {
        if (templ == 0) {
            templ = new TemplateCanvas(this);
            templ->show();
        }
        else
            templ->update();
    }
    else if (templ != 0) {
        templ->delete_it();
        templ = 0;
    }
}

void UcClassCanvas::draw(QPainter & p)
{
    if (! visible()) return;

    p.setRenderHint(QPainter::Antialiasing, true);
    QRect r = rect();
    QFontMetrics fm(the_canvas()->get_font(UmlNormalFont));
    QFontMetrics fim(the_canvas()->get_font(UmlNormalItalicFont));
    QColor bckgrnd = p.backgroundColor();
    double zoom = the_canvas()->zoom();
    FILE * fp = svg();

    if (fp != 0)
        fputs("<g>\n", fp);

    p.setBackgroundMode((used_color == UmlTransparent) ? ::Qt::TransparentMode : ::Qt::OpaqueMode);

    QColor co = color(used_color);

    if (used_view_mode == asClass) {
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
    }

    const ClassData * data = ((ClassData *) browser_node->get_data());
    const int two = (int)(2 * zoom);
    int he = fm.height() + two;

    if (data->get_n_formalparams() != 0)
        r.setTop(r.top() + fm.height());

    switch (used_view_mode) {
    case asInterface:
        draw_interface_icon(p, r, used_color, zoom);
        r.setTop(r.top() + (int)(INTERFACE_SIZE * zoom) + two);
        break;

    case asControl:
        draw_control_icon(p, r, used_color, zoom);
        r.setTop(r.top() + (int)(CONTROL_HEIGHT * zoom) + two);
        break;

    case asBoundary:
        draw_boundary_icon(p, r, used_color, zoom);
        r.setTop(r.top() + (int)(BOUNDARY_HEIGHT * zoom) + two);
        break;

    case asEntity:
        draw_entity_icon(p, r, used_color, zoom);
        r.setTop(r.top() + (int)(ENTITY_SIZE * zoom) + two);
        break;

    case asActor: {
        QRect ra = r;

        ra.setHeight((int)(ACTOR_SIZE * zoom));
        ra.setLeft(ra.left() +
                   (int)((ra.width() - ACTOR_SIZE * zoom) / 2));
        ra.setWidth(ra.height());
        draw_actor(&p, ra);
    }

    r.setTop(r.top() + (int)(ACTOR_SIZE * zoom) + two);
    break;

    case Natural: {
        const QPixmap * px =
            ProfiledStereotypes::diagramPixmap(data->get_stereotype(), zoom);
        int lft = (px->width() < width()) ? r.x() + (width() - px->width()) / 2 : r.x();

        p.drawPixmap(lft, r.y(), *px);

        if (fp != 0)
            // pixmap not really exported in SVG
            fprintf(fp, "\t<rect fill=\"%s\" stroke=\"black\" stroke-width=\"1\" stroke-opacity=\"1\""
                    " x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" />\n",
                    svg_color(UmlBlack), lft, r.y(), px->width() - 1, px->height() - 1);

        r.setTop(r.top() + px->height());
    }
    break;

    default:	// class
        r.setTop(r.top() + two);

        if (data->get_stereotype()[0]) {
            p.setFont(the_canvas()->get_font(UmlNormalFont));
            p.drawText(r, ::Qt::AlignHCenter + ::Qt::AlignTop,
                       QString("<<") + toUnicode(data->get_short_stereotype()) + ">>");

            if (fp != 0)
                draw_text(r, ::Qt::AlignHCenter + ::Qt::AlignTop,
                          QString("<<") + toUnicode(data->get_short_stereotype()) + ">>",
                          p.font(), fp);

            r.setTop(r.top() + he + two);
        }
    }

    p.setBackgroundMode(::Qt::TransparentMode);
    p.setFont((data->get_is_abstract())
              ? the_canvas()->get_font(UmlNormalItalicFont)
              : the_canvas()->get_font(UmlNormalFont));
    p.drawText(r, ::Qt::AlignHCenter + ::Qt::AlignTop, full_name);

    if (fp != 0)
        draw_text(r, ::Qt::AlignHCenter + ::Qt::AlignTop, full_name, p.font(), fp);

    p.setFont(the_canvas()->get_font(UmlNormalFont));

    if (used_view_mode == asClass) {
        r.setTop(r.top() + he);
        p.drawLine(r.topLeft(), r.topRight());

        if (fp != 0)
            fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                    " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                    r.left(), r.top(), r.right(), r.top());

        r.setTop(r.top() + (int)(8 * zoom));
        p.drawLine(r.topLeft(), r.topRight());

        if (fp != 0)
            fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                    " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                    r.left(), r.top(), r.right(), r.top());
    }

    if (fp != 0)
        fputs("</g>\n", fp);

    p.setBackgroundColor(bckgrnd);

    if (selected())
        show_mark(p, rect());
}

void UcClassCanvas::modified()
{
    if (visible()) {
        hide();
        hide_lines();
        compute_size();
        recenter();
        show();
        update_show_lines();
        force_self_rel_visible();

        if (the_canvas()->must_draw_all_relations()) {
            draw_all_depend_gene();
            draw_all_simple_relations();
        }

        check_stereotypeproperties();
        canvas()->update();
        package_modified();
    }
}

bool UcClassCanvas::get_show_stereotype_properties() const
{
    switch (used_settings.show_stereotype_properties) {
    case UmlYes:
        return TRUE;

    case UmlNo:
        return FALSE;

    default:
        return the_canvas()->browser_diagram()->get_show_stereotype_properties();
    }
}

void UcClassCanvas::change_scale()
{
    Q3CanvasRectangle::setVisible(FALSE);
    compute_size();
    recenter();

    if (templ != 0)
        templ->update();

    Q3CanvasRectangle::setVisible(TRUE);
}

void UcClassCanvas::moveBy(double dx, double dy)
{
    DiagramCanvas::moveBy(dx, dy);

    if (templ != 0)
        templ->update();
}

void UcClassCanvas::set_z(double z)
{
    setZ(z);

    if (templ)
        templ->update();
}

void UcClassCanvas::post_loaded()
{
    force_self_rel_visible();

    if (the_canvas()->must_draw_all_relations()) {
        draw_all_depend_gene();
        draw_all_simple_relations();
    }
}

UmlCode UcClassCanvas::type() const
{
    return UmlClass;
}

void UcClassCanvas::delete_available(BooL & in_model, BooL & out_model) const
{
    out_model |= TRUE;
    in_model |= browser_node->is_writable();
}

bool UcClassCanvas::alignable() const
{
    return TRUE;
}

bool UcClassCanvas::copyable() const
{
    return selected();
}

void UcClassCanvas::open()
{
    browser_node->open(FALSE);
}

void UcClassCanvas::menu(const QPoint &)
{
    Q3PopupMenu m(0);
    Q3PopupMenu toolm(0);

    MenuFactory::createTitle(m, browser_node->get_data()->definition(FALSE, TRUE));
    m.insertSeparator();
    m.insertItem(TR("Upper"), 0);
    m.insertItem(TR("Lower"), 1);
    m.insertItem(TR("Go up"), 8);
    m.insertItem(TR("Go down"), 9);
    m.insertSeparator();
    m.insertItem(TR("Add related elements"), 10);
    m.insertSeparator();
    m.insertItem(TR("Edit drawing settings"), 4);
    m.insertSeparator();

    if (browser_node->is_writable()) {
        m.insertItem(TR("Edit"), 7);
        m.insertSeparator();
    }

    m.insertItem(TR("Select in browser"), 2);

    if (linked())
        m.insertItem(TR("Select linked items"), 3);

    m.insertSeparator();
    m.insertItem(TR("Remove from diagram"), 5);

    if (browser_node->is_writable())
        m.insertItem(TR("Delete from model"), 6);

    m.insertSeparator();

    if (Tool::menu_insert(&toolm, UmlClass, 20))
        m.insertItem(TR("Tool"), &toolm);

    int rank = m.exec(QCursor::pos());

    switch (rank) {
    case 0:
        upper();
        hide();
        show();
        break;

    case 1:
        lower();
        hide();
        show();
        break;

    case 2:
        browser_node->select_in_browser();
        return;

    case 3:
        the_canvas()->unselect_all();
        select_associated();
        return;

    case 4:
        edit_drawing_settings();
        return;

    case 5:
        // remove from diagram
        delete_it();
        break;

    case 6:
        // delete from model
        browser_node->delete_it();	// will delete the canvas
        break;

    case 7:
        browser_node->open(TRUE);
        break;

    case 8:
        z_up();
        hide();
        show();
        break;

    case 9:
        z_down();
        hide();
        show();
        break;

    case 10:
        ((UmlCanvas *) canvas())->get_view()
        ->add_related_elements(this, TR("class/actor"), TRUE, FALSE);
        return;

    default:
        if (rank >= 20)
            ToolCom::run(Tool::command(rank - 20), browser_node);

        return;
    }

    package_modified();
}

void UcClassCanvas::apply_shortcut(QString s)
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
        ->add_related_elements(this, TR("class/actor"), TRUE, FALSE);
        return;
    }
    else {
        browser_node->apply_shortcut(s);
        return;
    }

    modified();
    package_modified();
}

void UcClassCanvas::edit_drawing_settings()
{
    for (;;) {
        StateSpecVector st;
        ColorSpecVector co(1);

        settings.complete(st);

        co[0].set(TR("class color"), &itscolor);

        SettingsDialog dialog(&st, &co, FALSE);

        dialog.raise();

        if (dialog.exec() == QDialog::Accepted) {
            modified();
            package_modified();
        }

        if (!dialog.redo())
            break;
    }
}

bool UcClassCanvas::has_drawing_settings() const
{
    return TRUE;
}

void UcClassCanvas::edit_drawing_settings(QList<DiagramItem *> & l)
{
    for (;;) {
        StateSpecVector st;
        ColorSpecVector co(1);
        UmlColor itscolor;
        SimpleClassDiagramSettings settings;

        settings.complete(st);

        co[0].set(TR("class color"), &itscolor);

        SettingsDialog dialog(&st, &co, FALSE, TRUE);

        dialog.raise();

        if (dialog.exec() == QDialog::Accepted) {

            foreach (DiagramItem *item, l) {
                UcClassCanvas *canvas = (UcClassCanvas *)item;
                if (!co[0].name.isEmpty())
                    canvas->itscolor = itscolor;

                canvas->settings.set(st, 0);
                canvas->modified();
                canvas->package_modified();
            }
        }

        if (!dialog.redo())
            break;
    }
}

void UcClassCanvas::clone_drawing_settings(const DiagramItem *src)
{
    const UcClassCanvas * x = (const UcClassCanvas *) src;
    itscolor = x->itscolor;
    settings = x->settings;
    modified();
    package_modified();
}

QString UcClassCanvas::may_start(UmlCode & c) const
{
    switch (c) {
    case UmlDirectionalAssociation:
    case UmlAssociation:
    case UmlAnchor:
        return 0;

    case UmlGeneralisation:
    case UmlDependency:
        return (browser_node->is_writable()) ? QString() : TR("read only");

    default:
        return TR("illegal");
    }
}

QString UcClassCanvas::may_connect(UmlCode & l, const DiagramItem * dest) const
{
    if (l == UmlAnchor)
        return dest->may_start(l);

    switch (dest->type()) {
    case UmlUseCase:
        return ((l == UmlAssociation) || (l == UmlDirectionalAssociation))
               ? QString() : TR("illegal");

    case UmlClass:
        return ((l == UmlGeneralisation) || (l == UmlDependency))
               ? ((BrowserClass *) browser_node)
               ->may_connect(l, (BrowserClass *)((const UcClassCanvas *) dest)->browser_node)
               : TR("illegal");

    default:
        return TR("illegal");
    }
}

bool UcClassCanvas::has_relation(BasicData * def) const
{
    foreach (ArrowCanvas *line, lines) {
        if (IsaRelation(line->type()) &&
            (((RelationCanvas *) line)->get_data() == def))
            return TRUE;
    }

    return FALSE;
}

static bool dependencyOrGeneralization(UmlCode t)
{
    switch (t) {
    case UmlGeneralisation:
    case UmlDependency:
    case UmlRealize:
        return TRUE;

    default:
        return FALSE;
    }
}

void UcClassCanvas::draw_all_depend_gene(UcClassCanvas * end)
{
    Q3ListViewItem * child;
    Q3CanvasItemList all = canvas()->allItems();
    Q3CanvasItemList::Iterator cit;

    for (child = browser_node->firstChild(); child; child = child->nextSibling()) {
        if (dependencyOrGeneralization(((BrowserNode *) child)->get_type()) &&
            !((BrowserNode *) child)->deletedp()) {
            RelationData * def =
                ((RelationData *)((BrowserNode *) child)->get_data());

            if ((def->get_start_class() == browser_node) && 	// rel begins by this
                ((end == 0) || (def->get_end_class() == end->browser_node)) &&
                !has_relation(def)) {
                // adds it in case the other class is drawn
                BrowserClass * end_class =
                    ((BrowserClass *) def->get_end_class());
                DiagramItem * di;

                if (end_class == browser_node)
                    di = this;
                else {
                    di = 0;

                    for (cit = all.begin(); cit != all.end(); ++cit) {
                        DiagramItem * adi = QCanvasItemToDiagramItem(*cit);

                        if ((adi != 0) &&		// an uml canvas item
                            (adi->type() == UmlClass) &&
                            (((UcClassCanvas *) adi)->browser_node == end_class) &&
                            ((((UcClassCanvas *) adi) == end) || (*cit)->visible())) {
                            // other class canvas find
                            di = adi;
                            break;
                        }
                    }
                }

                if (di != 0)
                    (new RelationCanvas(the_canvas(), this, di,
                                        ((BrowserClass *) browser_node),
                                        def->get_type(), 0, -1.0, -1.0, def))
                    ->show();
            }
        }
    }

    if ((end == 0) &&
        !DrawingSettings::just_modified() &&
        !on_load_diagram()) {
        for (cit = all.begin(); cit != all.end(); ++cit) {
            DiagramItem * di = QCanvasItemToDiagramItem(*cit);

            if ((di != 0) &&	// an uml canvas item
                (di->type() == UmlClass) &&
                (((UcClassCanvas *) di) != this) &&
                !((UcClassCanvas *) di)->browser_node->deletedp() &&
                ((UcClassCanvas *) di)->visible())
                ((UcClassCanvas *) di)->draw_all_depend_gene(this);
        }
    }
}

void UcClassCanvas::connexion(UmlCode action, DiagramItem * dest,
                              const QPoint &, const QPoint &)
{
    ArrowCanvas * a;

    if ((dest->type() == UmlClass) && IsaRelation(action))
        a = new RelationCanvas(the_canvas(), this, dest, 0, action, 0, -1.0, -1.0);
    else
        a = new ArrowCanvas(the_canvas(), this, dest, action, 0, FALSE, -1.0, -1.0);

    a->show();
    the_canvas()->select(a);
}

bool UcClassCanvas::move_with_its_package() const
{
    return TRUE;
}

void UcClassCanvas::save(QTextStream & st, bool ref, QString & warning) const
{
    if (ref)
        st << "classcanvas_ref " << get_ident() << " // " << browser_node->get_name();
    else {
        nl_indent(st);
        st << "classcanvas " << get_ident() << " ";
        browser_node->save(st, TRUE, warning);
        indent(+1);
        settings.save(st);
        nl_indent(st);

        if (itscolor != UmlDefaultColor)
            st << "color " << stringify(itscolor) << " ";

        save_xyz(st, this, "xyz");
        save_stereotype_property(st, warning);
        indent(-1);
        nl_indent(st);
        st << "end";
    }
}

UcClassCanvas * UcClassCanvas::read(char *& st, UmlCanvas * canvas, char * k)
{
    if (!strcmp(k, "classcanvas_ref"))
        return ((UcClassCanvas *) dict_get(read_id(st), "classcanvas", canvas));
    else if (!strcmp(k, "classcanvas")) {
        int id = read_id(st);
        BrowserNode * br = BrowserClass::read_ref(st);
        UcClassCanvas * result;

        if (read_file_format() < 52) {
            read_keyword(st, "xyz");

            int x = (int) read_double(st);

            result =
                new UcClassCanvas(br, canvas, x, (int) read_double(st), id);
            result->setZ(read_double(st));

            // move the actor in its initial position
            x = ACTOR_CANVAS_SIZE - result->width();

            if (x < 0)
                result->Q3CanvasRectangle::moveBy(x / 2, 0);

            k = read_keyword(st);
            read_double(st);
            read_double(st);

            if (!strcmp(k, "label_xyz"))
                // old version
                read_double(st);
            else if (strcmp(k, "label_xy"))
                wrong_keyword(k, "label_xy/label_xyz");
        }
        else {
            result = new UcClassCanvas(canvas, id);
            result->browser_node = br;
            connect(br->get_data(), SIGNAL(changed()), result, SLOT(modified()));
            connect(br->get_data(), SIGNAL(deleted()), result, SLOT(deleted()));

            k = read_keyword(st);
            result->settings.read(st, k);	// updates k
            read_color(st, "color", result->itscolor, k);	// updates k

            if (strcmp(k, "xyz"))
                wrong_keyword(k, "xyz");

            read_xyz(st, result);

            result->compute_size();

            if ((read_file_format() < 72) &&
                (result->used_view_mode == asInterface)) {
                result->settings.class_drawing_mode = asClass;
                result->compute_size();
            }

            if (read_file_format() >= 58) {
                k = read_keyword(st);
                result->read_stereotype_property(st, k);

                if (strcmp(k, "end"))
                    wrong_keyword(k, "end");
            }
        }

        result->set_center100();

        result->show();
        result->check_stereotypeproperties();
        return result;
    }
    else
        return 0;
}

void UcClassCanvas::history_hide()
{
    Q3CanvasItem::setVisible(FALSE);

    disconnect(DrawingSettings::instance(), SIGNAL(changed()),
               this, SLOT(modified()));
    disconnect(browser_node->get_data(), 0, this, 0);
}

void UcClassCanvas::history_load(QBuffer & b)
{
    DiagramCanvas::history_load(b);
    connect(browser_node->get_data(), SIGNAL(changed()), this, SLOT(modified()));
    connect(browser_node->get_data(), SIGNAL(deleted()), this, SLOT(deleted()));
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
    compute_size();
}
