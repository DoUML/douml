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
#include "misc/mystr.h"
#include <Q3ValueList>
//Added by qt3to4:
#include <Q3PtrList>

using namespace std;
#endif

#include "UmlClass.h"
#include "UmlPackage.h"
#include "Package.h"
#include "ClassContainer.h"
#include "Lex.h"
#include "UmlRelation.h"
#include "UmlOperation.h"
#include "CppSettings.h"
#include "UmlArtifact.h"
#include "UmlDeploymentView.h"
#include "Statistic.h"
#include "UmlCom.h"
#include "Logging/QsLog.h"
#ifdef REVERSE
#include "CppCatWindow.h"
# ifdef ROUNDTRIP
#include "UmlExtraClassMember.h"
#include "Class.h"
# endif
#endif

// contains the classes whose definition in currently read
// may be more than one because of the nested classes
QList<UmlClass *> UmlClass::UnderConstruction;

// used (by using) classes list
Q3Dict<UmlClass> UmlClass::Usings;

// to lost usings defined under a namespace/class block
Q3ValueList<Q3Dict<UmlClass> > UmlClass::UsingScope;

UmlClass::UmlClass(void * id, const WrapperStr & n)
    : UmlBaseClass(id, n)
#ifdef ROUNDTRIP
    , created(FALSE), the_class(0)
#endif
{}

bool UmlClass::manage_inherit(ClassContainer * container,
                              const Q3ValueList<FormalParameterList> & tmplts
#ifdef REVERSE
                              , bool libp
# ifdef ROUNDTRIP
                              , bool roundtrip, QList<UmlItem *> & expected_order
                              , bool container_roundtrip, QList<UmlItem *> & container_expected_order
# endif
#endif
                             )
{
#ifdef DEBUG_DOUML
    QLOG_INFO() << name() << "->manage_inherit()\n";
#endif

    WrapperStr s = Lex::read_word(TRUE);

    while (s != "{") {
#ifdef DEBUG_DOUML
        QLOG_INFO() << "Class::manage_inherit, visibility : " << s << '\n';
#endif

        bool is_virtual;

        if (s == "virtual") {
            is_virtual = TRUE;
            s = Lex::read_word(TRUE);
        }
        else
            is_virtual = FALSE;

        aVisibility v;

        if (s == "public") {
            v = PublicVisibility;
            s = Lex::read_word(TRUE);
        }
        else if (s == "protected") {
            v = ProtectedVisibility;
            s = Lex::read_word(TRUE);
        }
        else if (s == "private") {
            v = PrivateVisibility;
            s = Lex::read_word(TRUE);
        }
        else
            v = PrivateVisibility;

        if (s.isEmpty()) {
            Lex::premature_eof();
            return FALSE;
        }

#ifdef DEBUG_DOUML
        QLOG_INFO() << "UmlClass::manage_inherit, mother : " << s << '\n';
#endif

        WrapperStr mother_name = s;
        UmlTypeSpec mother;
        WrapperStr typeform;
        UmlRelation * rel = 0;
#ifdef ROUNDTRIP
        bool is_new = TRUE;
#endif

        container->compute_type(s, mother, typeform);

        s = Lex::read_word();

        if (s == "<") {
            Lex::mark();

            // goes up to the corresponding '>'
            WrapperStr after_gt;

            Lex::finish_template(after_gt);

            s = Lex::read_word(TRUE);

            if (*s == ':') {
                // inherits 'T<...>::...'
                // don't try to solve, use a typedef named 'Type_<n>' based on T<...>::...
                // creating this typedef if it doesn't yet exist

                mother_name += "<" + Lex::region();
                mother.type = 0;  // made below
                s = Lex::read_word();
            }
            else if (mother.type == 0) {
                mother_name += "<" + after_gt;
            }
            else {
                // inherits T<...>
#ifdef ROUNDTRIP
                is_new = !roundtrip || ((rel = search_for_inherit(mother.type)) == 0);
#endif
                mother_name += "<" + after_gt;
                Lex::come_back();

                // must add inheritance before setting actuals
                if (
#ifdef ROUNDTRIP
                    is_new &&
#endif
                    ((rel = UmlBaseRelation::create(aRealization, this, mother.type)) == 0)) {
                    Lex::warn("cannot inherit <font color =\"red\">" +
                              Lex::quote(mother_name) + " </font>");
#ifdef DEBUG_DOUML
                    QLOG_INFO() << "cannot create <|---\n";
#endif
                    return FALSE;
                }
                else if (!get_actuals(mother.type, container, tmplts
#ifdef ROUNDTRIP
                                      , !is_new
#endif
                                     ))
                    return FALSE;

#ifdef ROUNDTRIP

                if (! is_new) {
                    if (neq(rel->stereotype(), "bind")) {
                        rel->set_Stereotype("bind");
                        the_class->set_updated();
                    }
                }
                else
#endif
                    rel->set_Stereotype("bind");

                s = Lex::read_word();
            }
        }

        if (mother.type == 0) {
            mother.type = auxilarily_typedef(mother_name
#ifdef REVERSE
                                             , libp
# ifdef ROUNDTRIP
                                             , container_roundtrip
                                             , container_expected_order
# endif
#endif
                                            );

            if (mother.type == 0)
                return FALSE;
        }

#ifdef ROUNDTRIP

        if (rel == 0)
            is_new = !roundtrip || ((rel = search_for_inherit(mother.type)) == 0);

#endif

        if ((rel == 0) &&
            ((rel = UmlBaseRelation::create(aGeneralisation, this, mother.type)) == 0)) {
            Lex::warn("cannot inherit <font color =\"red\">" +
                      Lex::quote(mother_name) + " </font>");
#ifdef DEBUG_DOUML
            QLOG_INFO() << "cannot create <|---\n";
#endif
            return FALSE;
        }

#ifdef ROUNDTRIP
        expected_order.append(rel);

        if (!is_new) {
            rel->set_usefull();

            if (neq(rel->cppDecl(), "${type}")) {
                rel->set_CppDecl("${type}");
                the_class->set_updated();
            }

            if (rel->visibility() != v) {
                rel->set_Visibility(v);
                the_class->set_updated();
            }

            if (is_virtual != rel->cppVirtualInheritance()) {
                rel->set_CppVirtualInheritance(is_virtual);
                the_class->set_updated();
            }
        }
        else {
#elif defined(REVERSE)
        Statistic::one_relation_more();
#endif
            rel->set_CppDecl("${type}");
            rel->set_Visibility(v);

            if (is_virtual)
                rel->set_CppVirtualInheritance(TRUE);

#ifdef ROUNDTRIP
        }

#endif

        if (s == ",")
            s = Lex::read_word();
    }

    Lex::unread_word();	// '{'

    return TRUE;
}

UmlClass * UmlClass::auxilarily_typedef(const WrapperStr & base
#ifdef REVERSE
                                        , bool libp
# ifdef ROUNDTRIP
                                        , bool container_roundtrip
                                        , QList<UmlItem *> & container_expected_order
# endif
#endif
                                       )
{
    WrapperStr typedef_decl = CppSettings::typedefDecl();
    const Q3PtrVector<UmlItem> & children = parent()->children();
    unsigned n = children.count();
    unsigned index;

    // a typedef with the right definition already exist ?
    for (index = 0; index != n; index += 1) {
        if (children[index]->kind() == aClass) {
            UmlClass * cl = (UmlClass *) children[index];

            if ((cl->stereotype() == "typedef") &&
                (cl->cppDecl() == typedef_decl) &&
                (cl->baseType().explicit_type == base)) {
#ifdef ROUNDTRIP
                cl->set_usefull();

                if (container_roundtrip)
                    container_expected_order.append(cl);

#endif
                return cl;
            }
        }
    }

    // must create typedef with a new name
    for (;;) {
        static unsigned nty;

        QString s("typedef%1");
        s=s.arg(QString::number(++nty));


        for (index = 0; index != n; index += 1)
            if (children[index]->name() == s)
                break;

        if (index == n) {
            UmlClass * cl = UmlClass::create(parent(), s);

            if (cl == 0) {
#ifdef REVERSE
                UmlCom::message("");
                CppCatWindow::trace(WrapperStr("<font face=helvetica><b>cannot create class <i>")
                                    + s + "</i> under <i>"
                                    + parent()->name() + "</b></font><br>");
                throw 0;
#else
                QMessageBox::critical(0, "Fatal Error",
                                      WrapperStr("<font face=helvetica><b>cannot create class <i>")
                                      + s + "</i> under <i>"
                                      + parent()->name() + "</b></font><br>");
                QApplication::exit(1);
#endif
            }

            UmlTypeSpec typespec;

            typespec.explicit_type = base;
            cl->set_Stereotype("typedef");
            cl->set_BaseType(typespec);
            cl->set_CppDecl(typedef_decl);
#ifdef REVERSE

            if (!libp)
                cl->need_artifact(Namespace::current());

# ifdef ROUNDTRIP

            if (container_roundtrip)
                container_expected_order.append(cl);

# endif
#endif

            return cl;
        }
    }
}

bool UmlClass::get_actuals(UmlClass * mother, ClassContainer * container,
                           const Q3ValueList<FormalParameterList> & tmplts
#ifdef ROUNDTRIP
                           , bool roundtrip
#endif
                          )
{
    // < read
    const Q3ValueList<UmlActualParameter> actuals = this->actuals();
    Q3ValueList<UmlActualParameter>::ConstIterator it;
    unsigned rank;

    for (it = actuals.begin(), rank = 0; (*it).superClass() != mother; ++it, rank += 1)
        if (it == actuals.end())
            break;

    char c;

    while ((c = Lex::read_word_bis(TRUE, FALSE)) != 0) {
        if (c == '>')
            return TRUE;

        if (c == ',') {
            // value not given
            ++it;
            rank += 1;
        }
        else {
            if ((it == actuals.end()) || ((*it).superClass() != mother)) {
                Lex::warn("too many actuals for <font color =\"red\">" +
                          Lex::quote(mother->name()) + "</font>");
                return FALSE;
            }

            Lex::unread_word();

            WrapperStr s = Lex::read_list_elt();

            UmlTypeSpec typespec;
            WrapperStr typeform = "${type}";

            container->compute_type(s, typespec, typeform, FALSE, tmplts);

            if (typespec.explicit_type == "<complex type>")
                typespec.explicit_type = typeform;

#ifdef ROUNDTRIP

            if (roundtrip) {
                if (!(*it).value().equal(typespec)) {
                    the_class->set_updated();
                    replaceActual(rank, typespec);
                }
            }
            else
#endif
                replaceActual(rank, typespec);

            ++it;
            rank += 1;

            c = Lex::read_word_bis(TRUE, FALSE);

            if (c == 0)
                break;
            else if (c != ',')
                Lex::unread_word();
        }
    }

    Lex::premature_eof();
    return FALSE;
}

void UmlClass::set_under_construction(bool y, bool rec)
{
    if (y) {
        if (rec) {
            UmlItem * it = this;

            do {
                UnderConstruction.append((UmlClass *) it);
            }
            while ((it = it->parent())->kind() == aClass);
        }
        else
            UnderConstruction.append(this);
    }
    else if (rec)
        UnderConstruction.clear();
    else
        UnderConstruction.removeOne(this);
}

bool UmlClass::inside_its_definition()
{
    return UnderConstruction.contains(this);
}

bool UmlClass::is_itself(WrapperStr t)
{
    // class is a template class and t is x<...> where x is the class,
    // t is normalized
    // return true if t is the class with its formals
    int index = t.find('<');

    t = t.mid(index + 1, t.length() - index - 2);

    Q3ValueList<UmlFormalParameter> l = formals();
    Q3ValueList<UmlFormalParameter>::ConstIterator it = l.begin();
    WrapperStr t2 = (*it).name();

    while ((++it) != l.end())
        t2 += ',' + (*it).name();

    return (t == t2);
}

void UmlClass::restore_using_scope()
{
    Usings = UsingScope.first();
    UsingScope.remove(UsingScope.begin());
}

#ifdef REVERSE

void UmlClass::need_artifact(const WrapperStr & nmsp)
{
    if (parent()->kind() == aClassView) {
        UmlArtifact * cp;

        if ((cp = associatedArtifact()) == 0) {
            // search artifact
#ifdef ROUNDTRIP
            if ((cp = Package::get_artifact()) != 0) {
                cp->addAssociatedClass(this);
                return;
            }

#endif

            WrapperStr name = Package::get_fname();
            int index;

            if (name.isEmpty()) {
                // defined in a source rather than a header
                // will create it own artifact
                name = this->name();
                index = name.find('<');

                if (index != -1)
                    name = name.left(index);
            }

            UmlDeploymentView * cpv =
                ((UmlPackage *) parent()->parent())->get_deploymentview(nmsp);
            const Q3PtrVector<UmlItem> & children = cpv->children();
            int n = (int) children.count();

            for (index = 0; index != n; index += 1) {
                if ((children[index]->name() == name) &&
                    (children[index]->kind() == anArtifact)) {
                    cp = (UmlArtifact *) children[index];
                    break;
                }
            }

            if (cp == 0) {
                // does not exist, create associated artifact
                if ((cp = UmlBaseArtifact::create(cpv, name))
                    == 0) {
#ifdef REVERSE
                    UmlCom::trace(WrapperStr("<font face=helvetica><b>cannot create artifact <i>")
                                  + Lex::quote(name) + "</i> under <i>"
                                  + Lex::quote(cpv->name()) +
                                  "</b></font><br>");
                    UmlCom::message("");
                    throw 0;
#else
                    QMessageBox::critical(0, "Fatal Error",
                                          WrapperStr("<font face=helvetica><b>cannot create artifact <i>")
                                          + Lex::quote(name) + "</i> under <i>"
                                          + Lex::quote(cpv->Name()) + "</b></font><br>");
                    QApplication::exit(1);
#endif
                }

                cp->set_Stereotype("source");
                cp->set_CppHeader(CppSettings::headerContent());
            }

            cp->addAssociatedClass(this);
        }

        // source needed ?

        if (need_source() && cp->cppSource().isEmpty())
            cp->set_CppSource(CppSettings::sourceContent());
    }
}

bool UmlClass::need_source()
{
    const WrapperStr & stereotype = this->stereotype();

    if ((stereotype != "enum") && (stereotype != "typedef")) {
        Q3PtrVector<UmlItem> children = this->children();
        unsigned u = children.size();

        while (u--)
            if ((children[u]->kind() != aNcRelation) &&
                (((UmlClassItem *) children[u])->need_source()))
                return TRUE;
    }

    return FALSE;
}

# ifdef ROUNDTRIP

void UmlClass::upload(ClassContainer * cnt)
{
    UmlArtifact * art = associatedArtifact();
    WrapperStr na;

    if (art != 0) {
        na = ((UmlPackage *) art->parent()->parent())->cppNamespace();

        if (! na.isEmpty())
            Namespace::set(na);
    }

    the_class = cnt->upload_define(this);

    const Q3PtrVector<UmlItem> & ch = UmlItem::children();
    UmlItem ** v = ch.data();
    UmlItem ** const vsup = v + ch.size();

    for (; v != vsup; v += 1)
        (*v)->upload(the_class);

    if (! na.isEmpty())
        Namespace::unset();
}

bool UmlClass::set_roundtrip_expected()
{
    if (is_roundtrip_expected() ||
        ((parent()->kind() != aClass) &&
         ((associatedArtifact() == 0) ||
          !associatedArtifact()->set_roundtrip_expected_for_class())))
        return TRUE;

    const Q3PtrVector<UmlItem> & ch = UmlItem::children();
    UmlClassItem ** v = (UmlClassItem **) ch.data();
    UmlClassItem ** const vsup = v + ch.size();
    bool result = UmlClassItem::set_roundtrip_expected();

    for (; v != vsup; v += 1)
        result &= (*v)->set_roundtrip_expected();

    return result;

}

void UmlClass::mark_useless(QList<UmlItem *> & l)
{
    UmlClassItem::mark_useless(l);

    Q3PtrVector<UmlItem> ch = UmlItem::children();
    UmlClassItem ** v = (UmlClassItem **) ch.data();
    UmlClassItem ** const vsup = v + ch.size();

    for (; v != vsup; v += 1)
        (*v)->mark_useless(l);
}

void UmlClass::scan_it(int & n)
{
    if (is_roundtrip_expected())
        associatedArtifact()->scan_it(n);
}

void UmlClass::send_it(int n)
{
    if (is_roundtrip_expected())
        associatedArtifact()->send_it(n);
}

UmlItem * UmlClass::search_for_att_rel(const WrapperStr & name)
{
    const Q3PtrVector<UmlItem> & ch = UmlItem::children();
    UmlItem ** v = ch.data();
    UmlItem ** const vsup = v + ch.size();

    for (; v != vsup; v += 1) {
        switch ((*v)->kind()) {
        case anAttribute:
            if ((*v)->name() == name)
                return *v;

            break;

        case aRelation:
            if (((UmlRelation *) *v)->roleName() == name)
                return *v;

            break;

        default:
            break;
        }
    }

    return 0;
}

UmlExtraClassMember *
UmlClass::search_for_extra(const WrapperStr & name, const WrapperStr & decl)
{
    const Q3PtrVector<UmlItem> & ch = UmlItem::children();
    UmlItem ** v = ch.data();
    UmlItem ** const vsup = v + ch.size();

    for (; v != vsup; v += 1) {
        if (((*v)->kind() == anExtraClassMember) &&
            ((*v)->name() == name) &&
            !neq(((UmlExtraClassMember *) *v)->cppDecl(), decl))
            return (UmlExtraClassMember *) *v;
    }

    return 0;
}

UmlRelation * UmlClass::search_for_inherit(UmlClass * mother)
{
    const Q3PtrVector<UmlItem> & ch = UmlItem::children();
    UmlItem ** v = ch.data();
    UmlItem ** const vsup = v + ch.size();

    for (; v != vsup; v += 1) {
        if (((*v)->kind() == aRelation) &&
            (((UmlRelation *) *v)->roleType() == mother)) {
            switch (((UmlRelation *) *v)->relationKind()) {
            case aGeneralisation:
            case aRealization:
                return (UmlRelation *) *v;

            default:
                break;
            }
        }
    }

    return 0;
}

void UmlClass::reorder(QList<UmlItem *> & expected_order)
{
    if (expected_order.isEmpty())
        return;

    Q3PtrVector<UmlItem> ch = UmlItem::children(); // copy
    UmlItem ** v = ch.data();

    unload(); // to not reload children each time

    //bool updated = FALSE;
    UmlItem * expected_previous = 0;

    foreach (UmlItem * expected, expected_order) {
        if (*v != expected) {
            //updated = TRUE;
            expected->moveAfter(expected_previous);

            UmlItem * x1 = expected;

            do {
                UmlItem * x2 = *v;

                *v = x1;
                x1 = x2;
            }
            while (x1 != expected);
        }

        expected_previous = expected;
        v += 1;
    }

    //if (updated)
    //  get_class()->set_updated();
}

# endif

#endif
