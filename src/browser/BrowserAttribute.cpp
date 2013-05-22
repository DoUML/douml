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
#include <qpainter.h>
#include <qcursor.h>
//Added by qt3to4:
#include <QTextStream>
#include <QPixmap>
#include <QDropEvent>

#include "BrowserAttribute.h"
#include "BrowserOperation.h"
#include "BrowserClass.h"
#include "BrowserView.h"
#include "AttributeData.h"
#include "ClassData.h"
#include "BrowserActivityAction.h"
#include "UmlPixmap.h"
#include "UmlGlobal.h"
#include "myio.h"
#include "ToolCom.h"
#include "Tool.h"
#include "ui/menufactory.h"
#include "strutil.h"
#include "ReferenceDialog.h"
#include "DialogUtil.h"
#include "ProfiledStereotypes.h"
#include "mu.h"
#include "translate.h"

IdDict<BrowserAttribute> BrowserAttribute::all(1021, __FILE__);
QStringList BrowserAttribute::its_default_stereotypes;	// unicode

BrowserAttribute::BrowserAttribute(QString s, BrowserNode * p, AttributeData * d, int id)
    : BrowserNode(s, p), Labeled<BrowserAttribute>(all, id),
      def(d), get_oper(0), set_oper(0)
{
}

BrowserAttribute::BrowserAttribute(int id)
    : BrowserNode(), Labeled<BrowserAttribute>(all, id),
      def(new AttributeData), get_oper(0), set_oper(0)
{
    // not yet read
    def->set_browser_node(this, FALSE, FALSE);
}

BrowserAttribute::BrowserAttribute(const BrowserAttribute * model, BrowserNode * p)
    : BrowserNode(model->name, p), Labeled<BrowserAttribute>(all, 0),
      get_oper(0), set_oper(0)
{
    def = new AttributeData(model->def, this);
    comment = model->comment;
}

BrowserAttribute * BrowserAttribute::new_one(QString s, BrowserNode * p,
        bool enum_item)
{
    AttributeData * d = new AttributeData();
    BrowserAttribute * result = new BrowserAttribute(s, p, d);

    d->set_browser_node(result, TRUE, enum_item);

    return result;
}

BrowserNode * BrowserAttribute::duplicate(BrowserNode * p, QString name)
{
    BrowserAttribute * result = new BrowserAttribute(this, p);

    if (name.isEmpty())
    {
        result->set_name(get_name());
        result->def->replace((BrowserClass *) parent(), (BrowserClass *) p);
    }
    else
        result->set_name(name);

    result->update_stereotype();
    //move(result, this);
    //result->select_in_browser();

    return result;
}

BrowserAttribute::~BrowserAttribute()
{
    all.remove(get_ident());
    delete def;
}

uint BrowserAttribute::TypeID()
{
    return TypeIdentifier<BrowserAttribute>::id();
}

void BrowserAttribute::clear(bool old)
{
    all.clear(old);
}

void BrowserAttribute::update_idmax_for_root()
{
    all.update_idmax_for_root();
}

void BrowserAttribute::prepare_update_lib() const
{
    all.memo_id_oid(get_ident(), original_id);
}

void BrowserAttribute::renumber(int phase)
{
    if (phase != -1)
        new_ident(phase, all);
}

void BrowserAttribute::delete_it()
{
    BrowserNode::delete_it();

    if (get_oper != 0)
        get_oper->delete_it();

    if (set_oper != 0)
        set_oper->delete_it();
}

void BrowserAttribute::init()
{
    its_default_stereotypes.clear();
    its_default_stereotypes.append("list");
    its_default_stereotypes.append("set");
    its_default_stereotypes.append("vector");
    its_default_stereotypes.append("map");
}

void BrowserAttribute::set_get_oper(BrowserOperation * op)
{
    if (get_oper == 0) {
        get_oper = op;
        op->set_get_of(this);
    }
}

void BrowserAttribute::set_set_oper(BrowserOperation * op)
{
    if (set_oper == 0) {
        set_oper = op;
        op->set_set_of(this);
    }
}

void BrowserAttribute::update_get_oper()
{
    get_oper->update_get_of(name, def->get_cppdecl(),
                            def->get_javadecl(), def->get_phpdecl(),
                            def->get_pythondecl(), def->get_idldecl(),
                            def->get_isa_const_attribute(),
                            def->get_isa_class_attribute(),
                            def->get_type(), def->get_multiplicity(),
                            def->get_stereotype());
}

void BrowserAttribute::update_set_oper()
{
    set_oper->update_set_of(name, def->get_cppdecl(),
                            def->get_javadecl(), def->get_phpdecl(),
                            def->get_pythondecl(), def->get_idldecl(),
                            def->get_isa_const_attribute(),
                            def->get_isa_class_attribute(),
                            def->get_type(), def->get_multiplicity(),
                            def->get_stereotype());
}

void BrowserAttribute::add_get_oper()
{
    set_get_oper(BrowserOperation::new_one(QString(), (BrowserNode *) parent()));
    update_get_oper();
    ((BrowserNode *) parent())->modified();
}

void BrowserAttribute::add_set_oper()
{
    set_set_oper(BrowserOperation::new_one(QString(), (BrowserNode *) parent()));
    update_set_oper();
    ((BrowserNode *) parent())->modified();
}

const char * BrowserAttribute::constraint() const
{
    return def->get_constraint();
}

const QPixmap * BrowserAttribute::pixmap(int) const
{
    if (deletedp())
        return DeletedAttributeIcon;

    const QPixmap * px = ProfiledStereotypes::browserPixmap(def->get_stereotype());

    if (px != 0)
        return px;
    else {
        switch (def->get_uml_visibility()) {
        case UmlPublic:
            return PublicAttributeIcon;

        case UmlProtected:
            return ProtectedAttributeIcon;

        case UmlPrivate:
            return PrivateAttributeIcon;

        default:
            return PackageAttributeIcon;
        }
    }
}

void BrowserAttribute::paintCell(QPainter * p, const QColorGroup & cg, int column,
                                 int width, int alignment)
{
    const QColor & bg = p->backgroundColor();

    if (is_marked) {
        p->setBackgroundMode(Qt::OpaqueMode);
        p->setBackgroundColor(UmlRedColor);
    }

    p->setFont((def->get_isa_class_attribute())
               ? ((is_writable()) ? BoldUnderlineFont : UnderlineFont)
                   : ((is_writable()) ? BoldFont : NormalFont));
    Q3ListViewItem::paintCell(p, cg, column, width, alignment);

    if (is_marked) {
        p->setBackgroundMode(Qt::TransparentMode);
        p->setBackgroundColor(bg);
    }
}

void BrowserAttribute::menu()
{
    const char * st = ((BrowserClass *) parent())->get_stereotype();
    bool item = (!strcmp(st, "enum_pattern") || !strcmp(st, "enum")) &&
                strcmp(get_stereotype(), "attribute");
    Q3PopupMenu m(0, name);
    Q3PopupMenu toolm(0);

    MenuFactory::createTitle(m, def->definition(FALSE, TRUE));
    m.insertSeparator();

    if (!deletedp()) {
        if (!is_edited)
            if (get_container(UmlClass) != 0)
                m.setWhatsThis(m.insertItem(TR("Up"), 20),
                               TR("to return to parent node"));

        m.setWhatsThis(m.insertItem(TR("Edit"), 0),
                       (item)
                       ? TR("to edit the <i>item</i>,"
                            "a double click with the left mouse button does the same thing")
                       : TR("to edit the <i>attribute</i>,"
                            "a double click with the left mouse button does the same thing"));

        if (!is_read_only && (edition_number == 0)) {
            if (!item && (get_oper == 0))
                m.setWhatsThis(m.insertItem(TR("New get operation"), 3),
                               TR("to auto define the <i>get operation</i>"));

            if (!item && (set_oper == 0))
                m.setWhatsThis(m.insertItem(TR("New set operation"), 4),
                               TR("to auto define the <i>set operation</i>"));

            if (!item && (get_oper == 0) && (set_oper == 0))
                m.setWhatsThis(m.insertItem(TR("New get and set operation"), 5),
                               TR("to auto define the <i>get</i> and <i>set operation</i>s"));

            m.setWhatsThis(m.insertItem(TR("Duplicate"), 6),
                           TR("to copy the <i>attribute</i> in a new one"));
        }

        m.insertSeparator();
        m.setWhatsThis(m.insertItem(TR("Referenced by"), 7),
                       TR("to know who reference the <i>class</i>"));

        if (!is_read_only && (edition_number == 0)) {
            m.insertSeparator();
            m.setWhatsThis(m.insertItem(TR("Delete"), 1),
                           (item) ? TR("to delete the <i>item</i>. \
                                       Note that you can undelete it after")
                           : TR("to delete the <i>attribute</i>. \
                                            Note that you can undelete it after"));
        }

        mark_menu(m, TR("the attribute"), 90);
        ProfiledStereotypes::menu(m, this, 99990);

        if ((edition_number == 0) &&
            Tool::menu_insert(&toolm, get_type(), 100)) {
            m.insertSeparator();
            m.insertItem(TR("Tool"), &toolm);
        }
    }
    else if (!is_read_only && (edition_number == 0))
        m.setWhatsThis(m.insertItem(TR("Undelete"), 2),
                       (item) ? TR("to undelete the <i>item</i>")
                       : TR("to undelete the <i>attribute</i>"));

    exec_menu_choice(m.exec(QCursor::pos()));
}

void BrowserAttribute::exec_menu_choice(int rank)
{
    switch (rank) {
    case 0:
        open(TRUE);
        return;

    case 1:
        if (!strcmp(((BrowserNode *) parent())->get_data()->get_stereotype(),
                    "stereotype"))
            ProfiledStereotypes::deleted(this);

        delete_it();
        break;

    case 2:
        undelete(FALSE);

        if (!strcmp(((BrowserNode *) parent())->get_data()->get_stereotype(),
                    "stereotype"))
            // the deletion was may be not propaged
            ProfiledStereotypes::recompute(TRUE);

        break;

    case 3:
        add_get_oper();
        break;

    case 5:
        add_get_oper();

        // no break !
    case 4:
        add_set_oper();
        break;

    case 6:
    {
        BrowserNode * attrDuplicate = ((BrowserClass *) parent())->duplicate_attribute(this);
        move(attrDuplicate, this);
        attrDuplicate->select_in_browser();
        return;
    }

    case 7:
        ReferenceDialog::show(this);
        return;

    case 20: {
        BrowserView::deselect(this);
        BrowserView::select(get_container(UmlClass));
    }

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

void BrowserAttribute::apply_shortcut(QString s)
{
    int choice = -1;
    const char * st = ((BrowserClass *) parent())->get_stereotype();
    bool item = (!strcmp(st, "enum_pattern") || !strcmp(st, "enum")) &&
                strcmp(get_stereotype(), "attribute");

    if (!deletedp()) {
        if (!is_edited)
            if (s == "Edit")
                choice = 0;

        if (!is_read_only && (edition_number == 0)) {
            if (!item && (get_oper == 0))
                if (s == "New get operation")
                    choice = 3;

            if (!item && (set_oper == 0))
                if (s == "New set operation")
                    choice = 4;

            if (!item && (get_oper == 0) && (set_oper == 0))
                if (s == "New get and set operation")
                    choice = 5;
        }

        if (s == "Referenced by")
            choice = 7;

        if (!is_read_only && (edition_number == 0)) {
            if (s == "Delete")
                choice = 1;
            else if (s == "Duplicate")
                choice = 6;
        }

        mark_shortcut(s, choice, 90);

        if (edition_number == 0)
            Tool::shortcut(s, choice, get_type(), 100);
    }
    else if (!is_read_only && (edition_number == 0))
        if (s == "Undelete")
            choice = 2;

    exec_menu_choice(choice);
}

void BrowserAttribute::open(bool)
{
    if (!is_edited)
        def->edit(FALSE);
}

void BrowserAttribute::open_new_ste_attr()
{
    def->edit(TRUE);
}

void BrowserAttribute::modified()
{
    repaint();
    ((BrowserNode *) parent())->modified();
    def->modified();

    if (get_oper != 0)
        update_get_oper();

    if (set_oper != 0)
        update_set_oper();
}

UmlCode BrowserAttribute::get_type() const
{
    return UmlAttribute;
}

QString BrowserAttribute::get_stype() const
{
    return TR("attribute");
}

int BrowserAttribute::get_identifier() const
{
    return get_ident();
}

const char * BrowserAttribute::help_topic() const
{
    return "attribute";
}

BasicData * BrowserAttribute::get_data() const
{
    return def;
}

bool BrowserAttribute::allow_spaces() const
{
    return FALSE;
}

bool BrowserAttribute::same_name(const QString & s, UmlCode t) const
{
    return (((t == UmlAttribute) || IsaRelation(t)) && (name == s));
}

QString BrowserAttribute::full_name(bool rev, bool) const
{
    return fullname(rev);
}

void BrowserAttribute::member_cpp_def(const QString & prefix, const QString &,
                                      QString & s, bool templ) const
{
    if (! templ) {
        const char * decl = def->get_cppdecl();

        if (def->get_isa_class_attribute() && decl[0]) {
            s += prefix;
            s += true_name(get_name(), decl);
            s += "\n\n";
        }
    }
}

void BrowserAttribute::compute_referenced_by(QList<BrowserNode *> & l,
        BrowserNode * target)
{
    IdIterator<BrowserAttribute> it(all);

    while (it.current()) {
        if (!it.current()->deletedp()) {
            const AType & t = it.current()->def->get_type();

            if (t.type == target)
                l.append(it.current());
        }

        ++it;
    }
}

void BrowserAttribute::referenced_by(QList<BrowserNode *> & l, bool ondelete)
{
    BrowserNode::referenced_by(l, ondelete);

    if (! ondelete)
        BrowserActivityAction::compute_referenced_by(l, this);
}

bool BrowserAttribute::tool_cmd(ToolCom * com, const char * args)
{
    ClassData::ToolCmd dummy;

    return (def->tool_cmd(com, args, this, comment) ||
            BrowserNode::tool_cmd(com, args));
}

void BrowserAttribute::DropAfterEvent(QDropEvent * e, BrowserNode * after)
{
    if (after == 0)
        ((BrowserNode *) parent())->DropAfterEvent(e, this);
    else
        e->ignore();
}

// unicode
const QStringList & BrowserAttribute::default_stereotypes()
{
    return its_default_stereotypes;
}

void BrowserAttribute::post_load()
{
    // to manage deleted get/set operation
    IdIterator<BrowserAttribute> it(all);
    BrowserAttribute * at;

    while ((at = it.current()) != 0) {
        if ((at->get_oper != 0) && at->get_oper->is_undefined())
            // operation was deleted
            at->get_oper = 0;

        if ((at->set_oper != 0) && at->set_oper->is_undefined())
            // operation was deleted
            at->set_oper = 0;

        ++it;
    }
}

void BrowserAttribute::save_stereotypes(QTextStream & st)
{
    nl_indent(st);
    st << "attribute_stereotypes ";
    save_unicode_string_list(its_default_stereotypes, st);
}

void BrowserAttribute::read_stereotypes(char *& st, char *& k)
{
    if (!strcmp(k, "attribute_stereotypes") ||
        !strcmp(k, "attribut_stereotypes")) {
        read_unicode_string_list(its_default_stereotypes, st);
        k = read_keyword(st);
    }
}

void BrowserAttribute::save(QTextStream & st, bool ref, QString & warning)
{
    if (ref)
        st << "attribute_ref " << get_ident() << " // " << get_name();
    else {
        nl_indent(st);
        st << "attribute " << get_ident() << " ";
        save_string(name, st);
        indent(+1);
        def->save(st, warning);

        if (get_oper != 0) {
            nl_indent(st);
            st << "get_oper ";
            get_oper->save(st, TRUE, warning);
        }

        if (set_oper != 0) {
            nl_indent(st);
            st << "set_oper ";
            set_oper->save(st, TRUE, warning);
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

BrowserAttribute * BrowserAttribute::read_ref(char *& st)
{
    read_keyword(st, "attribut_ref", "attribute_ref");

    int id = read_id(st);
    BrowserAttribute * result = all[id];

    return (result == 0)
           ? new BrowserAttribute(id)
           : result;
}

BrowserAttribute * BrowserAttribute::read(char *& st, char * k,
        BrowserNode * parent,
        bool force)
{
    BrowserAttribute * result;
    int id;

    if (!strcmp(k, "attribute_ref") || !strcmp(k, "attribut_ref")) {
        if (((result = all[id = read_id(st)]) == 0) && force)
            result = new BrowserAttribute(id);

        return result;
    }
    else if (!strcmp(k, "attribute") || !strcmp(k, "attribut")) {
        id = read_id(st);

        QString s = read_string(st);

        k = read_keyword(st);

        if ((result = all[id]) == 0) {
            result = new BrowserAttribute(s, parent, new AttributeData, id);
            result->def->read(st, k);	// updates k2
        }
        else if (result->is_defined) {
            BrowserAttribute * already_exist = result;

            result = new BrowserAttribute(s, parent, new AttributeData, id);
            result->def->read(st, k);	// updates k2

            already_exist->must_change_id(all);
            already_exist->unconsistent_fixed("attribute", result);
        }
        else {
            result->def->read(st, k);	// updates k2
            result->set_parent(parent);
            result->set_name(s);
        }

        result->is_defined = TRUE;

        result->is_read_only = !parent->is_writable() ||
                               ((user_id() != 0) && result->is_api_base());
        result->def->set_browser_node(result, FALSE, FALSE);

        if (!strcmp(k, "get_oper")) {
            BrowserOperation * oper = BrowserOperation::read_ref(st);

            if (oper != 0)
                result->set_get_oper(oper);

            k = read_keyword(st);
        }

        if (!strcmp(k, "set_oper")) {
            BrowserOperation * oper = BrowserOperation::read_ref(st);

            if (oper != 0)
                result->set_set_oper(oper);

            k = read_keyword(st);
        }

        result->BrowserNode::read(st, k, id);

        if (strcmp(k, "end"))
            wrong_keyword(k, "end");

        return result;
    }
    else
        return 0;
}

BrowserNode * BrowserAttribute::get_it(const char * k, int id)
{
    return (!strcmp(k, "attribute_ref")) ? all[id] : 0;
}
