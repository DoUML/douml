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
#include <q3painter.h>
#include <qcursor.h>
//Added by qt3to4:
#include <QTextStream>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QPixmap>

#include "BrowserStateAction.h"
#include "BrowserTransition.h"
#include "StateActionData.h"
#include "BrowserState.h"
#include "TransitionData.h"
#include "ReferenceDialog.h"
#include "UmlDrag.h"
#include "UmlPixmap.h"
#include "StateActionDialog.h"
#include "myio.h"
#include "ToolCom.h"
#include "Tool.h"
#include "ui/menufactory.h"
#include "strutil.h"
#include "DialogUtil.h"
#include "ProfiledStereotypes.h"
#include "mu.h"
#include "BrowserStateDiagram.h"
#include "translate.h"

IdDict<BrowserStateAction> BrowserStateAction::all(257, __FILE__);
QStringList BrowserStateAction::its_default_stereotypes;	// unicode

BrowserStateAction::BrowserStateAction(BrowserNode * p,
                                       StateActionData * d, int id)
    : BrowserNode("", p), Labeled<BrowserStateAction>(all, id), def(d)
{
    def->set_browser_node(this);
}

BrowserStateAction::BrowserStateAction(int id)
    : BrowserNode(), Labeled<BrowserStateAction>(all, id),
      def(new StateActionData)
{
    // not yet read
    def->set_browser_node(this);
}

BrowserStateAction::BrowserStateAction(const BrowserStateAction * model, BrowserNode * p)
    : BrowserNode("", p), Labeled<BrowserStateAction>(all, 0)
{
    def = new StateActionData(model->def, this);
    comment = model->comment;
}

BrowserNode * BrowserStateAction::duplicate(BrowserNode * p, QString name)
{
    BrowserNode * result = new BrowserStateAction(this, p);

    result->set_name((name.isEmpty()) ? get_name() : name);
    result->update_stereotype();

    return result;
}

BrowserStateAction::~BrowserStateAction()
{
    all.remove(get_ident());
    delete def;
}

void BrowserStateAction::clear(bool old)
{
    all.clear(old);
}

void BrowserStateAction::update_idmax_for_root()
{
    all.update_idmax_for_root();
}

void BrowserStateAction::prepare_update_lib() const
{
    all.memo_id_oid(get_ident(), original_id);

    for (Q3ListViewItem * child = firstChild();
         child != 0;
         child = child->nextSibling())
        ((BrowserNode *) child)->prepare_update_lib();
}

void BrowserStateAction::referenced_by(QList<BrowserNode *> & l, bool ondelete)
{
    BrowserNode::referenced_by(l, ondelete);
    BrowserTransition::compute_referenced_by(l, this);

    if (! ondelete)
        BrowserStateDiagram::compute_referenced_by(l, this, "stateactioncanvas", "stateaction_ref");
}

void BrowserStateAction::renumber(int phase)
{
    if (phase != -1)
        new_ident(phase, all);
}

const QPixmap * BrowserStateAction::pixmap(int) const
{
    const char * st = def->get_stereotype();

    if (! deletedp()) {
        const QPixmap * px = ProfiledStereotypes::browserPixmap(st);

        if (px != 0)
            return px;
    }

    if (!strcmp(st, "receive-signal")) {
        if (deletedp())
            return DeletedSignalInIcon;
        else if (is_marked)
            return SignalInMarkedIcon;
        else
            return SignalInIcon;
    }
    else if (!strcmp(st, "send-signal")) {
        if (deletedp())
            return DeletedSignalOutIcon;
        else if (is_marked)
            return SignalOutMarkedIcon;
        else
            return SignalOutIcon;
    }
    else if (deletedp())
        return DeletedActionIcon;
    else if (is_marked)
        return ActionMarkedIcon;
    else
        return ActionIcon;
}

void BrowserStateAction::iconChanged()
{
    repaint();
    def->modified();
}

BrowserTransition * BrowserStateAction::add_transition(BrowserNode * end)
{
    BrowserTransition * result = new BrowserTransition(this, end);

    modified();
    return result;
}

BasicData * BrowserStateAction::add_relation(UmlCode, BrowserNode * end)
{
    return add_transition(end)->get_data();
}

BrowserStateAction *
BrowserStateAction::add_stateaction(BrowserNode * future_parent)
{
    StateActionData * d = new StateActionData();
    BrowserStateAction * r =
        new BrowserStateAction(future_parent, d);

    future_parent->setOpen(TRUE);
    future_parent->package_modified();

    return r;
}

void BrowserStateAction::menu()
{
    QString s;
    const char * st = def->get_stereotype();

    if (!strcmp(st, "receive-signal"))
        s = "receive-signal action";
    else if (!strcmp(st, "send-signal"))
        s = "send-signal action";
    else
        s = "action";

    Q3PopupMenu m(0, "action");
    Q3PopupMenu toolm(0);

    MenuFactory::createTitle(m, def->definition(FALSE, TRUE));
    m.insertSeparator();

    if (!deletedp()) {
        m.setWhatsThis(m.insertItem(TR("Edit"), 1),
                       TR("to edit the <i>" + s + "</i>, \
a double click with the left mouse button does the same thing"));

        if (!is_read_only) {
            m.setWhatsThis(m.insertItem(TR("Duplicate"), 2),
                           TR("to copy the <i>" + s + "</i> in a new one"));
            m.insertSeparator();

            if (edition_number == 0)
                m.setWhatsThis(m.insertItem(TR("Delete"), 3),
                               TR("to delete the <i>" + s + "</i>. \
Note that you can undelete it after"));
        }

        m.setWhatsThis(m.insertItem(TR("Referenced by"), 5),
                       TR("to know who reference the <i>" + s + "</i> \
through a transition"));
        mark_menu(m, TR("the " + s), 90);
        ProfiledStereotypes::menu(m, this, 99990);

        if ((edition_number == 0) &&
            Tool::menu_insert(&toolm, get_type(), 100)) {
            m.insertSeparator();
            m.insertItem(TR("Tool"), &toolm);
        }
    }
    else if (!is_read_only && (edition_number == 0)) {
        m.setWhatsThis(m.insertItem(TR("Undelete"), 4),
                       TR("to undelete the <i>" + s + "</i>"));
    }

    exec_menu_choice(m.exec(QCursor::pos()));
}

void BrowserStateAction::exec_menu_choice(int rank)
{
    switch (rank) {
    case 1:
        open(TRUE);
        return;

    case 2:
        duplicate((BrowserNode *) parent())->select_in_browser();
        break;

    case 3:
        delete_it();
        break;

    case 4:
        BrowserNode::undelete(FALSE);
        break;

    case 5:
        ReferenceDialog::show(this);
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

void BrowserStateAction::apply_shortcut(QString s)
{
    int choice = -1;

    if (!deletedp()) {
        if (s == "Edit")
            choice = 1;

        if (!is_read_only) {
            if (s == "Duplicate")
                choice = 2;

            if (edition_number == 0)
                if (s == "Delete")
                    choice = 3;
        }

        if (s == "Referenced by")
            choice = 5;

        mark_shortcut(s, choice, 90);

        if (edition_number == 0)
            Tool::shortcut(s, choice, get_type(), 100);
    }
    else if (!is_read_only && (edition_number == 0)) {
        if (s == "Undelete")
            choice = 4;
    }

    exec_menu_choice(choice);
}

void BrowserStateAction::open(bool)
{
    if (!is_edited) {
        (new StateActionDialog(def))->show();
    }
}

void BrowserStateAction::modified()
{
    repaint();
    ((BrowserNode *) parent())->modified();
    // to update drawing
    def->modified();
}

UmlCode BrowserStateAction::get_type() const
{
    return UmlStateAction;
}

QString BrowserStateAction::get_stype() const
{
    const char * st = def->get_stereotype();

    if (!strcmp(st, "receive-signal"))
        return TR("receive-signal action");
    else if (!strcmp(st, "send-signal"))
        return TR("send-signal action");
    else
        return TR("state action");
}

int BrowserStateAction::get_identifier() const
{
    return get_ident();
}

BasicData * BrowserStateAction::get_data() const
{
    return def;
}

const char * BrowserStateAction::help_topic() const
{
    return "stateaction";
}

bool BrowserStateAction::allow_empty() const
{
    return TRUE;
}

QString BrowserStateAction::full_name(bool rev, bool) const
{
    QString s = name;

    if (s.isEmpty()) {
        s = def->get_stereotype();

        if (s.isEmpty())
            s = "<anonymous state action>";
        else
            s = "<anonymous " + s + ">";
    }

    return fullname(s, rev);
}

void BrowserStateAction::init()
{
    its_default_stereotypes.clear();
    its_default_stereotypes.append("send-signal");
    its_default_stereotypes.append("receive-signal");
}

// unicode
const QStringList & BrowserStateAction::default_stereotypes()
{
    return its_default_stereotypes;
}

bool BrowserStateAction::tool_cmd(ToolCom * com, const char * args)
{
    switch ((unsigned char) args[-1]) {
    case createCmd: {
        bool ok = TRUE;

        if (is_read_only && !root_permission())
            ok = FALSE;
        else {
            UmlCode k = com->get_kind(args);

            switch (k) {
            case UmlTransition: {
                BrowserNode * end = (BrowserNode *) com->get_id(args);
                k = end->get_type();

                if ((k == UmlState) ||
                    (k == UmlStateAction) ||
                    IsaPseudoState(k))
                    (add_transition(end))->write_id(com);
                else
                    ok = FALSE;
            }
            break;

            default:
                ok = FALSE;
            }
        }

        if (! ok)
            com->write_id(0);
        else
            package_modified();

        return TRUE;
    }

    default:
        return (def->tool_cmd(com, args, this, comment) ||
                BrowserNode::tool_cmd(com, args));
    }
}

void BrowserStateAction::DragMoveEvent(QDragMoveEvent * e)
{
    if (UmlDrag::canDecode(e, BrowserTransition::drag_key(this))) {
        if (!is_read_only)
            e->accept();
        else
            e->ignore();
    }
    else
        ((BrowserNode *) parent())->DragMoveInsideEvent(e);
}

void BrowserStateAction::DropEvent(QDropEvent * e)
{
    DropAfterEvent(e, 0);
}

void BrowserStateAction::DragMoveInsideEvent(QDragMoveEvent * e)
{
    if (!is_read_only &&
        UmlDrag::canDecode(e, BrowserTransition::drag_key(this)))
        e->accept();
    else
        e->ignore();
}

void BrowserStateAction::DropAfterEvent(QDropEvent * e, BrowserNode * after)
{
    BrowserNode * bn;

    if ((((bn = UmlDrag::decode(e, BrowserTransition::drag_key(this))) != 0)) &&
        (bn != after) && (bn != this)) {
        if (may_contains(bn, FALSE))
            move(bn, after);
        else {
            msg_critical(TR("Error"), TR("Forbidden"));
            e->ignore();
        }
    }
    else if ((bn == 0) && (after == 0))
        ((BrowserNode *) parent())->DropAfterEvent(e, this);
    else
        e->ignore();
}

QString BrowserStateAction::drag_key() const
{
    return QString::number(UmlStateAction)
           + "#" + QString::number((unsigned long) BrowserState::get_machine(this));
}

QString BrowserStateAction::drag_postfix() const
{
    return "#" + QString::number((unsigned long) BrowserState::get_machine(this));
}

QString BrowserStateAction::drag_key(BrowserNode * p)
{
    return QString::number(UmlStateAction)
           + "#" + QString::number((unsigned long) BrowserState::get_machine(p));
}

void BrowserStateAction::save_stereotypes(QTextStream & st)
{
    nl_indent(st);
    st << "stateaction_stereotypes ";
    save_unicode_string_list(its_default_stereotypes, st);
}

void BrowserStateAction::read_stereotypes(char *& st, char *& k)
{
    if (!strcmp(k, "stateaction_stereotypes")) {
        read_unicode_string_list(its_default_stereotypes, st);
        k = read_keyword(st);
    }
    else
        init();
}

void BrowserStateAction::save(QTextStream & st, bool ref, QString & warning)
{
    if (ref)
        st << "stateaction_ref " << get_ident();
    else {
        nl_indent(st);
        st << "stateaction " << get_ident();
        indent(+1);
        def->save(st, warning);
        BrowserNode::save(st);

        // saves the sub elts

        Q3ListViewItem * child = firstChild();

        if (child != 0) {
            for (;;) {
                if (!((BrowserNode *) child)->deletedp()) {
                    ((BrowserNode *) child)->save(st, FALSE, warning);

                    child = child->nextSibling();

                    if (child != 0)
                        st << '\n';
                    else
                        break;
                }
                else if ((child = child->nextSibling()) == 0)
                    break;
            }
        }

        indent(-1);
        nl_indent(st);
        st << "end";

        // for saveAs
        if (!is_from_lib() && !is_api_base())
            is_read_only = FALSE;
    }
}

BrowserStateAction * BrowserStateAction::read_ref(char *& st)
{
    read_keyword(st, "stateaction_ref");

    int id = read_id(st);
    BrowserStateAction * result = all[id];

    return (result == 0)
           ? new BrowserStateAction(id)
           : result;
}

BrowserStateAction * BrowserStateAction::read(char *& st, char * k,
        BrowserNode * parent)
{
    BrowserStateAction * result;
    int id;

    if (!strcmp(k, "stateaction_ref")) {
        id = read_id(st);
        result = all[id];

        return (result == 0)
               ? new BrowserStateAction(id)
               : result;
    }
    else if (!strcmp(k, "stateaction")) {
        id = read_id(st);
        result = all[id];

        BrowserStateAction * already_exist = 0;

        if (result == 0)
            result = new BrowserStateAction(parent, new StateActionData, id);
        else if (result->is_defined) {
            already_exist = result;
            result = new BrowserStateAction(parent, new StateActionData, id);
        }
        else
            result->set_parent(parent);

        k = read_keyword(st);
        result->def->read(st, k);

        if (already_exist != 0) {
            already_exist->must_change_id(all);
            already_exist->unconsistent_fixed("state action", result);
        }

        result->is_defined = TRUE;
        result->BrowserNode::read(st, k, id);

        result->is_read_only = (!in_import() && read_only_file()) ||
                               ((user_id() != 0) && result->is_api_base());

        if (strcmp(k, "end")) {
            while (BrowserTransition::read(st, k, result))
                k = read_keyword(st);

            if (strcmp(k, "end"))
                wrong_keyword(k, "end");
        }

        return result;
    }
    else
        return 0;
}

BrowserNode * BrowserStateAction::get_it(const char * k, int id)
{
    // transition managed in state
    return (!strcmp(k, "stateaction_ref")) ? all[id] : 0;
}
