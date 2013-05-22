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
#include <q3painter.h>
#include <q3ptrdict.h>
//Added by qt3to4:
#include <QTextStream>
#include <QDropEvent>
#include <QPixmap>

#include "BrowserTransition.h"
#include "TransitionData.h"
#include "ReferenceDialog.h"
#include "UmlPixmap.h"
#include "UmlGlobal.h"
#include "myio.h"
#include "ToolCom.h"
#include "Tool.h"
#include "AType.h"
#include "ui/menufactory.h"
#include "strutil.h"
#include "ProfiledStereotypes.h"
#include "mu.h"
#include "BrowserStateDiagram.h"
#include "translate.h"

IdDict<BrowserTransition> BrowserTransition::all(1021, __FILE__);

BrowserTransition::BrowserTransition(BrowserNode * p, BrowserNode * end)
    : BrowserNode("<transition>", p), Labeled<BrowserTransition>(all, 0),
      def(new TransitionData)
{
    def->set_start_end(this, end);
}

BrowserTransition::BrowserTransition(BrowserNode * p, TransitionData * d, int id)
    : BrowserNode("<transition>", p), Labeled<BrowserTransition>(all, id), def(d)
{
}

BrowserTransition::BrowserTransition(const BrowserTransition * model,
                                     BrowserNode * p)
    : BrowserNode(model->name, p),
      Labeled<BrowserTransition>(all, 0)
{
    def = new TransitionData(model, this);
    comment = model->comment;
}

BrowserTransition::BrowserTransition(int id)
    : BrowserNode(), Labeled<BrowserTransition>(all, id)
{
    // was deleted but still referenced
    // create a pseudo definition
    def = new TransitionData();

    def->set_start_end(this, this);
}

BrowserTransition::~BrowserTransition()
{
    all.remove(get_ident());

    if (def)
        delete def;
}

BrowserNode * BrowserTransition::duplicate(BrowserNode * p, QString)
{
    BrowserTransition * result = new BrowserTransition(this, p);

    result->def->get_start()->modified();
    result->def->get_start()->package_modified();
    result->update_stereotype();

    return result;
}

bool BrowserTransition::undelete(bool, QString & warning, QString & renamed)
{
    if (! deletedp())
        return FALSE;

    if (def->get_start_node()->deletedp() ||
        def->get_end_node()->deletedp()) {
        warning += QString("<li><b>") + quote(name) + "</b> " + TR("from") + " <b>" +
                   def->get_start_node()->full_name() +
                   "</b> " + TR("to") + " <b>" + def->get_end_node()->full_name() + "</b>\n";
        return FALSE;
    }

    is_deleted = FALSE;
    def->undelete(warning, renamed);

    package_modified();
    repaint();

    return TRUE;
}

void BrowserTransition::referenced_by(QList<BrowserNode *> & l, bool ondelete)
{
    BrowserNode::referenced_by(l, ondelete);

    if (! ondelete)
        BrowserStateDiagram::compute_referenced_by(l, this, "transitioncanvas", "transition_ref");
}

void BrowserTransition::compute_referenced_by(QList<BrowserNode *> & l,
        BrowserNode * target)
{
    IdIterator<BrowserTransition> it(all);

    while (it.current()) {
        if (!it.current()->deletedp() &&
            (it.current()->def->get_end_node() == target))
            l.append(it.current());

        ++it;
    }
}

void BrowserTransition::clear(bool old)
{
    all.clear(old);
}

void BrowserTransition::update_idmax_for_root()
{
    all.update_idmax_for_root();
}

void BrowserTransition::prepare_update_lib() const
{
    all.memo_id_oid(get_ident(), original_id);
}

void BrowserTransition::renumber(int phase)
{
    if (phase != -1)
        new_ident(phase, all);
}

void BrowserTransition::update_stereotype(bool)
{
    if (def != 0) {
        QString n = name;
        const char * stereotype = def->get_stereotype();

        if (n.isEmpty())
            n = "<transition>";

        if (show_stereotypes && stereotype[0]) {
            QString s = toUnicode(stereotype);
            int index = s.find(':');

            setText(0,
                    "<<" + ((index == -1) ? s : s.mid(index + 1))
                    + ">> " + n);
        }
        else
            setText(0, n);
    }
}

const QPixmap * BrowserTransition::pixmap(int) const
{
    if (deletedp())
        return DeletedRelationIcon;

    const QPixmap * px = ProfiledStereotypes::browserPixmap(def->get_stereotype());

    return (px != 0) ? px : SimpleRelationIcon;
}

QString BrowserTransition::str(bool horiz, DrawingLanguage lg) const
{
    return def->str(horiz, lg);
}

void BrowserTransition::menu()
{
    Q3PopupMenu m(0, "transition");
    Q3PopupMenu toolm(0);

    MenuFactory::createTitle(m, def->definition(FALSE, TRUE));
    m.insertSeparator();

    if (!deletedp()) {
        if (!in_edition()) {
            m.setWhatsThis(m.insertItem(TR("Edit"), 0),
                           TR("to edit the <i>transition</i>, \
a double click with the left mouse button does the same thing"));

            if (!is_read_only && (edition_number == 0)) {
                m.setWhatsThis(m.insertItem(TR("Delete"), 2),
                               TR("to delete the <i>transition</i>. \
Note that you can undelete it after"));
            }

            m.insertSeparator();
        }

        QString s = def->get_end_node()->get_name();

        if (s.isEmpty())
            s = stringify(def->get_end_node()->get_type());

        m.setWhatsThis(m.insertItem(TR("Select ") + s, 7),
                       TR("to select the destination"));
        m.setWhatsThis(m.insertItem(TR("Referenced by"), 4),
                       TR("to know who reference the <i>transition</i>"));
        mark_menu(m, TR("the transition"), 90);
        ProfiledStereotypes::menu(m, this, 99990);

        if ((edition_number == 0)
            && Tool::menu_insert(&toolm, get_type(), 100)) {
            m.insertSeparator();
            m.insertItem(TR("Tool"), &toolm);
        }
    }
    else if (!is_read_only && (edition_number == 0)) {
        m.setWhatsThis(m.insertItem(TR("Undelete"), 3),
                       TR("undelete the <i>transition</i> \
(except if the other side is also deleted)"));

        if (def->get_start_node()->deletedp() ||
            def->get_end_node()->deletedp())
            m.setItemEnabled(3, FALSE);
    }

    exec_menu_choice(m.exec(QCursor::pos()));
}

void BrowserTransition::exec_menu_choice(int rank)
{
    switch (rank) {
    case 0:
        open(TRUE);
        break;

    case 2:
        delete_it();
        break;

    case 3:
        BrowserNode::undelete(FALSE);
        break;

    case 4:
        ReferenceDialog::show(this);
        return;

    case 7:
        def->get_end_node()->select_in_browser();
        return;

    default:
        if (rank >= 99990)
            ProfiledStereotypes::choiceManagement(this, rank - 99990);
        else if (rank >= 100)
            ToolCom::run(Tool::command(rank - 100), this);
        else
            mark_management(rank - 90);

        return;
    }

    ((BrowserNode *) parent())->modified();
    package_modified();
}

void BrowserTransition::apply_shortcut(QString s)
{
    int choice = -1;

    if (!deletedp()) {
        if (!in_edition()) {
            if (s == "Edit")
                choice = 0;

            if (!is_read_only && (edition_number == 0)) {
                if (s == "Delete")
                    choice = 2;
            }
        }

        if (s == "Referenced by")
            choice = 4;
        else if (s == "Select target")
            choice = 7;
        else
            mark_shortcut(s, choice, 90);

        if (edition_number == 0)
            Tool::shortcut(s, choice, get_type(), 100);
    }
    else if (!is_read_only && (edition_number == 0))
        if (s == "Undelete")
            choice = 3;

    exec_menu_choice(choice);
}

bool BrowserTransition::in_edition() const
{
    return (def->get_start()->is_edited);
}

void BrowserTransition::open(bool)
{
    if (! in_edition())
        def->edit();
}

void BrowserTransition::modified()
{
    repaint();
    ((BrowserNode *) parent())->modified();
}

UmlCode BrowserTransition::get_type() const
{
    return UmlTransition;
}

QString BrowserTransition::get_stype() const
{
    return TR("transition");
}

int BrowserTransition::get_identifier() const
{
    return get_ident();
}

BasicData * BrowserTransition::get_data() const
{
    return def;
}

QString BrowserTransition::full_name(bool rev, bool) const
{
    QString s = name;

    if (s.isEmpty())
        s = "<anonymous transition>";

    return fullname(s, rev);
}

bool BrowserTransition::allow_empty() const
{
    return TRUE;
}

bool BrowserTransition::same_name(const QString & s, UmlCode type) const
{
    return ((get_type() == type) &&
            (name == s) &&
            (name != "<transition>"));
}

bool BrowserTransition::tool_cmd(ToolCom * com, const char * args)
{
    return (def->tool_cmd(com, args, this, comment) ||
            BrowserNode::tool_cmd(com, args));
}

void BrowserTransition::DropAfterEvent(QDropEvent * e, BrowserNode * after)
{
    if (after == 0)
        ((BrowserNode *) parent())->DropAfterEvent(e, this);
    else
        e->ignore();
}

QString BrowserTransition::drag_key() const
{
    return QString::number(UmlTransition)
           + "#" + QString::number((unsigned long) parent());
}

QString BrowserTransition::drag_postfix() const
{
    return "#" + QString::number((unsigned long) parent());
}

QString BrowserTransition::drag_key(BrowserNode * p)
{
    return QString::number(UmlTransition)
           + "#" + QString::number((unsigned long) p);
}

void BrowserTransition::save(QTextStream & st, bool ref,
                             QString & warning)
{
    if (ref) {
        // for TransitionCanvas
        st << "transition_ref " << get_ident() << " // " << get_name();
    }
    else {
        nl_indent(st);
        st << "transition " << get_ident() << " ";
        save_string(name, st);
        indent(+1);
        def->save(st, warning);
        BrowserNode::save(st);

        if (! def->internal()) {
            nl_indent(st);
            st << "external ";
        }

        indent(-1);
        nl_indent(st);
        st << "end";

        // for saveAs
        if (!is_from_lib() && !is_api_base())
            is_read_only = FALSE;
    }
}

BrowserTransition * BrowserTransition::read_ref(char *& st)
{
    read_keyword(st, "transition_ref");

    int id = read_id(st);
    BrowserTransition * result = all[id];

    return (result == 0)
           ? new BrowserTransition(id)
           : result;
}

BrowserTransition *
BrowserTransition::read(char *& st, char * k,
                        BrowserNode * parent)
{
    if (!strcmp(k, "transition_ref")) {
        int id = read_id(st);
        BrowserTransition * result = all[id];

        return (result == 0)
               ? new BrowserTransition(id)
               : result;
    }
    else if (!strcmp(k, "transition")) {
        int id = read_id(st);
        QString s = read_string(st);

        k = read_keyword(st);

        TransitionData * d = TransitionData::read(st, k);	// update k
        BrowserTransition * result;

        if ((result = all[id]) == 0)
            result = new BrowserTransition(parent, d, id);
        else if (result->is_defined) {
            BrowserTransition * already_exist = result;

            result = new BrowserTransition(parent, d, id);

            already_exist->must_change_id(all);
            already_exist->unconsistent_fixed("transition", result);
        }
        else {
            if (result->def != 0)
                // re-load ???
                delete result->def;

            result->def = d;
            result->set_parent(parent);
        }

        result->is_defined = TRUE;
        result->set_name(s);

        result->is_read_only = (!in_import() && read_only_file()) ||
                               ((user_id() != 0) && result->is_api_base());

        result->BrowserNode::read(st, k, id);
        d->set_browser_node(result);	// call update_stereotype();

        if (strcmp(k, "external") == 0) {
            d->set_internal(FALSE);
            k = read_keyword(st);
        }
        else
            d->set_internal(parent == d->get_end_node());

        if (strcmp(k, "end"))
            wrong_keyword(k, "end");

        return result;
    }
    else
        return 0;
}

BrowserNode * BrowserTransition::get_it(const char * k, int id)
{
    return (!strcmp(k, "transition_ref")) ? all[id] : 0;
}
