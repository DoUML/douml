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

#include "BrowserExpansionRegion.h"
#include "ReferenceDialog.h"
#include "ExpansionRegionData.h"
#include "BrowserExpansionNode.h"
#include "BrowserActivityNode.h"
#include "BrowserActivityAction.h"
#include "BrowserActivityObject.h"
#include "BrowserInterruptibleActivityRegion.h"
#include "ActivityActionData.h"
#include "BrowserActivityDiagram.h"
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

IdDict<BrowserExpansionRegion> BrowserExpansionRegion::all(__FILE__);
QStringList BrowserExpansionRegion::its_default_stereotypes;	// unicode

BrowserExpansionRegion::BrowserExpansionRegion(QString s, BrowserNode * p, int id)
    : BrowserNode(s, p), Labeled<BrowserExpansionRegion>(all, id),
      def(new ExpansionRegionData), associated_diagram(0)
{
    def->set_browser_node(this);
}

BrowserExpansionRegion::BrowserExpansionRegion(int id)
    : BrowserNode(), Labeled<BrowserExpansionRegion>(all, id),
      def(new ExpansionRegionData), associated_diagram(0)
{
    // not yet read
    def->set_browser_node(this);
}

BrowserExpansionRegion::BrowserExpansionRegion(const BrowserExpansionRegion * model,
        BrowserNode * p)
    : BrowserNode(model->name, p), Labeled<BrowserExpansionRegion>(all, 0),
      associated_diagram(0)
{
    def = new ExpansionRegionData(model->def, this);
    comment = model->comment;
}

BrowserNode * BrowserExpansionRegion::duplicate(BrowserNode * p, QString name)
{
    BrowserNode * result = new BrowserExpansionRegion(this, p);

    result->set_name(name);
    result->update_stereotype(TRUE);

    return result;
}

BrowserExpansionRegion::~BrowserExpansionRegion()
{
    all.remove(get_ident());
    delete def;
}

void BrowserExpansionRegion::clear(bool old)
{
    all.clear(old);
    BrowserExpansionNode::clear(old);
}

void BrowserExpansionRegion::update_idmax_for_root()
{
    all.update_idmax_for_root();
    BrowserExpansionNode::update_idmax_for_root();
}

void BrowserExpansionRegion::prepare_update_lib() const
{
    all.memo_id_oid(get_ident(), original_id);

    for (Q3ListViewItem * child = firstChild();
         child != 0;
         child = child->nextSibling())
        ((BrowserNode *) child)->prepare_update_lib();
}

void BrowserExpansionRegion::referenced_by(QList<BrowserNode *> & l, bool ondelete)
{
    BrowserNode::referenced_by(l, ondelete);

    if (! ondelete)
        BrowserActivityDiagram::compute_referenced_by(l, this, "expansionregioncanvas", "expansionregion_ref");
}

void BrowserExpansionRegion::renumber(int phase)
{
    if (phase != -1)
        new_ident(phase, all);
}

const QPixmap * BrowserExpansionRegion::pixmap(int) const
{
    if (deletedp())
        return DeletedExpansionRegionIcon;

    const QPixmap * px = ProfiledStereotypes::browserPixmap(def->get_stereotype());

    if (px != 0)
        return px;
    else if (is_marked)
        return ExpansionRegionMarkedIcon;
    else
        return ExpansionRegionIcon;
}

BrowserExpansionRegion *
BrowserExpansionRegion::add_expansionregion(BrowserNode * future_parent)
{
    QString name;

    return (!future_parent->enter_child_name(name, TR("enter expansion region's \nname (may be empty) : "),
            UmlExpansionRegion, TRUE, TRUE))

           ? 0
           : add_expansionregion(future_parent, name);
}

BrowserExpansionRegion *
BrowserExpansionRegion::add_expansionregion(BrowserNode * future_parent,
        const char * name)
{
    BrowserExpansionRegion * r = new BrowserExpansionRegion(name, future_parent);

    future_parent->setOpen(TRUE);
    future_parent->package_modified();

    return r;
}

BrowserExpansionRegion *
BrowserExpansionRegion::get_expansionregion(BrowserNode * parent)
{
    BrowserNodeList l;
    Q3ListViewItem * child;

    for (child = parent->firstChild(); child != 0; child = child->nextSibling())
        if (!((BrowserNode *) child)->deletedp() &&
            (((BrowserNode *) child)->get_type() == UmlExpansionRegion))
            l.append((BrowserNode *) child);

    BrowserNode * old;
    QString name;

    if (!parent->enter_child_name(name, TR("enter expansion region's \nname (may be empty) : "),
                                  UmlExpansionRegion, l, &old,
                                  TRUE, TRUE))
        return 0;

    return (old != 0)
           ? ((BrowserExpansionRegion *) old)
           : add_expansionregion(parent, name);
}

void BrowserExpansionRegion::menu()
{
    Q3PopupMenu m(0, name);
    Q3PopupMenu toolm(0);

    MenuFactory::createTitle(m, def->definition(FALSE, TRUE));
    m.insertSeparator();

    if (!deletedp()) {
        if (!is_read_only) {
            m.setWhatsThis(m.insertItem(TR("New expansion node"), 0),
                           TR("to add an <i>expansion node</i> to the <i>expansion region</i>"));
            m.setWhatsThis(m.insertItem(TR("New nested expansion region"), 1),
                           TR("to add a nested <i>expansion region</i>"));
            m.setWhatsThis(m.insertItem(TR("New interruptible activity region"), 2),
                           TR("to add an <i>interruptible activity region</i>"));
            m.setWhatsThis(m.insertItem(TR("Add activity action"), 6),
                           TR("to add an <i>activity action</i> to the <i>region</i>"));
            m.setWhatsThis(m.insertItem(TR("Add object node"), 7),
                           TR("to add an <i>activity object node</i> to the <i>region</i>"));
            m.insertSeparator();
        }

        m.setWhatsThis(m.insertItem(TR("Edit"), 4),
                       TR("to edit the <i>expansion region</i>, \
a double click with the left mouse button does the same thing"));

        if (!is_read_only) {
            m.setWhatsThis(m.insertItem(TR("Duplicate"), 5),
                           TR("to copy the <i>expansion region</i> in a new one"));
            m.insertSeparator();

            if (edition_number == 0)
                m.setWhatsThis(m.insertItem(TR("Delete"), 8),
                               TR("to delete the <i>expansion region</i>. \
Note that you can undelete it after"));
        }

        m.setWhatsThis(m.insertItem(TR("Referenced by"), 3),
                       TR("to know who reference the <i>region</i>"));
        mark_menu(m, TR("the expansion region"), 90);
        ProfiledStereotypes::menu(m, this, 99990);

        if ((edition_number == 0) &&
            Tool::menu_insert(&toolm, get_type(), 100)) {
            m.insertSeparator();
            m.insertItem(TR("Tool"), &toolm);
        }
    }
    else if (!is_read_only && (edition_number == 0)) {
        m.setWhatsThis(m.insertItem(TR("Undelete"), 10),
                       TR("to undelete the <i>expansion region</i>"));

        Q3ListViewItem * child;

        for (child = firstChild(); child != 0; child = child->nextSibling()) {
            if (((BrowserNode *) child)->deletedp()) {
                m.setWhatsThis(m.insertItem(TR("Undelete recursively"), 11),
                               TR("undelete the expansion region and its children"));
                break;
            }
        }
    }

    exec_menu_choice(m.exec(QCursor::pos()));
}

void BrowserExpansionRegion::exec_menu_choice(int rank)
{
    switch (rank) {
    case 0:
        add_expansionnode();
        return;

    case 1:
        add_expansionregion(this);
        break;

    case 2:
        BrowserInterruptibleActivityRegion::add_interruptibleactivityregion(this);
        break;

    case 3:
        ReferenceDialog::show(this);
        return;

    case 4:
        open(TRUE);
        return;

    case 5: {
        QString name;

        if (((BrowserNode *) parent())->enter_child_name(name, TR("enter expansion region's \nname (may be empty) : "),
                UmlExpansionRegion, TRUE, TRUE))
            duplicate((BrowserNode *) parent(), name)->select_in_browser();
    }
    break;

    case 6: {
        BrowserActivityAction * r =
            BrowserActivityAction::add_activityaction(this, 0);

        if (r != 0)
            r->select_in_browser();
    }

    return;

    case 7: {
        BrowserActivityObject * r =
            BrowserActivityObject::add_activityobject(this, 0);

        if (r != 0)
            r->select_in_browser();
    }

    return;

    case 8:
        delete_it();
        break;

        /*
        case 9:
        ReferenceDialog::show(this);
        return;
        */
    case 10:
        BrowserNode::undelete(FALSE);
        break;

    case 11:
        BrowserNode::undelete(TRUE);
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

void BrowserExpansionRegion::apply_shortcut(QString s)
{
    int choice = -1;

    if (!deletedp()) {
        if (!is_read_only) {
            if (s == "New expansion node")
                choice = 0;
            else if (s == "New expansion region")
                choice = 1;
            else if (s == "New interruptible activity region")
                choice = 2;
            else if (s == "Add activity action")
                choice = 6;
            else if (s == "Add object node")
                choice = 7;
        }

        if (s == "Referenced by")
            choice = 3;

        if (s == "Edit")
            choice = 4;

        if (!is_read_only) {
            if (s == "Duplicate")
                choice = 5;

            if (edition_number == 0)
                if (s == "Delete")
                    choice = 8;
        }

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

BrowserNode *
BrowserExpansionRegion::add_expansionnode()
{
    QString name;

    if (enter_child_name(name, TR("enter expansion node's \nname (may be empty) : "),
                         UmlExpansionNode, TRUE, TRUE)) {
        BrowserExpansionNode * node = new BrowserExpansionNode(name, this);

        setOpen(TRUE);
        def->modified();
        package_modified();
        node->select_in_browser();

        return node;
    }

    return 0;
}

void BrowserExpansionRegion::open(bool force_edit)
{
    if (!force_edit &&
        (associated_diagram != 0) &&
        !associated_diagram->deletedp())
        associated_diagram->open(FALSE);
    else if (!is_edited)
        def->edit();
}

void BrowserExpansionRegion::modified()
{
    repaint();
    ((BrowserNode *) parent())->modified();
    // to update drawing
    def->modified();
}

UmlCode BrowserExpansionRegion::get_type() const
{
    return UmlExpansionRegion;
}

QString BrowserExpansionRegion::get_stype() const
{
    return TR("expansion region");
}

int BrowserExpansionRegion::get_identifier() const
{
    return get_ident();
}

const char * BrowserExpansionRegion::help_topic() const
{
    return "activityregion";
}

BasicData * BrowserExpansionRegion::get_data() const
{
    return def;
}

bool BrowserExpansionRegion::allow_empty() const
{
    return TRUE;
}

QString BrowserExpansionRegion::full_name(bool rev, bool) const
{
    QString s = name;

    if (s.isEmpty())
        s = "<anonymous expansion region>";

    return fullname(s, rev);
}

BrowserNode * BrowserExpansionRegion::get_associated() const
{
    return associated_diagram;
}

void BrowserExpansionRegion::set_associated_diagram(BrowserActivityDiagram * dg,
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

void BrowserExpansionRegion::on_delete()
{
    if (associated_diagram && associated_diagram->deletedp())
        associated_diagram = 0;
}

void BrowserExpansionRegion::init()
{
    its_default_stereotypes.clear();

    BrowserExpansionNode::init();
}

// unicode
const QStringList & BrowserExpansionRegion::default_stereotypes()
{
    return its_default_stereotypes;
}

bool BrowserExpansionRegion::api_compatible(unsigned v) const
{
    return (v > 24);
}

Q3ValueList<BrowserExpansionNode *> BrowserExpansionRegion::get_nodes() const
{
    Q3ValueList<BrowserExpansionNode *> l;
    Q3ListViewItem * child = firstChild();

    while (child != 0) {
        if (!((BrowserNode *) child)->deletedp() &&
            (((BrowserNode *) child)->get_type() == UmlExpansionNode))
            l.append((BrowserExpansionNode *) child);

        child = child->nextSibling();
    }

    return l;
}

bool BrowserExpansionRegion::tool_cmd(ToolCom * com, const char * args)
{
    switch ((unsigned char) args[-1]) {
    case createCmd: {
        bool ok = TRUE;

        if (is_read_only && !root_permission())
            ok = FALSE;
        else {
            UmlCode k = com->get_kind(args);

            switch (k) {
            case UmlExpansionNode:
                if (wrong_child_name(args, UmlExpansionNode, TRUE, TRUE))
                    ok = FALSE;
                else
                    (new BrowserExpansionNode(args, this))->write_id(com);

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

bool BrowserExpansionRegion::may_contains_them(const QList<BrowserNode *> & l,
        BooL & duplicable) const
{
    BrowserNode * activity = get_container(UmlActivity);

    foreach (BrowserNode *node, l) {
        switch (node->get_type()) {
        case UmlInterruptibleActivityRegion:
        case UmlExpansionRegion:
            if (((const BrowserNode *) node->get_container(UmlActivity)) != activity)
                return FALSE;

            break;

        case UmlExpansionNode:
        case UmlActivityAction:
        case UmlActivityObject:
        case InitialAN:
        case FlowFinalAN:
        case ActivityFinalAN:
        case DecisionAN:
        case MergeAN:
        case ForkAN:
        case JoinAN:
            if (((const BrowserNode *) node->get_container(UmlActivity)) != activity)
                return FALSE;

            break;

        default:
            return FALSE;
        }

        if (! may_contains(node, TRUE))
            return FALSE;

        duplicable = may_contains_it(node);
    }

    return TRUE;
}

void BrowserExpansionRegion::DragMoveEvent(QDragMoveEvent * e)
{
    if (UmlDrag::canDecode(e, BrowserExpansionNode::drag_key(this)) ||
        UmlDrag::canDecode(e, BrowserActivityNode::drag_key(this)) ||
        UmlDrag::canDecode(e, BrowserActivityAction::drag_key(this)) ||
        UmlDrag::canDecode(e, BrowserInterruptibleActivityRegion::drag_key(this)) ||
        UmlDrag::canDecode(e, BrowserExpansionRegion::drag_key(this))) {
        if (!is_read_only)
            e->accept();
        else
            e->ignore();
    }
    else
        ((BrowserNode *) parent())->DragMoveInsideEvent(e);
}

void BrowserExpansionRegion::DropEvent(QDropEvent * e)
{
    DropAfterEvent(e, 0);
}

void BrowserExpansionRegion::DragMoveInsideEvent(QDragMoveEvent * e)
{
    if (!is_read_only &&
        (UmlDrag::canDecode(e, BrowserExpansionNode::drag_key(this)) ||
         UmlDrag::canDecode(e, BrowserActivityNode::drag_key(this)) ||
         UmlDrag::canDecode(e, BrowserActivityAction::drag_key(this)) ||
         UmlDrag::canDecode(e, BrowserInterruptibleActivityRegion::drag_key(this)) ||
         UmlDrag::canDecode(e, BrowserExpansionRegion::drag_key(this))))
        e->accept();
    else
        e->ignore();
}

void BrowserExpansionRegion::DropAfterEvent(QDropEvent * e, BrowserNode * after)
{
    BrowserNode * bn;

    if ((((bn = UmlDrag::decode(e, BrowserExpansionNode::drag_key(this))) != 0) ||
         ((bn = UmlDrag::decode(e, BrowserActivityNode::drag_key(this))) != 0) ||
         ((bn = UmlDrag::decode(e, BrowserActivityAction::drag_key(this))) != 0) ||
         ((bn = UmlDrag::decode(e, BrowserInterruptibleActivityRegion::drag_key(this))) != 0) ||
         ((bn = UmlDrag::decode(e, BrowserExpansionRegion::drag_key(this))) != 0)) &&
        (bn != after) && (bn != this)) {
        if (may_contains(bn, TRUE)) {
            if ((after == 0) &&
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

QString BrowserExpansionRegion::drag_key() const
{
    return QString::number(UmlExpansionRegion)
           + "#" + QString::number((unsigned long) get_container(UmlActivity));
}

QString BrowserExpansionRegion::drag_postfix() const
{
    return "#" + QString::number((unsigned long) get_container(UmlActivity));
}

QString BrowserExpansionRegion::drag_key(BrowserNode * p)
{
    return QString::number(UmlExpansionRegion)
           + "#" + QString::number((unsigned long) p->get_container(UmlActivity));
}

void BrowserExpansionRegion::save_stereotypes(QTextStream & st)
{
    nl_indent(st);
    st << "expansionregion_stereotypes ";
    save_unicode_string_list(its_default_stereotypes, st);
}

void BrowserExpansionRegion::read_stereotypes(char *& st, char *& k)
{
    if (!strcmp(k, "expansionregion_stereotypes")) {
        read_unicode_string_list(its_default_stereotypes, st);
        k = read_keyword(st);
    }
    else
        init();
}

void BrowserExpansionRegion::save(QTextStream & st, bool ref, QString & warning)
{
    if (ref)
        st << "expansionregion_ref " << get_ident() << " // " << get_name();
    else {
        nl_indent(st);
        st << "expansionregion " << get_ident() << " ";
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

BrowserExpansionRegion * BrowserExpansionRegion::read_ref(char *& st)
{
    read_keyword(st, "expansionregion_ref");

    int id = read_id(st);
    BrowserExpansionRegion * result = all[id];

    return (result == 0)
           ? new BrowserExpansionRegion(id)
           : result;
}

BrowserExpansionRegion * BrowserExpansionRegion::read(char *& st, char * k,
        BrowserNode * parent)
{
    BrowserExpansionRegion * result;
    int id;

    if (!strcmp(k, "expansionregion_ref")) {
        id = read_id(st);
        result = all[id];

        return (result == 0)
               ? new BrowserExpansionRegion(id)
               : result;
    }
    else if (!strcmp(k, "expansionregion")) {
        id = read_id(st);
        result = all[id];

        if (result == 0)
            result = new BrowserExpansionRegion(read_string(st), parent, id);
        else if (result->is_defined) {
            BrowserExpansionRegion * already_exist = result;

            result = new BrowserExpansionRegion(read_string(st), parent, id);

            already_exist->must_change_id(all);
            already_exist->unconsistent_fixed("expansion region", result);
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
            while (BrowserExpansionNode::read(st, k, result) ||
                   BrowserActivityNode::read(st, k, result) ||
                   BrowserActivityObject::read(st, k, result) ||
                   BrowserActivityAction::read(st, k, result) ||
                   BrowserInterruptibleActivityRegion::read(st, k, result) ||
                   BrowserExpansionRegion::read(st, k, result))
                k = read_keyword(st);

            if (strcmp(k, "end"))
                wrong_keyword(k, "end");
        }

        return result;
    }
    else
        return 0;
}

BrowserNode * BrowserExpansionRegion::get_it(const char * k, int id)
{
    return (!strcmp(k, "expansionregion_ref"))
           ? all[id]
           : BrowserExpansionNode::get_it(k, id);
}
