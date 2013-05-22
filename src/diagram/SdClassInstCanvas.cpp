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
#include <q3popupmenu.h>
#include <qpainter.h>
//Added by qt3to4:
#include <QTextStream>


#include "SdClassInstCanvas.h"
#include "BrowserDiagram.h"
#include "BrowserClass.h"
#include "BrowserClassInstance.h"
#include "SdLifeLineCanvas.h"
#include "ClassInstanceData.h"
#include "UmlCanvas.h"
#include "ClassData.h"
#include "InstanceDialog.h"
#include "SettingsDialog.h"
#include "Settings.h"
#include "myio.h"
#include "strutil.h"
#include "ui/menufactory.h"
#include "ToolCom.h"
#include "translate.h"

SdClassInstCanvas::SdClassInstCanvas(BrowserNode * bn, UmlCanvas * canvas,
                                     int x, int id)
    : SdObjCanvas(0, canvas, x, CLASSINST_CANVAS_MIN_SIZE, 1, id),
      ClassInstCanvas()
{
    // bn may a class instance or a class
    browser_node = bn;

    itscolor = UmlDefaultColor;
    drawing_mode = DefaultClassDrawingMode;

    if (id == 0) {
        // not on read
        compute_size();

        if (browser_node->get_type() != UmlClass)
            check_stereotypeproperties();
    }

    BasicData * d = bn->get_data();

    connect(d, SIGNAL(changed()), this, SLOT(modified()));
    connect(d, SIGNAL(deleted()), this, SLOT(deleted()));
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));
}

SdClassInstCanvas::~SdClassInstCanvas()
{
}

void SdClassInstCanvas::delete_it()
{
    BasicData * d = browser_node->get_data();

    disconnect(d, SIGNAL(changed()), this, SLOT(modified()));
    disconnect(d, SIGNAL(deleted()), this, SLOT(deleted()));
    disconnect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));

    SdObjCanvas::delete_it();
}

void SdClassInstCanvas::deleted()
{
    delete_it();
    canvas()->update();
}

void SdClassInstCanvas::remove(bool from_model)
{
    if (!from_model || (browser_node->get_type() == UmlClass))
        delete_it();
    else
        browser_node->delete_it();	// will remove canvas
}

void SdClassInstCanvas::compute_size()
{
    int w, h;
    SequenceDiagramSettings dflt;

    dflt.instances_drawing_mode = drawing_mode;
    dflt.show_class_context_mode = show_context_mode;
    the_canvas()->browser_diagram()->get_sequencediagramsettings(dflt);

    used_drawing_mode = dflt.instances_drawing_mode;
    used_show_context_mode = dflt.show_class_context_mode;

    if (used_drawing_mode == Natural) {
        const char * st = get_class()->get_data()->get_short_stereotype();

        if (!strcmp(st, "control"))
            used_drawing_mode = asControl;
        else if (!strcmp(st, "entity"))
            used_drawing_mode = asEntity;
        else if (!strcmp(st, "boundary"))
            used_drawing_mode = asBoundary;
        else if (!strcmp(st, "actor"))
            used_drawing_mode = asActor;
        else if (!strcmp(st, "interface"))
            used_drawing_mode = asInterface;
        else
            used_drawing_mode = asClass;
    }

    if (used_drawing_mode == asClass)
        ClassInstCanvas::compute_size(w, h, the_canvas());
    else {
        used_color = (itscolor == UmlDefaultColor)
                     ? the_canvas()->browser_diagram()->get_color(UmlClass)
                     : itscolor;

        int minw;
        QFontMetrics fm(the_canvas()->get_font(UmlNormalUnderlinedFont));
        double zoom = the_canvas()->zoom();
        int delta = (int)(4 * zoom);

        h = fm.height() + delta;

        switch (used_drawing_mode) {
        case asInterface:
            minw = (int)(INTERFACE_SIZE * zoom);
            h += minw;
            break;

        case asControl:
            minw = (int)(CONTROL_WIDTH * zoom);
            h += (int)(CONTROL_HEIGHT * zoom);
            break;

        case asEntity:
            minw = (int)(ENTITY_SIZE * zoom);
            h += minw;
            break;

        case asBoundary:
            minw = (int)(BOUNDARY_WIDTH * zoom);
            h += (int)(BOUNDARY_HEIGHT * zoom);
            break;

        default: { // asActor
            // divide original size by two to not be on life line
            int actor_size = (int)(ACTOR_SIZE / 2.0 * zoom);

            h += actor_size;
            minw = actor_size;
        }
        }

        w = fm.width(full_name());

        if (w < minw)
            w = minw;

        w += delta;
    }

    resize(w, h);
}

void SdClassInstCanvas::modified()
{
    if (visible()) {
        int w = width();

        hide();
        compute_size();
        DiagramCanvas::moveBy((w - width()) / 2, 0);
        show();

        if (browser_node->get_type() != UmlClass)
            check_stereotypeproperties();

        canvas()->update();
        package_modified();
    }
}

void SdClassInstCanvas::draw(QPainter & p)
{
    if (visible()) {
        QRect r = rect();
        p.setRenderHint(QPainter::Antialiasing, true);

        if (used_drawing_mode == asClass)
            ClassInstCanvas::draw(p, the_canvas(), r);
        else {
            FILE * fp = svg();

            if (fp != 0)
                fputs("<g>\n", fp);

            double zoom = the_canvas()->zoom();

            switch (used_drawing_mode) {
            case asInterface:
                DiagramCanvas::draw_interface_icon(p, r, used_color, zoom);
                r.setTop(r.top() + (int)(INTERFACE_SIZE * zoom));
                break;

            case asControl:
                DiagramCanvas::draw_control_icon(p, r, used_color, zoom);
                r.setTop(r.top() + (int)(CONTROL_HEIGHT * zoom));
                break;

            case asEntity:
                DiagramCanvas::draw_entity_icon(p, r, used_color, zoom);
                r.setTop(r.top() + (int)(ENTITY_SIZE * zoom));
                break;

            case asBoundary:
                DiagramCanvas::draw_boundary_icon(p, r, used_color, zoom);
                r.setTop(r.top() + (int)(BOUNDARY_HEIGHT * zoom));
                break;

            default: { // asActor
                // divide original size by two to not be on life line
                double actor_size = ACTOR_SIZE / 2.0 * zoom;

                r.setHeight((int) actor_size);
                r.setLeft(r.left() +
                          (int)((r.width() - actor_size) / 2));
                r.setWidth(r.height());
                draw_actor(&p, r);
                r = rect();
                r.setTop(r.top() + (int) actor_size);
            }
            }

            QColor bckgrnd = p.backgroundColor();

            p.setBackgroundMode(::Qt::TransparentMode);
            p.setFont(the_canvas()->get_font(UmlNormalFont));
            p.drawText(r, ::Qt::AlignCenter, full_name());

            if (fp != 0)
                draw_text(r, ::Qt::AlignCenter, full_name(),
                          p.font(), fp);

            p.setBackgroundColor(bckgrnd);

            if (fp != 0)
                fputs("</g>\n", fp);
        }

        if (selected())
            show_mark(p, rect());
    }
}

// all cases
QString SdClassInstCanvas::get_name() const
{
    return (browser_node->get_type() == UmlClass)
           ? iname
           : QString(browser_node->get_name());
}

// out of model case
void SdClassInstCanvas::set_name(const QString & s)
{
    iname = s;
}

// UmlClass or UmlClassInstance
UmlCode SdClassInstCanvas::type() const
{
    return browser_node->get_type();
}

// return class, all cases
BrowserNode * SdClassInstCanvas::get_type() const
{
    return (browser_node->get_type() == UmlClass)
           ? browser_node
           : ((BrowserNode *)
              ((ClassInstanceData *) browser_node->get_data())->get_class());
}

// ut of model case
void SdClassInstCanvas::set_type(BrowserNode * t)
{
    if (t != browser_node) {
        disconnect(browser_node->get_data(), 0, this, 0);
        browser_node = t;
        connect(t->get_data(), SIGNAL(changed()), this, SLOT(modified()));
        connect(t->get_data(), SIGNAL(deleted()), this, SLOT(deleted()));
    }
}

BrowserNodeList & SdClassInstCanvas::get_types(BrowserNodeList & r) const
{
    return BrowserClass::instances(r);
}

BrowserNode * SdClassInstCanvas::container(UmlCode c) const
{
    return the_canvas()->browser_diagram()->container(c);
}

BrowserClass * SdClassInstCanvas::get_class() const
{
    return (BrowserClass *) get_type();
}

void SdClassInstCanvas::delete_available(BooL & in_model, BooL & out_model) const
{
    out_model |= TRUE;

    if (browser_node->get_type() == UmlClassInstance)
        in_model |= browser_node->is_writable();
}

void SdClassInstCanvas::open()
{
    if (browser_node->get_type() == UmlClass) {
        InstanceDialog d(this, "class", UmlClass);

        d.raise();

        if (d.exec() == QDialog::Accepted)
            modified();
    }
    else
        browser_node->open(FALSE);
}

void SdClassInstCanvas::menu(const QPoint &)
{
    Q3PopupMenu m(0);
    bool modelized = (browser_node->get_type() == UmlClassInstance);

    MenuFactory::createTitle(m, full_name());
    m.insertSeparator();
    m.insertItem(TR("Upper"), 0);
    m.insertItem(TR("Lower"), 1);
    m.insertItem(TR("Go up"), 13);
    m.insertItem(TR("Go down"), 14);
    m.insertSeparator();
    m.insertItem(TR("Edit"), 2);

    if (is_mortal())
        m.insertItem(TR("Become immortal"), 3);
    else
        m.insertItem(TR("Become mortal"), 4);

    if (life_line) {
        if (life_line->is_masked())
            m.insertItem(TR("Show life line"), 15);
        else if (life_line->can_be_masked())
            m.insertItem(TR("Hide life line"), 15);
    }

    m.insertSeparator();
    m.insertItem(TR("Edit drawing settings"), 5);
    m.insertSeparator();

    if (browser_node->get_type() == UmlClassInstance)
        m.insertItem(TR("Select in browser"), 6);

    m.insertItem(TR("Select class in browser"), 7);
    m.insertSeparator();

    if (modelized)
        m.insertItem(TR("Exit from model"), 10);
    else {
        if (container(UmlClass)->is_writable())
            m.insertItem(TR("Insert in model"), 11);

        m.insertItem(TR("Replace it"), 12);
    }

    m.insertSeparator();
    m.insertItem(TR("Remove from diagram"), 8);

    if ((browser_node->get_type() == UmlClassInstance) &&
        browser_node->is_writable())
        m.insertItem(TR("Delete from model"), 9);

    switch (m.exec(QCursor::pos())) {
    case 0:
        upper();
        modified();	// call package_modified
        return;

    case 1:
        lower();
        modified();	// call package_modified
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
        open();	// call modified()
        return;

    case 3:
        set_mortal(FALSE);
        break;

    case 4:
        set_mortal(TRUE);
        break;

    case 5:
        edit_drawing_settings();
        return;

    case 6:
        browser_node->select_in_browser();
        return;

    case 7:
        get_type()->select_in_browser();
        return;

    case 8:
        delete_it();
        break;

    case 9:
        //delete from model
        browser_node->delete_it();	// will delete the canvas
        break;

    case 10: {
        BasicData * d = browser_node->get_data();

        disconnect(d, SIGNAL(changed()), this, SLOT(modified()));
        disconnect(d, SIGNAL(deleted()), this, SLOT(deleted()));

        iname = browser_node->get_name();
        browser_node = ((ClassInstanceData *) d)->get_class();
        d = browser_node->get_data();
        connect(d, SIGNAL(changed()), this, SLOT(modified()));
        connect(d, SIGNAL(deleted()), this, SLOT(deleted()));
    }
    break;

    case 11: {
        BasicData * d = browser_node->get_data();

        disconnect(d, SIGNAL(changed()), this, SLOT(modified()));
        disconnect(d, SIGNAL(deleted()), this, SLOT(deleted()));

        browser_node =
            new BrowserClassInstance(iname, (BrowserClass *) browser_node,
                                     container(UmlClass));
        d = browser_node->get_data();
        connect(d, SIGNAL(changed()), this, SLOT(modified()));
        connect(d, SIGNAL(deleted()), this, SLOT(deleted()));
    }
    break;

    case 12: {
        BrowserNode * bn =
            BrowserClassInstance::get_classinstance((BrowserClass *) browser_node);

        if (bn == 0)
            return;

        BasicData * d = browser_node->get_data();

        disconnect(d, SIGNAL(changed()), this, SLOT(modified()));
        disconnect(d, SIGNAL(deleted()), this, SLOT(deleted()));

        browser_node = bn;
        d = browser_node->get_data();
        connect(d, SIGNAL(changed()), this, SLOT(modified()));
        connect(d, SIGNAL(deleted()), this, SLOT(deleted()));
        modified();	// call package_modified
        return;
    }

    case 15:
        life_line->set_masked(!life_line->is_masked());
        modified();	// call package_modified()
        return;

    default:
        return;
    }

    package_modified();
}

void SdClassInstCanvas::apply_shortcut(QString s)
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

void SdClassInstCanvas::edit_drawing_settings()
{
    for (;;) {
        StateSpecVector st((browser_node->get_type() != UmlClass) ? 4 : 3);
        ColorSpecVector co(1);

        st[0].set(TR("drawing mode"), &drawing_mode);
        st[1].set(TR("write name:type \nhorizontally"), &write_horizontally);
        st[2].set(TR("show class context"), &show_context_mode);

        if (browser_node->get_type() != UmlClass)
            st[3].set(TR("show stereotypes \nproperties"), &show_stereotype_properties);

        co[0].set(TR("class instance color"), &itscolor);

        SettingsDialog dialog(&st, &co, FALSE);

        dialog.raise();

        if (dialog.exec() == QDialog::Accepted)
            modified();

        if (!dialog.redo())
            break;
    }
}

bool SdClassInstCanvas::has_drawing_settings() const
{
    return TRUE;
}

void SdClassInstCanvas::edit_drawing_settings(QList<DiagramItem *> & l)
{
    for (;;) {
        StateSpecVector st(3);
        ColorSpecVector co(1);
        ClassDrawingMode drawing_mode;
        Uml3States write_horizontally;
        ShowContextMode show_context_mode;
        UmlColor itscolor;

        st[0].set(TR("drawing mode"), &drawing_mode);
        st[1].set(TR("write name:type \nhorizontally"), &write_horizontally);
        st[2].set(TR("show class context"), &show_context_mode);
        co[0].set(TR("class instance color"), &itscolor);

        SettingsDialog dialog(&st, &co, FALSE, TRUE);

        dialog.raise();

        if (dialog.exec() == QDialog::Accepted) {
            foreach (DiagramItem *item, l) {
                SdClassInstCanvas *canvas = (SdClassInstCanvas *)item;
                if (!st[0].name.isEmpty())
                    canvas->drawing_mode =
                        drawing_mode;

                if (!st[1].name.isEmpty())
                    canvas->write_horizontally =
                        write_horizontally;

                if (!st[2].name.isEmpty())
                    canvas->show_context_mode =
                        show_context_mode;

                if (!co[0].name.isEmpty())
                    canvas->itscolor = itscolor;

                canvas->modified();	// call package_modified()
            }
        }

        if (!dialog.redo())
            break;
    }
}

void SdClassInstCanvas::clone_drawing_settings(const DiagramItem *src)
{
    const SdClassInstCanvas * x = (const SdClassInstCanvas *) src;
    drawing_mode = x->drawing_mode;
    show_context_mode = x->show_context_mode;
    write_horizontally = x->write_horizontally;
    itscolor = x->itscolor;
    modified();
}

bool SdClassInstCanvas::get_show_stereotype_properties() const
{
    return (browser_node->get_type() != UmlClass) && show_properties;
}

void SdClassInstCanvas::save(QTextStream & st, bool ref, QString & warning) const
{
    if (ref)
        st << "classinstance_ref " << get_ident() << " // "
           << full_name();
    else if (browser_node->get_type() == UmlClass) {
        nl_indent(st);
        st << "classinstance " << get_ident() << " ";
        browser_node->save(st, TRUE, warning);
        nl_indent(st);
        ClassInstCanvas::save(st);

        if (drawing_mode != DefaultClassDrawingMode)
            st << "  drawing_mode " << stringify(drawing_mode);

        st << "  name ";
        save_string(iname, st);
        st << " ";
        SdObjCanvas::save(st);
    }
    else {
        nl_indent(st);
        st << "classinstancecanvas " << get_ident() << " ";
        browser_node->save(st, TRUE, warning);
        nl_indent(st);
        ClassInstCanvas::save(st);

        if (drawing_mode != DefaultClassDrawingMode)
            st << "  drawing_mode " << stringify(drawing_mode);

        SdObjCanvas::save(st);
        indent(+1);
        save_stereotype_property(st, warning);
        indent(-1);
        nl_indent(st);
        st << "end";
    }
}

SdClassInstCanvas * SdClassInstCanvas::read(char *& st, UmlCanvas * canvas, char * k)
{
    if (!strcmp(k, "classinstance_ref"))
        return ((SdClassInstCanvas *) dict_get(read_id(st), "classinstance", canvas));
    else if (!strcmp(k, "classinstance")) {
        // old release and graphic instance
        int id = read_id(st);
        BrowserClass * cl = BrowserClass::read_ref(st);
        SdClassInstCanvas * result =
            new SdClassInstCanvas(cl, canvas, 0, id);

        result->ClassInstCanvas::read(st, k);

        if (!strcmp(k, "drawing_mode")) {
            result->drawing_mode = ::drawing_mode(read_keyword(st));
            k = read_keyword(st);
        }
        else
            result->drawing_mode = DefaultClassDrawingMode;

        if (strcmp(k, "name"))
            wrong_keyword(k, "name");

        result->iname = read_string(st);
        result->compute_size();

        if ((read_file_format() < 72) &&
            (result->used_drawing_mode == asInterface)) {
            result->drawing_mode = asClass;
            result->compute_size();
        }

        result->SdObjCanvas::read(st, read_keyword(st));
        result->show();

        return result;
    }
    else if (!strcmp(k, "classinstancecanvas")) {
        int id = read_id(st);
        BrowserClassInstance * icl = BrowserClassInstance::read_ref(st);
        SdClassInstCanvas * result =
            new SdClassInstCanvas(icl, canvas, 0, id);

        result->ClassInstCanvas::read(st, k);

        if (!strcmp(k, "drawing_mode")) {
            result->drawing_mode = ::drawing_mode(read_keyword(st));
            k = read_keyword(st);
        }

        result->SdObjCanvas::read(st, k);

        if (read_file_format() >= 58) {
            k = read_keyword(st);
            result->read_stereotype_property(st, k);	// updates k

            if (strcmp(k, "end"))
                wrong_keyword(k, "end");
        }

        if (result->get_type() != 0) {
            // not a deleted instance
            result->compute_size();

            if ((read_file_format() < 72) &&
                (result->used_drawing_mode == asInterface)) {
                result->drawing_mode = asClass;
                result->compute_size();
            }

            result->set_center100();

            if (result->life_line)
                result->life_line->update_pos();

            result->show();
            result->check_stereotypeproperties();
        }

        return result;
    }
    else
        return 0;
}

void SdClassInstCanvas::history_hide()
{
    Q3CanvasItem::setVisible(FALSE);
    disconnect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));

    BasicData * d = browser_node->get_data();

    disconnect(d, SIGNAL(changed()), this, SLOT(modified()));
    disconnect(d, SIGNAL(deleted()), this, SLOT(deleted()));
}

void SdClassInstCanvas::history_load(QBuffer & b)
{
    SdObjCanvas::history_load(b);
    connect(DrawingSettings::instance(), SIGNAL(changed()), this, SLOT(modified()));

    BasicData * d = browser_node->get_data();

    disconnect(d, SIGNAL(changed()), this, SLOT(modified()));
    disconnect(d, SIGNAL(deleted()), this, SLOT(deleted()));
}

// for plug outs

void SdClassInstCanvas::send(ToolCom * com, Q3CanvasItemList & all)
{
    QList<SdClassInstCanvas *> l;
    Q3CanvasItemList::Iterator cit;

    for (cit = all.begin(); cit != all.end(); ++cit) {
        DiagramItem * di = QCanvasItemToDiagramItem(*cit);

        if ((di != 0) && (*cit)->visible()) {
            switch (di->type()) {
            case UmlClass:
            case UmlClassInstance:
                l.append((SdClassInstCanvas *) di);
                break;

            default:
                break;
            }
        }
    }

    com->write_unsigned(l.count());

    foreach (SdClassInstCanvas *i, l) {
        com->write_unsigned((unsigned) i->get_ident());

        if (i->browser_node->get_type() == UmlClass) {
            com->write_id(0);

            WrapperStr s = fromUnicode(i->iname);

            com->write_string((const char *) s);
        }

        i->browser_node->write_id(com);
    }
}
