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





#include <stdio.h>

#include <qcursor.h>
#include <qsplitter.h>
#include <q3grid.h>
#include <q3vbox.h>
#include <qlabel.h>
#include <q3combobox.h>
#include <qcheckbox.h>
#include <q3popupmenu.h>
#include <q3groupbox.h>
#include <q3buttongroup.h>
#include <qpushbutton.h>
#include <q3filedialog.h>
//Added by qt3to4:


#include "ClassDialog.h"
#include "ClassData.h"
#include "BrowserClass.h"
#include "KeyValueTable.h"
#include "ComboItem.h"
#include "UmlWindow.h"
#include "UmlDesktop.h"
#include "BrowserView.h"
#include "DialogUtil.h"
#include "GenerationSettings.h"
#include "BrowserRelation.h"
#include "RelationData.h"
#include "ActualParamData.h"
#include "AttributeData.h"
#include "OperationData.h"
#include "ExtraMemberData.h"
#include "BrowserArtifact.h"
#include "CustomWidgets/EdgeMenuToolBar.h"
#include "strutil.h"
#include "BodyDialog.h"
#include "AnnotationDialog.h"
#include "ProfiledStereotypes.h"
#include "Tool.h"
#include "Images.h"
#include "translate.h"
#include "misc/TypeIdentifier.h"

QSize ClassDialog::previous_size;

static QString RelativeRoot;
static QString RelativePrj;
static QString Absolute;
QSharedPointer<ClassDialog> ClassDialog::instance;
ClassDialog::ClassDialog(ClassData * c): EdgeMenuDialog(0, 0, FALSE), cl(c)
{
    // take time in case of many classes and artifacts
    isWritable = c->browser_node->is_writable();
    NullPointers();
    SetDialogMode(isWritable);
    SetCurrentNode(c->browser_node);
    InitGui();
    FillGuiElements(c);
    //open_dialog(this);
}

void ClassDialog::polish()
{
    Q3TabDialog::polish();
    UmlDesktop::limitsize_center(this, previous_size, 0.8, 0.8);
}

ClassDialog::~ClassDialog()
{
    previous_size = size();

    foreach (BodyDialog *dialog, edits)
        dialog->close();
    edits.clear();

    if(toolbar)
    {
        toolbar->setParent(0);
        toolbar->hide();
    }
}

QSharedPointer<ClassDialog> ClassDialog::Instance(ClassData * cl)
{
    if (instance.isNull())
        instance = QSharedPointer<ClassDialog>(new ClassDialog(cl));
    else
        instance->FillGuiElements(cl);

    return instance;
}

QString ClassDialog::cpp_stereotype(const QString & stereotype)
{
    QString s = GenerationSettings::cpp_class_stereotype(stereotype);

    return ((s == "struct") || (s == "union") || (s == "enum") ||
            (s == "typedef") || (s == "ignored") || (s == "metaclass"))
           ? s : QString("class");
}

QString ClassDialog::java_stereotype(const QString & stereotype)
{
    QString s = GenerationSettings::java_class_stereotype(stereotype);

    return ((s == "interface") || (s == "@interface") ||
            (s == "enum") || (s == "enum_pattern") ||
            (s == "ignored") || (s == "metaclass"))
           ? s : QString("class");
}

QString ClassDialog::php_stereotype(const QString & stereotype)
{
    QString s = GenerationSettings::php_class_stereotype(stereotype);

    return ((s == "interface") || (s == "enum") ||
            (s == "ignored") || (s == "metaclass"))
           ? s : QString("class");
}

QString ClassDialog::python_stereotype(const QString & stereotype)
{
    QString s = GenerationSettings::python_class_stereotype(stereotype);

    return ((s == "enum") || (s == "ignored") || (s == "metaclass"))
           ? s : QString("class");
}

QString ClassDialog::idl_stereotype(const QString & stereotype)
{
    QString s = GenerationSettings::idl_class_stereotype(stereotype);

    return ((s == "struct") || (s == "union") || (s == "enum") ||
            (s == "typedef") || (s == "exception") ||
            (s == "ignored") || (s == "interface") || (s == "metaclass"))
           ? s : QString("valuetype");
}

void ClassDialog::default_description()
{
    comment->setText(GenerationSettings::default_class_description());
}

void ClassDialog::edit_description()
{
    edit(comment->text(), edname->text().stripWhiteSpace() + "_description",
         cl, TxtEdit, this, (post_edit) post_edit_description, edits);
}

void ClassDialog::post_edit_description(ClassDialog * d, QString s)
{
    d->comment->setText(s);
}

void ClassDialog::edit_constraint()
{
    edit(constraint->text(), edname->text().stripWhiteSpace() + "_constraint",
         cl, TxtEdit, this, (post_edit) post_edit_constraint, edits);
}

void ClassDialog::post_edit_constraint(ClassDialog * d, QString s)
{
    d->constraint->setText(s);
}

void ClassDialog::edStereotypeActivated(const QString & s)
{
    QString stereotype = s.stripWhiteSpace();

    if ((stereotype == "stereotype") || (stereotype == "metaclass")) {
        if (GenerationSettings::cpp_get_default_defs())
            setTabEnabled(cppTab, FALSE);

        if (GenerationSettings::java_get_default_defs())
            setTabEnabled(javatab, FALSE);

        if (GenerationSettings::idl_get_default_defs())
            setTabEnabled(idltab, FALSE);

        if (GenerationSettings::php_get_default_defs())
            setTabEnabled(phptab, FALSE);

        if (GenerationSettings::python_get_default_defs())
            setTabEnabled(pythontab, FALSE);

        if (stereotypetab != 0)
            setTabEnabled(stereotypetab, (stereotype == "stereotype"));
    }
    else {
        if (GenerationSettings::cpp_get_default_defs())
            setTabEnabled(cppTab, TRUE);

        if (GenerationSettings::java_get_default_defs())
            setTabEnabled(javatab, TRUE);

        if (GenerationSettings::idl_get_default_defs())
            setTabEnabled(idltab, TRUE);

        if (GenerationSettings::php_get_default_defs())
            setTabEnabled(phptab, TRUE);

        if (GenerationSettings::python_get_default_defs())
            setTabEnabled(pythontab, TRUE);

        if (stereotypetab != 0)
            setTabEnabled(stereotypetab, FALSE);

        if (stereotype == "typedef") {
            basetypelbl->show();
            edbasetype->show();
            opt_bg->hide();
        }
        else {
            basetypelbl->hide();
            edbasetype->hide();

            if ((stereotype != "enum") && (stereotype != "enum_pattern"))
                opt_bg->show();
            else
                opt_bg->hide();
        }

        parametrized_vtab->setEnabled((stereotype != "enum") && (stereotype != "enum_pattern") && (stereotype != "typedef"));

        if (instantiate_vtab)
            instantiate_vtab->setEnabled((stereotype != "enum") && (stereotype != "enum_pattern") && (stereotype != "union"));

        if (current_cpp_stereotype != cpp_stereotype(stereotype))
            cpp_default_decl();

        if (current_java_stereotype != java_stereotype(stereotype))
            java_default_decl();

        if (current_php_stereotype != php_stereotype(stereotype))
            php_default_decl();

        if (current_python_stereotype != python_stereotype(stereotype))
            python_default_decl();

        QString idl_st = idl_stereotype(stereotype);

        switch_bg->setEnabled(idl_st == "union");
        idl_local_cb->setEnabled(idl_st == "interface");
        idl_custom_cb->setEnabled(idl_st == "valuetype");

        if (idl_st != current_idl_stereotype)
            idl_default_decl();
    }
}

void ClassDialog::icon_browse()
{
    QString s = ediconpath->text().simplifyWhiteSpace();
    const QString ns = Q3FileDialog::getOpenFileName(s, "", this, 0, TR("Select image"));

    if (! ns.isEmpty()) {
        ediconpath->setText(ns);
        iconpathrootbutton->setText(RelativeRoot);
        iconpathprjbutton->setText(RelativePrj);
    }
}

void ClassDialog::icon_root_relative()
{
    QString root = UmlWindow::images_root_dir();

    if (root.isEmpty()) {
        iconpathrootbutton->setEnabled(FALSE);
        return;
    }

    const QString s = ediconpath->text();

    if (root.at(root.length() - 1) != QChar('/'))
        root += '/';

    if (iconpathrootbutton->text() == RelativeRoot) {
        int len = root.length();

        if (



            (s.find(root) == 0) &&

            (s.length() >= len)) {
            ediconpath->setText(s.mid(len));
            iconpathrootbutton->setText(Absolute);
            iconpathprjbutton->setText(Absolute);
        }
    }
    else {
        ediconpath->setText(root + s);
        iconpathrootbutton->setText(RelativeRoot);
        iconpathprjbutton->setText(RelativePrj);
    }
}

void ClassDialog::icon_prj_relative()
{
    QString root = BrowserView::get_dir().absPath();
    const QString s = ediconpath->text();

    if (root.at(root.length() - 1) != QChar('/'))
        root += '/';

    if (iconpathprjbutton->text() == RelativePrj) {
        int len = root.length();

        if (



            (s.find(root) == 0) &&

            (s.length() >= len)) {
            ediconpath->setText(s.mid(len));
            iconpathrootbutton->setText(Absolute);
            iconpathprjbutton->setText(Absolute);
        }
    }
    else {
        ediconpath->setText(root + s);
        iconpathrootbutton->setText(RelativeRoot);
        iconpathprjbutton->setText(RelativePrj);
    }

    iconpathrootbutton->setEnabled(!UmlWindow::images_root_dir().isEmpty());
}

void ClassDialog::update_all_tabs(QWidget * w)
{
    formals_table->forceUpdateCells();

    if (cl->get_n_actualparams() != 0)
        actuals_table->forceUpdateCells();

    bool visit = !hasOkButton();

    if (w == 0) {
        cpp_update_decl();
        java_update_decl();
        php_update_decl();
        python_update_decl();
        idl_update_decl();
    }

    if (w == umltab) {
        if (!visit)
            edname->setFocus();
    }
    else if (w == cppTab) {
        cpp_update_decl();

        if (!visit)
            edcppdecl->setFocus();
    }
    else if (w == javatab) {
        java_update_decl();

        if (!visit)
            edjavadecl->setFocus();
    }
    else if (w == phptab) {
        php_update_decl();

        if (!visit)
            edphpdecl->setFocus();
    }
    else if (w == pythontab) {
        python_update_decl();

        if (!visit)
            edpythondecl->setFocus();
    }
    else if (w == idltab) {
        idl_update_decl();

        if (!visit)
            edidldecl->setFocus();
    }
}

static void generate_mother(BrowserClass * mother, bool cpp, QString & s,
                            ClassData * cl, ActualParamsTable * actuals_table,
                            BrowserNodeList & nodes, QStringList & node_names)
{
    if (((BrowserNode *) mother->parent())->get_type() == UmlClass) {
        generate_mother((BrowserClass *) mother->parent(), cpp,
                        s, cl, actuals_table, nodes, node_names);
        s += (cpp) ? "::" : ".";
    }

    s += mother->get_name();

    if (actuals_table != 0)
        actuals_table->generate(s, cl, mother, cpp, nodes, node_names);
}

static void cpp_generate_inherit(QString & s, ClassData * cl,
                                 ActualParamsTable * actuals_table,
                                 BrowserNodeList & nodes,
                                 QStringList & node_names)
{

    const char * sep = " : ";
    BrowserNodeList inh;
    cl->get_browser_node()->children(inh, UmlGeneralisation, UmlRealize);

    foreach (BrowserNode *item, inh) {
        RelationData * r = (RelationData *) item->get_data();

        if (r->get_cppdecl_a()[0]) {
            s += sep;
            sep = ", ";
            s += stringify((r->get_cpp_visibility_a() != UmlDefaultVisibility)
                           ? r->get_cpp_visibility_a() : r->get_uml_visibility_a());
            s += ((r->get_cpp_virtual_inheritance()) ? " virtual " : " ");

            generate_mother(r->get_end_class(), TRUE, s, cl, actuals_table, nodes, node_names);
        }
    }
}

static void cpp_generate_typedef_type(QString & s, ClassData * cl,
                                      ActualParamsTable * actuals_table,
                                      QString basetype,
                                      BrowserNodeList & nodes,
                                      QStringList & node_names)
{
    BrowserNodeList inh;

    cl->get_browser_node()->children(inh, UmlGeneralisation, UmlRealize);

    foreach (BrowserNode *item, inh) {
        RelationData * r = (RelationData *) item->get_data();
        /*if (r->get_cppdecl_a()[0])*/
        {
            BrowserClass * mother = r->get_end_class();

            if (mother->full_name(TRUE) == basetype) {
                s += mother->get_name();

                if (actuals_table != 0)
                    actuals_table->generate(s, cl, mother, TRUE, nodes, node_names);

                return;
            }
        }
    }

    s += GenerationSettings::cpp_type(type(basetype, node_names, nodes));
}

static void generate_members(BrowserNode * cl, QString & s, QString indent,
                             const char * (AttributeData::* att_f)() const,
                             const char * (OperationData::* oper_f)() const,
                             const char * (RelationData::* rel_a_f)() const,
                             const char * (RelationData::* rel_b_f)() const,
                             const char * (ExtraMemberData::* ex_f)() const,
                             const char * (ClassData::* class_f)() const,
                             QString(* ste_f)(const QString &),
                             bool idl_union = FALSE,
                             bool only_items = FALSE,
                             bool except_items = FALSE,
                             bool except_instance_att_rel = FALSE,
                             bool only_instance_att_rel = FALSE)
{
    Q3ListViewItem * child;

    for (child = cl->firstChild(); child; child = child->nextSibling()) {
        if (((BrowserNode *) child)->deletedp())
            continue;

        QString pre = indent + "...";
        QString decl;
        QString post;

        switch (((BrowserNode *) child)->get_type()) {
        case UmlDependency:
            if (!only_items && !only_instance_att_rel && (att_f == &AttributeData::get_cppdecl)) {
                RelationData * rel = (RelationData *)((BrowserNode *) child)->get_data();

                if (rel->is_a((BrowserRelation *) child)
                    && (!strcmp(rel->get_stereotype(), "friend"))) {
                    ClassData * cld = (ClassData *) rel->get_end_class()->get_data();
                    QString st = cld->get_stereotype();

                    if (st.isEmpty())
                        st = "class";

                    s += pre + "friend " + st + " " +
                         true_name(rel->get_end_class()->get_name(), cld->get_cppdecl())
                         + "\n";
                }
            }

            // no break !
        case UmlGeneralisation:
        case UmlRealize:
        case UmlDependOn:
            continue;

        case UmlAttribute: {
            // without the var 'at' G++ 3.2 generate a wrong code
            AttributeData * at = (AttributeData *)((BrowserNode *) child)->get_data();

            if ((at->get_isa_class_attribute())
                ? only_instance_att_rel
                : except_instance_att_rel)
                continue;

            const char * st = at->get_stereotype();

            if (only_items) {
                if (strcmp(st, "attribute") == 0)
                    continue;
            }
            else if (except_items) {
                if (strcmp(st, "attribute") != 0)
                    continue;
            }

            decl = (at->*att_f)();

            if (idl_union)
                pre += QString("case ") + at->get_idlcase() + " ... ";

            post = "\n";
        }
        break;

        case UmlOperation:
            if (!only_items && !only_instance_att_rel) {
                // without the var 'op' G++ 3.2 generate a wrong code
                OperationData * op = (OperationData *)((BrowserNode *) child)->get_data();

                decl = (op->*oper_f)();
                post = "()\n";
            }

            break;

        case UmlExtraMember:
            if (!only_items && !only_instance_att_rel) {
                // without the var 'emd' G++ 3.2 generate a wrong code
                ExtraMemberData * emd = (ExtraMemberData *)((BrowserNode *) child)->get_data();

                decl = (emd->*ex_f)();

                if (decl.isEmpty())
                    continue;

                s += decl;
                s += '\n';
                continue;
            }

        case UmlClass:
            if (!only_items && !only_instance_att_rel) {
                // without the var 'cl' G++ 3.2 generate a wrong code
                ClassData * cl = (ClassData *)((BrowserNode *) child)->get_data();

                decl = (cl->*class_f)();
                pre += ste_f(cl->get_stereotype()) + " ";
                post = (pre.left(11) == "...typedef ") ? "\n" : " {...}\n";
            }

            break;

        default: // relations
            if (! only_items) {
                RelationData * re = (RelationData *)((BrowserNode *) child)->get_data();

                if (re->is_a((BrowserRelation *) child)) {
                    if ((re->get_isa_class_relation_a())
                        ? only_instance_att_rel
                        : except_instance_att_rel)
                        continue;

                    decl = (re->*rel_a_f)();
                }
                else {
                    if ((re->get_isa_class_relation_b())
                        ? only_instance_att_rel
                        : except_instance_att_rel)
                        continue;

                    decl = (re->*rel_b_f)();
                }

                post = "\n";
            }
        }

        if (! decl.isEmpty()) {
            s += pre;
            s += true_name(((BrowserNode *) child)->get_name(), decl);
            s += post;
        }
    }
}

static void cpp_generate_inlines(BrowserNode * cl, const QString & cl_name,
                                 const QString & cl_decl, QString & s)
{
    ClassData * cd = (ClassData *) cl->get_data();
    bool templp = (cd->get_n_formalparams() != 0) || (cl_name.find('<') != -1);
    QString templates;
    QString names;
    QString templates_tmplop;
    QString names_tmplop;

    if (((BrowserNode *) cl->parent())->get_type() == UmlClass) {
        ((ClassData *)((BrowserNode *) cl->parent())->get_data())
        ->get_class_spec(templates, names, templates_tmplop, names_tmplop);
        names += "::";
    }

    QString t1;
    QString t2;

    cd->get_template_prefixes(t1, t2);
    templates_tmplop = templates + "template<>\n";
    templates += t1;
    names_tmplop = names + true_name(cl_name, cl_decl);
    names = names_tmplop + t2;

    QString prefix = '\n' + templates + "..." + names + "::";
    QString prefix_tmplop = '\n' + templates_tmplop + "..." + names_tmplop + "<...>::";
    Q3ListViewItem * child;

    for (child = cl->firstChild(); child; child = child->nextSibling()) {
        if (((BrowserNode *) child)->deletedp())
            continue;

        switch (((BrowserNode *) child)->get_type()) {
        case UmlAttribute: {
            AttributeData * d = (AttributeData *)((BrowserNode *) child)->get_data();
            const char * decl = d->get_cppdecl();

            if (templp &&
                d->get_isa_class_attribute() &&
                decl[0]) {
                s += prefix;
                s += true_name(((BrowserNode *) child)->get_name(), decl);
                s += "\n\n";
            }
        }
        break;

        case UmlClass:
            cpp_generate_inlines((BrowserNode *) child,
                                 ((BrowserNode *) child)->get_name(),
                                 ((ClassData *)((BrowserNode *) child)->get_data())->get_cppdecl(),
                                 s);

            break;

        default:
            // relation
        {
            RelationData * re = (RelationData *)((BrowserNode *) child)->get_data();

            if (re->is_a((BrowserRelation *) child)) {
                const char * decl = re->get_cppdecl_a();

                if (templp &&
                    re->get_isa_class_relation_a() &&
                    decl[0]) {
                    s += prefix;
                    s += true_name(re->get_role_a(), decl);
                    s += "\n\n";
                }
            }
            else {
                const char * decl = re->get_cppdecl_b();

                if (templp &&
                    re->get_isa_class_relation_b() &&
                    decl[0]) {
                    s += prefix;
                    s += true_name(re->get_role_b(), decl);
                    s += "\n";
                }
            }
        }
        break;

        case UmlDependency:
        case UmlGeneralisation:
        case UmlRealize:
        case UmlDependOn:
            break;

        case UmlOperation: {
            OperationData * d = (OperationData *)((BrowserNode *) child)->get_data();
            const char * def = d->get_cppdef();

            if ((templp || d->get_cpp_inline()) && def[0]) {
                s += ((d->is_template_operation()) ? prefix_tmplop : prefix);
                s += true_name(((BrowserNode *) child)->get_name(), def);
                s += "(...){...}\n";
            }
        }
        break;

        case UmlExtraMember: {
            ExtraMemberData * d = (ExtraMemberData *)((BrowserNode *) child)->get_data();
            const char * def = d->get_cpp_def();

            if ((templp || d->get_cpp_inline()) && def[0]) {
                s += '\n' + def + '\n';
            }
        }
        }
    }
}

void ClassDialog::cpp_generate_decl(QString & s, ClassData * cl,
                                    QString def, QString name,
                                    QString stereotype,
                                    QString basetype, QString comment,
                                    ActualParamsTable * actuals,
                                    FormalParamsTable * formals,
                                    BrowserNodeList & nodes,
                                    QStringList & node_names,
                                    KeyValuesTable * kvt)
{
    const char * p = def;

    if (p == 0)
        return;

    QString indent = "";
    bool a_typedef = (stereotype == "typedef");
    bool an_enum = (stereotype == "enum");

    while ((*p == ' ') || (*p == '\t'))
        indent += *p++;

    if (*p != '#')
        s += indent;

    const char * pp = 0;

    for (;;) {
        if (*p == 0) {
            if (pp == 0)
                break;

            // comment management done
            p = pp;
            pp = 0;

            if (*p == 0)
                break;

            if (*p != '#')
                s += indent;
        }

        if (!strncmp(p, "${comment}", 10))
            manage_comment(comment, p, pp,
                           GenerationSettings::cpp_javadoc_style());
        else if (!strncmp(p, "${description}", 14))
            manage_description(comment, p, pp);
        else if (!strncmp(p, "${name}", 7)) {
            p += 7;
            s += name;
        }
        else if (*p == '\n') {
            s += *p++;

            if (*p && (*p != '#') &&
                strncmp(p, "${members}", 10) &&
                strncmp(p, "${items}", 8))
                s += indent;
        }
        else if (a_typedef) {
            if (!strncmp(p, "${type}", 7)) {
                p += 7;
                cpp_generate_typedef_type(s, cl, actuals, basetype, nodes, node_names);
            }
            else
                s += *p++;
        }
        else if (an_enum) {
            if (!strncmp(p, "${items}", 8)) {
                p += 8;
                generate_members(cl->browser_node, s, indent,
                                 &AttributeData::get_cppdecl, &OperationData::get_cppdecl,
                                 &RelationData::get_cppdecl_a, &RelationData::get_cppdecl_b,
                                 &ExtraMemberData::get_cpp_decl, &ClassData::get_cppdecl,
                                 cpp_stereotype);

                if (*p == '}')
                    s += indent;
            }
            else
                s += *p++;
        }
        else if (!strncmp(p, "${template}", 11)) {
            p += 11;
            formals->generate_cpp(s, nodes, node_names, name);
        }
        else if (!strncmp(p, "${inherit}", 10)) {
            p += 10;
            cpp_generate_inherit(s, cl, actuals, nodes, node_names);
        }
        else if (!strncmp(p, "${members}", 10)) {
            p += 10;
            generate_members(cl->browser_node, s, indent,
                             &AttributeData::get_cppdecl, &OperationData::get_cppdecl,
                             &RelationData::get_cppdecl_a, &RelationData::get_cppdecl_b,
                             &ExtraMemberData::get_cpp_decl, &ClassData::get_cppdecl,
                             cpp_stereotype);

            if (*p == '}')
                s += indent;
        }
        else if (!strncmp(p, "${inlines}", 10)) {
            p += 10;
            cpp_generate_inlines(cl->browser_node, name, def, s);
        }
        else if (*p == '@')
            manage_alias(cl->browser_node, p, s, kvt);
        else
            s += *p++;
    }
}

void ClassDialog::cpp_update_decl()
{
    QString s;

    if (cl->browser_node->nestedp())
        cpp_visibility.update_default(uml_visibility);

    if (cpp_external_cb->isChecked())
        showcppdecl->setEnabled(FALSE);
    else {
        showcppdecl->setEnabled(TRUE);

        cpp_generate_decl(s, cl, edcppdecl->text(),
                          edname->text().stripWhiteSpace(), current_cpp_stereotype,
                          edbasetype->currentText().stripWhiteSpace(),
                          comment->text(), (instantiate_vtab != 0) ? actuals_table : 0,
                          formals_table, nodes, node_names, kvtable);
    }

    showcppdecl->setText(s);
}

void ClassDialog::cpp_default_decl()
{
    if (cpp_external_cb->isChecked())
        edcppdecl->setText(GenerationSettings::cpp_default_external_class_decl());
    else {
        current_cpp_stereotype =
            cpp_stereotype(edstereotype->currentText().stripWhiteSpace());

        if (current_cpp_stereotype == "struct")
            edcppdecl->setText(GenerationSettings::cpp_default_struct_decl());
        else if (current_cpp_stereotype == "union")
            edcppdecl->setText(GenerationSettings::cpp_default_union_decl());
        else if (current_cpp_stereotype == "enum")
            edcppdecl->setText(GenerationSettings::cpp_default_enum_decl());
        else if (current_cpp_stereotype == "typedef")
            edcppdecl->setText(GenerationSettings::cpp_default_typedef_decl());
        else if ((current_cpp_stereotype == "ignored") ||
                 (current_cpp_stereotype == "metaclass"))
            edcppdecl->setText(QString());
        else
            edcppdecl->setText(GenerationSettings::cpp_default_class_decl());

        cpp_update_decl();
    }
}

void ClassDialog::cpp_unmapped_decl()
{
    edcppdecl->setText(QString());
    showcppdecl->setText(QString());
}

void ClassDialog::cpp_generate_members_def(const BrowserNode * cl, QString & s)
{
    ClassData * cd = (ClassData *) cl->get_data();
    QString templates;
    QString names;
    QString templates_tmplop;
    QString names_tmplop;

    cd->get_class_spec(templates, names, templates_tmplop, names_tmplop);

    QString prefix = templates + "..." + names + "::";
    QString prefix_tmplop = templates_tmplop + "..." + names_tmplop + "<...>::";
    bool templ = !templates.isEmpty() || (strchr(cl->get_name(), '<') != 0);
    Q3ListViewItem * child;

    for (child = cl->firstChild(); child; child = child->nextSibling())
        if (!((BrowserNode *) child)->deletedp())
            ((BrowserNode *) child)->member_cpp_def(prefix, prefix_tmplop, s, templ);
}

static void java_generate_extends(QString & s, const QString & stereotype,
                                  ClassData * cl, ActualParamsTable * actuals_table,
                                  BrowserNodeList & nodes, QStringList & node_names)
{
    BrowserNodeList inh;
    const char * sep = " extends ";

    cl->get_browser_node()->children(inh, UmlGeneralisation, UmlRealize);

    foreach (BrowserNode *item, inh) {
        RelationData * r =
            (RelationData *)((BrowserRelation *) item)->get_data();

        if (r->get_javadecl_a()[0]) {
            bool gen = FALSE;
            BrowserClass * mother = r->get_end_class();
            QString other_stereotype =
                ClassDialog::java_stereotype(((ClassData *) mother->get_data())->get_stereotype());

            if ((stereotype == "interface") ||
                (stereotype == "@interface")) {
                gen = TRUE;
                s += sep;

                if ((other_stereotype != "interface") &&
                    (other_stereotype != "@interface"))
                    s += "!!!!!";
            }
            else if ((other_stereotype != "interface") &&
                     (other_stereotype != "@interface")) {
                gen = TRUE;
                s += sep;

                if ((*sep == ',') ||
                    (stereotype == "union") || (stereotype == "enum") || (stereotype == "enum_pattern") ||
                    (other_stereotype == "union") || (other_stereotype == "enum"))
                    s += "!!!!!";
            }

            if (gen) {
                sep = ", ";
                generate_mother(mother, FALSE, s, cl, actuals_table, nodes, node_names);
            }
        }
    }
}

static void java_generate_implements(QString & s, const QString & stereotype,
                                     ClassData * cl, ActualParamsTable * actuals_table,
                                     BrowserNodeList & nodes, QStringList & node_names)
{
    BrowserNodeList inh;
    const char * sep = " implements ";

    cl->get_browser_node()->children(inh, UmlGeneralisation, UmlRealize);

    foreach (BrowserNode *item, inh) {
        RelationData * r =
            (RelationData *)((BrowserRelation *) item)->get_data();

        if (r->get_javadecl_a()[0]) {
            BrowserClass * mother = r->get_end_class();
            QString other_stereotype =
                ClassDialog::java_stereotype(((ClassData *) mother->get_data())->get_stereotype());

            if ((other_stereotype == "interface") ||
                (other_stereotype == "@interface")) {
                s += sep;

                if ((stereotype == "union") || (stereotype == "enum_pattern"))
                    s += "!!!!!";

                sep = ", ";
                generate_mother(mother, FALSE, s, cl, actuals_table, nodes, node_names);
            }
        }
    }
}

void ClassDialog::java_generate_decl(QString & s, ClassData * cl, QString def,
                                     QString annotation, QString name,
                                     QString stereotype,
                                     QString comment, UmlVisibility visibility,
                                     bool is_final, bool is_abstract,
                                     ActualParamsTable * actuals,
                                     FormalParamsTable * formals,
                                     BrowserNodeList & nodes,
                                     QStringList & node_names,
                                     KeyValuesTable * kvt)
{
    const char * p = def;

    if (p == 0)
        return;

    QString indent = "";

    while ((*p == ' ') || (*p == '\t'))
        indent += *p++;

    s += indent;

    const char * pp = 0;

    for (;;) {
        if (*p == 0) {
            if (pp == 0)
                break;

            // comment management done
            p = pp;
            pp = 0;

            if (*p == 0)
                break;

            s += indent;
        }

        if (!strncmp(p, "${comment}", 10))
            manage_comment(comment, p, pp,
                           GenerationSettings::java_javadoc_style());
        else if (!strncmp(p, "${description}", 14))
            manage_description(comment, p, pp);
        else if (!strncmp(p, "${public}", 9)) {
            p += 9;

            if (visibility == UmlPublic)
                s += "public ";
        }
        else if (!strncmp(p, "${visibility}", 13)) {
            p += 13;

            if (visibility != UmlPackageVisibility)
                s += stringify(visibility) + QString(" ");
        }
        else if (!strncmp(p, "${final}", 8)) {
            p += 8;

            if (is_final)
                s += "final ";
        }
        else if (!strncmp(p, "${abstract}", 11)) {
            p += 11;

            if (is_abstract)
                s += "abstract ";
        }
        else if (!strncmp(p, "${name}", 7)) {
            p += 7;
            s += name;
            formals->generate_java(s, nodes, node_names);
        }
        else if (!strncmp(p, "${extends}", 10)) {
            p += 10;
            java_generate_extends(s, stereotype, cl, actuals, nodes, node_names);
        }
        else if (!strncmp(p, "${implements}", 13)) {
            p += 13;
            java_generate_implements(s, stereotype, cl, actuals, nodes, node_names);
        }
        else if (!strncmp(p, "${members}", 10)) {
            p += 10;
            generate_members(cl->browser_node, s, indent,
                             &AttributeData::get_javadecl, &OperationData::get_javadef,
                             &RelationData::get_javadecl_a, &RelationData::get_javadecl_b,
                             &ExtraMemberData::get_java_decl, &ClassData::get_javadecl,
                             java_stereotype, FALSE, FALSE, (stereotype == "enum"));

            if (*p == '}')
                s += indent;
        }
        else if (!strncmp(p, "${items}", 8)) {
            p += 8;
            generate_members(cl->browser_node, s, indent,
                             &AttributeData::get_javadecl, &OperationData::get_javadef,
                             &RelationData::get_javadecl_a, &RelationData::get_javadecl_b,
                             &ExtraMemberData::get_java_decl, &ClassData::get_javadecl,
                             java_stereotype, FALSE, TRUE, FALSE);

            if (*p == '}')
                s += indent;
        }
        else if (!strncmp(p, "${cases}", 8)) {
            p += 8;
            generate_members(cl->browser_node, s, indent,
                             &AttributeData::get_javadecl, &OperationData::get_javadef,
                             &RelationData::get_javadecl_a, &RelationData::get_javadecl_b,
                             &ExtraMemberData::get_java_decl, &ClassData::get_javadecl,
                             java_stereotype);
        }
        else if (!strncmp(p, "${@}", 4)) {
            p += 4;

            if (pp != 0)
                s += "${@}";
            else if (! annotation.isEmpty()) {
                pp = p;
                p = annotation;
            }
        }
        else if (*p == '\n') {
            s += *p++;

            if (*p &&
                strncmp(p, "${members}", 10) &&
                strncmp(p, "${items}", 8) &&
                strncmp(p, "${cases}", 8))
                s += indent;
        }
        else if (*p == '@')
            manage_alias(cl->browser_node, p, s, kvt);
        else
            s += *p++;
    }
}

void ClassDialog::java_update_decl()
{
    QString s;

    if (java_external_cb->isChecked())
        showjavadecl->setEnabled(FALSE);
    else {
        showjavadecl->setEnabled(TRUE);

        QString def = edjavadecl->text();

        java_generate_decl(s, cl, def, javaannotation,
                           edname->text().stripWhiteSpace(),
                           current_java_stereotype, comment->text(),
                           uml_visibility.value(), java_final_cb->isChecked(),
                           abstract_cb->isChecked(), actuals_table,
                           formals_table, nodes, node_names, kvtable);

        pbJavaAnnotation->setEnabled(def.find("${@}") != -1);
    }

    showjavadecl->setText(s);
}

void ClassDialog::java_default_decl()
{
    if (java_external_cb->isChecked())
        edjavadecl->setText(GenerationSettings::java_default_external_class_decl());
    else {
        current_java_stereotype =
            java_stereotype(edstereotype->currentText().stripWhiteSpace());

        if (current_java_stereotype == "enum")
            edjavadecl->setText(GenerationSettings::java_default_enum_decl());
        else if (current_java_stereotype == "enum_pattern")
            edjavadecl->setText(GenerationSettings::java_default_enum_pattern_decl());
        else if (current_java_stereotype == "interface")
            edjavadecl->setText(GenerationSettings::java_default_interface_decl());
        else if (current_java_stereotype == "@interface") {
            QString s = GenerationSettings::java_default_interface_decl();
            int index = s.find("interface");

            if (index != -1)
                s.insert(index, '@');

            edjavadecl->setText(s);
        }
        else if ((current_java_stereotype == "ignored") ||
                 (current_java_stereotype == "metaclass"))
            edjavadecl->setText(QString());
        else
            edjavadecl->setText(GenerationSettings::java_default_class_decl());

        java_update_decl();
    }
}

void ClassDialog::java_unmapped_decl()
{
    edjavadecl->setText(QString());
    showjavadecl->setText(QString());
}

void ClassDialog::java_edit_annotation()
{
    AnnotationDialog dialog(this, javaannotation, !hasOkButton());

    if (dialog.exec() == QDialog::Accepted)
        java_update_decl();
}

static void php_generate_extends(QString & s, const QString & stereotype,
                                 ClassData * cl)
{
    BrowserNodeList inh;
    const char * sep = " extends ";

    cl->get_browser_node()->children(inh, UmlGeneralisation, UmlRealize);

    foreach (BrowserNode *item, inh) {
        RelationData * r =
            (RelationData *)((BrowserRelation *) item)->get_data();

        if (r->get_phpdecl_a()[0]) {
            bool gen = FALSE;
            BrowserClass * mother = r->get_end_class();
            QString other_stereotype =
                ClassDialog::php_stereotype(((ClassData *) mother->get_data())->get_stereotype());

            if (stereotype == "interface") {
                gen = TRUE;
                s += sep;

                if (other_stereotype != "interface")
                    s += "!!!!!";
            }
            else if (other_stereotype != "interface") {
                gen = TRUE;
                s += sep;

                if ((*sep == ',') ||
                    (stereotype == "union") || (stereotype == "enum") ||
                    (other_stereotype == "union") || (other_stereotype == "enum"))
                    s += "!!!!!";
            }

            if (gen) {
                sep = ", ";
                s += mother->get_name();
            }
        }
    }
}

static void php_generate_implements(QString & s, const QString & stereotype,
                                    ClassData * cl)
{
    BrowserNodeList inh;
    const char * sep = " implements ";

    cl->get_browser_node()->children(inh, UmlGeneralisation, UmlRealize);

    foreach (BrowserNode *item, inh) {
        RelationData * r =
            (RelationData *)((BrowserRelation *) item)->get_data();

        if (r->get_phpdecl_a()[0]) {
            BrowserClass * mother = r->get_end_class();
            QString other_stereotype =
                ClassDialog::php_stereotype(((ClassData *) mother->get_data())->get_stereotype());

            if (other_stereotype == "interface") {
                s += sep;

                if (stereotype == "union")
                    s += "!!!!!";

                sep = ", ";
                s += mother->get_name();
            }
        }
    }
}

void ClassDialog::php_generate_decl(QString & s, ClassData * cl, QString def,
                                    QString name, QString stereotype,
                                    QString comment, bool is_final,
                                    bool is_abstract, KeyValuesTable * kvt)
{
    const char * p = def;

    if (p == 0)
        return;

    QString indent = "";

    while ((*p == ' ') || (*p == '\t'))
        indent += *p++;

    s += indent;

    const char * pp = 0;

    for (;;) {
        if (*p == 0) {
            if (pp == 0)
                break;

            // comment management done
            p = pp;
            pp = 0;

            if (*p == 0)
                break;

            s += indent;
        }

        if (!strncmp(p, "${comment}", 10))
            manage_comment(comment, p, pp,
                           GenerationSettings::php_javadoc_style());
        else if (!strncmp(p, "${description}", 14))
            manage_description(comment, p, pp);
        else if (!strncmp(p, "${final}", 8)) {
            p += 8;

            if (is_final)
                s += "final ";
        }
        else if (!strncmp(p, "${abstract}", 11)) {
            p += 11;

            if (is_abstract)
                s += "abstract ";
        }
        else if (!strncmp(p, "${name}", 7)) {
            p += 7;
            s += name;
        }
        else if (!strncmp(p, "${extends}", 10)) {
            p += 10;
            php_generate_extends(s, stereotype, cl);
        }
        else if (!strncmp(p, "${implements}", 13)) {
            p += 13;
            php_generate_implements(s, stereotype, cl);
        }
        else if (!strncmp(p, "${members}", 10)) {
            p += 10;

            if (stereotype != "enum") {
                generate_members(cl->browser_node, s, indent,
                                 &AttributeData::get_phpdecl, &OperationData::get_phpdef,
                                 &RelationData::get_phpdecl_a, &RelationData::get_phpdecl_b,
                                 &ExtraMemberData::get_php_decl, &ClassData::get_phpdecl,
                                 php_stereotype);

                if (*p == '}')
                    s += indent;
            }
        }
        else if (!strncmp(p, "${items}", 8)) {
            p += 8;
            generate_members(cl->browser_node, s, indent,
                             &AttributeData::get_phpdecl, &OperationData::get_phpdef,
                             &RelationData::get_phpdecl_a, &RelationData::get_phpdecl_b,
                             &ExtraMemberData::get_php_decl, &ClassData::get_phpdecl,
                             php_stereotype);

            if (*p == '}')
                s += indent;
        }
        else if (*p == '\n') {
            s += *p++;

            if (*p && strncmp(p, "${members}", 10) && strncmp(p, "${items}", 8))
                s += indent;
        }
        else if (*p == '@')
            manage_alias(cl->browser_node, p, s, kvt);
        else
            s += *p++;
    }
}

void ClassDialog::php_update_decl()
{
    QString s;

    if (php_external_cb->isChecked())
        showphpdecl->setEnabled(FALSE);
    else {
        showphpdecl->setEnabled(TRUE);

        php_generate_decl(s, cl, edphpdecl->text(),
                          edname->text().stripWhiteSpace(),
                          current_php_stereotype, comment->text(),
                          php_final_cb->isChecked(),
                          abstract_cb->isChecked(), kvtable);
    }

    showphpdecl->setText(s);
}

void ClassDialog::php_default_decl()
{
    if (php_external_cb->isChecked())
        edphpdecl->setText(GenerationSettings::php_default_external_class_decl());
    else {
        current_php_stereotype =
            php_stereotype(edstereotype->currentText().stripWhiteSpace());

        if (current_php_stereotype == "enum")
            edphpdecl->setText(GenerationSettings::php_default_enum_decl());
        else if (current_php_stereotype == "interface")
            edphpdecl->setText(GenerationSettings::php_default_interface_decl());
        else if ((current_php_stereotype == "ignored") ||
                 (current_php_stereotype == "metaclass"))
            edphpdecl->setText(QString());
        else
            edphpdecl->setText(GenerationSettings::php_default_class_decl());

        php_update_decl();
    }
}

void ClassDialog::php_unmapped_decl()
{
    edphpdecl->setText(QString());
    showphpdecl->setText(QString());
}

static void python_generate_inherit(QString & s, ClassData * cl, bool object,
                                    BrowserNodeList & nodes,
                                    QStringList & node_names)
{
    BrowserNodeList inh;
    const char * sep = "(";

    cl->get_browser_node()->children(inh, UmlGeneralisation, UmlRealize);

    foreach (BrowserNode *item, inh) {
        RelationData * r = (RelationData *) item->get_data();

        if (r->get_cppdecl_a()[0]) {
            s += sep;
            sep = ", ";

            generate_mother(r->get_end_class(), FALSE, s, cl, 0, nodes, node_names);
        }
    }

    if (*sep != '(')
        s += ")";
    else if (object)
        s += "(object)";
}

void ClassDialog::python_generate_decl(QString & s, ClassData * cl, QString def,
                                       QString name, QString,
                                       QString comment, bool is_2_2,
                                       BrowserNodeList & nodes,
                                       QStringList & node_names,
                                       KeyValuesTable * kvt)
{
    const char * p = def;

    if (p == 0)
        return;

    const char * pp = 0;
    BooL indent_needed = FALSE;
    QString indent;
    QString saved_indent = indent;
    QString indent_step =
        GenerationSettings::python_get_indent_step();

    for (;;) {
        if (*p == 0) {
            if (pp == 0)
                break;

            // comment management done
            p = pp;

            if (*p == 0)
                break;

            pp = 0;
            indent = saved_indent;
        }

        if (!strncmp(p, "${comment}", 10))
            manage_python_comment(comment, p, pp);
        else if (!strncmp(p, "${description}", 14))
            manage_python_description(comment, p, pp);
        else if (!strncmp(p, "${docstring}", 12))
            manage_python_docstring(comment, p, pp, indent_needed, indent, saved_indent);
        else if (!strncmp(p, "${name}", 7)) {
            if (indent_needed) {
                indent_needed = FALSE;
                s += indent;
            }

            p += 7;
            s += name;
        }
        else if (!strncmp(p, "${inherit}", 10)) {
            p += 10;
            python_generate_inherit(s, cl, is_2_2, nodes, node_names);
        }
        else if (!strncmp(p, "${members}", 10)) {
            p += 10;
            generate_members(cl->browser_node, s, indent,
                             &AttributeData::get_pythondecl, &OperationData::get_pythondef,
                             &RelationData::get_pythondecl_a, &RelationData::get_pythondecl_b,
                             &ExtraMemberData::get_python_decl, &ClassData::get_pythondecl,
                             python_stereotype, FALSE, FALSE, FALSE, TRUE);
            Q3ListViewItem * child;
            bool has__init__ = FALSE;

            for (child = cl->browser_node->firstChild(); child; child = child->nextSibling()) {
                if (!((BrowserNode *) child)->deletedp() &&
                    (((BrowserNode *) child)->get_type() == UmlOperation) &&
                    !strcmp(((BrowserNode *) child)->get_name(), "__init__")) {
                    has__init__ = TRUE;
                    break;
                }
            }

            if (! has__init__)
                s += indent + "...__init__()\n";

            indent_needed = TRUE;
        }
        else if (*p == '@')
            manage_alias(cl->browser_node, p, s, kvt);
        else {
            if (indent_needed) {
                indent_needed = FALSE;
                s += indent;
            }

            switch (*p) {
            case ':':
                if (pp == 0) {
                    indent += indent_step;
                    saved_indent = indent;
                    indent_step = "";
                }

                break;

            case '\n':
                indent_needed = TRUE;
                break;
            }

            s += *p++;
        }
    }
}

void ClassDialog::python_update_decl()
{
    QString s;

    if (python_external_cb->isChecked())
        showpythondecl->setEnabled(FALSE);
    else {
        showpythondecl->setEnabled(TRUE);

        python_generate_decl(s, cl, edpythondecl->text(),
                             edname->text().stripWhiteSpace(),
                             current_python_stereotype, comment->text(),
                             python_2_2_cb->isChecked(), nodes, node_names,
                             kvtable);
    }

    showpythondecl->setText(s);
}

void ClassDialog::python_default_decl()
{
    if (python_external_cb->isChecked())
        edpythondecl->setText(GenerationSettings::python_default_external_class_decl());
    else {
        current_python_stereotype =
            python_stereotype(edstereotype->currentText().stripWhiteSpace());

        if (current_python_stereotype == "enum")
            edpythondecl->setText(GenerationSettings::python_default_enum_decl());
        else if ((current_python_stereotype == "ignored") ||
                 (current_python_stereotype == "metaclass"))
            edpythondecl->setText(QString());
        else
            edpythondecl->setText(GenerationSettings::python_default_class_decl());

        python_update_decl();
    }
}

void ClassDialog::python_unmapped_decl()
{
    edpythondecl->setText(QString());
    showpythondecl->setText(QString());
}

QString ClassDialog::python_instance_att_rel(BrowserNode * cl)
{
    QString s;

    generate_members(cl, s, "",
                     &AttributeData::get_pythondecl, &OperationData::get_pythondef,
                     &RelationData::get_pythondecl_a, &RelationData::get_pythondecl_b,
                     &ExtraMemberData::get_python_decl, &ClassData::get_pythondecl,
                     python_stereotype, FALSE, FALSE, FALSE, FALSE, TRUE);

    return s;
}


static void idl_generate_inherit(QString & s, QString st, ClassData * cl)
{
    if ((st == "union") || (st == "enum"))
        return;

    BrowserNodeList inh;
    bool is_valuetype = (st == "valuetype");
    const char * sep = " : ";
    bool already = FALSE;

    cl->get_browser_node()->children(inh, UmlGeneralisation, UmlRealize);

    foreach (BrowserNode *item, inh) {
        RelationData * r = (RelationData *) item->get_data();

        if (r->get_idldecl_a()[0]) {
            BrowserClass * mother = r->get_end_class();
            QString other_st =
                ClassDialog::idl_stereotype(mother->get_stereotype());

            if ((other_st != "ignored") &&
                (other_st != "metaclass") &&
                (other_st != "union") &&
                (other_st != "struct") &&
                (other_st != "enum") &&
                (other_st != "typedef")) {
                if (is_valuetype) {
                    if (other_st == "valuetype") {
                        s += sep;

                        if (r->get_idl_truncatable())
                            s += "truncatable ";
                    }
                    else {
                        // other_st == "interface"
                        if (!already) {
                            already = TRUE;
                            s += " supports ";
                        }
                        else
                            s += sep;
                    }
                }
                else
                    s += sep;

                s += mother->get_name();
                sep = ", ";
            }
        }
    }
}

void ClassDialog::idl_generate_decl(QString & s, ClassData * cl, QString def,
                                    QString name, QString stereotype,
                                    QString basetype, QString comment,
                                    QString switch_type, bool is_local,
                                    bool is_custom, bool is_abstract,
                                    KeyValuesTable * kvt)
{
    const char * p = def;

    if (p == 0)
        return;

    QString indent = "";

    while ((*p == ' ') || (*p == '\t'))
        indent += *p++;

    if (*p != '#')
        s += indent;

    const char * pp = 0;

    for (;;) {
        if (*p == 0) {
            if (pp == 0)
                break;

            // comment management done
            p = pp;
            pp = 0;

            if (*p == 0)
                break;

            s += indent;
        }

        if (!strncmp(p, "${comment}", 10))
            manage_comment(comment, p, pp, FALSE);
        else if (!strncmp(p, "${description}", 14))
            manage_description(comment, p, pp);
        else if (!strncmp(p, "${abstract}", 11)) {
            p += 11;

            if (is_abstract)
                s += "abstract ";
        }
        else if (!strncmp(p, "${local}", 8)) {
            p += 8;

            if (is_local)
                s += "local ";
        }
        else if (!strncmp(p, "${custom}", 9)) {
            p += 9;

            if (is_custom)
                s += "custom ";
        }
        else if (!strncmp(p, "${name}", 7)) {
            p += 7;
            s += name;
        }
        else if (*p == '\n') {
            s += *p++;

            if (*p && (*p != '#') &&
                strncmp(p, "${members}", 10) &&
                strncmp(p, "${items}", 8))
                s += indent;
        }
        else if (stereotype == "typedef") {
            if (!strncmp(p, "${type}", 7)) {
                p += 7;
                s += GenerationSettings::idl_type(basetype);
            }
            else
                s += *p++;
        }
        else if (!strncmp(p, "${inherit}", 10)) {
            p += 10;
            idl_generate_inherit(s, stereotype, cl);
        }
        else if (!strncmp(p, "${members}", 10)) {
            p += 10;
            generate_members(cl->browser_node, s, indent,
                             &AttributeData::get_idldecl, &OperationData::get_idldecl,
                             &RelationData::get_idldecl_a, &RelationData::get_idldecl_b,
                             &ExtraMemberData::get_idl_decl, &ClassData::get_idldecl,
                             idl_stereotype, stereotype == "union");

            if (*p == '}')
                s += indent;
        }
        else if (!strncmp(p, "${items}", 8)) {
            p += 8;
            generate_members(cl->browser_node, s, indent,
                             &AttributeData::get_idldecl, &OperationData::get_idldecl,
                             &RelationData::get_idldecl_a, &RelationData::get_idldecl_b,
                             &ExtraMemberData::get_idl_decl, &ClassData::get_idldecl,
                             idl_stereotype);

            if (*p == '}')
                s += indent;
        }
        else if (!strncmp(p, "${switch}", 9)) {
            p += 9;
            s += switch_type;
        }
        else if (*p == '@')
            manage_alias(cl->browser_node, p, s, kvt);
        else
            s += *p++;
    }
}

void ClassDialog::idl_update_decl()
{
    QString s;

    if (idl_external_cb->isChecked())
        showidldecl->setEnabled(FALSE);
    else {
        showidldecl->setEnabled(TRUE);

        idl_generate_decl(s, cl, edidldecl->text(),
                          edname->text().stripWhiteSpace(), current_idl_stereotype,
                          type(edbasetype->currentText().stripWhiteSpace(),
                               node_names, nodes),
                          comment->text(),
                          edswitch_type->currentText().stripWhiteSpace(),
                          idl_local_cb->isChecked(), idl_custom_cb->isChecked(),
                          abstract_cb->isChecked(), kvtable);
    }

    showidldecl->setText(s);
}

void ClassDialog::idl_default_decl()
{
    if (idl_external_cb->isChecked())
        edidldecl->setText(GenerationSettings::idl_default_external_class_decl());
    else {
        current_idl_stereotype =
            idl_stereotype(edstereotype->currentText().stripWhiteSpace());

        if (current_idl_stereotype == "struct")
            edidldecl->setText(GenerationSettings::idl_default_struct_decl());
        else if (current_idl_stereotype == "union")
            edidldecl->setText(GenerationSettings::idl_default_union_decl());
        else if (current_idl_stereotype == "enum")
            edidldecl->setText(GenerationSettings::idl_default_enum_decl());
        else if (current_idl_stereotype == "exception")
            edidldecl->setText(GenerationSettings::idl_default_exception_decl());
        else if (current_idl_stereotype == "typedef")
            edidldecl->setText(GenerationSettings::idl_default_typedef_decl());
        else if (current_idl_stereotype == "interface")
            edidldecl->setText(GenerationSettings::idl_default_interface_decl());
        else if ((current_idl_stereotype == "ignored") ||
                 (current_idl_stereotype == "metaclass"))
            edidldecl->setText(QString());
        else
            edidldecl->setText(GenerationSettings::idl_default_valuetype_decl());

        idl_update_decl();
    }
}

void ClassDialog::idl_unmapped_decl()
{
    edidldecl->setText(QString());
    showidldecl->setText(QString());
}

void ClassDialog::accept()
{
    SaveData();
    Q3TabDialog::accept();
}

//
// Formal params
//

// copy/cut/paste
QString FormalParamsTable::name_copy;
QString FormalParamsTable::type_copy;
QString FormalParamsTable::default_value_copy;
QString FormalParamsTable::extends_copy;

FormalParamsTable::FormalParamsTable(ClassData * cl, QWidget * parent,
                                     QStringList & node_names, bool visit)
    : MyTable(cl->get_n_formalparams() + 1, (visit) ? 4 : 5, parent),
      types(node_names)
{
    setSorting(true);
    setSelectionMode(NoSelection);	// single does not work
    setRowMovingEnabled(TRUE);
    horizontalHeader()->setLabel(0, TR("Type (C++)"));
    horizontalHeader()->setLabel(1, TR("Name"));
    horizontalHeader()->setLabel(2, TR("Default value (C++)"));
    horizontalHeader()->setLabel(3, "Extends (Java)");

    if (! visit)
        horizontalHeader()->setLabel(4, TR("do"));

    int index;
    int sup = cl->get_n_formalparams();

    if (visit) {
        for (index = 0; index < sup; index += 1) {
            setItem(index, 0, new TableItem(this, Q3TableItem::Never, cl->get_formalparam_type(index)));
            setItem(index, 1, new TableItem(this, Q3TableItem::Never, cl->get_formalparam_name(index)));
            setItem(index, 2, new TableItem(this, Q3TableItem::Never,
                                            cl->get_formalparam_default_value(index, !node_names.isEmpty())));
            setItem(index, 3, new TableItem(this, Q3TableItem::Never,
                                            cl->get_formalparam_extends(index, !node_names.isEmpty())));
        }
    }
    else {
        for (index = 0; index < sup; index += 1) {
            setText(index, 0, cl->get_formalparam_type(index));
            setText(index, 1, cl->get_formalparam_name(index));
            setItem(index, 2, new ComboItem(this, cl->get_formalparam_default_value(index, TRUE), types));
            setItem(index, 3, new ComboItem(this, cl->get_formalparam_extends(index, TRUE), types));
            setText(index, 4, QString());
        }

        setText(index, 0, "class");
        setText(index, 1, QString());
        setItem(index, 2, new ComboItem(this, QString(), types));
        setItem(index, 3, new ComboItem(this, QString(), types));
        setText(index, 4, QString());
    }

    setColumnStretchable(0, TRUE);
    setColumnStretchable(1, TRUE);
    setColumnStretchable(2, TRUE);
    setColumnStretchable(3, TRUE);

    if (! visit) {
        adjustColumn(4);
        setColumnStretchable(4, FALSE);

        connect(this, SIGNAL(pressed(int, int, int, const QPoint &)),
                this, SLOT(button_pressed(int, int, int, const QPoint &)));
        connect(this, SIGNAL(valueChanged(int, int)),
                this, SLOT(value_changed(int, int)));
    }
}

void FormalParamsTable::Reinitiliaze(ClassData *cl, QStringList &node_names, bool isWritable)
{
    int oldRowCount = numRows()-1;
    for (int index =  oldRowCount; index > -1; index -= 1) {
        removeRow(index);
    }
    int sup = cl->get_n_formalparams();
    types = node_names;
    setNumRows(sup + 1);

    if (isWritable)
    {
        setNumCols(5);
        adjustColumn(4);
        setColumnStretchable(4, FALSE);
        horizontalHeader()->setLabel(4, TR("do"));

        connect(this, SIGNAL(pressed(int, int, int, const QPoint &)),
                this, SLOT(button_pressed(int, int, int, const QPoint &)));
        connect(this, SIGNAL(valueChanged(int, int)),
                this, SLOT(value_changed(int, int)));

        int index;
        for ( index = 0; index < sup; index += 1) {
            setText(index, 0, cl->get_formalparam_type(index));
            setText(index, 1, cl->get_formalparam_name(index));
            setItem(index, 2, new ComboItem(this, cl->get_formalparam_default_value(index, TRUE), types));
            setItem(index, 3, new ComboItem(this, cl->get_formalparam_extends(index, TRUE), types));
            setText(index, 4, QString());
        }

        setText(index, 0, "class");
        setText(index, 1, QString());
        setItem(index, 2, new ComboItem(this, QString(), types));
        setItem(index, 3, new ComboItem(this, QString(), types));
        setText(index, 4, QString());
    }
    else
    {
        setNumCols(4);
        disconnect(this, SIGNAL(pressed(int, int, int, const QPoint &)),
                   this, SLOT(button_pressed(int, int, int, const QPoint &)));
        disconnect(this, SIGNAL(valueChanged(int, int)),
                   this, SLOT(value_changed(int, int)));

        for (int index = 0; index < sup; index += 1) {
            setItem(index, 0, new TableItem(this, Q3TableItem::Never, cl->get_formalparam_type(index)));
            setItem(index, 1, new TableItem(this, Q3TableItem::Never, cl->get_formalparam_name(index)));
            setItem(index, 2, new TableItem(this, Q3TableItem::Never,
                                            cl->get_formalparam_default_value(index, !node_names.isEmpty())));
            setItem(index, 3, new TableItem(this, Q3TableItem::Never,
                                            cl->get_formalparam_extends(index, !node_names.isEmpty())));

        }


        for (int index = 0; index < sup; index += 1) {
            setItem(index, 0, new TableItem(this, Q3TableItem::Never, cl->get_actualparam_name(index)));

            if (!isWritable)
                setItem(index, 1, new TableItem(this, Q3TableItem::Never, cl->get_actualparam_value(index)));
            else
                setItem(index, 1, new ComboItem(this, cl->get_actualparam_value(index), types));
        }
    }
}

void FormalParamsTable::activateNextCell()
{
    int row = currentRow();
    int col = currentColumn();

    if (col == 3) {
        // goto next line
        if (++row == numRows()) {
            // adds a new line
            setNumRows(row + 1);
            setText(row, 0, "class");
            setText(row, 1, QString());
            setItem(row, 2, new ComboItem(this, QString(), types));
            setItem(row, 3, new ComboItem(this, QString(), types));
            setText(row, 4, QString());
        }

        setCurrentCell(row, 0);
    }
    else
        setCurrentCell(row, col + 1);
}

void FormalParamsTable::value_changed(int row, int col)
{
    if ((row == (numRows() - 1)) && !text(row, col).isEmpty())
        insert_row_after(row);
}

void FormalParamsTable::button_pressed(int row, int col, int, const QPoint &)
{
    if (col == 4) {
        char s[16];

        sprintf(s, "%d", row + 1);
        Q3PopupMenu m;
        m.insertItem(TR("formal param %1", s), -1);
        m.insertSeparator();
        m.insertItem(TR("Insert param before"), 0);
        m.insertItem(TR("Insert param after"), 1);
        m.insertSeparator();
        m.insertItem(TR("Delete param"), 2);
        m.insertSeparator();
        m.insertItem(TR("Copy param"), 3);
        m.insertItem(TR("Cut param"), 4);
        m.insertItem(TR("Paste param"), 5);
        m.insertSeparator();

        Q3PopupMenu mv;
        int rank;

        for (rank = 0; rank != numRows(); rank += 1)
            if (rank != row)
                mv.insertItem(QString::number(rank + 1), 10 + rank);

        m.insertItem(TR("Move param"), &mv);

        switch (rank = m.exec(QCursor::pos())) {
        case 0:
            insert_row_before(row);
            break;

        case 1:
            insert_row_after(row);
            break;

        case 2:
            delete_row(row);
            break;

        case 3:
            copy_row(row);
            break;

        case 4:
            cut_row(row);
            break;

        case 5:
            paste_row(row);
            break;

        default:
            if (rank >= 10)
                move_row(row, rank - 10);

            break;
        }
    }
}

void FormalParamsTable::insert_row_before(int row)
{
    int n = numRows();
    int index;

    setNumRows(n + 1);

    for (index = n; index != row; index -= 1) {
        setText(index, 0, text(index - 1, 0));
        setText(index, 1, text(index - 1, 1));

        Q3TableItem * it;

        it = item(index - 1, 2);
        takeItem(it);
        setItem(index, 2, it);

        it = item(index - 1, 3);
        takeItem(it);
        setItem(index, 3, it);
    }

    setText(row, 0, "class");
    setText(row, 1, QString());
    setItem(row, 2, new ComboItem(this, QString(), types));
    setItem(row, 3, new ComboItem(this, QString(), types));
    setText(row, 4, QString());
}

void FormalParamsTable::insert_row_after(int row)
{
    int n = numRows();
    int index;

    setNumRows(n + 1);

    for (index = n; index > row + 1; index -= 1) {
        setText(index, 0, text(index - 1, 0));
        setText(index, 1, text(index - 1, 1));

        Q3TableItem * it;

        it = item(index - 1, 2);
        takeItem(it);
        setItem(index, 2, it);

        it = item(index - 1, 3);
        takeItem(it);
        setItem(index, 3, it);
    }

    setText(row + 1, 0, "class");
    setText(row + 1, 1, QString());
    setItem(row + 1, 2, new ComboItem(this, QString(), types));
    setItem(row + 1, 3, new ComboItem(this, QString(), types));
    setText(row + 1, 4, QString());
}

void FormalParamsTable::delete_row(int row)
{
    int n = numRows();
    int index;

    clearCellWidget(row, 3);

    if (n == 1) {
        // the alone line : empty it
        setText(0, 0, "class");
        setText(0, 1, QString());
        setItem(0, 2, new ComboItem(this, QString(), types));
        setItem(0, 3, new ComboItem(this, QString(), types));
    }
    else {
        for (index = row; index != n - 1; index += 1) {
            setText(index, 0, text(index + 1, 0));
            setText(index, 1, text(index + 1, 1));

            Q3TableItem * it;

            it = item(index + 1, 2);
            takeItem(it);
            setItem(index, 2, it);

            it = item(index + 1, 3);
            takeItem(it);
            setItem(index, 3, it);
        }

        setNumRows(n - 1);
    }
}

void FormalParamsTable::copy_row(int row)
{
    type_copy = text(row, 0);
    name_copy = text(row, 1);
    default_value_copy = text(row, 2);
    extends_copy = text(row, 3);
}

void FormalParamsTable::cut_row(int row)
{
    copy_row(row);
    delete_row(row);
}

void FormalParamsTable::paste_row(int row)
{
    setText(row, 0, type_copy);
    setText(row, 1, name_copy);
    setText(row, 2, default_value_copy);
    setText(row, 3, extends_copy);

    if ((row == (numRows() - 1)) && !name_copy.isEmpty())
        insert_row_after(row);
}

void FormalParamsTable::move_row(int from, int to)
{
    QString save_type_copy = type_copy;
    QString save_name_copy = name_copy;
    QString save_default_value_copy = default_value_copy;
    QString save_extends_copy = extends_copy;

    cut_row(from);

    if (to > from)
        insert_row_after(to - 1);
    else
        insert_row_before(to);

    paste_row(to);

    type_copy = save_type_copy;
    name_copy = save_name_copy;
    default_value_copy = save_default_value_copy;
    extends_copy = save_extends_copy;
}

void FormalParamsTable::generate_cpp(QString & s, BrowserNodeList & nodes,
                                     QStringList & node_names,
                                     const QString & cl_name)
{
    int n = numRows();

    if (text(n - 1, 1).stripWhiteSpace().isEmpty())
        n -= 1;

    if (n != 0) {
        const char * sep = "template<";

        for (int index = 0; index != n; index += 1) {
            s += sep;
            s += text(index, 0).stripWhiteSpace();
            s += " ";
            s += text(index, 1).stripWhiteSpace();
            sep = ", ";

            QString v = type(text(index, 2).stripWhiteSpace(), node_names, nodes);

            if (! v.isEmpty()) {
                s += " = ";

                s += v;
            }
        }

        s += "> ";
    }
    else if (cl_name.find('<') != -1)
        s += "template<> ";
}

void FormalParamsTable::generate_java(QString & s, BrowserNodeList & nodes,
                                      QStringList & node_names)
{
    int n = numRows();

    if (text(n - 1, 1).stripWhiteSpace().isEmpty())
        n -= 1;

    if (n != 0) {
        const char * sep = "<";

        for (int index = 0; index != n; index += 1) {
            s += sep;
            s += text(index, 1).stripWhiteSpace();

            QString ex = type(text(index, 3).stripWhiteSpace(), node_names, nodes);

            if (! ex.isEmpty()) {
                s += " extends ";
                s += ex;
            }

            sep = ", ";
        }

        s += "> ";
    }
}

void FormalParamsTable::update(ClassData * cl, BrowserNodeList & nodes)
{
    forceUpdateCells();

    int n = numRows();
    int index;

    if (text(n - 1, 1).stripWhiteSpace().isEmpty())
        n -= 1;

    cl->set_n_formalparams(n);

    for (index = 0; index != n; index += 1) {
        cl->set_formalparam_type(index, text(index, 0).stripWhiteSpace());
        cl->set_formalparam_name(index, text(index, 1).stripWhiteSpace());

        int rank;

        rank = types.findIndex(text(index, 2).stripWhiteSpace());

        if (rank != -1)
            cl->set_formalparam_default_value(index, ((BrowserClass *) nodes.at(rank)),
                                              NULL); // [lgfreitas] expected const char *. QString() does not correspond to old null
        else
            cl->set_formalparam_default_value(index, 0, text(index, 2).stripWhiteSpace());

        rank = types.findIndex(text(index, 3).stripWhiteSpace());

        if (rank != -1)
            cl->set_formalparam_extends(index, ((BrowserClass *) nodes.at(rank)),
                                        NULL);
        else
            cl->set_formalparam_extends(index, 0, text(index, 3).stripWhiteSpace());
    }
}

//
// Actual params
//

ActualParamsTable::ActualParamsTable(ClassData * cl, QWidget * parent,
                                     QStringList & node_names, bool isWritable)
    : MyTable(cl->get_n_actualparams(), 2, parent), types(node_names)
{
    setSorting(true);
    setSelectionMode(NoSelection);	// single does not work
    setRowMovingEnabled(FALSE);
    horizontalHeader()->setLabel(0, TR("inherited class formal"));
    horizontalHeader()->setLabel(1, TR("actual"));

    int index;
    int sup = cl->get_n_actualparams();

    for (index = 0; index < sup; index += 1) {
        setItem(index, 0, new TableItem(this, Q3TableItem::Never, cl->get_actualparam_name(index)));

        if (!isWritable)
            setItem(index, 1, new TableItem(this, Q3TableItem::Never, cl->get_actualparam_value(index)));
        else
            setItem(index, 1, new ComboItem(this, cl->get_actualparam_value(index), types));
    }

    adjustColumn(0);
    setColumnStretchable(0, FALSE);
    setColumnStretchable(1, TRUE);
}

void ActualParamsTable::Reinitiliaze(ClassData * cl, QStringList & node_names, bool isWritable)
{
    int oldRowCount = numRows()-1;
    for (int index =  oldRowCount; index > -1; index -= 1) {
        removeRow(index);
    }

    int sup = cl->get_n_actualparams();
    types = node_names;
    setNumRows(sup);
    for (int index = 0; index < sup; index += 1) {
        setItem(index, 0, new TableItem(this, Q3TableItem::Never, cl->get_actualparam_name(index)));

        if (!isWritable)
            setItem(index, 1, new TableItem(this, Q3TableItem::Never, cl->get_actualparam_value(index)));
        else
            setItem(index, 1, new ComboItem(this, cl->get_actualparam_value(index), types));
    }
    types = node_names;

}

#if 0
void ActualParamsTable::activateNextCell()
{
    int row = currentRow();

    if (row != numRows())
        // goto next line
        setCurrentCell(row + 1, 1);
}
#endif

void ActualParamsTable::update(ClassData * cl, BrowserNodeList & nodes)
{
    forceUpdateCells();

    int n = numRows();
    int index;

    for (index = 0; index != n; index += 1)
        cl->set_actualparam_value(index,
                                  the_type(text(index, 1).stripWhiteSpace(),
                                           types, nodes));
}

void ActualParamsTable::generate(QString & s, ClassData * cl,
                                 BrowserNode * parent, bool cpp,
                                 BrowserNodeList & nodes,
                                 QStringList & node_names)
{
    unsigned index = 0;
    QListIterator<ActualParamData *> it(cl->actuals);

    for (index = 0; it.hasNext(); index += 1) {
        if (it.next()->get_class() == parent) {
            it.previous();
            break;
        }
    }

    int n = ((ClassData *) parent->get_data())->get_n_formalparams();
    int nth = 0;
    const char * sep = "<";
    bool need_space = FALSE;

    // progress on still present formals
    while (it.hasNext() && (nth < n) && (it.next()->get_class() == parent)) {
        AType t = the_type(text(index, 1).stripWhiteSpace(), node_names, nodes);

        if (t.type != 0) {
            s += sep;
            s += t.type->get_name();

            if (cpp && (((ClassData *) t.type->get_data())->get_n_formalparams() != 0)) {
                ClassData * cl2 = (ClassData *) t.type->get_data();
                int n2 = cl2->get_n_formalparams();
                const char * sep2 = "<";
                int index2;

                for (index2 = 0; index2 != n2; index2 += 1) {
                    s += sep2;
                    s += cl2->get_formalparam_name(index2);
                    sep = ", ";
                }

                s += ">";
                need_space = TRUE;
            }
        }
        else if (!t.explicit_type.isEmpty()) {
            s += sep;
            s += (cpp) ? GenerationSettings::cpp_type(t.explicit_type)
                 : GenerationSettings::java_type(t.explicit_type);
            need_space = FALSE;
        }
        else {
            if (nth == 0)
                s += "<>";

            break;
        }

        sep = ", ";
        nth += 1;
        index += 1;
    }

    if (nth != 0) {
        if (need_space)
            s += " >";
        else
            s += '>';
    }
}

//
// Stereotype target
//

ApplicableOnTable::ApplicableOnTable(QWidget * parent, QString s, bool visit)
    : MyTable(ProfiledStereotypes::availableTargets().count(), 2, parent),
      ro(visit)
{
    setSorting(true);
    setSelectionMode(NoSelection);	// single does not work
    setRowMovingEnabled(FALSE);
    horizontalHeader()->setLabel(0, TR("element kind"));
    horizontalHeader()->setLabel(1, TR("applicable"));

    const QStringList & available = ProfiledStereotypes::availableTargets();
    QStringList l = QStringList::split(" ", s);
    QStringList::ConstIterator it;
    int row = 0;
    QString yes = TR("yes");
    QString empty;

    for (it = available.begin(); it != available.end(); ++it) {
        setItem(row, 0,
                new TableItem(this, Q3TableItem::Never,
                              TR(ProfiledStereotypes::pretty(*it))));
        setItem(row, 1,
                new TableItem(this, Q3TableItem::Never,
                              (l.findIndex(*it) == -1) ? empty : yes));
        row += 1;
    }

    connect(this, SIGNAL(pressed(int, int, int, const QPoint &)),
            this, SLOT(button_pressed(int, int, int, const QPoint &)));

    adjustColumn(0);
    adjustColumn(1);
}

void ApplicableOnTable::button_pressed(int row, int col, int, const QPoint &)
{
    if (!ro && (col == 1))
        setText(row, col, (text(row, col).isEmpty()) ? TR("yes") : QString());
}

QString ApplicableOnTable::targets()
{
    forceUpdateCells();

    const QStringList & available = ProfiledStereotypes::availableTargets();
    QStringList::ConstIterator it;
    QString s;
    int row;

    for (row = 0, it = available.begin(); it != available.end(); row += 1, ++it)
        if (!text(row, 1).isEmpty())
            s += (*it) + " ";

    if (! s.isEmpty())
        s.truncate(s.length() - 1);

    return s;
}

uint ClassDialog::TypeID()
{
    return TypeIdentifier<ClassDialog>::id();
}

void ClassDialog::InitGui()
{

    setCaption(TR("Class dialog"));

    currentNode = (BrowserClass *) cl->get_browser_node();
    grandParent = (BrowserNode *) currentNode->parent()->parent();


    if (isWritable) {
        BrowserClass::instances(nodes);
        nodes.full_names(node_names);
    }

    Q3HBox * htab;
    Q3VBox * vtab;
    Q3Grid * grid;
    QSplitter * split;
    Q3GroupBox * bg;
//    QLabel * lbl1;
//    QLabel * lbl2;
//    QLabel * lbl3;
//    QLabel * lbl4;
    currentNode = (BrowserClass *) cl->get_browser_node();
    grandParent = (BrowserNode *) currentNode->parent()->parent();

    //!!!!! general tab elements
    grid = new Q3Grid(2, this);
    umltab = grid;
    RegisterTab("Uml", umltab);
    grid->setSpacing(5);
    grid->setMargin(5);

    new QLabel(TR("name : "), grid);
    edname = new LineEdit(cl->name(), grid); //toadd

    new QLabel(TR("stereotype : "), grid);

    edstereotype = new Q3ComboBox(isWritable, grid); // toadd

    // setting up checkboxes for active and abstract
    new QLabel(grid);
    htabUml = new Q3HBox(grid);
    opt_bg = new Q3GroupBox(2, Qt::Horizontal, QString(), htabUml);
    abstract_cb = new QCheckBox("abstract", opt_bg);
    active_cb = new QCheckBox("active", opt_bg);

    // setting up radiobutton group for accessibility
    // ASSUMING INSTANTIATE ON NULL
    uml_visibility.init(htabUml, cl->get_uml_visibility(), TRUE); //toAdd

    basetypelbl = new QLabel(TR("base type : "), grid);
    edbasetype = new Q3ComboBox(isWritable, grid);

    new QLabel(TR("artifact : "), grid);

    artifact = new Q3ComboBox(FALSE, grid);


    vtab = new Q3VBox(grid);
    new QLabel(TR("description :"), vtab);

    pbEditorForDescription = new SmallPushButton(TR("Editor"), vtab);
    pbDefaultForDescription = new SmallPushButton(TR("Default"), vtab);


    comment = new MultiLineEdit(grid);
    comment->setReadOnly(!isWritable);
    comment->setFont(font);

    vtab = new Q3VBox(grid);
    new QLabel(TR("constraint :"), vtab);
    pbEditorForConstrant = new SmallPushButton(TR("Editor"), vtab);

    constraint = new MultiLineEdit(grid);
    constraint->setReadOnly(!isWritable);
    constraint->setFont(font);

    addTab(grid, "Uml");


    // parameterized tab

    RegisterTab("Parametrized", parametrized_vtab);
    parametrized_vtab = new Q3VBox(this);

    parametrized_vtab->setMargin(5);


    (new QLabel(TR("\nEnter formals in case the class is parameterized\n"),
                parametrized_vtab))
    ->setAlignment(Qt::AlignHCenter | Qt::AlignTop);


    formals_table = new FormalParamsTable(cl, parametrized_vtab, node_names, isWritable); //todo VISIT NOT FIXED
    addTab(parametrized_vtab, TR("Parameterized"));


    //!!! parameters for a class

    instantiate_vtab = new Q3VBox(this);
    RegisterTab("Instantiate", instantiate_vtab);
    instantiate_vtab->setMargin(5);
    instantiateNotice = new QLabel(TR("\nSpecify actuals else formals default value will be used\n"),
                                   instantiate_vtab);
    instantiateNotice->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    actuals_table = new ActualParamsTable(cl, instantiate_vtab, node_names, isWritable); //todo VISIT NOT FIXED
    addTab(instantiate_vtab, TR("Instantiate"));

    //!!! C++
    split = new QSplitter(Qt::Vertical, this);
    cppTab = split;
    RegisterTab("Cpp", cppTab);
    split->setOpaqueResize(TRUE);

    vtab = new Q3VBox(split);

    htab = new Q3HBox(vtab);
    htab->setMargin(5);
    lbl1cpp = new QLabel(htab);
    bg = new Q3GroupBox(1, Qt::Horizontal, QString(), htab);
    cpp_external_cb = new QCheckBox("external", bg);


    cpp_visibility.init(htab, cl->get_cpp_visibility(),
                        FALSE, 0, TR("follow uml"));




    htab = new Q3HBox(vtab);
    htab->setMargin(5);
    lbl2cpp = new QLabel(TR("Declaration : "), htab);
    edcppdecl = new MultiLineEdit(htab);

    vtab = new Q3VBox(split);

    htab = new Q3HBox(vtab);
    htab->setMargin(5);
    lbl3cpp = new QLabel(TR("Result after\nsubstitution : "), htab);
    showcppdecl = new MultiLineEdit(htab);

    htabcpp = new Q3HBox(vtab);
    lbl4cpp = new QLabel(htabcpp);

    pbCppDefaultDeclaration = new QPushButton(TR("Default declaration"), htabcpp);
    pbNotGeneratedInCPP = new QPushButton(TR("Not generated in C++"), htabcpp);

    addTab(cppTab, "Cpp");

    //!!! JAVA

    split = new QSplitter(Qt::Vertical, this);
    javatab = split;
    RegisterTab("Java", javatab);
    split->setOpaqueResize(TRUE);

    vtab = new Q3VBox(split);

    htab = new Q3HBox(vtab);
    htab->setMargin(5);
    lbl1java = new QLabel(htab);
    bg = new Q3GroupBox(3, Qt::Horizontal, QString(), htab);
    java_final_cb = new QCheckBox("final", bg);
    java_external_cb = new QCheckBox("external", bg);


    htab = new Q3HBox(vtab);
    htab->setMargin(5);
    lbl2java = new QLabel(TR("Definition : "), htab);
    edjavadecl = new MultiLineEdit(htab);


    vtab = new Q3VBox(split);

    htab = new Q3HBox(vtab);
    htab->setMargin(5);
    lbl3java = new QLabel(TR("Result after\nsubstitution : "), htab);
    showjavadecl = new MultiLineEdit(htab);

    htab = new Q3HBox(vtab);
    lbl4java = new QLabel(htab);
    pbJavaDefaultDefinition = new QPushButton(TR("Default definition"), htab);
    pbNotGeneratedInJava = new QPushButton(TR("Not generated in Java"), htab);

    pbJavaAnnotation = new QPushButton("", htab);

    connect(pbJavaAnnotation, SIGNAL(clicked()),
            this, SLOT(java_edit_annotation()));

    addTab(split, "Java");


    //!!! PHP
    split = new QSplitter(Qt::Vertical, this);
    phptab = split;
    RegisterTab("Php", phptab);
    split->setOpaqueResize(TRUE);

    vtab = new Q3VBox(split);

    htab = new Q3HBox(vtab);
    htab->setMargin(5);
    lbl1php = new QLabel(htab);
    bg = new Q3GroupBox(3, Qt::Horizontal, QString(), htab);
    php_final_cb = new QCheckBox("final", bg);
    php_external_cb = new QCheckBox("external", bg);

    htab = new Q3HBox(vtab);
    htab->setMargin(5);
    lbl2php = new QLabel(TR("Definition : "), htab);
    edphpdecl = new MultiLineEdit(htab);
    edphpdecl->setFont(font);


    vtab = new Q3VBox(split);

    htab = new Q3HBox(vtab);
    htab->setMargin(5);
    pbPhpDefaultDefinition =  new QPushButton(TR("Default definition"), htab);
    pbNotGeneratedInPhp =  new QPushButton(TR("Not generated in Php"), htab);
    lbl3php = new QLabel(TR("Result after\nsubstitution : "), htab);
    showphpdecl = new MultiLineEdit(htab);
    showphpdecl->setReadOnly(TRUE);
    showphpdecl->setFont(font);

    htab = new Q3HBox(vtab);
    lbl4php = new QLabel(htab);


    addTab(phptab, "Php");



    // PYTHON

    split = new QSplitter(Qt::Vertical, this);
    pythontab = split;
    RegisterTab("Python", pythontab);
    split->setOpaqueResize(TRUE);

    vtab = new Q3VBox(split);

    htab = new Q3HBox(vtab);
    htab->setMargin(5);
    lbl1python = new QLabel(htab);
    bg = new Q3GroupBox(3, Qt::Horizontal, QString(), htab);
    python_2_2_cb = new QCheckBox("Python 2.2", bg);
    python_external_cb = new QCheckBox("external", bg);


    htab = new Q3HBox(vtab);
    htab->setMargin(5);
    lbl2python = new QLabel(TR("Definition : "), htab);
    edpythondecl = new MultiLineEdit(htab);

    vtab = new Q3VBox(split);

    htab = new Q3HBox(vtab);
    htab->setMargin(5);
    lbl3python = new QLabel(TR("Result after\nsubstitution : "), htab);
    showpythondecl = new MultiLineEdit(htab);
    showpythondecl->setReadOnly(TRUE);
    showpythondecl->setFont(font);

    htab = new Q3HBox(vtab);
    lbl4python = new QLabel(htab);

    pbPythonDefaultDefinition = new QPushButton(TR("Default definition"), htab);
    pbNotGeneratedInPython = new QPushButton(TR("Not generated in Python"), htab);

    addTab(pythontab, "Python");



    //IDL


    split = new QSplitter(Qt::Vertical, this);
    idltab = split;
    RegisterTab("Idl", idltab);
    split->setOpaqueResize(TRUE);

    vtab = new Q3VBox(split);

    htab = new Q3HBox(vtab);
    htab->setMargin(5);
    switch_bg = new Q3GroupBox(2, Qt::Horizontal, QString(), htab);
    new QLabel(TR("switch type : "), switch_bg);
    edswitch_type = new Q3ComboBox(isWritable, switch_bg);
    // !!!
    QSizePolicy sp = edstereotype->sizePolicy();
    sp.setHorData(QSizePolicy::Expanding);

    edswitch_type->setSizePolicy(sp);

    htab = new Q3HBox(vtab);
    htab->setMargin(5);
    lbl1idl = new QLabel(htab);
    bg = new Q3GroupBox(3, Qt::Horizontal, QString(), htab);
    idl_external_cb = new QCheckBox("external", bg);
    idl_local_cb = new QCheckBox("local", bg);
    idl_custom_cb = new QCheckBox("custom", bg);

    htab = new Q3HBox(vtab);
    htab->setMargin(5);
    lbl2idl = new QLabel(TR("Declaration : "), htab);
    edidldecl = new MultiLineEdit(htab);
    edidldecl->setFont(font);



    vtab = new Q3VBox(split);

    htab = new Q3HBox(vtab);
    htab->setMargin(5);
    lbl3idl = new QLabel(TR("Result after\nsubstitution : "), htab);
    showidldecl = new MultiLineEdit(htab);
    showidldecl->setReadOnly(TRUE);
    showidldecl->setFont(font);


    htabidl = new Q3HBox(vtab);
    lbl4idl = new QLabel(htabidl);
    pbIdlDefaultDeclaration = new QPushButton(TR("Default declaration"), htabidl);
    pbINotGeneratedInIdl = new QPushButton(TR("Not generated in IDL"), htabidl);

    addTab(idltab, "Idl");

    // Profiled stereotype
    stereotypeGrid = new Q3Grid(2, this);
    stereotypetab = stereotypeGrid;
    RegisterTab("Stereotype", stereotypetab);
    stereotypeGrid->setSpacing(5);
    stereotypeGrid->setMargin(5);

    new QLabel(TR("Initialization \nplug-out :"), stereotypeGrid);
    htab = new Q3HBox(stereotypeGrid);
    stereo_init_cb = new Q3ComboBox(FALSE, htab);

    new QLabel(TR("  parameter(s) : "), htab);
    edinitparam = new LineEdit(currentNode->get_value("stereotypeSetParameters"), htab);

    new QLabel(TR("Check \nplug-out :"), stereotypeGrid);
    htab = new Q3HBox(stereotypeGrid);
    stereo_check_cb = new Q3ComboBox(FALSE, htab);

    new QLabel(TR("  parameter(s) : "), htab);
    edcheckparam = new LineEdit(currentNode->get_value("stereotypeCheckParameters"), htab);

    new QLabel(TR("Icon path :"), stereotypeGrid);
    htab = new Q3HBox(stereotypeGrid);
    ediconpath = new LineEdit("", htab);

    lblProfiledEmpty = new QLabel("", htab);
    pbProfiledSteretypeBrowse = new SmallPushButton(TR("Browse"), htab);
    lblProfiledEmpty2 = new QLabel("", htab);
    vtabProfiled = new Q3VBox(htab);
    QString ip = currentNode->get_value("stereotypeIconPath");
    iconpathrootbutton = new SmallPushButton((ip.isEmpty() || QDir::isRelativePath(ip))
            ? Absolute : RelativeRoot, vtab);
    iconpathprjbutton = new SmallPushButton((ip.isEmpty() || QDir::isRelativePath(ip))
                                            ? Absolute : RelativePrj, vtab);

    lblProfiledEmpty3 = new QLabel("", htab);
    new QLabel(TR("Apply on : "), stereotypeGrid);
    applicableon_table =
        new ApplicableOnTable(stereotypeGrid, "", !isWritable); // todo!! important !!! turned off parameter

    addTab(stereotypetab, TR("Stereotype"));

    // USER : list key - value

    keyValueTab = new Q3VBox(this);
    RegisterTab("Properties", keyValueTab);
    kvtable = new KeyValuesTable(currentNode, keyValueTab, !isWritable);
    kvtable->remove("stereotypeSet");

    addTab(keyValueTab, TR("Properties"));

}

void ClassDialog::FillGuiElements(BrowserNode * bn)
{
    if (dynamic_cast<ClassData *>(bn->get_data()))
        FillGuiElements(dynamic_cast<ClassData *>(bn->get_data()));
}

void ClassDialog::FillGuiElements(ClassData * _cl)
{
    disconnect(this, SIGNAL(currentChanged(QWidget *)),
               this, SLOT(update_all_tabs(QWidget *)));
    cl = _cl;
    SetCurrentNode(cl->browser_node);

    isWritable = cl->browser_node->is_writable();
    SetDialogMode(isWritable);

    nodes.clear();
    node_names.clear();

    if (isWritable) {
        BrowserClass::instances(nodes);
        nodes.full_names(node_names);
    }

    // general tab
    currentNode = (BrowserClass *) cl->get_browser_node();
    grandParent = (BrowserNode *) currentNode->parent()->parent();
    edname->setText(cl->get_browser_node()->get_name());
    edname->setReadOnly(!isWritable);
    edinitparam->setText(currentNode->get_value("stereotypeSetParameters"));
    edcheckparam->setText(currentNode->get_value("stereotypeCheckParameters"));
    // filling stereotypes edit element
    edstereotype->clear();
    edstereotype->insertItem(toUnicode(cl->get_stereotype()));

    if (isWritable) {

        bool grannyIsUmlPackage = grandParent->get_type() == UmlPackage;
        bool grannyIsProfiled = !strcmp(grandParent->get_data()->get_stereotype(), "profile");

        if (grannyIsUmlPackage && grannyIsProfiled)
            edstereotype->insertStringList(BrowserClass::default_stereotypes());
        else {
            QStringList stereotypesList = BrowserClass::default_stereotypes();

            stereotypesList.remove("stereotype");
            edstereotype->insertStringList(stereotypesList);
        }

        edstereotype->insertStringList(ProfiledStereotypes::defaults(UmlClass));
        edstereotype->setAutoCompletion(completion());
        connect(edstereotype, SIGNAL(activated(const QString &)),
                this, SLOT(edStereotypeActivated(const QString &)));
    }
    else {
        disconnect(edstereotype, SIGNAL(activated(const QString &)),
                   this, SLOT(edStereotypeActivated(const QString &)));
        edstereotype->clear();
    }

    edstereotype->setCurrentItem(0);
    QSizePolicy sp = edstereotype->sizePolicy();
    sp.setHorData(QSizePolicy::Expanding);
    edstereotype->setSizePolicy(sp);

    abstract_cb->setChecked(false);

    if (cl->get_is_abstract()) {
        abstract_cb->setChecked(TRUE);

        if (currentNode->have_abstract_operation())
            abstract_cb->setEnabled(FALSE);
    }

    abstract_cb->setDisabled(!isWritable);


    active_cb->setChecked(false);

    if (cl->get_is_active())
        active_cb->setChecked(TRUE);

    active_cb->setDisabled(!isWritable);


    // setting up radiobutton group for accessibility

    bgvUml = uml_visibility.init(htabUml, cl->get_uml_visibility(), TRUE);

    if (!isWritable)
        bgvUml->setEnabled(FALSE);

    htabUml->setStretchFactor(bgvUml, 1000);

    BrowserNodeList inh;
    // setting base type combobox
    edbasetype->clear();

    if (cl->browser_node->children(inh, UmlGeneralisation, UmlRealize), inh.count() != 0) {
        // first inheritance is taken in all cases
        edbasetype->insertItem(((RelationData *) inh.at(0)->get_data())->get_end_class()->full_name(TRUE));
        edbasetype->setEnabled(FALSE);
    }
    else {
        QString bt = cl->get_base_type().get_full_type();

        if (bt.isEmpty() &&		// no base_type, try with a dependency
            (cl->browser_node->children(inh, UmlDependency), inh.count() != 0))
            bt = ((RelationData *) inh.at(0)->get_data())->get_end_class()->full_name(TRUE);

        edbasetype->insertItem(bt);

        if (isWritable) {
            edbasetype->insertStringList(GenerationSettings::basic_types());
            edbasetype->insertStringList(node_names);
            edbasetype->setAutoCompletion(completion());
        }
    }

    edbasetype->setCurrentItem(0);
    edbasetype->setSizePolicy(sp);
    artifact->clear();
    artifact->show();
    artifacts.clear();

    if (!currentNode->nestedp()) {
        BrowserNode * bc = currentNode->get_associated_artifact();

        if (!isWritable) {
            if ((bc != 0) && !bc->deletedp()) {
                artifact->insertItem(bc->full_name(TRUE));
            }
            else
                artifact->hide();
        }
        else {
            BrowserArtifact::instances(artifacts, TR("source"));

            if (!artifacts.isEmpty()) {
                QStringList artifact_names;
                artifacts.full_names(artifact_names);
                artifact->insertItem("");
                artifact->insertStringList(artifact_names);

                if (bc != 0) {
                    if (bc->deletedp()) {
                        QString fn = bc->full_name(TRUE);

                        artifact->insertItem(fn);
                        artifacts.append(bc);
                        artifact->setCurrentItem(artifacts.count());
                    }
                    else
                        artifact->setCurrentItem(artifacts.indexOf(bc) + 1);
                }
                else
                    artifact->setCurrentItem(0);
            }
            else
                artifact->hide();
        }
    }
    else
        artifact->hide();


    if (isWritable) {
        connect(pbEditorForDescription, SIGNAL(clicked()),
                this, SLOT(edit_description()));
        connect(pbDefaultForDescription, SIGNAL(clicked()),
                this, SLOT(default_description()));
        connect(pbEditorForConstrant, SIGNAL(clicked()),
                this, SLOT(edit_constraint()));
        pbEditorForDescription->show();
        pbDefaultForDescription->show();
        pbEditorForConstrant->show();
    }
    else {
        pbEditorForDescription->hide();
        pbDefaultForDescription->hide();
        pbEditorForConstrant->hide();
    }

    comment->setText(currentNode->get_comment());
    constraint->setText(cl->constraint);

    // parameterized tab
    formals_table->Reinitiliaze(cl, node_names, isWritable);
    formals_table->update(cl, inh);

    // instantiate tab
    //todo
    actuals_table->Reinitiliaze(cl, node_names, isWritable);
    actuals_table->update(cl, inh);

    if (cl->get_n_actualparams() != 0)
        //setTabEnabled(instantiate_vtab, true);
        ShowTab("Instantiate");
    else
        HideTab("Instantiate");

    //setTabEnabled(instantiate_vtab, false);

    // C++
    if (cl->cpp_is_external())
        cpp_external_cb->setChecked(TRUE);
    else
        cpp_external_cb->setChecked(false);

    if (!isWritable) {
        cpp_external_cb->setDisabled(TRUE);
        disconnect(cpp_external_cb, SIGNAL(toggled(bool)), this,
                   SLOT(cpp_default_decl()));
    }
    else {
        cpp_external_cb->setDisabled(false);
        connect(cpp_external_cb, SIGNAL(toggled(bool)), this,
                SLOT(cpp_default_decl()));
    }

    if (currentNode->nestedp()) {
        bgvCpp = cpp_visibility.init(htabcpp, cl->get_cpp_visibility(),
                                     FALSE, 0, TR("follow uml"));

        if (!isWritable)
            bgvCpp->setEnabled(FALSE);
        else
            bgvCpp->setEnabled(true);
    }

    edcppdecl->setText(cl->cpp_decl);
    font = edcppdecl->font();

    if (!hasCodec())
        font.setFamily("Courier");

    font.setFixedPitch(TRUE);
    edcppdecl->setFont(font);

    if (!isWritable) {
        edcppdecl->setReadOnly(TRUE);
        disconnect(edcppdecl, SIGNAL(textChanged()), this, SLOT(cpp_update_decl()));
    }
    else {
        edcppdecl->setReadOnly(FALSE);
        connect(edcppdecl, SIGNAL(textChanged()), this, SLOT(cpp_update_decl()));
    }

    showcppdecl->setReadOnly(TRUE);
    showcppdecl->setFont(font);

    if (!isWritable) {
        same_width(lbl1cpp, lbl2cpp, lbl3cpp);
        lbl4cpp->hide();
        htabcpp->hide();
        disconnect(pbCppDefaultDeclaration, SIGNAL(clicked()),
                   this, SLOT(cpp_default_decl()));
        disconnect(pbNotGeneratedInCPP, SIGNAL(clicked()),
                   this, SLOT(cpp_unmapped_decl()));

    }
    else {
        connect(pbCppDefaultDeclaration, SIGNAL(clicked()),
                this, SLOT(cpp_default_decl()));
        connect(pbNotGeneratedInCPP, SIGNAL(clicked()),
                this, SLOT(cpp_unmapped_decl()));
        lbl4cpp->show();
        htabcpp->show();
        same_width(lbl1cpp, lbl2cpp, lbl3cpp, lbl4cpp);
    }



    if (!GenerationSettings::cpp_get_default_defs())
        HideTab("Cpp");
    else
        ShowTab("Cpp");

    // Java


    if (cl->java_is_final())
        java_final_cb->setChecked(TRUE);
    else
        java_final_cb->setChecked(false);

    if (cl->java_is_external())
        java_external_cb->setChecked(TRUE);
    else
        java_external_cb->setChecked(false);

    if (!isWritable) {
        java_final_cb->setDisabled(TRUE);
        java_external_cb->setDisabled(TRUE);
        disconnect(java_final_cb, SIGNAL(toggled(bool)), this,
                   SLOT(java_update_decl()));
        disconnect(java_external_cb, SIGNAL(toggled(bool)), this,
                   SLOT(java_default_decl()));
    }
    else {
        java_final_cb->setDisabled(false);
        java_external_cb->setDisabled(false);
        connect(java_final_cb, SIGNAL(toggled(bool)), this,
                SLOT(java_update_decl()));
        connect(java_external_cb, SIGNAL(toggled(bool)), this,
                SLOT(java_default_decl()));
    }


    edjavadecl->setText(cl->java_decl);
    edjavadecl->setFont(font);

    if (!isWritable) {
        edjavadecl->setReadOnly(TRUE);
        disconnect(edjavadecl, SIGNAL(textChanged()), this, SLOT(java_update_decl()));
    }
    else {
        edjavadecl->setReadOnly(false);
        connect(edjavadecl, SIGNAL(textChanged()), this, SLOT(java_update_decl()));
    }

    showjavadecl->setReadOnly(TRUE);
    showjavadecl->setFont(font);


    if (isWritable) {
        connect(pbJavaDefaultDefinition, SIGNAL(clicked()),
                this, SLOT(java_default_decl()));
        connect(pbNotGeneratedInJava, SIGNAL(clicked()),
                this, SLOT(java_unmapped_decl()));
    }
    else {
        disconnect(pbJavaDefaultDefinition, SIGNAL(clicked()),
                   this, SLOT(java_default_decl()));
        disconnect(pbNotGeneratedInJava, SIGNAL(clicked()),
                   this, SLOT(java_unmapped_decl()));
    }

    same_width(lbl1java, lbl2java, lbl3java, lbl4java);
    javaannotation = (const char *) cl->java_annotation;
    pbJavaAnnotation->setText(!isWritable ? TR("Show annotation") : TR("Edit annotation"));

    if (!GenerationSettings::java_get_default_defs())
        HideTab("Java");
    else
        ShowTab("Java");

    // Php


    if (cl->php_is_final())
        php_final_cb->setChecked(TRUE);
    else
        php_final_cb->setChecked(false);

    if (cl->php_is_external())
        php_external_cb->setChecked(TRUE);
    else
        php_external_cb->setChecked(false);

    if (!isWritable) {
        php_final_cb->setDisabled(TRUE);
        php_external_cb->setDisabled(TRUE);
        disconnect(php_final_cb, SIGNAL(toggled(bool)), this,
                   SLOT(php_update_decl()));
        disconnect(php_external_cb, SIGNAL(toggled(bool)), this,
                   SLOT(php_default_decl()));
    }
    else {
        php_final_cb->setDisabled(false);
        php_external_cb->setDisabled(false);
        connect(php_final_cb, SIGNAL(toggled(bool)), this,
                SLOT(php_update_decl()));
        connect(php_external_cb, SIGNAL(toggled(bool)), this,
                SLOT(php_default_decl()));
    }


    edphpdecl->setText(cl->php_decl);

    if (!isWritable) {
        disconnect(edphpdecl, SIGNAL(textChanged()), this, SLOT(php_update_decl()));
        edphpdecl->setReadOnly(TRUE);
    }
    else {
        connect(edphpdecl, SIGNAL(textChanged()), this, SLOT(php_update_decl()));
        edphpdecl->setReadOnly(false);
    }

    if (isWritable) {
        connect(pbPhpDefaultDefinition, SIGNAL(clicked()),
                this, SLOT(php_default_decl()));
        connect(pbNotGeneratedInPhp, SIGNAL(clicked()),
                this, SLOT(php_unmapped_decl()));

    }
    else {
        disconnect(pbPhpDefaultDefinition, SIGNAL(clicked()),
                   this, SLOT(php_default_decl()));
        disconnect(pbNotGeneratedInPhp, SIGNAL(clicked()),
                   this, SLOT(php_unmapped_decl()));

    }

    same_width(lbl1php, lbl2php, lbl3php, lbl4php);

    if (!GenerationSettings::php_get_default_defs())
        HideTab("Php");
    else
        ShowTab("Php");


    // Python

    if (cl->python_is_2_2())
        python_2_2_cb->setChecked(TRUE);
    else
        python_2_2_cb->setChecked(false);

    if (cl->python_is_external())
        python_external_cb->setChecked(TRUE);
    else
        python_external_cb->setChecked(false);

    if (!isWritable) {
        disconnect(python_2_2_cb, SIGNAL(toggled(bool)), this,
                   SLOT(python_update_decl()));
        disconnect(python_external_cb, SIGNAL(toggled(bool)), this,
                   SLOT(python_default_decl()));
        python_2_2_cb->setDisabled(TRUE);
        python_external_cb->setDisabled(TRUE);
    }
    else {
        connect(python_2_2_cb, SIGNAL(toggled(bool)), this,
                SLOT(python_update_decl()));
        connect(python_external_cb, SIGNAL(toggled(bool)), this,
                SLOT(python_default_decl()));

        python_2_2_cb->setDisabled(false);
        python_external_cb->setDisabled(false);
    }


    edpythondecl->setText(cl->python_decl);
    edpythondecl->setFont(font);

    if (!isWritable) {
        disconnect(edpythondecl, SIGNAL(textChanged()), this, SLOT(python_update_decl()));
        edpythondecl->setReadOnly(TRUE);
    }
    else {
        connect(edpythondecl, SIGNAL(textChanged()), this, SLOT(python_update_decl()));
        edpythondecl->setReadOnly(false);
    }


    if (isWritable) {
        connect(pbPythonDefaultDefinition, SIGNAL(clicked()),
                this, SLOT(python_default_decl()));
        connect(pbNotGeneratedInPython, SIGNAL(clicked()),
                this, SLOT(python_unmapped_decl()));
    }
    else {
        disconnect(pbPythonDefaultDefinition, SIGNAL(clicked()),
                   this, SLOT(python_default_decl()));
        disconnect(pbNotGeneratedInPython, SIGNAL(clicked()),
                   this, SLOT(python_unmapped_decl()));
    }

    same_width(lbl1python, lbl2python, lbl3python, lbl4python);

    if (!GenerationSettings::python_get_default_defs())
        HideTab("Python");
    else
        ShowTab("Python");

    // IDL
    edswitch_type->clear();

    if (isWritable) {
        edswitch_type->setAutoCompletion(completion());
        edswitch_type->insertItem(cl->get_switch_type().get_full_type());
        edswitch_type->insertStringList(GenerationSettings::basic_types());

        foreach (BrowserNode *item, nodes) {
            QString st =
                idl_stereotype(((ClassData *)(item->get_data()))->get_stereotype());

            if (st == "enum")
                edswitch_type->insertItem(item->full_name(TRUE));
        }
    }

    edswitch_type->setCurrentItem(0);

    if (cl->idl_is_external())
        idl_external_cb->setChecked(TRUE);
    else
        idl_external_cb->setChecked(false);

    if (cl->idl_is_local())
        idl_local_cb->setChecked(TRUE);
    else
        idl_local_cb->setChecked(false);

    if (cl->idl_is_custom())
        idl_custom_cb->setChecked(TRUE);
    else
        idl_custom_cb->setChecked(false);

    if (!isWritable) {
        disconnect(idl_local_cb, SIGNAL(toggled(bool)), this,
                   SLOT(idl_update_decl()));
        disconnect(idl_custom_cb, SIGNAL(toggled(bool)), this,
                   SLOT(idl_update_decl()));
        disconnect(idl_external_cb, SIGNAL(toggled(bool)), this,
                   SLOT(idl_default_decl()));

        idl_local_cb->setDisabled(TRUE);
        idl_external_cb->setDisabled(TRUE);
    }
    else {
        connect(idl_local_cb, SIGNAL(toggled(bool)), this,
                SLOT(idl_update_decl()));
        connect(idl_custom_cb, SIGNAL(toggled(bool)), this,
                SLOT(idl_update_decl()));
        connect(idl_external_cb, SIGNAL(toggled(bool)), this,
                SLOT(idl_default_decl()));

        idl_local_cb->setDisabled(false);
        idl_external_cb->setDisabled(false);
    }


    edidldecl->setText(cl->idl_decl);

    if (!isWritable) {
        disconnect(edidldecl, SIGNAL(textChanged()), this, SLOT(idl_update_decl()));
        edidldecl->setReadOnly(TRUE);

    }
    else {
        connect(edidldecl, SIGNAL(textChanged()), this, SLOT(idl_update_decl()));
        edidldecl->setReadOnly(false);
    }


    if (!isWritable) {
        same_width(lbl1idl, lbl2idl, lbl3idl);
        disconnect(pbIdlDefaultDeclaration, SIGNAL(clicked()),
                   this, SLOT(idl_default_decl()));
        disconnect(pbINotGeneratedInIdl, SIGNAL(clicked()),
                   this, SLOT(idl_unmapped_decl()));
        htabidl->hide();
    }

    else {
        connect(pbIdlDefaultDeclaration, SIGNAL(clicked()),
                this, SLOT(idl_default_decl()));
        connect(pbINotGeneratedInIdl, SIGNAL(clicked()),
                this, SLOT(idl_unmapped_decl()));

        same_width(lbl1idl, lbl2idl, lbl3idl, lbl4idl);
        htabidl->show();
    }



    if (!GenerationSettings::idl_get_default_defs())
        HideTab("Idl");
    else
        ShowTab("Idl");

    // profiled stereotype

    stereo_init_cb->clear();
    bool grannyIsUmlPackage = grandParent->get_type() == UmlPackage;
    bool grannyIsProfiled = !strcmp(grandParent->get_data()->get_stereotype(), "profile");

    if (grannyIsUmlPackage && grannyIsProfiled)
    {

        QString s;
        s = currentNode->get_value("stereotypeSet");
        stereo_init_cb->insertItem(s);
        QStringList tools = Tool::all_display();

        if (isWritable) {
            if (!s.isEmpty())
                stereo_init_cb->insertItem("");

            stereo_init_cb->insertStringList(tools);
        }
        else {
            stereo_init_cb->clear();
        }

        stereo_init_cb->setCurrentItem(0);

        edinitparam->setReadOnly(!isWritable);

        s = currentNode->get_value("stereotypeCheck");

        stereo_check_cb->clear();
        stereo_check_cb->insertItem(s);

        if (isWritable) {
            if (! s.isEmpty())
                stereo_check_cb->insertItem("");

            stereo_check_cb->insertStringList(tools);
        }

        stereo_check_cb->setCurrentItem(0);

        edcheckparam->setReadOnly(!isWritable);

        QString ip = currentNode->get_value("stereotypeIconPath");

        ediconpath->setText(ip);

        if (!isWritable) {
            disconnect(pbProfiledSteretypeBrowse, SIGNAL(clicked()), this, SLOT(icon_browse()));
            disconnect(iconpathrootbutton, SIGNAL(clicked()), this, SLOT(icon_root_relative()));
            disconnect(iconpathprjbutton, SIGNAL(clicked()), this, SLOT(icon_prj_relative()));

            ediconpath->setReadOnly(TRUE);
            iconpathrootbutton = iconpathprjbutton = 0;

            RelativeRoot = TR("");
            RelativePrj = TR("");
            Absolute = TR("");
            lblProfiledEmpty->hide();
            lblProfiledEmpty2->hide();
            lblProfiledEmpty3->hide();
            vtabProfiled->hide();
            pbProfiledSteretypeBrowse->hide();
        }
        else {
            connect(pbProfiledSteretypeBrowse, SIGNAL(clicked()), this, SLOT(icon_browse()));
            connect(iconpathrootbutton, SIGNAL(clicked()), this, SLOT(icon_root_relative()));
            connect(iconpathprjbutton, SIGNAL(clicked()), this, SLOT(icon_prj_relative()));

            RelativeRoot = TR("Set it relative to image root");
            RelativePrj = TR("Set it relative to project");
            Absolute = TR("Set it absolute");

            lblProfiledEmpty->show();
            pbProfiledSteretypeBrowse->show();

            lblProfiledEmpty2->show();
            vtabProfiled->show();
            iconpathrootbutton->setText((ip.isEmpty() || QDir::isRelativePath(ip))
                                        ? Absolute : RelativeRoot);
            iconpathprjbutton->setText((ip.isEmpty() || QDir::isRelativePath(ip))
                                       ? Absolute : RelativePrj);

            iconpathrootbutton->setEnabled(!UmlWindow::images_root_dir().isEmpty());
            lblProfiledEmpty3->show();

        }


        if (applicableon_table)
            delete applicableon_table;

        applicableon_table = new ApplicableOnTable(stereotypeGrid, currentNode->get_value("stereotypeApplyOn"), !isWritable);

        ShowTab("Stereotype");
    }
    else
        HideTab("Stereotype");



    // USER : list key - value
    ShowTab("Properties");
    kvtable->updateThisFromNode(currentNode, !isWritable);
    kvtable->remove("stereotypeCheck");
    kvtable->remove("stereotypeSetParameters");
    kvtable->remove("stereotypeCheckParameters");
    kvtable->remove("stereotypeApplyOn");
    kvtable->remove("stereotypeIconPath");
    //

    QString stereotype = fromUnicode(edstereotype->currentText());

    current_cpp_stereotype = cpp_stereotype(stereotype);
    current_java_stereotype = java_stereotype(stereotype);
    current_php_stereotype = php_stereotype(stereotype);
    current_python_stereotype = python_stereotype(stereotype);
    current_idl_stereotype = idl_stereotype(stereotype);

    edStereotypeActivated(stereotype);

    connect(this, SIGNAL(currentChanged(QWidget *)),
            this, SLOT(update_all_tabs(QWidget *)));

    update_all_tabs(0);
}

void ClassDialog::NullPointers()
{
    instantiateNotice = 0;
    lblProfiledEmpty = 0;
    pbProfiledSteretypeBrowse = 0;
    lblProfiledEmpty2 = 0;
    lblProfiledEmpty3 = 0;
    vtabProfiled = 0;
    bgvUml = 0;
    bgvCpp = 0;
    bgvJava = 0;
    bgvPython = 0;
    bgvPhp = 0;
    bgvIDL = 0;
    htabcpp = 0;
    htabidl = 0;
    lbl1cpp = 0;
    lbl2cpp = 0;
    lbl3cpp = 0;
    lbl4cpp = 0;


    lbl1php = 0;
    lbl2php = 0;
    lbl3php = 0;
    lbl4php = 0;

    lbl1python = 0;
    lbl2python = 0;
    lbl3python = 0;
    lbl4python = 0;

    lbl1idl = 0;
    lbl2idl = 0;
    lbl3idl = 0;
    lbl4idl = 0;

    lbl1java = 0;
    lbl2java = 0;
    lbl3java = 0;
    lbl4java = 0;
    stereotypeGrid = 0;
    htabUml = 0;


    currentNode = 0;
    grandParent = 0;

    keyValueTab = 0;

    bgvUml = 0;
    bgvCpp = 0;
    bgvJava = 0;
    bgvPython = 0;
    bgvPhp = 0;
    bgvIDL = 0;
    applicableon_table = 0;
}

bool ClassDialog::SaveData()
{
    if (!check_edits(edits) || !kvtable->check_unique())
        return true;

    BrowserClass * bn = (BrowserClass *) cl->get_browser_node();
    QString oldname = cl->browser_node->get_name();
    bool was_st = !strcmp(cl->get_stereotype(), "stereotype");
    QString st = fromUnicode(edstereotype->currentText().stripWhiteSpace());
    QString err;
    QString s;

    s = edname->text().stripWhiteSpace();

    if (s != oldname) {
        if (((BrowserNode *) bn->parent())->wrong_child_name(s, UmlClass,
                bn->allow_spaces(),
                bn->allow_empty())) {
            msg_critical(TR("Error"), s + TR("\n\nillegal name or already used"));
            return true;
        }

        if ((st == "stereotype") &&
            (!(err = ProfiledStereotypes::canAddStereotype(bn, s)).isEmpty())) {
            msg_critical(TR("Error"), s + " " + err);
            return true;
        }

        bn->set_name(s);
    }
    else if (st == "stereotype") {
        if (!was_st &&
            (!(err = ProfiledStereotypes::canAddStereotype(bn, s)).isEmpty())) {
            msg_critical(TR("Error"), oldname + " " + err);
            return true;
        }
        else if (stereotypetab != 0) {
            WrapperStr path = fromUnicode(ediconpath->text().simplifyWhiteSpace());

            if (!path.isEmpty() && (get_pixmap((const char *) path) == 0)) {
                msg_critical(TR("Error"),
                             ((const char *) path) +
                             TR("\ndoesn't exist or is not a know image format"));
                return true;
            }
        }
    }

    bool newst = cl->set_stereotype(st);

    if (st == "typedef")
        cl->set_base_type(the_type(edbasetype->currentText().stripWhiteSpace(),
                                   node_names, nodes));

    cl->is_abstract = abstract_cb->isChecked();
    cl->is_active = active_cb->isChecked();

    cl->uml_visibility = uml_visibility.value();

    if (artifact != 0) {
        int index = artifact->currentItem();

        bn->set_associated_artifact((index == 0)
                                    ? 0 // "" : no artifact
                                    : ((BrowserArtifact *) artifacts.at(index - 1)));
    }

    formals_table->update(cl, nodes);

    if (cl->get_n_actualparams() != 0)
        actuals_table->update(cl, nodes);

    bn->set_comment(comment->text());
    UmlWindow::update_comment_if_needed(bn);

    cl->constraint = constraint->stripWhiteSpaceText();

    cl->cpp_external = cpp_external_cb->isChecked();
    cl->cpp_decl = edcppdecl->text();

    if (bn->nestedp())
        cl->cpp_visibility = cpp_visibility.value();

    cl->java_decl = edjavadecl->text();
    cl->java_final = java_final_cb->isChecked();
    cl->java_external = java_external_cb->isChecked();
    cl->java_annotation = javaannotation;

    cl->php_decl = edphpdecl->text();
    cl->php_final = php_final_cb->isChecked();
    cl->php_external = php_external_cb->isChecked();

    cl->python_decl = edpythondecl->text();
    cl->python_2_2 = python_2_2_cb->isChecked();
    cl->python_external = python_external_cb->isChecked();

    cl->idl_decl = edidldecl->text();
    cl->idl_local = idl_local_cb->isChecked();
    cl->idl_custom = idl_custom_cb->isChecked();
    cl->idl_external = idl_external_cb->isChecked();

    if (switch_bg->isEnabled()) {
        AType swt;

        s = edswitch_type->currentText().stripWhiteSpace();

        if (! s.isEmpty()) {
            int index = node_names.findIndex(s);

            if (index >= 0)
                swt.type = (BrowserClass *) nodes.at(index);
            else
                swt.explicit_type = s;
        }

        cl->set_switch_type(swt);
    }

    kvtable->updateNodeFromThis(bn);

    if (stereotypetab != 0) {
        if (st == "stereotype") {
            unsigned n = bn->get_n_keys();
            WrapperStr oldiconpath = bn->get_value("stereotypeIconPath");
            WrapperStr newiconpath = fromUnicode(ediconpath->text().simplifyWhiteSpace());

            bn->set_n_keys(n + 6);
            bn->set_key(n, "stereotypeSet");
            bn->set_value(n, stereo_init_cb->currentText());
            bn->set_key(++n, "stereotypeCheck");
            bn->set_value(n, stereo_check_cb->currentText());
            bn->set_key(++n, "stereotypeSetParameters");
            bn->set_value(n, fromUnicode(edinitparam->text().simplifyWhiteSpace()));
            bn->set_key(++n, "stereotypeCheckParameters");
            bn->set_value(n, fromUnicode(edcheckparam->text().simplifyWhiteSpace()));
            bn->set_key(++n, "stereotypeApplyOn");
            bn->set_value(n, applicableon_table->targets());
            bn->set_key(++n, "stereotypeIconPath");
            bn->set_value(n, newiconpath);

            if (newst)
                ProfiledStereotypes::added(bn);
            else
                ProfiledStereotypes::changed(bn, oldname, oldiconpath != newiconpath);
        }
        else if (was_st)
            ProfiledStereotypes::deleted(bn);
    }

    ProfiledStereotypes::modified(bn, newst);

    bn->modified();
    bn->package_modified();
    cl->modified();
    return true;
}
