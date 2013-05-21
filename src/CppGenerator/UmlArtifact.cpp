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
#include <QFile>
#include <QTextStream>
//Added by qt3to4:
#include "misc/mystr.h"
#include <QTextStream>
//Added by qt3to4:
#include <Q3PtrList>

#include "UmlArtifact.h"
#include "UmlPackage.h"
#include "UmlClass.h"
#include "UmlOperation.h"
#include "CppRefType.h"
#include "CppSettings.h"
#include "UmlCom.h"
#include "util.h"
#include "misc/codec.h"
#include <QSharedPointer>
#include <Logging/QsLog.h>

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

        const WrapperStr hdef = cppHeader();
        QLOG_INFO() << "Read header as: " + hdef.operator QString();
        const WrapperStr srcdef = cppSource();

        if (hdef.isEmpty() && srcdef.isEmpty()) {
            if (verbose())
                UmlCom::trace(WrapperStr("<hr><font face=helvetica>artifact <i>")
                              + name() + "</i> has an empty C++ definition</font><br>");

            return;
        }

        const WrapperStr & name = UmlArtifact::name();
        UmlPackage * pack = package();
        WrapperStr h_path = pack->header_path(name);
        WrapperStr src_path = pack->source_path(name);
        WrapperStr nasp_start;
        WrapperStr nasp_end;
        const char * cnasp = pack->cppNamespace();
        WrapperStr nasp = ((cnasp[0] == ':') && (cnasp[1] == ':'))
                         ? cnasp + 2 : cnasp;

        if (!nasp.isEmpty()) {
            int index = 0;
            int index2;
            WrapperStr closed = "\n} // namespace ";

            while ((index2 = nasp.find(':', index)) != -1) {
                WrapperStr na = nasp.mid(index, index2 - index);

                nasp_start += WrapperStr("namespace ") + na + " {\n\n";
                closed += na;
                nasp_end = closed + "\n" + nasp_end;
                closed += "::";
                nasp.replace(index2, 2, "_");
                index = index2 + 1;
            }

            nasp_start += WrapperStr("namespace ") + nasp.mid(index) + " {\n\n";
            closed += nasp.mid(index);
            nasp_end = closed + "\n" + nasp_end;
        }
        else {
            WrapperStr s;

            if (!hdef.isEmpty())
                s = " in <i> " + h_path + "</i>";

            if (!srcdef.isEmpty()) {
                if (!hdef.isEmpty())
                    s += " and <i> " + src_path + "</i>";
                else
                    s = " in <i> " + src_path + "</i>";
            }

            UmlCom::message(name);

            if (verbose())
                UmlCom::trace(WrapperStr("<hr><font face=helvetica>Generate code for <i> ")
                              + name + "</i>" + s + "</font><br>");
            else
                set_trace_header(WrapperStr("<font face=helvetica>Generate code for <i> ")
                                 + name + "</i>" + s + "</font><br>");
        }

        // get bodies if preserve
        const Q3PtrVector<UmlClass> & cls = associatedClasses();

        if (preserve())
            UmlOperation::read_bodies(h_path, src_path);

        // compute dependencies

        bool all_in_h = (hdef.find("${all_includes}") != -1);
        Q3PtrList<CppRefType> dependencies;
        unsigned n = cls.count();
        unsigned index;

        for (index = 0; index != n; index += 1)
            cls[index]->compute_dependencies(dependencies, all_in_h);

        // generate header file

        WrapperStr h_incl;
        WrapperStr src_incl;
        WrapperStr decl;
        bool incl_computed = FALSE;

        if (!hdef.isEmpty()) {


            //headerFile->append("Test DAta");

            QLOG_INFO() << "openign file for writing: ";
            //QTextStream f_h(file.data()); //[lgfreitas] Now QTextStream receives a pointer to a byte array...
            QSharedPointer<QByteArray> headerFile(new QByteArray());
            QTextStream f_h(headerFile.data(), QFile::WriteOnly);
            f_h.setCodec(QTextCodec::codecForLocale());
            //QTextStream f_h(headerFile.data(), QIODevice::WriteOnly);
            //QString h_copy = QString(hdef.operator QString());
            const char * p = hdef;
            const char * pp = 0;

            for (;;)
            {
                QLOG_INFO() << "At this point P is: " << QString(p);
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
                    manage_alias(p, f_h);
                else if (*p != '$')
                {
                    f_h << toLocale(p);
                }
                else if (!strncmp(p, "${comment}", 10))
                    manage_comment(p, pp, CppSettings::isGenerateJavadocStyleComment());
                else if (!strncmp(p, "${description}", 14))
                    manage_description(p, pp);
                else if (!strncmp(p, "${name}", 7)) {
                    p += 7;
                    f_h << name;
                }
                else if (!strncmp(p, "${Name}", 7)) {
                    p += 7;
                    //QLOG_INFO() << "Outputting name: " << name;
                    f_h << capitalize(name);
                }
                else if (!strncmp(p, "${NAME}", 7)) {
                    p += 7;
                    f_h << name.upper();
                }
                else if (!strncmp(p, "${nAME}", 7)) {
                    p += 7;
                    f_h << name.lower();
                }
                else if (!strncmp(p, "${namespace}", 12)) {
                    p += 12;
                    f_h << nasp;
                }
                else if (!strncmp(p, "${NAMESPACE}", 12)) {
                    p += 12;
                    f_h << nasp.upper();
                }
                else if (!strncmp(p, "${includes}", 11)
                         || !strncmp(p, "${all_includes}", 15))
                {
                    QLOG_INFO() << "REaDING INCLUDES";
                    p += (p[2] == 'a') ? 15 : 11;
                    QLOG_INFO() << "Modified p 1 to be" << QString(p);
                    if (!incl_computed)
                    {
                        incl_computed = TRUE;
                        CppRefType::compute(dependencies, hdef.operator QString(), srcdef, h_incl, decl, src_incl, this);
                        QLOG_INFO() << "Modified hdef to be: " << hdef.operator QString();

                    }
                    QLOG_INFO() << "Modified p 2 to be" << QString(p);
                    if (!h_incl.isEmpty())
                    {
                        f_h << h_incl;

                        if (*p != '\n')
                            f_h << '\n';
                    }
                    else if (*p == '\n')
                        p += 1;
                    QLOG_INFO() << "FINISHED INCLUDES";
                }
                else if (!strncmp(p, "${declarations}", 15))
                {
                    p += 15;

                    if (!incl_computed)
                    {
                        incl_computed = TRUE;
                        CppRefType::compute(dependencies, hdef.operator QString(), srcdef, h_incl, decl, src_incl, this);
                    }
                    QLOG_INFO() << "DECLS IS: " << decl.operator QString();
                    if (!decl.isEmpty())
                    {
                        f_h << decl;

                        if (*p != '\n')
                            f_h << '\n';
                    }
                    else if (*p == '\n')
                        p += 1;
                }
                else if (!strncmp(p, "${namespace_start}", 18)) {
                    p += 18;

                    if (!nasp_start.isEmpty())
                        f_h << nasp_start;

                    if (*p == '\n')
                        p += 1;
                }
                else if (!strncmp(p, "${namespace_end}", 16)) {
                    p += 16;

                    if (!nasp_end.isEmpty())
                        f_h << nasp_end;

                    if (*p == '\n')
                        p += 1;
                }
                else if (!strncmp(p, "${definition}", 13)) {
                    p += 13;

                    for (index = 0; index != n; index += 1)
                        cls[index]->generate_decl(f_h, current_indent(p, hdef.operator QString()));

                    if (*p == '\n')
                        p += 1;
                }
                else
                    // strange
                    f_h << toLocale(p);
            }

            f_h << '\000';
            f_h.flush();

            if (must_be_saved(h_path, headerFile->data())) {
                QLOG_INFO() << "this is essentially what goes to the header file: " << headerFile->size();
                write_trace_header();

                //FILE * fp_h;
                 QFile file(h_path);
                if (!file.open(QFile::WriteOnly))
                {
                    UmlCom::trace(WrapperStr("<font color=\"red\"><b><i> ")
                                  + name + "</i> : cannot open <i> "
                                  + h_path + "</i>, edit the <i> generation settings</i> (tab directory) or the <i>"
                                  + pack->name() + "</i> C++ directory specification</b></font><br>");
                    incr_error();
                }
                else {
                    QLOG_INFO() << "this is essentially what goes to the file: " << headerFile->constData();

                    QTextCodec* codec = QTextCodec::codecForLocale();
                    QTextStream out(&file);
                    out.setCodec(QTextCodec::codecForLocale());
                    QString temp(*headerFile.data());
                    out << codec->toUnicode(temp);
                    //out << *headerFile.data();
                    out.flush();
                }
            }
            else {
                WrapperStr th = get_trace_header();

                if (th.isEmpty())
                    UmlCom::trace(WrapperStr("<br><font face=helvetica><i> ")
                                  + h_path + "</i> not modified</font><br>");
                else
                    set_trace_header(th + "<font face=helvetica><i> "
                                     + h_path + "</i> not modified</font><br>");
            }
        }

        // generate source file

        if (!srcdef.isEmpty()) {
            QSharedPointer<QByteArray> file(new QByteArray());
            QTextStream f_src(file.data(), QIODevice::WriteOnly);
            const char * p = srcdef;
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
                    manage_alias(p, f_src);
                else if (*p != '$')
                    f_src << toLocale(p);
                else if (!strncmp(p, "${comment}", 10))
                    manage_comment(p, pp, CppSettings::isGenerateJavadocStyleComment());
                else if (!strncmp(p, "${description}", 14))
                    manage_description(p, pp);
                else if (!strncmp(p, "${name}", 7)) {
                    // for file header
                    p += 7;
                    f_src << name;
                }
                else if (!strncmp(p, "${includes}", 11)) {
                    p += 11;

                    if (!incl_computed) {
                        incl_computed = TRUE;
                        CppRefType::compute(dependencies, hdef.operator QString(), srcdef, h_incl, decl, src_incl, this);
                    }

                    if (!src_incl.isEmpty()) {
                        f_src << src_incl;

                        if (*p != '\n')
                            f_src << '\n';
                    }
                    else if (*p == '\n')
                        p += 1;
                }
                else if (!strncmp(p, "${members}", 10)) {
                    p += 10;

                    for (index = 0; index != n; index += 1)
                        cls[index]->generate_def(f_src, current_indent(p, srcdef), FALSE);

                    if (*p == '\n')
                        p += 1;
                }
                else if (!strncmp(p, "${namespace_start}", 18)) {
                    p += 18;

                    if (!nasp_start.isEmpty())
                        f_src << nasp_start;

                    if (*p == '\n')
                        p += 1;
                }
                else if (!strncmp(p, "${namespace_end}", 16)) {
                    p += 16;

                    if (!nasp_end.isEmpty())
                        f_src << nasp_end;

                    if (*p == '\n')
                        p += 1;
                }
                else
                    // strange
                    f_src << toLocale(p);
            }

            f_src << '\000';
            f_src.flush();

            if (must_be_saved(src_path, file->data())) {
                write_trace_header();

                FILE * fp_src;

                if ((fp_src = fopen((const char *) src_path, "wb")) == 0) {
                    write_trace_header();
                    UmlCom::trace(WrapperStr("<font color=\"red\"><b><i> ")
                                  + name + " : </i> cannot open <i> "
                                  + src_path + "</i>, edit the <i> generation settings</i> (tab directory) or the <i>"
                                  + pack->name() + "</i> C++ directory specification</b></font><br>");
                    incr_error();
                }
                else {
                    fputs((const char *) file->data(), fp_src);
                    fclose(fp_src);
                }
            }
            else if (get_trace_header().isEmpty())
                UmlCom::trace(WrapperStr("<font face=helvetica><i> ")
                              + src_path + "</i> not modified</font><br>");
        }
    }
}

void UmlArtifact::generate_text()
{
    const WrapperStr srcdef = cppSource();

    if (srcdef.isEmpty()) {
        if (verbose())
            UmlCom::trace(WrapperStr("<hr><font face=helvetica>artifact <i>")
                          + name() + "</i> has an empty C++ definition</font><br>");

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
                          + pack->name() + "</i> C++ directory specification</b></font><br>");
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
