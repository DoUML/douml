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
#include <qfile.h>
#include <qcursor.h>
#include <qapplication.h>
#include <q3filedialog.h>
//Added by qt3to4:
#include <QPixmap>
#include <QTextStream>
#include <QDropEvent>
#include <QDragMoveEvent>

#include "BrowserPackage.h"
#include "PackageData.h"
#include "BrowserClassView.h"
#include "BrowserUseCaseView.h"
#include "BrowserClassDiagram.h"
#include "BrowserActivityDiagram.h"
#include "BrowserColDiagram.h"
#include "BrowserComponentDiagram.h"
#include "BrowserDeploymentDiagram.h"
#include "BrowserObjectDiagram.h"
#include "BrowserSeqDiagram.h"
#include "BrowserStateDiagram.h"
#include "BrowserUseCaseDiagram.h"
#include "BrowserClass.h"
#include "BrowserUseCase.h"
#include "BrowserView.h"
#include "BrowserAttribute.h"
#include "BrowserOperation.h"
#include "BrowserComponent.h"
#include "BrowserArtifact.h"
#include "BrowserDeploymentNode.h"
#include "BrowserComponentView.h"
#include "BrowserDeploymentView.h"
#include "BrowserSimpleRelation.h"
#include "BrowserState.h"
#include "BrowserPseudoState.h"
#include "BrowserStateAction.h"
#include "BrowserActivity.h"
#include "BrowserFlow.h"
#include "BrowserInterruptibleActivityRegion.h"
#include "BrowserExpansionRegion.h"
#include "BrowserActivityPartition.h"
#include "BrowserParameter.h"
#include "BrowserParameterSet.h"
#include "BrowserActivityNode.h"
#include "BrowserActivityAction.h"
#include "BrowserActivityObject.h"
#include "BrowserPin.h"
#include "SimpleRelationData.h"
#include "UmlPixmap.h"
#include "UmlDrag.h"
#include "SettingsDialog.h"
#include "RevSettingsDialog.h"
#include "StereotypesDialog.h"
#include "GenerationSettings.h"
#include "myio.h"
#include "ToolCom.h"
#include "Tool.h"
#include "ui/menufactory.h"
#include "UmlWindow.h"
#include "ReferenceDialog.h"
#include "UmlGlobal.h"
#include "mu.h"
#include "SaveProgress.h"
#include "DialogUtil.h"
#include "err.h"
#include "strutil.h"
#include "ProfiledStereotypes.h"
#include "translate.h"

IdDict<BrowserPackage> BrowserPackage::all(__FILE__);
Q3PtrList<BrowserPackage> BrowserPackage::removed;

QStringList BrowserPackage::its_default_stereotypes;	// unicode
QStringList BrowserPackage::relation_default_stereotypes;	// unicode

BrowserPackage::BrowserPackage(QString s, BrowserView * parent, int id)
    : BrowserNode(s, parent), Labeled<BrowserPackage>(all, id),
      def(new PackageData), associated_diagram(0), is_imported(FALSE)
{
    // creates the project package
    is_modified = (id == 0);
    def->set_browser_node(this);
    revision = 1;
    owner = -1;

    classdiagram_settings.hide_attributes = UmlNo;
    classdiagram_settings.hide_operations = UmlNo;
    classdiagram_settings.hide_getset_operations = UmlNo;
    classdiagram_settings.show_full_members_definition = UmlNo;
    classdiagram_settings.show_members_visibility = UmlNo;
    classdiagram_settings.show_members_stereotype = UmlNo;
    classdiagram_settings.show_members_context = noContext;
    classdiagram_settings.show_members_multiplicity = UmlNo;
    classdiagram_settings.show_members_initialization = UmlNo;
    classdiagram_settings.show_attribute_modifiers = UmlNo;
    classdiagram_settings.member_max_width = UmlUnlimitedMemberWidth;
    classdiagram_settings.show_parameter_dir = UmlYes;
    classdiagram_settings.show_parameter_name = UmlYes;
    classdiagram_settings.package_name_in_tab = UmlNo;
    classdiagram_settings.class_drawing_mode = Natural;
    classdiagram_settings.drawing_language = UmlView;
    classdiagram_settings.show_context_mode = noContext;
    classdiagram_settings.auto_label_position = UmlYes;
    classdiagram_settings.draw_all_relations = UmlYes;
    classdiagram_settings.show_relation_modifiers = UmlNo;
    classdiagram_settings.show_relation_visibility = UmlNo;
    classdiagram_settings.show_infonote = UmlNo;
    classdiagram_settings.shadow = UmlYes;
    classdiagram_settings.show_stereotype_properties = UmlNo;

    class_settings.attribute_visibility = UmlPrivate;
    class_settings.relation_visibility = UmlPrivate;
    class_settings.operation_visibility = UmlPublic;

    usecasediagram_settings.package_name_in_tab = UmlNo;
    usecasediagram_settings.show_context_mode = noContext;
    usecasediagram_settings.auto_label_position = UmlYes;
    usecasediagram_settings.draw_all_relations = UmlYes;
    usecasediagram_settings.class_drawing_mode = asActor;
    usecasediagram_settings.shadow = UmlYes;
    usecasediagram_settings.show_stereotype_properties = UmlNo;

    sequencediagram_settings.show_full_operations_definition = UmlNo;
    sequencediagram_settings.write_horizontally = UmlYes;
    sequencediagram_settings.instances_drawing_mode = Natural;
    sequencediagram_settings.drawing_language = UmlView;
    sequencediagram_settings.draw_all_relations = UmlYes;
    sequencediagram_settings.shadow = UmlYes;
    sequencediagram_settings.show_stereotype_properties = UmlNo;
    sequencediagram_settings.show_class_context_mode = noContext;
    sequencediagram_settings.show_msg_context_mode = noContext;

    collaborationdiagram_settings.show_full_operations_definition = UmlNo;
    collaborationdiagram_settings.show_hierarchical_rank = UmlNo;
    collaborationdiagram_settings.write_horizontally = UmlYes;
    collaborationdiagram_settings.drawing_language = UmlView;
    collaborationdiagram_settings.package_name_in_tab = UmlNo;
    collaborationdiagram_settings.show_class_pack_context_mode = noContext;
    collaborationdiagram_settings.show_msg_context_mode = noContext;
    collaborationdiagram_settings.draw_all_relations = UmlYes;
    collaborationdiagram_settings.shadow = UmlYes;
    collaborationdiagram_settings.show_stereotype_properties = UmlNo;

    objectdiagram_settings.write_horizontally = UmlYes;
    objectdiagram_settings.package_name_in_tab = UmlNo;
    objectdiagram_settings.show_class_pack_context_mode = noContext;
    objectdiagram_settings.auto_label_position = UmlYes;
    objectdiagram_settings.draw_all_relations = UmlYes;
    objectdiagram_settings.shadow = UmlYes;
    objectdiagram_settings.show_stereotype_properties = UmlNo;

    componentdiagram_settings.package_name_in_tab = UmlNo;
    componentdiagram_settings.show_context_mode = noContext;
    componentdiagram_settings.auto_label_position = UmlYes;
    componentdiagram_settings.draw_all_relations = UmlYes;
    componentdiagram_settings.shadow = UmlYes;
    componentdiagram_settings.componentdrawingsettings.draw_component_as_icon = UmlNo;
    componentdiagram_settings.componentdrawingsettings.show_component_req_prov = UmlNo;
    componentdiagram_settings.componentdrawingsettings.show_component_rea = UmlNo;
    componentdiagram_settings.componentdrawingsettings.show_stereotype_properties = UmlNo;

    deploymentdiagram_settings.package_name_in_tab = UmlNo;
    deploymentdiagram_settings.show_context_mode = noContext;
    deploymentdiagram_settings.write_horizontally = UmlYes;
    deploymentdiagram_settings.auto_label_position = UmlYes;
    deploymentdiagram_settings.draw_all_relations = UmlYes;
    deploymentdiagram_settings.shadow = UmlYes;
    deploymentdiagram_settings.componentdrawingsettings.draw_component_as_icon = UmlNo;
    deploymentdiagram_settings.componentdrawingsettings.show_component_req_prov = UmlNo;
    deploymentdiagram_settings.componentdrawingsettings.show_component_rea = UmlNo;
    deploymentdiagram_settings.componentdrawingsettings.show_stereotype_properties = UmlNo;

    statediagram_settings.package_name_in_tab = UmlNo;
    statediagram_settings.show_context_mode = noContext;
    statediagram_settings.auto_label_position = UmlYes;
    statediagram_settings.write_label_horizontally = UmlYes;
    statediagram_settings.show_trans_definition = UmlNo;
    statediagram_settings.draw_all_relations = UmlYes;
    statediagram_settings.shadow = UmlYes;
    statediagram_settings.statedrawingsettings.show_activities = UmlYes;
    statediagram_settings.statedrawingsettings.region_horizontally = UmlYes;
    statediagram_settings.statedrawingsettings.drawing_language = UmlView;
    statediagram_settings.statedrawingsettings.show_stereotype_properties = UmlNo;

    activitydiagram_settings.package_name_in_tab = UmlNo;
    activitydiagram_settings.show_context_mode = noContext;
    activitydiagram_settings.show_opaque_action_definition = UmlNo;
    activitydiagram_settings.auto_label_position = UmlYes;
    activitydiagram_settings.write_label_horizontally = UmlNo;
    activitydiagram_settings.draw_all_relations = UmlYes;
    activitydiagram_settings.shadow = UmlYes;
    activitydiagram_settings.activitydrawingsettings.show_infonote = UmlYes;
    activitydiagram_settings.activitydrawingsettings.drawing_language = UmlView;
    activitydiagram_settings.activitydrawingsettings.show_stereotype_properties = UmlNo;

    class_color = UmlYellow;
    package_color = UmlTransparent;
    fragment_color = UmlTransparent;
    subject_color = UmlTransparent;
    note_color = UmlBlue;
    usecase_color = UmlYellow;
    component_color = UmlGreen;
    artifact_color = UmlGreen;
    deploymentnode_color = UmlGray;
    duration_color = UmlTransparent;
    continuation_color = UmlGray;
    state_color = UmlYellow;
    stateaction_color = UmlTransparent;
    activity_color = UmlTransparent;
    activityregion_color = UmlTransparent;
    activitypartition_color = UmlTransparent;
    activityaction_color = UmlTransparent;
    parameterpin_color = UmlWhite;
}

BrowserPackage::BrowserPackage(QString s, BrowserNode * parent, int id)
    : BrowserNode(s, parent), Labeled<BrowserPackage>(all, id),
      def(new PackageData), associated_diagram(0), is_imported(FALSE)
{
    make();
    is_modified = (id == 0);
    revision = 1;
    owner = -1;
}

BrowserPackage::BrowserPackage(const BrowserPackage * model,
                               BrowserNode * p)
    : BrowserNode(model->name, p), Labeled<BrowserPackage>(all, 0)
{
    def = new PackageData(model->def);
    def->set_browser_node(this);
    comment = model->comment;
    classdiagram_settings = model->classdiagram_settings;
    class_settings = model->class_settings;
    usecasediagram_settings = model->usecasediagram_settings;
    sequencediagram_settings = model->sequencediagram_settings;
    collaborationdiagram_settings = model->collaborationdiagram_settings;
    objectdiagram_settings = model->objectdiagram_settings;
    componentdiagram_settings = model->componentdiagram_settings;
    deploymentdiagram_settings = model->deploymentdiagram_settings;
    statediagram_settings = model->statediagram_settings;
    activitydiagram_settings = model->activitydiagram_settings;
    class_color = model->class_color;
    package_color = model->package_color;
    note_color = model->note_color;
    fragment_color = model->fragment_color;
    subject_color = model->subject_color;
    usecase_color = model->usecase_color;
    component_color = model->component_color;
    artifact_color = model->artifact_color;
    deploymentnode_color = model->deploymentnode_color;
    duration_color = model->duration_color;
    continuation_color = model->continuation_color;
    state_color = model->state_color;
    stateaction_color = model->stateaction_color;
    activity_color = model->activity_color;
    activityregion_color = model->activityregion_color;
    activitypartition_color = model->activitypartition_color;
    activityaction_color = model->activityaction_color;
    parameterpin_color = model->parameterpin_color;

    is_modified = true;
    revision = 1;
    owner = -1;
}

BrowserPackage::BrowserPackage(int id)
    : Labeled<BrowserPackage>(all, id),
      def(new PackageData), associated_diagram(0), is_imported(FALSE)
{
    make();
    is_modified = (id == 0);
    revision = 1;
    owner = -1;
}

BrowserPackage::~BrowserPackage()
{
    if (this == BrowserView::get_project()) {
        // to accelerate removals
        removed.clear();
        clear(FALSE);
    }
    else {
        if (deletedp() && !modifiedp()) {
            QString fn;

            fn.sprintf("%d", get_ident());

            QDir d = BrowserView::get_dir();

            QFile::remove(d.absFilePath(fn));
        }

        removed.remove(this);
        all.remove(get_ident());
    }

    delete def;
}

void BrowserPackage::make()
{
    def->set_browser_node(this);

    class_color = UmlDefaultColor;
    note_color = UmlDefaultColor;
    package_color = UmlDefaultColor;
    fragment_color = UmlDefaultColor;
    subject_color = UmlDefaultColor;
    duration_color = UmlDefaultColor;
    continuation_color = UmlDefaultColor;
    usecase_color = UmlDefaultColor;
    component_color = UmlDefaultColor;
    artifact_color = UmlDefaultColor;
    deploymentnode_color = UmlDefaultColor;
    state_color = UmlDefaultColor;
    stateaction_color = UmlDefaultColor;
    activity_color = UmlDefaultColor;
    activityregion_color = UmlDefaultColor;
    activitypartition_color = UmlDefaultColor;
    activityaction_color = UmlDefaultColor;
    parameterpin_color = UmlDefaultColor;
}

BrowserNode * BrowserPackage::duplicate(BrowserNode * p, QString name)
{
    BrowserPackage * result = new BrowserPackage(this, p);

    result->set_name((name.isEmpty()) ? get_name() : name);
    result->update_stereotype();

    return result;
}

void BrowserPackage::clear(bool old)
{
    if (! old)
        set_user_id(-1);

    all.clear(old);
    BrowserClassView::clear(old);
    BrowserUseCaseView::clear(old);
    BrowserComponentView::clear(old);
    BrowserDeploymentView::clear(old);
    BrowserDiagram::clear(old);

    if (! old)
        check_ids_cleared();
}

void BrowserPackage::update_idmax_for_root()
{
    all.update_idmax_for_root();
    BrowserClassView::update_idmax_for_root();
    BrowserUseCaseView::update_idmax_for_root();
    BrowserComponentView::update_idmax_for_root();
    BrowserDeploymentView::update_idmax_for_root();
    BrowserDiagram::update_idmax_for_root();
}

void BrowserPackage::prepare_update_lib() const
{
    all.memo_id_oid(get_ident(), original_id);

    for (Q3ListViewItem * child = firstChild();
         child != 0;
         child = child->nextSibling())
        ((BrowserNode *) child)->prepare_update_lib();
}

void BrowserPackage::support_file(Q3Dict<char> & files, bool add) const
{
    QString s;

    s.setNum(get_ident());

    if (add)
        files.insert(s, (char *) 1);
    else
        files.remove(s);

    for (Q3ListViewItem * child = firstChild();
         child != 0;
         child = child->nextSibling())
        ((BrowserNode *) child)->support_file(files, add);
}

void BrowserPackage::referenced_by(Q3PtrList<BrowserNode> & l, bool ondelete)
{
    BrowserNode::referenced_by(l, ondelete);

    if (! ondelete) {
        BrowserActivityDiagram::compute_referenced_by(l, this, "packagecanvas", "package_ref");
        BrowserClassDiagram::compute_referenced_by(l, this, "packagecanvas", "package_ref");
        BrowserColDiagram::compute_referenced_by(l, this, "packagecanvas", "package_ref");
        BrowserComponentDiagram::compute_referenced_by(l, this, "packagecanvas", "package_ref");
        BrowserDeploymentDiagram::compute_referenced_by(l, this, "packagecanvas", "package_ref");
        BrowserObjectDiagram::compute_referenced_by(l, this, "packagecanvas", "package_ref");
        BrowserSeqDiagram::compute_referenced_by(l, this, "packagecanvas", "package_ref");
        BrowserStateDiagram::compute_referenced_by(l, this, "packagecanvas", "package_ref");
        BrowserUseCaseDiagram::compute_referenced_by(l, this, "packagecanvas", "package_ref");
    }
}

const QPixmap * BrowserPackage::pixmap(int) const
{
    if (this == BrowserView::get_project())
        return 0;

    const char * st = def->get_stereotype();

    if (strcmp(st, "profile") == 0)
        return ((deletedp()) ? DeletedProfileIcon : ProfileIcon);

    if (deletedp())
        return DeletedPackageIcon;

    const QPixmap * px = ProfiledStereotypes::browserPixmap(st);

    return (px != 0) ? px : PackageIcon;
}

void BrowserPackage::iconChanged()
{
    repaint();
    def->modified();
}

void BrowserPackage::update_stereotype(bool rec)
{
    if (def != 0) {
        const char * stereotype = def->get_stereotype();

        if (show_stereotypes &&
                stereotype[0] &&
                (strcmp(stereotype, "profile") != 0)) {
            QString s = QString(QLatin1String(stereotype));
            int index = s.find(':');

            setText(0,
                    "<<" + ((index == -1) ? s : s.mid(index + 1))
                    + ">> " + name);
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

QString BrowserPackage::full_name(bool rev, bool) const
{
    if (!rev && !full_path.isEmpty())
        return full_path;

    if (this == BrowserView::get_project())
        return QString();

    QString p = ((BrowserNode *) parent())->full_name(FALSE, FALSE);

    if (p.isEmpty())
        full_path = name;
    else if (rev)
        return name + (FullPathPrefix + p + FullPathPostfix);
    else
        full_path = (p + (FullPathDotDot + name));

    return full_path;
}

void BrowserPackage::prepare_for_sort()
{
    IdIterator<BrowserPackage> it(all);

    while (it.current() != 0) {
        it.current()->full_path = QString();
        ++it;
    }
}

// just check if the inheritance already exist
QString BrowserPackage::check_inherit(const BrowserNode * new_parent) const
{
    Q3ListViewItem * child;

    for (child = firstChild(); child != 0; child = child->nextSibling()) {
        BrowserNode * ch = ((BrowserNode *) child);

        if ((ch->get_type() == UmlInherit) &&
                ((((SimpleRelationData *) ch->get_data())->get_end_node())
                 == new_parent))
            return TR("already exist");
    }

    return (new_parent != this) ? QString() : TR("circular inheritance");
}

static int phase_renumerotation = -1;

void BrowserPackage::menu()
{
    Q3PopupMenu m(0);
    Q3PopupMenu genm(0);
    Q3PopupMenu revm(0);
    Q3PopupMenu roundtripm(0);
    Q3PopupMenu roundtripbodym(0);
    Q3PopupMenu toolm(0);
    Q3PopupMenu importm(0);
    bool isprofile = (strcmp(def->get_stereotype(), "profile") == 0);

    MenuFactory::createTitle(m, def->definition(FALSE, TRUE));
    m.insertSeparator();

    if (!deletedp()) {
        if (!is_read_only && (edition_number == 0)) {
            m.setWhatsThis(m.insertItem(TR("New package"), 0),
                           TR("to add a new sub <i>package</i>"));
            m.setWhatsThis(m.insertItem(TR("New use case view"), 1),
                           TR("to add a new <i>use case view</i>"));

            if (isprofile)
                m.setWhatsThis(m.insertItem(TR("New class/stereotype view"), 2),
                               TR("to add a new <i>class/stereotype view</i>"));
            else {
                m.setWhatsThis(m.insertItem(TR("New class view"), 2),
                               TR("to add a new <i>class view</i>"));
                m.setWhatsThis(m.insertItem(TR("New component view"), 3),
                               TR("to add a new <i>class view</i>"));
                m.setWhatsThis(m.insertItem(TR("New deployment view"), 4),
                               TR("to add a new <i>deployment view</i>"));
            }

            m.setWhatsThis(m.insertItem(TR("New profile"), 15),
                           TR("to add a new <i>profile</i>"));
            m.insertSeparator();
            m.setWhatsThis(m.insertItem(TR("Import project"), 14),
                           TR("to import the contents of a <i>project</i> under \
                              the current <i>package</i>"));
                              m.setWhatsThis(m.insertItem(TR("Import project as library"), 16),
                                             TR("to import the contents of a <i>project</i> under \
                                                the current <i>package</i> and to be able later to reimport it to update it"));
        }

        if (!is_edited) {
            if ((edition_number == 0) &&
                    is_from_lib() &&
                    !((BrowserPackage *) parent())->is_from_lib()) {
                m.setWhatsThis(m.insertItem(TR("Update imported library"), 17),
                               TR("to update the previously imported <i>project</i>"));
                m.insertSeparator();
            }

            m.setWhatsThis(m.insertItem(TR("Edit"), 5),
                           TR("to edit the package"));

            if (!is_read_only) {
                m.insertSeparator();

                if (this == BrowserView::get_project()) {
                    m.setWhatsThis(m.insertItem(TR("Edit generation settings"), 8),
                                   TR("to set how an Uml type is compiled in C++ etc..., \
                                      to set the default parameter passing, to set the default code \
                                      produced for an attribute etc..., and to set the root directories"));
                                      m.setWhatsThis(m.insertItem(TR("Edit reverse/roundtrip settings"), 18),
                                                     TR("to specify directory/file to not reverse/roundtrip them"));
                                   m.setWhatsThis(m.insertItem(TR("Edit default stereotypes"), 6),
                                                  TR("to set the default stereotypes list"));
                }

                m.setWhatsThis(m.insertItem(TR("Edit class settings"), 7),
                               TR("to set the sub classes's settings"));
                m.setWhatsThis(m.insertItem(TR("Edit drawing settings"), 9),
                               TR("to set how the sub <i>diagrams</i>'s items must be drawn"));

                if (this == BrowserView::get_project()) {
                    m.insertItem(TR("Import"), &importm);
                    m.setWhatsThis(importm.insertItem(TR("Generation settings"), 28),
                                   TR("to import the generation settings from an other project, \
                                      note that the root directories are not imported"));
                                      m.setWhatsThis(importm.insertItem(TR("Default stereotypes"), 29),
                                                     TR("to import the default stereotypes from an other project"));
                }

                if ((edition_number == 0) && (this != BrowserView::get_project())) {
                    m.insertSeparator();
                    m.setWhatsThis(m.insertItem(TR("Delete"), 10),
                                   TR("to delete the <i>package</i> and its sub items. \
                                      Note that you can undelete it after"));
                }
            }
            else if ((edition_number == 0) &&
                     is_from_lib() &&
                     !((BrowserPackage *) parent())->is_from_lib() &&
                     !((BrowserPackage *) parent())->is_read_only) {
                m.insertSeparator();
                m.setWhatsThis(m.insertItem(TR("Delete"), 10),
                               TR("to delete the <i>package</i> and its sub items. \
                                  Note that you can undelete it after"));
            }
        }

        m.insertSeparator();
        m.setWhatsThis(m.insertItem(TR("Referenced by"), 13),
                       TR("to know who reference the <i>package</i> \
                          through a relation"));
                          mark_menu(m, TR("the package"), 90);
                       ProfiledStereotypes::menu(m, this, 99990);

                if (! isprofile) {
            bool cpp = GenerationSettings::cpp_get_default_defs();
            bool java = GenerationSettings::java_get_default_defs();
            bool php = GenerationSettings::php_get_default_defs();
            bool python = GenerationSettings::python_get_default_defs();
            bool idl = GenerationSettings::idl_get_default_defs();

            if (cpp || java || php || python || idl) {
                m.insertSeparator();
                m.insertItem(TR("Generate"), &genm);

                if (cpp)
                    genm.insertItem("C++", 20);

                if (java)
                    genm.insertItem("Java", 21);

                if (php)
                    genm.insertItem("Php", 22);

                if (python)
                    genm.insertItem("Python", 34);

                if (idl)
                    genm.insertItem("Idl", 23);
            }

            if (edition_number == 0) {
                if (!is_read_only && (cpp || java || php || python)) {
                    m.insertItem(TR("Reverse"), &revm);

                    if (cpp)
                        revm.insertItem("C++", 24);

                    if (java) {
                        revm.insertItem("Java", 25);
                        revm.insertItem("Java Catalog", 26);
                    }

                    if (php)
                        revm.insertItem("Php", 32);

//#warning reverse python


                    if (java || cpp) {
                        m.insertItem(TR("Roundtrip"), &roundtripm);

                        if (cpp) roundtripm.insertItem("C++", 38);

                        if (java) roundtripm.insertItem("Java", 37);
                    }

                    if (preserve_bodies()) {
                        m.insertItem(TR("Roundtrip body"), &roundtripbodym);

                        if (cpp)
                            roundtripbodym.insertItem("C++", 30);

                        if (java)
                            roundtripbodym.insertItem("Java", 31);

                        if (php)
                            roundtripbodym.insertItem("Php", 33);

                        if (python)
                            roundtripbodym.insertItem("Python", 36);
                    }
                }
            }
        }

        if (Tool::menu_insert(&toolm,
                              (this == BrowserView::get_project()) ? UmlProject : UmlPackage,
                              100)) {
            m.insertSeparator();
            m.insertItem(TR("Tool"), &toolm);
        }
    }
    else if (!is_read_only && (edition_number == 0)) {
        m.setWhatsThis(m.insertItem(TR("Undelete"), 11),
                       TR("undelete the <i>package</i>. Do not undelete its sub items"));
        m.setWhatsThis(m.insertItem(TR("Undelete recursively"), 12),
                       TR("undelete the <i>package</i> and its sub items"));
    }

    if (user_id() == 0) {
        m.insertSeparator();
        m.insertSeparator();
        m.insertSeparator();
        m.insertSeparator();
        m.insertSeparator();

        switch (phase_renumerotation) {
        case -1:
            if (this == BrowserView::get_project())
                m.insertItem("PREPARE la renumerotation", 27);

            break;

        case 0:
            m.insertItem("Renumerotation BASE", 27);
            break;

        case 1:
            m.insertItem("Renumerotation USER", 27);
            break;
        }
    }

    exec_menu_choice(m.exec(QCursor::pos()));
}

static bool contain_profile(BrowserPackage * p)
{
    Q3ListViewItem * child;

    for (child = p->firstChild(); child != 0; child = child->nextSibling()) {
        if (!((BrowserNode *) child)->deletedp() &&
                (((BrowserNode *) child)->get_type() == UmlPackage)) {
            if (!strcmp(((BrowserNode *) child)->get_data()->get_stereotype(), "profile"))
                return TRUE;
            else if (contain_profile((BrowserPackage *) child))
                return TRUE;
        }
    }

    return FALSE;
}

void BrowserPackage::exec_menu_choice(int rank)
{
    bool preserve = preserve_bodies();
    bool isprofile = (strcmp(def->get_stereotype(), "profile") == 0);

    switch (rank) {
    case 0:
        add_package(FALSE);
        break;

    case 1: {
        BrowserUseCaseView * v =
                BrowserUseCaseView::add_use_case_view(this);

        if (v == 0)
            return;

        v->select_in_browser();
    }
        break;

    case 2: {
        BrowserClassView * v =
                BrowserClassView::add_class_view(this);

        if (v == 0)
            return;

        v->select_in_browser();
    }
        break;

    case 3:
        if (isprofile)
            return;

    {
        BrowserComponentView * v =
                BrowserComponentView::add_component_view(this);

        if (v == 0)
            return;

        v->select_in_browser();
    }
        break;

    case 4:
        if (isprofile)
            return;

    {
        BrowserDeploymentView * v =
                BrowserDeploymentView::add_deployment_view(this);

        if (v == 0)
            return;

        v->select_in_browser();
    }
        break;

    case 5:
        def->edit();
        return;

    case 6:
        edit_stereotypes();
        return;

    case 7:
        edit_class_settings();
        return;

    case 8:
        edit_gen_settings();
        return;

    case 9:
        edit_drawing_settings();
        return;

    case 10:
        if (!strcmp(get_data()->get_stereotype(), "profile")) {
            bool propag = (msg_warning(TR("Question"),
                                       TR("Propagate the removal of the profile ?"),
                                       1, 2)
                           == 1);
            ProfiledStereotypes::deleted(this, propag);
            delete_it();
        }
        else {
            bool recomp = ProfiledStereotypes::hasStereotype() &&
                    contain_profile(this);

            delete_it();

            if (recomp)
                ProfiledStereotypes::recompute(FALSE);
        }

        ((BrowserNode *) parent())->package_modified();
        break;

    case 11:
        undelete(FALSE);
        ProfiledStereotypes::recompute(FALSE);
        break;

    case 12:
        undelete(TRUE);
        ProfiledStereotypes::recompute(FALSE);
        break;

    case 13:
        ReferenceDialog::show(this);
        return;

    case 14:
        import_project(QString());
        return;

    case 15:
        add_package(TRUE);
        break;

    case 16:
        import_project(QString(), TRUE);
        break;

    case 17:
        update_lib();
        return;

    case 18:
        edit_rev_settings();
        return;

    case 20:
        if (!isprofile) {
            ToolCom::run((verbose_generation())
                         ? ((preserve) ? "cpp_generator -v -p" : "cpp_generator -v")
                         : ((preserve) ? "cpp_generator -p" : "cpp_generator"),
                         this);
        }

        return;

    case 21:
        if (!isprofile) {
            ToolCom::run((verbose_generation())
                         ? ((preserve) ? "java_generator -v -p" : "java_generator -v")
                         : ((preserve) ? "java_generator -p" : "java_generator"),
                         this);
        }

        return;

    case 22:
        if (!isprofile) {
            ToolCom::run((verbose_generation())
                         ? ((preserve) ? "php_generator -v -p" : "php_generator -v")
                         : ((preserve) ? "php_generator -p" : "php_generator"),
                         this);
        }

        return;

    case 34:
        if (!isprofile) {
            ToolCom::run((verbose_generation())
                         ? ((preserve) ? "python_generator -v -p" : "python_generator -v")
                         : ((preserve) ? "python_generator -p" : "python_generator"),
                         this);
        }

        return;

    case 23:
        if (!isprofile)
            ToolCom::run((verbose_generation()) ? "idl_generator -v" : "idl_generator", this);

        return;

    case 24:
        if (!isprofile)
            ToolCom::run("cpp_reverse", this);

        return;

    case 25:
        if (!isprofile)
            ToolCom::run("java_reverse", this);

        return;

    case 37:
        if (!isprofile)
            ToolCom::run("java_roundtrip", this);

        return;

    case 38:
        if (!isprofile)
            ToolCom::run("cpp_roundtrip", this);

        return;

    case 26:
        if (!isprofile)
            ToolCom::run("java_catalog", this);

        return;

    case 32:
        if (!isprofile)
            ToolCom::run("php_reverse", this);

        return;

    case 35:
        if (!isprofile)
            ToolCom::run("python_reverse", this);

        return;

    case 27:
        renumber(phase_renumerotation++);

        if (phase_renumerotation == 2) {

            renumber(2);

            save_all(FALSE);
            QApplication::exit();
        }

        return;

    case 28:
        if (!GenerationSettings::import())
            return;

        break;

    case 29:
        if (!import_stereotypes())
            return;

        break;

    case 30:
        if (!isprofile)
            ToolCom::run((verbose_generation()) ? "roundtrip_body -v c++" : "roundtrip_body c++", this);

        return;

    case 31:
        if (!isprofile)
            ToolCom::run((verbose_generation()) ? "roundtrip_body -v java" : "roundtrip_body java", this);

        return;

    case 33:
        if (!isprofile)
            ToolCom::run((verbose_generation()) ? "roundtrip_body -v php" : "roundtrip_body php", this);

        return;

    case 36:
        if (!isprofile)
            ToolCom::run((verbose_generation()) ? "roundtrip_body -v python" : "roundtrip_body python", this);

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

    package_modified();
}

void BrowserPackage::apply_shortcut(QString s)
{
    int choice = -1;

    if (!deletedp()) {
        if (!is_read_only && (edition_number == 0)) {
            if (s == "New package")
                choice = 0;
            else if (s == "New use case view")
                choice = 1;
            else if (s == "New class view")
                choice = 2;
            else if (s == "New component view")
                choice = 3;
            else if (s == "New deployment view")
                choice = 4;
        }

        if (!is_edited) {
            if (s == "Edit")
                choice = 5;

            if (!is_read_only) {
                if (this == BrowserView::get_project()) {
                    if (s == "Edit generation settings")
                        choice = 8;
                    else if (s == "Edit reverse/roundtrip settings")
                        choice = 18;
                    else if (s == "Edit default stereotypes")
                        choice = 6;
                }

                if (s == "Edit class settings")
                    choice = 7;
                else if (s == "Edit drawing settings")
                    choice = 9;

                if ((edition_number == 0) && (this != BrowserView::get_project())) {
                    if (s == "Delete")
                        choice = 10;
                }
            }
            else if ((s == "Delete") &&
                     (edition_number == 0) &&
                     is_from_lib() &&
                     !((BrowserPackage *) parent())->is_from_lib() &&
                     !((BrowserPackage *) parent())->is_read_only)
                choice = 10;
        }

        if (s == "Referenced by")
            choice = 13;
        else
            mark_shortcut(s, choice, 90);

        if (choice == -1) {
            if (s == "Generate C++")
                choice = 20;
            else if (s == "Generate Java")
                choice = 21;
            else if (s == "Generate Php")
                choice = 22;
            else if (s == "Generate Python")
                choice = 34;
            else if (s == "Generate Idl")
                choice = 23;
            else if (preserve_bodies()) {
                if (s == "Roundtrip C++ operation body")
                    choice = 30;
                else if (s == "Roundtrip Java operation body")
                    choice = 31;
                else if (s == "Roundtrip Php operation body")
                    choice = 32;
                else if (s == "Roundtrip Python operation body")
                    choice = 35;
            }

            if ((choice == -1) && (edition_number == 0))
                Tool::shortcut(s, choice, get_type(), 100);
        }
    }
    else if (!is_read_only && (edition_number == 0)) {
        if (s == "Undelete")
            choice = 11;
        else if (s == "Undelete recursively")
            choice = 12;
    }

    exec_menu_choice(choice);
}

void BrowserPackage::edit_gen_settings()
{
    if (GenerationSettings::edit())
        package_modified();
}

void BrowserPackage::edit_rev_settings()
{
    RevSettingsDialog d;

    d.raise();

    if (d.exec())
        package_modified();
}

void BrowserPackage::edit_stereotypes()
{
    StereotypesDialog d;

    d.raise();

    if (d.exec())
        package_modified();
}

void BrowserPackage::edit_class_settings()
{
    if (class_settings.edit((this == BrowserView::get_project())
                            ? UmlProject : UmlPackage))
        package_modified();
}

void BrowserPackage::edit_drawing_settings()
{
    for (;;) {
        StateSpecVector st;
        ColorSpecVector co(18);

        classdiagram_settings.complete(st, UmlClassView);
        sequencediagram_settings.complete(st, FALSE);
        collaborationdiagram_settings.complete(st, FALSE);
        objectdiagram_settings.complete(st, FALSE);
        usecasediagram_settings.complete(st, FALSE);
        componentdiagram_settings.complete(st, FALSE);
        deploymentdiagram_settings.complete(st, FALSE);
        statediagram_settings.complete(st, FALSE);
        activitydiagram_settings.complete(st, FALSE);

        co[0].set(TR("class color"), &class_color);
        co[1].set(TR("note color"), &note_color);
        co[2].set(TR("package color"), &package_color);
        co[3].set(TR("fragment color"), &fragment_color);
        co[4].set(TR("subject color"), &subject_color);
        co[5].set(TR("use case color"), &usecase_color);
        co[6].set(TR("duration color"), &duration_color);
        co[7].set(TR("continuation color"), &continuation_color);
        co[8].set(TR("component color"), &component_color);
        co[9].set(TR("artifact color"), &artifact_color);
        co[10].set(TR("node color"), &deploymentnode_color);
        co[11].set(TR("state color"), &state_color);
        co[12].set(TR("state action color"), &stateaction_color);
        co[13].set(TR("activity color"), &activity_color);
        co[14].set(TR("activity region color"), &activityregion_color);
        co[15].set(TR("activity partition color"), &activitypartition_color);
        co[16].set(TR("activity action color"), &activityaction_color);
        co[17].set(TR("parameter and pin color"), &parameterpin_color);

        SettingsDialog dialog(&st, &co, this == BrowserView::get_project());

        dialog.raise();

        if (dialog.exec() == QDialog::Accepted) {
            DrawingSettings::modified();
            package_modified();
        }

        if (!dialog.redo())
            break;
    }
}


BrowserNodeList & BrowserPackage::instances(BrowserNodeList & result)
{
    IdIterator<BrowserPackage> it(all);

    while (it.current() != 0) {
        if (!it.current()->deletedp()) {
            it.current()->full_path = QString();
            result.append(it.current());
        }

        ++it;
    }

    result.sort_it();

    return result;
}

BrowserPackage * BrowserPackage::get_package()
{
    if (BrowserView::get_project()->firstChild() == 0)
        // only the prj
        return 0;

    BrowserNode * old;
    QString dummy;
    BrowserNodeList nodes;

    BrowserView::get_project()->enter_child_name(dummy, TR("choose existing package : "),
                                                 UmlPackage, instances(nodes), &old,
                                                 FALSE, TRUE, TRUE);

    return ((BrowserPackage *) old);
}

void BrowserPackage::add_package(bool profile)
{
    QString name;

    if (enter_child_name(name,
                         (profile) ? TR("enter profile's name : ")
                         : TR("enter package's name : "),
                         UmlPackage, TRUE, FALSE)) {
        BrowserPackage * p = new BrowserPackage(name, this);

        if (profile)
            p->def->set_stereotype("profile");

        p->select_in_browser();

        if ((owner != -1) &&
                (msg_warning("Douml",
                             TR("Do you want to be the owner of this new package ?\n"
                                "(other users can't modify it while you are the owner)"),
                             QMessageBox::Yes, QMessageBox::No)
                 == QMessageBox::Yes))
            p->owner = owner;
    }
}

BrowserPackage * BrowserPackage::import_project(QString fn, bool aslib, int id)
{
    bool manual = fn.isEmpty();

    if (manual) {
        fn = Q3FileDialog::getOpenFileName(last_used_directory(), "*.prj");

        if (fn.isEmpty())
            return 0;
    }

    set_last_used_directory(fn);

    QFileInfo fi(fn);

    if (!fi.exists() ||
            !fi.isFile() ||
            (fi.extension(FALSE).lower() != "prj"))
        return 0;

    QString bname = my_baseName(fi);

    if (wrong_child_name(bname, UmlPackage, TRUE, FALSE)) {
        if (manual)
            msg_critical(TR("Error"), TR("illegal name or already used"));

        return 0;
    }

    QApplication::setOverrideCursor(Qt::waitCursor);
    QDir di(fi.dirPath(TRUE));

    BrowserPackage * p = new BrowserPackage(bname, this, id);
    bool err = FALSE;
    unsigned fileformat = 0;

    set_in_import(TRUE, aslib);
    BrowserView::set_imported_project(di, p);
    BrowserNode::pre_load();

    PRE_TRY;

    try {
        fileformat = p->load(TRUE);
    }
    catch (int) {
        err = TRUE;
    }

    POST_TRY;

    if (! err) {
        if (GenerationSettings::add_class_rel_correspondences(fileformat))
            Tool::add();
    }

    BrowserNode::post_load(FALSE);
    idmax_add_margin();
    BrowserDiagram::import();
    BrowserView::set_imported_project(di, 0);
    set_in_import(FALSE);
    clear(TRUE);

    if (err) {
        p->delete_it();
        delete p;
        p = 0;
    }
    else
        package_modified();

    QApplication::restoreOverrideCursor();

    return p;
}

void BrowserPackage::update_lib()
{
    if (must_be_saved()) {
        if (msg_warning("Douml",
                        TR("The project is modified and need to be saved first.\n"
                           "If you don't want to save your modifications reload\n"
                           "the project then ask again for the update\n"
                           "\n"
                           "Save it to perform update ?\n"),
                        QMessageBox::Yes, QMessageBox::Cancel)
                != QMessageBox::Yes)
            return;

        UmlWindow::save_it();
    }

    Q3Dict<char> files;

    support_file(files, TRUE);

    Q3DictIterator<char> it(files);
    QDir d = BrowserView::get_dir();

    while (it.current()) {
        QFileInfo fi(d, it.currentKey());

        if (!fi.exists())
            files.remove(it.currentKey()); // update it
        else if (!fi.isWritable()) {
            msg_critical(TR("Error"), TR("file '%1' is read only, update aborted", it.currentKey()));
            files.clear();
            return;
        }
        else
            ++it;
    }

    QString fn = Q3FileDialog::getOpenFileName(last_used_directory(), name + ".prj");

    if (fn.isEmpty())
        return;

    QFileInfo fi(fn);

    if (my_baseName(fi) != name) {
        msg_critical(TR("Error"), TR("you didn't select project '%1'", name));
        return;
    }

    BrowserPackage * container = (BrowserPackage *) parent();
    Q3ListViewItem * previous = 0;
    Q3ListViewItem * child = container->firstChild();

    while (this != child) {
        previous = child;
        child = child->nextSibling();
    }

    prepare_update_lib();
    container->takeItem(this);

    BrowserPackage * updated = container->import_project(fn, TRUE, get_ident());

    updated->original_id = original_id;
    updated->support_file(files, FALSE);
    it.toFirst();

    for (; it.current(); ++it)
        QFile::remove(d.absFilePath(it.currentKey()));

    if (previous != 0)
        updated->moveItem(previous);
    else
        container->insertItem(updated);

    updated->select_in_browser();

    UmlWindow::save_it();
    container->insertItem(this);
    UmlWindow::reload_it();
}

UmlCode BrowserPackage::get_type() const
{
    return UmlPackage;
}

QString BrowserPackage::get_stype() const
{
    return TR("package");
}

int BrowserPackage::get_identifier() const
{
    return get_ident();
}

const char * BrowserPackage::help_topic() const
{
    return (!strcmp(def->get_stereotype(), "profile"))
            ? "profile"
            : "package";
}

BasicData * BrowserPackage::get_data() const
{
    return def;
}

void BrowserPackage::get_classdiagramsettings(ClassDiagramSettings & r) const
{
    if (! classdiagram_settings.complete(r))
        ((BrowserNode *) parent())->get_classdiagramsettings(r);
}

void BrowserPackage::get_sequencediagramsettings(SequenceDiagramSettings & r) const
{
    if (! sequencediagram_settings.complete(r))
        ((BrowserNode *) parent())->get_sequencediagramsettings(r);
}

void BrowserPackage::get_collaborationdiagramsettings(CollaborationDiagramSettings & r) const
{
    if (! collaborationdiagram_settings.complete(r))
        ((BrowserNode *) parent())->get_collaborationdiagramsettings(r);
}

void BrowserPackage::get_objectdiagramsettings(ObjectDiagramSettings & r) const
{
    if (! objectdiagram_settings.complete(r))
        ((BrowserNode *) parent())->get_objectdiagramsettings(r);
}

void BrowserPackage::get_usecasediagramsettings(UseCaseDiagramSettings & r) const
{
    if (! usecasediagram_settings.complete(r))
        ((BrowserNode *) parent())->get_usecasediagramsettings(r);
}

void BrowserPackage::get_componentdiagramsettings(ComponentDiagramSettings & r) const
{
    if (! componentdiagram_settings.complete(r))
        ((BrowserNode *) parent())->get_componentdiagramsettings(r);
}

void BrowserPackage::get_deploymentdiagramsettings(DeploymentDiagramSettings & r) const
{
    if (! deploymentdiagram_settings.complete(r))
        ((BrowserNode *) parent())->get_deploymentdiagramsettings(r);
}

void BrowserPackage::get_statediagramsettings(StateDiagramSettings & r) const
{
    if (! statediagram_settings.complete(r))
        ((BrowserNode *) parent())->get_statediagramsettings(r);
}

void BrowserPackage::get_activitydiagramsettings(ActivityDiagramSettings & r) const
{
    if (! activitydiagram_settings.complete(r))
        ((BrowserNode *) parent())->get_activitydiagramsettings(r);
}

UmlColor BrowserPackage::get_color(UmlCode who) const
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

    case UmlSubject:
        c = subject_color;
        break;

    case UmlUseCase:
        c = usecase_color;
        break;

    case UmlComponent:
        c = component_color;
        break;

    case UmlArtifact:
        c = artifact_color;
        break;

    case UmlDeploymentNode:
        c = deploymentnode_color;
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

    default: // UmlPackage:
        c = package_color;
        break;
    }

    return (c != UmlDefaultColor)
            ? c
            : ((BrowserNode *) parent())->get_color(who);
}

UmlVisibility BrowserPackage::get_visibility(UmlCode who) const
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

void BrowserPackage::open(bool force_edit)
{
    if (!force_edit &&
            (associated_diagram != 0) &&
            !associated_diagram->deletedp())
        associated_diagram->open(FALSE);
    else if (!is_edited)
        def->edit();
}

void BrowserPackage::on_close()
{
}

BrowserNode * BrowserPackage::get_associated() const
{
    return associated_diagram;
}

void BrowserPackage::set_associated_diagram(BrowserNode * dg,
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

void BrowserPackage::on_delete()
{
    if (associated_diagram && associated_diagram->deletedp())
        associated_diagram = 0;
}

void BrowserPackage::write_id(ToolCom * com)
{
    // to manage project case as any package
    com->write_id(this, UmlPackage - UmlRelations, name);
}

// connexion by a flow or a dependency
QString BrowserPackage::may_connect(UmlCode & l, const BrowserNode * dest) const
{
    switch (dest->get_type()) {
    case UmlPackage:
        switch (l) {
        case UmlDependency:
            l = UmlDependOn;

            //no break;
        case UmlDependOn:
            return 0;

        case UmlGeneralisation:
            l = UmlInherit;

            // no break;
        case UmlInherit:
            return check_inherit(dest);

        default:
            return TR("illegal");
        }

    case UmlActivity:
    case UmlActivityAction:
    case UmlActivityObject:
    case UmlComponent:
    case UmlArtifact:
    case UmlClass:
        switch (l) {
        case UmlDependency:
            l = UmlDependOn;

            //no break;
        case UmlDependOn:
            return 0;

        default:
            return TR("illegal");
        }

    default:
        return TR("illegal");
    }
}

bool BrowserPackage::tool_cmd(ToolCom * com, const char * args)
{
    switch ((unsigned char) args[-1]) {
    case supportFileCmd: {
        QString fn;

        if (this == BrowserView::get_project())
            fn = name +"s.prj" ;
        else
            fn.setNum(get_ident());

        QDir d = BrowserView::get_dir();

        com->write_string(d.absFilePath(fn));
    }

        return TRUE;

    case createCmd: {
        bool ok = TRUE;

        if (is_read_only && !root_permission())
            ok = FALSE;
        else {
            switch (com->get_kind(args)) {
            case UmlPackage:
                if (wrong_child_name(args, UmlPackage, TRUE, FALSE))
                    ok = FALSE;
                else
                    (new BrowserPackage(args, this))->write_id(com);

                break;

            case UmlSimpleRelations: {
                UmlCode c;

                if (!com->get_relation_kind(c, args))
                    ok = FALSE;
                else {
                    BrowserNode * end = (BrowserNode *) com->get_id(args);

                    if (may_connect(c, end).isEmpty())
                        add_relation(c, end)->get_browser_node()->write_id(com);
                    else
                        ok = FALSE;
                }
            }
                break;

            case UmlUseCaseView:
                if (wrong_child_name(args, UmlUseCaseView, TRUE, FALSE))
                    ok = FALSE;
                else
                    (new BrowserUseCaseView(args, this))->write_id(com);

                break;

            case UmlClassView:
                if (wrong_child_name(args, UmlClassView, TRUE, FALSE))
                    ok = FALSE;
                else
                    (new BrowserClassView(args, this))->write_id(com);

                break;

            case UmlComponentView:
                if (wrong_child_name(args, UmlComponentView, TRUE, FALSE))
                    ok = FALSE;
                else
                    (new BrowserComponentView(args, this))->write_id(com);

                break;

            case UmlDeploymentView:
                if (wrong_child_name(args, UmlDeploymentView, TRUE, FALSE))
                    ok = FALSE;
                else
                    (new BrowserDeploymentView(args, this))->write_id(com);

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
        break;

    case setAssocDiagramCmd:
        if (is_read_only && !root_permission())
            com->write_ack(FALSE);
        else {
            set_associated_diagram((BrowserClassDiagram *) com->get_id(args));
            com->write_ack(TRUE);
        }

        return TRUE;

    case importProjectCmd:
        if (is_read_only && !root_permission())
            com->write_id(0);
        else {
            BrowserPackage * p = import_project(args, FALSE);

            if (p == 0)
                com->write_id(0);
            else
                p->write_id(com);
        }

        return TRUE;

    case old_deleteCmd:
    case deleteCmd:
        if ((is_read_only && !root_permission()) ||
                (parent() == 0) ||
                (((BrowserPackage *) parent())->is_read_only && !root_permission()))
            com->write_ack(FALSE);
        else {
            delete_it();
            ((BrowserNode *) parent())->get_data()->modified();
            package_modified();
            ((BrowserNode *) parent())->package_modified();
            com->write_ack(TRUE);
        }

        return TRUE;

    default:
        return (def->tool_cmd(com, args, this, comment) ||
                BrowserNode::tool_cmd(com, args));
    }
}

bool BrowserPackage::tool_global_cmd(ToolCom * com, const char * args)
{
    const WrapperStr & (PackageData::* pf)() const;

    switch ((unsigned char) args[-1]) {
    case findCppNamespaceCmd:
        pf = &PackageData::get_cpp_namespace;
        break;

    case findJavaPackageCmd:
        pf = &PackageData::get_java_package;
        break;

    case findPhpNamespaceCmd:
        pf = &PackageData::get_php_namespace;
        break;

    case findIdlModuleCmd:
        pf = &PackageData::get_idl_module;
        break;

    case findPythonPackageCmd:
        pf = &PackageData::get_python_package;
        break;

    case getProjectCmd:
        BrowserView::get_project()->write_id(com);
        return TRUE;

    case isProjectModifiedCmd:
        com->write_char(must_be_saved() != FALSE);
        return TRUE;

    case saveProjectCmd:
        UmlWindow::save_it();
        return TRUE;

    case updateProfileCmd:
        ProfiledStereotypes::recompute(FALSE);
        return TRUE;

    case findStereotypeCmd: {
        bool case_sensitive;

        switch (*args) {
        case 'y':
            case_sensitive = TRUE;
            args += 2;
            break;

        case 'n':
            case_sensitive = FALSE;
            args += 2;
            break;

        default:
            // first pseudo definition for 4.2.1
            case_sensitive = (com->get_id(args) != 0);
        }

        BrowserClass * r = ProfiledStereotypes::isModeled(args, case_sensitive);

        if (r == 0)
            com->write_id(0);
        else
            r->write_id(com);
    }

        return TRUE;

    default:
        return FALSE;
    }

    // search for a package/namespace/module
    BrowserPackage * p = (BrowserPackage *) com->get_id(args);

    if (p == 0)
        // old version
        p = BrowserView::get_project();

    if ((p = p->find_it(args, pf)) != 0)
        p->write_id(com);
    else
        com->write_id(0);

    return TRUE;
}

BrowserPackage *
BrowserPackage::find_it(const char * s,
                        const WrapperStr & (PackageData::* pf)() const)
{
    if (!deletedp()) {
        if ((def->*pf)() == s) {
            // find !
            return this;
        }
        else {
            for (Q3ListViewItem * child = firstChild();
                 child != 0;
                 child = child->nextSibling()) {
                if (((BrowserNode *) child)->get_type() == UmlPackage) {
                    BrowserPackage * p =
                            ((BrowserPackage *) child)->find_it(s, pf);

                    if (p != 0)
                        return p;
                }
            }
        }
    }

    return 0;
}


void BrowserPackage::DragMoveEvent(QDragMoveEvent * e)
{
    bool rel = UmlDrag::canDecode(e, BrowserSimpleRelation::drag_key(this));

    if (rel ||
            UmlDrag::canDecode(e, UmlPackage) ||
            UmlDrag::canDecode(e, UmlUseCaseView) ||
            UmlDrag::canDecode(e, UmlClassView) ||
            UmlDrag::canDecode(e, UmlComponentView) ||
            UmlDrag::canDecode(e, UmlDeploymentView)) {
        if (!is_read_only ||
                (!rel && (parent() != 0) && !((BrowserPackage *) parent())->is_read_only))
            e->accept();
        else
            e->ignore();
    }
    else if (parent() != 0)
        ((BrowserNode *) parent())->DragMoveInsideEvent(e);
    else
        e->ignore();
}

void BrowserPackage::DragMoveInsideEvent(QDragMoveEvent * e)
{
    if (!is_read_only &&
            (UmlDrag::canDecode(e, UmlPackage) ||
             UmlDrag::canDecode(e, UmlUseCaseView) ||
             UmlDrag::canDecode(e, UmlClassView) ||
             UmlDrag::canDecode(e, UmlComponentView) ||
             UmlDrag::canDecode(e, UmlDeploymentView) ||
             UmlDrag::canDecode(e, BrowserSimpleRelation::drag_key(this))))
        e->accept();
    else
        e->ignore();
}

bool BrowserPackage::may_contains_them(const Q3PtrList<BrowserNode> & l,
                                       BooL & duplicable) const
{
    Q3PtrListIterator<BrowserNode> it(l);

    for (; it.current(); ++it) {
        switch (it.current()->get_type()) {
        case UmlPackage:
        case UmlUseCaseView:
        case UmlClassView:
        case UmlComponentView:
        case UmlDeploymentView:
            break;

        default:
            if (!IsaSimpleRelation(it.current()->get_type()) ||
                    (((const BrowserNode *) it.current()->parent()) != this))
                return FALSE;
        }

        duplicable = FALSE;

        if (! may_contains(it.current(), FALSE))
            return FALSE;
    }

    return TRUE;
}

void BrowserPackage::DropEvent(QDropEvent * e)
{
    DropAfterEvent(e, 0);
}

void BrowserPackage::DropAfterEvent(QDropEvent * e, BrowserNode * after)
{
    BrowserNode * bn = UmlDrag::decode(e, BrowserSimpleRelation::drag_key(this));
    bool rel = (bn != 0);

    if ((rel ||
         ((bn = UmlDrag::decode(e, UmlPackage)) != 0) ||
         ((bn = UmlDrag::decode(e, UmlUseCaseView)) != 0) ||
         ((bn = UmlDrag::decode(e, UmlClassView)) != 0) ||
         ((bn = UmlDrag::decode(e, UmlComponentView)) != 0) ||
         ((bn = UmlDrag::decode(e, UmlDeploymentView)) != 0)) &&
            (bn != after) && (bn != this)) {
        UmlCode what = bn->get_type();

        if (may_contains(bn, what == UmlPackage)) {
            BrowserNode * x = this;

            if ((after == 0) &&
                    !rel &&
                    (parent() != 0) &&
                    ((BrowserNode *) parent())->may_contains(bn, what == UmlPackage)) {
                // have choice
                Q3PopupMenu m(0);

                MenuFactory::createTitle(m, TR("move ") + bn->get_name());
                m.insertSeparator();

                if (!is_read_only)
                    m.insertItem(TR("In ") + QString(get_name()), 1);

                m.insertItem(TR("After ") + QString(get_name()), 2);

                switch (m.exec(QCursor::pos())) {
                case 1:
                    break;

                case 2:
                    after = this;
                    x = (BrowserNode *) parent();
                    break;

                default:
                    e->ignore();
                    return;
                }
            }

            BrowserNode * old_parent = (BrowserNode *) bn->parent();
            bool stereotype_moved = FALSE;

            if ((what == UmlClassView) &&
                    (x != old_parent) &&
                    !strcmp(old_parent->get_data()->get_stereotype(), "profile"))
                stereotype_moved = ((BrowserClassView *) bn)->extract_from_profile();

            if (after)
                bn->moveItem(after);
            else {
                old_parent->takeItem(bn);
                x->insertItem(bn);
            }

            if (old_parent != x) {
                if (stereotype_moved &&
                        (what == UmlClassView) &&
                        !strcmp(x->get_data()->get_stereotype(), "profile"))
                    ((BrowserClassView *) bn)->insert_in_profile();

                old_parent->package_modified();
                bn->modified();
            }

            x->package_modified();
        }
        else if ((parent() != 0) && (after == 0))
            ((BrowserNode *) parent())->DropAfterEvent(e, this);
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

// stereotypes (unicode)

const QStringList & BrowserPackage::default_stereotypes()
{
    return its_default_stereotypes;
}

const QStringList & BrowserPackage::default_stereotypes(UmlCode, const BrowserNode *) const
{
    return relation_default_stereotypes;
}

void BrowserPackage::init()
{
    its_default_stereotypes.clear();
    its_default_stereotypes.append("facade");
    its_default_stereotypes.append("framework");
    its_default_stereotypes.append("model library");
    its_default_stereotypes.append("stub");
    its_default_stereotypes.append("toplevel");
    its_default_stereotypes.append("profile");

    relation_default_stereotypes.clear();
    relation_default_stereotypes.append("access");
    relation_default_stereotypes.append("import");
    relation_default_stereotypes.append("from");
}

void BrowserPackage::save_stereotypes()
{
    QSharedPointer<QByteArray> newdef(new QByteArray());
    QTextStream st(newdef.data(), QIODevice::WriteOnly);
    //  QLOG_INFO() << newdef->data();
    st.setEncoding(QTextStream::Latin1);
    //  QLOG_INFO() << newdef->data();
    nl_indent(st);
    st << "package_stereotypes ";
    st.flush();
    QLOG_INFO() << newdef->data();
    save_unicode_string_list(its_default_stereotypes, st);
    nl_indent(st);
    st << "  " << stringify(UmlDependency);
    save_unicode_string_list(relation_default_stereotypes, st);
    nl_indent(st);
    st << "end";
    nl_indent(st);

    BrowserClass::save_stereotypes(st);
    BrowserUseCase::save_stereotypes(st);
    BrowserArtifact::save_stereotypes(st);
    BrowserAttribute::save_stereotypes(st);
    BrowserOperation::save_stereotypes(st);
    BrowserState::save_stereotypes(st);
    BrowserActivity::save_stereotypes(st);
    BrowserFlow::save_stereotypes(st);
    BrowserInterruptibleActivityRegion::save_stereotypes(st);
    BrowserPseudoState::save_stereotypes(st);
    BrowserStateAction::save_stereotypes(st);
    BrowserParameter::save_stereotypes(st);
    BrowserParameterSet::save_stereotypes(st);
    BrowserActivityNode::save_stereotypes(st);
    BrowserActivityAction::save_stereotypes(st);
    BrowserActivityObject::save_stereotypes(st);
    BrowserExpansionRegion::save_stereotypes(st);
    BrowserActivityPartition::save_stereotypes(st);
    BrowserPin::save_stereotypes(st);
    BrowserComponent::save_stereotypes(st);
    BrowserDeploymentNode::save_stereotypes(st);
    BrowserClassView::save_stereotypes(st);
    BrowserUseCaseView::save_stereotypes(st);
    BrowserComponentView::save_stereotypes(st);
    BrowserDeploymentView::save_stereotypes(st);
    BrowserDiagram::save_stereotypes(st);
    st << "\nend\n";

    st << '\000';
    st.flush();
    QLOG_INFO() << newdef->data();
    save_if_needed("stereotypes", newdef);
}

void BrowserPackage::read_stereotypes(char *& st, char *& k)
{
    if (!strcmp(k, "package_stereotypes")) {
        read_unicode_string_list(its_default_stereotypes, st);
        read_keyword(st, stringify(UmlDependency));
        read_unicode_string_list(relation_default_stereotypes, st);
        read_keyword(st, "end");
        k = read_keyword(st);

        // force profile
        if (its_default_stereotypes.findIndex("profile") == -1)
            its_default_stereotypes.append("profile");
    }
    else
        init();
}

bool BrowserPackage::read_stereotypes(const char * f)
{
    char * s = read_file((f == 0) ? "stereotypes" : f);

    if (s != 0) {
        PRE_TRY;

        try {
            char * st = s;
            char * k = read_keyword(st);

            read_stereotypes(st, k);				// updates k
            BrowserClass::read_stereotypes(st, k);		// updates k
            BrowserUseCase::read_stereotypes(st, k);		// updates k
            BrowserArtifact::read_stereotypes(st, k);		// updates k
            BrowserAttribute::read_stereotypes(st, k);	// updates k
            BrowserOperation::read_stereotypes(st, k);	// updates k
            BrowserState::read_stereotypes(st, k);		// updates k
            BrowserActivity::read_stereotypes(st, k);		// updates k
            BrowserFlow::read_stereotypes(st, k);		// updates k
            BrowserInterruptibleActivityRegion::read_stereotypes(st, k);		// updates k
            BrowserPseudoState::read_stereotypes(st, k);	// updates k
            BrowserStateAction::read_stereotypes(st, k);	// updates k
            BrowserParameter::read_stereotypes(st, k);	// updates k
            BrowserParameterSet::read_stereotypes(st, k);	// updates k
            BrowserActivityNode::read_stereotypes(st, k);	// updates k
            BrowserActivityAction::read_stereotypes(st, k);	// updates k
            BrowserActivityObject::read_stereotypes(st, k);	// updates k
            BrowserExpansionRegion::read_stereotypes(st, k);	// updates k
            BrowserActivityPartition::read_stereotypes(st, k);// updates k
            BrowserPin::read_stereotypes(st, k);		// updates k
            BrowserComponent::read_stereotypes(st, k);	// updates k
            BrowserDeploymentNode::read_stereotypes(st, k);	// updates k
            BrowserClassView::read_stereotypes(st, k);	// updates k
            BrowserUseCaseView::read_stereotypes(st, k);	// updates k
            BrowserComponentView::read_stereotypes(st, k);	// updates k
            BrowserDeploymentView::read_stereotypes(st, k);	// updates k
            BrowserDiagram::read_stereotypes(st, k);		// updates k
        }
        catch (int) {
            ;
        }

        POST_TRY;
        delete [] s;
        return TRUE;
    }

    return FALSE;
}

bool BrowserPackage::import_stereotypes()
{
    QString fn = Q3FileDialog::getOpenFileName(last_used_directory(), "stereotypes");

    if (fn.isEmpty())
        return FALSE;

    set_last_used_directory(fn);
    return read_stereotypes((const char *) fn);
}

// save / restore

void BrowserPackage::save(QTextStream & st, bool, QString &)
{
    // saves just its reference for its father
    nl_indent(st);
    st << "package_ref " << get_ident() << " // " << get_name();
}

// saves all (modified) packages

void BrowserPackage::init_save_counter()
{
    // does nothing
}

void BrowserPackage::save_all(bool modified_only)
{
    IdIterator<BrowserPackage> itcpt(all);

    must_be_saved_counter = 0;
    already_saved = 0;

    while (itcpt.current()) {
        BrowserPackage * pack = itcpt.current();

        if (!pack->deletedp() &&
                (!modified_only ||
                 (pack->is_modified &&
                  (pack->is_imported || !pack->is_read_only))))
            pack->BrowserNode::init_save_counter();

        ++itcpt;
    }

    if (must_be_saved_counter == 0)
        return;

    save_progress = new SaveProgress(must_be_saved_counter);

    IdIterator<BrowserPackage> it(all);
    QDir d = BrowserView::get_dir();
    QString warning;

    while (it.current()) {
        BrowserPackage * pack = it.current();
        bool prj = (pack == BrowserView::get_project());
        QString fn;

        if (prj)
            fn = ((const char *) pack->name) + QString(".prj");
        else
            fn.setNum(it.currentKey());

        if (pack->deletedp()) {
            pack->is_modified = FALSE;
            backup(d, fn);
        }
        else if (!modified_only ||
                 (pack->is_modified &&
                  (pack->is_imported || !pack->is_read_only))) {
            backup(d, fn);

            for (;;) {
                QFile fp(d.absFilePath(fn));

                while (!fp.open(QIODevice::WriteOnly))
                    (void) msg_critical(TR("Error"), TR("Cannot create file\n") + fn,
                                        QMessageBox::Retry);

                if (prj)
                    UmlWindow::historic_add(fp.name());

                QTextStream st(&fp);

                st.setEncoding(QTextStream::Latin1);

                // saves the package own data

                indent0();
                st << "format " << api_format() << "\n";

                save_string(pack->name, st);

                if (!prj)
                    st << " // " << (const char *) pack->full_name();

                indent(+1);

                nl_indent(st);
                st << "revision " << pack->revision;
                nl_indent(st);
                st << "modified_by " << user_id()
                   << " \"" << (const char *) user_name() << '"';

                if (pack->owner != -1) {
                    // owner specified, the save is done => owner is current user !
                    nl_indent(st);
                    st << "owner " << pack->owner << " ";

                    if (pack->owner > 1)
                        save_string(user_name(), st);
                }

                if (prj) {
                    GenerationSettings::save_dirs(st);
                    GenerationSettings::save_descriptions(st);
                    nl_indent(st);
                    save_stereotypes();
                    GenerationSettings::save();
                    Tool::save();
                }

                nl_indent(st);
                st << "// class settings";
                pack->class_settings.save(st);
                nl_indent(st);
                st << "//class diagram settings";
                pack->classdiagram_settings.save(st);
                nl_indent(st);
                st << "//use case diagram settings";
                pack->usecasediagram_settings.save(st);
                nl_indent(st);
                st << "//sequence diagram settings";
                pack->sequencediagram_settings.save(st);
                nl_indent(st);
                st << "//collaboration diagram settings";
                pack->collaborationdiagram_settings.save(st);
                nl_indent(st);
                st << "//object diagram settings";
                pack->objectdiagram_settings.save(st);
                nl_indent(st);
                st << "//component diagram settings";
                pack->componentdiagram_settings.save(st);
                nl_indent(st);
                st << "//deployment diagram settings";
                pack->deploymentdiagram_settings.save(st);
                nl_indent(st);
                st << "//state diagram settings";
                pack->statediagram_settings.save(st);
                nl_indent(st);
                st << "//activity diagram settings";
                pack->activitydiagram_settings.save(st);
                nl_indent(st);

                BooL nl = FALSE;

                save_color(st, "class_color", pack->class_color, nl);
                save_color(st, "duration_color", pack->duration_color, nl);
                save_color(st, "continuation_color", pack->continuation_color, nl);
                save_color(st, "note_color", pack->note_color, nl);
                save_color(st, "fragment_color", pack->fragment_color, nl);
                save_color(st, "subject_color", pack->subject_color, nl);
                save_color(st, "usecase_color", pack->usecase_color, nl);
                save_color(st, "package_color", pack->package_color, nl);
                save_color(st, "component_color", pack->component_color, nl);
                save_color(st, "artifact_color", pack->component_color, nl);
                save_color(st, "deploymentnode_color", pack->deploymentnode_color, nl);
                save_color(st, "state_color", pack->state_color, nl);
                save_color(st, "stateaction_color", pack->stateaction_color, nl);
                save_color(st, "activity_color", pack->activity_color, nl);
                save_color(st, "activityregion_color", pack->activityregion_color, nl);
                save_color(st, "activitypartition_color", pack->activitypartition_color, nl);
                save_color(st, "activityaction_color", pack->activityaction_color, nl);
                save_color(st, "parameterpin_color", pack->parameterpin_color, nl);

                if (prj) {
                    nl_indent(st);
                    st << "font_size " << NormalFont.pointSize();
                    nl_indent(st);
                    st << "diagram_format "
                       << stringify(UmlWindow::default_format()) << '\n';
                    nl_indent(st);
                    st << "image_root_dir ";
                    save_string(fromUnicode(UmlWindow::images_root_dir()), st);
                    st << '\n';
                    nl_indent(st);
                    st << "mark_for_import";
                    nl_indent(st);
                }

                pack->def->save(st, warning);

                if (pack->associated_diagram != 0) {
                    nl_indent(st);
                    st << "associated_diagram ";
                    pack->associated_diagram->save(st, TRUE, warning);
                }

                pack->BrowserNode::save(st);

                // saves the package's sub elts

                Q3ListViewItem * child = pack->firstChild();

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

                if (prj) {
                    if (preserve_bodies())
                        st << "\npreserve_bodies\n";
                    else if (add_operation_profile())
                        st << "\nadd_operation_profile\n";
                }

                st << "\nend\n";
                st.flush();

                fp.close();

                if (static_cast<uint>(fp.status()) == IO_Ok) {
                    pack->is_imported = pack->is_modified = FALSE;

                    // for saveAs
                    if (!modified_only && !pack->is_from_lib() && !pack->is_api_base())
                        pack->is_read_only = FALSE;

                    // all done
                    break;
                }

                (void) msg_critical(TR("Error"),
                                    TR("Error while writting in\n%1\nmay be your disk is full", fn),
                                    QMessageBox::Retry);
            }
        }

        ++it;
    }

    if (save_progress != 0)
        delete save_progress;

    if (!warning.isEmpty())
        warn(warning);

    delete_backup(d);
}

bool BrowserPackage::must_be_saved()
{
    IdIterator<BrowserPackage> it(all);

    while (it.current()) {
        if (it.current()->is_modified &&
                !it.current()->deletedp() &&
                !it.current()->is_read_only)
            return TRUE;

        ++it;
    }

    return FALSE;
}

void BrowserPackage::save_session(QTextStream & st)
{
    if (show_stereotypes)
        st << "show_stereotypes\n";

    QString warning;

    if (! marked_list.isEmpty()) {
        Q3PtrListIterator<BrowserNode> it(marked_list);

        st << "marked\n";

        for (; it.current() != 0; ++it) {
            st << "  ";
            it.current()->save(st, TRUE, warning);
            st << '\n';
        }

        st << "end\n";
    }

    BrowserNode * sel = (BrowserNode *) listView()->currentItem();

    if ((sel != 0) && !sel->deletedp()) {
        st << "selected ";
        sel->save(st, TRUE, warning);
        st << '\n';
    }

    if (isOpen()) {
        Q3ListViewItem * child;

        st << "open\n";

        for (child = firstChild(); child != 0; child = child->nextSibling()) {
            BrowserNode * ch = ((BrowserNode *) child);

            if (!ch->deletedp())
                ch->save_open_list(st);
        }

        st << "end\n";
    }

    if (!completion())
        st << "nocompletion\n";

    st << "end\n";
}

static void open_it(Q3ListViewItem * bn)
{
    if (bn && !bn->isOpen()) {
        open_it(bn->parent());
        bn->setOpen(TRUE);
    }
}

void BrowserPackage::read_session(char *& st, const char * k)
{
    if (!strcmp(k, "diagrams")) {
        BrowserDiagram * active_one = 0;

        while (strcmp(k = read_keyword(st), "end")) {
            bool active;

            if (!strcmp(k, "active")) {
                active = TRUE;
                k = read_keyword(st);
            }
            else
                active = FALSE;

            BrowserDiagram * bn = (BrowserDiagram *) get_it(k, read_id(st));

            if (bn != 0) {
                ReadContext context = current_context();

                bn->open(FALSE);
                restore_context(context);
                bn->read_session(st);

                if (active)
                    active_one = bn;
            }
        }

        if (active_one != 0)
            active_one->open(FALSE);

        k = read_keyword(st);
    }

    if (!strcmp(k, "show_stereotypes")) {
        if (!show_stereotypes)
            toggle_show_stereotypes();

        k = read_keyword(st);
    }
    else if (show_stereotypes)
        toggle_show_stereotypes();

    if (!strcmp(k, "marked")) {
        while (strcmp(k = read_keyword(st), "end")) {
            BrowserNode * bn = get_it(k, read_id(st));

            if (bn != 0)
                bn->toggle_mark();
        }

        k = read_keyword(st);
    }

    BrowserNode * sel;

    if (! strcmp(k, "selected")) {
        k = read_keyword(st);

        sel = get_it(k, read_id(st));

        k = read_keyword(st);
    }
    else
        sel = 0;

    if (! strcmp(k, "open")) {
        while (strcmp(k = read_keyword(st), "end"))
            open_it(get_it(k, read_id(st)));

        k = read_keyword(st);
    }

    if (! strcmp(k, "nocompletion")) {
        set_completion(FALSE);
        k = read_keyword(st);
    }
    else
        set_completion(TRUE);

    if (sel != 0) {
        listView()->setSelected(sel, TRUE);
        listView()->ensureItemVisible(sel);
    }

    // don't check old 'scroll n> <n>' or new 'end'
}

BrowserNode * BrowserPackage::get_it(const char * k, int id)
{
    if (!strcmp(k, "package_ref"))
        return all[id];

    BrowserNode * r;

    if (((r = BrowserUseCaseView::get_it(k, id)) == 0) &&
            ((r = BrowserClassView::get_it(k, id)) == 0) &&
            ((r = BrowserComponentView::get_it(k, id)) == 0) &&
            ((r = BrowserDeploymentView::get_it(k, id)) == 0))
        r = BrowserSimpleRelation::get_it(k, id);

    return r;
}

// id is the old package's ident in case of an import
unsigned BrowserPackage::load(bool recursive, int id)
{
    unsigned result = ~0;
    QString fn;
    bool prj = (this == BrowserView::get_project()) ||
            (this == BrowserView::get_imported_project());

    if (prj) {
        //const char * tName = name.operator WrapperStr();
        ///QByteArray ba = QString("test").toLatin1();
        const char * tName = this->name;
        fn.sprintf("%s.prj", tName);
        QLOG_INFO() << fn;
    }
    else
        fn.setNum((id == -1) ? get_ident() : id);

    QFile fp((in_import())
             ? BrowserView::get_import_dir().absFilePath(fn)
             : BrowserView::get_dir().absFilePath(fn));
    int sz;

    ReadContext context = current_context();

    if ((sz = open_file(fp, QIODevice::ReadOnly)) != -1) {
        char * s = new char[sz + 1];
        int offset = 0;
        int nread;

        is_read_only = (!in_import() && read_only_file()) ||
                ((user_id() != 0) && is_api_base());

        do {
            if ((nread = fp.readBlock(s + offset, sz - offset)) == -1) {
                msg_critical(TR("Error"),
                             BrowserView::get_dir().absFilePath(fn)
                             + TR("cannot be read"));
                delete [] s;
                THROW_ERROR 0;
            }

            offset += nread;
        }
        while (offset != sz);

        s[sz] = 0;

        bool err = FALSE;

        PRE_TRY;

        try {
            char * st = s;
            char * k;

            read_keyword(st, "format");
            set_read_file_format(read_unsigned(st));

            if (read_file_format() > api_format(true)) {
                msg_critical("Error",
                             TR("Your version of DOUML is too old to read this project"));
                throw 0;
            }

            if (in_import() && prj)
            {
                result = read_file_format();

                if (read_file_format() <= 21) {
                    msg_critical(TR("Error"),
                                 TR(
                                     "Sorry, the imported project has a too old format.\n"
                                     "To change its format : load this project and save it."));
                    throw 0;
                }

                k = skip_until(st, "mark_for_import");
            }
            else
            {
                set_name(read_string(st));

                k = read_keyword(st);

                if (!strcmp(k, "revision")) {
                    revision = read_unsigned(st) + 1;
                    k = read_keyword(st);

                    if (!strcmp(k, "modified_by")) {
                        read_keyword(st);
                        read_string(st);
                        k = read_keyword(st);
                    }

                    if (! strcmp(k, "owner")) {
                        owner = read_unsigned(st);

                        if (owner != user_id()) {
                            is_read_only = TRUE;
                            set_read_only_file();
                        }

                        if (owner > 1)
                            read_string(st);	// owner's name

                        k = read_keyword(st);
                    }
                }

                if (prj) {
                    result = read_file_format();
                    GenerationSettings::read_dirs(st, k);		// updates k
                    GenerationSettings::read_descriptions(st, k);		// updates k

                    if (read_file_format() <= 6)
                        GenerationSettings::read(st, k);		// updates k

                    if (read_file_format() <= 7) {
                        read_stereotypes(st, k);			// updates k
                        BrowserClass::read_stereotypes(st, k);	// updates k
                        BrowserUseCase::read_stereotypes(st, k);	// updates k
                        BrowserAttribute::read_stereotypes(st, k);	// updates k
                        BrowserOperation::read_stereotypes(st, k);	// updates k
                        BrowserComponent::read_stereotypes(st, k);	// updates k
                        BrowserDeploymentNode::read_stereotypes(st, k); // updates k
                        BrowserClassView::read_stereotypes(st, k);	// updates k
                        BrowserUseCaseView::read_stereotypes(st, k);// updates k
                        BrowserComponentView::read_stereotypes(st, k); // updates k
                        BrowserDeploymentView::read_stereotypes(st, k); // updates k
                        BrowserDiagram::read_stereotypes(st, k);	// updates k
                        Tool::read(st, k);				// updates k
                    }
                }

                if (read_file_format() <= 9)
                    def->read(st, k);			// updates k

                class_settings.read(st, k);		// updates k
                classdiagram_settings.read(st, k);	// updates k
                usecasediagram_settings.read(st, k);	// updates k
                sequencediagram_settings.read(st, k);	// updates k
                collaborationdiagram_settings.read(st, k); // updates k

                if (read_file_format() >= 25)
                    objectdiagram_settings.read(st, k);	// updates k

                componentdiagram_settings.read(st, k);	// updates k
                deploymentdiagram_settings.read(st, k);	// updates k

                if (read_file_format() >= 21)
                    statediagram_settings.read(st, k);	// updates k

                if (read_file_format() >= 26)
                    activitydiagram_settings.read(st, k);	// updates k

                if (this == BrowserView::get_project()) {
                    // old version
                    if (classdiagram_settings.auto_label_position == UmlDefaultState)
                        classdiagram_settings.auto_label_position = UmlYes;

                    if (usecasediagram_settings.auto_label_position == UmlDefaultState)
                        usecasediagram_settings.auto_label_position = UmlYes;

                    if (componentdiagram_settings.auto_label_position == UmlDefaultState)
                        componentdiagram_settings.auto_label_position = UmlYes;

                    if (deploymentdiagram_settings.auto_label_position == UmlDefaultState)
                        deploymentdiagram_settings.auto_label_position = UmlYes;
                }

                read_color(st, "class_color", class_color, k);	// updates k
                read_color(st, "duration", duration_color, k);	// old, updates k
                read_color(st, "duration_color", duration_color, k);	// updates k
                read_color(st, "continuation_color", continuation_color, k);	// updates k
                read_color(st, "note_color", note_color, k);	// updates k
                read_color(st, "fragment_color", fragment_color, k);	// updates k
                read_color(st, "subject_color", subject_color, k);	// updates k
                read_color(st, "usecase_color", usecase_color, k);	// updates k
                read_color(st, "package_color", package_color, k);	// updates k
                read_color(st, "component_color", component_color, k);	// updates k
                read_color(st, "artifact_color", artifact_color, k);	// updates k
                read_color(st, "deploymentnode_color", deploymentnode_color, k);	// updates k
                read_color(st, "state_color", state_color, k);		// updates k
                read_color(st, "stateaction_color", stateaction_color, k);		// updates k
                read_color(st, "activity_color", activity_color, k);		// updates k
                read_color(st, "activityregion_color", activityregion_color, k);		// updates k
                read_color(st, "activitypartition_color", activitypartition_color, k);		// updates k
                read_color(st, "activityaction_color", activityaction_color, k);		// updates k
                read_color(st, "parameterpin_color", parameterpin_color, k);		// updates k
            }

            if (read_file_format() > 9)
            {
                if (prj)
                {
                    if (!strcmp(k, "font_size")) {
                        resize_font((int) read_unsigned(st));
                        k = read_keyword(st);
                    }
                    else
                        init_font();

                    if (!strcmp(k, "diagram_format")) {
                        UmlWindow::set_default_format(canvas_format(read_keyword(st)));
                        k = read_keyword(st);
                    }
                    else
                        UmlWindow::set_default_format(IsoA4);

                    if (!strcmp(k, "image_root_dir")) {
                        UmlWindow::set_images_root_dir(toUnicode(read_string(st)));
                        k = read_keyword(st);
                    }
                    else
                        UmlWindow::set_images_root_dir("");

                    if (strcmp(k, "mark_for_import"))
                        wrong_keyword(k, "mark_for_import");

                    k = read_keyword(st);
                }

                def->read(st, k);			// updates k
            }
            else if (prj)
                init_font();

            if (!strcmp(k, "associated_class_diagram"))
            {
                // old format
                set_associated_diagram(BrowserClassDiagram::read_ref(st, "classdiagram_ref"),
                                       TRUE);
                k = read_keyword(st);
            }
            else if (!strcmp(k, "associated_diagram"))
            {
                set_associated_diagram(BrowserDiagram::read_diagram_ref(st), TRUE);
                k = read_keyword(st);
            }

            BrowserNode::read(st, k, id);	// updates k

            if (strcmp(k, "end"))
            {
                while (BrowserPackage::read(st, k, this, recursive) ||
                       BrowserUseCaseView::read(st, k, this, recursive) ||
                       BrowserClassView::read(st, k, this, recursive) ||
                       ((read_file_format() >= 20) &&
                        BrowserComponentView::read(st, k, this, recursive)) ||
                       BrowserDeploymentView::read(st, k, this, recursive) ||
                       BrowserSimpleRelation::read(st, k, this))
                    k = read_keyword(st);

                if (!strcmp(k, "preserve_bodies"))
                {
                    if (!in_import() && !preserve_bodies())
                        toggle_preserve_bodies();

                    if (add_operation_profile())
                        toggle_add_operation_profile();

                    k = read_keyword(st);
                }
                else
                {
                    if (preserve_bodies())
                        toggle_preserve_bodies();

                    if (!strcmp(k, "add_operation_profile")) {
                        if (!in_import() && !add_operation_profile())
                            toggle_add_operation_profile();

                        k = read_keyword(st);
                    }
                    else if (add_operation_profile())
                        toggle_add_operation_profile();
                }

                if (strcmp(k, "end"))
                    wrong_keyword(k, "end");
            }
        }
        catch (int) {
            err = TRUE;
        }

        POST_TRY;

        delete [] s;

        if (!err && (this == BrowserView::get_project())) {
            Tool::read();

            if (read_file_format() > 6) {
                GenerationSettings::read();

                if (read_file_format() > 7)
                    read_stereotypes();
            }

            GenerationSettings::read_includes_imports();
        }

        restore_context(context);

        if (err) {
            THROW_ERROR 0;
        }
    }
    else {
        THROW_ERROR 0;
    }

    is_imported = is_modified = in_import();

    return result;
}

bool BrowserPackage::load_version(QString fn)
{
    QFileInfo any_fi(fn);
    QDir d(any_fi.dirPath(TRUE));
    const QFileInfoList l = d.entryInfoList("*.prj"); // [lgfreitas] entryInfoList does not return a pointer anymore

    if (!l.empty()) {
        QListIterator<QFileInfo> it(l);
        QFileInfo fi = it.next(); // [lgfreitas] there is no it.current anymore to QList. Java-style iterators

        if (fi.exists()) {
            fn = fi.absFilePath();

            char * s = read_file(fn);

            if (s == 0) {
                msg_critical(TR("Error"), TR("can't read project file"));
                return FALSE;
            }

            bool ok = TRUE;

            PRE_TRY;

            try {
                char * st = s;

                read_keyword(st, "format");
                set_read_file_format(read_unsigned(st));

                if (read_file_format() > api_format(true)) {
                    msg_critical(TR("Error"),
                                 TR("Your version of DOUML is too old to read this project"));
                    throw 0;
                }
            }
            catch (int) {
                ok = FALSE;
            }

            POST_TRY;

            delete [] s;

            return ok;
        }
    }

    msg_critical(TR("Error"), TR("can't find project file"));

    return FALSE;
}


BrowserPackage * BrowserPackage::read_ref(char *& st)
{
    read_keyword(st, "package_ref");

    int id = read_id(st);
    BrowserPackage * result = all[id];

    return (result == 0)
            ? new BrowserPackage(id)
            : result;
}

BrowserPackage * BrowserPackage::read(char *& st, char * k,
                                      BrowserNode * parent,
                                      bool recursive)
{
    if (!strcmp(k, "package_ref")) {
        int id = read_id(st);

        BrowserPackage * r = all[id];

        if (r == 0)
            r = (parent)
                    ? new BrowserPackage("<not yet loaded>", parent, id)
                    : new BrowserPackage(id);
        else if (parent != 0) {
            r->set_parent(parent);
            r->set_name("<not yet loaded>");
        }

        if (recursive)
            r->load(recursive, id);

        return r;
    }
    else
        return 0;
}

void BrowserPackage::package_modified()
{
    repaint();
    is_modified = TRUE;
}

void BrowserPackage::renumber(int phase)
{
    switch (phase) {
    case -1: {
        BrowserNode::renumber(phase);

        QDir dir = BrowserView::get_dir();
        const QFileInfoList l = dir.entryInfoList();

        if (!l.empty()) {
            QListIterator<QFileInfo> it(l);
            QFileInfo fi;

            //[lgfreitas] Qlist changed
            while (it.hasNext()) {
                fi = it.next();

                if (fi.isFile() && fi.baseName().at(0).isDigit())
                    dir.rename(fi.fileName(), "_" + fi.fileName());

                //++it;
            }
        }

        clear(FALSE);
        set_user_id(0);
    }

        return;

    default: {
        QDir dir = BrowserView::get_dir();
        QString old = "_" + QString::number(get_ident());

        new_ident(phase, all);
        dir.rename(old, QString::number(get_ident()));
        BrowserNode::renumber(phase);
    }

        return;

    case 2: {
        QDir dir = BrowserView::get_dir();
        QString filter = "_*";
        const QFileInfoList l = dir.entryInfoList(filter); //[lgfreitas] QList has changed...

        if (!l.empty()) {
            QListIterator<QFileInfo> it(l);
            QFileInfo fi;

            // [lgfreitas] QList has changed
            while (it.hasNext()) {
                fi = it.next();
                QFile::remove(fi.absFilePath());
                //++it;
            }
        }
    }
    }
}
