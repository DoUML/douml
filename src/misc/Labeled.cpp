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





#include <q3intdict.h>
#include <qdatetime.h>
#include <qmessagebox.h>
#include <q3ptrlist.h>
#include <q3ptrlist.h>

#include "mu.h"
#include "Labeled.h"
#include "myio.h"

// to manage renumber case : old diagram (format < 9) and
// import project

static bool NeedRenumber;
static bool ImportLib;

void set_in_import(bool y, bool as_lib)
{
    NeedRenumber = y;
    ImportLib = as_lib;
}

bool in_import()
{
    return NeedRenumber;
}

bool in_lib_import()
{
    return ImportLib;
}

//

int place(IdDict<void> & d, int id, void * x)
{
    const int uid = user_id();

    if (id != -1) {
        // id is relevant
        bool check = TRUE;

        if (id == 0) {
            // compute a new id
            if ((d.idmax < FIRST_ID) && (uid != 0))
                d.idmax = FIRST_ID - 128;

            id = (d.idmax += 128) | uid;
        }
        else if (d.old_diagram) {
            // place id unchanged among the old ones
            d.dict[1].replace(id, x);

            if ((d.dict[1].count() / 2) >= d.dict[1].size())
                d.dict[1].resize(d.dict[1].size() * 2 - 1);

            // id doesn't contains a user_id field
            // create new one for the current user_id
            if ((d.idmax < FIRST_ID) && (uid != 0))
                d.idmax = FIRST_ID - 128;

            id = (d.idmax += 128) | uid;
        }
        else if (NeedRenumber) {
            // place id unchanged among the old ones
            d.dict[1].replace(id, x);

            if ((d.dict[1].count() / 2) >= d.dict[1].size())
                d.dict[1].resize(d.dict[1].size() * 2 - 1);

            int nid;

            if (ImportLib && ((nid = (int)((long) d.dictlib[id])) != 0)) {
                // an id was already attributed for it
                id = nid;

                if ((d.idmax < FIRST_ID) && (uid != 0))
                    d.idmax = FIRST_ID - 128;

                if ((((unsigned)(id & ~127)) > ((unsigned) d.idmax)) &&
                    ((id & 127) == uid))
                    d.idmax = id & ~127;
            }
            else if ((d.dict[0][id] != 0) || ((id & 127) != uid)) {
                // already used or for an other user, change id to a new one
                if ((id & 127) < 2) {
                    // import a plug out in a plug out !!!!!
                    // user_id part is unchanged
                    id = (id & 127) + FIRST_ID;

                    while (d.dict[0][id] != 0)
                        id += 128;

                    check = FALSE;
                }
                else {
                    // create new id for the current user_id
                    if ((d.idmax < FIRST_ID) && (uid != 0))
                        d.idmax = FIRST_ID - 128;

                    id = (d.idmax += 128) | uid;
                }
            }
            else
                // id unchanged for the current user
                check = FALSE;

            if ((((unsigned)(id & ~127)) > ((unsigned) d.idmax)) &&
                ((id & 127) == uid))
                d.idmax = id & ~127;
        }
        else {
            // no renum, id unchanged
            check = FALSE;

            if ((((unsigned)(id & ~127)) > ((unsigned) d.idmax)) &&
                ((id & 127) == uid))
                d.idmax = id & ~127;
        }

        if (check) {
            // useless except bug or project broken by user probably on merge
            while (d.dict[0][id] != 0)
                id = (d.idmax += 128) | uid;
        }

        d.dict[0].insert(id, x);

        if ((d.dict[0].count() / 2) >= d.dict[0].size())
            d.dict[0].resize(d.dict[0].size() * 2 - 1);
    }

    return id;
}

int new_place(IdDict<void> & d, int user_id, void * x)
{
    if (d.idmax == FIRST_ID)
        d.idmax = FIRST_BASE_ID;
    else
        d.idmax += 128;

    while (d.dict[0][d.idmax | user_id] != 0)
        // not possible except bug
        d.idmax += 128;

    d.dict[0].insert(d.idmax | user_id, x);
    return d.idmax | user_id;
}

// to change id when two elements use the same

struct NeedChange {
    IdDict<void> & dict;
    int & ident;
    void * elt;

    NeedChange(IdDict<void> & d, int & id, void * e) : dict(d), ident(id), elt(e) {}
};

static QList<NeedChange *> MustBeRenumered;

void will_change_id(IdDict<void> & d, int & id, void * x)
{
    MustBeRenumered.append(new NeedChange(d, id, x));
}

void do_change_shared_ids()
{
    int user = user_id();

    while (!MustBeRenumered.isEmpty()) {
        NeedChange * x = MustBeRenumered.takeFirst();

        x->ident = new_place(x->dict, user, x->elt);
        delete x;
    }
}

//

// don't use for instance a static global Q3PtrList to
// not be dependent on the initialization order made
// before 'main' execution

struct IntList {
    int * pint;
    const char * file;
    IntList * next;
};

// initialized to 0 before any execution associated
// to the initializations before 'main'
IntList * FirstCell;

void memo_idmax_loc(int & idmaxref, const char * who)
{
    if (who != 0) {
        IntList * cell = new IntList;

        cell->pint = &idmaxref;
        cell->next = FirstCell;
        cell->file = who;
        FirstCell = cell;
    }
}

// add a margin of 50
void idmax_add_margin()
{
    for (IntList * cell = FirstCell; cell != 0; cell = cell->next)
        * (cell->pint) += 128 * 50;
}

//

// to check all operation XX:clear() was called
// ie don't forget to call the operation when a new
// Labeled<> is added

void check_ids_cleared()
{
    for (IntList * cell = FirstCell; cell != 0; cell = cell->next)
        if (*(cell->pint) != FIRST_ID)
            QMessageBox::critical(0, "Douml",
                                  cell->file + QString("\nclear() not called !\n"
                                          "check also update_idmax_for_root()"));
}
