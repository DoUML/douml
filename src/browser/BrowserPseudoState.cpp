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

#include "BrowserPseudoState.h"
#include "PseudoStateData.h"
#include "StateData.h"
#include "BrowserState.h"
#include "BrowserTransition.h"
#include "TransitionData.h"
#include "ReferenceDialog.h"
#include "UmlDrag.h"
#include "UmlPixmap.h"
#include "BasicDialog.h"
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

IdDict<BrowserPseudoState> BrowserPseudoState::all(257, __FILE__);
QStringList BrowserPseudoState::its_default_stereotypes;	// unicode

BrowserPseudoState::BrowserPseudoState(UmlCode c, QString s, BrowserNode * p,
                                       PseudoStateData * d, int id)
    : BrowserNode(s, p), Labeled<BrowserPseudoState>(all, id), kind(c), def(d)
{
    def->set_browser_node(this);
}

BrowserPseudoState::BrowserPseudoState(int id)
    : BrowserNode(), Labeled<BrowserPseudoState>(all, id),
      kind(UmlPseudoState), def(new PseudoStateData)
{
    // not yet read
    def->set_browser_node(this);
}

BrowserPseudoState::BrowserPseudoState(const BrowserPseudoState * model, BrowserNode * p)
    : BrowserNode(model->name, p), Labeled<BrowserPseudoState>(all, 0)
{
    def = new PseudoStateData(model->def, this);
    comment = model->comment;
    kind = model->kind;
}

BrowserNode * BrowserPseudoState::duplicate(BrowserNode * p, QString name)
{
    BrowserNode * result = new BrowserPseudoState(this, p);

    if (!allow_empty())
        result->set_name((name.isEmpty()) ? get_name() : name);

    result->update_stereotype();
    return result;
}

BrowserPseudoState::~BrowserPseudoState()
{
    all.remove(get_ident());
    delete def;
}

void BrowserPseudoState::clear(bool old)
{
    all.clear(old);
}

void BrowserPseudoState::update_idmax_for_root()
{
    all.update_idmax_for_root();
}

void BrowserPseudoState::prepare_update_lib() const
{
    all.memo_id_oid(get_ident(), original_id);

    for (Q3ListViewItem * child = firstChild();
         child != 0;
         child = child->nextSibling())
        ((BrowserNode *) child)->prepare_update_lib();
}

void BrowserPseudoState::referenced_by(QList<BrowserNode *> & l, bool ondelete)
{
    BrowserNode::referenced_by(l, ondelete);
    BrowserTransition::compute_referenced_by(l, this);

    if (! ondelete)
        BrowserStateDiagram::compute_referenced_by(l, this, "pseudostatecanvas", "pseudostate_ref");
}

void BrowserPseudoState::renumber(int phase)
{
    if (phase != -1)
        new_ident(phase, all);
}

const QPixmap * BrowserPseudoState::pixmap(int) const
{
    if (!deletedp()) {
        const QPixmap * px = ProfiledStereotypes::browserPixmap(def->get_stereotype());

        if (px != 0)
            return px;
    }

    switch (kind) {
    case InitialPS:
        if (deletedp())
            return DeletedInitialIcon;
        else if (is_marked)
            return InitialMarkedIcon;
        else
            return InitialIcon;

    case EntryPointPS:
        return (deletedp()) ? DeletedEntryPointIcon : EntryPointIcon;

    case FinalPS:
        if (deletedp())
            return DeletedFinalIcon;
        else if (is_marked)
            return FinalMarkedIcon;
        else
            return FinalIcon;

    case TerminatePS:
        if (deletedp())
            return DeletedTerminateIcon;
        else if (is_marked)
            return TerminateMarkedIcon;
        else
            return TerminateIcon;

    case ExitPointPS:
        return (deletedp()) ? DeletedExitPointIcon : ExitPointIcon;

    case DeepHistoryPS:
        if (deletedp())
            return DeletedDeepHistoryIcon;
        else if (is_marked)
            return DeepHistoryMarkedIcon;
        else
            return DeepHistoryIcon;

    case ShallowHistoryPS:
        if (deletedp())
            return DeletedShallowHistoryIcon;
        else if (is_marked)
            return ShallowHistoryMarkedIcon;
        else
            return ShallowHistoryIcon;

    case JunctionPS:
        if (deletedp())
            return DeletedJunctionIcon;
        else if (is_marked)
            return JunctionMarkedIcon;
        else
            return JunctionIcon;

    case ChoicePS:
        if (deletedp())
            return DeletedChoiceIcon;
        else if (is_marked)
            return ChoiceMarkedIcon;
        else
            return ChoiceIcon;

    case ForkPS:
        if (deletedp())
            return DeletedForkIcon;
        else if (is_marked)
            return ForkMarkedIcon;
        else
            return ForkIcon;

    default: // JoinPS:
        if (deletedp())
            return DeletedJoinIcon;
        else if (is_marked)
            return JoinMarkedIcon;
        else
            return JoinIcon;
    }
}

bool BrowserPseudoState::allow_empty(UmlCode c)
{
    switch (c) {
    case EntryPointPS:
    case ExitPointPS:
        return FALSE;

    default:
        return TRUE;
    }
}

BrowserTransition * BrowserPseudoState::add_transition(BrowserNode * end)
{
    BrowserTransition * result = new BrowserTransition(this, end);

    modified();
    return result;
}

BasicData * BrowserPseudoState::add_relation(UmlCode, BrowserNode * end)
{
    return add_transition(end)->get_data();
}

// a transition may be added ?
QString BrowserPseudoState::may_start() const
{
    switch (get_type()) {
    case FinalPS:
    case TerminatePS:
        return TR("can't have transition");

    case InitialPS:
    case DeepHistoryPS:
    case ShallowHistoryPS:
    case JoinPS:
        // only one transition is allowed
    {
        Q3ListViewItem * child;

        for (child = firstChild(); child != 0; child = child->nextSibling())
            if ((((BrowserNode *) child)->get_type() == UmlTransition) &&
                (!((BrowserNode *) child)->deletedp()))
                return TR("can't have several transition");
    }

    return 0;

    default:
        // note : exit point may have several transition,
        // each to a child of a diferent region
        return 0;
    }
}

// connexion by a transition, dest is a state or a pseudo state
QString BrowserPseudoState::may_connect(const BrowserNode * dest) const
{
    // note if this is an entry point, dest must be a child of this->parent()
    switch (dest->get_type()) {
    case InitialPS:
        return TR("can't have a transition to initial pseudo state");

    case ForkPS:
        // only one transition is allowed from dest
    {
        Q3ListViewItem * child;

        for (child = dest->firstChild(); child != 0; child = child->nextSibling())
            if ((((BrowserNode *) child)->get_type() == UmlTransition) &&
                (!((BrowserNode *) child)->deletedp()))
                return TR("fork can't have several input transitions");
    }

    return 0;

    default:
        /*case EntryPointPS:
        case UmlState:
        case ChoicePS:
        case UmlStateAction:
        case FinalPS:
        case TerminatePS:
        case ExitPointPS:
        case DeepHistoryPS:
        case ShallowHistoryPS:
        case JunctionPS:
        case JoinPS:*/
        return 0;
    }
}

BrowserPseudoState *
BrowserPseudoState::add_pseudostate(BrowserNode * future_parent,
                                    UmlCode c)
{
    QString name;
    QString s = stringify(c);
    int index = s.find("_");

    if (index != -1)
        s.replace(index, 1, " ");

    if (!allow_empty(c) &&
        !future_parent->enter_child_name(name, TR("enter " + s + "'s name : "),
                                         UmlState, TRUE, FALSE))

        return 0;

    return add_pseudostate(future_parent, c, name);
}

BrowserPseudoState *
BrowserPseudoState::add_pseudostate(BrowserNode * future_parent,
                                    UmlCode c, const char * name)
{
    return new BrowserPseudoState(c, name, future_parent, new PseudoStateData());
}

BrowserPseudoState * BrowserPseudoState::get_pseudostate(BrowserNode * future_parent, UmlCode c)
{
    BrowserNodeList l;
    Q3ListViewItem * child;

    for (child = future_parent->firstChild(); child != 0; child = child->nextSibling())
        if (!((BrowserNode *) child)->deletedp() &&
            (((BrowserNode *) child)->get_type() == c))
            l.append((BrowserNode *) child);

    BrowserNode * old = 0;
    QString name;
    QString s = stringify(c);
    int index = s.find("_");

    if (index != -1)
        s.replace(index, 1, " ");

    if (!allow_empty(c) &&
        !future_parent->enter_child_name(name, TR("enter " + s + "'s name : "),
                                         UmlState, l, &old, TRUE, FALSE))
        return 0;

    if (old != 0)
        return ((BrowserPseudoState *) old);

    switch (c) {
    case DeepHistoryPS:
        if (! l.isEmpty()) {
            msg_critical("Douml", TR("already have a deep history"));
            return 0;
        }

        break;

    case ShallowHistoryPS:
        if (! l.isEmpty()) {
            msg_critical("Douml",  TR("already have a shallow history"));
            return 0;
        }

        break;

    default:
        break;
    }

    PseudoStateData * sd = new PseudoStateData();
    BrowserPseudoState * r = new BrowserPseudoState(c, name, future_parent, sd);

    sd->set_browser_node(r);
    future_parent->setOpen(TRUE);
    future_parent->package_modified();

    return r;
}

void BrowserPseudoState::menu()
{
    QString s = stringify(kind);
    int index = s.find("_");

    if (index != -1)
        s.replace(index, 1, " ");

    Q3PopupMenu m(0, "pseudo state");
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

void BrowserPseudoState::exec_menu_choice(int rank)
{
    switch (rank) {
    case 1:
        open(TRUE);
        return;

    case 2: {
        QString name;
        QString s = stringify(kind);
        int index = s.find("_");

        if (index != -1)
            s.replace(index, 1, " ");

        if (allow_empty() ||
            ((BrowserNode *) parent())->enter_child_name(name, TR("enter " + s + "'s name : "),
                    get_type(), allow_spaces(),
                    FALSE))
            duplicate((BrowserNode *) parent(), name)->select_in_browser();
    }
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

void BrowserPseudoState::apply_shortcut(QString s)
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

void BrowserPseudoState::open(bool)
{
    if (!is_edited)
        def->edit();
}

void BrowserPseudoState::modified()
{
    repaint();
    ((BrowserNode *) parent())->modified();
    // to update regions drawing
    def->modified();
}

UmlCode BrowserPseudoState::get_type() const
{
    return kind;
}

QString BrowserPseudoState::get_stype() const
{
    QString s = stringify(kind);
    int index = s.find("_");

    if (index != -1)
        s.replace(index, 1, " ");

    return TR(s);
}

int BrowserPseudoState::get_identifier() const
{
    return get_ident();
}

BasicData * BrowserPseudoState::get_data() const
{
    return def;
}

bool BrowserPseudoState::allow_empty() const
{
    return allow_empty(kind);
}

QString BrowserPseudoState::full_name(bool rev, bool) const
{
    QString s = name;

    if (s.isEmpty())
        s = stringify(kind);

    return fullname(s, rev);
}

bool BrowserPseudoState::can_reference(BrowserNode * x) const
{
    if ((x->get_type() != get_type()) ||
        (((BrowserNode *) parent())->get_type() != UmlState))
        return FALSE;

    BrowserNode * ref =
        ((StateData *)((BrowserNode *) parent())->get_data())->get_reference();

    return ((ref != 0) &&
            ((x->parent() == ref) ||
             ((((BrowserNode *) x->parent())->get_type() == UmlRegion) &&
              (x->parent()->parent() == ref))));
}

void BrowserPseudoState::init()
{
    its_default_stereotypes.clear();
}

// unicode
const QStringList & BrowserPseudoState::default_stereotypes()
{
    return its_default_stereotypes;
}

bool BrowserPseudoState::tool_cmd(ToolCom * com, const char * args)
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

                if ((k == UmlState) || IsaPseudoState(k))
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

void BrowserPseudoState::DragMoveEvent(QDragMoveEvent * e)
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

void BrowserPseudoState::DropEvent(QDropEvent * e)
{
    DropAfterEvent(e, 0);
}

void BrowserPseudoState::DragMoveInsideEvent(QDragMoveEvent * e)
{
    if (!is_read_only &&
        UmlDrag::canDecode(e, BrowserTransition::drag_key(this)))
        e->accept();
    else
        e->ignore();
}

void BrowserPseudoState::DropAfterEvent(QDropEvent * e, BrowserNode * after)
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

QString BrowserPseudoState::drag_key() const
{
    return QString::number(UmlPseudoState)
           + "#" + QString::number((unsigned long) BrowserState::get_machine(this));
}

QString BrowserPseudoState::drag_postfix() const
{
    return "#" + QString::number((unsigned long) BrowserState::get_machine(this));
}

QString BrowserPseudoState::drag_key(BrowserNode * p)
{
    return QString::number(UmlPseudoState)
           + "#" + QString::number((unsigned long) BrowserState::get_machine(p));
}

void BrowserPseudoState::save_stereotypes(QTextStream & st)
{
    nl_indent(st);
    st << "pseudostate_stereotypes ";
    save_unicode_string_list(its_default_stereotypes, st);
}

void BrowserPseudoState::read_stereotypes(char *& st, char *& k)
{
    if (!strcmp(k, "pseudostate_stereotypes")) {
        read_unicode_string_list(its_default_stereotypes, st);
        k = read_keyword(st);
    }
    else
        init();
}

void BrowserPseudoState::save(QTextStream & st, bool ref, QString & warning)
{
    if (ref) {
        st << "pseudostate_ref " << get_ident() << " // " << stringify(kind);

        if (!allow_spaces())
            st << " " << get_name();
    }
    else {
        nl_indent(st);
        st << "pseudostate " << get_ident() << " " << stringify(kind) << " ";

        if (!allow_empty())
            save_string(name, st);

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

BrowserPseudoState * BrowserPseudoState::read_ref(char *& st)
{
    read_keyword(st, "pseudostate_ref");

    int id = read_id(st);
    BrowserPseudoState * result = all[id];

    return (result == 0)
           ? new BrowserPseudoState(id)
           : result;
}

BrowserPseudoState * BrowserPseudoState::read(char *& st, char * k,
        BrowserNode * parent)
{
    BrowserPseudoState * result;
    int id;

    if (!strcmp(k, "pseudostate_ref")) {
        id = read_id(st);
        result = all[id];

        return (result == 0)
               ? new BrowserPseudoState(id)
               : result;
    }
    else if (!strcmp(k, "pseudostate")) {
        id = read_id(st);
        UmlCode c = pseudo_state_kind(read_keyword(st));
        result = all[id];

        if (result == 0)
            result = new BrowserPseudoState(c, (allow_empty(c)) ? "" : (const char *) read_string(st),
                                            parent, new PseudoStateData, id);
        else if (result->is_defined) {
            BrowserPseudoState * already_exist = result;

            result = new BrowserPseudoState(c, (allow_empty(c)) ? "" : (const char *) read_string(st),
                                            parent, new PseudoStateData, id);

            already_exist->must_change_id(all);
            already_exist->unconsistent_fixed("pseudo state", result);
        }
        else {
            result->kind = c;
            result->set_parent(parent);

            if (!allow_empty(c))
                result->set_name(read_string(st));
        }

        result->is_defined = TRUE;
        k = read_keyword(st);
        result->def->read(st, k);

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

BrowserNode * BrowserPseudoState::get_it(const char * k, int id)
{
    // transition managed in state
    return (!strcmp(k, "pseudostate_ref")) ? all[id] : 0;
}
