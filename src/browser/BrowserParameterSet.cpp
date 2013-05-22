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
#include <QDropEvent>
#include <QPixmap>

#include "BrowserParameterSet.h"
#include "ParameterSetData.h"
#include "BrowserActivityAction.h"
#include "BrowserActivityDiagram.h"
#include "ReferenceDialog.h"
#include "UmlPixmap.h"
#include "UmlGlobal.h"
#include "myio.h"
#include "ToolCom.h"
#include "Tool.h"
#include "ui/menufactory.h"
#include "strutil.h"
#include "ProfiledStereotypes.h"
#include "mu.h"
#include "translate.h"

IdDict<BrowserParameterSet> BrowserParameterSet::all(257, __FILE__);
QStringList BrowserParameterSet::its_default_stereotypes;	// unicode

BrowserParameterSet::BrowserParameterSet(QString s, BrowserNode * p, ParameterSetData * d, int id)
    : BrowserNode(s, p), Labeled<BrowserParameterSet>(all, id), def(d)
{
}

BrowserParameterSet::BrowserParameterSet(int id)
    : BrowserNode(), Labeled<BrowserParameterSet>(all, id),
      def(new ParameterSetData)
{
    // not yet read
    def->set_browser_node(this);
}

BrowserParameterSet::BrowserParameterSet(const BrowserParameterSet * model, BrowserNode * p)
    : BrowserNode(model->name, p), Labeled<BrowserParameterSet>(all, 0)
{
    def = new ParameterSetData(model->def, this);
    comment = model->comment;
}

BrowserParameterSet * BrowserParameterSet::new_one(const char * s,
        BrowserNode * p)
{
    ParameterSetData * d = new ParameterSetData();
    BrowserParameterSet * result = new BrowserParameterSet(s, p, d);

    d->set_browser_node(result);

    return result;
}

BrowserNode * BrowserParameterSet::duplicate(BrowserNode * p, QString)
{
    BrowserNode * result = new BrowserParameterSet(this, p);

    result->set_name("");
    result->update_stereotype();

    return result;
}

BrowserParameterSet::~BrowserParameterSet()
{
    all.remove(get_ident());
    delete def;
}

void BrowserParameterSet::clear(bool old)
{
    all.clear(old);
}

void BrowserParameterSet::update_idmax_for_root()
{
    all.update_idmax_for_root();
}

void BrowserParameterSet::prepare_update_lib() const
{
    all.memo_id_oid(get_ident(), original_id);
}

void BrowserParameterSet::referenced_by(QList<BrowserNode *> & l, bool ondelete)
{
    BrowserNode::referenced_by(l, ondelete);

    if (! ondelete)
        BrowserActivityDiagram::compute_referenced_by(l, this, "parametersetcanvas", "parameterset_ref");
}

void BrowserParameterSet::renumber(int phase)
{
    if (phase != -1)
        new_ident(phase, all);
}

const QPixmap * BrowserParameterSet::pixmap(int) const
{
    if (deletedp())
        return DeletedParameterSetIcon;

    const QPixmap * px = ProfiledStereotypes::browserPixmap(def->get_stereotype());

    if (px != 0)
        return px;
    else if (is_marked)
        return ParameterSetMarkedIcon;
    else
        return ParameterSetIcon;
}

void BrowserParameterSet::menu()
{
    Q3PopupMenu m(0, name);
    Q3PopupMenu toolm(0);

    MenuFactory::createTitle(m, def->definition(FALSE, TRUE));
    m.insertSeparator();

    if (!deletedp()) {
        if (!is_edited)
            m.setWhatsThis(m.insertItem(TR("Edit"), 0),
                           TR("to edit the <i>parameter set</i>, \
a double click with the left mouse button does the same thing"));

        if (!is_read_only && (edition_number == 0)) {
            m.setWhatsThis(m.insertItem(TR("Duplicate"), 1),
                           TR("to copy the <i>parameter set</i> in a new one"));
            m.insertSeparator();
            m.setWhatsThis(m.insertItem(TR("Delete"), 2),
                           TR("to delete the <i>parameter set</i>. \
Note that you can undelete it after"));
        }

        m.setWhatsThis(m.insertItem(TR("Referenced by"), 4),
                       TR("to know who reference the <i>parameter set</i>"));
        mark_menu(m, TR("the parameter set"), 90);
        ProfiledStereotypes::menu(m, this, 99990);

        if ((edition_number == 0) &&
            Tool::menu_insert(&toolm, get_type(), 100)) {
            m.insertSeparator();
            m.insertItem(TR("Tool"), &toolm);
        }
    }
    else if (!is_read_only && (edition_number == 0))
        m.setWhatsThis(m.insertItem(TR("Undelete"), 3),
                       TR("to undelete the <i>parameter set</i>"));

    exec_menu_choice(m.exec(QCursor::pos()));
}

void BrowserParameterSet::exec_menu_choice(int rank)
{
    switch (rank) {
    case 0:
        open(TRUE);
        return;

    case 1:
        ((BrowserActivityAction *) parent())->add_parameter_set(this, 0);
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

void BrowserParameterSet::apply_shortcut(QString s)
{
    int choice = -1;

    if (!deletedp()) {
        if (!is_edited)
            if (s == "Edit")
                choice = 0;

        if (s == "Referenced by")
            choice = 4;

        if (!is_read_only && (edition_number == 0)) {
            if (s == "Duplicate")
                choice = 1;
            else if (s == "Delete")
                choice = 2;
        }

        mark_shortcut(s, choice, 90);

        if (edition_number == 0)
            Tool::shortcut(s, choice, get_type(), 100);
    }
    else if (!is_read_only && (edition_number == 0))
        if (s == "Undelete")
            choice = 3;

    exec_menu_choice(choice);
}

void BrowserParameterSet::open(bool)
{
    if (!is_edited)
        def->edit();
}

void BrowserParameterSet::modified()
{
    repaint();
    ((BrowserNode *) parent())->modified();
}

UmlCode BrowserParameterSet::get_type() const
{
    return UmlParameterSet;
}

QString BrowserParameterSet::get_stype() const
{
    return TR("parameter set");
}

int BrowserParameterSet::get_identifier() const
{
    return get_ident();
}

BasicData * BrowserParameterSet::get_data() const
{
    return def;
}

bool BrowserParameterSet::allow_empty() const
{
    return TRUE;
}

QString BrowserParameterSet::full_name(bool rev, bool) const
{
    QString s = name;

    if (s.isEmpty())
        s = "<anonymous parameter set>";

    return fullname(s, rev);
}

bool BrowserParameterSet::tool_cmd(ToolCom * com, const char * args)
{
    return (def->tool_cmd(com, args, this, comment) ||
            BrowserNode::tool_cmd(com, args));
}

void BrowserParameterSet::DropAfterEvent(QDropEvent * e, BrowserNode * after)
{
    if (after == 0)
        ((BrowserNode *) parent())->DropAfterEvent(e, this);
    else
        e->ignore();
}

QString BrowserParameterSet::drag_key() const
{
    return QString::number(UmlParameterSet)
           + "#" + QString::number((unsigned long) parent());
}

QString BrowserParameterSet::drag_postfix() const
{
    return "#" + QString::number((unsigned long) parent());
}

QString BrowserParameterSet::drag_key(BrowserNode * p)
{
    return QString::number(UmlParameterSet)
           + "#" + QString::number((unsigned long) p);
}

void BrowserParameterSet::init()
{
    its_default_stereotypes.clear();
}

// unicode
const QStringList & BrowserParameterSet::default_stereotypes()
{
    return its_default_stereotypes;
}

void BrowserParameterSet::save_stereotypes(QTextStream & st)
{
    nl_indent(st);
    st << "parameterset_stereotypes ";
    save_unicode_string_list(its_default_stereotypes, st);
}

void BrowserParameterSet::read_stereotypes(char *& st, char *& k)
{
    if (!strcmp(k, "parameterset_stereotypes")) {
        read_unicode_string_list(its_default_stereotypes, st);
        k = read_keyword(st);
    }
}

void BrowserParameterSet::save(QTextStream & st, bool ref, QString & warning)
{
    if (ref)
        st << "parameterset_ref " << get_ident() << " // " << get_name();
    else {
        nl_indent(st);
        st << "parameterset " << get_ident() << " ";
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

BrowserParameterSet * BrowserParameterSet::read_ref(char *& st)
{
    read_keyword(st, "parameterset_ref");

    int id = read_id(st);
    BrowserParameterSet * result = all[id];

    return (result == 0)
           ? new BrowserParameterSet(id)
           : result;
}

BrowserParameterSet * BrowserParameterSet::read(char *& st, char * k,
        BrowserNode * parent)
{
    BrowserParameterSet * result;
    int id;

    if (!strcmp(k, "parameterset_ref")) {
        if (((result = all[id = read_id(st)]) == 0) && (parent != 0))
            result = new BrowserParameterSet(id);

        return result;
    }
    else if (!strcmp(k, "parameterset")) {
        id = read_id(st);

        QString s = read_string(st);

        k = read_keyword(st);

        if ((result = all[id]) == 0) {
            result = new BrowserParameterSet(s, parent, new ParameterSetData, id);
            result->def->read(st, k);	// updates k
        }
        else if (result->is_defined) {
            BrowserParameterSet * already_exist = result;

            result = new BrowserParameterSet(s, parent, new ParameterSetData, id);
            result->def->read(st, k);	// updates k

            already_exist->must_change_id(all);
            already_exist->unconsistent_fixed("parameter set", result);
        }
        else {
            result->def->read(st, k);	// updates k
            result->set_parent(parent);
            result->set_name(s);
        }

        result->is_defined = TRUE;

        result->is_read_only = (!in_import() && read_only_file()) ||
                               ((user_id() != 0) && result->is_api_base());
        result->def->set_browser_node(result);

        result->BrowserNode::read(st, k, id);

        if (strcmp(k, "end"))
            wrong_keyword(k, "end");

        return result;
    }
    else
        return 0;
}

BrowserNode * BrowserParameterSet::get_it(const char * k, int id)
{
    return (!strcmp(k, "parameterset_ref")) ? all[id] : 0;
}
