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





#include <stdlib.h>

#include "ColMsg.h"
#include "OperationData.h"
#include "CodMsgSupport.h"
#include "CodObjCanvas.h"
#include "UmlCanvas.h"
#include "BrowserOperation.h"
#include "BrowserClass.h"
#include "CodSelfLinkCanvas.h"
#include "CodDirsCanvas.h"
#include "myio.h"
#include "ToolCom.h"
//Added by qt3to4:
#include <QTextStream>
#include <Q3PtrCollection>

void ColMsgList::sort()
{
    qSort(begin(), end(), compareItems);
}

bool ColMsgList::compareItems(ColMsg *a,
                              ColMsg *b)
{
    unsigned r1 = ((ColMsg *) a)->get_rank();
    unsigned r2 = ((ColMsg *) b)->get_rank();

    if (r1 == r2)
        // in case sort compare an item with itself
        return 0;

    return r1 > r2;
}

//

ColMsg::ColMsg(bool forward, const OperationData * op,
               const QString & expl_op, const QString & hr, CodMsgSupport * su)
    : is_forward(forward), hierarchical_rank(hr),
      operation(op), explicit_operation(expl_op), in(su)
{
}

void ColMsg::delete_it(bool rec, ColMsgList & top)
{
    // extract it from all msg
    extract_it(top);

    if (rec)
        delete_it_internal();
    else {
        // moves sub messages at toplevel
        top << msgs;
        msgs.clear();

        // remove it from (self)link
        in->remove_it(this);

        delete this;
    }
}

void ColMsg::delete_it_internal()
{
    // delete sub msgs
    foreach (ColMsg *msg, msgs)
        msg->delete_it_internal();
    msgs.clear();

    // remove it from (self)link
    in->remove_it(this);

    delete this;
}

// warning, some items may have not already updated ranks
// search without considering ranks

bool ColMsg::extract_it(ColMsgList & top)
{
    int index = 0;
    foreach (ColMsg *msg, top) {
        if (msg == this) {
            top.removeAt(index);
            return TRUE;
        }

        if (extract_it(msg->msgs))
            return TRUE;
        ++index;
    }

    return FALSE;
}

QString ColMsg::def(bool hierarchical, bool full,
                    DrawingLanguage language,
                    ShowContextMode context)
{
    QString op;

    if (operation == 0)
        op = explicit_operation;
    else if (!full)
        op = operation->definition(FALSE, FALSE);
    else {
        op = operation->definition(TRUE, language, TRUE, TRUE, context);

        if (op.isEmpty())
            return op;

        if (operation->get_browser_node()->deletedp())
            op.insert(0, "<deleted> ");
    }

    return (op.isEmpty())
           ? op
           : (((hierarchical) ? hierarchical_rank : QString::number(absolute_rank))
              + " " + op);
}

QString ColMsg::next_hierarchical_rank() const
{
    int index = hierarchical_rank.findRev('.');

    return (index == -1)
           ? QString::number(hierarchical_rank.toUInt() + 1)
           : hierarchical_rank.left(index + 1) +
           QString::number(hierarchical_rank.mid(index + 1).toUInt() + 1);
}

void ColMsg::update_ranks(ColMsgList & msgs)
{
    unsigned rank = 0;
    unsigned hi_rank = 1;
    foreach (ColMsg *msg, msgs) {
        msg->update_ranks(rank, QString::number(hi_rank));	// rank is updated
        ++hi_rank;
    }
}

void ColMsg::update_ranks(unsigned & r, QString hr)
{
    absolute_rank = ++r;
    hierarchical_rank = hr;

    hr += '.';

    unsigned hi_rank = 1;
    foreach (ColMsg *msg, msgs) {
        msg->update_ranks(r, hr + QString::number(hi_rank));
        ++hi_rank;
    }
}

void ColMsg::update_rank(unsigned & r)
{
    absolute_rank = ++r;

    foreach (ColMsg *msg, msgs)
        msg->update_rank(r);
}

ColMsg * ColMsg::new_one(const OperationData * d, const QString & e, bool f,
                         const QString & hr, CodMsgSupport * in)
{
    ColMsg * result = new ColMsg(f, d, e, hr, in);

    result->place_in_its_support();

    return result;
}

void ColMsg::place_in_its_support()
{
    const char * hr = hierarchical_rank;

    unsigned index = 0;
    foreach (ColMsg *msg, in->get_msgs()) {
        if (!gt(hr, msg->hierarchical_rank)) {
            in->get_msgs().insert(index, this);
            return;
        }
        ++index;
    }

    in->get_msgs().append(this);
}

void ColMsg::place_in(ColMsgList & l)
{
    // only the hierarchical_rank is set, not the absolute_rank
    place_in_internal(l);

    // lazy implementation where all the objects's rank is updated
    unsigned r = 0;

    foreach (ColMsg *msg, l)
        msg->update_rank(r);
}

void ColMsg::place_in_internal(ColMsgList & l)
{
    ColMsgList * pl = &l;
    int ndots = hierarchical_rank.count('.'); //[lgfreitas] contains now returns a bool

    for (;;) {
        QListIterator<ColMsg *> it(*pl);
        ColMsg * m = 0;
        unsigned index = pl->count();

        it.toBack();
        while (it.hasPrevious()) {
            m = it.previous();
            if (gt(hierarchical_rank, m->hierarchical_rank))
                break;
            --index;
        }

        if ((m == 0) || // must be the first one
            (m->hierarchical_rank.contains('.') == ndots)) {	// must be placed after m
            pl->insert(index, this);
            return;
        }

        // must be placed in a sub tree of m
        pl = &m->msgs;
    }
}

void ColMsg::get_all_in_all_out(ColMsgList & all_in, ColMsgList & all_out,
                                const ColMsgList & msgs)
{
    foreach (ColMsg *msg, msgs) {
        ColMsgList & l = (msg->is_forward) ? all_in : all_out;
        const QString & hi = msg->hierarchical_rank;

        unsigned index = 0;
        foreach (ColMsg *nestedMsg, l) {
            if (lt(hi, nestedMsg->hierarchical_rank))
                break;
            ++index;
        }

        l.insert(index, msg);
    }
}

ColMsg * ColMsg::find(const QString & hi, ColMsgList & l)
{
    foreach (ColMsg *msg, l)
        if (msg->hierarchical_rank == hi)
            return msg;

    return 0;
}

// warning hr may be invalid, and tree may be not sorted

ColMsg * ColMsg::find_rec(const QString & hr, ColMsgList & top)
{
    int index = 0;

    foreach (ColMsg *msg, top) {
        if (msg->hierarchical_rank == hr)
            return msg;

        ColMsg * result = find_rec(hr, msg->msgs);

        if (result != 0)
            return result;
        ++index;
    }

    return 0;
}

unsigned ColMsg::last_rank(const ColMsgList & l)
{
    unsigned result = 0;
    const ColMsgList * pl = &l;

    while (!pl->isEmpty()) {
        ColMsg * m = pl->last();

        result = m->absolute_rank;
        pl = &m->msgs;
    }

    return result;
}

bool ColMsg::lt(const char * h1, const char * h2)
{
    for (;;) {
        int i1 = atoi(h1);
        int i2 = atoi(h2);

        if (i1 != i2)
            return (i1 < i2);

        h1 = strchr(h1, '.');
        h2 = strchr(h2, '.');

        if (h1 == 0)
            return (h2 != 0);

        if (h2 == 0)
            return FALSE;

        h1 = h1 + 1;
        h2 = h2 + 1;
    }
}

bool ColMsg::gt(const char * h1, const char * h2)
{
    for (;;) {
        int i1 = atoi(h1);
        int i2 = atoi(h2);

        if (i1 != i2)
            return (i1 > i2);

        h1 = strchr(h1, '.');
        h2 = strchr(h2, '.');

        if (h1 == 0)
            return FALSE;

        if (h2 == 0)
            return TRUE;

        h1 = h1 + 1;
        h2 = h2 + 1;
    }
}


//

void ColMsg::save(QTextStream & st, const ColMsgList & l, bool copy,
                  QString & warning, const QString & diag_name)
{
    nl_indent(st);

    bool first_one = TRUE;
    foreach (ColMsg *msg, l) {
        if (copy && !msg->in->copyable())
            continue;

        if (first_one) {
            first_one = FALSE;
            st << "msgs";
            indent(+1);
        }

        nl_indent(st);

        if (msg->operation != 0) {
            if (msg->operation->deletedp()) {
                BrowserOperation * br_op =
                    (BrowserOperation *) msg->operation->get_browser_node();

                warning += QString("<b>") + diag_name + "</b> reference the <b>" +
                           ((BrowserClass *) br_op->parent())->full_name() +
                           "</b> deleted operation <b>" + msg->operation->definition(TRUE, FALSE) +
                           "</b><br><br>\n";

                if (warning[0] == '!') {
                    st << "msg ";
                    msg->operation->save(st, TRUE, warning);
                }
                else {
                    st << "explicitmsg ";
                    save_string(br_op->get_name(), st);
                }
            }
            else {
                st << "msg ";
                msg->operation->save(st, TRUE, warning);
            }
        }
        else {
            st << "explicitmsg ";
            save_string(msg->explicit_operation, st);
        }

        indent(+1);
        nl_indent(st);
        st << ((msg->is_forward) ? "forward" : "backward")
           << " ranks " << msg->absolute_rank << " ";
        save_string(msg->hierarchical_rank, st);

        st << " ";
        msg->in->save(st, TRUE, warning);

        save(st, msg->msgs, copy, warning, diag_name);
        indent(-1);
    }

    if (first_one)
        st << "no_msg";
    else {
        indent(-1);
        nl_indent(st);
        st << "msgsend";
    }
}

void ColMsg::read(char *& st, ColMsgList & l, UmlCanvas * canvas)
{
    char * k = read_keyword(st);

    do {
        const OperationData * method = 0;
        QString expl;

        if (!strcmp(k, "msg")) {
            if ((method = OperationData::read_ref(st)) == 0)
                expl = "<deleted operation>";
        }
        else if (!strcmp(k, "explicitmsg"))
            expl = read_string(st);
        else
            return;

        bool forward = FALSE;

        k = read_keyword(st);

        if (!strcmp(k, "forward"))
            forward = TRUE;
        else if (!strcmp(k, "backward"))
            forward = FALSE;
        else
            wrong_keyword(k, "forward/backward");

        read_keyword(st, "ranks");
        unsigned abs = read_unsigned(st);
        QString hi = read_string(st);
        CodMsgSupport * in;

        k = read_keyword(st);

        if ((in = CodSelfLinkCanvas::read(st, canvas, k)) == 0)
            in = CodDirsCanvas::read(st, canvas, k);

        ColMsg * cm = new ColMsg(forward, method, expl, hi, in);

        cm->absolute_rank = abs;
        l.append(cm);
        in->get_msgs().append(cm);

        k = read_keyword(st);

        if (!strcmp(k, "msgs"))
            read(st, cm->msgs, canvas);
        else if (strcmp(k, "no_msg"))
            wrong_keyword(k, "msgs/no_msg");

        k = read_keyword(st);
    }
    while (strcmp(k, "msgsend"));
}

// for plug out

void ColMsg::get_all(const ColMsgList & l, ColMsgList & r)
{
    foreach (ColMsg *msg, l) {
        r.append(msg);
        get_all(msg->msgs, r);
    }
}

void ColMsg::send(ToolCom * com, const ColMsgList & l)
{
    ColMsgList lm;

    get_all(l, lm);
    lm.sort();

    com->write_unsigned(lm.count());

    foreach (ColMsg *msg, lm) {
        if (msg->operation == 0) {
            com->write_id(0);
            com->write_string(msg->explicit_operation);
        }
        else if (msg->operation->deletedp()) {
            com->write_id(0);
            com->write_string(msg->operation->get_browser_node()->get_name());
        }
        else
            msg->operation->get_browser_node()->write_id(com);

        CodObjCanvas * from;
        CodObjCanvas * to;

        msg->in->get_from_to(from, to, msg->is_forward);
        com->write_unsigned((unsigned) from->get_ident());
        com->write_unsigned((unsigned) to->get_ident());

        com->write_unsigned(msg->absolute_rank);
        com->write_string((const char *) msg->hierarchical_rank);
    }
}
