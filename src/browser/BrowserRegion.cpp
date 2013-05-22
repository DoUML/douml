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

#include "BrowserRegion.h"
#include "BrowserState.h"
#include "BrowserPseudoState.h"
#include "BrowserStateAction.h"
#include "SimpleData.h"
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

IdDict<BrowserRegion> BrowserRegion::all(__FILE__);
QStringList BrowserRegion::its_default_stereotypes;	// unicode

BrowserRegion::BrowserRegion(QString s, BrowserNode * p, BasicData * d, int id)
    : BrowserNode(s, p), Labeled<BrowserRegion>(all, id), def(d)
{
    def->set_browser_node(this);
}

BrowserRegion::BrowserRegion(const BrowserRegion * model,
                             BrowserNode * p)
    : BrowserNode(model->name, p), Labeled<BrowserRegion>(all, 0)
{
    def = new SimpleData(model->def);
    def->set_browser_node(this);
    comment = model->comment;
}

BrowserRegion::BrowserRegion(int id)
    : BrowserNode(), Labeled<BrowserRegion>(all, id),
      def(new SimpleData)
{
    // not yet read
    def->set_browser_node(this);
}

BrowserRegion::~BrowserRegion()
{
    all.remove(get_ident());
    delete def;
}

void BrowserRegion::delete_it()
{
    // to update state's region drawing
    ((BrowserNode *) parent())->modified();

    BrowserNode::delete_it();
}

BrowserNode * BrowserRegion::duplicate(BrowserNode * p, QString name)
{
    BrowserRegion * result = new BrowserRegion(this, p);

    result->set_name((name.isEmpty()) ? get_name() : name);
    result->update_stereotype();

    return result;
}

void BrowserRegion::clear(bool old)
{
    all.clear(old);
}

void BrowserRegion::update_idmax_for_root()
{
    all.update_idmax_for_root();
}

void BrowserRegion::prepare_update_lib() const
{
    all.memo_id_oid(get_ident(), original_id);

    for (Q3ListViewItem * child = firstChild();
         child != 0;
         child = child->nextSibling())
        ((BrowserNode *) child)->prepare_update_lib();
}

void BrowserRegion::renumber(int phase)
{
    if (phase != -1)
        new_ident(phase, all);
}

const QPixmap * BrowserRegion::pixmap(int) const
{
    if (deletedp())
        return DeletedRegionIcon;

    const QPixmap * px = ProfiledStereotypes::browserPixmap(def->get_stereotype());

    return (px != 0) ? px : RegionIcon;
}

BrowserRegion * BrowserRegion::add_region(BrowserNode * future_parent,
        const char * s)
{
    SimpleData * st = new SimpleData();
    BrowserRegion * r = new BrowserRegion(s, future_parent, st);

    future_parent->setOpen(TRUE);
    future_parent->package_modified();
    // to update state's region drawing
    future_parent->modified();

    return r;
}

void BrowserRegion::menu()
{
    Q3PopupMenu m(0, name);
    Q3PopupMenu toolm(0);

    MenuFactory::createTitle(m, def->definition(FALSE, TRUE));
    m.insertSeparator();

    if (!deletedp()) {
        if (!is_read_only) {
            m.setWhatsThis(m.insertItem(TR("New state"), 1),
                           TR("to add a <i>state</i> to the <i>region</i>"));
        }

        m.insertSeparator();
        m.setWhatsThis(m.insertItem(TR("Edit"), 3),
                       TR("to edit the <i>region</i>, \
a double click with the left mouse button does the same thing"));

        if (!is_read_only && (edition_number == 0)) {
            m.insertSeparator();
            m.setWhatsThis(m.insertItem(TR("Delete"), 7),
                           TR("to delete the <i>region</i>. \
Note that you can undelete it after"));
        }

        mark_menu(m, TR("the region"), 90);
        ProfiledStereotypes::menu(m, this, 99990);

        if ((edition_number == 0) &&
            Tool::menu_insert(&toolm, get_type(), 100)) {
            m.insertSeparator();
            m.insertItem(TR("Tool"), &toolm);
        }
    }
    else if (!is_read_only && (edition_number == 0)) {
        m.setWhatsThis(m.insertItem(TR("Undelete"), 8),
                       TR("to undelete the <i>region</i>"));

        Q3ListViewItem * child;

        for (child = firstChild(); child != 0; child = child->nextSibling()) {
            if (((BrowserNode *) child)->deletedp()) {
                m.setWhatsThis(m.insertItem(TR("Undelete recursively"), 9),
                               TR("undelete the region and its children"));
                break;
            }
        }
    }

    exec_menu_choice(m.exec(QCursor::pos()));
}

void BrowserRegion::exec_menu_choice(int rank)
{
    switch (rank) {
    case 1:
        if (BrowserState::add_state(this, (bool) FALSE) == 0)
            return;

        break;

    case 3:
        open(TRUE);
        return;

    case 7:
        delete_it();
        break;

    case 8:
        BrowserNode::undelete(FALSE);
        break;

    case 9:
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

void BrowserRegion::apply_shortcut(QString s)
{
    int choice = -1;

    if (!deletedp()) {
        if (!is_read_only) {
            if (s == "New state")
                choice = 1;
        }

        if (s == "Edit")
            choice = 3;

        if (!is_read_only && (edition_number == 0)) {
            if (s == "Delete")
                choice = 7;
        }

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

    exec_menu_choice(choice);
}

void BrowserRegion::open(bool)
{
    if (!is_edited)
        edit(TR("Region"), its_default_stereotypes);
}

void BrowserRegion::modified()
{
    repaint();
    ((BrowserNode *) parent())->modified();
}

UmlCode BrowserRegion::get_type() const
{
    return UmlRegion;
}

QString BrowserRegion::get_stype() const
{
    return TR("Region");
}

int BrowserRegion::get_identifier() const
{
    return get_ident();
}

BasicData * BrowserRegion::get_data() const
{
    return def;
}

QString BrowserRegion::full_name(bool rev, bool) const
{
    QString s = name;

    if (s.isEmpty())
        s = "<anonymous region>";

    return fullname(s, rev);
}

bool BrowserRegion::allow_empty() const
{
    return TRUE;
}

bool BrowserRegion::allow_spaces() const
{
    return TRUE;
}

bool BrowserRegion::same_name(const QString &, UmlCode) const
{
    return FALSE;
}

void BrowserRegion::init()
{
    its_default_stereotypes.clear();
}

// unicode
const QStringList & BrowserRegion::default_stereotypes()
{
    return its_default_stereotypes;
}

bool BrowserRegion::api_compatible(unsigned v) const
{
    return (v > 14);
}

bool BrowserRegion::tool_cmd(ToolCom * com, const char * args)
{
    switch ((unsigned char) args[-1]) {
    case createCmd: {
        bool ok = TRUE;

        if (is_read_only && !root_permission())
            ok = FALSE;
        else {
            UmlCode k = com->get_kind(args);

            switch (k) {
            case UmlState:
                if (wrong_child_name(args, UmlState, TRUE, FALSE))
                    ok = FALSE;
                else
                    // force QString(args) else call add_state(BrowserNode, bool) !
                    (BrowserState::add_state(this, QString(args)))->write_id(com);

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

bool BrowserRegion::may_contains_them(const QList<BrowserNode *> & l,
                                      BooL & duplicable) const
{
    foreach (BrowserNode *node, l) {
        switch (node->get_type()) {
        case UmlState:
            break;

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
            return (((const BrowserNode *) node->parent()) == this);

        default:
            return FALSE;
        }

        if (! may_contains(node, FALSE))
            return FALSE;

        duplicable = may_contains_it(node);
    }

    return TRUE;
}

void BrowserRegion::DragMoveEvent(QDragMoveEvent * e)
{
    if (UmlDrag::canDecode(e, UmlState) ||
        UmlDrag::canDecode(e, BrowserState::drag_key(this)) ||
        UmlDrag::canDecode(e, BrowserPseudoState::drag_key(this)) ||
        UmlDrag::canDecode(e, BrowserStateAction::drag_key(this))) {
        if (!is_read_only)
            e->accept();
        else
            e->ignore();
    }
    else
        ((BrowserNode *) parent())->DragMoveInsideEvent(e);
}

void BrowserRegion::DropEvent(QDropEvent * e)
{
    DropAfterEvent(e, 0);
}

void BrowserRegion::DragMoveInsideEvent(QDragMoveEvent * e)
{
    if (!is_read_only &&
        (UmlDrag::canDecode(e, UmlState) ||
         UmlDrag::canDecode(e, BrowserState::drag_key(this)) ||
         UmlDrag::canDecode(e, BrowserPseudoState::drag_key(this)) ||
         UmlDrag::canDecode(e, BrowserStateAction::drag_key(this))))
        e->accept();
    else
        e->ignore();
}

void BrowserRegion::DropAfterEvent(QDropEvent * e, BrowserNode * after)
{
    BrowserNode * bn;

    if ((((bn = UmlDrag::decode(e, BrowserPseudoState::drag_key(this))) != 0) ||
         ((bn = UmlDrag::decode(e, BrowserStateAction::drag_key(this))) != 0) ||
         ((bn = UmlDrag::decode(e, BrowserState::drag_key(this))) != 0)) &&
        (bn != after) && (bn != this)) {
        if (may_contains(bn, FALSE)) {
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

QString BrowserRegion::drag_key() const
{
    return QString::number(UmlRegion)
           + "#" + QString::number((unsigned long) BrowserState::get_machine(this));
}

QString BrowserRegion::drag_postfix() const
{
    return "#" + QString::number((unsigned long) BrowserState::get_machine(this));
}

QString BrowserRegion::drag_key(BrowserNode * p)
{
    return QString::number(UmlRegion)
           + "#" + QString::number((unsigned long) BrowserState::get_machine(p));
}

void BrowserRegion::save_stereotypes(QTextStream & st)
{
    nl_indent(st);
    st << "state_stereotypes ";
    save_unicode_string_list(its_default_stereotypes, st);
}

void BrowserRegion::read_stereotypes(char *& st, char *& k)
{
    if (!strcmp(k, "state_stereotypes")) {
        read_unicode_string_list(its_default_stereotypes, st);
        k = read_keyword(st);
    }
    else
        init();
}

void BrowserRegion::save(QTextStream & st, bool ref, QString & warning)
{
    if (ref)
        st << "region_ref " << get_ident() << " // " << get_name();
    else {
        nl_indent(st);
        st << "region " << get_ident() << " ";
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

BrowserRegion * BrowserRegion::read_ref(char *& st)
{
    read_keyword(st, "region_ref");

    int id = read_id(st);
    BrowserRegion * result = all[id];

    return (result == 0)
           ? new BrowserRegion(id)
           : result;
}

BrowserRegion * BrowserRegion::read(char *& st, char * k,
                                    BrowserNode * parent)
{
    BrowserRegion * result;
    int id;

    if (!strcmp(k, "region_ref")) {
        id = read_id(st);
        result = all[id];

        return (result == 0)
               ? new BrowserRegion(id)
               : result;
    }
    else if (!strcmp(k, "region")) {
        id = read_id(st);
        result = all[id];

        if (result == 0)
            result = new BrowserRegion(read_string(st), parent, new SimpleData, id);
        else if (result->is_defined) {
            BrowserRegion * already_exist = result;

            result = new BrowserRegion(read_string(st), parent, new SimpleData, id);

            already_exist->must_change_id(all);
            already_exist->unconsistent_fixed("region", result);
        }
        else {
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
            while (BrowserState::read(st, k, result) ||
                   BrowserPseudoState::read(st, k, result) ||
                   BrowserStateAction::read(st, k, result))
                k = read_keyword(st);

            if (strcmp(k, "end"))
                wrong_keyword(k, "end");
        }

        return result;
    }
    else
        return 0;
}

BrowserNode * BrowserRegion::get_it(const char * k, int id)
{
    return (!strcmp(k, "region_ref")) ? all[id] : 0;
}
