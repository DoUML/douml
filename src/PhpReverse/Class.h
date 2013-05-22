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

#ifndef CLASS_H
#define CLASS_H

#include "BrowserNode.h"
#include "ClassContainer.h"
#include "UmlClass.h"
#include "aRelationKind.h"
//Added by qt3to4:
#include <QPixmap>
#include "misc/mystr.h"
#include <Q3PtrList>

class Package;
class UmlArtifact;
#ifndef REVERSE
class QPixmap;
class QPainter;
class QColorGroup;
#endif

class Class : public BrowserNode
{
protected:
    WrapperStr filename;
    WrapperStr its_namespace;
    UmlClass * uml;
    char stereotype;	// 'c' : class, 'i' : interface, '@' @interface, 'e' : enum
    bool abstractp;
    bool reversedp;
#ifdef REVERSE
    bool from_lib;
#else
    bool description_updatedp;
    WrapperStr description;

    static QList<Class *> Historic;
#endif

    bool manage_extends(ClassContainer * container);
    bool manage_implements(ClassContainer * container, aRelationKind k);
    bool add_inherit(aRelationKind k, UmlTypeSpec & typespec);
    bool manage_member(WrapperStr s);
    void set_description(const char * p);

#ifndef REVERSE
    virtual void activate();
    void manage_historic();
#endif

public:
    Class(Package * parent, const char * n, char st);

    void compute_type(WrapperStr type, UmlTypeSpec & typespec,
                      Class ** need_object = 0);
    const WrapperStr & get_namespace() const {
        return its_namespace;
    }
    UmlClass * get_uml();
    bool reversed() const {
        return reversedp;
    };
    bool already_in_bouml();
    virtual QString get_path() const;
#ifndef REVERSE
    const WrapperStr & get_description() const {
        return description;
    };

    virtual void selected();
    virtual void activated();
    virtual void menu();
    virtual void refer(const QString & name);

    const QPixmap * pixmap(int) const;
    void paintCell(QPainter * p, const QColorGroup & cg, int column,
                   int width, int alignment);


    virtual void backup(QDataStream & dt) const;
#endif

#ifdef WITH_PHPCAT
    static void restore(QDataStream & dt, char c, Package * p);
#endif

    static bool reverse(Package * container, WrapperStr stereotype,
                        bool abstractp, bool finalp,
                        WrapperStr & f, UmlArtifact * art);

#ifndef REVERSE
    static void historic_back();
    static void historic_forward();
#endif
};

#endif
