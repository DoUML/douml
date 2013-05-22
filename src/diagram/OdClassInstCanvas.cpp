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

#include "OdClassInstCanvas.h"
#include "ObjectLinkCanvas.h"
#include "BrowserClass.h"
#include "BrowserClassInstance.h"
#include "ClassInstanceData.h"
#include "BrowserAttribute.h"
#include "AttributeData.h"
#include "RelationData.h"
#include "BrowserDiagram.h"
#include "UmlCanvas.h"
#include "ClassData.h"
#include "SettingsDialog.h"
#include "myio.h"
#include "ui/menufactory.h"
#include "ObjectDiagramView.h"
#include "Settings.h"
#include "UmlGlobal.h"
#include "strutil.h"
#include "translate.h"

OdClassInstCanvas::OdClassInstCanvas(BrowserClassInstance * bn, UmlCanvas * canvas,
                                     int x, int y, int id)
    : DiagramCanvas(0, canvas, x, y, CLASSINST_CANVAS_MIN_SIZE, 1, id),
      ClassInstCanvas()
{
    browser_node = bn;
    itscolor = UmlDefaultColor;

    BasicData * d = bn->get_data();

    connect(d, SIGNAL(changed()), this, SLOT(modified()));
    connect(d, SIGNAL(deleted()), this, SLOT(deleted()));
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));

    if (id == 0) {
        // not on read
        compute_size();	// update used_settings

        if (browser_node->get_type() != UmlClass)
            check_stereotypeproperties();

        if (canvas->must_draw_all_relations())
            draw_all_relations();
    }
}

OdClassInstCanvas::~OdClassInstCanvas()
{
}

void OdClassInstCanvas::delete_it()
{
    BasicData * d = browser_node->get_data();

    disconnect(d, SIGNAL(changed()), this, SLOT(modified()));
    disconnect(d, SIGNAL(deleted()), this, SLOT(deleted()));
    disconnect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));

    DiagramCanvas::delete_it();
}

void OdClassInstCanvas::deleted()
{
    delete_it();
    canvas()->update();
}

void OdClassInstCanvas::remove(bool from_model)
{
    if (! from_model)
        delete_it();
    else
        browser_node->delete_it();	// will remove canvas
}

void OdClassInstCanvas::compute_size()
{
    UmlCanvas * canvas = the_canvas();
    ClassInstanceData * data = (ClassInstanceData *) browser_node->get_data();

    used_color = (itscolor == UmlDefaultColor)
                 ? canvas->browser_diagram()->get_color(UmlClass)
                 : itscolor;

    switch (show_stereotype_properties) {
    case UmlYes:
        show_properties = TRUE;
        break;

    case UmlNo:
        show_properties = FALSE;
        break;

    default:
        // right arg set by the diagram itself
        show_properties = canvas->browser_diagram()->get_show_stereotype_properties();
    }

    QFontMetrics fm(canvas->get_font(UmlNormalUnderlinedFont));
    double zoom = canvas->zoom();
    const int two = (int)(2 * zoom);
    int wi, he;

    horiz = TRUE;

    if (!get_name().isEmpty()) {
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
    }

    he = fm.height() + two + two;

    used_show_context_mode = (show_context_mode == DefaultShowContextMode)
                             ? canvas->browser_diagram()->get_classinstshowmode()
                             : show_context_mode;

    if (horiz)
        wi = fm.width(full_name());
    else {
        he += fm.height();

        int w = fm.width(get_name() + ":");

        wi = fm.width(data->get_class()->contextual_name(used_show_context_mode));

        if (w > wi)
            wi = w;
    }

    const Q3ValueList<SlotAttr> & attributes = data->get_attributes();

    if (! attributes.isEmpty()) {
        Q3ValueList<SlotAttr>::ConstIterator it = attributes.begin();
        QString egal = " = ";

        do {
            QString s = (*it).att->get_name() + egal + (*it).value;
            int w = fm.width(s);

            if (w > wi)
                wi = w;

            ++it;
        }
        while (it != attributes.end());

        he += (fm.height() + two) * attributes.count() + two + two;
    }

    if (used_color != UmlTransparent) {
        const int shadow = canvas->shadow();

        wi += shadow;
        he += shadow;
    }

    const int eight = (int)(8 * zoom);

    wi += (((ClassData *) data->get_class()->get_data())->get_is_active())
          ? eight * 3 : eight;

    int minw = (int)(zoom * CLASSINST_CANVAS_MIN_SIZE);

    if (wi < minw)
        wi = minw;

    // force odd width and height for line alignment
    DiagramCanvas::resize(wi | 1, he | 1);
}

void OdClassInstCanvas::modified()
{
    if (visible()) {
        hide();
        hide_lines();
        compute_size();
        show();
        update_show_lines();

        if (browser_node->get_type() != UmlClass)
            check_stereotypeproperties();

        if (the_canvas()->must_draw_all_relations())
            draw_all_relations();

        foreach (ArrowCanvas *canvas, lines)
            if (IsaRelation(canvas->type()))
                // useless to check UmlObjectLink ie unset link
                ((ObjectLinkCanvas *) canvas)->check();

        canvas()->update();
        package_modified();
    }
}

void OdClassInstCanvas::post_loaded()
{
    if (the_canvas()->must_draw_all_relations())
        draw_all_relations();
}

bool OdClassInstCanvas::has_relation(const SlotRel & slot_rel) const
{
    foreach (ArrowCanvas *canvas, lines)
        if (IsaRelation(canvas->type()) &&
            (((ObjectLinkCanvas *) canvas)
             ->is(slot_rel, canvas->get_start() == (DiagramItem *) this)))
            return TRUE;

    return FALSE;
}

// return true if a clone of lnk already exist between this and other
bool OdClassInstCanvas::is_duplicated(ObjectLinkCanvas * lnk,
                                      OdClassInstCanvas * other) const
{
    RelationData * rel = lnk->get_rel();

    foreach (ArrowCanvas *ar, lines)
        if ((ar != lnk) &&
            IsaRelation(ar->type()) &&
            (((ObjectLinkCanvas *) ar)->get_rel() == rel) &&
            (ar->get_end() == other))
            return TRUE;

    return FALSE;
}

void OdClassInstCanvas::draw_all_relations(OdClassInstCanvas * end)
{
    Q3CanvasItemList all = canvas()->allItems();
    Q3CanvasItemList::Iterator cit;
    ClassInstanceData * d = (ClassInstanceData *) browser_node->get_data();
    const Q3ValueList<SlotRel> & rels = d->get_relations();
    Q3ValueList<SlotRel>::ConstIterator it_rel;

    for (it_rel = rels.begin(); it_rel != rels.end(); it_rel++) {
        const SlotRel & slot_rel = *it_rel;

        if (!has_relation(slot_rel)) {
            BrowserClassInstance * end_inst = slot_rel.value;
            DiagramItem * di;

            if (end_inst == browser_node)
                di = this;
            else {
                di = 0;

                for (cit = all.begin(); cit != all.end(); ++cit) {
                    DiagramItem * adi = QCanvasItemToDiagramItem(*cit);

                    if ((adi != 0) &&		// an uml canvas item
                        (adi->type() == UmlClassInstance) &&
                        (((OdClassInstCanvas *) adi)->browser_node == end_inst) &&
                        ((((OdClassInstCanvas *) adi) == end) || (*cit)->visible())) {
                        // other class canvas find
                        di = adi;
                        break;
                    }
                }
            }

            if (di != 0) {
                if (slot_rel.is_a)
                    (new ObjectLinkCanvas(the_canvas(), this, di,
                                          slot_rel.rel->get_type(),
                                          0, -1.0, -1.0, slot_rel.rel))
                    ->show();
                else
                    (new ObjectLinkCanvas(the_canvas(), di, this,
                                          slot_rel.rel->get_type(),
                                          0, -1.0, -1.0, slot_rel.rel))
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
                (di->type() == UmlClassInstance) &&
                (((OdClassInstCanvas *) di) != this) &&
                !((OdClassInstCanvas *) di)->browser_node->deletedp() &&
                ((OdClassInstCanvas *) di)->visible())
                ((OdClassInstCanvas *) di)->draw_all_relations(this);
        }
    }
}

void OdClassInstCanvas::change_scale()
{
    Q3CanvasRectangle::setVisible(FALSE);
    compute_size();
    recenter();
    Q3CanvasRectangle::setVisible(TRUE);
}

void OdClassInstCanvas::draw(QPainter & p)
{
    if (visible()) {
        QRect r = rect();
        QFontMetrics fm(the_canvas()->get_font(UmlNormalFont));
        double zoom = the_canvas()->zoom();
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setBackgroundMode((used_color == UmlTransparent) ? ::Qt::TransparentMode : ::Qt::OpaqueMode);

        QColor co = color(used_color);
        FILE * fp = svg();

        if (fp != 0)
            fputs("<g>\n", fp);

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

        if (used_color != UmlTransparent)
            p.fillRect(r, co);

        if (fp != 0)
            fprintf(fp, "\t<rect fill=\"%s\" stroke=\"black\" stroke-width=\"1\" stroke-opacity=\"1\""
                    " x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" />\n",
                    svg_color(used_color),
                    r.x(), r.y(), r.width() - 1, r.height() - 1);

        p.drawRect(r);

        BrowserClass * cl =
            ((ClassInstanceData *) browser_node->get_data())->get_class();

        if (((ClassData *) cl->get_data())->get_is_active()) {
            const int eight = (int)(8 * zoom);

            r.setLeft(r.left() + eight);
            r.setRight(r.right() - eight);

            p.drawLine(r.topLeft(), r.bottomLeft());
            p.drawLine(r.topRight(), r.bottomRight());

            if (fp != 0)
                fprintf(fp,
                        "\t<line stroke=\"black\" stroke-opacity=\"1\""
                        " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n"
                        "\t<line stroke=\"black\" stroke-opacity=\"1\""
                        " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                        r.left(), r.top(), r.left(), r.bottom(),
                        r.right(), r.top(), r.right(), r.bottom());
        }

        const int two = (int)(2 * zoom);
        int he = fm.height() + two;

        p.setFont(the_canvas()->get_font(UmlNormalUnderlinedFont));

        r.setTop(r.top() + two);

        if (horiz) {
            p.drawText(r, ::Qt::AlignHCenter + ::Qt::AlignTop,
                       full_name());

            if (fp != 0)
                draw_text(r, ::Qt::AlignHCenter + ::Qt::AlignTop,
                          full_name(),
                          p.font(), fp);
        }
        else {
            p.drawText(r, ::Qt::AlignHCenter + ::Qt::AlignTop,
                       get_name() + ":");

            if (fp != 0)
                draw_text(r, ::Qt::AlignHCenter + ::Qt::AlignTop,
                          get_name() + ":",
                          p.font(), fp);

            r.setTop(r.top() + fm.height());

            p.drawText(r, ::Qt::AlignHCenter + ::Qt::AlignTop,
                       cl->contextual_name(used_show_context_mode));

            if (fp != 0)
                draw_text(r, ::Qt::AlignHCenter + ::Qt::AlignTop,
                          cl->contextual_name(used_show_context_mode),
                          p.font(), fp);
        }

        p.setFont(the_canvas()->get_font(UmlNormalFont));

        const Q3ValueList<SlotAttr> & attributes =
            ((ClassInstanceData *) browser_node->get_data())->get_attributes();

        if (!attributes.isEmpty()) {
            r.setTop(r.top() + he + two);
            p.drawLine(r.topLeft(), r.topRight());

            if (fp != 0)
                fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                        " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                        r.left(), r.top(), r.right(), r.top());

            r.setTop(r.top() + two);
            r.setLeft(r.left() + (int)(4 * zoom));

            Q3ValueList<SlotAttr>::ConstIterator it = attributes.begin();
            QString egal = " = ";

            do {
                QString s = (*it).att->get_name() + egal + (*it).value;

                p.drawText(r, ::Qt::AlignTop, s);

                if (fp != 0)
                    draw_text(r, ::Qt::AlignTop, s,
                              p.font(), fp);

                r.setTop(r.top() + he);
                ++it;
            }
            while (it != attributes.end());
        }

        if (fp != 0)
            fputs("</g>\n", fp);

        if (selected())
            show_mark(p, rect());
    }
}

UmlCode OdClassInstCanvas::type() const
{
    return UmlClassInstance;
}

// all cases
QString OdClassInstCanvas::get_name() const
{
    return browser_node->get_name();
}

// out of model case : never called
void OdClassInstCanvas::set_name(const QString &)
{
}

// return class, all cases
BrowserNode * OdClassInstCanvas::get_type() const
{
    return ((ClassInstanceData *) browser_node->get_data())
           ->get_class();
}

// out of model case : never called
void OdClassInstCanvas::set_type(BrowserNode *)
{
}

BrowserNodeList & OdClassInstCanvas::get_types(BrowserNodeList & r) const
{
    return BrowserClass::instances(r);
}

QString OdClassInstCanvas::get_full_name() const
{
    return full_name();
}

void OdClassInstCanvas::open()
{
    browser_node->open(FALSE);
}

void OdClassInstCanvas::menu(const QPoint &)
{
    Q3PopupMenu m(0);

    MenuFactory::createTitle(m, full_name());
    m.insertSeparator();
    m.insertItem(TR("Upper"), 0);
    m.insertItem(TR("Lower"), 1);
    m.insertItem(TR("Go up"), 9);
    m.insertItem(TR("Go down"), 10);
    m.insertSeparator();
    m.insertItem(TR("Edit drawing settings"), 2);
    m.insertSeparator();
    m.insertItem(TR("Edit"), 3);
    m.insertSeparator();
    m.insertItem(TR("Select in browser"), 4);
    m.insertItem(TR("Select class in browser"), 5);

    if (linked())
        m.insertItem(TR("Select linked items"), 6);

    m.insertSeparator();
    m.insertItem(TR("Remove from diagram"), 7);

    if (browser_node->is_writable())
        m.insertItem(TR("Delete from model"), 8);

    switch (m.exec(QCursor::pos())) {
    case 0:
        upper();
        modified();	// call package_modified
        return;

    case 1:
        lower();
        modified();	// call package_modified
        return;

    case 9:
        z_up();
        modified();	// call package_modified()
        return;

    case 10:
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
        ((ClassInstanceData *) browser_node->get_data())
        ->get_class()->select_in_browser();
        return;

    case 6:
        the_canvas()->unselect_all();
        select_associated();
        return;

    case 7:
        delete_it();
        package_modified();
        return;

    case 8:
        //delete from model
        browser_node->delete_it();	// will delete the canvas
        break;

    default:
        return;
    }
}

void OdClassInstCanvas::apply_shortcut(QString s)
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
    else if (s == "Edit") {
        open();  // call modified then package_modified
        return;
    }
    else
        return;

    modified();
}

void OdClassInstCanvas::edit_drawing_settings()
{
    for (;;) {
        StateSpecVector st((browser_node->get_type() != UmlClass) ? 3 : 2);
        ColorSpecVector co(1);

        st[0].set(TR("write name:type \nhorizontally"), &write_horizontally);
        st[1].set(TR("show class context"), &show_context_mode);

        if (browser_node->get_type() != UmlClass)
            st[2].set(TR("show stereotypes \nproperties"), &show_stereotype_properties);

        co[0].set(TR("class instance color"), &itscolor);

        SettingsDialog dialog(&st, &co, FALSE);

        dialog.raise();

        if (dialog.exec() != QDialog::Accepted)
            return;

        modified();	// call package_modified

        if (!dialog.redo())
            break;
    }
}

bool OdClassInstCanvas::has_drawing_settings() const
{
    return TRUE;
}

void OdClassInstCanvas::edit_drawing_settings(QList<DiagramItem *> & l)
{
    for (;;) {
        StateSpecVector st(2);
        ColorSpecVector co(1);
        Uml3States write_horizontally;
        ShowContextMode show_context_mode;
        UmlColor itscolor;

        st[0].set(TR("write name:type \nhorizontally"), &write_horizontally);
        st[1].set(TR("show class context"), &show_context_mode);
        co[0].set(TR("class instance color"), &itscolor);

        SettingsDialog dialog(&st, &co, FALSE, TRUE);

        dialog.raise();

        if (dialog.exec() == QDialog::Accepted) {
            foreach (DiagramItem *item, l) {
                OdClassInstCanvas *canvas = (OdClassInstCanvas *)item;
                if (!st[0].name.isEmpty())
                    canvas->write_horizontally =
                        write_horizontally;

                if (!st[1].name.isEmpty())
                    canvas->show_context_mode =
                        show_context_mode;

                if (!co[0].name.isEmpty())
                    canvas->itscolor = itscolor;

                canvas->modified();
            }
        }

        if (!dialog.redo())
            break;
    }
}

void OdClassInstCanvas::clone_drawing_settings(const DiagramItem *src)
{
    const OdClassInstCanvas * x = (const OdClassInstCanvas *) src;
    write_horizontally = x->write_horizontally;
    show_context_mode = x->show_context_mode;
    itscolor = x->itscolor;
    modified();
}

bool OdClassInstCanvas::get_show_stereotype_properties() const
{
    return (browser_node->get_type() != UmlClass) && show_properties;
}

BrowserClassInstance * OdClassInstCanvas::get_instance() const
{
    return (BrowserClassInstance *) browser_node;
}

BrowserNode * OdClassInstCanvas::container(UmlCode c) const
{
    return the_canvas()->browser_diagram()->container(c);
}

void OdClassInstCanvas::delete_available(BooL & in_model, BooL & out_model) const
{
    out_model |= TRUE;

    if (browser_node->get_type() == UmlClassInstance)
        in_model |=  browser_node->is_writable();
}

bool OdClassInstCanvas::alignable() const
{
    return TRUE;
}

bool OdClassInstCanvas::copyable() const
{
    return selected();
}

QString OdClassInstCanvas::may_start(UmlCode &) const
{
    // all (object link & anchor) allowed
    return 0;
}

QString OdClassInstCanvas::may_connect(UmlCode & l, const DiagramItem * dest) const
{
    return ((dest->type() == UmlClassInstance)
            ? ((l == UmlObjectLink) || (l == UmlAnchor) || IsaRelation(l))
            : (l == UmlAnchor))
           ? QString() : TR("illegal");
}

void OdClassInstCanvas::connexion(UmlCode t, DiagramItem * dest, const QPoint &, const QPoint &)
{
    ArrowCanvas * l;

    if (t == UmlObjectLink)
        l = new ObjectLinkCanvas(the_canvas(), this, dest, UmlObjectLink, 0, -1.0, -1.0, 0);
    else
        l = new ArrowCanvas(the_canvas(), this, dest, t, 0, FALSE, -1.0, -1.0);

    l->show();
    the_canvas()->select(l);
}

bool OdClassInstCanvas::move_with_its_package() const
{
    return TRUE;
}

void OdClassInstCanvas::save(QTextStream & st, bool ref, QString & warning) const
{
    if (ref)
        st << "classinstance_ref " << get_ident() << " // "
           << browser_node->full_name();
    else {
        nl_indent(st);
        st << "classinstancecanvas " << get_ident() << " ";
        browser_node->save(st, TRUE, warning);

        indent(+1);

        nl_indent(st);
        save_xyz(st, this, "xyz");
        ClassInstCanvas::save(st);
        save_stereotype_property(st, warning);
        indent(-1);
        nl_indent(st);
        st << "end";
    }
}

OdClassInstCanvas * OdClassInstCanvas::read(char *& st, UmlCanvas * canvas,
        char * k)
{
    if (!strcmp(k, "classinstance_ref"))
        return ((OdClassInstCanvas *) dict_get(read_id(st), "classinstance", canvas));
    else if (!strcmp(k, "classinstance")) {
        // old release
        int id = read_id(st);
        BrowserClass * cl = BrowserClass::read_ref(st);

        k = read_keyword(st);

        UmlColor co = UmlDefaultColor;

        read_color(st, "color", co, k);	// updates k

        Uml3States ho;

        if (!strcmp(k, "write_horizontally") ||
            !strcmp(k, "write_horizontaly")) {
            ho = state(read_keyword(st));
            k = read_keyword(st);
        }
        else
            ho = UmlDefaultState;

        if (strcmp(k, "xyz"))
            wrong_keyword(k, "xyz");

        int x = (int) read_double(st);
        int y = (int) read_double(st);
        double z = read_double(st);

        read_keyword(st, "name");

        BrowserNode * parent =
            canvas->browser_diagram()->container(UmlClass);
        BrowserClassInstance * icl =
            // create a new one, don't look at already exising instances
            // contrarilly to the collaboration and sequence diagram
            // because of attributes & relations
            new BrowserClassInstance(read_string(st), cl, parent);
        OdClassInstCanvas * result =
            new OdClassInstCanvas(icl, canvas, x, y, id);

        result->setZ(z);
        result->itscolor = co;
        result->write_horizontally = ho;
        result->show_context_mode = noContext;

        k = read_keyword(st);

        if (!strcmp(k, "values"))
            ((ClassInstanceData *) icl->get_data())->read_attributes(st, k);	// updates k
        else if (strcmp(k, "end") &&
                 strcmp(k, "xyz"))
            wrong_keyword(k, "end or xyz");

        if (*k == 'x')
            read_xyz(st, result);

        result->compute_size();
        result->set_center100();
        result->show();

        // to save new instance and diagram def
        result->package_modified();
        canvas->browser_diagram()->modified();

        return result;
    }
    else if (!strcmp(k, "classinstancecanvas")) {
        int id = read_id(st);
        BrowserClassInstance * icl = BrowserClassInstance::read_ref(st);

        read_keyword(st, "xyz");

        int x = (int) read_double(st);
        OdClassInstCanvas * result =
            new OdClassInstCanvas(icl, canvas, x, (int) read_double(st), id);

        result->setZ(read_double(st));

        result->ClassInstCanvas::read(st, k);

        if (read_file_format() < 74)
            result->show_context_mode = noContext;

        result->read_stereotype_property(st, k);	// updates k

        if (strcmp(k, "end"))
            wrong_keyword(k, "end");

        if (result->get_type() != 0) {
            // not a deleted instance
            result->compute_size();
            result->set_center100();
            result->show();
            result->check_stereotypeproperties();
        }

        return result;
    }
    else
        return 0;
}

void OdClassInstCanvas::history_hide()
{
    Q3CanvasItem::setVisible(FALSE);
    disconnect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));

    BasicData * d = browser_node->get_data();

    disconnect(d, SIGNAL(changed()), this, SLOT(modified()));
    disconnect(d, SIGNAL(deleted()), this, SLOT(deleted()));
}

void OdClassInstCanvas::history_load(QBuffer & b)
{
    DiagramCanvas::history_load(b);
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));

    BasicData * d = browser_node->get_data();

    disconnect(d, SIGNAL(changed()), this, SLOT(modified()));
    disconnect(d, SIGNAL(deleted()), this, SLOT(deleted()));
}
