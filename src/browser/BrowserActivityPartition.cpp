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

#include "BrowserActivityPartition.h"
#include "ActivityPartitionData.h"
#include "ActivityPartitionDialog.h"
#include "ReferenceDialog.h"
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

// fixe bug
#include "BrowserActivityNode.h"
#include "BrowserActivityAction.h"
#include "BrowserActivityObject.h"
#include "BrowserInterruptibleActivityRegion.h"
#include "BrowserExpansionRegion.h"

IdDict<BrowserActivityPartition> BrowserActivityPartition::all(__FILE__);
QStringList BrowserActivityPartition::its_default_stereotypes;	// unicode

BrowserActivityPartition::BrowserActivityPartition(QString s, BrowserNode * p, int id)
    : BrowserNode(s, p), Labeled<BrowserActivityPartition>(all, id),
      def(new ActivityPartitionData), associated_diagram(0)
{
    def->set_browser_node(this);
}

BrowserActivityPartition::BrowserActivityPartition(int id)
    : BrowserNode(), Labeled<BrowserActivityPartition>(all, id),
      def(new ActivityPartitionData), associated_diagram(0)
{
    // not yet read
    def->set_browser_node(this);
}

BrowserActivityPartition::BrowserActivityPartition(const BrowserActivityPartition * model, BrowserNode * p)
    : BrowserNode("", p), Labeled<BrowserActivityPartition>(all, 0), associated_diagram(0)
{
    def = new ActivityPartitionData(model->def, this);
    comment = model->comment;
}

BrowserNode * BrowserActivityPartition::duplicate(BrowserNode * p, QString s)
{
    BrowserNode * result = new BrowserActivityPartition(this, p);

    result->set_name(s);
    result->update_stereotype(TRUE);

    return result;
}

BrowserActivityPartition::~BrowserActivityPartition()
{
    all.remove(get_ident());
    delete def;
}

void BrowserActivityPartition::clear(bool old)
{
    all.clear(old);
}

void BrowserActivityPartition::update_idmax_for_root()
{
    all.update_idmax_for_root();
}

void BrowserActivityPartition::prepare_update_lib() const
{
    all.memo_id_oid(get_ident(), original_id);

    for (Q3ListViewItem * child = firstChild();
         child != 0;
         child = child->nextSibling())
        ((BrowserNode *) child)->prepare_update_lib();
}

void BrowserActivityPartition::referenced_by(QList<BrowserNode *> & l, bool ondelete)
{
    BrowserNode::referenced_by(l, ondelete);

    if (! ondelete)
        BrowserActivityDiagram::compute_referenced_by(l, this, "activitypartitioncanvas", "activitypartition_ref");
}

void BrowserActivityPartition::renumber(int phase)
{
    if (phase != -1)
        new_ident(phase, all);
}

const QPixmap * BrowserActivityPartition::pixmap(int) const
{
    if (deletedp())
        return DeletedActivityPartitionIcon;

    const QPixmap * px = ProfiledStereotypes::browserPixmap(def->get_stereotype());

    if (px != 0)
        return px;
    else if (is_marked)
        return ActivityPartitionMarkedIcon;
    else
        return ActivityPartitionIcon;
}

BrowserActivityPartition *
BrowserActivityPartition::add_activitypartition(BrowserNode * future_parent)
{
    QString name;

    return (!future_parent->enter_child_name(name, TR("enter activity partition's \nname (may be empty) : "),
            UmlActivityPartition, TRUE, TRUE))

           ? 0
           : add_activitypartition(future_parent, name);
}

BrowserActivityPartition *
BrowserActivityPartition::add_activitypartition(BrowserNode * future_parent,
        const char * name)
{
    BrowserActivityPartition * r =
        new BrowserActivityPartition(name, future_parent);

    future_parent->setOpen(TRUE);
    future_parent->package_modified();

    return r;
}

BrowserActivityPartition *
BrowserActivityPartition::get_activitypartition(BrowserNode * parent)
{
    BrowserNodeList l;
    Q3ListViewItem * child;

    for (child = parent->firstChild(); child != 0; child = child->nextSibling())
        if (!((BrowserNode *) child)->deletedp() &&
            (((BrowserNode *) child)->get_type() == UmlActivityPartition))
            l.append((BrowserNode *) child);

    BrowserNode * old;
    QString name;

    if (!parent->enter_child_name(name, TR("enter activity partition's \nname (may be empty) : "),
                                  UmlActivityPartition, l, &old,
                                  TRUE, TRUE))
        return 0;

    if (old != 0)
        return ((BrowserActivityPartition *) old);

    BrowserActivityPartition * r =
        new BrowserActivityPartition(name, parent);

    parent->setOpen(TRUE);
    parent->package_modified();

    return r;
}

void BrowserActivityPartition::menu()
{
    Q3PopupMenu m(0, "activity partition");
    Q3PopupMenu toolm(0);

    MenuFactory::createTitle(m, def->definition(FALSE, TRUE));
    m.insertSeparator();

    if (!deletedp()) {
        if (!is_read_only) {
            m.setWhatsThis(m.insertItem(TR("New sub activity partition"), 0),
                           TR("to add a sub <i>activity partition</i>"));
            m.insertSeparator();
        }

        m.setWhatsThis(m.insertItem(TR("Edit"), 4),
                       TR("to edit the <i>activity partition</i>, \
a double click with the left mouse button does the same thing"));

        if (!is_read_only) {
            m.setWhatsThis(m.insertItem(TR("Duplicate"), 5),
                           TR("to copy the <i>activity partition</i> in a new one"));
            m.insertSeparator();

            if (edition_number == 0)
                m.setWhatsThis(m.insertItem(TR("Delete"), 8),
                               TR("to delete the <i>activity partition</i>. \
Note that you can undelete it after"));
        }

        m.setWhatsThis(m.insertItem(TR("Referenced by"), 3),
                       TR("to know who reference the <i>partition</i>"));
        mark_menu(m, TR("the activity partition"), 90);
        ProfiledStereotypes::menu(m, this, 99990);

        if ((edition_number == 0) &&
            Tool::menu_insert(&toolm, get_type(), 100)) {
            m.insertSeparator();
            m.insertItem(TR("Tool"), &toolm);
        }
    }
    else if (!is_read_only && (edition_number == 0)) {
        m.setWhatsThis(m.insertItem(TR("Undelete"), 9),
                       TR("to undelete the <i>activity partition</i>"));

        Q3ListViewItem * child;

        for (child = firstChild(); child != 0; child = child->nextSibling()) {
            if (((BrowserNode *) child)->deletedp()) {
                m.setWhatsThis(m.insertItem(TR("Undelete recursively"), 10),
                               TR("undelete the activity partition and its children"));
                break;
            }
        }
    }

    exec_menu_choice(m.exec(QCursor::pos()));
}

void BrowserActivityPartition::exec_menu_choice(int rank)
{
    switch (rank) {
    case 0:
        add_activitypartition(this);
        break;

    case 3:
        ReferenceDialog::show(this);
        return;

    case 4:
        open(TRUE);
        return;

    case 5: {
        QString name;

        if (((BrowserNode *) parent())->enter_child_name(name, TR("enter activity partition's \nname (may be empty) : "),
                UmlActivityPartition, TRUE, TRUE))
            duplicate((BrowserNode *) parent(), name)->select_in_browser();
    }
    break;

    case 8:
        delete_it();
        break;

    case 9:
        BrowserNode::undelete(FALSE);
        break;

    case 10:
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

void BrowserActivityPartition::apply_shortcut(QString s)
{
    int choice = -1;

    if (!deletedp()) {
        if (!is_read_only) {
            if (s == "New activity partition")
                choice = 0;
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
            choice = 9;

        Q3ListViewItem * child;

        for (child = firstChild(); child != 0; child = child->nextSibling()) {
            if (((BrowserNode *) child)->deletedp()) {
                if (s == "Undelete recursively")
                    choice = 10;

                break;
            }
        }
    }

    exec_menu_choice(choice);
}

void BrowserActivityPartition::open(bool force_edit)
{
    if (!force_edit &&
        (associated_diagram != 0) &&
        !associated_diagram->deletedp())
        associated_diagram->open(FALSE);
    else if (!is_edited) {
        (new ActivityPartitionDialog(def))->show();
    }
}

void BrowserActivityPartition::modified()
{
    repaint();
    ((BrowserNode *) parent())->modified();
    // to update drawing
    def->modified();
}

UmlCode BrowserActivityPartition::get_type() const
{
    return UmlActivityPartition;
}

QString BrowserActivityPartition::get_stype() const
{
    return TR("activity partition");
}

int BrowserActivityPartition::get_identifier() const
{
    return get_ident();
}

const char * BrowserActivityPartition::help_topic() const
{
    return "activitypartition";
}

BasicData * BrowserActivityPartition::get_data() const
{
    return def;
}

bool BrowserActivityPartition::allow_empty() const
{
    return TRUE;
}

QString BrowserActivityPartition::full_name(bool rev, bool) const
{
    QString s = name;

    if (s.isEmpty())
        s = "<anonymous activity partition>";

    return fullname(s, rev);
}

BrowserNode * BrowserActivityPartition::get_associated() const
{
    return associated_diagram;
}

void BrowserActivityPartition::set_associated_diagram(BrowserActivityDiagram * dg,
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

void BrowserActivityPartition::init()
{
    its_default_stereotypes.clear();
}

// unicode
const QStringList & BrowserActivityPartition::default_stereotypes()
{
    return its_default_stereotypes;
}

bool BrowserActivityPartition::api_compatible(unsigned v) const
{
    return (v > 42);
}

bool BrowserActivityPartition::tool_cmd(ToolCom * com, const char * args)
{
    switch ((unsigned char) args[-1]) {
    case createCmd: {
        bool ok = TRUE;

        if (is_read_only && !root_permission())
            ok = FALSE;
        else {
            UmlCode k = com->get_kind(args);

            switch (k) {
            case UmlActivityPartition:
                (new BrowserActivityPartition(args, this))->write_id(com);
                break;

            default:
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

bool BrowserActivityPartition::may_contains_them(const QList<BrowserNode *> & l,
        BooL & duplicable) const
{
    BrowserNode * activity = get_container(UmlActivity);
    foreach (BrowserNode *node, l) {
        switch (node->get_type()) {
        case UmlActivityPartition:
            return (((const BrowserNode *) node->get_container(UmlActivity)) == activity);

        default:
            return FALSE;
        }

        if (! may_contains(node, FALSE))
            return FALSE;

        duplicable = may_contains_it(node);
    }

    return TRUE;
}

void BrowserActivityPartition::DragMoveEvent(QDragMoveEvent * e)
{
    if (UmlDrag::canDecode(e, BrowserActivityPartition::drag_key(this))) {
        if (!is_read_only)
            e->accept();
        else
            e->ignore();
    }
    else
        ((BrowserNode *) parent())->DragMoveInsideEvent(e);
}

void BrowserActivityPartition::DropEvent(QDropEvent * e)
{
    DropAfterEvent(e, 0);
}

void BrowserActivityPartition::DragMoveInsideEvent(QDragMoveEvent * e)
{
    if (!is_read_only &&
        (UmlDrag::canDecode(e, BrowserActivityPartition::drag_key(this))))
        e->accept();
    else
        e->ignore();
}

void BrowserActivityPartition::DropAfterEvent(QDropEvent * e, BrowserNode * after)
{
    BrowserNode * bn;

    if ((((bn = UmlDrag::decode(e, BrowserActivityPartition::drag_key(this))) != 0)) &&
        (bn != after) && (bn != this)) {
        if (may_contains(bn, TRUE))  {
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

QString BrowserActivityPartition::drag_key() const
{
    return QString::number(UmlActivityPartition)
           + "#" + QString::number((unsigned long) get_container(UmlActivity));
}

QString BrowserActivityPartition::drag_postfix() const
{
    return "#" + QString::number((unsigned long) get_container(UmlActivity));
}

QString BrowserActivityPartition::drag_key(BrowserNode * p)
{
    return QString::number(UmlActivityPartition)
           + "#" + QString::number((unsigned long) p->get_container(UmlActivity));
}

void BrowserActivityPartition::post_load()
{
    // to manage deleted represented elts
    IdIterator<BrowserActivityPartition> it(all);

    while (it.current()) {
        it.current()->def->post_load();

        ++it;
    }
}

void BrowserActivityPartition::save_stereotypes(QTextStream & st)
{
    nl_indent(st);
    st << "activitypartition_stereotypes ";
    save_unicode_string_list(its_default_stereotypes, st);
}

void BrowserActivityPartition::read_stereotypes(char *& st, char *& k)
{
    if (!strcmp(k, "activitypartition_stereotypes")) {
        read_unicode_string_list(its_default_stereotypes, st);
        k = read_keyword(st);
    }
    else
        init();
}

void BrowserActivityPartition::save(QTextStream & st, bool ref, QString & warning)
{
    if (ref)
        st << "activitypartition_ref " << get_ident() << " // " << get_name();
    else {
        nl_indent(st);
        st << "activitypartition " << get_ident() << " ";
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

void BrowserActivityPartition::on_delete()
{
    if (associated_diagram && associated_diagram->deletedp())
        associated_diagram = 0;
}

BrowserActivityPartition *
BrowserActivityPartition::read_ref(char *& st)
{
    read_keyword(st, "activitypartition_ref");

    int id = read_id(st);
    BrowserActivityPartition * result = all[id];

    return (result == 0)
           ? new BrowserActivityPartition(id)
           : result;
}

BrowserActivityPartition *
BrowserActivityPartition::read(char *& st, char * k,
                               BrowserNode * parent)
{
    BrowserActivityPartition * result;
    int id;

    if (!strcmp(k, "activitypartition_ref")) {
        id = read_id(st);
        result = all[id];

        return (result == 0)
               ? new BrowserActivityPartition(id)
               : result;
    }
    else if (!strcmp(k, "activitypartition")) {
        id = read_id(st);
        result = all[id];

        if (result == 0)
            result = new BrowserActivityPartition(read_string(st), parent, id);
        else if (result->is_defined) {
            BrowserActivityPartition * already_exist = result;

            result = new BrowserActivityPartition(read_string(st), parent, id);

            already_exist->must_change_id(all);
            already_exist->unconsistent_fixed("activity partition", result);
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

        BrowserNode * act = parent;

        while (act->get_type() != UmlActivity)
            act = (BrowserNode *) act->parent();

        if (strcmp(k, "end")) {
            while (BrowserActivityPartition::read(st, k, result) ||
                   // fixe bug
                   BrowserActivityNode::read(st, k, act) ||
                   BrowserActivityAction::read(st, k, act) ||
                   BrowserActivityObject::read(st, k, act) ||
                   BrowserInterruptibleActivityRegion::read(st, k, act) ||
                   BrowserExpansionRegion::read(st, k, act))
                k = read_keyword(st);

            if (strcmp(k, "end"))
                wrong_keyword(k, "end");
        }

        return result;
    }
    else
        return 0;
}

BrowserNode * BrowserActivityPartition::get_it(const char * k, int id)
{
    return (!strcmp(k, "activitypartition_ref"))
           ? all[id]
           : 0;
}
