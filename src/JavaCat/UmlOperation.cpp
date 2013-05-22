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
#include <Q3ValueList>
//Added by qt3to4:
#include <Q3PtrList>

using namespace std;
#endif
#ifdef ROUNDTRIP
#include <stdlib.h>
#endif
#include <qstringlist.h>

#include "UmlOperation.h"
#include "UmlPackage.h"
#include "Class.h"
#include "UmlClass.h"
#include "UmlAttribute.h"
#include "JavaCatWindow.h"
#include "JavaSettings.h"
#include "Lex.h"
#ifdef REVERSE
#include "Statistic.h"
# ifdef ROUNDTRIP
# include "UmlCom.h"
# endif
#endif

bool UmlOperation::new_one(Class * container, const WrapperStr & name,
                           const Q3ValueList<FormalParameterList> & tmplts,
                           const WrapperStr & oper_templ,
                           UmlTypeSpec & type, WrapperStr str_actuals,
                           UmlClass * first_actual_class, WrapperStr type_def,
                           aVisibility visibility,
                           bool finalp, bool abstractp, bool staticp,
                           bool nativep, bool strictfp, bool synchronizedp,
                           const WrapperStr & array, WrapperStr comment,
                           WrapperStr description, WrapperStr annotation
#ifdef ROUNDTRIP
                           , bool roundtrip, QList<UmlItem *> & expected_order
#endif
                          )
{
    // the "(" was read

#ifdef TRACE
    QLOG_INFO() << "OPERATION '" << name << "'\n";
#endif

    UmlClass * cl = container->get_uml();
    UmlOperation * op;
#ifdef ROUNDTRIP
    bool may_roundtrip = roundtrip &&
                         (!container->from_libp() ||	(visibility != PrivateVisibility));
    UmlTypeSpec return_type;
    Q3ValueList<UmlParameter> params;
    Q3ValueList<UmlTypeSpec> exceptions;
    WrapperStr body;

    if (may_roundtrip)
#else
    if (
# ifdef REVERSE
        container->from_libp() &&
# endif
        (visibility == PrivateVisibility))
#endif
        op = 0;
    else {
        op = UmlBaseOperation::create(cl, name);

        if (op == 0) {
            JavaCatWindow::trace(WrapperStr("<font face=helvetica><b>cannot add operation <i>")
                                 + name + "</i> in <i>" + cl->name()
                                 + "</i></b></font><br>");
            return FALSE;
        }

#ifndef ROUNDTRIP
# if defined(REVERSE)
        Statistic::one_operation_more();
# endif
#endif
    }

    WrapperStr def;

#ifdef ROUNDTRIP

    if (may_roundtrip || (op != 0)) {
#else

    if (op != 0) {
        op->set_Visibility(visibility);

        if (staticp) op->set_isClassMember(TRUE);

        if (abstractp) op->set_isAbstract(TRUE);

        if (finalp) op->set_isJavaFinal(TRUE);

        if (synchronizedp) op->set_isJavaSynchronized(TRUE);

        if (! annotation.isEmpty()) op->set_JavaAnnotations(annotation);

#endif

        def = JavaSettings::operationDef();

        int index;

        if (((index = def.find("${(}")) == -1) ||
            ((index = def.find("${)}", index + 4)) == -1) ||
            ((index = def.find("${throws}", index + 4)) == -1) ||
            (def.find("${body}", index + 9) == -1) ||
            ((index = def.find("${type}")) == -1)) {
            // use a definition where ${body] is not indented
            def = "  ${comment}${@}${visibility}${final}${static}${abstract}${synchronized}${type} ${name}${(}${)}${throws}${staticnl}{\n${body}}\n";
            index = def.find("${type}");
        }

        if (!array.isEmpty())
            def.insert(index + 7, (const char *)array);

        if (nativep) {
            def.insert(index, "native ");
            index += 7;

            // no body
            int index2 = def.find("${throws}", index + 7);

            if (index2 != -1) {
                def.resize(index2 + 12);
                def[index2 + 9] = ';';
                def[index2 + 10] = '\n';
            }
        }

        if (strictfp) {
            def.insert(index, "strictfp ");
            index += 9;
        }

        if (! oper_templ.isEmpty())
            def.insert(index, (const char *)(oper_templ + " "));

        if (name == cl->name()) {
            // constructor, remove useless ${}
            if ((index = def.find("${static}")) != -1)
                def.remove(index, 9);

            if ((index = def.find("${type}")) != -1)
                def.remove(index, (((const char *) def)[index + 7] == ' ') ? 8 : 7);

            if ((index = def.find("${final}")) != -1)
                def.remove(index, 8);

            if ((index = def.find("${abstract}")) != -1)
                def.remove(index, 11);
        }

        if (type.type != 0) {
            UmlClass::manage_generic(def, type, str_actuals, "${type}");
#ifdef ROUNDTRIP
            return_type = type;
#else
            op->set_ReturnType(type);
#endif
        }
        else if (first_actual_class != 0) {
#ifndef ROUNDTRIP
            UmlTypeSpec return_type;
#endif

            return_type.type = first_actual_class;
            def.replace(def.find("${type}"), 7, type_def);
#ifndef ROUNDTRIP
            op->set_ReturnType(return_type);
#endif
        }
        else if (!type.explicit_type.isEmpty()) {
            // not a contructor
#ifdef ROUNDTRIP
            return_type = type;
#else
            op->set_ReturnType(type);
#endif
        }
    }

    // parameters

    unsigned rank = 0;
    UmlParameter param;

#ifdef ROUNDTRIP

    if (may_roundtrip)
        while (read_param(container, rank++, tmplts, param, def, FALSE))
            params.append(param);
    else
#endif
        while (read_param(container, rank, tmplts, param, def, op == 0)) {
            if ((op != 0) && ! op->addParameter(rank, param)) {
                JavaCatWindow::trace(WrapperStr("<font face=helvetica><b>cannot add param <i>")
                                     + name + "</i> in <i>" + cl->name()
                                     + "</i></b></font><br>");
# ifdef TRACE
                QLOG_INFO() << "ERROR cannot add param '" << param.name << "' type '" << param.type.Type() << '\n';
# endif
                return FALSE;
            }

            rank += 1;
        }

    WrapperStr s = Lex::read_word();

    if (!s.isEmpty() && (*((const char *) s) == '[')) {
#ifdef ROUNDTRIP

        if (may_roundtrip)
#else
        if (op != 0)
#endif
            // do not place it at the same place
            def.insert(def.find("${type}") + 7, (const char *)s);

        s = Lex::read_word();
    }

    if (s.isEmpty()) {
        Lex::premature_eof();
        return FALSE;
    }

    if (s == "throws") {

        // throws

        rank = 0;

        for (;;) {
            if ((s = Lex::read_word()).isEmpty()) {
                Lex::premature_eof();
                return FALSE;
            }

#ifdef ROUNDTRIP

            if (may_roundtrip) {
                UmlTypeSpec typespec;

                container->compute_type(s, typespec, tmplts);
                exceptions.append(typespec);
            }
            else
#endif
                if (op != 0) {
                    UmlTypeSpec typespec;

                    container->compute_type(s, typespec, tmplts);

                    if (! op->addException(rank++, typespec)) {
# ifdef TRACE
                        QLOG_INFO() << "cannot add exception " << s << '\n';
# endif
                        return FALSE;
                    }
                }

            if (((s = Lex::read_word()) == "{") || (s == ";"))
                break;

            if (s != ",") {
                Lex::error_near(s, " ',' expected");
                return FALSE;
            }
        }
    }

    // definition

    if (abstractp || nativep ||
        (cl->stereotype() == "interface") ||
        (cl->stereotype() == "@interface")) {
        if ((s == "default") && (cl->stereotype() == "@interface")) {
            int index = def.find("${)}");

            Lex::mark();
            s = Lex::read_word();

            if (s == "{") {
                int level = 1;
                char c;

                for (;;) {
                    if ((c = Lex::read_word_bis()) == 0)
                        return FALSE;
                    else if (c == '{')
                        level += 1;
                    else if ((c == '}') && (--level == 0))
                        break;
                }

                s = Lex::region();
            }

            def.insert(index + 4, (const char *)(" default" + s));
            s = Lex::read_word();
        }

        if (s != ";") {
            Lex::error_near(s, " ';' expected");
            return FALSE;
        }

#ifdef REVERSE
# ifndef ROUNDTRIP

        if ((op != 0) && !container->from_libp())
            op->set_JavaBody(0);

# endif
#endif
    }
    else if (s != "{") {
        Lex::error_near(s, " '{' expected");
        return FALSE;
    }
    else {
        Lex::mark();

        // goto the end of the body

#ifndef ROUNDTRIP
        WrapperStr body;
#endif
        int level = 1;	// '{' already read
        char c;

        for (;;) {
            if ((c = Lex::read_word_bis()) == 0)
                return FALSE;
            else if (c == '{')
                level += 1;
            else if ((c == '}') && (--level == 0))
                break;
        }

#ifdef REVERSE

        if (
# ifdef ROUNDTRIP
            may_roundtrip || (op != 0)
# else
            (op != 0) && !container->from_libp()
# endif
        ) {
            body = Lex::region();
            body.truncate(body.length() - 1);	// remove }

            // remove fist \n
            if (*((const char *) body) == '\n')
                body.remove(0, 1);

            // remove last spaces and tabs
            int ln = body.length();

            while (ln && ((body[ln - 1] == ' ') || (body[ln - 1] == '\t')))
                ln -= 1;

            body.truncate(ln);

            if (!body.isEmpty() && (body[ln - 1] != '\n'))
                body += "\n";

# ifndef ROUNDTRIP
            op->set_JavaBody(body);
            op->set_JavaContextualBodyIndent(FALSE);
# endif
        }

#endif
    }

#ifdef ROUNDTRIP

    if (may_roundtrip) {
        if (((op = already_exist_from_id(container, body)) != 0) ||
            ((op = already_exist(container, name, params)) != 0)) {
            // update already existing operation
            op->set_usefull();

            {
                // remove \r in case of preserve body
                WrapperStr current_body = op->javaBody();
                int index = 0;

                while ((index = current_body.find('\r', index)) != -1)
                    current_body.remove(index, 1);

                if (nequal(current_body, body)) {
                    container->set_updated();
                    op->set_JavaBody(body);
                    op->set_JavaContextualBodyIndent(FALSE);
                }
            }

            if (op->visibility() != visibility) {
                container->set_updated();
                op->set_Visibility(visibility);
            }

            if (op->isClassMember() != staticp) {
                container->set_updated();
                op->set_isClassMember(staticp);
            }

            if (op->isAbstract() != abstractp) {
                container->set_updated();
                op->set_isAbstract(abstractp);
            }

            if (op->isJavaFinal() != finalp) {
                container->set_updated();
                op->set_isJavaFinal(finalp);
            }

            if (op->isJavaSynchronized() != synchronizedp) {
                container->set_updated();
                op->set_isJavaSynchronized(synchronizedp);
            }

            if (nequal(op->javaAnnotations(), annotation)) {
                container->set_updated();
                op->set_JavaAnnotations(annotation);
            }

            if (!op->returnType().equal(return_type)) {
                container->set_updated();
                op->set_ReturnType(return_type);
            }

            Q3ValueList<UmlParameter>::Iterator itp1;
            const Q3ValueList<UmlParameter> old_params = op->params();
            Q3ValueList<UmlParameter>::ConstIterator itp2;

            for (rank = 0, itp1 = params.begin(), itp2 = old_params.begin();
                 (itp1 != params.end()) && (itp2 != old_params.end());
                 ++itp1, ++itp2, rank += 1) {
                UmlParameter & p1 = *itp1;
                const UmlParameter & p2 = *itp2;

                if ((p1.name != p2.name) ||
                    nequal(p1.default_value, p2.default_value) ||
                    !p1.type.equal(p2.type)) {
                    if (p1.dir != InputDirection)
                        p1.dir = p2.dir;

                    op->replaceParameter(rank, p1);
                    container->set_updated();
                }
                else if ((p1.dir == InputDirection) && (p2.dir != InputDirection)) {
                    op->replaceParameter(rank, p1);
                    container->set_updated();
                }
            }

            if (itp1 != params.end()) {
                // have missing params
                container->set_updated();

                do {
                    op->addParameter(rank, *itp1);
                    itp1++;
                    rank += 1;
                }
                while (itp1 != params.end());
            }
            else if (itp2 != old_params.end()) {
                // have extra params
                container->set_updated();

                do {
                    op->removeParameter(rank);
                    itp2++;
                }
                while (itp2 != old_params.end());
            }

            Q3ValueList<UmlTypeSpec>::ConstIterator ite1;
            const Q3ValueList<UmlTypeSpec> old_exceptions = op->exceptions();
            Q3ValueList<UmlTypeSpec>::ConstIterator ite2;

            for (rank = 0, ite1 = exceptions.begin(), ite2 = old_exceptions.begin();
                 (ite1 != exceptions.end()) && (ite2 != old_exceptions.end());
                 ++ite1, ++ite2, rank += 1) {
                const UmlTypeSpec & e1 = *ite1;

                if (!e1.equal(*ite2)) {
                    op->replaceException(rank, e1);
                    container->set_updated();
                }
            }

            if (ite1 != exceptions.end()) {
                // have missing exceptions
                container->set_updated();

                do {
                    op->addException(rank, *ite1);
                    ite1++;
                    rank += 1;
                }
                while (ite1 != exceptions.end());
            }
            else if (ite2 != old_exceptions.end()) {
                // have extra exceptions
                container->set_updated();

                do {
                    op->removeException(rank);
                    ite2++;
                }
                while (ite2 != old_exceptions.end());
            }

            if (neq(def, op->javaDecl())) {
                container->set_updated();
                op->set_JavaDecl(def);
            }

            Lex::clear_comments();	// params & body comments
            Lex::finish_line();

            if (def.find("${description}") != -1) {
                if (nequal(op->description(), description)) {
                    container->set_updated();
                    op->set_Description(description);
                }
            }
            else if (nequal(op->description(), Lex::simplify_comment(comment))) {
                op->set_Description(comment); // comment was set
                container->set_updated();
            }

            expected_order.append(op);

            return TRUE;
        }

        // operation doesn't yet exist
        container->set_updated();
        op = UmlBaseOperation::create(cl, name);

        if (op == 0) {
            JavaCatWindow::trace(WrapperStr("<font face=helvetica><b>cannot add operation <i>")
                                 + name + "</i> in <i>" + cl->name()
                                 + "</i></b></font><br>");
            throw 0;
        }

        expected_order.append(op);

        Q3ValueList<UmlParameter>::ConstIterator itp;

        for (rank = 0, itp = params.begin(); itp != params.end(); ++itp)
            op->addParameter(rank++, *itp);

        Q3ValueList<UmlTypeSpec>::ConstIterator ite;

        for (rank = 0, ite = exceptions.begin(); ite != exceptions.end(); ++ite)
            op->addException(rank++, *ite);
    }

    if (op != 0) {
        op->set_JavaContextualBodyIndent(FALSE);
        op->set_Visibility(visibility);

        if (staticp) op->set_isClassMember(TRUE);

        if (abstractp) op->set_isAbstract(TRUE);

        if (finalp) op->set_isJavaFinal(TRUE);

        if (synchronizedp) op->set_isJavaSynchronized(TRUE);

        if (! annotation.isEmpty()) op->set_JavaAnnotations(annotation);

        op->set_JavaBody(body);
        op->set_ReturnType(return_type);

        if (def != JavaSettings::operationDef())
            op->set_JavaDecl(def);
    }

#else

    if ((op != 0) && (def != JavaSettings::operationDef()))
        op->set_JavaDecl(def);

#endif

    Lex::clear_comments();	// params & body comments
    Lex::finish_line();

    if (!comment.isEmpty())
        if (op != 0)
            op->set_Description((def.find("${description}") != -1)
                                ? description : Lex::simplify_comment(comment));

    return TRUE;
}

bool UmlOperation::read_param(Class * container, unsigned rank,
                              const Q3ValueList<FormalParameterList> & tmplts,
                              UmlParameter & param, WrapperStr & def, bool bypass)
{
#ifdef TRACE
    QLOG_INFO() << "UmlOperation::manage_param " << rank << "\n";
#endif

    bool finalp = FALSE;
    bool in = FALSE;
    bool ellipsis = FALSE;
    WrapperStr array;
    bool type_read = FALSE;
    Q3ValueList<UmlTypeSpec> actuals;
    WrapperStr str_actuals;
    WrapperStr annotation;

    param.name = param.default_value = 0;

    WrapperStr s = Lex::read_word();

#ifdef TRACE
    QLOG_INFO() << "commence par " << s << '\n';
#endif

    if (s == ")")
        return FALSE;

    for (;;) {
        if (s.isEmpty()) {
            Lex::premature_eof();
            return FALSE;
        }
        else if (s == "final")
            finalp = TRUE;
        else if ((s == "void") || (s == "byte") || (s == "char") ||
                 (s == "short") || (s == "int") || (s == "long") ||
                 (s == "float") || (s == "double")) {
            if (type_read) {
                Lex::error_near(s);
                return FALSE;
            }

            param.type.type = 0;
            param.type.explicit_type = s;
            type_read = TRUE;
            in = TRUE;
        }
        else if ((s == ")") || (s == ",")) {
            if (param.name.isEmpty() && !type_read) {
                Lex::error_near(s);
                return FALSE;
            }

            if (s == ")")
                Lex::unread_word(s);

            if (! bypass) {
                param.dir = (finalp || in)
                            ? InputDirection : InputOutputDirection;

                WrapperStr s;

                if (rank != 0)
                    s = ", ";

                if (! annotation.isEmpty())
                    s += annotation + " ";

                if (finalp)
                    s += "final ";

                if ((param.type.type != 0) &&
                    !param.type.explicit_type.isEmpty())
                    s += param.type.explicit_type;
                else {
                    s += "${t";
                    s += WrapperStr().setNum(rank);
                    s += "}";

                    if (param.type.type != 0)
                        s += str_actuals;
                }

                s += array;
                s += (ellipsis) ? " ... ${p" : " ${p";
                s += WrapperStr().setNum(rank);
                s += "}";
                def.insert(def.find("${)}"), 	// cannot be -1
                           (const char *)s);
            }

            return TRUE;
        }
        else if (Lex::identifierp(s)) {
            if (!type_read) {
                while (s.at(s.length() - 1) == ".") {
                    // type on several lines, managed in this case
                    WrapperStr s2 = Lex::read_word();

                    if (Lex::identifierp(s2))
                        s += s2;
                    else {
                        Lex::error_near(s, " identifier expected");
                        return FALSE;
                    }
                }

#ifdef TRACE
                QLOG_INFO() << "type = '" << s << "...'\n";
#endif

                if (! bypass) {
                    WrapperStr dummy;

                    container->read_type(param.type, 0, tmplts, 0, str_actuals, s,
                                         0, dummy, dummy);

                    if (param.type.explicit_type == "String")
                        // at least for it !
                        in = TRUE;
                }
                else
                    Lex::bypass_type(s);

                type_read = TRUE;
            }
            else if (param.name.isEmpty()) {
                if (s == "...")
                    ellipsis = TRUE;
                else {
                    param.name = s;
#ifdef TRACE
                    QLOG_INFO() << "name = '" << param.name << "'\n";
#endif
                }
            }
            else {
                Lex::error_near(s);
#ifdef TRACE
                QLOG_INFO() << "ERROR '" << s << "' alors qu a deja le type et le nom '" << param.name << "'\n";
#endif
                return FALSE;
            }
        }
        else if (*((const char *) s) == '@')
            annotation = s;
        else if (*((const char *) s) == '[') {
            in = FALSE;
            array = s;
        }
        else {
            Lex::error_near(s);
#ifdef TRACE
            QLOG_INFO() << "ERROR : '" << s << "'\n";
#endif
            return FALSE;
        }

        s = Lex::read_word();
    }
}

void UmlOperation::skip_body(int level)
{
    char e;

    while ((e = Lex::read_word_bis()) != 0) {	// not end of file
        if (e == '}') {
            if (--level == 0)
                // end of body
                break;
        }
        else if (e == ';') {
            if (level == 0)
                // no body, probably a declaration
                break;
        }
        else if (e == '{')
            level += 1;
    }

    Lex::clear_comments();
}

char UmlOperation::skip_expr(int level)
{
    char e;
    int templ_level = 0;

    for (;;) {
        e = Lex::read_word_bis(templ_level != 0);

        if (e == 0)
            break;
        else if ((e == ';') || (e == ',')) {
            if (level == 0)
                break;
        }
        else {
            switch (e) {
            case '{':
            case '(':
                level += 1;
                break;

            case '}':
            case ')':
                level -= 1;
                break;

            case '<':
                if ((level == 0) || (templ_level != 0)) {
                    level += 1;
                    templ_level += 1;
                }

                break;

            case '>':
                if (templ_level != 0) {
                    level -= 1;
                    templ_level -= 1;
                }
            }
        }
    }

    Lex::clear_comments();
    return e;
}

#ifdef ROUNDTRIP
UmlOperation  * UmlOperation::already_exist_from_id(Class * container, WrapperStr & body)
{
    const char * BodyPrefix = "// Bouml preserved body begin ";
    const char * BodyPostfix = "// Bouml preserved body end ";
    const int BodyPrefixLength = 30;
    const int BodyPostfixLength = 28;

    int index = body.find(BodyPrefix);

    if (index != -1) {
        const char * b1 = ((const char *) body) + index + BodyPrefixLength;
        char * b2;
        long id = strtol(b1, &b2, 16);

        if (b2 != (b1 + 8)) {
            QString err = "<font color =\"red\"> Error in " + Lex::filename() +
                          " before line " + QString::number(Lex::line_number()) +
                          " : invalid preserve body identifier</font><br>";

            UmlCom::trace(err);
            throw 0;
        }

        if (*b2 == '\r')
            b2 += 1;

        if (*b2 == '\n')
            b2 += 1;
        else {
            QString err = "<font color =\"red\"> Error in " + Lex::filename() +
                          " before line " + QString::number(Lex::line_number()) +
                          " : invalid preserve body block, end of line expected</font><br>";

            UmlCom::trace(err);
            throw 0;
        }

        const char * e;

        if (((e = strstr(b2, BodyPostfix)) == 0) ||
            (strncmp(e + BodyPostfixLength, b1, 8) != 0)) {
            QString err = "<font color =\"red\"> Error in " + Lex::filename() +
                          " before line " + QString::number(Lex::line_number()) +
                          " : invalid preserve body block, wrong balanced</font><br>";

            UmlCom::trace(err);
            throw 0;
        }

        while ((e != b2) && (e[-1] != '\n'))
            e -= 1;

        body = body.mid(b2 - (const char *) body, e - b2);

        UmlOperation * op = (UmlOperation *) UmlBaseItem::from_id((unsigned) id, anOperation);

        if (op != 0) {
            if (!op->is_useless()) {
                QString err = "<font color =\"red\"> Error in " + Lex::filename() +
                              " before line " + QString::number(Lex::line_number()) +
                              " : preserve body block identifier used twice</font><br>";

                UmlCom::trace(err);
                throw 0;
            }

            if ((op->parent() == container->get_uml()) &&
                // currently get/set are removed then recreated
                (op->getOf() == 0) &&
                (op->setOf() == 0))
                return op;
        }
    }

    return 0;
}

UmlOperation * UmlOperation::already_exist(Class * container, const WrapperStr & name,
        Q3ValueList<UmlParameter> & params)
{
    const Q3PtrVector<UmlItem> & ch = container->get_uml()->UmlItem::children();
    UmlItem ** v = ch.data();
    UmlItem ** const vsup = v + ch.size();
    QList<UmlOperation *> opers;

    for (; v != vsup; v += 1)
        if (((*v)->kind() == anOperation) &&
            ((*v)->name() == name) &&
            ((UmlOperation *) *v)->is_useless())
            opers.append((UmlOperation *) *v);

    switch (opers.count()) {
    case 0:
        return 0;

    case 1:
        // suppose it is this one
        // even don't know if it is placed later in file
        return opers.first();

    default:
        break;
    }

    QList<UmlOperation *> same_names;

    // search for operation having the same params name and number
    foreach (UmlOperation *op, opers) {
        Q3ValueList<UmlParameter> ps = op->params();
        Q3ValueList<UmlParameter>::ConstIterator it1;
        Q3ValueList<UmlParameter>::ConstIterator it2;
        bool same_type = TRUE;
        bool same_name = TRUE;

        for (it1 = params.begin(), it2 = ps.begin();
             (it1 != params.end()) && (it2 != ps.end());
             ++it1, ++it2) {
            const UmlParameter & p1 = *it1;
            const UmlParameter & p2 = *it2;

            if (p1.name != p2.name) {
                same_name = FALSE;
                break;
            }

            if (!p1.type.equal(p2.type))
                same_type = FALSE;
        }

        if (same_name && (it1 == params.end()) && (it2 == ps.end())) {
            if (same_type)
                // perhaps several has the same number of param
                // and same param names and type because of []
                // not managed here, suppose the right is this one
                return op;

            same_names.append(op);
        }
    }

    if (same_names.count() == 1) {
        // only one having the same number of param
        // and same param names (type changed)
        // suppose this one
        return same_names.first();
    }

    // suppose not find
    return 0;
}
#endif

