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

#include "UmlClassMember.h"

#include "UmlCom.h"
#include "UmlTypeSpec.h"
#include "CppRefType.h"
#include "UmlSettings.h"
#include "CppSettings.h"

void UmlClassMember::generate_visibility(aVisibility & current_visibility,
        QTextStream & f_h, bool ahead,
        const WrapperStr & indent)
{
    aVisibility v = (cppVisibility() == DefaultVisibility)
                    ? visibility() : cppVisibility();

    if (v == PackageVisibility)
        v = PublicVisibility;

    if (v != current_visibility) {
        current_visibility = v;

        if (!ahead)
            f_h << '\n';

        f_h << indent << CppSettings::visibilityIndent();

        switch (v) {
        case PublicVisibility:
            f_h << "public:\n";
            break;

        case ProtectedVisibility:
            f_h << "protected:\n";
            break;

        default:
            f_h << "private:\n";
            break;
        }
    }
}

void UmlClassMember::remove_comments(WrapperStr & s)
{
    int index1 = 0;

    if ((index1 = s.find("${comment}")) != -1)
        s.remove((unsigned) index1, 10);
    else if ((index1 = s.find("${description}")) != -1)
        s.remove((unsigned) index1, 14);

    while ((index1 = s.find('/', index1)) != -1) {
        int index2;

        switch (((const char *) s)[index1 + 1]) {
        case '/':
            if ((index2 = s.find('\n', index1 + 2)) != -1)
                s.remove(index1, index2 - index1 + 1);
            else
                s.truncate(index1);

            break;

        case '*':
            if ((index2 = s.find("*/", index1 + 2)) != -1)
                s.replace(index1, index2 - index1 + 1, " ");
            else
                s.truncate(index1);

            break;

        default:
            index1 += 1;
        }
    }
}

void UmlClassMember::remove_preprocessor(WrapperStr & s)
{
    int index = 0;

    while ((index = s.find('#', index)) != -1) {
        // remove all up to the end of line
        int index2 = index + 1;
        int index3;

        while ((index3 = s.find('\n', index2)) != -1) {
            // manage multi lines #define
            if (((const char *) s)[index3 - 1] != '\\')
                break;
            else
                index2 = index3 + 1;
        }

        // the \n is still here to have a separator
        if (index3 == -1)
            s.truncate(index);
        else
            s.remove(index, index3 - index);
    }
}

void UmlClassMember::remove_arrays(WrapperStr & s)
{
    int index1 = 0;

    while ((index1 = s.find('[', index1)) != -1) {
        int index2 = index1 = s.find(']', index1 + 1);

        if (index2 == -1) {
            s.truncate(index1);
            return;
        }
        else
            s.replace(index1, index2 - index1 + 1, " ");
    }
}

// Between template < and > I suppose that a type is not included
// because I cannot know how the type is used and I do not want to
// produce circular #include
bool UmlClassMember::compute_dependency(QList<CppRefType *> & dependencies,
                                        WrapperStr decl, const UmlTypeSpec & t,
                                        bool force_incl)
{
    remove_comments(decl);
    remove_preprocessor(decl);
    remove_arrays(decl);

    int template_level = 0;
    bool have_type = FALSE;
    const char * p = decl;
    const char * dontsubstituteuntil = 0;

    for (;;) {
        UmlTypeSpec ts;
        char c;
        bool dontsearchend = FALSE;

        // search word beginning
        while ((c = *p) != 0) {
            if ((c == '_') ||
                ((c >= 'a') && (c <= 'z')) ||
                ((c >= 'A') && (c <= 'Z')))
                break;
            else if (dontsubstituteuntil != 0) {
                if (p >= dontsubstituteuntil)
                    dontsubstituteuntil = 0;
                else
                    p += 1;
            }
            else if (c == '=')
                // init, all is done
                return have_type;
            else if (!strncmp(p, "${type}", 7)) {
                p += 7;
                ts = t;

                if (ts.type != 0) {
                    dontsearchend = TRUE;
                    break;
                }
                else {
                    decl = ts.explicit_type + p;
                    p = decl;
                }
            }
            else {
                switch (c) {
                case '<':
                    template_level += 1;
                    break;

                case '>':
                    template_level -= 1;
                }

                p += 1;
            }
        }

        if (c == 0)
            return have_type;

        if (!dontsearchend) {
            // search word end
            const char * p2 = p;

            ts.type = 0;
            ts.explicit_type = p2;
            p += 1;

            while ((c = *p) != 0) {
                if ((c == '_') ||
                    (c == ':') ||
                    ((c >= 'a') && (c <= 'z')) ||
                    ((c >= 'A') && (c <= 'Z')) ||
                    ((c >= '0') && (c <= '9')))
                    p += 1;
                else {
                    ts.explicit_type = ts.explicit_type.left(p - p2);
                    //ts.explicit_type.truncate(p - p2);
                    break;
                }
            }

//#warning NAMESPACE

            if (dontsubstituteuntil == 0) {
                WrapperStr subst = CppSettings::type(ts.explicit_type);

                if (subst != ts.explicit_type) {
                    decl = subst + ' ' + p;
                    p = decl;
                    dontsubstituteuntil = p + subst.length();
                    continue;
                }
            }
        }

        // check manually added keyword
        if ((ts.explicit_type == "const") ||
            (ts.explicit_type == "static"))
            continue;

        // search for a * or & or < after the typename
        bool incl = (template_level == 0);

        while ((c = *p) != 0) {
            if ((c == '*') || (c == '&')) {
                incl = FALSE;
                p += 1;
                break;
            }

            if ((c == '<') ||
                (c == '>') ||
                (c == '(') ||
                (c == ')') ||
                (c == ',') ||
                (c == '_') ||
                (c == '$') ||
                ((c >= 'a') && (c <= 'z')) ||
                ((c >= 'A') && (c <= 'Z'))) {
                break;
            }

            p += 1;
        }

        if (CppRefType::add(ts, dependencies, force_incl || incl))
            have_type = TRUE;
    }

    return have_type;
}

// return TRUE if stop on comment/description
bool UmlClassMember::insert_template(const char *& p, QTextStream & fs,
                                     const WrapperStr & indent,
                                     const WrapperStr & templ)
{
    // search the beginning of the definition/declaration in p;
    for (;;) {
        if (*p == 0)
            return FALSE;

        if (*p == '\n') {
            fs << toLocale(p);

            if (*p != '#')
                fs << indent;
        }

        if (*p <= ' ')
            fs << toLocale(p);
        else if (*p == '/') {
            if (p[1] == '/') {
                // comment
                fs << toLocale(p);

                do fs << toLocale(p);

                while (*p && (*p != '\n'));
            }
            else if (p[1] == '*') {
                /* comment */
                fs << toLocale(p);

                do fs << toLocale(p);

                while (*p && ((*p != '*') || (p[1] != '/')));

                fs << "*/";
                p += 2;
            }
            else
                break;
        }
        else if (*p == '#') {
            do {
                fs << toLocale(p);

                if (*p == '\\') {
                    fs << toLocale(p);
                    fs << toLocale(p);
                }
                else if ((*p == '/') && (p[1] == '*')) {
                    /* comment */
                    p += 1;

                    do p += 1;

                    while (*p && ((*p != '*') || (p[1] != '/')));

                    p += 2;
                }
            }
            while (*p && (*p != '\n'));
        }
        else if ((strncmp(p, "${comment}", 10) == 0) ||
                 (strncmp(p, "${description}", 14) == 0))
            return TRUE;
        else
            break;
    }

    fs << templ;
    return FALSE;
}
