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

#include "UmlArtifact.h"

#ifdef ROUNDTRIP
#include "UmlPackage.h"
#include "Package.h"
//Added by qt3to4:
#include <Q3PtrList>

bool UmlArtifact::has_roundtrip_expected;
UmlArtifact * UmlArtifact::main_art;

bool UmlArtifact::set_roundtrip_expected()
{
    if ((stereotype() != "source") || cppSource().isEmpty())
        return TRUE;

    const Q3PtrVector<UmlClass> & cls = associatedClasses();

    if (cls.isEmpty()) {
        if ((name() == "main") && cppHeader().isEmpty())
            main_art = this;
        else
            return TRUE;
    }

    has_roundtrip_expected = TRUE;
    roundtrip_expected = TRUE;
    useless = TRUE;
    fully_updated = TRUE;
    ((UmlPackage *) parent()->parent())->get_package()->own(this);

    UmlClass ** v = cls.data();
    UmlClass ** vsup = v + cls.size();
    bool result = isWritable();

    for (; v != vsup; v += 1)
        result &= (*v)->set_roundtrip_expected();

    return result;
}

bool UmlArtifact::set_roundtrip_expected_for_class()
{
    if (roundtrip_expected)
        return TRUE;

    if ((stereotype() != "source") || cppSource().isEmpty())
        return FALSE;

    has_roundtrip_expected = TRUE;
    roundtrip_expected = TRUE;
    useless = TRUE;
    ((UmlPackage *) parent()->parent())->get_package()->own(this);

    return TRUE;
}

bool UmlArtifact::is_roundtrip_usefull()
{
    return has_roundtrip_expected;
}

UmlArtifact * UmlArtifact::get_main()
{
    return main_art;
}

void UmlArtifact::mark_useless(QList<UmlItem *> & l)
{
    if (useless) {
        set_isMarked(TRUE);
        parent()->set_childrenVisible(TRUE);
        l.append(this);
    }
}

bool UmlArtifact::is_considered(bool h, bool scan) const
{
    return (scan)
           ? ((h) ? h_scanned : src_scanned)
               : ((h) ? h_reversed : src_reversed);
}

void UmlArtifact::set_considered(bool h, bool scan)
{
    if (scan) {
        if (h)
            h_scanned = TRUE;
        else
            src_scanned = TRUE;
    }
    else {
        if (h)
            h_reversed = TRUE;
        else
            src_reversed = TRUE;
    }
}

void UmlArtifact::scan_it(int &)
{
    if (roundtrip_expected) {
        Package::set_step(1, 1);
        ((UmlPackage *) parent()->parent())->get_package()->reverse(this);
        Package::set_step(1, -1);
    }
}

void UmlArtifact::send_it(int n)
{
    if (roundtrip_expected) {
        Package::set_step(2, n);
        ((UmlPackage *) parent()->parent())->get_package()->reverse(this);
        Package::set_step(2, -1);
    }
}

#endif
