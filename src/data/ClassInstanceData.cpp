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





#include <qcursor.h>
//Added by qt3to4:
#include <QTextStream>
#include <Q3ValueList>
//Added by qt3to4:
#include <Q3PtrList>

#include "ClassInstanceData.h"
#include "BrowserClassInstance.h"
#include "BrowserClass.h"
#include "BrowserAttribute.h"
#include "BrowserRelation.h"
#include "ClassData.h"
#include "AttributeData.h"
#include "RelationData.h"
#include "ClassInstanceDialog.h"
#include "myio.h"
#include "ToolCom.h"
#include "mu.h"
#include "strutil.h"

ClassInstanceData::ClassInstanceData(BrowserClass * c) : cl(c)
{
    if (cl != 0) {
        // not on read
        BasicData * cld = cl->get_data();

        connect_list.append(cld);
        connect(cld, SIGNAL(changed()), this, SLOT(check()));
        connect(cld, SIGNAL(deleted()), this, SLOT(check()));
    }
}

ClassInstanceData::ClassInstanceData(ClassInstanceData * model)
    : SimpleData(model), cl(model->cl)
{
    BasicData * cld = cl->get_data();

    connect_list.append(cld);
    connect(cld, SIGNAL(changed()), this, SLOT(check()));
    connect(cld, SIGNAL(deleted()), this, SLOT(check()));

    attributes = model->attributes;
    // doesn't copy relations, else have to add also rel in other class instance
}

ClassInstanceData::~ClassInstanceData()
{
}


void ClassInstanceData::delete_it()
{
    // attributes unchanged,
    // relations cleared because also cleared on oposite side
    relations.clear();
    SimpleData::delete_it();
}

void ClassInstanceData::edit()
{
    // to take into account deleted class instances being
    // the value of relations
    check_rels();

    setName(browser_node->get_name());

    (new ClassInstanceDialog(this))->show();
}

//

void ClassInstanceData::init_other_side()
{
    Q3ValueList<SlotRel>::Iterator it_rel = relations.begin();

    while (it_rel != relations.end()) {
        const SlotRel & slot_rel = *it_rel;

        if (((BrowserNode *) slot_rel.rel->get_start()->parent())
            ->get_type() == UmlPackage) {
            // placed in temporary package
            // relation doesn't exist
            it_rel = relations.remove(it_rel);
            continue;
        }

        if (slot_rel.is_a &&
            !RelationData::uni_directional(slot_rel.rel->get_type())) {
            ClassInstanceData * other_data =
                (ClassInstanceData *) slot_rel.value->get_data();
            Q3ValueList<SlotRel> & other_rels =
                other_data->relations;
            Q3ValueList<SlotRel>::Iterator it_rel_other;

            for (it_rel_other = other_rels.begin();
                 it_rel_other != other_rels.end();
                 it_rel_other++) {
                const SlotRel & other_slot_rel = *it_rel_other;

                if (!other_slot_rel.is_a &&
                    (other_slot_rel.value == browser_node) &&
                    (other_slot_rel.rel == slot_rel.rel))
                    break;
            }

            if (it_rel_other == other_rels.end()) {
                // add it
                other_data->relations.append(SlotRel(FALSE, slot_rel.rel, (BrowserClassInstance *) browser_node));

                BasicData * d = slot_rel.rel->get_end_class()->get_data();

                if (! other_data->connect_list.contains(d)) {
                    other_data->connect_list.append(d);
                    connect(d, SIGNAL(changed()), other_data, SLOT(check()));
                    connect(d, SIGNAL(deleted()), other_data, SLOT(check()));
                }
            }
        }

        ++it_rel;
    }
}

// warning : don't remove connect because may be called
// during connexion list use

void ClassInstanceData::check()
{
    if ((cl == 0) || // deleted class instance referenced in a diagram
        cl->deletedp())
        browser_node->delete_it();
    else if (!attributes.isEmpty() || !relations.isEmpty()) {
        bool modif = FALSE;
        QList<BrowserClass *> l;

        cl->get_all_parents(l);
        l.append(cl);

        // check attributes

        Q3ValueList<SlotAttr>::Iterator it_attr = attributes.begin();

        while (it_attr != attributes.end()) {
            const SlotAttr & slot_attr = *it_attr;

            if (slot_attr.att->deletedp() ||
                (! l.contains((BrowserClass *) slot_attr.att->parent()))) {
                // must be removed
                it_attr = attributes.remove(it_attr);
                modif = TRUE;
            }
            else {
                // change on attribute modify class => memorize classes only
                BasicData * cld = ((BrowserNode *) slot_attr.att->parent())->get_data();

                if (! connect_list.contains(cld)) {
                    connect_list.append(cld);
                    connect(cld, SIGNAL(changed()), this, SLOT(check()));
                    connect(cld, SIGNAL(deleted()), this, SLOT(check()));
                }

                ++it_attr;
            }
        }

        // check relations (only this side, check also called on other class instance)

        Q3ValueList<SlotRel>::Iterator it_rel = relations.begin();

        while (it_rel != relations.end()) {
            const SlotRel & slot_rel = *it_rel;
            BrowserRelation * br = (slot_rel.is_a)
                                   ? ((slot_rel.rel->get_start()->deletedp())
                                      ? (BrowserRelation *) 0
                                      : slot_rel.rel->get_start())
                                       : (((RelationData::uni_directional(slot_rel.rel->get_type())) ||
                                           slot_rel.rel->get_end()->deletedp())
                                          ? (BrowserRelation *) 0
                                          : slot_rel.rel->get_end());

            if (br == 0) {
                // relation deleted, must be removed
                it_rel = relations.remove(it_rel);
                modif = TRUE;
            }
            else if (!l.contains((BrowserClass *) br->parent())) {
                // new instance type, must be removed in both side
                if (slot_rel.value->get_data() != this) {
                    // not reflexive
                    Q3ValueList<SlotRel> & other_rels =
                        ((ClassInstanceData *) slot_rel.value->get_data())->relations;
                    Q3ValueList<SlotRel>::Iterator it_rel_other;

                    for (it_rel_other = other_rels.begin();
                         it_rel_other != other_rels.end();
                         it_rel_other++) {
                        const SlotRel & other_slot_rel = *it_rel_other;

                        if ((other_slot_rel.value == browser_node) &&
                            (other_slot_rel.rel == slot_rel.rel)) {
                            other_rels.remove(it_rel_other);
                            break;
                        }
                    }
                }

                it_rel = relations.remove(it_rel);
                modif = TRUE;
            }
            else {
                // change on relation modify class => memorize classes only
                BasicData * d;

                d = ((BrowserNode *) br->parent())->get_data();

                if (! connect_list.contains(d)) {
                    connect_list.append(d);
                    connect(d, SIGNAL(changed()), this, SLOT(check()));
                    connect(d, SIGNAL(deleted()), this, SLOT(check()));
                }

                d = slot_rel.value->get_data();

                if (! connect_list.contains(d)) {
                    connect_list.append(d);
                    connect(d, SIGNAL(changed()), this, SLOT(check_rels()));
                    connect(d, SIGNAL(deleted()), this, SLOT(check_rels()));
                }

                ++it_rel;
            }
        }

        if (modif)
            modified();
    }
}

// check if the relations are right, other side modified or deleted
// don't change connexions because may be present several times

void ClassInstanceData::check_rels()
{
    bool modif = FALSE;
    Q3ValueList<SlotRel>::Iterator it_rel = relations.begin();

    while (it_rel != relations.end()) {
        const SlotRel & slot_rel = *it_rel;
        BasicData * d = slot_rel.value->get_data();
        bool remove = slot_rel.value->deletedp();

        if (!remove) {
            RelationData * rd = (RelationData *) slot_rel.rel;
            BrowserClass * other = (slot_rel.is_a)
                                   ? rd->get_end_class() : rd->get_start_class();
            BrowserClass * cl = ((ClassInstanceData *) d)->get_class();

            if (cl == 0) {
                // instance doesn't exist
                // note : its brower_node will be deleted
                remove = TRUE;
            }
            else if (cl != other) {
                QList<BrowserClass *> l;

                cl->get_all_parents(l);
                remove |= (! l.contains(other));
            }
        }

        if (remove) {
            it_rel = relations.remove(it_rel);
            modif = TRUE;
        }
        else
            ++it_rel;
    }

    if (modif)
        modified();
}

// returns true if the relation rd exist between this and other
// this must correspond to is_a
bool ClassInstanceData::exist(BrowserClassInstance * other,
                              RelationData * rd) const
{
    Q3ValueList<SlotRel>::ConstIterator it;

    for (it = relations.begin(); it != relations.end(); ++it) {
        const SlotRel & slot_rel = *it;

        if (slot_rel.is_a && (slot_rel.rel == rd) && (slot_rel.value == other))
            return TRUE;
    }

    const Q3ValueList<SlotRel> & rels =
        ((ClassInstanceData *) other->get_data())->relations;

    for (it = rels.begin(); it != rels.end(); ++it) {
        const SlotRel & slot_rel = *it;

        if (!slot_rel.is_a && (slot_rel.rel == rd) && (slot_rel.value == browser_node))
            return TRUE;
    }

    return FALSE;
}

// to add a relation

void ClassInstanceData::add(BrowserClassInstance * other,
                            RelationData * rd)
{
    // check if already exist
    Q3ValueList<SlotRel>::Iterator it;

    for (it = relations.begin(); it != relations.end(); ++it) {
        SlotRel & slot_rel = *it;

        if ((slot_rel.rel == rd) &&
            (slot_rel.value == other) &&
            (slot_rel.is_a == TRUE))
            // find
            return;
    }

    relations.append(SlotRel(TRUE, rd, other));

    BasicData * d = rd->get_start_class()->get_data();

    if (! connect_list.contains(d)) {
        connect_list.append(d);
        connect(d, SIGNAL(changed()), this, SLOT(check()));
        connect(d, SIGNAL(deleted()), this, SLOT(check()));
    }

    if (!RelationData::uni_directional(rd->get_type())) {
        ClassInstanceData * od = (ClassInstanceData *) other->get_data();

        od->relations.append(SlotRel(FALSE, rd, (BrowserClassInstance *) browser_node));

        d = rd->get_end_class()->get_data();

        if (! od->connect_list.contains(d)) {
            od->connect_list.append(d);
            connect(d, SIGNAL(changed()), od, SLOT(check()));
            connect(d, SIGNAL(deleted()), od, SLOT(check()));
        }
    }
}

// replace relation between this and other being current by future
// if future is 0 remove the relation

void ClassInstanceData::replace_internal(BrowserClassInstance * other,
        RelationData * current,
        RelationData * future,
        bool current_isa,
        bool future_isa)
{
    Q3ValueList<SlotRel>::Iterator it;

    for (it = relations.begin(); it != relations.end(); ++it) {
        SlotRel & slot_rel = *it;

        if ((slot_rel.rel == current) &&
            (slot_rel.value == other) &&
            (slot_rel.is_a == current_isa)) {
            if ((future == 0) ||
                (!future_isa && RelationData::uni_directional(future->get_type())))
                relations.remove(it);
            else {
                slot_rel.rel = future;
                slot_rel.is_a = future_isa;

                BasicData * d =
                    ((BrowserNode *) future->get_start()->parent())
                    ->get_data();

                if (! connect_list.contains(d)) {
                    connect_list.append(d);
                    connect(d, SIGNAL(changed()), this, SLOT(check()));
                    connect(d, SIGNAL(deleted()), this, SLOT(check()));
                }
            }

            return;
        }
    }

    // doesn't exist yet
    if ((future != 0) &&
        (future_isa || RelationData::uni_directional(future->get_type()))) {
        relations.append(SlotRel(future_isa, future, other));

        BasicData * d = future->get_start_class()->get_data();

        if (! connect_list.contains(d)) {
            connect_list.append(d);
            connect(d, SIGNAL(changed()), this, SLOT(check()));
            connect(d, SIGNAL(deleted()), this, SLOT(check()));
        }
    }
}

void ClassInstanceData::remove_clone(BrowserClassInstance * other,
                                     RelationData * rd, bool isa)
{
    bool find = FALSE;
    Q3ValueList<SlotRel>::Iterator it;

    for (it = relations.begin(); it != relations.end(); ++it) {
        SlotRel & slot_rel = *it;

        if ((slot_rel.rel == rd) &&
            (slot_rel.value == other) &&
            (slot_rel.is_a == isa)) {
            if (find) {
                // a clone
                relations.remove(it);
                return;
            }
            else
                find = TRUE;
        }
    }
}

void ClassInstanceData::replace(BrowserClassInstance * other,
                                RelationData * current,
                                RelationData * future,
                                bool current_isa,
                                bool future_isa)
{
    replace_internal(other, current, future, current_isa, future_isa);
    ((ClassInstanceData *) other->get_data())
    ->replace_internal((BrowserClassInstance *) browser_node,
                       current, future, !current_isa, !future_isa);

    if (future != 0) {
        remove_clone(other, future, future_isa);
        ((ClassInstanceData *) other->get_data())
        ->remove_clone((BrowserClassInstance *) browser_node,
                       future, !future_isa);
    }
}

void ClassInstanceData::set_class(BrowserClass * t)
{
    if (t != cl) {
        BrowserClass * previous = cl;

        cl = t;

        while (!relations.isEmpty()) {
            SlotRel & slot_rel = relations.first();

            replace(slot_rel.value, slot_rel.rel, 0, slot_rel.is_a, FALSE);
        }

        foreach (BasicData *data, connect_list)
            disconnect(data, 0, this, 0);
        connect_list.clear();

        BasicData * cld = cl->get_data();

        connect_list.append(cld);
        connect(cld, SIGNAL(changed()), this, SLOT(check()));
        connect(cld, SIGNAL(deleted()), this, SLOT(check()));

        if (previous != 0) {
            // not on read
            check();
            modified();
        }
    }
}

//

void ClassInstanceData::send_uml_def(ToolCom * com, BrowserNode * bn,
                                     const QString & comment)
{
    BasicData::send_uml_def(com, bn, comment);
    cl->write_id(com);
}

bool ClassInstanceData::tool_cmd(ToolCom * com, const char * args,
                                 BrowserNode * bn, const QString & comment)
{
    if (((unsigned char) args[-1]) >= firstSetCmd) {
        if (!bn->is_writable() && !root_permission())
            com->write_ack(FALSE);
        else {
            switch ((unsigned char) args[-1]) {
            case setTypeCmd:
                set_class((BrowserClass *) com->get_id(args));
                break;

            case setAttributeCmd: {
                BrowserAttribute * at = (BrowserAttribute *) com->get_id(args);
                bool find = FALSE;
                Q3ValueList<SlotAttr>::Iterator it_attr;

                for (it_attr = attributes.begin(); it_attr != attributes.end(); ++it_attr) {
                    SlotAttr & slot_attr = *it_attr;

                    if (slot_attr.att == at) {
                        find = TRUE;

                        if (*args == 0)
                            // remove it
                            attributes.remove(it_attr);
                        else
                            // replace it
                            slot_attr.value = args;

                        break;
                    }
                }

                if (! find) {
                    // add it
                    QList<BrowserClass *> l;

                    cl->get_all_parents(l);
                    l.append(cl);

                    if (at->deletedp() ||
                        (! l.contains((BrowserClass *)at->parent()))) {
                        // illegal
                        com->write_ack(FALSE);
                        return TRUE;
                    }

                    attributes.append(SlotAttr(at, args));
                }
            }
            break;

            case addRelationCmd:
                if (! change_rel(com, args, TRUE))
                    return TRUE;

                break;

            case removeRelationCmd:
                if (! change_rel(com, args, FALSE))
                    return TRUE;

                break;

            default:
                return BasicData::tool_cmd(com, args, bn, comment);
            }

            // ok case
            bn->modified();
            modified();
            com->write_ack(TRUE);
        }
    }
    else {
        switch ((unsigned char) args[-1]) {
        case attributesCmd:
            if (args[0] == 0) {
                com->write_unsigned(attributes.count());

                Q3ValueList<SlotAttr>::Iterator it;

                for (it = attributes.begin(); it != attributes.end(); ++it) {
                    const SlotAttr & slot = *it;

                    slot.att->write_id(com);
                    com->write_string(slot.value);
                }
            }
            else {
                // get all available attributes
                BrowserNodeList l;

                cl->get_attrs(l);
                com->write_unsigned(l.count());

                foreach (BrowserNode * node, l)
                    node->write_id(com);
            }

            break;

        case relationsCmd: {
            BrowserClassInstance * other =
                (BrowserClassInstance *) com->get_id(args);

            if (other == 0) {
                com->write_unsigned(relations.count());

                Q3ValueList<SlotRel>::Iterator it;

                for (it = relations.begin(); it != relations.end(); ++it) {
                    const SlotRel & slot = *it;

                    ((slot.is_a) ? slot.rel->get_start() : slot.rel->get_end())
                    ->write_id(com);
                    slot.value->write_id(com);
                }
            }
            else {
                // get all available relations
                QList<BrowserRelation *> l;

                cl->get_rels(((ClassInstanceData *)other->get_data())->cl, l);
                com->write_unsigned(l.count());

                foreach (BrowserRelation *r, l)
                    r->write_id(com);
            }
        }
        break;

        default:
            return BasicData::tool_cmd(com, args, bn, comment);
        }
    }

    return TRUE;
}

bool ClassInstanceData::change_rel(ToolCom * com, const char * args,
                                   bool isadd)
{
    com->get_unsigned(args); // 2

    BrowserRelation * r = (BrowserRelation *) com->get_id(args);
    BrowserClassInstance * other = (BrowserClassInstance *) com->get_id(args);

    if (r->deletedp() || other->deletedp()) {
        com->write_ack(FALSE);
        return FALSE;
    }

    RelationData * rd = (RelationData *) r->get_data();
    Q3ValueList<SlotRel>::Iterator it;

    for (it = relations.begin(); it != relations.end(); ++it) {
        const SlotRel & slot_rel = *it;

        if ((slot_rel.value == other) &&
            (r == ((slot_rel.is_a) ? rd->get_start() : rd->get_end())))
            break;
    }

    if (isadd) {
        if (it == relations.end()) {
            // not yet present
            QList<BrowserRelation *> l;

            cl->get_rels(((ClassInstanceData *)other->get_data())->cl, l);

            if ((! l.contains(r)) ||
                (!other->is_writable() && !root_permission())) {
                // illegal
                com->write_ack(FALSE);
                return FALSE;
            }

            // add it
            if (rd->is_a(r))
                add(other, rd);
            else
                ((ClassInstanceData *)other->get_data())
                ->add((BrowserClassInstance *) browser_node, rd);
        }
    }
    else if (it != relations.end()) {
        // remove it
        if (!other->is_writable() && !root_permission()) {
            // illegal
            com->write_ack(FALSE);
            return FALSE;
        }

        replace(other, rd, 0, (*it).is_a, 0);
    }

    return TRUE;
}
//

void ClassInstanceData::save(QTextStream & st, QString & warning) const
{
    BasicData::save(st, warning);
    nl_indent(st);
    st << "type ";
    cl->save(st, TRUE, warning);

    nl_indent(st);
    st << "attributes";

    Q3ValueList<SlotAttr>::ConstIterator it_attr = attributes.begin();

    while (it_attr != attributes.end()) {
        const SlotAttr & slot_attr = *it_attr;

        nl_indent(st);
        st << "  ";
        slot_attr.att->save(st, TRUE, warning);
        nl_indent(st);
        st << "    ";
        save_string(slot_attr.value, st);
        ++it_attr;
    }

    nl_indent(st);
    st << "  end";
    nl_indent(st);
    st << "relations";

    Q3ValueList<SlotRel>::ConstIterator it_rel = relations.begin();

    while (it_rel != relations.end()) {
        const SlotRel & slot_rel = *it_rel;

        if (slot_rel.is_a && !slot_rel.value->deletedp()) {
            // to not have unconsistency on load,
            // other side remade at load time
            nl_indent(st);
            st << "  ";
            slot_rel.rel->save(st, TRUE, warning);
            nl_indent(st);
            st << "    ";
            slot_rel.value->save(st, TRUE, warning);
        }

        ++it_rel;
    }

    nl_indent(st);
    st << "  end";
}

void ClassInstanceData::read(char *& st, char *& k)
{
    BasicData::read(st, k);	// updates k

    if (strcmp(k, "type"))
        wrong_keyword(k, "type");

    cl = BrowserClass::read_ref(st, 0);

    read_keyword(st, "attributes");

    while (strcmp(k = read_keyword(st), "end")) {
        BrowserAttribute * a = BrowserAttribute::read(st, k, 0, TRUE);

        attributes.append(SlotAttr(a, read_string(st)));
    }

    read_keyword(st, "relations");

    while (strcmp(k = read_keyword(st), "end")) {
        RelationData * r = RelationData::read_ref(st, TRUE, k);

        relations.append(SlotRel(TRUE, r, BrowserClassInstance::read_ref(st)));
    }

    k = read_keyword(st);
}

// to read a definition coming from an object diagram

void ClassInstanceData::read_attributes(char *& st, char *& k)
{
    while (strcmp(k = read_keyword(st), "end") &&
           strcmp(k, "xyz")) {	// very old version
        BrowserAttribute * a = BrowserAttribute::read(st, k, 0, FALSE);
        QString s = read_string(st);

        if (a != 0)
            attributes.append(SlotAttr(a, s));
    }

    check();
}

