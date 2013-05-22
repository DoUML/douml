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





#include <q3grid.h>
#include <q3vbox.h>
#include <qlabel.h>
#include <q3combobox.h>
#include <q3buttongroup.h>
#include <qcheckbox.h>
#include <qradiobutton.h>
#include <qpushbutton.h>
#include <q3popupmenu.h>
#include <qcursor.h>

#include "ParameterDialog.h"
#include "ParameterData.h"
#include "BrowserView.h"
#include "BrowserClass.h"
#include "BrowserParameter.h"
#include "KeyValueTable.h"
#include "UmlWindow.h"
#include "UmlDesktop.h"
#include "DialogUtil.h"
#include "GenerationSettings.h"
#include "strutil.h"
#include "BodyDialog.h"
#include "GenerationSettings.h"
#include "ProfiledStereotypes.h"
#include "translate.h"

QSize ParameterDialog::previous_size;

ParameterDialog::ParameterDialog(ParameterData * pa)
    : Q3TabDialog(0, 0, FALSE, Qt::WDestructiveClose), param(pa)
{
    pa->browser_node->edit_start();

    if (pa->browser_node->is_writable()) {
        setOkButton(TR("OK"));
        setCancelButton(TR("Cancel"));
    }
    else {
        setOkButton(QString());
        setCancelButton(TR("Close"));
    }

    visit = !hasOkButton();
    setCaption(TR("Parameter dialog"));

    Q3Grid * grid;
    Q3HBox * htab;
    QString s;

    // general tab

    grid = new Q3Grid(2, this);
    umltab = grid;
    grid->setMargin(5);
    grid->setSpacing(5);

    new QLabel(TR("name :"), grid);
    edname = new LineEdit(pa->name(), grid);
    edname->setReadOnly(visit);

    QFont font = edname->font();

    if (! hasCodec())
        font.setFamily("Courier");

    font.setFixedPitch(TRUE);

    new QLabel(TR("stereotype : "), grid);
    edstereotype = new Q3ComboBox(!visit, grid);
    edstereotype->insertItem(toUnicode(pa->stereotype));

    if (! visit) {
        edstereotype->insertStringList(BrowserParameter::default_stereotypes());
        edstereotype->insertStringList(ProfiledStereotypes::defaults(UmlParameter));
        edstereotype->setAutoCompletion(completion());
    }

    edstereotype->setCurrentItem(0);

    QSizePolicy sp = edstereotype->sizePolicy();

    sp.setHorData(QSizePolicy::Expanding);
    edstereotype->setSizePolicy(sp);

    connect(new SmallPushButton(TR("type :"), grid), SIGNAL(clicked()),
            this, SLOT(menu_type()));

    edtype = new Q3ComboBox(!visit, grid);
    edtype->insertItem(pa->get_type().get_full_type());

    if (!visit) {
        BrowserClass::instances(nodes);
        nodes.full_names(list);

        edtype->insertStringList(GenerationSettings::basic_types());
        offset = edtype->count();
        edtype->insertStringList(list);
        edtype->setAutoCompletion(completion());
        view = pa->browser_node->container(UmlClass);
    }

    edtype->setCurrentItem(0);
    edtype->setSizePolicy(sp);

    new QLabel(TR("direction :"), grid);
    htab = new Q3HBox(grid);
    eddir = new Q3ComboBox(FALSE, htab);

    UmlParamDirection dir = pa->get_dir();

    eddir->insertItem(stringify(dir));

    if (! visit) {
        if (dir != UmlInOut)
            eddir->insertItem(stringify(UmlInOut));

        if (dir != UmlIn)
            eddir->insertItem(stringify(UmlIn));

        if (dir != UmlOut)
            eddir->insertItem(stringify(UmlOut));

        if (dir != UmlReturn)
            eddir->insertItem(stringify(UmlReturn));
    }

    new QLabel(TR("   multiplicity : "), htab);
    edmultiplicity = new Q3ComboBox(!visit, htab);
    edmultiplicity->setSizePolicy(sp);
    edmultiplicity->insertItem(pa->get_multiplicity());

    if (!visit) {
        edmultiplicity->insertItem("1");
        edmultiplicity->insertItem("0..1");
        edmultiplicity->insertItem("*");
        edmultiplicity->insertItem("1..*");
    }

    new QLabel(TR("   ordering : "), htab);
    edordering = new Q3ComboBox(FALSE, htab);

    UmlOrderingKind o = pa->get_ordering();

    edordering->insertItem(stringify(o));

    if (!visit) {
        if (o != UmlUnordered)
            edordering->insertItem(stringify(UmlUnordered));

        if (o != UmlOrdered)
            edordering->insertItem(stringify(UmlOrdered));

        if (o != UmlLifo)
            edordering->insertItem(stringify(UmlLifo));

        if (o != UmlFifo)
            edordering->insertItem(stringify(UmlFifo));
    }

    new QLabel(TR("   effect : "), htab);
    edeffect = new Q3ComboBox(FALSE, htab);

    UmlParamEffect e = pa->get_effect();

    edeffect->insertItem(stringify(e));

    if (!visit) {
        if (e != UmlNoEffect)
            edeffect->insertItem(stringify(UmlNoEffect));

        if (e != UmlCreate)
            edeffect->insertItem(stringify(UmlCreate));

        if (e != UmlRead)
            edeffect->insertItem(stringify(UmlRead));

        if (e != UmlUpdate)
            edeffect->insertItem(stringify(UmlUpdate));

        if (e != UmlDelete)
            edeffect->insertItem(stringify(UmlDelete));
    }

    new QLabel(TR("in state : "), grid);
    edin_state = new LineEdit(pa->in_state, grid);
    edin_state->setReadOnly(visit);

    new QLabel(TR("default value :"), grid);
    htab = new Q3HBox(grid);
    edinit = new LineEdit(pa->get_default_value(), htab);

    if (visit)
        edinit->setReadOnly(TRUE);
    else
        connect(new SmallPushButton(TR("Editor"), htab), SIGNAL(clicked()),
                this, SLOT(edit_init()));

    new QLabel(grid);
    htab = new Q3HBox(grid);
    Q3ButtonGroup * bg =
        new Q3ButtonGroup(2, Qt::Horizontal, QString(), htab);

    is_control_cb = new QCheckBox(TR("is_control"), bg);

    if (pa->is_control)
        is_control_cb->setChecked(TRUE);

    is_control_cb->setDisabled(visit);

    unique_cb = new QCheckBox(TR("unique"), bg);

    if (pa->unique)
        unique_cb->setChecked(TRUE);

    unique_cb->setDisabled(visit);

    bg =
        new Q3ButtonGroup(3, Qt::Horizontal, QString(), htab);
    bg->setExclusive(TRUE);

    standard_rb = new QRadioButton(TR("standard"), bg);
    exception_rb = new QRadioButton(TR("exception"), bg);
    stream_rb = new QRadioButton(TR("stream"), bg);

    if (pa->exception)
        exception_rb->setChecked(TRUE);
    else if (pa->stream)
        stream_rb->setChecked(TRUE);
    else
        standard_rb->setChecked(TRUE);

    Q3VBox * vtab = new Q3VBox(grid);
    new QLabel(TR("description :"), vtab);

    if (! visit) {
        connect(new SmallPushButton(TR("Editor"), vtab), SIGNAL(clicked()),
                this, SLOT(edit_description()));
    }

    comment = new MultiLineEdit(grid);
    comment->setReadOnly(visit);
    comment->setText(pa->browser_node->get_comment());
    comment->setFont(font);

    addTab(grid, "Uml");

    init_tab(ocltab, eduml_selection, pa->uml_selection, "Ocl",
             SLOT(edit_uml_selection()), TRUE);

    // C++
    init_tab(cppTab, edcpp_selection, pa->cpp_selection, "C++",
             SLOT(edit_cpp_selection()),
             GenerationSettings::cpp_get_default_defs());

    // Java
    init_tab(javatab, edjava_selection, pa->java_selection, "Java",
             SLOT(edit_java_selection()),
             GenerationSettings::java_get_default_defs());

    // USER : list key - value

    grid = new Q3Grid(2, this);
    grid->setMargin(5);
    grid->setSpacing(5);

    kvtable = new KeyValuesTable(pa->browser_node, grid, visit);
    addTab(grid, TR("Properties"));

    //

    connect(this, SIGNAL(currentChanged(QWidget *)),
            this, SLOT(change_tabs(QWidget *)));

    open_dialog(this);
}

ParameterDialog::~ParameterDialog()
{
    param->browser_node->edit_end();
    previous_size = size();

    foreach (BodyDialog *dialog, edits)
        dialog->close();
    edits.clear();

    close_dialog(this);
}

void ParameterDialog::change_tabs(QWidget * w)
{
    if (hasOkButton()) {
        if (w == umltab)
            edname->setFocus();
        else if (w == ocltab)
            eduml_selection->setFocus();
        else if (w == cppTab)
            edcpp_selection->setFocus();
        else if (w == javatab)
            edjava_selection->setFocus();
    }
}

void ParameterDialog::polish()
{
    Q3TabDialog::polish();
    UmlDesktop::limitsize_center(this, previous_size, 0.8, 0.8);
}

void ParameterDialog::init_tab(QWidget *& tab, MultiLineEdit *& ed, const char * v,
                               const char * lbl, const char * sl, bool enabled)
{
    bool visit = !hasOkButton();
    Q3Grid * grid = new Q3Grid(2, this);

    tab = grid;
    grid->setMargin(5);
    grid->setSpacing(5);

    Q3VBox * vtab = new Q3VBox(grid);

    new QLabel(TR("selection : "), vtab);

    if (! visit)
        connect(new SmallPushButton(TR("Editor"), vtab), SIGNAL(clicked()), this, sl);

    ed = new MultiLineEdit(grid);

    QFont font = ed->font();

    if (! hasCodec())
        font.setFamily("Courier");

    font.setFixedPitch(TRUE);
    ed->setFont(font);
    ed->setText(v);

    if (visit)
        ed->setReadOnly(TRUE);

    addTab(grid, lbl);

    if (! enabled)
        removePage(grid);
}

void ParameterDialog::menu_type()
{
    Q3PopupMenu m(0);

    m.insertItem(TR("Choose"), -1);
    m.insertSeparator();

    int index = list.findIndex(edtype->currentText().stripWhiteSpace());

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
                edtype->insertItem(s, index + offset);
            }
        }

        edtype->setCurrentItem(index + offset);
        break;

        default:
            break;
        }
    }
}

#define DEF_EDIT(lang, Lang) \
void ParameterDialog::edit_##lang##_selection() {	\
  edit(ed##lang##_selection->text(), "object_"#lang"_selection", param, Lang,  \
       this, (post_edit) post_edit_##lang##_##selection, edits);	\
} \
  \
void ParameterDialog::post_edit_##lang##_selection(ParameterDialog * d, QString s) \
{ \
  d->ed##lang##_selection->setText(s); \
}

DEF_EDIT(uml, TxtEdit)
DEF_EDIT(cpp, CppEdit)
DEF_EDIT(java, JavaEdit)

void ParameterDialog::accept()
{
    if (!check_edits(edits) || !kvtable->check_unique())
        return;

    BrowserNode * bn = param->browser_node;
    QString s;

    s = edname->text().stripWhiteSpace();

    if ((s != param->name()) &&
        ((BrowserNode *) bn->parent())->wrong_child_name(s, UmlParameter,
                bn->allow_spaces(),
                bn->allow_empty()))
        msg_critical(TR("Error"), s + TR("\n\nillegal name or already used"));
    else {
        // check consistency
        UmlParamDirection dir = direction(eddir->currentText());
        bool exception = exception_rb->isChecked();
        UmlParamEffect effect = ::effect(edeffect->currentText());
        QString err;

        if ((dir == UmlIn) && exception)
            err = TR("An input parameter cannot be an exception.\n");

        switch (effect) {
        case UmlDelete:
            if ((dir != UmlIn) && (dir != UmlInOut))
                err += TR("Only in and inout parameter may have a delete effect.");

            break;

        case UmlCreate:
            if ((dir != UmlOut) && (dir != UmlInOut) && (dir != UmlReturn))
                err += TR("Only out, inout and return parameter may have a create effect.");

            break;

        default:
            break;
        }

        if (!err.isEmpty())
            msg_critical(TR("Error"), err);
        else {
            bn->set_name(s);

            bool newst = param->set_stereotype(fromUnicode(edstereotype->currentText().stripWhiteSpace()));
            AType t;

            s = edtype->currentText().stripWhiteSpace();

            if (!s.isEmpty()) {
                int index = list.findIndex(s);

                if (index >= 0)
                    t.type = (BrowserClass *) nodes.at(index);
                else
                    t.explicit_type = s;
            }

            param->set_type(t);
            param->dir = dir;
            param->multiplicity =
                edmultiplicity->currentText().stripWhiteSpace();
            param->ordering = ordering(edordering->currentText());
            param->effect = effect;
            param->is_control = is_control_cb->isChecked();
            param->unique = unique_cb->isChecked();
            param->exception = exception;
            param->stream = stream_rb->isChecked();
            param->in_state = edin_state->text().stripWhiteSpace();
            param->default_value = edinit->text();
            param->uml_selection = eduml_selection->text().stripWhiteSpace();
            param->cpp_selection = edcpp_selection->text().stripWhiteSpace();
            param->java_selection = edjava_selection->text().stripWhiteSpace();

            bn->set_comment(comment->text());
            UmlWindow::update_comment_if_needed(bn);

            kvtable->updateNodeFromThis(bn);

            ProfiledStereotypes::modified(bn, newst);

            bn->modified();
            bn->package_modified();
            param->modified();

            Q3TabDialog::accept();
        }
    }
}

void ParameterDialog::edit_description()
{
    edit(comment->text(), edname->text().stripWhiteSpace() + "_description",
         param, TxtEdit, this, (post_edit) post_edit_description, edits);
}

void ParameterDialog::post_edit_description(ParameterDialog * d, QString s)
{
    d->comment->setText(s);
}

void ParameterDialog::edit_init()
{
    edit(edinit->text(), edname->text().stripWhiteSpace() + "_initialization",
         param, TxtEdit, this, (post_edit) post_edit_init, edits);
}

void ParameterDialog::post_edit_init(ParameterDialog * d, QString s)
{
    d->edinit->setText(s);
}
