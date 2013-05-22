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

#ifdef TRACE
#include <iostream>
//Added by qt3to4:
#include "misc/mystr.h"
#include <QPixmap>
//Added by qt3to4:
#include <Q3PtrList>
using namespace std;
#endif

#include <qdir.h>
#include <qdatastream.h>
#ifndef REVERSE
#include <qapplication.h>
#include <q3popupmenu.h>
#include <qcursor.h>
#include <qpainter.h>
#include <qmessagebox.h>
#endif

#include "Class.h"
#include "UmlClass.h"
#include "Lex.h"
#include "Package.h"
#include "UmlPackage.h"
#include "UmlOperation.h"
#include "UmlRelation.h"
#include "UmlAttribute.h"
#include "UmlCom.h"
#include "PhpSettings.h"
#include "PhpCatWindow.h"
#include "UmlArtifact.h"
#ifdef REVERSE
#include "Statistic.h"
#else
#include "BrowserView.h"
#include "Pixmap.h"
#include "ShowFileDialog.h"

QList<Class *> Class::Historic;
#endif

Class::Class(Package * p, const char * n, char st)
    : BrowserNode(p, n),
      uml(0), stereotype(st), abstractp(FALSE), reversedp(FALSE),
#ifdef REVERSE
      from_lib(FALSE)
#else
      description_updatedp(FALSE)
#endif
{
    p->new_class(this);
}

UmlClass * Class::get_uml()
{
    if (uml != 0)
        return uml;

    UmlItem * p = // no nested classe in php
        (UmlItem *)((Package *) parent())->get_uml()->get_classview(get_namespace());
    WrapperStr str = WrapperStr(text(0).toAscii().constData());

    uml = UmlBaseClass::create(p, str);

    if (uml == 0) {
        // probably already exist
        Q3PtrVector<UmlItem> ch = p->children();
        UmlItem * x;

        for (unsigned chindex = 0; chindex != ch.size(); chindex += 1) {
            if (((x = ch[chindex])->kind() == aClass) && (x->name() == str)) {
                uml = (UmlClass *) x;
                break;
            }
        }

        if (uml == 0) {
#ifdef REVERSE
            UmlCom::message("");
            UmlCom::trace(QString("<font face=helvetica><b>cannot create class <i>")
                          + text(0) + "</i> under <i>"
                          + parent()->text(0) + "</b></font><br>");
            throw 0;
#else
            QMessageBox::critical(0, "Fatal Error",
                                  QString("<font face=helvetica><b>cannot create class <i>")
                                  + text(0) + "</i> under <i>"
                                  + parent()->text(0) + "</b></font><br>");
            QApplication::exit(1);
#endif
        }
    }

    switch (stereotype) {
    case 'i':
        uml->set_Stereotype("interface");
        uml->set_PhpDecl(PhpSettings::interfaceDecl());
        break;

    default:
        // class
        break;
    }

    if (abstractp)
        uml->set_isAbstract(TRUE);

    return uml;
}

bool Class::already_in_bouml()
{
    Q3PtrVector<UmlItem> ch = get_uml()->children();

    for (unsigned index = 0; index != ch.size(); index += 1)
        if (ch[index]->kind() != aClass)
            return TRUE;

    return FALSE;
}

bool Class::reverse(Package * container, WrapperStr stereotype,
                    bool abstractp, bool finalp,
                    WrapperStr & path, UmlArtifact * art)
{
    WrapperStr comment = Lex::get_comments();
    WrapperStr description = Lex::get_description();
    WrapperStr name;

    if ((name = Lex::read_word()).isEmpty())
        return FALSE;

    Class * cl = container->define(name, *((const char *) stereotype));
    UmlClass * cl_uml = 0;

    if ((cl == 0) || cl->reversedp)
        return FALSE;

    if (Package::scanning()) {
        cl->abstractp = abstractp;
        cl->filename = path;
        cl->its_namespace = Namespace::current();
#ifndef REVERSE
        cl->description = comment;
#endif
    }
    else {
#ifdef REVERSE
        Statistic::one_class_more();
#endif
        cl->reversedp = TRUE;

        cl_uml = cl->get_uml();

        if (cl->already_in_bouml()) {
            cl_uml->unload();
            return FALSE;
        }

        if (! comment.isEmpty())
            cl_uml->set_Description((cl_uml->phpDecl().find("${description}") != -1)
                                    ? description : comment);

        if (abstractp)
            cl_uml->set_isAbstract(abstractp);

        if (finalp)
            cl_uml->set_isPhpFinal(finalp);
    }

    WrapperStr s = Lex::read_word();

    if (s.isEmpty()) {
        if (! Package::scanning()) {
            Lex::premature_eof();
            cl_uml->unload();
        }

        return FALSE;
    }

    if (Package::scanning()) {
        char c = s.operator QString().toAscii().at(0);

        while (c != '{') {
            if ((c = Lex::read_word_bis()) == 0)
                return FALSE;
        }
    }
    else {
        if (s == "extends") {
            if ((cl->stereotype != 'i')
                ? !cl->manage_extends(container)
                : !cl->manage_implements(container, aGeneralisation))
                return FALSE;

            s = Lex::read_word();
        }

        if (s == "implements") {
            if (! cl->manage_implements(container, aRealization))
                return FALSE;

            s = Lex::read_word();
        }

        if (s != "{") {
            if (! Package::scanning()) {
                Lex::syntax_error("<font color =\"red\">{</font> is missing");
                cl_uml->unload();
            }

            return FALSE;
        }
    }

    // no internal class, bypass definition
    if (Package::scanning()) {
        UmlOperation::skip_body(1);
        return TRUE;
    }

    while ((s = Lex::read_word()) != "}") {
        if (s.isEmpty()) {
            Lex::premature_eof();
            cl_uml->unload();
            return FALSE;
        }

#ifdef TRACE
        QLOG_INFO() << "in class def a lu '" << s << "'\n";
#endif

        if (s == ";")
            ;
        else if (! cl->manage_member(s)) {
            cl_uml->unload();
            return FALSE;
        }
    }

#ifdef REVERSE

    if ((art != 0) && (cl_uml->associatedArtifact() == 0))
        art->addAssociatedClass(cl_uml);

#endif

    cl_uml->unload();

    return TRUE;
}

bool Class::manage_extends(ClassContainer * container)
{
    // out of scanning
#ifdef TRACE
    QLOG_INFO() << text(0) << "->manage_extends()\n";
#endif

    UmlTypeSpec typespec;
    Class * cl = 0;

    if (! container->read_type(typespec, &cl))
        return FALSE;

    if (typespec.type == 0) {
#ifdef TRACE
        QLOG_INFO() << "cannot create mother\n";
#endif
        return FALSE;
    }

    return add_inherit(aGeneralisation, typespec);
}

bool Class::manage_implements(ClassContainer * container, aRelationKind k)
{
    // out of scanning
#ifdef TRACE
    QLOG_INFO() << text(0) << "->manage_implements()\n";
#endif

    for (;;) {
        UmlTypeSpec typespec;
        Class * cl = 0;

        if (! container->read_type(typespec, &cl))
            return FALSE;

        if (typespec.type == 0) {
#ifdef TRACE
            QLOG_INFO() << "cannot create mother\n";
#endif
            return FALSE;
        }

        if (!add_inherit(k, typespec))
            return FALSE;

        WrapperStr s = Lex::read_word();

        if (s == "{") {
            Lex::unread_word(s);
            return TRUE;
        }

        if (s != ",") {
            Lex::error_near(s);
            return FALSE;
        }
    }
}

bool Class::add_inherit(aRelationKind k, UmlTypeSpec & typespec)
{
    UmlRelation * rel =
        UmlRelation::create(k, uml, typespec.type);

    if (rel == 0) {
#ifdef TRACE
        QLOG_INFO() << "cannot create <|---\n";
#endif
        return FALSE;
    }

#ifdef REVERSE
    Statistic::one_relation_more();
#endif

    if (!typespec.explicit_type.isEmpty())
        rel->set_PhpDecl(typespec.explicit_type);
    else {
        rel->set_PhpDecl("${type}");
    }

    return TRUE;
}

// not in scanning phase
//
// default visibility is 'package'
//
// [<visibility>] ['static'] ['var'] '$'<varname> ['=' <value>] ';'
// [<visibility>] 'const' ctename '=' value ';'
// ['final'] [<visibility>] ['static' | 'abstract'] 'function' ...

bool Class::manage_member(WrapperStr s)
{
    WrapperStr comment = Lex::get_comments();
    WrapperStr description = Lex::get_description();
    int index;
    WrapperStr access = value_of(description, "@access", index);
    aVisibility visibility;

    if (access == "public")
        visibility = PublicVisibility;
    else if (access == "protected")
        visibility = ProtectedVisibility;
    else if (access == "private")
        visibility = PrivateVisibility;
    else
        visibility = PackageVisibility;

    if (visibility != PackageVisibility) {
        description.replace(index, access.length(), "${visibility}");

        access = value_of(comment, "@access", index);
        comment.replace(index, access.length(), "${visibility}");
    }

    bool m_staticp = FALSE;
    bool m_constp = FALSE;
    bool m_abstractp = FALSE;
    bool m_finalp = FALSE;

#ifdef TRACE
    QLOG_INFO() << "Class::manage_member(" << s << ")\n";
#endif

    for (;;) {
        if (s == "public")
            visibility = PublicVisibility;
        else if (s == "protected")
            visibility = ProtectedVisibility;
        else if (s == "private")
            visibility = PrivateVisibility;
        else if (s == "static")
            m_staticp = TRUE;
        else if (s == "const")
            m_constp = TRUE;
        else if (s == "final")
            m_finalp = TRUE;
        else if (s == "abstract")
            m_abstractp = TRUE;
        else if (s != "var")
            break;

        s = Lex::read_word();
    }

    if (s == "function") {
        // an operation
        return UmlOperation::new_one(this, visibility, m_finalp, m_abstractp,
                                     m_staticp, comment, description);
    }

    for (;;) {
        WrapperStr name = s;
        WrapperStr value;

        s = Lex::read_word();

#ifdef TRACE
        QLOG_INFO() << "define var '" << ((const char *) name) << "' followed by '" << ((const char *) s) << "'\n";
#endif

        if (s == "=") {
            // initialized variable, by pass value
            Lex::mark();
            UmlOperation::skip_expr(0);

            value = Lex::region();

#ifdef TRACE
            QLOG_INFO() << "value form is '" << ((const char *) value) << "'\n";
#endif
            char c = ((const char *) value)[value.length() - 1];

            if ((c == ';') || (c == ',')) {
                value.truncate(value.length() - 1); // remove ';' or ','
                s = (c == ';') ? ";" : ",";
            }
            else
                s = Lex::read_word();
        }

        if ((s != ";") && (s != ",")) {
            Lex::error_near(s);
            return FALSE;
        }

        if (!UmlAttribute::new_one(this, name, visibility, m_constp, m_staticp,
                                   value, comment, description))
            return FALSE;

        if (s == ";")
            return TRUE;

        // var1, var2 ...;
        s = Lex::read_word();

        if (s.isEmpty()) {
            if (! Package::scanning())
                Lex::premature_eof();

            return FALSE;
        }
    }
}

void Class::compute_type(WrapperStr name, UmlTypeSpec & typespec,
                         Class ** need_object)
{
    // no nested classe in php
    ((Package *) parent())->compute_type(name, typespec, need_object);
}

QString Class::get_path() const
{
    return filename;
}

#ifndef REVERSE

const QPixmap * Class::pixmap(int) const
{
    return ((stereotype == 'i') || (stereotype == '@'))
           ? InterfaceIcon : ClassIcon;
}

void Class::paintCell(QPainter * p, const QColorGroup & cg, int column,
                      int width, int alignment)
{
    if (abstractp) {
        QFont fnt = p->font();

        fnt.setItalic(TRUE);
        p->setFont(fnt);
        BrowserNode::paintCell(p, cg, column, width, alignment);
        fnt.setItalic(FALSE);
        p->setFont(fnt);
    }
    else
        BrowserNode::paintCell(p, cg, column, width, alignment);
}

// called when the user click on the node

void Class::activate()
{
    manage_historic();
    // Qt will call activated()
}

// called when the node is selected through comment reference

void Class::selected()
{
    manage_historic();
    activated();
}

void Class::manage_historic()
{
    // manage historic
    while (Historic.getLast() != Historic.current())
        Historic.removeLast();

    Historic.append(this);
}

// called when the node is activated for any reason
// for instance because the user clink on it, or
// select it using keyboard arrows

void Class::activated()
{
    PhpCatWindow::clear_trace();

    if (! description_updatedp) {
        // modify description, made only now to take into account
        // future modifications on old backup

        const char * p = description;

        if (p == 0)
            return;

        while (*p == '\n')
            p += 1;

        QString s = p;

        if (s.left(3) != "<p>")
            s.insert(0, "<p>");

        int index = 0;

        while ((index = s.find("\n@", index)) != -1) {
            if (s[index + 2].isLetter()) {
                int index2 = index;

                s.replace(index + 1, 1, "<p><b>");

                for (index += 7; !s[index].isSpace(); index += 1)
                    ;

                s.insert(index, "</b>");

                if ((index == index2 + 10) && (s.mid(index2 + 7, 3) == "see")) {
                    for (index2 = index + 4; s[index2].isSpace(); index2 += 1)
                        ;

                    s.insert(index2, "{@link ");

                    for (index2 += 7; !s[index2].isSpace(); index2 += 1)
                        ;

                    s.insert(index2, '}');
                    index = index2 + 1;
                }
            }
            else
                s.replace(index + 1, 1, "<p>");
        }

        index = 0;

        while ((index = s.find("{@link ", index)) != -1) {
            int index2 = s.find("}", index + 7);

            if (index2 == -1)
                index += 7;
            else {
                int index3 = index2;

                while (!s[--index3].isSpace())
                    ;

                if (index3 == index + 6) {
                    if (s[index3 + 1] == QChar('#')) {
                        // does not have class sub artifacts
                        // {@link #X} => X
                        s.remove(index2, 1);
                        s.remove(index, 8);
                    }
                    else {
                        // {@link X} => <a href="X">X</a>
                        QString X = s.mid(index + 7, index2 - index - 7);
                        int index4 = X.find('#');

                        if (index4 != -1) {
                            // does not have class sub artifacts
                            // {@link X#Y} => <a href="X">X</a>.Y
                            X.truncate(index4);
                            s.replace(s.find('#', index), 1, "\">" + X + "</a>.");
                            s.replace(index, 7, "<a href=\"");
                        }
                        else {
                            s.replace(index2, 1, "\">" + X + "</a>");
                            s.replace(index, 7, "<a href=\"");
                        }
                    }
                }
                else {
                    QString X = s.mid(index + 7, index3 - index - 7);
                    int index4 = X.find('#');

                    if (index4 != -1) {
                        // does not have class sub artifacts
                        // {@link X#Y Y} => <a href="X">X</a>.Y
                        X.truncate(index4);
                        s.remove(index3, index2 - index3 + 1);
                        s.replace(s.find('#', index), 1, "\">" + X + "</a>.");
                        s.replace(index, 7, "<a href=\"");
                    }
                    else {
                        // {@link X Y} => <a href="X">Y</a>
                        s.replace(index2, 1, "</a>");
                        s.replace(index3, 1, "\">");
                        s.replace(index, 7, "<a href=\"");
                    }
                }
            }
        }

        description = s;
        description_updatedp = TRUE;
    }

    PhpCatWindow::trace(description);
}

void Class::historic_back()
{
    if (Historic.current() != Historic.getFirst())
        BrowserView::select(Historic.prev());
}

void Class::historic_forward()
{
    if (Historic.current() != Historic.getLast())
        BrowserView::select(Historic.next());
}

void Class::menu()
{
    Q3PopupMenu m(0);

    m.insertItem(text(0), -1);
    m.insertSeparator();

    if (! reversed())
        m.insertItem("Send it", 0);

    m.insertItem("Show file", 1);

    switch (m.exec(QCursor::pos())) {
    case 0: {
        PhpSettings::set_UseDefaults(TRUE);
        UmlCom::message("reverse " + filename);
        PhpCatWindow::clear_trace();
        ((Package *) parent())->reverse_file(filename);
        UmlCom::message("");
    }
    break;

    case 1:
        (new ShowFileDialog(filename))->show();
        break;
    }
}

void Class::refer(const QString & href)
{
    ((BrowserNode *) parent())->refer(href);
}

// note : 'QDataStream >> char *' bugged on windows

void Class::backup(QDataStream  & dt) const
{
    switch (stereotype) {
    case 'i':
        dt << ((Q_INT8)((abstractp) ? 'i' : 'I'));
        break;

    case 'e':
        dt << ((Q_INT8) 'E');
        break;

    default:
        // class
        dt << ((Q_INT8)((abstractp) ? 'c' : 'C'));
        break;
    }

    Q_INT32 len;

    len = text(0).length();
    dt << len;
    dt.writeRawBytes(text(0), len);

    len = filename.length();
    dt << len;
    dt.writeRawBytes(filename, len);

    len = description.length();
    dt << len;

    if (!description.isEmpty())
        dt.writeRawBytes(description, len);
}

#endif // REVERSE

#ifdef WITH_PHPCAT
void Class::restore(QDataStream  & dt, char c, Package * parent)
{
    Q_INT32 len;

    dt >> len;
    char * n = new char[len + 1];
    dt.readRawBytes(n, len);
    n[len] = 0;

    dt >> len;
    char * fn = new char[len + 1];
    dt.readRawBytes(fn, len);
    fn[len] = 0;

    dt >> len;
    char * d;

    if (len == 0)
        d = 0;
    else {
        d = new char[len + 1];
        dt.readRawBytes(d, len);
        d[len] = 0;
    }

    Class * cl = new Class(parent, n, ((c == 'c') || (c == 'i')) ? c : (c - 'A' + 'a'));

#ifdef REVERSE
    cl->from_lib = TRUE;
#endif

    WrapperStr name(n);

    parent->declare(n, cl);

    if ((c == 'i') || (c == 'c'))
        cl->abstractp = TRUE;

    cl->filename = fn;
#ifndef REVERSE
    cl->description = d;
#endif

    delete [] n;
    delete [] fn;

    if (d) delete [] d;
}
#endif

