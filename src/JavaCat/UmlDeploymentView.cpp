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

#include "UmlDeploymentView.h"

#ifdef ROUNDTRIP
#include "UmlArtifact.h"
#include "UmlPackage.h"
#include "Package.h"
//Added by qt3to4:
#include <Q3PtrList>

static QList<UmlArtifact *> Artifacts;

bool UmlDeploymentView::set_roundtrip_expected()
{
    const Q3PtrVector<UmlItem> & ch = UmlItem::children();
    UmlItem ** v = ch.data();
    UmlItem ** const vsup = v + ch.size();
    bool result = isWritable();

    for (; v != vsup; v += 1)
        result &= (*v)->set_roundtrip_expected();

    return result;
}

void UmlDeploymentView::mark_useless(QList<UmlItem *> & l)
{
    Q3PtrVector<UmlItem> ch = UmlItem::children();
    UmlClassItem ** v = (UmlClassItem **) ch.data();
    UmlClassItem ** const vsup = v + ch.size();

    for (; v != vsup; v += 1)
        (*v)->mark_useless(l);
}

void UmlDeploymentView::scan_it(int & n)
{
    // compute artifact list
    const Q3PtrVector<UmlItem> & ch = UmlItem::children();
    UmlItem ** v = ch.data();
    UmlItem ** const vsup = v + ch.size();

    n = 0;

    for (; v != vsup; v += 1) {
        if (((*v)->kind() == anArtifact) &&
            ((UmlArtifact *) *v)->is_roundtrip_expected()) {
            Artifacts.append((UmlArtifact *) *v);
            n += 1;
        }
    }

    if (n != 0) {
        Package::set_step(1, n);

        Package * pk =
            ((UmlPackage *) Artifacts.first()->parent()->parent())->get_package();

        foreach (UmlArtifact *artifact, Artifacts)
            pk->reverse(artifact);

        Package::set_step(1, -1);
    }
}

void UmlDeploymentView::send_it(int n)
{
    if (n != 0) {
        Package::set_step(2, n);

        Package * pk =
            ((UmlPackage *) Artifacts.first()->parent()->parent())->get_package();

        foreach (UmlArtifact *artifact, Artifacts)
            pk->reverse(artifact);


        Package::set_step(2, -1);
    }
}

#endif
