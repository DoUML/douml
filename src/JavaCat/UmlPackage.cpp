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

#ifndef REVERSE
#include <qapplication.h>
#include <qmessagebox.h>
//Added by qt3to4:
#include "misc/mystr.h"
//Added by qt3to4:
#include <Q3PtrList>
#endif

#include "UmlPackage.h"
#include "UmlCom.h"
#include "UmlClassView.h"
#ifdef REVERSE
#include "UmlDeploymentView.h"
# ifdef ROUNDTRIP
# include "Package.h"
# include "Progress.h"
# endif
#endif

UmlPackage::UmlPackage(void * id, const WrapperStr & n)
    : UmlBasePackage(id, n)
{
    classview = 0;
#ifdef REVERSE
    deploymentview = 0;
# ifdef ROUNDTRIP
    package = 0;
# endif
#endif
}

UmlClassView * UmlPackage::get_classview()
{
    if (classview == 0) {
        const Q3PtrVector<UmlItem> & ch = UmlItem::children();
        UmlItem ** v = ch.data();
        UmlItem ** const vsup = v + ch.size();

        for (; v != vsup; v += 1) {
            // return the first class view find
            if ((*v)->kind() == aClassView) {
                classview = (UmlClassView *) *v;
                return classview;
            }
        }

        if ((classview = UmlBaseClassView::create(this, name())) == 0) {
#ifdef REVERSE
            UmlCom::trace(WrapperStr("<font face=helvetica><b>cannot create class view <i>")
                          + name() + "</i> under package <i>"
                          + name() + "</b></font><br>");
            UmlCom::message("");
            throw 0;
#else
            QMessageBox::critical(0, "Fatal Error",
                                  WrapperStr("<font face=helvetica><b>cannot create class view <i>")
                                  + name() + "</i> under package <i>"
                                  + name() + "</b></font><br>");
            QApplication::exit(1);
#endif
        }
    }

    return classview;
}

#ifdef REVERSE

UmlDeploymentView * UmlPackage::get_deploymentview()
{
    if (deploymentview == 0) {
        const Q3PtrVector<UmlItem> & ch = UmlItem::children();
        UmlItem ** v = ch.data();
        UmlItem ** const vsup = v + ch.size();

        for (; v != vsup; v += 1) {
            // return the first class view found
            if ((*v)->kind() == aDeploymentView) {
                deploymentview = (UmlDeploymentView *) *v;
                return deploymentview;
            }
        }

        if ((deploymentview = UmlBaseDeploymentView::create(this, name())) == 0) {
#ifdef REVERSE
            UmlCom::trace(WrapperStr("<font face=helvetica><b>cannot create deployment view <i>")
                          + name() + "</i> under package <i>"
                          + name() + "</b></font><br>");
            UmlCom::message("");
            throw 0;
#else
            QMessageBox::critical(0, "Fatal Error",
                                  WrapperStr("<font face=helvetica><b>cannot create deployment view <i>")
                                  + name() + "</i> under package <i>"
                                  + name() + "</b></font><br>");
            QApplication::exit(1);
#endif
        }
    }

    return deploymentview;
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
    if (javaDir().isEmpty())
        package->accept_roundtrip_root();

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
