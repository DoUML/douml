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

#include <QTextStream>
//Added by qt3to4:
#include "misc/mystr.h"
#include <QTextStream>
#include <Q3ValueList>
//Added by qt3to4:
#include <Q3PtrList>

#include "UmlRelation.h"
#include "UmlClass.h"
#include "UmlSettings.h"
#include "CppSettings.h"
#include "CppRefType.h"
#include "UmlCom.h"
#include "util.h"

void UmlRelation::compute_dependency(QList<CppRefType *> & dependencies,
                                     const WrapperStr & cl_stereotype,
                                     bool all_in_h)
{
    if (cl_stereotype == "enum")
        return;

    switch (relationKind()) {
    case aDependency:
        if (stereotype() == "friend")
            break;

        CppRefType::add(roleType(), dependencies, cppDecl() != "#include in source",
                        roleType()->isCppExternal());
        break;

    case aGeneralisation:
    case aRealization:
        CppRefType::add(roleType(), dependencies, TRUE);
        break;

    default:
        WrapperStr decl = cppDecl();
        int index;

        if ((index = decl.find("${static}")) != -1)
            decl.remove((unsigned) index, 9);

        if ((index = decl.find("${mutable}")) != -1)
            decl.remove((unsigned) index, 10);

        if ((index = decl.find("${volatile}")) != -1)
            decl.remove((unsigned) index, 11);

        if ((index = decl.find("${const}")) != -1)
            decl.remove((unsigned) index, 8);

        if ((index = decl.find("${multiplicity}")) != -1)
            decl.remove((unsigned) index, 15);

        if ((index = decl.find("${name}")) != -1)
            decl.remove((unsigned) index, 7);

        if ((index = decl.find("${inverse_name}")) != -1)
            decl.remove((unsigned) index, 15);

        if ((index = decl.find("${value}")) != -1)
            decl.remove((unsigned) index, 8);

        if ((index = decl.find("${h_value}")) != -1)
            decl.remove((unsigned) index, 10);

        if ((index = decl.find("${stereotype}")) != -1)
            decl.replace((unsigned) index, 13,
                         CppSettings::relationAttributeStereotype(stereotype()));

        if ((index = decl.find("${association}")) != -1) {
            decl.replace((unsigned) index, 14,
                         association().toString());
        }

        replace_alias(decl);

        UmlTypeSpec type;

        type.type = roleType();
        UmlClassMember::compute_dependency(dependencies, decl, type, all_in_h);
    }
}

void UmlRelation::generate_inherit(const char *& sep, QTextStream & f_h,
                                   const Q3ValueList<UmlActualParameter> & actuals,
                                   const WrapperStr & cl_stereotype)
{
    switch (relationKind()) {
    default:
        break;

    case aGeneralisation:
    case aRealization:
        if ((cl_stereotype == "union") || (cl_stereotype == "enum")) {
            write_trace_header();
            UmlCom::trace(WrapperStr("&nbsp;&nbsp;&nbsp;&nbsp;<font color=\"red\"><b>an <i>")
                          + cl_stereotype + "</i> cannot inherits</b></font><br>");
            incr_warning();
            return;
        }
        else if (cl_stereotype == "typedef") {
            write_trace_header();
            UmlCom::trace("&nbsp;&nbsp;&nbsp;&nbsp;<font color=\"red\"><b>a <i>typedef</i> cannot inherits</b></font><br>");
            incr_warning();
            return;
        }

        UmlClass * role_type = roleType();
        const WrapperStr & other_stereotype = role_type->stereotype();

        if ((other_stereotype == "union") ||
            (other_stereotype == "enum")) {
            write_trace_header();
            UmlCom::trace(WrapperStr("&nbsp;&nbsp;&nbsp;&nbsp;<font color=\"red\"><b>cannot inherits an <i>")
                          + other_stereotype + "</i></b></font><br>");
            incr_warning();
            return;
        }

        f_h << sep;
        sep = ", ";

        if (cppVirtualInheritance())
            f_h << "virtual ";

        aVisibility visibility = (cppVisibility() == DefaultVisibility)
                                 ? this->visibility() : cppVisibility();

        switch (visibility) {
        case PublicVisibility:
        case PackageVisibility:
            f_h << "public ";
            break;

        case ProtectedVisibility:
            f_h << "protected ";
            break;

        default:
            f_h  << "private ";
        }

        const char * p = cppDecl();

        while (*p) {
            if (!strncmp(p, "${type}", 7)) {
                role_type->write(f_h, FALSE, 0, actuals);
                p += 7;
            }
            else if (*p == '@')
                manage_alias(p, f_h);
            else
                f_h << toLocale(p);
        }
    }
}

void UmlRelation::generate_decl(aVisibility & current_visibility, QTextStream & f_h,
                                const WrapperStr & cl_stereotype, WrapperStr indent,
                                BooL & first, bool)
{
    switch (relationKind()) {
    case aDependency:
        if (stereotype() == "friend") {
            Q3ValueList<UmlFormalParameter> formals = roleType()->formals();

            if (! formals.isEmpty()) {
                const char * sep = "  template <";
                Q3ValueList<UmlFormalParameter>::ConstIterator it;

                for (it = formals.begin(); it != formals.end(); ++it) {
                    f_h << sep << (*it).type() << ' ' << (*it).name();
                    sep = ", ";
                }

                f_h << "> ";
            }
            else
                f_h << "  ";

            f_h << "friend " << roleType()->cpp_stereotype() << " ";
            roleType()->write(f_h, FALSE);
            f_h << ";\n";
            first = FALSE;
        }

        break;

    case aGeneralisation:
    case aRealization:
        break;

    default:
        if (!cppDecl().isEmpty()) {
            if (cl_stereotype == "enum") {
                write_trace_header();
                UmlCom::trace("&nbsp;&nbsp;&nbsp;&nbsp;<font color=\"red\"><b>an <i>enum</i> cannot have relation</b></font><br>");
                incr_warning();
                return;
            }

            if (cl_stereotype == "typedef") {
                write_trace_header();
                UmlCom::trace("&nbsp;&nbsp;&nbsp;&nbsp;<font color=\"red\"><b>a <i>typedef</i> cannot have relation</b></font><br>");
                incr_warning();
                return;
            }

            generate_visibility(current_visibility, f_h, first, indent);
            first = FALSE;

            const char * p = cppDecl();
            const char * pp = 0;
            WrapperStr s;

            while ((*p == ' ') || (*p == '\t'))
                indent += toLocale(p);

            if (*p != '#')
                f_h << indent;

            for (;;) {
                if (*p == 0) {
                    if (pp == 0)
                        break;

                    // comment management done
                    p = pp;
                    pp = 0;

                    if (*p == 0)
                        break;

                    if (*p != '#')
                        f_h << indent;
                }

                if (*p == '\n') {
                    f_h << toLocale(p);

                    if (*p && (*p != '#'))
                        f_h << indent;
                }
                else if (*p == '@')
                    manage_alias(p, f_h);
                else if (*p != '$')
                    f_h << toLocale(p);
                else if (!strncmp(p, "${comment}", 10))
                    manage_comment(p, pp, CppSettings::isGenerateJavadocStyleComment());
                else if (!strncmp(p, "${description}", 14))
                    manage_description(p, pp);
                else if (!strncmp(p, "${static}", 9)) {
                    p += 9;

                    if (isClassMember())
                        f_h << "static ";
                }
                else if (!strncmp(p, "${const}", 8)) {
                    p += 8;

                    if (isReadOnly())
                        f_h << "const ";
                }
                else if (!strncmp(p, "${volatile}", 11)) {
                    p += 11;

                    if (isVolatile())
                        f_h << "volatile ";
                }
                else if (!strncmp(p, "${mutable}", 10)) {
                    p += 10;

                    if (isCppMutable())
                        f_h << "mutable ";
                }
                else if (!strncmp(p, "${type}", 7)) {
                    p += 7;
                    roleType()->write(f_h);
                }
                else if (!strncmp(p, "${name}", 7)) {
                    p += 7;
                    f_h << roleName();
                }
                else if (!strncmp(p, "${inverse_name}", 15)) {
                    p += 15;

                    switch (relationKind()) {
                    case anAssociation:
                    case anAggregation:
                    case anAggregationByValue:
                        f_h << side(side(TRUE) != this)->roleName();

                    default:
                        break;
                    }
                }
                else if (!strncmp(p, "${multiplicity}", 15)) {
                    p += 15;

                    const WrapperStr & m = multiplicity();

                    if (!m.isEmpty() && (*((const char *) m) == '['))
                        f_h << m;
                    else
                        f_h << '[' << m << ']';
                }
                else if (!strncmp(p, "${stereotype}", 13)) {
                    p += 13;
                    f_h << CppSettings::relationAttributeStereotype(stereotype());
                }
                else if (!strncmp(p, "${value}", 8))
                    p += 8;
                else if (!strncmp(p, "${h_value}", 10)) {
                    if (!defaultValue().isEmpty() && isClassMember()) {
                        if (need_equal(p, defaultValue()))
                            f_h << " = ";

                        f_h << defaultValue();
                    }

                    p += 10;
                }
                else if (!strncmp(p, "${association}", 14)) {
                    p += 14;
                    UmlClass::write(f_h, association());
                }
                else
                    // strange
                    f_h << toLocale(p);
            }

            f_h << '\n';
        }
    }
}

void UmlRelation::generate_def(QTextStream & f, WrapperStr indent, bool h,
                               WrapperStr templates, WrapperStr cl_names,
                               WrapperStr, WrapperStr)
{
    if (isClassMember() && !cppDecl().isEmpty()) {
        UmlClass * cl = (UmlClass *) parent();

        if ((!templates.isEmpty() || (cl->name().find('<') != -1))
            ? h : !h) {
            const char * p = cppDecl();
            const char * pp = 0;

            while ((*p == ' ') || (*p == '\t'))
                p += 1;

            bool re_template = !templates.isEmpty() &&
                               insert_template(p, f, indent, templates);

            if (*p != '#')
                f << indent;

            const char * pname = name_spec(p);

            for (;;) {
                if (*p == 0) {
                    if (pp == 0)
                        break;

                    // comment management done
                    p = pp;
                    pp = 0;

                    if (re_template)
                        f << templates;

                    if (*p == 0)
                        break;

                    if (*p != '#')
                        f << indent;
                }

                if (*p == '\n') {
                    f << toLocale(p);

                    if (*p && (*p != '#'))
                        f << indent;
                }
                else if (*p == '@')
                    manage_alias(p, f);
                else if (*p != '$') {
                    if (p == pname)
                        f << cl_names << "::";

                    f << toLocale(p);
                }
                else if (!strncmp(p, "${comment}", 10)) {
                    if (!manage_comment(p, pp, CppSettings::isGenerateJavadocStyleComment())
                        && re_template)
                        f << templates;
                }
                else if (!strncmp(p, "${description}", 14)) {
                    if (!manage_description(p, pp) && re_template)
                        f << templates;
                }
                else if (!strncmp(p, "${static}", 9)) {
                    p += 9;
                }
                else if (!strncmp(p, "${const}", 8)) {
                    p += 8;

                    if (isReadOnly())
                        f << "const ";
                }
                else if (!strncmp(p, "${volatile}", 11)) {
                    p += 11;

                    if (isVolatile())
                        f << "volatile ";
                }
                else if (!strncmp(p, "${mutable}", 10)) {
                    p += 10;
                }
                else if (!strncmp(p, "${type}", 7)) {
                    p += 7;
                    roleType()->write(f);
                }
                else if (!strncmp(p, "${name}", 7)) {
                    p += 7;

                    if (*pname == '$')
                        f << cl_names << "::";

                    f << roleName();
                }
                else if (!strncmp(p, "${inverse_name}", 15)) {
                    p += 15;

                    switch (relationKind()) {
                    case anAssociation:
                    case anAggregation:
                    case anAggregationByValue:
                        f << side(side(TRUE) != this)->roleName();

                    default:
                        break;
                    }
                }
                else if (!strncmp(p, "${multiplicity}", 15)) {
                    p += 15;

                    const WrapperStr & m = multiplicity();

                    if (!m.isEmpty() && (*((const char *) m) == '['))
                        f << m;
                    else
                        f << '[' << m << ']';
                }
                else if (!strncmp(p, "${stereotype}", 13)) {
                    p += 13;
                    f << CppSettings::relationAttributeStereotype(stereotype());
                }
                else if (!strncmp(p, "${value}", 8)) {
                    if (!defaultValue().isEmpty()) {
                        if (need_equal(p, defaultValue()))
                            f << " = ";

                        f << defaultValue();
                    }

                    p += 8;
                }
                else if (!strncmp(p, "${h_value}", 10))
                    p += 10;
                else if (!strncmp(p, "${association}", 14)) {
                    p += 14;
                    UmlClass::write(f, association());
                }
                else
                    // strange
                    f << toLocale(p);
            }

            f << '\n';
        }
    }
}

