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

#include "BrowserActivityNode.h"
#include "BrowserActivityDiagram.h"
#include "BrowserFlow.h"
#include "FlowData.h"
#include "ParameterData.h"
#include "ActivityObjectData.h"
#include "PinData.h"
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
#include "translate.h"

IdDict<BrowserActivityNode> BrowserActivityNode::all(257, __FILE__);
QStringList BrowserActivityNode::its_default_stereotypes;	// unicode

BrowserActivityNode::BrowserActivityNode(UmlCode c, QString s, BrowserNode * p, int id)
    : BrowserNode(s, p), Labeled<BrowserActivityNode>(all, id),
      kind(c), def(new SimpleData)
{
    def->set_browser_node(this);
}

BrowserActivityNode::BrowserActivityNode(int id)
    : BrowserNode(), Labeled<BrowserActivityNode>(all, id),
      kind(UmlActivityNode), def(new SimpleData)
{
    // not yet read
    def->set_browser_node(this);
}

BrowserActivityNode::BrowserActivityNode(const BrowserActivityNode * model, BrowserNode * p)
    : BrowserNode(model->name, p), Labeled<BrowserActivityNode>(all, 0)
{
    def = new SimpleData(model->def);
    def->set_browser_node(this);
    comment = model->comment;
    kind = model->kind;
}

BrowserNode * BrowserActivityNode::duplicate(BrowserNode * p, QString name)
{
    BrowserNode * result = new BrowserActivityNode(this, p);

    result->set_name(name);
    result->update_stereotype();
    return result;
}

BrowserActivityNode::~BrowserActivityNode()
{
    all.remove(get_ident());
    delete def;
}

void BrowserActivityNode::clear(bool old)
{
    all.clear(old);
}

void BrowserActivityNode::update_idmax_for_root()
{
    all.update_idmax_for_root();
}

void BrowserActivityNode::prepare_update_lib() const
{
    all.memo_id_oid(get_ident(), original_id);

    for (Q3ListViewItem * child = firstChild();
         child != 0;
         child = child->nextSibling())
        ((BrowserNode *) child)->prepare_update_lib();
}

void BrowserActivityNode::referenced_by(QList<BrowserNode *> & l, bool ondelete)
{
    BrowserNode::referenced_by(l, ondelete);
    BrowserFlow::compute_referenced_by(l, this);

    if (! ondelete)
        BrowserActivityDiagram::compute_referenced_by(l, this, "activitynodecanvas", "activitynode_ref");
}

void BrowserActivityNode::renumber(int phase)
{
    if (phase != -1)
        new_ident(phase, all);
}

const QPixmap * BrowserActivityNode::pixmap(int) const
{
    if (! deletedp()) {
        const QPixmap * px = ProfiledStereotypes::browserPixmap(def->get_stereotype());

        if (px != 0)
            return px;
    }

    switch (kind) {
    case InitialAN:
        if (deletedp())
            return DeletedInitialIcon;
        else if (is_marked)
            return InitialMarkedIcon;
        else
            return InitialIcon;

    case ActivityFinalAN:
        if (deletedp())
            return DeletedFinalIcon;
        else if (is_marked)
            return FinalMarkedIcon;
        else
            return FinalIcon;

    case FlowFinalAN:
        return (deletedp()) ? DeletedExitPointIcon : ExitPointIcon;

    case DecisionAN:
        if (deletedp())
            return DeletedDecisionIcon;
        else if (is_marked)
            return DecisionMarkedIcon;
        else
            return DecisionIcon;

    case MergeAN:
        if (deletedp())
            return DeletedMergeIcon;
        else if (is_marked)
            return MergeMarkedIcon;
        else
            return MergeIcon;

    case ForkAN:
        if (deletedp())
            return DeletedForkIcon;
        else if (is_marked)
            return ForkMarkedIcon;
        else
            return ForkIcon;

    default: // JoinAN:
        if (deletedp())
            return DeletedJoinIcon;
        else if (is_marked)
            return JoinMarkedIcon;
        else
            return JoinIcon;
    }
}

bool BrowserActivityNode::target_of_flow() const
{
    QList<BrowserNode *> l;

    BrowserFlow::compute_referenced_by(l, this);
    return !l.isEmpty();
}

// add flow (dependency not allowed)
BasicData * BrowserActivityNode::add_relation(UmlCode, BrowserNode * end)
{
    return (new BrowserFlow(this, end))->get_data();
}

// a flow may be added (dependency not allowed) ?
QString BrowserActivityNode::may_start() const
{
    switch (get_type()) {
    case FlowFinalAN:
    case ActivityFinalAN:
        return TR("can't have outgoing flow");

    case InitialAN:
    case MergeAN:	      // theo all input and output must
    case JoinAN:	      // be control/data exclusively
        // only one flow is allowed
    {
        Q3ListViewItem * child;

        for (child = firstChild(); child != 0; child = child->nextSibling())
            if ((((BrowserNode *) child)->get_type() == UmlFlow) &&
                (!((BrowserNode *) child)->deletedp()))
                return TR("can't have several outgoing flows");
    }

    // no break
    default:
        return 0;
    }
}

// connexion by a flow (dependency not allowed)
QString BrowserActivityNode::may_connect(const BrowserNode * dest) const
{
    BrowserNode * bn = dest->get_container(UmlActivity);

    if ((bn != 0) && (get_container(UmlActivity) != bn))
        return TR("not in the same activity");

    BasicData * data = dest->get_data();

    switch (dest->get_type()) {
    case InitialAN:
        return TR("initial node can't have incoming flow");

    case ForkAN:
        return (((BrowserActivityNode *) dest)->target_of_flow())
               ? TR("fork can't have several incoming flow")
               : QString();

    case UmlParameter:
        if (((ParameterData *) data)->get_dir() == UmlIn)
            return TR("an input parameter can't have incoming flows");
        else if (!((ParameterData *) data)->get_is_control())
            return TR("parameter can't accept control flow (not 'is_control')");
        else
            return QString();

    case UmlExpansionNode:
        return (!((ActivityObjectData *) data)->get_is_control())
               ? TR("can't accept control flow (not 'is_control')")
               : QString();

    case UmlActivityPin:
        return (!((PinData *) data)->get_is_control())
               ? TR("pin can't accept control flow (not 'is_control')")
               : QString();

    case UmlActivityObject:
    case UmlActivityAction:
    case FlowFinalAN:
    case ActivityFinalAN:
    case DecisionAN:
    case MergeAN:	      // theo all input and output must
    case JoinAN:	      // be control/data exclusively
        return 0;

    default:
        return TR("illegal");
    }
}

QString BrowserActivityNode::connexion_from(bool control) const
{
    switch (get_type()) {
    case ForkAN:  // theo all input and output must be control/data exclusively
        if (target_of_flow())
            return TR("fork can't have several incoming flow");
        else
            return 0;

    case FlowFinalAN:
    case ActivityFinalAN:
        if (! control)
            return TR("can't have incoming data flow");
        else
            return 0;

    case DecisionAN:    // theo all input and
    case MergeAN:	      // output must be
    case JoinAN:	      // control/data exclusively
        return 0;

    default:
        return TR("illegal");
    }
}

BrowserActivityNode *
BrowserActivityNode::add_activitynode(BrowserNode * future_parent,
                                      UmlCode c, const char * name)
{
    BrowserActivityNode * r =
        new BrowserActivityNode(c, name, future_parent);

    future_parent->setOpen(TRUE);
    future_parent->package_modified();

    return r;
}

BrowserActivityNode * BrowserActivityNode::get_activitynode(BrowserNode * future_parent, UmlCode c)
{
    BrowserActivityNode * r = new BrowserActivityNode(c, "", future_parent);

    future_parent->setOpen(TRUE);
    future_parent->package_modified();

    return r;
}

void BrowserActivityNode::menu()
{
    QString s = stringify(kind);
    int index = s.find("_");

    if (index != -1)
        s.replace(index, 1, " ");

    Q3PopupMenu m(0, "Activity node");
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
through a flow"));
        mark_menu(m, TR("the " + s), 90);
        ProfiledStereotypes::menu(m, this, 99990);;

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

void BrowserActivityNode::exec_menu_choice(int rank)
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

void BrowserActivityNode::apply_shortcut(QString s)
{
    int choice = -1;

    if (!deletedp()) {
        if (s == "Edit")
            choice = 1;

        if (!is_read_only) {
            if (s == "Duplicate")
                choice = 2;
            else if (edition_number == 0)
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

void BrowserActivityNode::open(bool)
{
    if (!is_edited) {
        QString s = stringify(kind);
        int index = 0;

        while ((index = s.find("_")) != -1)
            s.replace(index, 1, " ");

        static QSize previous_size;

        (new BasicDialog(get_data(), s, its_default_stereotypes,
                         previous_size))->show();
    }
}

void BrowserActivityNode::modified()
{
    repaint();
    ((BrowserNode *) parent())->modified();
}

UmlCode BrowserActivityNode::get_type() const
{
    return kind;
}

QString BrowserActivityNode::get_stype() const
{
    QString s = stringify(kind);
    int index = s.find("_");

    if (index != -1)
        s.replace(index, 1, " ");

    return TR(s);
}

int BrowserActivityNode::get_identifier() const
{
    return get_ident();
}

const char * BrowserActivityNode::help_topic() const
{
    return "activitycontrolnode";
}

BasicData * BrowserActivityNode::get_data() const
{
    return def;
}

bool BrowserActivityNode::allow_empty() const
{
    return TRUE;
}

QString BrowserActivityNode::full_name(bool rev, bool) const
{
    QString s = name;

    if (s.isEmpty())
        s = "<anonymous activity node>";

    return fullname(s, rev);
}

void BrowserActivityNode::init()
{
    its_default_stereotypes.clear();
}

// unicode
const QStringList & BrowserActivityNode::default_stereotypes()
{
    return its_default_stereotypes;
}

bool BrowserActivityNode::api_compatible(unsigned v) const
{
    return (v > 24);
}

bool BrowserActivityNode::tool_cmd(ToolCom * com, const char * args)
{
    switch ((unsigned char) args[-1]) {
    case createCmd: {
        bool ok = TRUE;

        if (is_read_only && !root_permission())
            ok = FALSE;
        else {
            UmlCode k = com->get_kind(args);

            switch (k) {
            case UmlFlow: {
                BrowserNode * end = (BrowserNode *) com->get_id(args);

                if (may_connect(end).isEmpty())
                    (new BrowserFlow(this, end))->write_id(com);
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

void BrowserActivityNode::DragMoveEvent(QDragMoveEvent * e)
{
    if (UmlDrag::canDecode(e, BrowserFlow::drag_key(this))) {
        if (!is_read_only)
            e->accept();
        else
            e->ignore();
    }
    else
        ((BrowserNode *) parent())->DragMoveInsideEvent(e);
}

void BrowserActivityNode::DropEvent(QDropEvent * e)
{
    DropAfterEvent(e, 0);
}

void BrowserActivityNode::DragMoveInsideEvent(QDragMoveEvent * e)
{
    if (!is_read_only &&
        UmlDrag::canDecode(e, BrowserFlow::drag_key(this)))
        e->accept();
    else
        e->ignore();
}

void BrowserActivityNode::DropAfterEvent(QDropEvent * e, BrowserNode * after)
{
    BrowserNode * bn;

    if ((((bn = UmlDrag::decode(e, BrowserFlow::drag_key(this))) != 0)) &&
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

QString BrowserActivityNode::drag_key() const
{
    return QString::number(UmlActivityNode)
           + "#" + QString::number((unsigned long) get_container(UmlActivity));
}

QString BrowserActivityNode::drag_postfix() const
{
    return "#" + QString::number((unsigned long) get_container(UmlActivity));
}

QString BrowserActivityNode::drag_key(BrowserNode * p)
{
    return QString::number(UmlActivityNode)
           + "#" + QString::number((unsigned long) p->get_container(UmlActivity));
}

void BrowserActivityNode::save_stereotypes(QTextStream & st)
{
    nl_indent(st);
    st << "activitynode_stereotypes ";
    save_unicode_string_list(its_default_stereotypes, st);
}

void BrowserActivityNode::read_stereotypes(char *& st, char *& k)
{
    if (!strcmp(k, "activitynode_stereotypes")) {
        read_unicode_string_list(its_default_stereotypes, st);
        k = read_keyword(st);
    }
    else
        init();
}

void BrowserActivityNode::save(QTextStream & st, bool ref, QString & warning)
{
    if (ref) {
        st << "activitynode_ref " << get_ident() << " // " << stringify(kind);

        if (!allow_spaces())
            st << " " << get_name();
    }
    else {
        nl_indent(st);
        st << "activitynode " << get_ident() << " " << stringify(kind) << " ";
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

BrowserActivityNode * BrowserActivityNode::read_ref(char *& st)
{
    read_keyword(st, "activitynode_ref");

    int id = read_id(st);
    BrowserActivityNode * result = all[id];

    return (result == 0)
           ? new BrowserActivityNode(id)
           : result;
}

BrowserActivityNode * BrowserActivityNode::read(char *& st, char * k,
        BrowserNode * parent)
{
    BrowserActivityNode * result;
    int id;

    if (!strcmp(k, "activitynode_ref")) {
        id = read_id(st);
        result = all[id];

        return (result == 0)
               ? new BrowserActivityNode(id)
               : result;
    }
    else if (!strcmp(k, "activitynode")) {
        id = read_id(st);
        UmlCode c = activity_node_kind(read_keyword(st));
        result = all[id];

        if (result == 0)
            result = new BrowserActivityNode(c, read_string(st), parent, id);
        else if (result->is_defined) {
            BrowserActivityNode * already_exist = result;

            result = new BrowserActivityNode(c, read_string(st), parent, id);

            already_exist->must_change_id(all);
            already_exist->unconsistent_fixed("activity node", result);
        }
        else {
            result->kind = c;
            result->set_parent(parent);
            result->set_name(read_string(st));
        }

        result->is_defined = TRUE;
        k = read_keyword(st);
        result->def->read(st, k);

        result->BrowserNode::read(st, k, id);

        result->is_read_only = (!in_import() && read_only_file()) ||
                               ((user_id() != 0) && result->is_api_base());

        if (strcmp(k, "end")) {
            while (BrowserFlow::read(st, k, result))
                k = read_keyword(st);

            if (strcmp(k, "end"))
                wrong_keyword(k, "end");
        }

        return result;
    }
    else
        return 0;
}

BrowserNode * BrowserActivityNode::get_it(const char * k, int id)
{
    // flow managed in activity
    return (!strcmp(k, "activitynode_ref")) ? all[id] : 0;
}
