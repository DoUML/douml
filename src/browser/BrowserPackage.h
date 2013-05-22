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

#ifndef BROWSER_PACKAGE_H
#define BROWSER_PACKAGE_H

#include <q3intdict.h>
//Added by qt3to4:
#include <QTextStream>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QPixmap>

#include "BrowserNode.h"
#include "Labeled.h"
#include "Settings.h"

class PackageData;
class BrowserClassDiagram;
class StereotypesDialog;

class BrowserPackage : public BrowserNode, public Labeled<BrowserPackage>
{
    friend class StereotypesDialog;

protected:
    static QList<BrowserPackage *> removed;
    static IdDict<BrowserPackage> all;

    static QStringList its_default_stereotypes;
    static QStringList relation_default_stereotypes;

    PackageData * def;
    BrowserNode * associated_diagram;
    ClassDiagramSettings classdiagram_settings;
    ClassSettings class_settings;
    UseCaseDiagramSettings usecasediagram_settings;
    SequenceDiagramSettings sequencediagram_settings;
    CollaborationDiagramSettings collaborationdiagram_settings;
    ObjectDiagramSettings objectdiagram_settings;
    ComponentDiagramSettings componentdiagram_settings;
    DeploymentDiagramSettings deploymentdiagram_settings;
    StateDiagramSettings statediagram_settings;
    ActivityDiagramSettings activitydiagram_settings;
    UmlColor class_color;
    UmlColor package_color;
    UmlColor note_color;
    UmlColor fragment_color;
    UmlColor subject_color;
    UmlColor usecase_color;
    UmlColor component_color;
    UmlColor artifact_color;
    UmlColor deploymentnode_color;
    UmlColor duration_color;
    UmlColor continuation_color;
    UmlColor state_color;
    UmlColor stateaction_color;
    UmlColor activity_color;
    UmlColor activityregion_color;
    UmlColor activitypartition_color;
    UmlColor activityaction_color;
    UmlColor parameterpin_color;
    bool is_imported;
    unsigned revision;
    int owner;
    mutable QString full_path;

protected:
    BrowserPackage(int id);
    BrowserPackage * find_it(const char * s, const WrapperStr & (PackageData::* pf)() const);
    void make();
    void exec_menu_choice(int rank);

public:
    BrowserPackage(QString s, BrowserNode * parent, int id = 0);
    BrowserPackage(QString s, BrowserView * parent, int id = 0);
    BrowserPackage(const BrowserPackage * model, BrowserNode * p);
    virtual ~BrowserPackage();

    virtual BrowserNode * get_associated() const;
    void set_associated_diagram(BrowserNode *, bool on_read = FALSE);

    virtual BrowserNode * duplicate(BrowserNode * p,
                                    QString name = QString());
    virtual QString full_name(bool rev = FALSE, bool itself = TRUE) const;
    virtual void menu();
    virtual void apply_shortcut(QString s);
    virtual void open(bool force_edit);
    virtual void on_close();
    virtual UmlCode get_type() const;
    virtual QString get_stype() const;
    virtual int get_identifier() const;
    virtual const char * help_topic() const;
    virtual bool may_contains_them(const QList<BrowserNode *> &,
                                   BooL & duplicable) const;
    virtual BasicData * get_data() const;
    virtual const QStringList & default_stereotypes(UmlCode, const BrowserNode *) const; // non class rel
    virtual void get_classdiagramsettings(ClassDiagramSettings &) const;
    virtual void get_usecasediagramsettings(UseCaseDiagramSettings &) const;
    virtual void get_sequencediagramsettings(SequenceDiagramSettings &) const;
    virtual void get_collaborationdiagramsettings(CollaborationDiagramSettings &) const;
    virtual void get_objectdiagramsettings(ObjectDiagramSettings &) const;
    virtual void get_componentdiagramsettings(ComponentDiagramSettings &) const;
    virtual void get_deploymentdiagramsettings(DeploymentDiagramSettings &) const;
    virtual void get_statediagramsettings(StateDiagramSettings & r) const;
    virtual void get_activitydiagramsettings(ActivityDiagramSettings & r) const;
    virtual UmlColor get_color(UmlCode) const;
    virtual UmlVisibility get_visibility(UmlCode) const;
    virtual void on_delete();
    virtual QString check_inherit(const BrowserNode * parent) const;
    QString may_connect(UmlCode & l, const BrowserNode * dest) const;

    virtual void referenced_by(QList<BrowserNode *> & l, bool ondelete);

    virtual bool tool_cmd(ToolCom * com, const char * args);
    static bool tool_global_cmd(ToolCom * com, const char * args);
    virtual void write_id(ToolCom * com);
    virtual void save(QTextStream &, bool ref, QString & warning);
    virtual void package_modified();

    void save_session(QTextStream & st);
    void read_session(char *& , const char * k);
    static BrowserNode * get_it(const char * k, int id);

    void edit_gen_settings();
    void edit_rev_settings();
    void edit_stereotypes();
    void edit_class_settings();
    void edit_drawing_settings();

    virtual const QPixmap * pixmap(int) const;
    virtual void update_stereotype(bool rec = FALSE);
    virtual void iconChanged();
    virtual void DragMoveEvent(QDragMoveEvent * e);
    virtual void DropEvent(QDropEvent * e);
    virtual void DragMoveInsideEvent(QDragMoveEvent * e);
    virtual void DropAfterEvent(QDropEvent * e, BrowserNode * after);

    void add_package(bool profile);
    unsigned load(bool recursive, int id = -1);
    BrowserPackage * import_project(QString fn, bool aslib = FALSE, int id = 0);

    static BrowserNodeList & instances(BrowserNodeList &);
    static BrowserPackage * get_package();

    static void init();
    static const QStringList & default_stereotypes();
    static void read_stereotypes(char *& , char *&);
    static bool read_stereotypes(const char * = 0);
    static bool import_stereotypes();
    static void save_stereotypes();

    static BrowserPackage * read_ref(char *&);
    static BrowserPackage * read(char *& , char *, BrowserNode *, bool);
    static void save_all(bool modified_only);
    static bool must_be_saved();
    virtual void init_save_counter();
    static bool load_version(QString fn);

    static void clear(bool old);
    static void update_idmax_for_root();
    virtual void renumber(int phase);
    virtual void prepare_update_lib() const;
    virtual void support_file(Q3Dict<char> & files, bool add) const;
    void update_lib();

    static void prepare_for_sort();
};

#endif
