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

#include "UmlAttribute.h"
#include "UmlPackage.h"
#include "UmlOperation.h"
#include "Package.h"
#include "UmlClass.h"
#include "Class.h"
#include "JavaCatWindow.h"
#include "JavaSettings.h"
#include "Lex.h"
#ifdef REVERSE
#include "Statistic.h"
# ifdef ROUNDTRIP
# include "UmlRelation.h"
# endif
#endif

#ifdef ROUNDTRIP
static UmlAttribute * search_attr(Class * container, const WrapperStr & name)
{
    UmlItem * x = container->get_uml()->search_for_att_rel(name);

    if (x == 0)
        return 0;
    else if (x->kind() == anAttribute)
        return (UmlAttribute *) x;

    switch (((UmlRelation *) x)->relationKind()) {
    case aDirectionalAssociation:
    case aDirectionalAggregation:
    case aDirectionalAggregationByValue:
        x->deleteIt();
        break;

    case anAssociation:
    case anAggregation:
    case anAggregationByValue:
        if (x == ((UmlRelation *) x)->side(TRUE))
            ((UmlRelation *) x)->side(FALSE)->set_unidir();
        else
            ((UmlRelation *) x)->set_unidir();

        break;

    default:
        break;
    }

    return 0;
}

#endif

bool UmlAttribute::new_one(Class * container, const WrapperStr & name,
                           UmlTypeSpec typespec, aVisibility visibility,
                           bool staticp, bool finalp, bool transientp,
                           bool volatilep, const WrapperStr & array,
                           const WrapperStr & value, WrapperStr comment,
                           WrapperStr description, WrapperStr annotation
#ifdef ROUNDTRIP
                           , bool roundtrip, QList<UmlItem *> & expected_order
#endif
                          )
{
#ifdef TRACE
    QLOG_INFO() << "ATTRIBUTE '" << name << "'\n";
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
    UmlAttribute * at;

#ifdef ROUNDTRIP
    bool created;

    if (!roundtrip ||
        ((at = search_attr(container, name)) == 0)) {
#endif
        at = UmlBaseAttribute::create(cl, name);

        if (at == 0) {
            JavaCatWindow::trace(WrapperStr("<font face=helvetica><b>cannot add attribute <i>")
                                 + name + "</i> in <i>" + cl->name()
                                 + "</i></b></font><br>");
            return FALSE;
        }

#ifdef REVERSE
# ifndef ROUNDTRIP
        Statistic::one_attribute_more();
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

        WrapperStr decl = JavaSettings::attributeDecl("");
        int index = decl.find("${type}");

        if ((index == -1) || (decl.find("${name}") == -1)) {
            decl = "  ${comment}${@}${visibility}${static}${final}${transient}${volatile}${type} ${name}${value};";
            index = decl.find("${type}");
        }

#ifdef ROUNDTRIP

        if (roundtrip && !created) {
            if (decl.find("${description}") != -1) {
                if (nequal(at->description(), description)) {
                    at->set_Description(description);
                    container->set_updated();
                }
            }
            else if (nequal(at->description(), Lex::simplify_comment(comment))) {
                at->set_Description(comment); // comment was set
                container->set_updated();
            }

            if (at->isReadOnly() != finalp) {
                at->set_isReadOnly(finalp);
                container->set_updated();
            }

            if (at->isJavaTransient() != transientp) {
                at->set_isJavaTransient(transientp);
                container->set_updated();
            }

            if (at->isVolatile() != volatilep) {
                at->set_isVolatile(volatilep);
                container->set_updated();
            }

            if (at->isClassMember() != staticp) {
                at->set_isClassMember(staticp);
                container->set_updated();
            }

            if (!array.isEmpty())
                decl.insert(index + 7, "${multiplicity}");

            if (neq(at->multiplicity(), array)) {
                at->set_Multiplicity(array);
                container->set_updated();
            }

            WrapperStr v = at->defaultValue();

            if (!v.isEmpty() && (((const char *) v)[0] == '='))
                v = v.mid(1);

            if (nequal(v, value)) {
                at->set_DefaultValue(value);
                container->set_updated();
            }

            if (nequal(at->javaAnnotations(), annotation)) {
                at->set_JavaAnnotations(annotation);
                container->set_updated();
            }

            WrapperStr stereotype;
            bool force_ste = FALSE;

            if (cl->stereotype() == "enum") {
                stereotype = "attribute";
                force_ste = TRUE;
            }
            else if (typespec.type == 0) {
                WrapperStr t = typespec.explicit_type;
                int index2;

                if (!t.isEmpty() &&
                    (t.at(t.length() - 1) == ">") &&
                    ((index2 = t.find('<')) > 0)) {
                    stereotype = t.left(index2);
                    typespec.explicit_type =
                        // may be a,b ...
                        t.mid(index2 + 1, t.length() - 2 - index2);
                    decl.replace(index, 7, "${stereotype}<${type}>");
                    force_ste = TRUE;
                }
            }

            if (at->visibility() != visibility) {
                at->set_Visibility(visibility);
                container->set_updated();
            }

            if (neq(at->stereotype(), stereotype)) {
                WrapperStr jst;

                if (! at->stereotype().isEmpty())
                    jst = JavaSettings::relationAttributeStereotype(at->stereotype());

                if ((force_ste) ? (jst != stereotype) : (jst == "attribute")) {
                    at->set_Stereotype(stereotype);
                    container->set_updated();
                }
            }

            if (neq(at->javaDecl(), decl)) {
                at->set_JavaDecl(decl);
                container->set_updated();
            }

            if (!at->type().equal(typespec)) {
                at->set_Type(typespec);
                container->set_updated();
            }

            at->set_usefull();

            expected_order.append(at);
        }
        else {
#endif

            if (!comment.isEmpty())
                at->set_Description((decl.find("${description}") != -1)
                                    ? description : Lex::simplify_comment(comment));

            if (finalp)
                at->set_isReadOnly(TRUE);

            if (transientp)
                at->set_isJavaTransient(TRUE);

            if (volatilep)
                at->set_isVolatile(TRUE);

            if (staticp)
                at->set_isClassMember(TRUE);

            if (!array.isEmpty()) {
                decl.insert(index + 7, "${multiplicity}");
                at->set_Multiplicity(array);
            }

            if (! value.isEmpty())
                at->set_DefaultValue(value);

            if (! annotation.isEmpty())
                at->set_JavaAnnotations(annotation);

            if ((typespec.type == 0) && (cl->stereotype() != "enum")) {
                WrapperStr t = typespec.explicit_type;
                int index2 = 0;

                if (!t.isEmpty() &&
                        (t.at(t.length() - 1) == ">") &&
                    ((index2 = t.find('<')) > 0))
                {
                    at->set_Stereotype(t.left(index2));
                    typespec.explicit_type =
                        // may be a,b ...
                        t.mid(index2 + 1, t.length() - 2 - index2);
                    decl.replace(index, 7, "${stereotype}<${type}>");
                }
            }

            at->set_Visibility(visibility);

            if (cl->stereotype() == "enum") {
                at->set_JavaDecl(decl);
                at->set_Stereotype("attribute");
            }
            else if (decl != JavaSettings::attributeDecl(""))
                at->set_JavaDecl(decl);

            at->set_Type(typespec);

#ifdef ROUNDTRIP

            if (roundtrip)
                expected_order.append(at);
        }

#endif

        return TRUE;
    }

    bool UmlAttribute::manage_enum_item(WrapperStr name, UmlClass * cl
#ifdef ROUNDTRIP
                                        , bool roundtrip,
                                        QList<UmlItem *> & expected_order
#endif
                                       )
    {
        WrapperStr comment = Lex::get_comments();
        WrapperStr description = Lex::get_description();
        UmlAttribute * item = 0;	// initialize to avoid warning
#ifdef ROUNDTRIP
        Class * container = 0;	// initialize to avoid warning
        bool created = FALSE;		// initialize to avoid warning
#endif

        if (!Package::scanning()) {
#ifdef ROUNDTRIP
            container = cl->get_class();

            if (!roundtrip ||
                ((item = search_attr(container, name)) == 0)) {
#endif

                if ((item = UmlBaseAttribute::create(cl, name)) == 0) {
                    JavaCatWindow::trace(WrapperStr("<font face=helvetica><b>cannot add enum item <i>")
                                         + name + "</i> in <i>" + cl->name()
                                         + "</i></b></font><br>");
                    return FALSE;
                }

                item->set_Visibility(PublicVisibility);
#ifdef ROUNDTRIP

                if (roundtrip)
                    container->set_updated();

                created = TRUE;
            }

#endif
        }

        Lex::mark();

        WrapperStr aux;
        WrapperStr s;

        if ((s = Lex::read_word()).isEmpty()) {
            if (! Package::scanning())
                Lex::premature_eof();

            return FALSE;
        }
        else if ((s == ";") || (s == "}")) {
            aux = Lex::region();
            Lex::unread_word(s);
        }
        else if (s == ",") {
            aux = Lex::region();
            Lex::finish_line();
            comment = Lex::get_comments(comment);
            description = Lex::get_description(description);
        }
        else if ((s == "(") || (s == "{")) {
            char c = UmlOperation::skip_expr(1);	// goes after opt init and body

            if (c == 0) {
                if (! Package::scanning())
                    Lex::premature_eof();

                return FALSE;
            }

            // c is ',' or ';'
            if (c == ';')
                Lex::unread_word(";");

            aux = Lex::region();
        }
        else {
            if (! Package::scanning())
                Lex::error_near(s);

            return FALSE;
        }

        if (!Package::scanning()) {
            // here aux = opt init and body + final character , ; or }
            WrapperStr decl = JavaSettings::enumItemDecl();
            int index;

            if ((decl.find("${name}") == -1) ||
                ((index = decl.find("${value}")) == -1)) {
                decl = "  ${name}${value},${comment}";
                index = decl.find("${value}");
            }

            //aux.resize(aux.length()); // remove , ; or }, warning resize count \000 //warn_WrapperStr

            if (!aux.stripWhiteSpace().isEmpty())
                decl.replace(index, 8, aux);

#ifdef ROUNDTRIP

            if (roundtrip && !created) {
                if (decl.find("${description}") != -1) {
                    if (nequal(item->description(), description)) {
                        item->set_Description(description);
                        container->set_updated();
                    }
                }
                else if (nequal(item->description(), Lex::simplify_comment(comment))) {
                    item->set_Description(comment); // comment was changed
                    container->set_updated();
                }

                if (neq(item->javaDecl(), decl)) {
                    item->set_JavaDecl(decl);
                    container->set_updated();
                }

                item->set_usefull();

                expected_order.append(item);
            }
            else {
#endif

                if (!comment.isEmpty())
                    item->set_Description((decl.find("${description}") != -1)
                                          ? description : Lex::simplify_comment(comment));

                item->set_JavaDecl(decl);

#ifdef ROUNDTRIP

                if (roundtrip)
                    expected_order.append(item);
            }

#endif
        }

        return TRUE;
    }
