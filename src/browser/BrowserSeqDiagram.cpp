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

#include "BrowserSeqDiagram.h"
#include "SimpleData.h"
#include "SeqDiagramWindow.h"
#include "SeqDiagramView.h"
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

QList<BrowserSeqDiagram *> BrowserSeqDiagram::imported;
Q3ValueList<int> BrowserSeqDiagram::imported_ids;
QStringList BrowserSeqDiagram::its_default_stereotypes;	// unicode
QStringList BrowserSeqDiagram::message_default_stereotypes;	// unicode


BrowserSeqDiagram::BrowserSeqDiagram(QString s, BrowserNode * p, int id)
    : BrowserDiagram(s, p, id), window(0), used_settings(0)
{
    make();
    is_modified = (id == 0);
}

BrowserSeqDiagram::BrowserSeqDiagram(int id)
    : BrowserDiagram(id), window(0), used_settings(0)
{
    // not yet read
    make();
    is_modified = (id == 0);
}

BrowserSeqDiagram::BrowserSeqDiagram(BrowserSeqDiagram * model, BrowserNode * p)
    : BrowserDiagram(p->get_name(), p, 0), window(0), used_settings(0)
{
    def = new SimpleData(model->def);
    def->set_browser_node(this);
    comment = model->comment;
    settings = model->settings;
    note_color = model->note_color;
    fragment_color = model->fragment_color;
    duration_color = model->duration_color;
    continuation_color = model->continuation_color;
    class_instance_color = model->class_instance_color;
    overlapping_bars = model->overlapping_bars;
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

BrowserSeqDiagram::~BrowserSeqDiagram()
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

void BrowserSeqDiagram::make()
{
    def = new SimpleData;
    def->set_browser_node(this);
    note_color = UmlDefaultColor;
    fragment_color = UmlDefaultColor;
    duration_color = UmlDefaultColor;
    continuation_color = UmlDefaultColor;
    class_instance_color = UmlDefaultColor;
    overlapping_bars = TRUE;
}

BrowserSeqDiagram * BrowserSeqDiagram::add_sequence_diagram(BrowserNode * future_parent)
{
    QString name;

    if (future_parent->enter_child_name(name, TR("enter sequence diagram's name : "),
                                        UmlSeqDiagram, TRUE, FALSE))
        return new BrowserSeqDiagram(name, future_parent);
    else
        return 0;
}

void BrowserSeqDiagram::set_name(const char * s)
{
    BrowserDiagram::set_name(s);

    if (window != 0)
        window->setCaption(s);
}

void BrowserSeqDiagram::import()
{
    Q3ValueList<int>::Iterator it = imported_ids.begin();

    foreach (BrowserSeqDiagram *d, imported) {
        (new SeqDiagramWindow(d->full_name(), d, *it))->close(TRUE);
        it = imported_ids.remove(it);
        d->is_modified = TRUE;
    }
    imported.clear();
}

void BrowserSeqDiagram::renumber(int phase)
{
    if (phase == -1)
        open(FALSE);
    else {
        new_ident(phase, all);
        window->get_view()->renumber(get_ident());
        is_modified = TRUE;
    }
}

void BrowserSeqDiagram::delete_it()
{
    if (window)
        delete window;

    BrowserNode::delete_it();
}

BrowserNode * BrowserSeqDiagram::duplicate(BrowserNode * p, QString name)
{
    BrowserSeqDiagram * result = new BrowserSeqDiagram(this, p);

    result->set_name((name.isEmpty()) ? get_name() : name);
    result->update_stereotype();

    return result;
}

const QPixmap * BrowserSeqDiagram::pixmap(int) const
{
    if (deletedp())
        return DeletedSeqDiagramIcon;

    const QPixmap * px = ProfiledStereotypes::browserPixmap(def->get_stereotype());

    return (px != 0) ? px : SeqDiagramIcon;
}

void BrowserSeqDiagram::draw_svg() const
{
    fputs("\t<rect fill=\"white\" stroke=\"black\" stroke-width=\"1\" stroke-opacity=\"1\" x=\"0\" y=\"0\" width=\"15\" height=\"15\" />\n"
          "\t<path fill=\"none\" stroke=\"black\" stroke-width=\"1\" stroke-opacity=\"1\" "
          "d=\"M 1 1 L 5 1 L 5 4 L 1 4 L 1 1 "
          "M 9 1 L 13 1 L 13 4 L 9 4 L 9 1 "
          "M 2 6 L 4 6 L 4 12 L 2 12 L 2 6 "
          "M 10 6 L 12 6 L 12 9 L 10 9 L 10 6 "
          "M 3 4 L 3 6 "
          "M 11 4 L 11 6 "
          "M 11 9 L 11 12 "
          "M 7 5 L 9 7 L 7 9 "
          "M 3 14 L 3 15 "
          "M 11 14 L 11 15 "
          "M 4 7 L 9 7 "
          "M 4 11 L 15 11\" />\n",
          svg());
}

void BrowserSeqDiagram::menu()
{
    Q3PopupMenu m(0, name);
    Q3PopupMenu toolm(0);

    MenuFactory::createTitle(m, def->definition(FALSE, TRUE));
    m.insertSeparator();

    if (!deletedp()) {
        m.setWhatsThis(m.insertItem(TR("Show"), 0),
                       TR("to show and edit the <i>sequence diagram</i>"));

        if (!is_edited) {
            m.setWhatsThis(m.insertItem(TR("Edit"), 1),
                           TR("to edit the <i>sequence diagram</i>"));

            if (!is_read_only) {
                m.setWhatsThis(m.insertItem(TR("Edit drawing settings"), 2),
                               TR("to set how the <i>sequence diagram</i>'s items must be drawn"));
                m.insertSeparator();
                m.setWhatsThis(m.insertItem(TR("Duplicate"), 3),
                               TR("to duplicate the <i>sequence diagram</i>"));

                if (edition_number == 0) {
                    m.insertSeparator();
                    m.setWhatsThis(m.insertItem(TR("Delete"), 4),
                                   TR("to delete the <i>sequence diagram</i>. \
Note that you can undelete it after"));
                }
            }
        }

        mark_menu(m, TR("the sequence diagram"), 90);
        ProfiledStereotypes::menu(m, this, 99990);

        if ((edition_number == 0) &&
            Tool::menu_insert(&toolm, get_type(), 100)) {
            m.insertSeparator();
            m.insertItem(TR("Tool"), &toolm);
        }
    }
    else if (!is_read_only && (edition_number == 0))
        m.setWhatsThis(m.insertItem(TR("Undelete"), 5),
                       TR("to undelete the <i>sequence diagram</i>"));

    exec_menu_choice(m.exec(QCursor::pos()));
}

void BrowserSeqDiagram::exec_menu_choice(int rank)
{
    switch (rank) {
    case 0:
        open(FALSE);
        break;

    case 1:
        edit(TR("Sequence diagram"), its_default_stereotypes);
        return;

    case 2:
        edit_settings();
        return;

    case 3: {
        QString name;

        if (((BrowserNode *)parent())->enter_child_name(name, TR("enter sequence diagram's name : "),
                UmlSeqDiagram, TRUE, FALSE))
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

void BrowserSeqDiagram::apply_shortcut(QString s)
{
    int choice = -1;

    if (!deletedp()) {
        if (s == "Show")
            choice = 0;

        if (!is_edited) {
            if (s == "Edit")
                choice = 1;
            else if (s == "Edit drawing settings")
                choice = 2;

            if (!is_read_only) {
                if (s == "Duplicate")
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

void BrowserSeqDiagram::open(bool)
{
    if (window == 0 || windowDestroyed)
        window = new SeqDiagramWindow(full_name(TRUE), this);
    else
        window->raise();

    window->setFocus();
}

void BrowserSeqDiagram::edit_settings()
{
    for (;;) {
        StateSpecVector st;
        ColorSpecVector co(5);

        settings.complete(st, TRUE);

        co[0].set(TR("note color"), &note_color);
        co[1].set(TR("class instance \ncolor"), &class_instance_color);
        co[2].set(TR("duration color"), &duration_color);
        co[3].set(TR("continuation color"), &continuation_color);
        co[4].set(TR("fragment color"), &fragment_color);

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

void BrowserSeqDiagram::on_close()
{
    window = 0;

    if (used_settings != 0) {
        delete used_settings;
        used_settings = 0;
    }
}

void BrowserSeqDiagram::read_session(char *& st)
{
    window->read_session(st);
}

UmlCode BrowserSeqDiagram::get_type() const
{
    return UmlSeqDiagram;
}

QString BrowserSeqDiagram::get_stype() const
{
    return TR("Sequence diagram");
}

int BrowserSeqDiagram::get_identifier() const
{
    return get_ident();
}

const char * BrowserSeqDiagram::help_topic() const
{
    return "sequencediagram";
}

void BrowserSeqDiagram::update_drawing_settings()
{
    if (used_settings == 0)
        used_settings = new SequenceDiagramSettings;

    *used_settings = settings;
    ((BrowserNode *) parent())->get_sequencediagramsettings(*used_settings);
}

void BrowserSeqDiagram::get_sequencediagramsettings(SequenceDiagramSettings & r) const
{
    r.assign(*used_settings);
}

void BrowserSeqDiagram::get_sequencediagramsettings_msg(SequenceDiagramSettings & r) const
{
    r.assign_msg(*used_settings);
}

UmlColor BrowserSeqDiagram::get_color(UmlCode who) const
{
    UmlColor c;

    switch (who) {
    case UmlNote:
        c = note_color;
        break;

    case UmlClass:
        c = class_instance_color;
        break;

    case UmlFragment:
        c = fragment_color;
        break;

    case UmlContinuation:
        c = continuation_color;
        break;

    default:
        c = duration_color;
    }

    return (c != UmlDefaultColor)
           ? c
           : ((BrowserNode *) parent())->get_color(who);
}

bool BrowserSeqDiagram::get_shadow() const
{
    return used_settings->shadow == UmlYes;
}

bool BrowserSeqDiagram::get_auto_label_position() const
{
    // never called
    return FALSE;
}

bool BrowserSeqDiagram::get_draw_all_relations() const
{
    return used_settings->draw_all_relations == UmlYes;
}

void BrowserSeqDiagram::dont_draw_all_relations()
{
    settings.draw_all_relations =
        used_settings->draw_all_relations = UmlNo;
}

bool BrowserSeqDiagram::get_show_stereotype_properties() const
{
    return used_settings->show_stereotype_properties == UmlYes;
}

bool BrowserSeqDiagram::get_classinstwritehorizontally() const
{
    return used_settings->write_horizontally == UmlYes;
}

ShowContextMode BrowserSeqDiagram::get_classinstshowmode() const
{
    return used_settings->show_class_context_mode;
}

BasicData * BrowserSeqDiagram::get_data() const
{
    return def;
}

bool BrowserSeqDiagram::tool_cmd(ToolCom * com, const char * args)
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
            SeqDiagramWindow * w = new SeqDiagramWindow(full_name(), this);

            com->write_ack(w->get_view()->save_pict(args,
                                                    !w->get_view()->has_preferred_size_zoom(),
                                                    TRUE));
            w->dont_save();
            w->close(TRUE);
        }

        return TRUE;

    case sideCmd:
        if (window != 0)
            ((SeqDiagramView *) window->get_view())->send(com);
        else {
            SeqDiagramWindow * w = new SeqDiagramWindow(full_name(), this);

            ((SeqDiagramView *) w->get_view())->send(com);
            w->dont_save();
            w->close(TRUE);
        }

        return TRUE;

    default:
        return (def->tool_cmd(com, args, this, comment) ||
                BrowserNode::tool_cmd(com, args));
    }
}

void BrowserSeqDiagram::compute_referenced_by(QList<BrowserNode *> & l,
        BrowserNode * bn,
        char const * kc,
        char const * kr)
{
    int id = bn->get_identifier();
    IdIterator<BrowserDiagram> it(all);
    BrowserDiagram * d;

    while ((d = it.current()) != 0) {
        if (!d->deletedp() && (d->get_type() == UmlSeqDiagram)) {
            if ((((BrowserSeqDiagram *) d)->window != 0)
                ? ((BrowserSeqDiagram *) d)->window->get_view()->is_present(bn)
                : is_referenced(read_definition(d->get_ident(), "diagram"), id, kc, kr))
                l.append((BrowserSeqDiagram *) d);
        }

        ++it;
    }
}

void BrowserSeqDiagram::save_stereotypes(QTextStream & st)
{
    nl_indent(st);
    st << "seqdiagram_stereotypes ";
    save_unicode_string_list(its_default_stereotypes, st);
    nl_indent(st);
    st << "msg_stereotypes ";
    save_unicode_string_list(message_default_stereotypes, st);
}

void BrowserSeqDiagram::read_stereotypes(char *& st, char *& k)
{
    if (!strcmp(k, "seqdiagram_stereotypes")) {
        read_unicode_string_list(its_default_stereotypes, st);
        k = read_keyword(st);
    }

    if (!strcmp(k, "msg_stereotypes")) {
        read_unicode_string_list(message_default_stereotypes, st);
        k = read_keyword(st);
    }
}

const QStringList & BrowserSeqDiagram::default_stereotypes()
{
    return its_default_stereotypes;
}

const QStringList & BrowserSeqDiagram::msg_default_stereotypes()
{
    return message_default_stereotypes;
}

void BrowserSeqDiagram::save(QTextStream & st, bool ref, QString & warning)
{
    if (ref)
        st << "sequencediagram_ref " << get_ident() << " // " << get_name();
    else {
        nl_indent(st);
        st << "sequencediagram " << get_ident() << " ";
        save_string(name, st);
        indent(+1);
        def->save(st, warning);
        settings.save(st);

        BooL nl = FALSE;

        save_color(st, "duration_color", duration_color, nl);
        save_color(st, "continuation_color", continuation_color, nl);
        save_color(st, "note_color", note_color, nl);
        save_color(st, "class_instance_color", class_instance_color, nl);
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

        if (overlapping_bars)
            st << "overlapping_bars ";

        st << "size " << stringify(get_format());

        indent(-1);
        nl_indent(st);
        st << "end";

        // for saveAs
        if (!is_from_lib() && !is_api_base())
            is_read_only = FALSE;
    }
}

BrowserSeqDiagram * BrowserSeqDiagram::read(char *& st, char * k,
        BrowserNode * parent)
{
    BrowserSeqDiagram * r;
    int id;

    if (!strcmp(k, "sequencediagram_ref")) {
        if ((r = (BrowserSeqDiagram *) all[id = read_id(st)]) == 0)
            r = new BrowserSeqDiagram(id);

        return r;
    }
    else if (!strcmp(k, "sequencediagram")) {
        id = read_id(st);

        QString s = read_string(st);

        if ((r = (BrowserSeqDiagram *) all[id]) == 0)
            r = new BrowserSeqDiagram(s, parent, id);
        else if (r->is_defined) {
            BrowserSeqDiagram * already_exist = r;

            r = new BrowserSeqDiagram(s, parent, id);

            already_exist->must_change_id(all);
            already_exist->unconsistent_fixed("sequence diagram", r);
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

        r->def->read(st, k);				// updates k
        r->settings.read(st, k);				// updates k
        read_color(st, "duration", r->duration_color, k);	// old, updates k
        read_color(st, "duration_color", r->duration_color, k);	 // updates k
        read_color(st, "continuation_color", r->continuation_color, k);	 // updates k
        read_color(st, "note_color", r->note_color, k);	// updates k
        read_color(st, "class_instance", r->class_instance_color, k);// old release, updates k
        read_color(st, "class_instance_color", r->class_instance_color, k);// updates k
        read_color(st, "fragment_color", r->fragment_color, k);	// updates k
        r->BrowserNode::read(st, k, id);			// updates k

        if (!strcmp(k, "overlapping_bars"))
            // constructor set overlapping_bars to TRUE
            k = read_keyword(st);
        else
            r->overlapping_bars = FALSE;

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

BrowserNode * BrowserSeqDiagram::get_it(const char * k, int id)
{
    return (!strcmp(k, "sequencediagram_ref")) ? all[id] : 0;
}
