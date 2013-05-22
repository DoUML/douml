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

#include "TransitionCanvas.h"
#include "ArrowPointCanvas.h"
#include "UmlCanvas.h"
#include "LabelCanvas.h"
#include "BrowserTransition.h"
#include "BrowserStateDiagram.h"
#include "TransitionData.h"
#include "StereotypePropertiesCanvas.h"
#include "UmlGlobal.h"
#include "myio.h"
#include "ToolCom.h"
#include "Tool.h"
#include "ui/menufactory.h"
#include "Settings.h"
#include "SettingsDialog.h"
#include "DialogUtil.h"
#include "strutil.h"

TransitionCanvas::TransitionCanvas(UmlCanvas * canvas,
                                   DiagramItem * b, DiagramItem * e,
                                   BrowserNode * bb, int id,
                                   float d_begin, float d_end,
                                   TransitionData * d)
    : ArrowCanvas(canvas, b, e, UmlTransition, id, TRUE, d_begin, d_end),
      br_begin(bb), data(d), stereotypeproperties(0)
{
    if ((e->type() != UmlArrowPoint) && (bb == 0)) {
        // end of line construction
        update_begin(e);
    }
    else if (d != 0) {
        if (e->type() != UmlArrowPoint)
            check_stereotypeproperties();

        connect(d, SIGNAL(changed()), this, SLOT(modified()));
        connect(d, SIGNAL(deleted()), this, SLOT(deleted()));
    }

    drawing_language = DefaultDrawingLanguage;
    write_horizontally = UmlDefaultState;
    show_definition = UmlDefaultState;

    connect(DrawingSettings::instance(), SIGNAL(changed()),
            this, SLOT(modified()));

    // manages the case start == end
    if (b == e)
        cut_self();

    if (d != 0)
        update(TRUE);
}

TransitionCanvas::~TransitionCanvas()
{
}

void TransitionCanvas::delete_it()
{
    if (stereotypeproperties != 0)
        ((UmlCanvas *) canvas())->del(stereotypeproperties);

    ArrowCanvas::delete_it(); // call unconnect
}

void TransitionCanvas::unconnect()
{
    disconnect(data, 0, this, 0);
    disconnect(DrawingSettings::instance(), SIGNAL(changed()),
               this, SLOT(modified()));

    ArrowCanvas::unconnect();
}

void TransitionCanvas::deleted()
{
    delete_it();
    canvas()->update();
    package_modified();
}

void TransitionCanvas::delete_available(BooL & in_model, BooL & out_model) const
{
    out_model |= TRUE;
    in_model |= data->get_start()->is_writable();
}

void TransitionCanvas::remove(bool from_model)
{
    if (! from_model) {
        if (the_canvas()->must_draw_all_relations()) {
            const TransitionCanvas * a = this;

            while (a->begin->type() == UmlArrowPoint) {
                a = (TransitionCanvas *)((ArrowPointCanvas *) a->begin)->get_other(a);

                if (a == 0)
                    break;
            }

            if (a && !a->begin->isSelected() && !a->begin->get_bn()->deletedp()) {
                a = this;

                while (a->end->type() == UmlArrowPoint) {
                    a = (TransitionCanvas *)((ArrowPointCanvas *) a->end)->get_other(a);

                    if (a == 0)
                        break;
                }

                if (a && !a->end->isSelected() && !a->end->get_bn()->deletedp()) {
                    msg_warning("Douml", "<i>Draw all relations</i> forced to <i>no</i>");
                    the_canvas()->dont_draw_all_relations();
                }
            }
        }

        delete_it();
    }
    else
        data->delete_it();	// will remove canvas
}


// Updates br_begin going back, Returns the start browser node

BrowserNode * TransitionCanvas::update_begin(DiagramItem * cnend)
{
    // static to be updated in all the cases
    static TransitionData * d = 0;

    if (begin->type() == UmlArrowPoint) {
        TransitionCanvas * other =
            ((TransitionCanvas *)((ArrowPointCanvas *) begin)->get_other(this));

        br_begin = other->update_begin(cnend);	// update d
    }
    else {
        // the relation start here
        br_begin = begin->get_bn();
        d = (TransitionData *) begin->add_relation(UmlTransition, cnend);
    }

    connect(d, SIGNAL(changed()), this, SLOT(modified()));
    connect(d, SIGNAL(deleted()), this, SLOT(deleted()));
    data = d;

    return br_begin;
}

void TransitionCanvas::propagate_drawing_settings()
{
    TransitionCanvas * t;

    t = this;

    while (t->begin->type() == UmlArrowPoint) {
        t = ((TransitionCanvas *)((ArrowPointCanvas *) t->begin)->get_other(t));
        t->drawing_language = drawing_language;
        t->write_horizontally = write_horizontally;
        t->show_definition = show_definition;
    }

    t = this;

    while (t->end->type() == UmlArrowPoint) {
        t = ((TransitionCanvas *)((ArrowPointCanvas *) t->end)->get_other(t));
        t->drawing_language = drawing_language;
        t->write_horizontally = write_horizontally;
        t->show_definition = show_definition;
    }
}

void TransitionCanvas::open()
{
    if (data &&
        !data->get_start()->in_edition())	// for dble click on diagram
        data->edit();
}

void TransitionCanvas::menu(const QPoint &)
{
    if ((data != 0) && !data->get_start()->in_edition()) {
        TransitionCanvas * plabel;
        TransitionCanvas * pstereotype;

        {
            ArrowCanvas * aplabel;
            ArrowCanvas * apstereotype;

            search_supports(aplabel, apstereotype);
            plabel = (TransitionCanvas *) aplabel;
            pstereotype = (TransitionCanvas *) apstereotype;
        }

        Q3PopupMenu m(0);
        Q3PopupMenu geo(0);
        Q3PopupMenu toolm(0);

        MenuFactory::createTitle(m, data->definition(FALSE, TRUE));
        m.insertSeparator();
        m.insertItem("Edit", 0);
        m.insertSeparator();

        m.insertItem("Select in browser", 2);

        if (plabel || pstereotype) {
            m.insertSeparator();
            m.insertItem("Edit drawing settings", 1);
            m.insertItem("Select labels", 3);
            m.insertItem("Labels default position", 4);

            if (plabel && (label == 0))
                m.insertItem("Attach transition's name to this segment", 5);

            if (pstereotype && (stereotype == 0))
                m.insertItem("Attach stereotype to this segment", 6);
        }

        if (get_start() != get_end()) {
            m.insertSeparator();
            init_geometry_menu(geo, 10);
            m.insertItem("Geometry (Ctrl+l)", &geo);
        }

        m.insertSeparator();
        m.insertItem("Remove from diagram", 7);

        if (data->get_start()->is_writable())
            m.insertItem("Delete from model", 8);

        m.insertSeparator();

        if (Tool::menu_insert(&toolm, itstype, 20))
            m.insertItem("Tool", &toolm);

        int rank = m.exec(QCursor::pos());

        switch (rank) {
        case 0:
            data->edit();
            return;

        case 1:
            edit_drawing_settings();
            return;

        case 2:
            data->get_start()->select_in_browser();
            return;

        case 3:
            the_canvas()->unselect_all();

            if (plabel)
                the_canvas()->select(plabel->label);

            if (pstereotype)
                the_canvas()->select(pstereotype->stereotype);

            return;

        case 4:
            if (plabel)
                plabel->default_label_position();

            if (pstereotype)
                pstereotype->stereotype_default_position();

            return;

        case 5:
            label = plabel->label;
            plabel->label = 0;
            default_label_position();
            return;

        case 6:
            stereotype = pstereotype->stereotype;
            pstereotype->stereotype = 0;

            if (label != 0)
                default_label_position();

            stereotype_default_position();
            return;

        case 7:
            // not removed from the browser : just hide it
            remove(FALSE);
            break;

        case 8:
            data->delete_it();	// will delete the canvas
            break;

        default:
            if (rank >= 20) {
                ToolCom::run(Tool::command(rank - 20), data->get_start());
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

void TransitionCanvas::apply_shortcut(QString s)
{
    if (s == "Select in browser") {
        data->get_start()->select_in_browser();
        return;
    }
    else if (s == "Edit drawing settings") {
        edit_drawing_settings();
        return;
    }
    else {
        data->get_start()->apply_shortcut(s);
        return;
    }

    modified(); // call package_modified()
}

void TransitionCanvas::edit_drawing_settings()
{
    for (;;) {
        StateSpecVector st(3);

        st[0].set("language", &drawing_language);
        st[1].set("write horizontally", &write_horizontally);
        st[2].set("show definition", &show_definition);

        SettingsDialog dialog(&st, 0, FALSE);

        dialog.setCaption("Transition Drawing Settings dialog");
        dialog.raise();

        if (dialog.exec() == QDialog::Accepted) {
            propagate_drawing_settings();
            modified();
        }

        if (!dialog.redo())
            break;
    }
}

bool TransitionCanvas::has_drawing_settings() const
{
    return TRUE;
}

void TransitionCanvas::edit_drawing_settings(QList<DiagramItem *> & l)
{
    for (;;) {
        StateSpecVector st(3);
        DrawingLanguage drawing_language;
        Uml3States write_horizontally;
        Uml3States show_definition;

        st[0].set("language", &drawing_language);
        st[1].set("write horizontally", &write_horizontally);
        st[2].set("show definition", &show_definition);

        SettingsDialog dialog(&st, 0, FALSE, TRUE);

        dialog.setCaption("Transition Drawing Settings dialog");
        dialog.raise();

        if (dialog.exec() == QDialog::Accepted) {
            foreach (DiagramItem *item, l) {
                TransitionCanvas *canvas = (TransitionCanvas *)item;
                if (!st[0].name.isEmpty())
                    canvas->drawing_language =
                        drawing_language;

                if (!st[1].name.isEmpty())
                    canvas->write_horizontally =
                        write_horizontally;

                if (!st[2].name.isEmpty())
                    canvas->show_definition =
                        show_definition;

                canvas->propagate_drawing_settings();
                canvas->modified();	// call package_modified()
            }
        }

        if (!dialog.redo())
            break;
    }
}

void TransitionCanvas::clone_drawing_settings(const DiagramItem *src)
{
    const TransitionCanvas * x = (const TransitionCanvas *) src;
    drawing_language = x->drawing_language;
    write_horizontally = x->write_horizontally;
    show_definition = x->show_definition;
    propagate_drawing_settings();
    modified();
}

ArrowPointCanvas * TransitionCanvas::brk(const QPoint & p)
{
    ArrowPointCanvas * ap =
        new ArrowPointCanvas(the_canvas(), p.x(), p.y());

    ap->setZ(z() + 1);

    TransitionCanvas * other =
        // do not give data to not call update()
        new TransitionCanvas(the_canvas(), ap, end, br_begin, 0,
                             decenter_begin, decenter_end);

    other->data = data;
    other->drawing_language = drawing_language;
    other->write_horizontally = write_horizontally;
    other->show_definition = show_definition;
    connect(data, SIGNAL(changed()), other, SLOT(modified()));
    connect(data, SIGNAL(deleted()), other, SLOT(deleted()));

    ap->add_line(this);
    end->remove_line(this, TRUE);
    end = ap;

    if ((p - beginp).manhattanLength() < (p - endp).manhattanLength()) {
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


ArrowCanvas * TransitionCanvas::join(ArrowCanvas * other, ArrowPointCanvas * ap)
{
    // has already check is join is possible (self relation must have two points)
    ArrowCanvas * result = ArrowCanvas::join(other, ap);

    ((TransitionCanvas *) result)->modified();

    return result;
}

void TransitionCanvas::modified()
{
    if (visible()) {
        hide();
        update(TRUE);
        show();

        if (begin->type() != UmlArrowPoint)
            check_stereotypeproperties();

        canvas()->update();
        package_modified();
    }
}

void TransitionCanvas::update(bool updatepos)
{
    if (data) {
        QFontMetrics fm(the_canvas()->get_font(UmlNormalFont));
        TransitionCanvas * plabel;
        TransitionCanvas * pstereotype;

        {
            ArrowCanvas * aplabel;
            ArrowCanvas * apstereotype;

            search_supports(aplabel, apstereotype);
            plabel = (TransitionCanvas *) aplabel;
            pstereotype = (TransitionCanvas *) apstereotype;
        }

        // manages relation's stereotype

        QString s = data->get_short_stereotype();
        bool update_label = FALSE;

        if (s.isEmpty()) {
            // relation does not have stereotype
            if (pstereotype != 0) {
                // removes it
                the_canvas()->del(pstereotype->stereotype);
                pstereotype->stereotype = 0;
            }
        }
        else {
            s = QString("<<") + toUnicode(s) + ">>";

            if ((pstereotype == 0) && (begin->type() != UmlArrowPoint)) {
                // adds relation's stereotype
                stereotype = new LabelCanvas(s, the_canvas(), 0, 0);
                update_label = (label != 0);
                stereotype_default_position();
            }
            else if ((pstereotype != 0) &&
                     (pstereotype->stereotype->get_name() != s)) {
                // update name
                pstereotype->stereotype->set_name(s);

                if (auto_pos)
                    pstereotype->stereotype_default_position();
            }
        }

        // manages relation's name

        BrowserStateDiagram * diagram =
            (BrowserStateDiagram *) the_canvas()->browser_diagram();

        if ((show_definition == UmlDefaultState)
            ? diagram->get_show_trans_definition()
            : (show_definition == UmlYes)) {
            s = data->str((write_horizontally == UmlDefaultState)
                          ? diagram->get_write_label_horizontally()
                          : (write_horizontally == UmlYes),
                          (drawing_language == DefaultDrawingLanguage)
                          ? diagram->get_language()
                          : drawing_language);
            s = toUnicode(s);
        }
        else if ((s = data->get_start()->get_name()) == "<transition>")
            s = QString();

        if (s.isEmpty()) {
            // relation does not have name
            if (plabel != 0) {
                // removes it
                the_canvas()->del(plabel->label);
                plabel->label = 0;
            }
        }
        else if ((plabel == 0) && (begin->type() != UmlArrowPoint)) {
            // adds relation's name
            label = new LabelCanvas(s, the_canvas(), 0, 0);
            default_label_position();
        }
        else if ((plabel != 0) && (plabel->label->get_name() != s)) {
            // update name
            plabel->label->set_name(s);

            if (auto_pos || update_label)
                plabel->default_label_position();
        }
        else if (update_label)
            plabel->default_label_position();
    }

    if (updatepos)
        update_pos();
}

void TransitionCanvas::default_label_position() const
{
    QPoint c = center();
    QFontMetrics fm(the_canvas()->get_font(UmlNormalFont));
    QString s = label->get_name();
    QSize sz = fm.size(0, s);

    label->move(c.x() - sz.width() / 2,
                c.y() - ((stereotype != 0) ? sz.height() : sz.height() / 2));
    package_modified();
}

void TransitionCanvas::stereotype_default_position() const
{
    default_stereotype_position();
    package_modified();
}

BasicData * TransitionCanvas::get_data() const
{
    return data;
}

void TransitionCanvas::drop(BrowserNode * bn, UmlCanvas * canvas)
{
    TransitionData * def = (TransitionData *) bn->get_data();
    BrowserNode * from = def->get_start_node();
    BrowserNode * to = def->get_end_node();
    DiagramItem * difrom = 0;
    DiagramItem * dito = 0;
    Q3CanvasItemList all = canvas->allItems();
    Q3CanvasItemList::Iterator cit;

    // the two extremities are drawn ?
    for (cit = all.begin(); cit != all.end(); ++cit) {
        if ((*cit)->visible()) {
            DiagramItem * di = QCanvasItemToDiagramItem(*cit);

            if (di != 0) {
                if (di->get_bn() == from) {
                    difrom = di;

                    if (dito != 0)
                        break;
                }

                if (di->get_bn() == to) {
                    dito = di;

                    if (difrom != 0)
                        break;
                }
            }
        }
    }

    if ((difrom != 0) && (dito != 0)) {
        if (difrom->has_relation(UmlTransition, def))
            msg_information("Douml", "transition already drawn");
        else {
            TransitionCanvas * tr =
                new TransitionCanvas(canvas, difrom, dito, from, 0, -1.0, -1.0, def);

            tr->show();
            tr->package_modified();
        }
    }
}

//

void TransitionCanvas::setVisible(bool yes)
{
    ArrowCanvas::setVisible(yes);

    if (stereotypeproperties)
        stereotypeproperties->setVisible(yes);
}

void TransitionCanvas::moveBy(double dx, double dy)
{
    ArrowCanvas::moveBy(dx, dy);

    if ((stereotypeproperties != 0) && !stereotypeproperties->selected())
        stereotypeproperties->moveBy(dx, dy);
}

void TransitionCanvas::select_associated()
{
    if (!selected()) {
        if ((stereotypeproperties != 0) && !stereotypeproperties->selected())
            the_canvas()->select(stereotypeproperties);

        ArrowCanvas::select_associated();
    }
}

void TransitionCanvas::check_stereotypeproperties()
{
    // the note is memorized by the first segment
    if (begin->type() == UmlArrowPoint)
        ((TransitionCanvas *)((ArrowPointCanvas *) begin)->get_other(this))
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

bool TransitionCanvas::represents(BrowserNode * bn)
{
    return (data == bn->get_data());
}

//
void TransitionCanvas::save(QTextStream & st, bool ref, QString & warning) const
{
    if (ref)
        st << "transitioncanvas_ref " << get_ident();
    else if (begin->type() != UmlArrowPoint) {
        // relation canvas start
        nl_indent(st);
        st << "transitioncanvas " << get_ident() << " ";
        data->get_start()->save(st, TRUE, warning);	// data doesn't have id
        indent(+1);

        if (geometry != NoGeometry) {
            nl_indent(st);
            st << "geometry " << stringify(geometry);

            if (!fixed_geometry)
                st << " unfixed";
        }

        if (decenter_begin >= 0) {
            nl_indent(st);
            // float output/input bugged
            st << "decenter_begin " << ((int)(decenter_begin * 1000));
        }

        if (decenter_end >= 0) {
            // float output/input bugged
            nl_indent(st);
            st << "decenter_end " << ((int)(decenter_end * 1000));
        }

        nl_indent(st);
        (const TransitionCanvas *) ArrowCanvas::save_lines(st, TRUE, TRUE, warning);
        nl_indent(st);
        st << "write_horizontally " << stringify(write_horizontally)
           << " show_definition " << stringify(show_definition)
           << " drawing_language " << stringify(drawing_language);

        if (stereotypeproperties != 0)
            stereotypeproperties->save(st, FALSE, warning);

        indent(-1);
        nl_indent(st);
        st << "end";
    }
}

TransitionCanvas * TransitionCanvas::read(char *& st, UmlCanvas * canvas, char * k)
{
    if (!strcmp(k, "transitioncanvas_ref"))
        return ((TransitionCanvas *) dict_get(read_id(st), "transitioncanvas", canvas));
    else if (!strcmp(k, "transitioncanvas")) {
        int id = read_id(st);
        BrowserTransition * br = BrowserTransition::read_ref(st);
        TransitionData * tr = (TransitionData *) br->get_data();
        LineGeometry geo;
        bool fixed = FALSE;

        k = read_keyword(st);

        if (! strcmp(k, "geometry")) {
            geo = line_geometry(read_keyword(st));
            k = read_keyword(st);

            if (! strcmp(k, "unfixed"))
                k = read_keyword(st);
            else
                fixed = TRUE;
        }
        else
            geo = NoGeometry;

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

        DiagramItem * bi = dict_get(read_id(st), "canvas", canvas);
        BrowserNode * b = tr->get_start_node();
        QString s = br->get_name();
        TransitionCanvas * first = 0;
        TransitionCanvas * result;
        LabelCanvas * label;
        LabelCanvas * stereotype;
        double z;

        for (;;) {
            read_keyword(st, "z");
            z = read_double(st);
            k = read_keyword(st);

            if ((label = LabelCanvas::read(st, canvas, k)) != 0) {
                // the transition name can't be empty
                label->setZ(z);
                k = read_keyword(st);
            }

            if (!strcmp(k, "stereotype")) {
                (void) read_string(st);
                read_keyword(st, "xyz");

                int x = (int) read_double(st);
                int y = (int) read_double(st);

                if ((tr == 0) || (*(tr->get_stereotype()) == 0)) {
                    stereotype = 0;
                    read_double(st);	// z
                }
                else {
                    stereotype =
                        new LabelCanvas(QString("<<") + toUnicode(tr->get_short_stereotype()) + ">>",
                                        canvas, x, y);
                    stereotype->setZ(read_double(st));
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
                di = dict_get(read_id(st), "statecanvas/pseudostate", canvas);

            // do not give tr to not call update()
            result = new TransitionCanvas(canvas, bi, di, b, id, dbegin, dend);
            result->geometry = geo;
            result->fixed_geometry = fixed;
            result->set_z(z);
            result->data = tr;
            connect(tr, SIGNAL(changed()), result, SLOT(modified()));
            connect(tr, SIGNAL(deleted()), result, SLOT(deleted()));

            if (first == 0) {
                first = result;

                if (read_file_format() == 30)
                    // to remove redondant transitions made by release 2.22
                    RelsToCheck.append(result);
            }

            if (label != 0)
                result->label = label;

            if (stereotype != 0)
                (result->stereotype = stereotype)->show();

            result->show();

            if (di->type() != UmlArrowPoint)
                break;

            bi = di;

            read_keyword(st, "line");
            id = read_id(st);
        }

        read_keyword(st, "write_horizontally");
        result->write_horizontally = state(read_keyword(st));

        k = read_keyword(st);

        if (!strcmp(k, "show_definition")) {
            result->show_definition = state(read_keyword(st));
            k = read_keyword(st);
        }

        if (strcmp(k, "drawing_language"))
            wrong_keyword(k, "drawing_language");

        result->drawing_language = ::drawing_language(read_keyword(st));

        result->propagate_drawing_settings();

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
        first->update(FALSE);

        if (first != result)
            result->update(FALSE);

        first->check_stereotypeproperties();

        // manage case where the relation is deleted but present in the browser
        if (result->data->get_start()->deletedp())
            RelsToDel.append(result);
        else
            result->update_geometry();

        return result;
    }
    else
        return 0;
}

void TransitionCanvas::history_hide()
{
    Q3CanvasItem::setVisible(FALSE);
    unconnect();
}

void TransitionCanvas::history_load(QBuffer & b)
{
    ArrowCanvas::history_load(b);
    connect(data, SIGNAL(changed()), this, SLOT(modified()));
    connect(data, SIGNAL(deleted()), this, SLOT(deleted()));
    connect(DrawingSettings::instance(), SIGNAL(changed()),
            this, SLOT(modified()));
}
