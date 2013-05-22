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

#include "BrowserUseCaseDiagram.h"
#include "RelationData.h"
#include "SimpleData.h"
#include "UseCaseDiagramWindow.h"
#include "BrowserUseCaseView.h"
#include "UseCaseDiagramView.h"
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

QList<BrowserUseCaseDiagram *> BrowserUseCaseDiagram::imported;
Q3ValueList<int> BrowserUseCaseDiagram::imported_ids;
QStringList BrowserUseCaseDiagram::its_default_stereotypes;	// unicode

BrowserUseCaseDiagram::BrowserUseCaseDiagram(QString s, BrowserNode * p, int id)
    : BrowserDiagram(s, p, id), window(0), used_settings(0)
{
    make();
    is_modified = (id == 0);
}

BrowserUseCaseDiagram::BrowserUseCaseDiagram(int id)
    : BrowserDiagram(id), window(0), used_settings(0)
{
    // not yet read
    make();
    is_modified = (id == 0);
}

BrowserUseCaseDiagram::BrowserUseCaseDiagram(BrowserUseCaseDiagram * model, BrowserNode * p)
    : BrowserDiagram(p->get_name(), p, 0), window(0), used_settings(0)
{
    def = new SimpleData(model->def);
    def->set_browser_node(this);
    comment = model->comment;
    settings = model->settings;
    usecase_color = model->usecase_color;
    note_color = model->note_color;
    package_color = model->package_color;
    fragment_color = model->fragment_color;
    subject_color = model->subject_color;
    class_color = model->class_color;
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

BrowserUseCaseDiagram::~BrowserUseCaseDiagram()
{
    if (deletedp() && !modifiedp()) {
        QString fn;

        fn.sprintf("%d.diagram", get_ident());

        QDir d = BrowserView::get_dir();

        QFile::remove(d.absFilePath(fn));
    }

#if 0

    // already done before browser->clear
    if (window)
        delete window;

#endif
    all.remove(get_ident());
    delete def;
}

void BrowserUseCaseDiagram::make()
{
    def = new SimpleData;
    def->set_browser_node(this);
    usecase_color = UmlDefaultColor;
    note_color = UmlDefaultColor;
    package_color = UmlDefaultColor;
    fragment_color = UmlDefaultColor;
    subject_color = UmlDefaultColor;
    class_color = UmlDefaultColor;
}

BrowserUseCaseDiagram * BrowserUseCaseDiagram::add_use_case_diagram(BrowserNode * future_parent)
{
    QString name;

    if (future_parent->enter_child_name(name, TR("enter use case diagram's name : "),
                                        UmlUseCaseDiagram, TRUE, FALSE))
        return new BrowserUseCaseDiagram(name, future_parent);
    else
        return 0;
}

void BrowserUseCaseDiagram::set_name(const char * s)
{
    BrowserDiagram::set_name(s);

    if (window != 0)
        window->setCaption(s);
}

void BrowserUseCaseDiagram::import()
{
    Q3ValueList<int>::Iterator it = imported_ids.begin();

    foreach (BrowserUseCaseDiagram *d, imported) {
        (new UseCaseDiagramWindow(d->full_name(), d, *it))->close(TRUE);
        it = imported_ids.remove(it);
        d->is_modified = TRUE;
    }
    imported.clear();
}

void BrowserUseCaseDiagram::renumber(int phase)
{
    if (phase == -1)
        open(FALSE);
    else {
        new_ident(phase, all);
        window->get_view()->renumber(get_ident());
        is_modified = TRUE;
    }
}

void BrowserUseCaseDiagram::delete_it()
{
    if (window)
        delete window;

    BrowserNode::delete_it();
}

BrowserNode * BrowserUseCaseDiagram::duplicate(BrowserNode * p, QString name)
{
    BrowserUseCaseDiagram * result = new BrowserUseCaseDiagram(this, p);

    result->set_name((name.isEmpty()) ? get_name() : name);
    result->update_stereotype();

    return result;
}

const QPixmap * BrowserUseCaseDiagram::pixmap(int) const
{
    if (deletedp())
        return DeletedUseCaseDiagramIcon;

    const QPixmap * px = ProfiledStereotypes::browserPixmap(def->get_stereotype());

    return (px != 0) ? px : UseCaseDiagramIcon;
}

void BrowserUseCaseDiagram::draw_svg() const
{
    fputs("\t<rect fill=\"white\" stroke=\"black\" stroke-width=\"1\" stroke-opacity=\"1\" x=\"0\" y=\"0\" width=\"15\" height=\"15\" />\n"
          "\t<path fill=\"none\" stroke=\"black\" stroke-width=\"1\" stroke-opacity=\"1\" "
          "d=\"M 3 0 L 4 1 L 4 2 L 3 3 L 2 2 L 2 1 L 2 0 "
          "M 3 3 L 3 6 "
          "M 1 8 L 3 6 L 5 8 "
          "M 7 3 L 9 5 "
          "M 9 7 L 11 7 L 11 5 "
          "M 7 14 L 6 14 L 5 13 L 5 11 L 6 10 L 7 10 L 8 9 L 12 9 L 13 10 L 14 10 L 15 11"
          "M 1 4 L 5 4\" />\n",
          svg());
}

void BrowserUseCaseDiagram::menu()
{
    Q3PopupMenu m(0, name);
    Q3PopupMenu toolm(0);

    MenuFactory::createTitle(m, def->definition(FALSE, TRUE));
    m.insertSeparator();

    if (!deletedp()) {
        m.setWhatsThis(m.insertItem(TR("Show"), 0),
                       TR("to show and edit the <i>use case diagram</i>"));

        if (!is_edited) {
            m.setWhatsThis(m.insertItem(TR("Edit"), 1),
                           TR("to edit the <i>use case diagram</i>"));

            if (!is_read_only) {
                m.setWhatsThis(m.insertItem(TR("Edit drawing settings"), 2),
                               TR("to set how the <i>use case diagram</i>'s items must be drawn"));
                m.insertSeparator();
                m.setWhatsThis(m.insertItem(TR("Duplicate"), 3),
                               TR("to duplicate the <i>use case diagram</i>"));

                if (edition_number == 0) {
                    m.insertSeparator();
                    m.setWhatsThis(m.insertItem(TR("Delete"), 4),
                                   TR("to delete the <i>use case diagram</i>. \
Note that you can undelete it after"));
                }
            }
        }

        mark_menu(m, TR("the use case diagram"), 90);
        ProfiledStereotypes::menu(m, this, 99990);

        if ((edition_number == 0) &&
            Tool::menu_insert(&toolm, get_type(), 100)) {
            m.insertSeparator();
            m.insertItem(TR("Tool"), &toolm);
        }
    }
    else if (!is_read_only && (edition_number == 0))
        m.setWhatsThis(m.insertItem(TR("Undelete"), 5),
                       TR("to undelete the <i>use case diagram</i>"));

    exec_menu_choice(m.exec(QCursor::pos()));
}

void BrowserUseCaseDiagram::exec_menu_choice(int rank)
{
    switch (rank) {
    case 0:
        open(FALSE);
        return;

    case 1:
        edit(TR("Use Case diagram"), its_default_stereotypes);
        return;

    case 2:
        edit_settings();
        return;

    case 3: {
        QString name;

        if (((BrowserNode *)parent())->enter_child_name(name, TR("enter use case diagram's name : "),
                UmlUseCaseDiagram, TRUE, FALSE))
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

void BrowserUseCaseDiagram::apply_shortcut(QString s)
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

void BrowserUseCaseDiagram::open(bool)
{
    if (window == 0 || windowDestroyed)
        window = new UseCaseDiagramWindow(full_name(TRUE), this);
    else
        window->raise();

    window->setFocus();
}

void BrowserUseCaseDiagram::edit_settings()
{
    for (;;) {
        StateSpecVector st;
        ColorSpecVector co(6);

        settings.complete(st, TRUE);

        co[0].set(TR("note color"), &note_color);
        co[1].set(TR("use case color"), &usecase_color);
        co[2].set(TR("package color"), &package_color);
        co[3].set(TR("fragment color"), &fragment_color);
        co[4].set(TR("subject color"), &subject_color);
        co[5].set(TR("class color"), &class_color);

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

void BrowserUseCaseDiagram::on_close()
{
    window = 0;

    if (used_settings != 0) {
        delete used_settings;
        used_settings = 0;
    }
}

void BrowserUseCaseDiagram::read_session(char *& st)
{
    window->read_session(st);
}

UmlCode BrowserUseCaseDiagram::get_type() const
{
    return UmlUseCaseDiagram;
}

QString BrowserUseCaseDiagram::get_stype() const
{
    return TR("Use Case diagram");
}

int BrowserUseCaseDiagram::get_identifier() const
{
    return get_ident();
}

const char * BrowserUseCaseDiagram::help_topic() const
{
    return "usecasediagram";
}

void BrowserUseCaseDiagram::update_drawing_settings()
{
    if (used_settings == 0)
        used_settings = new UseCaseDiagramSettings;

    *used_settings = settings;
    ((BrowserNode *) parent())->get_usecasediagramsettings(*used_settings);
}

void BrowserUseCaseDiagram::get_usecasediagramsettings(UseCaseDiagramSettings & r) const
{
    r.assign(*used_settings);
}

void BrowserUseCaseDiagram::get_simpleclassdiagramsettings(SimpleClassDiagramSettings & r) const
{
    r.assign(*used_settings);
}

void BrowserUseCaseDiagram::package_settings(BooL & name_in_tab,
        ShowContextMode & show_context) const
{
    name_in_tab = used_settings->package_name_in_tab == UmlYes;
    show_context = used_settings->show_context_mode;
}

UmlColor BrowserUseCaseDiagram::get_color(UmlCode who) const
{
    UmlColor c;

    switch (who) {
    case UmlUseCase:
        c = usecase_color;
        break;

    case UmlNote:
        c = note_color;
        break;

    case UmlFragment:
        c = fragment_color;
        break;

    case UmlSubject:
        c = subject_color;
        break;

    case UmlClass:
        c = class_color;
        break;

    default:
        c = package_color;
    }

    return (c != UmlDefaultColor)
           ? c
           : ((BrowserNode *) parent())->get_color(who);
}

bool BrowserUseCaseDiagram::get_shadow() const
{
    return used_settings->shadow == UmlYes;
}

bool BrowserUseCaseDiagram::get_draw_all_relations() const
{
    return used_settings->draw_all_relations == UmlYes;
}

void BrowserUseCaseDiagram::dont_draw_all_relations()
{
    settings.draw_all_relations =
        used_settings->draw_all_relations = UmlNo;
}

bool BrowserUseCaseDiagram::get_show_stereotype_properties() const
{
    return used_settings->show_stereotype_properties == UmlYes;
}

bool BrowserUseCaseDiagram::get_auto_label_position() const
{
    return used_settings->auto_label_position == UmlYes;
}

BasicData * BrowserUseCaseDiagram::get_data() const
{
    return def;
}

bool BrowserUseCaseDiagram::tool_cmd(ToolCom * com, const char * args)
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
            UseCaseDiagramWindow * w = new UseCaseDiagramWindow(full_name(), this);

            com->write_ack(w->get_view()->save_pict(args,
                                                    !w->get_view()->has_preferred_size_zoom(),
                                                    TRUE));
            w->dont_save();
            w->close(TRUE);
        }

        return TRUE;

    case sideCmd:
        if (window != 0)
            ((UseCaseDiagramView *) window->get_view())->send(com);
        else {
            UseCaseDiagramWindow * w = new UseCaseDiagramWindow(full_name(), this);

            ((UseCaseDiagramView *) w->get_view())->send(com);
            w->dont_save();
            w->close(TRUE);
        }

        return TRUE;

    default:
        return (def->tool_cmd(com, args, this, comment) ||
                BrowserNode::tool_cmd(com, args));
    }
}

void BrowserUseCaseDiagram::compute_referenced_by(QList<BrowserNode *> & l,
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
        if (!d->deletedp() && (d->get_type() == UmlUseCaseDiagram)) {
            if ((((BrowserUseCaseDiagram *) d)->window != 0)
                ? ((BrowserUseCaseDiagram *) d)->window->get_view()->is_present(bn)
                : is_referenced(read_definition(d->get_ident(), "diagram"), id, kc, kr))
                l.append((BrowserUseCaseDiagram *) d);
        }

        ++it;
    }
}

void BrowserUseCaseDiagram::save_stereotypes(QTextStream & st)
{
    nl_indent(st);
    st << "usecasediagram_stereotypes ";
    save_unicode_string_list(its_default_stereotypes, st);
}

void BrowserUseCaseDiagram::read_stereotypes(char *& st, char *& k)
{
    if (!strcmp(k, "usecasediagram_stereotypes")) {
        read_unicode_string_list(its_default_stereotypes, st);
        k = read_keyword(st);
    }
}

void BrowserUseCaseDiagram::save(QTextStream & st, bool ref, QString & warning)
{
    if (ref)
        st << "usecasediagram_ref " << get_ident() << " // " << get_name();
    else {
        nl_indent(st);
        st << "usecasediagram " << get_ident() << " ";
        save_string(name, st);
        indent(+1);
        def->save(st, warning);
        settings.save(st);

        BooL nl = FALSE;

        save_color(st, "note_color", note_color, nl);
        save_color(st, "usecase_color", usecase_color, nl);
        save_color(st, "package_color", package_color, nl);
        save_color(st, "fragment_color", fragment_color, nl);
        save_color(st, "subject_color", subject_color, nl);
        save_color(st, "class_color", class_color, nl);

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


BrowserUseCaseDiagram * BrowserUseCaseDiagram::read_ref(char *& st, const char * k)
{
    if (strcmp(k, "usecasediagram_ref"))
        wrong_keyword(k, "usecasediagram_ref");

    int id = read_id(st);
    BrowserUseCaseDiagram * result =
        (BrowserUseCaseDiagram *) all[id];

    return (result == 0)
           ? new BrowserUseCaseDiagram(id)
           : result;
}

BrowserUseCaseDiagram * BrowserUseCaseDiagram::read(char *& st, char * k,
        BrowserNode * parent)
{
    BrowserUseCaseDiagram * r;
    int id;

    if (!strcmp(k, "usecasediagram_ref")) {
        if ((r = (BrowserUseCaseDiagram *) all[id = read_id(st)]) == 0)
            r = new BrowserUseCaseDiagram(id);

        return r;
    }
    else if (!strcmp(k, "usecasediagram")) {
        id = read_id(st);

        QString s = read_string(st);

        if ((r = (BrowserUseCaseDiagram *) all[id]) == 0)
            r = new BrowserUseCaseDiagram(s, parent, id);
        else if (r->is_defined) {
            BrowserUseCaseDiagram * already_exist = r;

            r = new BrowserUseCaseDiagram(s, parent, id);

            already_exist->must_change_id(all);
            already_exist->unconsistent_fixed("use case diagram", r);
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
        read_color(st, "usecase_color", r->usecase_color, k);	// updates k
        read_color(st, "package_color", r->package_color, k);	// updates k
        read_color(st, "fragment_color", r->fragment_color, k);	// updates k
        read_color(st, "subject_color", r->subject_color, k);	// updates k

        if (read_file_format() >= 52)
            read_color(st, "class_color", r->class_color, k);	// updates k
        else
            r->class_color = UmlDefaultColor;

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

BrowserNode * BrowserUseCaseDiagram::get_it(const char * k, int id)
{
    return (!strcmp(k, "usecasediagram_ref")) ? all[id] : 0;
}
