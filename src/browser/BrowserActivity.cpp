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
#include <Q3ValueList>
#include <QPixmap>
#include <QDragMoveEvent>
#include <QDropEvent>

#include "BrowserActivity.h"
#include "ActivityData.h"
#include "BrowserFlow.h"
#include "BrowserSimpleRelation.h"
#include "BrowserParameter.h"
#include "BrowserActivityNode.h"
#include "BrowserActivityAction.h"
#include "BrowserActivityObject.h"
#include "BrowserExpansionRegion.h"
#include "BrowserInterruptibleActivityRegion.h"
#include "ActivityActionData.h"
#include "BrowserActivityPartition.h"
#include "BrowserActivityDiagram.h"
#include "ReferenceDialog.h"
#include "UmlDrag.h"
#include "UmlPixmap.h"
#include "myio.h"
#include "ToolCom.h"
#include "Tool.h"
#include "strutil.h"
#include "DialogUtil.h"
#include "ProfiledStereotypes.h"
#include "mu.h"
#include "translate.h"
#include "ui/menufactory.h"

IdDict<BrowserActivity> BrowserActivity::all(__FILE__);
QStringList BrowserActivity::its_default_stereotypes;	// unicode

BrowserActivity::BrowserActivity(QString s, BrowserNode * p, int id)
    : BrowserNode(s, p), Labeled<BrowserActivity>(all, id),
      def(new ActivityData), associated_diagram(0)
{
    def->set_browser_node(this);
}

BrowserActivity::BrowserActivity(int id)
    : BrowserNode(), Labeled<BrowserActivity>(all, id),
      def(new ActivityData), associated_diagram(0)
{
    // not yet read
    def->set_browser_node(this);
}

BrowserActivity::BrowserActivity(const BrowserActivity * model, BrowserNode * p)
    : BrowserNode(model->name, p), Labeled<BrowserActivity>(all, 0), associated_diagram(0)
{
    def = new ActivityData(model->def, this);
    comment = model->comment;
}

BrowserNode * BrowserActivity::duplicate(BrowserNode * p, QString name)
{
    BrowserNode * result = new BrowserActivity(this, p);

    result->set_name((name.isEmpty()) ? get_name() : name);
    result->update_stereotype(TRUE);

    return result;
}

BrowserActivity::~BrowserActivity()
{
    all.remove(get_ident());
    delete def;
}

void BrowserActivity::clear(bool old)
{
    all.clear(old);
    BrowserActivityNode::clear(old);
    BrowserParameter::clear(old);
    BrowserActivityAction::clear(old);
    BrowserInterruptibleActivityRegion::clear(old);
    BrowserExpansionRegion::clear(old);
    BrowserActivityObject::clear(old);
    BrowserActivityPartition::clear(old);
    BrowserFlow::clear(old);
}

void BrowserActivity::update_idmax_for_root()
{
    all.update_idmax_for_root();
    BrowserActivityNode::update_idmax_for_root();
    BrowserParameter::update_idmax_for_root();
    BrowserActivityAction::update_idmax_for_root();
    BrowserInterruptibleActivityRegion::update_idmax_for_root();
    BrowserExpansionRegion::update_idmax_for_root();
    BrowserActivityObject::update_idmax_for_root();
    BrowserActivityPartition::update_idmax_for_root();
    BrowserFlow::update_idmax_for_root();
}

void BrowserActivity::prepare_update_lib() const
{
    all.memo_id_oid(get_ident(), original_id);

    for (Q3ListViewItem * child = firstChild();
         child != 0;
         child = child->nextSibling())
        ((BrowserNode *) child)->prepare_update_lib();
}

void BrowserActivity::referenced_by(QList<BrowserNode *> & l, bool ondelete)
{
    BrowserNode::referenced_by(l, ondelete);

    if (! ondelete) {
        BrowserActivityAction::compute_referenced_by(l, this);
        BrowserActivityDiagram::compute_referenced_by(l, this, "activitycanvas", "activity_ref");
    }
}

// callers suppose this only take specification into acount
void BrowserActivity::compute_referenced_by(QList<BrowserNode *> & l,
        BrowserOperation * op)
{
    IdIterator<BrowserActivity> it(all);

    while (it.current()) {
        if (!it.current()->deletedp() &&
            (it.current()->def->get_specification() == op))
            l.append(it.current());

        ++it;
    }
}

void BrowserActivity::renumber(int phase)
{
    if (phase != -1)
        new_ident(phase, all);
}

const QPixmap * BrowserActivity::pixmap(int) const
{
    if (deletedp())
        return DeletedActivityIcon;

    const QPixmap * px = ProfiledStereotypes::browserPixmap(def->get_stereotype());

    return (px != 0) ? px : ActivityIcon;
}

BrowserActivity * BrowserActivity::add_activity(BrowserNode * future_parent)
{
    QString name;

    return (!future_parent->enter_child_name(name, TR("enter activity's name : "),
            UmlActivity, TRUE, FALSE))

           ? 0
           : add_activity(future_parent, name);
}

BrowserActivity * BrowserActivity::add_activity(BrowserNode * future_parent,
        const char * name)
{
    BrowserActivity * r = new BrowserActivity(name, future_parent);

    future_parent->setOpen(TRUE);
    future_parent->package_modified();

    return r;
}

BrowserActivity * BrowserActivity::get_activity(BrowserNode * parent)
{
    BrowserNodeList l;
    Q3ListViewItem * child;

    for (child = parent->firstChild(); child != 0; child = child->nextSibling())
        if (!((BrowserNode *) child)->deletedp() &&
            (((BrowserNode *) child)->get_type() == UmlActivity))
            l.append((BrowserNode *) child);

    BrowserNode * old;
    QString name;

    if (!parent->enter_child_name(name, TR("enter activity's name : "),
                                  UmlActivity, l, &old,
                                  TRUE, FALSE))
        return 0;

    if (old != 0)
        return ((BrowserActivity *) old);

    BrowserActivity * r = new BrowserActivity(name, parent);

    parent->setOpen(TRUE);
    parent->package_modified();

    return r;
}

BrowserNode * BrowserActivity::add_parameter(BrowserParameter * param)
{
    QString name;

    if (enter_child_name(name, TR("enter parameter's name : "),
                         UmlParameter, TRUE, FALSE)) {
        param = (param == 0) ? new BrowserParameter(name, this)
                : (BrowserParameter *) param->duplicate(this, name);

        setOpen(TRUE);
        def->modified();
        package_modified();
        param->select_in_browser();

        return param;
    }

    return 0;
}

void BrowserActivity::menu()
{
    MenuFactory builder(name);
    Q3PopupMenu toolm(0);

    builder.createTitle(def->definition(FALSE, TRUE));
    builder.insertSeparator();

    if (!deletedp()) {
        if (!is_read_only) {
            MenuFactory::Item items[] = {
                {
                    "New activity diagram",
                    0,
                    "to add a <i>activity diagram</i>"
                },
                {
                    "New parameter",
                    1,
                    "to add a <i>Parameter</i> to the <i>activity</i>"
                } ,
                {
                    "New interruptible activity region",
                    2,
                    "to add an <i>Interruptible Activity Region</i> to the <i>activity</i>"
                },
                {
                    "New expansion region",
                    3,
                    "to add a nested <i>expansion region</i>"
                },
                {
                    "New partition",
                    4,
                    "to add a <i>Partition</i> to the <i>activity</i>"
                },
                {
                    "New activity action",
                    7,
                    "to add an <i>activity action</i> to the <i>activity</i>"
                },
                {
                    "New object node",
                    8,
                    "to add an <i>activity object node</i> to the <i>activity</i>"
                }
            };
            builder.addItems(items, sizeof(items) / sizeof(MenuFactory::Item));
            builder.insertSeparator();
        }

        builder.addItem(
            "Edit",
            5,
            "to edit the <i>artivity</i>, \
a double click with the left mouse button does the same thing");

        if (!is_read_only) {
            builder.addItem(
                "Duplicate",
                6,
                "to copy the <i>activity</i> in a new one");
            builder.insertSeparator();

            if (edition_number == 0) {
                builder.addItem(
                    "Delete",
                    9,
                    "to delete the <i>activity</i>. \
Note that you can undelete it after");
            }
        }

        builder.insertSeparator();
        builder.addItem(
            "Referenced by",
            12,
            "to know who reference the <i>activity</i>");
        mark_menu(builder.menu(), TR("the activity"), 90);
        ProfiledStereotypes::menu(builder.menu(), this, 99990);

        if ((edition_number == 0) &&
            Tool::menu_insert(&toolm, get_type(), 100)) {
            builder.insertSeparator();
            builder.addItem("Tools", &toolm);
        }
    }
    else if (!is_read_only && (edition_number == 0)) {
        builder.addItem(
            "Undelete",
            10,
            "to undelete the <i>activity</i>");

        Q3ListViewItem * child;

        for (child = firstChild(); child != 0; child = child->nextSibling()) {
            if (((BrowserNode *) child)->deletedp()) {
                builder.addItem(
                    "Undelete recursively",
                    11,
                    "undelete the activity and its children");
                break;
            }
        }
    }

    exec_menu_choice(builder.menu().exec(QCursor::pos()));
}

void BrowserActivity::exec_menu_choice(int rank)
{
    switch (rank) {
    case 0: {
        BrowserActivityDiagram * d =
            BrowserActivityDiagram::add_activity_diagram(this);

        if (d == 0)
            return;

        d->select_in_browser();
    }
    break;

    case 1:
        add_parameter(0);
        return;

    case 2:
        BrowserInterruptibleActivityRegion::add_interruptibleactivityregion(this);
        break;

    case 3:
        BrowserExpansionRegion::add_expansionregion(this);
        break;

    case 4:
        BrowserActivityPartition::add_activitypartition(this);
        return;

    case 5:
        open(TRUE);
        return;

    case 6: {
        QString name;

        if (((BrowserNode *) parent())->enter_child_name(name, TR("enter activity's name : "),
                UmlActivity, TRUE, FALSE))
            duplicate((BrowserNode *) parent(), name)->select_in_browser();
    }
    break;

    case 7: {
        BrowserActivityAction * r =
            BrowserActivityAction::add_activityaction(this, 0);

        if (r != 0)
            r->select_in_browser();
    }

    return;

    case 8: {
        BrowserActivityObject * r =
            BrowserActivityObject::add_activityobject(this, 0);

        if (r != 0)
            r->select_in_browser();
    }

    return;

    case 9:
        delete_it();
        break;

    case 10:
        BrowserNode::undelete(FALSE);
        break;

    case 11:
        BrowserNode::undelete(TRUE);
        break;

    case 12:
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

void BrowserActivity::apply_shortcut(QString s)
{
    int choice = -1;

    if (!deletedp()) {
        if (!is_read_only) {
            if (s == "New activity diagram")
                choice = 0;
            else if (s == "New parameter")
                choice = 1;
            else if (s == "New interruptible activity region")
                choice = 2;
            else if (s == "New expansion region")
                choice = 3;
            else if (s == "New partition")
                choice = 4;
            else if (s == "New activity action")
                choice = 7;
            else if (s == "New object node")
                choice = 8;
        }

        if (s == "Edit")
            choice = 5;

        if (!is_read_only) {
            if (s == "Duplicate")
                choice = 6;

            if (edition_number == 0)
                if (s == "Delete")
                    choice = 9;
        }

        if (s == "Referenced by")
            choice = 12;

        mark_shortcut(s, choice, 90);

        if (edition_number == 0)
            Tool::shortcut(s, choice, get_type(), 100);
    }
    else if (!is_read_only && (edition_number == 0)) {
        if (s == "Undelete")
            choice = 10;

        Q3ListViewItem * child;

        for (child = firstChild(); child != 0; child = child->nextSibling()) {
            if (((BrowserNode *) child)->deletedp()) {
                if (s == "Undelete recursively")
                    choice = 11;

                break;
            }
        }
    }

    exec_menu_choice(choice);
}

void BrowserActivity::open(bool force_edit)
{
    if (!force_edit &&
        (associated_diagram != 0) &&
        !associated_diagram->deletedp())
        associated_diagram->open(FALSE);
    else if (!is_edited)
        def->edit();
}

void BrowserActivity::modified()
{
    repaint();
    ((BrowserNode *) parent())->modified();
    // to update activity drawing
    def->modified();
}

UmlCode BrowserActivity::get_type() const
{
    return UmlActivity;
}

QString BrowserActivity::get_stype() const
{
    return TR("activity");
}

int BrowserActivity::get_identifier() const
{
    return get_ident();
}

const char * BrowserActivity::help_topic() const
{
    return "activity";
}

BasicData * BrowserActivity::get_data() const
{
    return def;
}

QString BrowserActivity::full_name(bool rev, bool) const
{
    return fullname(rev);
}

BrowserNodeList & BrowserActivity::instances(BrowserNodeList & result, bool sort)
{
    IdIterator<BrowserActivity> it(all);
    BrowserActivity * a;

    while ((a = it.current()) != 0) {
        if (!a->deletedp())
            result.append(a);

        ++it;
    }

    if (sort)
        result.sort_it();

    return result;
}

BrowserNode * BrowserActivity::get_associated() const
{
    return associated_diagram;
}

void BrowserActivity::set_associated_diagram(BrowserActivityDiagram * dg,
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

void BrowserActivity::on_delete()
{
    if (associated_diagram && associated_diagram->deletedp())
        associated_diagram = 0;
}

void BrowserActivity::init()
{
    its_default_stereotypes.clear();

    BrowserParameter::init();
    BrowserActivityAction::init();
    BrowserActivityObject::init();
    BrowserActivityNode::init();
    BrowserInterruptibleActivityRegion::init();
    BrowserExpansionRegion::init();
    BrowserActivityPartition::init();
    BrowserFlow::init();
}

// unicode
const QStringList & BrowserActivity::default_stereotypes()
{
    return its_default_stereotypes;
}

bool BrowserActivity::api_compatible(unsigned v) const
{
    return (v > 24);
}

Q3ValueList<BrowserParameter *> BrowserActivity::get_params() const
{
    Q3ValueList<BrowserParameter *> l;
    Q3ListViewItem * child = firstChild();

    while (child != 0) {
        if (!((BrowserNode *) child)->deletedp() &&
            (((BrowserNode *) child)->get_type() == UmlParameter))
            l.append((BrowserParameter *) child);

        child = child->nextSibling();
    }

    return l;
}

bool BrowserActivity::tool_cmd(ToolCom * com, const char * args)
{
    switch ((unsigned char) args[-1]) {
    case createCmd: {
        bool ok = TRUE;

        if (is_read_only && !root_permission())
            ok = FALSE;
        else {
            UmlCode k = com->get_kind(args);

            switch (k) {
            case UmlActivityDiagram:
                if (wrong_child_name(args, UmlActivityDiagram, TRUE, FALSE))
                    ok = FALSE;
                else
                    (new BrowserActivityDiagram(args, this))->write_id(com);

                break;

            case UmlParameter:
                if (wrong_child_name(args, UmlParameter, TRUE, FALSE))
                    ok = FALSE;
                else
                    (new BrowserParameter(args, this))->write_id(com);

                break;

            case UmlInterruptibleActivityRegion:
                (new BrowserInterruptibleActivityRegion(args, this))->write_id(com);
                break;

            case UmlExpansionRegion:
                (new BrowserExpansionRegion(args, this))->write_id(com);
                break;

            case UmlActivityObject:
                (new BrowserActivityObject(args, this))->write_id(com);
                break;

            case UmlActivityPartition:
                (BrowserActivityPartition::add_activitypartition(this, args))->write_id(com);
                break;

            case UmlSimpleRelations: {
                UmlCode c;

                if (!com->get_relation_kind(c, args))
                    ok = FALSE;
                else {
                    BrowserNode * end = (BrowserNode *) com->get_id(args);

                    if (may_connect(end).isEmpty())
                        add_relation(c, end)->get_browser_node()->write_id(com);
                    else
                        ok = FALSE;
                }
            }
            break;

            default:
                if (IsaActivityNode(k))
                    (new BrowserActivityNode(k, args, this))->write_id(com);
                else if (IsaActivityAction(k))
                    (new BrowserActivityAction(args, this, (UmlActionKind)(k - UmlActivityAction)))->write_id(com);
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

    case setAssocDiagramCmd:
        if (is_read_only && !root_permission())
            com->write_ack(FALSE);
        else {
            set_associated_diagram((BrowserActivityDiagram *) com->get_id(args));
            com->write_ack(TRUE);
        }

        return TRUE;

    default:
        return (def->tool_cmd(com, args, this, comment) ||
                BrowserNode::tool_cmd(com, args));
    }
}

bool BrowserActivity::may_contains_them(const QList<BrowserNode *> & l,
                                        BooL & duplicable) const
{
    foreach (BrowserNode *node, l) {
        switch (node->get_type()) {
        case UmlInterruptibleActivityRegion:
        case UmlExpansionRegion:
        case UmlParameter:
        case UmlActivityDiagram:
        case UmlActivityAction:
        case UmlActivityObject:
        case UmlActivityPartition:
        case InitialAN:
        case FlowFinalAN:
        case ActivityFinalAN:
        case DecisionAN:
        case MergeAN:
        case ForkAN:
        case JoinAN:
        case UmlDependOn:
            return (((const BrowserNode *) node->get_container(UmlActivity)) == this);

        default:
            return FALSE;
        }

        if (! may_contains(node, FALSE))
            return FALSE;

        duplicable = may_contains_it(node);
    }

    return TRUE;
}

// connexion by a dependency
QString BrowserActivity::may_connect(const BrowserNode * dest) const
{
    switch (dest->get_type()) {
    case UmlPackage:
    case UmlActivity:
    case UmlActivityAction:
    case UmlActivityObject:
        return 0;

    default:
        return TR("illegal");
    }
}

void BrowserActivity::DragMoveEvent(QDragMoveEvent * e)
{
    if (UmlDrag::canDecode(e, BrowserParameter::drag_key(this)) ||
        UmlDrag::canDecode(e, BrowserActivityNode::drag_key(this)) ||
        UmlDrag::canDecode(e, BrowserActivityAction::drag_key(this)) ||
        UmlDrag::canDecode(e, BrowserActivityObject::drag_key(this)) ||
        UmlDrag::canDecode(e, BrowserInterruptibleActivityRegion::drag_key(this)) ||
        UmlDrag::canDecode(e, BrowserExpansionRegion::drag_key(this)) ||
        UmlDrag::canDecode(e, BrowserSimpleRelation::drag_key(this)) ||
        UmlDrag::canDecode(e, BrowserActivityPartition::drag_key(this)) ||
        UmlDrag::canDecode(e, BrowserActivityDiagram::drag_key(this))) {
        if (!is_read_only)
            e->accept();
        else
            e->ignore();
    }
    else
        ((BrowserNode *) parent())->DragMoveInsideEvent(e);
}

void BrowserActivity::DropEvent(QDropEvent * e)
{
    DropAfterEvent(e, 0);
}

void BrowserActivity::DragMoveInsideEvent(QDragMoveEvent * e)
{
    if (!is_read_only &&
        (UmlDrag::canDecode(e, BrowserParameter::drag_key(this)) ||
         UmlDrag::canDecode(e, BrowserActivityNode::drag_key(this)) ||
         UmlDrag::canDecode(e, BrowserActivityAction::drag_key(this)) ||
         UmlDrag::canDecode(e, BrowserActivityObject::drag_key(this)) ||
         UmlDrag::canDecode(e, BrowserInterruptibleActivityRegion::drag_key(this)) ||
         UmlDrag::canDecode(e, BrowserExpansionRegion::drag_key(this)) ||
         UmlDrag::canDecode(e, BrowserSimpleRelation::drag_key(this)) ||
         UmlDrag::canDecode(e, BrowserActivityPartition::drag_key(this)) ||
         UmlDrag::canDecode(e, BrowserActivityDiagram::drag_key(this))))
        e->accept();
    else
        e->ignore();
}

void BrowserActivity::DropAfterEvent(QDropEvent * e, BrowserNode * after)
{
    BrowserNode * bn;

    if ((((bn = UmlDrag::decode(e, BrowserParameter::drag_key(this))) != 0) ||
         ((bn = UmlDrag::decode(e, BrowserActivityNode::drag_key(this))) != 0) ||
         ((bn = UmlDrag::decode(e, BrowserActivityAction::drag_key(this))) != 0) ||
         ((bn = UmlDrag::decode(e, BrowserActivityObject::drag_key(this))) != 0) ||
         ((bn = UmlDrag::decode(e, BrowserInterruptibleActivityRegion::drag_key(this))) != 0) ||
         ((bn = UmlDrag::decode(e, BrowserExpansionRegion::drag_key(this))) != 0) ||
         ((bn = UmlDrag::decode(e, BrowserSimpleRelation::drag_key(this))) != 0) ||
         ((bn = UmlDrag::decode(e, BrowserActivityPartition::drag_key(this))) != 0) ||
         ((bn = UmlDrag::decode(e, BrowserActivityDiagram::drag_key(this))) != 0)) &&
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

void BrowserActivity::save_stereotypes(QTextStream & st)
{
    nl_indent(st);
    st << "activity_stereotypes ";
    save_unicode_string_list(its_default_stereotypes, st);
}

void BrowserActivity::read_stereotypes(char *& st, char *& k)
{
    if (!strcmp(k, "activity_stereotypes")) {
        read_unicode_string_list(its_default_stereotypes, st);
        k = read_keyword(st);
    }
    else
        init();
}

void BrowserActivity::save(QTextStream & st, bool ref, QString & warning)
{
    if (ref)
        st << "activity_ref " << get_ident() << " // " << get_name();
    else {
        nl_indent(st);
        st << "activity " << get_ident() << " ";
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

BrowserActivity * BrowserActivity::read_ref(char *& st)
{
    read_keyword(st, "activity_ref");

    int id = read_id(st);
    BrowserActivity * result = all[id];

    return (result == 0)
           ? new BrowserActivity(id)
           : result;
}

BrowserActivity * BrowserActivity::read(char *& st, char * k,
                                        BrowserNode * parent)
{
    BrowserActivity * result;
    int id;

    if (!strcmp(k, "activity_ref")) {
        id = read_id(st);
        result = all[id];

        return (result == 0)
               ? new BrowserActivity(id)
               : result;
    }
    else if (!strcmp(k, "activity")) {
        id = read_id(st);
        result = all[id];

        if (result == 0)
            result = new BrowserActivity(read_string(st), parent, id);
        else if (result->is_defined) {
            BrowserActivity * already_exist = result;

            result = new BrowserActivity(read_string(st), parent, id);

            already_exist->must_change_id(all);
            already_exist->unconsistent_fixed("activity", result);
        }
        else {
            result->set_parent(parent);
            result->set_name(read_string(st));
        }

        result->is_defined = TRUE;
        k = read_keyword(st);
        result->def->read(st, k);

        if (!strcmp(k, "associated_diagram")) {
            result->set_associated_diagram(BrowserActivityDiagram::read_ref(st, read_keyword(st)),
                                           TRUE);
            k = read_keyword(st);
        }

        result->BrowserNode::read(st, k, id);

        result->is_read_only = (!in_import() && read_only_file()) ||
                               ((user_id() != 0) && result->is_api_base());

        if (strcmp(k, "end")) {
            while (BrowserParameter::read(st, k, result) ||
                   BrowserActivityNode::read(st, k, result) ||
                   BrowserActivityAction::read(st, k, result) ||
                   BrowserActivityObject::read(st, k, result) ||
                   BrowserInterruptibleActivityRegion::read(st, k, result) ||
                   BrowserExpansionRegion::read(st, k, result) ||
                   BrowserActivityPartition::read(st, k, result) ||
                   BrowserActivityDiagram::read(st, k, result) ||
                   BrowserSimpleRelation::read(st, k, result))
                k = read_keyword(st);

            if (strcmp(k, "end"))
                wrong_keyword(k, "end");
        }

        return result;
    }
    else
        return 0;
}

BrowserNode * BrowserActivity::read_any_ref(char *& st, char * k)
{
    BrowserNode * r;

    if (((r = BrowserActivity::read(st, k, 0)) == 0) &&
        ((r = BrowserParameter::read(st, k, 0)) == 0) &&
        ((r = BrowserActivityNode::read(st, k, 0)) == 0) &&
        ((r = BrowserActivityAction::read(st, k, 0)) == 0) &&
        ((r = BrowserActivityObject::read(st, k, 0)) == 0) &&
        ((r = BrowserInterruptibleActivityRegion::read(st, k, 0)) == 0) &&
        ((r = BrowserExpansionRegion::read(st, k, 0)) == 0) &&
        ((r = BrowserFlow::read(st, k, 0)) == 0))
        r = BrowserActivityPartition::read(st, k, 0);

    return r;
}

BrowserNode * BrowserActivity::get_it(const char * k, int id)
{
    if (!strcmp(k, "activity_ref"))
        return all[id];

    BrowserNode * r;

    if (((r = BrowserParameter::get_it(k, id)) == 0) &&
        ((r = BrowserActivityNode::get_it(k, id)) == 0) &&
        ((r = BrowserActivityAction::get_it(k, id)) == 0) &&
        ((r = BrowserActivityObject::get_it(k, id)) == 0) &&
        ((r = BrowserInterruptibleActivityRegion::get_it(k, id)) == 0) &&
        ((r = BrowserExpansionRegion::get_it(k, id)) == 0) &&
        ((r = BrowserFlow::get_it(k, id)) == 0) &&
        ((r = BrowserSimpleRelation::get_it(k, id)) == 0) &&
        ((r = BrowserActivityPartition::get_it(k, id)) == 0)
       )
        r = BrowserActivityDiagram::get_it(k, id);

    return r;
}
