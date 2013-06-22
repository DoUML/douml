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

#include <stdio.h>
#include <stdlib.h>
#include <QTextStream>
#include <qfile.h>
#include <qfileinfo.h>
//Added by qt3to4:
#include "misc/mystr.h"

#include "UmlOperation.h"
#include "UmlCom.h"
#include "util.h"

const char * BodyPrefix = "// Douml preserved body begin ";
const char * BodyPostfix = "// Douml preserved body end ";
const char * BodyPythonPrefix = "## Douml preserved body begin ";
const char * BodyPythonPostfix = "## Douml preserved body end ";
const int BodyPrefixLength = 30;
const int BodyPostfixLength = 28;

static char * read_file(const char * filename)
{
    QFile fp(filename);

    if (fp.open(QIODevice::ReadOnly)) {
        QFileInfo fi(fp);
        int size = fi.size();
        char * s = new char[size + 1];

        if (fp.readBlock(s, size) == -1) {
            delete [] s;
            return 0;
        }
        else {
            s[size] = 0;

            return s;
        }
    }
    else
        return 0;
}

static WrapperStr linenumber(char * all, char * here)
{
    char c = *here;
    int n = 1;

    *here = 0;

    while ((all = strchr(all, '\n')) != 0) {
        all += 1;
        n += 1;
    }

    *here = c;

    char sn[24];

    sprintf(sn, " line %d", n);
    return sn;
}

void UmlOperation::roundtrip(const char * path, aLanguage who)
{
    char * s = read_file(path);

    if (s != 0) {
        char * p1 = s;
        char * p2;
        WrapperStr(UmlOperation::*get_body)();
        bool (UmlOperation::*set_body)(const char * s);
        bool (UmlOperation::*set_contextualbodyindent)(bool v);
        const char * prefix;
        const char * postfix;

        switch (who) {
        case cppLanguage:
            get_body = &UmlOperation::cppBody;
            set_body = &UmlOperation::set_CppBody;
            set_contextualbodyindent = &UmlOperation::set_CppContextualBodyIndent;
            prefix = BodyPrefix;
            postfix = BodyPostfix;
            break;

        case javaLanguage:
            get_body = &UmlOperation::javaBody;
            set_body = &UmlOperation::set_JavaBody;
            set_contextualbodyindent = &UmlOperation::set_JavaContextualBodyIndent;
            prefix = BodyPrefix;
            postfix = BodyPostfix;
            break;

        case phpLanguage:
            get_body = &UmlOperation::phpBody;
            set_body = &UmlOperation::set_PhpBody;
            set_contextualbodyindent = &UmlOperation::set_PhpContextualBodyIndent;
            prefix = BodyPrefix;
            postfix = BodyPostfix;
            break;

        default:
            // python
            get_body = &UmlOperation::pythonBody;
            set_body = &UmlOperation::set_PythonBody;
            set_contextualbodyindent = &UmlOperation::set_PythonContextualBodyIndent;
            prefix = BodyPythonPrefix;
            postfix = BodyPythonPostfix;
        }

        while ((p2 = strstr(p1, prefix)) != 0) {
            p2 += BodyPrefixLength;

            char * body;
            long id = strtol(p2, &body, 16);

            if (body != (p2 + 8)) {
                UmlCom::trace(WrapperStr("<font color =\"red\"> Error in ") + path +
                              linenumber(s, p2 - BodyPrefixLength) +
                              " : invalid preserve body identifier</font><br>");
                UmlCom::bye(n_errors() + 1);
                UmlCom::fatal_error("read_bodies 1");
            }

            if (*body == '\r')
                body += 1;

            if (*body == '\n')
                body += 1;
            else {
                UmlCom::trace(WrapperStr("<font  color =\"red\"> Error in ") + path +
                              linenumber(s, p2 - BodyPrefixLength) +
                              " : invalid preserve body block, end of line expected</font><br>");
                UmlCom::bye(n_errors() + 1);
                UmlCom::fatal_error("read_bodies 2");
            }

            UmlOperation * op = (UmlOperation *)
                                UmlBaseItem::from_id((unsigned) id, anOperation);

            if (op == 0) {
                QString n("%1");
                n.arg(QString::number((unsigned) id));
                UmlCom::trace(WrapperStr("<font  color =\"red\"> Error in ") + path +
                              linenumber(s, p2 - BodyPrefixLength) +
                              " : invalid operation id " + n + "</font><br>");
                UmlCom::bye(n_errors() + 1);
                UmlCom::fatal_error("read_bodies 3");
                return;
            }

            if (((p1 = strstr(body, postfix)) == 0) ||
                (strncmp(p1 + BodyPostfixLength, p2, 8) != 0)) {
                UmlCom::trace(WrapperStr("<font  color =\"red\"> Error in ") + path +
                              linenumber(s, p2 - BodyPrefixLength) +
                              " : invalid preserve body block, wrong balanced</font><br>");
                UmlCom::bye(n_errors() + 1);
                UmlCom::fatal_error("read_bodies 4");
            }

            p2 = p1;

            while ((p2 != body) && (p2[-1] != '\n'))
                p2 -= 1;

            char c = *p2;

            *p2 = 0;

            WrapperStr previous = (op->*get_body)();

            if (!op->isBodyGenerationForced() && (body != previous)) {
                if (!(op->*set_body)(body)) {
                    write_trace_header();
                    UmlCom::trace("&nbsp;&nbsp;&nbsp;&nbsp;<font color=\"red\"><b>cannot update body of <i>"
                                  + op->name() +
                                  ((op->isWritable()) ? "</i>, it is probably deleted</b></font><br>"
                                   : "</i>, it is read-only</b></font><br>"));
                    incr_error();
                }
                else {
                    (op->*set_contextualbodyindent)(FALSE);

                    write_trace_header();
                    UmlCom::trace("&nbsp;&nbsp;&nbsp;&nbsp;update body of <i>"
                                  + op->name() + "</i><br>");
                }
            }
            else if (verbose()) {
                write_trace_header();
                UmlCom::trace("&nbsp;&nbsp;&nbsp;&nbsp;body of <i>"
                              + op->name() + "</i> unchanged<br>");
            }

            *p2 = c;
            p1 += BodyPostfixLength + 8;
        }

        delete [] s;
    }
}
