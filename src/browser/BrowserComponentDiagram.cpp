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

#include "BrowserComponentDiagram.h"
#include "SimpleData.h"
#include "ComponentDiagramWindow.h"
#include "BrowserComponentView.h"
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

QList<BrowserComponentDiagram *> BrowserComponentDiagram::imported;
Q3ValueList<int> BrowserComponentDiagram::imported_ids;
QStringList BrowserComponentDiagram::its_default_stereotypes;	// unicode

BrowserComponentDiagram::BrowserComponentDiagram(QString s, BrowserNode * p, int id)
    : BrowserDiagram(s, p, id), window(0), used_settings(0)
{
    make();
    is_modified = (id == 0);
}

BrowserComponentDiagram::BrowserComponentDiagram(int id)
    : BrowserDiagram(id), window(0), used_settings(0)
{
    // not yet read
    make();
    is_modified = (id == 0);
}

BrowserComponentDiagram::BrowserComponentDiagram(BrowserComponentDiagram * model, BrowserNode * p)
    : BrowserDiagram(p->get_name(), p, 0), window(0), used_settings(0)
{
    def = new SimpleData(model->def);
    def->set_browser_node(this);
    comment = model->comment;
    settings = model->settings;
    component_color = model->component_color;
    note_color = model->note_color;
    package_color = model->package_color;
    fragment_color = model->fragment_color;
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

BrowserComponentDiagram::~BrowserComponentDiagram()
{
    if (deletedp() && !modifiedp()) {
        QString fn;

        fn.sprintf("%d.diagram", get_ident());

        QDir d = BrowserView::get_dir();

        QFile::remove(d.absFilePath(fn));
    }

    all.remove(get_ident());
    delete def;
}

void BrowserComponentDiagram::make()
{
    def = new SimpleData;
    def->set_browser_node(this);
    component_color = UmlDefaultColor;
    note_color = UmlDefaultColor;
    package_color = UmlDefaultColor;
    fragment_color = UmlDefaultColor;
}

BrowserComponentDiagram * BrowserComponentDiagram::add_component_diagram(BrowserNode * future_parent)
{
    QString name;

    if (future_parent->enter_child_name(name, TR("enter component diagram's name : "),
                                        UmlComponentDiagram, TRUE, FALSE))
        return new BrowserComponentDiagram(name, future_parent);
    else
        return 0;
}

void BrowserComponentDiagram::set_name(const char * s)
{
    BrowserDiagram::set_name(s);

    if (window != 0)
        window->setCaption(s);
}

void BrowserComponentDiagram::import()
{
    Q3ValueList<int>::Iterator it = imported_ids.begin();

    foreach (BrowserComponentDiagram *d, imported) {
        (new ComponentDiagramWindow(d->full_name(), d, *it))->close(TRUE);
        it = imported_ids.remove(it);
        d->is_modified = TRUE;
    }
    imported.clear();
}

void BrowserComponentDiagram::renumber(int phase)
{
    if (phase == -1)
        open(FALSE);
    else {
        new_ident(phase, all);
        window->get_view()->renumber(get_ident());
        is_modified = TRUE;
    }
}

void BrowserComponentDiagram::delete_it()
{
    if (window)
        delete window;

    BrowserNode::delete_it();
}

BrowserNode * BrowserComponentDiagram::duplicate(BrowserNode * p, QString name)
{
    BrowserComponentDiagram * result = new BrowserComponentDiagram(this, p);

    result->set_name((name.isEmpty()) ? get_name() : name);
    result->update_stereotype();

    return result;
}

const QPixmap * BrowserComponentDiagram::pixmap(int) const
{
    if (deletedp())
        return DeletedComponentDiagramIcon;

    const QPixmap * px = ProfiledStereotypes::browserPixmap(def->get_stereotype());

    return (px != 0) ? px : ComponentDiagramIcon;
}

void BrowserComponentDiagram::draw_svg() const
{
    fputs("\t<rect fill=\"white\" stroke=\"black\" stroke-width=\"1\" stroke-opacity=\"1\" x=\"0\" y=\"0\" width=\"15\" height=\"15\" />\n"
          "\t<path fill=\"none\" stroke=\"black\" stroke-width=\"1\" stroke-opacity=\"1\" "
          "d=\"M 2 5 L 7 5 L 7 13 L 2 13 L 2 5 "
          "M 10 0 L 10 8 L 15 8 "
          "M 7 11 L 12 11 L 12 8 "
          "M 1 7 L 3 7 L 3 8 L 1 8 L 1 7 "
          "M 1 10 L 3 10 L 3 11 L 1 11 L 1 10 "
          "M 9 2 L 11 2 L 11 3 L 9 3 L 9 2 "
          "M 9 5 L 11 5 L 11 6 L 9 6 L 9 5 \" />\n",
          svg());
}

void BrowserComponentDiagram::menu()
{
    Q3PopupMenu m(0, name);
    Q3PopupMenu toolm(0);

    MenuFactory::createTitle(m, def->definition(FALSE, TRUE));
    m.insertSeparator();

    if (!deletedp()) {
        m.setWhatsThis(m.insertItem(TR("Show"), 0),
                       TR("to show and edit the <i>component diagram</i>"));

        if (!is_edited) {
            m.setWhatsThis(m.insertItem(TR("Edit"), 1),
                           TR("to edit the <i>component diagram</i>"));

            if (!is_read_only) {
                m.setWhatsThis(m.insertItem(TR("Edit drawing settings"), 2),
                               TR("to set how the <i>component diagram</i>'s items must be drawn"));
                m.insertSeparator();
                m.setWhatsThis(m.insertItem(TR("Duplicate"), 3),
                               TR("to duplicate the <i>component diagram</i>"));

                if (edition_number == 0) {
                    m.insertSeparator();
                    m.setWhatsThis(m.insertItem(TR("Delete"), 4),
                                   TR("to delete the <i>component diagram</i>. \
Note that you can undelete it after"));
                }
            }
        }

        mark_menu(m, TR("the component diagram"), 90);
        ProfiledStereotypes::menu(m, this, 99990);

        if ((edition_number == 0) &&
            Tool::menu_insert(&toolm, get_type(), 100)) {
            m.insertSeparator();
            m.insertItem(TR("Tool"), &toolm);
        }
    }
    else if (!is_read_only && (edition_number == 0))
        m.setWhatsThis(m.insertItem(TR("Undelete"), 5),
                       TR("to undelete the <i>component diagram</i>"));

    exec_menu_choice(m.exec(QCursor::pos()));
}

void BrowserComponentDiagram::exec_menu_choice(int rank)
{
    switch (rank) {
    case 0:
        open(FALSE);
        return;

    case 1:
        edit(TR("Component diagram"), its_default_stereotypes);
        return;

    case 2:
        edit_settings();
        return;

    case 3: {
        QString name;

        if (((BrowserNode *)parent())->enter_child_name(name, TR("enter component diagram's name : "),
                UmlComponentDiagram, TRUE, FALSE))
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

void BrowserComponentDiagram::apply_shortcut(QString s)
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

void BrowserComponentDiagram::open(bool)
{
    if (window == 0 || windowDestroyed)
        window = new ComponentDiagramWindow(full_name(TRUE), this);
    else
        window->raise();

    window->setFocus();
}

void BrowserComponentDiagram::edit_settings()
{
    for (;;) {
        StateSpecVector st;
        ColorSpecVector co(4);

        settings.complete(st, TRUE);

        co[0].set(TR("note color"), &note_color);
        co[1].set(TR("component color"), &component_color);
        co[2].set(TR("package color"), &package_color);
        co[3].set(TR("fragment color"), &fragment_color);

        SettingsDialog dialog(&st, &co, FALSE);

        dialog.raise();

        if (dialog.exec() == QDialog::Accepted) {
            DrawingSettings::modified();
            package_modified();
        }

        if (!dialog.redo())
            break;
    }
}

void BrowserComponentDiagram::on_close()
{
    window = 0;

    if (used_settings != 0) {
        delete used_settings;
        used_settings = 0;
    }
}

void BrowserComponentDiagram::read_session(char *& st)
{
    window->read_session(st);
}

UmlCode BrowserComponentDiagram::get_type() const
{
    return UmlComponentDiagram;
}

QString BrowserComponentDiagram::get_stype() const
{
    return TR("component diagram");
}

int BrowserComponentDiagram::get_identifier() const
{
    return get_ident();
}

const char * BrowserComponentDiagram::help_topic() const
{
    return "componentdiagram";
}

void BrowserComponentDiagram::update_drawing_settings()
{
    if (used_settings == 0)
        used_settings = new ComponentDiagramSettings;

    *used_settings = settings;
    ((BrowserNode *) parent())->get_componentdiagramsettings(*used_settings);
}

void BrowserComponentDiagram::get_componentdiagramsettings(ComponentDiagramSettings & r) const
{
    r.assign(*used_settings);
}

void BrowserComponentDiagram::get_componentdrawingsettings(ComponentDrawingSettings & r) const
{
    r.assign(*used_settings);
}

void BrowserComponentDiagram::package_settings(BooL & name_in_tab,
        ShowContextMode & show_context) const
{
    name_in_tab = used_settings->package_name_in_tab == UmlYes;
    show_context = used_settings->show_context_mode;
}

UmlColor BrowserComponentDiagram::get_color(UmlCode who) const
{
    UmlColor c;

    switch (who) {
    case UmlComponent:
        c = component_color;
        break;

    case UmlNote:
        c = note_color;
        break;

    case UmlFragment:
        c = fragment_color;
        break;

    default:
        c = package_color;
    }

    return (c != UmlDefaultColor)
           ? c
           : ((BrowserNode *) parent())->get_color(who);
}

bool BrowserComponentDiagram::get_shadow() const
{
    return used_settings->shadow == UmlYes;
}

bool BrowserComponentDiagram::get_draw_all_relations() const
{
    return used_settings->draw_all_relations == UmlYes;
}

void BrowserComponentDiagram::dont_draw_all_relations()
{
    settings.draw_all_relations =
        used_settings->draw_all_relations = UmlNo;
}

bool BrowserComponentDiagram::get_show_stereotype_properties() const
{
    return used_settings->componentdrawingsettings.show_stereotype_properties
           == UmlYes;
}

bool BrowserComponentDiagram::get_auto_label_position() const
{
    return used_settings->auto_label_position == UmlYes;
}

BasicData * BrowserComponentDiagram::get_data() const
{
    return def;
}

bool BrowserComponentDiagram::tool_cmd(ToolCom * com, const char * args)
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
            ComponentDiagramWindow * w = new ComponentDiagramWindow(full_name(), this);

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

void BrowserComponentDiagram::compute_referenced_by(QList<BrowserNode *> & l,
        BrowserNode * bn,
        char const * kc,
        char const * kr)
{
    int id = bn->get_identifier();
    IdIterator<BrowserDiagram> it(all);
    BrowserDiagram * d;

    while ((d = it.current()) != 0) {
        if (!d->deletedp() && (d->get_type() == UmlComponentDiagram)) {
            if ((((BrowserComponentDiagram *) d)->window != 0)
                ? ((BrowserComponentDiagram *) d)->window->get_view()->is_present(bn)
                : is_referenced(read_definition(d->get_ident(), "diagram"), id, kc, kr))
                l.append((BrowserComponentDiagram *) d);
        }

        ++it;
    }
}

void BrowserComponentDiagram::save_stereotypes(QTextStream & st)
{
    nl_indent(st);
    st << "componentdiagram_stereotypes ";
    save_unicode_string_list(its_default_stereotypes, st);
}

void BrowserComponentDiagram::read_stereotypes(char *& st, char *& k)
{
    if (!strcmp(k, "componentdiagram_stereotypes")) {
        read_unicode_string_list(its_default_stereotypes, st);
        k = read_keyword(st);
    }
}

void BrowserComponentDiagram::save(QTextStream & st, bool ref, QString & warning)
{
    if (ref)
        st << "componentdiagram_ref " << get_ident() << " // " << get_name();
    else {
        nl_indent(st);
        st << "componentdiagram " << get_ident() << " ";
        save_string(name, st);
        indent(+1);
        def->save(st, warning);
        settings.save(st);

        BooL nl = FALSE;

        save_color(st, "note_color", note_color, nl);
        save_color(st, "component_color", component_color, nl);
        save_color(st, "package_color", package_color, nl);
        save_color(st, "fragment_color", fragment_color, nl);

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

BrowserComponentDiagram * BrowserComponentDiagram::read_ref(char *& st, char * k)
{
    if (strcmp(k, "componentdiagram_ref"))
        wrong_keyword(k, "componentdiagram_ref");

    int id = read_id(st);
    BrowserComponentDiagram * result =
        (BrowserComponentDiagram *) all[id];

    return (result == 0)
           ? new BrowserComponentDiagram(id)
           : result;
}

BrowserComponentDiagram *
BrowserComponentDiagram::read(char *& st, char * k,
                              BrowserNode * parent)
{
    BrowserComponentDiagram * r;
    int id;

    if (!strcmp(k, "componentdiagram_ref")) {
        if ((r = (BrowserComponentDiagram *) all[id = read_id(st)]) == 0)
            r = new BrowserComponentDiagram(id);

        return r;
    }
    else if (!strcmp(k, "componentdiagram")) {
        id = read_id(st);

        QString s = read_string(st);

        if ((r = (BrowserComponentDiagram *) all[id]) == 0)
            r = new BrowserComponentDiagram(s, parent, id);
        else if (r->is_defined) {
            BrowserComponentDiagram * already_exist = r;

            r = new BrowserComponentDiagram(s, parent, id);

            already_exist->must_change_id(all);
            already_exist->unconsistent_fixed("component diagram", r);
        }
        else {
            r->set_parent(parent);
            r->set_name(s);
        }

        r->is_defined = TRUE;

        r->is_read_only = (!in_import() && read_only_file()) ||
                          ((user_id() != 0) && r->is_api_base());

        QFileInfo fi(BrowserView::get_dir(), QString::number(id) + ".diagram");

        if (!in_import() && fi.exists() && !fi.isWritable())
            r->is_read_only = TRUE;

        k = read_keyword(st);

        r->def->read(st, k);					// updates k
        r->settings.read(st, k);					// updates k

        if (read_file_format() < 30)
            r->settings.draw_all_relations = UmlNo;

        read_color(st, "note_color", r->note_color, k);		// updates k
        read_color(st, "component_color", r->component_color, k);	// updates k
        read_color(st, "package_color", r->package_color, k);	// updates k
        read_color(st, "fragment_color", r->fragment_color, k);	// updates k
        r->BrowserNode::read(st, k, id);	// updates k

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

BrowserNode * BrowserComponentDiagram::get_it(const char * k, int id)
{
    return (!strcmp(k, "componentdiagram_ref")) ? all[id] : 0;
}
