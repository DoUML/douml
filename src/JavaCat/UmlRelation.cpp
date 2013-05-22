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

#ifdef TRACE
#include <iostream>
//Added by qt3to4:
#include "misc/mystr.h"
//Added by qt3to4:
#include <Q3PtrList>

using namespace std;
#endif

#include "UmlRelation.h"
#include "UmlPackage.h"
#include "JavaSettings.h"
#include "UmlClass.h"
#include "Class.h"
#include "JavaCatWindow.h"
#include "Lex.h"
#ifdef REVERSE
#include "Statistic.h"
# ifdef ROUNDTRIP
# include "UmlAttribute.h"
# include "UmlOperation.h"
# include "UmlCom.h"
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
             (((x == r1) ? r2 : r1)->javaDecl().find("${stereotype}") == -1)))
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

// not from a form 'generic<...C...> var' where C is a class
bool UmlRelation::new_one(Class * container, const WrapperStr & name,
                          UmlTypeSpec & dest, WrapperStr str_actuals,
                          aVisibility visibility, bool staticp,
                          bool constp, bool transientp, bool volatilep,
                          const WrapperStr & array, const WrapperStr & value,
                          WrapperStr comment, WrapperStr description,
                          WrapperStr annotation
#ifdef ROUNDTRIP
                          , bool roundtrip, QList<UmlItem *> & expected_order
#endif
                         )
{
#ifdef TRACE
    QLOG_INFO() << "RELATION '" << name << "' from '" << cl->Name() << "' to '" << dest.type->Name()
                << "' array '" << array << "'\n";
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
    bool created;

    if (!roundtrip ||
        ((rel = search_rel(container, name, dest.type, "")) == 0)) {
#endif
        rel = UmlBaseRelation::create(aDirectionalAssociation, cl, dest.type);

        if (rel == 0) {
            JavaCatWindow::trace(WrapperStr("<font face=helvetica><b>cannot add relation <i>")
                                 + name + "</i> in <i>" + cl->name() + "</i> to <i>"
                                 + dest.type->name() + "</i></b></font><br>");
            return FALSE;
        }

#ifdef REVERSE
# ifndef ROUNDTRIP
        Statistic::one_relation_more();
# else

        if (roundtrip)
            container->set_updated();

        created = TRUE;
    }
    else
        created = FALSE;

# endif
#endif

        WrapperStr decl = JavaSettings::relationDecl(array);

        UmlClass::manage_generic(decl, dest, str_actuals, "${type}");

        Lex::finish_line();
        comment = Lex::get_comments(comment);
        description = Lex::get_description(description);

#ifdef ROUNDTRIP

        if (roundtrip && !created) {
            if (rel->visibility() != visibility) {
                rel->set_Visibility(visibility);
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

            if (rel->isReadOnly() != constp) {
                rel->set_isReadOnly(constp);
                container->set_updated();
            }

            if (rel->isJavaTransient() != transientp) {
                rel->set_isJavaTransient(transientp);
                container->set_updated();
            }

            if (rel->isVolatile() != volatilep) {
                rel->set_isVolatile(volatilep);
                container->set_updated();
            }

            if (rel->isClassMember() != staticp) {
                rel->set_isClassMember(staticp);
                container->set_updated();
            }

            if (neq(rel->multiplicity(), array)) {
                rel->set_Multiplicity(array);
                container->set_updated();
            }

            WrapperStr v = rel->defaultValue();

            if (!v.isEmpty() && (((const char *) v)[0] == '='))
                v = v.mid(1);

            if (nequal(v, value)) {
                rel->set_DefaultValue(value);
                container->set_updated();
            }

            if (nequal(rel->javaAnnotations(), annotation)) {
                rel->set_JavaAnnotations(annotation);
                container->set_updated();
            }

            if (neq(rel->javaDecl(), decl)) {
                rel->set_JavaDecl(decl);
                container->set_updated();
            }

            rel->set_usefull();

            expected_order.append(rel);
        }
        else {
#endif
            rel->set_Visibility(visibility);

            if (!comment.isEmpty())
                rel->set_Description((decl.find("${description}") != -1)
                                     ? description : comment);

            if (constp)
                rel->set_isReadOnly(TRUE);

            if (transientp)
                rel->set_isJavaTransient(TRUE);

            if (volatilep)
                rel->set_isVolatile(TRUE);

            if (staticp)
                rel->set_isClassMember(TRUE);

            if (!array.isEmpty())
                rel->set_Multiplicity(array);

            if (! value.isEmpty())
                rel->set_DefaultValue(value);

            if (! annotation.isEmpty())
                rel->set_JavaAnnotations(annotation);

            rel->set_JavaDecl(decl);

            rel->set_RoleName(name);

#ifdef ROUNDTRIP

            if (roundtrip)
                expected_order.append(rel);
        }

#endif

        return TRUE;
    }

// from a form 'generic<...C...> var' where C is a class
    bool UmlRelation::new_one(Class * container, const WrapperStr & name,
                              UmlClass * type, WrapperStr type_def,
                              WrapperStr genericname,
                              aVisibility visibility, bool staticp,
                              bool constp, bool transientp, bool volatilep,
                              const WrapperStr & array, const WrapperStr & value,
                              WrapperStr comment, WrapperStr description,
                              WrapperStr annotation
#ifdef ROUNDTRIP
                              , bool roundtrip, QList<UmlItem *> & expected_order
#endif
                             )
    {
#ifdef TRACE
        QLOG_INFO() << "RELATION '" << name << "' from '" << cl->Name() << "' to '" << type->Name()
                    << "' array '" << array << "'\n";
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

        WrapperStr st = JavaSettings::umlType(genericname);

        if (st.isEmpty())
            st = genericname;

        UmlClass * cl = container->get_uml();
        UmlRelation * rel;

#ifdef ROUNDTRIP
        bool created;

        if (!roundtrip ||
            ((rel = search_rel(container, name, type, st)) == 0)) {
#endif
            rel = UmlBaseRelation::create(aDirectionalAssociation, cl, type);

            if (rel == 0) {
                JavaCatWindow::trace(WrapperStr("<font face=helvetica><b>cannot add relation <i>")
                                     + name + "</i> in <i>" + cl->name() + "</i> to <i>"
                                     + type->name() + "</i></b></font><br>");
                return FALSE;
            }

#ifdef REVERSE
# ifndef ROUNDTRIP
            Statistic::one_relation_more();
# else

            if (roundtrip)
                container->set_updated();

            created = TRUE;
        }
        else
            created = FALSE;

# endif
#endif

            Lex::finish_line();

            comment = Lex::get_comments(comment);
            description = Lex::get_description(description);

            WrapperStr decl = JavaSettings::relationDecl(array);

            type_def.replace(0, genericname.length(), "${stereotype}");
            decl.replace(decl.find("${type}"), 7, type_def);


#ifdef ROUNDTRIP

            if (roundtrip && !created) {
                if (rel->visibility() != visibility) {
                    rel->set_Visibility(visibility);
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

                if (rel->isReadOnly() != constp) {
                    rel->set_isReadOnly(constp);
                    container->set_updated();
                }

                if (rel->isJavaTransient() != transientp) {
                    rel->set_isJavaTransient(transientp);
                    container->set_updated();
                }

                if (rel->isVolatile() != volatilep) {
                    rel->set_isVolatile(volatilep);
                    container->set_updated();
                }

                if (rel->isClassMember() != staticp) {
                    rel->set_isClassMember(staticp);
                    container->set_updated();
                }

                if (neq(rel->multiplicity(), array)) {
                    rel->set_Multiplicity(array);
                    container->set_updated();
                }

                if (neq(rel->defaultValue(), value)) {
                    rel->set_DefaultValue(value);
                    container->set_updated();
                }

                if (nequal(rel->javaAnnotations(), annotation)) {
                    rel->set_JavaAnnotations(annotation);
                    container->set_updated();
                }

                if (neq(rel->stereotype(), st) &&
                    (rel->stereotype().isEmpty() ||
                     (JavaSettings::relationAttributeStereotype(rel->stereotype()) != st))) {
                    rel->set_Stereotype(st);
                    container->set_updated();
                }

                if (neq(rel->javaDecl(), decl)) {
                    rel->set_JavaDecl(decl);
                    container->set_updated();
                }

                // role name is the right one

                rel->set_usefull();
                expected_order.append(rel);
            }
            else {
#endif
                rel->set_Visibility(visibility);

                if (!comment.isEmpty())
                    rel->set_Description((decl.find("${description}") != -1)
                                         ? description : Lex::simplify_comment(comment));

                if (constp)
                    rel->set_isReadOnly(TRUE);

                if (transientp)
                    rel->set_isJavaTransient(TRUE);

                if (volatilep)
                    rel->set_isVolatile(TRUE);

                if (staticp)
                    rel->set_isClassMember(TRUE);

                if (!array.isEmpty())
                    rel->set_Multiplicity(array);

                if (! value.isEmpty())
                    rel->set_DefaultValue(value);

                if (! annotation.isEmpty())
                    rel->set_JavaAnnotations(annotation);

                rel->set_Stereotype(st);

                rel->set_JavaDecl(decl);

                rel->set_RoleName(name);

#ifdef ROUNDTRIP

                if (roundtrip)
                    expected_order.append(rel);
            }

#endif

            return TRUE;
        }

#ifdef ROUNDTRIP
        static void copy(UmlOperation * from, UmlOperation * to)
        {
            to->set_roundtrip_expected();
            to->set_usefull(!from->is_useless());
            to->set_JavaGetSetFrozen(from->javaGetSetFrozen());
            to->set_JavaNameSpec(from->javaNameSpec());
            to->set_Name(from->name());
            to->set_isJavaFinal(from->isJavaFinal());
            to->set_isJavaSynchronized(from->isJavaSynchronized());
            to->set_JavaBody(from->javaBody());
            to->set_JavaContextualBodyIndent(from->javaContextualBodyIndent());
            to->set_ReturnType(from->returnType());
            to->set_JavaDef(from->javaDef());
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

                if (isJavaTransient())
                    rel->set_isJavaTransient(TRUE);

                if (isVolatile())
                    rel->set_isVolatile(TRUE);

                if (isClassMember())
                    rel->set_isClassMember(TRUE);

                if (!multiplicity().isEmpty())
                    rel->set_Multiplicity(multiplicity());

                if (!defaultValue().isEmpty())
                    rel->set_DefaultValue(defaultValue());

                if (!javaAnnotations().isEmpty())
                    rel->set_JavaAnnotations(javaAnnotations());

                if (!stereotype().isEmpty())
                    rel->set_Stereotype(stereotype());

                rel->set_JavaDecl(javaDecl());

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
