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
#include <qfileinfo.h>
//Added by qt3to4:
#include <QTextStream>
#include <Q3ValueList>
#include <QPixmap>

#include "BrowserObjectDiagram.h"
#include "SimpleData.h"
#include "RelationData.h"
#include "ObjectDiagramWindow.h"
#include "DiagramView.h"
#include "UmlPixmap.h"
#include "SettingsDialog.h"
#include "myio.h"
#include "strutil.h"
#include "ToolCom.h"
#include "Tool.h"
#include "ui/menufactory.h"
#include "BrowserView.h"
#include "ProfiledStereotypes.h"
#include "mu.h"
#include "translate.h"

QList<BrowserObjectDiagram *> BrowserObjectDiagram::imported;
Q3ValueList<int> BrowserObjectDiagram::imported_ids;
QStringList BrowserObjectDiagram::its_default_stereotypes;	// unicode

BrowserObjectDiagram::BrowserObjectDiagram(QString s, BrowserNode * p, int id)
    : BrowserDiagram(s, p, id), window(0), used_settings(0)
{
    make();
    is_modified = (id == 0);
}

BrowserObjectDiagram::BrowserObjectDiagram(int id)
    : BrowserDiagram(id), window(0), used_settings(0)
{
    // not yet read
    make();
    is_modified = (id == 0);
}

BrowserObjectDiagram::BrowserObjectDiagram(BrowserObjectDiagram * model, BrowserNode * p)
    : BrowserDiagram(p->get_name(), p, 0), window(0), used_settings(0)
{
    def = new SimpleData(model->def);
    def->set_browser_node(this);
    comment = model->comment;
    settings = model->settings;
    note_color = model->note_color;
    fragment_color = model->fragment_color;
    package_color = model->package_color;
    class_instance_color = model->class_instance_color;
    canvas_size = model->canvas_size;
    is_modified = TRUE;

    if (model->window != 0)
        model->window->duplicate(get_ident(), "diagram");
    else {
        char * diagram;

        if ((diagram = read_definition(model->get_ident(), "diagram")) != 0) {
            save_definition(get_ident(), "diagram", diagram, is_new);
            delete [] diagram;
        }
    }
}

BrowserObjectDiagram::~BrowserObjectDiagram()
{
    if (deletedp() && !modifiedp()) {
        QString fn;

        fn.sprintf("%d.diagram", get_ident());

        QDir d = BrowserView::get_dir();

        QFile::remove(d.absFilePath(fn));
    }
    on_close();
//    if(window)
//        window->set_browser_diagram(0);

    all.remove(get_ident());
    delete def;
}

void BrowserObjectDiagram::set_diagram_window(DiagramWindow * value)
{
    window = dynamic_cast<ObjectDiagramWindow*>(value);
}

void BrowserObjectDiagram::make()
{
    def = new SimpleData;
    def->set_browser_node(this);
    note_color = UmlDefaultColor;
    fragment_color = UmlDefaultColor;
    package_color = UmlDefaultColor;
    class_instance_color = UmlDefaultColor;
}

BrowserObjectDiagram * BrowserObjectDiagram::add_object_diagram(BrowserNode * future_parent)
{
    QString name;

    if (future_parent->enter_child_name(name, TR("enter object diagram's name : "),
                                        UmlObjectDiagram, TRUE, FALSE))
        return new BrowserObjectDiagram(name, future_parent);
    else
        return 0;
}

void BrowserObjectDiagram::set_name(const char * s)
{
    BrowserDiagram::set_name(s);

    if (window != 0)
        window->setCaption(s);
}

void BrowserObjectDiagram::import()
{
    Q3ValueList<int>::Iterator it = imported_ids.begin();

    foreach (BrowserObjectDiagram *d, imported) {
        (new ObjectDiagramWindow(d->full_name(), d, *it))->close(TRUE);
        d->set_diagram_window(nullptr);
        it = imported_ids.remove(it);
        d->is_modified = TRUE;
    }
    imported.clear();
}

void BrowserObjectDiagram::renumber(int phase)
{
    if (phase == -1)
        open(FALSE);
    else {
        new_ident(phase, all);
        window->get_view()->renumber(get_ident());
        is_modified = TRUE;
    }
}

void BrowserObjectDiagram::delete_it()
{
    if (window)
        delete window;

    BrowserNode::delete_it();
}

BrowserNode * BrowserObjectDiagram::duplicate(BrowserNode * p, QString name)
{
    BrowserObjectDiagram * result = new BrowserObjectDiagram(this, p);

    result->set_name((name.isEmpty()) ? get_name() : name);
    result->update_stereotype();

    return result;
}

const QPixmap * BrowserObjectDiagram::pixmap(int) const
{
    if (deletedp())
        return DeletedObjectDiagramIcon;

    const QPixmap * px = ProfiledStereotypes::browserPixmap(def->get_stereotype());

    return (px != 0) ? px : ObjectDiagramIcon;
}

void BrowserObjectDiagram::draw_svg() const
{
    fputs("\t<rect fill=\"white\" stroke=\"black\" stroke-width=\"1\" stroke-opacity=\"1\" x=\"0\" y=\"0\" width=\"15\" height=\"15\" />\n"
          "\t<path fill=\"none\" stroke=\"black\" stroke-width=\"1\" stroke-opacity=\"1\" "
          "d=\"M 2 2 L 10 2 L 10 6 L 2 6 L 2 2 "
          "M 5 10 L 13 10 L 13 13 L 5 13 L 5 10 "
          "M 5 6 L 9 10\" />\n",
          svg());
}

void BrowserObjectDiagram::menu()
{
    Q3PopupMenu m(0, name);
    Q3PopupMenu toolm(0);

    MenuFactory::createTitle(m, def->definition(FALSE, TRUE));
    m.insertSeparator();

    if (!deletedp()) {
        m.setWhatsThis(m.insertItem(TR("Show"), 0),
                       TR("to show and edit the <i>object diagram</i>"));

        if (!is_edited) {
            m.setWhatsThis(m.insertItem(TR("Edit"), 1),
                           TR("to edit the <i>object diagram</i>"));

            if (!is_read_only) {
                m.setWhatsThis(m.insertItem(TR("Edit drawing settings"), 2),
                               TR("to set how the <i>object diagram</i>'s items must be drawn"));
                m.insertSeparator();
                m.setWhatsThis(m.insertItem(TR("Duplicate"), 3),
                               TR("to duplicate the <i>object diagram</i>"));

                if (edition_number == 0) {
                    m.insertSeparator();
                    m.setWhatsThis(m.insertItem(TR("Delete"), 4),
                                   TR("to delete the <i>object diagram</i>. \
Note that you can undelete it after"));
                }
            }
        }

        mark_menu(m, TR("the object diagram"), 90);
        ProfiledStereotypes::menu(m, this, 99990);

        if ((edition_number == 0) &&
            Tool::menu_insert(&toolm, get_type(), 100)) {
            m.insertSeparator();
            m.insertItem(TR("Tool"), &toolm);
        }
    }
    else if (!is_read_only && (edition_number == 0))
        m.setWhatsThis(m.insertItem(TR("Undelete"), 5),
                       TR("to undelete the <i>object diagram</i>"));

    exec_menu_choice(m.exec(QCursor::pos()));
}

void BrowserObjectDiagram::exec_menu_choice(int rank)
{
    switch (rank) {
    case 0:
        open(FALSE);
        break;

    case 1:
        edit(TR("Object diagram"), its_default_stereotypes);
        return;

    case 2:
        edit_settings();
        return;

    case 3: {
        QString name;

        if (((BrowserNode *)parent())->enter_child_name(name, TR("enter object diagram's name : "),
                UmlObjectDiagram, TRUE, FALSE))
            duplicate((BrowserNode *) parent(), name)->select_in_browser();
        else
            return;
    }
    break;

    case 4:
        delete_it();
        break;

    case 5:
        undelete(FALSE);
        break;

    default:
        if (rank >= 99990)
            ProfiledStereotypes::choiceManagement(this, rank - 99990);
        else if (rank >= 100)
            ToolCom::run(Tool::command(rank - 100), this);
        else
            mark_management(rank - 90);

        return;
    }

    package_modified();
}

void BrowserObjectDiagram::apply_shortcut(QString s)
{
    int choice = -1;

    if (!deletedp()) {
        if (s == "Show")
            choice = 0;

        if (!is_edited) {
            if (s == "Edit")
                choice = 1;

            if (!is_read_only) {
                if (s == "Edit drawing settings")
                    choice = 2;
                else if (s == "Duplicate")
                    choice = 3;

                if (edition_number == 0) {
                    if (s == "Delete")
                        choice = 4;
                }
            }
        }

        mark_shortcut(s, choice, 90);

        if (edition_number == 0)
            Tool::shortcut(s, choice, get_type(), 100);
    }
    else if (!is_read_only && (edition_number == 0))
        if (s == "Undelete")
            choice = 5;

    exec_menu_choice(choice);
}

void BrowserObjectDiagram::open(bool)
{
    if (window == 0 || windowDestroyed)
        window = new ObjectDiagramWindow(full_name(TRUE), this);
    else
        window->raise();

    window->setFocus();
}

void BrowserObjectDiagram::edit_settings()
{
    for (;;) {
        StateSpecVector st;
        ColorSpecVector co(4);

        settings.complete(st, TRUE);

        co[0].set(TR("package color"), &package_color);
        co[1].set(TR("fragment color"), &fragment_color);
        co[2].set(TR("note color"), &note_color);
        co[3].set(TR("class instance \ncolor"), &class_instance_color);

        SettingsDialog dialog(&st, &co, FALSE);

        dialog.raise();

        if (dialog.exec() == QDialog::Accepted) {
            DrawingSettings::modified();
            modified();
            package_modified();
        }

        if (!dialog.redo())
            break;
    }
}

void BrowserObjectDiagram::on_close()
{
    //window = 0;

    if (used_settings != 0) {
        delete used_settings;
        used_settings = 0;
    }
}

void BrowserObjectDiagram::read_session(char *& st)
{
    window->read_session(st);
}

UmlCode BrowserObjectDiagram::get_type() const
{
    return UmlObjectDiagram;
}

QString BrowserObjectDiagram::get_stype() const
{
    return TR("Object diagram");
}

int BrowserObjectDiagram::get_identifier() const
{
    return get_ident();
}

const char * BrowserObjectDiagram::help_topic() const
{
    return "objectdiagram";
}

void BrowserObjectDiagram::update_drawing_settings()
{
    if (used_settings == 0)
        used_settings = new ObjectDiagramSettings;

    *used_settings = settings;
    ((BrowserNode *) parent())->get_objectdiagramsettings(*used_settings);
}

void BrowserObjectDiagram::get_objectdiagramsettings(ObjectDiagramSettings & r) const
{
    r.assign(*used_settings);
}

void BrowserObjectDiagram::package_settings(BooL & name_in_tab,
        ShowContextMode & show_context) const
{
    name_in_tab = used_settings->package_name_in_tab == UmlYes;
    show_context = used_settings->show_class_pack_context_mode;
}

UmlColor BrowserObjectDiagram::get_color(UmlCode who) const
{
    UmlColor c;

    switch (who) {
    case UmlNote:
        c = note_color;
        break;

    case UmlPackage:
        c = package_color;
        break;

    case UmlFragment:
        c = fragment_color;
        break;

    default:
        c = class_instance_color;
    }

    return (c != UmlDefaultColor)
           ? c
           : ((BrowserNode *) parent())->get_color(who);
}

bool BrowserObjectDiagram::get_auto_label_position() const
{
    return used_settings->auto_label_position == UmlYes;
}

bool BrowserObjectDiagram::get_shadow() const
{
    return used_settings->shadow == UmlYes;
}

bool BrowserObjectDiagram::get_draw_all_relations() const
{
    return used_settings->draw_all_relations == UmlYes;
}

void BrowserObjectDiagram::dont_draw_all_relations()
{
    settings.draw_all_relations =
        used_settings->draw_all_relations = UmlNo;
}

bool BrowserObjectDiagram::get_show_stereotype_properties() const
{
    return used_settings->show_stereotype_properties == UmlYes;
}

bool BrowserObjectDiagram::get_classinstwritehorizontally() const
{
    return used_settings->write_horizontally == UmlYes;
}

ShowContextMode BrowserObjectDiagram::get_classinstshowmode() const
{
    return used_settings->show_class_pack_context_mode;
}

BasicData * BrowserObjectDiagram::get_data() const
{
    return def;
}

bool BrowserObjectDiagram::tool_cmd(ToolCom * com, const char * args)
{
    switch ((unsigned char) args[-1]) {
    case supportFileCmd: {
        QString fn;

        fn.sprintf("%d.diagram", get_ident());

        QDir d = BrowserView::get_dir();

        com->write_string(d.absFilePath(fn));
    }

    return TRUE;

    case saveInCmd:
        if (window != 0)
            com->write_ack(window->get_view()->save_pict(args, TRUE, FALSE));
        else {
            ObjectDiagramWindow * w = new ObjectDiagramWindow(full_name(), this);

            com->write_ack(w->get_view()->save_pict(args,
                                                    !w->get_view()->has_preferred_size_zoom(),
                                                    TRUE));
            w->dont_save();
            w->close(TRUE);
        }

        return TRUE;

    default:
        return (def->tool_cmd(com, args, this, comment) ||
                BrowserNode::tool_cmd(com, args));
    }
}

void BrowserObjectDiagram::compute_referenced_by(QList<BrowserNode *> & l,
        BrowserNode * bn,
        char const * kc,
        char const * kr)
{
    int id = (IsaRelation(bn->get_type()))
             ? ((RelationData *) bn->get_data())->get_ident()
             : bn->get_identifier();

    IdIterator<BrowserDiagram> it(all);
    BrowserDiagram * d;

    while ((d = it.current()) != 0) {
        if (!d->deletedp() && (d->get_type() == UmlObjectDiagram)) {
            if ((((BrowserObjectDiagram *) d)->window != 0)
                ? ((BrowserObjectDiagram *) d)->window->get_view()->is_present(bn)
                : is_referenced(read_definition(d->get_ident(), "diagram"), id, kc, kr))
                l.append((BrowserObjectDiagram *) d);
        }

        ++it;
    }
}

bool BrowserObjectDiagram::api_compatible(unsigned v) const
{
    return v >= 24;
}

void BrowserObjectDiagram::save_stereotypes(QTextStream & st)
{
    nl_indent(st);
    st << "objdiagram_stereotypes ";
    save_unicode_string_list(its_default_stereotypes, st);
}

void BrowserObjectDiagram::read_stereotypes(char *& st, char *& k)
{
    if (!strcmp(k, "objdiagram_stereotypes")) {
        read_unicode_string_list(its_default_stereotypes, st);
        k = read_keyword(st);
    }
}

void BrowserObjectDiagram::save(QTextStream & st, bool ref, QString & warning)
{
    if (ref)
        st << "objectdiagram_ref " << get_ident() << " // " << get_name();
    else {
        nl_indent(st);
        st << "objectdiagram " << get_ident() << " ";
        save_string(name, st);
        indent(+1);
        def->save(st, warning);
        settings.save(st);

        BooL nl = FALSE;

        save_color(st, "class_instance_color", class_instance_color, nl);
        save_color(st, "note_color", note_color, nl);
        save_color(st, "fragment_color", fragment_color, nl);
        save_color(st, "package_color", package_color, nl);

        BrowserNode::save(st);

        if (is_modified) {
            is_modified = FALSE;

            if (window)
                window->save("diagram", warning, is_new);
            else
                BrowserDiagram::save();
        }

        nl_indent(st);
        st << "size " << stringify(get_format());

        indent(-1);
        nl_indent(st);
        st << "end";

        // for saveAs
        if (!is_from_lib() && !is_api_base())
            is_read_only = FALSE;
    }
}

BrowserObjectDiagram * BrowserObjectDiagram::read_ref(char *& st, char * k)
{
    if (strcmp(k, "objectdiagram_ref"))
        wrong_keyword(k, "objectdiagram_ref");

    int id = read_id(st);
    BrowserObjectDiagram * result =
        (BrowserObjectDiagram *) all[id];

    return (result == 0)
           ? new BrowserObjectDiagram(id)
           : result;
}

BrowserObjectDiagram * BrowserObjectDiagram::read(char *& st, char * k,
        BrowserNode * parent)
{
    BrowserObjectDiagram * r;
    int id;

    if (!strcmp(k, "objectdiagram_ref")) {
        if ((r = (BrowserObjectDiagram *) all[id = read_id(st)]) == 0)
            r = new BrowserObjectDiagram(id);

        return r;
    }
    else if (!strcmp(k, "objectdiagram")) {
        id = read_id(st);

        QString s = read_string(st);

        if ((r = (BrowserObjectDiagram *) all[id]) == 0)
            r = new BrowserObjectDiagram(s, parent, id);
        else if (r->is_defined) {
            BrowserObjectDiagram * already_exist = r;

            r = new BrowserObjectDiagram(s, parent, id);

            already_exist->must_change_id(all);
            already_exist->unconsistent_fixed("object diagram", r);
        }
        else {
            r->set_parent(parent);
            r->set_name(s);
        }
        r->set_diagram_window(0);
        r->is_defined = TRUE;

        r->is_read_only = (!in_import() && read_only_file()) ||
                          ((user_id() != 0) && r->is_api_base());

        QFileInfo fi(BrowserView::get_dir(), QString::number(id) + ".diagram");

        if (!in_import() && fi.exists() && !fi.isWritable())
            r->is_read_only = TRUE;

        k = read_keyword(st);

        r->def->read(st, k);				// updates k
        r->settings.read(st, k);				// updates k

        if (read_file_format() < 30)
            r->settings.draw_all_relations = UmlNo;

        read_color(st, "class_instance_color", r->class_instance_color, k); // updates k
        read_color(st, "note_color", r->note_color, k);	// updates k
        read_color(st, "fragment_color", r->fragment_color, k);	// updates k
        read_color(st, "package_color", r->package_color, k);	// updates k
        r->BrowserNode::read(st, k, id);			// updates k

        if (!strcmp(k, "size")) {
            r->set_format(canvas_format(read_keyword(st)));
            k = read_keyword(st);
        }
        else
            r->set_format(IsoA4);

        if (strcmp(k, "end"))
            wrong_keyword(k, "end");

        if (in_import()) {
            imported.append(r);
            imported_ids.append(id);
        }

        return r;
    }
    else
        return 0;
}

BrowserNode * BrowserObjectDiagram::get_it(const char * k, int id)
{
    return (!strcmp(k, "objectdiagram_ref")) ? all[id] : 0;
}
