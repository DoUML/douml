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

#ifdef DEBUG_DOUML
#include <iostream>
//Added by qt3to4:
#include <Q3ValueList>
#include "misc/mystr.h"
//Added by qt3to4:
#include <Q3PtrList>
#include "Logging/QsLog.h"
using namespace std;
#endif

#include <q3filedialog.h>
#include <qapplication.h>
#include <qmessagebox.h>
#include <qdir.h>
#include <qregexp.h>

#include "Package.h"
#include "Class.h"
#include "UmlPackage.h"
#include "UmlClass.h"
#include "UmlAttribute.h"
#include "UmlRelation.h"
#include "UmlOperation.h"
#include "Lex.h"
#include "UmlCom.h"
#include "CppSettings.h"
#include "CppCatWindow.h"
#include "Statistic.h"
#include "Progress.h"

#ifdef ROUNDTRIP
#include "UmlArtifact.h"
#endif

// create all packages under it
Package * Package::Root;

// regexp to filter dirs/files
QRegExp * Package::DirFilter;
QRegExp * Package::FileFilter;

// the packages selected by the user to reverse them
QList<Package *> Package::Choozen;

// all the classes presents in the environment, the
// key is the full name including namespaces
NDict<Class> Package::Declared(511);
NDict<Class> Package::Defined(511);

// to place unknown classes
//Package * Package::Unknown;

// to know if it is the scan step or the reverse step
bool Package::Scan;

// current template forms
Q3ValueList<FormalParameterList> Package::Formals;

QApplication * Package::app;

// memorise the name of the header currently reversed
// to create artifact having the right name and respect
// the classes defined in a same header file
WrapperStr Package::fname;

#ifdef ROUNDTRIP
// currently roundtriped artifact
UmlArtifact * Package::artfct;

static Q3Dict<bool> H_Managed(97);
static Q3Dict<bool> Src_Managed(97);
static Q3Dict<Package> Pack_From_Path(97);
static Q3Dict<UmlArtifact> Roundtriped(199);
#endif

static QString RootSDir;	// empty or finish by a /
static WrapperStr RootCDir;	// empty or finish by a /

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
    unsigned rln = RootCDir.length();

    return ((p.length() >= rln) && (p.left(rln) == RootCDir))
           ? p.mid(rln)
           : p;
}

#ifndef REVERSE
Package::Package(BrowserView * parent, UmlPackage * u)
    : BrowserNode(parent, u->Name())
{
    uml = u;
}
#endif

Package::Package(Package * parent, QString p, QString n)
    : BrowserNode(parent, n)
{
    h_path = src_path = force_final_slash(p);
    uml = 0;
}

#ifdef ROUNDTRIP
Package::Package(Package * parent, UmlPackage * pk)
    : BrowserNode(parent, pk->name())
{
    //if (text(0) == "unknown")
    //  unknown = this;

    uml = pk;

    QDir d_root(RootSDir);

    h_path = force_final_slash(pk->cppHDir());

    if (h_path.isEmpty())
        h_path = RootCDir;
    else if (QDir::isRelativePath(h_path)) {
        if (RootCDir.isEmpty()) {
            WrapperStr err = "<font face=helvetica><b>root path not set in <i>generation settings</i>, "
                            "don't know where is <i>" + h_path + "<i></b></font><br>";

            UmlCom::trace(err);
        }
        else
            h_path = force_final_slash(d_root.filePath(h_path));
    }

    src_path = force_final_slash(pk->cppSrcDir());

    if (src_path.isEmpty())
        src_path = RootCDir;
    else if (QDir::isRelativePath(src_path)) {
        if (RootCDir.isEmpty()) {
            WrapperStr err = "<font face=helvetica><b>root path not set in <i>generation settings</i>, "
                            "don't know where is <i>" + src_path + "<i></b></font><br>";

            UmlCom::trace(err);
        }
        else
            src_path = force_final_slash(d_root.filePath(src_path));
    }

    namespace_ = pk->cppNamespace();

    if (!h_path.isEmpty())
        Pack_From_Path.replace(h_path, this);

    if ((h_path != src_path) && !src_path.isEmpty())
        Pack_From_Path.replace(src_path, this);
}
#endif

// to not have warning
Package::~Package()
{
}

bool Package::isa_package() const
{
    return TRUE;
}

#ifndef REVERSE
QString Package::get_h_path() const
{
    return h_path;
}

QString Package::get_src_path() const
{
    return src_path;
}
#endif

void Package::init(UmlPackage * r, QApplication * a)
{
    app = a;

    RootCDir = force_final_slash(CppSettings::rootDir());
    RootSDir = RootCDir.operator QString();

    if (!RootSDir.isEmpty() && QDir::isRelativePath(RootSDir)) {
        QFileInfo f(UmlPackage::getProject()->supportFile());
        QDir d(f.dirPath());

        RootSDir = force_final_slash(d.filePath(RootSDir)).operator QString();
        RootCDir = RootSDir;
    }

    QString s;

    DirFilter = (!(s = (const char *) CppSettings::reverseRoundtripDirRegExp()).isEmpty())
                ? new QRegExp(s, CppSettings::isReverseRoundtripDirRegExpCaseSensitive(), TRUE)
                : 0;

    FileFilter = (!(s = (const char *) CppSettings::reverseRoundtripFileRegExp()).isEmpty())
                 ? new QRegExp(s, CppSettings::isReverseRoundtripFileRegExpCaseSensitive(), TRUE)
                 : 0;

#ifdef ROUNDTRIP
    Root = new Package(0, r);
    r->init(Root);
#elif defined(REVERSE)
    Root = new Package(0, 0, r->name());
    Root->uml = r;
#else
    root = new Package(BrowserView::instance(), r);
#endif
}

void Package::set_step(int s, int n)
{
    if (n == -1) {
        Scan = FALSE;
        Progress::delete_it();
    }
    else {
        switch (s) {
#ifdef ROUNDTRIP

        case 0:
            Scan = FALSE;

            if (n > 1)
                new Progress(n, "Preparation, please wait ...", app);

            break;
#endif

        case 1:
            Scan = TRUE;

            if (n > 1)
                new Progress(n, "Scanning in progress, please wait ...", app);

            break;

        default:
            // 2
            Scan = FALSE;

            if (n > 1) {
#ifdef ROUNDTRIP
                new Progress(n, "Roundtrip in progress, please wait ...", app);
#else
                new Progress(n, "Reverse in progress, please wait ...", app);
#endif
            }
        }
    }
}


#ifdef ROUNDTRIP
enum ReverseRootCases { DontKnow, ReverseRoot, DontReverseRoot };

void Package::accept_roundtrip_root(bool h)
{
    static ReverseRootCases reverseRoot = DontKnow;

    switch (reverseRoot) {
    case DontKnow:
        switch (QMessageBox::warning(0, "C++ Roundtrip",
                                     "directory not specified at package level, "
                                     "do you want to reverse all files under <i>"
                                     + RootSDir + "</i> and its sub directories ?",
                                     "yes", "no", "cancel", 0, 2)) {
        case 0:
            reverseRoot = ReverseRoot;
            break;

        case 1:
            reverseRoot = DontReverseRoot;

            if (h)
                h_path = 0;
            else
                src_path = 0;

            break;

        default: // 2
            throw 0;
        }

        break;

    case ReverseRoot:
        break;

    default: // DontReverseRoot
        if (h)
            h_path = 0;
        else
            src_path = 0;

        break;
    }
}

void Package::own(UmlArtifact * art)
{
    QDir hd(h_path);
    QDir sd(src_path);

    Roundtriped.insert(hd.absFilePath(art->name() + "." + CppSettings::headerExtension()), art);
    Roundtriped.insert(sd.absFilePath(art->name() + "." + CppSettings::sourceExtension()), art);
}

int Package::count_file_number()
{
    int result =
        file_number(h_path, TRUE, CppSettings::headerExtension())
        + file_number(src_path, TRUE, CppSettings::sourceExtension());

    foreach (TreeItem * child, children())
        if (((BrowserNode *) child)->isa_package())
            result += ((Package *) child)->count_file_number();

    return result;
}

void Package::scan_dir(int & n)
{
    // count files
    UmlCom::message("count files ...");

    n = count_file_number();
    H_Managed.clear();
    Src_Managed.clear();

    set_step(1, n);
    scan_dir();
    H_Managed.clear();
    Src_Managed.clear();
    set_step(1, -1);
}

void Package::scan_dir()
{
    reverse_directory(h_path, TRUE, CppSettings::headerExtension(), TRUE);
    reverse_directory(src_path, TRUE, CppSettings::sourceExtension(), FALSE);

    foreach (TreeItem * child, children())
        if (((BrowserNode *) child)->isa_package())
            ((Package *) child)->scan_dir();
}
#else
/* lgfreitas: Reversing top-level method. It starts in package-level */
void Package::scan_dirs(int & n)
{
    n = 0;

    QStringList dirs;
    QString path = QDir::currentDirPath();

    /* lgfreitas: This is the loop that keeps asking for dirs to reverse. */
    // get input C++ source dirs
    while (!(path = Q3FileDialog::getExistingDirectory(path, 0, 0,
                    "select a directory to reverse, press cancel to finish"))
           .isEmpty()) {

        int pathlen = path.length();

        if (((const char *) path)[pathlen - 1] != '/') {
            path += "/";
            pathlen += 1;
        }

        QString err = 0;

        for (QStringList::Iterator it = dirs.begin(); it != dirs.end(); ++it) {
            int dlen = (*it).length();

            if (dlen >= pathlen) {
                if ((*it).left(pathlen) == path) {
                    err = (dlen == pathlen)
                          ? path + " is already selected\n"
                          : "An already selected directory is a subdirectory of " + path;
                    break;
                }
            }
            else if (path.left(dlen) == *it) {
                err = path + " is a subdirectory of an already selected directory\n";
                break;
            }
        }

        if (!err.isEmpty())
            QMessageBox::warning(0, "C++ reverse",
                                 err + "Press 'cancel' to reverse selected directories");
        else {
            QDir d(path);
            WrapperStr s;

            dirs.append(path);
            QString temp = "<font face=helvetica>%dth directory to reverse : <b>";
            temp=temp.arg(QString::number((int) dirs.count()));
            s+=temp;
            s += WrapperStr(path.toAscii().constData()) + "</b><br></font>\n";
            UmlCom::trace(s);

            /* lgfreitas: Append the root path as a root package */
            Choozen.append(new Package(Root, path, d.dirName()));

            d.cdUp();
            path = d.absPath();
        }

        // just to give a chance to Douml to update trace window before the
        // file dialog appears to not have it under the trace windows
        UmlBasePackage::getProject();
    }


    if (dirs.isEmpty())
        return;

    UmlCom::trace("<hr>\n");

    dirs.clear();

#ifndef REVERSE
    QApplication::setOverrideCursor(Qt::waitCursor);
    CppCatWindow::clear_trace();
#endif

    // count files
    UmlCom::message("count files ...");

    /* lgfreitas: This just counts the packages */
    foreach (Package *p, Choozen) {
        n += file_number(p->h_path, TRUE,
                         CppSettings::headerExtension())
             + file_number(p->src_path, TRUE,
                           CppSettings::sourceExtension());
    }

    // scanning phase
    set_step(1, n);

    /* lgfreitas: This is where the reversing magic happens. It iterates
       through each package, asking for it to do the reversing on the
       given path */
    foreach (Package *p, Choozen) {
        p->reverse_directory(p->h_path, TRUE, CppSettings::headerExtension(), TRUE);
        p->reverse_directory(p->src_path, TRUE, CppSettings::sourceExtension(), FALSE);
    }

    set_step(1, -1);
#ifndef REVERSE
    QApplication::restoreOverrideCursor();

    Root->setOpen(TRUE);
#endif
}
#endif

static bool allowed(QRegExp * rg, QString f)
{
    if (rg != 0) {
        //int matchLen;
        return rg->exactMatch(f); //[lgfreitas] I do not know if this will work
        /*return ((rg->match(f, 0, &matchLen) != 0) ||
            (matchLen != (int) f.length()));*/
    }
    else
        return TRUE;
}

int Package::file_number(QString path, bool rec, const char * ext)
{
    int result = 0;

    if (! path.isEmpty()) {
        QDir d(path);

        if (! allowed(DirFilter, d.dirName()))
            return 0;

        /*
        const QFileInfoList * list = d.entryInfoList(QDir::Files | QDir::Readable);

        if (list != 0) {
          QFileInfoListIterator it(*list);
          QFileInfo * fi;

          while ((fi = it.current()) != 0) {
        if (fi->extension(FALSE) == ext)
          result += 1;
        ++it;
          }
          */
        QFileInfoList list = d.entryInfoList(QDir::Files | QDir::Readable);

        if (!list.isEmpty()) {
            QFileInfoList::iterator it = list.begin();

            while (it != list.end()) {
                if ((*it).extension(FALSE) == ext) result += 1;

                it++;
            }
        }

        if (rec) {
            /*      // sub directories
                  list = d.entryInfoList(QDir::Dirs | QDir::NoSymLinks);

                  if (list) {
            	QFileInfoListIterator itd(*list);
            	QFileInfo * di;

            	while ((di = itd.current()) != 0) {
            	  if (((const char *) di->fileName())[0] != '.')
            	    result += file_number(di->filePath(), rec, ext);
            	  ++itd;
            	}
                  }
                }
                */

            // sub directories
            list = d.entryInfoList(QDir::Dirs | QDir::NoSymLinks);

            if (!list.isEmpty()) {
                QFileInfoList::iterator it = list.begin();

                while (it != list.end()) {
                    if ((*it).fileName()[0] != '.') {
                        result += file_number((*it).filePath(), rec, ext);
                    }

                    it++;
                }
            }
        }
    }

    return result;
}

#ifdef ROUNDTRIP
void Package::send_dir(int n)
{
    // reverse phase
    UmlPackage::getProject()->set_childrenVisible(FALSE);

    set_step(2, n);
    send_dir();
    H_Managed.clear();
    Src_Managed.clear();
    set_step(2, -1);

    ((uml) ? uml : Root->uml)->set_childrenVisible(TRUE);
}

void Package::send_dir()
{
    reverse_directory(h_path, TRUE, CppSettings::headerExtension(), TRUE);
    reverse_directory(src_path, TRUE, CppSettings::sourceExtension(), FALSE);

    foreach (TreeItem * child, children())
        if (((BrowserNode *) child)->isa_package())
            ((Package *) child)->send_dir();
}
#else
void Package::send_dirs(int n, bool rec)
{
    // reverse phase
    UmlPackage::getProject()->set_childrenVisible(FALSE);

#ifndef REVERSE
    QApplication::setOverrideCursor(Qt::waitCursor);
#endif

    set_step(2, n);

    foreach (Package *p, Choozen) {
        p->reverse_directory(p->h_path, rec, CppSettings::headerExtension(), TRUE);
        p->reverse_directory(p->src_path, rec, CppSettings::sourceExtension(), FALSE);
    }

    set_step(2, -1);

#ifndef REVERSE
    QApplication::restoreOverrideCursor();
#endif

#ifdef REVERSE
    Root->uml->set_childrenVisible(TRUE);
#else
    root->set_childrenVisible(TRUE);
#endif
}
#endif

QString my_baseName(QFileInfo * fi)
{
    QString fn = fi->fileName();
    int index = fn.findRev('.');

    return (index == -1)
           ? fn
           : fn.left(index);
}

/*	lgfreitas: this steps through files in the directory
	and reverse them one by one */
void Package::reverse_directory(QString path, bool rec,
                                QString ext, bool h)
{
#ifdef ROUNDTRIP

    if (path.isEmpty())
        return;

    if (h) {
        if (H_Managed.find(path) != 0)
            return;

        H_Managed.insert(path, (bool *) 1);
    }
    else {
        if (Src_Managed.find(path) != 0)
            return;

        Src_Managed.insert(path, (bool *) 1);
    }

#endif

    // reads files
    QDir d(path);

    if (! allowed(DirFilter, d.dirName()))
        return;

    /*
    const QFileInfoList list =
      d.entryInfoList("*." + ext, QDir::Files | QDir::Readable);

    if (list != 0) {
      QFileInfoListIterator it(*list);

      while (it.current() != 0) {
        if (allowed(FileFilter, it.current()->fileName())) {
    #ifdef ROUNDTRIP
    QString fn = it.current()->absFilePath();
    UmlArtifact * art = Roundtriped.find(fn);

    if (h)
      fname = my_baseName(it.current());

    if ((art != 0) && (art->parent()->parent() != uml))
      // own by an other package
      ((UmlPackage *) art->parent()->parent())->get_package()->reverse_file(WrapperStr(fn), art, h);
    else
      reverse_file(WrapperStr(fn), art, h);
    #else
    if (h)
      fname = my_baseName(it.current());
    reverse_file(WrapperStr(it.current()->filePath()));
    #endif
        }
        Progress::tic_it();
        ++it;
      }
    }*/

    QFileInfoList list =
        d.entryInfoList("*." + ext, QDir::Files | QDir::Readable);

    /* lgfreitas: If the package/path has files step thru them */
    if (list.isEmpty() == false)
    {
        QFileInfoList::iterator it = list.begin();

        while (it != list.end())
        {
            if (allowed(FileFilter, (*it).fileName())) {
#ifdef ROUNDTRIP
                QString fn = (*it).absFilePath();
                UmlArtifact * art = Roundtriped.find(fn);

                if (h)
                    fname = my_baseName(&(*it));

                if ((art != 0) && (art->parent()->parent() != uml))
                    // own by an other package
                    ((UmlPackage *) art->parent()->parent())->get_package()->reverse_file(WrapperStr(fn.toAscii().constData()), art, h);
                else
                    reverse_file(WrapperStr(fn.toAscii().constData()), art, h);

#else

                if (h)
                    fname = my_baseName(&(*it));

                QByteArray tmp = (*it).filePath().toAscii();
                const char * c = tmp.constData();
                reverse_file(c);
#endif
            }

            Progress::tic_it();
            ++it;
        }
    }


    if (rec) {
        // sub directories
        /*list = d.entryInfoList(QDir::Dirs | QDir::NoSymLinks);

        if (list != 0) {
          QFileInfoListIterator itd(*list);
          QFileInfo * di;

          while ((di = itd.current()) != 0) {
        if (((const char *) di->fileName())[0] != '.')
          find(di)->reverse_directory(di->filePath(), TRUE, ext, h);
        ++itd;
          }
        }*/
        // sub directories
        list = d.entryInfoList(QDir::Dirs | QDir::NoSymLinks);

        if (!list.isEmpty()) {
            QFileInfoList::iterator itd = list.begin();

            while (itd != list.end()) {
                if ((*itd).fileName()[0] != '.') {
                    find(&(*itd))->reverse_directory((*itd).filePath(), TRUE, ext, h);
                }

                ++itd;
            }
        }
    }

    fname = "";
}

#ifdef ROUNDTRIP
void Package::reverse(UmlArtifact * art)
{
    WrapperStr f;

    f = h_path + art->name() + "." + CppSettings::headerExtension();

    if (allowed(FileFilter, art->name() + "." + CppSettings::headerExtension()) &&
        QFile::exists(f))
        reverse_file(f, art, TRUE);

    f = src_path + art->name() + "." + CppSettings::sourceExtension();

    if (allowed(FileFilter, art->name() + "." + CppSettings::sourceExtension()) &&
        QFile::exists(f))
        reverse_file(f, art, FALSE);
}
#endif

void Package::reverse_file(WrapperStr f
#ifdef ROUNDTRIP
                           , UmlArtifact * art, bool h
#endif
                          )
{
#ifdef ROUNDTRIP

    if (art != 0) {
        if (art->is_considered(h, Scan))
            return;

        art->set_considered(h, Scan);
    }

#endif

    /* lgfreitas: Tries to open the lexical analyzer */
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
            CppCatWindow::trace(WrapperStr("<font face=helvetica><b>cannot open <i>")
                                + f + "</i></b></font><br><hr><br>");
    }
    else {
#ifdef ROUNDTRIP

        if (!Scan) {
            if ((art != 0) && art->is_fully_updated() && !art->is_usefull()) {
                // don't change the artifact header and source definition to not removed
                // #include or decl added by hand
                art->set_usefull();

                Q3PtrVector<UmlClass> empty;

                art->set_AssociatedClasses(empty);
            }
        }

        UmlArtifact * a = artfct;

        artfct = art;
#endif

        UmlCom::message(((Scan) ? "scan " : "reverse ") + f);

        reverse_toplevel_forms(f, FALSE);

#ifdef ROUNDTRIP
        artfct = a;
#endif

        Formals.clear();
        UmlClass::clear_usings();
        Namespace::clear_usings();
        Namespace::clear_aliases();
        Lex::close();
    }
}


void Package::reverse_toplevel_forms(WrapperStr f, bool sub_block)
{
    WrapperStr pretype;
    WrapperStr s;

    /* lgfreitas: Read each word from the file and analyzes it */
    while(true)
    {
        s = Lex::read_word();
        if(s.isEmpty())
            break;
        if (s == "template") {
            FormalParameterList fmt;

            Formals.append(fmt);
            get_template(Formals.last());
        }
        else {
            if ((s == "class") || (s == "struct") || (s == "union")) {
                Lex::mark();
                /* lgfreitas: We are building a class here, so get the class name */
                WrapperStr s2 = Lex::read_word();

                /* lgfreitas: This seems to have the purpose of ignoring these parameters (personal use?) */
                if ((strncmp(s2, "Q_EXPORT", 8) == 0) ||
                    (strncmp(s2, "QM_EXPORT", 9) == 0) ||
                    (strncmp(s2, "Q_PNGEXPORT", 11) == 0))
                    s2 = Lex::read_word();

                if (Lex::identifierp(s2, TRUE) &&
                    ((s2 = Lex::read_word()) != "{") && (s2 != ":") && (s2 != ";")) {
                    // form like 'class X Y'
                    pretype = s;
                    Lex::come_back();
                }
                else {
                    Lex::come_back();
                    // do not manage 'struct {..} var;' or 'struct {..} f()' ...
#ifdef ROUNDTRIP
                    QList<UmlItem *> dummy;

                    Class::reverse(this, s, Formals, f, 0, FALSE, dummy);
#else
                    Class::reverse(this, s, Formals, f, 0);
#endif
                }
            }
            else if (s == "enum") {
                Lex::mark();

                WrapperStr s2 = Lex::read_word();

                if (Lex::identifierp(s2, TRUE) && (Lex::read_word() != "{")) {
                    // form like 'enum X Y'
                    pretype = s;
                    Lex::come_back();
                }
                else {
                    Lex::come_back();
                    // do not manage 'enum {..} var;' or 'enum {..} f()' ...
#ifdef ROUNDTRIP
                    QList<UmlItem *> dummy;

                    Class::reverse_enum(this, f, 0, FALSE, dummy);
#else
                    Class::reverse_enum(this, f, 0);
#endif
                }
            }
            else if (s == "typedef") {
#ifdef ROUNDTRIP
                QList<UmlItem *> dummy;

                Class::reverse_typedef(this, f, Formals, FALSE, dummy);
#else
                Class::reverse_typedef(this, f, Formals);
#endif
            }
            else if (s == "namespace") {
                s = Lex::read_word();

                if (s == "{") {
                    // anonymous namespace
                    Namespace::enter_anonymous();
                    reverse_toplevel_forms(f, TRUE);
                    Namespace::exit_anonymous();
                }
                else {
                    WrapperStr s2 = Lex::read_word();

                    if (s2.isEmpty()) {
                        if (!Scan)
                            Lex::premature_eof();

                        return;
                    }
                    else if (s2 == "{") {
                        Namespace::enter(s);
                        UmlClass::save_using_scope();
                        reverse_toplevel_forms(f, TRUE);
                        UmlClass::restore_using_scope();
                        Namespace::exit();
                    }
                    else if (Scan)
                        UmlOperation::skip_body();
                    else if (s2 != "=") {
                        Lex::error_near(s2);
                        UmlOperation::skip_body();
                    }
                    else if ((s2 = Lex::read_word()).isEmpty()) {
                        Lex::premature_eof();
                        return;
                    }
                    else
                        Namespace::add_alias(s, s2);
                }
            }
            else if (s == "using") {
                if ((s = Lex::read_word()).isEmpty()) {
                    if (!Scan)
                        Lex::premature_eof();

                    return;
                }
                else if (s == "namespace") {
                    if (Scan)
                        UmlOperation::skip_body(0);
                    else if ((s = Lex::read_word()).isEmpty()) {
                        Lex::premature_eof();
                        return;
                    }
                    else
                        Namespace::add_using(s);
                }
                else if (!Scan) {
                    s = Lex::complete_template_type(s);

                    UmlTypeSpec typespec;

                    if (!find_type(s, typespec)) {
#if 0
                        int index = s.findRev("::");

                        if ((index == -1) ||
                            (!find_type(s = s.left(index), typespec) &&
                             !s.isEmpty()))
                            Lex::warn("unknown class <font color =\"red\">" +
                                      Lex::quote(s) + "</font>");
                        else
                            Lex::warn("using form lost");

#endif
                    }
                    else
                        typespec.type->using_it();
                }
                else
                    UmlOperation::skip_body(0);
            }
            else if (s == ";")
                ;
            else if (sub_block && (s == "}")) {
                Formals.clear();
                break;
            }
            else if ((s == "QM_TEMPLATE_EXTERN_CANVAS") ||
                     (s == "Q_TEMPLATE_EXTERN_CANVAS") ||
                     (s == "Q_TYPENAME"))
                ; // lost
            else if (Scan && (s != "extern") && (s != "typename")) {
#ifdef DEBUG_DOUML
                QLOG_INFO() << "reverse_toplevel_forms skip_body sur '" << s << "'\n";
#endif
                UmlOperation::skip_body();
            }
            else
                reverse_toplevel_form(f, s);

            Formals.clear();
        }
    }

#ifdef DEBUG_DOUML
    QLOG_INFO() << "exit Package::reverse_toplevel_forms avec '" << s << "'\n";
#endif
}

void Package::reverse_toplevel_form(WrapperStr f, WrapperStr s)
{
    WrapperStr comment = Lex::get_comments();
    WrapperStr description = Lex::get_description();
    WrapperStr q_modifier;	// not yet used
    bool inlinep = FALSE;
    WrapperStr type;
    WrapperStr name;
    WrapperStr array;

#ifdef DEBUG_DOUML
    QLOG_INFO() << "Package::reverse_toplevel_form(" << s << ")\n";
#endif

    for (;;) {
        if ((s == "static") || (s == "const") ||
            (s == "typename") || (s == "volatile"))
            // note that for an operation 'const' or 'typename' are
            // useless to find the corresponding declaration
            ;
        else if ((s == "class") || (s == "struct") ||
                 (s == "enum") || (s == "union"))
            // pre-type useless to find the corresponding declaration
            ;
        else if (s == "inline")
            inlinep = TRUE;
        else if ((s == "unsigned") || (s == "signed") ||
                 (s == "void") || (s == "bool"))
            type = s;
        else if ((s == "char") || (s == "short") || (s == "int") ||
                 (s == "long") || (s == "float") || (s == "double")) {
            type = (type.isEmpty()) ? s : type + " " + s;
        }
        else if (Lex::star(s) || (s == "&"))
            ;
        else if (s == "(") {
            // suppose a function or an operation
            if (Scan)
                UmlOperation::skip_body(0);
            else if (name == "main")
                get_uml()->reverse_main(type, comment);
            else
                UmlOperation::reverse_definition(this, name, type, Formals,
                                                 inlinep, comment, description);

            return;
        }
        else if (s == "=") {
            // initialized variable
            reverse_variable(name);
            return;
        }
        else if (s == ";") {
            // not initialized variable, does nothing
            return;
        }
        else if ((s == "extern") || (s == "Q_TEMPLATE_EXTERN")) {
            if (!(s = Lex::read_word()).isEmpty()) {
                if (*s == '"') {
                    // extern "language" {
                    if (! Scan)
                        Lex::warn("sorry, <font color =\"red\">extern " + Lex::quote(s)
                                  + "</font> is lost");

                    if ((s = Lex::read_word()) == "{")
                        reverse_toplevel_forms(f, TRUE);
                    else {
                        if (! Scan)
                            Lex::error_near(s);

                        UmlOperation::skip_body(0);
                    }
                }
                else
                    UmlOperation::skip_body(0);
            }

            return;
        }
        else if ((strncmp(s, "Q_EXPORT", 8) == 0) ||
                 (strncmp(s, "QM_EXPORT", 9) == 0) ||
                 (strncmp(s, "Q_PNGEXPORT", 11) == 0))
            q_modifier = s;
        else if ((s == "Q_INLINE_TEMPLATES") ||
                 (s == "Q_TYPENAME"))
            ; // lost
        else if (Lex::identifierp(s, FALSE)) {
            if (type.isEmpty()) {
                type = s = Lex::complete_template_type(s);

#ifdef DEBUG_DOUML
                QLOG_INFO() << "type = '" << type << "'\n";
#endif
            }
            else if (name.isEmpty()) {
                if (type.right(3) == "::~") {
                    type += s;
#ifdef DEBUG_DOUML
                    QLOG_INFO() << "type => '" << type << "'\n";
#endif
                }
                else {
                    name = Lex::complete_template_type(s);

#ifdef DEBUG_DOUML
                    QLOG_INFO() << "name = '" << name << "'\n";
#endif
                }
            }
            else {
#ifdef DEBUG_DOUML
                QLOG_INFO() << "ERROR '" << s << "' alors qu a deja le type '" << type << "' et le nom '" << name << "'\n";
#endif
                break;
            }
        }
        else if (*((const char *) s) == '[')
            array += s;
        else {
#ifdef DEBUG_DOUML
            QLOG_INFO() << "ERROR : '" << s << "'\n";
#endif
            break;
        }

        WrapperStr s2 = Lex::read_word();

        if (s2.isEmpty())
            break;

        s = s2;
    }

    // here we are in error
    Lex::error_near(s);
    UmlOperation::skip_body((s == "{") ? 1 : 0);
    Lex::finish_line();
    Lex::clear_comments();
}

void Package::reverse_variable(const WrapperStr & name)
{
    // '=' or '(' read
    Lex::mark();

    char c;

    while ((c = Lex::read_word_bis(TRUE, TRUE)) != ';')
        if (c == 0)
            return;

    WrapperStr init = Lex::region();

    if (name.isEmpty()) {
        Lex::syntax_error();
#ifdef DEBUG_DOUML
        QLOG_INFO() << "ERROR ';' et name.isEmpty() || type.isEmpty()\n";
#endif
        return;
    }

    UmlTypeSpec typespec;
    int index = name.operator QString().lastIndexOf("::");
    WrapperStr varname;

    if ((index <= 0) ||
        !find_type(Lex::normalize(name.left(index)), typespec)) {
        Lex::warn(WrapperStr("<font color =\"red\"> ") + Lex::quote(name) +
                  "</font> is lost");
#ifdef DEBUG_DOUML
        QLOG_INFO() << "ERROR " << name << " lost";
#endif
        return;
    }
    else
        varname = name.mid(index + 2);

    init = WrapperStr("=") + init.left(init.length() - 1);

    // search the corresponding attribute
    Q3PtrVector<UmlItem> children = typespec.type->children();
    unsigned rank = children.size();

    while (rank--) {
        UmlItem * it = children[rank];

        switch (it->kind()) {
        case anAttribute:
            if (it->name() == varname) {
                UmlAttribute * at = (UmlAttribute *) it;

                if (at->isClassMember()) {
#ifdef ROUNDTRIP

                    if (at->is_roundtrip_expected()) {
                        WrapperStr v = at->defaultValue();

                        if (!v.isEmpty() && (((const char *) v)[0] == '='))
                            v = v.mid(1);

                        if (nequal(v, init))
                            at->set_DefaultValue(init);
                    }
                    else
#endif
                        at->set_DefaultValue(init);

                    WrapperStr decl = at->cppDecl();
                    int index = decl.find("${h_value}");

                    if (index != -1) {
                        decl.remove(index + 2,  2);
                        // have to update decl even in roundtrip case
                        at->set_CppDecl(decl);
                    }
                }

                return;
            }

            break;

        case aRelation:
            if (((UmlRelation *) it)->roleName() == varname) {
                UmlRelation * rel = (UmlRelation *) it;

                if (rel->isClassMember()) {
#ifdef ROUNDTRIP

                    if (rel->is_roundtrip_expected()) {
                        WrapperStr v = rel->defaultValue();

                        if (!v.isEmpty() && (((const char *) v)[0] == '='))
                            v = v.mid(1);

                        if (nequal(v, init))
                            rel->set_DefaultValue(init);
                    }
                    else
#endif
                        rel->set_DefaultValue(init);

                    WrapperStr decl = ((UmlRelation *) it)->cppDecl();
                    int index = decl.find("${h_value}");

                    if (index != -1) {
                        decl.remove(index + 2,  2);
                        // have to update decl even in roundtrip case
                        rel->set_CppDecl(decl);
                    }
                }

                return;
            }

        default:	// to avoid compiler warning
            break;
        }
    }

    // no compatible variable
    Lex::warn(WrapperStr("<font color =\"red\"> ") + Lex::quote(name)
              + "</font> is not a static attribute of <font color =\"red\"> " +
              Lex::quote(typespec.type->name()) + "</font>");
#ifdef DEBUG_DOUML
    QLOG_INFO() << "ERROR " << name << " undeclared static attribute\n";
#endif
}

Class * Package::new_class(const WrapperStr & name,
                           const WrapperStr & stereotype,
                           bool declaration)
{
    Class * cl = new Class(this, name, stereotype);

    if (! name.isEmpty())
        ((declaration) ? Declared : Defined).insert(name, cl);

    return cl;
}

bool Package::find_type(WrapperStr type, UmlTypeSpec & typespec)
{
    return ClassContainer::find_type(type, typespec, Defined);
}

Class * Package::declare_if_needed(const WrapperStr & name,
                                   WrapperStr stereotype)
{
    FormalParameterList l;

    return ClassContainer::declare_if_needed(name, stereotype, l,
            Declared, Defined);
}

void Package::declare_if_needed(WrapperStr name, Class * cl)
{
    if (Defined[name] == 0)
        Declared.replace(name, cl);
}

Class * Package::define(const WrapperStr & name, WrapperStr stereotype)
{
    return ClassContainer::define(name, stereotype, Declared, Defined);
}

void Package::define(WrapperStr name, Class * cl)
{
    if (! name.isEmpty()) {
        if (Declared[name] != 0)
            Declared.remove(name);

        Defined.replace(name, cl);
    }
}

void Package::declaration(const WrapperStr &, const WrapperStr &,
                          const WrapperStr &
#ifdef ROUNDTRIP
                          , bool, QList<UmlItem *> &
#endif
                         )
{
    // lost it
}

#ifdef ROUNDTRIP
Class * Package::upload_define(UmlClass * ucl)
{
    Class * cl = new Class(this, ucl);
    WrapperStr s = ucl->name();	// not defined inside an other class

    Defined.insert(s, cl);

    return cl;
}
#endif


UmlPackage * Package::get_uml(bool mandatory)
{
    if (uml == 0) {
        const char * name = text(0);
        Package * pa = (Package *) parent();
        UmlPackage * uml_pa = pa->get_uml();	// will end on project

        Q3PtrVector<UmlItem> ch = uml_pa->children();

        for (unsigned index = 0; index != ch.size(); index += 1) {
            UmlItem * it = ch[index];

            if ((it->kind() == aPackage) && (it->name() == name))
                return uml = (UmlPackage *) it;
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
            throw 0;
#endif
        }

        if (! namespace_.isEmpty())
            uml->set_CppNamespace(namespace_);

        uml->set_CppSrcDir(root_relative_if_possible(h_path));
        uml->set_CppHDir(root_relative_if_possible(src_path));
    }

    return uml;
}

Package * Package::find(QFileInfo * di)
{
#ifdef ROUNDTRIP
    Package * p = Pack_From_Path[di->filePath()];

    if (p != 0)
        return p;

#endif

    QString s = di->fileName();
    foreach (TreeItem * child, children())
        if (((BrowserNode *) child)->isa_package() &&
            (child->text(0) == s))
            return (Package *) child;

    return new Package(this, WrapperStr((di->filePath()).toAscii().constData()), WrapperStr(s.toAscii().constData()));
}

/*
Package * Package::unknown()
{
  if (Unknown != 0)
    return Unknown;

  Unknown = new Package(Root, "<unknown>", "unknown");

  if (Unknown->get_uml(FALSE) == 0) {
    int i = 1;

    do {
      Unknown->setText(0, "unknown" + QString::number(i));
    } while (Unknown->get_uml(FALSE) == 0);
  }

  return Unknown;
}
*/
