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
#include "Namespace.h"
//Added by qt3to4:
#include <QPixmap>
#include "misc/mystr.h"
#include <Q3ValueList>
#include <Q3PtrList>

class Class : public BrowserNode, public ClassContainer
{
protected:
    WrapperStr filename;
    WrapperStr stereotype;
    WrapperStr its_namespace;
    UmlClass * uml;
    bool reversedp;
    bool stereotype_declared;
#ifdef REVERSE
    bool from_lib;
# ifdef ROUNDTRIP
    bool updated;
# endif
    FormalParameterList formals;
#else
    bool have_formals;
#endif
    NDict<Class> declared;
    NDict<Class> defined;

    void manage_member(WrapperStr s, aVisibility visibility,
                       ClassContainer * container, const WrapperStr & path
#ifdef ROUNDTRIP
                       , bool roundtrip, QList<UmlItem *> & expected_order
#endif
                      );

public:
    Class(BrowserNode * parent, const char * name, const WrapperStr & st);
#ifdef ROUNDTRIP
    Class(BrowserNode * parent, UmlClass * ucl);
#endif
    virtual ~Class();	// just to not have warning

    bool set_stereotype(const WrapperStr & st);

    virtual bool find_type(WrapperStr type, UmlTypeSpec & typespec);
    virtual Class * declare_if_needed(const WrapperStr & name,
                                      WrapperStr stereotype = 0);
    virtual Class * define(const WrapperStr & name, WrapperStr stereotype = 0);
    virtual void declare_if_needed(WrapperStr name, Class * cl);
    virtual void define(WrapperStr name, Class * cl);
    virtual Class * new_class(const WrapperStr & name,
                              const WrapperStr & stereotype,
                              bool declaration);
    virtual void declaration(const WrapperStr & name, const WrapperStr & stereotype,
                             const WrapperStr & decl
#ifdef ROUNDTRIP
                             , bool roundtrip, QList<UmlItem *> & expected_order
#endif
                            );

    virtual bool isa_package() const;
    const WrapperStr & get_namespace() const {
        return its_namespace;
    }
    UmlClass * get_uml();
    void set_uml(UmlClass *);
    bool reversed() const {
        return reversedp;
    };
    bool already_in_bouml();
    virtual QString get_path() const;
#ifdef ROUNDTRIP
    virtual Class * upload_define(UmlClass *);
    void set_updated() {
        updated = TRUE;
    }
    bool is_roundtrip_expected() const;
#endif

#ifdef REVERSE
    bool from_libp() {
        return from_lib;
    };
#else
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

    static Class * reverse(ClassContainer * container,
                           WrapperStr stereotype,
                           const Q3ValueList<FormalParameterList> & tmplt,
                           const WrapperStr & path, WrapperStr name
#ifdef ROUNDTRIP
                           , bool rndtrp, QList<UmlItem *> & expectedorder
#endif
                          );
    static Class * reverse_enum(ClassContainer * container,
                                const WrapperStr & path, WrapperStr name
#ifdef ROUNDTRIP
                                , bool rndtrp, QList<UmlItem *> & expectedorder
#endif
                               );
    static bool reverse_typedef(ClassContainer * container, const WrapperStr & path,
                                Q3ValueList<FormalParameterList> & tmplts
#ifdef ROUNDTRIP
                                , bool rndtrp, QList<UmlItem *> & expectedorder
#endif
                               );
};

#endif
