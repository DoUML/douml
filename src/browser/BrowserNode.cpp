// *************************************************************************
//
// Copyright 2004-2010 Bruno PAGES  .
// Copyright 2012-2013 Nikolai Marchenko.
// Copyright 2012-2013 Leonardo Guilherme.
//
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

#include <q3dragobject.h>
#include <qcursor.h>
#include <q3ptrdict.h>
#include <qpainter.h>
#include <q3popupmenu.h>
#include <qapplication.h>
//Added by qt3to4:
#include <QTextStream>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <Q3PtrCollection>

#include "BrowserView.h"
#include "BrowserNode.h"
#include "BrowserUseCase.h"
#include "BrowserState.h"
#include "BrowserDiagram.h"
#include "BrowserOperation.h"
#include "BrowserActivity.h"
#include "BrowserRelation.h"
#include "BrowserArtifact.h"
#include "BrowserAttribute.h"
#include "BrowserComponent.h"
#include "BrowserDeploymentNode.h"
#include "BrowserSimpleRelation.h"
#include "SimpleRelationData.h"
#include "BrowserClass.h"
#include "BrowserClassInstance.h"
#include "BrowserActivityPartition.h"
#include "BasicData.h"
#include "BrowserPackage.h"
#include "BrowserView.h"
#include "UmlWindow.h"
#include "MyInputDialog.h"
#include "BasicDialog.h"
#include "UmlGlobal.h"
#include "myio.h"
#include "ToolCom.h"
#include "SaveProgress.h"
#include "strutil.h"
#include "DialogUtil.h"
#include "mu.h"
#include "ProfiledStereotypes.h"
#include "BrowserSearchDialog.h"
#include "ReferenceDialog.h"
#include "OperationData.h"
#include "translate.h"
#include "../Logging/QsLog.h"

QList<UmlCode> BrowserNode::generatable_types;
QList<BrowserNode *> BrowserNode::marked_list;

static BrowserPackage * UndefinedNodePackage;

bool BrowserNode::show_stereotypes = TRUE;
unsigned BrowserNode::edition_number;

// to solve problem on some windows configurations
bool BrowserNode::popup_menu_active;

// indicate saving  progress
SaveProgress * BrowserNode::save_progress;
int BrowserNode::must_be_saved_counter;
int BrowserNode::already_saved;

// to accelerate sorting
Q3PtrDict<QString> SynonymousPath(259);

// to accelerate full_name
QString BrowserNode::FullPathPrefix = "   [";
QString BrowserNode::FullPathPostfix = "]";
QString BrowserNode::FullPathDotDot = "::";
BrowserView* BrowserNode::viewptr = 0;
BrowserNode::BrowserNode(QString s, BrowserView * parent)
    : Q3ListViewItem(parent, s),
      name(s)
{
    viewptr = parent;
}

BrowserNode::BrowserNode(QString s, BrowserNode * parent)
    : Q3ListViewItem(parent, s),
      name(s)
{

    // move it at end
    Q3ListViewItem * child = parent->firstChild();

    while (child->nextSibling())
        child = child->nextSibling();

    if (child != this)
        moveItem(child);
}

BrowserNode::BrowserNode()
    : Q3ListViewItem(UndefinedNodePackage, "<not yet read>")
{
}

BrowserNode::BrowserNode(Q3ListView * view)
    : Q3ListViewItem(view, "<not yet read>")
{
}

BrowserNode::~BrowserNode()
{
    if (is_marked)
        marked_list.remove(this);
}

bool BrowserNode::is_undefined() const
{
    return (parent() == UndefinedNodePackage);
}

void BrowserNode::delete_it()
{
    if (! deletedp()) {
        if (UndefinedNodePackage == 0) {
            // not during a read
            QString warning;

            if (!delete_internal(warning)) {
                warning = full_name() + TR(" cannot be deleted because :") + warning;

                warn(warning);
            }
            else {
                // mark modified to not delete the associated file(s)
                // on exit if no save was done before
                is_modified = TRUE;
            }

            if (BrowserSearchDialog::get() != 0)
                BrowserSearchDialog::get()->update();

            if (ReferenceDialog::get() != 0)
                ReferenceDialog::get()->update();
        }
        else {
            // during a read : no check
            is_deleted = TRUE;

            if (is_marked) {
                marked_list.remove(this);
                is_marked = FALSE;
            }

            if (is_writable())
                package_modified();

            if (get_data() != 0)
                get_data()->delete_it();

            // delete the sub elts
            Q3ListViewItem * child;

            for (child = firstChild(); child != 0; child = child->nextSibling())
                if (!((BrowserNode *) child)->deletedp())
                    ((BrowserNode *) child)->delete_it();

            setOpen(FALSE);
            must_be_deleted();
        }
    }
}

void BrowserNode::set_deleted(bool value)
{
    if(value)
        delete_it();
    else
        undelete(true);

}

bool BrowserNode::delete_internal(QString & warning)
{
    if (deletedp())
        return TRUE;

    if (!is_writable() && !root_permission() && !is_from_lib()) {
        warning += "\n    " + TR("%1 is read-only", full_name());
        return FALSE;
    }

    static QList<BrowserNode *> targetof;
    static bool made = FALSE;
    bool made_here;

    if (!made) {
        made = TRUE;
        made_here = TRUE;
        referenced_by(targetof, TRUE);
    }
    else
        made_here = FALSE;

    bool ro = FALSE;

    if (!root_permission()) {
        foreach (BrowserNode *r, targetof) {
            if (!r->is_writable() && !r->is_from_lib()) {
                ro = TRUE;
                warning += "\n    ";

                switch (r->get_type()) {
                case UmlComponent:
                    warning += TR("%1 referenced by the read-only component %2", full_name(), r->full_name());
                    break;

                case UmlArtifact:
                    warning += TR("%1 referenced by the read-only artifact %2", full_name(), r->full_name());
                    break;

                default:
                    warning += TR("%1 is the target of the read-only relation %2", full_name(), r->full_name());
                    break;
                }
            }
        }
    }

    if (made_here) {
        made = FALSE;
        targetof.clear();
    }

    if (ro)
        return FALSE;

    // sub elts
    Q3ListViewItem * child;
    bool ok = TRUE;

    for (child = firstChild(); child != 0; child = child->nextSibling())
        ok &= ((BrowserNode *) child)->delete_internal(warning);

    if (ok) {
        is_deleted = TRUE;

        if (is_marked) {
            marked_list.remove(this);
            is_marked = FALSE;
        }

        get_data()->delete_it();
        repaint();
        package_modified();
    }

    return ok;
}

const char * BrowserNode::get_comment() const
{
    return comment.operator const char *();
}

void BrowserNode::set_comment(QString c)
{
    comment = c;
}

void BrowserNode::referenced_by(QList<BrowserNode *> & l, bool)
{
    BrowserSimpleRelation::compute_referenced_by(l, this);
}

AType BrowserNode::class_association() const
{
    AType r;

    return r;
}

const char * BrowserNode::constraint() const
{
    return "";
}

const char * BrowserNode::help_topic() const
{
    return ((BrowserNode *) parent())->help_topic();
}

// undelete entry operation

void BrowserNode::undelete(bool recursive)
{
    QString warning;
    QString renamed;

    if (undelete(recursive, warning, renamed) && recursive) {
        // Redo it because now all classes are undeleted but it is
        // possible that some relations was not undeleted because
        // at least one of the two extremities was not undeleted
        warning = QString();
        undelete(recursive, warning, renamed);
    }

    if (!warning.isEmpty())
        warning = "<p>" + TR("Some items cannot be undeleted") + " :\n<ul>"
                  + warning + "</ul></p>";

    if (! renamed.isEmpty()) {
        warning += "<p>" + TR("Some items are renamed") + " :\n<ul>"
                   + renamed + "</ul></p>";
    }

    if (!warning.isEmpty())
        warn(warning);
}

// undelete internal processing

bool BrowserNode::undelete(bool rec, QString & warning, QString & renamed)
{
    bool result;

    if (deletedp()) {
        // undelete the node
        QString s = name;
        bool ren = FALSE;

        while (((BrowserNode *)parent())
               ->wrong_child_name(s, get_type(),
                                  allow_spaces(), allow_empty())) {
            s = "_" + s;
            ren = TRUE;
        }

        is_deleted = FALSE;
        is_modified = TRUE;
        get_data()->undelete(warning, renamed);

        if (ren) {
            set_name(s);
            renamed += QString("<li><b>") + full_name() + "</b>\n";
        }

        result = TRUE;
        package_modified();
    }
    else
        result = FALSE;

    if (rec) {
        // undelete the sub elts
        Q3ListViewItem * child;

        for (child = firstChild(); child != 0; child = child->nextSibling())
            result |= ((BrowserNode *) child)->undelete(rec, warning, renamed);
    }

    if (result)
        repaint();

    return result;
}

bool BrowserNode::in_edition() const
{
    return !is_edited;
}

void BrowserNode::edit_start()
{
    edition_number += 1;
    is_edited = TRUE;
}

void BrowserNode::edit_end()
{
    edition_number -= 1;
    is_edited = FALSE;
}

bool BrowserNode::is_writable() const
{
    return !is_read_only;
}

void BrowserNode::set_name(QString  s)
{
    bool firsttime = name.isEmpty();
    name = s;

        if (! firsttime)
            // else set by BrowserNode::post_load()
            update_stereotype();
}

//

void BrowserNode::update_stereotype(bool rec)
{
    BasicData * data = get_data();

    if (data != 0) {
        const char * stereotype = data->get_stereotype();

        if (show_stereotypes && stereotype[0]) {
            QString s = toUnicode(stereotype);
            int index = s.find(':');

            QString str = ((index == -1) ? s : s.mid(index + 1));
            setText(0, "<<" + str + ">> " + name);
        }
        else
            setText(0, name);
    }

    if (rec) {
        Q3ListViewItem * child;

        for (child = firstChild(); child != 0; child = child->nextSibling())
            ((BrowserNode *) child)->update_stereotype(TRUE);
    }
}

const char * BrowserNode::get_stereotype() const
{
    return get_data()->get_stereotype();
}

bool BrowserNode::toggle_show_stereotypes()
{
    show_stereotypes = !show_stereotypes;

    BrowserNode * prj = BrowserView::get_project();

    if (prj != 0)
        prj->update_stereotype(TRUE);

    return show_stereotypes;
}

void BrowserNode::iconChanged()
{
    // do nothing
}

QString BrowserNode::stereotypes_properties() const
{
    const char * stereotype = get_data()->get_stereotype();

    if (show_stereotypes &&
        stereotype[0] &&
        ProfiledStereotypes::isModeled(stereotype)) {
        int nk = (int) get_n_keys();
        QString nl("\n");
        QString result;

        for (int ik = 0; ik != nk; ik += 1) {
            const char * k =  get_key(ik);
            const char * p;
            unsigned nseps = 0;

            for (p = k; *p; p += 1) {
                if (*p == ':') {
                    nseps += 1;
                    k = p + 1;
                }
            }

            if (nseps == 2) {
                p = get_value(ik);

                if (p && *p)
                    result += nl + k + QString("=") + p;
            }
        }

        if (! result.isEmpty())
            return QString("<<") + get_data()->get_short_stereotype() +
                   QString(">>") + result;
    }

    return QString();
}

void BrowserNode::paintCell(QPainter * p, const QColorGroup & cg, int column,
                            int width, int alignment)
{
    const QColor & bg = p->backgroundColor();

    if (is_marked) {
        p->setBackgroundMode(::Qt::OpaqueMode);
        p->setBackgroundColor(UmlRedColor);
    }

    p->setFont((is_writable()) ? BoldFont : NormalFont);
    Q3ListViewItem::paintCell(p, cg, column, width, alignment);

    if (is_marked) {
        p->setBackgroundMode(::Qt::TransparentMode);
        p->setBackgroundColor(bg);
    }

}

void BrowserNode::pre_load()
{
    UndefinedNodePackage =
        new BrowserPackage("<temporary package>", BrowserView::get_project(), -1);
}

void BrowserNode::post_load(bool light)
{
    UmlWindow::set_commented(0);

    signal_unconsistencies();

    bool redo;

    do {
        BrowserClass::post_load();
        BrowserClassInstance::post_load();
        BrowserActivityPartition::post_load(); // must be call after other ones

        Q3ListViewItem * child;

        redo = FALSE;

        for (child = UndefinedNodePackage->firstChild();
             child;
             child = child->nextSibling()) {
            if (!((BrowserNode *) child)->deletedp()) {
                ((BrowserNode *) child)->delete_it();
                redo = TRUE;
            }
        }
    }
    while (redo);

    // note : don't delete UndefinedNodePackage because of QT bug :
    // deleted elements are find among old parent children !
    BrowserPackage * undefpack = UndefinedNodePackage;

    UndefinedNodePackage = 0;

    if (! light) {
        ProfiledStereotypes::post_load();
        BrowserView::get_project()->update_stereotype(TRUE);
    }

    delete undefpack;
}

void BrowserNode::must_be_deleted()
{
    if (parent() != UndefinedNodePackage) {
        parent()->takeItem(this);
        UndefinedNodePackage->insertItem(this);
        UmlWindow::historic_forget(this);
    }
}

void BrowserNode::set_parent(Q3ListViewItem * p)
{
    parent()->takeItem(this);
    p->insertItem(this);

    // move it at end
    Q3ListViewItem * child = p->firstChild();

    while (child->nextSibling())
        child = child->nextSibling();

    if (child != this)
        moveItem(child);
}

void BrowserNode::on_close()
{
    // does nothing
}

uint BrowserNode::TypeID()
{
   return TypeIdentifier<BrowserNode>::id();
}

BrowserNode * BrowserNode::get_container(UmlCode k) const
{
    BrowserNode * p  = (BrowserNode *) this;

    while ((p != 0) && (p->get_type() != k))
        p = (BrowserNode *) p->parent();

    return p;
}

BrowserNode * BrowserNode::container(UmlCode c) const
{
    // currently only for class, state machine and activity
    BrowserNode * p = (BrowserNode *) parent();

    return (p == 0) ? 0 : p->container(c);
}

//

QString BrowserNode::full_name(bool, bool) const
{
    return name;
}

void BrowserNode::modified()
{
    package_modified();
    repaint();
}

void BrowserNode::on_delete()
{
    // does nothing at this level
}

void BrowserNode::get_classdiagramsettings(ClassDiagramSettings & d) const
{
    ((BrowserNode *) parent())->get_classdiagramsettings(d);
}

void BrowserNode::get_usecasediagramsettings(UseCaseDiagramSettings & d) const
{
    ((BrowserNode *) parent())->get_usecasediagramsettings(d);
}

void BrowserNode::get_sequencediagramsettings(SequenceDiagramSettings & d) const
{
    ((BrowserNode *) parent())->get_sequencediagramsettings(d);
}

void BrowserNode::get_collaborationdiagramsettings(CollaborationDiagramSettings & d) const
{
    ((BrowserNode *) parent())->get_collaborationdiagramsettings(d);
}

void BrowserNode::get_objectdiagramsettings(ObjectDiagramSettings & d) const
{
    ((BrowserNode *) parent())->get_objectdiagramsettings(d);
}

void BrowserNode::get_statediagramsettings(StateDiagramSettings & d) const
{
    ((BrowserNode *) parent())->get_statediagramsettings(d);
}

void BrowserNode::get_activitydiagramsettings(ActivityDiagramSettings & d) const
{
    ((BrowserNode *) parent())->get_activitydiagramsettings(d);
}

void BrowserNode::get_componentdiagramsettings(ComponentDiagramSettings & d) const
{
    ((BrowserNode *) parent())->get_componentdiagramsettings(d);
}

void BrowserNode::get_deploymentdiagramsettings(DeploymentDiagramSettings & d) const
{
    ((BrowserNode *) parent())->get_deploymentdiagramsettings(d);
}

UmlColor BrowserNode::get_color(UmlCode who) const
{
    return ((BrowserNode *) parent())->get_color(who);
}

UmlVisibility BrowserNode::get_visibility(UmlCode who) const
{
    return ((BrowserNode *) parent())->get_visibility(who);
}

QString BrowserNode::visibility_as_string() const
{
    return stringify(get_visibility());
}

UmlVisibility BrowserNode::encode_visibility(QString val)
{
    return visibility(val);
}

void BrowserNode::package_settings(BooL &, ShowContextMode &) const
{
    // never called
}

//

QString BrowserNode::drag_key() const
{
    return QString::number(get_type());
}

QString BrowserNode::drag_postfix() const
{
    return QString();
}

void BrowserNode::DragMoveEvent(QDragMoveEvent * e)
{
    ((BrowserNode *) parent())->DragMoveInsideEvent(e);
}

void BrowserNode::DropEvent(QDropEvent * e)
{
    ((BrowserNode *) parent())->DropAfterEvent(e, this);
}

void BrowserNode::DragMoveInsideEvent(QDragMoveEvent * e)
{
    e->ignore();
}

void BrowserNode::DropAfterEvent(QDropEvent * e, BrowserNode *)
{
    e->ignore();
    msg_critical(TR("Error"), TR("Forbidden"));
}

BrowserNode *BrowserNode::get_first_generatable_node()
{
    BrowserNode * result = nullptr;
    BrowserNode * testSubject = this;
    while(!generatable_types.contains(testSubject->get_type()))
    {
        testSubject = (BrowserNode *)testSubject->parent();
        if(testSubject == 0)
        {
            break;
        }
    }
    result = testSubject;
    return result;
}

QList<BrowserNode *> BrowserNode::get_generation_list()
{
    QList<BrowserNode *> result;
    if (marked_list.count() > 0)
    {
        foreach (BrowserNode *current, marked_list) {
            BrowserNode * temp = current->get_first_generatable_node();
            if (temp != 0 && !result.contains(temp))
                result << temp;
        }
    }
    else
        result << this->get_first_generatable_node();
    return result;
}

//

void BrowserNode::open(bool)
{
    // does nothing
}

BasicData * BrowserNode::add_relation(UmlCode t, BrowserNode * end)
{
    SimpleRelationData * d = new SimpleRelationData(t);
    BrowserSimpleRelation * br = new BrowserSimpleRelation(this, d);

    d->set_start_end(br, end);

    br->modified();	// updates name

    return d;
}

void BrowserNode::edit(const char * s, const QStringList & default_stereotypes)
{
    if (!is_edited) {
        static QSize previous_size[BrowserNodeSup];

        (new BasicDialog(get_data(), s, default_stereotypes,
                         previous_size[get_type()]))
        ->show();
    }
}

//

// returns all parents for NON class
QList<BrowserNode *> BrowserNode::parents() const
{
    QList<BrowserNode *> l;
    Q3ListViewItem * child;

    for (child = firstChild(); child != 0; child = child->nextSibling()) {
        BrowserNode * ch = ((BrowserNode *) child);

        switch (ch->get_type()) {
        case UmlInherit:
            if (!ch->deletedp())
                l.append(((SimpleRelationData *) ch->get_data())
                         ->get_end_node());

        default:
            break;
        }
    }

    return l;
}

// check inheritance
QString BrowserNode::check_inherit(const BrowserNode * new_parent) const
{
    QList<BrowserNode *> all_parents;
    QList<BrowserNode *> notyet = parents();

    if (notyet.contains(const_cast<BrowserNode *>(new_parent)))
        return TR("already generalize / realize");

    notyet.append(const_cast<BrowserNode *>(new_parent));

    do {
        BrowserNode *cl = notyet.takeFirst();

        if (cl == this)
            return TR("can't have circular generalization / realization");

        if (! all_parents.contains(cl)) {
            all_parents.append(cl);

            foreach (BrowserNode *parent, cl->parents())
                if (! notyet.contains(parent))
                    notyet.append(parent);
        }
    }
    while (! notyet.isEmpty());

    return 0;
}

bool BrowserNode::may_contains_them(const QList<BrowserNode *> & ,
                                    BooL & ) const
{
    //for inheritance
    return FALSE;
}

bool BrowserNode::may_contains_it(BrowserNode * bn) const
{
    // for the type point of view bn is legal for 'this'
    UmlCode type = bn->get_type();
    QString s = (const char *) bn->name;

    for (Q3ListViewItem * child = firstChild(); child; child = child->nextSibling()) {
        if (!((BrowserNode *) child)->deletedp() &&
            ((BrowserNode *) child)->same_name(s, type))
            return FALSE;
    }

    return TRUE;
}

//

// only simple items are duplicated, and they are duplicated
// recurssively :
#define SIMPLE_DUPLICATION

void BrowserNode::mark_menu(Q3PopupMenu & m, const char * s, int bias) const
{
    if (! is_marked) {
        m.insertSeparator();
        m.setWhatsThis(m.insertItem(TR("Mark"), bias),
                       TR("to mark %1", s));

        if (!marked_list.isEmpty()) {
            bool parents_marked = FALSE;
            const BrowserNode * bn = this;

            while (bn != BrowserView::get_project()) {
                bn = (BrowserNode *) bn->parent();

                if (bn->is_marked) {
                    parents_marked = TRUE;
                    break;
                }
            }

            bool moveable = TRUE;
#ifndef SIMPLE_DUPLICATION
            bool rec = FALSE;
#endif
            bool moveInsideSameClass = true;
            foreach (BrowserNode *node, marked_list) {
                if(node->parent() != this->parent())
                    moveInsideSameClass = false;
                if ((node == BrowserView::get_project()) ||
                    !((BrowserNode *) node->parent())->is_writable()) {
                    moveable = FALSE;
#ifndef SIMPLE_DUPLICATION
                }

                if (node->firstChild() != 0) {
                    rec = TRUE;
#else
                    break;
#endif
                }
            }

            BooL duplicable_into = TRUE;
            BooL duplicable_after = TRUE;
            bool canContain = may_contains_them(marked_list, duplicable_into);
            bool into = canContain && is_writable();
            bool thisIsntProject = this != BrowserView::get_project();
            bool isWritable = true;
            bool parentCanContain = false;
            if(((BrowserNode *) parent()) != 0) {
                isWritable = ((BrowserNode *) parent())->is_writable();
                parentCanContain = ((BrowserNode *) parent())->may_contains_them(marked_list, duplicable_after);
            }
            bool after = thisIsntProject && isWritable &&  ( parentCanContain || moveInsideSameClass);

            if (!parents_marked) {
                if (moveable) {
                    if (into)
                        m.setWhatsThis(m.insertItem(TR("Move marked into"), bias + 3),
                                       TR("to move the marked items into %1", s));

                    if (after)
                        m.setWhatsThis(m.insertItem(TR("Move marked after"), bias + 4),
                                       TR("to move the marked items after %1", s));
                    m.insertSeparator();
                }
            }

            if (into && duplicable_into) {
                m.setWhatsThis(m.insertItem(TR("Duplicate marked into"), bias + 5),
                               TR("to duplicate the marked items into %1", s));
#ifndef SIMPLE_DUPLICATION

                if (rec && !parents_marked)
                    m.setWhatsThis(m.insertItem(TR("Duplicate recursivelly marked into"), bias + 6),
                                   TR("to recurcivelly duplicate the marked items into %1", s));

#endif
            }

            if (after && duplicable_after) {
                m.setWhatsThis(m.insertItem(TR("Duplicate marked after"), bias + 7),
                               TR("to duplicate the marked items after %1", s));
#ifndef SIMPLE_DUPLICATION

                if (rec && !parents_marked)
                    m.setWhatsThis(m.insertItem(TR("Duplicate marked recursivelly after"), bias + 8),
                                   TR("to recurcivelly duplicate the marked items after %1", s));

#endif
            }
        }
    }
    else {
        m.insertSeparator();
        m.setWhatsThis(m.insertItem(TR("Unmark"), bias + 1),
                       TR("to unmark %1", s));

        if (!marked_list.isEmpty())
            m.setWhatsThis(m.insertItem(TR("Unmark all"), bias + 2),
                           QString(TR("to unmark all the marked items")));
    }
}

void BrowserNode::mark_shortcut(QString s, int & index, int bias)
{
    if (! is_marked) {
        if (s == "Mark")
            index = bias;
    }
    else {
        if (s == "Unmark")
            index = bias + 1;
    }

    if (s == "Unmark all")
        index = bias + 2;
}

void BrowserNode::mark_management(int choice)
{
    BrowserNode * bn;

    switch (choice) {
    case 0:
    case 1:
        // mark / unmark
        toggle_mark();
        return;

    case 2:
        // unmark all
        unmark_all();
        return;

    case 3:	// move into
        for (int i = marked_list.size() - 1; i >= 0; --i) {
            bn = marked_list[i];
            if (bn->get_type() == UmlAttribute )
            {
                BrowserAttribute* asAttribute =  dynamic_cast<BrowserAttribute*>(bn);
                if(((BrowserNode *) asAttribute->get_get_oper()))
                    move(((BrowserNode *) asAttribute->get_get_oper()), 0);
                if(((BrowserNode *) asAttribute->get_set_oper()))
                    move(((BrowserNode *) asAttribute->get_set_oper()), 0);
            }
            else if (bn->get_type() >= UmlAggregation &&  bn->get_type() <= UmlDirectionalAggregationByValue)
            {
                BrowserRelation* asRelation =  dynamic_cast<BrowserRelation*>(bn);
                if(((BrowserNode *) asRelation->get_get_oper()))
                    move(((BrowserNode *) asRelation->get_get_oper()), 0);
                if(((BrowserNode *) asRelation->get_set_oper()))
                    move(((BrowserNode *) asRelation->get_set_oper()), 0);
            }
            //QLOG_INFO() << stringify(bn->get_type());
            move(bn, 0);
        }

        break;

    case 4: {	// move after
        BrowserNode * p = (BrowserNode *) parent();

        for (int i = marked_list.size() - 1; i >= 0; --i) {
            bn = marked_list[i];
            //BrowserView::removeItem(p->get_)
//            BrowserAttribute* asAttribute = ((BrowserAttribute *) bn);
//            if(p != (BrowserNode *) bn->parent() && asAttribute != 0)
//            {
//                p->move(((BrowserNode *) asAttribute->get_get_oper()), this);
//                p->move(((BrowserNode *) asAttribute->get_set_oper()), this);
//            }

            if(p != (BrowserNode *) bn->parent())
            {
                if (bn->get_type() == UmlAttribute )
                {
                    BrowserAttribute* asAttribute =  dynamic_cast<BrowserAttribute*>(bn);
                    if(((BrowserNode *) asAttribute->get_get_oper()))
                        p->move(((BrowserNode *) asAttribute->get_get_oper()), this);
                    if(((BrowserNode *) asAttribute->get_set_oper()))
                        p->move(((BrowserNode *) asAttribute->get_set_oper()), this);
                }
                else if (bn->get_type() >= UmlAggregation &&  bn->get_type() <= UmlDirectionalAggregationByValue)
                {
                    BrowserRelation* asRelation =  dynamic_cast<BrowserRelation*>(bn);
                    if(((BrowserNode *) asRelation->get_get_oper()))
                        p->move(((BrowserNode *) asRelation->get_get_oper()), this);
                    if(((BrowserNode *) asRelation->get_set_oper()))
                        p->move(((BrowserNode *) asRelation->get_set_oper()), this);
                }
            }

            p->move(bn, this);

        }
    }
    break;

    case 5:	// duplicate into
        for (int i = marked_list.size() - 1; i >= 0; --i)
        {
            bn = marked_list[i];
            BrowserNode* getOperCopy = nullptr;
            BrowserNode* setOperCopy = nullptr;
            if((BrowserNode *) bn->parent()  != this)
            {
                BrowserNode* nodeCopy = bn->duplicate(this);
                move(nodeCopy, this);
                nodeCopy->select_in_browser();

                if (nodeCopy->get_type() == UmlAttribute)
                {
                    BrowserAttribute* asAttribute =  dynamic_cast<BrowserAttribute*>(bn);
                    getOperCopy = ((BrowserNode *) asAttribute->get_get_oper())->duplicate(this);
                    setOperCopy = ((BrowserNode *) asAttribute->get_set_oper())->duplicate(this);
                    move(getOperCopy, 0);
                    move(setOperCopy, 0);

                    ((BrowserAttribute *) nodeCopy)->set_get_oper((BrowserOperation *) getOperCopy);
                    ((BrowserAttribute *) nodeCopy)->set_set_oper((BrowserOperation *) setOperCopy);
                }
                else if(bn->get_type() >= UmlAggregation &&  bn->get_type() <= UmlDirectionalAggregationByValue)
                {
                    BrowserRelation* asRelation =  dynamic_cast<BrowserRelation*>(bn);
                    BrowserNode* getOper=((BrowserNode *) asRelation->get_get_oper());
                    BrowserNode* setOper=((BrowserNode *) asRelation->get_get_oper());
                    if(getOper)
                        getOperCopy = getOper->duplicate(this);
                    if(setOper)
                        setOperCopy = setOper->duplicate(this);
                    if(getOperCopy)
                    {
                        move(getOperCopy, 0);
                        ((BrowserRelation *) nodeCopy)->set_get_oper((BrowserOperation *) getOperCopy);
                    }
                    if(setOperCopy)
                    {
                        move(setOperCopy, 0);
                        ((BrowserRelation *) nodeCopy)->set_set_oper((BrowserOperation *) setOperCopy);
                    }
                }
                if((BrowserOperation*)setOperCopy)
                {
                    ((OperationData*)((BrowserOperation*)setOperCopy)->get_data())->set_get_or_set(true);
                    ((BrowserOperation*)setOperCopy)->set_set_of(nodeCopy);
                }
                if((BrowserOperation*)getOperCopy)
                {
                    ((OperationData*)((BrowserOperation*)setOperCopy)->get_data())->set_get_or_set(true);
                    ((BrowserOperation*)getOperCopy)->set_get_of(nodeCopy);
                }
            }
        }

        break;
#ifndef SIMPLE_DUPLICATION

    case 6:	// duplicate recursivelly into
        break;
#endif

    case 7: {	// duplicate after
        BrowserNode * p = (BrowserNode *) parent();

        for (int i = marked_list.size() - 1; i >= 0; --i) {
            bn = marked_list[i];
            p->move(bn->duplicate(p), this);
        }
    }
    break;
#ifndef SIMPLE_DUPLICATION

    case 8:	// duplicate recursivelly after
        break;
#endif
    }
}

void BrowserNode::unmark_all()
{
    foreach (BrowserNode * bn, marked_list) {
        bn->is_marked = FALSE;
        bn->repaint();
    }

    marked_list.clear();

    if (BrowserSearchDialog::get() != 0)
        BrowserSearchDialog::get()->update();

    if (ReferenceDialog::get() != 0)
        ReferenceDialog::get()->update();

    if(marked_list.count() != 0)
        UmlWindow::set_marked_generation();
    else
        UmlWindow::set_selected_generation();
    if(viewptr)
        viewptr->send_marked(get_marked_nodes());
}

void BrowserNode::move(BrowserNode * bn, BrowserNode * after)
{
    BrowserNode * old_parent = (BrowserNode *) bn->parent();

    if (after)
        bn->moveItem(after);
    else {
        old_parent->takeItem(bn);
        insertItem(bn);
    }

    package_modified();

    if (old_parent != this) {
        old_parent->package_modified();
        bn->modified();
    }
}

void BrowserNode::toggle_mark()
{
    if (is_marked) {
        is_marked = FALSE;
        marked_list.remove(this);
    }
    else {
        is_marked = TRUE;
        marked_list.append(this);
    }

    repaint();

    if (BrowserSearchDialog::get() != 0)
        BrowserSearchDialog::get()->update();

    if (ReferenceDialog::get() != 0)
        ReferenceDialog::get()->update();
    if(marked_list.count() != 0)
        UmlWindow::set_marked_generation();
    else
        UmlWindow::set_selected_generation();
    if(viewptr)
        viewptr->send_marked(get_marked_nodes());
}

void BrowserNode::set_marked(bool value)
{
    is_marked = value;
    if (value)
        marked_list.append(this);
    else
        marked_list.remove(this);

    //repaint();

//    if (BrowserSearchDialog::get() != 0)
//        BrowserSearchDialog::get()->update();

//    if (ReferenceDialog::get() != 0)
//        ReferenceDialog::get()->update();
    if(marked_list.count() != 0)
        UmlWindow::set_marked_generation();
    else
        UmlWindow::set_selected_generation();
    if(viewptr)
        viewptr->send_marked(get_marked_nodes());
}

void BrowserNode::setup_generatable_types()
{
    generatable_types << UmlClass << UmlComponent << UmlDeploymentNode << UmlArtifact
                         << UmlClassView << UmlComponentView << UmlDeploymentView << UmlPackage << UmlProject;
}

//

bool BrowserNode::may_contains(BrowserNode * bn, bool rec) const
{
    // for the type point of view bn is legal for 'this'
    if (((BrowserNode *) bn->parent()) == this)
        return TRUE;

    QString s = (const char *) bn->name;

    if (! s.isEmpty()) {
        UmlCode type = bn->get_type();

        for (Q3ListViewItem * child = firstChild(); child; child = child->nextSibling()) {
            if (!((BrowserNode *) child)->deletedp() &&
                // case already check : (child != bn) &&
                ((BrowserNode *) child)->same_name(s, type))
                return FALSE;
        }
    }

    if (!rec)
        return TRUE;

    const BrowserNode * pa = this;

    for (;;) {
        if (pa == 0)
            return TRUE;
        else if (pa == bn)
            // cannot move a parent in a child !
            return FALSE;

        pa = ((BrowserNode *) pa->parent());
    }
}

//

void BrowserNode::children(BrowserNodeList & nodes,
                           UmlCode kind1, UmlCode kind2) const
{
    Q3ListViewItem * child;

    for (child = firstChild(); child; child = child->nextSibling())
    {
        bool isAlive = !((BrowserNode *) child)->is_deleted;
        UmlCode type = ((BrowserNode *) child)->get_type();
        //QLOG_INFO() << stringify(type);
        bool isFirstKind = (type == kind1);
        bool isSecondKind = (type == kind2);
        if ( isAlive && ( isFirstKind || isSecondKind))
            nodes.append((BrowserNode *) child);
    }

}

QList<BrowserNode*> BrowserNode::children(QList<UmlCode> searchTypes, bool includeDeleted) const
{
    QList<BrowserNode*> items;
    Q3ListViewItem * child;

    for (child = firstChild(); child; child = child->nextSibling())
    {
        bool isAlive = !((BrowserNode *) child)->is_deleted;
        UmlCode type = ((BrowserNode *) child)->get_type();
        //QLOG_INFO() << stringify(type);
        bool validType = searchTypes.contains(type);
        if ( (isAlive || includeDeleted) && validType)
            items.append((BrowserNode *) child);
    }
    return items;
}

bool BrowserNode::enter_child_name(QString & r, const QString & msg, UmlCode type,
                                   bool allow_spaces, bool allow_empty)
{
    for (;;) {
        BooL ok = FALSE;
        r = MyInputDialog::getText("Uml", msg, r, ok);

        if (ok) {
            if (wrong_child_name(r, type, allow_spaces, allow_empty))
                msg_critical(TR("Error"), r + "\n\n" + TR("illegal name or already used"));
            else
                return TRUE;
        }
        else
            return FALSE;
    }
}

bool BrowserNode::enter_child_name(QString & r, const QString & msg, UmlCode type,
                                   BrowserNodeList & nodes,
                                   BrowserNode ** old, bool allow_spaces,
                                   bool allow_empty, bool existing)
{

    if (existing && nodes.isEmpty()) {
        msg_warning(TR("Error"), TR("nothing available"));
        return FALSE;
    }

    QStringList list;

    nodes.full_names(list);
    list.prepend(QString());

    *old = 0;

    for (;;) {
        BooL ok = FALSE;

        r = (list.count() == 1)
                ? MyInputDialog::getText("Uml", msg, QString(), ok)
            : MyInputDialog::getText("Uml", msg, list,  QString(), existing, ok);

        if (! ok)
            return FALSE;

        if (!r.isEmpty()) {
            int index = list.findIndex(r);

            if (index != -1) {
                *old = nodes.at(index - 1);
                return TRUE;
            }
        }

        if (wrong_child_name(r, type, allow_spaces, allow_empty))
            msg_critical(TR("Error"), r + "\n\n" + TR("illegal name or already used"));
        else
            return TRUE;
    }
}

bool BrowserNode::wrong_child_name( const QString & s, UmlCode type,
                                   bool allow_spaces, bool allow_empty) const
{
    if (s.isEmpty())
        return !allow_empty;

    const char * str = s;

    bool nonUnicodeName = str != fromUnicode(s);
    bool controllable = type >= UmlAssociation && type <= UmlClass;
    if (nonUnicodeName && controllable)
        return true;

    if (allow_empty)
        // synonymous allowed
        return FALSE;

    switch (type) {
    case UmlOperation:
        switch (*str) {
        case ' ' :
        case '\t':
        case '\r':
            return TRUE;

        default:
            // synonymous allowed
            return FALSE;
        }

    default:
        if (type <= UmlClass) {
            while (*str) {
                char c = *str++;

                if (((c >= 'a') && (c <= 'z')) ||
                    ((c >= 'A') && (c <= 'Z')) ||
                    ((c >= '0') && (c <= '9')) ||
                    (c == '_'))
                    ;
                else if ((strchr("()&^[]%|!+-*/=>", c) != 0) ||
                         (((c == ' ') || (c == '\t') || (c == '\r')) &&
                          !allow_spaces))
                    return TRUE;
                else if (c == '<') {
                    if (type == UmlClass)
                        // suppose it is a valid template
                        break;
                    else
                        return TRUE;
                }
            }
        }
    }

    // check unicity

    for (Q3ListViewItem * child = firstChild(); child; child = child->nextSibling())
    {
        if (!((BrowserNode *) child)->deletedp() &&
            ((BrowserNode *) child)->same_name(s, type))
            return TRUE;
    }

    return FALSE;
}

bool BrowserNode::allow_spaces() const
{
    return TRUE;
}

bool BrowserNode::allow_empty() const
{
    return FALSE;
}

bool BrowserNode::same_name(const QString & s, UmlCode type) const
{
    return ((get_type() == type) && (name == s));
}

void BrowserNode::select_in_browser()
{
    BrowserView::select(this);
}

// unicode
const QStringList & BrowserNode::default_stereotypes(UmlCode, const BrowserNode *) const
{
    static QStringList empty;

    return empty;
}

BrowserNode * BrowserNode::get_associated() const
{
    // cannot have associated diagram/view
    return (BrowserNode *) this;
}

void BrowserNode::package_modified()
{
    ((BrowserNode *) parent())->package_modified();
}

bool BrowserNode::api_compatible(unsigned) const
{
    return TRUE;
}

bool BrowserNode::tool_cmd(ToolCom * com, const char * args)
{
    switch ((unsigned char) args[-1]) {
    case applyCmd: {
        QLOG_FATAL() << Q_FUNC_INFO << "If this got called then we have a logic flaw going on and BrowserNode needs to have Q_OBJECT in it to properly catch ToolCom::Run execution result";
        Q_ASSERT_X(0, "applyCmd happened", "very bad");
        int runResult = ToolCom::run(args, this, FALSE, FALSE);
        com->write_unsigned(runResult);
        break;
    }

    case createCmd:
        // invalid creation
        com->write_id(0);
        break;

    case parentCmd:
        if (this != BrowserView::get_project())
            ((BrowserNode *) parent())->write_id(com);
        else
            com->write_id(0);

        break;

    case childrenCmd: {
        unsigned v = com->api_format(true);
        unsigned n = 0;
        Q3ListViewItem * child;

        for (child = firstChild(); child != 0; child = child->nextSibling())
            if (!((BrowserNode *) child)->deletedp() &&
                ((BrowserNode *) child)->api_compatible(v))
                n += 1;

        com->write_unsigned(n);

        for (child = firstChild(); child != 0; child = child->nextSibling())
            if (!((BrowserNode *) child)->deletedp() &&
                ((BrowserNode *) child)->api_compatible(v))
                ((BrowserNode *) child)->write_id(com);
    }
    break;

    case getDefCmd:
    case getUmlDefCmd:
    case getCppDefCmd:
    case getJavaDefCmd:
    case getPhpDefCmd:
    case getPythonDefCmd:
    case getIdlDefCmd:
        get_data()->send_uml_def(com, this, comment);
        break;

    case isWritableCmd:
        com->write_bool(!is_read_only);
        break;

    case supportFileCmd:
        // goes up to the package
        return ((BrowserNode *) parent())->tool_cmd(com, args);

    case isOpenCmd:
        com->write_bool(isOpen());
        break;

    case referencedByCmd: {
        BrowserNodeList targetof;

        referenced_by(targetof);
        // remove duplicats
        targetof.sort_it();

        BrowserNode *prevNode = 0;
        foreach (BrowserNode *bn, targetof) {
            if (bn == prevNode) {
                targetof.clear();
                break;
            }
            prevNode = bn;
        }

        com->write_unsigned(targetof.size());
        foreach (BrowserNode *bn, targetof)
            bn->write_id(com);
    }
    break;

    case setCoupleValueCmd:
        if (is_read_only && !root_permission())
            com->write_ack(FALSE);
        else {
            set_value(args, args + strlen(args) + 1);
            package_modified();
            get_data()->modified();
            com->write_ack(TRUE);
        }

        break;

    case setDescriptionCmd:
        if (is_read_only && !root_permission())
            com->write_ack(FALSE);
        else {
            set_comment(args);
            package_modified();
            com->write_ack(TRUE);
        }

        break;

    case setNameCmd:
        if (is_read_only && !root_permission())
            com->write_ack(FALSE);
        else {
            if (name != args) {
                if (((BrowserNode *) parent())->wrong_child_name(args, get_type(),
                        allow_spaces(),
                        allow_empty())) {
                    com->write_ack(FALSE);
                    return TRUE;
                }
                else {
                    set_name(args);
                    update_stereotype();
                    package_modified();
                    get_data()->modified();
                }
            }

            com->write_ack(TRUE);
        }

        break;

    case setOpenCmd:
        BrowserView::select(this);
        setOpen(*args);
        com->write_ack(TRUE);
        break;

    case setMarkedCmd:
        if (*args) {
            if (this == BrowserView::get_project())
                com->write_ack(FALSE);
            else {
                if (!is_marked)
                    toggle_mark();

                com->write_ack(TRUE);
            }
        }
        else {
            if (is_marked)
                toggle_mark();

            com->write_ack(TRUE);
        }

        break;

    case moveAfterCmd:
        if (is_read_only && !root_permission())
            com->write_ack(FALSE);
        else {
            BrowserNode * p = (BrowserNode *) parent();
            BrowserNode * after = (BrowserNode *) com->get_id(args);

            if (after == 0) {
                if (p == 0)
                    com->write_ack(FALSE);
                else {
                    p->takeItem(this);
                    p->insertItem(this);
                    com->write_ack(TRUE);
                    p->package_modified();
                }
            }
            else if ((after->parent() != p) ||
                     (after == this)) {
                com->write_ack(FALSE);
            }
            else {
                moveItem(after);
                com->write_ack(TRUE);
                p->package_modified();
            }
        }

        break;

    case moveInCmd:

        // plug-out upgrade, limited checks
        if (is_read_only && !root_permission())
            com->write_ack(FALSE);
        else {
            BrowserNode * newparent = (BrowserNode *) com->get_id(args);
            BrowserNode * oldparent = (BrowserNode *) parent();

            if ((newparent == oldparent) || (newparent == this)) {
                com->write_ack(FALSE);
            }
            else {
                oldparent->takeItem(this);
                newparent->insertItem(this);
                com->write_ack(TRUE);
                oldparent->package_modified();
                newparent->package_modified();
            }
        }

        break;

    case old_deleteCmd:
    case deleteCmd:
        if (is_read_only && !root_permission())
            com->write_ack(FALSE);
        else {
            delete_it();
            ((BrowserNode *) parent())->get_data()->modified();
            package_modified();
            com->write_ack(TRUE);
        }

        break;

    case applyStereotypeCmd:
        if (is_read_only && !root_permission())
            com->write_ack(FALSE);
        else {
            ProfiledStereotypes::applyStereotype(this); // call package_modified() if needed
            com->write_ack(TRUE);
        }

        break;

    default:
        return FALSE;
    }

    return TRUE;
}

void BrowserNode::member_cpp_def(const QString &, const QString &,
                                 QString & s, bool) const
{
    s += "MUST NOT BE CALLED\n";
}

void BrowserNode::write_id(ToolCom * com)
{
    com->write_id(this, get_type() - UmlRelations, name);
}

void BrowserNode::init_save_counter()
{
    if (! deletedp()) {
        must_be_saved_counter += 1;

        Q3ListViewItem * child = firstChild();

        while (child != 0) {
            ((BrowserNode *) child)->init_save_counter();
            child = child->nextSibling();
        }
    }
}

bool BrowserNode::save_open_list(QTextStream & st)
{
    if (!isOpen())
        return FALSE;

    bool have_open_child = FALSE;

    for (Q3ListViewItem * child = firstChild(); child != 0; child = child->nextSibling()) {
        BrowserNode * ch = ((BrowserNode *) child);

        if (!ch->deletedp())
            have_open_child |= ch->save_open_list(st);
    }

    if (! have_open_child) {
        QString warning;

        st << "  ";
        save(st, TRUE, warning);
        st << '\n';
    }

    return TRUE;
}

void BrowserNode::save_progress_closed()
{
    save_progress = 0;
}

void BrowserNode::save(QTextStream & st) const
{
    if (save_progress != 0)
        save_progress->setProgress(already_saved++);

    if (original_id != 0) {
        nl_indent(st);
        st << "oid " << original_id;
    }

    HaveKeyValueData::save(st);

    if (! comment.isEmpty()) {
        nl_indent(st);
        st << "comment ";
        save_string(comment, st);
    }
}

void BrowserNode::read(char *& st, char *& k, int id)
{
    if (in_lib_import()) {
        original_id = id;
        is_read_only = TRUE;

        if (! strcmp(k, "oid")) {
            // a sub lib is imported as a part of the imported lib
            (void) read_id(st);
            k = read_keyword(st);
        }
    }
    else if (! strcmp(k, "oid")) {
        original_id = read_id(st);
        is_read_only = TRUE;
        k = read_keyword(st);
    }

    HaveKeyValueData::read(st, k);

    if (!strcmp(k, "comment")) {
        comment = QString(QTextCodec::codecForName(codec())->fromUnicode(read_string(st)));
        k = read_keyword(st);
    }

    is_new = FALSE;
}

BrowserNode * BrowserNode::read_any_ref(char *& st, char * k)
{
    BrowserNode * r;

    if (((r = BrowserActivity::read_any_ref(st, k)) == 0) &&
        ((r = BrowserState::read_any_ref(st, k)) == 0) &&
        ((r = BrowserClass::read_any_ref(st, k)) == 0) &&
        ((r = BrowserDiagram::read_any_ref(st, k)) == 0) &&
        ((r = BrowserPackage::read(st, k, 0, FALSE)) == 0) &&
        ((r = BrowserUseCase::read(st, k, 0)) == 0) &&
        ((r = BrowserClassInstance::read(st, k, 0)) == 0) &&
        ((r = BrowserComponent::read(st, k, 0)) == 0) &&
        ((r = BrowserArtifact::read(st, k, 0)) == 0) &&
        ((r = BrowserDeploymentNode::read(st, k, 0)) == 0))
        r = BrowserSimpleRelation::read(st, k, 0);

    return r;
}

void BrowserNode::save_stereotypes(QTextStream & st,
                                   QStringList relations_stereotypes[])
{
    int r;

    for (r = 0; r != UmlRelations; r += 1) {
        QStringList & list = relations_stereotypes[r];

        if (! list.isEmpty()) {
            nl_indent(st);
            st << "  " << stringify((UmlCode) r);
            save_unicode_string_list(list, st);
        }
    }

    nl_indent(st);
    st << "end";
    nl_indent(st);
}

void BrowserNode::read_stereotypes(char *& st,
                                   QStringList relations_stereotypes[])
{
    int r;

    for (r = 0; r != UmlRelations; r += 1)
        relations_stereotypes[r].clear();

    char * k;

    while (strcmp((k = read_keyword(st)), "end")) {
        QStringList & list = relations_stereotypes[relation_type(k)];

        list.clear();
        read_unicode_string_list(list, st);
    }
}

void BrowserNode::renumber(int phase)
{
    for (Q3ListViewItem * child = firstChild();
         child != 0;
         child = child->nextSibling())
        ((BrowserNode *) child)->renumber(phase);
}

void BrowserNode::support_file(Q3Dict<char> & files, bool add) const
{
    for (Q3ListViewItem * child = firstChild();
         child != 0;
         child = child->nextSibling())
        ((BrowserNode *) child)->support_file(files, add);
}

// manage unconsistencies

static QString UnconsistencyDeletedMsg;
static QString UnconsistencyFixedMsg;
static QList<BrowserNode *> ModifiedPackages;

void BrowserNode::signal_unconsistencies()
{
    QString pfix =
        TR("<p><b>Warning, the model is not consistent because some elements have\n"
           "the same internal identifier.</b></p>\n"
           "<p>Users working on the same project have the same use identifier,\n"
           "or you had change the model files, or used Project synchro\n"
           "without following the mandatory rules</p>\n");
    QString msg;

    if (!UnconsistencyDeletedMsg.isEmpty())
        msg = pfix + "<p>" + TR("These elements was <b>removed</b>") + " :</p>\n <ul>"
              + UnconsistencyDeletedMsg + "</ul>\n";

    if (!UnconsistencyFixedMsg.isEmpty()) {
        if (UnconsistencyDeletedMsg.isEmpty())
            msg = pfix;

        msg += "<p>"
               + TR("The internal identifier of these elements was changed,\n"
                    "but <u>I can't garantee the references to them are the right one</u>,\n"
                    "check your model")
               + " :</p>\n<ul>" + UnconsistencyFixedMsg + "</ul>\n";

        do_change_shared_ids();

        foreach (BrowserNode *node, ModifiedPackages)
            node->is_modified = TRUE;
        ModifiedPackages.clear();
    }

    if (! msg.isEmpty()) {
        UnconsistencyDeletedMsg = UnconsistencyFixedMsg = QString();
        warn(msg);
    }
}

void BrowserNode::unconsistent_fixed(const char * what, BrowserNode * newone)
{
    UnconsistencyFixedMsg += QString("<li>") + what + QString(" <i>") +
                             quote(full_name()) + QString("</i> and <i>") +
                             quote(newone->full_name()) + QString("</i></li>\n");

    BrowserNode * bn = this;

    while (bn->get_type() != UmlPackage)
        bn = (BrowserNode *) bn->parent();

    if (!ModifiedPackages.contains(bn))
        ModifiedPackages.append(bn);
}

void BrowserNode::unconsistent_removed(const char * what, BrowserNode * newone)
{
    UnconsistencyDeletedMsg += QString("<li>") + what + QString(" <i>") +
                               quote(full_name()) + QString("</i> and <i>") +
                               quote(newone->full_name()) + QString("</i></li>\n");

    // deletion managed elsewhere
}

//

void BrowserNodeList::sort_it()
{
    if (count() > 1000)
        QApplication::setOverrideCursor(Qt::waitCursor);

    BrowserPackage::prepare_for_sort();

    qSort(begin(), end(), lessThan);

    SynonymousPath.setAutoDelete(TRUE);
    SynonymousPath.clear();

    if (count() > 1000)
        QApplication::restoreOverrideCursor();
}

void BrowserNodeList::sort()
{
    qSort(begin(), end(), lessThan);
}

bool BrowserNodeList::lessThan(BrowserNode *a, BrowserNode *b)
{
    QString s1 = ((BrowserNode *) a)->get_name();
    QString s2 = ((BrowserNode *) b)->get_name();

    if (s1 != s2) {
        s1 += " ";
        s2 += " ";
    }
    else {
        QString * ps1 = SynonymousPath[(BrowserNode *) a];
        QString * ps2 = SynonymousPath[(BrowserNode *) b];

        if (ps1 == 0) {
            s1 = ((BrowserNode *) a)->full_name(TRUE);
            ps1 = new QString(s1);

            SynonymousPath.insert((BrowserNode *) a, ps1);
        }
        else
            s1 = *ps1;

        if (ps2 == 0) {
            s2 = ((BrowserNode *) b)->full_name(TRUE);
            ps2 = new QString(s2);

            SynonymousPath.insert((BrowserNode *) b, ps2);
        }
        else
            s2 = *ps2;
    }

    return s1 < s2;
}

void BrowserNodeList::search(BrowserNode * bn, UmlCode k, const QString & s,
                             bool cs, bool even_deleted, bool for_name,
                             bool for_stereotype)
{
    Q3ListViewItem * child;

    for (child = bn->firstChild(); child != 0; child = child->nextSibling()) {
        if (even_deleted || !((BrowserNode *) child)->deletedp()) {
            BrowserNode * ch = (BrowserNode *) child;

            if (((k == UmlCodeSup) ||
                 ((k == UmlRelations)
                  ? IsaRelation(ch->get_type())
                  : (ch->get_type() == k))) &&
                (s.isEmpty() ||
                 (QString((for_name)
                          ? ch->get_name()
                          : ((for_stereotype) ? QString(ch->get_stereotype())
                             : QString(ch->get_comment())))
                  .find(s, 0, cs) != -1)))
                append((BrowserNode *) child);

            search((BrowserNode *) child, k, s, cs, even_deleted,
                   for_name, for_stereotype);
        }
    }
}

void BrowserNodeList::search_ddb(BrowserNode * bn, UmlCode k, const QString & s,
                                 bool cs, bool even_deleted)
{
    Q3ListViewItem * child;

    for (child = bn->firstChild(); child != 0; child = child->nextSibling()) {
        if (even_deleted || !((BrowserNode *) child)->deletedp()) {
            BrowserNode * ch = (BrowserNode *) child;

            if (((k == UmlCodeSup) ||
                 ((k == UmlRelations)
                  ? IsaRelation(ch->get_type())
                  : (ch->get_type() == k))) &&
                (s.isEmpty() ||
                 ch->get_data()->decldefbody_contain(s, cs, ch)))
                append(ch);

            search_ddb(ch, k, s, cs, even_deleted);
        }
    }
}

void BrowserNodeList::names(QStringList & list) const
{
    list.clear();

    foreach (BrowserNode *node, *this) {
        const char * s = node->get_name();
        list.append(((s == 0) || (*s == 0)) ? "<anonymous>" : s);
    }
}

void BrowserNodeList::full_names(QStringList & list) const
{
    list.clear();

    foreach (BrowserNode *node, *this)
        list.append(node->full_name(true));
}

void BrowserNodeList::full_defs(QStringList & list) const
{
    list.clear();

    foreach (BrowserNode *node, *this)
        list.append(node->get_data()->definition(true, false));
}
