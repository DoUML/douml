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

#ifndef PACKAGE_H
#define PACKAGE_H

#include <q3dict.h>
#include <q3ptrlist.h>
//Added by qt3to4:
#include <QPixmap>
#include "misc/mystr.h"
#include <Q3ValueList>
#include <Q3PtrList>

#include "BrowserNode.h"
#include "ClassContainer.h"

class QDir;
class QFileInfo;
class QRegExp;
class UmlPackage;
class QApplication;

/* lgfreitas: Represents a UML package */
class Package : public BrowserNode, public ClassContainer
{
public:
#ifndef REVERSE
    Package(BrowserView * parent, UmlPackage * u);
#endif
    Package(Package * parent, QString p, QString n);
    virtual ~Package();	// just to not have warning

#ifdef ROUNDTRIP
    Package(Package * parent, UmlPackage * pk);

    virtual Class * upload_define(UmlClass *);
    void own(UmlArtifact *);
    void reverse(UmlArtifact *);
#endif
    void reverse_variable(const WrapperStr & name);

    Class * declare_if_needed(const WrapperStr & name,
                              WrapperStr stereotype = 0);
    virtual Class * define(const WrapperStr & name, WrapperStr stereotype = 0);
    virtual void declare_if_needed(WrapperStr name, Class * cl);
    virtual void define(WrapperStr name, Class * cl);
    virtual bool find_type(WrapperStr type, UmlTypeSpec & typespec);
    virtual void declaration(const WrapperStr & name, const WrapperStr & stereotype,
                             const WrapperStr & decl
#ifdef ROUNDTRIP
                             , bool roundtrip, QList<UmlItem *> & expected_order
#endif
                            );

    virtual bool isa_package() const;
    const WrapperStr & get_h_path() const {
        return h_path;
    };
    const WrapperStr & get_src_path() const {
        return src_path;
    };
    UmlPackage * get_uml(bool mandatory = TRUE);
#ifndef REVERSE
    virtual void menu();
    virtual void refer(const QString & name);

    const QPixmap * pixmap(int) const;

    virtual void backup(QDataStream  & dts) const;
    void backup_children(QDataStream  & ts) const;
#endif
    void restore_children(QDataStream & dts);
    static void restore(QDataStream  & dt, Package *);

    static void init(UmlPackage *, QApplication *);
    static bool scanning() {
        return Scan;
    };
    static void set_step(int s, int n);
#ifdef ROUNDTRIP
    void scan_dir(int & n);
    void send_dir(int n);
    void accept_roundtrip_root(bool h);
#else
    static void scan_dirs(int & n);
    static void send_dirs(int n, bool rec);
#endif

    static const WrapperStr & get_fname() {
        return fname;
    }
#ifdef ROUNDTRIP
    static UmlArtifact * get_artifact() {
        return artfct;
    }
#endif

    //static Package * unknown();

private:
    UmlPackage * uml;
    WrapperStr namespace_;
    WrapperStr h_path;	// empty or finish by a /
    WrapperStr src_path;	// empty or finish by a /

    static QApplication * app;
    static QList<Package *> Choozen; /* List of chosen "packages" to reverse */
    static int Nfiles;
    static bool Scan;
    static Package * Root;
    static QRegExp * DirFilter;
    static QRegExp * FileFilter;
    //static Package * Unknown;
    static Q3ValueList<FormalParameterList> Formals;
    static QList<UmlClass *> UsedClasses;

    static NDict<Class> Declared;
    static NDict<Class> Defined;

    static WrapperStr fname;	// without extension, empty for non header file
#ifdef ROUNDTRIP
    static UmlArtifact * artfct; // currently roundtriped artifact
#endif
    /* lgfreitas: Reverse directories.
    	I suspect that parameter h means "ITS A HEADER"
    */
    void reverse_directory(QString path, bool rec, QString ext, bool h);
#ifdef ROUNDTRIP
    void reverse_file(WrapperStr f, UmlArtifact * art, bool h);
#else
    /* lgfreitas: This does the reversing of files */
    void reverse_file(WrapperStr f);
#endif
    /* lgfreitas: This seems to be where the reversing is finally done, word by word. */
    void reverse_toplevel_forms(WrapperStr f, bool sub_block);
    void reverse_toplevel_form(WrapperStr f, WrapperStr s);

    Class * new_class(const WrapperStr & name, const WrapperStr & stereotype,
                      bool declaration);

    void set_namespace(WrapperStr s);
    Package * find(QFileInfo * di);

#ifdef ROUNDTRIP
    int count_file_number();
    void scan_dir();
    void send_dir();
#endif

    static int file_number(QString path, bool rec, const char * ext);
};

#endif
