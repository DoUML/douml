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

#include <q3filedialog.h>
#include <qapplication.h>
#ifndef REVERSE
#include <qmessagebox.h>
#include <q3popupmenu.h>
#include <qcursor.h>
#include <qinputdialog.h>
#endif
#include <qdatastream.h>
#include <qdir.h>
#include <qregexp.h>
#ifdef ROUNDTRIP
#include <qmessagebox.h>
//Added by qt3to4:
#include <Q3ValueList>
#include "misc/mystr.h"
#include <QPixmap>
//Added by qt3to4:
#include <Q3PtrList>
#endif

#include "Package.h"
#include "Class.h"
#include "UmlPackage.h"
#include "UmlClass.h"
#ifdef REVERSE
#include "UmlArtifact.h"
#else
#include "BrowserView.h"
#include "Pixmap.h"
#endif
#include "Progress.h"
#include "Lex.h"
#include "UmlOperation.h"
#include "JavaCatWindow.h"
#include "JavaSettings.h"
#include "UmlCom.h"

// create all packages under it
Package * Package::root;

// to place unknown classes
Package * Package::unknown;

// regexp to filter dirs/files
QRegExp * Package::DirFilter;
QRegExp * Package::FileFilter;

// "java"
QString Package::Ext;

// to know if it is the scan step or the reverse step
bool Package::scan;

// all the classes presents in the environment, the
// key is the full name including package
Q3Dict<Class> Package::classes(1001);

// the java classes, the key are just the name without package
Q3Dict<Class> Package::java_classes(1001);

// all the classes defined in bouml, not known through a .cat
// nor reversed, the key is the full name including package
Q3Dict<UmlClass> Package::user_classes(1001);

// packages known through a .cat or reversed, the key is the
// Java package spec
Q3Dict<Package> Package::known_packages(101);

// all the packages defined in bouml, not known through a .cat
// nor reversed (except roundtrip), the key is the java package spec
Q3Dict<UmlPackage> Package::user_packages(101);

// package which does not exist even in bouml
QStringList Package::unknown_packages;

// to know if java.lang. was artificialy added in Package::imports
bool Package::java_lang_added;

// the current import list
QStringList Package::imports;

// the current static import list
QStringList Package::static_imports;

// to save / restore the context
QStack<QStringList> Package::stack;

// current template forms
Q3ValueList<FormalParameterList> Package::Formals;

QApplication * Package::app;

static QString RootSDir;	// empty or finish by a /
static WrapperStr RootCDir;	// empty or finish by a /

static Q3Dict<bool> FileManaged;

#ifdef ROUNDTRIP
static Q3Dict<bool> DirManaged;
#endif

static WrapperStr force_final_slash(WrapperStr p)
{
    int ln = p.length();

    if (ln < 2)
        return p;

    return (p[ln - 1] != '/')
           ? WrapperStr(p + '/')
           : p;
}

static inline WrapperStr force_final_slash(QString p)
{
    return force_final_slash(WrapperStr(p.toAscii().constData()));
}

static WrapperStr root_relative_if_possible(WrapperStr p)
{
    int pln = p.length();
    int rln = RootCDir.length();

    //todo try to fix root relative
    return ((pln >= rln) && (p.left(rln).length() == (unsigned)rln)) // [lgfreitas] i am assuming its comparing lengths...
           ? p.mid(rln)
           : p;
}

#ifndef REVERSE
Package::Package(BrowserView * parent, UmlPackage * u)
    : BrowserNode(parent, u->name())
{
    uml = u;
}
#endif

Package::Package(Package * parent, const char * p, const char * n)
    : BrowserNode(parent, n)
{
    path = p;

    if (path != "<unknown>")
        path = force_final_slash(path);

    uml = 0;
}

#ifdef ROUNDTRIP
Package::Package(Package * parent, UmlPackage * pk)
    : BrowserNode(parent, pk->name())
{
    if (text(0) == "unknown")
        unknown = this;

    uml = pk;

    QDir d_root(RootSDir);

    path = force_final_slash(pk->javaDir());

    if (path.isEmpty())
        path = RootCDir;
    else if (QDir::isRelativePath(path)) {
        if (RootCDir.isEmpty()) {
            WrapperStr err = "<font face=helvetica><b>root path not set in <i>generation settings</i>, "
                            "don't know where is <i>" + path + "<i></b></font><br>";

            UmlCom::trace(err);
        }
        else
            path = force_final_slash(d_root.filePath(path));
    }

    package = pk->javaPackage();

    if (!package.isEmpty()) {
        QString full = package;

        user_packages.insert(full, pk);
        known_packages.insert(full, this);
    }
}
#endif

bool Package::isa_package() const
{
    return TRUE;
}

#ifndef REVERSE
QString Package::get_path() const
{
    return path;
}
#endif

void Package::init(UmlPackage * r, QApplication * a)
{
    app = a;

    RootCDir = force_final_slash(JavaSettings::rootDir());
    RootSDir = RootCDir.operator QString();

    if (!RootSDir.isEmpty() && QDir::isRelativePath(RootSDir)) {
        QFileInfo f(UmlPackage::getProject()->supportFile());
        QDir d(f.dirPath());

        RootSDir = force_final_slash(d.filePath(RootSDir)).operator QString();
        RootCDir = RootSDir;
    }

    QString s;

    DirFilter = (!(s = (const char *) JavaSettings::reverseRoundtripDirRegExp()).isEmpty())
                ? new QRegExp(s, JavaSettings::isReverseRoundtripDirRegExpCaseSensitive(), TRUE)
                : 0;

    FileFilter = (!(s = (const char *) JavaSettings::reverseRoundtripFileRegExp()).isEmpty())
                 ? new QRegExp(s, JavaSettings::isReverseRoundtripFileRegExpCaseSensitive(), TRUE)
                 : 0;

    Ext = JavaSettings::sourceExtension().operator QString();

#ifdef ROUNDTRIP
    root = new Package(0, r);
    r->init(root);
#elif defined(REVERSE)
    root = new Package(0, 0, r->name());
    root->uml = r;
#else
    root = new Package(BrowserView::instance(), r);
#endif
}

void Package::new_class(Class * cl)
{
    if ((package == "java") || (package == "java.lang"))
        java_classes.insert(cl->text(0), cl);
}

void Package::set_step(int s, int n)
{
    if (n == -1) {
        scan = FALSE;
        Progress::delete_it();
    }
    else {
        switch (s) {
#ifdef ROUNDTRIP

        case 0:
            scan = FALSE;

            if (n > 1)
                new Progress(n, "Preparation, please wait ...", app);

            break;
#endif

        case 1:
            scan = TRUE;
            FileManaged.resize(101);

            if (n > 1)
                new Progress(n, "Scanning in progress, please wait ...", app);

            break;

        default:
            // 2
            scan = FALSE;
            FileManaged.clear();
            FileManaged.resize(101);

            if (n > 1)
#ifdef ROUNDTRIP
                new Progress(n, "Roundtrip in progress, please wait ...", app);

#else
                new Progress(n, "Reverse in progress, please wait ...", app);
#endif
        }
    }
}

#ifdef ROUNDTRIP
void Package::own(UmlArtifact * art)
{
    roundtriped.insert(art->name(), art);
}

int Package::count_file_number()
{
    if (path.isEmpty())
        return 0;

    QDir d(path);
    int result = file_number(d, TRUE);
    foreach (TreeItem *child, children())
        if (((BrowserNode *) child)->isa_package())
            result += ((Package *) child)->count_file_number();

    return result;
}

void Package::scan_dir(int & n)
{
    n = count_file_number();
    DirManaged.clear();

    set_step(1, n);
    scan_dir();
    DirManaged.clear();
    set_step(1, -1);
}

void Package::scan_dir()
{
    QDir d(path);

    reverse_directory(d, TRUE);

    foreach (TreeItem *child, children())
        if (((BrowserNode *) child)->isa_package())
            ((Package *) child)->scan_dir();
}
#else
Package * Package::scan_dir(int & n)
{
    n = 0;

    // get input java source dir
    QString path = Q3FileDialog::getExistingDirectory(QDir::currentDirPath(), 0, 0,
                   "select the base directory to reverse");

    if (! path.isEmpty()) {
        QDir d(path);

        Package * p = new Package(root, path, d.dirName());

        // scanning phase
#ifndef REVERSE
        QApplication::setOverrideCursor(Qt::waitCursor);
        JavaCatWindow::clear_trace();
#endif
        UmlCom::message("count files ...");
        n = file_number(d, TRUE);
        set_step(1, n);
        p->reverse_directory(d, TRUE);
        set_step(1, -1);
#ifndef REVERSE
        QApplication::restoreOverrideCursor();

        root->setOpen(TRUE);
#endif

        return p;
    }
    else
        return 0;
}
#endif

static bool allowed(QRegExp * rg, QString f)
{
    if (rg != 0) {
        return rg->exactMatch(f); //[lgfreitas] I do not know if this will work
        /*return ((rg->match(f, 0, &matchLen) != 0) ||
            (matchLen != (int) f.length()));*/
    }
    else
        return TRUE;
}

int Package::file_number(QDir & d, bool rec)
{
    if (! allowed(DirFilter, d.dirName()))
        return 0;

#ifdef ROUNDTRIP

    if (DirManaged[d.path()] != 0)
        return 0;

    DirManaged.insert(d.path(), (bool *) 1);
#endif

    int result = 0;
    /*const QFileInfoList * list = d.entryInfoList(QDir::Files | QDir::Readable);

    if (list != 0) {
      QFileInfoListIterator it(*list);
      QFileInfo * fi;

      while ((fi = it.current()) != 0) {
        if (fi->extension(FALSE) == Ext)
    result += 1;

        ++it;
      }
    }*/
    QFileInfoList list = d.entryInfoList(QDir::Files | QDir::Readable);

    if (!list.isEmpty()) {
        QFileInfoList::iterator it = list.begin();

        while (it != list.end()) {
            if ((*it).extension(FALSE) == Ext) result += 1;

            it++;
        }
    }

    if (rec) {
        // sub directories
        /*list = d.entryInfoList(QDir::Dirs | QDir::NoSymLinks);

        if (list != 0) {
          QFileInfoListIterator itd(*list);
          QFileInfo * di;

          while ((di = itd.current()) != 0) {
        if (((const char *) di->fileName())[0] != '.') {
          QDir sd(di->filePath());

          result += file_number(sd, rec);
        }
        ++itd;
          }
        }*/
        list = d.entryInfoList(QDir::Dirs | QDir::NoSymLinks);

        if (!list.isEmpty()) {
            QFileInfoList::iterator it = list.begin();

            while (it != list.end()) {
                if ((*it).fileName()[0] != '.') {
                    QDir sd((*it).filePath());
                    result += file_number(sd, rec);
                }

                it++;
            }
        }
    }

    return result;
}

#ifdef ROUNDTRIP
enum ReverseRootCases { DontKnow, ReverseRoot, DontReverseRoot };

void Package::accept_roundtrip_root()
{
    static ReverseRootCases reverseRoot = DontKnow;

    switch (reverseRoot) {
    case DontKnow:
        switch (QMessageBox::warning(0, "Java Roundtrip",
                                     "directory not specified at package level, "
                                     "do you want to reverse all files under <i>"
                                     + RootSDir + "</i> and its sub directories ?",
                                     "yes", "no", "cancel", 0, 2)) {
        case 0:
            reverseRoot = ReverseRoot;
            break;

        case 1:
            reverseRoot = DontReverseRoot;
            path = 0;
            break;

        default: // 2
            throw 0;
        }

        break;

    case ReverseRoot:
        break;

    default: // DontReverseRoot
        path = 0;
        break;
    }
}

void Package::send_dir(int n)
{
    // reverse phase
    UmlPackage::getProject()->set_childrenVisible(FALSE);

    QDir d(path);

    set_step(2, n);
    send_dir();
    DirManaged.clear();
    set_step(2, -1);

    ((uml) ? uml : root->uml)->set_childrenVisible(TRUE);
}

void Package::send_dir()
{
    QDir d(path);

    reverse_directory(d, TRUE);

    foreach (TreeItem *child, children())
        if (((BrowserNode *) child)->isa_package())
            ((Package *) child)->send_dir();
}
#elif defined(REVERSE)
void Package::send_dir(int n)
{
    // reverse phase
    UmlPackage::getProject()->set_childrenVisible(FALSE);

    QDir d(path);

    set_step(2, n);
    reverse_directory(d, TRUE);
    set_step(2, -1);

    ((uml) ? uml : root->uml)->set_childrenVisible(TRUE);
}
#endif

// void Package::reverse_directory(QDir & d, bool rec) {
// #ifdef ROUNDTRIP
//   if (d.path().isEmpty() || DirManaged.find(d.path()) != 0)
//     return;
//
//   DirManaged.insert(d.path(), (bool *) 1);
// #endif
//
//   if (! allowed(DirFilter, d.dirName()))
//     return;
//
//   // reads files
//   const QFileInfoList * list = d.entryInfoList(QDir::Files | QDir::Readable);
//
//   if (list != 0) {
//     QFileInfoListIterator it(*list);
//     QFileInfo * fi;
//
//     while ((fi = it.current()) != 0) {
//       if (fi->extension() == Ext) {
// 	if (allowed(FileFilter, fi->fileName()))
// 	  reverse_file(WrapperStr(fi->filePath())
// #ifdef ROUNDTRIP
// 		       , roundtriped.find(fi->baseName())
// #endif
// 		       );
// 	Progress::tic_it();
//       }
//       ++it;
//     }
//   }
//
//   if (rec) {
//     // sub directories
//     list = d.entryInfoList(QDir::Dirs | QDir::NoSymLinks);
//
//     if (list != 0) {
//       QFileInfoListIterator itd(*list);
//       QFileInfo * di;
//
//       while ((di = itd.current()) != 0) {
// 	if (((const char *) di->fileName())[0] != '.') {
// 	  QDir sd(di->filePath());
// #ifndef REVERSE
// 	  if (allowed(DirFilter, sd.dirName())) {
// #endif
// 	    Package * p = find(WrapperStr(sd.dirName()), TRUE);
//
// 	    if (p != 0)
// 	      p->reverse_directory(sd, TRUE);
// #ifndef REVERSE
// 	  }
// #endif
// 	}
// 	++itd;
//       }
//     }
//   }
// }
void Package::reverse_directory(QDir & d, bool rec)
{
#ifdef ROUNDTRIP

    if (d.path().isEmpty() || DirManaged.find(d.path()) != 0)
        return;

    DirManaged.insert(d.path(), (bool *) 1);
#endif

    if (! allowed(DirFilter, d.dirName()))
        return;

    // reads files
    QFileInfoList list = d.entryInfoList(QDir::Files | QDir::Readable);

    if (!list.isEmpty()) {
        QFileInfoList::iterator it = list.begin();

        while (it != list.end()) {
            if ((*it).extension() == Ext) {
                if (allowed(FileFilter, (*it).fileName()))
                    reverse_file(WrapperStr((*it).filePath().toAscii().constData())
#ifdef ROUNDTRIP
                                 , roundtriped.find((*it).baseName())
#endif
                                );

                Progress::tic_it();
            }

            ++it;
        }
    }

    if (rec) {
        // sub directories
        list = d.entryInfoList(QDir::Dirs | QDir::NoSymLinks);

        if (!list.isEmpty()) {
            QFileInfoList::iterator itd = list.begin();

            while (itd != list.end()) {
                if ((*itd).fileName()[0] != '.') {
                    QDir sd((*itd).filePath());
#ifndef REVERSE

                    if (allowed(DirFilter, sd.dirName())) {
#endif
                        Package * p = find(WrapperStr(sd.dirName().toAscii().constData()), TRUE);

                        if (p != 0)
                            p->reverse_directory(sd, TRUE);

#ifndef REVERSE
                    }

#endif
                }

                ++itd;
            }
        }
    }
}

#ifdef ROUNDTRIP
void Package::reverse(UmlArtifact * art)
{
    if (allowed(FileFilter, art->name() + "." + Ext))
        reverse_file(path + "/" + art->name() + "." + WrapperStr(Ext.toAscii().constData()), art);
}
#endif

void Package::reverse_file(WrapperStr f
#ifdef ROUNDTRIP
                           , UmlArtifact * art
#endif
                          )
{
    if (FileManaged[f])
        return;

    FileManaged.insert(f, (bool *) 1);

    if (! Lex::open(f)) {
#ifdef ROUNDTRIP

        if (art != 0) {
            UmlCom::trace(WrapperStr("<font face=helvetica><b>cannot open <i>")
                          + f + "</i></b></font><br>");
            throw 0;
        }
        else
#endif

            // very strange !
            if (!scan)
                UmlCom::trace(WrapperStr("<font face=helvetica><b>cannot open <i>")
                              + f + "</i></b></font><br>");
    }
    else {
#ifdef ROUNDTRIP

        if (!scan) {
            if ((art != 0) && art->is_fully_updated()) {
                art->set_usefull();
                art->set_JavaSource(JavaSettings::sourceContent());
                art->set_Description("");

                Q3PtrVector<UmlClass> empty;

                art->set_AssociatedClasses(empty);
            }

            Class::new_artifact(art);
        }

#elif defined(REVERSE)
        Class::new_artifact(0);
#endif

        for (;;) {		// not a true loop, to use break on error
            UmlCom::message(((scan) ? "scan " : "reverse ") + f);

            WrapperStr s = Lex::read_word();
#ifdef REVERSE
            WrapperStr art_comment;
            WrapperStr art_description;
#endif

            if (s == "package") {
#ifdef REVERSE

                if (! scan) {
                    art_comment = Lex::get_comments();
                    art_description = Lex::get_description();
                }

#endif

                if ((s = Lex::read_word()).isEmpty()) {
                    Lex::premature_eof();
                    break;
                }

                if (Lex::read_word() != ";") {
                    Lex::error_near(s, " ';' expected");
                    break;
                }

                if (scan)
                    set_package(s);

                Lex::finish_line();
                Lex::clear_comments();
                s = Lex::read_word();
            }

#ifdef REVERSE
            else if (!scan && (s == "import")) {
                art_comment = Lex::get_comments();
                art_description = Lex::get_description();
            }

#endif

            imports.clear();
            static_imports.clear();

            while (s == "import") {
                manage_import();
                s = Lex::read_word();
            }

            if (imports.findIndex("java.lang.") == -1) {
                imports.append("java.lang.");
                java_lang_added = TRUE;
            }
            else
                java_lang_added = FALSE;

            aVisibility visibility = PackageVisibility;
            bool abstractp = FALSE;
            bool finalp = FALSE;
            WrapperStr annotation;

            while (!s.isEmpty()) {
                if ((s == "class") || (s == "enum") ||
                    (s == "interface") || (s == "@interface")) {
#ifdef ROUNDTRIP
                    QList<UmlItem *> dummy;
#endif

                    if (!Class::reverse(this, s, annotation, abstractp,
                                        finalp, visibility, f, Formals
#ifdef ROUNDTRIP
                                        , FALSE, dummy
#endif
                                       ))
                        break;

                    visibility = PackageVisibility;
                    abstractp = FALSE;
                    finalp = FALSE;
                }
                else if (s == "public")
                    visibility = PublicVisibility;
                else if (s == "protected")
                    visibility = ProtectedVisibility;
                else if (s == "private")
                    visibility = PrivateVisibility;
                else if (s == "final")
                    finalp = TRUE;
                else if (s == "abstract")
                    abstractp = TRUE;
                else if ((s == "strictfp") ||
                         (s == "synchronized"))	// !
                    ;
                else if (*s == '@')
                    annotation = s;
                else if (s != ";") {
                    if (! scan)
                        Lex::error_near(s);

                    UmlOperation::skip_body(0);
                    Lex::finish_line();
                    Lex::clear_comments();
                }

                s = Lex::read_word();
            }

#ifdef REVERSE

            if (! scan) {
                UmlArtifact * art = Class::current_artifact();

                if ((art != 0) &&
# ifdef ROUNDTRIP
                    art->is_fully_updated() &&
# endif
                    !art_comment.isEmpty())
                    art->set_Description((art->javaSource().find("${description}") != -1)
                                         ? art_description : Lex::simplify_comment(art_comment));
            }

#endif

            break;
        }

        Formals.clear();
        Lex::close();
    }
}

void Package::manage_import()
{
    WrapperStr s;
    WrapperStr s2;

    if ((s = Lex::read_word()).isEmpty()) {
        if (! scan)
            Lex::premature_eof();
    }
    else if (s == "static") {
        Lex::mark();

        char c;

        while ((c = Lex::read_word_bis()) != ';') {
            if (c == 0) {
                if (! scan)
                    Lex::premature_eof();

                return;
            }
        }

        if (! scan)
            static_imports.append(Lex::region());
    }
    else if ((s2 = Lex::read_word()).isEmpty()) {
        if (! scan)
            Lex::premature_eof();
    }
    else {
        if (s2 == "*") {
            if (! scan)
                update_package_list(s.left(s.length() - 1));

            s2 = Lex::read_word();	// probably the ;
        }
        else {
            while (Lex::identifierp(s2)) {
                // to manage line break inside a.b...z
                s += s2;

                if ((s2 = Lex::read_word()).isEmpty()) {
                    if (! scan)
                        Lex::premature_eof();

                    return;
                }
            }

            if (! scan)
                update_package_list(s);
        }

        // memorize x.y. or  x.y.z
        imports.append(s);

        if (s2 != ";") {
            if (! scan)
                Lex::syntax_error();

            UmlOperation::skip_body(0);
        }

        Lex::finish_line();
        Lex::clear_comments();
    }
}

void Package::update_package_list(WrapperStr name)
{
    if ((known_packages[name] == 0) &&
        (user_packages[name] == 0) &&
        (classes[name] == 0) &&
        (user_classes[name] == 0) &&
        (unknown_packages.findIndex(name) == -1)) {
        // try to find the package in bouml
        UmlPackage * up = UmlPackage::getProject()->findJavaPackage(name);

        if (up == 0) {
            int index;

            if ((index = name.operator QString().lastIndexOf('.')) != -1) {
                WrapperStr subname = name.left(index);

                if ((user_packages[subname] != 0) ||
                    (unknown_packages.findIndex(subname) != -1))
                    return;

                if ((up = UmlPackage::getProject()->findJavaPackage(subname)) != 0)
                    name = subname;
                else {
                    unknown_packages.append(subname);
                    unknown_packages.append(name);
                    return;
                }
            }
            else {
                unknown_packages.append(name);
                return;
            }
        }

        user_packages.insert(name, up);

        update_class_list(name + ".", up);
    }
}

void Package::update_class_list(WrapperStr pack, UmlItem * container)
{
    const Q3PtrVector<UmlItem> & ch = container->children();
    UmlItem ** v = ch.data();
    UmlItem ** const vsup = v + ch.size();
    UmlItem * it;

    for (; v != vsup; v += 1) {
        it = *v;

        switch (it->kind()) {
        case aClass:
            user_classes.replace(pack + it->name(), (UmlClass *) it);
            update_class_list(pack + it->name() + ".", (UmlClass *) it);
            break;

        case aClassView:
            update_class_list(pack, it);

        default:
            break;
        }
    }
}

Class * Package::define(const WrapperStr & name, char st)
{
    Class * cl = Defined[name];

    if (cl != 0)
        return cl;

    if ((cl = Undefined[name]) == 0) {
        // not yet referenced
        cl = declare_if_needed(name, st);
    }

    if (scan) {
        classes.insert(package + "." + name, cl);

        if (classes.count() / 2 >= classes.size())
            classes.resize(classes.size() * 2 - 1);
    }
    else {
        Defined.insert(name, cl);
        Undefined.remove(name);
    }

    return cl;
}

#ifdef ROUNDTRIP
Class * Package::upload_define(UmlClass * ucl)
{
    Class * cl = new Class(this, ucl);
    WrapperStr s = ucl->name();	// not defined inside an other class

    classes.insert(package + "." + s, cl);
    Defined.insert(s, cl);

    return cl;
}

Class * Package::localy_defined(QString name) const
{
    return Defined[name];
}
#endif

void Package::compute_type(WrapperStr name, UmlTypeSpec & typespec,
                           const Q3ValueList<FormalParameterList> & tmplts,
                           Class ** need_object)
{
    typespec.type = 0;
    typespec.explicit_type = 0;

    if ((name == "void") || (name == "boolean") || (name == "byte") ||
        (name == "char") || (name == "short") || (name == "int") ||
        (name == "long") || (name == "float") || (name == "double")) {
        if ((typespec.explicit_type = JavaSettings::umlType(name)).isEmpty())
            typespec.explicit_type = name;
    }
    else {
        Class * cl;

        if (! tmplts.isEmpty()) {
            Q3ValueList<FormalParameterList>::ConstIterator it1;

            for (it1 = tmplts.begin(); it1 != tmplts.end(); ++it1) {
                FormalParameterList::ConstIterator it2;

                for (it2 = (*it1).begin(); it2 != (*it1).end(); ++it2) {
                    if ((*it2).name() == name) {
                        // find !
                        if (need_object != 0)
                            force_class(name, typespec, need_object);
                        else {
                            typespec.type = 0;
                            typespec.explicit_type = name;
                        }

                        return;
                    }
                }
            }
        }

        if (((cl = Defined[name]) != 0) ||
            ((cl = Undefined[name]) != 0) ||
            ((cl = classes[name]) != 0) ||
            ((cl = classes[package + '.' + name]) != 0)) {
            if (need_object != 0)
                *need_object = cl;

            typespec.type = cl->get_uml();

            return;
        }

        UmlClass * uml_cl;

        if ((uml_cl = user_classes[name]) != 0) {
            typespec.type = uml_cl;
            return;
        }

        int index =  name.find('.');
        QString left = WrapperStr((index == -1) ? name : name.left(index));

        for (QStringList::iterator it = imports.begin(); it != imports.end(); ++it) {
            const QString & import = *it;
            QString s;
            int index2;

            if (((const char *) import)[import.length() - 1] == '.')
                // import a.b.* ('*' removed)
                s = import + name.operator QString();	// a.b.name
            else if (((index2 = import.findRev('.')) != -1) &&
                     (import.mid(index2 + 1) == left))
                // import a.b.c & name = c.d...
                s = import.left(index2 + 1) + name.operator QString();	// a.b.name
            else
                continue;

            if ((cl = classes[s]) != 0) {
                if (need_object != 0) {
                    *need_object = cl;
                    typespec.type = cl->get_uml();
                }
                else
                    typespec.type = cl->get_uml();

                return;
            }

            if ((uml_cl = user_classes[s]) != 0) {
                typespec.type = uml_cl;
                return;
            }
        }

        // class not find using imports

        if ((index == -1) && ((cl = java_classes[name]) != 0)) {
            // a java class
            if (need_object != 0)
                *need_object = cl;

            typespec.type = cl->get_uml();
            return;
        }


        if ((typespec.explicit_type = JavaSettings::umlType(name)).isEmpty()) {
            if (need_object == 0)
                typespec.explicit_type = name;
            else if (! Lex::identifierp(name))
                // template form
                typespec.explicit_type = name;
            else {
                // the class does not already exist, create it
                Package * pack = package_unknown();
                int index = name.operator QString().lastIndexOf('.');

                if (index != -1) {
                    pack = pack->find(name.left(index), FALSE);
                    pack->set_package(name.left(index));
                    name = name.mid(index + 1);
                }

                cl = pack->declare_if_needed(name, FALSE);
                typespec.type = cl->get_uml();

                *need_object = cl;
            }
        }
    }
}

// name is a formal but a class is needed,
// create a pseudo one

void Package::force_class(WrapperStr name, UmlTypeSpec & typespec,
                          Class ** need_object)
{
    Class * cl = Undefined[name];

    if (cl != 0)  {
        *need_object = cl;
        typespec.type = cl->get_uml();
    }
    else {
        // the class does not already exist, create it
        Package * pack = package_unknown();

        cl = pack->declare_if_needed(name, FALSE);
        typespec.type = cl->get_uml();

        *need_object = cl;
    }
}

// the class is not a builtin, the package is the right,
// the name doesn't contains '.'

Class * Package::declare_if_needed(WrapperStr name, char st)
{
    Class * cl;

    // the class already exist ?
    if (((cl = Defined[name]) != 0) || ((cl = Undefined[name]) != 0))
        return cl;

    // create class
    cl = new Class(this, name, st);

    Undefined.insert(name, cl);

    return cl;
}

void Package::declare(const WrapperStr & name, Class * cl)
{
    if (! package.isEmpty())
        classes.insert(package + "." + name, cl);

    if (name.find('.') == -1)
        Undefined.insert(name, cl);
}

Package * Package::find(WrapperStr s, bool nohack)
{
    int index;
    WrapperStr name = ((index = s.find('.')) != -1) ? s.left(index) : s;

    // hack
    if (!nohack && (text(0) == (const char *) name)) {
        if (index == -1)
            return this;

        s = s.mid(index + 1);
        name = ((index = s.find('.')) != -1) ? s.left(index) : s;
    }

    Package * p = 0;

    // TODO: remove suspicious macro TreeItem in order to remove Qt3 dependency.
#ifdef REVERSE
    foreach (TreeItem * child,  children()) {
#else
    for (TreeItem *child = firstChild(); child; child = child->nextSibling()) {
#endif
        if (((BrowserNode *) child)->isa_package() &&
            (child->text(0) == (const char *) name)) {
            p = (Package *) child;
            break;
        }
    }

    if (p == 0)
        p = new Package(this, path + name, name);

    return (index == -1) ? p : p->find(s.mid(index + 1), nohack);
}

Package * Package::package_unknown()
{
    if (unknown != 0)
        return unknown;

    unknown = new Package(root, "<unknown>", "unknown");

    if (unknown->get_uml(FALSE) == 0) {
        int i = 1;

        do {
            unknown->setText(0, "unknown" + QString::number(i));
        }
        while (unknown->get_uml(FALSE) == 0);
    }

    return unknown;
}

void Package::set_package(WrapperStr s)
{
    Package * pack = this;

    while ((pack != 0) && pack->package.isEmpty()) {
        known_packages.replace(s, pack);
        pack->package = s;

        if (pack->uml != 0)
            pack->uml->set_JavaPackage(s);

        int index = s.operator QString().lastIndexOf('.');

        if (index == -1)
            return;

        s.truncate(index);
        pack = (Package *) pack->parent();
    }
}

UmlPackage * Package::get_uml(bool mandatory)
{
    if (uml == 0) {
        if (!package.isEmpty() &&
            ((uml = UmlPackage::getProject()->findJavaPackage(package)) != 0))
            return uml;

        const char * name = text(0);
        Package * pa = (Package *) parent();
        UmlPackage * uml_pa = pa->get_uml();	// will end on project

        const Q3PtrVector<UmlItem> & ch = uml_pa->children();
        UmlItem ** v = ch.data();
        UmlItem ** const vsup = v + ch.size();
        UmlItem * it;

        for (; v != vsup; v += 1) {
            it = *v;

            if ((it->kind() == aPackage) && (it->name() == name)) {
                uml = (UmlPackage *) it;
                return uml;
            }
        }

        if ((uml = UmlBasePackage::create(uml_pa, name)) == 0) {
            if (!mandatory)
                return 0;

#ifdef REVERSE
            UmlCom::trace(WrapperStr("<font face=helvetica><b>cannot create package <i>")
                          + name + "</i> under package <i>" + uml_pa->name() +
                          "</b></font><br>");
            UmlCom::message("");
            throw 0;
#else
            QMessageBox::critical(0, "Fatal Error",
                                  QString("<font face=helvetica><b>cannot create package <i>")
                                  + name + "</i> under package <i>"
                                  + ((const char *) uml_pa->name()) + "</b></font><br>");
            QApplication::exit(1);
#endif
        }

        if (! package.isEmpty())
            uml->set_JavaPackage(package);

        uml->set_JavaDir(root_relative_if_possible(path));
    }

    return uml;
}

void Package::push_context()
{
    stack.push(imports);
    imports.clear();
}

void Package::pop_context()
{
    QStringList l = stack.pop();
    /*
     imports = *l;
     delete l;*/
}

#ifndef REVERSE

void Package::refer(const QString & name)
{
    Class * cl;

    if (((cl = Defined[name]) == 0) &&
        ((cl = Undefined[name]) == 0))
        cl = classes[name];

    if (cl != 0)
        BrowserView::select(cl);
}

const QPixmap * Package::pixmap(int) const
{
    return (this == root) ? 0 : PackageIcon;
}

void Package::menu()
{
    if (path.isEmpty())
        // the project
        return;

    Q3PopupMenu m(0);

    m.insertItem(text(0), -1);
    m.insertSeparator();

    if (parent() == root) {
        m.insertItem("Rename", 2);
        m.insertSeparator();
    }

    bool have_classe = FALSE;
    bool have_package = FALSE;

    Q3ListViewItem * child;

    for (child = firstChild(); child != 0; child = child->nextSibling()) {
        if (!((BrowserNode *) child)->isa_package()) {
            if (! have_classe) {
                have_classe = TRUE;
                m.insertItem("Send its classes", 0);

                if (have_package)
                    break;
            }
        }
        else if (! have_package) {
            have_package = TRUE;
            m.insertItem("Send its classes and classes of sub-packages", 1);

            if (have_classe)
                break;
        }
    }

    int choice = m.exec(QCursor::pos());

    switch (choice) {
    case 0:
    case 1: {
        QDir d(path);

        if (d.exists()) {
            QApplication::setOverrideCursor(Qt::waitCursor);
            new Progress(file_number(d, choice == 1), "Reverse in progress, please wait ...", app);
            JavaCatWindow::clear_trace();
            JavaSettings::set_UseDefaults(TRUE);
            reverse_directory(d, choice == 1);
            Progress::delete_it();
            UmlCom::message("");
            QApplication::restoreOverrideCursor();
        }
        else
            UmlCom::trace(WrapperStr("<font face=helvetica><i>") + path +
                          "</i> <b>doesn't exist !</b></font><br>");

    }
    break;

    case 2: {
        bool ok = FALSE;
        QString s = QInputDialog::getText("Rename", "new name :",
                                          QLineEdit::Normal, text(0), &ok);

        if (ok && !s.isEmpty() && (s != text(0)))
            setText(0, s);
    }
    }
}

// note : 'QDataStream >> char *' bugged on windows

void Package::backup(QDataStream  & dt) const
{
    dt << ((Q_INT8) '{');

    Q_INT32 len;

    len = text(0).length();
    dt << len;
    dt.writeRawBytes(text(0), len);

    len = path.length();
    dt << len;
    dt.writeRawBytes(path, len);

    len = package.length();
    dt << len;

    if (!package.isEmpty())
        dt.writeRawBytes(package, len);

    backup_children(dt);
}

void Package::backup_children(QDataStream  & dt) const
{
    Q3ListViewItem * child;

    for (child = firstChild(); child != 0; child = child->nextSibling())
        ((BrowserNode *) child)->backup(dt);

    dt << ((Q_INT8) '}');
}

#endif // !REVERSE

void Package::restore(QDataStream  & dt, Package * parent)
{
    // '{' already read
    Q_INT32 len;

    dt >> len;
    char * name = new char[len + 1];
    dt.readRawBytes(name, len);
    name[len] = 0;

    dt >> len;
    char * path = new char[len + 1];
    dt.readRawBytes(path, len);
    path[len] = 0;

    dt >> len;
    char * package;

    if (len == 0)
        package = 0;
    else {
        package = new char[len + 1];
        dt.readRawBytes(package, len);
        package[len] = 0;
    }

    Package * pack;

#ifdef ROUNDTRIP

    if ((package != 0) &&
        ((pack = known_packages[package]) != 0))
        delete [] package;
    else {
#endif
        pack = new Package(parent, path, name);

        if (package != 0) {
            known_packages.replace(package, pack);
            pack->package = package;
            delete [] package;
        }

#ifdef ROUNDTRIP
    }

#endif

    delete [] name;
    delete [] path;

    pack->restore_children(dt);
}

void Package::restore_children(QDataStream  & dt)
{
    for (;;) {
        Q_INT8 c;

        dt >> c;

        switch (c) {
        case '}':
            return;

        case '{':
            restore(dt, this);
            break;

        default:
            Class::restore(dt, (char) c, this);
        }
    }
}
