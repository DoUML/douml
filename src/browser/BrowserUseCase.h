//Added by qt3to4:
#include <QTextStream>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QPixmap>
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

#ifndef BROWSER_USECASE_H
#define BROWSER_USECASE_H

class QPixmap;
class StereotypesDialog;
class BrowserUseCaseDiagram;
class UseCaseData;

#include "BrowserNode.h"
#include "Labeled.h"
#include "Settings.h"

class BrowserUseCase : public BrowserNode, public Labeled<BrowserUseCase>
{
    friend class StereotypesDialog;

protected:
    static IdDict<BrowserUseCase> all;
    static QStringList its_default_stereotypes;
    static QStringList relations_default_stereotypes[UmlRelations];

    UseCaseData * def;
    BrowserUseCaseDiagram * associated_diagram;
    UseCaseDiagramSettings usecasediagram_settings;
    SequenceDiagramSettings sequencediagram_settings;
    CollaborationDiagramSettings collaborationdiagram_settings;
    ClassDiagramSettings classdiagram_settings;
    ObjectDiagramSettings objectdiagram_settings;
    StateDiagramSettings statediagram_settings;
    ActivityDiagramSettings activitydiagram_settings;
    // note : does not have class settings because classes defined under
    //	      a use case cannot be generated
    UmlColor note_color;
    UmlColor fragment_color;
    UmlColor subject_color;
    UmlColor duration_color;
    UmlColor continuation_color;
    UmlColor usecase_color;
    UmlColor package_color;
    UmlColor state_color;
    UmlColor stateaction_color;
    UmlColor activity_color;
    UmlColor activityregion_color;
    UmlColor activitypartition_color;
    UmlColor activityaction_color;
    UmlColor parameterpin_color;
    UmlColor class_color;

    BrowserUseCase(int id);
    void make();
    void exec_menu_choice(int rank);

public:
    BrowserUseCase(QString s, BrowserNode * p, int id = 0);
    BrowserUseCase(const BrowserUseCase * model, BrowserNode * p);
    virtual ~BrowserUseCase();

    virtual BrowserNode * get_associated() const;
    void set_associated_diagram(BrowserUseCaseDiagram *, bool on_read = FALSE);

    virtual BrowserNode * duplicate(BrowserNode * p,
                                    QString name = QString());
    virtual QString full_name(bool rev = FALSE, bool itself = TRUE) const;
    virtual void menu();
    virtual void apply_shortcut(QString s);
    virtual void open(bool force_edit);
    virtual UmlCode get_type() const;
    virtual QString get_stype() const;
    virtual int get_identifier() const;
    virtual const char * help_topic() const;
    virtual void modified();
    virtual bool may_contains_them(const QList<BrowserNode *> &,
                                   BooL & duplicable) const;
    virtual BasicData * get_data() const;
    virtual void get_usecasediagramsettings(UseCaseDiagramSettings &) const;
    virtual void get_sequencediagramsettings(SequenceDiagramSettings &) const;
    virtual void get_collaborationdiagramsettings(CollaborationDiagramSettings &) const;
    virtual void get_classdiagramsettings(ClassDiagramSettings &) const;
    virtual void get_objectdiagramsettings(ObjectDiagramSettings &) const;
    virtual void get_statediagramsettings(StateDiagramSettings &) const;
    virtual void get_activitydiagramsettings(ActivityDiagramSettings &) const;
    virtual UmlColor get_color(UmlCode) const;
    virtual const QStringList & default_stereotypes(UmlCode, const BrowserNode *) const; // non class rel
    virtual void on_delete();
    virtual bool tool_cmd(ToolCom * com, const char * args);
    virtual void save(QTextStream &, bool ref, QString & warning);
    static BrowserUseCase * read_ref(char *&);
    static BrowserUseCase * read(char *& , char *, BrowserNode *);
    static BrowserNode * get_it(const char * k, int id);

    static BrowserNodeList & instances(BrowserNodeList &);
    static BrowserUseCase * add_use_case(BrowserNode * future_parent);
    static BrowserNode * get_use_case(BrowserNode * future_parent);

    static void init();
    static const QStringList & default_stereotypes();
    static void read_stereotypes(char *& , char *& k);
    static void save_stereotypes(QTextStream &);

    virtual void referenced_by(QList<BrowserNode *> & l, bool ondelete);

    static void clear(bool old);
    static void update_idmax_for_root();
    virtual void renumber(int phase);
    virtual void prepare_update_lib() const;

    virtual const QPixmap * pixmap(int) const;

    virtual void DragMoveEvent(QDragMoveEvent * e);
    virtual void DropEvent(QDropEvent * e);
    virtual void DragMoveInsideEvent(QDragMoveEvent * e);
    virtual void DropAfterEvent(QDropEvent * e, BrowserNode * after);
};

#endif

