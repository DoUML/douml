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

#ifndef BROWSER_NODE_H
#define BROWSER_NODE_H

#include <QTextStream>
#include <qlist.h>
#include <q3listview.h>
#include <qstringlist.h>
#include <q3dict.h>
//Added by qt3to4:
#include <QDragMoveEvent>
#include <QDropEvent>
#include <Q3PopupMenu>
#include <Q3PtrCollection>

#include "UmlEnum.h"
#include "HaveKeyValueData.h"
#include "mystr.h"
#include "Editable.h"
#include "AType.h"
#include "misc/TypeIdentifier.h"
class QDragMoveEvent;
class Q3PopupMenu;
template <class K> class Q3PtrDict;

class BrowserView;
class BasicData;
class ClassDiagramSettings;
class UseCaseDiagramSettings;
class SequenceDiagramSettings;
class CollaborationDiagramSettings;
class StateDiagramSettings;
class ActivityDiagramSettings;
class ComponentDiagramSettings;
class DeploymentDiagramSettings;
class ComponentDrawingSettings;
class StateDrawingSettings;
class ActivityDrawingSettings;
class ObjectDiagramSettings;
class SimpleClassDiagramSettings;
class BrowserNodeList;
class ToolCom;
class SaveProgress;

/* This class is the base for every node presented in the tree-view on the left */
class BrowserNode : public Q3ListViewItem,
    public HaveKeyValueData,
    public Editable
{
protected:
    QString name;
    WrapperStr comment;
    QByteArray tempBa;
    friend class BrowserView;
    int original_id = 0;	// from project library
    bool is_new = true;	// backup file useless
    bool is_deleted = false;
    bool is_modified = false;
    bool is_read_only = false;
    bool is_edited = false;
    bool is_marked = false;
    //bool is_marked_for_move = false;
    bool is_defined = false;	// to indicate unconsistency due to projectSynchro
    // pre condition not followed

    static bool show_stereotypes;
    static unsigned edition_number;
    static QList<BrowserNode *> marked_list;
    static bool popup_menu_active;
    static QList<UmlCode> generatable_types;
    static SaveProgress * save_progress;
    static int must_be_saved_counter;
    static int already_saved;

    static QString FullPathPrefix;
    static QString FullPathPostfix;
    static QString FullPathDotDot;
    static BrowserView* viewptr;

    BrowserNode();
    BrowserNode(Q3ListView*);

    void set_parent(Q3ListViewItem * parent);
    virtual bool delete_internal(QString & warning);

public:
    BrowserNode(QString s, BrowserView * parent);
    BrowserNode(QString s, BrowserNode * parent);
    virtual ~BrowserNode();

    virtual bool is_undefined() const;

   virtual QString get_name() const {
        return name;
    }
    virtual void set_name(QString s);
    virtual void update_stereotype(bool rec = FALSE);

    static bool edition_active() {
        return edition_number != 0;
    };
    virtual void edit_start();
    virtual void edit_end();
    virtual bool in_edition() const;

    void mark_menu(Q3PopupMenu & m, const char *, int bias) const;
    void mark_shortcut(QString s, int & index, int bias);
    void mark_management(int choice);
    void toggle_mark();
    void set_marked(bool value);//{is_marked = value;}
    bool markedp() const {
        return is_marked;
    }
    static const QList<BrowserNode *> & marked_nodes() {
        return marked_list;
    }
    static const QList<BrowserNode*>  get_marked_nodes()
    {
        return marked_list;
    }

    static void setup_generatable_types();
    static void unmark_all();

//    bool marked_for_move(){return is_marked_for_move;}
//    void set_marked_for_move(bool value){ is_marked_for_move = value;}


    bool is_from_lib() const {
        return original_id != 0;
    }
    virtual void prepare_update_lib() const = 0;
    virtual void support_file(Q3Dict<char> & files, bool add) const;

    virtual bool is_writable() const;	// file writable & not api base
    virtual void delete_it();
    virtual void set_deleted(bool);
    bool deletedp() const {
        return is_deleted;
    };
    void undelete(bool recursive);
    virtual bool undelete(bool rec, QString & warning, QString & renamed);
    void must_be_deleted(); // deleted after load time
    virtual BrowserNode * duplicate(BrowserNode * p,
                                    QString name = QString()) = 0;

    bool nestedp() const {
        return ((BrowserNode *) parent())->get_type() == get_type();
    };
    virtual const char * get_comment() const;
    virtual void set_comment(QString c);
    const char * get_stereotype() const;
    virtual QString stereotypes_properties() const;
    bool may_contains(BrowserNode *, bool rec) const;
    virtual bool may_contains_them(const QList<BrowserNode *> &,
                                   BooL & duplicable) const;
    bool may_contains_it(BrowserNode * bn) const;
    virtual void move(BrowserNode *, BrowserNode * after);
    bool enter_child_name(QString & r, const QString & msg, UmlCode type,
                          bool allow_spaces, bool allow_empty);
    bool enter_child_name(QString & r, const QString & msg, UmlCode type,
                          BrowserNodeList & nodes, BrowserNode ** old,
                          bool allow_spaces, bool allow_empty,
                          bool exiting = FALSE);
    bool wrong_child_name(const QString & s, UmlCode type,
                          bool allow_spaces, bool allow_empty) const;
    virtual bool allow_spaces() const;
    virtual bool allow_empty() const;
    virtual bool same_name(const QString & s, UmlCode type) const;
    void select_in_browser();
    void edit(const char *, const QStringList & default_stereotypes);
    void children(BrowserNodeList & nodes,
                  UmlCode kind1, UmlCode kind2 = UmlRelations) const;
    QList<BrowserNode*> children(QList<UmlCode>, bool includeDeleted = true) const;

    virtual QString full_name(bool rev = FALSE, bool itself = TRUE) const;
    QString fullname(bool rev) const;
    QString fullname(QString & s, bool rev) const;
    virtual void menu() = 0;
    virtual void apply_shortcut(QString s) = 0;
    /* Open a Diagram Window */
    virtual const QPixmap * pixmap(int) const{return 0;}
    virtual void open(bool force_edit);
    virtual void on_close();
    virtual UmlCode get_type() const = 0;
    virtual uint TypeID();
    virtual QString get_stype() const = 0;
    virtual int get_identifier() const = 0;
    virtual const char * help_topic() const;
    virtual void modified();
    bool modifiedp() {
        return is_modified;
    }
    void unmodified() {
        is_modified = FALSE;
    }
    bool modifiedp() const {
        return is_modified;
    };
    virtual void on_delete();
    virtual BasicData * get_data() const = 0;
    virtual QString drag_key() const;
    virtual QString drag_postfix() const;
    virtual void save(QTextStream &, bool ref, QString & warning) = 0;
    virtual void package_modified();
    virtual void get_classdiagramsettings(ClassDiagramSettings &) const;
    virtual void get_usecasediagramsettings(UseCaseDiagramSettings &) const;
    virtual void get_sequencediagramsettings(SequenceDiagramSettings &) const;
    virtual void get_collaborationdiagramsettings(CollaborationDiagramSettings &) const;
    virtual void get_objectdiagramsettings(ObjectDiagramSettings &) const;
    virtual void get_componentdiagramsettings(ComponentDiagramSettings &) const;
    virtual void get_deploymentdiagramsettings(DeploymentDiagramSettings &) const;
    virtual void get_statediagramsettings(StateDiagramSettings &) const;
    virtual void get_activitydiagramsettings(ActivityDiagramSettings &) const;
    virtual UmlColor get_color(UmlCode) const;
    virtual UmlVisibility get_visibility(UmlCode) const;
    virtual UmlVisibility get_visibility() const {return UmlDefaultVisibility;}
    //stringifies visibility of this node
    QString visibility_as_string() const;
    //converts visibility from string to enum representation
    static UmlVisibility encode_visibility(QString) ;

    virtual void package_settings(BooL & name_in_tab, ShowContextMode & show_context) const;
    virtual const QStringList & default_stereotypes(UmlCode, const BrowserNode *) const; // non class rel
    virtual BrowserNode * get_associated() const;
    virtual BasicData * add_relation(UmlCode, BrowserNode *);
    virtual QList<BrowserNode *> parents() const;
    BrowserNode * get_container(UmlCode) const;
    virtual BrowserNode * container(UmlCode) const; // container for class, state machine and activity
    virtual QString check_inherit(const BrowserNode * parent) const;
    virtual void write_id(ToolCom * com);
    virtual bool tool_cmd(ToolCom * com, const char * args);
    virtual bool api_compatible(unsigned v) const;
    virtual void member_cpp_def(const QString & prefix,
                                const QString & prefix_tmplop,
                                QString & s, bool templ) const;
    virtual void referenced_by(QList<BrowserNode *> &, bool ondelete = FALSE);
    virtual AType class_association() const;
    virtual const char * constraint() const;

    bool save_open_list(QTextStream &);
    void save(QTextStream &) const;
    static void save_progress_closed();
    virtual void init_save_counter();
    void read(char *& , char *& k, int id);
    static BrowserNode * read_any_ref(char *& , char *);
    static void save_stereotypes(QTextStream & st,
                                 QStringList relations_stereotypes[]);
    static void read_stereotypes(char *& ,
                                 QStringList relations_stereotypes[]);

    static bool toggle_show_stereotypes();
    virtual void iconChanged();
    virtual void paintCell(QPainter * p, const QColorGroup & cg, int column,
                           int width, int alignment);

    void unconsistent_fixed(const char * what, BrowserNode * newone);
    void unconsistent_removed(const char * what, BrowserNode * newone);
    static void signal_unconsistencies();

    static void pre_load();
    static void post_load(bool light);

    virtual void renumber(int phase);

    virtual void DragMoveEvent(QDragMoveEvent * e);
    virtual void DropEvent(QDropEvent * e);
    virtual void DragMoveInsideEvent(QDragMoveEvent * e);
    virtual void DropAfterEvent(QDropEvent * e, BrowserNode * after);

    // because of a Qt bug on windows
    static bool popupMenuActive() {
        return popup_menu_active;
    }
    static void setPopupMenuActive(bool y) {
        popup_menu_active = y;
    }

    BrowserNode* get_first_generatable_node();
    QList<BrowserNode*> get_generation_list();
};

inline QString BrowserNode::fullname(bool rev) const
{
    QString p = ((BrowserNode *) parent())->full_name(FALSE, FALSE);

    if (p.isEmpty())
        return QString((const char *) name);
    else if (rev)
        return name + (FullPathPrefix + p + FullPathPostfix);
    else
        return p + (FullPathDotDot + name);
}

inline QString BrowserNode::fullname(QString & s, bool rev) const
{
    QString p = ((BrowserNode *) parent())->full_name(FALSE, FALSE);

    if (p.isEmpty())
        return s;
    else if (rev)
        return s + (FullPathPrefix + p + FullPathPostfix);
    else
        return p + (FullPathDotDot + s);
}

// a sortable list of BrowserNode

class BrowserNodeList : public QList<BrowserNode *>
{
public:
    void search(BrowserNode * bn, UmlCode k, const QString & s,
                bool cs, bool even_deleted, bool for_name,
                bool for_stereotype);
    void search_ddb(BrowserNode * bn, UmlCode k, const QString & s,
                    bool cs, bool even_deleted);

    void names(QStringList & list) const;
    void full_names(QStringList & list) const;
    void full_defs(QStringList & list) const;
    void sort_it();
    void sort();

private:
    static bool lessThan(BrowserNode *a, BrowserNode *b);
};




#endif

