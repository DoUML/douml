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

#ifndef UMLCLASS_H
#define UMLCLASS_H

#include "UmlBaseClass.h"
//Added by qt3to4:
#include "misc/mystr.h"
#include <Q3PtrList>

#ifdef ROUNDTRIP
class Class;
class ClassContainer;
#endif

#ifdef REVERSE
#include <qstringlist.h>
#endif

class UmlClass : public UmlBaseClass
{
#ifdef ROUNDTRIP
private:
    bool created;
    Class * the_class;
#endif

public:
    UmlClass(void * id, const WrapperStr & n);
#ifdef REVERSE
    void need_artifact(const QStringList & imports, bool remove_java_lang,
                       const QStringList & static_imports,
                       const WrapperStr & path, UmlArtifact *& cp);

# ifdef ROUNDTRIP
    virtual void upload(ClassContainer * cnt);
    virtual bool set_roundtrip_expected();
    virtual void mark_useless(QList<UmlItem *> & l);
    virtual void scan_it(int & n);
    virtual void send_it(int n);
    bool is_created() const {
        return created;
    }
    void set_created() {
        created = TRUE;
    }
    Class * get_class() const {
        return the_class;
    }
    UmlItem * search_for_att_rel(const WrapperStr & name);
    void reorder(QList<UmlItem *> & expected_order);
# endif
#endif
    static void manage_generic(WrapperStr & form, UmlTypeSpec & typespec,
                               WrapperStr str_actuals, const char * k);
};

#endif
