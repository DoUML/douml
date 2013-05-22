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
//Added by qt3to4:
#include <QTextStream>
#include <math.h>
#include <q3popupmenu.h>

#include "RelationCanvas.h"
#include "ArrowPointCanvas.h"
#include "CdClassCanvas.h"
#include "UmlCanvas.h"
#include "LabelCanvas.h"
#include "BrowserDiagram.h"
#include "DiagramView.h"
#include "BrowserRelation.h"
#include "BrowserClass.h"
#include "ClassData.h"
#include "RelationData.h"
#include "UmlWindow.h"
#include "UmlGlobal.h"
#include "myio.h"
#include "ToolCom.h"
#include "Tool.h"
#include "ui/menufactory.h"
#include "DialogUtil.h"
#include "strutil.h"
#include "ProfiledStereotypes.h"
#include "StereotypePropertiesCanvas.h"
#include "translate.h"

RelationCanvas::RelationCanvas(UmlCanvas * canvas, DiagramItem * b,
                               DiagramItem * e, BrowserClass * bb,
                               UmlCode t, int id, float d_start, float d_end,
                               RelationData * d)
    : ArrowCanvas(canvas, b, e, t, id, TRUE, d_start, d_end), br_begin(bb),
      data(d), role_a(0), role_b(0), multiplicity_a(0), multiplicity_b(0),
      stereotypeproperties(0), show_modifier(FALSE), show_visibility(FALSE)
{
    if ((e->type() != UmlArrowPoint) && (bb == 0)) {
        // end of line construction, update all the line bb & be
        // adds browser relation 2 times
        update_begin(e);
    }
    else if (d != 0) {
        if (e->type() != UmlArrowPoint) {
            check_stereotypeproperties();
        }

        connect(d, SIGNAL(changed()), this, SLOT(modified()));
        connect(d, SIGNAL(deleted()), this, SLOT(deleted()));
        connect(br_begin->get_data(), SIGNAL(actuals_changed()),
                this, SLOT(actuals_modified()));
        connect(DrawingSettings::instance(), SIGNAL(changed()),
                this, SLOT(drawing_settings_modified()));
    }

    // manages the case start == end
    if (b == e)
        cut_self();

    if (d != 0)
        update(TRUE);
}

RelationCanvas::~RelationCanvas()
{
}

void RelationCanvas::delete_it()
{
    if (stereotypeproperties != 0)
        ((UmlCanvas *) canvas())->del(stereotypeproperties);

    ArrowCanvas::delete_it(); // call unconnect
}

void RelationCanvas::unconnect()
{
    disconnect(data, 0, this, 0);
    disconnect(br_begin->get_data(), SIGNAL(actuals_changed()),
               this, SLOT(actuals_modified()));
    disconnect(DrawingSettings::instance(), SIGNAL(changed()),
               this, SLOT(drawing_settings_modified()));

    ArrowCanvas::unconnect();
}

void RelationCanvas::deleted()
{
    delete_it();
    canvas()->update();
    package_modified();
}

void RelationCanvas::delete_available(BooL & in_model, BooL & out_model) const
{
    out_model |= TRUE;
    in_model |= (data->get_end())
                ? data->is_writable(data->get_end())
                : data->is_writable(data->get_start());
}

void RelationCanvas::remove(bool from_model)
{
    if (! from_model) {
        if (the_canvas()->must_draw_all_relations()) {
            const RelationCanvas * a = this;

            while (a->begin->type() == UmlArrowPoint) {
                a = (RelationCanvas *)((ArrowPointCanvas *) a->begin)->get_other(a);

                if (a == 0)
                    break;
            }

            if (a && !a->begin->isSelected() && !a->begin->get_bn()->deletedp()) {
                a = this;

                while (a->end->type() == UmlArrowPoint) {
                    a = (RelationCanvas *)((ArrowPointCanvas *) a->end)->get_other(a);

                    if (a == 0)
                        break;
                }

                if (a && !a->end->isSelected() && !a->end->get_bn()->deletedp()) {
                    msg_warning("Douml", TR("<i>Draw all relations</i> forced to <i>no</i>"));
                    the_canvas()->dont_draw_all_relations();
                }
            }
        }

        delete_it();
    }
    else {
        bool from_st = ((itstype == UmlGeneralisation) || (itstype == UmlRealize)) &&
                       !strcmp(data->get_start_class()->get_data()->get_stereotype(), "stereotype");

        data->delete_it();	// will remove canvas

        if (from_st)
            ProfiledStereotypes::recompute(TRUE);
    }
}


// Updates br_begin going back, Returns the start browser node

BrowserClass * RelationCanvas::update_begin(DiagramItem * cnend)
{
    // static to be updated in all the cases
    static RelationData * d = 0;

    if (begin->type() == UmlArrowPoint) {
        RelationCanvas * other =
            ((RelationCanvas *)((ArrowPointCanvas *) begin)->get_other(this));

        br_begin = other->update_begin(cnend);	// update d
    }
    else {
        // the relation start here
        br_begin = ((BrowserClass *) begin->get_bn());
        d = (RelationData *) begin->add_relation(type(), cnend);
    }

    data = d;

    connect(data, SIGNAL(changed()), this, SLOT(modified()));
    connect(data, SIGNAL(deleted()), this, SLOT(deleted()));
    connect(br_begin->get_data(), SIGNAL(actuals_changed()),
            this, SLOT(actuals_modified()));
    connect(DrawingSettings::instance(), SIGNAL(changed()),
            this, SLOT(drawing_settings_modified()));

    return br_begin;
}

void RelationCanvas::update_pos()
{
    ArrowCanvas::update_pos();

    if (auto_pos) {
        if ((role_a != 0) && !role_a->selected())
            role_a_default_position();

        if ((role_b != 0) && !role_b->selected())
            role_b_default_position();

        if ((multiplicity_a != 0) && !multiplicity_a->selected())
            multiplicity_a_default_position();

        if ((multiplicity_b != 0) && !multiplicity_b->selected())
            multiplicity_b_default_position();
    }

    update_show_lines();
}

void RelationCanvas::setVisible(bool yes)
{
    ArrowCanvas::setVisible(yes);

    if (role_a != 0)
        role_a->setVisible(yes);

    if (role_b != 0)
        role_b->setVisible(yes);

    if (multiplicity_a != 0)
        multiplicity_a->setVisible(yes);

    if (multiplicity_b != 0)
        multiplicity_b->setVisible(yes);

    if (stereotypeproperties)
        stereotypeproperties->setVisible(yes);
}

void RelationCanvas::moveBy(double dx, double dy)
{
    ArrowCanvas::moveBy(dx, dy);

    if ((role_a != 0) && !role_a->selected())
        role_a->moveBy(dx, dy);

    if ((role_b != 0) && !role_b->selected())
        role_b->moveBy(dx, dy);

    if ((multiplicity_a != 0) && !multiplicity_a->selected())
        multiplicity_a->moveBy(dx, dy);

    if ((multiplicity_b != 0) && !multiplicity_b->selected())
        multiplicity_b->moveBy(dx, dy);

    if ((stereotypeproperties != 0) && !stereotypeproperties->selected())
        stereotypeproperties->moveBy(dx, dy);
}

void RelationCanvas::set_z(double z)
{
    ArrowCanvas::set_z(z);

    if (role_a != 0)
        role_a->setZ(z);

    if (role_b != 0)
        role_b->setZ(z);

    if (multiplicity_a != 0)
        multiplicity_a->setZ(z);

    if (multiplicity_b != 0)
        multiplicity_b->setZ(z);
}

void RelationCanvas::setSelected(bool yes)
{
    UmlWindow::set_commented((yes) ? data->get_start() : 0);

    Q3CanvasPolygon::setSelected(yes);
}

void RelationCanvas::select_associated()
{
    // special case to move a class with its self relations goes
    // throw all the relation (even the already selected sections)
    // and does not check that this is or not already selected

    RelationCanvas * rel = this;

    // goes to the beginning of the (broken) line
    while (rel->begin->type() == UmlArrowPoint)
        rel = ((RelationCanvas *)((ArrowPointCanvas *) rel->begin)->get_other(rel));

    rel->begin->select_associated();

    if ((rel->stereotypeproperties != 0) && !rel->stereotypeproperties->selected())
        the_canvas()->select(rel->stereotypeproperties);

    for (;;) {
        // select labels
        if ((rel->role_a != 0) && !rel->role_a->selected())
            the_canvas()->select(rel->role_a);

        if ((rel->role_b != 0) && !rel->role_b->selected())
            the_canvas()->select(rel->role_b);

        if ((rel->multiplicity_a != 0) && !rel->multiplicity_a->selected())
            the_canvas()->select(rel->multiplicity_a);

        if ((rel->multiplicity_b != 0) && !rel->multiplicity_b->selected())
            the_canvas()->select(rel->multiplicity_b);

        if ((rel->label != 0) && !rel->label->selected())
            the_canvas()->select(rel->label);

        if ((rel->stereotype != 0) && !rel->stereotype->selected())
            the_canvas()->select(rel->stereotype);

        if (! rel->selected())
            the_canvas()->select(rel);

        // goes forward
        if (rel->end->type() != UmlArrowPoint)
            break;

        if (!((ArrowPointCanvas *) rel->end)->selected())
            the_canvas()->select((ArrowPointCanvas *) rel->end);

        rel = (RelationCanvas *)((ArrowPointCanvas *) rel->end)->get_other(rel);
    }

    rel->end->select_associated();
}

bool RelationCanvas::reflexive() const
{
    return (get_start() == get_end());
}

void RelationCanvas::open()
{
    if (data &&
        !data->get_start()->in_edition())	// for dble click on diagram
        data->edit();
}

void RelationCanvas::menu(const QPoint & lpos)
{
    if ((data != 0) && !data->get_start()->in_edition()) {
        RelationCanvas * plabel;
        RelationCanvas * pstereotype;

        {
            ArrowCanvas * aplabel;
            ArrowCanvas * apstereotype;

            search_supports(aplabel, apstereotype);
            plabel = (RelationCanvas *) aplabel;
            pstereotype = (RelationCanvas *) apstereotype;
        }

        const RelationCanvas * first = this;
        const RelationCanvas * last = this;

        while (first->begin->type() == UmlArrowPoint)
            first = ((RelationCanvas *)((ArrowPointCanvas *) first->begin)->get_other(first));

        while (last->end->type() == UmlArrowPoint)
            last = (RelationCanvas *)((ArrowPointCanvas *) last->end)->get_other(last);

        bool prefer_start =
            (lpos - first->beginp).manhattanLength() >
            (lpos - last->endp).manhattanLength();
        Q3PopupMenu m(0);
        Q3PopupMenu geo(0);
        Q3PopupMenu toolm(0);

        MenuFactory::createTitle(m, data->definition(FALSE, TRUE));
        m.insertSeparator();
        m.insertItem(TR("Edit"), 0);
        m.insertSeparator();

        m.insertItem(TR("Select in browser"), 2);

        if (plabel || pstereotype ||
            first->role_b || first->multiplicity_b ||
            last->role_a || last->multiplicity_a) {
            m.insertSeparator();
            m.insertItem(TR("Select labels"), 3);
            m.insertItem(TR("Labels default position"), 4);

            if (plabel && (label == 0))
                m.insertItem(TR("Attach relation's name to this segment"), 5);

            if (pstereotype && (stereotype == 0))
                m.insertItem(TR("Attach relation's stereotype to this segment"), 6);
        }

        if (get_start() != get_end()) {
            m.insertSeparator();
            init_geometry_menu(geo, 10);
            m.insertItem(TR("Geometry (Ctrl+l)"), &geo);
        }

        m.insertSeparator();
        m.insertItem(TR("Remove from diagram"), 7);

        if ((data->get_end()) ? data->is_writable(data->get_end())
            : data->is_writable(data->get_start()))
            m.insertItem(TR("Delete from model"), 8);

        m.insertSeparator();

        if (Tool::menu_insert(&toolm, itstype, 20))
            m.insertItem(TR("Tool"), &toolm);

        int rank = m.exec(QCursor::pos());

        switch (rank) {
        case 0:
            data->edit();
            return;

        case 2:
            data->select_in_browser(prefer_start);
            return;

        case 3:
            the_canvas()->unselect_all();

            if (plabel)
                the_canvas()->select(plabel->label);

            if (last->role_a)
                the_canvas()->select(last->role_a);

            if (first->role_b)
                the_canvas()->select(first->role_b);

            if (last->multiplicity_a)
                the_canvas()->select(last->multiplicity_a);

            if (first->multiplicity_b)
                the_canvas()->select(first->multiplicity_b);

            if (pstereotype)
                the_canvas()->select(pstereotype->stereotype);

            return;

        case 4:
            if (plabel)
                plabel->label_default_position();

            if (last->role_a != 0)
                last->role_a_default_position();

            if (first->role_b != 0)
                first->role_b_default_position();

            if (last->multiplicity_a != 0)
                last->multiplicity_a_default_position();

            if (first->multiplicity_b != 0)
                first->multiplicity_b_default_position();

            if (pstereotype)
                pstereotype->stereotype_default_position();

            break;

        case 5:
            label = plabel->label;
            plabel->label = 0;
            label_default_position();
            return;

        case 6:
            stereotype = pstereotype->stereotype;
            pstereotype->stereotype = 0;
            stereotype_default_position();
            return;

        case 7:
            // not removed from the browser : just hide it
            remove(FALSE);
            break;

        case 8:
            remove(TRUE);	// will delete the canvas
            break;

        default:
            if (rank >= 20) {
                ToolCom::run(Tool::command(rank - 20),
                             ((prefer_start || RelationData::uni_directional(itstype))
                              ? data->get_start()
                              : data->get_end()));
                return;
            }
            else if (rank >= 10) {
                rank -= 10;

                if (rank == RecenterBegin)
                    set_decenter(-1.0, decenter_end);
                else if (rank == RecenterEnd)
                    set_decenter(decenter_begin, -1.0);
                else if (rank != (int) geometry)
                    set_geometry((LineGeometry) rank, TRUE);
                else
                    return;
            }
            else
                return;
        }

        package_modified();
    }
}


ArrowPointCanvas * RelationCanvas::brk(const QPoint & p)
{
    ArrowPointCanvas * ap =
        new ArrowPointCanvas(the_canvas(), p.x(), p.y());

    ap->setZ(z() + 1);

    DiagramItem * e = end;

    ap->add_line(this);
    end->remove_line(this, TRUE);
    end = ap;

    RelationCanvas * other =
        // do not give data to not call update()
        new RelationCanvas(the_canvas(), ap, e, br_begin, itstype, 0,
                           decenter_begin, decenter_end);

    other->data = data;
    other->show_modifier = show_modifier;
    other->show_visibility = show_visibility;
    connect(data, SIGNAL(changed()), other, SLOT(modified()));
    connect(data, SIGNAL(deleted()), other, SLOT(deleted()));
    connect(br_begin->get_data(), SIGNAL(actuals_changed()),
            other, SLOT(actuals_modified()));
    connect(DrawingSettings::instance(), SIGNAL(changed()),
            this, SLOT(drawing_settings_modified()));

    if ((p - beginp).manhattanLength() < (p - endp).manhattanLength()) {
        if (label != 0) {
            other->label = label;
            label = 0;
        }

        if (stereotype != 0) {
            other->stereotype = stereotype;
            stereotype = 0;
        }
    }

    ap->show();
    other->show();

    modified();		// to set/unset name, role_b and multiplicity_b
    other->modified();	// to set/unset role_a and multiplicity_a

    return ap;
}


ArrowCanvas * RelationCanvas::join(ArrowCanvas * other, ArrowPointCanvas * ap)
{
    // has already check is join is possible (self relation must have two points)
    ArrowCanvas * result = ArrowCanvas::join(other, ap);
    ((RelationCanvas *) result)->modified();

    return result;
}

void RelationCanvas::modified()
{
    if (visible()) {
        hide();
        update(TRUE);
        show();
        canvas()->update();

        if (begin->type() != UmlArrowPoint) {
            if (data->get_association().type != 0)
                show_assoc_class(0);
            else
                hide_assoc_class();

            check_stereotypeproperties();
        }

        package_modified();
    }
}

void RelationCanvas::actuals_modified()
{
    if ((begin != 0) && (end != 0) &&	// not removed because of a join
        (data->get_type() == UmlRealize)) {
        ArrowCanvas * aplabel;
        ArrowCanvas * apstereotype;

        search_supports(aplabel, apstereotype);
        update_actuals((RelationCanvas *) aplabel);
    }
}

void RelationCanvas::drawing_settings_modified()
{
    ClassDiagramSettings settings;

    the_canvas()->browser_diagram()->get_classdiagramsettings(settings);

    if ((show_modifier != (settings.show_relation_modifiers == UmlYes)) ||
        (show_visibility != (settings.show_relation_visibility == UmlYes)))
        update(TRUE);
}

void RelationCanvas::update(bool updatepos)
{
    if (data) {
        itstype = data->get_type();

        bool unamed = !RelationData::isa_association(itstype);
        QFontMetrics fm(the_canvas()->get_font(UmlNormalFont));
        int h = fm.height();
        RelationCanvas * plabel;
        RelationCanvas * pstereotype;

        {
            ArrowCanvas * aplabel;
            ArrowCanvas * apstereotype;

            search_supports(aplabel, apstereotype);
            plabel = (RelationCanvas *) aplabel;
            pstereotype = (RelationCanvas *) apstereotype;
        }

        // manages relation's name

        QString s = data->get_name();
        QPoint c = center();

        if (itstype == UmlRealize)
            update_actuals(plabel);
        else if (unamed || s.isEmpty() || (s == RelationData::default_name(itstype))) {
            // relation does not have name or 'true' name
            if (plabel != 0) {
                // removes it
                the_canvas()->del(plabel->label);
                plabel->label = 0;
            }
        }
        else if ((plabel == 0) && (begin->type() != UmlArrowPoint)) {
            // adds relation's name
            label =
                new LabelCanvas(s, the_canvas(), c.x() - fm.width(s) / 2,
                                c.y() - h, FALSE, TRUE, FALSE);
        }
        else if ((plabel != 0) && (plabel->label->get_name() != s)) {
            // update name
            plabel->label->set_name(s);
            plabel->label_default_position();
        }

        // manages relation's stereotype

        s = data->get_short_stereotype();

        if (s.isEmpty()) {
            // relation does not have stereotype
            if (pstereotype != 0) {
                // removes it
                the_canvas()->del(pstereotype->stereotype);
                pstereotype->stereotype = 0;
            }
        }
        else {
            s = toUnicode(s);

            if (s[0] != '{')
                s = QString("<<") + s + ">>";

            if ((pstereotype == 0) && (begin->type() != UmlArrowPoint)) {
                // adds relation's stereotype
                stereotype = new LabelCanvas(s, the_canvas(), 0, 0);
                stereotype_default_position();
            }
            else if ((pstereotype != 0) &&
                     (pstereotype->stereotype->get_name() != s)) {
                // update name
                pstereotype->stereotype->set_name(s);
                pstereotype->stereotype_default_position();
            }
        }

        // manages role_a

        static const QString v[] = { "+", "#", "-", "~" };

        ClassDiagramSettings settings;

        the_canvas()->browser_diagram()->get_classdiagramsettings(settings);

        show_modifier = (settings.show_relation_modifiers == UmlYes);
        show_visibility = (settings.show_relation_visibility == UmlYes);

        s = data->get_role_a();

        if (unamed || s.isEmpty() || (end->type() == UmlArrowPoint)) {
            // relation does not have role_a name or it must be hidden
            if (role_a != 0) {
                the_canvas()->del(role_a);
                role_a = 0;
            }
        }
        else {
            if (data->get_is_derived_a())
                s = "/" + s;

            if (show_visibility) {
                int vi = ((settings.drawing_language == CppView) &&
                          (data->get_cpp_visibility_a() != UmlDefaultVisibility))
                         ? data->get_cpp_visibility_a() : data->get_uml_visibility_a();

                if ((vi == UmlPackageVisibility) &&
                    ((settings.drawing_language == CppView) || (settings.drawing_language == IdlView)))
                    vi = UmlPublic;

                s = v[vi] + s;
            }

            if (show_modifier) {
                const char * sep = " {";

                if (data->get_isa_const_relation_a()) {
                    s += QString(sep) + "readOnly";
                    sep = ",";
                }

                if (data->get_is_derived_a() && data->get_is_derivedunion_a()) {
                    s += QString(sep) + "union";
                    sep = ",";
                }

                if (data->get_is_ordered_a()) {
                    s += QString(sep) + "ordered";
                    sep = ",";
                }

                if (data->get_is_unique_a()) {
                    s += QString(sep) + "unique";
                    sep = ",";
                }

                if (*sep == ',')
                    s += "}";
            }

            if (role_a == 0) {
                // adds role_a
                role_a = new LabelCanvas(s, the_canvas(), 0, 0, FALSE, FALSE,
                                         data->get_isa_class_relation_a());
                role_a_default_position();
            }
            else {
                if (role_a->get_name() != s) {
                    role_a->set_name(s);
                    role_a_default_position();
                }

                role_a->set_underlined(data->get_isa_class_relation_a());
            }
        }


        // manages role_b

        s = data->get_role_b();

        if (unamed ||
            s.isEmpty() ||
            RelationData::uni_directional(itstype) ||
            (begin->type() == UmlArrowPoint)) {
            // relation does not have role_b name
            if (role_b != 0) {
                the_canvas()->del(role_b);
                role_b = 0;
            }
        }
        else {
            if (data->get_is_derived_b())
                s = "/" + s;

            if (show_visibility) {
                int vi = ((settings.drawing_language == CppView) &&
                          (data->get_cpp_visibility_b() != UmlDefaultVisibility))
                         ? data->get_cpp_visibility_b() : data->get_uml_visibility_b();

                if ((vi == UmlPackageVisibility) &&
                    ((settings.drawing_language == CppView) || (settings.drawing_language == IdlView)))
                    vi = UmlPublic;

                s = v[vi] + s;
            }

            if (show_modifier) {
                const char * sep = " {";

                if (data->get_isa_const_relation_b()) {
                    s += QString(sep) + "readOnly";
                    sep = ",";
                }

                if (data->get_is_derived_b() && data->get_is_derivedunion_b()) {
                    s += QString(sep) + "union";
                    sep = ",";
                }

                if (data->get_is_ordered_b()) {
                    s += QString(sep) + "ordered";
                    sep = ",";
                }

                if (data->get_is_unique_b()) {
                    s += QString(sep) + "unique";
                    sep = ",";
                }

                if (*sep == ',')
                    s += "}";
            }

            if (role_b == 0) {
                // adds role_b
                role_b = new LabelCanvas(s, the_canvas(), 0, 0, FALSE, FALSE,
                                         data->get_isa_class_relation_b());
                role_b_default_position();
            }
            else {
                if (role_b->get_name() != s) {
                    role_b->set_name(s);
                    role_b_default_position();
                }

                role_b->set_underlined(data->get_isa_class_relation_b());
            }
        }

        // manages multiplicity_a

        s = data->get_multiplicity_a();

        if (unamed || s.isEmpty() || (end->type() == UmlArrowPoint)) {
            // relation does not have multiplicity_a or it must be hidden
            if (multiplicity_a != 0) {
                the_canvas()->del(multiplicity_a);
                multiplicity_a = 0;
            }
        }
        else if (multiplicity_a == 0) {
            // adds multiplicity_a

            multiplicity_a = new LabelCanvas(s, the_canvas(), 0, 0);
            multiplicity_a_default_position();
        }
        else if (multiplicity_a->get_name() != s) {
            multiplicity_a->set_name(s);
            multiplicity_a_default_position();
        }

        // manages multiplicity_b

        s = data->get_multiplicity_b();

        if (unamed || s.isEmpty() || (begin->type() == UmlArrowPoint)) {
            // relation does not have multiplicity_b or it must be hidden
            if (multiplicity_b != 0) {
                the_canvas()->del(multiplicity_b);
                multiplicity_b = 0;
            }
        }
        else if (multiplicity_b == 0) {
            // adds multiplicity_b

            multiplicity_b = new LabelCanvas(s, the_canvas(), 0, 0);
            multiplicity_b_default_position();
        }
        else if ((multiplicity_b != 0) && (multiplicity_b->get_name() != s)) {
            multiplicity_b->set_name(s);
            multiplicity_b_default_position();
        }
    }

    if (updatepos)
        update_pos();
}

void RelationCanvas::update_actuals(RelationCanvas * plabel)
{
    if ((plabel == 0) && (begin->type() == UmlArrowPoint))
        return;

    QString s =
        ((ClassData *) br_begin->get_data())->get_actuals(data->get_end_class());
    QPoint c = center();
    QFontMetrics fm(the_canvas()->get_font(UmlNormalFont));
    int h = fm.height();

    if (s.isEmpty()) {
        // remove label
        if (plabel != 0) {
            // removes it
            the_canvas()->del(plabel->label);
            plabel->label = 0;
        }
    }
    else if (plabel == 0)
        // begin->type() != UmlArrowPoint
        // adds label
        label = new LabelCanvas(s, the_canvas(),
                                c.x() - fm.width(s) / 2, c.y() - h);
    else if (plabel->label->get_name() != s) {
        // update label, position will be updated later
        plabel->label->set_name(s);
        plabel->label_default_position();
    }
}

void RelationCanvas::label_default_position() const
{
    default_label_position();
    package_modified();
}

void RelationCanvas::stereotype_default_position() const
{
    default_stereotype_position();
    package_modified();
}

void RelationCanvas::role_a_default_position() const
{
    QFontMetrics fm(the_canvas()->get_font(UmlNormalFont));
    int h = fm.height();
    int dx = endp.x() - beginp.x();
    int dy = endp.y() - beginp.y();

    if (fabs(dx) >= fabs(dy)) {
        // horizontal line
        if (dx == 0) dx = 1;

        if (dx > 0) {
            int w = fm.width(role_a->get_name());

            role_a->move(endp.x() - w - ARROW_LENGTH,
                         endp.y() - 4 * h / 3 + (dy * ARROW_LENGTH) / dx);
        }
        else
            role_a->move(endp.x() + ARROW_LENGTH,
                         endp.y() - 4 * h / 3 + (dy * ARROW_LENGTH) / dx);
    }
    else {
        // vertical line
        if (dy == 0) dy = 1;

        if (dy > 0)
            role_a->move(endp.x() + ARROW_LENGTH - (dx * h / 2) / dy,
                         endp.y() - 4 * h / 3);
        else
            role_a->move(endp.x() + ARROW_LENGTH + (dx * h / 2) / dy,
                         endp.y() + h / 2);
    }

    package_modified();
}

void RelationCanvas::role_b_default_position() const
{
    QFontMetrics fm(the_canvas()->get_font(UmlNormalFont));
    int h = fm.height();
    int dx = endp.x() - beginp.x();
    int dy = endp.y() - beginp.y();

    if (fabs(dx) >= fabs(dy)) {
        // horizontal line
        if (dx == 0) dx = 1;

        if (dx > 0)
            role_b->move(beginp.x() + ARROW_LENGTH,
                         beginp.y() - 4 * h / 3 + (dy * ARROW_LENGTH) / dx);
        else {
            int w = fm.width(role_b->get_name());

            role_b->move(beginp.x() - w - ARROW_LENGTH,
                         beginp.y() - 4 * h / 3 + (dy * ARROW_LENGTH) / dx);
        }
    }
    else {
        // vertical line
        if (dy == 0) dy = 1;

        if (dy > 0)
            role_b->move(beginp.x() + ARROW_LENGTH + (dx * h / 2) / dy,
                         beginp.y() + h / 2);
        else
            role_b->move(beginp.x() + ARROW_LENGTH - (dx * h / 2) / dy,
                         beginp.y() - 4 * h / 3);
    }

    package_modified();
}

void RelationCanvas::multiplicity_a_default_position() const
{
    QFontMetrics fm(the_canvas()->get_font(UmlNormalFont));
    int h = fm.height();
    int w = fm.width(data->get_multiplicity_a());
    int dx = endp.x() - beginp.x();
    int dy = endp.y() - beginp.y();

    if (fabs(dx) >= fabs(dy)) {
        // horizontal line
        if (dx == 0) dx = 1;

        if (dx > 0)
            multiplicity_a->move(endp.x() - w - ARROW_LENGTH,
                                 endp.y() + h / 2 + (dy * ARROW_LENGTH) / dx);
        else
            multiplicity_a->move(endp.x() + ARROW_LENGTH,
                                 endp.y() + h / 2 + (dy * ARROW_LENGTH) / dx);
    }
    else {
        // vertical line
        if (dy == 0) dy = 1;

        if (dy > 0)
            multiplicity_a->move(endp.x() - w - ARROW_LENGTH - (dx * h / 2) / dy,
                                 endp.y() - 4 * h / 3);
        else
            multiplicity_a->move(endp.x() - w - ARROW_LENGTH + (dx * h / 2) / dy,
                                 endp.y() + h / 2);
    }

    package_modified();
}

void RelationCanvas::multiplicity_b_default_position() const
{
    QFontMetrics fm(the_canvas()->get_font(UmlNormalFont));
    int h = fm.height();
    int dx = endp.x() - beginp.x();
    int dy = endp.y() - beginp.y();
    int w = fm.width(data->get_multiplicity_b());

    if (fabs(dx) >= fabs(dy)) {
        // horizontal line
        if (dx == 0) dx = 1;

        if (dx > 0)
            multiplicity_b->move(beginp.x() + ARROW_LENGTH,
                                 beginp.y() + h / 2 + (dy * ARROW_LENGTH) / dx);
        else
            multiplicity_b->move(beginp.x() - w - ARROW_LENGTH,
                                 beginp.y() + h / 2 + (dy * ARROW_LENGTH) / dx);
    }
    else {
        // vertical line
        if (dy == 0) dy = 1;

        if (dy > 0)
            multiplicity_b->move(beginp.x() - w - ARROW_LENGTH + (dx * h / 2) / dy,
                                 beginp.y() + h / 2);
        else
            multiplicity_b->move(beginp.x() - w - ARROW_LENGTH - (dx * h / 2) / dy,
                                 beginp.y() - 4 * h / 3);
    }

    package_modified();
}

BasicData * RelationCanvas::get_data() const
{
    return data;
}

void RelationCanvas::drop(BrowserNode * bn, UmlCanvas * canvas)
{
    RelationData * def = (RelationData *) bn->get_data();
    BrowserClass * from = def->get_start_class();
    BrowserClass * to = def->get_end_class();
    DiagramItem * ccfrom = 0;
    DiagramItem * ccto = 0;
    Q3CanvasItemList all = canvas->allItems();
    Q3CanvasItemList::Iterator cit;

    // the two classes are drawn ?
    for (cit = all.begin(); cit != all.end(); ++cit) {
        if ((*cit)->visible()) {
            DiagramItem * adi = QCanvasItemToDiagramItem(*cit);

            if ((adi != 0) &&		// an uml canvas item
                (adi->type() == UmlClass)) {
                if (adi->get_bn() == from) {
                    ccfrom = adi;

                    if (ccto != 0)
                        break;
                }

                if (adi->get_bn() == to) {
                    ccto = adi;

                    if (ccfrom != 0)
                        break;
                }
            }
        }
    }

    if ((ccfrom != 0) && (ccto != 0)) {
        if (ccfrom->has_relation(def))
            msg_information("Douml", TR("relation already drawn"));
        else {
            RelationCanvas * rel =
                new RelationCanvas(canvas, ccfrom, ccto, from, bn->get_type(),
                                   0, -1.0, -1.0, def);

            rel->hide();	// else self rel not fully shown
            rel->show();

            if (rel->begin->type() != UmlArrowPoint) {
                if (rel->data->get_association().type != 0)
                    rel->show_assoc_class(0);
                else
                    rel->hide_assoc_class();
            }

            rel->package_modified();
        }
    }
}

// the relation is not yet drawn,
void RelationCanvas::drop(BrowserNode * bn, UmlCanvas * canvas,
                          Q3PtrDict<DiagramItem> & drawn)
{
    RelationData * def = (RelationData *) bn->get_data();
    BrowserClass * from = def->get_start_class();
    BrowserClass * to = def->get_end_class();
    DiagramItem * ccfrom = drawn[from->get_data()];
    DiagramItem * ccto = drawn[to->get_data()];

    if ((ccfrom != 0) && (ccto != 0)) {
        RelationCanvas * rel =
            new RelationCanvas(canvas, ccfrom, ccto, from, bn->get_type(),
                               0, -1.0, -1.0, def);

        rel->hide();	// else self rel not fully shown
        rel->show();

        if (rel->data->get_association().type != 0) {
            DiagramItem * ac = drawn[rel->data->get_association().type->get_data()];

            if (ac != 0)
                rel->show_assoc_class((CdClassCanvas *) ac);
        }

        drawn.replace(def, rel);

        // package set modified by caller
    }
}

// the relation has association class
// this assoc class is cc or cc is 0

void RelationCanvas::show_assoc_class(CdClassCanvas * cc)
{
    if (cc == 0) {
        // search for the assoc class drawing
        BrowserNode * assoc = data->get_association().type;

        Q3CanvasItemList all = canvas()->allItems();
        Q3CanvasItemList::Iterator cit;

        for (cit = all.begin(); cit != all.end(); ++cit) {
            DiagramItem * di = QCanvasItemToDiagramItem(*cit);

            if ((di != 0) &&
                (di->type() == UmlClass) &&
                (((CdClassCanvas *) di)->get_bn() == assoc)) {
                cc = (CdClassCanvas *) di;
                break;
            }
        }

        if (cc == 0)
            return;

        // check if draw_all_relations is desired
        if (! the_canvas()->must_draw_all_relations())
            return;
    }

    // assoc already drawn ?
    RelationCanvas * rc = this;

    while (rc->begin->type() == UmlArrowPoint)
        // go to beginning of line
        rc = (RelationCanvas *)
             ((ArrowPointCanvas *) rc->begin)->get_other(rc);

    RelationCanvas * first = rc;

    for (;;) {
        // goes throw the line and check / search assoc
        foreach (ArrowCanvas *a, rc->lines) {
            if (a->type() == UmlAnchor) {
                DiagramItem * s = a->get_start();
                DiagramItem * e = a->get_end();

                if (s == this) {
                    if (e == cc)
                        // already drawn
                        return;
                    else if (e->type() == UmlClass)
                        // not the good one
                        a->delete_it();
                }
                else if (s == cc)
                    // already drawn
                    return;
                else if (s->type() == UmlClass)
                    // not the good one
                    a->delete_it();
            }
        }

        // goes to the next line part
        if (rc->end->type() == UmlArrowPoint)
            rc = (RelationCanvas *)
                 ((ArrowPointCanvas *) rc->end)->get_other(rc);
        else
            break;
    }

    // not drawn, add it
    (new ArrowCanvas(the_canvas(), first, cc, UmlAnchor, 0, FALSE, -1.0, -1.0))
    ->show();
}

// the relation doesn't have association class

void RelationCanvas::hide_assoc_class()
{
    // assoc drawn ?
    RelationCanvas * rc = this;

    while (rc->begin->type() == UmlArrowPoint)
        // go to beginning of line
        rc = (RelationCanvas *)
             ((ArrowPointCanvas *) rc->begin)->get_other(rc);

    for (;;) {
        // goes through the line to remove existing assoc
        foreach (ArrowCanvas *a, rc->lines) {
            if ((a->type() == UmlAnchor) &&
                ((a->get_start()->type() == UmlClass) ||
                 (a->get_end()->type() == UmlClass))) {
                a->delete_it();

                the_canvas()->get_view()->protect_history(FALSE);
                the_canvas()->get_view()->update_history();
            }
        }

        // goes to the next line part
        if (rc->end->type() == UmlArrowPoint)
            rc = (RelationCanvas *)
                 ((ArrowPointCanvas *) rc->end)->get_other(rc);
        else
            break;
    }
}

QString RelationCanvas::may_connect(UmlCode & l, const DiagramItem * dest) const
{
    if ((l == UmlAnchor) && (dest->type() == UmlClass)) {
        if (data->get_association().type != dest->get_bn())
            return ((data->get_end())
                    ? data->is_writable(data->get_end())
                    : data->is_writable(data->get_start()))
                   ? QString() // ok
                   : TR("read-only relation");
        else
            // ok
            return QString();
    }
    else
        return ArrowCanvas::may_connect(l, dest);
}

void RelationCanvas::post_connexion(UmlCode l, DiagramItem * dest)
{
    if ((l == UmlAnchor) && (dest->type() == UmlClass)) {
        BrowserClass * cl = data->get_association().type;

        if (cl != dest->get_bn()) {
            if (cl != 0)
                hide_assoc_class();

            AType t;

            t.type = (BrowserClass *) dest->get_bn();
            data->set_association(t);
        }
    }
}

void RelationCanvas::check_stereotypeproperties()
{
    // the note is memorized by the first segment
    if (begin->type() == UmlArrowPoint)
        ((RelationCanvas *)((ArrowPointCanvas *) begin)->get_other(this))
        ->check_stereotypeproperties();
    else {
        QString s = data->get_start()->stereotypes_properties();

        if (!s.isEmpty() &&
            the_canvas()->browser_diagram()->get_show_stereotype_properties())
            StereotypePropertiesCanvas::needed(the_canvas(), this, s,
                                               stereotypeproperties, center());
        else if (stereotypeproperties != 0) {
            stereotypeproperties->delete_it();
            stereotypeproperties = 0;
        }
    }
}

bool RelationCanvas::represents(BrowserNode * bn)
{
    return (data == bn->get_data());
}

void RelationCanvas::save(QTextStream & st, bool ref, QString & warning) const
{
    if (ref)
        st << "relationcanvas_ref " << get_ident()
           << " // " << data->get_name();
    else if (begin->type() != UmlArrowPoint) {
        // relation canvas start
        nl_indent(st);
        st << "relationcanvas " << get_ident() << " ";
        data->save(st, TRUE, warning);
        indent(+1);

        if (geometry != NoGeometry) {
            nl_indent(st);
            st << "geometry " << stringify(geometry);

            if (!fixed_geometry)
                st << " unfixed";
        }

        if (decenter_begin >= 0) {
            // float output/input bugged
            nl_indent(st);
            st << "decenter_begin " << ((int)(decenter_begin * 1000));
        }

        if (decenter_end >= 0) {
            // float output/input bugged
            nl_indent(st);
            st << "decenter_end " << ((int)(decenter_end * 1000));
        }

        const RelationCanvas * last =
            (const RelationCanvas *) ArrowCanvas::save_lines(st, TRUE, TRUE, warning);

        nl_indent(st);

        if (last->role_a)
            save_xyz(st, last->role_a, "role_a_pos");
        else
            st << "no_role_a";

        if (role_b)
            save_xyz(st, role_b, " role_b_pos");
        else
            st << " no_role_b";

        nl_indent(st);

        if (last->multiplicity_a)
            save_xyz(st, last->multiplicity_a, "multiplicity_a_pos");
        else
            st << "no_multiplicity_a";

        if (multiplicity_b)
            save_xyz(st, multiplicity_b, " multiplicity_b_pos");
        else
            st << " no_multiplicity_b";

        if (stereotypeproperties != 0)
            stereotypeproperties->save(st, FALSE, warning);

        indent(-1);
        nl_indent(st);
        st << "end";
    }
}

RelationCanvas * RelationCanvas::read(char *& st, UmlCanvas * canvas, char * k)
{
    if (!strcmp(k, "relationcanvas_ref"))
        return ((RelationCanvas *) dict_get(read_id(st), "relationcanvas", canvas));
    else if (!strcmp(k, "relationcanvas")) {
        ClassDiagramSettings settings;

        canvas->browser_diagram()->get_classdiagramsettings(settings);

        bool show_modif = (settings.show_relation_modifiers == UmlYes);
        bool show_visi = (settings.show_relation_visibility == UmlYes);

        int id = read_id(st);
        RelationData * rd = RelationData::read_ref(st, TRUE);
        LineGeometry geo;
        bool fixed;

        k = read_keyword(st);

        if (! strcmp(k, "geometry")) {
            geo = line_geometry(read_keyword(st));
            k = read_keyword(st);

            if (! strcmp(k, "unfixed")) {
                k = read_keyword(st);
                fixed = FALSE;
            }
            else
                fixed = TRUE;
        }
        else {
            geo = NoGeometry;
            fixed = FALSE;
        }

        float dbegin;
        float dend;

        if (! strcmp(k, "decenter_begin")) {
            dbegin = read_double(st) / 1000;
            k = read_keyword(st);
        }
        else
            dbegin = -1;

        if (! strcmp(k, "decenter_end")) {
            dend = read_double(st) / 1000;
            k = read_keyword(st);
        }
        else
            dend = -1;

        unread_keyword(k, st);

        read_keyword(st, "from");
        read_keyword(st, "ref");

        DiagramItem * bi = dict_get(read_id(st), "classcanvas", canvas);
        BrowserClass * b = rd->get_start_class();
        bool temporary = b->is_undefined();
        UmlCode t = rd->get_type();
        bool unamed = !RelationData::isa_association(t);
        QString s = rd->get_name();
        RelationCanvas * first = 0;
        RelationCanvas * result;
        LabelCanvas * label;
        LabelCanvas * stereotype;
        double z;

        for (;;) {
            if (read_file_format() < 5)
                z = OLD_ARROW_Z;
            else {
                read_keyword(st, "z");
                z = read_double(st);
            }

            k = read_keyword(st);

            if ((label = LabelCanvas::read(st, canvas, k)) != 0) {
                if ((t != UmlRealize) &&
                    (unamed || s.isEmpty() || (s == RelationData::default_name(t)))) {
                    // relation does not have name or 'true' name
                    canvas->del(label);
                    label = 0;
                }
                else
                    label->setZ(z);

                k = read_keyword(st);
            }

            if (!strcmp(k, "stereotype")) {
                (void) read_string(st);

                if (read_file_format() < 5)
                    read_keyword(st, "xy");
                else
                    read_keyword(st, "xyz");

                int x = (int) read_double(st);
                int y = (int) read_double(st);

                if (*(rd->get_stereotype()) == 0) {
                    stereotype = 0;

                    if (read_file_format() >= 5)
                        read_double(st);	// z
                }
                else {
                    QString s = toUnicode(rd->get_short_stereotype());

                    if (s[0] != '{')
                        s = QString("<<") + s + ">>";

                    stereotype = new LabelCanvas(s, canvas, x, y);
                    stereotype->setZ((read_file_format() < 5) ? OLD_ARROW_Z
                                     : read_double(st));
                }

                k = read_keyword(st);
            }
            else
                stereotype = 0;

            if (strcmp(k, "to"))
                wrong_keyword(k, "to");

            DiagramItem * di;

            if (strcmp(k = read_keyword(st), "ref")) {
                di = ArrowPointCanvas::read(st, canvas, k);

                if (di == 0)
                    unknown_keyword(k);
            }
            else
                di = dict_get(read_id(st), "classcanvas", canvas);

            // do not give rd to not call update()
            result = new RelationCanvas(canvas, bi, di, b, t, id, dbegin, dend);

            result->geometry = geo;
            result->fixed_geometry = fixed;
            result->set_z(z);
            result->data = rd;

            if (! temporary) {
                connect(rd, SIGNAL(deleted()), result, SLOT(deleted()));
                connect(rd, SIGNAL(changed()), result, SLOT(modified()));
                connect(b->get_data(), SIGNAL(actuals_changed()),
                        result, SLOT(actuals_modified()));
                connect(DrawingSettings::instance(), SIGNAL(changed()),
                        result, SLOT(drawing_settings_modified()));
            }

            result->show_modifier = show_modif;
            result->show_visibility = show_visi;

            if (first == 0)
                first = result;

            if (label != 0)
                (result->label = label)->show();

            if (stereotype != 0)
                (result->stereotype = stereotype)->show();

            result->show();

            if (di->type() != UmlArrowPoint)
                break;

            bi = di;

            read_keyword(st, "line");
            id = read_id(st);
        }

        // roles & multiplicity

        int x, y;

        k = read_keyword(st);

        if (!strcmp(k, "role_a_pos")) {
            x = (int) read_double(st);
            y = (int) read_double(st);
            z = (read_file_format() < 5) ? /*OLD_*/LABEL_Z : read_double(st);

            s = rd->get_role_a();	// note : '/' added by update if needed

            if (!unamed && !s.isEmpty()) {
                result->role_a = new LabelCanvas(s, canvas, x, y, FALSE, FALSE,
                                                 rd->get_isa_class_relation_a());
                result->role_a->setZ(z);
                result->role_a->show();
            }
        }
        else if (strcmp(k, "no_role_a"))
            wrong_keyword(k, "no_role_a");

        k = read_keyword(st);

        if (!strcmp(k, "role_b_pos")) {
            x = (int) read_double(st);
            y = (int) read_double(st);
            z = (read_file_format() < 5) ? /*OLD_*/LABEL_Z : read_double(st);

            s = rd->get_role_b();	// note : '/' added by update if needed

            if (!unamed && !s.isEmpty()) {
                first->role_b = new LabelCanvas(s, canvas, x, y, FALSE, FALSE,
                                                rd->get_isa_class_relation_b());
                first->role_b->setZ(z);
                first->role_b->show();
            }
        }
        else if (strcmp(k, "no_role_b"))
            wrong_keyword(k, "no_role_b");

        k = read_keyword(st);

        if (!strcmp(k, "multiplicity_a_pos")) {
            x = (int) read_double(st);
            y = (int) read_double(st);
            z = (read_file_format() < 5) ? /*OLD_*/LABEL_Z : read_double(st);

            s = rd->get_multiplicity_a();

            if (!unamed && !s.isEmpty()) {
                result->multiplicity_a = new LabelCanvas(s, canvas, x, y);
                result->multiplicity_a->setZ(z);
                result->multiplicity_a->show();
            }
        }
        else if (strcmp(k, "no_multiplicity_a"))
            wrong_keyword(k, "no_multiplicity_a");

        k = read_keyword(st);

        if (!strcmp(k, "multiplicity_b_pos")) {
            x = (int) read_double(st);
            y = (int) read_double(st);
            z = (read_file_format() < 5) ? /*OLD_*/LABEL_Z : read_double(st);

            s = rd->get_multiplicity_b();

            if (!unamed && !s.isEmpty()) {
                first->multiplicity_b = new LabelCanvas(s, canvas, x, y);
                first->multiplicity_b->setZ(z);
                first->multiplicity_b->show();
            }
        }
        else if (strcmp(k, "no_multiplicity_b"))
            wrong_keyword(k, "no_multiplicity_b");

        if (read_file_format() >= 58) {
            // stereotype property

            k = read_keyword(st);

            first->stereotypeproperties =
                StereotypePropertiesCanvas::read(st,  canvas, k, first);

            if (first->stereotypeproperties != 0)
                k = read_keyword(st);

            if (strcmp(k, "end"))
                wrong_keyword(k, "end");
        }

        // to add label, stereotype ... if needed

        if (! temporary) {
            result->hide();
            first->update(FALSE);

            if (first != result)
                result->update(FALSE);

            result->show();

            first->check_stereotypeproperties();
        }

        if (result->data->get_start()->deletedp() || // the relation exist but is deleted
            temporary)
            RelsToDel.append(result);
        else {
            if (read_file_format() == 30)
                // to remove redondant relation made by release 2.22
                RelsToCheck.append(first);

            result->update_geometry();
        }

        return result;
    }
    else
        return 0;
}

void RelationCanvas::history_save(QBuffer & b) const
{
    ArrowCanvas::history_save(b);
    // save labels because they may change
    // when an arrow point is added/removed
    ::save(role_a, b);
    ::save(role_b, b);
    ::save(multiplicity_a, b);
    ::save(multiplicity_b, b);
}

void RelationCanvas::history_load(QBuffer & b)
{
    ArrowCanvas::history_load(b);
    role_a = (LabelCanvas *) load_item(b);
    role_b = (LabelCanvas *) load_item(b);
    multiplicity_a = (LabelCanvas *) load_item(b);
    multiplicity_b = (LabelCanvas *) load_item(b);
    connect(data, SIGNAL(changed()), this, SLOT(modified()));
    connect(data, SIGNAL(deleted()), this, SLOT(deleted()));
    connect(br_begin->get_data(), SIGNAL(actuals_changed()),
            this, SLOT(actuals_modified()));
    connect(DrawingSettings::instance(), SIGNAL(changed()),
            this, SLOT(drawing_settings_modified()));
}

void RelationCanvas::history_hide()
{
    Q3CanvasItem::setVisible(FALSE);
    unconnect();
}
