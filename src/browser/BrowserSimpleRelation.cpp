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

#include "BrowserSimpleRelation.h"
#include "SimpleRelationData.h"
#include "BrowserUseCase.h"
#include "ReferenceDialog.h"
#include "BrowserActivityDiagram.h"
#include "BrowserClassDiagram.h"
#include "BrowserColDiagram.h"
#include "BrowserComponentDiagram.h"
#include "BrowserDeploymentDiagram.h"
#include "BrowserObjectDiagram.h"
#include "BrowserSeqDiagram.h"
#include "BrowserStateDiagram.h"
#include "BrowserUseCaseDiagram.h"
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
#include "translate.h"

IdDict<BrowserSimpleRelation> BrowserSimpleRelation::all(257, __FILE__);

BrowserSimpleRelation::BrowserSimpleRelation(BrowserNode * p, SimpleRelationData * d, int id)
    : BrowserNode(d->definition(FALSE, FALSE), p), Labeled<BrowserSimpleRelation>(all, id),
      def(d)
{
}

BrowserSimpleRelation::BrowserSimpleRelation(const BrowserSimpleRelation * model,
        BrowserNode * p)
    : BrowserNode(model->def->definition(FALSE, FALSE), p),
      Labeled<BrowserSimpleRelation>(all, 0)
{
    def = new SimpleRelationData(model, this);
    comment = model->comment;
}

BrowserSimpleRelation::BrowserSimpleRelation(int id)
    : BrowserNode(), Labeled<BrowserSimpleRelation>(all, id)
{
    // was deleted but still referenced
    // create a pseudo definition
    def = new SimpleRelationData(UmlDependOn);

    def->set_start_end(this, this);
}

BrowserSimpleRelation::~BrowserSimpleRelation()
{
    all.remove(get_ident());

    if (def)
        delete def;
}

BrowserNode * BrowserSimpleRelation::duplicate(BrowserNode * p, QString)
{
    BrowserSimpleRelation * result = new BrowserSimpleRelation(this, p);

    result->def->get_start()->modified();
    result->def->get_start()->package_modified();
    result->update_stereotype();

    return result;
}

bool BrowserSimpleRelation::undelete(bool, QString & warning, QString & renamed)
{
    if (! deletedp())
        return FALSE;

    if (def->get_start_node()->deletedp() ||
        def->get_end_node()->deletedp()) {
        warning += QString("<li><b>") + quote(def->definition(FALSE, FALSE)) + "</b> " + TR("from") + " <b>" +
                   def->get_start_node()->full_name() +
                   "</b> " + TR("to") + " <b>" + def->get_end_node()->full_name() + "</b>\n";
        return FALSE;
    }
    else {
        switch (get_type()) {
        case UmlInherit:

            // use case
            if (!def->get_start_node()->check_inherit(def->get_end_node()).isEmpty()) {
                warning += QString("<li><b>") + quote(def->definition(FALSE, FALSE)) + "</b> "
                           + TR("because <b>%1</b> cannot (or already) inherit on <b>%2</b>",
                                def->get_start_node()->full_name(),
                                def->get_end_node()->full_name())
                           + "\n";
                return FALSE;
            }

            break;

        default:
            break;
        }
    }

    is_deleted = FALSE;
    def->undelete(warning, renamed);

    package_modified();
    repaint();

    return TRUE;
}

void BrowserSimpleRelation::referenced_by(QList<BrowserNode *> & l, bool ondelete)
{
    BrowserNode::referenced_by(l, ondelete);

    if (! ondelete) {
        BrowserActivityDiagram::compute_referenced_by(l, this, "simplerelationcanvas", "simplerelation_ref");
        BrowserClassDiagram::compute_referenced_by(l, this, "simplerelationcanvas", "simplerelation_ref");
        BrowserColDiagram::compute_referenced_by(l, this, "simplerelationcanvas", "simplerelation_ref");
        BrowserComponentDiagram::compute_referenced_by(l, this, "simplerelationcanvas", "simplerelation_ref");
        BrowserDeploymentDiagram::compute_referenced_by(l, this, "simplerelationcanvas", "simplerelation_ref");
        BrowserObjectDiagram::compute_referenced_by(l, this, "simplerelationcanvas", "simplerelation_ref");
        BrowserSeqDiagram::compute_referenced_by(l, this, "simplerelationcanvas", "simplerelation_ref");
        BrowserStateDiagram::compute_referenced_by(l, this, "simplerelationcanvas", "simplerelation_ref");
        BrowserUseCaseDiagram::compute_referenced_by(l, this, "simplerelationcanvas", "simplerelation_ref");
    }
}

void BrowserSimpleRelation::compute_referenced_by(QList<BrowserNode *> & l,
        BrowserNode * target)
{
    IdIterator<BrowserSimpleRelation> it(all);

    while (it.current()) {
        if (!it.current()->deletedp() &&
            (it.current()->def->get_end_node() == target))
            l.append(it.current());

        ++it;
    }
}

void BrowserSimpleRelation::clear(bool old)
{
    all.clear(old);
}

void BrowserSimpleRelation::update_idmax_for_root()
{
    all.update_idmax_for_root();
}

void BrowserSimpleRelation::prepare_update_lib() const
{
    all.memo_id_oid(get_ident(), original_id);
}

void BrowserSimpleRelation::renumber(int phase)
{
    if (phase != -1)
        new_ident(phase, all);
}

void BrowserSimpleRelation::update_stereotype(bool)
{
    if (def != 0) {
        QString n = def->definition(FALSE, FALSE) + " " +
                    def->get_end_node()->get_name();
        const char * stereotype = def->get_stereotype();

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

const QPixmap * BrowserSimpleRelation::pixmap(int) const
{
    if (deletedp())
        return DeletedRelationIcon;

    const QPixmap * px = ProfiledStereotypes::browserPixmap(def->get_stereotype());

    return (px != 0) ? px : SimpleRelationIcon;
}

void BrowserSimpleRelation::menu()
{
    Q3PopupMenu m(0, name);
    Q3PopupMenu toolm(0);

    MenuFactory::createTitle(m, def->definition(FALSE, TRUE));
    m.insertSeparator();

    if (!deletedp()) {
        if (!in_edition()) {
            m.setWhatsThis(m.insertItem(TR("Edit"), 0),
                           TR("to edit the <i>relation</i>, \
a double click with the left mouse button does the same thing"));

            if (!is_read_only && (edition_number == 0)) {
                m.setWhatsThis(m.insertItem(TR("Delete"), 2),
                               TR("to delete the <i>relation</i>. \
Note that you can undelete it after"));
            }

            m.insertSeparator();
        }

        m.setWhatsThis(m.insertItem(TR("Referenced by"), 8),
                       TR("to know who reference the <i>relation</i>"));

        m.setWhatsThis(m.insertItem(QString(TR("select ")) + def->get_end_node()->get_name(),
                                    7),
                       TR("to select the destination"));
        mark_menu(m, TR("the relation"), 90);
        ProfiledStereotypes::menu(m, this, 99990);

        if ((edition_number == 0)
            && Tool::menu_insert(&toolm, get_type(), 100)) {
            m.insertSeparator();
            m.insertItem(TR("Tool"), &toolm);
        }
    }
    else if (!is_read_only && (edition_number == 0)) {
        m.setWhatsThis(m.insertItem(TR("Undelete"), 3),
                       TR("undelete the <i>relation</i> \
(except if the other side is also deleted)"));

        if (def->get_start_node()->deletedp() ||
            def->get_end_node()->deletedp())
            m.setItemEnabled(3, FALSE);
    }

    exec_menu_choice(m.exec(QCursor::pos()));
}

void BrowserSimpleRelation::exec_menu_choice(int rank)
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

    case 7:
        def->get_end_node()->select_in_browser();
        return;

    case 8:
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

void BrowserSimpleRelation::apply_shortcut(QString s)
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

        if (s == "Select target")
            choice = 7;
        else if (s == "Referenced by")
            choice = 8;
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

bool BrowserSimpleRelation::in_edition() const
{
    return (def->get_start()->is_edited);
}

void BrowserSimpleRelation::open(bool)
{
    if (! in_edition())
        def->edit();
}

void BrowserSimpleRelation::modified()
{
    set_name(def->definition(FALSE, FALSE));
    update_stereotype(FALSE);
    repaint();
    ((BrowserNode *) parent())->modified();
}

UmlCode BrowserSimpleRelation::get_type() const
{
    return def->get_type();
}

QString BrowserSimpleRelation::get_stype() const
{
    return TR("relation");
}

int BrowserSimpleRelation::get_identifier() const
{
    return get_ident();
}

BasicData * BrowserSimpleRelation::get_data() const
{
    return def;
}

QString BrowserSimpleRelation::full_name(bool rev, bool) const
{
    return fullname(rev);
}

void BrowserSimpleRelation::write_id(ToolCom * com)
{
    com->write_id(this, UmlSimpleRelations - UmlRelations, name);
}

bool BrowserSimpleRelation::tool_cmd(ToolCom * com, const char * args)
{
    return (def->tool_cmd(com, args, this, comment) ||
            BrowserNode::tool_cmd(com, args));
}

void BrowserSimpleRelation::DropAfterEvent(QDropEvent * e, BrowserNode * after)
{
    if (after == 0)
        ((BrowserNode *) parent())->DropAfterEvent(e, this);
    else
        e->ignore();
}

QString BrowserSimpleRelation::drag_key() const
{
    return QString::number(UmlSimpleRelations)
           + "#" + QString::number((unsigned long) parent());
}

QString BrowserSimpleRelation::drag_postfix() const
{
    return "#" + QString::number((unsigned long) parent());
}

QString BrowserSimpleRelation::drag_key(BrowserNode * p)
{
    return QString::number(UmlSimpleRelations)
           + "#" + QString::number((unsigned long) p);
}

void BrowserSimpleRelation::save(QTextStream & st, bool ref,
                                 QString & warning)
{
    if (ref) {
        // for SimpleRelationCanvas
        st << "simplerelation_ref " << get_ident();
    }
    else {
        nl_indent(st);
        st << "simplerelation " << get_ident();
        indent(+1);
        nl_indent(st);
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

BrowserSimpleRelation * BrowserSimpleRelation::read_ref(char *& st)
{
    read_keyword(st, "simplerelation_ref");

    int id = read_id(st);
    BrowserSimpleRelation * result = all[id];

    return (result == 0)
           ? new BrowserSimpleRelation(id)
           : result;
}

BrowserSimpleRelation *
BrowserSimpleRelation::read(char *& st, char * k,
                            BrowserNode * parent)
{
    if (!strcmp(k, "simplerelation_ref")) {
        int id = read_id(st);
        BrowserSimpleRelation * result = all[id];

        return (result == 0)
               ? new BrowserSimpleRelation(id)
               : result;
    }
    else if (!strcmp(k, "simplerelation")) {
        int id = read_id(st);

        SimpleRelationData * d = SimpleRelationData::read(st);
        BrowserSimpleRelation * result;

        if ((result = all[id]) == 0)
            result = new BrowserSimpleRelation(parent, d, id);
        else if (result->is_defined) {
            BrowserSimpleRelation * already_exist = result;

            result = new BrowserSimpleRelation(parent, d, id);

            already_exist->must_change_id(all);
            already_exist->unconsistent_fixed("relation", result);
        }
        else {
            if (result->def != 0)
                // re-load ???
                delete result->def;

            result->def = d;
            result->set_parent(parent);
            result->set_name(d->definition(FALSE, FALSE));
        }

        result->is_defined = TRUE;

        result->is_read_only = !parent->is_writable() ||
                               ((user_id() != 0) && result->is_api_base());

        k = read_keyword(st);

        result->BrowserNode::read(st, k, id);

        if (strcmp(k, "end"))
            wrong_keyword(k, "end");

        d->set_browser_node(result);	// call update_stereotype();

        return result;
    }
    else
        return 0;
}

BrowserNode * BrowserSimpleRelation::get_it(const char * k, int id)
{
    return (!strcmp(k, "simplerelation_ref")) ? all[id] : 0;
}

void BrowserSimpleRelation::get_relating(BrowserNode * elt, Q3PtrDict<BrowserNode> & d,
        BrowserNodeList & newones, bool inh,
        bool dep, bool sametype, UmlCode k)
{
    IdIterator<BrowserSimpleRelation> it(all);
    BrowserSimpleRelation * r;

    for (; (r = it.current()) != 0; ++it) {
        if (!r->deletedp() &&
            (r->def->get_end_node() == elt)) {
            BrowserNode * src = r->def->get_start_node();

            if ((!sametype || (src->get_type() == k)) && (d[src] == 0)) {
                switch (r->get_type()) {
                case UmlDependOn:
                    if (! dep)
                        continue;

                    break;

                case UmlInherit:
                    if (! inh)
                        continue;

                    break;

                default:
                    continue;
                }

                d.insert(src, src);
                newones.append(src);
            }
        }
    }
}
