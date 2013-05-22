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

#include "BrowserActivityAction.h"
#include "ActivityActionData.h"
#include "BrowserActivityDiagram.h"
#include "BrowserStateDiagram.h"
#include "BrowserPin.h"
#include "BrowserParameterSet.h"
#include "PinData.h"
#include "ParameterData.h"
#include "ActivityObjectData.h"
#include "BrowserFlow.h"
#include "BrowserSimpleRelation.h"
#include "BrowserActivityNode.h"
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

IdDict<BrowserActivityAction> BrowserActivityAction::all(257, __FILE__);
QStringList BrowserActivityAction::its_default_stereotypes;	// unicode

BrowserActivityAction::BrowserActivityAction(QString s, BrowserNode * p, int id)
    : BrowserNode(s, p), Labeled<BrowserActivityAction>(all, id),
      def(new ActivityActionData), associated_diagram(0)
{
    def->set_browser_node(this);
}

BrowserActivityAction::BrowserActivityAction(int id)
    : BrowserNode(), Labeled<BrowserActivityAction>(all, id),
      def(new ActivityActionData), associated_diagram(0)
{
    // not yet read
    def->set_browser_node(this);
}

BrowserActivityAction::BrowserActivityAction(const BrowserActivityAction * model, BrowserNode * p)
    : BrowserNode(model->name, p), Labeled<BrowserActivityAction>(all, 0), associated_diagram(0)
{
    def = new ActivityActionData(model->def, this);
    comment = model->comment;
}

BrowserActivityAction::BrowserActivityAction(QString s, BrowserNode * p,
        UmlActionKind k)
    : BrowserNode(s, p), Labeled<BrowserActivityAction>(all, 0),
      def(new ActivityActionData(k)), associated_diagram(0)
{
    def->set_browser_node(this);
}

BrowserNode * BrowserActivityAction::duplicate(BrowserNode * p, QString name)
{
    BrowserNode * result = new BrowserActivityAction(this, p);

    result->set_name(name);
    result->update_stereotype();

    return result;
}

BrowserActivityAction::~BrowserActivityAction()
{
    all.remove(get_ident());
    delete def;
}

void BrowserActivityAction::clear(bool old)
{
    all.clear(old);
    BrowserParameterSet::clear(old);
    BrowserPin::clear(old);
}

void BrowserActivityAction::update_idmax_for_root()
{
    all.update_idmax_for_root();
    BrowserParameterSet::update_idmax_for_root();
    BrowserPin::update_idmax_for_root();
}

void BrowserActivityAction::prepare_update_lib() const
{
    all.memo_id_oid(get_ident(), original_id);

    for (Q3ListViewItem * child = firstChild();
         child != 0;
         child = child->nextSibling())
        ((BrowserNode *) child)->prepare_update_lib();
}

void BrowserActivityAction::referenced_by(QList<BrowserNode *> & l, bool ondelete)
{
    BrowserNode::referenced_by(l, ondelete);
    BrowserFlow::compute_referenced_by(l, this);

    if (! ondelete)
        BrowserActivityDiagram::compute_referenced_by(l, this, "activityactioncanvas", "activityaction_ref");
}

void BrowserActivityAction::compute_referenced_by(QList<BrowserNode *> & l,
        BrowserNode * target)
{
    IdIterator<BrowserActivityAction> it(all);

    while (it.current()) {
        if (!it.current()->deletedp()) {
            const char * what;
            BrowserNode * who =
                it.current()->def->get_action()->referenced(what);

            if (who == target)
                l.append(it.current());
        }

        ++it;
    }
}

void BrowserActivityAction::renumber(int phase)
{
    if (phase != -1)
        new_ident(phase, all);
}

const QPixmap * BrowserActivityAction::pixmap(int) const
{
    if (! deletedp()) {
        const QPixmap * px = ProfiledStereotypes::browserPixmap(def->get_stereotype());

        if (px != 0)
            return px;
    }

    switch (def->get_action_kind()) {
    case UmlAcceptEventAction:
        if (((AcceptEventAction *) def->get_action())->timeevent) {
            if (deletedp())
                return DeletedActivityAcceptTimeIcon;
            else if (is_marked)
                return ActivityAcceptTimeMarkedIcon;
            else
                return ActivityAcceptTimeIcon;
        }
        else {
            if (deletedp())
                return DeletedActivityAcceptEventIcon;
            else if (is_marked)
                return ActivityAcceptEventMarkedIcon;
            else
                return ActivityAcceptEventIcon;
        }

    case UmlSendSignalAction:
    case UmlBroadcastSignalAction:
        if (deletedp())
            return DeletedActivitySendSignalIcon;
        else if (is_marked)
            return ActivitySendSignalMarkedIcon;
        else
            return ActivitySendSignalIcon;

    default:
        if (deletedp())
            return DeletedActivityActionIcon;
        else if (is_marked)
            return ActivityActionMarkedIcon;
        else
            return ActivityActionIcon;
    }
}

BrowserNode *
BrowserActivityAction::add_parameter_set(BrowserParameterSet * paramset,
        const char * name)
{
    paramset = (paramset == 0) ? BrowserParameterSet::new_one(name, this)
               : (BrowserParameterSet *) paramset->duplicate(this, name);

    setOpen(TRUE);
    def->modified();
    package_modified();
    paramset->select_in_browser();

    return paramset;
}

// flow or dependency
BasicData * BrowserActivityAction::add_relation(UmlCode l, BrowserNode * end)
{
    if (l == UmlFlow) {
        BasicData * result = (new BrowserFlow(this, end))->get_data();

        modified();	// to update diagrams
        return result;
    }
    else
        return BrowserNode::add_relation(l, end);
}

// a flow/dependency may be added in all the cases
QString BrowserActivityAction::may_start() const
{
    return 0;
}

// connexion by a flow or a dependency
QString BrowserActivityAction::may_connect(UmlCode & l, const BrowserNode * dest) const
{
    switch (l) {
    case UmlFlow: {
        BrowserNode * container = dest->get_container(UmlActivity);

        if (container == 0)
            return TR("illegal");

        if (get_container(UmlActivity) != container)
            return TR("not in the same activity");

        const BrowserActivityElement * elt =
            dynamic_cast<const BrowserActivityElement *>(dest);

        return (elt == 0)
               ? TR("illegal")
               : elt->connexion_from(TRUE);
    }

    case UmlDependency:
        l = UmlDependOn;

        // no break;
    case UmlDependOn:
        switch (dest->get_type()) {
        case UmlPackage:
        case UmlActivity:
        case UmlActivityAction:
        case UmlActivityObject:
            return 0;

        default:
            return TR("illegal");
        }

    default:
        return TR("illegal");
    }
}

QString BrowserActivityAction::connexion_from(bool) const
{
    // theo all input and output must be control/data exclusively
    return 0;
}

BrowserActivityAction *
BrowserActivityAction::add_activityaction(BrowserNode * future_parent,
        const char * s)
{
    QString name;

    if (s != 0)
        name = s;
    else if (!future_parent->enter_child_name(name, TR("enter activity action's \nname (may be empty) : "),
             UmlActivityAction, TRUE, TRUE))
        return 0;

    BrowserActivityAction * r =
        new BrowserActivityAction(name, future_parent);

    future_parent->setOpen(TRUE);
    future_parent->package_modified();

    return r;
}

BrowserActivityAction * BrowserActivityAction::get_activityaction(BrowserNode * future_parent)
{
    BrowserNodeList l;
    Q3ListViewItem * child;

    for (child = future_parent->firstChild(); child != 0; child = child->nextSibling())
        if (!((BrowserNode *) child)->deletedp() &&
            (((BrowserNode *) child)->get_type() == UmlActivityAction) &&
            (((BrowserNode *) child)->get_name()[0] != 0))
            l.append((BrowserNode *) child);

    BrowserNode * old = 0;
    QString name;

    if (!future_parent->enter_child_name(name, TR("enter activity action's \nname (may be empty) : "),
                                         UmlActivityAction, l, &old, TRUE, TRUE))
        return 0;

    if (old != 0)
        return ((BrowserActivityAction *) old);

    BrowserActivityAction * r =
        new BrowserActivityAction(name, future_parent);

    future_parent->setOpen(TRUE);
    future_parent->package_modified();

    return r;
}

BrowserActivityAction *
BrowserActivityAction::add_call_behavior(BrowserNode * future_parent,
        BrowserNode * beh)
{
    QString name;

    if (!future_parent->enter_child_name(name, TR("enter activity action's \nname (may be empty) : "),
                                         UmlActivityAction, TRUE, TRUE))
        return 0;

    BrowserActivityAction * r =
        new BrowserActivityAction(name, future_parent, UmlCallBehaviorAction);

    ((CallBehaviorAction *)((ActivityActionData *) r->get_data())->get_action())
    ->set_behavior(beh);
    r->set_pins();
    future_parent->setOpen(TRUE);
    future_parent->package_modified();

    return r;
}

BrowserActivityAction *
BrowserActivityAction::add_call_operation(BrowserNode * future_parent,
        BrowserOperation * oper)
{
    QString name;

    if (!future_parent->enter_child_name(name, TR("enter activity action's \nname (may be empty) : "),
                                         UmlActivityAction, TRUE, TRUE))
        return 0;

    BrowserActivityAction * r =
        new BrowserActivityAction(name, future_parent, UmlCallOperationAction);


    ((CallOperationAction *)((ActivityActionData *) r->get_data())->get_action())
    ->set_operation(oper);
    r->set_pins();
    future_parent->setOpen(TRUE);
    future_parent->package_modified();

    return r;
}

BrowserActivityAction *
BrowserActivityAction::add_read_variable_value(BrowserNode * future_parent,
        BrowserNode * var)
{
    QString name;

    if (!future_parent->enter_child_name(name, TR("enter activity action's \nname (may be empty) : "),
                                         UmlActivityAction, TRUE, TRUE))
        return 0;

    BrowserActivityAction * r =
        new BrowserActivityAction(name, future_parent, UmlReadVariableValueAction);


    ((ReadVariableValueAction *)((ActivityActionData *) r->get_data())->get_action())
    ->set_variable(var);
    r->set_pins();
    future_parent->setOpen(TRUE);
    future_parent->package_modified();

    return r;
}

BrowserActivityAction *
BrowserActivityAction::add_clear_variable_value(BrowserNode * future_parent,
        BrowserNode * var)
{
    QString name;

    if (!future_parent->enter_child_name(name, TR("enter activity action's \nname (may be empty) : "),
                                         UmlActivityAction, TRUE, TRUE))
        return 0;

    BrowserActivityAction * r =
        new BrowserActivityAction(name, future_parent, UmlClearVariableValueAction);


    ((ClearVariableValueAction *)((ActivityActionData *) r->get_data())->get_action())
    ->set_variable(var);
    r->set_pins();
    future_parent->setOpen(TRUE);
    future_parent->package_modified();

    return r;
}

BrowserActivityAction *
BrowserActivityAction::add_write_variable_value(BrowserNode * future_parent,
        BrowserNode * var)
{
    QString name;

    if (!future_parent->enter_child_name(name, TR("enter activity action's \nname (may be empty) : "),
                                         UmlActivityAction, TRUE, TRUE))
        return 0;

    BrowserActivityAction * r =
        new BrowserActivityAction(name, future_parent, UmlWriteVariableValueAction);


    ((WriteVariableValueAction *)((ActivityActionData *) r->get_data())->get_action())
    ->set_variable(var);
    r->set_pins();
    future_parent->setOpen(TRUE);
    future_parent->package_modified();

    return r;
}

BrowserActivityAction *
BrowserActivityAction::add_add_variable_value(BrowserNode * future_parent,
        BrowserNode * var)
{
    QString name;

    if (!future_parent->enter_child_name(name, TR("enter activity action's \nname (may be empty) : "),
                                         UmlActivityAction, TRUE, TRUE))
        return 0;

    BrowserActivityAction * r =
        new BrowserActivityAction(name, future_parent, UmlAddVariableValueAction);

    ((AddVariableValueAction *)((ActivityActionData *) r->get_data())->get_action())
    ->set_variable(var);

    r->set_pins();
    future_parent->setOpen(TRUE);
    future_parent->package_modified();

    return r;
}

BrowserActivityAction *
BrowserActivityAction::add_remove_variable_value(BrowserNode * future_parent,
        BrowserNode * var)
{
    QString name;

    if (!future_parent->enter_child_name(name, TR("enter activity action's \nname (may be empty) : "),
                                         UmlActivityAction, TRUE, TRUE))
        return 0;

    BrowserActivityAction * r =
        new BrowserActivityAction(name, future_parent, UmlRemoveVariableValueAction);

    ((RemoveVariableValueAction *)((ActivityActionData *) r->get_data())->get_action())
    ->set_variable(var);
    r->set_pins();
    future_parent->setOpen(TRUE);
    future_parent->package_modified();

    return r;
}

void BrowserActivityAction::menu()
{
    QString s = name;
    BrowserNode * who = 0;
    QString kind = stringify(def->get_action_kind());
    int index = 0;

    while ((index = kind.find("_")) != -1)
        kind.replace(index, 1, " ");

    Q3PopupMenu m(0, "Activity action");
    Q3PopupMenu toolm(0);

    MenuFactory::createTitle(m, def->definition(FALSE, TRUE));
    m.insertSeparator();

    if (!deletedp()) {
        if (!is_read_only) {
            if (def->may_add_pin())
                m.setWhatsThis(m.insertItem(TR("New pin"), 0),
                               TR("to add a <i>pin</i>"));

            m.setWhatsThis(m.insertItem(TR("New parameter set"), 7),
                           TR("to add a <i>Parameter Set</i>"));
            m.insertSeparator();
        }

        m.setWhatsThis(m.insertItem(TR("Edit"), 1),
                       TR("to edit the <i>" + kind + "</i>, \
a double click with the left mouse button does the same thing"));

        if (!is_read_only) {
            m.setWhatsThis(m.insertItem(TR("Duplicate"), 2),
                           TR("to copy the <i>" + kind + "</i> in a new one"));
        }

        const char * what;

        if ((who = def->get_action()->referenced(what)) != 0) {
            m.insertSeparator();
            m.insertItem(TR(QString("Select ") + what), 8);
        }

        if (!is_read_only) {
            m.insertSeparator();

            if (edition_number == 0)
                m.setWhatsThis(m.insertItem(TR("Delete"), 3),
                               TR("to delete the <i>" + kind + "</i>. \
Note that you can undelete it after"));
        }

        m.setWhatsThis(m.insertItem(TR("Referenced by"), 4),
                       TR("to know who reference the <i>" + kind + "</i> \
through a flow or dependency"));
        mark_menu(m, TR("the " + kind), 90);
        ProfiledStereotypes::menu(m, this, 99990);

        if ((edition_number == 0) &&
            Tool::menu_insert(&toolm, get_type(), 100)) {
            m.insertSeparator();
            m.insertItem(TR("Tool"), &toolm);
        }
    }
    else if (!is_read_only && (edition_number == 0)) {
        m.setWhatsThis(m.insertItem(TR("Undelete"), 5),
                       TR("to undelete the <i>" + kind + "</i>"));

        Q3ListViewItem * child;

        for (child = firstChild(); child != 0; child = child->nextSibling()) {
            if (((BrowserNode *) child)->deletedp()) {
                m.setWhatsThis(m.insertItem(TR("Undelete recursively"), 6),
                               TR("to undelete the <i>" + kind + "</i> and its <i>pins</i> and \
<i>flows</i> or <i>dependencies</i> (except if the element at the other side is also deleted)"));
                break;
            }
        }
    }

    exec_menu_choice(m.exec(QCursor::pos()), who);
}

void BrowserActivityAction::exec_menu_choice(int rank,
        BrowserNode * who)
{
    switch (rank) {
    case 0:
        if (BrowserPin::add_pin(0, this) != 0)
            modified();

        return;

    case 7:
        add_parameter_set(0, 0);
        return;

    case 1:
        open(TRUE);
        return;

    case 2: {
        QString name;

        if (((BrowserNode *) parent())->enter_child_name(name, TR("enter activity action's \nname (may be empty) : "),
                get_type(), TRUE, TRUE))
            duplicate((BrowserNode *) parent(), name)->select_in_browser();
    }
    break;

    case 3:
        delete_it();
        break;

    case 4:
        ReferenceDialog::show(this);
        return;

    case 5:
        BrowserNode::undelete(FALSE);
        break;

    case 6:
        BrowserNode::undelete(TRUE);
        break;

    case 8:
        who->select_in_browser();
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

void BrowserActivityAction::apply_shortcut(QString s)
{
    int choice = -1;

    if (!deletedp()) {
        if (!is_read_only) {
            if (def->may_add_pin())
                if (s == "New pin")
                    choice = 0;

            if (s == "New parameter set")
                choice = 7;
        }

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
            choice = 4;

        mark_shortcut(s, choice, 90);

        if (edition_number == 0)
            Tool::shortcut(s, choice, get_type(), 100);
    }
    else if (!is_read_only && (edition_number == 0)) {
        if (s == "Undelete")
            choice = 5;

        Q3ListViewItem * child;

        for (child = firstChild(); child != 0; child = child->nextSibling()) {
            if (((BrowserNode *) child)->deletedp()) {
                if (s == "Undelete recursively")
                    choice = 6;

                break;
            }
        }
    }

    exec_menu_choice(choice, 0);
}

void BrowserActivityAction::open(bool force_edit)
{
    if (!force_edit &&
        (associated_diagram != 0) &&
        !associated_diagram->deletedp())
        associated_diagram->open(FALSE);
    else if (!is_edited)
        def->edit();
}

void BrowserActivityAction::modified()
{
    repaint();
    ((BrowserNode *) parent())->modified();
    // to update drawing
    def->modified();
}

UmlCode BrowserActivityAction::get_type() const
{
    return UmlActivityAction;
}

QString BrowserActivityAction::get_stype() const
{
    QString s = stringify(def->get_action_kind());
    int index = 0;

    while ((index = s.find("_")) != -1)
        s.replace(index, 1, " ");

    return TR(s);
}

int BrowserActivityAction::get_identifier() const
{
    return get_ident();
}

const char * BrowserActivityAction::help_topic() const
{
    return "activityaction";
}

BasicData * BrowserActivityAction::get_data() const
{
    return def;
}

bool BrowserActivityAction::allow_empty() const
{
    return TRUE;
}

QString BrowserActivityAction::full_name(bool rev, bool) const
{
    QString s = name;

    if (s.isEmpty())
        s = stringify(def->get_action_kind());

    return fullname(s, rev);
}

BrowserNode * BrowserActivityAction::get_associated() const
{
    return associated_diagram;
}

void BrowserActivityAction::set_associated_diagram(BrowserNode * dg,
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

void BrowserActivityAction::on_delete()
{
    if (associated_diagram && associated_diagram->deletedp())
        associated_diagram = 0;
}

void BrowserActivityAction::init()
{
    its_default_stereotypes.clear();

    BrowserParameterSet::init();
    BrowserPin::init();
}

// unicode
const QStringList & BrowserActivityAction::default_stereotypes()
{
    return its_default_stereotypes;
}

bool BrowserActivityAction::api_compatible(unsigned v) const
{
    return ((v > 43) ||
            ((v > 24) &&
             (def->get_action_kind() <= UmlValueSpecificationAction)));
}

Q3ValueList<BrowserPin *> BrowserActivityAction::get_pins() const
{
    Q3ValueList<BrowserPin *> l;
    Q3ListViewItem * child = firstChild();

    while (child != 0) {
        if (!((BrowserNode *) child)->deletedp() &&
            (((BrowserNode *) child)->get_type() == UmlActivityPin))
            l.append((BrowserPin *) child);

        child = child->nextSibling();
    }

    return l;
}

void BrowserActivityAction::remove_pins()
{
    Q3ListViewItem * child = firstChild();

    while (child != 0) {
        if (!((BrowserNode *) child)->deletedp() &&
            (((BrowserNode *) child)->get_type() == UmlActivityPin))
            ((BrowserPin *) child)->delete_it();

        child = child->nextSibling();
    }
}

void BrowserActivityAction::set_pins()
{
    remove_pins();

    Q3ValueList<PinDescr> l = def->get_action()->pins();
    Q3ValueList<PinDescr>::ConstIterator iter;

    for (iter = l.begin(); iter != l.end(); ++iter) {
        const PinDescr & pd = *iter;

        BrowserPin::new_one(this, pd.name, (const PinParamData &) pd);
    }
}

Q3ValueList<BrowserParameterSet *> BrowserActivityAction::get_parametersets() const
{
    Q3ValueList<BrowserParameterSet *> l;
    Q3ListViewItem * child = firstChild();

    while (child != 0) {
        if (!((BrowserNode *) child)->deletedp() &&
            (((BrowserNode *) child)->get_type() == UmlParameterSet))
            l.append((BrowserParameterSet *) child);

        child = child->nextSibling();
    }

    return l;
}

void BrowserActivityAction::write_id(ToolCom * com)
{
    com->write_id(this,
                  UmlActivityAction - UmlRelations + def->get_action_kind(),
                  name);
}

bool BrowserActivityAction::tool_cmd(ToolCom * com, const char * args)
{
    switch ((unsigned char) args[-1]) {
    case createCmd: {
        bool ok = TRUE;

        if (is_read_only && !root_permission())
            ok = FALSE;
        else {
            UmlCode k = com->get_kind(args);

            switch (k) {
            case UmlParameterSet:
                add_parameter_set(0, args)->write_id(com);
                break;

            case UmlFlow: {
                BrowserNode * end = (BrowserNode *) com->get_id(args);

                if (may_connect(k, end).isEmpty())
                    (new BrowserFlow(this, end))->write_id(com);
                else
                    ok = FALSE;
            }
            break;

            case UmlSimpleRelations: {
                UmlCode c;

                if (!com->get_relation_kind(c, args))
                    ok = FALSE;
                else {
                    BrowserNode * end = (BrowserNode *) com->get_id(args);

                    if (may_connect(c, end).isEmpty())
                        add_relation(c, end)->get_browser_node()->write_id(com);
                    else
                        ok = FALSE;
                }
            }
            break;

            case UmlActivityPin:
                BrowserPin::new_one(this, args)->write_id(com);
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

    case setAssocDiagramCmd:
        if (is_read_only && !root_permission())
            com->write_ack(FALSE);
        else {
            set_associated_diagram((BrowserNode *) com->get_id(args));
            com->write_ack(TRUE);
        }

        return TRUE;

    default:
        return (def->tool_cmd(com, args, this, comment) ||
                BrowserNode::tool_cmd(com, args));
    }
}

bool BrowserActivityAction::may_contains_them(const QList<BrowserNode *> & l,
        BooL & duplicable) const
{
    foreach (BrowserNode *node, l) {
        switch (node->get_type()) {
        case UmlParameterSet:
        case UmlActivityPin:
        case UmlFlow:
        case UmlDependOn:
            if (((const BrowserNode *) node->parent()) != this)
                return FALSE;

            break;

        default:
            return FALSE;
        }

        if (! may_contains(node, FALSE))
            return FALSE;

        duplicable = may_contains_it(node);
    }

    return TRUE;
}

void BrowserActivityAction::DragMoveEvent(QDragMoveEvent * e)
{
    if (UmlDrag::canDecode(e, BrowserFlow::drag_key(this)) ||
        UmlDrag::canDecode(e, BrowserSimpleRelation::drag_key(this)) ||
        UmlDrag::canDecode(e, BrowserPin::drag_key(this)) ||
        UmlDrag::canDecode(e, BrowserParameterSet::drag_key(this))) {
        if (!is_read_only)
            e->accept();
        else
            e->ignore();
    }
    else
        ((BrowserNode *) parent())->DragMoveInsideEvent(e);
}

void BrowserActivityAction::DropEvent(QDropEvent * e)
{
    DropAfterEvent(e, 0);
}

void BrowserActivityAction::DragMoveInsideEvent(QDragMoveEvent * e)
{
    if (!is_read_only &&
        (UmlDrag::canDecode(e, BrowserFlow::drag_key(this)) ||
         UmlDrag::canDecode(e, BrowserSimpleRelation::drag_key(this)) ||
         UmlDrag::canDecode(e, BrowserPin::drag_key(this)) ||
         UmlDrag::canDecode(e, BrowserParameterSet::drag_key(this))))
        e->accept();
    else
        e->ignore();
}

void BrowserActivityAction::DropAfterEvent(QDropEvent * e, BrowserNode * after)
{
    BrowserNode * bn;

    if ((((bn = UmlDrag::decode(e, BrowserFlow::drag_key(this))) != 0) ||
         ((bn = UmlDrag::decode(e, BrowserSimpleRelation::drag_key(this))) != 0) ||
         ((bn = UmlDrag::decode(e, BrowserPin::drag_key(this))) != 0) ||
         ((bn = UmlDrag::decode(e, BrowserParameterSet::drag_key(this))) != 0)) &&
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

QString BrowserActivityAction::drag_key() const
{
    return QString::number(UmlActivityAction)
           + "#" + QString::number((unsigned long) get_container(UmlActivity));
}

QString BrowserActivityAction::drag_postfix() const
{
    return "#" + QString::number((unsigned long) get_container(UmlActivity));
}

QString BrowserActivityAction::drag_key(BrowserNode * p)
{
    return QString::number(UmlActivityAction)
           + "#" + QString::number((unsigned long) p->get_container(UmlActivity));
}

void BrowserActivityAction::save_stereotypes(QTextStream & st)
{
    nl_indent(st);
    st << "activityaction_stereotypes ";
    save_unicode_string_list(its_default_stereotypes, st);
}

void BrowserActivityAction::read_stereotypes(char *& st, char *& k)
{
    if (!strcmp(k, "activityaction_stereotypes")) {
        read_unicode_string_list(its_default_stereotypes, st);
        k = read_keyword(st);
    }
    else
        init();
}

void BrowserActivityAction::save(QTextStream & st, bool ref, QString & warning)
{
    if (ref) {
        st << "activityaction_ref " << get_ident()
           << " // activity action " << get_name();
    }
    else {
        nl_indent(st);
        st << "activityaction " << get_ident() << " ";
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

BrowserActivityAction * BrowserActivityAction::read_ref(char *& st)
{
    read_keyword(st, "activityaction_ref");

    int id = read_id(st);
    BrowserActivityAction * result = all[id];

    return (result == 0)
           ? new BrowserActivityAction(id)
           : result;
}

BrowserActivityAction * BrowserActivityAction::read(char *& st, char * k,
        BrowserNode * parent)
{
    BrowserActivityAction * result;
    int id;

    if (!strcmp(k, "activityaction_ref")) {
        id = read_id(st);
        result = all[id];

        return (result == 0)
               ? new BrowserActivityAction(id)
               : result;
    }
    else if (!strcmp(k, "activityaction")) {
        id = read_id(st);
        result = all[id];

        if (result == 0)
            result = new BrowserActivityAction(read_string(st), parent, id);
        else if (result->is_defined) {
            BrowserActivityAction * already_exist = result;

            result = new BrowserActivityAction(read_string(st), parent, id);

            already_exist->must_change_id(all);
            already_exist->unconsistent_fixed("activity action", result);
        }
        else {
            result->set_parent(parent);
            result->set_name(read_string(st));
        }

        result->is_defined = TRUE;
        k = read_keyword(st);
        result->def->read(st, k);

        if (!strcmp(k, "associated_diagram")) {
            k = read_keyword(st);

            BrowserNode * diag;

            if (((diag = BrowserActivityDiagram::read(st, k, 0)) == 0) &&
                ((diag = BrowserStateDiagram::read(st, k, 0)) == 0))
                wrong_keyword(k, "state/activity diagram ref");

            result->set_associated_diagram(diag, TRUE);
            k = read_keyword(st);
        }

        result->BrowserNode::read(st, k, id);

        // was not done because of internal action not read soon
        result->update_stereotype(FALSE);


        result->is_read_only = (!in_import() && read_only_file()) ||
                               ((user_id() != 0) && result->is_api_base());

        if (strcmp(k, "end")) {
            while (BrowserPin::read(st, k, result) ||
                   BrowserParameterSet::read(st, k, result) ||
                   BrowserFlow::read(st, k, result) ||
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

BrowserNode * BrowserActivityAction::get_it(const char * k, int id)
{
    // flow managed in activity
    if (!strcmp(k, "activityaction_ref"))
        return all[id];

    BrowserNode * r;

    if ((r = BrowserPin::get_it(k, id)) == 0)
        r = BrowserParameterSet::get_it(k, id);

    return r;
}
