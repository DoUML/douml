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
//Added by qt3to4:
#include <Q3PtrList>

#include "UmlAttribute.h"
#include "UmlSettings.h"
#include "CppSettings.h"
#include "UmlClass.h"
#include "UmlCom.h"
#include "util.h"
#include "misc/codec.h"

void UmlAttribute::compute_dependency(QList<CppRefType *> & dependency,
                                      const WrapperStr & cl_stereotype,
                                      bool all_in_h)
{
    if ((cl_stereotype == "enum") || (cl_stereotype == "typedef"))
        return;

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

    if ((index = decl.find("${value}")) != -1)
        decl.remove((unsigned) index, 8);

    if ((index = decl.find("${h_value}")) != -1)
        decl.remove((unsigned) index, 10);

    if ((index = decl.find("${name}")) != -1)
        decl.remove((unsigned) index, 7);

    if ((index = decl.find("${stereotype}")) != -1)
        decl.replace((unsigned) index, 13,
                     CppSettings::relationAttributeStereotype(stereotype()));

    replace_alias(decl);

    if (!UmlClassMember::compute_dependency(dependency, decl, type(), all_in_h)) {
        write_trace_header();
        UmlCom::trace(WrapperStr("&nbsp;&nbsp;&nbsp;&nbsp;<font color=\"red\"><b>type missing for attribute <i>")
                      + name() + "</i></b></font><br>");
        incr_error();
    }
}




void UmlAttribute::generate_decl(aVisibility & current_visibility, QTextStream & f_h,
                                 const WrapperStr & cl_stereotype, WrapperStr indent,
                                 BooL & first, bool last)
{
    if (cl_stereotype == "typedef") {
        write_trace_header();
        UmlCom::trace("&nbsp;&nbsp;&nbsp;&nbsp;<font color=\"red\"><b>a <i>typedef</i> cannot have attribute</b></font><br>");
        incr_warning();
        return;
    }

    const char * p = cppDecl();
    const char * pp = 0;
    bool in_enum = (cl_stereotype == "enum");

    if (!in_enum)
        generate_visibility(current_visibility, f_h, first, indent);

    first = FALSE;

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
        {
            //f_h << toLocale(p);
            f_h << toLocale(p);
        }
        else if (!strncmp(p, "${comment}", 10))
            manage_comment(p, pp, CppSettings::isGenerateJavadocStyleComment());
        else if (!strncmp(p, "${description}", 14))
            manage_description(p, pp);
        else if (!strncmp(p, "${name}", 7)) {
            p += 7;
            f_h << name();
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
        else if (!strncmp(p, "${value}", 8) || !strncmp(p, "${h_value}", 10)) {
            const char * pb = p;

            p += (p[2] == 'h') ? 10 : 8;

            if (!defaultValue().isEmpty() && !isClassMember()) {
                if (need_equal(pb, defaultValue()))
                    f_h << " = ";

                f_h << defaultValue();
            }

            if (in_enum) {
                if (last) {
                    if (*p == ',')
                        p += 1;
                }
                else if (*p != ',')
                    f_h << ',';
            }

//            else if ((p[-8] == 'h') && isClassMember() && !defaultValue().isEmpty())
//            {
//                if (need_equal(pb, defaultValue()))
//                    f_h << " = ";
//                f_h << defaultValue();
//            }
        }
        else if (in_enum)
            // strange
            f_h << toLocale(p);
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
            UmlClass::write(f_h, type());
            //f_h << CppSettings::Type(Type().Type());
        }
        else
            // strange
            f_h << toLocale(p);
    }

    f_h << '\n';
}

void UmlAttribute::generate_def(QTextStream & f, WrapperStr indent, bool h,
                                WrapperStr templates, WrapperStr cl_names,
                                WrapperStr, WrapperStr)
{
    if (isClassMember() && !cppDecl().isEmpty()) {
        UmlClass * cl = (UmlClass *) parent();

        if ((!templates.isEmpty() || ((cl->name().find('<') != -1))) ? h : !h) {
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
                else if (!strncmp(p, "${name}", 7)) {
                    if (*pname == '$')
                        f << cl_names << "::";

                    p += 7;
                    f << name();
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
                    UmlClass::write(f, type());
                    //f << CppSettings::Type(Type().Type());
                }
                else
                    // strange
                    f << toLocale(p);
            }

            f << '\n';
        }
    }
}
