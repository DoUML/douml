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
#include <qcursor.h>
#include <q3painter.h>
#include <q3ptrdict.h>
//Added by qt3to4:
#include <QTextStream>
#include <QDropEvent>
#include <QPixmap>

#include "BrowserFlow.h"
#include "FlowData.h"
#include "ReferenceDialog.h"
#include "UmlPixmap.h"
#include "UmlGlobal.h"
#include "myio.h"
#include "ToolCom.h"
#include "Tool.h"
#include "AType.h"
#include "ui/menufactory.h"
#include "strutil.h"
#include "ProfiledStereotypes.h"
#include "mu.h"
#include "BrowserActivityDiagram.h"
#include "translate.h"

IdDict<BrowserFlow> BrowserFlow::all(1021, __FILE__);
QStringList BrowserFlow::its_default_stereotypes;	// unicode

BrowserFlow::BrowserFlow(BrowserNode * p, BrowserNode * end)
    : BrowserNode("<flow>", p), Labeled<BrowserFlow>(all, 0),
      def(new FlowData)
{
    def->set_start_end(this, end);
}

BrowserFlow::BrowserFlow(BrowserNode * p, FlowData * d, int id)
    : BrowserNode("<flow>", p), Labeled<BrowserFlow>(all, id), def(d)
{
    // for read
}

BrowserFlow::BrowserFlow(const BrowserFlow * model, BrowserNode * p)
    : BrowserNode(model->name, p),
      Labeled<BrowserFlow>(all, 0)
{
    def = new FlowData(model, this);
    comment = model->comment;
}

BrowserFlow::BrowserFlow(int id)
    : BrowserNode(), Labeled<BrowserFlow>(all, id)
{
    // was deleted but still referenced
    // create a pseudo definition
    def = new FlowData();

    def->set_start_end(this, this);
}

BrowserFlow::~BrowserFlow()
{
    all.remove(get_ident());

    if (def)
        delete def;
}

BrowserNode * BrowserFlow::duplicate(BrowserNode * p, QString)
{
    BrowserFlow * result = new BrowserFlow(this, p);

    result->def->get_start()->modified();
    result->def->get_start()->package_modified();
    result->update_stereotype();

    return result;
}

bool BrowserFlow::undelete(bool, QString & warning, QString & renamed)
{
    if (! deletedp())
        return FALSE;

    if (def->get_start_node()->deletedp() ||
        def->get_end_node()->deletedp()) {
        warning += QString("<li><b>") + quote(name) + "</b> " + TR("from") + " <b>" +
                   def->get_start_node()->full_name() +
                   "</b> " + TR("to") + " <b>" + def->get_end_node()->full_name() + "</b>\n";
        return FALSE;
    }

    is_deleted = FALSE;
    def->undelete(warning, renamed);

    package_modified();
    repaint();

    return TRUE;
}

void BrowserFlow::referenced_by(QList<BrowserNode *> & l, bool ondelete)
{
    BrowserNode::referenced_by(l, ondelete);

    if (! ondelete)
        BrowserActivityDiagram::compute_referenced_by(l, this, "flowcanvas", "flow_ref");
}

void BrowserFlow::compute_referenced_by(QList<BrowserNode *> & l,
                                        const BrowserNode * target)
{
    IdIterator<BrowserFlow> it(all);

    while (it.current()) {
        if (!it.current()->deletedp() &&
            (it.current()->def->get_end_node() == target))
            l.append(it.current());

        ++it;
    }
}

void BrowserFlow::clear(bool old)
{
    all.clear(old);
}

void BrowserFlow::update_idmax_for_root()
{
    all.update_idmax_for_root();
}

void BrowserFlow::prepare_update_lib() const
{
    all.memo_id_oid(get_ident(), original_id);
}

void BrowserFlow::renumber(int phase)
{
    if (phase != -1)
        new_ident(phase, all);
}

void BrowserFlow::update_stereotype(bool)
{
    if (def != 0) {
        QString n = name;
        const char * stereotype = def->get_stereotype();

        if (n.isEmpty())
            n = "<flow>";

        if (show_stereotypes && stereotype[0]) {
            QString s = toUnicode(stereotype);
            int index = s.find(':');

            setText(0,
                    "<<" + ((index == -1) ? s : s.mid(index + 1))
                    + ">> " + n);
        }
        else
            setText(0, n);
    }
}

const QPixmap * BrowserFlow::pixmap(int) const
{
    if (deletedp())
        return DeletedRelationIcon;

    const QPixmap * px = ProfiledStereotypes::browserPixmap(def->get_stereotype());

    return (px != 0) ? px : SimpleRelationIcon;
}

void BrowserFlow::menu()
{
    Q3PopupMenu m(0, "flow");
    Q3PopupMenu toolm(0);

    MenuFactory::createTitle(m, def->definition(FALSE, TRUE));
    m.insertSeparator();

    if (!deletedp()) {
        if (!in_edition()) {
            m.setWhatsThis(m.insertItem(TR("Edit"), 0),
                           TR("to edit the <i>flow</i>, \
a double click with the left mouse button does the same thing"));

            if (!is_read_only && (edition_number == 0)) {
                m.setWhatsThis(m.insertItem(TR("Delete"), 2),
                               TR("to delete the <i>flow</i>. \
Note that you can undelete it after"));
            }

            m.insertSeparator();
        }

        QString s = def->get_end_node()->get_name();

        if (s.isEmpty())
            s = stringify(def->get_end_node()->get_type());

        m.setWhatsThis(m.insertItem(TR("Select ") + s, 7),
                       TR("to select the destination"));
        m.setWhatsThis(m.insertItem(TR("Referenced by"), 4),
                       TR("to know who reference the <i>region</i>"));
        mark_menu(m, TR("the flow"), 90);
        ProfiledStereotypes::menu(m, this, 99990);

        if ((edition_number == 0)
            && Tool::menu_insert(&toolm, get_type(), 100)) {
            m.insertSeparator();
            m.insertItem(TR("Tool"), &toolm);
        }
    }
    else if (!is_read_only && (edition_number == 0)) {
        m.setWhatsThis(m.insertItem(TR("Undelete"), 3),
                       TR("undelete the <i>flow</i> \
(except if the other side is also deleted)"));

        if (def->get_start_node()->deletedp() ||
            def->get_end_node()->deletedp())
            m.setItemEnabled(3, FALSE);
    }

    exec_menu_choice(m.exec(QCursor::pos()));
}

void BrowserFlow::exec_menu_choice(int rank)
{
    switch (rank) {
    case 0:
        open(TRUE);
        break;

    case 2:
        delete_it();
        break;

    case 3:
        BrowserNode::undelete(FALSE);
        break;

    case 4:
        ReferenceDialog::show(this);
        return;

    case 7:
        def->get_end_node()->select_in_browser();
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

void BrowserFlow::apply_shortcut(QString s)
{
    int choice = -1;

    if (!deletedp()) {
        if (!in_edition()) {
            if (s == "Edit")
                choice = 0;

            if (!is_read_only && (edition_number == 0)) {
                if (s == "Delete")
                    choice = 2;
            }
        }

        if (s == "Referenced by")
            choice = 4;
        else if (s == "Select target")
            choice = 7;
        else
            mark_shortcut(s, choice, 90);

        if (edition_number == 0)
            Tool::shortcut(s, choice, get_type(), 100);
    }
    else if (!is_read_only && (edition_number == 0))
        if (s == "Undelete")
            choice = 3;

    exec_menu_choice(choice);
}

bool BrowserFlow::in_edition() const
{
    return (def->get_start()->is_edited);
}

void BrowserFlow::open(bool)
{
    if (! in_edition())
        def->edit();
}

void BrowserFlow::modified()
{
    repaint();
    ((BrowserNode *) parent())->modified();
}

UmlCode BrowserFlow::get_type() const
{
    return UmlFlow;
}

QString BrowserFlow::get_stype() const
{
    return TR("flow");
}

int BrowserFlow::get_identifier() const
{
    return get_ident();
}

const char * BrowserFlow::help_topic() const
{
    return "flow";
}

BasicData * BrowserFlow::get_data() const
{
    return def;
}

QString BrowserFlow::full_name(bool rev, bool) const
{
    QString s = name;

    if (s.isEmpty())
        s = "<anonymous flow>";

    return fullname(s, rev);
}

bool BrowserFlow::allow_empty() const
{
    return TRUE;
}

bool BrowserFlow::same_name(const QString & s, UmlCode type) const
{
    return ((get_type() == type) &&
            (name == s) &&
            (name != "<flow>"));
}

void BrowserFlow::init()
{
    its_default_stereotypes.clear();

    its_default_stereotypes.append("interrupt");
    its_default_stereotypes.append("multicast");
    its_default_stereotypes.append("multireceive");
}

// unicode
const QStringList & BrowserFlow::default_stereotypes()
{
    return its_default_stereotypes;
}

bool BrowserFlow::api_compatible(unsigned v) const
{
    return ((v > 24) &&
            ((BrowserNode *) parent())->api_compatible(v) &&
            def->get_end_node()->api_compatible(v));
}

bool BrowserFlow::tool_cmd(ToolCom * com, const char * args)
{
    return (def->tool_cmd(com, args, this, comment) ||
            BrowserNode::tool_cmd(com, args));
}

void BrowserFlow::DropAfterEvent(QDropEvent * e, BrowserNode * after)
{
    if (after == 0)
        ((BrowserNode *) parent())->DropAfterEvent(e, this);
    else
        e->ignore();
}

QString BrowserFlow::drag_key() const
{
    return QString::number(UmlFlow)
           + "#" + QString::number((unsigned long) parent());
}

QString BrowserFlow::drag_postfix() const
{
    return "#" + QString::number((unsigned long) parent());
}

QString BrowserFlow::drag_key(BrowserNode * p)
{
    return QString::number(UmlFlow)
           + "#" + QString::number((unsigned long) p);
}

void BrowserFlow::save_stereotypes(QTextStream & st)
{
    nl_indent(st);
    st << "flow_stereotypes ";
    save_unicode_string_list(its_default_stereotypes, st);
}

void BrowserFlow::read_stereotypes(char *& st, char *& k)
{
    if (!strcmp(k, "flow_stereotypes")) {
        read_unicode_string_list(its_default_stereotypes, st);
        k = read_keyword(st);
    }
    else
        init();
}

void BrowserFlow::save(QTextStream & st, bool ref, QString & warning)
{
    if (ref) {
        // for FlowCanvas
        st << "flow_ref " << get_ident() << " // " << get_name();
    }
    else {
        nl_indent(st);
        st << "flow " << get_ident() << " ";
        save_string(name, st);
        indent(+1);
        def->save(st, warning);
        BrowserNode::save(st);
        indent(-1);
        nl_indent(st);
        st << "end";

        // for saveAs
        if (!is_from_lib() && !is_api_base())
            is_read_only = FALSE;
    }
}

BrowserFlow * BrowserFlow::read_ref(char *& st)
{
    read_keyword(st, "flow_ref");

    int id = read_id(st);
    BrowserFlow * result = all[id];

    return (result == 0)
           ? new BrowserFlow(id)
           : result;
}

BrowserFlow * BrowserFlow::read(char *& st, char * k, BrowserNode * parent)
{
    if (!strcmp(k, "flow")) {
        int id = read_id(st);
        QString s = read_string(st);

        k = read_keyword(st);

        FlowData * d = FlowData::read(st, k);	// update k
        BrowserFlow * result;

        if ((result = all[id]) == 0) {
            result = new BrowserFlow(parent, d, id);
            result->set_name(s);
        }
        else if (result->is_defined) {
            BrowserFlow * already_exist = result;

            result = new BrowserFlow(parent, d, id);
            result->set_name(s);

            already_exist->must_change_id(all);
            already_exist->unconsistent_fixed("flow", result);
        }
        else {
            if (result->def != 0)
                // re-load ???
                delete result->def;

            result->def = d;
            result->set_parent(parent);
            result->set_name(s);
        }

        result->is_defined = TRUE;

        result->is_read_only = (!in_import() && read_only_file()) ||
                               ((user_id() != 0) && result->is_api_base());

        result->BrowserNode::read(st, k, id);

        if (strcmp(k, "end"))
            wrong_keyword(k, "end");

        d->set_browser_node(result);	// call update_stereotype();

        return result;
    }
    else
        return 0;
}

BrowserNode * BrowserFlow::get_it(const char * k, int id)
{
    return (!strcmp(k, "flow_ref")) ? all[id] : 0;
}
