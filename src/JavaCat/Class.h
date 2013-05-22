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
#include <Q3ValueList>
#include <Q3PtrList>

#ifdef REVERSE
class UmlArtifact;
#else
class QPixmap;
class QPainter;
class QColorGroup;
#endif

class Class;

class HistoricList
{
    QList<Class *> Historic;
    int current;
public:
    void appendClass(Class *klass);
    bool hasPrev() const;
    Class *prev();
    bool hasNext() const;
    Class *next();
};

class Class : public BrowserNode, public ClassContainer
{
protected:
    WrapperStr filename;
    UmlClass * uml;
    Q3Dict<UmlClass> user_defined;
    char stereotype;	// 'c' : class, 'i' : interface, '@' @interface, 'e' : enum
    bool abstractp;
    bool reversedp;
#ifdef REVERSE
    bool from_lib;
# ifdef ROUNDTRIP
    bool updated;
# endif

    static UmlArtifact * CurrentArtifact;
#else
    bool description_updatedp;
    WrapperStr description;

    static HistoricList Historic;
#endif
    FormalParameterList formals;

    bool get_formals(FormalParameterList & tmplt, bool name_only,
                     Q3ValueList<FormalParameterList> & tmplts);
    bool manage_extends(ClassContainer * container,
                        const Q3ValueList<FormalParameterList> & tmplts
#ifdef ROUNDTRIP
                        , bool roundtrip, QList<UmlItem *> & expected_order
#endif
                       );
    bool manage_implements(ClassContainer * container, aRelationKind k,
                           const Q3ValueList<FormalParameterList> & tmplts
#ifdef ROUNDTRIP
                           , bool roundtrip, QList<UmlItem *> & expected_order
#endif
                          );
    bool add_inherit(aRelationKind k, UmlTypeSpec & typespec,
                     Q3ValueList<UmlTypeSpec> & actuals, WrapperStr & str_actual
#ifdef ROUNDTRIP
                     , bool roundtrip, QList<UmlItem *> & expected_order
#endif
                    );
    void inherit(Class * cl);
    void inherit(UmlClass * uml_cl, WrapperStr header = 0);
    bool manage_member(WrapperStr s, WrapperStr & path
#ifdef ROUNDTRIP
                       , bool roundtrip, QList<UmlItem *> & expected_order
#endif
                      );
    bool manage_enum_items(
#ifdef ROUNDTRIP
        bool roundtrip, QList<UmlItem *> & expected_order
#endif
    );
    void set_description(const char * p);

#ifndef REVERSE
    virtual void activate();
    void manage_historic();
#endif
#ifndef ROUNDTRIP
    bool already_in_bouml();
#endif

public:
    Class(BrowserNode * parent, const char * n, char st);
#ifdef ROUNDTRIP
    Class(BrowserNode * parent, UmlClass * ucl);
#endif

    virtual void compute_type(WrapperStr type, UmlTypeSpec & typespec,
                              const Q3ValueList<FormalParameterList> & tmplts,
                              Class ** need_object = 0);
    virtual Class * define(const WrapperStr & name, char st);
    virtual void declare(const WrapperStr &, Class *);
    virtual bool isa_package() const;
    UmlClass * get_uml();
    bool reversed() const {
        return reversedp;
    };
    virtual QString get_path() const;

#ifdef ROUNDTRIP
    virtual Class * upload_define(UmlClass *);
    virtual Class * localy_defined(QString name) const;
    void set_updated() {
        updated = TRUE;
    }
#endif

#ifdef REVERSE
    static void new_artifact(UmlArtifact * a = 0) {
        CurrentArtifact = a;
    }
    static UmlArtifact * current_artifact() {
        return CurrentArtifact;
    }
    bool from_libp() const {
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

    static void restore(QDataStream & dt, char c, BrowserNode * p);

    // note tmplts equals its input value on exit
    static bool reverse(ClassContainer * container, WrapperStr stereotype,
                        WrapperStr annotation, bool abstractp, bool finalp,
                        aVisibility visibility,	WrapperStr & f,
                        Q3ValueList<FormalParameterList> tmplts
#ifdef ROUNDTRIP
                        , bool rndtrp, QList<UmlItem *> & expectedorder
#endif
                       );

#ifndef REVERSE
    static void historic_back();
    static void historic_forward();
#endif
};

#endif
