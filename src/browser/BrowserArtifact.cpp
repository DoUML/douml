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
#include <qdir.h>
//Added by qt3to4:
#include <QTextStream>
#include <Q3ValueList>
#include <QPixmap>
#include <QDragMoveEvent>
#include <QDropEvent>

#include "BrowserArtifact.h"
#include "ArtifactData.h"
#include "BrowserDeploymentDiagram.h"
#include "BrowserSimpleRelation.h"
#include "SimpleRelationData.h"
#include "BrowserClass.h"
#include "ClassData.h"
#include "PackageData.h"
#include "UmlPixmap.h"
#include "myio.h"
#include "ToolCom.h"
#include "Tool.h"
#include "ui/menufactory.h"
#include "ClassListDialog.h"
#include "GenerationSettings.h"
#include "SourceDialog.h"
#include "ReferenceDialog.h"
#include "UmlDrag.h"
#include "DialogUtil.h"
#include "BrowserView.h"
#include "ProfiledStereotypes.h"
#include "mu.h"
#include "translate.h"

IdDict<BrowserArtifact> BrowserArtifact::all(257, __FILE__);
QStringList BrowserArtifact::its_default_stereotypes;	// unicode
QStringList BrowserArtifact::relation_default_stereotypes;	// unicode

BrowserArtifact::BrowserArtifact(QString s, BrowserNode * p, int id)
    : BrowserNode(s, p), Labeled<BrowserArtifact>(all, id),
      cpp_h_edited(FALSE), cpp_src_edited(FALSE),
      java_edited(FALSE), php_edited(FALSE),
      python_edited(FALSE), idl_edited(FALSE)
{
    def = new ArtifactData;
    def->set_browser_node(this);

    associated_diagram = 0;
}

BrowserArtifact::BrowserArtifact(const BrowserArtifact * model,
                                 BrowserNode * p)
    : BrowserNode(model->name, p), Labeled<BrowserArtifact>(all, 0),
      cpp_h_edited(FALSE), cpp_src_edited(FALSE),
      java_edited(FALSE), php_edited(FALSE),
      python_edited(FALSE), idl_edited(FALSE)
{
    def = new ArtifactData(model->def, this);
    comment = model->comment;
    associated_diagram = 0;
}

BrowserArtifact::BrowserArtifact(int id)
    : BrowserNode(), Labeled<BrowserArtifact>(all, id),
      cpp_h_edited(FALSE), cpp_src_edited(FALSE),
      java_edited(FALSE), php_edited(FALSE),
      python_edited(FALSE), idl_edited(FALSE)
{
    // not yet read
    def = new ArtifactData;
    def->set_browser_node(this);

    associated_diagram = 0;
}

BrowserArtifact::~BrowserArtifact()
{
    all.remove(get_ident());
    delete def;
}

void BrowserArtifact::delete_it()
{
    BrowserNode::delete_it();

    Q3ValueList<BrowserClass *>::Iterator it;

    for (it = associated_classes.begin(); it != associated_classes.end(); ++it)
        (*it)->set_associated_artifact(0, FALSE);

    associated_classes.clear();
}

BrowserNode * BrowserArtifact::duplicate(BrowserNode * p, QString name)
{
    BrowserArtifact * result = new BrowserArtifact(this, p);

    result->set_name((name.isEmpty()) ? get_name() : name);
    result->update_stereotype();
    return result;
}

void BrowserArtifact::clear(bool old)
{
    all.clear(old);
}

void BrowserArtifact::update_idmax_for_root()
{
    all.update_idmax_for_root();
}

void BrowserArtifact::prepare_update_lib() const
{
    all.memo_id_oid(get_ident(), original_id);

    for (Q3ListViewItem * child = firstChild();
         child != 0;
         child = child->nextSibling())
        ((BrowserNode *) child)->prepare_update_lib();
}

void BrowserArtifact::referenced_by(QList<BrowserNode *> & l, bool ondelete)
{
    BrowserNode::referenced_by(l, ondelete);
    compute_referenced_by(l, this);

    if (! ondelete)
        BrowserDeploymentDiagram::compute_referenced_by(l, this, "artifactcanvas", "artifact_ref");
}

void BrowserArtifact::compute_referenced_by(QList<BrowserNode *> & l,
        BrowserClass * target)
{
    IdIterator<BrowserArtifact> it(all);
    BrowserArtifact * a;

    while ((a = it.current()) != 0) {
        if (!a->deletedp() &&
            (a->associated_classes.findIndex(target) != -1))
            l.append(a);

        ++it;
    }
}

void BrowserArtifact::compute_referenced_by(QList<BrowserNode *> & l,
        BrowserArtifact * target)
{
    IdIterator<BrowserArtifact> it(all);

    while (it.current()) {
        if (!it.current()->deletedp() &&
            (it.current()->def->get_associated() != 0) &&
            (it.current()->def->get_associated()->find(target) != 0))
            l.append(it.current());

        ++it;
    }
}

void BrowserArtifact::renumber(int phase)
{
    if (phase != -1)
        new_ident(phase, all);
}

const QPixmap * BrowserArtifact::pixmap(int) const
{
    if (deletedp())
        return DeletedArtifactIcon;

    const QPixmap * px = ProfiledStereotypes::browserPixmap(def->get_stereotype());

    return (px != 0) ? px : ArtifactIcon;
}

void BrowserArtifact::iconChanged()
{
    repaint();
    def->modified();
}

QString BrowserArtifact::full_name(bool rev, bool) const
{
    return fullname(rev);
}

QString BrowserArtifact::get_path(QString path, QString root,
                                  const char * ext) const
{
    if (QDir::isRelativePath(root))
        root = BrowserView::get_dir().filePath(root);

    QDir d_root(root);

    if (path.isEmpty())
        path = root;
    else if (QDir::isRelativePath(path))
        path = d_root.filePath(path);

    if (path.isEmpty() || QDir::isRelativePath(path))
        return QString();

    QDir d(path);

    path = (ext != 0) ? d.filePath(name + "." + ext) : d.filePath(name);

    return (QFile::exists(path)) ? path : QString();
}

void BrowserArtifact::get_paths(QString & cpp_h_path, QString & cpp_src_path,
                                QString & java_path, QString & php_path,
                                QString & python_path, QString & idl_path) const
{
    bool a_text = !strcmp(def->get_stereotype(), "text");

    if (a_text || !strcmp(def->get_stereotype(), "source")) {
        PackageData * pd = (PackageData *)
                           ((BrowserNode *) parent()->parent())->get_data();

        if (!a_text && (def->get_cpp_h()[0] != 0))
            cpp_h_path = get_path((const char *) pd->get_cpp_h_dir(),
                                  GenerationSettings::get_cpp_root_dir(),
                                  GenerationSettings::get_cpp_h_extension());

        if (def->get_cpp_src()[0] != 0)
            cpp_src_path = get_path((const char *) pd->get_cpp_src_dir(),
                                    GenerationSettings::get_cpp_root_dir(),
                                    (a_text) ? (const char *) 0
                                    : (const char *) GenerationSettings::get_cpp_src_extension());

        if (def->get_java_src()[0] != 0)
            java_path = get_path((const char *) pd->get_java_dir(),
                                 GenerationSettings::get_java_root_dir(),
                                 (a_text) ? (const char *) 0
                                 : (const char *) GenerationSettings::get_java_extension());

        if (def->get_php_src()[0] != 0)
            php_path = get_path((const char *) pd->get_php_dir(),
                                GenerationSettings::get_php_root_dir(),
                                (a_text) ? (const char *) 0
                                : (const char *) GenerationSettings::get_php_extension());

        if (def->get_python_src()[0] != 0)
            python_path = get_path((const char *) pd->get_python_dir(),
                                   GenerationSettings::get_python_root_dir(),
                                   (a_text) ? (const char *) 0
                                   : (const char *) GenerationSettings::get_python_extension());

        if (def->get_idl_src()[0] != 0)
            idl_path = get_path((const char *) pd->get_idl_dir(),
                                GenerationSettings::get_idl_root_dir(),
                                (a_text) ? (const char *) 0
                                : (const char *) GenerationSettings::get_idl_extension());
    }
}

// just check if the inheritance already exist
QString BrowserArtifact::check_inherit(const BrowserNode * new_parent) const
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

void BrowserArtifact::menu()
{
    Q3PopupMenu m(0, name);
    Q3PopupMenu clsubm(0);
    Q3PopupMenu gensubm(0);
    Q3PopupMenu editsubm(0);
    Q3PopupMenu roundtripbodysubm(0);
    Q3PopupMenu roundtripsubm(0);
    Q3PopupMenu toolm(0);
    QString cpp_h_path;
    QString cpp_src_path;
    QString java_path;
    QString php_path;
    QString python_path;
    QString idl_path;

    MenuFactory::createTitle(m, def->definition(FALSE, TRUE));
    m.insertSeparator();

    if (!deletedp()) {
        if (!is_edited) {
            m.setWhatsThis(m.insertItem(TR("Edit"), 0),
                           TR("to edit the <i>artifact</i>, \
a double click with the left mouse button does the same thing"));

            if (!is_read_only && (edition_number == 0)) {
                m.insertSeparator();
                m.setWhatsThis(m.insertItem(TR("Delete"), 1),
                               TR("to delete the <i>artifact</i>. \
Note that you can undelete it after"));
            }
        }

        bool a_text = !strcmp(def->get_stereotype(), "text");
        bool cpp = GenerationSettings::cpp_get_default_defs();
        bool java = GenerationSettings::java_get_default_defs();
        bool php = GenerationSettings::php_get_default_defs();
        bool python = GenerationSettings::python_get_default_defs();
        bool idl = GenerationSettings::idl_get_default_defs();

        if ((a_text || !strcmp(def->get_stereotype(), "source")) &&
            (cpp || java || php || python || idl)) {
            m.insertSeparator();
            m.insertItem(TR("Generate"), &gensubm);

            if (cpp)
                gensubm.insertItem("C++", 10);

            if (java)
                gensubm.insertItem("Java", 11);

            if (php)
                gensubm.insertItem("Php", 22);

            if (python)
                gensubm.insertItem("Python", 25);

            if (idl)
                gensubm.insertItem("Idl", 12);

            if (!a_text && preserve_bodies() && (cpp || java || php || python)) {
                m.insertItem(TR("Roundtrip body"), &roundtripbodysubm);

                if (cpp)
                    roundtripbodysubm.insertItem("C++", 30);

                if (java)
                    roundtripbodysubm.insertItem("Java", 31);

                if (php)
                    roundtripbodysubm.insertItem("Php", 32);

                if (python)
                    roundtripbodysubm.insertItem("Python", 33);
            }

            get_paths(cpp_h_path, cpp_src_path, java_path, php_path, python_path, idl_path);

            if (cpp && !a_text && !cpp_h_edited && !cpp_h_path.isEmpty()) {
                editsubm.insertItem(TR("C++ header file"), 14);

                if (! is_read_only) roundtripsubm.insertItem("C++", 15);
            }

            if (cpp && !cpp_src_edited && !cpp_src_path.isEmpty()) {
                editsubm.insertItem(TR("C++ source file"), 16);
            }

            if (java && !java_edited && !java_path.isEmpty()) {
                editsubm.insertItem(TR("Java source file"), 18);

                if (! is_read_only) roundtripsubm.insertItem("Java", 19);
            }

            if (php && !php_edited && !php_path.isEmpty()) {
                editsubm.insertItem(TR("Php source file"), 23);
                //if (! is_read_only) roundtripsubm.insertItem("Php", 24);
            }

            if (python && !python_edited && !python_path.isEmpty()) {
                editsubm.insertItem(TR("Python source file"), 26);
                //if (! is_read_only) roundtripsubm.insertItem("Python", 27);
            }

            if (idl && !idl_edited && !idl_path.isEmpty()) {
                editsubm.insertItem(TR("Idl source file"), 20);
                //if (! is_read_only) roundtripsubm.insertItem("Idl", 21);
            }

            if (roundtripsubm.count() != 0)
                m.insertItem(TR("Roundtrip"), &roundtripsubm);

            if (editsubm.count() != 0)
                m.insertItem(TR("See file"), &editsubm);
        }

        m.insertSeparator();
        m.setWhatsThis(m.insertItem(TR("Referenced by"), 3),
                       TR("to know who reference the <i>artifact</i> \
through a relation"));
        mark_menu(m, TR("the artifact"), 90);
        ProfiledStereotypes::menu(m, this, 99990);

        if ((edition_number == 0) &&
            Tool::menu_insert(&toolm, get_type(), 100)) {
            m.insertSeparator();
            m.insertItem(TR("Tool"), &toolm);
        }
    }
    else if (!is_read_only && (edition_number == 0))
        m.setWhatsThis(m.insertItem(TR("Undelete"), 2),
                       TR("to undelete the <i>artifact</i>"));

    int n = 0;
    Q3ValueList<BrowserClass *>::ConstIterator end = associated_classes.end();
    Q3ValueList<BrowserClass *>::ConstIterator it;

    for (it = associated_classes.begin(); it != end; ++it)
        if (!(*it)->deletedp())
            n += 1;

    if (n == 1) {
        m.insertSeparator();
        m.setWhatsThis(m.insertItem(TR("Select the associated class"), 10000),
                       TR("to select the associated <i>class</i>"));
    }
    else if (n > 20) {
        m.insertSeparator();
        m.setWhatsThis(m.insertItem(TR("Select an associated class"), 9999),
                       TR("to select an associated <i>class</i>"));
    }
    else if (n != 0) {
        m.insertSeparator();
        MenuFactory::createTitle(clsubm, TR("Choose class"));
        clsubm.insertSeparator();

        for (it = associated_classes.begin(), n = 10000; it != end; ++it)
            if (!(*it)->deletedp())
                clsubm.insertItem((*it)->full_name(TRUE), n++);

        m.setWhatsThis(m.insertItem(TR("Select an associated class"), &clsubm),
                       TR("to select an associated <i>class</i>"));
    }

    exec_menu_choice(m.exec(QCursor::pos()),
                     cpp_h_path, cpp_src_path,
                     java_path, php_path, python_path, idl_path);
}

void BrowserArtifact::exec_menu_choice(int rank,
                                       QString cpp_h_path,
                                       QString cpp_src_path,
                                       QString java_path,
                                       QString php_path,
                                       QString python_path,
                                       QString idl_path)
{
    switch (rank) {
    case 0:
        def->edit();
        return;

    case 1:
        delete_it();
        break;

    case 2:
        undelete(FALSE);
        break;

    case 3:
        ReferenceDialog::show(this);
        return;

    case 10:
    {
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

    case 22: {
        bool preserve = preserve_bodies();

        ToolCom::run((verbose_generation())
                     ? ((preserve) ? "php_generator -v -p" : "php_generator -v")
                         : ((preserve) ? "php_generator -p" : "php_generator"),
                         this);
    }

    return;

    case 25: {
        bool preserve = preserve_bodies();

        ToolCom::run((verbose_generation())
                     ? ((preserve) ? "python_generator -v -p" : "python_generator -v")
                         : ((preserve) ? "python_generator -p" : "python_generator"),
                         this);
    }

    return;

    case 12:
        ToolCom::run((verbose_generation()) ? "idl_generator -v" : "idl_generator", this);
        return;

    case 13:
        //roundtrip C++ header & source files
        return;

    case 14:
        (new SourceDialog(cpp_h_path, cpp_h_edited, edition_number))->show();
        return;

    case 15:
        ToolCom::run("cpp_roundtrip", this);
        return;

    case 16:
        (new SourceDialog(cpp_src_path, cpp_src_edited, edition_number))->show();
        return;

    case 18:
        (new SourceDialog(java_path, java_edited, edition_number))->show();
        return;

    case 19:
        ToolCom::run("java_roundtrip", this);
        return;

    case 23:
        (new SourceDialog(php_path, php_edited, edition_number))->show();
        return;

    case 24:
        //roundtrip Php source files
        return;

    case 26:
        (new SourceDialog(python_path, python_edited, edition_number))->show();
        return;

    case 27:
        //roundtrip Python source files
        return;

    case 20:
        (new SourceDialog(idl_path, idl_edited, edition_number))->show();
        return;

    case 21:
        //roundtrip Idl source files
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

    case 9999: {
        ClassListDialog dialog(TR("Choose class"), associated_classes);

        dialog.raise();

        if (dialog.exec() != QDialog::Accepted)
            return;

        rank = dialog.choosen() + 10000;
    }

    // no break
    default:
        if (rank >= 99990)
            ProfiledStereotypes::choiceManagement(this, rank - 99990);
        else if (rank >= 10000) {
            int n;
            Q3ValueList<BrowserClass *>::ConstIterator it;

            for (it = associated_classes.begin(), n = 10000;
                 (*it)->deletedp() || (n++ != rank);
                 ++it)
                ;

            (*it)->select_in_browser();
        }
        else if (rank >= 100)
            ToolCom::run(Tool::command(rank - 100), this);
        else
            mark_management(rank - 90);

        return;
    }

    package_modified();
}

void BrowserArtifact::apply_shortcut(QString s)
{
    int choice = -1;
    QString cpp_h_path;
    QString cpp_src_path;
    QString java_path;
    QString php_path;
    QString python_path;
    QString idl_path;

    if (!deletedp()) {
        if (!is_edited) {
            if (s == "Edit")
                choice = 0;

            if (!is_read_only && (edition_number == 0)) {
                if (s == "Delete")
                    choice = 1;
            }
        }

        bool a_text = !strcmp(def->get_stereotype(), "text");

        if (a_text || !strcmp(def->get_stereotype(), "source")) {
            if (s == "Generate C++")
                choice = 10;
            else if (s == "Generate Java")
                choice = 11;
            else if (s == "Generate Php")
                choice = 22;
            else if (s == "Generate Python")
                choice = 25;
            else if (s == "Generate Idl")
                choice = 12;
            else {
                if (!a_text && preserve_bodies()) {
                    if (s == "Roundtrip C++ operation body")
                        choice = 30;
                    else if (s == "Roundtrip Java operation body")
                        choice = 31;
                    else if (s == "Roundtrip Php operation body")
                        choice = 32;
                    else if (s == "Roundtrip Python operation body")
                        choice = 33;
                }

                if (choice == -1) {
                    get_paths(cpp_h_path, cpp_src_path, java_path, php_path, python_path, idl_path);

                    if (!a_text && !cpp_h_edited && !cpp_h_path.isEmpty()) {
                        //if (! cpp_src_path.isEmpty())
                        //roundtripsubm.insertItem("C++ header & source files", 13);
                        if (s == "See C++ header file")
                            choice = 14;

                        //roundtripsubm.insertItem("C++ header file", 15);
                    }

                    if (!cpp_src_edited && !cpp_src_path.isEmpty()) {
                        if (s == "See C++ source file")
                            choice = 16;

                        //roundtripsubm.insertItem("C++ source file", 17);
                    }

                    if (!java_edited && !java_path.isEmpty()) {
                        if (s == "Java source file")
                            choice = 18;

                        //roundtripsubm.insertItem("Java source file", 17);
                    }

                    if (!php_edited && !php_path.isEmpty()) {
                        if (s == "Php source file")
                            choice = 23;

                        //roundtripsubm.insertItem("Php source file", 24);
                    }

                    if (!python_edited && !python_path.isEmpty()) {
                        if (s == "Python source file")
                            choice = 26;

                        //roundtripsubm.insertItem("Python source file", 27);
                    }

                    if (!idl_edited && !idl_path.isEmpty()) {
                        if (s == "Idl source file")
                            choice = 20;

                        //roundtripsubm.insertItem("Idl source file", 21);
                    }
                }
            }
        }

        if (choice == -1) {
            if (s == "Referenced by")
                choice = 3;
            else {
                mark_shortcut(s, choice, 90);

                if ((choice == -1) && (edition_number == 0))
                    Tool::shortcut(s, choice, get_type(), 100);
            }
        }
    }
    else if (!is_read_only && (edition_number == 0))
        if (s == "Undelete")
            choice = 2;

    exec_menu_choice(choice, cpp_h_path, cpp_src_path,
                     java_path, php_path, python_path, idl_path);
}

void BrowserArtifact::open(bool force_edit)
{

    if (!force_edit )
    {
        if(associated_diagram != 0)
        {
            bool deletedDigram = associated_diagram->deletedp();
            if(!deletedDigram)
            {
                associated_diagram->open(FALSE);
            }
            else if (!is_edited)
                def->edit();
        }
        else if (!is_edited)
            def->edit();
    }
    else if (!is_edited)
        def->edit();
}

void BrowserArtifact::modified()
{
    repaint();
    ((BrowserNode *) parent())->modified();
    // to update regions drawing
    def->modified();
}

UmlCode BrowserArtifact::get_type() const
{
    return UmlArtifact;
}

QString BrowserArtifact::get_stype() const
{
    return TR("artifact");
}

int BrowserArtifact::get_identifier() const
{
    return get_ident();
}

const char * BrowserArtifact::help_topic() const
{
    return "artifact";
}

void BrowserArtifact::DragMoveEvent(QDragMoveEvent * e)
{
    if (UmlDrag::canDecode(e, BrowserSimpleRelation::drag_key(this))) {
        if (!is_read_only)
            e->accept();
        else
            e->ignore();
    }
    else
        ((BrowserNode *) parent())->DragMoveInsideEvent(e);
}

void BrowserArtifact::DragMoveInsideEvent(QDragMoveEvent * e)
{
    if (!is_read_only &&
        UmlDrag::canDecode(e, BrowserSimpleRelation::drag_key(this)))
        e->accept();
    else
        e->ignore();
}

void BrowserArtifact::DropEvent(QDropEvent * e)
{
    DropAfterEvent(e, 0);
}

void BrowserArtifact::DropAfterEvent(QDropEvent * e, BrowserNode * after)
{
    BrowserNode * bn;

    if (((bn = UmlDrag::decode(e, BrowserSimpleRelation::drag_key(this))) != 0) &&
        (bn != after) && (bn != this)) {
        if (may_contains(bn, FALSE)) {
            BrowserNode * old = ((BrowserNode *) bn->parent());

            if (after)
                bn->moveItem(after);
            else {
                bn->parent()->takeItem(bn);
                insertItem(bn);
            }

            if (old != this) {
                old->modified();
                old->package_modified();
                bn->modified();
            }

            modified();
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

BrowserArtifact * BrowserArtifact::get_artifact(BrowserNode * future_parent,
        bool existing)
{
    BrowserNode * old;
    QString name;
    BrowserNodeList nodes;

    if (!future_parent->enter_child_name(name,
                                         (existing) ? TR("choose existing artifact")
                                         : TR("enter artifact's name : "),
                                         UmlArtifact, instances(nodes), &old,
                                         TRUE, FALSE, existing))
        return 0;

    if (old != 0)
        return ((BrowserArtifact *) old);

    future_parent->setOpen(TRUE);
    future_parent->package_modified();

    return new BrowserArtifact(name, future_parent);
}

BrowserArtifact * BrowserArtifact::add_artifact(BrowserNode * future_parent)
{
    QString name;

    if (!future_parent->enter_child_name(name, TR("enter artifact's name : "),
                                         UmlArtifact, TRUE, FALSE))
        return 0;

    future_parent->setOpen(TRUE);
    future_parent->package_modified();
    return new BrowserArtifact(name, future_parent);
}

BasicData * BrowserArtifact::get_data() const
{
    return def;
}

BrowserNodeList & BrowserArtifact::instances(BrowserNodeList & result, const char * st)
{
    IdIterator<BrowserArtifact> it(all);

    if ((st == 0) || (*st == 0)) {
        while (it.current() != 0) {
            if (!it.current()->deletedp())
                result.append(it.current());

            ++it;
        }
    }
    else {
        while (it.current() != 0) {
            if (!it.current()->deletedp() &&
                !strcmp(it.current()->get_data()->get_stereotype(), st))
                result.append(it.current());

            ++it;
        }
    }

    result.sort_it();

    return result;
}

BrowserArtifact * BrowserArtifact::find(const char * s)
{
    IdIterator<BrowserArtifact> it(all);
    BrowserArtifact * cl;

    while ((cl = it.current()) != 0) {
        if (strcmp(cl->name, s) == 0)
            return cl;

        ++it;
    }

    return 0;
}

BrowserNode * BrowserArtifact::get_associated() const
{
    return associated_diagram;
}

void BrowserArtifact::set_associated_diagram(BrowserDeploymentDiagram * dg,
        bool on_read)
{
    if (associated_diagram != dg) {
        if (associated_diagram != 0)
            QObject::disconnect(associated_diagram->get_data(), SIGNAL(deleted()),
                                def, SLOT(on_delete()));

        if ((associated_diagram = dg) != 0)
            QObject::connect(associated_diagram->get_data(), SIGNAL(deleted()),
                             def, SLOT(on_delete()));

        if (!on_read)
            package_modified();
    }
}

void BrowserArtifact::on_delete()
{
    if (associated_diagram && associated_diagram->deletedp())
        associated_diagram = 0;

    /*
    QValueList<BrowserClass *>::Iterator it =
      associated_classes.begin();

    while (it != associated_classes.end()) {
      if ((*it)->deletedp())
        it = associated_classes.remove(it);
      else
        ++it;
    }
    */
}

const Q3ValueList<BrowserClass *> & BrowserArtifact::get_associated_classes() const
{
    return associated_classes;
}

void BrowserArtifact::associate_class(BrowserClass * c, bool on_read)
{
    c->set_associated_artifact(this, on_read);

    /*
    QObject::connect(c->get_data(), SIGNAL(deleted()),
    	   def, SLOT(on_delete()));
    	   */
    if (!on_read) {
        ClassData * cd = (ClassData *) c->get_data();

        if (*cd->get_cppdecl() != 0) {
            if (*def->get_cpp_h() == 0)
                def->use_default_cpp_h();

            if (*def->get_cpp_src() == 0)
                def->use_default_cpp_src();
        }

        if (*cd->get_javadecl() != 0) {
            if (*def->get_java_src() == 0)
                def->use_default_java_src();
        }

        if (*cd->get_phpdecl() != 0) {
            if (*def->get_php_src() == 0)
                def->use_default_php_src();
        }

        if (*cd->get_pythondecl() != 0) {
            if (*def->get_python_src() == 0)
                def->use_default_python_src();
        }

        if (*cd->get_idldecl() != 0) {
            if (*def->get_idl_src() == 0)
                def->use_default_idl_src();
        }

        package_modified();
    }
}

bool BrowserArtifact::add_associated_class(BrowserClass * c, bool on_read)
{
    if (associated_classes.findIndex(c) == -1) {
        associated_classes.append(c);
        associate_class(c, on_read);
        return TRUE;
    }

    return FALSE;
}

bool BrowserArtifact::remove_associated_class(BrowserClass * c, bool on_read)
{
    Q3ValueList<BrowserClass *>::Iterator it = associated_classes.find(c);

    if (it != associated_classes.end()) {
        associated_classes.remove(it);
        c->set_associated_artifact(0, on_read);

        /*
        QObject::disconnect(c->get_data(), SIGNAL(deleted()),
        		def, SLOT(on_delete()));
        		*/
        if (!on_read)
            package_modified();

        return TRUE;
    }

    return FALSE;
}

void BrowserArtifact::set_associated_classes(const Q3ValueList<BrowserClass *> & l,
        bool on_read)
{
    // manage removed classes
    Q3ValueList<BrowserClass *>::Iterator it = associated_classes.begin();

    while (it != associated_classes.end()) {
        if (l.findIndex(*it) == -1) {
            BrowserClass * cl = *it;

            it = associated_classes.remove(it);
            cl->set_associated_artifact(0, on_read);
        }
        else
            ++it;
    }

    // manage added classes
    Q3ValueList<BrowserClass *> old = associated_classes;
    associated_classes = l;	// to follow order
    Q3ValueList<BrowserClass *>::Iterator end = associated_classes.end();

    for (it = associated_classes.begin(); it != end; ++it)
        if (old.findIndex(*it) == -1)
            // new
            associate_class(*it, on_read);

    if (!on_read)
        package_modified();
}

void BrowserArtifact::init()
{
    its_default_stereotypes.clear();
    its_default_stereotypes.append("document");
    its_default_stereotypes.append("file");
    its_default_stereotypes.append("script");
    its_default_stereotypes.append("source");
    its_default_stereotypes.append("text");
    its_default_stereotypes.append("library");
    its_default_stereotypes.append("executable");

    relation_default_stereotypes.clear();
    relation_default_stereotypes.append("deploy");
    relation_default_stereotypes.append("manifest");
    relation_default_stereotypes.append("import");
    relation_default_stereotypes.append("from");
}

// unicode
const QStringList & BrowserArtifact::default_stereotypes()
{
    return its_default_stereotypes;
}

// unicode
const QStringList & BrowserArtifact::default_stereotypes(UmlCode, const BrowserNode *) const
{
    return relation_default_stereotypes;
}

bool BrowserArtifact::tool_cmd(ToolCom * com, const char * args)
{
    if (((unsigned char) args[-1]) >= firstSetCmd) {
        if (is_read_only && !root_permission())
            com->write_ack(FALSE);
        else {
            switch ((unsigned char) args[-1]) {
            case setAssocDiagramCmd:
                set_associated_diagram((BrowserDeploymentDiagram *) com->get_id(args));
                break;

            case addAssocClassCmd:
                com->write_ack(add_associated_class((BrowserClass *) com->get_id(args)));
                break;

            case removeAssocClassCmd:
                com->write_ack(remove_associated_class((BrowserClass *) com->get_id(args)));
                break;

            case setAssocClassesCmd: {
                // check redondency
                Q3ValueList<BrowserClass *> l;
                unsigned n = com->get_unsigned(args);

                while (n--)
                    l.append((BrowserClass *) com->get_id(args));

                Q3ValueList<BrowserClass *>::ConstIterator it = l.begin();
                Q3ValueList<BrowserClass *>::ConstIterator end = l.end();

                while (it != end) {
                    BrowserClass * cl = *it;

                    if (l.find(++it, cl) != end) {
                        com->write_ack(FALSE);
                        return TRUE;
                    }
                }

                set_associated_classes(l);
                com->write_ack(TRUE);
            }
            break;

            case setUserCmd:
                // plug-out upgrade
                all.remove(get_ident());
                new_ident(com->get_unsigned(args), all);
                package_modified();
                break;

            case createCmd: {
                bool ok = TRUE;

                switch (com->get_kind(args)) {
                case UmlSimpleRelations: {
                    UmlCode c;

                    if (!com->get_relation_kind(c, args))
                        ok = FALSE;
                    else {
                        BrowserNode * end;

                        switch (c) {
                        case UmlDependency:
                            end = (BrowserNode *) com->get_id(args);

                            switch (end->get_type()) {
                            case UmlPackage:
                            case UmlArtifact:
                            case UmlComponent:
                            case UmlDeploymentNode:
                                add_relation(UmlDependOn, end)->get_browser_node()->write_id(com);
                                break;

                            default:
                                ok = FALSE;
                            }

                            break;

                        case UmlGeneralisation:
                            end = (BrowserNode *) com->get_id(args);

                            if ((end->get_type() == UmlArtifact) &&
                                (check_inherit(end).isEmpty()))
                                add_relation(UmlInherit, end)->get_browser_node()->write_id(com);
                            else
                                ok = FALSE;

                            break;

                        default:
                            ok = FALSE;
                        }
                    }
                }
                break;

                default:
                    ok = FALSE;
                }

                if (! ok)
                    com->write_id(0);
                else
                    package_modified();

                break;
            }

            default:
                return (def->tool_cmd(com, args, this, comment) ||
                        BrowserNode::tool_cmd(com, args));
            }
        }
    }
    else
        return (def->tool_cmd(com, args, this, comment) ||
                BrowserNode::tool_cmd(com, args));

    return TRUE;
}

void BrowserArtifact::save_stereotypes(QTextStream & st)
{
    nl_indent(st);
    st << "artifact_stereotypes ";
    save_unicode_string_list(its_default_stereotypes, st);
    nl_indent(st);
    st << "  " << stringify(UmlDependency);
    save_unicode_string_list(relation_default_stereotypes, st);
    nl_indent(st);
    st << "end";
    nl_indent(st);
}

void BrowserArtifact::read_stereotypes(char *& st, char *& k)
{
    if (!strcmp(k, "artifact_stereotypes")) {
        read_unicode_string_list(its_default_stereotypes, st);
        read_keyword(st, stringify(UmlDependency));
        read_unicode_string_list(relation_default_stereotypes, st);
        read_keyword(st, "end");
        k = read_keyword(st);
    }
    else if ((read_file_format() < 20) && !strcmp(k, "component_stereotypes")) {
        // component -> artifact
        read_unicode_string_list(its_default_stereotypes, st);
        k = read_keyword(st);
    }
    else
        init();
}

void BrowserArtifact::save(QTextStream & st, bool ref, QString & warning)
{
    if (ref)
        st << "artifact_ref " << get_ident() << " // " << get_name();
    else {
        nl_indent(st);
        st << "artifact " << get_ident() << " ";
        save_string(name, st);
        indent(+1);
        def->save(st, warning);

        if (associated_diagram != 0) {
            nl_indent(st);
            st << "associated_diagram ";
            associated_diagram->save(st, TRUE, warning);
        }

        if (!strcmp(def->get_stereotype(), "source")) {
            nl_indent(st);
            st << "associated_classes";
            indent(+1);

            Q3ValueList<BrowserClass *>::ConstIterator it;
            Q3ValueList<BrowserClass *>::ConstIterator end = associated_classes.end();

            for (it = associated_classes.begin(); it != end; ++it) {
                nl_indent(st);
                (*it)->save(st, TRUE, warning);
            }

            indent(-1);
            nl_indent(st);
            st << "end";
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
                else if ((child = child->nextSibling()) == 0)
                    break;
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

BrowserArtifact * BrowserArtifact::read_ref(char *& st, const char * k)
{
    if (strcmp(k, "artifact_ref") &&
        ((read_file_format() >= 20) || strcmp(k, "component_ref")))
        wrong_keyword(k, "artifact_ref");

    int id = read_id(st);
    BrowserArtifact * result = all[id];

    return (result == 0)
           ? new BrowserArtifact(id)
           : result;
}

BrowserArtifact * BrowserArtifact::read(char *& st, char * k,
                                        BrowserNode * parent)
{
    BrowserArtifact * result;
    int id;

    if (!strcmp(k, "artifact_ref") ||
        ((read_file_format() < 20) && !strcmp(k, "component_ref"))) {
        if ((result = all[id = read_id(st)]) == 0)
            result = new BrowserArtifact(id);

        return result;
    }
    else if (!strcmp(k, "artifact") ||
             ((read_file_format() < 20) && !strcmp(k, "component"))) {
        id = read_id(st);

        QString s = read_string(st);

        k = read_keyword(st);

        if ((result = all[id]) == 0) {
            result = new BrowserArtifact(s, parent, id);
            result->def->read(st, k);	// updates k
        }
        else if (result->is_defined) {
            BrowserArtifact * already_exist = result;

            result = new BrowserArtifact(s, parent, id);
            result->def->read(st, k);	// updates k

            already_exist->must_change_id(all);
            already_exist->unconsistent_fixed("artifact", result);
        }
        else {
            result->def->read(st, k);	// updates k
            result->set_parent(parent);
            result->set_name(s);
        }

        result->is_defined = TRUE;

        result->is_read_only = (!in_import() && read_only_file()) ||
                               ((user_id() != 0) && result->is_api_base());

        if ((read_file_format() < 20) && !strcmp(k, "associated_component_diagram")) {
            // old format && component -> artifact
            result->set_associated_diagram(BrowserDeploymentDiagram::read_ref(st, "componentdiagram_ref"),
                                           TRUE);
            k = read_keyword(st);
        }
        else if (!strcmp(k, "associated_diagram")) {
            result->set_associated_diagram(BrowserDeploymentDiagram::read_ref(st, read_keyword(st)),
                                           TRUE);
            k = read_keyword(st);
        }

        if ((read_file_format() < 20) && !strcmp(k, "associated_class")) {
            // old format from component definition (component -> artifact)
            Q3ValueList<BrowserClass *> l;

            if (read_file_format() < 3)
                // very old format
                l.append(BrowserClass::read_ref(st, "class_ref"));
            else
                l.append(BrowserClass::read_ref(st));

            result->set_associated_classes(l, TRUE);
            k = read_keyword(st);
        }
        else if (!strcmp(k, "associated_classes")) {
            Q3ValueList<BrowserClass *> l;

            while (strcmp((k = read_keyword(st)), "end"))
                l.append(BrowserClass::read_ref(st, k));

            result->set_associated_classes(l, TRUE);
            k = read_keyword(st);
        }

        result->BrowserNode::read(st, k, id);	// updates k

        if (strcmp(k, "end")) {
            while (BrowserSimpleRelation::read(st, k, result))
                k = read_keyword(st);

            if (strcmp(k, "end"))
                wrong_keyword(k, "end");
        }

        return result;
    }
    else
        return 0;
}

BrowserNode * BrowserArtifact::get_it(const char * k, int id)
{
    return (!strcmp(k, "artifact_ref")) ? all[id] : 0;
}
