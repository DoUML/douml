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

using namespace std;
#endif

#include "UmlAttribute.h"
#include "UmlPackage.h"
#include "Class.h"
#include "UmlClass.h"
#include "UmlCom.h"
#include "CppSettings.h"
#include "Lex.h"
#include "Logging/QsLog.h"
#ifdef REVERSE
#include "Statistic.h"
# ifdef ROUNDTRIP
# include "UmlRelation.h"
# endif
#endif

#ifdef ROUNDTRIP
UmlAttribute * UmlAttribute::search_attr(UmlClass * cl, const WrapperStr & name)
{
    UmlItem * x = cl->search_for_att_rel(name);

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
                           const WrapperStr & type, const WrapperStr & modifier,
                           const WrapperStr & pretype, const WrapperStr & array,
                           aVisibility visibility, bool staticp, bool constp,
                           bool typenamep, bool mutablep, bool volatilep,
                           const WrapperStr & bitfield, const WrapperStr & value,
                           WrapperStr comment, WrapperStr description
#ifdef ROUNDTRIP
                           , bool roundtrip, QList<UmlItem *> & expected_order
#endif
                          )
{
#ifdef DEBUG_DOUML
    QLOG_INFO() << "ATTRIBUTE '" << name << "' type '" << type << "' modifier '" << modifier << "' array '" << array << "'\n";
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
        ((at = search_attr(cl, name)) == 0)) {
#endif
        at = UmlBaseAttribute::create(cl, name);

        if (at == 0) {
            UmlCom::trace(WrapperStr("<font face=helvetica><b>cannot add attribute <i>")
                          + name + "</i> in <i>" + WrapperStr(cl->name())
                          + "</i></b></font><br><hr>");
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

        bool pfunc = (type.find('$') != -1);
        UmlTypeSpec typespec;
        WrapperStr typeform;
        WrapperStr stereotype;

        if (! pfunc) {
            typeform = (pretype.isEmpty())
                       ? WrapperStr("${type}")
                       : pretype + " ${type}";

            container->compute_type(type, typespec, typeform);
        }
        else {
            typespec.explicit_type = type.simplifyWhiteSpace();

            int index = typespec.explicit_type.find("${name}");

            if (index != -1)
                typespec.explicit_type.remove(index, 7);
        }

        WrapperStr decl = CppSettings::attributeDecl("");
        int index = decl.find("${type}");

        if ((index == -1) ||
            (decl.find("${const}") == -1) ||
            (decl.find("${name}") == -1) ||
            (decl.find("${mutable}") == -1) ||
            (decl.find("${volatile}") == -1) ||
            (decl.find(';') == -1)) {
            decl = "    ${comment}${static}${mutable}${volatile}${const}${type} ${name}${value};";
            index = decl.find("${type}");
        }

        if (pfunc)
            decl.replace(index, decl.find("${name}") + 7 - index, type);
        else {
            if (!modifier.isEmpty())
                decl.insert(index + 7, (const char *)(WrapperStr(" ") + modifier));

            if (typeform != "${type}")
                decl.replace(index, 7, typeform);
            else if (typespec.type == 0) {
                WrapperStr t = typespec.explicit_type;
                int index2 = 0;

                if (!t.isEmpty() && (t.at(t.length() - 1) == ">") && ((index2 = t.find('<')) > 0))
                {
                    stereotype = t.left(index2);
                    typespec.explicit_type =
                        // may be a,b ...
                        t.mid(index2 + 1, t.length() - 2 - index2);
                    decl.replace(index, 7, "${stereotype}<${type}>");
                }
            }

            if (!array.isEmpty())
                decl.insert(decl.find("${name}") + 7, "${multiplicity}");

            if (!bitfield.isEmpty())
                decl.insert(decl.find(';'), (const char *)(WrapperStr(" : ") + bitfield));
        }

        if (typenamep) {
            int index = decl.find("${const}") + 8; // find cannot return -1
            int index2 = decl.find("${mutable}") + 10; // find cannot return -1
            int index3 = decl.find("${volatile}") + 11; // find cannot return -1

            if (index2 > index) index = index2;

            if (index3 > index) index = index3;

            decl.insert(index, "typename ");
        }

        if (!value.isEmpty() && ((index = decl.find("${value}")) != -1))
            decl.insert(index + 2,  "h_");

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

            if (at->isReadOnly() != constp) {
                at->set_isReadOnly(constp);
                container->set_updated();
            }

            if (at->isCppMutable() != mutablep) {
                at->set_isCppMutable(mutablep);
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

            if (neq(at->multiplicity(), array)) {
                at->set_Multiplicity(array);
                container->set_updated();
            }

            if (!staticp) {
                WrapperStr v = at->defaultValue();

                if (!v.isEmpty() && (((const char *) v)[0] == '='))
                    v = v.mid(1);

                if (nequal(v, value)) {
                    at->set_DefaultValue(value);
                    container->set_updated();
                }
            }

            if (at->visibility() != visibility) {
                at->set_Visibility(visibility);
                container->set_updated();
            }

            if (!stereotype.isEmpty()) {
                WrapperStr cppst;

                if (!at->stereotype().isEmpty())
                    cppst = CppSettings::relationAttributeStereotype(at->stereotype());

                if (cppst != stereotype) {
                    at->set_Stereotype(stereotype);
                    container->set_updated();
                }
            }

            if (!at->type().equal(typespec)) {
                at->set_Type(typespec);
                container->set_updated();
            }

            if (neq(at->cppDecl(), decl)) {
                at->set_CppDecl(decl);
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

            if (constp)
                at->set_isReadOnly(TRUE);

            if (mutablep)
                at->set_isCppMutable(TRUE);

            if (volatilep)
                at->set_isVolatile(TRUE);

            if (staticp)
                at->set_isClassMember(TRUE);

            if (!array.isEmpty())
                at->set_Multiplicity(array);

            if (! value.isEmpty())
                at->set_DefaultValue(value);

            at->set_Visibility(visibility);

            if (! stereotype.isEmpty())
                at->set_Stereotype(stereotype);

            at->set_Type(typespec);

            at->set_CppDecl(decl);

#ifdef ROUNDTRIP

            if (roundtrip)
                expected_order.append(at);
        }

#endif

        return TRUE;
    }
