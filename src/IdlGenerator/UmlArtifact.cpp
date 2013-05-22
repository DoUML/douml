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
#include <QTextStream>
//Added by qt3to4:
#include "misc/mystr.h"
#include <QTextStream>
//Added by qt3to4:
#include <Q3PtrList>

#include "UmlArtifact.h"
#include "IdlSettings.h"
#include "UmlPackage.h"
#include "UmlClass.h"
#include "UmlCom.h"
#include "util.h"

UmlPackage * UmlArtifact::package_of_generated_artifact;

void UmlArtifact::generate()
{
    if (! managed) {
        managed = TRUE;

        if (stereotype() == "text") {
            generate_text();
            return;
        }
        else if (stereotype() != "source")
            return;

        package_of_generated_artifact = package();

        const WrapperStr def = idlSource();

        if (def.isEmpty())
            return;

        const WrapperStr & name = this->name();
        UmlPackage * pack = package();
        WrapperStr path = pack->path(name);
        WrapperStr mod_start;
        WrapperStr mod_end;
        WrapperStr mod = pack->idlModule();

        if (! mod.isEmpty()) {
            int index = 0;
            int index2;
            WrapperStr closed = "\n}; // module ";

            while ((index2 = mod.find(':', index)) != -1) {
                WrapperStr mo = mod.mid(index, index2 - index);

                mod_start += WrapperStr("module ") + mo + " {\n\n";
                closed += mo;
                mod_end = closed + "\n" + mod_end;
                closed += "::";
                mod.replace(index2, 2, "_");
                index = index2 + 1;
            }

            mod_start += WrapperStr("module ") + mod.mid(index) + " {\n\n";
            closed += mod.mid(index);
            mod_end = closed + "\n" + mod_end;
        }

        UmlCom::message(name);

        if (verbose())
            UmlCom::trace(WrapperStr("<hr><font face=helvetica>Generate code for <i> ")
                          + name + "</i> in <i>" + path + "</i></font><br>");
        else
            set_trace_header(WrapperStr("<font face=helvetica>Generate code for <i> ")
                             + name + "</i> in <i>" + path + "</i></font><br>");

#if 0
        // compute dependencies

        QList<CppRefType *> dependencies;
#endif
        const Q3PtrVector<UmlClass> & cls = associatedClasses();
        unsigned n = cls.count();
        unsigned index;

#if 0

        for (index = 0; index != n; index += 1)
            cls[index]->compute_dependencies(dependencies);

#endif

        // generate source file

        QByteArray file;
        QTextStream f(&file);
        const char * p = def;
        const char * pp = 0;

        for (;;) {
            if (*p == 0) {
                if (pp == 0)
                    break;

                // comment management done
                p = pp;
                pp = 0;

                if (*p == 0)
                    break;
            }

            if (*p == '@')
                manage_alias(p, f);
            else if (*p != '$')
                f << *p++;
            else if (!strncmp(p, "${comment}", 10))
                manage_comment(p, pp);
            else if (!strncmp(p, "${description}", 14))
                manage_description(p, pp);
            else if (!strncmp(p, "${name}", 7)) {
                p += 7;
                f << name;
            }
            else if (!strncmp(p, "${Name}", 7)) {
                p += 7;
                f << capitalize(name);
            }
            else if (!strncmp(p, "${NAME}", 7)) {
                p += 7;
                f << name.upper();
            }
            else if (!strncmp(p, "${nAME}", 7)) {
                p += 7;
                f << name.lower();
            }
            else if (!strncmp(p, "${module}", 9)) {
                p += 9;
                f << mod;
            }
            else if (!strncmp(p, "${MODULE}", 9)) {
                p += 9;
                f << mod.upper();
            }
            else if (!strncmp(p, "${includes}", 11)) {
                p += 11;

                /*if (!incl_computed) {
                  incl_computed = TRUE;
                  CppRefType::compute(dependencies, hdef, srcdef, h_incl, decl, incl, this);
                }
                if (!incl.isEmpty()) {
                  f << incl;
                  if (*p != '\n')
                    f << '\n';
                }
                else*/

                if (*p == '\n')
                    p += 1;
            }
            else if (!strncmp(p, "${definition}", 13)) {
                p += 13;

                for (index = 0; index != n; index += 1)
                    cls[index]->generate(f);

                if (*p == '\n')
                    p += 1;
            }
            else if (!strncmp(p, "${module_start}", 15)) {
                p += 15;

                if (!mod_start.isEmpty())
                    f << mod_start;

                if (*p == '\n')
                    p += 1;
            }
            else if (!strncmp(p, "${module_end}", 13)) {
                p += 13;

                if (!mod_end.isEmpty())
                    f << mod_end;

                if (*p == '\n')
                    p += 1;
            }
            else
                // strange
                f << *p++;
        }

        f << '\000';
        f.flush();

        if (must_be_saved(path, file)) {
            write_trace_header();

            FILE * fp;

            if ((fp = fopen((const char *) path, "wb")) == 0) {
                write_trace_header();
                UmlCom::trace(WrapperStr("<font color=\"red\"><b><i> ")
                              + name + " : </i> cannot open <i> "
                              + path + "</i>, edit the <i> generation settings</i> (tab directory) or the <i>"
                              + pack->name() + "</i> Idl directory specification</b></font><br>");
                incr_error();
            }
            else {
                fputs((const char *) file, fp);
                fclose(fp);
            }
        }
        else if (get_trace_header().isEmpty())
            UmlCom::trace(WrapperStr("<font face=helvetica><i> ")
                          + path + "</i> not modified</font><br>");
    }
}

void UmlArtifact::generate_text()
{
    const WrapperStr srcdef = idlSource();

    if (srcdef.isEmpty()) {
        if (verbose())
            UmlCom::trace(WrapperStr("<hr><font face=helvetica>artifact <i>")
                          + name() + "</i> has an empty IDL definition</font><br>");

        return;
    }

    UmlPackage * pack = package();
    const WrapperStr & name = UmlArtifact::name();
    WrapperStr src_path = pack->text_path(name);

    WrapperStr s = " in <i> " + src_path + "</i>";

    UmlCom::message(name);

    if (verbose())
        UmlCom::trace(WrapperStr("<hr><font face=helvetica>Generate code for <i> ")
                      + name + "</i>" + s + "</font><br>");
    else
        set_trace_header(WrapperStr("<font face=helvetica>Generate code for <i> ")
                         + name + "</i>" + s + "</font><br>");

    if (must_be_saved(src_path, (const char *) srcdef)) {
        write_trace_header();

        FILE * fp_src;

        if ((fp_src = fopen((const char *) src_path, "wb")) == 0) {
            write_trace_header();
            UmlCom::trace(WrapperStr("<font color=\"red\"><b><i> ")
                          + name + " : </i> cannot open <i> "
                          + src_path + "</i>, edit the <i> generation settings</i> (tab directory) or the <i>"
                          + pack->name() + "</i> IDL directory specification</b></font><br>");
            incr_error();
        }
        else {
            fputs((const char *) srcdef, fp_src);
            fclose(fp_src);
        }
    }
    else if (get_trace_header().isEmpty())
        UmlCom::trace(WrapperStr("<font face=helvetica><i> ")
                      + src_path + "</i> not modified</font><br>");
}

UmlPackage * UmlArtifact::package()
{
    UmlItem * parent = this->parent();

    while (parent->kind() != aPackage)
        parent = parent->parent();

    return (UmlPackage *) parent;
}

UmlPackage * UmlArtifact::generation_package()
{
    return package_of_generated_artifact;
}

bool UmlArtifact::must_be_saved(const char * path, const char * new_contains)
{
    FILE * fp = fopen(path, "rb");

    if (fp == 0)
        return TRUE;

    int c;

    while ((c = fgetc(fp)) != EOF) {
        if (((char) c) != *new_contains++) {
            fclose(fp);
            return TRUE;
        }
    }

    fclose(fp);

    return *new_contains != 0;
}

