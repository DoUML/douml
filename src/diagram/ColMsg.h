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

#ifndef COLMSG_H
#define COLMSG_H

#include <qstring.h>
#include <qlist.h>

#include "UmlEnum.h"

#include <QTextStream>

class OperationData;
class CodObjCanvas;
class CodMsgSupport;
class QTextStream;
class UmlCanvas;
class ToolCom;

class ColMsg;
//[lgfreitas] replacing qcollection by q3ptrcollection
#include <q3ptrcollection.h>
//[lgfreitas] q3ptrlist needed
#include <q3ptrlist.h>

class ColMsgList : public QList<ColMsg *>
{
public:
    void sort();
protected:
    static bool compareItems(ColMsg *a, ColMsg *b);
};


class ColMsg
{
    friend class CodAddMsgDialog;
    friend class ColMsgTable;
    friend class CodChangeMsgDialog;

protected:
    bool is_forward;
    unsigned absolute_rank;
    QString hierarchical_rank;
    const OperationData * operation;
    QString explicit_operation;
    CodMsgSupport * in;

    ColMsgList msgs;	// the sub messages from the current one

    ColMsg(bool forward, const OperationData * op, const QString & expl_op,
           const QString & hi_rank, CodMsgSupport * su);

    void update_ranks(unsigned & r, QString hr);
    void place_in_internal(ColMsgList & l);
    void place_in_its_support();
    void update_rank(unsigned & r);
    void delete_it_internal();

    static void get_all(const ColMsgList & l, ColMsgList & r);

public:
    void delete_it(bool rec, ColMsgList & top);

    ColMsgList & get_msgs() {
        return msgs;
    };
    const QString & get_hierarchical_rank() const {
        return hierarchical_rank;
    };
    unsigned get_rank() const {
        return absolute_rank;
    };
    QString next_hierarchical_rank() const;
    bool get_is_forward() {
        return is_forward;
    };
    const OperationData * get_operation() {
        return operation;
    };
    QString def(bool hierarchical, bool full, DrawingLanguage language, ShowContextMode context);

    static void update_ranks(ColMsgList & msgs);
    void place_in(ColMsgList & l);
    bool extract_it(ColMsgList & l);

    static void save(QTextStream & st, const ColMsgList & l, bool copy,
                     QString & warning, const QString & diag_name);
    static void read(char *& st, ColMsgList & l, UmlCanvas * canvas);

    static ColMsg * new_one(const OperationData * d, const QString & e,
                            bool f, const QString & hr, CodMsgSupport * in);
    static void get_all_in_all_out(ColMsgList & all_in, ColMsgList & all_out,
                                   const ColMsgList & msgs);
    static ColMsg * find(const QString & hi, ColMsgList & l);
    static ColMsg * find_rec(const QString & hi, ColMsgList & l);
    static unsigned last_rank(const ColMsgList & l);

    static bool lt(const char * h1, const char * h2);
    static bool gt(const char * h1, const char * h2);

    static void send(ToolCom * com, const ColMsgList & l);
};

#endif
