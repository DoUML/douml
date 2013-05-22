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

using namespace std;
#endif

#include <qstringlist.h>

#include "Class.h"
#include "Package.h"
#include "UmlPackage.h"
#include "UmlRelation.h"
#include "UmlOperation.h"
#include "UmlAttribute.h"
#include "CppSettings.h"
#include "UmlExtraClassMember.h"
#include "Lex.h"
#include "UmlCom.h"
#ifdef REVERSE
#include "CppCatWindow.h"
#endif
#include "Statistic.h"
#include "Logging/QsLog.h"

Class::Class(BrowserNode * parent, const char * name, const WrapperStr & st)
    : BrowserNode(parent, name), uml(0), reversedp(FALSE),
#ifdef REVERSE
      from_lib(FALSE)
# ifdef ROUNDTRIP
      , updated(FALSE)
# endif
#else
      have_formals(FALSE)
#endif
{
    if (!st.isEmpty()) {
        stereotype_declared = TRUE;

        if (st != "class")
            stereotype = st;
    }
    else
        stereotype_declared = FALSE;
}

#ifdef ROUNDTRIP
Class::Class(BrowserNode * p, UmlClass * ucl)
    : BrowserNode(p, ucl->name()),
      uml(ucl), reversedp(FALSE), from_lib(FALSE), updated(FALSE)
{
    // to allow a new stereotype
    stereotype_declared = FALSE;
}
#endif

// to not have warning
Class::~Class()
{
}

Class * Class::reverse(ClassContainer * container, WrapperStr stereotype,
                       const Q3ValueList<FormalParameterList> & tmplts,
                       const WrapperStr & path, WrapperStr name
#ifdef ROUNDTRIP
                       , bool rndtrp, QList<UmlItem *> & expectedorder
#endif
                      )
{
    bool from_typedef = !name.isEmpty();
    WrapperStr comment = Lex::get_comments();
    WrapperStr description = Lex::get_description();
    WrapperStr q_modifier;

    if (!from_typedef)
        if ((name = Lex::read_word()).isEmpty())
            return 0;

    if ((strncmp(name, "Q_EXPORT", 8) == 0) ||
        (strncmp(name, "QM_EXPORT", 9) == 0) ||
        (strncmp(name, "Q_PNGEXPORT", 11) == 0)) {
        q_modifier = name + " ";

        if ((name = Lex::read_word()).isEmpty())
            return 0;
    }

    WrapperStr s;

    if ((s = Lex::read_word()).isEmpty()) {
        Lex::premature_eof();
        return 0;
    }

    if (s == ";") {
        if (!Package::scanning()) {
            s = "    " + stereotype + " " + name + ";\n";
            container->declaration(name, stereotype, s
#ifdef ROUNDTRIP
                                   , rndtrp, expectedorder
#endif
                                  );
        }

        Lex::finish_line();
        Lex::clear_comments();

        return 0;
    }

    // class definition
    Class * cl = container->define(Lex::normalize(name), stereotype);

    if ((cl == 0) || cl->reversedp) {
        Lex::unread_word();
        return 0;
    }

#ifdef ROUNDTRIP
    UmlClass * cl_uml = cl->uml;

    if ((cl_uml != 0) &&
        !cl_uml->is_roundtrip_expected() &&
        !cl_uml->is_created()) {
        // bypass all the class
        // note : user can't ask for update a nested class
        char c;

        while ((c = Lex::read_word_bis(TRUE, FALSE)) != '{') {
            if (c == 0) {
                Lex::premature_eof();
                throw 0;
            }
        }

        UmlOperation::skip_body(1);
        return cl;
    }

    bool roundtrip = FALSE;
    QList<UmlItem *> expected_order;
#else
    UmlClass * cl_uml = 0;
#endif

    if (Package::scanning()) {
        cl->filename = path;
        cl->its_namespace = Namespace::current();
#ifdef REVERSE

        if (! tmplts.isEmpty())
            cl->formals = tmplts.first();

#else
        cl->have_formals = !tmplts.isEmpty();
        cl->description = comment;
#endif
    }
    else {
        cl->reversedp = TRUE;
        cl_uml = cl->get_uml();

#ifdef ROUNDTRIP

        if (rndtrp)
            expectedorder.append(cl_uml);

        if (cl_uml->is_created())
            Statistic::one_class_created_more();
        else
            roundtrip = TRUE;

#else

        if (cl->already_in_bouml()) {
            cl_uml->unload();
            return 0;
        }

# ifdef REVERSE
        Statistic::one_class_more();
# endif
#endif

        WrapperStr d;

        if (stereotype == "union")
            d = CppSettings::unionDecl();
        else if (stereotype == "struct")
            d = CppSettings::structDecl();
        else if (! q_modifier.isEmpty())
            d = CppSettings::classDecl();

        if (! d.isEmpty()) {
            if (! q_modifier.isEmpty()) {
                int index = d.find("${name}");

                if (index != -1)
                    d.insert(index, (const char *)q_modifier);
            }
        }

#ifdef ROUNDTRIP

        if (roundtrip) {
            if (d.isEmpty())
                d = CppSettings::classDecl();

            if (d.find("${description}") != -1) {
                if (nequal(cl_uml->description(), description)) {
                    cl_uml->set_Description(description);
                    cl->set_updated();
                }
            }
            else if (nequal(cl_uml->description(), Lex::simplify_comment(comment))) {
                // comment simplified
                cl_uml->set_Description(comment);
                cl->set_updated();
            }

            if (stereotype == "class") {
                if (!cl_uml->stereotype().isEmpty()) {
                    WrapperStr cppst = CppSettings::classStereotype(cl_uml->stereotype());

                    if ((cppst == "union") || (cppst == "struct") || (cppst == "enum")) {
                        cl_uml->set_Stereotype("");
                        cl->set_updated();
                    }
                }
            }
            else if (CppSettings::classStereotype(cl_uml->stereotype()) != stereotype) {
                cl_uml->set_Stereotype(stereotype);
                cl->set_updated();
            }

            if (neq(cl_uml->cppDecl(), d)) {
                cl_uml->set_CppDecl(d);
                cl->set_updated();
            }
        }
        else {
#endif

            if (! d.isEmpty())
                // not a class being the default decl
                cl_uml->set_CppDecl(d);

            if (! comment.isEmpty()) {
                if (d.isEmpty())
                    d = CppSettings::classDecl();

                cl_uml->set_Description((d.find("${description}") != -1)
                                        ? description : Lex::simplify_comment(comment));
            }

#ifdef ROUNDTRIP
        }

#endif
    }

    if (s == ":") {
        if (Package::scanning()) {
            char c;

            do
                c = Lex::read_word_bis(FALSE, FALSE);

            while ((c != '{') && (c != 0));

            s = (c == 0) ? "" : "{";
        }
        else {
            if (! cl_uml->manage_inherit(container, tmplts
#ifdef REVERSE
                                         , cl->from_libp()
# ifdef ROUNDTRIP
                                         , roundtrip, expected_order
                                         , rndtrp, expectedorder
# endif
#endif
                                        ))
                return 0;

            s = Lex::read_word();
        }
    }

    if (from_typedef &&
        (s != "{") &&
        (s = Lex::read_word()).isEmpty()) {
        if (! Package::scanning())
            Lex::premature_eof();

        return 0;
    }

    if (s != "{") {
        if (! Package::scanning())
            Lex::syntax_error("<font color =\"red\">{</font> is missing");

        UmlOperation::skip_body(0);
        return 0;
    }

    aVisibility visibility =
        (stereotype == "class") ? PrivateVisibility : PublicVisibility;
    bool qt_object = (name == "QObject");
    bool contains_using_namespace = FALSE;
    bool contains_using_class = FALSE;

    if (cl_uml)
        cl_uml->set_under_construction(TRUE);

    while (Lex::mark(), ((s = Lex::read_word()) != "}")) {
        if (s.isEmpty()) {
            if (! Package::scanning())
                Lex::premature_eof();

            break;
        }

#ifdef DEBUG_DOUML
        QLOG_INFO() << "in class def a lu '" << s << "'\n";
#endif

        if (s == "public")
            visibility = PublicVisibility;
        else if (s == "protected")
            visibility = ProtectedVisibility;
        else if (s == "private")
            visibility = PrivateVisibility;
        else if (qt_object && (s == "signals")) {
            if (Lex::read_word() != ":") {
                if (! Package::scanning())
                    Lex::syntax_error("<font color =\"red\">:</font> is missing after <font color =\"red\"> signals</font>");

                Lex::unread_word();
            }

            if (!Package::scanning())
                cl->declaration("signals", 0, "  signals:"
#ifdef ROUNDTRIP
                                , roundtrip, expected_order
#endif
                               );

            continue;
        }
        else if (s == "Q_OBJECT") {
            if (!Package::scanning())
                cl->declaration("Q_OBJECT", 0, "  Q_OBJECT"
#ifdef ROUNDTRIP
                                , roundtrip, expected_order
#endif
                               );

            qt_object = TRUE;
            continue;
        }
        else if ((s == "Q_PROPERTY") || (s == "Q_ENUMS") ||
                 (s == "Q_OVERRIDE") || (s == "Q_SETS") ||
                 (s == "Q_DUMMY_COMPARISON_OPERATOR")) {
            Lex::mark();
            Lex::read_word();	// (
            UmlOperation::skip_expr(")");

            if (!Package::scanning())
                cl->declaration(s, 0, "    " + s + Lex::region()
#ifdef ROUNDTRIP
                                , roundtrip, expected_order
#endif
                               );

            continue;
        }
        else if (s == "using") {
            if ((s = Lex::read_word()).isEmpty()) {
                if (!Package::scanning())
                    Lex::premature_eof();

                break;
            }
            else if (s == "namespace") {
                if (Package::scanning())
                    UmlOperation::skip_body(0);
                else if ((s = Lex::read_word()).isEmpty()) {
                    Lex::premature_eof();
                    break;
                }
                else {
                    if (!contains_using_namespace) {
                        contains_using_namespace = TRUE;
                        Namespace::save_using_scope();
                    }

                    Namespace::add_using(s);
                    cl->declaration("using", 0, "  using namespace " + s + ";"
#ifdef ROUNDTRIP
                                    , roundtrip, expected_order
#endif
                                   );
                }
            }
            else if (!Package::scanning()) {
                s = Lex::complete_template_type(s);

                UmlTypeSpec typespec;

                if (!cl->find_type(s, typespec)) {
#if 0
                    int index = s.findRev("::");

                    if ((index == -1) || !cl->find_type(s.left(index), typespec))
                        Lex::warn("unknown class <font color =\"red\">" +
                                  Lex::quote((index == -1) ? s : s.left(index)) + "</font>");

#endif
                    cl->declaration("using", 0, "  using " + s + ";"
#ifdef ROUNDTRIP
                                    , roundtrip, expected_order
#endif
                                   );
                }
                else {
                    if (!contains_using_class) {
                        contains_using_class = TRUE;
                        UmlClass::save_using_scope();
                    }

                    typespec.type->using_it();
                    cl->declaration("using", 0, "  using " + s + ";"
#ifdef ROUNDTRIP
                                    , roundtrip, expected_order
#endif
                                   );
                }
            }
            else
                UmlOperation::skip_body(0);

            continue;
        }
        else {
            cl->manage_member(s, visibility, container, path
#ifdef ROUNDTRIP
                              , roundtrip, expected_order
#endif
                             );
            continue;
        }

#ifdef DEBUG_DOUML
        QLOG_INFO() << "visi = " << s << '\n';
#endif

        s = Lex::read_word();

        if (qt_object && (s == "slots")) {
            s = Lex::read_word();

            if (!Package::scanning() && (s == ":")) {
                switch (visibility) {
                case PublicVisibility:
                    cl->declaration("slots", 0, "  public slots:"
#ifdef ROUNDTRIP
                                    , roundtrip, expected_order
#endif
                                   );
                    break;

                case ProtectedVisibility:
                    cl->declaration("slots", 0, "  protected slots:"
#ifdef ROUNDTRIP
                                    , roundtrip, expected_order
#endif
                                   );
                    break;

                default:
                    cl->declaration("slots", 0, "  private slots:"
#ifdef ROUNDTRIP
                                    , roundtrip, expected_order
#endif
                                   );
                    break;
                }
            }
        }

        if ((s != ":") && !Package::scanning())
            Lex::syntax_error("<font color =\"red\">:</font> is missing after visibility");
    }

    if (!from_typedef && !Package::scanning()) {
        if ((s = Lex::read_word()).isEmpty())
            Lex::premature_eof();
        else if (s != ";") {
            Lex::error_near(s);
            UmlOperation::skip_body((s == "{") ? 1 : 0);
        }
    }

    if (!Package::scanning()) {
        cl_uml->set_under_construction(FALSE);
#ifdef REVERSE
# ifdef ROUNDTRIP

        if (roundtrip) {
            cl_uml->reorder(expected_order);

            if (cl->updated) {
                Statistic::one_class_updated_more();

                UmlCom::trace(QString("<font face=helvetica>class <i>")
                              + cl->text(0) + "</i> updated from <i>"
                              + Lex::filename() + "</i></font><br>");
            }
        }

# endif

        if (!cl->from_libp())
            cl_uml->need_artifact(cl->get_namespace());

#endif

        // do not unload cl_uml because of the (sub class) friend operations
    }

    if (contains_using_namespace)
        Namespace::restore_using_scope();

    if (contains_using_class)
        UmlClass::restore_using_scope();

    return cl;
}

void Class::declaration(const WrapperStr & name, const WrapperStr & stereotype,
                        const WrapperStr & decl
#ifdef ROUNDTRIP
                        , bool rndtrp, QList<UmlItem *> & expectedorder
#endif
                       )
{
    UmlExtraClassMember * ex;

#ifdef ROUNDTRIP

    if (rndtrp &&
        ((ex = get_uml()->search_for_extra(name, decl)) != 0)) {
        ex->set_usefull();
        expectedorder.append(ex);

        if (!stereotype.isEmpty() && neq(ex->stereotype(), stereotype)) {
            ex->set_Stereotype(stereotype);
            set_updated();
        }
    }
    else
#endif
        if ((ex = UmlExtraClassMember::create(get_uml(), name)) != 0) {
            if (! stereotype.isEmpty())
                ex->set_Stereotype(stereotype);

            if (! decl.isEmpty())
                ex->set_CppDecl(decl);

#ifdef ROUNDTRIP

            if (rndtrp) {
                set_updated();
                expectedorder.append(uml);
            }

#endif
        }
}

void Class::manage_member(WrapperStr s, aVisibility visibility,
                          ClassContainer * /*container*/,
                          const WrapperStr & path
#ifdef ROUNDTRIP
                          , bool roundtrip, QList<UmlItem *> & expected_order
#endif
                         )
{
    WrapperStr comment = Lex::get_comments();
    WrapperStr description = Lex::get_description();

    if (s == ";")
        return;

    bool destructor = FALSE;
    bool staticp = FALSE;
    bool virtualp = FALSE;
    bool constp = FALSE;
    bool mutablep = FALSE;
    bool volatilep = FALSE;
    bool typenamep = FALSE;
    bool inlinep = FALSE;
    bool explicitp = FALSE;
    bool friendp = FALSE;
    WrapperStr q_modifier;	// not yet used
    WrapperStr modifier;
    WrapperStr type;
    WrapperStr name;
    WrapperStr array;
    WrapperStr value;
    WrapperStr bitfield;
    WrapperStr pretype;	// struct/union/class/enum
    Q3ValueList<FormalParameterList> tmplts;
    WrapperStr friend_template;

#ifdef DEBUG_DOUML
    QLOG_INFO() << "Class::manage_member(" << s << ")\n";
#endif

    for (;;) {
        if (s == "static") {
            staticp = TRUE;
            Lex::mark();
        }
        else if (s == "virtual") {
            virtualp = TRUE;
            Lex::mark();
        }
        else if (s == "const") {
            if (!modifier.isEmpty())
                modifier += WrapperStr(" ") + s;
            else {
                constp = TRUE;
                Lex::mark();
            }
        }
        else if (s == "mutable") {
            if (!modifier.isEmpty())
                modifier += WrapperStr(" ") + s;
            else {
                mutablep = TRUE;
                Lex::mark();
            }
        }
        else if (s == "volatile") {
            if (!modifier.isEmpty())
                modifier += WrapperStr(" ") + s;
            else {
                volatilep = TRUE;
                Lex::mark();
            }
        }
        else if (s == "typename") {
            // note : may have "const typename ...", but not "typename const ..."
            if (!modifier.isEmpty())
                modifier += WrapperStr(" ") + s;
            else {
                typenamep = TRUE;
                Lex::mark();
            }
        }
        else if (s == "inline") {
            inlinep = TRUE;
            Lex::mark();
        }
        else if (s == "explicit") {
            explicitp = TRUE;
            Lex::mark();
        }
        else if ((s == "unsigned") || (s == "signed") ||
                 (s == "void") || (s == "bool"))
            type = s;
        else if ((s == "char") || (s == "short") || (s == "int") ||
                 (s == "long") || (s == "float") || (s == "double")) {
            type = (type.isEmpty()) ? s : type + " " + s;
        }
        else if (Lex::star(s) || (s == "&") || (s == "volatile")) {
            if (!modifier.isEmpty())
                modifier += WrapperStr(" ") + s;
            else if (!type.isEmpty())
                modifier = s;
            else {
#ifdef DEBUG_DOUML
                QLOG_INFO() << "ERROR modifier " << s << " et type empty\n";
#endif

                if (!Package::scanning())
                    Lex::error_near(s);

                break;
            }
        }
        else if (s == "(") {
            // suppose an operation
            if (Package::scanning())
                break;

            if (type.isEmpty()) {
                if (!Package::scanning())
                    Lex::error_near("(");

                break;
            }

            BooL func = FALSE;
            WrapperStr cl_name = WrapperStr(text(0).toAscii().constData());
            WrapperStr cl_full_name = cl_name;
            int index;

            if ((index = cl_name.find('<')) != -1)
                cl_name.truncate(index);

            if (name.isEmpty()) {
                if ((type == cl_full_name) ||
                    (type == (WrapperStr("~") + cl_full_name))) {
                    name = type;
                    type = "";
                }
                else if ((type == cl_name) ||
                         (type == (WrapperStr("~") + cl_name))) {
                    name = type;
                    type = "";
                }
                else if (type.left(9) == "operator ") {
                    name = type;
                    type = 0;
                }
                else {
                    WrapperStr typ;

                    if (((index = type.find('<')) != -1) &&
                        // wrong tests : not so simple
                        (((typ = type.left(index)) == cl_name) ||
                         (typ == (WrapperStr("~") + cl_name)))) {
                        name = type;
                        type = "";
                    }
                    else {
                        if (! UmlOperation::pfunc(func, name, type, "${name}"))
                            break;

                        //???modifier = FALSE;
                        if (!func) {
                            s = Lex::read_word();
                            continue;
                        }
                    }
                }
            }

            UmlOperation::new_one(this, name, tmplts, type, modifier, pretype, visibility,
                                  inlinep, virtualp, staticp, constp, volatilep,
                                  typenamep, explicitp, friendp, friend_template,
                                  comment, description, func
#ifdef ROUNDTRIP
                                  , roundtrip, expected_order
#endif
                                 );
            return;
        }
        else if (s == "=") {
            // initialized variable, by pass value
            Lex::mark();
            UmlOperation::skip_expr(";");
            Lex::unread_word();	// ';'

            if (Package::scanning())
                break;

            value = "= " + Lex::region();
        }
        else if (s == ";") {
            if (Package::scanning())
                return;

            if (name.isEmpty() || type.isEmpty()) {
                Lex::error_near(";");
                return;
            }

            UmlTypeSpec dest_type;
            WrapperStr typeform = "${type}";

            if (CppSettings::umlType(type).isEmpty() &&
                (modifier.isEmpty() || (modifier == "*") || (modifier == "&")) &&
                bitfield.isEmpty() &&
                !typenamep &&
                (compute_type(type, dest_type, typeform, TRUE), dest_type.type != 0) &&
                ((typeform == "${type}") || (array.isEmpty() && modifier.isEmpty())))
                UmlRelation::new_one(this, name, dest_type.type, modifier, pretype,
                                     array, typeform, visibility, staticp, constp,
                                     mutablep, volatilep, value, comment, description
#ifdef ROUNDTRIP
                                     , roundtrip, expected_order
#endif
                                    );
            else
                UmlAttribute::new_one(this, name, type, modifier, pretype, array,
                                      visibility, staticp, constp, typenamep, mutablep,
                                      volatilep, bitfield, value, comment, description
#ifdef ROUNDTRIP
                                      , roundtrip, expected_order
#endif
                                     );

            return;
        }
        else if (s == "friend") {
            s = Lex::read_word();

            if (s == "QMAC_PASCAL")
                s = Lex::read_word();

            if (s.isEmpty()) {
                if (! Package::scanning())
                    Lex::premature_eof();

                break;
            }
            else if ((s != "class") && (s != "struct") && (s != "union")) {
#if 0

                if (! Package::scanning()) {
                    Lex::unread_word();
                    UmlOperation::skip_body(0);
                    s = Lex::region() + '\n';
                    declaration(0, "friend", s);
                    return;
                }

                break;
#else
                friendp = TRUE;
                Lex::unread_word();
                Lex::mark();
#endif
            }
            else if (Package::scanning())
                break;
            else {
                s = Lex::read_word();

                int index = s.find('<');

                if (index != -1)
                    s.truncate(index);

                UmlTypeSpec other;
                WrapperStr typeform;

                if (s.isEmpty()) {
                    Lex::error_near("friend");
                    break;
                }
                else if (compute_type(s, other, typeform), other.type == 0)
                    Lex::warn("unknown class <font color =\"red\">" + Lex::quote(s) + "</font>");

                s = Lex::read_word();

                if (s.isEmpty() || (s != ";")) {
                    Lex::error_near("friend");
#ifdef DEBUG_DOUML
                    QLOG_INFO() << "ERROR ';' doit terminer un 'friend'\n";
#endif
                    break;
                }

                if (other.type != 0) {
#ifdef ROUNDTRIP

                    if (roundtrip)
                        UmlRelation::new_friend(this, other.type, expected_order);
                    else
#endif
                        UmlRelation::new_friend(get_uml(), other.type);
                }
                else {
                    s = Lex::region();

#ifdef ROUNDTRIP
                    UmlExtraClassMember * ex;

                    if (roundtrip &&
                        ((ex = uml->search_for_extra("friend", s)) != 0)) {
                        ex->set_usefull();
                        expected_order.append(ex);
                    }
                    else
#endif
                        UmlExtraClassMember::create(get_uml(), "friend")->set_CppDecl(s);
                }

                Lex::finish_line();
                Lex::clear_comments();
                return;
            }
        }
        else if ((s == "struct") ||
                 (s == "union") ||
                 (s == "enum")) {
            Lex::mark();

            WrapperStr s2 = Lex::read_word();

            if ((strncmp(s2, "Q_EXPORT", 8) == 0) ||
                (strncmp(s2, "QM_EXPORT", 9) == 0) ||
                (strncmp(s2, "Q_PNGEXPORT", 11) == 0))
                s2 = Lex::read_word();

            if (s2 == "{") {
                // anonymous type
                UmlOperation::skip_body(1);

                if (Package::scanning())
                    return;

                if (! type.isEmpty()) {
                    if (! Package::scanning())
                        Lex::error_near(s);

#ifdef DEBUG_DOUML
                    QLOG_INFO() << "ERROR '" << s << " {' alors qu a deja le type '" << type << "' et le nom '" << name << "'\n";
#endif
                    return;
                }

                type = s + " " + Lex::region();

                if ((s2 = Lex::read_word()) == ";") {
                    type = 0;
                    Lex::come_back();

                    if (s == "enum")
                        reverse_enum(this, path, 0
#ifdef ROUNDTRIP
                                     , roundtrip, expected_order
#endif
                                    );
                    else
                        reverse(this, s, tmplts, path, 0
#ifdef ROUNDTRIP
                                , roundtrip, expected_order
#endif
                               );

                    return;
                }
                else
                    Lex::unread_word();
            }
            else if (! Lex::identifierp(s2, TRUE)) {
                if (! Package::scanning())
                    Lex::error_near(s2);

                return;
            }
            else if ((s2 = Lex::read_word()) == ";") {
                // form like 'struct X;'
                Lex::come_back();
                reverse(this, s, tmplts, path, 0
#ifdef ROUNDTRIP
                        , roundtrip, expected_order
#endif
                       );
                return;
            }
            else if ((s2 != "{") && (s2 != ":")) {
                // form like 'struct X Y'
                pretype = s;
                Lex::come_back();
            }
            else {
                Lex::come_back();

                if (s == "enum")
                    reverse_enum(this, path, 0
#ifdef ROUNDTRIP
                                 , roundtrip, expected_order
#endif
                                );
                else
                    reverse(this, s, tmplts, path, 0
#ifdef ROUNDTRIP
                            , roundtrip, expected_order
#endif
                           );

                return;
            }
        }
        else if (s == "class") {
            Lex::mark();

            WrapperStr s2 = Lex::read_word();

            if ((strncmp(s2, "Q_EXPORT", 8) == 0) ||
                (strncmp(s2, "QM_EXPORT", 9) == 0) ||
                (strncmp(s2, "Q_PNGEXPORT", 11) == 0))
                s2 = Lex::read_word();

            if (! Lex::identifierp(s2, TRUE)) {
                if (! Package::scanning())
                    Lex::error_near(s2);

                return;
            }
            else if (((s2 = Lex::read_word()) != "{") && (s2 != ":") && (s2 != ";")) {
                // form like 'class X Y'
                pretype = s;
                Lex::come_back();
            }
            else {
                Lex::come_back();
                reverse(this, s, tmplts, path, 0
#ifdef ROUNDTRIP
                        , roundtrip, expected_order
#endif
                       );
                return;
            }
        }
        else if (s == "typedef") {
            reverse_typedef(this, path, tmplts
#ifdef ROUNDTRIP
                            , roundtrip, expected_order
#endif
                           );
            return;
        }
        else if (s == "template") {
            FormalParameterList tmplt;

            tmplts.append(tmplt);

            if (!get_template(tmplts.last()))
                break;
        }
        else if (s == "QT_STATIC_CONST")
            staticp = constp = TRUE;
        else if ((strncmp(s, "Q_EXPORT", 8) == 0) ||
                 (strncmp(s, "QM_EXPORT", 9) == 0) ||
                 (strncmp(s, "Q_PNGEXPORT", 11) == 0))
            q_modifier = s;
        else if ((s == "QMAC_PASCAL") || (s == "Q_TYPENAME"))
            ; // lost
        else if (Lex::identifierp(s, FALSE)) {
            if (type.isEmpty()) {
                s = Lex::complete_template_type(s);
#ifdef DEBUG_DOUML
                QLOG_INFO() << "type = '" << s << "'\n";
#endif
                type = (destructor) ? WrapperStr("~") + s : s;	// will be the name if ~
            }
            else if (name.isEmpty()) {
                name = s;
#ifdef DEBUG_DOUML
                QLOG_INFO() << "name = '" << name << "'\n";
#endif
            }
            else {
                if (!Package::scanning())
                    Lex::error_near(s);

#ifdef DEBUG_DOUML
                QLOG_INFO() << "ERROR '" << s << "' alors qu a deja le type '" << type << "' et le nom '" << name << "'\n";
#endif
                break;
            }
        }
        else if (s == "~")
            destructor = TRUE;
        else if (s.isEmpty()) {
            if (!Package::scanning())
                Lex::premature_eof();

            return;
        }
        else if (*((const char *) s) == '[')
            array += s;
        else if (s == ":") {
            bitfield = Lex::read_word();
        }
        else if ((s == "<") && friendp &&
                 friend_template.isEmpty() && Lex::finish_template(s))
            friend_template = " " + s;
        else {
            if (!Package::scanning())
                Lex::error_near(s);

#ifdef DEBUG_DOUML
            QLOG_INFO() << "ERROR : '" << s << "'\n";
#endif
            break;
        }

        s = Lex::read_word();
    }

    // here the end of member must be skip, because of an error
    // or because it is useless in scan case
    UmlOperation::skip_body(0);
}

Class * Class::reverse_enum(ClassContainer * container,
                            const WrapperStr & path, WrapperStr name
#ifdef ROUNDTRIP
                            , bool rndtrp, QList<UmlItem *> & expectedorder
#endif
                           )
{
    bool from_typedef = !name.isEmpty();
    WrapperStr comment = Lex::get_comments();
    WrapperStr description = Lex::get_description();
    WrapperStr s;

    if (!from_typedef)
    {
        name = Lex::read_word();
        //QLOG_INFO() << "Name is: "     << name;
        if (name.isEmpty())
        {
            if (!Package::scanning())
                Lex::premature_eof();

            return 0;
        }

        if (name == "{")
        {
            if (!Package::scanning()) {
                Lex::unread_word();
                name = 0;
            }
            else {
                // will see later
                UmlOperation::skip_body(1);
                return 0;
            }
        }
    }
    s = Lex::read_word();
    QLOG_INFO() << "S is: " << s.operator QString();
    if (s.isEmpty())
    {
        if (!Package::scanning())
            Lex::premature_eof();

        return 0;
    }

    if (s == ";") {
        Lex::finish_line();
        Lex::clear_comments();
        return (Package::scanning())
               ? 0 : container->declare_if_needed(name, "enum");
    }

    if (from_typedef &&
        (s != "{") &&
        (s = Lex::read_word()).isEmpty()) {
        if (!Package::scanning())
            Lex::premature_eof();

        return 0;
    }

    if (s != "{") {
        if (!Package::scanning())
            Lex::syntax_error(WrapperStr("font color =\"red\">{</font> expected rather than <font color =\"red\"> ")
                              + s + "</font>");

        return 0;
    }

    // enum definition
    Class * cl = container->define(name, "enum");

    if(cl) {
        QLOG_INFO() << "RE got definition" << cl->stereotype.operator QString();
    }
    //QLOG_INFO() << "RE got definition" << cl->uml;
    if ((cl == 0) || cl->reversedp)
    {
        QLOG_INFO() << "RE returning;";
        Lex::unread_word();
        return 0;
    }

    if (Package::scanning())
    {
        cl->filename = path;
        cl->its_namespace = Namespace::current();
        QLOG_INFO() << "RE got namespace" << cl->its_namespace.operator QString();
#ifndef REVERSE
        cl->description = comment;
#endif
        QLOG_INFO() << "RE skipping body";
        UmlOperation::skip_body(1);
        return cl;
    }

#ifdef ROUNDTRIP
    UmlClass * cl_uml = cl->uml;

    if ((cl_uml != 0) &&
        !cl_uml->is_roundtrip_expected() &&
        !cl_uml->is_created()) {
        // bypass all the class
        // note : user can't ask for update a nested class
        char c;

        while ((c = Lex::read_word_bis(TRUE, FALSE)) != '{') {
            if (c == 0) {
                Lex::premature_eof();
                throw 0;
            }
        }

        UmlOperation::skip_body(1);
        return cl;
    }

    cl->reversedp = TRUE;

    cl_uml = cl->get_uml();

    if (rndtrp)
        expectedorder.append(cl_uml);

    bool roundtrip;
    QList<UmlItem *> expected_order;

    if (cl_uml->is_created()) {
        Statistic::one_class_created_more();
        roundtrip = FALSE;
    }
    else
        roundtrip = TRUE;

#else
    QLOG_INFO() << "RE getting uml";
    UmlClass * cl_uml = cl->get_uml();

    cl->reversedp = TRUE;

    if (!name.isEmpty() && cl->already_in_bouml()) {
        cl_uml->unload();
        UmlOperation::skip_body(1);
        return 0;
    }

# ifdef REVERSE
    QLOG_INFO() << "RE getting more class";
    Statistic::one_class_more();
# endif
#endif

    int index;
    QLOG_INFO() << "RE getting enum decl";
    s = CppSettings::enumDecl();

    if (name.isEmpty() && ((index = s.find("${name}")) != -1))
        // define a C++ anonymous enum even the enum have a name in uml
        s.remove(index, 7);

#ifdef ROUNDTRIP

    if (roundtrip) {
        if (CppSettings::classStereotype(cl_uml->stereotype()) != "enum") {
            cl_uml->set_Stereotype("enum");
            cl->set_updated();
        }

        if (s.find("${description}") != -1) {
            if (nequal(cl_uml->description(), description)) {
                cl_uml->set_Description(description);
                cl->set_updated();
            }
        }
        else if (nequal(cl_uml->description(), Lex::simplify_comment(comment))) {
            // comment simplified
            cl_uml->set_Description(comment);
            cl->set_updated();
        }

        if (neq(cl_uml->cppDecl(), s)) {
            cl_uml->set_CppDecl(s);
            cl->set_updated();
        }
    }
    else {
#endif

        if (! comment.isEmpty())
            cl_uml->set_Description((s.find("${description}") != -1)
                                    ? description : Lex::simplify_comment(comment));

        cl_uml->set_CppDecl(s);
#ifdef ROUNDTRIP
    }

#endif

    for (;;) {
        if ((s = Lex::read_word()).isEmpty()) {
            Lex::premature_eof();
            return 0;
        }

        if (! Lex::identifierp(s, TRUE)) {
            if (s == "}")
                break;

            Lex::syntax_error(WrapperStr("enum item expected rather than <font color =\"red\"> ")
                              + s + "</font>");
            UmlOperation::skip_body(1);
            return 0;
        }

        UmlAttribute * it;

#ifdef ROUNDTRIP
        bool created;

        if (!roundtrip ||
            ((it = UmlAttribute::search_attr(cl_uml, s)) == 0)) {
#endif
            it = UmlBaseAttribute::create(cl_uml, s);

            if (it == 0) {
                Lex::warn("<font color =\"red\"> " + Lex::quote(s) +
                          "</font> multiply defined (class)");
                UmlOperation::skip_body(1);
                return 0;
            }

#ifdef REVERSE
# ifndef ROUNDTRIP
            Statistic::one_attribute_more();
# else

            if (roundtrip)
                cl->set_updated();

            created = TRUE;
        }
        else
            created = FALSE;

# endif
#endif

#ifdef ROUNDTRIP
            expected_order.append(it);

            if (roundtrip && !created) {
                it->set_usefull();

                if (neq(it->cppDecl(), CppSettings::enumItemDecl())) {
                    it->set_CppDecl(CppSettings::enumItemDecl());
                    cl->set_updated();
                }

                if (it->visibility() != PublicVisibility) {
                    it->set_Visibility(PublicVisibility);
                    cl->set_updated();
                }
            }
            else {
#endif
                it->set_CppDecl(CppSettings::enumItemDecl());
                it->set_Visibility(PublicVisibility);
#ifdef ROUNDTRIP
            }

#endif

            if ((s = Lex::read_word()).isEmpty()) {
                Lex::premature_eof();
                return 0;
            }

            if (s == "}")
                break;

            if (s == "=") {
                Lex::mark();
                UmlOperation::skip_expr("},");
                Lex::unread_word();	// '}' or ','
                s = Lex::region();
#ifdef ROUNDTRIP

                if (roundtrip && !created) {
                    WrapperStr v = it->defaultValue();

                    if (!v.isEmpty() && (((const char *) v)[0] == '='))
                        v = v.mid(1);

                    if (nequal(v, s)) {
                        it->set_DefaultValue(s);
                        cl->set_updated();
                    }
                }
                else
#endif
                    it->set_DefaultValue(s);

                if ((s = Lex::read_word()).isEmpty()) {
                    Lex::premature_eof();
                    return 0;
                }

                if (s == "}")
                    break;
            }

            if (s != ",") {
                Lex::error_near(s);
                UmlOperation::skip_body(1);
                return 0;
            }
        }

#ifdef REVERSE
# ifdef ROUNDTRIP

        if (roundtrip) {
            cl_uml->reorder(expected_order);

            if (cl->updated) {
                Statistic::one_class_updated_more();

                UmlCom::trace(QString("<font face=helvetica>class <i>")
                              + cl->text(0) + "</i> updated from <i>"
                              + Lex::filename() + "</i></font><br>");
            }
        }

# endif

        if (!cl->from_libp())
            cl_uml->need_artifact(cl->get_namespace());

#endif

        cl_uml->unload();

        return cl;
    }

    bool Class::reverse_typedef(ClassContainer  * container, const WrapperStr & path,
                                Q3ValueList<FormalParameterList> & tmplts
#ifdef ROUNDTRIP
                                , bool rndtrp, QList<UmlItem *> & expectedorder
#endif
                               )
    {
        Lex::mark();

        WrapperStr s;
        WrapperStr prefix;

        if ((s = Lex::read_word(/*TRUE*/)).isEmpty()) {
            if (!Package::scanning())
                Lex::premature_eof();

            return FALSE;
        }

        if ((s == "enum") || (s == "struct") || (s == "union") || (s == "class")) {
            Lex::mark();

            WrapperStr name1 = Lex::read_word();
            WrapperStr name2;

            UmlOperation::skip_body((name1 == "{") ? 1 : 0);

            if ((name1 != "{") && (Lex::region().find('{') == -1)) {
                // form like 'typedef struct X Y;'
                Lex::come_back();
                prefix = s;
                s = Lex::read_word(TRUE);
            }
            else {
                do {
                    if ((name2 = Lex::read_word()).isEmpty()) {
                        if (!Package::scanning())
                            Lex::premature_eof();

                        return FALSE;
                    }
                }
                while ((name2 != ";") && !Lex::identifierp(name2, TRUE));

                Lex::come_back();

                if (name1 == "{") {
                    if (name2 == ";") {
                        if (!Package::scanning())
                            Lex::syntax_error();

                        return FALSE;
                    }

                    name1 = name2;
                }
                else if (name2 == ";")
                    // for instance 'typedef struct name1 { ...};'
                    name2 = name1;

                Class * cl;

                if (s == "enum") {
                    if ((cl = Class::reverse_enum(container, path, name1
#ifdef ROUNDTRIP
                                                  , rndtrp, expectedorder
#endif
                                                 )) == 0)
                        return FALSE;
                }
                else if ((cl = Class::reverse(container, s, tmplts, path, name1
#ifdef ROUNDTRIP
                                              , rndtrp, expectedorder
#endif
                                             )) == 0)
                    return FALSE;

                // here '}' is the last item read

                Lex::mark();

                char c;

                while ((c = Lex::read_word_bis(TRUE, FALSE)) != ';') {
                    if (c == 0) {
                        if (!Package::scanning())
                            Lex::premature_eof();

                        return FALSE;
                    }
                }

                if (name1 != name2) {
                    Class * ty = container->define(name2, "typedef");

                    if (Package::scanning()) {
                        if (ty != 0)
                            ty->its_namespace = Namespace::current();

                        return (ty != 0);
                    }

                    UmlClass * ty_uml;

                    if ((ty == 0) || ((ty_uml = ty->get_uml()) == 0))
                        return FALSE;

                    ty->reversedp = TRUE;
                    ty->its_namespace = Namespace::current();

#ifdef ROUNDTRIP

                    if (rndtrp)
                        expectedorder.append(ty_uml);

                    bool roundtrip;

                    if (ty_uml->is_created()) {
                        roundtrip = FALSE;
                        Statistic::one_class_created_more();
                    }
                    else
                        roundtrip = TRUE;

#else

                    if (ty->already_in_bouml()) {
                        ty_uml->unload();
                        return 0;
                    }

# ifdef REVERSE
                    Statistic::one_class_more();
# endif
#endif

                    WrapperStr decl = CppSettings::typedefDecl();
                    int index = decl.find("${name}");

                    if (index == -1) {
                        decl = "${comment}typedef ${type} ${name};";
                        index = decl.find("${name}");
                    }

                    s = Lex::region();
                    s.truncate(s.length() - 1);	// remove ';'

                    if (s[0] == ' ')
                        s.remove(0, 1);

                    int index2 = s.find(name2);

                    if (index2 == -1) {
                        // severe syntax error
                        Lex::come_back();
                        return FALSE;
                    }

                    s.replace(s.find(name2), name2.length(), "${name}");
                    decl.replace(index, 7, s);

                    UmlTypeSpec t;

                    if ((t.type = cl->get_uml()) == 0)
                        return FALSE;

#ifdef ROUNDTRIP

                    if (roundtrip) {
                        if (neq(ty_uml->cppDecl(), decl)) {
                            ty_uml->set_CppDecl(decl);
                            ty->set_updated();
                        }

                        if (!ty_uml->baseType().equal(t)) {
                            ty_uml->set_BaseType(t);
                            ty->set_updated();
                        }
                    }
                    else {
#endif
                        ty_uml->set_CppDecl(decl);
                        ty_uml->set_BaseType(t);
#ifdef ROUNDTRIP
                    }

#endif

#ifdef REVERSE

                    if (!ty->from_libp())
                        ty_uml->need_artifact(ty->get_namespace());

#endif

                    ty_uml->unload();
                }

                return TRUE;
            }
        }

        WrapperStr comment = Lex::get_comments();
        WrapperStr description = Lex::get_description();
        //QValueList<WrapperStr> actuals;
        //bool tmpl = FALSE;
        WrapperStr type;
        UmlTypeSpec base_type;
        WrapperStr typeform = "${type}";

        for (;;) {
            if ((s == "unsigned") || (s == "signed") ||
                (s == "void") || (s == "bool")) {
                type = s;
                Lex::mark();
            }
            else if ((s == "char") || (s == "short") || (s == "int") ||
                     (s == "long") || (s == "float") || (s == "double")) {
                type = (type.isEmpty()) ? s : type + " " + s;
                Lex::mark();
            }
            else if (s == ";") {
                if (!Package::scanning())
                    Lex::error_near(";");

                return FALSE;
            }
            else if (Lex::identifierp(s, FALSE) &&
                     (s != "typename") && (s != "Q_TYPENAME") &&
                     (s != "const") && (s != "mutable") && (s != "volatile")) {
                if (type.isEmpty()) {
                    type = s = Lex::complete_template_type(s);
                    Lex::mark();

                    if (!Package::scanning())
                        container->compute_type(s, base_type, typeform, TRUE);
                }
                else
                    break;
            }
            else if (type.isEmpty()) {
                if (prefix.isEmpty())
                    prefix = Lex::region();
                else
                    prefix = prefix + " " + Lex::region();
            }

            if ((s = Lex::read_word(/*!tmpl*/)).isEmpty()) {
                if (!Package::scanning())
                    Lex::premature_eof();

                return FALSE;
            }
        }

        // here 'type' is the base type, 's' is the typedef name (I hope !)
        // prefix if the code before 'type' and Lex::region() the code after
        // 'type' up to 's' (including 's', may be except template actuals)

#ifdef DEBUG_DOUML
        QLOG_INFO() << "typedef " << s << "...\n";
#endif
        Class * ty = container->define(s, "typedef");

        if (Package::scanning()) {
            UmlOperation::skip_body();

            if (ty != 0)
                ty->its_namespace = Namespace::current();

            return (ty != 0);
        }

        WrapperStr intermediate = Lex::region();
        UmlClass * ty_uml;

        if ((ty == 0) || ((ty_uml = ty->get_uml()) == 0)) {
#ifdef DEBUG_DOUML
            QLOG_INFO() << ((ty == 0) ? "typedef ty\n" : "typedef ty_uml\n");
#endif
            UmlOperation::skip_body();
            return FALSE;
        }

        ty->reversedp = TRUE;
        ty->its_namespace = Namespace::current();

#ifdef ROUNDTRIP

        if (rndtrp)
            expectedorder.append(ty_uml);

        bool roundtrip;

        if (ty_uml->is_created()) {
            roundtrip = FALSE;
            Statistic::one_class_created_more();
        }
        else
            roundtrip = TRUE;

#else

        if (ty->already_in_bouml()) {
            UmlOperation::skip_body();
            ty_uml->unload();
            return 0;
        }

# ifdef REVERSE
        Statistic::one_class_more();
# endif
#endif

        if (base_type.type == 0)
            // for builtin types
            base_type.explicit_type = type;
        else {
#ifdef ROUNDTRIP

            if (roundtrip) {
                UmlRelation * dep = 0;
                const Q3PtrVector<UmlItem> & ch = ty_uml->children();
                UmlItem ** v = ch.data();
                UmlItem ** const vsup = v + ch.size();

                for (; v != vsup; v += 1) {
                    if (((*v)->kind() == aRelation) &&
                        (((UmlRelation *) *v)->relationKind() == aDependency) &&
                        (((UmlRelation *) *v)->roleType() == base_type.type)) {
                        dep = (UmlRelation *) *v;
                        dep->set_usefull();
                    }
                }

                if (dep == 0)
                    (void) UmlBaseRelation::create(aDependency, ty_uml, base_type.type);
            }
            else
#endif
                UmlBaseRelation::create(aDependency, ty_uml, base_type.type);
        }

#ifdef ROUNDTRIP

        if (roundtrip) {
            if (!ty_uml->baseType().equal(base_type)) {
                ty_uml->set_BaseType(base_type);
                ty->set_updated();
            }
        }
        else
#endif
            ty_uml->set_BaseType(base_type);

        /*
        if (tmpl && (base_type.type != 0)) {
          unsigned n = base_type.type->Formals().count();
          unsigned index = 0;

          for (QStringList::Iterator it = actuals.begin();
         it != actuals.end();
         ++it) {
            if (index == n) {
        Lex::warn("too many actuals for " + Lex::quote(base_type.type->Name()));
        break;
            }


            WrapperStr typeform;
            UmlTypeSpec t;

            container->compute_type(WrapperStr(*it), t, typeform);
            ty_uml->replaceActual(index++, t);
          }
        }*/

        intermediate.truncate(intermediate.length() - s.length()); // remove s

        if (intermediate[0] == ' ')
            // normally it is the case
            intermediate.remove(0, 1);

        char c;

        Lex::mark();

        while ((c = Lex::read_word_bis(TRUE, FALSE)) != ';') {
            if (c == 0) {
                Lex::premature_eof();
                return FALSE;
            }
        }

        s = Lex::region();
        s.truncate(s.length() - 1);	// remove ';', 's' is the code after the typedef name

        WrapperStr decl = CppSettings::typedefDecl();
        int index;

        if ((decl.find("${name}") == -1) ||
            ((index = decl.find("${type}")) == -1)) {
            decl = "${comment}typedef ${type} ${name};";
            index = decl.find("${type}");
        }

        if (!prefix.isEmpty()) {
            decl.insert(index, (const char *)(prefix + " "));
            index += prefix.length() + 1;
        }

        if (!intermediate.isEmpty())
            decl.insert(index + 7, (const char *)intermediate);

        if ((base_type.type != 0) && (typeform != "${type}"))
            decl.replace(index, 7, typeform);

        if (!s.isEmpty())
            decl.insert(decl.find("${name}") + 7, (const char *) s);	// find cannot return -1

#ifdef ROUNDTRIP

        if (roundtrip) {
            if (neq(ty_uml->cppDecl(), decl)) {
                ty_uml->set_CppDecl(decl);
                ty->set_updated();
            }

            if (decl.find("${description}") != -1) {
                if (nequal(ty_uml->description(), description)) {
                    ty_uml->set_Description(description);
                    ty->set_updated();
                }
            }
            else if (nequal(ty_uml->description(), Lex::simplify_comment(comment))) {
                ty_uml->set_Description(comment); // comment was set
                ty->set_updated();
            }
        }
        else {
#endif
            ty_uml->set_CppDecl(decl);

            if (! comment.isEmpty())
                ty_uml->set_Description((decl.find("${description}") != -1)
                                        ? description : Lex::simplify_comment(comment));

#ifdef ROUNDTRIP
        }

#endif

        Lex::finish_line();
        Lex::clear_comments();

#ifdef REVERSE

        if (!ty->from_libp())
            ty_uml->need_artifact(ty->get_namespace());

#endif

        ty_uml->unload();

        return TRUE;
    }

//

    bool Class::set_stereotype(const WrapperStr & st)
    {
        if (!st.isEmpty()) {
            if (stereotype_declared) {
                if (st != ((stereotype.isEmpty()) ? WrapperStr("class") : stereotype)) {
                    UmlCom::trace(WrapperStr("<font face=helvetica><b><i>") +
                                  WrapperStr(text(0).toAscii().constData()) +
                                  "</i> have several stereotypes</b></font><br><hr><br>");
                    return FALSE;
                }
            }
            else if (st != "class")
                stereotype = st;
        }

        return TRUE;
    }

    Class * Class::declare_if_needed(const WrapperStr & name,
                                     WrapperStr stereotype)
    {
        Class * result =
            ClassContainer::declare_if_needed(name, stereotype, formals,
                                              declared, defined);

        if (result != 0)
            declare_if_needed(name, result);

        return result;
    }

    void Class::declare_if_needed(WrapperStr name, Class * cl)
    {
        BrowserNode * p = (BrowserNode *) parent();

        ((p->isa_package()) ? ((ClassContainer *)((Package *) p))
         : ((ClassContainer *)((Class *) p)))
        ->declare_if_needed(WrapperStr(text(0).toAscii().constData()) + "::" + name, cl);
    }

    Class * Class::define(const WrapperStr & name, WrapperStr stereotype)
    {
        Class * result =
            ClassContainer::define(name, stereotype, declared, defined);

        if (result != 0)
            define(name, result);

        return result;
    }

    void Class::define(WrapperStr name, Class * cl)
    {
        if (! name.isEmpty()) {
            BrowserNode * p = (BrowserNode *) parent();

            ((p->isa_package()) ? ((ClassContainer *)((Package *) p))
             : ((ClassContainer *)((Class *) p)))
            ->define(WrapperStr(text(0).toAscii().constData()) + "::" + name, cl);
        }
    }

#ifdef ROUNDTRIP
    Class * Class::upload_define(UmlClass * ucl)
    {
        Class * cl = new Class(this, ucl);

        defined.insert(ucl->name(), cl);
        define(ucl->name(), cl);

        return cl;
    }

    bool Class::is_roundtrip_expected() const
    {
        return ((uml != 0) && uml->is_roundtrip_expected());
    }
#endif

    bool Class::find_type(WrapperStr type, UmlTypeSpec & typespec)
    {
        if (! formals.isEmpty()) {
            FormalParameterList::ConstIterator it;

            for (it = formals.begin(); it != formals.end(); ++it) {
                if ((*it).name() == type) {
                    typespec.type = 0;
                    typespec.explicit_type = type;
                    return TRUE;
                }
            }
        }

        return ((ClassContainer::find_type(type, typespec, defined)) ||
                ((BrowserNode *) parent())->find_type(type, typespec));
    }

    Class * Class::new_class(const WrapperStr & name,
                             const WrapperStr & stereotype,
                             bool declaration)
    {
        Class * cl = new Class(this, name, stereotype);

        if (! name.isEmpty())
            ((declaration) ? declared : defined).insert(name, cl);

        return cl;
    }

    UmlClass * Class::get_uml()
    {
#ifndef ROUNDTRIP

        if (uml != 0)
            return uml;

#else
        bool roundtrip;

        if (uml != 0) {
            if (!uml->is_roundtrip_expected() ||
                !uml->is_useless())
                return uml;

            uml->set_usefull();
            roundtrip = TRUE;
        }
        else {
            roundtrip = FALSE;
#endif

        UmlItem * p = (((BrowserNode *) parent())->isa_package())
                      ? (UmlItem *)((Package *) parent())->get_uml()->get_classview(get_namespace())
                      : (UmlItem *)((Class *) parent())->get_uml();
        WrapperStr str = WrapperStr(text(0).toAscii().constData());
        bool anonymous = str.isEmpty();

        for (;;) {
            if (anonymous) {
                static unsigned n = 0;

                QString temp = "anonymous%1";
                temp=temp.arg(QString::number(++n));
                str+=temp;
            }

            uml = UmlBaseClass::create(p, str);

            if (uml == 0) {
                // probably already exist
                Q3PtrVector<UmlItem> ch = p->children();
                UmlItem * x;

                for (unsigned chindex = 0; chindex != ch.size(); chindex += 1) {
                    if (((x = ch[chindex])->kind() == aClass) && (x->name() == str)) {
                        uml = (UmlClass *) x;

                        if (anonymous)
                            break;

                        FormalParameterList xformals = uml->formals();

                        if (formals.count() == xformals.count()) {
                            FormalParameterList::ConstIterator end = formals.end();
                            FormalParameterList::ConstIterator it;
                            FormalParameterList::ConstIterator xit;

                            for (it = formals.begin(), xit = xformals.begin();
                                 it != end;
                                 ++it, ++xit) {
                                if (((*it).type() != (*xit).type()) ||
                                    ((*it).name() != (*xit).name()) ||
                                    ((*it).defaultValue().toString() !=
                                     (*xit).defaultValue().toString())) {
                                    // uncompatible
                                    uml = 0;
                                    break;
                                }
                            }
                        }
                        else
                            // uncompatible
                            uml = 0;

                        break;
                    }
                }

                if (uml == 0) {
#ifdef REVERSE
                    UmlCom::message("");
                    CppCatWindow::trace(QString("<font face=helvetica><b>cannot create class <i>")
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
                else if (! anonymous)
                    break;
            }
            else {
                unsigned rank;
                FormalParameterList::ConstIterator formals_end = formals.end();
                FormalParameterList::ConstIterator it;

#ifdef ROUNDTRIP

                if (roundtrip) {
                    Q3ValueList<UmlFormalParameter> fs = uml->formals();
                    Q3ValueList<UmlFormalParameter>::ConstIterator it2;

                    for (rank = 0, it = formals.begin(), it2 = fs.begin();
                         (it != formals_end) && (it2 != fs.end());
                         it++, it2++, rank += 1) {
                        const UmlFormalParameter & p1 = *it;
                        const UmlFormalParameter & p2 = *it2;

                        if (neq(p1.name(), p2.name()) ||
                            neq(p1.type(), p2.type()) ||
                            !p1.defaultValue().equal(p2.defaultValue()) ||
                            !p1.extend().equal(p2.extend())) {
                            uml->replaceFormal(rank, p1);
                            set_updated();
                        }
                    }

                    if (it != formals_end) {
                        // have missing formals
                        set_updated();

                        do {
                            uml->addFormal(rank, *it);
                            it++;
                            rank += 1;
                        }
                        while (it != formals_end);
                    }
                    else if (it2 != fs.end()) {
                        // have extra formals
                        set_updated();

                        do {
                            uml->removeFormal(rank);
                            it2++;
                            rank += 1;
                        }
                        while (it2 != fs.end());
                    }
                }
                else
#endif
                    for (rank = 0, it = formals.begin(); it != formals_end; rank += 1, ++it)
                        uml->addFormal(rank, *it);

                break;
            }
        }

#ifdef ROUNDTRIP
        uml->set_created();
    }

#endif

#ifdef ROUNDTRIP

    if (roundtrip) {
        if (stereotype.isEmpty()) {
            if (!uml->stereotype().isEmpty()) {
                WrapperStr cppst = CppSettings::classStereotype(uml->stereotype());

                if ((cppst == "union") || (cppst == "struct") || (cppst == "enum")) {
                    uml->set_Stereotype("");
                    set_updated();
                }
            }
        }
        else if (CppSettings::classStereotype(uml->stereotype()) != stereotype) {
            uml->set_Stereotype(stereotype);
            set_updated();
        }
    }
    else
#endif
        if (!stereotype.isEmpty())
            uml->set_Stereotype(stereotype);

    return uml;
}

void Class::set_uml(UmlClass * c)
{
    uml = c;
}

#ifndef ROUNDTRIP
bool Class::already_in_bouml()
{
    Q3PtrVector<UmlItem> ch = get_uml()->children();

    for (unsigned index = 0; index != ch.size(); index += 1)
        if (ch[index]->kind() != aClass)
            return TRUE;

    return FALSE;
}
#endif

bool Class::isa_package() const
{
    return FALSE;
}

QString Class::get_path() const
{
    return filename;
}
