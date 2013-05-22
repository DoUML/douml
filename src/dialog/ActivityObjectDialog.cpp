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

#include "ActivityObjectDialog.h"
#include "ActivityObjectData.h"
#include "BrowserActivityObject.h"
#include "BrowserView.h"
#include "BrowserClass.h"
#include "KeyValueTable.h"
#include "UmlWindow.h"
#include "UmlDesktop.h"
#include "DialogUtil.h"
#include "GenerationSettings.h"
#include "strutil.h"
#include "BodyDialog.h"
#include "ProfiledStereotypes.h"
#include "translate.h"

QSize ActivityObjectDialog::previous_size;

ActivityObjectDialog::ActivityObjectDialog(ActivityObjectData * d, const char * what,
        QStringList & st)
    : Q3TabDialog(0, 0, FALSE, Qt::WDestructiveClose), data(d)
{
    d->browser_node->edit_start();

    if (d->browser_node->is_writable()) {
        setOkButton(TR("OK"));
        setCancelButton(TR("Cancel"));
    }
    else {
        setOkButton(QString());
        setCancelButton(TR("Close"));
    }

    visit = !hasOkButton();
    setCaption(TR(QString(what) + " dialog"));

    Q3Grid * grid;
    Q3HBox * htab;
    QString s;

    // general tab

    grid = new Q3Grid(2, this);
    umltab = grid;
    grid->setMargin(5);
    grid->setSpacing(5);

    new QLabel(TR("name :"), grid);
    edname = new LineEdit(data->name(), grid);
    edname->setReadOnly(visit);

    QFont font = edname->font();

    if (! hasCodec())
        font.setFamily("Courier");

    font.setFixedPitch(TRUE);

    new QLabel(TR("stereotype : "), grid);
    edstereotype = new Q3ComboBox(!visit, grid);
    edstereotype->insertItem(toUnicode(data->stereotype));

    if (! visit) {
        edstereotype->insertStringList(st);
        edstereotype->insertStringList(ProfiledStereotypes::defaults(UmlActivityObject));
        edstereotype->setAutoCompletion(completion());
    }

    edstereotype->setCurrentItem(0);

    QSizePolicy sp = edstereotype->sizePolicy();

    sp.setHorData(QSizePolicy::Expanding);
    edstereotype->setSizePolicy(sp);

    connect(new SmallPushButton(TR("type :"), grid), SIGNAL(clicked()),
            this, SLOT(menu_type()));

    edtype = new Q3ComboBox(!visit, grid);
    edtype->insertItem(data->get_type().get_full_type());

    if (!visit) {
        BrowserClass::instances(nodes);
        nodes.full_names(list);

        edtype->insertStringList(GenerationSettings::basic_types());
        offset = edtype->count();
        edtype->insertStringList(list);
        edtype->setAutoCompletion(completion());

        // search for the view containing the activity
        view = d->browser_node;

        do {
            view = (BrowserNode *) view->parent();
        }
        while (view->get_type() != UmlActivity);

        view = (BrowserNode *) view->parent();
    }

    edtype->setCurrentItem(0);
    edtype->setSizePolicy(sp);

    new QLabel(TR("multiplicity : "), grid);
    htab = new Q3HBox(grid);
    edmultiplicity = new Q3ComboBox(!visit, htab);
    edmultiplicity->setSizePolicy(sp);
    edmultiplicity->insertItem(data->get_multiplicity());

    if (!visit) {
        edmultiplicity->insertItem("1");
        edmultiplicity->insertItem("0..1");
        edmultiplicity->insertItem("*");
        edmultiplicity->insertItem("1..*");
    }

    new QLabel(TR("   ordering : "), htab);
    edordering = new Q3ComboBox(FALSE, htab);

    UmlOrderingKind o = data->get_ordering();

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

    new QLabel(TR("in state : "), grid);
    htab = new Q3HBox(grid);
    edin_state = new LineEdit(data->in_state, htab);
    edin_state->setReadOnly(visit);

    new QLabel("  ", htab);
    is_control_cb = new QCheckBox(TR("is control"), htab);

    if (data->is_control)
        is_control_cb->setChecked(TRUE);

    is_control_cb->setDisabled(visit);

    Q3VBox * vtab = new Q3VBox(grid);
    new QLabel(TR("description :"), vtab);

    if (! visit) {
        connect(new SmallPushButton(TR("Editor"), vtab), SIGNAL(clicked()),
                this, SLOT(edit_description()));
    }

    comment = new MultiLineEdit(grid);
    comment->setReadOnly(visit);
    comment->setText(data->browser_node->get_comment());
    comment->setFont(font);

    addTab(grid, "Uml");

    // UML / OCL
    init_tab(ocltab, eduml_selection, data->uml_selection, "Ocl",
             SLOT(edit_uml_selection()), TRUE);

    // C++
    init_tab(cppTab, edcpp_selection, data->cpp_selection, "C++",
             SLOT(edit_cpp_selection()),
             GenerationSettings::cpp_get_default_defs());

    // Java
    init_tab(javatab, edjava_selection, data->java_selection, "Java",
             SLOT(edit_java_selection()),
             GenerationSettings::java_get_default_defs());

    // USER : list key - value

    grid = new Q3Grid(2, this);
    grid->setMargin(5);
    grid->setSpacing(5);

    kvtable = new KeyValuesTable(data->browser_node, grid, visit);
    addTab(grid, TR("Properties"));

    //

    connect(this, SIGNAL(currentChanged(QWidget *)),
            this, SLOT(change_tabs(QWidget *)));

    open_dialog(this);
}

void ActivityObjectDialog::polish()
{
    Q3TabDialog::polish();
    UmlDesktop::limitsize_center(this, previous_size, 0.8, 0.8);
}

ActivityObjectDialog::~ActivityObjectDialog()
{
    data->browser_node->edit_end();
    previous_size = size();

    foreach (BodyDialog *dialog, edits)
        dialog->close();
    edits.clear();

    close_dialog(this);
}

void ActivityObjectDialog::change_tabs(QWidget * w)
{
    if (! visit) {
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

void ActivityObjectDialog::init_tab(QWidget *& w, MultiLineEdit *& ed,
                                    const char * v, const char * lbl,
                                    const char * sl, bool enabled)
{
    bool visit = !hasOkButton();
    Q3Grid * grid = new Q3Grid(2, this);

    w = grid;
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

void ActivityObjectDialog::menu_type()
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
                QStringList::Iterator iter = list.begin();
                QStringList::Iterator iter_end = list.end();

                index = 0;

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

void ActivityObjectDialog::edit_description()
{
    edit(comment->text(), edname->text().stripWhiteSpace() + "_description",
         data, TxtEdit, this, (post_edit) post_edit_description, edits);
}

void ActivityObjectDialog::post_edit_description(ActivityObjectDialog * d, QString s)
{
    d->comment->setText(s);
}

#define DEF_EDIT(lang, Lang) \
void ActivityObjectDialog::edit_##lang##_selection() {	\
  edit(ed##lang##_selection->text(), "object_"#lang"_selection", data, Lang,  \
       this, (post_edit) post_edit_##lang##_##selection, edits);	\
} \
  \
void ActivityObjectDialog::post_edit_##lang##_selection(ActivityObjectDialog * d, QString s) \
{ \
  d->ed##lang##_selection->setText(s); \
}

DEF_EDIT(uml, TxtEdit)
DEF_EDIT(cpp, CppEdit)
DEF_EDIT(java, JavaEdit)

void ActivityObjectDialog::accept()
{
    if (!check_edits(edits))
        return;

    BrowserNode * bn = data->browser_node;
    QString s = edname->text().stripWhiteSpace();

    bn->set_name(s);

    bool newst = data->set_stereotype(fromUnicode(edstereotype->currentText().stripWhiteSpace()));
    AType t;

    s = edtype->currentText().stripWhiteSpace();

    if (!s.isEmpty()) {
        int index = list.findIndex(s);

        if (index >= 0)
            t.type = (BrowserClass *) nodes.at(index);
        else
            t.explicit_type = s;
    }

    data->set_type(t);
    data->multiplicity =
        edmultiplicity->currentText().stripWhiteSpace();
    data->ordering = ordering(edordering->currentText());
    data->is_control = is_control_cb->isChecked();
    data->in_state = edin_state->text().stripWhiteSpace();
    data->uml_selection = eduml_selection->text().stripWhiteSpace();
    data->cpp_selection = edcpp_selection->text().stripWhiteSpace();
    data->java_selection = edjava_selection->text().stripWhiteSpace();

    bn->set_comment(comment->text());
    UmlWindow::update_comment_if_needed(bn);

    kvtable->updateNodeFromThis(bn);

    ProfiledStereotypes::modified(bn, newst);

    bn->modified();
    bn->package_modified();
    data->modified();

    Q3TabDialog::accept();
}
