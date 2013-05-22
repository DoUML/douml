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

#include "UmlPackage.h"
#include "UmlClassView.h"
#include "UmlArtifact.h"
#ifdef REVERSE
#include "UmlDeploymentView.h"
# ifdef ROUNDTRIP
# include "Package.h"
# include "Progress.h"
# endif
#endif
#include "UmlOperation.h"
#include "Lex.h"
#include "UmlCom.h"
//Added by qt3to4:
#include "misc/mystr.h"
//Added by qt3to4:
#include <Q3PtrList>

UmlPackage::UmlPackage(void * id, const WrapperStr & n)
    : UmlBasePackage(id, n)
{
    class_view = 0;
#ifdef REVERSE
    deployment_view = 0;
# ifdef ROUNDTRIP
    package = 0;
# endif
#endif
    namespace_fixedp = FALSE;
}

UmlClassView * UmlPackage::get_classview(const WrapperStr & nmsp)
{
    UmlPackage * pack;

    if (nmsp != cppNamespace()) {
        if (namespace_fixedp) {
            if ((pack = findCppNamespace(nmsp)) == 0) {
                WrapperStr s = nmsp;

                if (s.isEmpty())
                    s = name();
                else {
                    int index = 0;

                    while ((index = s.find("::", index)) != -1)
                        s.replace(index++, 2, " ");
                }

                if (((pack = UmlBasePackage::create(this, s)) == 0) &&
                    ((pack = UmlBasePackage::create(this, s += "_")) == 0) &&
                    ((pack = UmlBasePackage::create(this, s += "_")) == 0) &&
                    ((pack = UmlBasePackage::create(this, s += "_")) == 0) &&
                    ((pack = UmlBasePackage::create(this, s += "_")) == 0)) {
#ifdef REVERSE
                    UmlCom::trace(WrapperStr("<font face=helvetica><b>cannot create package <i>")
                                  + s + "</i> under package <i>"
                                  + name() + "</b></font><br>");
                    UmlCom::message("");
                    throw 0;
#else
                    QMessageBox::critical(0, "Fatal Error",
                                          WrapperStr("<font face=helvetica><b>cannot create package <i>")
                                          + s + "</i> under package <i>"
                                          + Name() + "</b></font><br>");
                    QApplication::exit(1);
#endif
                }

                pack->set_CppNamespace(nmsp);
                pack->set_CppSrcDir(cppSrcDir());
                pack->set_CppHDir(cppHDir());
                pack->namespace_fixedp = TRUE;
            }
        }
        else {
            pack = this;
            pack->set_CppNamespace(nmsp);
            pack->namespace_fixedp = TRUE;
        }
    }
    else
        pack = this;

    if (pack->class_view == 0) {
        Q3PtrVector<UmlItem> ch = pack->children();

        for (unsigned index = 0; index != ch.size(); index += 1)

            // return the first class view find
            if (ch[index]->kind() == aClassView)
                return pack->class_view = (UmlClassView *) ch[index];

        if ((pack->class_view = UmlBaseClassView::create(pack, name())) == 0) {
#ifdef REVERSE
            UmlCom::trace(WrapperStr("<font face=helvetica><b>cannot create class view <i>")
                          + name() + "</i> under package <i>"
                          + pack->name() + "</b></font><br>");
            UmlCom::message("");
            throw 0;
#else
            QMessageBox::critical(0, "Fatal Error",
                                  WrapperStr("<font face=helvetica><b>cannot create class view <i>")
                                  + name() + "</i> under package <i>"
                                  + pack->name() + "</b></font><br>");
            QApplication::exit(1);
#endif
        }
    }

    return pack->class_view;
}

#ifdef REVERSE

void UmlPackage::reverse_main(const WrapperStr & type, WrapperStr comment)
{
    // do not lost main !
    Lex::mark();
    UmlOperation::skip_body();

    UmlArtifact * cp;

#ifdef ROUNDTRIP
    bool roundtrip = FALSE;

    if ((cp = UmlArtifact::get_main()) != 0) {
        roundtrip = TRUE;
        cp->set_usefull();
    }
    else
#endif
        if ((cp = UmlBaseArtifact::create(get_deploymentview(0), "main")) == 0) {
            UmlCom::trace("<font face=helvetica><b>cannot create <i>artifact main</i></b></font><br><hr>");
            return;
        }

    if (! comment.isEmpty()) {
        unsigned start = 0;

        do {
            comment.insert(start, "//");
            start = comment.find('\n', start + 2) + 1;
        }
        while (start != 0);

        comment.append("\n\n");
    }

    comment.append(type);
    comment.append(" main(");
    comment.append(Lex::region());
    comment.append("\n");

#ifdef ROUNDTRIP

    if (roundtrip) {
        if (cp->cppSource() != comment)
            cp->set_CppSource(comment);

        return;
    }

#endif

    cp->set_Stereotype("source");
    cp->set_CppSource(comment);
    cp->set_CppHeader(0);
}


UmlDeploymentView * UmlPackage::get_deploymentview(const WrapperStr & nmsp)
{
    UmlPackage * pack;

    if (nmsp != cppNamespace()) {
        if (namespace_fixedp) {
            if ((pack = findCppNamespace(nmsp)) == 0) {
                WrapperStr s = nmsp;

                if (s.isEmpty())
                    s = name();
                else {
                    int index = 0;

                    while ((index = s.find("::", index)) != -1)
                        s.replace(index++, 2, " ");
                }

                if (((pack = UmlBasePackage::create(this, s)) == 0) &&
                    ((pack = UmlBasePackage::create(this, s += "_")) == 0) &&
                    ((pack = UmlBasePackage::create(this, s += "_")) == 0) &&
                    ((pack = UmlBasePackage::create(this, s += "_")) == 0) &&
                    ((pack = UmlBasePackage::create(this, s += "_")) == 0)) {
#ifdef REVERSE
                    UmlCom::trace(WrapperStr("<font face=helvetica><b>cannot create package <i>")
                                  + s + "</i> under package <i>"
                                  + name() + "</b></font><br>");
                    UmlCom::message("");
                    throw 0;
#else
                    QMessageBox::critical(0, "Fatal Error",
                                          WrapperStr("<font face=helvetica><b>cannot create package <i>")
                                          + s + "</i> under package <i>"
                                          + Name() + "</b></font><br>");
                    QApplication::exit(1);
#endif
                }

                pack->set_CppNamespace(nmsp);
                pack->set_CppSrcDir(cppSrcDir());
                pack->set_CppHDir(cppHDir());
                pack->namespace_fixedp = TRUE;
            }
        }
        else {
            pack = this;
            pack->set_CppNamespace(nmsp);
            pack->namespace_fixedp = TRUE;
        }
    }
    else
        pack = this;

    if (pack->deployment_view == 0) {
        Q3PtrVector<UmlItem> ch = pack->children();

        for (unsigned index = 0; index != ch.size(); index += 1)

            // return the first class view find
            if (ch[index]->kind() == aDeploymentView)
                return pack->deployment_view = (UmlDeploymentView *) ch[index];

        if ((pack->deployment_view = UmlBaseDeploymentView::create(pack, name())) == 0) {
#ifdef REVERSE
            UmlCom::trace(WrapperStr("<font face=helvetica><b>cannot create deployment view <i>")
                          + name() + "</i> under package <i>"
                          + pack->name() + "</b></font><br>");
            UmlCom::message("");
            throw 0;
#else
            QMessageBox::critical(0, "Fatal Error",
                                  WrapperStr("<font face=helvetica><b>cannot create deployment view <i>")
                                  + name() + "</i> under package <i>"
                                  + pack->name() + "</b></font><br>");
            QApplication::exit(1);
#endif
        }
    }

    return pack->deployment_view;
}

# ifdef ROUNDTRIP

// applied on the project
void UmlPackage::init(Package * p)
{
    package = p;

    const Q3PtrVector<UmlItem> & ch = UmlItem::children();
    UmlItem ** v = ch.data();
    UmlItem ** const vsup = v + ch.size();

    for (; v != vsup; v += 1)
        (*v)->upload(package);
}

void UmlPackage::upload(ClassContainer * cnt)
{
    package = new Package((Package *) cnt, this);

    const Q3PtrVector<UmlItem> & ch = UmlItem::children();
    UmlItem ** v = ch.data();
    UmlItem ** const vsup = v + ch.size();

    for (; v != vsup; v += 1)
        (*v)->upload(package);
}

int UmlPackage::count_roundtriped()
{
    int result = 1;

    const Q3PtrVector<UmlItem> & ch = UmlItem::children();
    UmlItem ** v = ch.data();
    UmlItem ** const vsup = v + ch.size();

    for (; v != vsup; v += 1)
        if ((*v)->kind() == aPackage)
            result += ((UmlPackage *) *v)->count_roundtriped();

    return result;
}

bool UmlPackage::set_roundtrip_expected()
{
    if (cppHDir().isEmpty())
        package->accept_roundtrip_root(TRUE);

    if (cppSrcDir().isEmpty())
        package->accept_roundtrip_root(FALSE);

    Progress::tic_it();

    const Q3PtrVector<UmlItem> & ch = UmlItem::children();
    UmlItem ** v = ch.data();
    UmlItem ** const vsup = v + ch.size();
    bool result = isWritable();

    for (; v != vsup; v += 1)
        result &= (*v)->set_roundtrip_expected();

    return result;
}

void UmlPackage::mark_useless(QList<UmlItem *> & l)
{
    Q3PtrVector<UmlItem> ch = UmlItem::children();
    UmlClassItem ** v = (UmlClassItem **) ch.data();
    UmlClassItem ** const vsup = v + ch.size();

    for (; v != vsup; v += 1)
        (*v)->mark_useless(l);
}

void UmlPackage::scan_it(int & n)
{
    package->scan_dir(n);
}

void UmlPackage::send_it(int n)
{
    package->send_dir(n);
}

# endif
#endif

