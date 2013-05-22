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

#ifndef BROWSER_ACTIVITYDIAGRAM_H
#define BROWSER_ACTIVITYDIAGRAM_H

#include "BrowserDiagram.h"
#include "Settings.h"
//Added by qt3to4:
#include <QPixmap>
#include <QDropEvent>
#include <QTextStream>
#include <Q3ValueList>

class QPixmap;
class ActivityDiagramWindow;
class SimpleData;

class BrowserActivityDiagram : public BrowserDiagram
{
    friend class StereotypesDialog;

protected:
    static QList<BrowserActivityDiagram *> imported;
    static Q3ValueList<int> imported_ids;
    static QStringList its_default_stereotypes;

    SimpleData * def;
    ActivityDiagramWindow * window;
    ActivityDiagramSettings settings;
    ActivityDiagramSettings * used_settings;
    UmlColor activity_color;
    UmlColor activityregion_color;
    UmlColor activitypartition_color;
    UmlColor activityaction_color;
    UmlColor parameterpin_color;
    UmlColor note_color;
    UmlColor fragment_color;
    UmlColor package_color;

    BrowserActivityDiagram(BrowserActivityDiagram * model, BrowserNode * p);
    BrowserActivityDiagram(int id);
    void make();
    void exec_menu_choice(int rank);

public:
    BrowserActivityDiagram(QString s, BrowserNode * p, int id = 0);
    virtual ~BrowserActivityDiagram();

    virtual void delete_it();
    virtual BrowserNode * duplicate(BrowserNode * p,
                                    QString name = QString());

    virtual const QPixmap * pixmap(int) const;
    virtual void draw_svg() const;
    bool api_compatible(unsigned v) const;

    virtual void menu();
    virtual void apply_shortcut(QString s);
    virtual void open(bool);
    virtual void on_close();
    virtual void read_session(char *& st);
    virtual UmlCode get_type() const;
    virtual QString get_stype() const;
    virtual int get_identifier() const;
    virtual const char * help_topic() const;
    virtual BasicData * get_data() const;
    virtual void set_name(const char * s);
    virtual void update_drawing_settings();
    virtual void get_activitydiagramsettings(ActivityDiagramSettings &) const;
    virtual void get_activitydrawingsettings(ActivityDrawingSettings &) const;
    virtual bool get_auto_label_position() const;
    virtual bool get_write_label_horizontally() const;
    virtual bool get_show_opaque_action_definition() const;
    virtual bool get_shadow() const;
    virtual bool get_draw_all_relations() const;
    virtual void dont_draw_all_relations();
    virtual bool get_show_stereotype_properties() const;
    virtual UmlColor get_color(UmlCode) const;
    virtual void package_settings(BooL & name_in_tab, ShowContextMode & show_context) const;
    virtual DrawingLanguage get_language() const;
    virtual bool tool_cmd(ToolCom * com, const char * args);
    virtual void save(QTextStream &, bool ref, QString & warning);
    static BrowserActivityDiagram * read(char *& , char *, BrowserNode *);
    static BrowserActivityDiagram * read_ref(char *& , char *);
    static BrowserNode * get_it(const char * k, int id);
    static BrowserActivityDiagram  * add_activity_diagram(BrowserNode * future_parent);
    void edit_settings();

    static const QStringList & default_stereotypes();
    static void read_stereotypes(char *& , char *& k);
    static void save_stereotypes(QTextStream &);

    virtual void renumber(int phase);
    static void open_all();
    static void import();

    static void compute_referenced_by(QList<BrowserNode *> & l, BrowserNode *,
                                      const char * kc, char const * kr);

    static QString drag_key(BrowserNode * p);
    virtual QString drag_key() const;
    virtual QString drag_postfix() const;
    virtual void DropAfterEvent(QDropEvent * e, BrowserNode * after);
};

#endif
