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
#include <QPixmap>
#include <QDragMoveEvent>
#include <QDropEvent>

#include "BrowserState.h"
#include "StateData.h"
#include "BrowserTransition.h"
#include "BrowserPseudoState.h"
#include "BrowserStateAction.h"
#include "BrowserRegion.h"
#include "BrowserStateDiagram.h"
#include "TransitionData.h"
#include "BrowserActivityAction.h"
#include "ReferenceDialog.h"
#include "UmlDrag.h"
#include "UmlPixmap.h"
#include "myio.h"
#include "ToolCom.h"
#include "Tool.h"
#include "ui/menufactory.h"
#include "strutil.h"
#include "DialogUtil.h"
#include "ProfiledStereotypes.h"
#include "mu.h"
#include "translate.h"

IdDict<BrowserState> BrowserState::all(257, __FILE__);
QStringList BrowserState::its_default_stereotypes;	// unicode

BrowserState::BrowserState(QString s, BrowserNode * p, StateData * d, int id)
    : BrowserNode(s, p), Labeled<BrowserState>(all, id),
      def(d), associated_diagram(0)
{
    def->set_browser_node(this);
}

BrowserState::BrowserState(int id)
    : BrowserNode(), Labeled<BrowserState>(all, id),
      def(new StateData), associated_diagram(0)
{
    // not yet read
    def->set_browser_node(this);
}

BrowserState::BrowserState(const BrowserState * model, BrowserNode * p)
    : BrowserNode(model->name, p), Labeled<BrowserState>(all, 0), associated_diagram(0)
{
    def = new StateData(model->def, this);
    comment = model->comment;
}

BrowserNode * BrowserState::duplicate(BrowserNode * p, QString name)
{
    BrowserNode * result = new BrowserState(this, p);

    result->set_name((name.isEmpty()) ? get_name() : name);
    result->update_stereotype(TRUE);

    return result;
}

BrowserState::~BrowserState()
{
    all.remove(get_ident());
    delete def;
}

void BrowserState::clear(bool old)
{
    all.clear(old);
    BrowserTransition::clear(old);
    BrowserRegion::clear(old);
    BrowserPseudoState::clear(old);
    BrowserStateAction::clear(old);
}

void BrowserState::update_idmax_for_root()
{
    all.update_idmax_for_root();
    BrowserTransition::update_idmax_for_root();
    BrowserRegion::update_idmax_for_root();
    BrowserPseudoState::update_idmax_for_root();
    BrowserStateAction::update_idmax_for_root();
}

void BrowserState::prepare_update_lib() const
{
    all.memo_id_oid(get_ident(), original_id);

    for (Q3ListViewItem * child = firstChild();
         child != 0;
         child = child->nextSibling())
        ((BrowserNode *) child)->prepare_update_lib();
}

void BrowserState::referenced_by(QList<BrowserNode *> & l, bool ondelete)
{
    BrowserNode::referenced_by(l, ondelete);
    BrowserTransition::compute_referenced_by(l, this);

    if (! ondelete) {
        BrowserActivityAction::compute_referenced_by(l, this);
        BrowserStateDiagram::compute_referenced_by(l, this, "statecanvas", "state_ref");

        IdIterator<BrowserState> it(all);

        while (it.current()) {
            if (!it.current()->deletedp() &&
                (it.current()->def->get_reference() == this))
                l.append(it.current());

            ++it;
        }
    }
}

// callers suppose this only take specification into acount
void BrowserState::compute_referenced_by(QList<BrowserNode *> & l,
        BrowserOperation * op)
{
    IdIterator<BrowserState> it(all);

    while (it.current()) {
        if (!it.current()->deletedp() &&
            (it.current()->def->get_specification() == op))
            l.append(it.current());

        ++it;
    }
}

bool BrowserState::is_ref() const
{
    return (def->get_reference() != 0);
}

bool BrowserState::can_reference() const
{
    for (Q3ListViewItem * child = firstChild();
         child != 0;
         child = child->nextSibling()) {
        switch (((BrowserNode *) child)->get_type()) {
        case UmlTransition:
        case EntryPointPS:
        case ExitPointPS:
            break;

        default:
            return FALSE;
        }
    }

    return TRUE;
}

bool BrowserState::can_reference(BrowserState * st) const
{
    const BrowserNode * p = this;

    for (;;) {
        if (p == (BrowserNode *) st)
            return FALSE;

        p = (BrowserNode *) p->parent();

        if (p->get_type() != UmlState)
            return TRUE;
    }
}

void BrowserState::renumber(int phase)
{
    if (phase != -1)
        new_ident(phase, all);
}

const QPixmap * BrowserState::pixmap(int) const
{
    if (deletedp())
        return DeletedStateIcon;

    const QPixmap * px = ProfiledStereotypes::browserPixmap(def->get_stereotype());

    return (px != 0) ? px : StateIcon;
}

BrowserTransition * BrowserState::add_transition(BrowserNode * end)
{
    BrowserTransition * result = new BrowserTransition(this, end);

    modified();
    return result;
}

BasicData * BrowserState::add_relation(UmlCode, BrowserNode * end)
{
    return add_transition(end)->get_data();
}

BrowserState * BrowserState::add_state(BrowserNode * future_parent,
                                       bool machine)
{
    QString name;

    if (!future_parent->enter_child_name(name, TR("enter state's name : "),
                                         UmlState, TRUE, FALSE))

        return 0;

    BrowserState * st = add_state(future_parent, name);

    if (machine)
        st->def->set_stereotype((future_parent->get_type() == UmlState)
                                ? "submachine" : "machine");

    return st;
}

BrowserState * BrowserState::add_state(BrowserNode * future_parent,
                                       QString name)
{
    return new BrowserState(name, future_parent, new StateData());
}

BrowserState * BrowserState::get_state(BrowserNode * parent)
{
    BrowserNodeList l;
    Q3ListViewItem * child;

    for (child = parent->firstChild(); child != 0; child = child->nextSibling())
        if (!((BrowserNode *) child)->deletedp() &&
            (((BrowserNode *) child)->get_type() == UmlState))
            l.append((BrowserNode *) child);

    BrowserNode * old;
    QString name;

    if (!parent->enter_child_name(name, TR("enter state's name : "),
                                  UmlState, l, &old,
                                  TRUE, FALSE))
        return 0;

    if (old != 0)
        return ((BrowserState *) old);

    StateData * sd = new StateData();
    BrowserState * r = new BrowserState(name, parent, sd);

    sd->set_browser_node(r);
    parent->setOpen(TRUE);
    parent->package_modified();

    return r;
}

static bool is_machine(const BrowserState * s)
{
    switch (((BrowserNode *) s->parent())->get_type()) {
    case UmlState:
    case UmlRegion:
        return FALSE;
        break;

    default:
        return TRUE;
    }
}

void BrowserState::menu()
{
    Q3PopupMenu m(0, name);
    Q3PopupMenu toolm(0);
    QString what;
    bool ref = is_ref();
    bool mach = is_machine(this);

    if (ref)
        what = "state machine reference";
    else if (mach)
        what = "state machine";
    else
        what = (!strcmp(get_stereotype(), "submachine"))
               ? "state submachine" : "state";

    MenuFactory::createTitle(m, def->definition(FALSE, TRUE));
    m.insertSeparator();

    if (!deletedp()) {
        if (!is_read_only && !ref) {
            m.setWhatsThis(m.insertItem(TR("New state diagram"), 0),
                           TR("to add a <i>state diagram</i>"));

            if (mach) {
                m.setWhatsThis(m.insertItem(TR("New submachine"), 1),
                               TR("to add a <i>submachine</i> to the <i>machine</i>"));
                m.setWhatsThis(m.insertItem(TR("New state"), 2),
                               TR("to add a <i>state</i> to the <i>machine</i>"));
            }
            else if (what == "state")
                m.setWhatsThis(m.insertItem(TR("New nested state"), 2),
                               TR("to add a <i>nested state</i> to the <i>state</i>"));
            else
                m.setWhatsThis(m.insertItem(TR("New state"), 2),
                               TR("to add a <i>state</i> to the <i>submachine</i>"));

            m.setWhatsThis(m.insertItem(TR("New region"), 11),
                           TR("to add a <i>region</i>"));
            m.insertSeparator();
        }

        m.setWhatsThis(m.insertItem(TR("Edit"), 3),
                       TR("to edit the <i>" + what + "</i>, \
a double click with the left mouse button does the same thing"));

        if (!is_read_only) {
            m.setWhatsThis(m.insertItem(TR("Duplicate"), 4),
                           TR("to copy the <i>" + what + "</i> in a new one"));
            m.insertSeparator();

            if (edition_number == 0)
                m.setWhatsThis(m.insertItem(TR("Delete"), 7),
                               TR("to delete the <i>" + what + "</i>. \
Note that you can undelete it after"));
        }

        m.setWhatsThis(m.insertItem(TR("Referenced by"), 10),
                       TR("to know who reference the <i>state</i> \
through a transition or a reference"));
        mark_menu(m, TR("the " + what), 90);
        ProfiledStereotypes::menu(m, this, 99990);

        if ((edition_number == 0) &&
            Tool::menu_insert(&toolm, get_type(), 100)) {
            m.insertSeparator();
            m.insertItem(TR("Tool"), &toolm);
        }
    }
    else if (!is_read_only && (edition_number == 0)) {
        m.setWhatsThis(m.insertItem(TR("Undelete"), 8),
                       TR("to undelete the <i>" + what + "</i>"));

        Q3ListViewItem * child;

        for (child = firstChild(); child != 0; child = child->nextSibling()) {
            if (((BrowserNode *) child)->deletedp()) {
                m.setWhatsThis(m.insertItem(TR("Undelete recursively"), 9),
                               TR("undelete the " + what + " and its children"));
                break;
            }
        }
    }

    exec_menu_choice(m.exec(QCursor::pos()), mach);
}

void BrowserState::exec_menu_choice(int rank,
                                    bool mach)
{
    switch (rank) {
    case 0: {
        BrowserStateDiagram * d =
            BrowserStateDiagram::add_state_diagram(this);

        if (d == 0)
            return;

        d->select_in_browser();
    }
    break;

    case 1:
        add_state(this, (bool) TRUE);
        return;

    case 2:
        add_state(this, (bool) FALSE);
        return;

    case 3:
        open(TRUE);
        return;

    case 4: {
        QString name;
        QString what;

        if (mach)
            what = TR("state machine");
        else
            what = (!strcmp(get_stereotype(), "submachine"))
                   ? TR("state submachine")
                   : TR("state");

        if (((BrowserNode *) parent())->enter_child_name(name, TR("enter " + what + "'s name : "),
                UmlState, TRUE, FALSE))
            duplicate((BrowserNode *) parent(), name)->select_in_browser();
    }
    break;

    case 7:
        delete_it();
        break;

    case 8:
        BrowserNode::undelete(FALSE);
        break;

    case 9:
        BrowserNode::undelete(TRUE);
        break;

    case 10:
        ReferenceDialog::show(this);
        return;

    case 11:
        BrowserRegion::add_region(this);
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

    ((BrowserNode *) parent())->modified();
    package_modified();
}

void BrowserState::apply_shortcut(QString s)
{
    int choice = -1;
    bool ref = is_ref();
    bool mach = is_machine(this);

    if (!deletedp()) {
        if (!is_read_only && !ref) {
            if (s == "New state diagram")
                choice = 0;

            if (mach) {
                if (s == "New submachine")
                    choice = 1;
            }

            if (s == "New state")
                choice = 2;
            else if (s == "New region")
                choice = 11;
        }

        if (s == "Edit")
            choice = 3;

        if (!is_read_only) {
            if (s == "Duplicate")
                choice = 4;

            if (edition_number == 0)
                if (s == "Delete")
                    choice = 7;
        }

        if (s == "Referenced by")
            choice = 10;

        mark_shortcut(s, choice, 90);

        if (edition_number == 0)
            Tool::shortcut(s, choice, get_type(), 100);
    }
    else if (!is_read_only && (edition_number == 0)) {
        if (s == "Undelete")
            choice = 8;

        Q3ListViewItem * child;

        for (child = firstChild(); child != 0; child = child->nextSibling()) {
            if (((BrowserNode *) child)->deletedp()) {
                if (s == "Undelete recursively")
                    choice = 9;

                break;
            }
        }
    }

    exec_menu_choice(choice, mach);
}

void BrowserState::open(bool force_edit)
{
    if (!force_edit &&
        (associated_diagram != 0) &&
        !associated_diagram->deletedp())
        associated_diagram->open(FALSE);
    else if (!is_edited)
        def->edit();
}

void BrowserState::modified()
{
    repaint();
    ((BrowserNode *) parent())->modified();
    // to update regions drawing
    def->modified();
}

UmlCode BrowserState::get_type() const
{
    return UmlState;
}

QString BrowserState::get_stype() const
{
    if (is_ref())
        return TR("state machine reference");
    else if (is_machine(this))
        return TR("state machine");
    else
        return TR("state");
}

int BrowserState::get_identifier() const
{
    return get_ident();
}

const char * BrowserState::help_topic() const
{
    return "state";
}

BasicData * BrowserState::get_data() const
{
    return def;
}

QString BrowserState::full_name(bool rev, bool) const
{
    return fullname(rev);
}

BrowserNodeList & BrowserState::instances(BrowserNodeList & result, bool sort)
{
    IdIterator<BrowserState> it(all);
    BrowserState * st;

    while ((st = it.current()) != 0) {
        if (!st->deletedp())
            result.append(st);

        ++it;
    }

    if (sort)
        result.sort_it();

    return result;
}

BrowserNode * BrowserState::get_associated() const
{
    return associated_diagram;
}

void BrowserState::set_associated_diagram(BrowserStateDiagram * dg,
        bool on_read)
{
    if (associated_diagram != dg) {
        if (associated_diagram != 0)
            QObject::disconnect(associated_diagram->get_data(), SIGNAL(deleted()),
                                def, SLOT(on_delete()));

        if ((associated_diagram = dg) != 0)
            QObject::connect(associated_diagram->get_data(), SIGNAL(deleted()),
                             def, SLOT(on_delete()));

        if (! on_read)
            package_modified();
    }
}

void BrowserState::on_delete()
{
    if (associated_diagram && associated_diagram->deletedp())
        associated_diagram = 0;
}

void BrowserState::init()
{
    its_default_stereotypes.clear();
    its_default_stereotypes.append("machine");
    its_default_stereotypes.append("submachine");
    its_default_stereotypes.append("top");

    BrowserStateAction::init();
    BrowserPseudoState::init();
    BrowserRegion::init();
}

// unicode
const QStringList & BrowserState::default_stereotypes()
{
    return its_default_stereotypes;
}

bool BrowserState::api_compatible(unsigned v) const
{
    return (v > 14);
}

bool BrowserState::tool_cmd(ToolCom * com, const char * args)
{
    switch ((unsigned char) args[-1]) {
    case createCmd: {
        bool ok = TRUE;

        if (is_read_only && !root_permission())
            ok = FALSE;
        else {
            UmlCode k = com->get_kind(args);

            switch (k) {
            case UmlStateDiagram:
                if (wrong_child_name(args, UmlStateDiagram, TRUE, FALSE))
                    ok = FALSE;
                else
                    (new BrowserStateDiagram(args, this))->write_id(com);

                break;

            case UmlState:
                if (wrong_child_name(args, UmlState, TRUE, FALSE))
                    ok = FALSE;
                else
                    // force QString(args) else call add_state(BrowserNode, bool) !
                    (BrowserState::add_state(this, QString(args)))->write_id(com);

                break;

            case UmlRegion:
                (BrowserRegion::add_region(this, args))->write_id(com);
                break;

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

            case UmlStateAction:
                BrowserStateAction::add_stateaction(this)->write_id(com);
                break;

            default:
                if (IsaPseudoState(k) &&
                    !wrong_child_name(args, k, TRUE,
                                      BrowserPseudoState::allow_empty(k)))
                    BrowserPseudoState::add_pseudostate(this, k, args)->write_id(com);
                else
                    ok = FALSE;

                break;
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

BrowserState * BrowserState::get_machine(const BrowserNode * bn)
{
    for (;;) {
        switch (((BrowserNode *) bn->parent())->get_type()) {
        case UmlState:
        case UmlRegion:
        case InitialPS:
        case EntryPointPS:
        case FinalPS:
        case TerminatePS:
        case ExitPointPS:
        case DeepHistoryPS:
        case ShallowHistoryPS:
        case JunctionPS:
        case ChoicePS:
        case ForkPS:
        case JoinPS:
        case UmlStateAction:
            bn = (BrowserNode *) bn->parent();
            break;

        default:
            return (BrowserState *) bn;
        }
    }
}

bool BrowserState::may_contains_them(const QList<BrowserNode *> & l,
                                     BooL & duplicable) const
{
    BrowserNode * machine = get_machine(this);

    foreach (BrowserNode *node, l) {
        switch (node->get_type()) {
        case UmlTransition:

            // no break !
        case UmlRegion:
            duplicable = FALSE;

            // no break !
        case UmlState:
        case UmlStateDiagram:
        case InitialPS:
        case FinalPS:
        case TerminatePS:
        case DeepHistoryPS:
        case ShallowHistoryPS:
        case JunctionPS:
        case ChoicePS:
        case ForkPS:
        case JoinPS:
        case UmlStateAction:
            if (is_ref())
                return FALSE;

            // no break
        case EntryPointPS:
        case ExitPointPS:
            if (get_machine(node) != machine)
                return FALSE;

            break;

        default:
            return FALSE;
        }

        if (! BrowserNode::may_contains(node, TRUE))
            return FALSE;

        duplicable = may_contains_it(node);
    }

    return TRUE;
}


bool BrowserState::may_contains(UmlCode k) const
{
    if (is_ref()) {
        switch (k) {
        case UmlTransition:
        case UmlRegion:
        case UmlState:
        case UmlStateDiagram:
        case InitialPS:
        case FinalPS:
        case TerminatePS:
        case DeepHistoryPS:
        case ShallowHistoryPS:
        case JunctionPS:
        case ChoicePS:
        case ForkPS:
        case JoinPS:
        case UmlStateAction:
            if (is_ref())
                return FALSE;

        default:
            // exit and entry point
            break;
        }
    }

    return TRUE;
}

void BrowserState::DragMoveEvent(QDragMoveEvent * e)
{
    if (UmlDrag::canDecode(e, BrowserState::drag_key(this)) ||
        UmlDrag::canDecode(e, BrowserRegion::drag_key(this)) ||
        UmlDrag::canDecode(e, BrowserPseudoState::drag_key(this)) ||
        UmlDrag::canDecode(e, BrowserStateAction::drag_key(this)) ||
        UmlDrag::canDecode(e, BrowserStateDiagram::drag_key(this)) ||
        UmlDrag::canDecode(e, BrowserTransition::drag_key(this))) {
        if (!is_read_only)
            e->accept();
        else
            e->ignore();
    }
    else
        ((BrowserNode *) parent())->DragMoveInsideEvent(e);
}

void BrowserState::DropEvent(QDropEvent * e)
{
    DropAfterEvent(e, 0);
}

void BrowserState::DragMoveInsideEvent(QDragMoveEvent * e)
{
    if (!is_read_only &&
        (UmlDrag::canDecode(e, UmlState) ||
         UmlDrag::canDecode(e, BrowserState::drag_key(this)) ||
         UmlDrag::canDecode(e, BrowserRegion::drag_key(this)) ||
         UmlDrag::canDecode(e, BrowserPseudoState::drag_key(this)) ||
         UmlDrag::canDecode(e, BrowserStateAction::drag_key(this)) ||
         UmlDrag::canDecode(e, BrowserStateDiagram::drag_key(this)) ||
         UmlDrag::canDecode(e, BrowserTransition::drag_key(this))))
        e->accept();
    else
        e->ignore();
}

void BrowserState::DropAfterEvent(QDropEvent * e, BrowserNode * after)
{
    BrowserNode * bn;
    bool free_move = TRUE;

    if ((((bn = UmlDrag::decode(e, UmlState)) != 0) ||
         (free_move = FALSE, (bn = UmlDrag::decode(e, BrowserState::drag_key(this))) != 0) ||
         ((bn = UmlDrag::decode(e, BrowserPseudoState::drag_key(this))) != 0) ||
         ((bn = UmlDrag::decode(e, BrowserStateAction::drag_key(this))) != 0) ||
         ((bn = UmlDrag::decode(e, BrowserRegion::drag_key(this))) != 0) ||
         ((bn = UmlDrag::decode(e, BrowserStateDiagram::drag_key(this))) != 0) ||
         ((bn = UmlDrag::decode(e, BrowserTransition::drag_key(this))) != 0)) &&
        (bn != after) && (bn != this)) {
        if (may_contains(bn->get_type()) && BrowserNode::may_contains(bn, TRUE))  {
            if ((after == 0) &&
                free_move &&
                ((BrowserNode *) parent())->may_contains(bn, TRUE)) {
                // have choice
                Q3PopupMenu m(0);

                MenuFactory::createTitle(m, TR("move ") + bn->get_name());
                m.insertSeparator();
                m.insertItem(TR("In ") + QString(get_name()), 1);
                m.insertItem(TR("After ") + QString(get_name()), 2);

                switch (m.exec(QCursor::pos())) {
                case 1:
                    break;

                case 2:
                    ((BrowserNode *) parent())->DropAfterEvent(e, this);
                    return;

                default:
                    e->ignore();
                    return;
                }
            }

            move(bn, after);

            if (free_move && (!strcmp(bn->get_stereotype(), "machine")))
                ((BrowserState *) bn)->def->set_stereotype("submachine");
        }
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

#if 0
static void get_under(const BrowserNode * bn, Q3PtrList<const BrowserNode> & l)
{
    l.append(bn);

    for (Q3ListViewItem * child = bn->firstChild();
         child != 0;
         child = child->nextSibling())
        if (((BrowserNode *) child)->get_type() != UmlTransition)
            get_under((BrowserNode *) child, l);
}

static bool check_trans(const BrowserNode * bn, Q3PtrList<const BrowserNode> & l,
                        bool under, const BrowserNode * st)
{
    under |= (bn == st);

    for (Q3ListViewItem * child = bn->firstChild();
         child != 0;
         child = child->nextSibling()) {
        if (((BrowserNode *) child)->get_type() == UmlTransition) {
            if (under !=
                (l.find(((TransitionData *)(((BrowserNode *) child)->get_data()))->get_end_node()) != -1))
                return FALSE;

        }
        else if (! check_trans((BrowserNode *) child, l, under, st))
            return FALSE;
    }

    return TRUE;
}

bool BrowserState::free_of_move() const
{
    BrowserState * m = get_machine(this);

    if (this == m)
        return TRUE;

    Q3PtrList<const BrowserNode> l;

    get_under(this, l);
    return check_trans(m, l, FALSE, this);
}
#endif

QString BrowserState::drag_key() const
{
    BrowserState * m = get_machine(this);

    return (m == this)
           ? QString::number(UmlState)
           : QString::number(UmlState)
           + "#" + QString::number((unsigned long) m);
}

QString BrowserState::drag_postfix() const
{
    BrowserState * m = get_machine(this);

    return (m == this)
           ? QString()
           : "#" + QString::number((unsigned long) m);
}

QString BrowserState::drag_key(BrowserNode * p)
{
    return QString::number(UmlState)
           + "#" + QString::number((unsigned long) get_machine(p));
}

void BrowserState::save_stereotypes(QTextStream & st)
{
    nl_indent(st);
    st << "state_stereotypes ";
    save_unicode_string_list(its_default_stereotypes, st);
}

void BrowserState::read_stereotypes(char *& st, char *& k)
{
    if (!strcmp(k, "state_stereotypes")) {
        read_unicode_string_list(its_default_stereotypes, st);
        k = read_keyword(st);
    }
    else
        init();
}

void BrowserState::save(QTextStream & st, bool ref, QString & warning)
{
    if (ref)
        st << "state_ref " << get_ident() << " // " << get_name();
    else {
        nl_indent(st);
        st << "state " << get_ident() << " ";
        save_string(name, st);
        indent(+1);
        def->save(st, warning);

        if (associated_diagram != 0) {
            nl_indent(st);
            st << "associated_diagram ";
            associated_diagram->save(st, TRUE, warning);
        }

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

BrowserState * BrowserState::read_ref(char *& st)
{
    read_keyword(st, "state_ref");

    int id = read_id(st);
    BrowserState * result = all[id];

    return (result == 0)
           ? new BrowserState(id)
           : result;
}

BrowserState * BrowserState::read(char *& st, char * k,
                                  BrowserNode * parent)
{
    BrowserState * result;
    int id;

    if (!strcmp(k, "state_ref")) {
        id = read_id(st);
        result = all[id];

        return (result == 0)
               ? new BrowserState(id)
               : result;
    }
    else if (!strcmp(k, "state")) {
        id = read_id(st);
        result = all[id];

        if (result == 0)
            result = new BrowserState(read_string(st), parent, new StateData, id);
        else if (result->is_defined) {
            BrowserState * already_exist = result;

            result = new BrowserState(read_string(st), parent, new StateData, id);

            already_exist->must_change_id(all);
            already_exist->unconsistent_fixed("state", result);
        }
        else {
            result->set_parent(parent);
            result->set_name(read_string(st));
        }

        result->is_defined = TRUE;
        k = read_keyword(st);
        result->def->read(st, k);

        if (!strcmp(k, "associated_diagram")) {
            result->set_associated_diagram(BrowserStateDiagram::read_ref(st, read_keyword(st)),
                                           TRUE);
            k = read_keyword(st);
        }


        result->BrowserNode::read(st, k, id);

        result->is_read_only = (!in_import() && read_only_file()) ||
                               ((user_id() != 0) && result->is_api_base());

        if (strcmp(k, "end")) {
            while (BrowserState::read(st, k, result) ||
                   BrowserRegion::read(st, k, result) ||
                   BrowserPseudoState::read(st, k, result) ||
                   BrowserStateAction::read(st, k, result) ||
                   BrowserStateDiagram::read(st, k, result) ||
                   BrowserTransition::read(st, k, result))
                k = read_keyword(st);

            if (strcmp(k, "end"))
                wrong_keyword(k, "end");
        }

        return result;
    }
    else
        return 0;
}

BrowserNode * BrowserState::read_any_ref(char *& st, char * k)
{
    BrowserNode * r;

    if (((r = BrowserState::read(st, k, 0)) == 0) &&
        ((r = BrowserRegion::read(st, k, 0)) == 0) &&
        ((r = BrowserPseudoState::read(st, k, 0)) == 0) &&
        ((r = BrowserStateAction::read(st, k, 0)) == 0))
        r = BrowserTransition::read(st, k, 0);

    return r;
}

BrowserNode * BrowserState::get_it(const char * k, int id)
{
    if (!strcmp(k, "state_ref"))
        return all[id];

    BrowserNode * r;

    if (((r = BrowserRegion::get_it(k, id)) == 0) &&
        ((r = BrowserPseudoState::get_it(k, id)) == 0) &&
        ((r = BrowserStateAction::get_it(k, id)) == 0) &&
        ((r = BrowserStateDiagram::get_it(k, id)) == 0))
        r = BrowserTransition::get_it(k, id);

    return r;
}
