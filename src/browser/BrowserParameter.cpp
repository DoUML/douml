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

#include "BrowserParameter.h"
#include "ParameterData.h"
#include "BrowserActivity.h"
#include "BrowserFlow.h"
#include "BrowserClass.h"
#include "BrowserActivityNode.h"
#include "PinData.h"
#include "ActivityObjectData.h"
#include "ReferenceDialog.h"
#include "UmlDrag.h"
#include "UmlPixmap.h"
#include "UmlGlobal.h"
#include "myio.h"
#include "ToolCom.h"
#include "Tool.h"
#include "ui/menufactory.h"
#include "strutil.h"
#include "DialogUtil.h"
#include "ProfiledStereotypes.h"
#include "mu.h"
#include "BrowserActivityDiagram.h"
#include "translate.h"

IdDict<BrowserParameter> BrowserParameter::all(257, __FILE__);
QStringList BrowserParameter::its_default_stereotypes;	// unicode

BrowserParameter::BrowserParameter(QString s, BrowserNode * p, int id)
    : BrowserNode(s, p), Labeled<BrowserParameter>(all, id),
      def(new ParameterData)
{
    def->set_browser_node(this);
}

BrowserParameter::BrowserParameter(int id)
    : BrowserNode(), Labeled<BrowserParameter>(all, id),
      def(new ParameterData)
{
    // not yet read
    def->set_browser_node(this);
}

BrowserParameter::BrowserParameter(const BrowserParameter * model, BrowserNode * p)
    : BrowserNode(model->name, p), Labeled<BrowserParameter>(all, 0)
{
    def = new ParameterData(model->def, this);
    comment = model->comment;
}

BrowserNode * BrowserParameter::duplicate(BrowserNode * p, QString name)
{
    BrowserNode * result = new BrowserParameter(this, p);

    result->set_name((name.isEmpty()) ? get_name() : name);
    result->update_stereotype();

    return result;
}

BrowserParameter::~BrowserParameter()
{
    all.remove(get_ident());
    delete def;
}

void BrowserParameter::clear(bool old)
{
    all.clear(old);
}

void BrowserParameter::update_idmax_for_root()
{
    all.update_idmax_for_root();
}

void BrowserParameter::prepare_update_lib() const
{
    all.memo_id_oid(get_ident(), original_id);

    for (Q3ListViewItem * child = firstChild();
         child != 0;
         child = child->nextSibling())
        ((BrowserNode *) child)->prepare_update_lib();
}

void BrowserParameter::renumber(int phase)
{
    if (phase != -1)
        new_ident(phase, all);
}

const QPixmap * BrowserParameter::pixmap(int) const
{
    if (deletedp())
        return DeletedParameterIcon;

    const QPixmap * px = ProfiledStereotypes::browserPixmap(def->get_stereotype());

    return (px != 0) ? px : ParameterIcon;
}

// add flow (dependency not allowed)
BasicData * BrowserParameter::add_relation(UmlCode, BrowserNode * end)
{
    return (new BrowserFlow(this, end))->get_data();
}

// a flow may be added  (dependency not allowed) ?
QString BrowserParameter::may_start() const
{
    switch (def->get_dir()) {
    case UmlIn:
    case UmlInOut:
        return 0;

    default:
        return TR("can't have outgoing flow");
    }
}

// connexion by a flow (dependency not allowed)
QString BrowserParameter::may_connect(const BrowserNode * dest) const
{
    BrowserNode * container = dest->get_container(UmlActivity);

    if (container == 0)
        return TR("illegal");

    if (get_container(UmlActivity) != container)
        return TR("not in the same activity");

    const BrowserActivityElement * elt =
        dynamic_cast<const BrowserActivityElement *>(dest);

    return (elt == 0)
           ? TR("illegal")
           : elt->connexion_from(def->get_is_control());
}

QString BrowserParameter::connexion_from(bool control) const
{
    if (def->get_dir() == UmlIn)
        return TR("an input parameter can't have incoming flows");
    else if (def->get_is_control() != control)
        return (control)
               ? TR("parameter can't accept control flow (not 'is_control')")
               : TR("parameter can't accept data flow (is 'is_control')");
    else
        return 0;
}

void BrowserParameter::menu()
{
    Q3PopupMenu m(0, name);
    Q3PopupMenu toolm(0);

    MenuFactory::createTitle(m, def->definition(FALSE, TRUE));
    m.insertSeparator();

    if (!deletedp()) {
        if (!is_edited)
            m.setWhatsThis(m.insertItem(TR("Edit"), 0),
                           TR("to edit the <i>parameter</i>, \
a double click with the left mouse button does the same thing"));

        if (!is_read_only && (edition_number == 0)) {
            m.setWhatsThis(m.insertItem(TR("Duplicate"), 1),
                           TR("to copy the <i>parameter</i> in a new one"));
            m.insertSeparator();
            m.setWhatsThis(m.insertItem(TR("Delete"), 2),
                           TR("to delete the <i>parameter</i>. \
Note that you can undelete it after"));
        }

        m.setWhatsThis(m.insertItem(TR("Referenced by"), 4),
                       TR("to know who reference the <i>parameter</i> \
through a flow"));
        mark_menu(m, TR("the parameter"), 90);
        ProfiledStereotypes::menu(m, this, 99990);

        if ((edition_number == 0) &&
            Tool::menu_insert(&toolm, get_type(), 100)) {
            m.insertSeparator();
            m.insertItem(TR("Tool"), &toolm);
        }
    }
    else if (!is_read_only && (edition_number == 0))
        m.setWhatsThis(m.insertItem(TR("Undelete"), 3),
                       TR("to undelete the <i>parameter</i>"));

    exec_menu_choice(m.exec(QCursor::pos()));
}

void BrowserParameter::exec_menu_choice(int rank)
{
    switch (rank) {
    case 0:
        open(TRUE);
        return;

    case 1:
        ((BrowserActivity *) parent())->add_parameter(this);
        return;

    case 2:
        delete_it();
        break;

    case 3:
        undelete(FALSE);
        break;

    case 4:
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

void BrowserParameter::apply_shortcut(QString s)
{
    int choice = -1;

    if (!deletedp()) {
        if (!is_edited)
            if (s == "Edit")
                choice = 0;

        if (!is_read_only && (edition_number == 0)) {
            if (s == "Duplicate")
                choice = 1;
            else if (s == "Delete")
                choice = 2;
        }

        if (s == "Referenced by")
            choice = 4;

        mark_shortcut(s, choice, 90);

        if (edition_number == 0)
            Tool::shortcut(s, choice, get_type(), 100);
    }
    else if (!is_read_only && (edition_number == 0))
        if (s == "Undelete")
            choice = 3;

    exec_menu_choice(choice);
}

void BrowserParameter::open(bool)
{
    if (!is_edited)
        def->edit();
}

void BrowserParameter::modified()
{
    repaint();
    ((BrowserNode *) parent())->modified();
}

UmlCode BrowserParameter::get_type() const
{
    return UmlParameter;
}

QString BrowserParameter::get_stype() const
{
    return TR("parameter");
}

int BrowserParameter::get_identifier() const
{
    return get_ident();
}

BasicData * BrowserParameter::get_data() const
{
    return def;
}

QString BrowserParameter::full_name(bool rev, bool) const
{
    return fullname(rev);
}

void BrowserParameter::referenced_by(QList<BrowserNode *> & l, bool ondelete)
{
    BrowserNode::referenced_by(l, ondelete);
    BrowserFlow::compute_referenced_by(l, this);

    if (! ondelete)
        BrowserActivityDiagram::compute_referenced_by(l, this, "parametercanvas", "parameter_ref");
}

void BrowserParameter::compute_referenced_by(QList<BrowserNode *> & l,
        BrowserNode * target)
{
    IdIterator<BrowserParameter> it(all);

    while (it.current()) {
        if (!it.current()->deletedp()) {
            const AType & t = it.current()->def->get_type();

            if (t.type == target)
                l.append(it.current());
        }

        ++it;
    }
}


bool BrowserParameter::api_compatible(unsigned v) const
{
    return (v > 24);
}

bool BrowserParameter::tool_cmd(ToolCom * com, const char * args)
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

void BrowserParameter::DragMoveEvent(QDragMoveEvent * e)
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

void BrowserParameter::DropEvent(QDropEvent * e)
{
    DropAfterEvent(e, 0);
}

void BrowserParameter::DragMoveInsideEvent(QDragMoveEvent * e)
{
    if (!is_read_only &&
        UmlDrag::canDecode(e, BrowserFlow::drag_key(this)))
        e->accept();
    else
        e->ignore();
}

void BrowserParameter::DropAfterEvent(QDropEvent * e, BrowserNode * after)
{
    BrowserNode * bn;

    if (((bn = UmlDrag::decode(e, BrowserFlow::drag_key(this))) != 0) &&
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

QString BrowserParameter::drag_key() const
{
    return QString::number(UmlParameter)
           + "#" + QString::number((unsigned long) parent());
}

QString BrowserParameter::drag_postfix() const
{
    return "#" + QString::number((unsigned long) parent());
}

QString BrowserParameter::drag_key(BrowserNode * p)
{
    return QString::number(UmlParameter)
           + "#" + QString::number((unsigned long) p);
}

void BrowserParameter::init()
{
    its_default_stereotypes.clear();
}

// unicode
const QStringList & BrowserParameter::default_stereotypes()
{
    return its_default_stereotypes;
}

void BrowserParameter::save_stereotypes(QTextStream & st)
{
    nl_indent(st);
    st << "parameter_stereotypes ";
    save_unicode_string_list(its_default_stereotypes, st);
}

void BrowserParameter::read_stereotypes(char *& st, char *& k)
{
    if (!strcmp(k, "parameter_stereotypes")) {
        read_unicode_string_list(its_default_stereotypes, st);
        k = read_keyword(st);
    }
}

void BrowserParameter::save(QTextStream & st, bool ref, QString & warning)
{
    if (ref)
        st << "parameter_ref " << get_ident() << " // " << get_name();
    else {
        nl_indent(st);
        st << "parameter " << get_ident() << " ";
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

BrowserParameter * BrowserParameter::read_ref(char *& st)
{
    read_keyword(st, "parameter_ref");

    int id = read_id(st);
    BrowserParameter * result = all[id];

    return (result == 0)
           ? new BrowserParameter(id)
           : result;
}

BrowserParameter * BrowserParameter::read(char *& st, char * k,
        BrowserNode * parent)
{
    BrowserParameter * result;
    int id;

    if (!strcmp(k, "parameter_ref")) {
        if ((result = all[id = read_id(st)]) == 0)
            result = new BrowserParameter(id);

        return result;
    }
    else if (!strcmp(k, "parameter")) {
        id = read_id(st);

        QString s = read_string(st);

        k = read_keyword(st);

        if ((result = all[id]) == 0) {
            result = new BrowserParameter(s, parent, id);
            result->def->read(st, k);	// updates k2
        }
        else if (result->is_defined) {
            BrowserParameter * already_exist = result;

            result = new BrowserParameter(s, parent, id);
            result->def->read(st, k);	// updates k2

            already_exist->must_change_id(all);
            already_exist->unconsistent_fixed("activity parameter", result);
        }
        else {
            result->def->read(st, k);	// updates k2
            result->set_parent(parent);
            result->set_name(s);
        }

        result->is_defined = TRUE;
        result->BrowserNode::read(st, k, id);

        if (strcmp(k, "end")) {
            while (BrowserFlow::read(st, k, result))
                k = read_keyword(st);

            if (strcmp(k, "end"))
                wrong_keyword(k, "end");
        }

        result->is_read_only = (!in_import() && read_only_file()) ||
                               ((user_id() != 0) && result->is_api_base());
        result->def->set_browser_node(result);

        if (strcmp(k, "end"))
            wrong_keyword(k, "end");

        return result;
    }
    else
        return 0;
}

BrowserNode * BrowserParameter::get_it(const char * k, int id)
{
    return (!strcmp(k, "parameter_ref")) ? all[id] : 0;
}
