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





#include <qlabel.h>
#include <q3popupmenu.h>
#include <qcursor.h>
//Added by qt3to4:
#include <Q3PtrList>

#include "BrowserView.h"
#include "BrowserNode.h"
#include "SynchroWindow.h"

#define DICT_SIZE 101

BrowserView::BrowserView(QWidget * parent)
    : Q3ListView(parent), lbl(0), need_update(FALSE), cant_update(FALSE)
{
    nodes.resize(DICT_SIZE);

    setSorting(-1);		// manual sorting
    addColumn("browser          ");	// will be changed
    addColumn("Rev.");
    addColumn("Modified by");
    setColumnWidthMode(0, Maximum);
    setColumnWidthMode(1, Maximum);
    setColumnWidthMode(2, Maximum);
    setColumnAlignment(1, ::Qt::AlignHCenter);
    setColumnAlignment(2, ::Qt::AlignHCenter);
    setTreeStepSize(18);

    connect(this, SIGNAL(selectionChanged(Q3ListViewItem *)),
            this, SLOT(select(Q3ListViewItem *)));
}

BrowserView::~BrowserView()
{
}

void BrowserView::close()
{
    if (firstChild() != 0) {
        dir.rmdir("all.lock");
        clear();
    }
}

BrowserNode * BrowserView::get_project() const
{
    return (BrowserNode *) firstChild();
}

void BrowserView::set_project(QDir di)
{
    dir = di;
    new BrowserNode(this, dir.dirName() + ".prj");
    setRootIsDecorated(TRUE/*FALSE*/);

    di.cdUp();
    setColumnText(0, di.path());
}

void BrowserView::select(Q3ListViewItem * b)
{
    static bool ongoing = FALSE;

    if (!ongoing) {
        ongoing = TRUE;

        const QList<BrowserView *> & l = SynchroWindow::get_browsers();
        Q3PtrListIterator<BrowserView> it(l);
        QString fn = ((BrowserNode *) b)->file_name();

        for (; it.current(); ++it) {
            it.current()->clearSelection();

            BrowserNode * bn = it.current()->nodes.find(fn);

            if (bn != 0) {
                it.current()->ensureItemVisible(bn);
                it.current()->Q3ListView::setSelected(bn, TRUE);
            }
        }

        ongoing = FALSE;
    }
}

//

void BrowserView::add_node(BrowserNode * bn)
{
    nodes.insert(bn->file_name(), bn);
}

BrowserNode * BrowserView::get_node(QString f) const
{
    return nodes.find(f);
}

//

struct Use {
    int rev_min;
    int rev_max;
    int count;

    Use(int rev) : rev_min(rev), rev_max(rev), count(1) {}
};

void BrowserView::update(const QList<BrowserView *> & lv)
{
    Q3Dict<Use> all_nodes(DICT_SIZE);
    Q3PtrListIterator<BrowserView> itv(lv);

    all_nodes.setAutoDelete(TRUE);

    // look at the revision in each view

    for (; itv.current(); ++itv) {
        Q3DictIterator<BrowserNode> itd(itv.current()->nodes);

        for (; itd.current(); ++itd) {
            BrowserNode * bn = itd.current();
            int rev = bn->get_rev();
            Use * use = all_nodes.find(itd.currentKey());

            if (use == 0)
                all_nodes.insert(itd.currentKey(), new Use(rev));
            else {
                if (rev < use->rev_min)
                    use->rev_min = rev;

                if (rev > use->rev_max)
                    use->rev_max = rev;

                use->count += 1;
            }
        }
    }

    // first solve step
    // only the package existing in all the view are solved

    int nviews = lv.count();
    QStringList deleted_or_new;

    Q3DictIterator<Use> itu(all_nodes);

    for (; itu.current(); ++itu) {
        QString who = itu.currentKey();
        Use * use = itu.current();

        if (use->count == nviews) {
            // exist in all views : solve the state
            if (use->rev_min == use->rev_max) {
                // up to date in all views
                for (itv.toFirst(); itv.current(); ++itv)
                    itv.current()->nodes.find(who)->set_state(UpToDate);
            }
            else {
                int max = use->rev_max;

                for (itv.toFirst(); itv.current(); ++itv) {
                    BrowserNode * pack = itv.current()->nodes.find(who);

                    pack->set_state((pack->get_rev() == max) ? Young : Old);
                }
            }
        }
        else {
            // deleted or new, mark it unknown for this step
            deleted_or_new.append(who);

            for (itv.toFirst(); itv.current(); ++itv) {
                BrowserNode * pack = itv.current()->nodes.find(who);

                if (pack != 0)
                    pack->set_state(Unknown);
            }
        }
    }

    all_nodes.clear();

    // solve packages marked unknown
    // a package is deleted if its parent is never 'Young'

    QStringList::Iterator it;

    for (it = deleted_or_new.begin(); it != deleted_or_new.end(); ++it) {
        QString who = *it;
        QList<BrowserNode *> images;
        bool young = FALSE;

        // set the state in each view without looking at the others
        for (itv.toFirst(); itv.current(); ++itv) {
            BrowserNode * pack = itv.current()->nodes.find(who);

            if (pack != 0) {
                images.append(pack);

                if (pack->solve())
                    young = TRUE;
            }
        }

        // set the final state if young, else all already marked deleted
        if (young) {
            BrowserNode * pack;

            for (pack = images.first(); pack != 0; pack = images.next())
                pack->set_state(Young);
        }
    }

    // force update on views
    for (itv.toFirst(); itv.current(); ++itv)
        itv.current()->update_it();
}

void BrowserView::update_it()
{
    if (lbl == 0)
        lbl = new QLabel(parentWidget());

    int nold = 0;
    int ndel = 0;

    Q3DictIterator<BrowserNode> itd(nodes);

    for (; itd.current(); ++itd) {
        BrowserNode * bn = itd.current();

        switch (bn->get_state()) {
        case Old:
            nold += 1;
            break;

        case Deleted:
            ndel += 1;
            break;

        default:
            break;
        }
    }

    QString s;

    if (nold != 0) {
        need_update = TRUE;

        s = QString::number(nold) + " need update";

        if (ndel != 0)
            s += ", " + QString::number(ndel) + " deleted";

        if (cant_update)
            s += ", RO !";

        lbl->setText(s);
    }
    else
        lbl->setText("Up to date");

    lbl->show();

    triggerUpdate();
}

//

void BrowserView::synchronize()
{
    BrowserNode::synchronize(dir, nodes);
}
