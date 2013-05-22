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





#include <qapplication.h>
#include <q3popupmenu.h>
#include <q3dragobject.h>
#include <q3header.h>
#include <qcursor.h>
//Added by qt3to4:
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMouseEvent>
#include <QKeyEvent>

#include "BrowserView.h"
#include "UmlWindow.h"
#include "BrowserPackage.h"
#include "UmlDrag.h"
#include "BrowserSearchDialog.h"
#include "Shortcut.h"
#include "myio.h"
#include "mu.h"
#include "translate.h"
#include "ToolCom.h"
#include "Tool.h"

BrowserPackage * BrowserView::project;
BrowserPackage * BrowserView::imported_project;
QDir BrowserView::dir;
QDir BrowserView::import_dir;
BrowserView * BrowserView::the;

BrowserView::BrowserView(QWidget * parent) : Q3ListView(parent)
{
    the = this;
    BrowserNode::setup_generatable_types();
    project = 0;			// no project yet
    mousePressed = FALSE;

    setSorting(-1);		// manual sorting
    setSelectionMode(Q3ListView::Single);
    addColumn(TR("browser          "));
    setTreeStepSize(18);
    //setSelectionMode(Extended);	// multi selection
    header()->setClickEnabled(TRUE);
    setAcceptDrops(TRUE);
    viewport()->setAcceptDrops(TRUE);
    setDragAutoScroll(TRUE);

    connect(this, SIGNAL(rightButtonPressed(Q3ListViewItem *, const QPoint &, int)),
            this, SLOT(rightPressed(Q3ListViewItem *)));

    connect(this, SIGNAL(doubleClicked(Q3ListViewItem *)),
            this, SLOT(doubleClick(Q3ListViewItem *)));

    connect(header(), SIGNAL(clicked(int)), this, SLOT(menu()));

    connect(this, SIGNAL(selectionChanged(Q3ListViewItem *)),
            this, SLOT(selected(Q3ListViewItem *)));
}

void BrowserView::remove_temporary_files()
{
    if (project) {
        QString filter;

        filter.sprintf("*_%d.*", user_id());

        const QFileInfoList l = dir.entryInfoList(filter);

        if (!l.empty()) {
            QListIterator<QFileInfo> it(l);
            QFileInfo fi;

            while (it.hasNext()) {
                fi = it.next();

                if (fi.extension(FALSE).lower() != "prj")
                    QFile::remove(fi.absFilePath());
            }
        }
    }
}

void BrowserView::send_marked(QList<BrowserNode *> list)
{
    emit marked_list(list);
}

BrowserView::~BrowserView()
{
    remove_temporary_files();
    set_user_id(-1);
}

void BrowserView::clear()
{
    remove_temporary_files();
    set_user_id(-1);

    Q3ListView::clear();
    project = 0;
}

void BrowserView::set_project(const QDir & di)
{
    dir = di;
    project = new BrowserPackage(dir.dirName(), the, PROJECT_ID);
    setRootIsDecorated(TRUE/*FALSE*/);
}

void BrowserView::set_imported_project(const QDir & di,
                                       BrowserPackage * p)
{
    import_dir = di;
    imported_project = p;
}

bool BrowserView::save_as(const QDir & new_dir)
{
    // copy USER temporary files
    QString filter;

    filter.sprintf("*_%d.?", user_id());

    QFileInfoList l = dir.entryInfoList(filter); //[lgfreitas] Since Mr. Pagès uses l again, removing const from l

    if (!l.empty()) {
        QListIterator<QFileInfo> it(l);
        QFileInfo fi;

        while (it.hasNext()) {
            fi = it.next();

            if (!copy_file(&fi, new_dir)) //[lgfreitas] entryInfoList does not return a pointer anymore, but copy_file expects it
                return FALSE;
        }
    }

    // copy non package non temporary files
    l = dir.entryInfoList("*.??*");

    if (!l.empty()) {
        QListIterator<QFileInfo> it(l);
        QFileInfo fi;

        while (it.hasNext()) {
            fi = it.next();

            if ((fi.extension(FALSE) != "prj") &&
                (fi.extension(FALSE) != "lock") &&
                !copy_file(&fi, new_dir))
                return FALSE;
        }
    }

    // delete old temporary files and rename the project
    remove_temporary_files();
    set_user_id(-1);
    get_project()->set_name(new_dir.dirName());
    dir = new_dir;

    // create new lock
    user_id();

    return TRUE;
}

void BrowserView::select(Q3ListViewItem * i)
{
    the->ensureItemVisible(i);
    the->setSelected(i, TRUE);
}

void BrowserView::deselect(Q3ListViewItem * i)
{
    the->setSelected(i, false);
}

void BrowserView::force_visible(Q3ListViewItem * i)
{
    the->ensureItemVisible(i);
}

BrowserNode * BrowserView::selected_item()
{
    return (BrowserNode *) the->selectedItem();
}

void BrowserView::rightPressed(Q3ListViewItem * item)
{
    if (item &&
        ((!((BrowserNode *) item)->deletedp()) ||
         (!((BrowserNode *) item->parent())->deletedp())) &&
        !BrowserNode::popupMenuActive()) {	// Qt bug
        BrowserNode::setPopupMenuActive(TRUE);
        ((BrowserNode *) item)->menu();
        BrowserNode::setPopupMenuActive(FALSE);
    }
}

void BrowserView::doubleClick(Q3ListViewItem * item)
{
    UmlWindow::abort_line_construction();

    if (item && !((BrowserNode *) item)->deletedp())
        ((BrowserNode *) item)->open(FALSE);
}

void BrowserView::contentsDragMoveEvent(QDragMoveEvent * e)
{
    if (!BrowserNode::edition_active()) {
        Q3ListViewItem * item = itemAt(contentsToViewport(e->pos()));

        if (item && !((BrowserNode *) item)->deletedp())
            ((BrowserNode *) item)->DragMoveEvent(e);
    }
}

void BrowserView::contentsDropEvent(QDropEvent * e)
{
    if (!BrowserNode::edition_active()) {
        Q3ListViewItem * item = itemAt(contentsToViewport(e->pos()));

        if (item && !((BrowserNode *) item)->deletedp())
            ((BrowserNode *) item)->DropEvent(e);
    }
}

void BrowserView::contentsMousePressEvent(QMouseEvent * e)
{
    UmlWindow::abort_line_construction();

    QPoint p(contentsToViewport(e->pos()));
    Q3ListViewItem * i = itemAt(p);

    if (i != 0) {
        // to force update of comment else nothing done
        // when the click is made on the already selected item
        // (but the comment is one of an other element selected
        // in a diagram)
        selected(i);

        if (e->button() == ::Qt::LeftButton) {
            if (!((BrowserNode *) i)->deletedp() && (i != project))
            {
                if (e->state() & ::Qt::ControlModifier)
                {
                    ((BrowserNode *) i)->toggle_mark();
                }
                else if (e->pos().x() > header()->cellPos(header()->mapToActual(0)) +
                         treeStepSize() * (i->depth() + (rootIsDecorated() ? 1 : 0)) + itemMargin()/* ||
											      p.x() < header()->cellPos(header()->mapToActual(0))*/) {
                    // if the user clicked into the root decoration of the item, don't try to start a drag!
                    presspos = e->pos();
                    mousePressed = TRUE;
                }
            }
        }
    }

    Q3ListView::contentsMousePressEvent(e);
}

void BrowserView::contentsMouseMoveEvent(QMouseEvent * e)
{
    if (mousePressed &&
        ((presspos - e->pos()).manhattanLength() > QApplication::startDragDistance())) {
        mousePressed = FALSE;

        Q3ListViewItem * item = itemAt(contentsToViewport(presspos));

        if (item) {
            UmlDrag * di = new UmlDrag((BrowserNode *) item, this);

            di->setPixmap(*(item->pixmap(0)), QPoint(8, 8));
            di->dragMove();
        }
    }
}

void BrowserView::contentsMouseReleaseEvent(QMouseEvent *)
{
    mousePressed = FALSE;
}

void BrowserView::OnGenerateCpp()
{
    QList<BrowserNode*> toGenerate = static_cast<BrowserNode*>(this->currentItem())->get_generation_list();
    for(BrowserNode* node: toGenerate)
    {
        if(!node)
            continue;

        bool preserve = preserve_bodies();


        ToolCom::run((verbose_generation())
                     ? ((preserve) ? "cpp_generator -v -p" : "cpp_generator -v")
                     : ((preserve) ? "cpp_generator -p" : "cpp_generator"),
                     node);
    }

}

void BrowserView::OnGenerateJava()
{
    QList<BrowserNode*> toGenerate = static_cast<BrowserNode*>(this->currentItem())->get_generation_list();
    for(BrowserNode* node: toGenerate)
    {
        if(!node)
            continue;

        bool preserve = preserve_bodies();



        ToolCom::run((verbose_generation())
                     ? ((preserve) ? "java_generator -v -p" : "java_generator -v")
                     : ((preserve) ? "java_generator -p" : "java_generator"),
                     node);
    }
}

void BrowserView::OnGeneratePhp()
{
    QList<BrowserNode*> toGenerate = static_cast<BrowserNode*>(this->currentItem())->get_generation_list();
    for(BrowserNode* node: toGenerate)
    {
        if(!node)
            continue;

        bool preserve = preserve_bodies();


        ToolCom::run((verbose_generation())
                     ? ((preserve) ? "php_generator -v -p" : "php_generator -v")
                     : ((preserve) ? "php_generator -p" : "php_generator"),
                     node);
    }
}

void BrowserView::OnGeneratePython()
{
    QList<BrowserNode*> toGenerate = static_cast<BrowserNode*>(this->currentItem())->get_generation_list();
    for(BrowserNode* node: toGenerate)
    {
        if(!node)
            continue;

        bool preserve = preserve_bodies();

        ToolCom::run((verbose_generation())
                     ? ((preserve) ? "python_generator -v -p" : "python_generator -v")
                     : ((preserve) ? "python_generator -p" : "python_generator"),
                     node);
    }
}

void BrowserView::OnGenerateIdl()
{
    QList<BrowserNode*> toGenerate = static_cast<BrowserNode*>(this->currentItem())->get_generation_list();
    for(BrowserNode* node: toGenerate)
    {
        if(!node)
            continue;

        ToolCom::run((verbose_generation()) ? "idl_generator -v" : "idl_generator", node);
    }
}

void BrowserView::OnUnmarkItem(QString name, int type)
{
    foreach (BrowserNode *node, BrowserNode::marked_list)
        if(node->get_name() == name && node->get_type() == type)
            node->toggle_mark();
}

void BrowserView::OnUnmarkAll()
{
    BrowserNode::unmark_all();
}

void BrowserView::keyPressEvent(QKeyEvent * e)
{
    UmlWindow::abort_line_construction();

    QString s = Shortcut::shortcut(e->key(), e->state());

    if (!s.isEmpty()) {
        e->accept();

        if (s == "Save")
            UmlWindow::save_it();
        else if (s == "Save as")
            UmlWindow::saveas_it();
        else if (s == "Close")
            UmlWindow::do_close();
        else if (s == "Quit")
            UmlWindow::do_quit();
        else if (s == "Browser search")
            UmlWindow::browser_search_it();
        else if (s == "Open project")
            UmlWindow::load_it();
        else {
            BrowserNode * bn = (BrowserNode *) currentItem();

            if (bn != 0) {
                if (s == "Menu")
                    rightPressed(bn);
                else if (s == "Delete") {
                    QApplication::setOverrideCursor(::Qt::waitCursor);
                    bn->apply_shortcut("Delete");
                    QApplication::restoreOverrideCursor();
                }
                else if ((s != "Move left") && (s != "Move right") &&
                         (s != "Move up") && (s != "Move down"))
                    bn->apply_shortcut(s);
                else
                    Q3ListView::keyPressEvent(e);
            }
        }
    }
    else {
        // no shortcut
        Q3ListView::keyPressEvent(e);
    }
}

void BrowserView::menu()
{
    if (project != 0) {
        if (BrowserSearchDialog::get() == 0)
            (new BrowserSearchDialog())->show();
        else
            BrowserSearchDialog::get()->raise();
    }
}

void BrowserView::selected(Q3ListViewItem * b)
{
    UmlWindow::set_commented((BrowserNode *) b);
}
