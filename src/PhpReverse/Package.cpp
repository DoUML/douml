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
//Added by qt3to4:
#include "misc/mystr.h"
#include <QPixmap>

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
#include "PhpCatWindow.h"
#include "PhpSettings.h"
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
NDict<Class> Package::classes(1001);

// the php classes, the key are just the name without package
NDict<Class> Package::php_classes(1001);

// all the classes defined in Douml, not known through a .cat
// nor reversed, the key is the full name including package
NDict<UmlClass> Package::user_classes(1001);

// to show a progress bar
Progress * Package::progress;
QApplication * Package::app;

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
    uml = 0;
}

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
    QString s;

    DirFilter = (!(s = (const char *) PhpSettings::reverseRoundtripDirRegExp()).isEmpty())
                ? new QRegExp(s, PhpSettings::isReverseRoundtripDirRegExpCaseSensitive(), TRUE)
                : 0;

    FileFilter = (!(s = (const char *) PhpSettings::reverseRoundtripFileRegExp()).isEmpty())
                 ? new QRegExp(s, PhpSettings::isReverseRoundtripFileRegExpCaseSensitive(), TRUE)
                 : 0;

    Ext = PhpSettings::sourceExtension().operator QString();

#ifdef REVERSE
    root = new Package(0, 0, r->name());
    root->uml = r;
#else
    root = new Package(BrowserView::instance(), r);
#endif
    app = a;
}

void Package::new_class(Class * cl)
{
    php_classes.insert((const char *) cl->text(0), cl);
}

Package * Package::scan_dir()
{
    // get input php source dir

    QString path = Q3FileDialog::getExistingDirectory(QDir::currentDirPath(), 0, 0,
                   "select the base directory to reverse");

    if (! path.isEmpty()) {
        QDir d(path);
        Package * p = new Package(root, path, d.dirName());

        // scanning phase
#ifndef REVERSE
        QApplication::setOverrideCursor(Qt::waitCursor);
        PhpCatWindow::clear_trace();
#endif
        UmlCom::message("count files ...");
        progress = new Progress(file_number(d, TRUE), "Scanning in progress, please wait ...");
        scan = TRUE;
        p->reverse_directory(d, TRUE);
        scan = FALSE;

        if (progress != 0)
            delete progress;

#ifndef REVERSE
        QApplication::restoreOverrideCursor();

        root->setOpen(TRUE);
#endif

        return p;
    }
    else
        return 0;
}

void Package::progress_closed()
{
    progress = 0;
}

static bool allowed(QRegExp * rg, QString f)
{
    if (rg != 0) {
        int matchLen;
        return rg->exactMatch(f);/*
    return ((rg->match(f, 0, &matchLen) != 0) ||
	    (matchLen != (int) f.length()));*/
    }
    else
        return TRUE;
}

int Package::file_number(QDir & d, bool rec)
{
    if (! allowed(DirFilter, d.dirName()))
        return 0;

    int result = 0;
//  const QFileInfoList * list = d.entryInfoList(QDir::Files | QDir::Readable);
    /*
     if (list != 0) {
       QFileInfoListIterator it(*list);
       QFileInfo * fi;

       while ((fi = it.current()) != 0) {
         if (fi->extension(FALSE) == Ext)
    result += 1;
         ++it;
       }
     }*/

    // Oh lord, Bruno is so used to cut-n-paste, I am getting sick.
    QFileInfoList list = d.entryInfoList(QDir::Files | QDir::Readable);

    if (!list.isEmpty()) {
        QFileInfoList::iterator it = list.begin();

        while (it != list.end()) {
            if ((*it).extension(FALSE) == Ext) result += 1;

            it++;
        }
    }

    if (rec) {
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
//
// 	  result += file_number(sd, rec);
// 	}
// 	++itd;
//       }
//     }
//   }
//
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

#ifdef REVERSE
void Package::send_dir(bool rec)
{
    // reverse phase
    UmlPackage * prj = root->uml;

    while (prj->parent() != 0)
        prj = (UmlPackage *) prj->parent();

    prj->set_childrenVisible(FALSE);

    QDir d(path);

    progress = new Progress(file_number(d, TRUE),
                            (scan) ? "Scanning in progress, please wait ..."
                            : "Reverse in progress, please wait ...");

    reverse_directory(d, rec);

    if (progress != 0)
        delete progress;

    ((uml) ? uml : root->uml)->set_childrenVisible(TRUE);
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

void Package::reverse_directory(QDir & d, bool rec)
{
    if (! allowed(DirFilter, d.dirName()))
        return;

    // reads files
    QFileInfoList list = d.entryInfoList(QDir::Files | QDir::Readable);

    if (!list.isEmpty()) {
        QFileInfoList::iterator it = list.begin();

        while (it != list.end()) {
            if ((*it).extension(FALSE) == Ext) {
                if (allowed(FileFilter, (*it).fileName()))
                    reverse_file(WrapperStr((*it).filePath().toAscii().constData()), WrapperStr(my_baseName(&(*it)).toAscii().constData()));

                if (progress)
                    progress->tic();

                app->processEvents();
            }

            ++it;
        }

        /*
            while ((fi = it.current()) != 0) {
              if (fi->extension(FALSE) == Ext) {
        	if (allowed(FileFilter, fi->fileName()))
        	  reverse_file(WrapperStr(fi->filePath()), WrapperStr(my_baseName(fi)));
        	if (progress)
        	  progress->tic();
        	app->processEvents();
              }
              ++it;
            }*/
    }

    if (rec) {
        // sub directories
        list = d.entryInfoList(QDir::Dirs | QDir::NoSymLinks);

        if (!list.isEmpty()) {
            QFileInfoList::iterator itd = list.begin();

            while (itd != list.end()) {

                if ((*itd).fileName()[0] != '.') {
                    QDir sd((*itd).filePath());

                    Package * p = find(WrapperStr(sd.dirName().toAscii().constData()), TRUE);

                    if (p != 0)
                        p->reverse_directory(sd, TRUE);
                }

                ++itd;
            }
        }
    }
}

void Package::reverse_file(WrapperStr path, WrapperStr name)
{
    if (! Lex::open(path)) {
        // very strange !
        if (! scan)
            UmlCom::trace(WrapperStr("<font face=helvetica><b>cannot open <i>")
                          + path + "</i></b></font><br>");
    }
    else {
        UmlArtifact * art = 0;
        WrapperStr file_start;
        WrapperStr file_end;

        UmlCom::message(((scan) ? "scan " : "reverse ") + path);

        // go after <?[php]
        Lex::mark();

        bool redo;
        bool before_class;

        do {
            redo = FALSE;
            before_class = TRUE;

            WrapperStr s;
            char c = Lex::read_word_bis();

            while (c != 0) {
                if (c == '<') {
                    c = Lex::read_word_bis();

                    if (c == '?') {
                        if (!scan) file_start = Lex::region();

                        s = Lex::read_word();

                        if (s.lower() == "php") {
                            if (!scan) file_start = Lex::region();

                            s = Lex::read_word();
                        }

                        break;
                    }
                }
                else
                    c = Lex::read_word_bis();
            }

            aVisibility visibility = PackageVisibility;
            bool abstractp = FALSE;
            bool finalp = FALSE;
            bool inside_namespace_brace = FALSE;

            while (!s.isEmpty()) {
                if ((s == "class") || (s == "interface")) {
#ifdef REVERSE

                    if (!scan && (art == 0)) {
                        UmlPackage * pack = get_uml(TRUE);

                        if ((art = UmlBaseArtifact::create(pack->get_deploymentview(Namespace::current()), name)) == 0) {
                            UmlCom::trace(WrapperStr("<font face=helvetica><b>cannot create<i> artifact ")
                                          + name + "</i></b></font><br>");
                            Namespace::exit();
                            Lex::close();
                            return;
                        }

                        art->set_Stereotype("source");
                    }

#endif

                    if (!Class::reverse(this, s, abstractp, finalp, path, art))
                        break;

                    visibility = PackageVisibility;
                    abstractp = FALSE;
                    finalp = FALSE;

                    before_class = FALSE;
                    Lex::mark();
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
                else if ((s == "namespace") && before_class) {
                    Namespace::exit();

                    s = Lex::read_word();

                    if (s == "{")
                        inside_namespace_brace = TRUE;
                    else if (s.isEmpty()) {
                        if (!scan)
                            Lex::premature_eof();

                        break;
                    }
                    else {
                        Namespace::enter(s);
                        s = Lex::read_word();

                        if (s == "{")
                            inside_namespace_brace = TRUE;

                        // else is ';'
                    }
                }
                else if ((s == "use") && before_class) {
                    if (!scan)
                        use();
                    else
                        UmlOperation::skip_body(0);

                    Lex::finish_line();
                    Lex::clear_comments();
                }
                else if ((s == "}") && inside_namespace_brace) {
                    inside_namespace_brace = FALSE;
                    Namespace::exit();
                }
                else if (s != ";") {
                    if (before_class) {
                        if ((s == "?") && ((s = Lex::read_word()) == ">")) {
                            // this <?php ..?> doesn't contains classes
                            // search for a next <?php ..?>
                            redo = TRUE;
                            Lex::clear_comments();

                            if (!scan)
                                file_start = Lex::region();

                            break;
                        }

                        UmlOperation::skip_body(0);
                        Lex::finish_line();
                        Lex::clear_comments();

                        if (!scan)
                            file_start = Lex::region();
                    }
                    else if (!scan) {
                        // go to end of file
                        while (Lex::read_word_bis() != 0)
                            ;

                        file_end = Lex::region();
                        break;
                    }
                    else
                        break;
                }

                s = Lex::read_word();
            }
        }
        while (redo);

#ifdef REVERSE

        if (! scan) {
            if (art == 0) {
                UmlPackage * pack = get_uml(TRUE);

                if ((art = UmlBaseArtifact::create(pack->get_deploymentview(Namespace::current()), name)) == 0) {
                    UmlCom::trace(WrapperStr("<font face=helvetica><b>cannot create<i> artifact ")
                                  + name + "</i></b></font><br>");
                    Namespace::exit();
                    Lex::close();
                    return;
                }

                art->set_Stereotype("source");
            }

            if (before_class)
                art->set_PhpSource(file_start);
            else if (!Namespace::current().isEmpty()) {
                int p1 = file_start.operator QString().lastIndexOf("namespace");
                int p2 = file_start.find(';', p1);
                int p3 = file_start.find('{', p1);

                if ((p3 != -1) && ((p3 < p2) || (p2 == -1)))
                    file_start.replace(p1, p3 - p1 + 1, "${namespace}");
                else
                    file_start.replace(p1, p2 - p1 + 1, "${namespace}");

                art->set_PhpSource(file_start + "\n${definition}\n\n" + file_end);
            }
            else
                art->set_PhpSource(file_start + "\n${definition}\n\n" + file_end);

            art->unload();
        }

#endif

        Namespace::exit();

        Lex::close();
    }
}

void Package::use()
{
    WrapperStr s1;

    while (!(s1 = Lex::read_word()).isEmpty()) {
        WrapperStr s = Lex::read_word();
        WrapperStr s2;

        if (s == "as") {
            s2 = Lex::read_word();
            s = Lex::read_word();
        }
        else {
            int p = s1.operator QString().lastIndexOf('\\');

            s2 = (p == -1)
                 ? WrapperStr() // strange
                 : s1.mid(p + 1);
        }

        if (! s2.isEmpty())
            Namespace::add_alias(s2, s1);

        if ((s == ";") || s.isEmpty())
            return;
        else if (s != ",") {
            UmlOperation::skip_body(0);
            return;
        }
    }
}

void Package::update_class_list(WrapperStr pack, UmlItem * container)
{
    const Q3PtrVector<UmlItem> & ch = container->children();

    for (unsigned i = 0; i != ch.size(); i += 1) {
        UmlItem * it = ch[i];

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
        classes.insert(name, cl);
    }
    else {
        Defined.insert(name, cl);
        Undefined.remove(name);
    }

    return cl;
}

void Package::compute_type(WrapperStr name, UmlTypeSpec & typespec,
                           Class ** need_object)
{
    typespec.type = 0;
    typespec.explicit_type = 0;

    // no base type like int
    Class * cl;

    if (((cl = Defined[name]) != 0) ||
        ((cl = Undefined[name]) != 0) ||
        ((cl = classes[name]) != 0)) {
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

    if ((cl = php_classes[name]) != 0) {
        // a php class
        if (need_object != 0)
            *need_object = cl;

        typespec.type = cl->get_uml();

        return;
    }

    if (need_object == 0)
        typespec.explicit_type = name;
    else if (! Lex::identifierp(name))
        // strange
        typespec.explicit_type = name;
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

#ifdef WITH_PHPCAT
void Package::declare(const WrapperStr & name, Class * cl)
{
    Undefined.insert(name, cl);
}
#endif

Package * Package::find(WrapperStr name, bool nohack)
{
    // hack
    if (!nohack && (text(0) == (const char *) name))
        return this;

    Package * p = 0;

    foreach (TreeItem *child, children()) {
        if (((BrowserNode *) child)->isa_package() &&
            (child->text(0) == (const char *) name)) {
            p = (Package *) child;
            break;
        }
    }

    if (p == 0)
        p = new Package(this, path + "/" + name, name);

    return p;
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
#endif
        }
    }

    return uml;
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
            progress = new Progress(file_number(d, choice == 1), "Reverse in progress, please wait ...");
            PhpCatWindow::clear_trace();
            PhpSettings::set_UseDefaults(TRUE);
            reverse_directory(d, choice == 1);

            if (progress != 0)
                delete progress;

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

#ifdef WITH_PHPCAT
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

    Package  * pack = new Package(parent, path, name);

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
#endif
