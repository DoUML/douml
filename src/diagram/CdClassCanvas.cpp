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
#include <QPixmap>
#include <Q3ValueList>
#include <QTextStream>

#include "CdClassCanvas.h"
#include "TemplateCanvas.h"
#include "RelationCanvas.h"
#include "SimpleRelationCanvas.h"
#include "ArrowPointCanvas.h"
#include "ConstraintCanvas.h"
#include "BrowserClass.h"
#include "BrowserOperation.h"
#include "UmlPixmap.h"
#include "UmlGlobal.h"
#include "BrowserAttribute.h"
#include "AttributeData.h"
#include "OperationData.h"
#include "ClassData.h"
#include "RelationData.h"
#include "PackageData.h"
#include "DiagramView.h"
#include "SettingsDialog.h"
#include "HideShowDialog.h"
#include "myio.h"
#include "ToolCom.h"
#include "Tool.h"
#include "ui/menufactory.h"
#include "OperationListDialog.h"
#include "BrowserClassDiagram.h"
#include "strutil.h"
#include "GenerationSettings.h"
#include "ProfiledStereotypes.h"
#include "translate.h"

CdClassCanvas::CdClassCanvas(BrowserNode * bn, UmlCanvas * canvas,
                             int x, int y)
    : DiagramCanvas(0, canvas, x, y, CLASS_CANVAS_MIN_SIZE,
                    CLASS_CANVAS_MIN_SIZE, 0)
{
    browser_node = bn;
    templ = 0;	// may be updated by check_size()
    itscolor = UmlDefaultColor;
    indicate_visible_attr = FALSE;
    indicate_visible_oper = FALSE;
    manual_size = FALSE;
    constraint = 0;

    check_size();	// update used_settings
    check_constraint();
    check_stereotypeproperties();

    subscribe(bn->get_data());	// = TRUE
    connect(bn->get_data(), SIGNAL(changed()), this, SLOT(modified()));
    connect(bn->get_data(), SIGNAL(deleted()), this, SLOT(deleted()));
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));

    if (canvas->must_draw_all_relations()) {
        draw_all_relations();
        draw_all_class_assoc();
        draw_all_simple_relations();
    }
}

CdClassCanvas::CdClassCanvas(UmlCanvas * canvas, int id)
    : DiagramCanvas(canvas, id)
{
    // for read operation
    browser_node = 0;
    templ = 0;	// may be updated by check_size()
    itscolor = UmlDefaultColor;
    indicate_visible_attr = FALSE;
    indicate_visible_oper = FALSE;
    manual_size = FALSE;
    constraint = 0;
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
}

CdClassCanvas::~CdClassCanvas()
{
}

void CdClassCanvas::delete_it()
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

    if (templ)
        templ->delete_it();

    if (constraint)
        constraint->delete_it();

    DiagramCanvas::delete_it();
}

void CdClassCanvas::deleted()
{
    delete_it();
    canvas()->update();
}

void CdClassCanvas::remove(bool from_model)
{
    if (! from_model)
        delete_it();
    else
        browser_node->delete_it();	// will remove canvas
}

void CdClassCanvas::check_size()
{
    // does not unsubscribe & disconnect signals because check_size may
    // be called during a signal management, and the signal connection list
    // cannot be modified in this case
    used_settings = settings;
    the_canvas()->browser_diagram()->get_classdiagramsettings(used_settings);

    full_name = ((BrowserClass *) browser_node)
                ->contextual_name(used_settings.show_context_mode);

    QFontMetrics fm(the_canvas()->get_font(UmlNormalFont));
    QFontMetrics fbm(the_canvas()->get_font(UmlNormalBoldFont));
    QFontMetrics fim(the_canvas()->get_font(UmlNormalItalicFont));
    QFontMetrics fbim(the_canvas()->get_font(UmlNormalBoldItalicFont));
    const ClassData * data = ((ClassData *) browser_node->get_data());
    int wi =
        (data->get_is_abstract()) ? fbim.width(full_name) : fbm.width(full_name);
    Q3ListViewItem * child;
    int natt = 0;
    int noper = 0;
    bool full_members = (used_settings.show_full_members_definition == UmlYes);
    bool show_visibility = (used_settings.show_members_visibility == UmlYes);
    bool show_stereotype = (used_settings.show_members_stereotype == UmlYes);
    bool show_stereotype_properties = (used_settings.show_stereotype_properties == UmlYes);
    bool show_multiplicity = (used_settings.show_members_multiplicity == UmlYes);
    bool show_initialization = (used_settings.show_members_initialization == UmlYes);
    bool show_modifiers = (used_settings.show_attribute_modifiers == UmlYes);
    bool show_dir = (used_settings.show_parameter_dir == UmlYes);
    bool show_name = (used_settings.show_parameter_name == UmlYes);
    bool hide_attrs = (used_settings.hide_attributes == UmlYes);
    bool hide_opers = (used_settings.hide_operations == UmlYes);
    bool hide_getset_opers = (used_settings.hide_getset_operations == UmlYes);
    int max_member_width = used_settings.member_max_width;
    int offset_visi = fbim.width("#_");
    double zoom = the_canvas()->zoom();
    const QPixmap * px = 0;

    if (max_member_width == UmlUnlimitedMemberWidth)
        max_member_width = 1000000;

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

    if (!hide_attrs || !hide_opers) {
        QString s;

        for (child = browser_node->firstChild(); child; child = child->nextSibling()) {
            if (!((BrowserNode *) child)->deletedp()) {
                BasicData * child_data = ((BrowserNode *) child)->get_data();
                int wa;

                switch (((BrowserNode *) child)->get_type()) {
                case UmlAttribute:

                    if (hide_attrs ||
                        ((indicate_visible_attr)
                         ? (hidden_visible_attributes.findIndex((BrowserNode *) child) == -1)
                         : (hidden_visible_attributes.findIndex((BrowserNode *) child) != -1)))
                        continue;

                    s = ((AttributeData *) child_data)
                        ->definition(full_members, show_multiplicity, show_initialization,
                                     show_modifiers, used_settings.drawing_language,
                                     used_settings.show_members_context);

                    if (s.isEmpty())
                        continue;

                    if ((int) s.length() >= max_member_width)
                        s = s.left(max_member_width) + "...";

                    natt += 1;
                    wa = fm.width(s);

                    if (full_members || show_stereotype_properties) {
                        BrowserClass * t = ((AttributeData *) child_data)->get_type().type;

                        if (t && subscribe(t->get_data()))
                            connect(t->get_data(), SIGNAL(changed()),
                                    this, SLOT(modified()));
                    }

                    break;

                case UmlOperation:
                    if (hide_opers ||
                        (hide_getset_opers && ((OperationData *) child_data)->get_or_set()) ||
                        ((indicate_visible_oper)
                         ? (hidden_visible_operations.findIndex((BrowserNode *) child) == -1)
                         : (hidden_visible_operations.findIndex((BrowserNode *) child) != -1)))
                        continue;

                    s = ((OperationData *) child_data)
                        ->definition(full_members, used_settings.drawing_language, show_dir,
                                     show_name, used_settings.show_members_context);

                    if (s.isEmpty())
                        continue;

                    if ((int) s.length() >= max_member_width)
                        s = s.left(max_member_width) + "...";

                    noper += 1;

                    if (((OperationData *) child_data)->get_is_abstract())
                        wa = fim.width(s);
                    else
                        wa = fm.width(s);

                    if (full_members || show_stereotype_properties) {
                        BrowserClass * t =
                            ((OperationData *) child_data)->get_return_type().type;

                        if ((t != 0) && subscribe(t->get_data()))
                            connect(t->get_data(), SIGNAL(changed()),
                                    this, SLOT(modified()));

                        if (full_members) {
                            int n = (int)((OperationData *) child_data)->get_n_params();

                            for (int i = 0; i != n; i += 1)
                                if (((t = ((OperationData *) child_data)->get_param_type(i).type) != 0) &&
                                    subscribe(t->get_data()))
                                    connect(t->get_data(), SIGNAL(changed()),
                                            this, SLOT(modified()));
                        }
                    }

                    break;

                default:
                    continue;
                }

                if (show_visibility)
                    wa += offset_visi;

                QString st = child_data->get_short_stereotype();

                if (show_stereotype && !st.isEmpty())
                    wa += fm.width("<<" + st + ">>_");

                if ((full_members || show_visibility || show_stereotype) &&
                    subscribe(child_data))
                    connect(child_data, SIGNAL(changed()), this, SLOT(modified()));

                if (wa > wi)
                    wi = wa;

                if (show_stereotype_properties &&
                    !st.isEmpty() &&
                    (ProfiledStereotypes::isModeled(child_data->get_stereotype()) != 0)) {
                    int & nmember = (((BrowserNode *) child)->get_type() == UmlAttribute)
                                    ? natt : noper;

                    const HaveKeyValueData * kv =
                        dynamic_cast<HaveKeyValueData *>((BrowserNode *) child);
                    int nk = (int) kv->get_n_keys();
                    bool have_stprop = FALSE;

                    for (int ik = 0; ik != nk; ik += 1) {
                        const char * k =  kv->get_key(ik);
                        const char * p;
                        unsigned nseps = 0;

                        for (p = k; *p; p += 1) {
                            if (*p == ':') {
                                nseps += 1;
                                k = p + 1;
                            }
                        }

                        if (nseps == 2) {
                            p = kv->get_value(ik);

                            if (p && *p) {
                                s = k + QString("=") + p;

                                if ((int) s.length() >= max_member_width)
                                    s = s.left(max_member_width) + "...";

                                wa = offset_visi + fm.width(s);

                                if (wa > wi)
                                    wi = wa;

                                nmember += 1;
                                have_stprop = TRUE;
                            }
                        }
                    }

                    if (!show_stereotype && have_stprop) {
                        wa = offset_visi + fm.width("<<" + st + ">>_");

                        if (wa > wi)
                            wi = wa;

                        nmember += 1;
                    }
                }
            }
        }
    }

    const int two = (int)(2 * zoom);
    const int four = (int)(4 * zoom);
    const int six = (int)(6 * zoom);
    const int eight = (int)(8 * zoom);

    int he = (fbm.height() + two) * (natt + noper + 1) + six;

    bool tmpl = data->get_n_formalparams() != 0;

    if (tmpl)
        he += fbm.height();

    if (natt == 0)
        he += six;

    if (noper == 0)
        he += six;

    if ((used_view_mode == asClass) && data->get_stereotype()[0]) {
        int stw = fm.width(QString("<<") + toUnicode(data->get_short_stereotype()) + ">>");

        if (wi < stw)
            wi = stw;

        he += fbm.height() + four;
    }

    wi += (data->get_is_active()) ? eight * 3 : eight;

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

    if (used_view_mode != asClass)
        manual_size = FALSE;
    else if (used_color != UmlTransparent) {
        const int shadow = the_canvas()->shadow();

        wi += shadow;
        he += shadow;
    }

    // force odd width and height for line alignment
    width_min = wi | 1;
    height_min = he | 1;

    if (manual_size)
        DiagramCanvas::resize((width() < width_min) ? width_min : width(),
                              (height() < height_min) ? height_min : height());
    else
        DiagramCanvas::resize(width_min, height_min);

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

void CdClassCanvas::modified()
{
    if (visible()) {
        hide();

        hide_lines();
        check_size();
        recenter();
        show();
        update_show_lines();
        force_self_rel_visible();
        check_inner();

        if (the_canvas()->must_draw_all_relations()) {
            draw_all_relations();
            draw_all_simple_relations();
        }

        check_constraint();
        check_stereotypeproperties();
        canvas()->update();
        package_modified();
    }
}

void CdClassCanvas::post_loaded()
{
    force_self_rel_visible();
    check_inner();

    if (the_canvas()->must_draw_all_relations()) {
        draw_all_relations();
        draw_all_simple_relations();
    }

    check_constraint();
}

void CdClassCanvas::check_inner()
{
    foreach (ArrowCanvas *canvas, lines) {
        if (canvas->type() == UmlInner) {
            DiagramItem * b = canvas->get_start();
            DiagramItem * e = canvas->get_end();

            if (((BrowserNode *) b->get_bn()->parent()) != e->get_bn())
                canvas->delete_it();
        }
    }
}
bool CdClassCanvas::has_relation(BasicData * def) const
{
    foreach (ArrowCanvas *canvas, lines) {
        if (IsaRelation(canvas->type()) &&
            (((RelationCanvas *) canvas)->get_data() == def))
            return TRUE;
    }

    return FALSE;
}

bool CdClassCanvas::has_inner(DiagramItem * end) const
{
    foreach (ArrowCanvas *canvas, lines) {
        if ((canvas->type() == UmlInner) &&
            (canvas->get_end() == end))
            return TRUE;
    }

    return FALSE;
}

void CdClassCanvas::draw_all_relations(CdClassCanvas * end)
{
    Q3ListViewItem * child;
    Q3CanvasItemList all = canvas()->allItems();
    Q3CanvasItemList::Iterator cit;

    for (child = browser_node->firstChild(); child; child = child->nextSibling()) {
        if (!((BrowserNode *) child)->deletedp()) {
            UmlCode k = ((BrowserNode *) child)->get_type();

            if (IsaRelation(k)) {
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
                                (((CdClassCanvas *) adi)->browser_node == end_class) &&
                                ((((CdClassCanvas *) adi) == end) || (*cit)->visible())) {
                                // other class canvas find
                                di = adi;
                                break;
                            }
                        }
                    }

                    if (di != 0)
                        (new RelationCanvas(the_canvas(), this, di,
                                            ((BrowserClass *) browser_node),
                                            def->get_type(), 0, -1.0, -1.0, def))->show();
                }
            }
            else if (k == UmlClass) {
                for (cit = all.begin(); cit != all.end(); ++cit) {
                    DiagramItem * adi = QCanvasItemToDiagramItem(*cit);

                    if ((adi != 0) &&		// an uml canvas item
                        (adi->type() == UmlClass) &&
                        (((CdClassCanvas *) adi)->browser_node == child) &&
                        ((((CdClassCanvas *) adi) == end) || (*cit)->visible())) {
                        if (!((CdClassCanvas *) adi)->has_inner(this))
                            (new ArrowCanvas(the_canvas(), adi, this, UmlInner, 0, FALSE, -1.0, -1.0))
                            ->show();

                        break;
                    }
                }
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
                (((CdClassCanvas *) di) != this) &&
                !((CdClassCanvas *) di)->browser_node->deletedp() &&
                ((CdClassCanvas *) di)->visible()) {
                ((CdClassCanvas *) di)->draw_all_relations(this);
            }
        }
    }
}

// draw class association being 'this' or from
// a drawn relation to/from 'this'

void CdClassCanvas::draw_all_class_assoc()
{
    Q3CanvasItemList all = canvas()->allItems();
    Q3CanvasItemList::Iterator cit;

    for (cit = all.begin(); cit != all.end(); ++cit) {
        if ((*cit)->visible() &&
            isa_arrow(*cit) &&
            IsaRelation(((ArrowCanvas *) *cit)->type())) {
            RelationCanvas * r = (RelationCanvas *) *cit;
            BrowserNode * assoc =
                ((RelationData *) r->get_data())->get_association().type;

            if (assoc == browser_node)
                r->show_assoc_class(this);
            else {
                DiagramItem * b;
                DiagramItem * e;

                r->extremities(b, e);

                if ((b == this) || (e == this))
                    r->show_assoc_class(0);
            }
        }
    }
}

void CdClassCanvas::check_constraint()
{
    // update must be called before
    if (used_settings.show_infonote == UmlYes) {
        ConstraintCanvas * c =
            ConstraintCanvas::compute(the_canvas(), this, constraint);

        if (constraint == 0) {
            constraint = c;
            constraint->upper();

            constraint->move((int)(x() + width() + the_canvas()->zoom() * 20),
                             (int) y() + height());
            constraint->show();
            (new ArrowCanvas(the_canvas(), this, constraint, UmlAnchor, 0, FALSE, -1.0, -1.0))->show();
        }
    }
    else if (constraint != 0) {
        constraint->delete_it();
        constraint = 0;
    }
}

bool CdClassCanvas::get_show_stereotype_properties() const
{
    switch (used_settings.show_stereotype_properties) {
    case UmlYes:
        return TRUE;

    case UmlNo:
        return FALSE;

    default:
        return ((BrowserClassDiagram *) the_canvas()->browser_diagram())
               ->get_show_stereotype_properties();
    }
}

void CdClassCanvas::change_scale()
{
    Q3CanvasRectangle::setVisible(FALSE);

    if (manual_size) {
        double scale = the_canvas()->zoom();

        setSize((int)(width_scale100 * scale), (int)(height_scale100 * scale));
    }

    check_size();
    recenter();

    if (templ != 0)
        templ->update();

    Q3CanvasRectangle::setVisible(TRUE);
}

void CdClassCanvas::moveBy(double dx, double dy)
{
    DiagramCanvas::moveBy(dx, dy);

    if (templ != 0)
        templ->update();

    if ((constraint != 0) && !constraint->selected())
        constraint->moveBy(dx, dy);
}

bool CdClassCanvas::move_with_its_package() const
{
    return TRUE;
}

void CdClassCanvas::set_z(double z)
{
    setZ(z);

    if (templ)
        templ->update();
}

void write_member_st_prop(QPainter & p, FILE * fp, QRect & r,
                          const HaveKeyValueData * kv,
                          QString st, int he, int max_member_width,
                          bool stwritten)
{
    int nk = (int) kv->get_n_keys();

    for (int ik = 0; ik != nk; ik += 1) {
        const char * k =  kv->get_key(ik);
        const char * ps;
        unsigned nseps = 0;

        for (ps = k; *ps; ps += 1) {
            if (*ps == ':') {
                nseps += 1;
                k = ps + 1;
            }
        }

        if (nseps == 2) {
            ps = kv->get_value(ik);

            if (ps && *ps) {
                if (!stwritten) {
                    stwritten = TRUE;
                    st = "<<" + st + ">>";
                    p.drawText(r, ::Qt::AlignLeft + ::Qt::AlignTop, st);

                    if (fp != 0)
                        draw_text(r, ::Qt::AlignLeft + ::Qt::AlignTop, st, p.font(), fp);

                    r.setTop(r.top() + he);
                }

                QString s = k + QString("=") + ps;

                if ((int) s.length() >= max_member_width)
                    s = s.left(max_member_width) + "...";

                p.drawText(r, ::Qt::AlignLeft + ::Qt::AlignTop, s);

                if (fp != 0)
                    draw_text(r, ::Qt::AlignLeft + ::Qt::AlignTop, s, p.font(), fp);

                r.setTop(r.top() + he);
            }
        }
    }
}

void CdClassCanvas::draw(QPainter & p)
{
    if (! visible()) return;

    p.setRenderHint(QPainter::Antialiasing, true);
    QRect r = rect();
    QFontMetrics fm(the_canvas()->get_font(UmlNormalFont));
    QFontMetrics fbm(the_canvas()->get_font(UmlNormalBoldFont));
    QFontMetrics fbim(the_canvas()->get_font(UmlNormalBoldItalicFont));
    QColor bckgrnd = p.backgroundColor();
    double zoom = the_canvas()->zoom();
    FILE * fp = svg();

    if (fp != 0)
        fputs("<g>\n", fp);

    p.setBackgroundMode((used_color == UmlTransparent) ? ::Qt::TransparentMode : ::Qt::OpaqueMode);

    QColor co = color(used_color);
    const ClassData * data = ((ClassData *) browser_node->get_data());

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
                            QColor(::Qt::darkGray).rgb() & 0xffffff, // [lgfreitas] these became enumerators, so QColor() was added
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

        if (data->get_is_active()) {
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
    }

    const int two = (int)(2 * zoom);
    int he = fbm.height() + two;

    if (data->get_n_formalparams() != 0)
        r.setTop(r.top() + fbm.height());

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
              ? the_canvas()->get_font(UmlNormalBoldItalicFont)
              : the_canvas()->get_font(UmlNormalBoldFont));
    p.drawText(r, ::Qt::AlignHCenter + ::Qt::AlignTop, full_name);

    if (fp != 0)
        draw_text(r, ::Qt::AlignHCenter + ::Qt::AlignTop, full_name, p.font(), fp);

    p.setFont(the_canvas()->get_font(UmlNormalFont));

    r.setTop(r.top() + he);

    if (used_settings.hide_attributes != UmlYes) {
        p.drawLine(r.topLeft(), r.topRight());

        if (fp != 0)
            fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                    " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                    r.left(), r.top(), r.right(), r.top());
    }

    static const QString v[] = { "+", "#", "-", "~" };

    bool have = FALSE;
    int left0 = r.left();
    int left1 = left0 + (int)(4 * zoom);
    int left2 = left1 + fbim.width("#_");
    int space = fm.width("_");
    Q3ListViewItem * child;
    bool full_members = (used_settings.show_full_members_definition == UmlYes);
    bool show_visibility = (used_settings.show_members_visibility == UmlYes);
    bool show_stereotype = (used_settings.show_members_stereotype == UmlYes);
    bool show_stereotype_properties = (used_settings.show_stereotype_properties == UmlYes);
    bool show_multiplicity = (used_settings.show_members_multiplicity == UmlYes);
    bool show_initialization = (used_settings.show_members_initialization == UmlYes);
    bool show_modifiers = (used_settings.show_attribute_modifiers == UmlYes);
    bool show_dir = (used_settings.show_parameter_dir == UmlYes);
    bool show_name = (used_settings.show_parameter_name == UmlYes);
    int max_member_width = used_settings.member_max_width;

    if (max_member_width == UmlUnlimitedMemberWidth)
        max_member_width = 1000000;

    r.setTop(r.top() + two);

    if (used_settings.hide_attributes != UmlYes) {
        r.setLeft(left1);

        for (child = browser_node->firstChild(); child; child = child->nextSibling()) {
            if (!((BrowserNode *) child)->deletedp() &&
                (((BrowserNode *) child)->get_type() == UmlAttribute) &&
                ((indicate_visible_attr)
                 ? (hidden_visible_attributes.findIndex((BrowserNode *) child) != -1)
                 : (hidden_visible_attributes.findIndex((BrowserNode *) child) == -1))) {
                AttributeData * data =
                    ((AttributeData *)((BrowserNode *) child)->get_data());
                QString s = data->definition(full_members, show_multiplicity, show_initialization,
                                             show_modifiers, used_settings.drawing_language,
                                             used_settings.show_members_context);

                if (s.isEmpty())
                    continue;

                r.setLeft(left1);

                if (show_visibility) {
                    int vi = ((used_settings.drawing_language == CppView) &&
                              (data->get_cpp_visibility() != UmlDefaultVisibility))
                             ? data->get_cpp_visibility() : data->get_uml_visibility();

                    if ((vi == UmlPackageVisibility) &&
                        ((used_settings.drawing_language == CppView) || (used_settings.drawing_language == IdlView)))
                        vi = UmlPublic;

                    p.setFont(the_canvas()->get_font(UmlNormalBoldFont));
                    p.drawText(r, ::Qt::AlignLeft + ::Qt::AlignTop, v[vi]);

                    if (fp != 0)
                        draw_text(r, ::Qt::AlignLeft + ::Qt::AlignTop, v[vi], p.font(), fp);

                    r.setLeft(left2);
                }

                QString st = data->get_short_stereotype();

                if (show_stereotype && !st.isEmpty()) {
                    st = "<<" + st + ">>";
                    p.setFont(the_canvas()->get_font(UmlNormalFont));
                    p.drawText(r, ::Qt::AlignLeft + ::Qt::AlignTop, st);

                    if (fp != 0)
                        draw_text(r, ::Qt::AlignLeft + ::Qt::AlignTop, st, p.font(), fp);

                    r.setLeft(r.left() + fm.width(st) + space);
                }

                p.setFont((data->get_isa_class_attribute()) ? the_canvas()->get_font(UmlNormalUnderlinedFont)
                          : the_canvas()->get_font(UmlNormalFont));

                if ((int) s.length() >= max_member_width)
                    s = s.left(max_member_width) + "...";

                p.drawText(r, ::Qt::AlignLeft + ::Qt::AlignTop, s);

                if (fp != 0)
                    draw_text(r, ::Qt::AlignLeft + ::Qt::AlignTop, s, p.font(), fp);

                r.setTop(r.top() + he);
                have = TRUE;

                if (show_stereotype_properties &&
                    !st.isEmpty() &&
                    (ProfiledStereotypes::isModeled(data->get_stereotype()) != 0)) {
                    r.setLeft(left2);
                    p.setFont(the_canvas()->get_font(UmlNormalFont));
                    write_member_st_prop(p, fp, r, (BrowserAttribute *) child,
                                         st, he, max_member_width, show_stereotype);
                }
            }
        }

        r.setLeft(left0);
    }

    if (! have)
        r.setTop(r.top() + (int)(6 * zoom));

    if (used_settings.hide_operations != UmlYes) {
        p.drawLine(r.topLeft(), r.topRight());

        if (fp != 0)
            fprintf(fp, "\t<line stroke=\"black\" stroke-opacity=\"1\""
                    " x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />\n",
                    r.left(), r.top(), r.right(), r.top());
    }

    r.setLeft(left1);
    r.setTop(r.top() + two);

    if (used_settings.hide_operations != UmlYes) {
        for (child = browser_node->firstChild(); child; child = child->nextSibling()) {
            if (!((BrowserNode *) child)->deletedp() &&
                (((BrowserNode *) child)->get_type() == UmlOperation) &&
                ((indicate_visible_oper)
                 ? (hidden_visible_operations.findIndex((BrowserNode *) child) != -1)
                 : (hidden_visible_operations.findIndex((BrowserNode *) child) == -1))) {
                OperationData * data =
                    ((OperationData *)((BrowserNode *) child)->get_data());

                if ((used_settings.hide_getset_operations == UmlYes) && data->get_or_set())
                    continue;

                QString s = data->definition(full_members, used_settings.drawing_language,
                                             show_dir, show_name,
                                             used_settings.show_members_context);

                if (s.isEmpty())
                    continue;

                r.setLeft(left1);

                if (show_visibility) {
                    int vi = ((used_settings.drawing_language == CppView) &&
                              (data->get_cpp_visibility() != UmlDefaultVisibility))
                             ? data->get_cpp_visibility() : data->get_uml_visibility();

                    if ((vi == UmlPackageVisibility) &&
                        ((used_settings.drawing_language == CppView) || (used_settings.drawing_language == IdlView)))
                        vi = UmlPublic;

                    p.setFont(the_canvas()->get_font(UmlNormalBoldFont));
                    p.drawText(r, ::Qt::AlignLeft + ::Qt::AlignTop, v[vi]);

                    if (fp != 0)
                        draw_text(r, ::Qt::AlignLeft + ::Qt::AlignTop, v[vi], p.font(), fp);

                    r.setLeft(left2);
                }

                QString st = data->get_short_stereotype();

                if (show_stereotype) {
                    if (! st.isEmpty()) {
                        st = "<<" + st + ">>";
                        p.setFont(the_canvas()->get_font(UmlNormalFont));
                        p.drawText(r, ::Qt::AlignLeft + ::Qt::AlignTop, st);

                        if (fp != 0)
                            draw_text(r, ::Qt::AlignLeft + ::Qt::AlignTop, st, p.font(), fp);

                        r.setLeft(r.left() + fm.width(st) + space);
                    }
                }

                if (data->get_isa_class_operation())
                    p.setFont(the_canvas()->get_font(UmlNormalUnderlinedFont));
                else if (data->get_is_abstract())
                    p.setFont(the_canvas()->get_font(UmlNormalItalicFont));
                else
                    p.setFont(the_canvas()->get_font(UmlNormalFont));

                if ((int) s.length() >= max_member_width)
                    s = s.left(max_member_width) + "...";

                p.drawText(r, ::Qt::AlignLeft + ::Qt::AlignTop, s);

                if (fp != 0)
                    draw_text(r, ::Qt::AlignLeft + ::Qt::AlignTop, s, p.font(), fp);

                r.setTop(r.top() + he);

                if (show_stereotype_properties &&
                    !st.isEmpty() &&
                    (ProfiledStereotypes::isModeled(data->get_stereotype()) != 0)) {
                    r.setLeft(left2);
                    p.setFont(the_canvas()->get_font(UmlNormalFont));
                    write_member_st_prop(p, fp, r, (BrowserOperation *) child,
                                         st, he, max_member_width, show_stereotype);
                }
            }
        }
    }

    if (fp != 0)
        fputs("</g>\n", fp);

    p.setBackgroundColor(bckgrnd);

    if (selected())
        show_mark(p, rect());
}

UmlCode CdClassCanvas::type() const
{
    return UmlClass;
}

void CdClassCanvas::delete_available(BooL & in_model, BooL & out_model) const
{
    out_model |= TRUE;
    in_model |= browser_node->is_writable();
}

bool CdClassCanvas::alignable() const
{
    return TRUE;
}

bool CdClassCanvas::copyable() const
{
    return selected();
}

void CdClassCanvas::open()
{
    browser_node->open(FALSE);
}

static CdClassCanvas * container_class_without_inner(CdClassCanvas * cln)
{
    if (cln->the_canvas()->must_draw_all_relations())
        return 0;

    BrowserNode * p = (BrowserNode *) cln->get_bn()->parent();

    if (p->get_type() != UmlClass)
        return 0;

    Q3CanvasItemList all = cln->canvas()->allItems();
    Q3CanvasItemList::Iterator cit;

    for (cit = all.begin(); cit != all.end(); ++cit) {
        if ((*cit)->visible()) {
            CdClassCanvas * cn = dynamic_cast<CdClassCanvas *>(*cit);

            if ((cn != 0) && (cn->get_bn() == p))
                return (cln->has_inner(cn)) ? 0 : cn;
        }
    }

    return 0;
}

void CdClassCanvas::menu(const QPoint &)
{
    QList<BrowserOperation *> l =
        ((BrowserClass *) browser_node)->inherited_operations(21);
    Q3PopupMenu m(0);
    Q3PopupMenu gensubm(0);
    Q3PopupMenu attrsubm(0);
    Q3PopupMenu opersubm(0);
    Q3PopupMenu inhopersubm(0);
    Q3PopupMenu toolm(0);
    const char * stereotype = browser_node->get_data()->get_stereotype();
    CdClassCanvas * nesting_cl = container_class_without_inner(this);
    BrowserNodeList attributes;
    BrowserNodeList operations;
    QStringList attributes_def;
    QStringList operations_def;
    int index;

    browser_node->children(attributes, UmlAttribute);
    browser_node->children(operations, UmlOperation);

    MenuFactory::createTitle(m, browser_node->get_data()->definition(FALSE, TRUE));
    m.insertSeparator();
    m.insertItem(TR("Upper"), 0);
    m.insertItem(TR("Lower"), 1);
    m.insertItem(TR("Go up"), 21);
    m.insertItem(TR("Go down"), 22);
    m.insertSeparator();
    m.insertItem(TR("Add related elements"), 5);

    if (nesting_cl != 0)
        m.insertItem(TR("Show nesting relation"), 23);

    m.insertSeparator();
    m.insertItem(TR("Edit drawing settings"), 2);

    if (attributes.count() != 0)
        m.insertItem(TR("Individual attribute visibility"), 3);

    if (operations.count() != 0)
        m.insertItem(TR("Individual operation visibility"), 4);

    m.insertSeparator();
    m.insertItem(TR("Edit class"), 6);

    if (!strcmp(stereotype, "enum")) {
        if (browser_node->is_writable()) {
            m.insertItem(TR("Add item"), 7);
            m.insertItem(TR("Add attribute"), 9);
        }

        if (attributes.count() != 0)
            m.insertItem(TR("Edit item or attribute"), &attrsubm);

        if (browser_node->is_writable())
            m.insertItem(TR("Add operation"), 8);
    }
    else if (!strcmp(stereotype, "enum_pattern")) {
        if (browser_node->is_writable()) {
            m.insertItem(TR("Add item"), 7);
        }

        if (attributes.count() != 0)
            m.insertItem(TR("Edit item"), &attrsubm);
    }
    else if (strcmp(stereotype, "typedef")) {
        if (browser_node->is_writable())
            m.insertItem(TR("Add attribute"), 9);

        if (attributes.count() != 0)
            m.insertItem(TR("Edit attribute"), &attrsubm);

        if (browser_node->is_writable())
            m.insertItem(TR("Add operation"), 8);

        if (browser_node->is_writable() &&
            strcmp(stereotype, "union") &&
            (l.count() != 0)) {
            if (l.count() > 20)
                m.insertItem(TR("Add inherited operation"), 2999);
            else {
                MenuFactory::createTitle(inhopersubm, TR("Choose operation"));
                inhopersubm.insertSeparator();

                index = 3000;
                foreach (BrowserOperation *oper, l) {
                    QString menuItemText = ((BrowserNode *) oper->parent())->get_name() +
                            QString("::") + oper->get_data()->definition(TRUE, FALSE);
                    if (((OperationData *) oper->get_data())->get_is_abstract())
                    {
                        QFont font = inhopersubm.font();
                        font.setItalic(true);
                        inhopersubm.insertItem(menuItemText, index);
                        //somewhat hackish but seems to work
                        inhopersubm.actions().last()->setFont(font);
                    }
                    else
                        inhopersubm.insertItem(menuItemText,index);
                    ++index;
                }

                m.insertItem(TR("Add inherited operation"), &inhopersubm);
            }
        }

        if (operations.count() != 0) {
            if (operations.count() <= 20)
                m.insertItem(TR("Edit operation"), &opersubm);
            else
                m.insertItem(TR("Edit operation"), 1999);
        }
    }

    m.insertSeparator();
    m.insertItem(TR("Select in browser"), 10);

    if (linked())
        m.insertItem(TR("Select linked items"), 17);

    m.insertSeparator();

    if (browser_node->is_writable()) {
        if (browser_node->get_associated() !=
            (BrowserNode *) the_canvas()->browser_diagram())
            m.insertItem(TR("Set associated diagram"), 11);

        if (browser_node->get_associated())
            m.insertItem(TR("Remove diagram association"), 18);
    }

    m.insertSeparator();
    m.insertItem(TR("Remove from diagram"), 12);

    if (browser_node->is_writable())
        m.insertItem(TR("Delete from model"), 13);

    m.insertSeparator();
    bool cpp = GenerationSettings::cpp_get_default_defs();
    bool java = GenerationSettings::java_get_default_defs();
    bool php = GenerationSettings::php_get_default_defs();
    bool python = GenerationSettings::python_get_default_defs();
    bool idl = GenerationSettings::idl_get_default_defs();

    if (cpp || java || php || python || idl)
        m.insertItem(TR("Generate"), &gensubm);

    if (Tool::menu_insert(&toolm, UmlClass, 30))
        m.insertItem(TR("Tool"), &toolm);

    if (cpp)
        gensubm.insertItem("C++", 14);

    if (java)
        gensubm.insertItem("Java", 15);

    if (php)
        gensubm.insertItem("Php", 19);

    if (python)
        gensubm.insertItem("Python", 20);

    if (idl)
        gensubm.insertItem("Idl", 16);

    QStringList::Iterator it;

    if (attributes.count() != 0) {
        attributes.full_defs(attributes_def);
        index = 1000;

        for (it = attributes_def.begin(); it != attributes_def.end(); ++it)
            attrsubm.insertItem(*it, index++);
    }

    if ((operations.count() != 0) && (operations.count() <= 20)) {
        operations.full_defs(operations_def);
        index = 2000;

        for (it = operations_def.begin(); it != operations_def.end(); ++it)
            opersubm.insertItem(*it, index++);
    }

    switch (index = m.exec(QCursor::pos())) {
    case 0:
        upper();
        break;

    case 1:
        lower();
        break;

    case 21:
        z_up();
        break;

    case 22:
        z_down();
        break;

    case 2:
        edit_drawing_settings();
        return;

    case 3: {
        BooL visible = indicate_visible_attr;
        HideShowDialog dialog(attributes, hidden_visible_attributes, visible);

        dialog.raise();

        if (dialog.exec() != QDialog::Accepted)
            return;

        indicate_visible_attr = visible;
    }
    break;

    case 4: {
        BooL visible = indicate_visible_oper;
        HideShowDialog dialog(operations, hidden_visible_operations, visible);

        dialog.raise();

        if (dialog.exec() != QDialog::Accepted)
            return;

        indicate_visible_oper = visible;
    }
    break;

    case 5:
        ((UmlCanvas *) canvas())->get_view()
        ->add_related_elements(this, TR("class"), TRUE, TRUE);
        return;

    case 6:
        browser_node->open(TRUE);
        return;

    case 7:
        browser_node->apply_shortcut("New item");
        return;

    case 9:
        browser_node->apply_shortcut("New attribute");
        return;

    case 8:
        browser_node->apply_shortcut("New operation");
        return;

    case 10:
        browser_node->select_in_browser();
        return;

    case 11:
        ((BrowserClass *) browser_node)->set_associated_diagram((BrowserClassDiagram *)
                the_canvas()->browser_diagram());
        return;

    case 18:
        ((BrowserClass *) browser_node)
        ->set_associated_diagram(0);
        return;

    case 12:
        //remove from diagram
        delete_it();
        break;

    case 13:
        //delete from model
        browser_node->delete_it();	// will delete the canvas
        break;

    case 14:
        browser_node->apply_shortcut("Generate C++");
        return;

    case 15:
        browser_node->apply_shortcut("Generate Java");
        return;

    case 16:
        browser_node->apply_shortcut("Generate Idl");
        return;

    case 17:
        the_canvas()->unselect_all();
        select_associated();
        return;

    case 19:
        browser_node->apply_shortcut("Generate Php");
        return;

    case 20:
        browser_node->apply_shortcut("Generate Python");
        return;

    case 23:
        (new ArrowCanvas(the_canvas(), this, nesting_cl, UmlInner, 0, FALSE, -1.0, -1.0))
        ->show();
        break;

    case 1999: {
        OperationListDialog dialog(TR("Choose operation to edit"),
                                   (QList<BrowserOperation *> &) operations);

        dialog.raise();

        if (dialog.exec() == QDialog::Accepted)
            operations.at(dialog.choosen())->open(FALSE);
    }

    return;

    case 2999: {
        l = ((BrowserClass *) browser_node)->inherited_operations(~0u);

        OperationListDialog dialog(TR("Choose inherited operation to add it"), l);

        dialog.raise();

        if (dialog.exec() == QDialog::Accepted)
            ((BrowserClass *) browser_node)->add_inherited_operation(l.at(dialog.choosen()));
    }

    return;

    default:
        if (index >= 3000)
            // inherited operation
            ((BrowserClass *) browser_node)->add_inherited_operation(l.at(index - 3000));
        else if (index >= 2000)
            // operation
            operations.at(index - 2000)->open(FALSE);
        else if (index >= 1000)
            // attribute
            attributes.at(index - 1000)->open(FALSE);
        else if (index >= 30)
            ToolCom::run(Tool::command(index - 30), browser_node);

        return;
    }

    modified();
    package_modified();
}

void CdClassCanvas::apply_shortcut(QString s)
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
        ->add_related_elements(this, TR("class"), TRUE, TRUE);
        return;
    }
    else {
        browser_node->apply_shortcut(s);
        return;
    }

    modified();
    package_modified();
}

void CdClassCanvas::edit_drawing_settings()
{
    for (;;) {
        StateSpecVector st;
        ColorSpecVector co(1);

        settings.complete(st, UmlClass);

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

bool CdClassCanvas::has_drawing_settings() const
{
    return TRUE;
}

void CdClassCanvas::edit_drawing_settings(QList<DiagramItem *> & l)
{
    for (;;) {
        StateSpecVector st;
        ColorSpecVector co(1);
        UmlColor itscolor;
        ClassDiagramSettings settings;

        settings.complete(st, UmlClass);

        co[0].set(TR("class color"), &itscolor);

        SettingsDialog dialog(&st, &co, FALSE, TRUE);

        dialog.raise();

        if (dialog.exec() == QDialog::Accepted) {
            foreach (DiagramItem *item, l) {
                CdClassCanvas *canvas = (CdClassCanvas *)item;
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

void CdClassCanvas::clone_drawing_settings(const DiagramItem *src)
{
    const CdClassCanvas * x = (const CdClassCanvas *) src;
    itscolor = x->itscolor;
    settings = x->settings;
    modified();
    package_modified();
}

QString CdClassCanvas::may_start(UmlCode & l) const
{
    return ((BrowserClass *) browser_node)->may_start(l);
}

QString CdClassCanvas::may_connect(UmlCode & l, const DiagramItem * dest) const
{
    if (l == UmlAnchor)
        return (IsaRelation(dest->type()))
               ? ((RelationCanvas *) dest)->may_connect(l, this)
               : dest->may_start(l);

    switch (dest->type()) {
    case UmlClass:
        return ((BrowserClass *) browser_node)
               ->may_connect(l, (BrowserClass *) dest->get_bn());

    case UmlArrowPoint:
        return TR("illegal");

    case UmlPackage:
        if (l != UmlDependency)
            return TR("illegal");

        l = UmlDependOn;
        return 0;

    default:
        return TR("illegal");
    }
}

void CdClassCanvas::post_connexion(UmlCode l, DiagramItem * dest)
{
    switch (l) {
    case UmlAnchor:
        if (IsaRelation(dest->type()))
            ((RelationCanvas *) dest)->post_connexion(l, this);

        break;

    case UmlGeneralisation:
    case UmlRealize:
        if (!strcmp(browser_node->get_data()->get_stereotype(), "stereotype"))
            ProfiledStereotypes::recompute(TRUE);

        break;

    default:
        break;
    }
}

void CdClassCanvas::connexion(UmlCode action, DiagramItem * dest,
                              const QPoint &, const QPoint &)
{
    ArrowCanvas * a;

    if (IsaRelation(action))
        a = new RelationCanvas(the_canvas(), this, dest, 0, action, 0, -1.0, -1.0);
    else if (IsaSimpleRelation(action))
        a = new SimpleRelationCanvas(the_canvas(), this, dest, 0, action, 0, -1.0, -1.0);
    else
        a = new ArrowCanvas(the_canvas(), this, dest, action, 0, FALSE, -1.0, -1.0);

    a->show();
    the_canvas()->select(a);
}

aCorner CdClassCanvas::on_resize_point(const QPoint & p)
{
    return (used_view_mode != asClass)
           ? NoCorner
           : ::on_resize_point(p, rect());
}

void CdClassCanvas::resize(aCorner c, int dx, int dy, QPoint & o)
{
    int ow = width();
    int oh = height();

    DiagramCanvas::resize(c, dx, dy, o, width_min, height_min, TRUE);

    int nw = width();
    int nh = height();

    if ((nw = width_min) && (nh == height_min))
        manual_size = FALSE;
    else
        manual_size |= ((nw != ow) || (nh != oh));
}

void CdClassCanvas::resize(const QSize & sz, bool w, bool h)
{
    int ow = width();
    int oh = height();

    if (DiagramCanvas::resize(sz, w, h, width_min, height_min, TRUE)) {
        int nw = width();
        int nh = height();

        if ((nw = width_min) && (nh == height_min))
            manual_size = FALSE;
        else
            manual_size |= ((nw != ow) || (nh != oh));
    }
}

//

static void save_hidden_list(BrowserNode * bn, UmlCode c, QTextStream & st,
                             const char * s,
                             const Q3ValueList<BrowserNode *> & hidden_visible)
{
    BrowserNodeList l;

    bn->children(l, c);

    foreach (BrowserNode *node, l) {
        QString dummy;

        if (hidden_visible.findIndex(node) != -1) {
            if (s != 0) {
                nl_indent(st);
                st << s;
                s = 0;
            }

            nl_indent(st);
            st << "  ";
            node->save(st, TRUE, dummy);
        }
    }

    if ((s != 0) && (*s == 'v')) {
        nl_indent(st);
        st << s;
    }
}

void CdClassCanvas::save(QTextStream & st, bool ref, QString & warning) const
{
    if (ref) {
        st << "classcanvas_ref " << get_ident() << " // "
           << browser_node->full_name();
    }
    else {
        nl_indent(st);
        st << "classcanvas " << get_ident() << " ";
        browser_node->save(st, TRUE, warning);
        indent(+1);
        settings.save(st);

        if (itscolor != UmlDefaultColor) {
            nl_indent(st);
            st << "color " << stringify(itscolor);
        }

        save_hidden_list(browser_node, UmlAttribute, st,
                         (indicate_visible_attr) ? "visible_attributes"
                         : "hidden_attributes",
                         hidden_visible_attributes);
        save_hidden_list(browser_node, UmlOperation, st,
                         (indicate_visible_oper) ? "visible_operations"
                         : "hidden_operations",
                         hidden_visible_operations);
        nl_indent(st);

        if (manual_size)
            save_xyzwh(st, this, "xyzwh");
        else
            save_xyz(st, this, "xyz");

        if (constraint != 0)
            constraint->save(st, FALSE, warning);

        save_stereotype_property(st, warning);

        indent(-1);
        nl_indent(st);
        st << "end";
    }
}

CdClassCanvas * CdClassCanvas::read(char *& st, UmlCanvas * canvas,
                                    char * k)
{
    if (!strcmp(k, "classcanvas_ref"))
        return ((CdClassCanvas *) dict_get(read_id(st), "classcanvas", canvas));
    else if (!strcmp(k, "classcanvas")) {
        int id = read_id(st);
        BrowserClass * br = BrowserClass::read_ref(st);
        CdClassCanvas * result = new CdClassCanvas(canvas, id);

        result->browser_node = br;
        result->subscribe(br->get_data());	// = TRUE
        connect(br->get_data(), SIGNAL(changed()), result, SLOT(modified()));
        connect(br->get_data(), SIGNAL(deleted()), result, SLOT(deleted()));

        k = read_keyword(st);

        result->settings.read(st, k);	// updates k

        if (result->settings.draw_all_relations == UmlNo)
            // old release
            canvas->dont_draw_all_relations();

        result->settings.draw_all_relations = UmlDefaultState;

        read_color(st, "color", result->itscolor, k);	// updates k

        if (!strcmp(k, "hidden_attributes") || !strcmp(k, "visible_attributes")) {
            result->indicate_visible_attr = (*k == 'v');

            BrowserNodeList l;

            br->children(l, UmlAttribute);

            while (strcmp(k = read_keyword(st), "hidden_operations") &&
                   strcmp(k, "visible_operations") &&
                   strcmp(k, "xyzwh") &&
                   strcmp(k, "xyz")) {
                BrowserNode * b = BrowserAttribute::read(st, k, 0, FALSE);

                if ((b != 0) && (l.contains(b)))
                    result->hidden_visible_attributes.append(b);
            }
        }

        if (!strcmp(k, "hidden_operations") || !strcmp(k, "visible_operations")) {
            result->indicate_visible_oper = (*k == 'v');

            BrowserNodeList l;

            br->children(l, UmlOperation);

            while (strcmp(k = read_keyword(st), "xyzwh") && strcmp(k, "xyz")) {
                BrowserNode * b = BrowserOperation::read(st, k, 0, FALSE);

                if ((b != 0) && (l.contains(b)))
                    result->hidden_visible_operations.append(b);
            }
        }

        if (!strcmp(k, "xyz"))
            read_xyz(st, result);
        else if (!strcmp(k, "xyzwh")) {
            read_xyzwh(st, result);
            result->manual_size = TRUE;
        }
        else
            wrong_keyword(k, "xyz");

        if (read_file_format() >= 37) {
            k = read_keyword(st);

            if (! strcmp(k, "manual_size")) {
                result->manual_size = TRUE;
                k = read_keyword(st);
            }

            if (! strcmp(k, "constraint")) {
                result->constraint = ConstraintCanvas::read(st, canvas, k, result);
                k = read_keyword(st);
            }

            result->read_stereotype_property(st, k);

            if (strcmp(k, "end"))
                wrong_keyword(k, "end");
        }

        result->check_size();

        if ((read_file_format() < 72) &&
            (result->used_view_mode == asInterface)) {
            result->settings.class_drawing_mode = asClass;
            result->check_size();
        }

        result->set_center100();
        result->show();
        result->check_constraint();
        result->check_stereotypeproperties();
        return result;
    }
    else
        return 0;
}

void CdClassCanvas::history_save(QBuffer & b) const
{
    DiagramCanvas::history_save(b);
    ::save((int) manual_size, b);

    if (manual_size) {
        ::save(width_scale100, b);
        ::save(height_scale100, b);
        ::save(width(), b);
        ::save(height(), b);
    }
}

void CdClassCanvas::history_load(QBuffer & b)
{
    DiagramCanvas::history_load(b);

    int ms;

    ::load(ms, b);
    manual_size = (ms != 0);

    if (manual_size) {
        ::load(width_scale100, b);
        ::load(height_scale100, b);

        int w, h;

        ::load(w, b);
        ::load(h, b);
        Q3CanvasRectangle::setSize(w, h);
    }
    else
        check_size();

    connect(browser_node->get_data(), SIGNAL(changed()), this, SLOT(modified()));
    connect(browser_node->get_data(), SIGNAL(deleted()), this, SLOT(deleted()));
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
}

void CdClassCanvas::history_hide()
{
    Q3CanvasItem::setVisible(FALSE);

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
