// *************************************************************************cr();

//
// Copyright 2004-2010 Bruno PAGES  .
// Copyright 2012-2013 Nikolai Marchenko.
// Copyright 2012-2013 Leonardo Guilherme.
// Copyright 2011-2012 Nikolai Marchenko.
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
// home   : https://sourceforge.net/projects/douml
//
// *************************************************************************





#include <q3popupmenu.h>
#include <qpainter.h>
#include <qcursor.h>
#include <qapplication.h>
#include <qfile.h>
#include <q3dict.h>
//Added by qt3to4:
#include <QPixmap>
#include <Q3ValueList>
#include <QTextStream>
#include <QDropEvent>
#include <QDragMoveEvent>


#include "BrowserClass.h"
#include "BrowserClassInstance.h"
#include "BrowserAttribute.h"
#include "BrowserOperation.h"
#include "BrowserExtraMember.h"
#include "BrowserRelation.h"
#include "BrowserSimpleRelation.h"
#include "BrowserArtifact.h"
#include "BrowserComponent.h"
#include "ArtifactData.h"
#include "AttributeData.h"
#include "OperationData.h"
#include "RelationData.h"
#include "ClassData.h"
#include "PackageData.h"
#include "BrowserClassView.h"
#include "BrowserClassDiagram.h"
#include "BrowserUseCaseDiagram.h"
#include "BrowserDeploymentView.h"
#include "UmlPixmap.h"
#include "UmlDrag.h"
#include "UmlGlobal.h"
#include "myio.h"
#include "ToolCom.h"
#include "Tool.h"
#include "ui/menufactory.h"
#include "ClassDialog.h"
#include "OperationListDialog.h"
#include "BrowserView.h"
#include "BrowserPackage.h"
#include "BrowserActivityObject.h"
#include "BrowserPin.h"
#include "BrowserParameter.h"
#include "BrowserExpansionNode.h"
#include "ReferenceDialog.h"
#include "DialogUtil.h"
#include "mu.h"
#include "GenerationSettings.h"
#include "strutil.h"
#include "ProfiledStereotypes.h"
#include "translate.h"
#include "RelationData.h"
#include "menufactory.h"
#include "misc/TypeIdentifier.h"

// Added for porting by lgfreitas
#include <QChar>

IdDict<BrowserClass> BrowserClass::all(257, __FILE__);

QStringList BrowserClass::its_default_stereotypes;	// unicode
QStringList BrowserClass::relations_default_stereotypes[UmlRelations];	// unicode

static bool NeedPostLoad = FALSE;
static BrowserClass * TemporaryClass;

BrowserClass::BrowserClass(QString s, BrowserNode * p, ClassData * d, int id)
    : BrowserNode(s, p), Labeled<BrowserClass>(all, id),
      def(d), associated_diagram(0), associated_artifact(0)
{
}

BrowserClass::BrowserClass(const BrowserClass * model, BrowserNode * p)
    : BrowserNode(model->get_name(), p), Labeled<BrowserClass>(all, 0),
      associated_diagram(0), associated_artifact(0)
{
    def = new ClassData(model->def, this);
    comment = model->comment;
}

BrowserClass::BrowserClass(int id)
    : BrowserNode(), Labeled<BrowserClass>(all, id),
      def(new ClassData), associated_diagram(0),
      associated_artifact(0)
{
    // not yet read
    def->set_browser_node(this);
}

BrowserClass::~BrowserClass()
{
    if (deletedp() && !modifiedp())
        QFile::remove(bodies_file());

    all.remove(get_ident());
    delete def;
}

void BrowserClass::delete_it()
{
    BrowserNode::delete_it();

    if ((associated_artifact != 0) &&
            associated_artifact->is_writable())
        associated_artifact->remove_associated_class(this);

    Q3ValueList<BrowserComponent *>::Iterator it;

    for (it = associated_components.begin();
         it != associated_components.end();
         it++)
        if ((*it)->is_writable())
            (*it)->remove_associated_class(this);
}

// undelete internal processing

bool BrowserClass::undelete(bool rec, QString & warning, QString & renamed)
{
    bool result;
    bool ste = (strcmp(def->get_stereotype(), "stereotype") == 0);

    if (deletedp()) {
        // undelete the node
        QString s = name;
        bool ren = FALSE;

        while (((BrowserNode *) parent())
               ->wrong_child_name(s, get_type(),
                                  allow_spaces(), allow_empty()) ||
               (ste && !ProfiledStereotypes::canAddStereotype(this, s).isEmpty())) {
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

void BrowserClass::clear(bool old)
{
    all.clear(old);
    BrowserAttribute::clear(old);
    BrowserRelation::clear(old);
    BrowserOperation::clear(old);
    BrowserExtraMember::clear(old);
}

void BrowserClass::update_idmax_for_root()
{
    all.update_idmax_for_root();
    BrowserAttribute::update_idmax_for_root();
    BrowserRelation::update_idmax_for_root();
    BrowserOperation::update_idmax_for_root();
    BrowserExtraMember::update_idmax_for_root();
}

void BrowserClass::prepare_update_lib() const
{
    all.memo_id_oid(get_ident(), original_id);

    for (Q3ListViewItem * child = firstChild();
         child != 0;
         child = child->nextSibling())
        ((BrowserNode *) child)->prepare_update_lib();
}

void BrowserClass::support_file(Q3Dict<char> & files, bool add) const
{
    QString s;

    s = s.setNum(get_ident()) + ".bodies";

    if (add)
        files.insert(s, (char *) 1);
    else
        files.remove(s);

    for (Q3ListViewItem * child = firstChild();
         child != 0;
         child = child->nextSibling())
        ((BrowserNode *) child)->support_file(files, add);
}

void BrowserClass::referenced_by(QList<BrowserNode *> & l, bool ondelete)
{
    IdIterator<BrowserClass> it(all);
    BrowserClass * c;

    while ((c = it.current()) != 0) {
        if (!c->deletedp() &&
                (c != this) &&
                c->def->reference(this))
            l.append(c);

        ++it;
    }

    BrowserRelation::compute_referenced_by(l, this);
    BrowserSimpleRelation::compute_referenced_by(l, this);
    BrowserArtifact::compute_referenced_by(l, this);
    BrowserComponent::compute_referenced_by(l, this);

    if (! ondelete) {
        BrowserAttribute::compute_referenced_by(l, this);
        BrowserOperation::compute_referenced_by(l, this);
        BrowserActivityObject::compute_referenced_by(l, this);
        BrowserPin::compute_referenced_by(l, this);
        BrowserParameter::compute_referenced_by(l, this);
        BrowserExpansionNode::compute_referenced_by(l, this);
        BrowserClassInstance::compute_referenced_by(l, this);
        BrowserClassDiagram::compute_referenced_by(l, this, "classcanvas", "class_ref");
        BrowserUseCaseDiagram::compute_referenced_by(l, this, "classcanvas", "class_ref");
    }
}

void BrowserClass::renumber(int phase)
{
    if (phase != -1)
        new_ident(phase, all);

    BrowserNode::renumber(phase);
}

bool BrowserClass::new_java_enums(QString new_st)
{
    IdIterator<BrowserClass> it(all);
    bool result = FALSE;

    while (it.current()) {
        ClassData * d = it.current()->def;
        const char * st = d->get_stereotype();

        if (*st != 0) {
            if (strcmp(st, "enum_pattern") == 0) {
                d->set_stereotype(new_st);

                if (it.current()->is_read_only)
                    result = TRUE;
                else
                    it.current()->package_modified();
            }
            else if ((strcmp(st, "enum") == 0) && (*(d->get_javadecl()) != 0)) {
                d->set_stereotype("enum_pattern");

                if (it.current()->is_read_only)
                    result = TRUE;
                else
                    it.current()->package_modified();
            }
        }

        ++it;
    }

    if (new_st.isEmpty())
        new_st = "enum_pattern";

    its_default_stereotypes.append(new_st);

    return result;
}

const QPixmap * BrowserClass::pixmap(int) const
{
    const char * st = def->get_stereotype();
    bool actor = (strcmp(st, "actor") == 0);
    bool stereotype = (strcmp(st, "stereotype") == 0);
    bool tmplt = def->get_n_formalparams();

    if (deletedp()) {
        if (actor)
            return DeletedActorIcon;
        else if (stereotype)
            return DeletedStereotypeIcon;
        else if (tmplt)
            return DeletedTemplateIcon;
        else
            return DeletedClassIcon;
    }

    if (actor)
        return ActorIcon;
    else if (stereotype)
        return StereotypeIcon;

    const QPixmap * px = ProfiledStereotypes::browserPixmap(st);

    if (px != 0)
        return px;
    else if (! nestedp())
        return (tmplt) ? TemplateIcon : ClassIcon;
    else {
        switch (def->get_uml_visibility()) {
        case UmlPublic:
            return (tmplt) ? PublicEmbeddedTemplateIcon
                           : PublicEmbeddedClassIcon;

        case UmlProtected:
            return (tmplt) ? ProtectedEmbeddedTemplateIcon
                           : ProtectedEmbeddedClassIcon;

        case UmlPrivate:
            return (tmplt) ? PrivateEmbeddedTemplateIcon
                           : PrivateEmbeddedClassIcon;

        default:
            return (tmplt) ? PackageEmbeddedTemplateIcon
                           : PackageEmbeddedClassIcon;
        }
    }
}

void BrowserClass::iconChanged()
{
    repaint();
    def->modified();
}

void BrowserClass::update_stereotype(bool rec)
{
    if (def != 0) {
        const char * stereotype = def->get_stereotype();

        if (show_stereotypes &&
                stereotype[0] &&
                (strcmp(stereotype, "stereotype") != 0)) {
            QString s = toUnicode(stereotype);
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

void BrowserClass::paintCell(QPainter * p, const QColorGroup & cg, int column,
                             int width, int alignment)
{
    const QColor & bg = p->backgroundColor();

    if (is_marked) {
        p->setBackgroundMode(::Qt::OpaqueMode);
        p->setBackgroundColor(UmlRedColor);
    }

    p->setFont((def->get_is_abstract())
               ? ((is_writable()) ? BoldItalicFont : ItalicFont)
               : ((is_writable()) ? BoldFont : NormalFont));
    Q3ListViewItem::paintCell(p, cg, column, width, alignment);

    if (is_marked) {
        p->setBackgroundMode(::Qt::TransparentMode);
        p->setBackgroundColor(bg);
    }
}

QString BrowserClass::full_name(bool rev, bool) const
{
    return fullname(rev);
}

QString BrowserClass::contextual_name(ShowContextMode mode) const
{
    const WrapperStr & (PackageData::*f)() const;
    QString sep;

    switch (mode) {
    case noContext:
        return get_name();

    case umlContext:
        return fullname(FALSE);

    case namespaceContext:
        f = &PackageData::get_cpp_namespace;
        sep = "::";
        break;

    case javaPackageContext:
        f = &PackageData::get_java_package;
        sep = ".";
        break;

    case pythonPackageContext:
        f = &PackageData::get_python_package;
        sep = ".";
        break;

    case moduleContext:
        f = &PackageData::get_idl_module;
        sep = "::";
        break;

    case DefaultShowContextMode:
        f = 0;
        sep = "::";
        break;
    }

    QString s = get_name();
    const BrowserClass * cl = this;

    if (cl->nestedp()) {
        do {
            cl = (BrowserClass *) cl->parent();
            s = cl->get_name() + sep + s;
        }
        while (cl->nestedp());
    }

    if (f == 0)
        return s;

    if (((BrowserNode *) cl->parent())->get_type() == UmlClassView) {
        // not under a use case
        BrowserArtifact * cp = cl->get_associated_artifact();

        QString context =
                (((PackageData *)
                  ((BrowserNode *)
                   (((cp == 0) ? (BrowserNode *) cl : (BrowserNode *) cp)
                    ->parent()->parent()))->get_data())->*f)();

        if (!context.isEmpty())
            s = context + sep + s;
    }

    return s;
}

void BrowserClass::member_cpp_def(const QString &, const QString &,
                                  QString & s, bool) const
{
    ClassDialog::cpp_generate_members_def(this, s);
}


static const int add_item_index = 8;
static const int add_attribute_index = 0;
static const int add_operation_index = 1;
static const int add_nested_class_index = 14;
static const int add_extra_member_index = 2;
static const int edit_index = 3;
static const int duplicate_index = 13;
static const int delete_index = 4;
static const int select_artifact_index = 5;
static const int select_component_index = 100000;
static const int see_references_index = 15;
static const int undelete_index = 6;
static const int undelete_recursive_index = 7;
static const int generate_cpp_index = 10;
static const int roundtrip_cpp_index = 31;
static const int generate_java_index = 11;
static const int roundtrip_java_index = 32;
static const int generate_php_index = 22;
static const int generate_python_index = 25;
static const int generate_idl_index = 12;
static const int first_inherited_function_index = 10000;
static const int too_much_inherited_functions_index = 9999;
static const int sensible_amount_of_visible_entries = 25;


void BrowserClass::InstallParentsMenuItems(Q3PopupMenu& inhopersubm)
{
    QStringList parents = get_parents_names();
    for(QList<QString>::Iterator it = parents.begin(); it != parents.end(); it++)
    {
        int currentId = first_inherited_function_index + sensible_amount_of_visible_entries + (it - parents.begin());
        inhopersubm.insertItem(TR("Implement whole " + *it), currentId);
        inhopersubm.setWhatsThis(currentId, QString("to implement whole ") + *it);
    }
    if(!parents.isEmpty())
        inhopersubm.insertSeparator();
}

void BrowserClass::menu()
{
    QList<BrowserOperation *> inheritedOperations = inherited_operations(sensible_amount_of_visible_entries);

    Q3PopupMenu gensubm(0);
    Q3PopupMenu roundtripm(0);
    Q3PopupMenu inhopersubm(0);
    Q3PopupMenu compsubm(0);
    Q3PopupMenu toolm(0);
    MenuFactory builder(WrapperStr("Menu"));
    Q3PopupMenu& m = builder.menu();
    bool isstereotype = (strcmp(def->get_stereotype(), "stereotype") == 0);
    bool ismetaclass = (strcmp(def->get_stereotype(), "metaclass") == 0);
    QString what = (isstereotype) ? "<i>stereotype</i>"
                                  : ((ismetaclass) ? "<i>meta class</i>"
                                                   : "<i>class</i>");
    int index;

    MenuFactory::createTitle(m, def->definition(FALSE, TRUE));

    if (!deletedp())
    {
        if (!is_read_only)
        {
            if (edition_number == 0)
            {
                m.insertSeparator();

                const char * stereotype = def->get_stereotype();

                if (!strcmp(stereotype, "enum") || !strcmp(stereotype, "enum_pattern"))
                    m.setWhatsThis(m.insertItem(TR("Add item"), add_item_index),
                                   TR("to add an <i>item</i> to the <i>enum</i>"));

                if (strcmp(stereotype, "typedef") && strcmp(stereotype, "enum_pattern"))
                {
                    m.setWhatsThis(m.insertItem(TR("Add attribute"), add_attribute_index),
                                   TR("to add an <i>attribute</i> to the ") + what);

                    m.setWhatsThis(m.insertItem(TR("Add operation"), add_operation_index),
                                   TR("to add an <i>operation</i> to the ") + what);

                    bool isUnion = strcmp(stereotype, "union");
                    if (!inheritedOperations.isEmpty() && isUnion)
                    {

                        InstallParentsMenuItems(inhopersubm);

                        if (inheritedOperations.count() > sensible_amount_of_visible_entries)
                            m.setWhatsThis(m.insertItem(TR("Add inherited operation"), too_much_inherited_functions_index),
                                           TR("to redefine an inherited <i>operation</i> in the <i>class</i>"));
                        else
                        {
                            MenuFactory::createTitle(inhopersubm, TR("Choose operation to add it"));
                            inhopersubm.insertSeparator();

                            index = first_inherited_function_index;
                            foreach (BrowserOperation * oper, inheritedOperations)
                            {
                                QString menuItemText = ((BrowserNode *) oper->parent())->get_name() +
                                        QString("::") + oper->get_data()->definition(TRUE, FALSE);

                                if (((OperationData *) oper->get_data())->get_is_abstract())
                                {
                                    QFont font = inhopersubm.font();
                                    font.setItalic(true);
                                    inhopersubm.insertItem(menuItemText, index);
                                    //somewhat hackish but seems to work
                                    inhopersubm.actions().last()->setFont(font);
                                }
                                else
                                    inhopersubm.insertItem(menuItemText, index);
                                ++index;
                            }

                            m.setWhatsThis(m.insertItem(TR("Add inherited operation"), &inhopersubm),
                                           TR("to redefine an inherited <i>operation</i> in the <i>class</i>"));
                        }
                    }
                    if (!isstereotype &&
                            !ismetaclass &&
                            strcmp(stereotype, "enum") &&
                            strcmp(stereotype, "enum_pattern"))
                    {
                        m.setWhatsThis(m.insertItem(TR("Add nested class"), add_nested_class_index),
                                       TR("to add an <i>nested class</i> to the <i>class</i>"));
                    }

                    m.setWhatsThis(m.insertItem(TR("Add extra member"), add_extra_member_index),
                                   TR("to add an <i>extra member</i> to the <i>class</i>"));
                }

                m.insertSeparator();
                m.setWhatsThis(m.insertItem(TR("Edit"), edit_index),
                               TR("to edit the " + what + ","
                                  "a double click with the left mouse button does the same thing"));
                m.setWhatsThis(m.insertItem(TR("Duplicate"), duplicate_index),
                               TR("to duplicate the " + what));
                m.insertSeparator();
                m.setWhatsThis(m.insertItem(TR("Delete"), delete_index),
                               TR("to delete the " + what + "."
                                  "Note that you can undelete it after"));

                if (!isstereotype &&
                        !ismetaclass &&
                        (associated_artifact == 0) &&
                        (((BrowserNode *) parent())->get_type() == UmlClassView))
                {
                    BrowserNode * bcv = ((BrowserClassView *) parent())->get_associated();

                    if ((bcv != 0) && !bcv->deletedp())
                    {
                        Q3ListViewItem * child;

                        for (child = bcv->firstChild(); child; child = child->nextSibling())
                            if (!((BrowserNode *) child)->deletedp() &&
                                    (((BrowserNode *) child)->get_type() == UmlArtifact) &&
                                    (((BrowserNode *) child)->get_name() == name))
                                break;

                        if (child == 0)
                        {
                            // no artifact having the same name, propose to create it
                            m.insertSeparator();
                            m.setWhatsThis(m.insertItem(TR("Create source artifact"), 5),
                                           TR("to add a <i>&lt;&lt;source&gt;&gt; artifact</i> in the <i>deployment view</i> associated to "
                                              "the <i>class view</i>, this artifact will contain the generated code of the class"));
                        }
                    }
                }
            }
        }
        else
        {
            m.setWhatsThis(m.insertItem(TR("Edit"), edit_index),
                           TR("to edit the " + what + ", "
                              "a double click with the left mouse button does the same thing"));
            m.setWhatsThis(m.insertItem(TR("Duplicate"), duplicate_index),
                           TR("to duplicate the " + what));
        }

        bool have_sep = FALSE;

        if (!isstereotype && !ismetaclass &&
                (associated_artifact != 0) &&
                !associated_artifact->deletedp())
        {
            m.insertSeparator();
            have_sep = TRUE;
            m.setWhatsThis(m.insertItem(TR("Select associated artifact"), select_artifact_index),
                           TR("to select the associated <i>&lt;&lt;source&gt;&gt; artifact</i>"));
        }

        if (!isstereotype &&
                !ismetaclass &&
                !associated_components.isEmpty())
        {
            if (! have_sep)
                m.insertSeparator();

            if (associated_components.first() == associated_components.last())
                // only one component
                m.setWhatsThis(m.insertItem(TR("Select associated component"), select_component_index),
                               TR("to select the <i>component</i> providing the <i>class</i>"));
            else
            {
                MenuFactory::createTitle(compsubm, TR("Choose component"));
                compsubm.insertSeparator();

                m.setWhatsThis(m.insertItem(TR("Select an associated component"), &compsubm),
                               TR("to select a <i>component</i> providing the <i>class</i>"));

                index = select_component_index;

                Q3ValueList<BrowserComponent *>::Iterator it;

                for (it = associated_components.begin(), index = select_component_index;
                     it != associated_components.end();
                     it++, index += 1)
                    compsubm.insertItem((*it)->full_name(TRUE), index);
            }
        }

        m.insertSeparator();
        m.setWhatsThis(m.insertItem(TR("Referenced by"), see_references_index),
                       TR("to know who reference the " + what));
        mark_menu(m, TR("the class"), 90);
        ProfiledStereotypes::menu(m, this, 99990);

        if (!isstereotype && !ismetaclass)
        {
            bool cpp = GenerationSettings::cpp_get_default_defs();
            bool java = GenerationSettings::java_get_default_defs();
            bool php = GenerationSettings::php_get_default_defs();
            bool python = GenerationSettings::python_get_default_defs();
            bool idl = GenerationSettings::idl_get_default_defs();

            if (! nestedp() && (cpp || java || php || python || idl))
            {
                m.insertSeparator();
                m.insertItem(TR("Generate"), &gensubm);

                if (cpp)
                {
                    gensubm.insertItem("C++", generate_cpp_index);

                    if ((edition_number == 0) && !is_read_only)
                        roundtripm.insertItem("C++", roundtrip_cpp_index);
                }

                if (java)
                {
                    gensubm.insertItem("Java", generate_java_index);

                    if ((edition_number == 0) && !is_read_only)
                        roundtripm.insertItem("Java", roundtrip_java_index);
                }

                if (php)
                    gensubm.insertItem("Php", generate_php_index);

                if (python)
                    gensubm.insertItem("Python", generate_python_index);

                if (idl)
                    gensubm.insertItem("Idl", generate_idl_index);

                if (roundtripm.count() != 0)
                    m.insertItem(TR("Roundtrip"), &roundtripm);
            }
        }

        if ((edition_number == 0) &&
                Tool::menu_insert(&toolm, get_type(), 100))
        {
            m.insertSeparator();
            m.insertItem(TR("Tool"), &toolm);
        }
    }
    else if (!is_read_only && (edition_number == 0))
    {
        m.setWhatsThis(m.insertItem(TR("Undelete"), undelete_index),
                       TR("undelete the " + what + ". "
                          "Do not undelete its <i>attributes</i>, <i>operations</i> and <i>relations</i>"));

        Q3ListViewItem * child;

        for (child = firstChild(); child != 0; child = child->nextSibling())
        {
            if (((BrowserNode *) child)->deletedp())
            {
                m.setWhatsThis(m.insertItem(TR("Undelete recursively"), undelete_recursive_index),
                               TR("undelete the " + what + " and its "
                                  "nested <i>classes</i>, <i>attributes</i>, <i>operations</i> and "
                                  "<i>relations</i> (except if the class at the other side is also deleted)"));
                break;
            }
        }
    }

    exec_menu_choice(m.exec(QCursor::pos()), inheritedOperations);
}

void BrowserClass::AddInheritedOperations(int rank)
{
    int firstParentIndex = first_inherited_function_index + sensible_amount_of_visible_entries;
    int parentIndex = rank - firstParentIndex;
    QString parentName = get_parents_names().at(parentIndex);
    QList<BrowserOperation *> inheritedOperations = inherited_operations(sensible_amount_of_visible_entries,parentName);
    for(auto operation: inheritedOperations)
    {
        add_inherited_operation(operation);
    }
}

QList<OperationData*> BrowserClass::CollectSameThroughInheritance(OperationData * oper, QList<BrowserNode *> & passedNodes, bool goBack )
{
    QList<OperationData*> result;
    if(passedNodes.contains(static_cast<BrowserNode*>(this)))
        return result;
    passedNodes.append(static_cast<BrowserNode*>(this));
    QList<BrowserNode*> listOfChildren;
    IdIterator<BrowserClass> it(all);
    BrowserClass* testedClass;

    while ((testedClass = it.current()) != 0)
    {
        BrowserNodeList listOfTested;
        UmlCode kind1 = UmlGeneralisation;
        UmlCode kind2 = UmlRealize;
//        if(!goBack)
//            kind1=kind2;
        testedClass->children(listOfTested,kind1, kind2);
        for(BrowserNode* node : listOfTested)
        {
            BrowserRelation* generalization = static_cast<BrowserRelation*>(node);
            RelationData* relData = static_cast<RelationData*>(generalization->get_data());
            BrowserClass* startClass = relData->get_start_class();
            BrowserClass* endClass = relData->get_end_class();
            if(endClass != this && startClass != this)
                continue;
            //QLOG_INFO() << relData->get_start_class()->get_name() << stringify(generalization->get_type()) << relData->get_end_class()->get_name();
            if(endClass == this)
                listOfChildren.append(startClass);
            else if (startClass == this  && goBack )
                listOfChildren.append(endClass);
        }
        ++it;
    }

    QList<BrowserNode *>  passedCopy = passedNodes;
    for(BrowserNode* child : listOfChildren)
    {
        if(!passedCopy.contains(static_cast<BrowserNode*>(child)))
        {
            //QLOG_INFO() << "Child is" << child->get_name();
            result.append(static_cast<BrowserClass*>(child)->CollectSameThroughInheritance(oper, passedNodes, goBack));
        }
    }
    //QLOG_INFO() << "Using :" << this->get_name();

    QList<BrowserClass*> parents = get_all_parents();
    QStringList parentsNames;
    if(goBack)
    {
        for(BrowserNode* parent : parents)
        {
            if(passedNodes.contains(parent))
                continue;
            parentsNames.append(parent->full_name());
            result.append(static_cast<BrowserClass*>(parent)->CollectSameThroughInheritance(oper, passedNodes, goBack));
        }
    }
    //QLOG_INFO() << "Using 2: " << this->get_name();
    BrowserClass* operContainter = static_cast<BrowserClass*>(oper->get_browser_node()->get_container(UmlClass));
    if(operContainter == this)
        return result;
    Q3ValueList<OperationData *> usedInheritedOpers;
    QStringList listUsedInheritedOpers;
    get_own_opers(usedInheritedOpers,listUsedInheritedOpers);
    //QLOG_INFO() << oper->definition(TRUE, FALSE);
    for(OperationData* testedOper: usedInheritedOpers)
    {


        if(oper->definition(TRUE, FALSE) == testedOper->definition(TRUE, FALSE))
        //if(compare)
        {
            bool compare = PropagationEquality(*oper,*testedOper);
            if(!compare)
                continue;

            //QLOG_INFO() << testedOper->definition(TRUE, FALSE);
            QString name = full_name();
            QString origin_name = oper->get_origin_class();
            QString containerName = operContainter->full_name();
            if(parentsNames.contains(origin_name) ||
                    name ==  origin_name ||
                    origin_name == "0" ||
                    containerName == testedOper->get_origin_class())
                result.append(testedOper);
        }
    }
    return result;
}

void BrowserClass::exec_menu_choice(int rank,
                                    QList<BrowserOperation *> & l)
{
    bool isstereotypemetaclass =
            ((strcmp(def->get_stereotype(), "stereotype") == 0) ||
             (strcmp(def->get_stereotype(), "metaclass") == 0));

    switch (rank) {

    case add_attribute_index:
    case add_item_index: {
        BrowserAttribute * bn = (BrowserAttribute *) add_attribute(0, rank == add_item_index);

        if (bn != 0)
            bn->open_new_ste_attr();
    }

        return;

    case add_operation_index: {
        BrowserNode * bn = add_operation();

        if (bn != 0)
            bn->open(TRUE);
    }

        return;

    case add_extra_member_index: {
        BrowserNode * bn = add_extra_member();

        if (bn != 0)
            bn->open(TRUE);
    }

        return;

    case edit_index:
        // modal edition
        def->edit();
        return;

    case delete_index:
        ProfiledStereotypes::deleted(this);
        delete_it();
        break;

    case select_artifact_index:
        if (isstereotypemetaclass)
            return;

        if (associated_artifact == 0) {
            BrowserArtifact * ba =
                    new BrowserArtifact(name,
                                        ((BrowserClassView *) parent())->get_associated());

            ba->get_data()->set_stereotype("source");
            set_associated_artifact(ba);
        }
        else {
            associated_artifact->select_in_browser();
            return;
        }

        break;

    case undelete_index:
        BrowserNode::undelete(FALSE);

        if (!strcmp(get_data()->get_stereotype(), "stereotype"))
            ProfiledStereotypes::recompute(FALSE);

        break;

    case undelete_recursive_index:
        BrowserNode::undelete(TRUE);

        if (!strcmp(get_data()->get_stereotype(), "stereotype"))
            ProfiledStereotypes::recompute(FALSE);

        break;

    case generate_cpp_index:
        if (! isstereotypemetaclass) {
            bool preserve = preserve_bodies();

            ToolCom::run((verbose_generation())
                         ? ((preserve) ? "cpp_generator -v -p" : "cpp_generator -v")
                         : ((preserve) ? "cpp_generator -p" : "cpp_generator"),
                         this);
        }

        return;

    case generate_java_index:
        if (! isstereotypemetaclass) {
            bool preserve = preserve_bodies();

            ToolCom::run((verbose_generation())
                         ? ((preserve) ? "java_generator -v -p" : "java_generator -v")
                         : ((preserve) ? "java_generator -p" : "java_generator"),
                         this);
        }

        return;

    case generate_php_index:
        if (! isstereotypemetaclass) {
            bool preserve = preserve_bodies();

            ToolCom::run((verbose_generation())
                         ? ((preserve) ? "php_generator -v -p" : "php_generator -v")
                         : ((preserve) ? "php_generator -p" : "php_generator"),
                         this);
        }

        return;

    case generate_python_index:
        if (! isstereotypemetaclass) {
            bool preserve = preserve_bodies();

            ToolCom::run((verbose_generation())
                         ? ((preserve) ? "python_generator -v -p" : "python_generator -v")
                         : ((preserve) ? "python_generator -p" : "python_generator"),
                         this);
        }

        return;

    case generate_idl_index:
        if (! isstereotypemetaclass)
            ToolCom::run((verbose_generation()) ? "idl_generator -v" : "idl_generator", this);

        return;

    case roundtrip_cpp_index:
        if (! isstereotypemetaclass)
            ToolCom::run("cpp_roundtrip", this);

        return;

    case roundtrip_java_index:
        if (! isstereotypemetaclass)
            ToolCom::run("java_roundtrip", this);

        return;

    case duplicate_index: {
        QString name = get_name();

        if (((BrowserNode *) parent())->enter_child_name(name, TR("enter class's name : "),
                                                         UmlClass, FALSE, FALSE))

        {
            BrowserNode * duplicatedNode = duplicate((BrowserNode *) parent(), name);
            move(duplicatedNode, this);
            duplicatedNode->select_in_browser();
        }
        else
            return;
    }
        break;

    case add_nested_class_index:
        if (! isstereotypemetaclass) {
            BrowserClass * cl = add_class(FALSE, this);

            if (cl != 0)
                cl->select_in_browser();
        }

        break;

    case see_references_index:
        ReferenceDialog::show(this);
        return;

    case too_much_inherited_functions_index:
    {
        l = inherited_operations(~0u);

        OperationListDialog dialog(TR("Choose inherited operation"), l);

        dialog.raise();

        if (dialog.exec() != QDialog::Accepted)
            return;

        rank = dialog.choosen() + 10000;
    }

        // no break
    default:
        if (rank >= select_component_index)
            // assoc comp
            associated_components[rank - select_component_index]->select_in_browser();
        else if (rank >= 99990)
            ProfiledStereotypes::choiceManagement(this, rank - 99990);
        else if (rank >= first_inherited_function_index + sensible_amount_of_visible_entries)
            AddInheritedOperations(rank);
        else if (rank >= first_inherited_function_index)
            // inherited operation
            add_inherited_operation(l.at(rank - first_inherited_function_index));
        else if (rank >= 100)
            ToolCom::run(Tool::command(rank - 100), this);
        else
            mark_management(rank - 90);

        return;
    }

    package_modified();
}

void BrowserClass::apply_shortcut(QString s)
{
    int choice = -1;

    if (!deletedp()) {
        if (!is_read_only) {
            if (edition_number == 0) {

                const char * stereotype = def->get_stereotype();

                if (!strcmp(stereotype, "enum") || !strcmp(stereotype, "enum_pattern"))
                    if (s == "New item")
                        choice = 8;

                if (strcmp(stereotype, "typedef") && strcmp(stereotype, "enum_pattern")) {
                    if (s == "New attribute")
                        choice = 0;
                    else if (s == "New operation")
                        choice = 1;

                    if (strcmp(stereotype, "enum") && strcmp(stereotype, "enum_pattern")) {
                        if (s == "New class")
                            choice = 14;
                    }

                    if (s == "New extra member")
                        choice = 2;
                }

                if (s == "Edit")
                    choice = 3;
                else if (s == "Duplicate")
                    choice = 13;
                else if (s == "Delete")
                    choice = 4;

                if ((associated_artifact == 0) &&
                        (((BrowserNode *) parent())->get_type() == UmlClassView)) {
                    BrowserNode * bcv = ((BrowserClassView *) parent())->get_associated();

                    if ((bcv != 0) && !bcv->deletedp()) {
                        Q3ListViewItem * child;

                        for (child = bcv->firstChild(); child; child = child->nextSibling())
                            if (!((BrowserNode *) child)->deletedp() &&
                                    (((BrowserNode *) child)->get_type() == UmlArtifact) &&
                                    (((BrowserNode *) child)->get_name() == name))
                                break;

                        if (child == 0) {
                            // no artifact having the same name, propose to create it
                            if (s == "Create source artifact")
                                choice = 5;
                        }
                    }
                }
            }
        }
        else if (s == "Edit")
            choice = 3;

        if ((associated_artifact != 0) && !associated_artifact->deletedp()) {
            if (s == "Select associated artifact")
                choice = 5;
        }

        if (s == "Referenced by")
            choice = 15;

        mark_shortcut(s, choice, 90);

        if (edition_number == 0)
            Tool::shortcut(s, choice, get_type(), 100);

        if (! nestedp()) {
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
        }
    }
    else if (!is_read_only && (edition_number == 0)) {
        if (s == "Undelete")
            choice = 6;

        Q3ListViewItem * child;

        for (child = firstChild(); child != 0; child = child->nextSibling()) {
            if (((BrowserNode *) child)->deletedp()) {
                if (s == "Undelete recursively")
                    choice = 7;

                break;
            }
        }
    }

    QList<BrowserOperation *> l;

    exec_menu_choice(choice, l);
}

BrowserNode * BrowserClass::duplicate(BrowserNode * p, QString name)
{
    BrowserClass * result = new BrowserClass(this, p);

    result->set_name((name.isEmpty()) ? get_name() : name);

    // count the current sub elt to not take into account two time the
    // bi-dir self relations
    unsigned n = 0;
    Q3ListViewItem * child;

    for (child = firstChild(); child != 0; child = child->nextSibling())
        n += 1;

    // duplicates the sub elts
    for (child = firstChild(); n != 0; child = child->nextSibling(), n -= 1)
    {
        if (!((BrowserNode *) child)->deletedp())
        {
            if (IsaRelation(((BrowserNode *) child)->get_type()))
            {
                RelationData * rd = (RelationData *)
                        ((BrowserNode *) child)->get_data();

                if ((rd->get_start_class() != this) || (rd->get_end_class() != this) || rd->is_a((BrowserRelation *) child))
                    ((BrowserNode *) child)->duplicate(result);
            }
            else
                ((BrowserNode *) child)->duplicate(result);
        }
    }

    // update the getter/setter references
    for (child = result->firstChild(); child != 0; child = child->nextSibling())
        if (!((BrowserNode *) child)->deletedp() &&
                (((BrowserNode *) child)->get_type() == UmlOperation))
            ((BrowserOperation *) child)->post_duplicate();

    result->update_stereotype(TRUE);

    return result;
}

void BrowserClass::set_name(const char * s)
{
    if (name != s) {
        bool firsttime = name.isEmpty();

        if (! firsttime) {
            // rename constructors and destructors
            Q3ListViewItem * child;

            for (child = firstChild(); child != 0; child = child->nextSibling()) {
                if (!((BrowserNode *) child)->deletedp() &&
                        (((BrowserNode *) child)->get_type() == UmlOperation)) {
                    const char * op_name = ((BrowserNode *) child)->get_name();

                    if (op_name == name)
                        // renamed
                        ((BrowserNode *) child)->set_name(s);
                    else if ((*((const char *) op_name) == '~') &&
                             ((((const char *) op_name) + 1) == name)) {
                        // destructor renamed
                        WrapperStr d = "~";

                        d += s;
                        ((BrowserNode *) child)->set_name(d);
                    }
                }
            }

            name = s;

            // to update inherit/realize/dependency names
            // else set by BrowserNode::post_load()
            BrowserView::get_project()->update_stereotype(TRUE);

            // rename constructors and destructors

        }
        else
            name = s;
    }
}

BrowserNode * BrowserClass::add_attribute(BrowserAttribute * attr,
                                          bool enum_item, bool sameName)
{
    QString name;
    if(attr && sameName)
        name = attr->get_name();
    if (enter_child_name(name, (enum_item) ? TR("enter item's name : ")
                         : TR("enter attribute's name : "),
                         UmlAttribute, FALSE, FALSE)) {
        bool newone = (attr == nullptr);

        attr = (newone) ? BrowserAttribute::new_one(name, this, enum_item)
                        : (BrowserAttribute *) attr->duplicate(this, name);

        setOpen(TRUE);
        def->modified();
        package_modified();
        attr->select_in_browser();

        if (enum_item)
            ((AttributeData *) attr->get_data())->set_visibility(UmlPublic);
        else if (!newone && 	// ProfiledStereotypes::added called later
                 !strcmp(def->get_stereotype(), "stereotype"))
            ProfiledStereotypes::added(attr);

        return attr;
    }

    return 0;
}

BrowserNode *BrowserClass::addAttribute()
{
    BrowserAttribute * attr = nullptr;
    QString name;
    attr = BrowserAttribute::new_one(name, this, false);
    setOpen(TRUE);
    def->modified();
    package_modified();
    attr->select_in_browser();
    return attr;
}

BrowserNode *BrowserClass::addEnumItem()
{
    BrowserAttribute * attr = nullptr;
    QString name;
    attr = BrowserAttribute::new_one(name, this, true);
    setOpen(TRUE);
    def->modified();
    package_modified();
    attr->select_in_browser();
    return attr;
}

BrowserNode *BrowserClass::duplicate_attribute(BrowserAttribute *attr, bool enum_item)
{
    //BrowserNode * attrDuplicate = add_attribute(attr, enum_item, true);
    return add_attribute(attr, enum_item, true);;
}

BrowserNode * BrowserClass::add_extra_member(BrowserExtraMember * em)
{
    em = (em == 0) ? BrowserExtraMember::new_one(QString(), this)
                   : (BrowserExtraMember *) em->duplicate(this, name);

    setOpen(TRUE);
    def->modified();
    package_modified();
    em->select_in_browser();

    return em;
}

BrowserNode * BrowserClass::add_relation(BrowserRelation * rel)
{
    // the relation is an association, not a dependency/inheritance
    rel = (BrowserRelation *) rel->duplicate(this);

    def->modified();
    package_modified();
    rel->select_in_browser();

    return rel;
}

BrowserNode * BrowserClass::add_operation(BrowserOperation * oper, bool sameName)
{
    QString name;
    if(oper && sameName)
        name = oper->get_name();

    if (enter_child_name(name, TR("enter operation's name : "),
                         UmlOperation, FALSE, FALSE))
    {
        oper = (oper == 0) ? BrowserOperation::new_one(name, this)
                           : (BrowserOperation *) oper->duplicate(this);

        setOpen(TRUE);
        def->modified();
        package_modified();
        oper->set_name(name);
        oper->select_in_browser();

        return oper;
    }

    return 0;
}

BrowserNode *BrowserClass::duplicate_operation(BrowserOperation *oper)
{
    return add_operation(oper, true);
}

BrowserNode *BrowserClass::addOperation(BrowserOperation *oper)
{

    QString name;
    oper = BrowserOperation::new_one(name, this);


    setOpen(TRUE);
    def->modified();
    package_modified();
    oper->set_name(name);
    oper->select_in_browser();

    return oper;
}

BrowserNode * BrowserClass::add_inherited_operation(BrowserOperation * model)
{
    BrowserOperation * oper = new BrowserOperation(model, this);
    BrowserClass* modelContainer = static_cast<BrowserClass*>(model->get_container(UmlClass));
    OperationData* data = (OperationData *) oper->get_data();
    if(data->get_is_abstract())
    {
        data->set_is_abstract(0);
        data->set_cppdef(data->default_cpp_def(oper->get_name()));

    }
    data->set_origin_class(modelContainer->full_name());
    setOpen(TRUE);
    def->modified();
    package_modified();
    oper->select_in_browser();

    return oper;
}

QString BrowserClass::may_start(UmlCode l) const
{
    const char * stereotype = def->get_stereotype();

    if (!strcmp(stereotype, "typedef")) {
        switch (l) {
        case UmlGeneralisation:
        case UmlRealize:
            if (is_read_only && !root_permission())
                return TR("read only");
            else {
                BrowserNodeList inh;
                children(inh, UmlGeneralisation, UmlRealize);
                return (inh.count() <= 1)
                        ? QString()
                        : TR("typedef can't generalize / realize several times");
            }

        case UmlDependency:
            if (is_read_only && !root_permission())
                return TR("read only");

            // no break
        case UmlAnchor:
            return QString();

        default:
            return TR("a typedef can't have relation");
        }
    }

    switch (l) {
    case UmlGeneralisation:
    case UmlRealize:
        if (!strcmp(stereotype, "union"))
            return TR("an union can't generalize / realize");

        // no break;
    case UmlAssociation:
    case UmlDirectionalAssociation:
    case UmlAggregation:
    case UmlAggregationByValue:
    case UmlDirectionalAggregation:
    case UmlDirectionalAggregationByValue:
    case UmlDependency:
        return (!is_read_only || root_permission()) ? QString() : TR("read only");

    default:
        return 0;
    }
}

QString BrowserClass::may_connect(UmlCode l, BrowserClass * other)
{
    switch (l) {
    case UmlGeneralisation:
    case UmlRealize:
        return check_inherit(other);

    case UmlAssociation:
    case UmlAggregation:
    case UmlAggregationByValue:
        return (!other->is_read_only || root_permission()) ? QString() : TR("read only");

    default:
        return (l != UmlAnchor) ? QString() : TR("can't have anchor between classes");
    }
}

BasicData * BrowserClass::add_relation(UmlCode t, BrowserNode * end)
{
    if (IsaRelation(t))
    {
        RelationData * d = new RelationData(t);

        BrowserRelation * br1 = new BrowserRelation(this, d);
        BrowserRelation * br2 = d->set_start_end(br1, (BrowserClass *) end);

        br1->modified();	// updates name
        br1->package_modified();

        if (br2 != 0) {
            br2->modified();	// updates name
            br2->package_modified();
        }

        switch (t) {
        case UmlGeneralisation:
        case UmlRealize:
            def->inherit_or_instantiate((BrowserClass *) end);
            break;

        default:
            break;
        }

        return d;
    }
    else
        return BrowserNode::add_relation(t, end);
}

void BrowserClass::open(bool force_edit)
{
    if (!force_edit &&
            (associated_diagram != 0) &&
            !associated_diagram->deletedp())
        associated_diagram->open(FALSE);
    else if (edition_number == 0)
        // modal edition
        def->edit();
}

UmlCode BrowserClass::get_type() const
{
    return UmlClass;
}

QString BrowserClass::get_stype() const
{
    return TR("class");
}

int BrowserClass::get_identifier() const
{
    return get_ident();
}

const char * BrowserClass::help_topic() const
{
    return (!strcmp(def->get_stereotype(), "metaclass") ||
            (!strcmp(def->get_stereotype(), "stereotype") &&
             (((BrowserNode *) parent()->parent())->get_type() == UmlPackage) &&
             !strcmp(((BrowserNode *) parent()->parent())->get_data()->get_stereotype(),
                     "profile")))
            ? "profile"
            : "class";
}

void BrowserClass::modified()
{
    repaint();
    def->modified();
    ((BrowserNode *) parent())->modified();
}

BasicData * BrowserClass::get_data() const
{
    return def;
}

bool BrowserClass::allow_spaces() const
{
    return FALSE;
}

QList<BrowserClass*> BrowserClass::get_all_parents()
{
    QList<BrowserClass*> result;
    QList<BrowserNode *> notyet = parents();

    while (! notyet.isEmpty())
    {
        BrowserClass * cl = (BrowserClass *) notyet.first();
        notyet.removeFirst();

        if ((cl != this) && (!result.contains(cl)))
        {
            result.append(cl);

            QList<BrowserNode *> grand_parents = cl->parents();

            foreach (BrowserNode * gp, grand_parents)
                if (! notyet.contains(gp))
                    notyet.append(gp);
        }
    }
    return result;
}

QStringList BrowserClass::get_parents_names()
{
    QStringList result;
    QList<BrowserClass*> parents = get_all_parents();
    for(BrowserClass* parent: parents)
    {
        result << parent->full_name();
    }
    return result;
}

UmlVisibility BrowserClass::get_visibility() const
{
    return def->get_uml_visibility();
}


// compute all parents, grand parents ...
void BrowserClass::get_all_parents(QList<BrowserClass *> & l) const
{
    QList<BrowserNode *> notyet = parents();

    while (! notyet.isEmpty()) {
        BrowserClass * cl = (BrowserClass *) notyet.first();
        notyet.removeFirst();

        if ((cl != this) && (! l.contains(cl))) {
            l.append(cl);

            QList<BrowserNode *> grand_parents = cl->parents();

            foreach (BrowserNode * gp, grand_parents)
                if (! notyet.contains(gp))
                    notyet.append(gp);
        }
    }
}

// returns all attributes included the inherited

void BrowserClass::get_attrs(BrowserNodeList & attributes)
{
    attributes.clear();

    QList<BrowserClass *> lcl;

    get_all_parents(lcl);
    lcl.append(this);

    foreach (BrowserClass *current, lcl) {
        Q3ListViewItem * child;
        for (child = current->firstChild(); child; child = child->nextSibling())
            if (!((BrowserNode *) child)->deletedp() &&
                    (((BrowserNode *) child)->get_type() == UmlAttribute))
                attributes.append((BrowserNode *) child);
    }
}

// returns all operations included the inherited (except the redefined ones
// and the contructor and destructor of parents)

void BrowserClass::get_opers(Q3ValueList<const OperationData *> & opers,
                             QStringList & list) const
{
    QList<BrowserClass *> all_parents;

    get_all_parents(all_parents);

    // own operations
    Q3ListViewItem * child;
    Q3Dict<void> already(499);

    for (child = firstChild(); child; child = child->nextSibling()) {
        if ((((BrowserNode *) child)->get_type() == UmlOperation) &&
                !((BrowserNode *) child)->deletedp()) {
            OperationData * od =
                    (OperationData *)((BrowserNode *) child)->get_data();
            QString msg = od->definition(TRUE, FALSE);

            opers.append(od);
            list.append(msg);
            already.insert(msg, (void *) 1);
        }
    }

    // inherited

    foreach (BrowserClass * cl, all_parents)
    {
        for (child = cl->firstChild(); child; child = child->nextSibling())
        {
            if ((((BrowserNode *) child)->get_type() == UmlOperation) &&
                    (((BrowserNode *) child)->get_name() != cl->name) &&		// not a constructor
                    (((BrowserNode *) child)->get_name()[0] != '~')) {// not a destructor
                OperationData * data = (OperationData *)
                        ((BrowserOperation *) child)->get_data();
                QString profile = data->definition(TRUE, FALSE);

                if (already.find(profile) == 0)
                {
                    // not yet added
                    opers.append(data);
                    list.append(profile);
                    already.insert(profile, (void *) 1);
                }
            }
        }
    }
}

void BrowserClass::get_own_opers(Q3ValueList<OperationData *> &opers, QStringList &list)
{
    // own operations
    Q3ListViewItem * child;
    Q3Dict<void> already(499);

    for (child = firstChild(); child; child = child->nextSibling()) {
        if ((((BrowserNode *) child)->get_type() == UmlOperation) &&
                !((BrowserNode *) child)->deletedp()) {
            OperationData * od =
                    (OperationData *)((BrowserNode *) child)->get_data();
            QString msg = od->definition(TRUE, FALSE);

            opers.append(od);
            list.append(msg);
            already.insert(msg, (void *) 1);
        }
    }
}

void BrowserClass::get_inherited_opers(Q3ValueList< OperationData *> &opers, QStringList &list)
{
    QList<BrowserClass *> all_parents;

    get_all_parents(all_parents);

    Q3ListViewItem * child;
    Q3Dict<void> already(499);

    foreach (BrowserClass * cl, all_parents)
    {
        for (child = cl->firstChild(); child; child = child->nextSibling())
        {
            if ((((BrowserNode *) child)->get_type() == UmlOperation) &&
                    (((BrowserNode *) child)->get_name() != cl->name) &&		// not a constructor
                    (((BrowserNode *) child)->get_name()[0] != '~')) {// not a destructor
                OperationData * data = (OperationData *)
                        ((BrowserOperation *) child)->get_data();
                QString profile = data->definition(TRUE, FALSE);

                if (already.find(profile) == 0)
                {
                    // not yet added
                    opers.append(data);
                    list.append(profile);
                    already.insert(profile, (void *) 1);
                }
            }
        }
    }
}

void BrowserClass::get_used_inherited_opers()
{
    Q3ValueList<OperationData *> ownOpers;
    QStringList listOwnOpers;
    Q3ValueList<OperationData *> inheritedOpers;
    QStringList listInheritedOpers;
    Q3ValueList<OperationData *> usedInheritedOpers;
    QStringList listUsedInheritedOpers;
    get_own_opers(ownOpers,listOwnOpers);
    get_inherited_opers(inheritedOpers,listInheritedOpers);
    for(OperationData* oper: ownOpers)
    {
        QString profile = oper->definition(TRUE, FALSE);
        for(OperationData* inhritedOper: inheritedOpers)
        {
            QString testProfile = inhritedOper->definition(TRUE, FALSE);
            if(profile == testProfile && !listUsedInheritedOpers.contains(profile))
            {
                usedInheritedOpers.append(oper);
                listUsedInheritedOpers.append(profile);
            }
        }
    }
}

// search for the relations from classes of 'from' having
// target in 'to'

static void get_assocs(QList<BrowserClass *> & from,
                       QList<BrowserClass *> & to,
                       QList<RelationData *> & rels)
{
    foreach (BrowserClass *current, from) {
        Q3ListViewItem * child;

        for (child = current->firstChild(); child; child = child->nextSibling()) {
            if (!((BrowserNode *) child)->deletedp()) {
                switch (((BrowserNode *) child)->get_type()) {
                case UmlAssociation:
                case UmlDirectionalAssociation:
                case UmlAggregation:
                case UmlAggregationByValue:
                case UmlDirectionalAggregation:
                case UmlDirectionalAggregationByValue: {
                    RelationData * rd = (RelationData *)
                            ((BrowserRelation *) child)->get_data();

                    if ((rels.indexOf(rd) == -1) &&
                            (to.contains(rd->get_end_class())))
                        rels.append(rd);
                }

                default:
                    break;
                }
            }
        }
    }
}

// returns all relation to target included the inherited
// if 'rev' != 0 search also for relation in opposite direction
// and set rev to the number of relation in the first direction
void BrowserClass::get_rels(BrowserClass * target,
                            QList<RelationData *> & l, int * rev) const
{
    QList<BrowserClass *> la;
    QList<BrowserClass *> lb;

    get_all_parents(la);
    target->get_all_parents(lb);
    la.append((BrowserClass *) this);
    lb.append(target);
    get_assocs(la, lb, l);

    if (rev != 0) {
        *rev = l.count();
        QList<RelationData *> ll;

        get_assocs(lb, la, ll);

        l << ll;
        ll.clear();
    }
}

// search for the relations from classes of 'from' having
// target in 'to' (may be the reverse direction of a bidir)

static void get_assocs(QList<BrowserClass *> & from,
                       QList<BrowserClass *> & to,
                       QList<BrowserRelation *> & rels)
{
    foreach (BrowserClass *current, from) {
        Q3ListViewItem * child;

        for (child = current->firstChild(); child; child = child->nextSibling()) {
            if (!((BrowserNode *) child)->deletedp()) {
                switch (((BrowserNode *) child)->get_type()) {
                case UmlAssociation:
                case UmlDirectionalAssociation:
                case UmlAggregation:
                case UmlAggregationByValue:
                case UmlDirectionalAggregation:
                case UmlDirectionalAggregationByValue:
                    if (! rels.contains((BrowserRelation *) child)) {
                        BrowserRelation * r = (BrowserRelation *) child;
                        RelationData * rd = (RelationData *) r->get_data();

                        if ((rd->is_a(r))
                                ? (to.contains(rd->get_end_class()))
                                : (to.contains(rd->get_start_class())))
                            rels.append(r);
                    }

                default:
                    break;
                }
            }
        }
    }
}

// returns all relation to target included the inherited
// (may be the reverse direction of a bidir)
void BrowserClass::get_rels(BrowserClass * target,
                            QList<BrowserRelation *> & l) const
{
    QList<BrowserClass *> la;
    QList<BrowserClass *> lb;

    get_all_parents(la);
    target->get_all_parents(lb);
    la.append((BrowserClass *) this);
    lb.append(target);
    get_assocs(la, lb, l);
}

// get the class and its members, and all parent and their members
// except deleted
void BrowserClass::get_tree(BrowserNodeList & l)
{
    QList<BrowserClass *> all_parents;

    all_parents.append(this);
    get_all_parents(all_parents);
    l.clear();

    foreach (BrowserClass *cl, all_parents) {
        if (! cl->deletedp()) {
            l.append(cl);

            for (Q3ListViewItem * child = cl->firstChild(); child; child = child->nextSibling()) {
                if (!((BrowserNode *) child)->deletedp()) {
                    UmlCode k = ((BrowserNode *) child)->get_type();

                    switch (k) {
                    default:
                        if (! IsaRelation(k))
                            break;

                        // no break;
                    case UmlAttribute:
                    case UmlOperation:
                        l.append((BrowserNode *) child);
                    }
                }
            }
        }
    }
}

const char * BrowserClass::constraint() const
{
    return def->get_constraint();
}

void BrowserClass::DragMoveEvent(QDragMoveEvent * e)
{
    if (UmlDrag::canDecode(e, UmlClass) ||
            UmlDrag::canDecode(e, UmlAttribute) ||
            UmlDrag::canDecode(e, UmlOperation) ||
            UmlDrag::canDecode(e, UmlExtraMember) ||
            UmlDrag::canDecode(e, BrowserOperation::drag_key(this)) ||
            UmlDrag::canDecode(e, BrowserRelation::drag_key(this)) ||
            UmlDrag::canDecode(e, BrowserSimpleRelation::drag_key(this))) {
        if (!is_read_only)
            e->accept();
        else
            e->ignore();
    }
    else
        ((BrowserNode *) parent())->DragMoveInsideEvent(e);
}

void BrowserClass::DragMoveInsideEvent(QDragMoveEvent * e)
{
    if (!is_read_only &&
            (UmlDrag::canDecode(e, UmlClass) ||
             UmlDrag::canDecode(e, UmlAttribute) ||
             UmlDrag::canDecode(e, UmlOperation) ||
             UmlDrag::canDecode(e, UmlExtraMember) ||
             UmlDrag::canDecode(e, BrowserOperation::drag_key(this)) ||
             UmlDrag::canDecode(e, BrowserRelation::drag_key(this)) ||
             UmlDrag::canDecode(e, BrowserSimpleRelation::drag_key(this))))
        e->accept();
    else
        e->ignore();
}

bool BrowserClass::may_contains_them(const QList<BrowserNode *> & l,
                                     BooL & duplicable) const
{
    foreach (BrowserNode *current, l) {
        switch (current->get_type()) {
        case UmlOperation: {
            BrowserOperation* asOperation = ((BrowserOperation *) current);
            if (asOperation->get_get_of()) {
                if (! l.contains(asOperation->get_get_of()))
                    return false;
            }

            if (asOperation->get_set_of()) {
                if (! l.contains(asOperation->get_set_of()))
                    return false;
            }


            break;
        }

        case UmlAttribute: {
//            BrowserAttribute* asAttribute = ((BrowserAttribute *) it.current());
//            if (asAttribute->get_get_oper())
//            {
//                int numReferencesToGetOperations=l.containsRef(asAttribute->get_get_oper());
//                if (numReferencesToGetOperations == 0)
//                    return FALSE;
//            }

//            if (asAttribute->get_set_oper())
//            {
//                int numReferencesToSetOperations=l.containsRef(asAttribute->get_set_oper());
//                if (numReferencesToSetOperations == 0)
//                    return FALSE;
//            }

            break;
        }

        case UmlClass:
        case UmlExtraMember:
            break;

        case UmlDependOn:
            if (((const BrowserNode *) current->parent()) != this)
                return FALSE;

            duplicable = FALSE;
            return TRUE;

        default: {
            bool isNotRelation = !IsaRelation(current->get_type()) ;
            //bool parentIsOtherNode =  (((const BrowserNode *) it.current()->parent()) != this);
            if (isNotRelation/* || parentIsOtherNode*/)
                return FALSE;

            duplicable = true;
            return TRUE;
        }
        }

        if (! may_contains(current, FALSE))
            return FALSE;

        duplicable = may_contains_it(current);
    }

    return TRUE;
}

void BrowserClass::move(BrowserNode * bn, BrowserNode * after)
{
    UmlCode what = bn->get_type();

    if ((what == UmlClass) && !bn->nestedp()) {
        ((BrowserClass *) bn)->def->set_uml_visibility(UmlProtected);
        ((BrowserClass *) bn)->def->set_cpp_visibility(UmlDefaultVisibility);
        ((BrowserClass *) bn)->set_associated_artifact(0);
    }

    BrowserNode * old_parent = ((BrowserNode *) bn->parent());
    char * cpp;
    char * java;
    char * php;
    char * python;

    if ((old_parent != this) && (what == UmlOperation)) {
        OperationData * d = (OperationData *) bn->get_data();

        cpp = d->get_body('c');
        java = d->get_body('j');
        php = d->get_body('p');
        python = d->get_body('y');
        d->create_modified_body_file();
    }
    else
        cpp = java = php = python = 0;

    if ((old_parent != this) &&
            (what == UmlAttribute) &&
            (old_parent->get_type() == UmlClass) &&
            !strcmp(old_parent->get_data()->get_stereotype(), "stereotype") &&
            strcmp(get_data()->get_stereotype(), "stereotype")) // else recompute called
        ProfiledStereotypes::deleted((BrowserAttribute *) bn);

    if (after)
        bn->moveItem(after);
    else {
        bn->parent()->takeItem(bn);
        insertItem(bn);
    }

    if (old_parent != this) {
        old_parent->modified();
        old_parent->package_modified();

        switch (what) {
        case UmlOperation: {
            OperationData * d = (OperationData *) bn->get_data();

            if (cpp) {
                d->new_body(cpp, 'c');
                delete [] cpp;
            }

            if (java) {
                d->new_body(java, 'j');
                delete [] java;
            }

            if (php) {
                d->new_body(php, 'p');
                delete [] php;
            }

            if (python) {
                d->new_body(python, 'y');
                delete [] python;
            }

            if (d->get_is_abstract())
                def->set_is_abstract(TRUE);
        }
            break;

        case UmlAttribute:

            // if the attribute is marked the associated get & set are also moved
            if (! bn->markedp()) {
                if (((BrowserAttribute *) bn)->get_get_oper() != 0)
                    ((BrowserAttribute *) bn)->get_get_oper()->delete_it();

                if (((BrowserAttribute *) bn)->get_set_oper() != 0)
                    ((BrowserAttribute *) bn)->get_set_oper()->delete_it();
            }

            if (!strcmp(get_data()->get_stereotype(), "stereotype"))
                ProfiledStereotypes::added((BrowserAttribute *) bn);

            break;

        default:
            break;
        }

        bn->modified();
    }

    modified();
    package_modified();
}

void BrowserClass::DropEvent(QDropEvent * e)
{
    DropAfterEvent(e, 0);
}

void BrowserClass::DropAfterEvent(QDropEvent * e, BrowserNode * after)
{
    BrowserNode * bn;

    if ((((bn = UmlDrag::decode(e, UmlClass)) != 0) ||
         ((bn = UmlDrag::decode(e, UmlAttribute)) != 0) ||
         ((bn = UmlDrag::decode(e, UmlOperation)) != 0) ||
         ((bn = UmlDrag::decode(e, UmlExtraMember)) != 0) ||
         ((bn = UmlDrag::decode(e, BrowserOperation::drag_key(this))) != 0) ||
         ((bn = UmlDrag::decode(e, BrowserRelation::drag_key(this))) != 0) ||
         ((bn = UmlDrag::decode(e, BrowserSimpleRelation::drag_key(this))) != 0)) &&
            (bn != after) && (bn != this))
    {
        bool a_class = (bn->get_type() == UmlClass);

        if (may_contains(bn, a_class))
        {
            if ((after == 0) && a_class && ((BrowserNode *) parent())->may_contains(bn, TRUE))
            {
                // have choice
                Q3PopupMenu m(0);

                MenuFactory::createTitle(m, TR("move ") + bn->get_name());
                m.insertSeparator();
                m.insertItem(TR("In ") + QString(get_name()), 1);
                m.insertItem(TR("After ") + QString(get_name()), 2);

                switch (m.exec(QCursor::pos())) {
                case 1:
                    break;

                case 2:
                    ((BrowserNode *) parent())->DropAfterEvent(e, this);
                    return;

                default:
                    e->ignore();
                    return;
                }
            }

            move(bn, after);
        }
        else if (after == 0)
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

BrowserClass * BrowserClass::get_class(BrowserNode * future_parent,
                                       const char * stereotype)
{
    BrowserNode * old;
    QString name;
    BrowserNodeList nodes;

    if (!future_parent->enter_child_name(name, TR("enter class's name : "),
                                         UmlClass, instances(nodes), &old,
                                         FALSE, FALSE))
        return 0;

    if (old != 0)
        return ((BrowserClass *) old);

    ClassData * cd = new ClassData();

    BrowserClass * r = new BrowserClass(name, future_parent, cd);

    cd->set_browser_node(r);

    if (stereotype != 0)
        cd->set_stereotype("actor");

    future_parent->setOpen(TRUE);
    future_parent->package_modified();

    return r;
}

BrowserClass * BrowserClass::add_class(bool stereotypep,
                                       BrowserNode * future_parent,
                                       QString name)
{
    if (name.isEmpty()) {
        if (!future_parent->enter_child_name(name,
                                             (stereotypep) ? TR("enter stereotype's name : ")
                                             : TR("enter class's name : "),
                                             UmlClass, FALSE, FALSE))
            return 0;

        QString err;

        if (stereotypep &&
                (!(err = ProfiledStereotypes::canAddStereotype((BrowserClassView *) future_parent,
                                                               name)).isEmpty())) {
            msg_critical(TR("Error"), name + " " + err);
            return 0;
        }
    }

    ClassData * cd = new ClassData();
    BrowserClass * r = new BrowserClass(name, future_parent, cd);

    cd->set_browser_node(r);

    if (stereotypep) {
        cd->set_stereotype("stereotype");
        ProfiledStereotypes::added(r);
    }

    return r;
}


QList<BrowserNode *> BrowserClass::parents() const
{
    QList<BrowserNode *> l;
    Q3ListViewItem * child;

    for (child = firstChild(); child != 0; child = child->nextSibling()) {
        BrowserNode * ch = ((BrowserNode *) child);

        switch (ch->get_type()) {
        case UmlGeneralisation:
        case UmlRealize:
            if (!ch->deletedp())
                l.append(((RelationData *) ch->get_data())->get_end_class());

        default:
            break;
        }
    }

    return l;
}

QString BrowserClass::check_inherit(const BrowserNode * new_parent) const
{
#if 0

    // To allows to inherit sereval times on the same template class
    // there are many problems. The only way is probably to memorize
    // the inheritance relation more/rather than the parent class in
    // the actuals, else it is not possible to update the actuals when
    // the number of formals of an inherited class is changed
    if ((new_parent->get_type() == UmlClass) &&
            (((ClassData *) new_parent->get_data())->get_n_formalparams() != 0))
        // don't check more templates inheritance
        return TRUE;

#endif

    const char * parent_stereotype =
            new_parent->get_data()->get_stereotype();

    if ((!strcmp(parent_stereotype, "stereotype"))
            ? strcmp(def->get_stereotype(), "stereotype")
            : !strcmp(def->get_stereotype(), "stereotype"))
        return TR("one of the two classes is not a stereotype");

    return (!strcmp(parent_stereotype, "union"))
            ? TR("an union can't generalize / realize")
            : BrowserNode::check_inherit(new_parent);
}

QList<BrowserOperation *> BrowserClass::inherited_operations(unsigned limit, QString parent_name) const
{
    QApplication::setOverrideCursor(::Qt::waitCursor);

    QList<BrowserClass *> all_parents;

    get_all_parents(all_parents);

    // computes own operations profile list
    Q3ListViewItem * child;
    Q3Dict<BrowserOperation> already(499);

    for (child = firstChild(); child; child = child->nextSibling()) {
        BrowserNode * childAsNode = dynamic_cast<BrowserNode*>(child);
        BrowserOperation * childAsOperation = dynamic_cast<BrowserOperation*>(child);

        if ((childAsNode->get_type() == UmlOperation) &&
                !childAsNode->deletedp())
            already.insert(childAsOperation->get_data()->definition(TRUE, FALSE),
                           childAsOperation);
    }

    // computes undefined inherited operations list
    QList<BrowserOperation *> l;

    foreach (BrowserClass *cl, all_parents) {
        if(!parent_name.isEmpty() && QString(cl->full_name()) != parent_name)
            continue;

        for (child = cl->firstChild(); child; child = child->nextSibling()) {
            BrowserNode * childAsNode = dynamic_cast<BrowserNode*>(child);
            BrowserOperation * childAsOperation = dynamic_cast<BrowserOperation*>(child);

            if ((childAsNode->get_type() == UmlOperation) &&
                    !childAsNode->deletedp() &&
                    (childAsNode->get_name() != cl->name) &&		// not a constructor
                    (childAsNode->get_name()[0] != '~')) {// not a destructor
                OperationData * data = (OperationData *)
                        childAsOperation->get_data();

                if ((data->get_uml_visibility() != UmlPrivate) &&
                        !data->get_isa_class_operation() &&
                        !data->get_or_set()) {
                    // may be refined
                    QString profile = data->definition(TRUE, FALSE);
                    BrowserOperation * other = already.find(profile);

                    if (other == 0) {
                        l.append(childAsOperation);
                        already.insert(profile, childAsOperation);

                        if (l.count() >= static_cast<int>(limit)) {
                            QApplication::restoreOverrideCursor();

                            return l;
                        }
                    } else if (! data->get_is_abstract() &&
                             ((OperationData *) other->get_data())
                             ->get_is_abstract()) {
                        // to know later if the oper is abstract or not
                        already.replace(profile, childAsOperation);
                        l.remove(other);
                        l.append(childAsOperation);
                    }
                }
            }
        }
    }

    QApplication::restoreOverrideCursor();

    return l;
}

bool BrowserClass::have_abstract_operation()
{
    Q3ListViewItem * child;

    for (child = firstChild(); child != 0; child = child->nextSibling()) {
        BrowserNode * ch = ((BrowserNode *) child);

        if ((ch->get_type() == UmlOperation) &&
                ((OperationData *) ch->get_data())->get_is_abstract())
            return TRUE;
    }

    return FALSE;
}

// unicode
const QStringList & BrowserClass::default_stereotypes()
{
    return its_default_stereotypes;
}

// unicode
const QStringList & BrowserClass::default_stereotypes(UmlCode arrow, const BrowserNode * target) const
{
    switch (target->get_type()) {
    case UmlUseCase:
        return target->default_stereotypes(arrow, this);

    case UmlPackage: {
        static QStringList l;

        l.clear();

        if (arrow == UmlDependOn)
            l.append("import");

        return l;
    }

    case UmlClass:
        if (IsaRelation(arrow))
            return relations_default_stereotypes[arrow];

        // no break;
    default: {
        static QStringList empty;

        return empty;
    }
    }
}

BrowserNodeList & BrowserClass::instances(BrowserNodeList & result,
                                          const char * st, bool non_nested)
{
    IdIterator<BrowserClass> it(all);
    BrowserClass * cl;

    if (non_nested) {
        if ((st == 0) || (*st == 0)) {
            while ((cl = it.current()) != 0) {
                if (!cl->deletedp() && !cl->nestedp())
                    result.append(cl);

                ++it;
            }
        }
        else {
            while ((cl = it.current()) != 0) {
                if (!cl->deletedp() &&
                        !cl->nestedp() &&
                        !strcmp(cl->get_data()->get_stereotype(), st))
                    result.append(cl);

                ++it;
            }
        }
    }
    else {
        if ((st == 0) || (*st == 0)) {
            while ((cl = it.current()) != 0) {
                if (!cl->deletedp())
                    result.append(cl);

                ++it;
            }
        }
        else {
            while ((cl = it.current()) != 0) {
                if (!cl->deletedp() && !strcmp(cl->get_data()->get_stereotype(), st))
                    result.append(cl);

                ++it;
            }
        }
    }

    result.sort_it();

    return result;
}

BrowserNode * BrowserClass::get_associated() const
{
    return associated_diagram;
}

void BrowserClass::set_associated_diagram(BrowserClassDiagram * dg,
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

BrowserArtifact * BrowserClass::get_associated_artifact() const
{
    return associated_artifact;
}

void BrowserClass::set_associated_artifact(BrowserArtifact * a,
                                           bool on_read)
{
    if (associated_artifact != a) {
        if (associated_artifact != 0) {
            BrowserArtifact * old = associated_artifact;

            associated_artifact = 0;	// to not loop until end of days

            if (! old->deletedp())
                old->remove_associated_class(this, on_read);
        }

        if ((associated_artifact = a) != 0)
            a->add_associated_class(this, on_read);

        if (! on_read)
            // to upgrade drawing context
            // not done on read to not remove actuals because
            // inherited class is not yet read
            def->modified();
    }
}

const Q3ValueList<BrowserComponent *> & BrowserClass::get_associated_components() const
{
    return associated_components;
}

void BrowserClass::add_associated_component(BrowserComponent * c)
{
    if (associated_components.findIndex(c) == -1) {
        associated_components.append(c);
        QObject::connect(c->get_data(), SIGNAL(deleted()),
                         def, SLOT(on_delete()));
    }
}

void BrowserClass::remove_associated_component(BrowserComponent * c)
{
    Q3ValueList<BrowserComponent *>::Iterator it = associated_components.find(c);

    if (it != associated_components.end()) {
        associated_components.remove(it);
        QObject::disconnect(c->get_data(), SIGNAL(deleted()),
                            def, SLOT(on_delete()));
    }
}

void BrowserClass::on_delete()
{
    if (associated_diagram && associated_diagram->deletedp())
        associated_diagram = 0;

    Q3ValueList<BrowserComponent *>::Iterator it = associated_components.begin();

    while (it != associated_components.end()) {
        if ((*it)->deletedp())
            it = associated_components.remove(it);
        else
            it++;
    }
}

void BrowserClass::init()
{
    its_default_stereotypes.clear();
    its_default_stereotypes.append("actor");
    its_default_stereotypes.append("auxiliary");
    its_default_stereotypes.append("boundary");
    its_default_stereotypes.append("control");
    its_default_stereotypes.append("entity");
    its_default_stereotypes.append("enum");
    its_default_stereotypes.append("enum_pattern");
    its_default_stereotypes.append("exception");
    its_default_stereotypes.append("focus");
    its_default_stereotypes.append("implementationClass");
    its_default_stereotypes.append("interface");
    its_default_stereotypes.append("@interface");
    its_default_stereotypes.append("metaclass");
    its_default_stereotypes.append("stereotype");
    its_default_stereotypes.append("struct");
    its_default_stereotypes.append("type");
    its_default_stereotypes.append("typedef");
    its_default_stereotypes.append("union");
    its_default_stereotypes.append("utility");

    int r;

    for (r = 0; r != UmlRelations; r += 1) {
        relations_default_stereotypes[r].clear();

        switch (r) {
        case UmlGeneralisation:
            relations_default_stereotypes[r].append("{complete,disjoint}");
            relations_default_stereotypes[r].append("{incomplete,disjoint}");
            relations_default_stereotypes[r].append("{complete,overlapping}");
            relations_default_stereotypes[r].append("{incomplete,overlapping}");
            break;

        case UmlDependency:
            relations_default_stereotypes[r].append("friend");
            relations_default_stereotypes[r].append("from");
            relations_default_stereotypes[r].append("import");
            relations_default_stereotypes[r].append("instantiate");
            break;

        case UmlRealize:
            relations_default_stereotypes[r].append("bind");
            break;

        default:
            // relations corresponding to attribute
            relations_default_stereotypes[r].append("list");
            relations_default_stereotypes[r].append("set");
            relations_default_stereotypes[r].append("vector");
            relations_default_stereotypes[r].append("map");
        }
    }

    BrowserAttribute::init();
    ProfiledStereotypes::init();
}

bool BrowserClass::tool_cmd(ToolCom * com, const char * args)
{
    ClassData::ToolCmd dummy;

    switch ((unsigned char) args[-1]) {
    case createCmd: {
        bool ok = TRUE;

        if (is_read_only && !root_permission())
            ok = FALSE;
        else {
            switch (com->get_kind(args)) {
            case UmlAttribute:
                if (wrong_child_name(args, UmlAttribute, FALSE, FALSE))
                    ok = FALSE;
                else
                    (BrowserAttribute::new_one(args, this,
                                               !strcmp(def->get_stereotype(), "enum") ||
                                               !strcmp(def->get_stereotype(), "enum_pattern")))
                        ->write_id(com);

                break;

            case UmlOperation:
                if (wrong_child_name(args, UmlOperation, FALSE, FALSE))
                    ok = FALSE;
                else
                    (BrowserOperation::new_one(args, this))->write_id(com);

                break;

            case UmlRelations: {
                UmlCode c;

                if (!com->get_relation_kind(c, args))
                    ok = FALSE;
                else {
                    BrowserClass * end = (BrowserClass *) com->get_id(args);

                    if (may_start(c).isEmpty() && may_connect(c, end).isEmpty()) {
                        switch (c) {
                        case UmlGeneralisation:
                        case UmlRealize:
                            ClassData::setDontUpdateActuals(FALSE);
                            break;

                        default:
                            break;
                        }

                        ((RelationData *) add_relation(c, end))->get_start()->write_id(com);
                    }
                    else
                        ok = FALSE;
                }
            }
                break;

            case UmlSimpleRelations: {
                UmlCode c;

                if (!com->get_relation_kind(c, args))
                    ok = FALSE;
                else {
                    BrowserNode * end = (BrowserNode *) com->get_id(args);

                    if (end->get_type() == UmlPackage)
                        add_relation(UmlDependOn, end)->get_browser_node()->write_id(com);
                    else
                        ok = FALSE;
                }
            }
                break;

            case UmlExtraMember:
                (BrowserExtraMember::new_one(args, this))->write_id(com);
                break;

            case UmlClass:
                if (wrong_child_name(args, UmlClass, FALSE, FALSE))
                    ok = FALSE;
                else {
                    (BrowserClass::add_class(FALSE, this, args))->write_id(com);
                    package_modified();
                    // def unmodified
                    return TRUE;
                }

                break;

            default:
                ok = FALSE;
            }
        }

        if (! ok)
            com->write_id(0);
        else {
            def->modified();
            package_modified();
        }

        return TRUE;
    }

    case setAssocDiagramCmd:
        if (is_read_only && !root_permission())
            com->write_ack(FALSE);
        else {
            set_associated_diagram((BrowserClassDiagram *) com->get_id(args));
            com->write_ack(TRUE);
        }

        return TRUE;

    case assocArtifactCmd:
        if ((associated_artifact == 0) || associated_artifact->deletedp())
            com->write_id(0);
        else
            associated_artifact->write_id(com);

        return TRUE;

    case assocComponentCmd:
        if (com->api_format() < 17) {
            if (!associated_components.isEmpty())
                associated_components.first()->write_id(com);
            else
                com->write_id(0);
        }
        else {
            Q3ValueList<BrowserComponent *>::Iterator it;

            com->write_unsigned(associated_components.count());

            for (it = associated_components.begin();
                 it != associated_components.end();
                 it++)
                (*it)->write_id(com);
        }

        return TRUE;

    case setUserCmd:

        // plug-out upgrade, limited checks
        if (!is_read_only || root_permission()) {
            all.remove(get_ident());
            new_ident(com->get_unsigned(args), all);
            package_modified();
        }

        return TRUE;

    default: {
        QString s = name;
        bool result = (def->tool_cmd(com, args, this, comment) ||
                       BrowserNode::tool_cmd(com, args));

        if (s != name)
            // to update inherit/realize/dependency names
            BrowserView::get_project()->update_stereotype(TRUE);

        return result;
    }
    }
}

bool BrowserClass::tool_global_cmd(ToolCom * com, const char * args)
{
    switch ((unsigned char) args[-1]) {
    case findClassCmd: {
        BrowserNode * p = (BrowserNode *) com->get_id(args);	// package

        if (p != 0) {
            Q3ListViewItem * child;

            for (child = p->firstChild(); child != 0; child = child->nextSibling()) {
                // search a class view
                if (!((BrowserNode *) child)->deletedp() &&
                        (((BrowserNode *) child)->get_type() == UmlClassView)) {
                    Q3ListViewItem * ch;

                    for (ch = child->firstChild(); ch != 0; ch = ch->nextSibling()) {
                        // search the class
                        if (!((BrowserNode *) ch)->deletedp() &&
                                (((BrowserNode *) ch)->get_type() == UmlClass) &&
                                (((BrowserClass *) ch)->name == args)) {
                            ((BrowserClass *) ch)->write_id(com);
                            return TRUE;
                        }
                    }
                }
            }
        }
        else {
            IdIterator<BrowserClass> it(all);
            BrowserClass * cl;

            while ((cl = it.current()) != 0) {
                if (!cl->deletedp() && cl->name == args) {
                    cl->write_id(com);
                    return TRUE;
                }

                ++it;
            }
        }

        com->write_id(0);
        return TRUE;
    }

    default:
        return FALSE;
    }
}

void BrowserClass::save_stereotypes(QTextStream & st)
{
    nl_indent(st);
    st << "class_stereotypes ";
    save_unicode_string_list(its_default_stereotypes, st);
    BrowserNode::save_stereotypes(st, relations_default_stereotypes);
}

void BrowserClass::read_stereotypes(char *& st, char *& k)
{
    if (!strcmp(k, "class_stereotypes")) {
        read_unicode_string_list(its_default_stereotypes, st);
        BrowserNode::read_stereotypes(st, relations_default_stereotypes);
        k = read_keyword(st);

        // force stereotype
        if (its_default_stereotypes.findIndex("stereotype") == -1)
            its_default_stereotypes.append("stereotype");
    }
    else
        init();
}

void BrowserClass::save(QTextStream & st, bool ref, QString & warning)
{
    if (ref)
        st << "class_ref " << get_ident() << " // " << get_name();
    else {
        nl_indent(st);
        st << "class " << get_ident() << " ";
        save_string(name, st);

        indent(+1);
        def->save(st, warning);

        if (associated_diagram != 0) {
            nl_indent(st);
            st << "associated_diagram ";
            associated_diagram->save(st, TRUE, warning);
        }

        BrowserNode::save(st);

        // bodies file
        QString bodyfn;
        QFile qf;
        char * modified_bodies = (def->get_bodies_modified())
                ? read_definition(get_ident(), "b")
                : 0;

        // saves the sub elts & operations body

        Q3ListViewItem * child = firstChild();

        if (child != 0) {
            for (;;) {
                if (!((BrowserNode *) child)->deletedp()) {
                    ((BrowserNode *) child)->save(st, FALSE, warning);

                    if ((((BrowserNode *) child)->get_type() == UmlOperation) &&
                            (modified_bodies != 0)) {
                        OperationData * od =
                                (OperationData *)((BrowserNode *) child)->get_data();

                        od->save_body(qf, bodyfn, TRUE, modified_bodies, 'c');
                        od->save_body(qf, bodyfn, TRUE, modified_bodies, 'j');
                        od->save_body(qf, bodyfn, TRUE, modified_bodies, 'p');
                        od->save_body(qf, bodyfn, TRUE, modified_bodies, 'y');
                    }

                    child = child->nextSibling();

                    if (child != 0)
                        st << '\n';
                    else
                        break;
                }
                else {
                    // set 'not modified' to delete the associated file on exit
                    ((BrowserNode *) child)->unmodified();

                    if (((BrowserNode *) child)->get_type() == UmlOperation)
                        ((OperationData *)((BrowserNode *) child)->get_data())
                            ->raz_body();

                    if ((child = child->nextSibling()) == 0)
                        break;
                }
            }

            if (!bodyfn.isEmpty()) {
                qf.close();

                if (static_cast<uint>(qf.status()) != IO_Ok) {
                    // error, redo
                    for (;;) {
                        (void) msg_critical(TR("Error"),
                                            TR("Error while writting in\n%1\nmay be your disk is full", bodyfn),
                                            QMessageBox::Retry);

                        QFile qf2;

                        for (child = firstChild(); child; child = child->nextSibling()) {
                            if (!((BrowserNode *) child)->deletedp() &&
                                    (((BrowserNode *) child)->get_type() == UmlOperation)) {
                                OperationData * od =
                                        (OperationData *)((BrowserNode *) child)->get_data();

                                od->save_body(qf2, bodyfn, FALSE, modified_bodies, 'c');
                                od->save_body(qf2, bodyfn, FALSE, modified_bodies, 'j');
                                od->save_body(qf2, bodyfn, FALSE, modified_bodies, 'p');
                                od->save_body(qf2, bodyfn, FALSE, modified_bodies, 'y');
                            }
                        }

                        qf2.close();

                        if (static_cast<uint>(qf2.status()) == IO_Ok)
                            // all is ok
                            break;
                    }
                }
            }
        }

        delete [] modified_bodies;

        if (def->get_bodies_modified()) {
            delete_definition(get_ident(), "b");
            def->set_bodies_modified(FALSE);
        }

        indent(-1);
        nl_indent(st);
        st << "end";

        // for saveAs
        if (!is_from_lib() && !is_api_base())
            is_read_only = FALSE;
    }
}

bool BrowserClass::get_bodies_read()
{
    return def->get_bodies_read();
}

void BrowserClass::set_bodies_read(bool y)
{
    def->set_bodies_read(y);
}

bool BrowserClass::get_bodies_modified()
{
    return def->get_bodies_modified();
}

void BrowserClass::set_bodies_modified(bool y)
{
    def->set_bodies_modified(y);
}

QString BrowserClass::bodies_file() const
{
    QString fn;

    fn.sprintf("%d.bodies", get_ident());

    QDir d = BrowserView::get_dir();

    return d.absFilePath(fn);
}

BrowserClass * BrowserClass::find(const char * s)
{
    IdIterator<BrowserClass> it(all);
    BrowserClass * cl;

    while ((cl = it.current()) != 0) {
        if (strcmp(cl->name, s) == 0)
            return cl;

        ++it;
    }

    return 0;
}

// translate plug out release < 2.0 ... if it is a plug out

static const char * component2artifact(const char * b)
{
    static const char * t[][2] = {
        { "ComponentView", "DeploymentView" },
        { "ComponentDiagram", "DeploymentDiagram" },
        { "aComponent", "anArtifact" },
        { "Component", "Artifact" }
    };
    static WrapperStr s;

    s = b;

    for (int index1 = 0; index1 != sizeof(t) / sizeof(*t); index1 += 1) {
        const char * o = t[index1][0];
        const char * n = t[index1][1];
        int l = strlen(o);
        int index2 = 0;
        int index3;

        while ((index3 = s.find(o, index2)) != -1) {
            s.replace(index3, l, n);
            index2 = index3;
        }
    }

    return s;
}

void BrowserClass::plug_out_conversion()
{
    BrowserClass * baseArtifact = 0;	// all vars set to 0 to avoid
    BrowserClass * artifact = 0;		// stupid g++ 4 warnings
    BrowserArtifact * baseArtifactArtifact = 0;
    BrowserArtifact * artifactArtifact = 0;
    BrowserClass * deploymentView = 0;
    BrowserClass * deploymentDiagram = 0;
    BrowserClass * baseClass = 0;
    BrowserClass * baseClassView = 0;
    BrowserClass * baseItem = 0;
    BrowserClass * item = 0;
    BrowserArtifact * baseItemArtifact = 0;
    BrowserClass * onInstanceCmd = 0;
    BrowserClass * anItemKind = 0;
    BrowserClass * com = 0;
    BrowserArtifact * comArtifact = 0;

    if (((baseArtifact = find("UmlBaseComponent")) != 0) &&
            (find("UmlBaseArtifact") == 0) &&
            ((artifact = find("UmlComponent")) != 0) &&
            ((baseArtifactArtifact = BrowserArtifact::find("UmlBaseComponent")) != 0) &&
            ((artifactArtifact = BrowserArtifact::find("UmlComponent")) != 0) &&
            ((deploymentView = find("UmlDeploymentView")) != 0) &&
            ((deploymentDiagram = find("UmlDeploymentDiagram")) != 0) &&
            ((baseClass = find("UmlBaseClass")) != 0) &&
            ((baseClassView = find("UmlBaseClassView")) != 0) &&
            ((item = find("UmlItem")) != 0) &&
            ((baseItem = find("UmlBaseItem")) != 0) &&
            ((baseItemArtifact = BrowserArtifact::find("UmlBaseItem")) != 0) &&
            ((onInstanceCmd = find("OnInstanceCmd")) != 0) &&
            ((anItemKind = find("anItemKind")) != 0) &&
            ((com = find("UmlCom")) != 0) &&
            ((comArtifact = BrowserArtifact::find("UmlCom")) != 0)) {
        // do conversion
        int uid = user_id();
        Q3ListViewItem * child;
        WrapperStr s;
        int index;
        AType t_deplview;
        AType t_depldiag;
        AType t_artifact;
        char * b;
        OperationData * op1;
        OperationData * op2;

        if (uid == 0)
            uid = 1;

        BrowserPackage::update_idmax_for_root();

        t_deplview.type = deploymentView;
        t_depldiag.type = deploymentDiagram;
        t_artifact.type = artifact;

        // create UmlBaseComponent class and artifact
        set_user_id(0);

        BrowserClass * baseComponent =
                new BrowserClass(baseArtifact, (BrowserNode *) baseArtifact->parent());
        BrowserArtifact * baseComponentArtifact =
                new BrowserArtifact(baseArtifactArtifact,
                                    (BrowserNode *) baseArtifactArtifact->parent());

        baseComponent->moveItem(baseArtifact);
        baseComponentArtifact->moveItem(baseArtifactArtifact);

        baseComponent->set_comment(baseArtifact->get_comment());
        baseComponentArtifact->set_comment(baseArtifactArtifact->get_comment());
        baseComponentArtifact->get_data()->set_stereotype("source");
        baseComponentArtifact->add_associated_class(baseComponent, TRUE);

        // create UmlComponent class and artifact
        set_user_id(uid);

        BrowserClass * component =
                new BrowserClass(artifact, (BrowserNode *) artifact->parent());
        BrowserArtifact * componentArtifact =
                new BrowserArtifact(artifactArtifact,
                                    (BrowserNode *) artifactArtifact->parent());

        component->moveItem(artifact);
        componentArtifact->moveItem(artifactArtifact);

        component->set_comment(artifact->get_comment());
        componentArtifact->set_comment(artifactArtifact->get_comment());
        componentArtifact->get_data()->set_stereotype("source");
        componentArtifact->add_associated_class(component, TRUE);

        AType t_component;

        t_component.type = component;

        // manage UmlBaseArtifact
        set_user_id(0);

        baseArtifact->set_name("UmlBaseArtifact");
        baseArtifact->package_modified();
        baseArtifact->set_comment("Manage the artifacts");

        for (child = baseArtifact->firstChild(); child != 0; child = child->nextSibling()) {
            if (((BrowserNode *) child)->get_type() == UmlOperation) {
                if (strcmp(((BrowserNode *) child)->get_name(), "UmlBaseComponent") == 0) {
                    // UmlBaseComponent constructor
                    BrowserOperation * baseComponentCreation =
                            new BrowserOperation((BrowserOperation *) child, baseComponent);

                    set_user_id(uid);	// to create saved  *_<n>.b file name
                    op1 = (OperationData *)((BrowserNode *) child)->get_data();
                    op2 = (OperationData *) baseComponentCreation->get_data();

                    if ((b = op1->get_body('c')) != 0) {
                        op2->new_body(b, 'c');
                        delete [] b;
                    }

                    if ((b = op1->get_body('j')) != 0) {
                        op2->new_body(b, 'j');
                        delete [] b;
                    }

                    baseComponentCreation->set_comment(((BrowserNode *) child)->get_comment());
                    set_user_id(0);

                    // UmlBaseArtifact constructor
                    ((BrowserNode *) child)->set_name("UmlBaseArtifact");
                }
                else if (strcmp(((BrowserNode *) child)->get_name(), "create") == 0) {
                    // UmlBaseComponent create operation
                    BrowserOperation * baseComponentCreate =
                            new BrowserOperation((BrowserOperation *) child, baseComponent);

                    set_user_id(uid);	// to create saved  *_<n>.b file name
                    op1 = (OperationData *) baseComponentCreate->get_data();
                    op2 = (OperationData *)((BrowserNode *) child)->get_data();

                    if ((b = op2->get_body('c')) != 0) {
                        op1->new_body(b, 'c');
                        op2->new_body(component2artifact(b), 'c');
                        delete [] b;
                    }

                    if ((b = op2->get_body('j')) != 0) {
                        op1->new_body(b, 'j');
                        op2->new_body(component2artifact(b), 'j');
                        delete [] b;
                    }

                    baseComponentCreate->set_comment(((BrowserNode *) child)->get_comment());
                    op1->set_return_type(t_component);
                    op2->set_param_type(0, t_deplview);
                    set_user_id(0);
                }
                else if (strcmp(((BrowserNode *) child)->get_name(), "kind") == 0) {
                    // UmlBaseComponent kind operation
                    BrowserOperation * baseComponentKind =
                            new BrowserOperation((BrowserOperation *) child, baseComponent);

                    set_user_id(uid);	// to create saved  *_<n>.b file name
                    op1 = (OperationData *) baseComponentKind->get_data();
                    op2 = (OperationData *)((BrowserNode *) child)->get_data();

                    if ((b = op2->get_body('c')) != 0) {
                        op1->new_body(b, 'c');
                        op2->new_body(component2artifact(b), 'c');
                        delete [] b;
                    }

                    if ((b = op2->get_body('j')) != 0) {
                        op1->new_body(b, 'j');
                        op2->new_body(component2artifact(b), 'j');
                        delete [] b;
                    }

                    baseComponentKind->set_comment(((BrowserNode *) child)->get_comment());
                    set_user_id(0);
                }
                else if (strcmp(((BrowserNode *) child)->get_name(), "read_uml_") == 0) {
                    // UmlBaseComponent read_uml_ operation
                    BrowserOperation * baseComponentRead_uml_ =
                            new BrowserOperation((BrowserOperation *) child, baseComponent);

                    set_user_id(uid);	// to create saved  *_<n>.b file name
                    op1 = (OperationData *) baseComponentRead_uml_->get_data();
                    op1->new_body("  _assoc_diagram = (UmlComponentDiagram *) UmlBaseItem::read_();\n\
                                  UmlBaseItem::read_uml_();\n", 'c');
                            op1->new_body("  _assoc_diagram = (UmlComponentDiagram) UmlBaseItem.read_();\n\
                                          super.read_uml_();\n", 'j');
                            baseComponentRead_uml_->set_comment(((BrowserNode *) child)->get_comment());

                    // UmlBaseArtifact read_uml_ operation
                    op2 = (OperationData *)((BrowserNode *) child)->get_data();

                    if ((b = op2->get_body('c')) != 0) {
                        op2->new_body(component2artifact(b), 'c');
                        delete [] b;
                    }

                    if ((b = op2->get_body('j')) != 0) {
                        op2->new_body(component2artifact(b), 'j');
                        delete [] b;
                    }

                    set_user_id(0);
                }
                else if (strcmp(((BrowserNode *) child)->get_name(), "associatedDiagram") == 0) {
                    // UmlBaseComponent associatedDiagram operation
                    BrowserOperation * baseComponentAssociatedDiagram =
                            new BrowserOperation((BrowserOperation *) child, baseComponent);

                    set_user_id(uid);	// to create saved  *_<n>.b file name
                    op1 = (OperationData *) baseComponentAssociatedDiagram->get_data();
                    op2 = (OperationData *)((BrowserNode *) child)->get_data();

                    if ((b = op2->get_body('c')) != 0) {
                        op1->new_body(b, 'c');
                        delete [] b;
                    }

                    if ((b = op2->get_body('j')) != 0) {
                        op1->new_body(b, 'j');
                        delete [] b;
                    }

                    baseComponentAssociatedDiagram->set_comment(((BrowserNode *) child)->get_comment());

                    op2->set_return_type(t_depldiag);
                    set_user_id(0);
                }
                else if (strcmp(((BrowserNode *) child)->get_name(), "set_AssociatedDiagram") == 0) {
                    // UmlBaseComponent set_AssociatedDiagram operation
                    BrowserOperation * baseComponentSet_AssociatedDiagram =
                            new BrowserOperation((BrowserOperation *) child, baseComponent);

                    set_user_id(uid);	// to create saved  *_<n>.b file name
                    op1 = (OperationData *) baseComponentSet_AssociatedDiagram->get_data();
                    op2 = (OperationData *)((BrowserNode *) child)->get_data();

                    if ((b = op2->get_body('c')) != 0) {
                        op1->new_body(b, 'c');
                        delete [] b;
                    }

                    if ((b = op2->get_body('j')) != 0) {
                        op1->new_body(b, 'j');
                        delete [] b;
                    }

                    baseComponentSet_AssociatedDiagram->set_comment(((BrowserNode *) child)->get_comment());

                    op2->set_param_type(0, t_depldiag);
                    set_user_id(0);
                }
                else if (strcmp(((BrowserNode *) child)->get_name(), "associatedComponents") == 0) {
                    ((BrowserNode *) child)->set_name("associatedArtifacts");
                    set_user_id(uid);	// to create saved  *_<n>.b file name
                    op1 = (OperationData *)((BrowserNode *) child)->get_data();

                    if ((b = op1->get_body('c')) != 0) {
                        op1->new_body(component2artifact(b), 'c');
                        delete [] b;
                    }

                    if ((b = op1->get_body('j')) != 0) {
                        op1->new_body(component2artifact(b), 'j');
                        delete [] b;
                    }

                    set_user_id(0);
                }
                else if (strcmp(((BrowserNode *) child)->get_name(), "addAssociatedComponent") == 0) {
                    ((BrowserNode *) child)->set_name("addAssociatedArtifact");
                    set_user_id(uid);	// to create saved  *_<n>.b file name
                    op1 = (OperationData *)((BrowserNode *) child)->get_data();
                    op1->set_param_type(0, t_artifact);

                    if ((b = op1->get_body('c')) != 0) {
                        op1->new_body(component2artifact(b), 'c');
                        delete [] b;
                    }

                    if ((b = op1->get_body('j')) != 0) {
                        op1->new_body(component2artifact(b), 'j');
                        delete [] b;
                    }

                    set_user_id(0);
                }
                else if (strcmp(((BrowserNode *) child)->get_name(), "removeAssociatedComponent") == 0) {
                    ((BrowserNode *) child)->set_name("removeAssociatedArtifact");
                    set_user_id(uid);	// to create saved  *_<n>.b file name
                    op1 = (OperationData *)((BrowserNode *) child)->get_data();
                    op1->set_param_type(0, t_artifact);

                    if ((b = op1->get_body('c')) != 0) {
                        op1->new_body(component2artifact(b), 'c');
                        delete [] b;
                    }

                    if ((b = op1->get_body('j')) != 0) {
                        op1->new_body(component2artifact(b), 'j');
                        delete [] b;
                    }

                    set_user_id(0);
                }
                else if (strcmp(((BrowserNode *) child)->get_name(), "removeAllAssociatedComponents") == 0) {
                    ((BrowserNode *) child)->set_name("removeAllAssociatedArtifacts");
                    set_user_id(uid);	// to create saved  *_<n>.b file name
                    op1 = (OperationData *)((BrowserNode *) child)->get_data();

                    if ((b = op1->get_body('c')) != 0) {
                        op1->new_body(component2artifact(b), 'c');
                        delete [] b;
                    }

                    if ((b = op1->get_body('j')) != 0) {
                        op1->new_body(component2artifact(b), 'j');
                        delete [] b;
                    }

                    set_user_id(0);
                }

                // update operation comment
                s = ((BrowserNode *) child)->get_comment();
                index = 0;

                while ((index = s.find("component", index)) != -1) {
                    s.replace(index, 9, "artifact");
                    index += 8;
                }

                index = 0;

                while ((index = s.find("componant", index)) != -1) {	// this fault exist
                    s.replace(index, 9, "artifact");
                    index += 8;
                }

                ((BrowserNode *) child)->set_comment(s);
            }
            else if (((BrowserNode *) child)->get_type() == UmlGeneralisation) {
                RelationData * r = (RelationData *)
                        ((BrowserNode *) child)->get_data();

                if (r->get_end_class() == item)
                    ((RelationData *) baseComponent->add_relation(UmlGeneralisation, item))
                        ->copy(r);
            }
            else if (((BrowserNode *) child)->get_type() == UmlDirectionalAssociation) {
                RelationData * r = (RelationData *)((BrowserNode *) child)->get_data();

                if (strcmp(r->get_role_a(), "_assoc_diagram") == 0) {
                    ((RelationData *) baseComponent->add_relation(UmlDirectionalAssociation,
                                                                  r->get_end_class()))
                            ->copy(r);
                    r->unidir_change_dest(deploymentDiagram);
                }
            }
        }

        baseArtifactArtifact->package_modified();
        baseArtifactArtifact->set_name("UmlBaseArtifact");

        // manage UmlBaseClass
        baseClass->package_modified();

        for (child = baseClass->firstChild(); child != 0; child = child->nextSibling()) {
            if (((BrowserNode *) child)->get_type() == UmlOperation) {
                if (strcmp(((BrowserNode *) child)->get_name(), "associatedComponent") == 0) {
                    ((BrowserNode *) child)->set_name("associatedArtifact");
                    ((BrowserNode *) child)->set_comment(" returns the optional associated artifact,\n\
                                                         to set it refer to the UmlBaseArtifact's operations\n\
                                                         addAssociatedClass(), removeAssociatedClass() and\n\
                                                         setAssociatedClasses()");

                                                         set_user_id(uid);	// to create saved  *_<n>.b file name
                            op1 = (OperationData *)((BrowserNode *) child)->get_data();

                    if ((b = op1->get_body('c')) != 0) {
                        op1->new_body(component2artifact(b), 'c');
                        delete [] b;
                    }

                    if ((b = op1->get_body('j')) != 0) {
                        op1->new_body(component2artifact(b), 'j');
                        delete [] b;
                    }
                }
            }
        }

        // manage UmlBaseClassView
        baseClassView->package_modified();

        for (child = baseClassView->firstChild(); child != 0; child = child->nextSibling()) {
            if (((BrowserNode *) child)->get_type() == UmlOperation) {
                if (strcmp(((BrowserNode *) child)->get_name(), "associatedComponentView") == 0) {
                    ((BrowserNode *) child)->set_name("associatedDeploymentView");
                    ((BrowserNode *) child)->set_comment(" returns the optional associated deployment view");

                    op1 = (OperationData *)((BrowserNode *) child)->get_data();
                    op1->set_return_type(t_deplview);
                }
                else if (strcmp(((BrowserNode *) child)->get_name(), "set_AssociatedComponentView") == 0) {
                    ((BrowserNode *) child)->set_name("set_AssociatedDeploymentView");
                    ((BrowserNode *) child)->set_comment(" sets the associated deployment view, arg may be null to unset it\n\
                                                         \n\
                                                         On error return FALSE in C++, produce a RuntimeException in Java\n");

                                                         op1 = (OperationData *)((BrowserNode *) child)->get_data();
                            op1->set_param_type(0, t_deplview);
                }
                else if (strcmp(((BrowserNode *) child)->get_name(), "read_uml_") == 0) {
                    set_user_id(uid);	// to create saved  *_<n>.b file name
                    op1 = (OperationData *)((BrowserNode *) child)->get_data();

                    if ((b = op1->get_body('c')) != 0) {
                        op1->new_body(component2artifact(b), 'c');
                        delete [] b;
                    }

                    if ((b = op1->get_body('j')) != 0) {
                        op1->new_body(component2artifact(b), 'j');
                        delete [] b;
                    }
                }
            }
            else if (((BrowserNode *) child)->get_type() == UmlDirectionalAssociation) {
                RelationData * r = (RelationData *)((BrowserNode *) child)->get_data();

                if (strcmp(r->get_role_a(), "_assoc_view") == 0)
                    r->unidir_change_dest(deploymentView);
            }
        }

        // manage UmlBaseItem
        baseItem->package_modified();

        for (child = baseItem->firstChild(); child != 0; child = child->nextSibling()) {
            if (((BrowserNode *) child)->get_type() == UmlOperation) {
                if (strcmp(((BrowserNode *) child)->get_name(), "read_") == 0) {
                    set_user_id(uid);	// to create saved  *_<n>.b file name
                    op1 = (OperationData *)((BrowserNode *) child)->get_data();

                    if ((b = op1->get_body('c')) != 0) {
                        s = b;
                        delete [] b;
                        s.insert(s.find("case aNode:"),
                                 "case anArtifact:\n\
                                 result = new UmlArtifact(id, name);\n\
                                break;\n    ");
                                op1->new_body(s, 'c');
                    }

                    if ((b = op1->get_body('j')) != 0) {
                        s = b;
                        delete [] b;
                        s.insert(s.find("case anItemKind._aNode:"),
                                 "case anItemKind._anArtifact:\n\
                                 return new UmlArtifact(id, name);\n    ");
                                op1->new_body(s, 'j');
                    }

                    set_user_id(0);
                }
            }
            else if (((BrowserNode *) child)->get_type() == UmlDependency) {
                RelationData * r = (RelationData *)
                        ((BrowserNode *) child)->get_data();

                if (r->get_end_class() == baseArtifact)
                    ((RelationData *) baseItem->add_relation(UmlDependency, baseComponent))
                        ->copy(r);
            }
        }

        baseItemArtifact->package_modified();
        ((ArtifactData *) baseItemArtifact->get_data())->convert_add_include_artifact();

        // manage onInstanceCmd
        BrowserAttribute * a = 0;

        onInstanceCmd->package_modified();

        for (child = onInstanceCmd->firstChild(); child != 0; child = child->nextSibling()) {
            if (((BrowserNode *) child)->get_type() == UmlAttribute) {
                if (strcmp(((BrowserNode *) child)->get_name(), "assocComponentCmd") == 0) {
                    a = (BrowserAttribute *)
                            ((BrowserAttribute *) child)->duplicate(onInstanceCmd, "assocArtifactCmd");
                    ((AttributeData *) a->get_data())->set_visibility(UmlPublic);
                    a->moveItem(child);
                }
                else if (strcmp(((BrowserNode *) child)->get_name(), "addAssocComponentCmd") == 0)
                    ((BrowserNode *) child)->set_name("addAssocArtifactCmd");
                else if (strcmp(((BrowserNode *) child)->get_name(), "removeAssocComponentCmd") == 0)
                    ((BrowserNode *) child)->set_name("removeAssocArtifactCmd");
                else if (strcmp(((BrowserNode *) child)->get_name(), "removeAllAssocComponentsCmd") == 0)
                    ((BrowserNode *) child)->set_name("removeAllAssocArtifactsCmd");
            }
        }

        if (a != 0) {
            // manage anItemKind
            a = (BrowserAttribute *) a->duplicate(anItemKind, "anArtifact");
            ((AttributeData *) a->get_data())->set_visibility(UmlPublic);

            for (child = anItemKind->firstChild(); child != 0; child = child->nextSibling()) {
                if (strcmp(((BrowserNode *) child)->get_name(), "aNode") == 0) {
                    a->moveItem(child);
                    break;
                }
            }
        }

        // manage UmlCom
        com->package_modified();

        for (child = com->firstChild(); child != 0; child = child->nextSibling()) {
            if ((((BrowserNode *) child)->get_type() == UmlOperation) &&
                    (strcmp(((BrowserNode *) child)->get_name(), "connect") == 0)) {
                set_user_id(uid);	// to create saved  *_<n>.b file name
                op1 = (OperationData *)((BrowserNode *) child)->get_data();

                if ((b = op1->get_body('c')) != 0) {
                    s = b;
                    delete [] b;
                    s.insert(s.find("write_unsigned(") + 15, "1");
                    op1->new_body(s, 'c');
                }

                if ((b = op1->get_body('j')) != 0) {
                    s = b;
                    delete [] b;
                    s.insert(s.find("write_unsigned(") + 15, "1");
                    op1->new_body(s, 'j');
                }

                set_user_id(0);
                break;
            }
        }

        // manage UmlArtifact
        set_user_id(uid);

        artifact->set_name("UmlArtifact");
        artifact->package_modified();
        s = artifact->get_comment();

        if ((index = s.find("'components'")) != -1) {
            s.replace(index + 1, 9, "artifact");
            artifact->set_comment(s);
        }

        for (child = artifact->firstChild(); child != 0; child = child->nextSibling()) {
            if (((BrowserNode *) child)->get_type() == UmlOperation) {
                if (strcmp(((BrowserNode *) child)->get_name(), "UmlComponent") == 0) {
                    BrowserOperation * componentCreate =
                            new BrowserOperation((BrowserOperation *) child, component);

                    ((BrowserNode *) child)->set_name("UmlArtifact");
                    componentCreate->set_comment(((BrowserNode *) child)->get_comment());

                    OperationData::convert((OperationData *) componentCreate->get_data(),
                                           (OperationData *)((BrowserNode *) child)->get_data());
                }
                else if (strcmp(((BrowserNode *) child)->get_name(), "~UmlComponent") == 0)
                    ((BrowserNode *) child)->set_name("~UmlArtifact");
            }
            else if (((BrowserNode *) child)->get_type() == UmlGeneralisation) {
                RelationData * r = (RelationData *)
                        ((BrowserNode *) child)->get_data();

                if (r->get_end_class() == baseArtifact)
                    ((RelationData *) component->add_relation(UmlGeneralisation, baseComponent))
                        ->copy(r);
            }
        }

        artifactArtifact->package_modified();
        artifactArtifact->set_name("UmlArtifact");

        // rename component views -> deployment views
        BrowserNode * bn;

        bn = (BrowserNode *) baseArtifactArtifact->parent();
        bn->set_name("API BASE Deployment view");
        bn->set_comment("The API system artifacts\n\nDO NOT MODIFY THEM !!!\n");

        bn = (BrowserNode *) comArtifact->parent();

        if (strcmp(bn->get_name(), "Com Component view") == 0) {
            bn->set_name("Com Deployment view");
            bn->set_comment("The API system artifacts concerning the communication with DOUML\n\nDO NOT MODIFY THEM !!!\n");
        }

        BrowserArtifact * settingsArtifact = BrowserArtifact::find("CppSettings");

        if (settingsArtifact != 0) {
            bn = (BrowserNode *) settingsArtifact->parent();

            if (strcmp(bn->get_name(), "Settings Component view") == 0) {
                bn->set_name("Settings Deployment view");
                bn->set_comment("The API system artifacts concerning the settings\n\nDO NOT MODIFY THEM !!!\n");
            }
        }

        bn = (BrowserNode *) artifactArtifact->parent();

        if (strcmp(bn->get_name(), "API USER component view") == 0) {
            bn->set_name("API USER Deployment view");
            s = bn->get_comment();
            index = 0;

            while ((index = s.find("component", index)) != -1) {
                s.replace(index, 9, "artifact");
                index += 8;
            }

            bn->set_comment(s);
        }
    }
}

static BrowserClass * add_metaclass(BrowserClass * cl, const char * mclname,
                                    const char * mclpath)
{
    BrowserNode * p = 0;
    BrowserNode * pack = (BrowserNode *) cl->parent()->parent();

    for (Q3ListViewItem * child = pack->firstChild(); child; child = child->nextSibling()) {
        if ((((BrowserNode *) child)->get_type() == UmlClassView) &&
                !strncmp(((BrowserNode *) child)->get_name(), "meta classes", 12) &&
                !((BrowserNode *) child)->wrong_child_name(mclname, UmlClass, FALSE, FALSE))
            p = (BrowserNode *) child;
    }

    if (p == 0) {
        QString s = "meta classes";

        while (pack->wrong_child_name(s, UmlClassView, TRUE, FALSE))
            s += "_";

        p = new BrowserClassView(s, pack);
    }

    BrowserClass * r = BrowserClass::add_class(FALSE, p, mclname);

    r->get_data()->set_stereotype("metaclass");

    if (mclpath != 0)
        r->set_value("metaclassPath", mclpath);

    return r;
}

static void extend(BrowserClass * cl, WrapperStr mclpath,
                   QList<BrowserClass *> & metaclasses)
{
    int index = mclpath.find('#');

    if (index != -1) {
        const char * mclname = ((const char *) mclpath) + (index + 1);
        WrapperStr path = mclpath.left(index);

        const char * defltpath0 = "http://schema.omg.org/spec/UML/2.0/uml.xml";
        const char * defltpath1 = "http://schema.omg.org/spec/UML/2.1/uml.xml";
        bool dflt = ((path == defltpath0) || (path == defltpath1));

        BrowserClass *metaclass = 0;
        if (dflt) {
            foreach (BrowserClass *mcl, metaclasses) {
                if (!strcmp(mclname, mcl->get_name())) {
                    const char * s = mcl->get_value("metaclassPath");

                    if ((s == 0) || !strcmp(s, defltpath0) || !strcmp(s, defltpath1)) {
                        metaclass = mcl;
                        break;
                    }
                }
            }
        }
        else {
            foreach (BrowserClass *mcl, metaclasses) {
                if (!strcmp(mclname, mcl->get_name())) {
                    const char * s = mcl->get_value("metaclassPath");

                    if ((s != 0) && (path == s)) {
                        metaclass = mcl;
                        break;
                    }
                }
            }
        }

        if (metaclass == 0) {
            metaclass = add_metaclass(cl, mclname, (dflt) ? 0 : path.data());
            metaclasses.append(metaclass);
        }

        cl->add_relation(UmlDirectionalAssociation, metaclass);
    }
}

void BrowserClass::post_load()
{
    TemporaryClass = 0;

    BrowserRelation::post_load();
    BrowserAttribute::post_load();
    BrowserOperation::post_load(); // must be done after rel and attr post_load

    if (NeedPostLoad) {
        QList<BrowserClass *> stereotypes;
        QList<BrowserClass *> metaclasses;

        IdIterator<BrowserClass> it(all);
        BrowserClass * cl;

        while ((cl = it.current()) != 0) {
            if (!cl->is_undefined()) {
                const char * s = cl->def->get_stereotype();

                if (strcmp(s, "metaclass") == 0)
                    metaclasses.append(cl);

                if ((strcmp(s, "stereotype") == 0) &&
                        (((BrowserNode *) cl->parent()->parent())->get_type() == UmlPackage) &&
                        !strcmp(((BrowserNode *) cl->parent()->parent())->get_stereotype(), "profile"))
                    stereotypes.append(cl);
            }

            ++it;
        }

        foreach (cl, stereotypes) {
            WrapperStr s = cl->get_value("stereotypeExtension"); // non empty

            s = s.simplifyWhiteSpace();

            int index1 = 0;
            int index2;

            while ((index2 = s.find(" ", index1)) != -1) {
                extend(cl, s.mid(index1, index2 - index1), metaclasses);
                index1 = index2 + 1;
            }

            extend(cl, s.mid(index1), metaclasses);

            cl->remove_key_value("stereotypeExtension");
        }
        stereotypes.clear();

        NeedPostLoad = FALSE;
    }
}


BrowserClass * BrowserClass::read_ref(char *& st, const char * k)
{
    if (k == 0)
        read_keyword(st, "class_ref");
    else if (strcmp(k, "class_ref"))
        wrong_keyword(k, "class_ref");

    int id = read_id(st);
    BrowserClass * result = all[id];

    return (result == 0)
            ? new BrowserClass(id)
            : result;
}

BrowserClass * BrowserClass::read(char *& st, char * k,
                                  BrowserNode * parent,
                                  bool force)
{
    BrowserClass * result;
    int id;

    if (!strcmp(k, "class_ref")) {
        if (((result = all[id = read_id(st)]) == 0) && force)
            result = new BrowserClass(id);

        return result;
    }
    else if (!strcmp(k, "class") || !strcmp(k, "actor")) {
        bool actor = (*k == 'a');

        id = read_id(st);

        QString s = read_string(st);

        if ((result = all[id]) == 0)
            result = new BrowserClass(s, parent, new ClassData, id);
        else if (result->is_defined) {
            BrowserClass * already_exist = result;

            result = new BrowserClass(s, parent, new ClassData, id);

            already_exist->must_change_id(all);
            already_exist->unconsistent_fixed("class", result);
        }
        else {
            result->set_parent(parent);
            result->set_name(s);
        }

        result->is_defined = TRUE;

        result->is_read_only = (!in_import() && read_only_file()) ||
                !parent->is_writable() ||
                ((user_id() != 0) && result->is_api_base());

        QFileInfo fi(BrowserView::get_dir(), QString::number(id) + ".bodies");

        if (!in_import() && fi.exists() && !fi.isWritable())
            result->is_read_only = TRUE;

        k = read_keyword(st);

        if (! actor)
            result->def->read(st, k);	// updates k

        result->def->set_browser_node(result);

        if (!strcmp(k, "associated_class_diagram")) {
            // old format
            result->set_associated_diagram(BrowserClassDiagram::read_ref(st, "classdiagram_ref"),
                                           TRUE);
            k = read_keyword(st);
        }
        else if (!strcmp(k, "associated_diagram")) {
            result->set_associated_diagram(BrowserClassDiagram::read_ref(st, read_keyword(st)),
                                           TRUE);
            k = read_keyword(st);
        }

        if ((read_file_format() < 20) && !strcmp(k, "associated_component")) {
            // old formats or component -> artifact
            if (read_file_format() < 3)
                // old format
                result->set_associated_artifact(BrowserArtifact::read_ref(st, "artifact_ref"),
                                                TRUE);
            else
                result->set_associated_artifact(BrowserArtifact::read_ref(st, read_keyword(st)),
                                                TRUE);

            k = read_keyword(st);
        }

        result->BrowserNode::read(st, k, id);	// updates k

        if (strcmp(k, "end")) {
            while (BrowserAttribute::read(st, k, result) ||
                   BrowserOperation::read(st, k, result) ||
                   BrowserExtraMember::read(st, k, result) ||
                   BrowserRelation::read(st, k, result) ||
                   BrowserSimpleRelation::read(st, k, result) ||
                   BrowserClass::read(st, k, result))
                k = read_keyword(st);

            if (in_import()) {
                result->set_bodies_modified(TRUE);
                OperationData::import(result, id);
            }

            if (strcmp(k, "end"))
                wrong_keyword(k, "end");
        }

        if (actor)
            result->def->set_stereotype("actor");
        else if (!NeedPostLoad &&
                 (read_file_format() < 63) &&
                 !strcmp(result->def->get_stereotype(), "stereotype"))
            NeedPostLoad = (result->get_value("stereotypeExtension") != 0);

        return result;
    }
    else
        return 0;
}

BrowserClass * BrowserClass::temporary()
{
    if (TemporaryClass == 0)
        TemporaryClass = new BrowserClass(-1);

    return TemporaryClass;
}

BrowserNode * BrowserClass::read_any_ref(char *& st, char * k)
{
    BrowserNode * r;

    if (((r = BrowserClass::read(st, k, 0)) == 0) &&
            ((r = BrowserAttribute::read(st, k, 0)) == 0) &&
            ((r = BrowserOperation::read(st, k, 0)) == 0) &&
            ((r = BrowserRelation::read(st, k, 0)) == 0))
        r = BrowserExtraMember::read(st, k, 0);

    return r;
}

BrowserNode * BrowserClass::get_it(const char * k, int id)
{
    if (!strcmp(k, "class_ref"))
        return all[id];

    BrowserNode * r;

    if (((r = BrowserAttribute::get_it(k, id)) == 0) &&
            ((r = BrowserOperation::get_it(k, id)) == 0) &&
            ((r = BrowserRelation::get_it(k, id)) == 0))
        r = BrowserExtraMember::get_it(k, id);

    return r;
}

uint BrowserClass::TypeID()
{
    return TypeIdentifier<BrowserClass>::id();
}
