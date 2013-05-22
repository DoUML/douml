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
#include <Q3PointArray>
#include <QPixmap>

#include "ArtifactCanvas.h"
#include "AssocContainCanvas.h"
#include "SimpleRelationCanvas.h"
#include "ArtifactData.h"
#include "DiagramView.h"
#include "BrowserArtifact.h"
#include "BrowserDeploymentDiagram.h"
#include "UmlPixmap.h"
#include "UmlGlobal.h"
#include "SettingsDialog.h"
#include "myio.h"
#include "ToolCom.h"
#include "Tool.h"
#include "ui/menufactory.h"
#include "Settings.h"
#include "strutil.h"
#include "GenerationSettings.h"
#include "ProfiledStereotypes.h"
#include "translate.h"

ArtifactCanvas::ArtifactCanvas(BrowserNode * bn, UmlCanvas * canvas,
                               int x, int y)
    : DiagramCanvas(0, canvas, x, y, ARTIFACT_CANVAS_MIN_SIZE, 1, 0)
{
    browser_node = bn;
    itscolor = UmlDefaultColor;

    compute_size();
    check_stereotypeproperties();

    connect(bn->get_data(), SIGNAL(changed()), this, SLOT(modified()));
    connect(bn->get_data(), SIGNAL(deleted()), this, SLOT(deleted()));
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));

    if (canvas->must_draw_all_relations()) {
        draw_all_relations();
        draw_all_simple_relations();
    }
}

ArtifactCanvas::ArtifactCanvas(UmlCanvas * canvas, int id)
    : DiagramCanvas(canvas, id)
{
    // for read operation
    browser_node = 0;
    itscolor = UmlDefaultColor;
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
}

ArtifactCanvas::~ArtifactCanvas()
{
}

void ArtifactCanvas::delete_it()
{
    disconnect(DrawingSettings::instance(), SIGNAL(changed()),
               this, SLOT(modified()));
    disconnect(browser_node->get_data(), 0, this, 0);
    DiagramCanvas::delete_it();
}

void ArtifactCanvas::deleted()
{
    delete_it();
    canvas()->update();
}

void ArtifactCanvas::remove(bool from_model)
{
    if (! from_model)
        delete_it();
    else
        browser_node->delete_it();	// will remove canvas
}

void ArtifactCanvas::compute_size()
{
    used_color = (itscolor == UmlDefaultColor)
                 ? the_canvas()->browser_diagram()->get_color(UmlArtifact)
                 : itscolor;

    const BasicData * data = browser_node->get_data();
    QFontMetrics fm(the_canvas()->get_font(UmlNormalBoldFont));
    int he;
    int wi;

    const QPixmap * px =
        ProfiledStereotypes::diagramPixmap(data->get_stereotype(),
                                           the_canvas()->zoom());

    if (px != 0) {
        wi = fm.width(browser_node->get_name());

        if (wi < px->width())
            wi = px->width();

        he = px->height() + fm.height();
    }
    else {
        // <<artifact>>/stereotype on 2*font_height with the icon on the right
        // the icon height = 2*font_height
        // the icon width = 3*height/4
        // name on font_height+4 points
        const int four = (int)(4 * the_canvas()->zoom());
        int stw = fm.width((data->get_stereotype()[0])
                           ? (QString("<<") + toUnicode(data->get_short_stereotype()) + ">>")
                           : QString("<<artifact>>"))
                  + 3 * fm.height();


        he = 3 * fm.height() + four;
        wi = fm.width(browser_node->get_name());

        if (wi < stw)
            wi = stw;

        wi += four * 2;

        int min_w = (int)(ARTIFACT_CANVAS_MIN_SIZE * the_canvas()->zoom());

        if (wi < min_w)
            wi = min_w;

        if (used_color != UmlTransparent) {
            const int shadow = the_canvas()->shadow();

            wi += shadow;
            he += shadow;
        }
    }

    // force odd width and height for line alignment
    DiagramCanvas::resize(wi | 1, he | 1);
}

void ArtifactCanvas::change_scale()
{
    Q3CanvasRectangle::setVisible(FALSE);
    compute_size();
    recenter();
    Q3CanvasRectangle::setVisible(TRUE);
}

void ArtifactCanvas::modified()
{
    hide();
    hide_lines();
    compute_size();
    show();
    update_show_lines();
    check_stereotypeproperties();

    if (the_canvas()->must_draw_all_relations()) {
        draw_all_relations();
        draw_all_simple_relations();
    }

    canvas()->update();
    package_modified();
}

void ArtifactCanvas::post_loaded()
{
    if (the_canvas()->must_draw_all_relations()) {
        draw_all_relations();
        draw_all_simple_relations();
    }
}

void ArtifactCanvas::connexion(UmlCode action, DiagramItem * dest,
                               const QPoint &, const QPoint &)
{
    ArrowCanvas * a;

    if (action == UmlContain)
        a = new AssocContainCanvas(the_canvas(), this, dest, 0, -1.0, -1.0);
    else if (IsaSimpleRelation(action))
        a = new SimpleRelationCanvas(the_canvas(), this, dest, 0, action, 0, -1.0, -1.0);
    else
        a = new ArrowCanvas(the_canvas(), this, dest, action, 0, FALSE, -1.0, -1.0);

    a->show();
    the_canvas()->select(a);
}

void ArtifactCanvas::post_connexion(UmlCode action, DiagramItem * dest)
{
    if (action == UmlContain) {
        UmlCanvas * canvas = the_canvas();

        canvas->freeze_draw_all_relations();

        ((ArtifactData *) browser_node->get_data())
        ->associate((BrowserArtifact *)
                    ((ArtifactCanvas *) dest)->browser_node);

        canvas->unfreeze_draw_all_relations();
    }
}

void ArtifactCanvas::unassociate(DiagramItem * other)
{
    ((ArtifactData *) browser_node->get_data())
    ->unassociate((BrowserArtifact *)
                  ((ArtifactCanvas *) other)->browser_node);
}

void ArtifactCanvas::draw_all_relations()
{
    if (strcmp(browser_node->get_stereotype(), "source") != 0)
        // may start association
        update_relations();
    else if (!DrawingSettings::just_modified() &&
             !on_load_diagram()) {
        // remove all association starting from 'this'
        foreach (ArrowCanvas *canvas, lines) {
            if ((canvas->type() == UmlContain) &&
                (((AssocContainCanvas *) canvas)->get_start() == this))
                canvas->delete_it();
        }

        // update non source artifact vis a vis 'this'
        Q3CanvasItemList all = canvas()->allItems();
        Q3CanvasItemList::Iterator cit;

        for (cit = all.begin(); cit != all.end(); ++cit) {
            if ((*cit)->visible()) {
                DiagramItem * adi = QCanvasItemToDiagramItem(*cit);

                if ((adi != 0) &&		// an uml canvas item
                    (adi->type() == UmlArtifact) &&
                    strcmp(((ArtifactCanvas *) adi)->browser_node->get_stereotype(), "source"))
                    ((ArtifactCanvas *) adi)->update_relations(this);
            }
        }
    }
}

void ArtifactCanvas::update_relations(ArtifactCanvas * other)
{
    // 'this' is a non source, check association with 'other'
    const Q3PtrDict<BrowserArtifact> * associated =
        ((ArtifactData *) browser_node->get_data())->get_associated();
    bool association_must_exist =
        ((associated != 0) &&
         (associated->find((BrowserArtifact *) other->browser_node) != 0));

    foreach (ArrowCanvas *canvas, lines) {
        if ((canvas->type() == UmlContain) &&
            (((AssocContainCanvas *) canvas)->get_end() == other)) {
            if (! association_must_exist)
                canvas->delete_it();

            return;
        }
    }

    // association not yet exist

    if (association_must_exist)
        (new AssocContainCanvas(the_canvas(), this, other, 0, -1.0, -1.0))
        ->show();
}

void ArtifactCanvas::update_relations()
{
    // 'this' is a non source, check its associations
    const Q3PtrDict<BrowserArtifact> * associated =
        ((ArtifactData *) browser_node->get_data())->get_associated();
    Q3PtrDict<BrowserArtifact> associations;
    foreach (ArrowCanvas *canvas, lines) {
        if ((canvas->type() == UmlContain) &&
            (((AssocContainCanvas *)canvas)->get_start() == this)) {
            DiagramItem * adi = ((AssocContainCanvas *) canvas)->get_end();

            if ((adi->type() == UmlArtifact) &&
                (associated != 0) &&
                (associated->find((BrowserArtifact *)
                                  ((ArtifactCanvas *) adi)->browser_node) != 0)) {
                // association must exist
                BrowserArtifact * c = (BrowserArtifact *)
                                      ((ArtifactCanvas *) adi)->browser_node;

                associations.insert(c, c);
            }
            else
                // association must not exist
                canvas->delete_it();
        }
    }

    if (associated != 0) {
        Q3PtrDictIterator<BrowserArtifact> it(*associated);

        while (it.current()) {
            if (associations.find(it.current()) == 0) {
                // the association to 'it.current()' is not yet drawn

                // search 'it.current()' cancas
                Q3CanvasItemList all = canvas()->allItems();
                Q3CanvasItemList::Iterator cit;

                for (cit = all.begin(); cit != all.end(); ++cit) {
                    if ((*cit)->visible()) {
                        DiagramItem * adi = QCanvasItemToDiagramItem(*cit);

                        if ((adi != 0) &&		// an uml canvas item
                            (adi->type() == UmlArtifact) &&
                            (((ArtifactCanvas *) adi)->browser_node == it.current())) {
                            // find
                            (new AssocContainCanvas(the_canvas(), this, adi, 0, -1.0, -1.0))
                            ->show();
                            break;
                        }
                    }
                }
            }

            ++it;
        }
    }
}

void ArtifactCanvas::draw(QPainter & p)
{
    if (! visible()) return;

    p.setRenderHint(QPainter::Antialiasing, true);
    QRect r = rect();
    const BasicData * data = browser_node->get_data();
    FILE * fp = svg();

    if (fp != 0)
        fputs("<g>\n", fp);

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

        r.moveBy(0, px->height());
        p.setFont(the_canvas()->get_font(UmlNormalBoldFont));
        p.drawText(r, ::Qt::AlignHCenter, browser_node->get_name());

        if (fp != 0) {
            draw_text(r, ::Qt::AlignHCenter, browser_node->get_name(), p.font(), fp);
            fputs("</g>\n", fp);
        }
    }
    else {
        QColor bckgrnd = p.backgroundColor();

        p.setBackgroundMode((used_color == UmlTransparent) ? ::Qt::TransparentMode : ::Qt::OpaqueMode);

        QColor co = color(used_color);

        p.setBackgroundColor(co);

        // <<artifact>>/stereotype on 2*font_height with the icon on the right
        // the icon height = 2*font_height
        // the icon width = 3*height/4
        // name on font_height+4 points
        const int four = (int)(4 * the_canvas()->zoom());
        QFontMetrics fm(the_canvas()->get_font(UmlNormalBoldFont));
        const int he = fm.height();

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
            p.drawText(r, ::Qt::AlignCenter, "<<artifact>>");

            if (fp != 0)
                draw_text(r, ::Qt::AlignCenter, "<<artifact>>",
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
        re.setLeft(re.right() - 6 * he / 4 + four);
        re.setRight(re.right() - four);
        re.setTop(re.top() + four);
        re.setHeight(2 * (he - four));

        Q3PointArray a(7);
        const int corner_size = re.width() / 3;

        a.setPoint(0, re.left(), re.top());
        a.setPoint(1, re.right() - corner_size, re.top());
        a.setPoint(2, re.right() - corner_size, re.top() + corner_size);
        a.setPoint(3, re.right(), re.top() + corner_size);
        a.setPoint(4, re.right(), re.bottom());
        a.setPoint(5, re.left(), re.bottom());
        a.setPoint(6, re.left(), re.top());
        p.drawPolyline(a);
        //p.moveTo(re.right() - corner_size, re.top());
        //p.lineTo(re.right(), re.top() + corner_size);
        p.drawLine(re.right() - corner_size, re.top(), re.right(), re.top() + corner_size);

        if (fp != 0) {
            draw_poly(fp, a, UmlTransparent);
            fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                    " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n"
                    "</g>\n",
                    re.right() - corner_size, re.top(), re.right(), re.top() + corner_size);
        }

        p.setBackgroundColor(bckgrnd);
    }

    if (selected())
        show_mark(p, rect());
}

UmlCode ArtifactCanvas::type() const
{
    return UmlArtifact;
}

void ArtifactCanvas::delete_available(BooL & in_model, BooL & out_model) const
{
    out_model |= TRUE;
    in_model |= browser_node->is_writable();
}

bool ArtifactCanvas::alignable() const
{
    return TRUE;
}

bool ArtifactCanvas::copyable() const
{
    return selected();
}

void ArtifactCanvas::open()
{
    browser_node->open(FALSE);
}

void ArtifactCanvas::menu(const QPoint &)
{
    Q3PopupMenu m(0);
    Q3PopupMenu gensubm(0);
    Q3PopupMenu toolm(0);
    int index;

    MenuFactory::createTitle(m, browser_node->get_data()->definition(FALSE, TRUE));
    m.insertSeparator();
    m.insertItem(TR("Upper"), 0);
    m.insertItem(TR("Lower"), 1);
    m.insertItem(TR("Go up"), 15);
    m.insertItem(TR("Go down"), 16);
    m.insertSeparator();
    m.insertItem(TR("Add related elements"), 17);
    m.insertSeparator();
    m.insertItem(TR("Edit drawing settings"), 2);
    m.insertSeparator();
    m.insertItem(TR("Edit artifact"), 3);
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
            m.insertItem(TR("Remove diagram association"), 12);
    }

    m.insertSeparator();
    m.insertItem(TR("Remove from diagram"), 7);

    if (browser_node->is_writable())
        m.insertItem(TR("Delete from model"), 8);

    m.insertSeparator();

    bool cpp = GenerationSettings::cpp_get_default_defs();
    bool java = GenerationSettings::java_get_default_defs();
    bool php = GenerationSettings::php_get_default_defs();
    bool python = GenerationSettings::python_get_default_defs();
    bool idl = GenerationSettings::idl_get_default_defs();

    if (cpp || java || php || python || idl)
        m.insertItem(TR("Generate"), &gensubm);

    if (Tool::menu_insert(&toolm, UmlArtifact, 20))
        m.insertItem(TR("Tool"), &toolm);

    if (cpp)
        gensubm.insertItem("C++", 9);

    if (java)
        gensubm.insertItem("Java", 10);

    if (php)
        gensubm.insertItem("Php", 13);

    if (python)
        gensubm.insertItem("Python", 14);

    if (idl)
        gensubm.insertItem("Idl", 11);

    switch (index = m.exec(QCursor::pos())) {
    case 0:
        upper();
        modified();	// call package_modified()
        return;

    case 1:
        lower();
        modified();	// call package_modified()
        return;

    case 15:
        z_up();
        modified();	// call package_modified()
        return;

    case 16:
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
        ((BrowserArtifact *) browser_node)
        ->set_associated_diagram((BrowserDeploymentDiagram *)
                                 the_canvas()->browser_diagram());
        return;

    case 12:
        ((BrowserArtifact *) browser_node)
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
        browser_node->apply_shortcut("Generate C++");
        return;

    case 10:
        browser_node->apply_shortcut("Generate Java");
        return;

    case 11:
        browser_node->apply_shortcut("Generate Idl");
        return;

    case 13:
        browser_node->apply_shortcut("Generate Php");
        return;

    case 14:
        browser_node->apply_shortcut("Generate Python");
        return;

    case 17:
        ((UmlCanvas *) canvas())->get_view()
        ->add_related_elements(this, TR("artifact"), TRUE, FALSE);
        return;

    default:
        if (index >= 20)
            ToolCom::run(Tool::command(index - 20), browser_node);

        return;
    }

    package_modified();
}

void ArtifactCanvas::apply_shortcut(QString s)
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
        ->add_related_elements(this, TR("artifact"), TRUE, FALSE);
        return;
    }
    else {
        browser_node->apply_shortcut(s);
        return;
    }

    modified();
    package_modified();
}

void ArtifactCanvas::edit_drawing_settings()
{
    for (;;) {
        StateSpecVector st;
        ColorSpecVector co(1);

        co[0].set(TR("artifact color"), &itscolor);

        SettingsDialog dialog(0, &co, FALSE);

        dialog.raise();

        if (dialog.exec() == QDialog::Accepted)
            modified();	// call package_modified()

        if (!dialog.redo())
            break;
    }
}

bool ArtifactCanvas::has_drawing_settings() const
{
    return TRUE;
}

void ArtifactCanvas::edit_drawing_settings(QList<DiagramItem *> & l)
{
    for (;;) {
        ColorSpecVector co(1);
        UmlColor itscolor;

        co[0].set(TR("artifact color"), &itscolor);

        SettingsDialog dialog(0, &co, FALSE, TRUE);

        dialog.raise();

        if ((dialog.exec() == QDialog::Accepted) && !co[0].name.isEmpty()) {
            foreach (DiagramItem *item, l) {
                ArtifactCanvas *canvas = (ArtifactCanvas *)item;
                canvas->itscolor = itscolor;
                canvas->modified();
            }
        }

        if (!dialog.redo())
            break;
    }
}

void ArtifactCanvas::clone_drawing_settings(const DiagramItem *src)
{
    const ArtifactCanvas * x = (const ArtifactCanvas *) src;
    itscolor = x->itscolor;
    modified();
}

QString ArtifactCanvas::may_start(UmlCode & l) const
{
    switch (l) {
    case UmlContain:
        if (!browser_node->is_writable())
            return TR("read only");
        else if (strcmp(browser_node->get_stereotype(), "source") == 0)
            return TR("illegal for a source artifact");
        else
            return 0;

    case UmlDependency:
        l = UmlDependOn;
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

QString ArtifactCanvas::may_connect(UmlCode & l, const DiagramItem * dest) const
{
    if (l == UmlAnchor)
        return dest->may_start(l);

    switch (dest->type()) {
    case UmlArtifact:
        switch (l) {
        case UmlContain:
        case UmlDependOn:
            return (dest == this) ? TR("illegal") : QString();

        case UmlInherit:
            return browser_node->check_inherit(dest->get_bn());;

        default:
            return TR("illegal");
        }

    case UmlPackage:
    case UmlComponent:
    case UmlDeploymentNode:
        return (l == UmlDependOn) ? QString() : TR("illegal");

    default:
        return TR("illegal");
    }
}

bool ArtifactCanvas::move_with_its_package() const
{
    return TRUE;
}

void ArtifactCanvas::save(QTextStream & st, bool ref, QString & warning) const
{
    if (ref) {
        st << "artifactcanvas_ref " << get_ident() << " // "
           << browser_node->full_name();
    }
    else {
        nl_indent(st);
        st << "artifactcanvas " << get_ident() << " ";
        browser_node->save(st, TRUE, warning);
        indent(+1);
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

ArtifactCanvas * ArtifactCanvas::read(char *& st, UmlCanvas * canvas,
                                      char * k)
{
    if (!strcmp(k, "artifactcanvas_ref") ||
        ((read_file_format() < 20) && !strcmp(k, "componentcanvas_ref")))
        return ((ArtifactCanvas *) dict_get(read_id(st), "artifactcanvas", canvas));
    else if (!strcmp(k, "artifactcanvas") ||
             ((read_file_format() < 20) && !strcmp(k, "componentcanvas"))) {
        int id = read_id(st);
        BrowserArtifact * br = BrowserArtifact::read_ref(st, read_keyword(st));
        ArtifactCanvas * result = new ArtifactCanvas(canvas, id);

        result->browser_node = br;
        connect(br->get_data(), SIGNAL(changed()), result, SLOT(modified()));
        connect(br->get_data(), SIGNAL(deleted()), result, SLOT(deleted()));

        k = read_keyword(st);

        read_color(st, "color", result->itscolor, k);	// updates k

        if (strcmp(k, "xyz"))
            wrong_keyword(k, "xyz");

        read_xyz(st, result);

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

void ArtifactCanvas::history_hide()
{
    DiagramCanvas::setVisible(FALSE);
    disconnect(DrawingSettings::instance(), SIGNAL(changed()),
               this, SLOT(modified()));
    disconnect(browser_node->get_data(), 0, this, 0);
}

void ArtifactCanvas::history_load(QBuffer & b)
{
    DiagramCanvas::history_load(b);
    connect(browser_node->get_data(), SIGNAL(changed()), this, SLOT(modified()));
    connect(browser_node->get_data(), SIGNAL(deleted()), this, SLOT(deleted()));
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
}
