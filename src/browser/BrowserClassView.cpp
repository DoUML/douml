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
#include <QPixmap>
#include <QDragMoveEvent>
#include <QDropEvent>

#include "BrowserClassView.h"
#include "ClassViewDialog.h"
#include "BrowserClassDiagram.h"
#include "BrowserColDiagram.h"
#include "BrowserSeqDiagram.h"
#include "BrowserObjectDiagram.h"
#include "BrowserDeploymentView.h"
#include "BrowserClass.h"
#include "BrowserClassInstance.h"
#include "BrowserState.h"
#include "BrowserActivity.h"
#include "BrowserActivityAction.h"
#include "SimpleData.h"
#include "ClassData.h"
#include "UmlPixmap.h"
#include "UmlDrag.h"
#include "SettingsDialog.h"
#include "myio.h"
#include "ToolCom.h"
#include "Tool.h"
#include "ui/menufactory.h"
#include "DialogUtil.h"
#include "mu.h"
#include "GenerationSettings.h"
#include "ProfiledStereotypes.h"
#include "translate.h"

IdDict<BrowserClassView> BrowserClassView::all(__FILE__);
QStringList BrowserClassView::its_default_stereotypes;	// unicode

BrowserClassView::BrowserClassView(QString s, BrowserNode * p, int id)
    : BrowserNode(s, p), Labeled<BrowserClassView>(all, id),
      associated_deployment_view(0)
{
    def = new SimpleData;
    def->set_browser_node(this);

    class_color = UmlDefaultColor;
    note_color = UmlDefaultColor;
    fragment_color = UmlDefaultColor;
    package_color = UmlDefaultColor;
    duration_color = UmlDefaultColor;
    continuation_color = UmlDefaultColor;
    state_color = UmlDefaultColor;
    stateaction_color = UmlDefaultColor;
    activity_color = UmlDefaultColor;
    activityregion_color = UmlDefaultColor;
    activitypartition_color = UmlDefaultColor;
    activityaction_color = UmlDefaultColor;
    parameterpin_color = UmlDefaultColor;
}

BrowserClassView::BrowserClassView(const BrowserClassView * model,
                                   BrowserNode * p)
    : BrowserNode(model->name, p), Labeled<BrowserClassView>(all, 0)
{
    def = new SimpleData(model->def);
    def->set_browser_node(this);
    comment = model->comment;
    classdiagram_settings = model->classdiagram_settings;
    sequencediagram_settings = model->sequencediagram_settings;
    collaborationdiagram_settings = model->collaborationdiagram_settings;
    objectdiagram_settings = model->objectdiagram_settings;
    statediagram_settings = model->statediagram_settings;
    activitydiagram_settings = model->activitydiagram_settings;
    class_settings = model->class_settings;
    class_color = model->class_color;
    note_color = model->note_color;
    fragment_color = model->fragment_color;
    package_color = model->package_color;
    duration_color = model->duration_color;
    continuation_color = model->continuation_color;
    state_color = model->state_color;
    stateaction_color = model->stateaction_color;
    activity_color = model->activity_color;
    activityregion_color = model->activityregion_color;
    activitypartition_color = model->activitypartition_color;
    activityaction_color = model->activityaction_color;
    parameterpin_color = model->parameterpin_color;
}

BrowserClassView::~BrowserClassView()
{
    all.remove(get_ident());
    delete def;
}

BrowserNode * BrowserClassView::duplicate(BrowserNode * p, QString name)
{
    BrowserClassView * result = new BrowserClassView(this, p);

    result->set_name((name.isEmpty()) ? get_name() : name);
    result->update_stereotype();

    return result;
}

BrowserClassView * BrowserClassView::add_class_view(BrowserNode * future_parent)
{
    QString name;

    if (future_parent->enter_child_name(name, TR("enter class view's name : "),
                                        UmlClassView, TRUE, FALSE))
        return new BrowserClassView(name, future_parent);
    else
        return 0;
}

void BrowserClassView::clear(bool old)
{
    all.clear(old);
    BrowserClass::clear(old);
    BrowserState::clear(old);
    BrowserActivity::clear(old);
    BrowserClassInstance::clear(old);
}

void BrowserClassView::update_idmax_for_root()
{
    all.update_idmax_for_root();
    BrowserClass::update_idmax_for_root();
    BrowserState::update_idmax_for_root();
    BrowserActivity::update_idmax_for_root();
    BrowserClassInstance::update_idmax_for_root();
}

void BrowserClassView::prepare_update_lib() const
{
    all.memo_id_oid(get_ident(), original_id);

    for (Q3ListViewItem * child = firstChild();
         child != 0;
         child = child->nextSibling())
        ((BrowserNode *) child)->prepare_update_lib();
}

void BrowserClassView::renumber(int phase)
{
    if (phase != -1)
        new_ident(phase, all);

    BrowserNode::renumber(phase);
}

const QPixmap * BrowserClassView::pixmap(int) const
{
    if (deletedp())
        return DeletedClassViewIcon;

    const QPixmap * px = ProfiledStereotypes::browserPixmap(def->get_stereotype());

    return (px != 0) ? px : ClassViewIcon;
}

QString BrowserClassView::full_name(bool rev, bool itself) const
{
    QString p = ((BrowserNode *) parent())->full_name(FALSE, FALSE);

    if (! itself)
        return p; // to goes up
    else if (p.isEmpty())
        return name;
    else if (rev)
        return name + (FullPathPrefix + p + FullPathPostfix);
    else
        return p + (FullPathDotDot + name);
}

void BrowserClassView::menu()
{
    Q3PopupMenu m(0);
    Q3PopupMenu subm(0);
    Q3PopupMenu roundtripm(0);
    Q3PopupMenu toolm(0);
    bool isprofile = (strcmp(((BrowserNode *) parent())->get_data()->get_stereotype(),
                             "profile")
                      == 0);

    MenuFactory::createTitle(m, def->definition(FALSE, TRUE));
    m.insertSeparator();

    if (!deletedp()) {
        if (!is_read_only && (edition_number == 0)) {
            if (isprofile)
                m.setWhatsThis(m.insertItem(TR("New stereotype"), 18),
                               TR("to add a <i>stereotype</i>"));

            m.setWhatsThis(m.insertItem(TR("New class diagram"), 0),
                           TR("to add a <i>class diagram</i>"));
            m.setWhatsThis(m.insertItem(TR("New sequence diagram"), 1),
                           TR("to add a <i>sequence diagram</i>"));
            m.setWhatsThis(m.insertItem(TR("New communication diagram"), 2),
                           TR("to add a <i>communication diagram</i>"));
            m.setWhatsThis(m.insertItem(TR("New object diagram"), 15),
                           TR("to add a <i>object diagram</i>"));
            m.setWhatsThis(m.insertItem(TR("New class"), 3),
                           TR("to add a <i>class</i>"));
            m.setWhatsThis(m.insertItem(TR("New class instance"), 17),
                           TR("to add a <i>class instance</i>"));
            m.setWhatsThis(m.insertItem(TR("New state machine"), 4),
                           TR("to add a <i>state machine</i>"));
            m.setWhatsThis(m.insertItem(TR("New activity"), 16),
                           TR("to add an <i>activity</i>"));
            m.insertSeparator();
        }

        if (!is_edited) {
            m.setWhatsThis(m.insertItem(TR("Edit"), 5),
                           TR("to edit the <i>class view</i>"));

            if (!is_read_only) {
                m.insertSeparator();
                m.setWhatsThis(m.insertItem(TR("Edit class settings"), 6),
                               TR("to set the sub classes's settings"));
                m.setWhatsThis(m.insertItem(TR("Edit drawing settings"), 7),
                               TR("to set how the sub <i>class diagrams</i>'s items must be drawn"));

                if (edition_number == 0) {
                    m.insertSeparator();
                    m.setWhatsThis(m.insertItem(TR("Delete"), 8),
                                   TR("to delete the <i>class view</i> and its sub items. \
Note that you can undelete them after"));
                }
            }
        }

        mark_menu(m, TR("the class view"), 90);
        ProfiledStereotypes::menu(m, this, 99990);

        if (! isprofile) {
            bool cpp = GenerationSettings::cpp_get_default_defs();
            bool java = GenerationSettings::java_get_default_defs();
            bool php = GenerationSettings::php_get_default_defs();
            bool python = GenerationSettings::python_get_default_defs();
            bool idl = GenerationSettings::idl_get_default_defs();

            if (cpp || java || php || python || idl) {
                m.insertSeparator();
                m.insertItem(TR("Generate"), &subm);

                if (cpp) {
                    subm.insertItem("C++", 11);

                    if ((edition_number == 0) && !is_read_only)
                        roundtripm.insertItem("C++", 31);
                }

                if (java) {
                    subm.insertItem("Java", 12);

                    if ((edition_number == 0) && !is_read_only)
                        roundtripm.insertItem("Java", 32);
                }

                if (php)
                    subm.insertItem("Php", 22);

                if (python)
                    subm.insertItem("Python", 25);

                if (idl)
                    subm.insertItem("Idl", 13);

                if (roundtripm.count() != 0)
                    m.insertItem(TR("Roundtrip"), &roundtripm);
            }
        }

        if ((edition_number == 0) &&
            (Tool::menu_insert(&toolm, get_type(), 100))) {
            m.insertSeparator();
            m.insertItem(TR("Tool"), &toolm);
        }
    }
    else if (!is_read_only && (edition_number == 0)) {
        m.setWhatsThis(m.insertItem(TR("Undelete"), 9),
                       TR("undelete the <i>class view</i>. \
Do not undelete its sub items"));
        m.setWhatsThis(m.insertItem(TR("Undelete recursively"), 10),
                       TR("undelete the <i>class view</i> and its sub items"));
    }

    if (!isprofile && (associated_deployment_view != 0)) {
        m.insertSeparator();
        m.setWhatsThis(m.insertItem(TR("Select associated deployment view"), 14),
                       TR("to select the associated <i>deployment view</i>"));
    }

    exec_menu_choice(m.exec(QCursor::pos()));
}

void BrowserClassView::exec_menu_choice(int rank)
{
    bool isprofile =
        (strcmp(((BrowserNode *) parent())->get_data()->get_stereotype(),
                "profile")
         == 0);

    switch (rank) {
    case 0: {
        BrowserClassDiagram * d =
            BrowserClassDiagram::add_class_diagram(this);

        if (d == 0)
            return;

        d->select_in_browser();
    }
    break;

    case 1: {
        BrowserSeqDiagram * d =
            BrowserSeqDiagram::add_sequence_diagram(this);

        if (d == 0)
            return;

        d->select_in_browser();
    }
    break;

    case 2: {
        BrowserColDiagram * d =
            BrowserColDiagram::add_collaboration_diagram(this);

        if (d == 0)
            return;

        d->select_in_browser();
    }
    break;

    case 3: {
        BrowserClass * cl = BrowserClass::add_class(FALSE, this);

        if (cl == 0)
            return;

        cl->select_in_browser();
    }
    break;

    case 4: {
        BrowserState * st = BrowserState::add_state(this, (bool) TRUE);

        if (st == 0)
            return;

        st->select_in_browser();
    }
    break;

    case 5:
        (new ClassViewDialog(get_data()))->show();
        return;

    case 6:
        if (! class_settings.edit(UmlClassView))
            return;

        break;

    case 7:
        for (;;) {
            StateSpecVector st;
            ColorSpecVector co(13);

            classdiagram_settings.complete(st, UmlClassView);
            sequencediagram_settings.complete(st, FALSE);
            collaborationdiagram_settings.complete(st, FALSE);
            objectdiagram_settings.complete(st, FALSE);
            statediagram_settings.complete(st, FALSE);
            activitydiagram_settings.complete(st, FALSE);

            co[0].set(TR("class color"), &class_color);
            co[1].set(TR("note color"), &note_color);
            co[2].set(TR("package color"), &package_color);
            co[3].set(TR("fragment color"), &fragment_color);
            co[4].set(TR("duration color"), &duration_color);
            co[5].set(TR("continuation color"), &continuation_color);
            co[6].set(TR("state color"), &state_color);
            co[7].set(TR("state action color"), &stateaction_color);
            co[8].set(TR("activity color"), &activity_color);
            co[9].set(TR("activity region color"), &activityregion_color);
            co[10].set(TR("activity partition color"), &activitypartition_color);
            co[11].set(TR("activity action color"), &activityaction_color);
            co[12].set(TR("parameter and pin color"), &parameterpin_color);

            SettingsDialog dialog(&st, &co, FALSE);

            dialog.raise();

            if (dialog.exec() != QDialog::Accepted)
                return;

            DrawingSettings::modified();

            if (!dialog.redo())
                break;
            else
                package_modified();
        }

        break;

    case 8:
        delete_it();

        if (ProfiledStereotypes::hasStereotype())
            ProfiledStereotypes::recompute(FALSE);

        break;

    case 9:
        undelete(FALSE);
        ProfiledStereotypes::recompute(FALSE);
        break;

    case 10:
        undelete(TRUE);
        ProfiledStereotypes::recompute(FALSE);
        break;

    case 11:
        if (! isprofile) {
            bool preserve = preserve_bodies();

            ToolCom::run((verbose_generation())
                         ? ((preserve) ? "cpp_generator -v -p" : "cpp_generator -v")
                             : ((preserve) ? "cpp_generator -p" : "cpp_generator"),
                             this);
        }

        return;

    case 12:
        if (! isprofile) {
            bool preserve = preserve_bodies();

            ToolCom::run((verbose_generation())
                         ? ((preserve) ? "java_generator -v -p" : "java_generator -v")
                             : ((preserve) ? "java_generator -p" : "java_generator"),
                             this);
        }

        return;

    case 22:
        if (! isprofile) {
            bool preserve = preserve_bodies();

            ToolCom::run((verbose_generation())
                         ? ((preserve) ? "php_generator -v -p" : "php_generator -v")
                             : ((preserve) ? "php_generator -p" : "php_generator"),
                             this);
        }

        return;

    case 25:
        if (! isprofile) {
            bool preserve = preserve_bodies();

            ToolCom::run((verbose_generation())
                         ? ((preserve) ? "python_generator -v -p" : "python_generator -v")
                             : ((preserve) ? "python_generator -p" : "python_generator"),
                             this);
        }

        return;

    case 13:
        if (! isprofile)
            ToolCom::run((verbose_generation()) ? "idl_generator -v" : "idl_generator", this);

        return;

    case 31:
        if (! isprofile)
            ToolCom::run("cpp_roundtrip", this);

        return;

    case 32:
        if (! isprofile)
            ToolCom::run("java_roundtrip", this);

        return;

    case 14:
        if (! isprofile)
            associated_deployment_view->select_in_browser();

        return;

    case 15: {
        BrowserObjectDiagram * d =
            BrowserObjectDiagram::add_object_diagram(this);

        if (d == 0)
            return;

        d->select_in_browser();
    }
    break;

    case 16: {
        BrowserActivity * a = BrowserActivity::add_activity(this);

        if (a != 0)
            a->select_in_browser();
    }

    return; // package_modified called

    case 17: {
        BrowserClassInstance * c =
            BrowserClassInstance::add_classinstance(this);

        if (c != 0)
            c->select_in_browser();
    }

    return; // package_modified called

    case 18: {
        BrowserClass * cl = BrowserClass::add_class(TRUE, this);

        if (cl == 0)
            return;

        cl->select_in_browser();
    }
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

void BrowserClassView::apply_shortcut(QString s)
{
    int choice = -1;

    if (!deletedp()) {
        if (!is_read_only && (edition_number == 0)) {
            if (s == "New class diagram")
                choice = 0;
            else if (s == "New sequence diagram")
                choice = 1;
            else if (s == "New communication diagram")
                choice = 2;
            else if (s == "New object diagram")
                choice = 15;
            else if (s == "New class")
                choice = 3;
            else if (s == "New class instance")
                choice = 17;
            else if (s == "New state machine")
                choice = 4;
            else if (s == "New activity")
                choice = 16;
        }

        if (!is_edited) {
            if (s == "Edit")
                choice = 5;

            if (!is_read_only) {
                if (s == "Edit class settings")
                    choice = 6;
                else if (s == "Edit drawing settings")
                    choice = 7;

                if (edition_number == 0) {
                    if (s == "Delete")
                        choice = 8;
                }
            }
        }

        mark_shortcut(s, choice, 90);

        if (edition_number == 0)
            Tool::shortcut(s, choice, get_type(), 100);

        if (s == "Generate C++")
            choice = 11;
        else if (s == "Generate Java")
            choice = 12;
        else if (s == "Generate Php")
            choice = 22;
        else if (s == "Generate Python")
            choice = 25;
        else if (s == "Generate Idl")
            choice = 13;
    }
    else if (!is_read_only && (edition_number == 0)) {
        if (s == "Undelete")
            choice = 9;
        else if (s == "Undelete recursively")
            choice = 10;
    }

    if (associated_deployment_view != 0) {
        if (s == "Select associated deployment view")
            choice = 14;
    }

    exec_menu_choice(choice);
}

void BrowserClassView::open(bool)
{
    if (!is_edited)
        (new ClassViewDialog(get_data()))->show();
}

UmlCode BrowserClassView::get_type() const
{
    return UmlClassView;
}

QString BrowserClassView::get_stype() const
{
    return TR("class view");
}

int BrowserClassView::get_identifier() const
{
    return get_ident();
}

const char * BrowserClassView::help_topic() const
{
    return "classview";
}

BasicData * BrowserClassView::get_data() const
{
    return def;
}


BrowserNode * BrowserClassView::get_associated() const
{
    return associated_deployment_view;
}

void BrowserClassView::set_associated_deploymentview(BrowserDeploymentView * cv,
        bool on_read)
{
    if (associated_deployment_view != cv) {
        if (associated_deployment_view != 0)
            QObject::disconnect(associated_deployment_view->get_data(), SIGNAL(deleted()),
                                def, SLOT(on_delete()));

        if ((associated_deployment_view = cv) != 0)
            QObject::connect(associated_deployment_view->get_data(), SIGNAL(deleted()),
                             def, SLOT(on_delete()));

        if (! on_read)
            package_modified();
    }
}

void BrowserClassView::on_delete()
{
    if (associated_deployment_view &&
        associated_deployment_view->deletedp())
        associated_deployment_view = 0;
}


void BrowserClassView::get_classdiagramsettings(ClassDiagramSettings & r) const
{
    if (! classdiagram_settings.complete(r))
        ((BrowserNode *) parent())->get_classdiagramsettings(r);
}

void BrowserClassView::get_sequencediagramsettings(SequenceDiagramSettings & r) const
{
    if (! sequencediagram_settings.complete(r))
        ((BrowserNode *) parent())->get_sequencediagramsettings(r);
}

void BrowserClassView::get_collaborationdiagramsettings(CollaborationDiagramSettings & r) const
{
    if (! collaborationdiagram_settings.complete(r))
        ((BrowserNode *) parent())->get_collaborationdiagramsettings(r);
}

void BrowserClassView::get_objectdiagramsettings(ObjectDiagramSettings & r) const
{
    if (! objectdiagram_settings.complete(r))
        ((BrowserNode *) parent())->get_objectdiagramsettings(r);
}

void BrowserClassView::get_statediagramsettings(StateDiagramSettings & r) const
{
    if (! statediagram_settings.complete(r))
        ((BrowserNode *) parent())->get_statediagramsettings(r);
}

void BrowserClassView::get_activitydiagramsettings(ActivityDiagramSettings & r) const
{
    if (! activitydiagram_settings.complete(r))
        ((BrowserNode *) parent())->get_activitydiagramsettings(r);
}

UmlColor BrowserClassView::get_color(UmlCode who) const
{
    UmlColor c;

    switch (who) {
    case UmlClass:
        c = class_color;
        break;

    case UmlNote:
        c = note_color;
        break;

    case UmlFragment:
        c = fragment_color;
        break;

    case UmlContinuation:
        c = continuation_color;
        break;

    case UmlActivityDuration:
        c = duration_color;
        break;

    case UmlState:
        c = state_color;
        break;

    case UmlStateAction:
        c = stateaction_color;
        break;

    case UmlActivity:
        c = activity_color;
        break;

    case UmlInterruptibleActivityRegion:
    case UmlExpansionRegion:
        c = activityregion_color;
        break;

    case UmlActivityPartition:
        c = activitypartition_color;
        break;

    case UmlActivityAction:
        c = activityaction_color;
        break;

    case UmlExpansionNode:
    case UmlParameter:
    case UmlActivityPin:
        c = parameterpin_color;
        break;

    default: // UmlPackage
        c = package_color;
        break;
    }

    return (c != UmlDefaultColor)
           ? c
           : ((BrowserNode *) parent())->get_color(who);
}

UmlVisibility BrowserClassView::get_visibility(UmlCode who) const
{
    UmlVisibility v;

    switch (who) {
    case UmlAttribute:
        v = class_settings.attribute_visibility;
        break;

    case UmlRelations:
        v = class_settings.relation_visibility;
        break;

    default:	// UmlOperation
        v = class_settings.operation_visibility;
        break;
    }

    return (v != UmlDefaultVisibility)
           ? v
           : ((BrowserNode *) parent())->get_visibility(who);
}

bool BrowserClassView::tool_cmd(ToolCom * com, const char * args)
{
    switch ((unsigned char) args[-1]) {
    case createCmd: {
        bool ok = TRUE;

        if (is_read_only && !root_permission())
            ok = FALSE;
        else {
            switch (com->get_kind(args)) {
            case UmlClassDiagram:
                if (wrong_child_name(args, UmlClassDiagram, TRUE, FALSE))
                    ok = FALSE;
                else
                    (new BrowserClassDiagram(args, this))->write_id(com);

                break;

            case UmlSeqDiagram:
                if (wrong_child_name(args, UmlSeqDiagram, TRUE, FALSE))
                    ok = FALSE;
                else
                    (new BrowserSeqDiagram(args, this))->write_id(com);

                break;

            case UmlColDiagram:
                if (wrong_child_name(args, UmlColDiagram, TRUE, FALSE))
                    ok = FALSE;
                else
                    (new BrowserColDiagram(args, this))->write_id(com);

                break;

            case UmlObjectDiagram:
                if (wrong_child_name(args, UmlObjectDiagram, TRUE, FALSE))
                    ok = FALSE;
                else
                    (new BrowserObjectDiagram(args, this))->write_id(com);

                break;

            case UmlClass:
                if (wrong_child_name(args, UmlClass, FALSE, FALSE))
                    ok = FALSE;
                else
                    (BrowserClass::add_class(FALSE, this, args))->write_id(com);

                break;

            case UmlClassInstance:
                BrowserClassInstance::add_from_tool(this, com, args);
                break;

            case UmlState:
                if (wrong_child_name(args, UmlState, TRUE, FALSE))
                    ok = FALSE;
                else
                    // force QString(args) else call add_state(BrowserNode, bool) !
                    (BrowserState::add_state(this, QString(args)))->write_id(com);

                break;

            case UmlActivity:
                if (wrong_child_name(args, UmlActivity, TRUE, FALSE))
                    ok = FALSE;
                else
                    (BrowserActivity::add_activity(this, args))->write_id(com);

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

    case setAssocViewCmd:
        if (is_read_only && !root_permission())
            com->write_ack(FALSE);
        else {
            set_associated_deploymentview((BrowserDeploymentView *) com->get_id(args));
            com->write_ack(TRUE);
        }

        return TRUE;

    default:
        return (def->tool_cmd(com, args, this, comment) ||
                BrowserNode::tool_cmd(com, args));
    }
}

void BrowserClassView::DragMoveEvent(QDragMoveEvent * e)
{
    if (UmlDrag::canDecode(e, UmlClass) ||
        UmlDrag::canDecode(e, UmlClassInstance) ||
        UmlDrag::canDecode(e, UmlClassDiagram) ||
        UmlDrag::canDecode(e, UmlSeqDiagram) ||
        UmlDrag::canDecode(e, UmlColDiagram) ||
        UmlDrag::canDecode(e, UmlObjectDiagram) ||
        UmlDrag::canDecode(e, UmlState) ||
        UmlDrag::canDecode(e, UmlActivity)) {
        if (!is_read_only)
            e->accept();
        else
            e->ignore();
    }
    else
        ((BrowserNode *) parent())->DragMoveInsideEvent(e);
}

void BrowserClassView::DragMoveInsideEvent(QDragMoveEvent * e)
{
    if (!is_read_only &&
        (UmlDrag::canDecode(e, UmlClass) ||
         UmlDrag::canDecode(e, UmlClassInstance) ||
         UmlDrag::canDecode(e, UmlClassDiagram) ||
         UmlDrag::canDecode(e, UmlSeqDiagram) ||
         UmlDrag::canDecode(e, UmlColDiagram) ||
         UmlDrag::canDecode(e, UmlObjectDiagram) ||
         UmlDrag::canDecode(e, UmlState) ||
         UmlDrag::canDecode(e, UmlActivity)))
        e->accept();
    else
        e->ignore();
}

bool BrowserClassView::may_contains_them(const QList<BrowserNode *> & l,
        BooL & duplicable) const
{
    foreach (BrowserNode *current, l) {
        switch (current->get_type()) {
        case UmlClass:
        case UmlClassInstance:
        case UmlClassDiagram:
        case UmlSeqDiagram:
        case UmlColDiagram:
        case UmlObjectDiagram:
        case UmlState:
        case UmlActivity:
            if (! may_contains(current, FALSE))
                return FALSE;

            break;

        default:
            return FALSE;
        }

        duplicable = may_contains_it(current);
    }

    return TRUE;
}

BrowserNode * BrowserClassView::container(UmlCode) const
{
    // currently only for class, state machine and activity
    return (BrowserNode *) this;
}

void BrowserClassView::DropEvent(QDropEvent * e)
{
    DropAfterEvent(e, 0);
}

void BrowserClassView::DropAfterEvent(QDropEvent * e, BrowserNode * after)
{
    BrowserNode * bn;

    if ((((bn = UmlDrag::decode(e, UmlClass)) != 0) ||
         ((bn = UmlDrag::decode(e, UmlClassInstance)) != 0) ||
         ((bn = UmlDrag::decode(e, UmlClassDiagram)) != 0) ||
         ((bn = UmlDrag::decode(e, UmlSeqDiagram)) != 0) ||
         ((bn = UmlDrag::decode(e, UmlColDiagram)) != 0) ||
         ((bn = UmlDrag::decode(e, UmlObjectDiagram)) != 0) ||
         ((bn = UmlDrag::decode(e, UmlState)) != 0) ||
         ((bn = UmlDrag::decode(e, UmlActivity)) != 0)) &&
        (bn != after) && (bn != this)) {
        if (may_contains(bn, FALSE)) {
            BrowserNode * old_parent = (BrowserNode *) bn->parent();

            if ((old_parent->parent() != parent()) &&
                (bn->get_type() == UmlClass) &&
                !strcmp(bn->get_data()->get_stereotype(), "stereotype") &&
                (((BrowserNode *) old_parent->parent())->get_type() == UmlPackage) &&
                !strcmp(((BrowserNode *) old_parent->parent())->get_data()->get_stereotype(), "profile"))
                ProfiledStereotypes::deleted((BrowserClass *) bn);

            if (after)
                bn->moveItem(after);
            else {
                bn->parent()->takeItem(bn);
                insertItem(bn);
            }

            if (old_parent != this) {
                if (!strcmp(bn->get_data()->get_stereotype(), "stereotype") &&
                    !strcmp(((BrowserNode *) parent())->get_data()->get_stereotype(), "profile"))
                    ProfiledStereotypes::added((BrowserClass *) bn);

                old_parent->package_modified();
                bn->modified();
            }

            package_modified();
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

bool BrowserClassView::extract_from_profile()
{
    Q3ListViewItem * bn = firstChild();
    bool has_stereotypes = FALSE;

    while (bn != 0) {
        if (!((BrowserNode *) bn)->deletedp() &&
            (((BrowserNode *) bn)->get_type() == UmlClass) &&
            !strcmp(((BrowserNode *) bn)->get_data()->get_stereotype(), "stereotype")) {
            has_stereotypes = TRUE;
            ProfiledStereotypes::deleted((BrowserClass *) bn);
        }

        bn = bn->nextSibling();
    }

    return has_stereotypes;
}

void BrowserClassView::insert_in_profile()
{
    Q3ListViewItem * bn = firstChild();

    while (bn != 0) {
        if (!((BrowserNode *) bn)->deletedp() &&
            (((BrowserNode *) bn)->get_type() == UmlClass) &&
            !strcmp(((BrowserNode *) bn)->get_data()->get_stereotype(), "stereotype"))
            ProfiledStereotypes::added((BrowserClass *) bn);

        bn = bn->nextSibling();
    }
}

// unicode
const QStringList & BrowserClassView::default_stereotypes()
{
    return its_default_stereotypes;
}

void BrowserClassView::save_stereotypes(QTextStream & st)
{
    nl_indent(st);
    st << "classview_stereotypes ";
    save_unicode_string_list(its_default_stereotypes, st);
}

void BrowserClassView::read_stereotypes(char *& st, char *& k)
{
    if (!strcmp(k, "classview_stereotypes")) {
        read_unicode_string_list(its_default_stereotypes, st);
        k = read_keyword(st);
    }
}

void BrowserClassView::save(QTextStream & st, bool ref, QString & warning)
{
    if (ref)
        st << "classview_ref " << get_ident() << " // " << get_name();
    else {
        nl_indent(st);
        st << "classview " << get_ident() << " ";
        save_string(name, st);
        indent(+1);

        def->save(st, warning);
        nl_indent(st);
        st << "//class diagram settings";
        classdiagram_settings.save(st);
        nl_indent(st);
        st << "//collaboration diagram settings";
        collaborationdiagram_settings.save(st);
        nl_indent(st);
        st << "//object diagram settings";
        objectdiagram_settings.save(st);
        nl_indent(st);
        st << "//sequence diagram settings";
        sequencediagram_settings.save(st);
        nl_indent(st);
        st << "//state diagram settings";
        statediagram_settings.save(st);
        nl_indent(st);
        st << "//class settings";
        class_settings.save(st);
        nl_indent(st);
        st << "//activity diagram settings";
        activitydiagram_settings.save(st);

        BooL nl = FALSE;

        save_color(st, "class_color", class_color, nl);
        save_color(st, "note_color", note_color, nl);
        save_color(st, "package_color", package_color, nl);
        save_color(st, "fragment_color", fragment_color, nl);
        save_color(st, "duration_color", duration_color, nl);
        save_color(st, "continuation_color", continuation_color, nl);
        save_color(st, "state_color", state_color, nl);
        save_color(st, "stateaction_color", stateaction_color, nl);
        save_color(st, "activity_color", activity_color, nl);
        save_color(st, "activityregion_color", activityregion_color, nl);
        save_color(st, "activitypartition_color", activitypartition_color, nl);
        save_color(st, "activityaction_color", activityaction_color, nl);
        save_color(st, "parameterpin_color", parameterpin_color, nl);

        if (associated_deployment_view != 0) {
            nl_indent(st);
            st << "associated_deployment_view ";
            associated_deployment_view->save(st, TRUE, warning);
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
                else {
                    // set 'not modified' to delete the associated file on exit
                    ((BrowserNode *) child)->unmodified();

                    if ((child = child->nextSibling()) == 0)
                        break;
                }
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

BrowserClassView * BrowserClassView::read(char *& st, char * k,
        BrowserNode * parent,
        bool /*recursive*/)
{
    if (!strcmp(k, "classview")) {
        int id = read_id(st);
        BrowserClassView * already_exist = all[id];
        BrowserClassView * r = new BrowserClassView(read_string(st), parent, id);

        if (already_exist != 0) {
            already_exist->must_change_id(all);
            already_exist->unconsistent_fixed("class views", r);
        }

        r->is_read_only = (!in_import() && read_only_file()) ||
                          ((user_id() != 0) && r->is_api_base());

        k = read_keyword(st);

        r->def->read(st, k);					// updates k
        r->classdiagram_settings.read(st, k);			// updates k

        if (read_file_format() >= 6) {
            r->collaborationdiagram_settings.read(st, k);		// updates k

            if (read_file_format() >= 25)
                r->objectdiagram_settings.read(st, k);		// updates k

            r->sequencediagram_settings.read(st, k);		// updates k
        }

        if (read_file_format() >= 21)
            r->statediagram_settings.read(st, k);			// updates k

        r->class_settings.read(st, k);				// updates k

        if (read_file_format() >= 26)
            r->activitydiagram_settings.read(st, k);		// updates k

        read_color(st, "class_color", r->class_color, k);		// updates k
        read_color(st, "note_color", r->note_color, k);		// updates k
        read_color(st, "package_color", r->package_color, k);	// updates k
        read_color(st, "fragment_color", r->fragment_color, k);	// updates k
        read_color(st, "duration", r->duration_color, k);	// old, updates k
        read_color(st, "duration_color", r->duration_color, k);	// updates k
        read_color(st, "continuation_color", r->continuation_color, k);	// updates k
        read_color(st, "state_color", r->state_color, k);		// updates k
        read_color(st, "stateaction_color", r->stateaction_color, k);		// updates k
        read_color(st, "activity_color", r->activity_color, k);		// updates k
        read_color(st, "activityregion_color", r->activityregion_color, k);		// updates k
        read_color(st, "activitypartition_color", r->activitypartition_color, k);		// updates k
        read_color(st, "activityaction_color", r->activityaction_color, k);		// updates k
        read_color(st, "parameterpin_color", r->parameterpin_color, k);		// updates k

        if ((read_file_format() < 20) && !strcmp(k, "associated_componentview")) {
            // old format
            r->set_associated_deploymentview(BrowserDeploymentView::read_ref(st, "deploymentview_ref"),
                                             TRUE);
            k = read_keyword(st);
        }
        else if (!strcmp(k, "associated_deployment_view") ||
                 ((read_file_format() < 20) && !strcmp(k, "associated_component_view"))) {
            r->set_associated_deploymentview(BrowserDeploymentView::read_ref(st, read_keyword(st)),
                                             TRUE);
            k = read_keyword(st);
        }

        r->BrowserNode::read(st, k, id);				// updates k

        if (strcmp(k, "end")) {
            while (BrowserClassDiagram::read(st, k, r) ||
                   BrowserColDiagram::read(st, k, r) ||
                   BrowserSeqDiagram::read(st, k, r) ||
                   BrowserObjectDiagram::read(st, k, r) ||
                   BrowserClass::read(st, k, r) ||
                   BrowserClassInstance::read(st, k, r) ||
                   BrowserState::read(st, k, r) ||
                   BrowserActivity::read(st, k, r))
                k = read_keyword(st);

            if (strcmp(k, "end"))
                wrong_keyword(k, "end");
        }

        return r;
    }
    else
        return 0;
}

BrowserNode * BrowserClassView::get_it(const char * k, int id)
{
    if (!strcmp(k, "classview_ref"))
        return all[id];

    BrowserNode * r;

    if (((r = BrowserClassDiagram::get_it(k, id)) == 0) &&
        ((r = BrowserColDiagram::get_it(k, id)) == 0) &&
        ((r = BrowserSeqDiagram::get_it(k, id)) == 0) &&
        ((r = BrowserObjectDiagram::get_it(k, id)) == 0) &&
        ((r = BrowserClass::get_it(k, id)) == 0) &&
        ((r = BrowserState::get_it(k, id)) == 0))
        r = BrowserActivity::get_it(k, id);

    return r;
}

