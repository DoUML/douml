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
#include <qpainter.h>
#include <q3ptrdict.h>
//Added by qt3to4:
#include <QTextStream>
#include <QPixmap>
#include <QDropEvent>

#include "BrowserRelation.h"
#include "BrowserClassDiagram.h"
#include "BrowserObjectDiagram.h"
#include "BrowserUseCaseDiagram.h"
#include "BrowserOperation.h"
#include "BrowserClass.h"
#include "RelationData.h"
#include "ClassData.h"
#include "BrowserClassInstance.h"
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
#include "translate.h"

IdDict<BrowserRelation> BrowserRelation::all(1021, __FILE__);
static QList<BrowserRelation *> Unconsistent;

BrowserRelation::BrowserRelation(BrowserNode * p, RelationData * d, int id)
    : BrowserNode(d->get_name(), p), Labeled<BrowserRelation>(all, id),
      def(d), get_oper(0), set_oper(0)
{
}

BrowserRelation::BrowserRelation(const BrowserRelation * model,
                                 BrowserNode * p)
    : BrowserNode(model->def->get_name(), p),
      Labeled<BrowserRelation>(all, 0),
      get_oper(0), set_oper(0)
{
    def = new RelationData(model, this);
    comment = model->comment;
}

BrowserRelation::BrowserRelation(int id)
    : BrowserNode(), Labeled<BrowserRelation>(all, id), def(0), get_oper(0), set_oper(0)
{
    // for reading
}

BrowserRelation::~BrowserRelation()
{
    all.remove(get_ident());

    if (def)
        def->garbage(this);
}

bool BrowserRelation::is_undefined() const
{
    // defined under a class even when undefined
    return ((BrowserNode *) parent())->is_undefined();
}

BrowserNode * BrowserRelation::duplicate(BrowserNode * p, QString)
{
    BrowserRelation * result = new BrowserRelation(this, p);

    result->def->get_start()->modified();
    result->def->get_start()->package_modified();

    if (result->def->get_end() != 0) {
        result->def->get_end()->modified();
        result->def->get_end()->package_modified();
    }

    result->update_stereotype();
    //move(result, this);
    //result->select_in_browser();
    return result;
}

void BrowserRelation::delete_it()
{
    if (get_oper != 0)
        get_oper->delete_it();

    if (set_oper != 0)
        set_oper->delete_it();

    BrowserNode::delete_it();

    // warning, def may be 0 in case the relation was a
    // temporary invalidated
    if (def != 0) {
        BrowserRelation * b = def->get_start();

        if (b != 0) {
            b->modified();
            def->get_start_class()->modified();
            def->get_end_class()->modified();
        }
    }
}

bool BrowserRelation::undelete(bool, QString & warning, QString & renamed)
{
    if (! deletedp())
        return FALSE;

    if (def->get_start_class()->deletedp() ||
        def->get_end_class()->deletedp()) {
        if (def->is_a(this))
            warning += QString("<li><b>") + quote(def->get_name(this)) + "</b> " + TR("from") + " <b>" +
                       def->get_start_class()->full_name() +
                       "</b> " + TR("to") + " <b>" + def->get_end_class()->full_name() + "</b>\n";
        else
            warning += QString("<li><b>") + def->get_name(this) + "</b> " + TR("from") + " <b>" +
                       def->get_end_class()->full_name() +
                       "</b> " + TR("to") + " <b>" + quote(def->get_start_class()->full_name()) + "</b>\n";

        return FALSE;
    }
    else {
        switch (get_type()) {
        case UmlGeneralisation:
        case UmlRealize:
            if (!def->get_start_class()->check_inherit(def->get_end_class()).isEmpty()) {
                warning += QString("<li><b>") + quote(def->get_name(this)) + "</b> "
                           + TR("because <b>%1</b> cannot (or already) inherit on <b>%2</b>",
                                def->get_start_class()->full_name(),
                                def->get_end_class()->full_name())
                           + "\n";
                return FALSE;
            }

            break;

        default:
            break;
        }
    }

    if (def->undelete(warning, renamed, this, is_deleted)) {
        renamed += QString("<li><b>") + full_name() + "</b>\n";
        modified();
    }

    if (RelationData::isa_inherit(get_type()) &&
        !strcmp(def->get_start_class()->get_data()->get_stereotype(), "stereotype") &&
        !strcmp(def->get_end_class()->get_data()->get_stereotype(), "stereotype"))
        ProfiledStereotypes::recompute(TRUE);

    package_modified();
    repaint();

    return TRUE;
}

const char * BrowserRelation::get_comment() const
{
    return (def->is_a(this))
           ? def->get_comment_a()
           : def->get_comment_b();
}

void BrowserRelation::set_comment(const char * c)
{
    if (def->is_a(this))
        def->set_comment_a(c);
    else
        def->set_comment_b(c);
}

UmlVisibility BrowserRelation::get_visibility() const
{
    return def->get_uml_visibility_a();
}

void BrowserRelation::compute_referenced_by(QList<BrowserNode *> & l,
        BrowserClass * target)
{
    IdIterator<BrowserRelation> it(all);
    BrowserRelation * r;

    while ((r = it.current()) != 0) {
        if (!r->deletedp() &&
            (((r->def->is_a(it.current()))
              ? (r->def->get_end_class() == target)
              : (r->def->get_start_class() == target))
             || (r->def->get_association().type == target)))
            l.append(r);

        ++it;
    }
}

void BrowserRelation::referenced_by(QList<BrowserNode *> & l, bool ondelete)
{
    BrowserNode::referenced_by(l, ondelete);
    BrowserClassInstance::compute_referenced_by(l, this);

    if (! ondelete) {
        BrowserClassDiagram::compute_referenced_by(l, this, "relationcanvas", "relation_ref");
        BrowserUseCaseDiagram::compute_referenced_by(l, this, "relationcanvas", "relation_ref");
        BrowserObjectDiagram::compute_referenced_by(l, this, "objectlinkcanvas", "rel relation_ref");
    }
}

BrowserRelation * BrowserRelation::temporary(RelationData * d)
{
    BrowserRelation * result = new BrowserRelation(-1);

    result->parent()->takeItem(result);
    BrowserClass::temporary()->insertItem(result);

    result->def = d;
    return result;
}

void BrowserRelation::clear(bool old)
{
    all.clear(old);
    RelationData::clear(old);
}

void BrowserRelation::update_idmax_for_root()
{
    all.update_idmax_for_root();
    RelationData::update_idmax_for_root();
}

void BrowserRelation::prepare_update_lib() const
{
    all.memo_id_oid(get_ident(), original_id);

    if (def->is_a(this))
        def->prepare_update_lib();
}

void BrowserRelation::renumber(int phase)
{
    if (phase != -1) {
        new_ident(phase, all);
        def->renumber(phase, this);
    }
}

void BrowserRelation::set_get_oper(BrowserOperation * op)
{
    if (get_oper == 0) {
        get_oper = op;
        op->set_get_of(this);
    }
}

void BrowserRelation::set_set_oper(BrowserOperation * op)
{
    if (set_oper == 0) {
        set_oper = op;
        op->set_set_of(this);
    }
}

void BrowserRelation::update_get_oper()
{
    AType t;

    if (def->is_a(this)) {
        t.type = def->get_end_class();
        get_oper->update_get_of(def->get_role_a(), def->get_cppdecl_a(),
                                def->get_javadecl_a(),
                                def->get_phpdecl_a(),
                                def->get_pythondecl_a(),
                                def->get_idldecl_a(),
                                def->get_isa_const_relation_a(),
                                def->get_isa_class_relation_a(),
                                t, def->get_multiplicity_a(),
                                def->get_stereotype());
    }
    else {
        t.type = def->get_start_class();
        get_oper->update_get_of(def->get_role_b(), def->get_cppdecl_b(),
                                def->get_javadecl_b(),
                                def->get_phpdecl_b(),
                                def->get_pythondecl_b(),
                                def->get_idldecl_b(),
                                def->get_isa_const_relation_b(),
                                def->get_isa_class_relation_b(),
                                t, def->get_multiplicity_b(),
                                def->get_stereotype());
    }
}

void BrowserRelation::update_set_oper()
{
    AType t;

    if (def->is_a(this)) {
        t.type = def->get_end_class();
        set_oper->update_set_of(def->get_role_a(), def->get_cppdecl_a(),
                                def->get_javadecl_a(),
                                def->get_phpdecl_a(),
                                def->get_pythondecl_a(),
                                def->get_idldecl_a(),
                                def->get_isa_const_relation_a(),
                                def->get_isa_class_relation_a(),
                                t, def->get_multiplicity_a(),
                                def->get_stereotype());
    }
    else {
        t.type = def->get_start_class();
        set_oper->update_set_of(def->get_role_b(), def->get_cppdecl_b(),
                                def->get_javadecl_b(),
                                def->get_phpdecl_b(),
                                def->get_pythondecl_b(),
                                def->get_idldecl_b(),
                                def->get_isa_const_relation_b(),
                                def->get_isa_class_relation_b(),
                                t, def->get_multiplicity_b(),
                                def->get_stereotype());
    }
}

void BrowserRelation::add_get_oper()
{
    set_get_oper(BrowserOperation::new_one(QString(), (BrowserNode *) parent()));
    update_get_oper();
    ((BrowserNode *) parent())->modified();
}

void BrowserRelation::add_set_oper()
{
    set_set_oper(BrowserOperation::new_one(QString(), (BrowserNode *) parent()));
    update_set_oper();
    ((BrowserNode *) parent())->modified();
}

void BrowserRelation::update_stereotype(bool)
{
    if (def != 0) {
        QString n;

        switch (get_type()) {
        case UmlGeneralisation:
        case UmlDependency:
        case UmlRealize: {
            bool is_aVar = def->is_a(this);
            QString latterPart;

            if (is_aVar)
                latterPart = def->get_end_class()->get_name();
            else
                latterPart = def->get_start_class()->get_name();

            const char * tName = def->get_name(this);
            n = tName + QString(" ") + latterPart;

        }
        break;

        default:
            n = this->get_name();
        }

        const char * stereotype = def->get_stereotype();

        if (show_stereotypes && stereotype[0]) {
            QString s = stereotype;
            int index = s.find(':');

            setText(0,
                    "<<" + ((index == -1) ? s : s.mid(index + 1))
                    + ">> " + n);
        }
        else
            setText(0, n);
    }
}

QString BrowserRelation::stereotypes_properties() const
{
    if (def->uni_directional(def->get_type()))
        return BrowserNode::stereotypes_properties();

    QString sta = def->get_start()->BrowserNode::stereotypes_properties();
    QString stb = def->get_end()->BrowserNode::stereotypes_properties();

    if (!sta.isEmpty()) {
        sta += "\n";

        return (!stb.isEmpty())
               ? sta + stb
               : sta + QString("<<") + def->get_short_stereotype() + QString(">>");
    }
    else if (!stb.isEmpty())
        return QString("<<") + def->get_short_stereotype() +
               QString(">>\n") + stb;
    else
        return QString();
}

AType BrowserRelation::class_association() const
{
    return def->get_association();
}

const char * BrowserRelation::constraint() const
{
    return (def->is_a(this) ? def->get_constraint_a()
            : def->get_constraint_b());
}

const QPixmap * BrowserRelation::pixmap(int) const
{
    if (deletedp())
        return DeletedRelationIcon;


    const QPixmap * px = ProfiledStereotypes::browserPixmap(def->get_stereotype());

    if (px != 0)
        return px ;

    switch (def->is_a(this) ? def->get_uml_visibility_a()
            : def->get_uml_visibility_b()) {
    case UmlPublic:
        return PublicRelationIcon;

    case UmlProtected:
        return ProtectedRelationIcon;

    case UmlPrivate:
        return PrivateRelationIcon;

    default:
        return PackageRelationIcon;
    }
}

void BrowserRelation::paintCell(QPainter * p, const QColorGroup & cg, int column,
                                int width, int alignment)
{
    bool class_relation = def->is_a(this)
                          ? def->get_isa_class_relation_a() : def->get_isa_class_relation_b();

    const QColor & bg = p->backgroundColor();

    if (is_marked) {
        p->setBackgroundMode(Qt::OpaqueMode);
        p->setBackgroundColor(UmlRedColor);
    }

    p->setFont((class_relation)
               ? ((is_writable()) ? BoldUnderlineFont : UnderlineFont)
                   : ((is_writable()) ? BoldFont : NormalFont));
    Q3ListViewItem::paintCell(p, cg, column, width, alignment);

    if (is_marked) {
        p->setBackgroundMode(Qt::TransparentMode);
        p->setBackgroundColor(bg);
    }
}

void BrowserRelation::menu()
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
                if (RelationData::isa_association(def->get_type())) {
                    m.setWhatsThis(m.insertItem(TR("Duplicate"), 1),
                                   TR("to copy the <i>relation</i> in a new one"));

                    if (get_oper == 0)
                        m.setWhatsThis(m.insertItem(TR("New get operation"), 4),
                                       TR("to auto define the <i>get operation</i>"));

                    if (set_oper == 0)
                        m.setWhatsThis(m.insertItem(TR("New set operation"), 5),
                                       TR("to auto define the <i>set operation</i>"));

                    if ((get_oper == 0) && (set_oper == 0))
                        m.setWhatsThis(m.insertItem(TR("New get and set operation"), 6),
                                       TR("to auto define the <i>get</i> and <i>set operation</i>s"));

                    m.insertSeparator();
                }
            }

            m.setWhatsThis(m.insertItem(TR("Referenced by"), 8),
                           TR("to know who reference the <i>relation</i>"));
            m.insertSeparator();

            if (!is_read_only && (edition_number == 0)) {
                m.setWhatsThis(m.insertItem(TR("Delete"), 2),
                               TR("to delete the <i>relation</i>. \
Note that you can undelete it after"));
            }

            m.insertSeparator();
        }

        m.setWhatsThis(m.insertItem(QString(TR("select ")) +
                                    ((def->is_a(this))
                                     ? def->get_end_class()
                                     : def->get_start_class())->get_name(),
                                    7),
                       TR("to select the destination class"));
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
(except if the class at the other side is also deleted)"));

        if (def->get_start_class()->deletedp() ||
            def->get_end_class()->deletedp())
            m.setItemEnabled(3, FALSE);
    }

    exec_menu_choice(m.exec(QCursor::pos()));
}

void BrowserRelation::exec_menu_choice(int rank)
{
    switch (rank) {
    case 0:
        open(TRUE);
        return;

    case 1:
        ((BrowserClass *) parent())->add_relation(this);
        return;

    case 2:
        delete_it();

        if (RelationData::isa_inherit(get_type()) &&
            !strcmp(def->get_start_class()->get_data()->get_stereotype(),
                    "stereotype") &&
            !strcmp(def->get_end_class()->get_data()->get_stereotype(),
                    "stereotype"))
            ProfiledStereotypes::recompute(TRUE);

        break;

    case 3:
        BrowserNode::undelete(FALSE);

        if (RelationData::isa_inherit(get_type()) &&
            !strcmp(def->get_start_class()->get_data()->get_stereotype(), "stereotype") &&
            !strcmp(def->get_end_class()->get_data()->get_stereotype(), "stereotype"))
            ProfiledStereotypes::recompute(TRUE);

        break;

    case 4:
        add_get_oper();
        break;

    case 6:
        add_get_oper();

        // no break !
    case 5:
        add_set_oper();
        break;

    case 7:
        ((def->is_a(this)) ? def->get_end_class()
         : def->get_start_class())->select_in_browser();
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

void BrowserRelation::apply_shortcut(QString s)
{
    int choice = -1;

    if (!deletedp()) {
        if (!in_edition()) {
            if (s == "Edit")
                choice = 0;

            if (!is_read_only && (edition_number == 0)) {
                if (RelationData::isa_association(def->get_type())) {
                    if (s == "Duplicate")
                        choice = 1;

                    if (get_oper == 0)
                        if (s == "New get operation")
                            choice = 4;

                    if (set_oper == 0)
                        if (s == "New set operation")
                            choice = 5;

                    if ((get_oper == 0) && (set_oper == 0))
                        if (s == "New get and set operation")
                            choice = 6;
                }

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

bool BrowserRelation::in_edition() const
{
    return (def->get_start()->is_edited ||
            ((def->get_end() != 0) && def->get_end()->is_edited));
}

void BrowserRelation::open(bool)
{
    if (! in_edition())
        def->edit();
}

void BrowserRelation::modified()
{
    const char * newName = def->get_name(this);
    set_name(newName);
    update_stereotype(FALSE);
    repaint();

    if (get_oper != 0)
        update_get_oper();

    if (set_oper != 0)
        update_set_oper();

    ((BrowserNode *) parent())->modified();
}

UmlCode BrowserRelation::get_type() const
{
    return (def == 0)
           ? UmlRelations	// legal case to manage get/set operation read
           : def->get_type();
}

QString BrowserRelation::get_stype() const
{
    return TR("relation");
}

int BrowserRelation::get_identifier() const
{
    return get_ident();
}

const char * BrowserRelation::help_topic() const
{
    return "relation";
}

BasicData * BrowserRelation::get_data() const
{
    return def;
}

bool BrowserRelation::same_name(const QString & s, UmlCode t) const
{
    if ((t == UmlAttribute) || IsaRelation(t)) {
        QString role = name;

        if (role[0] != '(')
            // role have name, compare with the role name
            // followed by " " then '(' then the relation's name
            return (s == role.left(role.find(" ")));
    }

    return FALSE;
}

QString BrowserRelation::full_name(bool rev, bool) const
{
    QString s = (def->is_a(this)) ? def->get_role_a() : def->get_role_b();

    if (s.isEmpty())
        s = name;

    return fullname(s, rev);
}

void BrowserRelation::member_cpp_def(const QString & prefix, const QString &,
                                     QString & s, bool templ) const
{
    if (! templ) {
        if (def->is_a(this)) {
            const char * decl = def->get_cppdecl_a();

            if (def->get_isa_class_relation_a() && decl[0]) {
                s += prefix;
                s += true_name(def->get_role_a(), decl);
                s += "\n\n";
            }
        }
        else {
            const char * decl = def->get_cppdecl_b();

            if (def->get_isa_class_relation_b() && decl[0]) {
                s += prefix;
                s += true_name(def->get_role_b(), decl);
                s += "\n\n";
            }
        }
    }
}

void BrowserRelation::write_id(ToolCom * com)
{
    com->write_id(this, 0, name);
}

bool BrowserRelation::tool_cmd(ToolCom * com, const char * args)
{
    ClassData::ToolCmd dummy;

    switch ((unsigned char) args[-1]) {
    case isWritableCmd:
        com->write_bool(def->is_writable(this));
        break;

    case addGetOperCmd:
        if (def->is_writable(this) && (get_oper == 0)) {
            add_get_oper();
            com->write_ack(TRUE);
        }
        else
            com->write_ack(FALSE);

        break;

    case addSetOperCmd:
        if (def->is_writable(this) && (set_oper == 0)) {
            add_set_oper();
            com->write_ack(TRUE);
        }
        else
            com->write_ack(FALSE);

        break;

    case moveAfterCmd:
    case moveInCmd:
        switch (get_type()) {
        case UmlGeneralisation:
        case UmlRealize:
            // may change order
            ClassData::setDontUpdateActuals(FALSE);

        default:
            break;
        }

        return BrowserNode::tool_cmd(com, args);

    default:
        return (def->tool_cmd(com, this, args) ||
                BrowserNode::tool_cmd(com, args));
    }

    // cmd managed
    return TRUE;
}

void BrowserRelation::DropAfterEvent(QDropEvent * e, BrowserNode * after)
{
    if (after == 0)
        ((BrowserNode *) parent())->DropAfterEvent(e, this);
    else
        e->ignore();
}

QString BrowserRelation::drag_key() const
{
    return QString::number(UmlRelations)
           + "#" + QString::number((unsigned long) parent());
}

QString BrowserRelation::drag_postfix() const
{
    return "#" + QString::number((unsigned long) parent());
}

QString BrowserRelation::drag_key(BrowserNode * p)
{
    return QString::number(UmlRelations)
           + "#" + QString::number((unsigned long) p);
}

// Removes the relation from its parent tree, delete it
// Needed to not show all the sub classes of a class and directional
// relation which are not navigable in the direction of the relation
BrowserNode * BrowserRelation::extract()
{
    BrowserNode * p = ((BrowserNode *) parent());

    p->takeItem(this);
    def = 0;

    if (get_oper != 0) {
        p->takeItem(get_oper);
        delete get_oper;
    }

    if (set_oper != 0) {
        p->takeItem(set_oper);
        delete set_oper;
    }

    delete this;
    return p;
}

// the relation returns bi-directionnal
BrowserRelation * BrowserRelation::reinsert(BrowserNode * p, RelationData * d)
{
    p->package_modified();
    return new BrowserRelation(p, d);
}

void BrowserRelation::save(QTextStream & st, bool ref, QString & warning)
{
    if (ref)
        st << "classrelation_ref " << get_ident() << " // " << get_name();
    else {
        nl_indent(st);
        st << "classrelation " << get_ident() << " // " << get_name();
        indent(+1);

        if (!def->is_a(this)) {
            nl_indent(st);
            def->save(st, TRUE, warning);
        }
        else
            def->save(st, FALSE, warning);

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

void BrowserRelation::post_load()
{
    RelationData::post_load();

    IdIterator<BrowserRelation> it(all);
    BrowserRelation * br;

    while ((br = it.current()) != 0) {
        if (!br->is_undefined()) {
            // not yet moved in UndefinedNodePackage

            RelationData * d;
            BrowserRelation * endrel;

            if ((d = br->def) == 0)
                // end -> start relation, not read
                br->must_be_deleted();
            else if (d->get_start() == 0) {
                // end -> start read, but start -> end not read
                d->set_unconsistent();
            }
            else if (d->get_type() == UmlRelations) {
                // temporary data, rel not read,
                br->delete_it();	// to update diagrams
                d->set_unconsistent();
            }
            else if ((d->get_start() == br) &&
                     ((endrel = d->get_end()) != 0) &&
                     (endrel->def != d)) {
                // br is the 'forward' direction of a bi dir relation,
                // endrel is the 'backward' direction not read
                if (endrel->def != 0)
                    endrel->def->set_unconsistent();

                d->set_unconsistent();
            }
            else {
                // not a temporary or unconsistent relation
                if ((br->get_oper != 0) && br->get_oper->is_undefined())
                    // operation was deleted
                    br->get_oper = 0;

                if ((br->set_oper != 0) && br->set_oper->is_undefined())
                    // operation was deleted
                    br->set_oper = 0;
            }
        }

        ++it;
    }

    foreach (BrowserRelation *br, Unconsistent) {
        br->def->set_unconsistent();
        br->def = 0;
        br->must_be_deleted();
    }
    Unconsistent.clear();

    if (RelationData::has_unconsistencies()) {
        IdIterator<BrowserRelation> it2(all);

        while ((br = it2.current()) != 0) {
            if ((br->def != 0) && br->def->unconsistentp()) {
                br->def = 0; // to not call garbage
                br->must_be_deleted();
            }

            ++it2;
        }

        RelationData::delete_unconsistent();
    }
}

// Created for a relation data ref read before its definition
// The browser node become useless when the data definition
// is read
void BrowserRelation::invalidate()
{
    def = 0;
}

BrowserRelation * BrowserRelation::read_ref(char *& st)
{
    read_keyword(st, "classrelation_ref");
    return all[read_id(st)];
}

BrowserRelation * BrowserRelation::read_ref(char *& st, char * k)
{
    if (strcmp(k, "classrelation_ref"))
        wrong_keyword(k, "classrelation_ref");

    int id = read_id(st);
    BrowserRelation * result = all[id];

    return (result == 0)
           ? new BrowserRelation(id)
           : result;
}

BrowserRelation * BrowserRelation::read(char *& st, char * k,
                                        BrowserNode * parent,
                                        bool force)
{
    BrowserRelation * result;
    int id;

    if (! strcmp(k, "classrelation_ref")) {
        if (((result = all[id = read_id(st)]) == 0) && force)
            result = new BrowserRelation(id);

        return result;
    }
    else if (!strcmp(k, "classrelation")) {
        id = read_id(st);
        k = read_keyword(st);

        BrowserRelation * unconsistent;   // data id shared
        RelationData * d = RelationData::read(st, k, unconsistent);	// updates k
        BrowserRelation * already_exist = 0;

        if ((result = all[id]) == 0)
            result = new BrowserRelation(parent, d, id);
        else if (result->is_defined) {
            already_exist = result;
            result = new BrowserRelation(parent, d, id);
            d->set_unconsistent();
        }
        else {
            if (result->def != 0)
                // re-load
                delete result->def;

            result->def = d;
            result->set_parent(parent);
            result->set_name(d->get_name(result));
        }

        if (already_exist != 0) {
            result->unconsistent_removed("relation", already_exist);
            Unconsistent.append(already_exist);
        }

        if ((unconsistent != 0) && (unconsistent != already_exist))
            result->unconsistent_removed("relation", unconsistent);

        result->is_defined = TRUE;
        result->is_read_only = !parent->is_writable() ||
                               ((user_id() != 0) && result->is_api_base());

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

        result->update_stereotype();

        return result;
    }
    else
        return 0;
}

BrowserNode * BrowserRelation::get_it(const char * k, int id)
{
    return (!strcmp(k, "classrelation_ref")) ? all[id] : 0;
}

//elt isa class
void BrowserRelation::get_relating(BrowserNode * elt, Q3PtrDict<BrowserNode> & d,
                                   BrowserNodeList & newones,
                                   bool inh, bool dep, bool assoc)
{
    IdIterator<BrowserRelation> it(all);
    BrowserRelation * r;

    for (; (r = it.current()) != 0; ++it) {
        if (!r->deletedp() &&
            (r->def->get_end_class() == elt)) {
            BrowserNode * src = r->def->get_start_class();

            if (d[src] == 0) {
                switch (r->get_type()) {
                case UmlDependency:
                    if (! dep)
                        continue;

                    break;

                case UmlGeneralisation:
                case UmlRealize:
                    if (! inh)
                        continue;

                    break;

                default:
                    if (! assoc)
                        continue;

                    break;
                }

                d.insert(src, src);
                newones.append(src);
            }
        }
    }
}
