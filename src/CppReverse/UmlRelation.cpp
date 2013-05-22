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

#ifdef DEBUG_DOUML
#include <iostream>
//Added by qt3to4:
#include "misc/mystr.h"
//Added by qt3to4:
#include <Q3PtrList>
#include "Logging/QsLog.h"

using namespace std;
#endif

#include "UmlRelation.h"
#include "UmlPackage.h"
#include "CppSettings.h"
#include "UmlClass.h"
#include "Class.h"
#include "UmlCom.h"
#include "Lex.h"
#ifdef REVERSE
#include "Statistic.h"
# ifdef ROUNDTRIP
# include "UmlAttribute.h"
# include "UmlOperation.h"
# endif
#endif

#ifdef ROUNDTRIP
static UmlRelation * search_rel(Class * container, const WrapperStr & name,
                                UmlClass * dest, const WrapperStr & st)
{
    UmlItem * x = container->get_uml()->search_for_att_rel(name);

    if (x == 0)
        return 0;
    else if (x->kind() == anAttribute) {
        ((UmlAttribute *) x)->deleteIt();
        return 0;
    }
    else {
        UmlRelation * r1 = ((UmlRelation *) x)->side(TRUE);
        UmlRelation * r2 = (r1 != x)
                           ? ((UmlRelation *) x)
                           : ((UmlRelation *) x)->side(FALSE);

        if ((((UmlRelation *) x)->roleType() == dest) &&
            ((r2 == 0) ||
             st.isEmpty() ||
             (((UmlRelation *) x)->stereotype() == st) ||
             (((x == r1) ? r2 : r1)->cppDecl().find("${stereotype}") == -1)))
            return (UmlRelation *) x;

        // rel target or new stereotype not compatible with other side
        if (x != r1)
            r1->set_unidir();
        else if (r2 == 0)
            r1->deleteIt();
        else
            r2->set_unidir();

        return 0;
    }
}
#endif

bool UmlRelation::new_one(Class * container, const WrapperStr & name,
                          UmlClass * dest, const WrapperStr & modifier,
                          const WrapperStr & pretype, const WrapperStr & array,
                          const WrapperStr & typeform, aVisibility visibility,
                          bool staticp, bool constp, bool mutablep, bool volatilep,
                          const WrapperStr & value, WrapperStr comment,
                          WrapperStr description
#ifdef ROUNDTRIP
                          , bool roundtrip, QList<UmlItem *> & expected_order
#endif
                         )
{
#ifdef DEBUG_DOUML
//    QLOG_INFO() << "RELATION '" << name << "' from '" << cl->name() << "' to '" << dest->name()
//                << "' modifier '" << modifier << "' array '" << array
//                << "' typeform '" << typeform << "'\n";
#endif

    if (
#ifdef REVERSE
        container->from_libp() &&
#endif
        (visibility == PrivateVisibility)) {
        Lex::finish_line();
        Lex::clear_comments();
        return TRUE;
    }

    UmlClass * cl = container->get_uml();
    UmlRelation * rel;

#ifdef ROUNDTRIP

    if (roundtrip &&
        ((rel = search_rel(container, name, dest, "")) != 0)) {
        rel->set_usefull();
        expected_order.append(rel);
    }
    else {
#endif
        rel = UmlBaseRelation::create((modifier.isEmpty() &&
                                       ((typeform == "${type}") ||
                                        (typeform[typeform.find("${type}") + 7] != '*')))
                                      ? aDirectionalAggregationByValue
                                      : aDirectionalAssociation,
                                      cl, dest);

        if (rel == 0) {
            UmlCom::trace(WrapperStr("<font face=helvetica><b>cannot add relation <i>")
                          + name + "</i> in <i>" + cl->name() + "</i> to <i>"
                          + dest->name() + "</i></b></font><br><hr>");
            return FALSE;
        }

#ifdef REVERSE
# ifndef ROUNDTRIP
        Statistic::one_relation_more();
# else

        if (roundtrip) {
            expected_order.append(rel);
            container->set_updated();
            roundtrip = FALSE;
        }
    }

# endif
#endif

        Lex::finish_line();

        comment = Lex::get_comments(comment);
        description = Lex::get_description(description);

#ifdef ROUNDTRIP

        if (roundtrip) {
            if (rel->visibility() != visibility) {
                rel->set_Visibility(visibility);
                container->set_updated();
            }

            if (rel->isReadOnly() != constp) {
                rel->set_isReadOnly(constp);
                container->set_updated();
            }

            if (rel->isClassMember() != staticp) {
                rel->set_isClassMember(staticp);
                container->set_updated();
            }

            if (rel->isCppMutable() != mutablep) {
                rel->set_isCppMutable(mutablep);
                container->set_updated();
            }

            if (rel->isVolatile() != volatilep) {
                rel->set_isVolatile(volatilep);
                container->set_updated();
            }
        }
        else {
#endif
            rel->set_Visibility(visibility);

            if (constp) rel->set_isReadOnly(TRUE);

            if (staticp) rel->set_isClassMember(TRUE);

            if (mutablep) rel->set_isCppMutable(TRUE);

            if (volatilep) rel->set_isVolatile(TRUE);

#ifdef ROUNDTRIP
        }

#endif

        WrapperStr decl;

        if (typeform != "${type}") {
            // array & modified are empty, pretype is empty ?
            decl = CppSettings::relationDecl(TRUE, "*");

            int index = typeform.find("<");	// cannot be -1
            WrapperStr st = typeform.left(index);
            WrapperStr st_uml = CppSettings::umlType(st);

#ifdef ROUNDTRIP

            if (roundtrip) {
                if (st_uml.isEmpty())
                    st_uml = st;

                if (neq(rel->stereotype(), st_uml)) {
                    rel->set_Stereotype(st_uml);
                    container->set_updated();
                }
            }
            else
#endif
                rel->set_Stereotype((st_uml.isEmpty()) ? st : st_uml);

            int index2;

            if ((index2 = decl.find("<${type}>")) == -1) {
                decl = "    ${comment}${static}${mutable}${volatile}${const}${stereotype}<${type}> ${name}${value};";
                index2 = decl.find("<${type}>");
            }

            decl.replace(index2, 9, typeform.mid(index));
        }
        else {
            if (!array.isEmpty()) {
#ifdef ROUNDTRIP

                if (roundtrip) {
                    if (neq(rel->multiplicity(), array)) {
                        rel->set_Multiplicity(array);
                        container->set_updated();
                    }
                }
                else
#endif
                    rel->set_Multiplicity(array);
            }

            decl = CppSettings::relationDecl(modifier != "*", array);

            int index;

            if (!pretype.isEmpty() &&
                ((index = decl.find("${type}")) != 0))
                decl.insert(index, (const char *)(pretype + " "));

            if ((modifier == "&") && ((index = decl.find("${type}")) != 0))
                decl.insert(index + 7, " &");
        }

        if (! value.isEmpty()) {
            int index = decl.find("${value}");

            if (index != -1)
                decl.insert(index + 2,  "h_");

#ifdef ROUNDTRIP

            if (roundtrip) {
                if (!staticp) {
                    WrapperStr v = rel->defaultValue();

                    if (!v.isEmpty() && (((const char *) v)[0] == '='))
                        v = v.mid(1);

                    if (nequal(v, value)) {
                        rel->set_DefaultValue(value);
                        container->set_updated();
                    }
                }
            }
            else
#endif
                rel->set_DefaultValue(value);
        }

#ifdef ROUNDTRIP

        if (roundtrip) {
            if (neq(rel->cppDecl(), decl)) {
                rel->set_CppDecl(decl);
                container->set_updated();
            }

            if (decl.find("${description}") != -1) {
                if (nequal(rel->description(), description)) {
                    rel->set_Description(description);
                    container->set_updated();
                }
            }
            else if (nequal(rel->description(), Lex::simplify_comment(comment))) {
                rel->set_Description(comment); // comment was set
                container->set_updated();
            }

            // role name is the right one
            return TRUE;
        }

#endif

        rel->set_CppDecl(decl);

        if (!comment.isEmpty())
            rel->set_Description((decl.find("${description") != -1)
                                 ? description : Lex::simplify_comment(comment));

        return rel->set_RoleName(name);
    }

    bool UmlRelation::new_friend(UmlClass * from, UmlClass * to)
    {
#ifdef DEBUG_DOUML
        QLOG_INFO() << "FRIEND from '" << from->name() << "' to '" << to->name() << "'\n";
#endif

        UmlRelation * rel = UmlBaseRelation::create(aDependency, from, to);

        if (rel == 0) {
            UmlCom::trace(WrapperStr("<font face=helvetica><b>cannot add friend relation in <i>")
                          + from->name() + "</i> to <i>"
                          + to->name() + "</i></b></font><br><hr><br>");
            return FALSE;
        }

#ifndef ROUNDTRIP
        Statistic::one_relation_more();
#endif

        rel->set_CppDecl("Generated");

        return rel->set_Stereotype("friend");
    }

#ifdef ROUNDTRIP

    bool UmlRelation::new_friend(Class * container, UmlClass * to,
                                 QList<UmlItem *> & expected_order)
    {
        UmlClass * from = container->get_uml();

#ifdef DEBUG_DOUML
        QLOG_INFO() << "FRIEND from '" << from->name() << "' to '" << to->name() << "'\n";
#endif

        const Q3PtrVector<UmlItem> & ch = from->children();
        UmlItem ** v = ch.data();
        UmlItem ** const vsup = v + ch.size();

        for (; v != vsup; v += 1) {
            if (((*v)->kind() == aRelation) &&
                (((UmlRelation *) *v)->relationKind() == aDependency) &&
                (((UmlRelation *) *v)->roleType() == to) &&
                !neq((*v)->stereotype(), "friend")) {
                expected_order.append(*v);
                ((UmlRelation *) *v)->set_usefull();
                return TRUE;
            }
        }

        // relation not found

        UmlRelation * rel = UmlBaseRelation::create(aDependency, from, to);

        if (rel == 0) {
            UmlCom::trace(WrapperStr("<font face=helvetica><b>cannot add friend relation in <i>")
                          + from->name() + "</i> to <i>"
                          + to->name() + "</i></b></font><br><hr><br>");
            return FALSE;
        }

        expected_order.append(rel);
        container->set_updated();

        rel->set_CppDecl("Generated");

        return rel->set_Stereotype("friend");
    }

    static void copy(UmlOperation * from, UmlOperation * to)
    {
        to->set_roundtrip_expected();
        to->set_usefull(!from->is_useless());

        to->set_CppGetSetFrozen(from->cppGetSetFrozen());
        to->set_CppNameSpec(from->cppNameSpec());
        to->set_Name(from->name());
        to->set_isCppConst(from->isCppConst());
        to->set_isCppFriend(from->isCppFriend());
        to->set_isCppVirtual(from->isCppVirtual());
        to->set_CppBody(from->cppBody());
        to->set_CppContextualBodyIndent(from->cppContextualBodyIndent());
        to->set_ReturnType(from->returnType());
        to->set_CppDecl(from->cppDecl());
        to->set_CppDef(from->cppDef());
    }

    void UmlRelation::set_unidir()
    {
        UmlRelation * r1 = side(TRUE);
        UmlRelation * r2 = (r1 != this) ? this : side(FALSE);

        if (r1->isReadOnly() || r2->isReadOnly()) {
            UmlCom::trace(WrapperStr("<font face=helvetica>in <i>") + WrapperStr(Lex::filename().toAscii().constData())
                          + "</i> line " + WrapperStr().setNum(Lex::line_number())
                          + " <b>cannot remove relation between classes <i>"
                          + roleType()->name() + "</i> and <i>" + parent()->name()
                          + "</i> because one is read only</b></font><br>");
            throw 0;
        }

        aRelationKind k;

        switch (relationKind()) {
        case anAssociation:
            k = aDirectionalAssociation;
            break;

        case anAggregation:
            k = aDirectionalAggregation;
            break;

        default:
            k = aDirectionalAggregationByValue;
        }

        if (this == r1)
            set_rel_kind(k);
        else {
            UmlRelation * rel =
                UmlBaseRelation::create(aDirectionalAssociation,
                                        (UmlClass *) parent(), roleType());
            WrapperStr role = roleName();

            rel->moveAfter(this);
            rel->set_Visibility(visibility());

            if (!description().isEmpty())
                rel->set_Description(description());

            if (isReadOnly())
                rel->set_isReadOnly(TRUE);

            if (isVolatile())
                rel->set_isVolatile(TRUE);

            if (isClassMember())
                rel->set_isClassMember(TRUE);

            if (!multiplicity().isEmpty())
                rel->set_Multiplicity(multiplicity());

            if (!defaultValue().isEmpty())
                rel->set_DefaultValue(defaultValue());

            if (!stereotype().isEmpty())
                rel->set_Stereotype(stereotype());

            rel->set_CppDecl(cppDecl());
            rel->set_CppVisibility(cppVisibility());

            if (cppVirtualInheritance())
                rel->set_CppVirtualInheritance(TRUE);

            if (isCppMutable())
                rel->set_isCppMutable(TRUE);

            UmlOperation * op;
            UmlOperation * oper;

            if (((op = getOperation()) != 0) &&
                rel->addGetOperation() &&
                ((oper = rel->getOperation()) != 0))
                copy(op, oper);

            if (((op = setOperation()) != 0) &&
                rel->addSetOperation() &&
                ((oper = rel->getOperation()) != 0))
                copy(op, oper);

            r1->deleteIt();
            r2->deleteIt();

            rel->set_RoleName(role);
        }
    }

#endif

