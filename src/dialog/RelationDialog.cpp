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





#include <q3vbox.h>
#include <qlabel.h>
#include <q3combobox.h>
#include <q3buttongroup.h>
#include <qcheckbox.h>
#include <qradiobutton.h>
#include <qpushbutton.h>
#include <qsplitter.h>
#include <q3popupmenu.h>
#include <qcursor.h>
//Added by qt3to4:
#include <QPixmap>

#include "RelationDialog.h"
#include "RelationData.h"
#include "BrowserRelation.h"
#include "BrowserOperation.h"
#include "ClassData.h"
#include "BrowserView.h"
#include "BrowserClass.h"
#include "KeyValueTable.h"
#include "UmlPixmap.h"
#include "DialogUtil.h"
#include "ClassDialog.h"
#include "GenerationSettings.h"
#include "strutil.h"
#include "UmlDesktop.h"
#include "BodyDialog.h"
#include "AnnotationDialog.h"
#include "ProfiledStereotypes.h"
#include "translate.h"

QSize RelationDialog::previous_size;

static const struct {
    UmlCode type;
    const char * lbl;
    QPixmap ** pix;
} RelTypes[] = {
    {UmlGeneralisation, "generalisation", &generalisationButton },
    {UmlDependency, "dependency", &dependencyButton},
    {UmlAssociation, "association", &associationButton},
    {UmlDirectionalAssociation, "unidirectional association", &directionalAssociationExtensionButton},
    {UmlAggregation, "aggregation", &aggregationButton},
    {UmlAggregationByValue, "composition", &aggregationByValueButton},
    {UmlDirectionalAggregation, "directional aggregation", &directionalAggregationButton},
    {UmlDirectionalAggregationByValue, "directional composition", &directionalAggregationByValueButton},
    {UmlRealize, "realization", &realizeButton}
};

static const struct {
    UmlCode type;
    const char * lbl;
    QPixmap ** pix;
} UniRelTypes[] = {
    {UmlGeneralisation, "generalisation", &generalisationButton },
    {UmlDependency, "dependency", &dependencyButton},
    {UmlDirectionalAssociation, "unidirectional association", &directionalAssociationExtensionButton},
    {UmlDirectionalAggregation, "directional aggregation", &directionalAggregationButton},
    {UmlDirectionalAggregationByValue, "directional composition", &directionalAggregationByValueButton},
    {UmlRealize, "realization", &realizeButton}
};

static const struct {
    UmlCode type;
    const char * lbl;
    QPixmap ** pix;
} BiRelTypes[] = {
    {UmlAssociation, "association", &associationButton},
    {UmlAggregation, "aggregation", &aggregationButton},
    {UmlAggregationByValue, "composition", &aggregationByValueButton},
};

RelationDialog::RelationDialog(RelationData * r)
    : Q3TabDialog(0, 0, FALSE, Qt::WDestructiveClose), rel(r)
{
    setCaption(TR("Relation dialog"));

    visit = a.ro = !r->start->is_writable();
    b.ro = (r->end) ? !r->end->is_writable()
           : !r->end_removed_from->is_writable();

    if (!a.ro || !b.ro) {
        setOkButton(TR("OK"));
        setCancelButton(TR("Cancel"));
    }
    else {
        setOkButton(QString());
        setCancelButton(TR("Close"));
    }

    r->start->edit_start();

    onstereotype =
        !strcmp(r->get_start_class()->get_data()->get_stereotype(), "stereotype") &&
        !strcmp(r->get_end_class()->get_data()->get_stereotype(), "stereotype");

    UmlCode type = rel->get_type();

    current_type = type;

    //
    // general tab
    //

    Q3VBox * vtab = new Q3VBox(this);
    Q3HBox * htab;
    Q3GroupBox  * bg;

    umltab = vtab;
    vtab->setMargin(3);

    htab = new Q3HBox(vtab);
    htab->setMargin(3);
    QLabel * lbl1 = new QLabel(TR("name : "), htab);
    edname = new LineEdit(rel->get_name(), htab);
    edname->setReadOnly(visit);

    htab = new Q3HBox(vtab);
    htab->setMargin(3);
    QLabel * lbl2 = new QLabel(TR("type : "), htab);

    edtype = new Q3ComboBox(FALSE, htab);

    int index;

    if (visit) {
        // relation kind cannot be changed
        for (index = 0; index != sizeof(RelTypes) / sizeof(*RelTypes); index += 1) {
            if (RelTypes[index].type == type) {
                edtype->insertItem(**RelTypes[index].pix, RelTypes[index].lbl);
                edtype->setCurrentItem(0);
                break;
            }
        }
    }
    else {
        if (b.ro) {
            // bi/uni dir cannot be changed
            if (r->end == 0) {
                // unidir
                for (index = 0; index != sizeof(UniRelTypes) / sizeof(*UniRelTypes); index += 1) {
                    edtype->insertItem(**UniRelTypes[index].pix, UniRelTypes[index].lbl);

                    if (UniRelTypes[index].type == type)
                        edtype->setCurrentItem(index);
                }
            }
            else {
                // bidir
                for (index = 0; index != sizeof(BiRelTypes) / sizeof(*BiRelTypes); index += 1) {
                    edtype->insertItem(**BiRelTypes[index].pix, BiRelTypes[index].lbl);

                    if (BiRelTypes[index].type == type)
                        edtype->setCurrentItem(index);
                }
            }
        }
        else {
            for (index = 0; index != sizeof(RelTypes) / sizeof(*RelTypes); index += 1) {
                edtype->insertItem(**RelTypes[index].pix, RelTypes[index].lbl);

                if (RelTypes[index].type == type)
                    edtype->setCurrentItem(index);
            }
        }

        connect(edtype, SIGNAL(activated(int)), this, SLOT(edTypeActivated(int)));
    }

    new QLabel(TR("    stereotype : "), htab);
    edstereotype = new Q3ComboBox(!visit, htab);
    edstereotype->insertItem(toUnicode(rel->get_stereotype()));

    if (!visit) {
        edstereotype->insertStringList(rel->get_start_class()
                                       ->default_stereotypes(type,
                                               rel->get_end_class()));
        edstereotype->insertStringList(ProfiledStereotypes::defaults(UmlRelations));
        edstereotype->setAutoCompletion(completion());
    }

    edstereotype->setCurrentItem(0);
    QSizePolicy sp = edstereotype->sizePolicy();
    sp.setHorData(QSizePolicy::Expanding);
    edstereotype->setSizePolicy(sp);

    htab = new Q3HBox(vtab);
    htab->setMargin(3);

    SmallPushButton * button_assoc =
        new SmallPushButton(TR("association :"), htab);

    connect(button_assoc, SIGNAL(clicked()), this, SLOT(menu_assoc()));

    edassociation = new Q3ComboBox(!visit, htab);
    edassociation->insertItem(rel->association.get_full_type());
    BrowserClass::instances(nodes);
    nodes.full_names(list);

    if (!visit) {
        edassociation->insertStringList(GenerationSettings::basic_types());
        offset = edassociation->count();
        edassociation->insertStringList(list);
        edassociation->setAutoCompletion(completion());
        view = rel->get_start_class()->container(UmlClass);
    }

    edassociation->setCurrentItem(0);
    edassociation->setSizePolicy(sp);

    same_width(lbl1, lbl2, button_assoc);

    QSplitter * split = new QSplitter(Qt::Vertical, vtab);

    split->setOpaqueResize(TRUE);

    QString ina = TR("in ") + rel->get_start_class()->full_name(TRUE);
    QString inb = TR("in ") + rel->get_end_class()->full_name(TRUE);

    // role A
    bg = new Q3GroupBox(2, Qt::Horizontal, ina, split);
    init_uml_role(a, rel->a, bg, rel->get_start_class());
    aOldText = rel->a.role.operator QString();
    bOldText = rel->b.role.operator QString();

    // role B
    bg = new Q3GroupBox(2, Qt::Horizontal, inb, split);
    init_uml_role(b, rel->b, bg, rel->get_end_class());

    addTab(vtab, "Uml");

    //
    // C++
    //

    vtab = new Q3VBox(this);
    cppTab = vtab;
    vtab->setMargin(5);

    // A
    bg = new Q3GroupBox(2, Qt::Horizontal, ina, vtab);
    new QLabel(bg);
    a.cpp_virtual_inheritance_cb = new QCheckBox("virtual", bg);

    if (rel->a.cpp_virtual_inheritance)
        a.cpp_virtual_inheritance_cb->setChecked(TRUE);

    if (visit)
        a.cpp_virtual_inheritance_cb->setEnabled(FALSE);
    else
        connect(a.cpp_virtual_inheritance_cb, SIGNAL(toggled(bool)), this, SLOT(cpp_update_a()));

    init_cpp_role(a, rel->a, bg, SLOT(cpp_update_a()),
                  SLOT(cpp_default_a()), SLOT(cpp_unmapped_a()),
                  SLOT(cpp_include_in_header()));

    // B
    htab = new Q3HBox(vtab);	// to have a vertical margin
    htab->setMargin(5);

    cpp_b = new Q3GroupBox(2, Qt::Horizontal, inb, vtab);
    b.cpp_virtual_inheritance_cb = 0;
    init_cpp_role(b, rel->b, cpp_b, SLOT(cpp_update_b()),
                  SLOT(cpp_default_b()), SLOT(cpp_unmapped_b()), 0);

    addTab(vtab, "C++");

    if (onstereotype || !GenerationSettings::cpp_get_default_defs())
        removePage(vtab);

    //
    // Java
    //

    vtab = new Q3VBox(this);
    javatab = vtab;
    vtab->setMargin(5);

    // A
    bg = new Q3GroupBox(2, Qt::Horizontal, ina, vtab);
    init_java_role(a, rel->a, bg, SLOT(java_update_a()), SLOT(java_default_a()),
                   SLOT(java_unmapped_a()), SLOT(java_edit_annotation_a()));

    // B
    htab = new Q3HBox(vtab);	// to have a vertical margin
    htab->setMargin(5);

    java_b = new Q3GroupBox(2, Qt::Horizontal, inb, vtab);
    init_java_role(b, rel->b, java_b, SLOT(java_update_b()), SLOT(java_default_b()),
                   SLOT(java_unmapped_b()), SLOT(java_edit_annotation_b()));

    addTab(vtab, "Java");

    if (onstereotype || !GenerationSettings::java_get_default_defs())
        removePage(vtab);

    //
    // Php
    //

    vtab = new Q3VBox(this);
    phptab = vtab;
    vtab->setMargin(5);

    // A
    bg = new Q3GroupBox(2, Qt::Horizontal, ina, vtab);
    init_php_role(a, rel->a, bg, SLOT(php_update_a()),
                  SLOT(php_default_a()), SLOT(php_unmapped_a()));

    // B
    htab = new Q3HBox(vtab);	// to have a vertical margin
    htab->setMargin(5);

    php_b = new Q3GroupBox(2, Qt::Horizontal, inb, vtab);
    init_php_role(b, rel->b, php_b, SLOT(php_update_b()),
                  SLOT(php_default_b()), SLOT(php_unmapped_b()));

    addTab(vtab, "Php");

    if (onstereotype || !GenerationSettings::php_get_default_defs())
        removePage(vtab);

    //
    // Python
    //

    vtab = new Q3VBox(this);
    pythontab = vtab;
    vtab->setMargin(5);

    // A
    bg = new Q3GroupBox(2, Qt::Horizontal, ina, vtab);
    init_python_role(a, rel->a, rel->get_start_class(), bg, SLOT(python_update_a()),
                     SLOT(python_default_a()), SLOT(python_unmapped_a()));

    // B
    htab = new Q3HBox(vtab);	// to have a vertical margin
    htab->setMargin(5);

    python_b = new Q3GroupBox(2, Qt::Horizontal, inb, vtab);
    init_python_role(b, rel->b, rel->get_end_class(),
                     python_b, SLOT(python_update_b()),
                     SLOT(python_default_b()), SLOT(python_unmapped_b()));

    addTab(vtab, "Python");

    if (onstereotype || !GenerationSettings::python_get_default_defs())
        removePage(vtab);

    //
    // IDL
    //
    vtab = new Q3VBox(this);
    idltab = vtab;
    vtab->setMargin(5);

    // A
    bg = new Q3GroupBox(2, Qt::Horizontal, ina, vtab);

    ClassData * start_data = (ClassData *) rel->get_start_class()->get_data();
    ClassData * end_data = (ClassData *) rel->get_end_class()->get_data();

    if ((ClassDialog::idl_stereotype(start_data->get_stereotype())
         == "valuetype") &&
        (ClassDialog::idl_stereotype(end_data->get_stereotype())
         == "valuetype")) {
        new QLabel(bg);
        a.idl_truncatable_inheritance_cb = new QCheckBox("truncatable", bg);

        if (rel->a.idl_truncatable_inheritance)
            a.idl_truncatable_inheritance_cb->setChecked(TRUE);

        if (visit)
            a.idl_truncatable_inheritance_cb->setEnabled(FALSE);
        else
            connect(a.idl_truncatable_inheritance_cb, SIGNAL(toggled(bool)), this,
                    SLOT(idl_update_a()));
    }
    else
        a.idl_truncatable_inheritance_cb = 0;

    init_idl_role(a, rel->a, start_data, bg, SLOT(idl_update_a()),
                  SLOT(idl_default_a()), SLOT(idl_unmapped_a()));

    // B
    htab = new Q3HBox(vtab);	// to have a vertical margin
    htab->setMargin(5);

    idl_b = new Q3GroupBox(2, Qt::Horizontal, inb, vtab);
    b.idl_truncatable_inheritance_cb = 0;
    init_idl_role(b, rel->b, end_data, idl_b, SLOT(idl_update_b()),
                  SLOT(idl_default_b()), SLOT(idl_unmapped_b()));

    addTab(vtab, "IDL");

    if (onstereotype || !GenerationSettings::idl_get_default_defs())
        removePage(vtab);

    //
    // USER : list key - value
    //

    vtab = new Q3VBox(this);
    bg = new Q3GroupBox(1, Qt::Horizontal, ina, vtab);
    a.kvtable = new KeyValuesTable(rel->get_start(), bg, a.ro);
    new QLabel(vtab);
    bg = new Q3GroupBox(1, Qt::Horizontal, inb, vtab);
    b.opt.append(bg);
    b.kvtable = new KeyValuesTable(rel->get_end(), bg, b.ro);
    addTab(vtab, TR("Properties"));

    //

    edTypeActivated(edtype->currentItem());

    connect(this, SIGNAL(currentChanged(QWidget *)),
            this, SLOT(update_all_tabs(QWidget *)));

    open_dialog(this);
}

RelationDialog::~RelationDialog()
{
    rel->start->edit_end();
    previous_size = size();

    foreach (BodyDialog *dialog, edits)
        dialog->close();
    edits.clear();

    close_dialog(this);
}

void RelationDialog::polish()
{
    Q3TabDialog::polish();
    UmlDesktop::limitsize_center(this, previous_size, 0.8, 0.8);
}

void RelationDialog::init_uml_role(RoleDialog & role, const RoleData & rel,
                                   Q3GroupBox * bg, BrowserClass * cl1)
{
    bool roleb = (&role == &b);
    QString stereotype = ((ClassData *) cl1->get_data())->get_stereotype();
    bool undef = (stereotype == "enum") || (stereotype == "typedef");
    QString st;
    QLabel * lbl;

    st = ClassDialog::cpp_stereotype(stereotype);
    role.cpp_undef = undef || (st == "enum") || (st == "typedef") || (st == "ignored");

    st = ClassDialog::java_stereotype(stereotype);
    role.java_undef = undef || (st == "enum") || (st == "ignored");

    st = ClassDialog::php_stereotype(stereotype);
    role.php_undef = undef || (st == "enum") || (st == "ignored");

    st = ClassDialog::python_stereotype(stereotype);
    role.python_undef = undef || (st == "enum") || (st == "ignored");

    st = ClassDialog::idl_stereotype(stereotype);
    role.idl_undef = undef || (st == "enum") || (st == "typedef") || (st == "ignored");
    role.idl_in_struct = (st == "struct");
    role.idl_in_union = (st == "union");
    role.idl_in_valuetype = (st == "valuetype");

    lbl = new QLabel(TR("name : "), bg);
    role.opt.append(lbl);
    role.edrole = new LineEdit(rel.role, bg);
    role.edrole->setReadOnly(visit);
    role.opt.append(role.edrole);

    if (roleb) {
        groupb.append(role.edrole);
        groupb.append(lbl);
    }

    Q3HBox * htab;
    QSizePolicy sp;

    role.opt.append(new QLabel(TR("multiplicity : "), bg));
    htab = new Q3HBox(bg);
    htab->setMargin(0);
    role.multiplicity = new Q3ComboBox(!visit, htab);
    sp = role.multiplicity->sizePolicy();
    sp.setHorData(QSizePolicy::Expanding);
    role.multiplicity->setSizePolicy(sp);
    role.previous_multiplicity = (const char *) rel.multiplicity;
    role.multiplicity->insertItem(rel.multiplicity);

    if (!visit) {
        role.multiplicity->insertItem("1");
        role.multiplicity->insertItem("0..1");
        role.multiplicity->insertItem("*");
        role.multiplicity->insertItem("1..*");
    }

    role.opt.append(role.multiplicity);

    lbl = new QLabel(TR("   initial value : "), htab);
    role.opt.append(lbl);
    role.edinit = new LineEdit(rel.init_value, htab);
    role.opt.append(role.edinit);

    if (roleb) {
        groupb.append(role.edinit);
        groupb.append(lbl);
    }

    if (visit)
        role.edinit->setReadOnly(TRUE);
    else {
        SmallPushButton * bt = new SmallPushButton(TR("Editor"), htab);

        connect(bt, SIGNAL(clicked()),
                this, (roleb) ? SLOT(edit_init_b()) : SLOT(edit_init_a()));

        if (roleb)
            groupb.append(bt);
    }

    (void) new QLabel(bg);
    htab = new Q3HBox(bg);
    htab->setMargin(0);

    Q3ButtonGroup * vg = role.uml_visibility.init(htab, rel.uml_visibility, TRUE);

    if (visit)
        vg ->setEnabled(FALSE);
    else if (roleb)
        groupb.append(vg);

    Q3ButtonGroup * bg2 = new Q3ButtonGroup(7, Qt::Horizontal, QString(), htab);

    role.opt.append(bg2);

    if (roleb)
        groupb.append(bg2);

    role.classrelation_cb = new QCheckBox("static", bg2);

    if (rel.isa_class_relation)
        role.classrelation_cb->setChecked(TRUE);

    role.classrelation_cb->setDisabled(visit);

    role.volatile_cb = new QCheckBox("volatile", bg2);

    if (rel.isa_volatile_relation)
        role.volatile_cb->setChecked(TRUE);

    role.volatile_cb->setDisabled(visit);

    role.constrelation_cb = new QCheckBox(TR("read-only"), bg2);

    if (rel.isa_const_relation)
        role.constrelation_cb->setChecked(TRUE);

    role.constrelation_cb->setDisabled(visit);

    role.derived_cb = new QCheckBox(TR("derived"), bg2);

    if (rel.is_derived)
        role.derived_cb->setChecked(TRUE);

    role.derived_cb->setDisabled(visit);
    connect(role.derived_cb, SIGNAL(toggled(bool)),
            (roleb) ? SLOT(derived_changed_b(bool))
            : SLOT(derived_changed_a(bool)));

    role.derivedunion_cb = new QCheckBox("union", bg2);

    if (rel.is_derivedunion)
        role.derivedunion_cb->setChecked(TRUE);

    role.derivedunion_cb->setDisabled(visit || !role.derived_cb->isChecked());

    role.ordered_cb = new QCheckBox(TR("ordered"), bg2);

    if (rel.is_ordered)
        role.ordered_cb->setChecked(TRUE);

    role.ordered_cb->setDisabled(visit);

    role.unique_cb = new QCheckBox("unique", bg2);

    if (rel.is_unique)
        role.unique_cb->setChecked(TRUE);

    role.unique_cb->setDisabled(visit);

    lbl = new QLabel(TR("description : "), bg);

    if (roleb)
        groupb.append(lbl);

    htab = new Q3HBox(bg);
    role.comment = new MultiLineEdit(htab);
    role.comment->setText(rel.comment);
    QFont font = role.comment->font();

    if (! hasCodec())
        font.setFamily("Courier");

    font.setFixedPitch(TRUE);
    role.comment->setFont(font);

    if (visit)
        role.comment->setReadOnly(TRUE);
    else {
        Q3VBox * vtab = new Q3VBox(htab);
        SmallPushButton * bt1 = new SmallPushButton(TR("Editor"), vtab);

        connect(bt1, SIGNAL(clicked()), this,
                (roleb) ? SLOT(edit_description_b()) : SLOT(edit_description_a()));

        SmallPushButton * bt2 = new SmallPushButton(TR("Default"), vtab);

        connect(bt2, SIGNAL(clicked()), this,
                (roleb) ? SLOT(default_description_b()) : SLOT(default_description_a()));

        if (roleb) {
            groupb.append(lbl);
            groupb.append(role.comment);
            groupb.append(bt1);
            groupb.append(bt2);
        }
    }

    lbl = new QLabel(TR("constraint : "), bg);

    if (roleb)
        groupb.append(lbl);

    htab = new Q3HBox(bg);
    role.constraint = new MultiLineEdit(htab);
    role.constraint->setText(rel.constraint);
    role.constraint->setFont(font);

    if (visit)
        role.constraint->setReadOnly(TRUE);
    else {
        Q3VBox * vtab = new Q3VBox(htab);
        SmallPushButton * bt1 = new SmallPushButton(TR("Editor"), vtab);

        connect(bt1, SIGNAL(clicked()), this,
                (roleb) ? SLOT(edit_constraint_b()) : SLOT(edit_constraint_a()));

        if (roleb) {
            groupb.append(lbl);
            groupb.append(role.constraint);
        }
    }
}

void RelationDialog::default_defs_if_needed(UmlCode prev_type)
{
    QString am = a.multiplicity->currentText().stripWhiteSpace();
    QString bm = b.multiplicity->currentText().stripWhiteSpace();

    if ((prev_type != current_type) || (am != a.previous_multiplicity)) {
        default_a_cpp_if_needed(prev_type, a.previous_multiplicity, am);
        default_a_java_if_needed(prev_type, a.previous_multiplicity, am);
        default_a_idl_if_needed(prev_type, a.previous_multiplicity, am);
        default_a_php_if_needed(prev_type, a.previous_multiplicity, am);
        default_a_python_if_needed(prev_type, a.previous_multiplicity, am);

        a.previous_multiplicity = am;
    }

    if ((prev_type != current_type) || (bm != b.previous_multiplicity)) {
        default_b_cpp_if_needed(prev_type, b.previous_multiplicity, bm);
        default_b_java_if_needed(prev_type, b.previous_multiplicity, bm);
        default_b_idl_if_needed(prev_type, b.previous_multiplicity, bm);
        default_b_php_if_needed(prev_type, b.previous_multiplicity, bm);
        default_b_python_if_needed(prev_type, b.previous_multiplicity, bm);

        b.previous_multiplicity = am;
    }
}

void RelationDialog::default_description_a()
{
    a.comment->setText(GenerationSettings::default_relation_description());
}

void RelationDialog::edit_description_a()
{
    edit(a.comment->text(), a.edrole->text().stripWhiteSpace() + "_description",
         &rel->a, TxtEdit, this, (post_edit) post_edit_description_a, edits);
}

void RelationDialog::post_edit_description_a(RelationDialog * d, QString s)
{
    d->a.comment->setText(s);
}

void RelationDialog::default_description_b()
{
    b.comment->setText(GenerationSettings::default_relation_description());
}

void RelationDialog::edit_description_b()
{
    edit(b.comment->text(), a.edrole->text().stripWhiteSpace() + "_description",
         &rel->b, TxtEdit, this, (post_edit) post_edit_description_b, edits);
}

void RelationDialog::post_edit_description_b(RelationDialog * d, QString s)
{
    d->b.comment->setText(s);
}

void RelationDialog::edit_constraint_a()
{
    edit(a.constraint->text(), a.edrole->text().stripWhiteSpace() + "_constraint",
         &rel->a, TxtEdit, this, (post_edit) post_edit_constraint_a, edits);
}

void RelationDialog::post_edit_constraint_a(RelationDialog * d, QString s)
{
    d->a.constraint->setText(s);
}

void RelationDialog::edit_constraint_b()
{
    edit(b.constraint->text(), a.edrole->text().stripWhiteSpace() + "_constraint",
         &rel->b, TxtEdit, this, (post_edit) post_edit_constraint_b, edits);
}

void RelationDialog::post_edit_constraint_b(RelationDialog * d, QString s)
{
    d->b.constraint->setText(s);
}

void RelationDialog::derived_changed_a(bool on)
{
    a.derivedunion_cb->setEnabled(on);
}

void RelationDialog::derived_changed_b(bool on)
{
    b.derivedunion_cb->setEnabled(on);
}

void RelationDialog::init_cpp_role(RoleDialog & role, const RoleData & rel,
                                   Q3GroupBox * bg,
                                   const char * cpp_update_slot,
                                   const char * cpp_default_slot,
                                   const char * cpp_unmapped_slot,
                                   const char * cpp_include_in_header_slot)
{
    Q3HBox * htab;

    new QLabel(TR("Visibility : "), bg);
    htab = new Q3HBox(bg);
    role.cpp_visibility.init(htab, rel.cpp_visibility, FALSE, 0, TR("follow uml"))
    ->setEnabled(!visit);

    new QLabel(" ", htab);

    role.mutable_cb = new QCheckBox("mutable", htab);
    role.opt.append(role.mutable_cb);

    if (rel.cpp_mutable)
        role.mutable_cb->setChecked(TRUE);

    if (visit)
        role.mutable_cb->setEnabled(FALSE);
    else
        connect(role.mutable_cb, SIGNAL(toggled(bool)), this, cpp_update_slot);

    role.opt.append(new QLabel(TR("Declaration : "), bg));
    role.edcppdecl = new MultiLineEdit(bg);
    QFont font = role.edcppdecl->font();

    if (! hasCodec())
        font.setFamily("Courier");

    font.setFixedPitch(TRUE);
    role.opt.append(role.edcppdecl);
    role.edcppdecl->setText(rel.cpp_decl);
    role.edcppdecl->setFont(font);

    if (visit)
        role.edcppdecl->setReadOnly(TRUE);
    else
        connect(role.edcppdecl, SIGNAL(textChanged()), this, cpp_update_slot);

    new QLabel(TR("Result after\nsubstitution : "), bg);
    role.showcppdecl = new MultiLineEdit(bg);
    role.showcppdecl->setReadOnly(TRUE);
    role.showcppdecl->setFont(font);

    if (! visit) {
        new QLabel("", bg);
        htab = new Q3HBox(bg);
        htab->setMargin(5);

        role.cpp_default_decl_bt = new QPushButton(TR("Default declaration"), htab);
        connect(role.cpp_default_decl_bt, SIGNAL(clicked()),
                this, cpp_default_slot);

        if (cpp_include_in_header_slot != 0) {
            role.cpp_include_in_header = new QPushButton(TR("#include in header"), htab);
            connect(role.cpp_include_in_header , SIGNAL(clicked()),
                    this, cpp_include_in_header_slot);
        }

        role.cpp_unmapped_decl_bt = new QPushButton(TR("Not generated in C++"), htab);
        connect(role.cpp_unmapped_decl_bt, SIGNAL(clicked()),
                this, cpp_unmapped_slot);
    }
}

void RelationDialog::init_java_role(RoleDialog & role, const RoleData & rel,
                                    Q3GroupBox * bg,
                                    const char * java_update_slot,
                                    const char * java_default_slot,
                                    const char * java_unmapped_slot,
                                    const char * java_edit_annotation)
{
    if (! role.java_undef) {
        new QLabel("", bg);
        role.transient_cb = new QCheckBox("transient", bg);
        role.opt.append(role.transient_cb);

        if (rel.java_transient)
            role.transient_cb->setChecked(TRUE);

        if (visit)
            role.transient_cb->setEnabled(FALSE);
        else
            connect(role.transient_cb, SIGNAL(toggled(bool)), this, java_update_slot);
    }

    role.opt.append(new QLabel(TR("Declaration : "), bg));
    role.edjavadecl = new MultiLineEdit(bg);
    role.opt.append(role.edjavadecl);
    QFont font = role.edjavadecl->font();

    if (! hasCodec())
        font.setFamily("Courier");

    font.setFixedPitch(TRUE);
    role.edjavadecl->setText(rel.java_decl);
    role.edjavadecl->setFont(font);

    if (visit)
        role.edjavadecl->setReadOnly(TRUE);
    else
        connect(role.edjavadecl, SIGNAL(textChanged()), this, java_update_slot);

    role.opt.append(new QLabel(TR("Result after\nsubstitution : "), bg));
    role.showjavadecl = new MultiLineEdit(bg);
    role.opt.append(role.showjavadecl);
    role.showjavadecl->setReadOnly(TRUE);
    role.showjavadecl->setFont(font);

    new QLabel("", bg);
    Q3HBox * htab = new Q3HBox(bg);
    htab->setMargin(5);

    if (! visit) {
        role.java_default_decl_bt = new QPushButton(TR("Default declaration"), htab);
        connect(role.java_default_decl_bt, SIGNAL(clicked()),
                this, java_default_slot);
        role.java_unmapped_decl_bt = new QPushButton(TR("Not generated in Java"), htab);
        connect(role.java_unmapped_decl_bt, SIGNAL(clicked()),
                this, java_unmapped_slot);
    }

    role.javaannotation = (const char *) rel.java_annotation;
    role.editjavaannotation =
        new QPushButton((visit) ? TR("Show annotation") : TR("Edit annotation"),
                        htab);
    connect(role.editjavaannotation, SIGNAL(clicked()),
            this, java_edit_annotation);
}

void RelationDialog::init_php_role(RoleDialog & role, const RoleData & rel,
                                   Q3GroupBox * bg,
                                   const char * php_update_slot,
                                   const char * php_default_slot,
                                   const char * php_unmapped_slot)
{
    role.opt.append(new QLabel(TR("Declaration : "), bg));
    role.edphpdecl = new MultiLineEdit(bg);
    role.opt.append(role.edphpdecl);
    QFont font = role.edphpdecl->font();

    if (! hasCodec())
        font.setFamily("Courier");

    font.setFixedPitch(TRUE);
    role.edphpdecl->setText(rel.php_decl);
    role.edphpdecl->setFont(font);

    if (visit)
        role.edphpdecl->setReadOnly(TRUE);
    else
        connect(role.edphpdecl, SIGNAL(textChanged()), this, php_update_slot);

    role.opt.append(new QLabel(TR("Result after\nsubstitution : "), bg));
    role.showphpdecl = new MultiLineEdit(bg);
    role.opt.append(role.showphpdecl);
    role.showphpdecl->setReadOnly(TRUE);
    role.showphpdecl->setFont(font);

    new QLabel("", bg);
    Q3HBox * htab = new Q3HBox(bg);
    htab->setMargin(5);

    if (! visit) {
        role.php_default_decl_bt = new QPushButton(TR("Default declaration"), htab);
        connect(role.php_default_decl_bt, SIGNAL(clicked()),
                this, php_default_slot);
        role.php_unmapped_decl_bt = new QPushButton(TR("Not generated in Php"), htab);
        connect(role.php_unmapped_decl_bt, SIGNAL(clicked()),
                this, php_unmapped_slot);
    }
}

void RelationDialog::init_python_role(RoleDialog & role, const RoleData & rel,
                                      BrowserClass * cl, Q3GroupBox * bg,
                                      const char * python_update_slot,
                                      const char * python_default_slot,
                                      const char * python_unmapped_slot)
{
    role.python_self =
        BrowserOperation::python_init_self(cl) + ".";

    role.opt.append(new QLabel(TR("Declaration : "), bg));
    role.edpythondecl = new MultiLineEdit(bg);
    role.opt.append(role.edpythondecl);
    QFont font = role.edpythondecl->font();

    if (! hasCodec())
        font.setFamily("Courier");

    font.setFixedPitch(TRUE);
    role.edpythondecl->setText(rel.python_decl);
    role.edpythondecl->setFont(font);

    if (visit)
        role.edpythondecl->setReadOnly(TRUE);
    else
        connect(role.edpythondecl, SIGNAL(textChanged()), this, python_update_slot);

    role.opt.append(new QLabel(TR("Result after\nsubstitution : "), bg));
    role.showpythondecl = new MultiLineEdit(bg);
    role.opt.append(role.showpythondecl);
    role.showpythondecl->setReadOnly(TRUE);
    role.showpythondecl->setFont(font);

    new QLabel("", bg);
    Q3HBox * htab = new Q3HBox(bg);
    htab->setMargin(5);

    if (! visit) {
        role.python_default_decl_bt = new QPushButton(TR("Default declaration"), htab);
        connect(role.python_default_decl_bt, SIGNAL(clicked()),
                this, python_default_slot);
        role.python_unmapped_decl_bt = new QPushButton(TR("Not generated in Python"), htab);
        connect(role.python_unmapped_decl_bt, SIGNAL(clicked()),
                this, python_unmapped_slot);
    }
}

void RelationDialog::init_idl_role(RoleDialog & role, const RoleData & rel,
                                   ClassData * cld, Q3GroupBox * bg,
                                   const char * idl_update_slot,
                                   const char * idl_default_slot,
                                   const char * idl_unmapped_slot)
{
    if (role.idl_in_union) {
        role.opt.append(new QLabel("Case : ", bg));
        role.edcase = new Q3ComboBox(!visit, bg);
        role.edcase->insertItem(RelationData::get_idlcase(rel));

        if (!visit) {
            role.edcase->setAutoCompletion(completion());

            AType switch_type = cld->get_switch_type();

            if (switch_type.type != 0) {
                switch_type.type->children(role.enums, UmlAttribute);
                role.enums.names(role.enum_names);
                role.edcase->insertStringList(role.enum_names);
            }
        }

        role.edcase->setCurrentItem(0);

        QSizePolicy sp = role.edcase->sizePolicy();

        sp.setHorData(QSizePolicy::Expanding);
        role.edcase->setSizePolicy(sp);

        if (!visit)
            connect(role.edcase, SIGNAL(activated(int)), this, idl_update_slot);
    }

    role.opt.append(new QLabel(TR("Declaration : "), bg));
    role.edidldecl = new MultiLineEdit(bg);
    role.opt.append(role.edidldecl);
    role.edidldecl->setText(rel.idl_decl);
    QFont font = role.edidldecl->font();

    if (! hasCodec())
        font.setFamily("Courier");

    font.setFixedPitch(TRUE);
    role.edidldecl->setFont(font);

    if (visit)
        role.edidldecl->setReadOnly(TRUE);
    else
        connect(role.edidldecl, SIGNAL(textChanged()), this, idl_update_slot);

    role.opt.append(new QLabel(TR("Result after\nsubstitution : "), bg));
    role.showidldecl = new MultiLineEdit(bg);
    role.opt.append(role.showidldecl);
    role.showidldecl->setReadOnly(TRUE);
    role.showidldecl->setFont(font);

    if (! visit) {
        new QLabel("", bg);
        Q3HBox * htab = new Q3HBox(bg);
        htab->setMargin(5);
        role.idl_default_decl_bt = new QPushButton(TR("Default declaration"), htab);
        connect(role.idl_default_decl_bt, SIGNAL(clicked()),
                this, idl_default_slot);
        role.idl_unmapped_decl_bt = new QPushButton(TR("Not generated in Idl"), htab);
        connect(role.idl_unmapped_decl_bt, SIGNAL(clicked()),
                this, idl_unmapped_slot);
    }
}

static void set_enabled(QWidgetList & l, bool y)
{
    // QWidget * w;
    for (QWidgetList::iterator it = l.begin(); it != l.end(); it++)
        (*it)->setEnabled(y);

    /*[lgfreitas] better change here
    for (w = l.first(); w != 0; w = l.next())
      w->setEnabled(y);
    */
}

void RelationDialog::set_inherit_or_dependency(UmlCode type)
{
    if (! visit) {
        switch (type) {
        case UmlRealize:
        case UmlGeneralisation:
            a.cpp_virtual_inheritance_cb->setEnabled(TRUE);

            if (! a.java_undef) a.transient_cb->setEnabled(FALSE);

            if (a.idl_truncatable_inheritance_cb != 0)
                a.idl_truncatable_inheritance_cb->setEnabled(TRUE);

            break;

        case UmlDependency:
            a.cpp_virtual_inheritance_cb->setEnabled(FALSE);

            if (! a.java_undef) a.transient_cb->setEnabled(FALSE);

            if (a.idl_truncatable_inheritance_cb != 0)
                a.idl_truncatable_inheritance_cb->setEnabled(FALSE);

            break;

        default:
            a.cpp_virtual_inheritance_cb->setEnabled(FALSE);

            if (! a.java_undef) a.transient_cb->setEnabled(TRUE);

            if (a.idl_truncatable_inheritance_cb != 0)
                a.idl_truncatable_inheritance_cb->setEnabled(FALSE);
        }
    }
}

void RelationDialog::edTypeActivated(int r)
{
    UmlCode type;

    if (visit)
        type = current_type;
    else if (b.ro)
        type = (rel->end == 0) ? UniRelTypes[r].type : BiRelTypes[r].type;
    else
        type = RelTypes[r].type;

    if ((edname->text() == RelationData::default_name(current_type)) ||
        !RelationData::isa_association(type))
        edname->setText(RelationData::default_name(type));

    if (!RelationData::isa_association(type)) {
        edname->setEnabled(FALSE);
        edassociation->setEnabled(FALSE);
        ::set_enabled(a.opt, type != UmlDependency);
        // note : management of dependency for buttons presence
        // and text done in update_all_tabs() to take into account
        // stereotype change

        ::set_enabled(groupb, FALSE);
        cpp_b->setEnabled(FALSE);
        java_b->setEnabled(FALSE);
        php_b->setEnabled(FALSE);
        python_b->setEnabled(FALSE);
        idl_b->setEnabled(FALSE);
    }
    else {
        if (! visit) {
            edname->setEnabled(TRUE);
            edassociation->setEnabled(TRUE);
            ::set_enabled(a.opt, TRUE);
        }

        if (RelationData::uni_directional(type)) {
            ::set_enabled(groupb, FALSE);
            cpp_b->setEnabled(FALSE);
            java_b->setEnabled(FALSE);
            php_b->setEnabled(FALSE);
            python_b->setEnabled(FALSE);
            idl_b->setEnabled(FALSE);
        }
        else if (! visit) {
            ::set_enabled(groupb, TRUE);
            cpp_b->setEnabled(TRUE);
            java_b->setEnabled(TRUE);
            php_b->setEnabled(TRUE);
            python_b->setEnabled(TRUE);
            idl_b->setEnabled(TRUE);
        }
    }

    if (current_type != type) {
        UmlCode t = current_type;

        current_type = type;

        if (!visit) {
            edstereotype->clear();
            edstereotype->insertItem("");
            edstereotype->insertStringList(rel->get_start_class()
                                           ->default_stereotypes(type,
                                                   rel->get_end_class()));
            edstereotype->insertStringList(ProfiledStereotypes::defaults(UmlRelations));
            edstereotype->setCurrentItem(0);

            default_defs_if_needed(t);
        }
    }

    set_inherit_or_dependency(type);
}



void RelationDialog::menu_assoc()
{
    Q3PopupMenu m(0);

    m.insertItem(TR("Choose"), -1);
    m.insertSeparator();

    int index = list.findIndex(edassociation->currentText().stripWhiteSpace());

    if (index != -1)
        m.insertItem(TR("Select in browser"), 0);

    BrowserNode * bn = 0;

    if (! visit) {
        bn = BrowserView::selected_item();

        if ((bn != 0) && (bn->get_type() == UmlClass) && !bn->deletedp())
            m.insertItem(TR("Choose class selected in browser"), 1);
        else
            bn = 0;

        m.insertItem(TR("Create class and choose it"), 2);
    }

    if (!visit || (index != -1) || (bn != 0)) {
        switch (m.exec(QCursor::pos())) {
        case 0:
            nodes.at(index)->select_in_browser();
            break;

        case 2:
            bn = BrowserClass::add_class(FALSE, view);

            if (bn == 0)
                return;

            bn->select_in_browser();

            // no break
        case 1: {
            QString s = bn->full_name(TRUE);

            if ((index = list.findIndex(s)) == -1) {
                // new class, may be created through an other dialog
                index = 0;
                QStringList::Iterator iter = list.begin();
                QStringList::Iterator iter_end = list.end();

                while ((iter != iter_end) && (*iter < s)) {
                    ++iter;
                    index += 1;
                }

                nodes.insert((unsigned) index, bn);
                list.insert(iter, s);
                edassociation->insertItem(s, index + offset);
            }
        }

        edassociation->setCurrentItem(index + offset);
        break;

        default:
            break;
        }
    }
}

void RelationDialog::update_all_tabs(QWidget * w)
{
    if (!visit)
        default_defs_if_needed(current_type);

    if (current_type == UmlDependency) {
        QString s = a.edcppdecl->text().stripWhiteSpace();

        if (GenerationSettings::cpp_relationattribute_stereotype(fromUnicode(edstereotype->currentText().stripWhiteSpace()))
            != "friend") {
            if (! visit) {
                a.cpp_include_in_header->show();
                a.cpp_default_decl_bt->setText(TR("#include in source"));
                a.cpp_default_decl_bt->setEnabled(TRUE);
                a.cpp_unmapped_decl_bt->setEnabled(TRUE);
            }

            if (!s.isEmpty() &&
                (s != TR("#include in source")) &&
                (s != TR("#include in header")))
                s = TR("#include in header");
        }
        else {
            if (! visit) {
                a.cpp_include_in_header->hide();
                a.cpp_default_decl_bt->setText(TR("Default declaration"));
                a.cpp_default_decl_bt->setEnabled(FALSE);
                a.cpp_unmapped_decl_bt->setEnabled(FALSE);
            }

            s = "friend " +
                ClassDialog::cpp_stereotype(rel->get_end_class()->get_stereotype()) +
                " ${type};";
        }

        a.edcppdecl->setText(s);
    }
    else if (! visit) {
        a.cpp_include_in_header->hide();
        a.cpp_default_decl_bt->setText(TR("Default declaration"));
        a.cpp_default_decl_bt->setEnabled(TRUE);
    }

    a.edrole->setText(a.edrole->text().stripWhiteSpace());
    b.edrole->setText(b.edrole->text().stripWhiteSpace());

    if (w == umltab) {
        if (! visit)
            a.edrole->setFocus();
    }
    else if (w == cppTab) {
        cpp_update_a();
        cpp_update_b();

        if (! visit)
            a.edcppdecl->setFocus();
    }
    else if (w == javatab) {
        java_update_a();
        java_update_b();

        if (! visit)
            a.edjavadecl->setFocus();
    }
    else if (w == phptab) {
        php_update_a();
        php_update_b();

        if (! visit)
            a.edphpdecl->setFocus();
    }
    else if (w == pythontab) {
        python_update_a();
        python_update_b();

        if (! visit)
            a.edpythondecl->setFocus();
    }
    else if (w == idltab) {
        idl_update_a();
        idl_update_b();

        if (! visit)
            a.edidldecl->setFocus();
    }
}

void RelationDialog::edit_init_a()
{
    edit(a.edinit->text(), a.edrole->text().stripWhiteSpace() + "_initialization",
         &rel->a, TxtEdit, this, (post_edit) post_edit_init_a, edits);
}

void RelationDialog::post_edit_init_a(RelationDialog * d, QString s)
{
    d->a.edinit->setText(s);
}

void RelationDialog::edit_init_b()
{
    edit(b.edinit->text(), b.edrole->text().stripWhiteSpace() + "_initialization",
         &rel->b, TxtEdit, this, (post_edit) post_edit_init_b, edits);
}

void RelationDialog::post_edit_init_b(RelationDialog * d, QString s)
{
    d->b.edinit->setText(s);
}

// C ++ management

void RelationDialog::cpp_update(RoleDialog & role, BrowserClass * cl, BrowserNode * rl)
{
    role.cpp_visibility.update_default(role.uml_visibility);

    QString s;

    switch (current_type) {
    case UmlRealize:
    case UmlGeneralisation: {
        // do NOT write
        //	const char * p = role.edcppdecl->text();
        // because the QString is immediatly destroyed !
        QString def = role.edcppdecl->text();
        const char * p = def;

        while (*p) {
            if (!strncmp(p, "${type}", 7)) {
                if (role.cpp_virtual_inheritance_cb->isChecked())
                    s = "virtual ";

                s += get_cpp_name(cl);
                p += 7;
            }
            else if (*p == '@')
                manage_alias(rl, p, s, role.kvtable);
            else
                s += *p++;
        }
    }
    break;

    case UmlDependency:
        if (&role == &b) {
            s  = "";
            break;
        }

        // no break
    default: {
        // do NOT write
        //	const char * p = role.edcppdecl->text();
        // because the QString is immediatly destroyed !
        QString def = role.edcppdecl->text();
        const char * p = def;
        const char * pp = 0;
        QString indent = "";

        while ((*p == ' ') || (*p == '\t'))
            indent += *p++;

        if (*p != '#')
            s = indent;

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
                manage_comment(role.comment->text(), p, pp,
                               GenerationSettings::cpp_javadoc_style());
            else if (!strncmp(p, "${description}", 14))
                manage_description(role.comment->text(), p, pp);
            else if (!strncmp(p, "${static}", 9)) {
                p += 9;

                if (role.classrelation_cb->isChecked())
                    s += "static ";
            }
            else if (!strncmp(p, "${const}", 8)) {
                p += 8;

                if (role.constrelation_cb->isChecked())
                    s += "const ";
            }
            else if (!strncmp(p, "${mutable}", 10)) {
                p += 10;

                if (role.mutable_cb->isChecked())
                    s += "mutable ";
            }
            else if (!strncmp(p, "${volatile}", 11)) {
                p += 11;

                if (role.volatile_cb->isChecked())
                    s += "volatile ";
            }
            else if (!strncmp(p, "${type}", 7)) {
                p += 7;
                s += get_cpp_name(cl);
            }
            else if (!strncmp(p, "${name}", 7)) {
                p += 7;
                s += role.edrole->text();
            }
            else if (!strncmp(p, "${inverse_name}", 15)) {
                p += 15;

                if (! RelationData::uni_directional(current_type))
                    s += ((&role == &a) ? b : a).edrole->text();
            }
            else if (!strncmp(p, "${multiplicity}", 15)) {
                p += 15;

                QString m = role.multiplicity->currentText().stripWhiteSpace();

                s += (*m == '[') ? m : QString("[") + m + "]";
            }
            else if (!strncmp(p, "${stereotype}", 13)) {
                p += 13;
                s += GenerationSettings::cpp_relationattribute_stereotype(fromUnicode(edstereotype->currentText().stripWhiteSpace()));
            }
            else if (!strncmp(p, "${h_value}", 10)) {
                p += 10;

                if (!role.edinit->text().stripWhiteSpace().isEmpty() &&
                    role.classrelation_cb->isChecked()) {
                    if (role.edinit->text().at(0) == QChar('='))
                        s += " ";

                    s += role.edinit->text();
                }
            }
            else if (!strncmp(p, "${value}", 8))
                p += 8;
            else if (!strncmp(p, "${association}", 14)) {
                p += 14;
                s += GenerationSettings::cpp_type(type(edassociation->currentText().stripWhiteSpace(),
                                                       list, nodes));
            }
            else if (*p == '\n') {
                s += *p++;

                if (*p && (*p != '#'))
                    s += indent;
            }
            else if (*p == '@')
                manage_alias(rl, p, s, role.kvtable);
            else
                s += *p++;
        }
    }
    }

    role.showcppdecl->setText(s);
}

void RelationDialog::cpp_update_a()
{
    cpp_update(a, rel->get_end_class(), rel->start);
}

void RelationDialog::cpp_update_b()
{
    if (! RelationData::uni_directional(current_type))
        cpp_update(b, rel->get_start_class(), rel->end);
}

void RelationDialog::cpp_default_a()
{
    if (a.cpp_undef)
        a.edcppdecl->setText(QString());
    else if (RelationData::isa_association(current_type))
        a.edcppdecl->setText(GenerationSettings::cpp_default_rel_decl(current_type,
                             a.multiplicity->currentText().stripWhiteSpace()));
    else {
        if (current_type != UmlDependency)
            a.edcppdecl->setText("${type}");
        else if (GenerationSettings::cpp_relationattribute_stereotype(fromUnicode(edstereotype->currentText().stripWhiteSpace()))
                 != "friend")
            a.edcppdecl->setText(TR("#include in source"));
        else
            a.edcppdecl->setText("friend " +
                                 ClassDialog::cpp_stereotype(rel->get_end_class()->get_stereotype()) +
                                 " ${type};");

        if (! visit)
            a.cpp_unmapped_decl_bt->setOn(FALSE);
    }

    cpp_update_a();
}

void RelationDialog::default_a_cpp_if_needed(UmlCode prev_type,
        QString prev_mult,
        QString new_mult)
{
    if (!a.edcppdecl->text().isEmpty()) {
        if (!RelationData::isa_association(prev_type) ||
            !RelationData::isa_association(current_type)) {
            if (prev_type != current_type)
                cpp_default_a();
        }
        else {
            QString n = GenerationSettings::cpp_default_rel_decl(current_type, new_mult);

            if (n != GenerationSettings::cpp_default_rel_decl(prev_type, prev_mult))
                a.edcppdecl->setText(n);
        }
    }
}

void RelationDialog::cpp_include_in_header()
{
    a.edcppdecl->setText(TR("#include in header"));

    if (! visit)
        a.cpp_unmapped_decl_bt->setOn(FALSE);

    cpp_update_a();
}

void RelationDialog::cpp_unmapped_a()
{
    a.edcppdecl->setText(QString());
    a.showcppdecl->setText(QString());

    if (!RelationData::isa_association(current_type) && !visit)
        a.cpp_default_decl_bt->setOn(FALSE);
}

void RelationDialog::cpp_default_b()
{
    if (b.cpp_undef)
        b.edcppdecl->setText(QString());
    else
        b.edcppdecl->setText(GenerationSettings::cpp_default_rel_decl(UmlAssociation,
                             b.multiplicity->currentText().stripWhiteSpace()));

    cpp_update_b();
}

void RelationDialog::default_b_cpp_if_needed(UmlCode prev_type,
        QString prev_mult,
        QString new_mult)
{
    if (!b.edcppdecl->text().isEmpty()) {
        QString n = GenerationSettings::cpp_default_rel_decl(current_type, new_mult);

        if (n != GenerationSettings::cpp_default_rel_decl(prev_type, prev_mult))
            b.edcppdecl->setText(n);
    }
}

void RelationDialog::cpp_unmapped_b()
{
    b.edcppdecl->setText(QString());
    b.showcppdecl->setText(QString());
}

// Java management

void RelationDialog::java_update(RoleDialog & role, BrowserClass * cl, BrowserNode * rl)
{
    QString s;

    switch (current_type) {
    case UmlRealize:
    case UmlGeneralisation:
        role.editjavaannotation->setEnabled(FALSE);
        {
            // do NOT write
            //	const char * p = role.edjavadecl->text();
            // because the QString is immediatly destroyed !
            QString def = role.edjavadecl->text();
            const char * p = def;

            while (*p) {
                if (!strncmp(p, "${type}", 7)) {
                    s += get_java_name(cl);
                    p += 7;
                }
                else if (*p == '@')
                    manage_alias(rl, p, s, role.kvtable);
                else
                    s += *p++;
            }
        }
        break;

    case UmlDependency:
        role.editjavaannotation->setEnabled(FALSE);
        s = "";
        break;

    default: {
        // do NOT write
        //	const char * p = role.edjavadecl->text();
        // because the QString is immediatly destroyed !
        QString def = role.edjavadecl->text();
        const char * p = def;
        const char * pp = 0;
        QString indent = "";

        while ((*p == ' ') || (*p == '\t'))
            indent += *p++;

        s = indent;

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
                manage_comment(role.comment->text(), p, pp,
                               GenerationSettings::java_javadoc_style());
            else if (!strncmp(p, "${description}", 14))
                manage_description(role.comment->text(), p, pp);
            else if (!strncmp(p, "${visibility}", 13)) {
                p += 13;

                if (role.uml_visibility.value() != UmlPackageVisibility)
                    s += role.uml_visibility.state() + " ";
            }
            else if (!strncmp(p, "${static}", 9)) {
                p += 9;

                if (role.classrelation_cb->isChecked())
                    s += "static ";
            }
            else if (!strncmp(p, "${transient}", 12)) {
                p += 12;

                if (!role.java_undef && role.transient_cb->isChecked())
                    s += "transient ";
            }
            else if (!strncmp(p, "${volatile}", 11)) {
                p += 11;

                if (role.volatile_cb->isChecked())
                    s += "volatile ";
            }
            else if (!strncmp(p, "${final}", 8)) {
                p += 8;

                if (role.constrelation_cb->isChecked())
                    s += "final ";
            }
            else if (!strncmp(p, "${type}", 7)) {
                p += 7;
                s += get_java_name(cl);
            }
            else if (!strncmp(p, "${name}", 7)) {
                p += 7;
                s += role.edrole->text();
            }
            else if (!strncmp(p, "${inverse_name}", 15)) {
                p += 15;

                if (! RelationData::uni_directional(current_type))
                    s += ((&role == &a) ? b : a).edrole->text();
            }
            else if (!strncmp(p, "${multiplicity}", 15)) {
                p += 15;
                s += java_multiplicity(role.multiplicity->currentText().stripWhiteSpace());
            }
            else if (!strncmp(p, "${stereotype}", 13)) {
                p += 13;
                s += GenerationSettings::java_relationattribute_stereotype(fromUnicode(edstereotype->currentText().stripWhiteSpace()));
            }
            else if (!strncmp(p, "${value}", 8)) {
                QString i = role.edinit->text().stripWhiteSpace();

                if (!i.isEmpty()) {
                    if (need_equal(p, i, FALSE))
                        s += " = ";

                    s += role.edinit->text();
                }

                p += 8;
            }
            else if (!strncmp(p, "${association}", 14)) {
                p += 14;
                s += GenerationSettings::java_type(type(edassociation->currentText().stripWhiteSpace(),
                                                        list, nodes));
            }
            else if (!strncmp(p, "${@}", 4)) {
                p += 4;

                if (pp != 0)
                    s += "${@}";
                else if (!role.javaannotation.isEmpty()) {
                    pp = p;
                    p = role.javaannotation;
                }
            }
            else if (*p == '\n') {
                s += *p++;

                if (*p)
                    s += indent;
            }
            else if (*p == '@')
                manage_alias(rl, p, s, role.kvtable);
            else
                s += *p++;
        }

        role.editjavaannotation->setEnabled(def.find("${@}") != -1);
    }
    }

    role.showjavadecl->setText(s);
}

void RelationDialog::java_update_a()
{
    java_update(a, rel->get_end_class(), rel->start);
}

void RelationDialog::java_update_b()
{
    if (! RelationData::uni_directional(current_type))
        java_update(b, rel->get_start_class(), rel->end);
}

void RelationDialog::java_default_a()
{
    if (a.java_undef)
        a.edjavadecl->setText(QString());
    else if (RelationData::isa_association(current_type))
        a.edjavadecl->setText(GenerationSettings::java_default_rel_decl(a.multiplicity->currentText().stripWhiteSpace()));
    else {
        a.edjavadecl->setText("${type}");
        a.java_unmapped_decl_bt->setOn(FALSE);
    }

    java_update_a();
}

void RelationDialog::default_a_java_if_needed(UmlCode prev_type,
        QString prev_mult,
        QString new_mult)
{
    if (!a.edjavadecl->text().isEmpty()) {
        if (!RelationData::isa_association(prev_type) ||
            !RelationData::isa_association(current_type)) {
            if (prev_type != current_type)
                java_default_a();
        }
        else {
            QString n = GenerationSettings::java_default_rel_decl(new_mult);

            if (n != GenerationSettings::java_default_rel_decl(prev_mult))
                a.edjavadecl->setText(n);
        }
    }
}

void RelationDialog::java_unmapped_a()
{
    a.edjavadecl->setText(QString());
    a.showjavadecl->setText(QString());

    if (!RelationData::isa_association(current_type))
        a.java_default_decl_bt->setOn(FALSE);
}

void RelationDialog::java_default_b()
{
    if (b.java_undef)
        b.edjavadecl->setText(QString());
    else
        b.edjavadecl->setText(GenerationSettings::java_default_rel_decl(b.multiplicity->currentText().stripWhiteSpace()));

    java_update_b();
}

void RelationDialog::default_b_java_if_needed(UmlCode,
        QString prev_mult,
        QString new_mult)
{
    if (!b.edjavadecl->text().isEmpty()) {
        QString n = GenerationSettings::java_default_rel_decl(new_mult);

        if (n != GenerationSettings::java_default_rel_decl(prev_mult))
            b.edjavadecl->setText(n);
    }
}

void RelationDialog::java_unmapped_b()
{
    b.edjavadecl->setText(QString());
    b.showjavadecl->setText(QString());
}

void RelationDialog::java_edit_annotation_a()
{
    AnnotationDialog dialog(this, a.javaannotation, !hasOkButton());

    if (dialog.exec() == QDialog::Accepted)
        java_update_a();
}

void RelationDialog::java_edit_annotation_b()
{
    AnnotationDialog dialog(this, b.javaannotation, !hasOkButton());

    if (dialog.exec() == QDialog::Accepted)
        java_update_b();
}

// Php management

void RelationDialog::php_update(RoleDialog & role, BrowserClass * cl, BrowserNode * rl)
{
    QString s;

    switch (current_type) {
    case UmlRealize:
    case UmlGeneralisation: {
        // do NOT write
        //	const char * p = role.edphpdecl->text();
        // because the QString is immediatly destroyed !
        QString def = role.edphpdecl->text();
        const char * p = def;

        while (*p) {
            if (!strncmp(p, "${type}", 7)) {
                s += get_php_name(cl);
                p += 7;
            }
            else if (*p == '@')
                manage_alias(rl, p, s, role.kvtable);
            else
                s += *p++;
        }
    }
    break;

    case UmlDependency:
        s = "";
        break;

    default: {
        // do NOT write
        //	const char * p = role.edphpdecl->text();
        // because the QString is immediatly destroyed !
        QString def = role.edphpdecl->text();
        const char * p = def;
        const char * pp = 0;
        QString indent = "";

        while ((*p == ' ') || (*p == '\t'))
            indent += *p++;

        s = indent;

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
                manage_comment(role.comment->text(), p, pp,
                               GenerationSettings::php_javadoc_style());
            else if (!strncmp(p, "${description}", 14))
                manage_description(role.comment->text(), p, pp);
            else if (!strncmp(p, "${visibility}", 13)) {
                p += 13;

                if (role.uml_visibility.value() != UmlPackageVisibility)
                    s += role.uml_visibility.state() + " ";
            }
            else if (!strncmp(p, "${static}", 9)) {
                p += 9;

                if (role.classrelation_cb->isChecked())
                    s += "static ";
            }
            else if (!strncmp(p, "${name}", 7)) {
                p += 7;

                if (!role.constrelation_cb->isChecked())
                    s += "$";

                s += role.edrole->text();
            }
            else if (!strncmp(p, "${inverse_name}", 15)) {
                p += 15;

                if (! RelationData::uni_directional(current_type))
                    s += ((&role == &a) ? b : a).edrole->text();
            }
            else if (!strncmp(p, "${const}", 8)) {
                p += 8;

                if (role.constrelation_cb->isChecked())
                    s += "const ";
            }
            else if (!strncmp(p, "${var}", 6)) {
                p += 6;

                if (!role.constrelation_cb->isChecked() &&
                    !role.classrelation_cb->isChecked() &&
                    (role.uml_visibility.value() == UmlPackageVisibility))
                    s += "var ";
            }
            else if (!strncmp(p, "${value}", 8)) {
                QString i = role.edinit->text().stripWhiteSpace();

                if (!i.isEmpty()) {
                    if (need_equal(p, i, FALSE))
                        s += " = ";

                    s += role.edinit->text();
                }
                else if (need_equal(p, "None", FALSE))
                    s += " = None";
                else
                    s += "None";

                p += 8;
            }
            else if (!strncmp(p, "${type}", 7)) {
                // for comment
                p += 7;
                s += get_php_name(cl);
            }
            else if (*p == '\n') {
                s += *p++;

                if (*p)
                    s += indent;
            }
            else if (*p == '@')
                manage_alias(rl, p, s, role.kvtable);
            else
                s += *p++;
        }
    }
    }

    role.showphpdecl->setText(s);
}

void RelationDialog::php_update_a()
{
    php_update(a, rel->get_end_class(), rel->start);
}

void RelationDialog::php_update_b()
{
    if (! RelationData::uni_directional(current_type))
        php_update(b, rel->get_start_class(), rel->end);
}

void RelationDialog::php_default_a()
{
    if (a.php_undef)
        a.edphpdecl->setText(QString());
    else if (RelationData::isa_association(current_type))
        a.edphpdecl->setText(GenerationSettings::php_default_rel_decl());
    else {
        a.edphpdecl->setText("${type}");
        a.php_unmapped_decl_bt->setOn(FALSE);
    }

    php_update_a();
}

void RelationDialog::default_a_php_if_needed(UmlCode prev_type,
        QString,
        QString)
{
    if (!a.edphpdecl->text().isEmpty()) {
        if (!RelationData::isa_association(prev_type) ||
            !RelationData::isa_association(current_type)) {
            if (prev_type != current_type)
                php_default_a();
        }

        // multiplicity unused
    }
}

void RelationDialog::php_unmapped_a()
{
    a.edphpdecl->setText(QString());
    a.showphpdecl->setText(QString());

    if (!RelationData::isa_association(current_type))
        a.php_default_decl_bt->setOn(FALSE);
}

void RelationDialog::php_default_b()
{
    if (b.php_undef)
        b.edphpdecl->setText(QString());
    else
        b.edphpdecl->setText(GenerationSettings::php_default_rel_decl());

    php_update_b();
}

void RelationDialog::default_b_php_if_needed(UmlCode,
        QString,
        QString)
{
    // multiplicity unused
}

void RelationDialog::php_unmapped_b()
{
    b.edphpdecl->setText(QString());
    b.showphpdecl->setText(QString());
}

// Python management

void RelationDialog::python_update(RoleDialog & role, BrowserClass * cl, BrowserNode * rl)
{
    QString s;

    switch (current_type) {
    case UmlRealize:
    case UmlGeneralisation: {
        // do NOT write
        //	const char * p = role.edpythondecl->text();
        // because the QString is immediatly destroyed !
        QString def = role.edpythondecl->text();
        const char * p = def;

        while (*p) {
            if (!strncmp(p, "${type}", 7)) {
                s += get_python_name(cl);
                p += 7;
            }
            else if (*p == '@')
                manage_alias(rl, p, s, role.kvtable);
            else
                s += *p++;
        }
    }
    break;

    case UmlDependency:
        if (&role == &b) {
            s  = "";
            break;
        }

        // no break
    default: {
        // do NOT write
        //	const char * p = role.edpythondecl->text();
        // because the QString is immediatly destroyed !
        QString def = role.edpythondecl->text();
        const char * p = def;
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
            }

            if (!strncmp(p, "${comment}", 10))
                manage_python_comment(role.comment->text(), p, pp);
            else if (!strncmp(p, "${description}", 14))
                manage_python_description(role.comment->text(), p, pp);
            else if (!strncmp(p, "${name}", 7)) {
                p += 7;
                s += role.edrole->text();
            }
            else if (!strncmp(p, "${inverse_name}", 15)) {
                p += 15;

                if (! RelationData::uni_directional(current_type))
                    s += ((&role == &a) ? b : a).edrole->text();
            }
            else if (!strncmp(p, "${value}", 8)) {
                QString i = role.edinit->text().stripWhiteSpace();

                if (!i.isEmpty()) {
                    if (need_equal(p, i, FALSE))
                        s += " = ";

                    s += role.edinit->text();
                }
                else if (need_equal(p, "None", FALSE))
                    s += " = None";
                else
                    s += "None";

                p += 8;
            }
            else if (!strncmp(p, "${self}", 7)) {
                p += 7;

                if (!role.classrelation_cb->isChecked())
                    s += role.python_self;
            }
            else if (!strncmp(p, "${stereotype}", 13)) {
                p += 13;
                s += GenerationSettings::python_relationattribute_stereotype(fromUnicode(edstereotype->currentText().stripWhiteSpace()));
            }
            else if (!strncmp(p, "${type}", 7)) {
                p += 7;
                s += get_python_name(cl);
            }
            else if (!strncmp(p, "${association}", 14)) {
                p += 14;
                s += type(edassociation->currentText().stripWhiteSpace(),
                          list, nodes);
            }
            else if (*p == '@')
                manage_alias(rl, p, s, role.kvtable);
            else
                s += *p++;
        }
    }
    }

    role.showpythondecl->setText(s);
}

void RelationDialog::python_update_a()
{
    python_update(a, rel->get_end_class(), rel->start);
}

void RelationDialog::python_update_b()
{
    if (! RelationData::uni_directional(current_type))
        python_update(b, rel->get_start_class(), rel->end);
}

void RelationDialog::python_default_a()
{
    if (a.python_undef)
        a.edpythondecl->setText(QString());
    else if (RelationData::isa_association(current_type))
        a.edpythondecl->setText(GenerationSettings::python_default_rel_decl(current_type,
                                a.multiplicity->currentText().stripWhiteSpace()));
    else {
        a.edpythondecl->setText("${type}");
        a.python_unmapped_decl_bt->setOn(FALSE);
    }

    python_update_a();
}

void RelationDialog::default_a_python_if_needed(UmlCode prev_type,
        QString prev_mult,
        QString new_mult)
{
    if (!a.edpythondecl->text().isEmpty()) {
        if (!RelationData::isa_association(prev_type) ||
            !RelationData::isa_association(current_type)) {
            if (prev_type != current_type)
                python_default_a();
        }
        else {
            QString n = GenerationSettings::python_default_rel_decl(current_type, new_mult);

            if (n != GenerationSettings::python_default_rel_decl(prev_type, prev_mult))
                a.edpythondecl->setText(n);
        }
    }
}

void RelationDialog::python_unmapped_a()
{
    a.edpythondecl->setText(QString());
    a.showpythondecl->setText(QString());

    if (!RelationData::isa_association(current_type))
        a.python_default_decl_bt->setOn(FALSE);
}

void RelationDialog::python_default_b()
{
    if (b.python_undef)
        b.edpythondecl->setText(QString());
    else
        b.edpythondecl->setText(GenerationSettings::python_default_rel_decl(UmlAssociation,
                                b.multiplicity->currentText().stripWhiteSpace()));

    python_update_b();
}

void RelationDialog::default_b_python_if_needed(UmlCode prev_type,
        QString prev_mult,
        QString new_mult)
{
    if (!b.edpythondecl->text().isEmpty()) {
        QString n = GenerationSettings::python_default_rel_decl(current_type, new_mult);

        if (n != GenerationSettings::python_default_rel_decl(prev_type, prev_mult))
            b.edpythondecl->setText(n);
    }
}

void RelationDialog::python_unmapped_b()
{
    b.edpythondecl->setText(QString());
    b.showpythondecl->setText(QString());
}

// Idl management

void RelationDialog::idl_update(RoleDialog & role, BrowserClass * cl, BrowserNode * rl)
{
    QString s;

    switch (current_type) {
    case UmlRealize:
    case UmlGeneralisation: {
        // do NOT write
        //	const char * p = role.edidldecl->text();
        // because the QString is immediatly destroyed !
        QString def = role.edidldecl->text();
        const char * p = def;

        while (*p) {
            if (!strncmp(p, "${type}", 7)) {
                if ((role.idl_truncatable_inheritance_cb != 0) &&
                    role.idl_truncatable_inheritance_cb->isChecked())
                    s += "truncatable ";

                s += get_idl_name(cl);
                p += 7;
            }
            else if (*p == '@')
                manage_alias(rl, p, s, role.kvtable);
            else
                s += *p++;
        }
    }
    break;

    case UmlDependency:
        s = "";
        break;

    default: {
        // do NOT write
        //	const char * p = role.edidldecl->text();
        // because the QString is immediatly destroyed !
        QString def = role.edidldecl->text();
        const char * p = def;
        const char * pp = 0;
        QString indent = "";

        while ((*p == ' ') || (*p == '\t'))
            indent += *p++;

        if (*p != '#')
            s = indent;

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
                manage_comment(role.comment->text(), p, pp, FALSE);
            else if (!strncmp(p, "${description}", 14))
                manage_description(role.comment->text(), p, pp);
            else if (!strncmp(p, "${readonly}", 11)) {
                p += 11;

                if (role.constrelation_cb->isChecked())
                    s += "readonly ";
            }
            else if (role.idl_in_union && !strncmp(p, "${case}", 7)) {
                p += 7;
                s += role.edcase->currentText().stripWhiteSpace();
            }
            else if (!strncmp(p, "${attribut}", 11)) {
                // old version
                p += 11;

                if (!role.idl_in_struct && !role.idl_in_valuetype)
                    s += "attribute ";
            }
            else if (!strncmp(p, "${attribute}", 12)) {
                p += 12;

                if (!role.idl_in_struct && !role.idl_in_valuetype)
                    s += "attribute ";
            }
            else if (role.idl_in_valuetype && !strncmp(p, "${visibility}", 13)) {
                p += 13;

                switch (role.uml_visibility.value()) {
                case UmlPublic:
                    // case UmlPackage: // [SciBoy] This is probably an error, I have uncommented for now. UmlPackage is a UmlCode value, not a UmlVisibility value.
                    s += "public ";

                default:
                    s += "private ";
                }
            }
            else if (!strncmp(p, "${type}", 7)) {
                p += 7;
                s += get_idl_name(cl);
            }
            else if (!strncmp(p, "${name}", 7)) {
                p += 7;
                s += role.edrole->text();
            }
            else if (!strncmp(p, "${inverse_name}", 15)) {
                p += 15;

                if (! RelationData::uni_directional(current_type))
                    s += ((&role == &a) ? b : a).edrole->text();
            }
            else if (!strncmp(p, "${stereotype}", 13)) {
                p += 13;
                s += GenerationSettings::idl_relationattribute_stereotype(fromUnicode(edstereotype->currentText().stripWhiteSpace()));
            }
            else if (!strncmp(p, "${multiplicity}", 15)) {
                p += 15;
                s += role.multiplicity->currentText().stripWhiteSpace();
            }
            else if (!strncmp(p, "${association}", 14)) {
                p += 14;
                s += GenerationSettings::idl_type(type(edassociation->currentText().stripWhiteSpace(),
                                                       list, nodes));
            }
            else if (*p == '\n') {
                s += *p++;

                if (*p && (*p != '#'))
                    s += indent;
            }
            else if (*p == '@')
                manage_alias(rl, p, s, role.kvtable);
            else
                s += *p++;
        }
    }
    }

    role.showidldecl->setText(s);
}

void RelationDialog::idl_update_a()
{
    idl_update(a, rel->get_end_class(), rel->start);
}

void RelationDialog::idl_update_b()
{
    if (! RelationData::uni_directional(current_type))
        idl_update(b, rel->get_start_class(), rel->end);
}

void RelationDialog::idl_default_a()
{
    if (a.idl_undef)
        a.edidldecl->setText(QString());
    else if (RelationData::isa_association(current_type)) {
        QString mult = a.multiplicity->currentText().stripWhiteSpace();

        a.edidldecl->setText((a.idl_in_union)
                             ? GenerationSettings::idl_default_union_rel_decl(mult)
                             : ((a.idl_in_valuetype)
                                ? GenerationSettings::idl_default_valuetype_rel_decl(mult)
                                : GenerationSettings::idl_default_rel_decl(mult)));
    }
    else {
        a.edidldecl->setText("${type}");
        a.idl_unmapped_decl_bt->setOn(FALSE);
    }

    idl_update_a();
}

void RelationDialog::default_a_idl_if_needed(UmlCode prev_type,
        QString prev_mult,
        QString new_mult)
{
    if (!a.edidldecl->text().isEmpty()) {
        if (!RelationData::isa_association(prev_type) ||
            !RelationData::isa_association(current_type)) {
            if (prev_type != current_type)
                idl_default_a();
        }
        else {
            QString o;
            QString n;

            if (a.idl_in_union) {
                o = GenerationSettings::idl_default_union_rel_decl(prev_mult);
                n = GenerationSettings::idl_default_union_rel_decl(new_mult);
            }
            else if (a.idl_in_valuetype) {
                o = GenerationSettings::idl_default_valuetype_rel_decl(prev_mult);
                n = GenerationSettings::idl_default_valuetype_rel_decl(new_mult);
            }
            else {
                o = GenerationSettings::idl_default_rel_decl(prev_mult);
                n = GenerationSettings::idl_default_rel_decl(new_mult);
            }

            if (n != o)
                a.edidldecl->setText(n);
        }
    }
}

void RelationDialog::idl_unmapped_a()
{
    a.edidldecl->setText(QString());
    a.showidldecl->setText(QString());

    if (!RelationData::isa_association(current_type))
        a.idl_default_decl_bt->setOn(FALSE);
}

void RelationDialog::idl_default_b()
{
    if (b.idl_undef)
        b.edidldecl->setText(QString());
    else {
        QString mult = b.multiplicity->currentText().stripWhiteSpace();

        b.edidldecl->setText((b.idl_in_union)
                             ? GenerationSettings::idl_default_union_rel_decl(mult)
                             : ((b.idl_in_valuetype)
                                ? GenerationSettings::idl_default_valuetype_rel_decl(mult)
                                : GenerationSettings::idl_default_rel_decl(mult)));
    }

    idl_update_b();
}

void RelationDialog::default_b_idl_if_needed(UmlCode,
        QString prev_mult,
        QString new_mult)
{
    if (!b.edidldecl->text().isEmpty()) {
        QString o;
        QString n;

        if (b.idl_in_union) {
            o = GenerationSettings::idl_default_union_rel_decl(prev_mult);
            n = GenerationSettings::idl_default_union_rel_decl(new_mult);
        }
        else if (b.idl_in_valuetype) {
            o = GenerationSettings::idl_default_valuetype_rel_decl(prev_mult);
            n = GenerationSettings::idl_default_valuetype_rel_decl(new_mult);
        }
        else {
            o = GenerationSettings::idl_default_rel_decl(prev_mult);
            n = GenerationSettings::idl_default_rel_decl(new_mult);
        }

        if (n != o)
            b.edidldecl->setText(n);
    }
}

void RelationDialog::idl_unmapped_b()
{
    b.edidldecl->setText(QString());
    b.showidldecl->setText(QString());
}

//

static void accept_role(RoleDialog & role, RoleData & rel,
                        bool assoc, RelationData * r)
{
    rel.uml_visibility = role.uml_visibility.value();

    if (!assoc) {
        rel.isa_class_relation = FALSE;
        rel.isa_volatile_relation = FALSE;
        rel.isa_const_relation = FALSE;
        rel.is_derived = FALSE;
        rel.is_derivedunion = FALSE;
        rel.is_ordered = FALSE;
        rel.is_unique = FALSE;
        rel.cpp_mutable = FALSE;
        rel.java_transient = FALSE;
    }
    else {
        rel.isa_class_relation = role.classrelation_cb->isChecked();
        rel.isa_volatile_relation = role.volatile_cb->isChecked();
        rel.isa_const_relation = role.constrelation_cb->isChecked();
        rel.is_derived = role.derived_cb->isChecked();
        rel.is_derivedunion = role.derivedunion_cb->isChecked();
        rel.is_ordered = role.ordered_cb->isChecked();
        rel.is_unique = role.unique_cb->isChecked();
        rel.cpp_mutable = !role.cpp_undef && role.mutable_cb->isChecked();
        rel.java_transient = !role.java_undef && role.transient_cb->isChecked();
    }

    rel.role = role.edrole->text().stripWhiteSpace();
    rel.multiplicity = role.multiplicity->currentText().stripWhiteSpace();
    rel.init_value = role.edinit->text();
    rel.comment = role.comment->text();
    rel.constraint = role.constraint->stripWhiteSpaceText();

    rel.cpp_visibility = role.cpp_visibility.value();

    if (role.idl_in_union) {
        int index;
        QString s = role.edcase->currentText().stripWhiteSpace();

        if (!s.isEmpty() && ((index = role.enum_names.findIndex(s)) != -1))
            r->set_idlcase(rel, ((BrowserAttribute *) role.enums.at(index)), "");
        else
            r->set_idlcase(rel, 0, s);
    }

    rel.cpp_decl = role.edcppdecl->text();
    rel.java_decl = role.edjavadecl->text();
    rel.java_annotation = role.javaannotation;
    rel.php_decl = role.edphpdecl->text();
    rel.python_decl = role.edpythondecl->text();
    rel.idl_decl = role.edidldecl->text();
}

void RelationDialog::accept()
{
    if (!check_edits(edits) ||
        !a.kvtable->check_unique() ||
        !b.kvtable->check_unique())
        return;

    QString ra = a.edrole->text().stripWhiteSpace();
    QString rb = b.edrole->text().stripWhiteSpace();
    bool aTextSame = ra == aOldText;
    bool bTextSame = rb == bOldText;
    if (!aTextSame && rel->wrong_role_a_name(ra))
        msg_critical(TR("Error"), ra + TR("\n\nillegal name or already used"));
    else if (!bTextSame && rel->wrong_role_b_name(rb))
        msg_critical(TR("Error"), rb + TR("\n\nillegal name or already used"));
    else {
        rel->name = edname->text().stripWhiteSpace();

        int index = 0;
        QString s = edtype->currentText();
        bool wasinherit = RelationData::isa_inherit(rel->type);

        while (RelTypes[index].lbl != s)
            index += 1;

        rel->type = RelTypes[index].type;

        bool isinherit = RelationData::isa_inherit(rel->type);
        bool newst = !visit && rel->set_stereotype(fromUnicode(edstereotype->currentText().stripWhiteSpace()));
        bool assoc = RelationData::isa_association(rel->get_type());

        if (!visit && assoc) {
            AType t;

            s = edassociation->currentText().stripWhiteSpace();

            if (!s.isEmpty()) {
                index = list.findIndex(s);

                if (index >= 0)
                    t.type = (BrowserClass *) nodes.at(index);
                else
                    t.explicit_type = s;
            }

            rel->set_association(t);
        }

        if (!visit) {
            rel->a.cpp_virtual_inheritance =
                ((!assoc) && a.cpp_virtual_inheritance_cb->isChecked());
            rel->a.idl_truncatable_inheritance =
                ((!assoc) &&
                 (a.idl_truncatable_inheritance_cb != 0) &&
                 a.idl_truncatable_inheritance_cb->isChecked());

            default_defs_if_needed(rel->type);
            accept_role(a, rel->a, assoc, rel);
            accept_role(b, rel->b, assoc, rel);
        }

        if (! a.ro)
            a.kvtable->updateNodeFromThis(rel->start);

        if (! b.ro) {
            if (rel->end)
                rel->end->package_modified();

            if (rel->check_end_visibility()) {
                rel->end->modified();
                b.kvtable->updateNodeFromThis(rel->end);
            }
        }

        if (onstereotype && (isinherit || wasinherit))
            ProfiledStereotypes::recompute(TRUE);
        else if (! visit) {
            ProfiledStereotypes::modified(rel->start, newst);

            if (rel->end)
                ProfiledStereotypes::modified(rel->end, newst);
        }

        rel->start->modified();

        if (! visit)
            rel->start->package_modified();

        if (!a.ro || !b.ro)
            rel->modified();

        Q3TabDialog::accept();
    }
}
