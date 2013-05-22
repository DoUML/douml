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

#include "BrowserDeploymentView.h"
#include "BrowserDeploymentDiagram.h"
#include "BrowserDeploymentNode.h"
#include "BrowserArtifact.h"
#include "SimpleData.h"
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

IdDict<BrowserDeploymentView> BrowserDeploymentView::all(__FILE__);
QStringList BrowserDeploymentView::its_default_stereotypes;	// unicode

BrowserDeploymentView::BrowserDeploymentView(QString s, BrowserNode * p, int id)
    : BrowserNode(s, p), Labeled<BrowserDeploymentView>(all, id)
{
    make();
    is_modified = (id == 0);
}

BrowserDeploymentView::BrowserDeploymentView(const BrowserDeploymentView * model,
        BrowserNode * p)
    : BrowserNode(model->name, p), Labeled<BrowserDeploymentView>(all, 0)
{
    def = new SimpleData(model->def);
    def->set_browser_node(this);
    comment = model->comment;
    deploymentdiagram_settings = model->deploymentdiagram_settings;
    package_color = model->package_color;
    deploymentnode_color = model->deploymentnode_color;
    component_color = model->component_color;
    artifact_color = model->artifact_color;
    note_color = model->note_color;
    fragment_color = model->fragment_color;
}

BrowserDeploymentView::BrowserDeploymentView(int id)
    : BrowserNode(), Labeled<BrowserDeploymentView>(all, id)
{
    make();
    is_modified = (id == 0);
}

BrowserDeploymentView::~BrowserDeploymentView()
{
    all.remove(get_ident());
    delete def;
}

BrowserNode * BrowserDeploymentView::duplicate(BrowserNode * p, QString name)
{
    BrowserDeploymentView * result = new BrowserDeploymentView(this, p);

    result->set_name((name.isEmpty()) ? get_name() : name);
    result->update_stereotype();

    return result;
}

void BrowserDeploymentView::make()
{
    def = new SimpleData;
    def->set_browser_node(this);

    package_color = UmlDefaultColor;
    fragment_color = UmlDefaultColor;
    deploymentnode_color = UmlDefaultColor;
    component_color = UmlDefaultColor;
    artifact_color = UmlDefaultColor;
    note_color = UmlDefaultColor;
}

BrowserDeploymentView * BrowserDeploymentView::add_deployment_view(BrowserNode * future_parent)
{
    QString name;

    if (future_parent->enter_child_name(name, TR("enter deployment view's name : "),
                                        UmlDeploymentView, TRUE, FALSE))
        return new BrowserDeploymentView(name, future_parent);
    else
        return 0;
}

void BrowserDeploymentView::clear(bool old)
{
    all.clear(old);
    BrowserArtifact::clear(old);
    BrowserDeploymentNode::clear(old);
}

void BrowserDeploymentView::update_idmax_for_root()
{
    all.update_idmax_for_root();
    BrowserArtifact::update_idmax_for_root();
    BrowserDeploymentNode::update_idmax_for_root();
}

void BrowserDeploymentView::prepare_update_lib() const
{
    all.memo_id_oid(get_ident(), original_id);

    for (Q3ListViewItem * child = firstChild();
         child != 0;
         child = child->nextSibling())
        ((BrowserNode *) child)->prepare_update_lib();
}

void BrowserDeploymentView::renumber(int phase)
{
    if (phase != -1)
        new_ident(phase, all);

    BrowserNode::renumber(phase);
}

const QPixmap * BrowserDeploymentView::pixmap(int) const
{
    if (deletedp())
        return DeletedDeploymentViewIcon;

    const QPixmap * px = ProfiledStereotypes::browserPixmap(def->get_stereotype());

    return (px != 0) ? px : DeploymentViewIcon;
}

QString BrowserDeploymentView::full_name(bool rev, bool itself) const
{
    QString p = ((BrowserNode *) parent())->full_name(FALSE, FALSE);

    if (!itself)
        return p;
    else if (p.isEmpty())
        return name;
    else if (rev)
        return name + (FullPathPrefix + p + FullPathPostfix);
    else
        return p + (FullPathDotDot + name);
}

void BrowserDeploymentView::menu()
{
    Q3PopupMenu m(0);
    Q3PopupMenu subm(0);
    Q3PopupMenu roundtripm(0);
    Q3PopupMenu roundtripbodym(0);
    Q3PopupMenu toolm(0);

    MenuFactory::createTitle(m, def->definition(FALSE, TRUE));
    m.insertSeparator();

    if (!deletedp()) {
        if (!is_read_only && (edition_number == 0)) {
            m.setWhatsThis(m.insertItem(TR("New deployment diagram"), 0),
                           TR("to add a <i>deployment diagram</i>"));
            m.setWhatsThis(m.insertItem(TR("New node"), 1),
                           TR("to add a <i>node</i>"));
            m.setWhatsThis(m.insertItem(TR("New artifact"), 2),
                           TR("to add an <i>artifact</i>"));
            m.insertSeparator();
        }

        if (!is_edited) {
            m.setWhatsThis(m.insertItem(TR("Edit"), 3),
                           TR("to edit the <i>deployment view</i>"));

            if (!is_read_only) {
                m.insertSeparator();
                //m.setWhatsThis(m.insertItem("Edit node settings", 4),
                //		   "to set the sub node's settings");
                m.setWhatsThis(m.insertItem(TR("Edit drawing settings"), 5),
                               TR("to set how the sub <i>deployment diagrams</i>'s items must be drawn"));

                if (edition_number == 0) {
                    m.insertSeparator();
                    m.setWhatsThis(m.insertItem(TR("Delete"), 6),
                                   TR("to delete the <i>deployment view</i> and its sub items. \
Note that you can undelete them after"));
                }
            }
        }

        mark_menu(m, TR("the deployment view"), 90);
        ProfiledStereotypes::menu(m, this, 99990);

        bool cpp = GenerationSettings::cpp_get_default_defs();
        bool java = GenerationSettings::java_get_default_defs();
        bool php = GenerationSettings::php_get_default_defs();
        bool python = GenerationSettings::python_get_default_defs();
        bool idl = GenerationSettings::idl_get_default_defs();

        if (cpp || java || php || python || idl) {
            m.insertSeparator();
            m.insertItem(TR("Generate"), &subm);

            if (cpp) {
                subm.insertItem("C++", 10);

                if ((edition_number == 0) && !is_read_only)
                    roundtripm.insertItem("C++", 41);
            }

            if (java) {
                subm.insertItem("Java", 11);

                if ((edition_number == 0) && !is_read_only)
                    roundtripm.insertItem("Java", 42);
            }

            if (php)
                subm.insertItem("Php", 12);

            if (python)
                subm.insertItem("Python", 14);

            if (idl)
                subm.insertItem("Idl", 13);

            if (roundtripm.count() != 0)
                m.insertItem(TR("Roundtrip"), &roundtripm);
        }

        if (edition_number == 0) {
            if (preserve_bodies() && (cpp || java || php || python)) {
                m.insertItem(TR("Roundtrip body"), &roundtripbodym);

                if (cpp)
                    roundtripbodym.insertItem("C++", 30);

                if (java)
                    roundtripbodym.insertItem("Java", 31);

                if (php)
                    roundtripbodym.insertItem("Php", 32);

                if (python)
                    roundtripbodym.insertItem("Python", 33);
            }

            if (Tool::menu_insert(&toolm, get_type(), 100)) {
                m.insertSeparator();
                m.insertItem(TR("Tool"), &toolm);
            }
        }
    }
    else if (!is_read_only && (edition_number == 0)) {
        m.setWhatsThis(m.insertItem(TR("Undelete"), 7),
                       TR("undelete the <i>deployment view</i>. \
Do not undelete its sub items"));
        m.setWhatsThis(m.insertItem(TR("Undelete recursively"), 8),
                       TR("undelete the <i>deployment view</i> and its sub items"));
    }

    exec_menu_choice(m.exec(QCursor::pos()));
}

void BrowserDeploymentView::exec_menu_choice(int rank)
{
    switch (rank) {
    case 0: {
        BrowserDeploymentDiagram * d =
            BrowserDeploymentDiagram::add_deployment_diagram(this);

        if (d != 0)
            d->select_in_browser();
    }
    break;
    break;

    case 1: {
        BrowserDeploymentNode * dn = BrowserDeploymentNode::add_deploymentnode(this);

        if (dn != 0)
            dn->select_in_browser();
    }
    break;

    case 2: {
        BrowserArtifact * dn = BrowserArtifact::add_artifact(this);

        if (dn != 0)
            dn->select_in_browser();
    }
    break;

    case 3:
        edit(TR("Deployment view"), its_default_stereotypes);
        return;

    case 4:
        //if (! node_settings.edit(UmlDeploymentView))
        //  return;
        break;

    case 5:
        for (;;) {
            StateSpecVector st;
            ColorSpecVector co(6);

            deploymentdiagram_settings.complete(st, FALSE);

            co[0].set(TR("node color"), &deploymentnode_color);
            co[1].set(TR("artifact color"), &artifact_color);
            co[2].set(TR("component color"), &component_color);
            co[3].set(TR("note color"), &note_color);
            co[4].set(TR("package color"), &package_color);
            co[5].set(TR("fragment color"), &fragment_color);

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

    case 6:
        delete_it();
        break;

    case 7:
        undelete(FALSE);
        break;

    case 8:
        undelete(TRUE);
        break;

    case 10: {
        bool preserve = preserve_bodies();

        ToolCom::run((verbose_generation())
                     ? ((preserve) ? "cpp_generator -v -p" : "cpp_generator -v")
                         : ((preserve) ? "cpp_generator -p" : "cpp_generator"),
                         this);
    }

    return;

    case 11: {
        bool preserve = preserve_bodies();

        ToolCom::run((verbose_generation())
                     ? ((preserve) ? "java_generator -v -p" : "java_generator -v")
                         : ((preserve) ? "java_generator -p" : "java_generator"),
                         this);
    }

    return;

    case 12: {
        bool preserve = preserve_bodies();

        ToolCom::run((verbose_generation())
                     ? ((preserve) ? "php_generator -v -p" : "php_generator -v")
                         : ((preserve) ? "php_generator -p" : "php_generator"),
                         this);
    }

    return;

    case 14: {
        bool preserve = preserve_bodies();

        ToolCom::run((verbose_generation())
                     ? ((preserve) ? "python_generator -v -p" : "python_generator -v")
                         : ((preserve) ? "python_generator -p" : "python_generator"),
                         this);
    }

    return;

    case 13:
        ToolCom::run((verbose_generation()) ? "idl_generator -v" : "idl_generator", this);
        return;

    case 30:
        ToolCom::run((verbose_generation()) ? "roundtrip_body -v c++" : "roundtrip_body c++", this);
        return;

    case 31:
        ToolCom::run((verbose_generation()) ? "roundtrip_body -v java" : "roundtrip_body java", this);
        return;

    case 32:
        ToolCom::run((verbose_generation()) ? "roundtrip_body -v php" : "roundtrip_body php", this);
        return;

    case 33:
        ToolCom::run((verbose_generation()) ? "roundtrip_body -v python" : "roundtrip_body python", this);
        return;

    case 41:
        ToolCom::run("cpp_roundtrip", this);
        return;

    case 42:
        ToolCom::run("java_roundtrip", this);
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

void BrowserDeploymentView::apply_shortcut(QString s)
{
    int choice = -1;

    if (!deletedp()) {
        if (!is_read_only && (edition_number == 0)) {
            if (s == "New deployment diagram")
                choice = 0;
            else if (s == "New node")
                choice = 1;
            else if (s == "New artifact")
                choice = 2;
        }

        if (choice == -1) {
            if (!is_edited) {
                if (s == "Edit")
                    choice = 3;
                else if (!is_read_only) {
                    //m.setWhatsThis(m.insertItem("Edit node settings", 4),
                    //		   "to set the sub node's settings");
                    if (s == "Edit drawing settings")
                        choice = 5;

                    if (edition_number == 0) {
                        if (s == "Delete")
                            choice = 6;
                    }
                }
            }

            if (choice == -1) {
                if (preserve_bodies()) {
                    if (s == "Roundtrip C++ operation body")
                        choice = 30;
                    else if (s == "Roundtrip Java operation body")
                        choice = 31;
                    else if (s == "Roundtrip Php operation body")
                        choice = 32;
                    else if (s == "Roundtrip Python operation body")
                        choice = 33;
                }

                if (choice == -1)
                    mark_shortcut(s, choice, 90);

                if (choice == -1) {
                    if (edition_number == 0)
                        Tool::shortcut(s, choice, get_type(), 100);

                    if (s == "Generate C++")
                        choice = 10;
                    else if (s == "Generate Java")
                        choice = 11;
                    else if (s == "Generate Php")
                        choice = 12;
                    else if (s == "Generate Python")
                        choice = 14;
                    else if (s == "Generate Idl")
                        choice = 13;
                }
            }
        }
    }
    else if (!is_read_only && (edition_number == 0)) {
        if (s == "Undelete")
            choice = 7;
        else if (s == "Undelete recursively")
            choice = 8;
    }

    exec_menu_choice(choice);
}

void BrowserDeploymentView::open(bool)
{
    if (!is_edited)
        edit(TR("Deployment view"), its_default_stereotypes);
}

UmlCode BrowserDeploymentView::get_type() const
{
    return UmlDeploymentView;
}

QString BrowserDeploymentView::get_stype() const
{
    return TR("deployment view");
}

int BrowserDeploymentView::get_identifier() const
{
    return get_ident();
}

const char * BrowserDeploymentView::help_topic() const
{
    return "deploymentview";
}

BasicData * BrowserDeploymentView::get_data() const
{
    return def;
}

void BrowserDeploymentView::get_deploymentdiagramsettings(DeploymentDiagramSettings & r) const
{
    if (! deploymentdiagram_settings.complete(r))
        ((BrowserNode *) parent())->get_deploymentdiagramsettings(r);
}

UmlColor BrowserDeploymentView::get_color(UmlCode who) const
{
    UmlColor c;

    switch (who) {
    case UmlComponent:
        c = component_color;
        break;

    case UmlArtifact:
        c = artifact_color;
        break;

    case UmlDeploymentNode:
        c = deploymentnode_color;
        break;

    case UmlNote:
        c = note_color;
        break;

    case UmlFragment:
        c = fragment_color;
        break;

    default:
        c = package_color;
    }

    return (c != UmlDefaultColor)
           ? c
           : ((BrowserNode *) parent())->get_color(who);
}

BrowserNodeList & BrowserDeploymentView::instances(BrowserNodeList & result)
{
    IdIterator<BrowserDeploymentView> it(all);

    while (it.current() != 0) {
        if (!it.current()->deletedp())
            result.append(it.current());

        ++it;
    }

    result.sort_it();

    return result;
}

bool BrowserDeploymentView::tool_cmd(ToolCom * com, const char * args)
{
    switch ((unsigned char) args[-1]) {
    case createCmd: {
        bool ok = TRUE;

        if (is_read_only && !root_permission())
            ok = FALSE;
        else {
            switch (com->get_kind(args)) {
            case UmlDeploymentDiagram:
                if (wrong_child_name(args, UmlDeploymentDiagram, TRUE, FALSE))
                    ok = FALSE;
                else
                    (new BrowserDeploymentDiagram(args, this))->write_id(com);

                break;

            case UmlDeploymentNode:
                if (wrong_child_name(args, UmlDeploymentNode, FALSE, FALSE))
                    ok = FALSE;
                else
                    (new BrowserDeploymentNode(args, this))->write_id(com);

                break;

            case UmlArtifact:
                if (wrong_child_name(args, UmlArtifact, FALSE, FALSE))
                    ok = FALSE;
                else
                    (new BrowserArtifact(args, this))->write_id(com);

                break;

            default:
                ok = FALSE;
            }
        }

        if (! ok)
            com->write_id(0);
        else {
            package_modified();
        }

        return TRUE;
    }

    default:
        return (def->tool_cmd(com, args, this, comment) ||
                BrowserNode::tool_cmd(com, args));
    }
}

void BrowserDeploymentView::DragMoveEvent(QDragMoveEvent * e)
{
    if (UmlDrag::canDecode(e, UmlArtifact) ||
        UmlDrag::canDecode(e, UmlDeploymentNode) ||
        UmlDrag::canDecode(e, UmlDeploymentDiagram)) {
        if (!is_read_only)
            e->accept();
        else
            e->ignore();
    }
    else
        ((BrowserNode *) parent())->DragMoveInsideEvent(e);
}

void BrowserDeploymentView::DragMoveInsideEvent(QDragMoveEvent * e)
{
    if (!is_read_only &&
        (UmlDrag::canDecode(e, UmlArtifact) ||
         UmlDrag::canDecode(e, UmlDeploymentNode) ||
         UmlDrag::canDecode(e, UmlDeploymentDiagram)))
        e->accept();
    else
        e->ignore();
}

bool BrowserDeploymentView::may_contains_them(const QList<BrowserNode *> & l,
        BooL & duplicable) const
{
    foreach (BrowserNode *node, l) {
        switch (node->get_type()) {
        case UmlArtifact:
            duplicable = FALSE;

            // no break
        case UmlDeploymentNode:
        case UmlDeploymentDiagram:
            if (! may_contains(node, FALSE))
                return FALSE;

            break;

        default:
            return FALSE;
        }

        duplicable = may_contains_it(node);
    }

    return TRUE;
}

void BrowserDeploymentView::DropEvent(QDropEvent * e)
{
    DropAfterEvent(e, 0);
}

void BrowserDeploymentView::DropAfterEvent(QDropEvent * e, BrowserNode * after)
{
    BrowserNode * bn;

    if ((((bn = UmlDrag::decode(e, UmlArtifact)) != 0) ||
         ((bn = UmlDrag::decode(e, UmlDeploymentNode)) != 0) ||
         (((bn = UmlDrag::decode(e, UmlDeploymentDiagram)) != 0))) &&
        (bn != after) && (bn != this)) {
        if (may_contains(bn, FALSE)) {
            BrowserNode * old_parent = (BrowserNode *) bn->parent();

            if (after)
                bn->moveItem(after);
            else {
                bn->parent()->takeItem(bn);
                insertItem(bn);
            }

            package_modified();

            if (old_parent != this) {
                old_parent->package_modified();
                bn->modified();
            }
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

void BrowserDeploymentView::save_stereotypes(QTextStream & st)
{
    nl_indent(st);
    st << "deploymentview_stereotypes ";
    save_unicode_string_list(its_default_stereotypes, st);
}

void BrowserDeploymentView::read_stereotypes(char *& st, char *& k)
{
    if (!strcmp(k, "deploymentview_stereotypes")) {
        read_unicode_string_list(its_default_stereotypes, st);
        k = read_keyword(st);
    }
}

void BrowserDeploymentView::save(QTextStream & st, bool ref, QString & warning)
{
    if (ref)
        st << "deploymentview_ref " << get_ident() << " // " << get_name();
    else {
        nl_indent(st);
        st << "deploymentview " << get_ident() << " ";
        save_string(name, st);
        indent(+1);

        def->save(st, warning);
        nl_indent(st);
        st << "//deployment diagram settings";
        deploymentdiagram_settings.save(st);
        //nl_indent(st);
        //st << "//deployment node settings";
        //deploymentnode_settings.save(st);

        BooL nl = FALSE;

        save_color(st, "deploymentnode_color", deploymentnode_color, nl);
        save_color(st, "artifact_color", artifact_color, nl);
        save_color(st, "component_color", component_color, nl);
        save_color(st, "note_color", note_color, nl);
        save_color(st, "package_color", package_color, nl);
        save_color(st, "fragment_color", fragment_color, nl);

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

BrowserDeploymentView * BrowserDeploymentView::read_ref(char *& st, const char * k)
{
    if (strcmp(k, "deploymentview_ref") &&
        ((read_file_format() >= 20) || strcmp(k, "componentview_ref")))
        wrong_keyword(k, "deploymentview_ref");

    int id = read_id(st);
    BrowserDeploymentView * result = all[id];

    return (result == 0)
           ? new BrowserDeploymentView(id)
           : result;
}

BrowserDeploymentView * BrowserDeploymentView::read(char *& st, char * k,
        BrowserNode * parent,
        bool /*recursive*/)
{
    bool from_componentview =
        // component -> artifact
        (read_file_format() < 20) && !strcmp(k, "componentview");

    if (!strcmp(k, "deploymentview") || from_componentview) {
        int id = read_id(st);
        QString s = read_string(st);
        BrowserDeploymentView * r = all[id];

        if (r == 0)
            r = new BrowserDeploymentView(s, parent, id);
        else if (r->is_defined) {
            BrowserDeploymentView * already_exist = r;

            r = new BrowserDeploymentView(s, parent, id);

            already_exist->must_change_id(all);
            already_exist->unconsistent_fixed("deployment view", r);
        }
        else {
            r->set_parent(parent);
            r->set_name(s);
        }

        r->is_defined = TRUE;

        r->is_read_only = (!in_import() && read_only_file()) ||
                          ((user_id() != 0) && r->is_api_base());

        k = read_keyword(st);

        r->def->read(st, k);					// updates k

        if (from_componentview || (read_file_format() >= 20))
            r->deploymentdiagram_settings.read(st, k);		// updates k

        //r->deploymentnode_settings.read(st, k);			// updates k
        read_color(st, "deploymentnode_color", r->deploymentnode_color, k);	// updates k
        read_color(st, "artifact_color", r->component_color, k);	// updates k
        read_color(st, "component_color", r->component_color, k);	// updates k
        read_color(st, "note_color", r->note_color, k);		// updates k
        read_color(st, "package_color", r->package_color, k);	// updates k
        read_color(st, "fragment_color", r->fragment_color, k);	// updates k
        r->BrowserNode::read(st, k, id);				// updates k

        if (strcmp(k, "end")) {
            while (BrowserDeploymentDiagram::read(st, k, r) ||
                   BrowserDeploymentNode::read(st, k, r) ||
                   BrowserArtifact::read(st, k, r))
                k = read_keyword(st);

            if (strcmp(k, "end"))
                wrong_keyword(k, "end");
        }

        return r;
    }
    else
        return 0;
}

BrowserNode * BrowserDeploymentView::get_it(const char * k, int id)
{
    if (!strcmp(k, "deploymentview_ref"))
        return all[id];

    BrowserNode * r;

    if (((r = BrowserDeploymentDiagram::get_it(k, id)) == 0) &&
        ((r = BrowserDeploymentNode::get_it(k, id)) == 0))
        r = BrowserArtifact::get_it(k, id);

    return r;
}
