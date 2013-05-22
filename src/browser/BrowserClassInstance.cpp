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
//Added by qt3to4:
#include <QTextStream>
#include <Q3ValueList>
#include <QPixmap>

#include "BrowserClassInstance.h"
#include "BrowserClass.h"
#include "ClassInstanceData.h"
#include "BrowserSeqDiagram.h"
#include "BrowserColDiagram.h"
#include "BrowserObjectDiagram.h"
#include "BrowserPackage.h"
#include "BrowserAttribute.h"
#include "BrowserRelation.h"
#include "RelationData.h"
#include "UmlPixmap.h"
#include "myio.h"
#include "ToolCom.h"
#include "Tool.h"
#include "ui/menufactory.h"
#include "UmlDrag.h"
#include "ReferenceDialog.h"
#include "DialogUtil.h"
#include "ProfiledStereotypes.h"
#include "mu.h"
#include "strutil.h"
#include "translate.h"

IdDict<BrowserClassInstance> BrowserClassInstance::all(__FILE__);

BrowserClassInstance::BrowserClassInstance(QString s, BrowserClass * cl,
        BrowserNode * p, int id)
    : BrowserNode(s, p), Labeled<BrowserClassInstance>(all, id)
{
    def = new ClassInstanceData(cl);
    def->set_browser_node(this);

    associated_diagram = 0;
}

BrowserClassInstance::BrowserClassInstance(const BrowserClassInstance * model,
        BrowserNode * p)
    : BrowserNode(model->name, p), Labeled<BrowserClassInstance>(all, 0)
{
    def = new ClassInstanceData(model->def);
    def->set_browser_node(this);
    comment = model->comment;
    associated_diagram = 0;
}

BrowserClassInstance::BrowserClassInstance(int id)
    : BrowserNode(), Labeled<BrowserClassInstance>(all, id)
{
    def = new ClassInstanceData((BrowserClass *) 0);
    def->set_browser_node(this);

    associated_diagram = 0;
}

BrowserClassInstance::~BrowserClassInstance()
{
    all.remove(get_ident());
    delete def;
}

bool BrowserClassInstance::undelete(bool, QString & warning, QString & renamed)
{
    if (! deletedp())
        return FALSE;

    if (def->get_class()->deletedp()) {
        warning += QString("<li><b>") + quote(name) +
                   ":" + quote(def->get_class()->get_name()) + "</b>\n";
        return FALSE;
    }

    is_deleted = FALSE;
    def->undelete(warning, renamed);

    package_modified();
    repaint();

    return TRUE;
}

BrowserNode * BrowserClassInstance::duplicate(BrowserNode * p, QString name)
{
    BrowserClassInstance * result = new BrowserClassInstance(this, p);

    result->set_name((name.isEmpty()) ? get_name() : name);
    result->update_stereotype();

    return result;
}

void BrowserClassInstance::post_load()
{
    IdIterator<BrowserClassInstance> it(all);

    while (it.current()) {
        it.current()->def->init_other_side();
        ++it;
    }

    it.toFirst();

    while (it.current()) {
        it.current()->def->check();
        it.current()->def->check_rels();

        ++it;
    }
}

void BrowserClassInstance::clear(bool old)
{
    all.clear(old);
}

void BrowserClassInstance::update_idmax_for_root()
{
    all.update_idmax_for_root();
}

void BrowserClassInstance::prepare_update_lib() const
{
    all.memo_id_oid(get_ident(), original_id);
}

void BrowserClassInstance::renumber(int phase)
{
    if (phase != -1)
        new_ident(phase, all);
}

const QPixmap * BrowserClassInstance::pixmap(int) const
{
    if (deletedp())
        return DeletedClassInstanceIcon;

    const QPixmap * px = ProfiledStereotypes::browserPixmap(def->get_stereotype());

    return (px != 0) ? px : ClassInstanceIcon;
}

bool BrowserClassInstance::allow_empty() const
{
    return TRUE;
}

QString BrowserClassInstance::full_name(bool rev, bool) const
{
    return fullname(rev);
}

void BrowserClassInstance::update_stereotype(bool)
{
    if (def != 0) {
        const char * stereotype = def->get_stereotype();
        QString n = get_name() + QString(" : ");

        if (def->get_class() != 0)
            n += def->get_class()->get_name();

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

void BrowserClassInstance::referenced_by(QList<BrowserNode *> & l, bool ondelete)
{
    BrowserNode::referenced_by(l, ondelete);
    BrowserClassInstance::compute_referenced_by(l, this);

    if (! ondelete) {
        BrowserSeqDiagram::compute_referenced_by(l, this, "classinstancecanvas", "classinstance_ref");
        BrowserColDiagram::compute_referenced_by(l, this, "classinstancecanvas", "classinstance_ref");
        BrowserObjectDiagram::compute_referenced_by(l, this, "classinstancecanvas", "classinstance_ref");
    }
}

void BrowserClassInstance::compute_referenced_by(QList<BrowserNode *> & l,
        BrowserNode * target)
{
    IdIterator<BrowserClassInstance> it(all);

    while (it.current()) {
        if (!it.current()->deletedp()) {
            ClassInstanceData * data = it.current()->def;
            bool add = (data->get_class() == target);

            if (!add && (target->get_type() == UmlAttribute)) {
                const Q3ValueList<SlotAttr> & attrs = data->get_attributes();
                Q3ValueList<SlotAttr>::ConstIterator it_a;

                for (it_a = attrs.begin(); it_a != attrs.end(); ++it_a) {
                    if ((*it_a).att == target) {
                        add = TRUE;
                        break;
                    }
                }
            }

            if (!add) {
                const Q3ValueList<SlotRel> & rels = data->get_relations();
                Q3ValueList<SlotRel>::ConstIterator it_r;

                for (it_r = rels.begin(); it_r != rels.end(); ++it_r) {
                    const SlotRel & slot = *it_r;

                    if ((slot.is_a) ? (slot.rel->get_start() == target)
                        : (slot.rel->get_end() == target)) {
                        add = TRUE;
                        break;
                    }

                    if (slot.value == target) {
                        add = TRUE;
                        break;
                    }
                }
            }

            if (add)
                l.append(it.current());
        }

        ++it;
    }
}

void BrowserClassInstance::menu()
{
    Q3PopupMenu m(0, "class instance");
    Q3PopupMenu toolm(0);

    MenuFactory::createTitle(m, def->definition(FALSE, TRUE));
    m.insertSeparator();

    if (!deletedp()) {
        if (!is_edited) {
            m.setWhatsThis(m.insertItem(TR("Edit"), 0),
                           TR("to edit the <i>class instance</i>, \
a double click with the left mouse button does the same thing"));
            m.setWhatsThis(m.insertItem(TR("Duplicate"), 4),
                           TR("to duplicate the <i>class instance</i>"));

            if (!is_read_only && (edition_number == 0)) {
                m.insertSeparator();
                m.setWhatsThis(m.insertItem(TR("Delete"), 1),
                               TR("to delete the <i>class instance</i>. \
Note that you can undelete it after"));
            }
        }

        m.insertSeparator();
        m.setWhatsThis(m.insertItem(TR("Referenced by"), 3),
                       TR("to know who reference the <i>class instance</i> \
through a relation"));
        mark_menu(m, TR("the class instance"), 90);
        ProfiledStereotypes::menu(m, this, 99990);

        if ((edition_number == 0) &&
            Tool::menu_insert(&toolm, get_type(), 100)) {
            m.insertSeparator();
            m.insertItem(TR("Tool"), &toolm);
        }
    }
    else if (!is_read_only && (edition_number == 0)) {
        m.setWhatsThis(m.insertItem(TR("Undelete"), 2),
                       TR("to undelete the <i>class instance</i>"));

        if (def->get_class()->deletedp())
            m.setItemEnabled(2, FALSE);
    }

    exec_menu_choice(m.exec(QCursor::pos()));
}

void BrowserClassInstance::exec_menu_choice(int rank)
{
    switch (rank) {
    case 0:
        open(TRUE);
        return;

    case 1:
        delete_it();
        break;

    case 2:
        BrowserNode::undelete(FALSE);
        break;

    case 3:
        ReferenceDialog::show(this);
        return;

    case 4:
        duplicate((BrowserNode *) parent(), "")->select_in_browser();
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

    package_modified();
}

void BrowserClassInstance::apply_shortcut(QString s)
{
    int choice = -1;

    if (!deletedp()) {
        if (!is_edited) {
            if (s == "Edit")
                choice = 0;

            if (!is_read_only && (edition_number == 0)) {
                if (s == "Delete")
                    choice = 1;
            }
        }

        if (s == "Referenced by")
            choice = 3;
        else if (s == "Duplicate")
            choice = 4;

        mark_shortcut(s, choice, 90);

        if (edition_number == 0)
            Tool::shortcut(s, choice, get_type(), 100);
    }
    else if (!is_read_only && (edition_number == 0))
        if (s == "Undelete")
            choice = 2;

    exec_menu_choice(choice);
}

void BrowserClassInstance::open(bool force_edit)
{
    if (!force_edit &&
        (associated_diagram != 0) &&
        !associated_diagram->deletedp())
        associated_diagram->open(FALSE);
    else if (!is_edited)
        def->edit();
}

UmlCode BrowserClassInstance::get_type() const
{
    return UmlClassInstance;
}

QString BrowserClassInstance::get_stype() const
{
    return TR("class instance");
}

int BrowserClassInstance::get_identifier() const
{
    return get_ident();
}

const char * BrowserClassInstance::help_topic() const
{
    return "classinstance";
}

BrowserClassInstance * BrowserClassInstance::get_classinstance(BrowserNode * future_parent)
{
    BrowserNode * old;
    QString name;
    BrowserNodeList nodes;

    if (!future_parent->enter_child_name(name, TR("enter class instance's name (may be empty) : "),
                                         UmlClassInstance, instances(nodes),
                                         &old, TRUE, TRUE))
        return 0;

    if (old != 0)
        return ((BrowserClassInstance *) old);

    BrowserClass * cl = BrowserClass::get_class(future_parent);

    if (cl == 0)
        return 0;

    BrowserClassInstance * result =
        new BrowserClassInstance(name, cl, future_parent);

    future_parent->setOpen(TRUE);
    future_parent->package_modified();

    return result;
}

BrowserClassInstance * BrowserClassInstance::get_classinstance(BrowserClass * cl)
{
    BrowserNode * old;
    QString dummy;
    BrowserNodeList nodes;

    IdIterator<BrowserClassInstance> it(all);

    while (it.current() != 0) {
        if (!it.current()->deletedp() &&
            (((ClassInstanceData *)(it.current()->get_data()))->get_class() == cl))
            nodes.append(it.current());

        ++it;
    }

    nodes.sort_it();

    // use cl here but any element is good for
    return (! cl->enter_child_name(dummy, TR("choose existing instance : "),
                                   UmlClassInstance, nodes, &old,
                                   TRUE, TRUE, TRUE))
           ? 0 : ((BrowserClassInstance *) old);
}

BrowserClassInstance * BrowserClassInstance::add_classinstance(BrowserNode * future_parent)
{
    QString name;

    if (! future_parent->enter_child_name(name, TR("enter class instance's name (may be empty) : "),
                                          UmlClassInstance, TRUE, TRUE))
        return 0;

    BrowserClass * cl = BrowserClass::get_class(future_parent);

    if (cl == 0)
        return 0;

    BrowserClassInstance * result =
        new BrowserClassInstance(name, cl, future_parent);

    future_parent->setOpen(TRUE);
    future_parent->package_modified();

    return result;
}

// to add BrowserClassInstance from old diagrams
BrowserClassInstance * BrowserClassInstance::get_it(QString s, BrowserClass * cl,
        BrowserNode * parent)
{
    if (! s.isEmpty()) {
        Q3ListViewItem * child;

        for (child = parent->firstChild(); child != 0; child = child->nextSibling())
            if (!((BrowserNode *) child)->deletedp() &&
                (((BrowserNode *) child)->get_type() == UmlClassInstance) &&
                (((BrowserClassInstance *) child)->name == s) &&
                (((BrowserClassInstance *) child)->def->get_class() == cl))
                return ((BrowserClassInstance *) child);
    }

    return new BrowserClassInstance(s, cl, parent);
}

BasicData * BrowserClassInstance::get_data() const
{
    return def;
}

BrowserNodeList & BrowserClassInstance::instances(BrowserNodeList & result,
        const char * st)
{
    IdIterator<BrowserClassInstance> it(all);

    if ((st == 0) || (*st == 0)) {
        while (it.current() != 0) {
            if (!it.current()->deletedp())
                result.append(it.current());

            ++it;
        }
    }
    else {
        while (it.current() != 0) {
            if (!it.current()->deletedp() &&
                !strcmp(it.current()->get_data()->get_stereotype(), st))
                result.append(it.current());

            ++it;
        }
    }

    result.sort_it();

    return result;
}

BrowserNode * BrowserClassInstance::get_associated() const
{
    return associated_diagram;
}

void BrowserClassInstance::set_associated_diagram(BrowserObjectDiagram * dg,
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

void BrowserClassInstance::on_delete()
{
    if (associated_diagram && associated_diagram->deletedp())
        associated_diagram = 0;
}

bool BrowserClassInstance::tool_cmd(ToolCom * com, const char * args)
{
    switch ((unsigned char) args[-1]) {
    case setAssocDiagramCmd:
        if (is_read_only && !root_permission())
            com->write_ack(FALSE);
        else {
            set_associated_diagram((BrowserObjectDiagram *) com->get_id(args));
            com->write_ack(TRUE);
        }

        break;

    default:
        return (def->tool_cmd(com, args, this, comment) ||
                BrowserNode::tool_cmd(com, args));
    }

    return TRUE;
}

bool BrowserClassInstance::api_compatible(unsigned v) const
{
    return v >= 31;
}


void BrowserClassInstance::add_from_tool(BrowserNode * parent, ToolCom * com,
        const char * args)
{
    BrowserClass * cl = (BrowserClass *) com->get_id(args);
    BrowserClassInstance * ci = new BrowserClassInstance("", cl, parent);

    parent->package_modified();
    ci->write_id(com);
}

void BrowserClassInstance::save(QTextStream & st, bool ref, QString & warning)
{
    if (ref)
        st << "classinstance_ref " << get_ident() << " // " << get_name();
    else {
        nl_indent(st);
        st << "classinstance " << get_ident() << " ";
        save_string(name, st);
        indent(+1);
        def->save(st, warning);

        if (associated_diagram != 0) {
            nl_indent(st);
            st << "associated_diagram ";
            associated_diagram->save(st, TRUE, warning);
        }

        BrowserNode::save(st);

        indent(-1);
        nl_indent(st);
        st << "end";

        // for saveAs
        if (!is_from_lib() && !is_api_base())
            is_read_only = FALSE;
    }
}

BrowserClassInstance * BrowserClassInstance::read_ref(char *& st)
{
    read_keyword(st, "classinstance_ref");

    int id = read_id(st);
    BrowserClassInstance * result = all[id];

    return (result == 0)
           ? new BrowserClassInstance(id)
           : result;
}

BrowserClassInstance * BrowserClassInstance::read(char *& st, char * k,
        BrowserNode * parent)
{
    BrowserClassInstance * result;
    int id;

    if (!strcmp(k, "classinstance_ref")) {
        id = read_id(st);
        result = all[id];

        return (result == 0)
               ? new BrowserClassInstance(id)
               : result;
    }
    else if (!strcmp(k, "classinstance")) {
        id = read_id(st);
        result = all[id];

        if (result == 0)
            result =
                new BrowserClassInstance(read_string(st), (BrowserClass *) 0,
                                         parent, id);
        else if (result->is_defined) {
            BrowserClassInstance * already_exist = result;

            result =
                new BrowserClassInstance(read_string(st), (BrowserClass *) 0,
                                         parent, id);

            already_exist->must_change_id(all);
            already_exist->unconsistent_fixed("class instance", result);
        }
        else {
            result->set_parent(parent);
            result->set_name(read_string(st));
        }

        result->is_defined = TRUE;

        result->is_read_only = (!in_import() && read_only_file()) ||
                               ((user_id() != 0) && result->is_api_base());

        k = read_keyword(st);

        result->def->read(st, k);	// updates k

        if (!strcmp(k, "associated_diagram")) {
            result->set_associated_diagram(BrowserObjectDiagram::read_ref(st, read_keyword(st)),
                                           TRUE);
            k = read_keyword(st);
        }

        result->BrowserNode::read(st, k, id);	// updates k

        if (strcmp(k, "end"))
            wrong_keyword(k, "end");

        return result;
    }
    else
        return 0;
}

BrowserNode * BrowserClassInstance::get_it(const char * k, int id)
{
    return (!strcmp(k, "classinstance_ref")) ? all[id] : 0;
}
