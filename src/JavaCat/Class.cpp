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

#include <qdir.h>
#include <qdatastream.h>
#ifndef REVERSE
#include <qapplication.h>
#include <q3popupmenu.h>
#include <qcursor.h>
#include <qpainter.h>
#include <qmessagebox.h>
//Added by qt3to4:
#include <Q3ValueList>
#include "misc/mystr.h"
#include <QPixmap>
//Added by qt3to4:
#include <Q3PtrList>
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
#include "JavaSettings.h"
#include "JavaCatWindow.h"
#ifdef REVERSE
#include "UmlArtifact.h"
#include "Statistic.h"
#include "UmlExtraClassMember.h"

UmlArtifact * Class::CurrentArtifact;
#else
#include "BrowserView.h"
#include "Pixmap.h"
#include "ShowFileDialog.h"

HistoricList Class::Historic;

void HistoricList::appendClass(Class *klass)
{
    while (current + 1 != Historic.size())
        Historic.removeLast();

    Historic.append(klass);
    ++current;
}

bool HistoricList::hasPrev() const
{
    return current != 0;
}

Class *HistoricList::prev()
{
    --current;
    return Historic[current];
}

bool HistoricList::hasNext() const
{
    return current + 1 < Historic.size();
}

Class *HistoricList::next()
{
    ++current;
    return Historic[current];
}

#endif

Class::Class(BrowserNode * p, const char * n, char st)
    : BrowserNode(p, n),
      uml(0), stereotype(st), abstractp(FALSE), reversedp(FALSE),
#ifdef REVERSE
      from_lib(FALSE)
# ifdef ROUNDTRIP
      , updated(FALSE)
# endif
#else
      description_updatedp(FALSE)
#endif
{
    // in case the package is java[.*] the (sub)class is exported
    while (!p->isa_package())
        p = (BrowserNode *) p->parent();

    ((Package *) p)->new_class(this);
}

#ifdef ROUNDTRIP
Class::Class(BrowserNode * p, UmlClass * ucl)
    : BrowserNode(p, ucl->name()),
      uml(ucl), abstractp(FALSE), reversedp(FALSE), from_lib(FALSE), updated(FALSE)
{
    WrapperStr st = ucl->stereotype();

    if (!st.isEmpty())
        st = JavaSettings::classStereotype(st);

    stereotype = *((const char *) st);

    // in case the package is java[.*] the (sub)class is exported
    while (!p->isa_package())
        p = (BrowserNode *) p->parent();

    ((Package *) p)->new_class(this);
}
#endif

UmlClass * Class::get_uml()
{
#ifndef ROUNDTRIP

    if (uml != 0)
        return uml;

#else
    bool roundtrip;
    bool abstract_relevant = TRUE;

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
                  ? (UmlItem *)((Package *) parent())->get_uml()->get_classview()
                  : (UmlItem *)((Class *) parent())->get_uml();
    WrapperStr str = WrapperStr(text(0).toAscii().constData());

    uml = UmlBaseClass::create(p, str);

    if (uml == 0) {
        // probably already exist
        const Q3PtrVector<UmlItem> & ch = p->children();
        UmlItem ** v = ch.data();
        UmlItem ** const vsup = v + ch.size();
        UmlItem * x;

        for (; v != vsup; v += 1) {
            if (((x = *v)->kind() == aClass) && (x->name() == str)) {
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

#ifdef ROUNDTRIP
    uml->set_created();
}

#endif

switch (stereotype) {
case 'i':
#ifdef ROUNDTRIP
    if (roundtrip) {
        if (JavaSettings::classStereotype(uml->stereotype()) != "interface") {
            uml->set_Stereotype("interface");
            set_updated();
        }

        if (neq(uml->javaDecl(), JavaSettings::interfaceDecl())) {
            uml->set_JavaDecl(JavaSettings::interfaceDecl());
            set_updated();
        }

        abstract_relevant = FALSE;
    }
    else {
#endif
        uml->set_Stereotype("interface");
        uml->set_JavaDecl(JavaSettings::interfaceDecl());
#ifdef ROUNDTRIP
    }

#endif
    break;

case '@': {
    WrapperStr s = JavaSettings::interfaceDecl();
    int index = s.find("interface");

    if (index != -1)
        s.insert(index, "@");

#ifdef ROUNDTRIP

    if (roundtrip) {
        if (JavaSettings::classStereotype(uml->stereotype()) != "@interface") {
            uml->set_Stereotype("@interface");
            set_updated();
        }

        if (neq(uml->javaDecl(), s)) {
            uml->set_JavaDecl(s);
            set_updated();
        }

        abstract_relevant = FALSE;
    }
    else {
#endif
        uml->set_Stereotype("@interface");
        uml->set_JavaDecl(s);
#ifdef ROUNDTRIP
    }

#endif
}
break;

case 'e':
#ifdef ROUNDTRIP
    if (roundtrip) {
        if (JavaSettings::classStereotype(uml->stereotype()) != "enum") {
            uml->set_Stereotype("enum");
            set_updated();
        }

        if (neq(uml->javaDecl(), JavaSettings::enumDecl())) {
            uml->set_JavaDecl(JavaSettings::enumDecl());
            set_updated();
        }
    }
    else {
#endif
        uml->set_Stereotype("enum");
        uml->set_JavaDecl(JavaSettings::enumDecl());
#ifdef ROUNDTRIP
    }

#endif
    break;

default:
    // class
#ifdef ROUNDTRIP
    if (roundtrip) {
        if (!uml->stereotype().isEmpty()) {
            WrapperStr jst = JavaSettings::classStereotype(uml->stereotype());

            if ((jst == "interface") || (jst == "@interface") || (jst == "enum")) {
                uml->set_Stereotype("");
                set_updated();
            }
        }

        if (neq(uml->javaDecl(), JavaSettings::classDecl())) {
            uml->set_JavaDecl(JavaSettings::classDecl());
            set_updated();
        }
    }

#endif
    break;
}

#ifdef ROUNDTRIP

if (roundtrip) {
    if (abstract_relevant && (uml->isAbstract() != abstractp)) {
        uml->set_isAbstract(abstractp);
        set_updated();
    }
}
else
#endif
    if (abstractp)
        uml->set_isAbstract(abstractp);

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

        if (neq(p1.name(), p2.name()) || !p1.extend().equal(p2.extend())) {
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
    for (rank = 0, it = formals.begin(); it != formals_end; it++, rank += 1)
        uml->addFormal(rank, *it);

formals.clear();

return uml;
}

#ifndef ROUNDTRIP
bool Class::already_in_bouml()
{
    const Q3PtrVector<UmlItem> & ch = get_uml()->children();
    UmlItem ** v = ch.data();
    UmlItem ** const vsup = v + ch.size();

    for (; v != vsup; v += 1)
        if ((*v)->kind() != aClass)
            return TRUE;

    return FALSE;
}
#endif

// note : 'tmplts' must be given by value to not have to
// remove the may be added formals in all the return cases

bool Class::reverse(ClassContainer * container, WrapperStr stereotype,
                    WrapperStr annotation, bool abstractp, bool finalp,
                    aVisibility visibility, WrapperStr & path,
                    Q3ValueList<FormalParameterList> tmplts
#ifdef ROUNDTRIP
                    , bool rndtrp, QList<UmlItem *> & expectedorder
#endif
                   )
{
    WrapperStr comment = Lex::get_comments();
    WrapperStr description = Lex::get_description();
    WrapperStr name;

    if ((name = Lex::read_word()).isEmpty())
        return FALSE;

    Class * cl = container->define(name, *((const char *) stereotype));

    if ((cl == 0) || cl->reversedp)
        return FALSE;

#ifdef ROUNDTRIP
    UmlClass * cl_uml = cl->uml;

    if ((cl_uml != 0) &&
        !cl_uml->is_roundtrip_expected() &&
        !cl_uml->is_created()) {
        // bypass all the class
        // note : user can't ask for update a nested class
        char c;

        while ((c = Lex::read_word_bis()) != '{') {
            if (c == 0) {
                Lex::premature_eof();
                throw 0;
            }
        }

        UmlOperation::skip_body(1);
        return TRUE;
    }

    bool roundtrip = FALSE;
    QList<UmlItem *> expected_order;
#else
    UmlClass * cl_uml = 0;
#endif

    if (Package::scanning()) {
        cl->abstractp = abstractp;
        cl->filename = path;
#ifndef REVERSE
        cl->description = comment;
#endif
    }
    else {
        cl->reversedp = TRUE;
        cl_uml = cl->get_uml();

#ifdef ROUNDTRIP

        if (cl_uml->is_created())
            Statistic::one_class_created_more();
        else
            roundtrip = TRUE;

#else

        if (cl->already_in_bouml()) {
            cl_uml->unload();
            return FALSE;
        }

# ifdef REVERSE
        Statistic::one_class_more();
# endif
#endif

#ifdef ROUNDTRIP

        if (roundtrip) {
            if (cl_uml->javaDecl().find("${description}") != -1) {
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

            if (cl_uml->visibility() != visibility) {
                cl_uml->set_Visibility(visibility);
                cl->set_updated();
            }

            if (nequal(cl_uml->javaAnnotations(), annotation)) {
                cl_uml->set_JavaAnnotations(annotation);
                cl->set_updated();
            }

            if (cl_uml->isAbstract() != abstractp) {
                switch (cl->stereotype) {
                case 'i':
                case '@':
                    break;

                default:
                    cl_uml->set_isAbstract(abstractp);
                    cl->set_updated();
                }
            }

            if (cl_uml->isJavaFinal() != finalp) {
                cl_uml->set_isJavaFinal(finalp);
                cl->set_updated();
            }
        }
        else {
#endif

            if (! comment.isEmpty())
                cl_uml->set_Description((cl_uml->javaDecl().find("${description}") != -1)
                                        ? description : Lex::simplify_comment(comment));

            cl_uml->set_Visibility(visibility);

            if (! annotation.isEmpty())
                cl_uml->set_JavaAnnotations(annotation);

            if (abstractp)
                cl_uml->set_isAbstract(abstractp);

            if (finalp)
                cl_uml->set_isJavaFinal(finalp);

#ifdef ROUNDTRIP
        }

#endif
    }

    WrapperStr s = Lex::read_word();

    if (s == "<") {
        if (!cl->get_formals(cl->formals, FALSE, tmplts))
            return FALSE;

        tmplts.append(cl->formals);

        // note : formals set on cl_uml by get_uml()

        s = Lex::read_word();
    }

    if (s.isEmpty()) {
        if (! Package::scanning()) {
            Lex::premature_eof();
            cl_uml->unload();
        }

        return FALSE;
    }

    if (Package::scanning()) {
        if (s != "{") {
            char c;

            while ((c = Lex::read_word_bis()) != '{')
                if (c == 0)
                    return FALSE;
        }
    }
    else {
        if (s == "extends") {
            if (cl->stereotype == 'e') {
                Lex::syntax_error("an enum can't extend");

                if (! Package::scanning())
                    cl_uml->unload();

                return FALSE;
            }
            else if ((cl->stereotype != 'i')
                     ? !cl->manage_extends(container, tmplts
#ifdef ROUNDTRIP
                                           , roundtrip, expected_order
#endif
                                          )
                     : !cl->manage_implements(container, aGeneralisation, tmplts
#ifdef ROUNDTRIP
                                              , roundtrip, expected_order
#endif
                                             ))
                return FALSE;

            s = Lex::read_word();
        }

        if (s == "implements") {
            if (! cl->manage_implements(container, aRealization, tmplts
#ifdef ROUNDTRIP
                                        , roundtrip, expected_order
#endif
                                       ))
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

    if ((cl->stereotype == 'e') &&
        !cl->manage_enum_items(
#ifdef ROUNDTRIP
            roundtrip, expected_order
#endif
        )) {
        if (! Package::scanning())
            cl_uml->unload();

        return FALSE;
    }

    while ((s = Lex::read_word()) != "}") {
        if (s.isEmpty()) {
            if (! Package::scanning()) {
                Lex::premature_eof();
                cl_uml->unload();
            }

            return FALSE;
        }

#ifdef TRACE
        QLOG_INFO() << "in class def a lu '" << s << "'\n";
#endif

        if (s == ";")
            ;
        else if (! cl->manage_member(s, path
#ifdef ROUNDTRIP
                                     , roundtrip, expected_order
#endif
                                    )) {
            if (! Package::scanning())
                cl_uml->unload();

            return FALSE;
        }
    }

    if (! Package::scanning()) {
        if (cl->stereotype == 'e') {
            // may be not abstract even an oper is abstract
#ifdef ROUNDTRIP
            if (roundtrip) {
                if (cl_uml->isAbstract() != abstractp) {
                    cl_uml->set_isAbstract(abstractp);
                    cl->set_updated();
                }
            }
            else
#endif
                cl_uml->set_isAbstract(abstractp);
        }

#ifdef REVERSE
# ifdef ROUNDTRIP

        if (roundtrip) {
            if (!cl->from_libp() && ((BrowserNode *) cl->parent())->isa_package()) {
                if (CurrentArtifact == 0) {
                    // roundtriped class was moved in an other file
                    Package * pack = (Package *) cl->parent();

                    cl_uml->need_artifact(pack->get_imports(), pack->is_java_lang_added(),
                                          pack->get_static_imports(), path, CurrentArtifact);
                }
                else if (CurrentArtifact->is_fully_updated()) {
                    if (CurrentArtifact->associatedClasses().isEmpty()) {
                        // first association, to set artifact def
                        Package * pack = (Package *) cl->parent();
                        UmlArtifact * art = 0;

                        CurrentArtifact->addAssociatedClass(cl_uml);
                        cl_uml->need_artifact(pack->get_imports(), pack->is_java_lang_added(),
                                              pack->get_static_imports(), path, art);
                    }
                    else
                        CurrentArtifact->addAssociatedClass(cl_uml);
                }
            }

            cl_uml->reorder(expected_order);

            if (cl->updated) {
                Statistic::one_class_updated_more();

                UmlCom::trace(QString("<font face=helvetica>class <i>")
                              + cl->text(0) + "</i> updated from <i>"
                              + Lex::filename() + "</i></font><br>");
            }
        }
        else {
# endif

            if (!cl->from_libp() && ((BrowserNode *) cl->parent())->isa_package()) {
                Package * pack = (Package *) cl->parent();

                cl_uml->need_artifact(pack->get_imports(), pack->is_java_lang_added(),
                                      pack->get_static_imports(), path, CurrentArtifact);
            }

#endif

            cl_uml->unload();
        }

#ifdef ROUNDTRIP

        if (rndtrp)
            expectedorder.append(cl_uml);
    }

#endif

    return TRUE;
}

bool Class::manage_extends(ClassContainer * container,
                           const Q3ValueList<FormalParameterList> & tmplts
#ifdef ROUNDTRIP
                           , bool roundtrip, QList<UmlItem *> & expected_order
#endif
                          )
{
    // out of scanning
#ifdef TRACE
    QLOG_INFO() << name() << "->manage_extends()\n";
#endif

    UmlTypeSpec typespec;
    Class * cl = 0;
    Q3ValueList<UmlTypeSpec> actuals;
    WrapperStr str_actuals;
    WrapperStr dummy;

    if (! container->read_type(typespec, &cl, tmplts, &actuals, str_actuals,
                               dummy, 0, dummy, dummy))
        return FALSE;

    if (typespec.type == 0) {
#ifdef TRACE
        QLOG_INFO() << "cannot create mother\n";
#endif
        return FALSE;
    }

    if (cl != 0)
        inherit(cl);
    else
        inherit(typespec.type);

    return add_inherit(aGeneralisation, typespec, actuals, str_actuals
#ifdef ROUNDTRIP
                       , roundtrip, expected_order
#endif
                      );
}

bool Class::manage_implements(ClassContainer * container, aRelationKind k,
                              const Q3ValueList<FormalParameterList> & tmplts
#ifdef ROUNDTRIP
                              , bool roundtrip, QList<UmlItem *> & expected_order
#endif
                             )
{
    // out of scanning
#ifdef TRACE
    QLOG_INFO() << name() << "->manage_implements()\n";
#endif

    for (;;) {
        UmlTypeSpec typespec;
        Class * cl = 0;
        Q3ValueList<UmlTypeSpec> actuals;
        WrapperStr str_actuals;
        WrapperStr dummy;

        if (! container->read_type(typespec, &cl, tmplts, &actuals, str_actuals,
                                   dummy, 0, dummy, dummy))
            return FALSE;

        if (typespec.type == 0) {
#ifdef TRACE
            QLOG_INFO() << "cannot create mother\n";
#endif
            return FALSE;
        }

        if (cl != 0)
            inherit(cl);
        else
            inherit(typespec.type);

        if (!add_inherit(k, typespec, actuals, str_actuals
#ifdef ROUNDTRIP
                         , roundtrip, expected_order
#endif
                        ))
            return FALSE;

        WrapperStr s = Lex::read_word();

        if (s == "{") {
            Lex::unread_word(s);
            return TRUE;
        }

        if (s != ",") {
            Lex::error_near(s, " ',' expected");
            return FALSE;
        }
    }
}

bool Class::add_inherit(aRelationKind k, UmlTypeSpec & typespec,
                        Q3ValueList<UmlTypeSpec> & actuals,
                        WrapperStr & str_actuals
#ifdef ROUNDTRIP
                        , bool roundtrip, QList<UmlItem *> & expected_order
#endif
                       )
{
    if (!actuals.isEmpty())
        k = aRealization;

    UmlRelation * rel;

#ifdef ROUNDTRIP

    if (roundtrip) {
        const Q3PtrVector<UmlItem> & ch = uml->children();
        UmlItem ** v = ch.data();
        UmlItem ** const vsup = v + ch.size();
        UmlItem * x;

        rel = 0;

        for (; v != vsup; v += 1) {
            if (((x = *v)->kind() == aRelation) &&
                (((UmlRelation *) x)->roleType() == typespec.type)) {
                aRelationKind rk = ((UmlRelation *) x)->relationKind();

                if ((rk == aRealization) || (rk == aGeneralisation)) {
                    rel = (UmlRelation *) x;
                    rel->set_usefull();

                    if (rk != k)
                        rel->set_rel_kind(k);

                    WrapperStr expected_decl;

                    if (!typespec.explicit_type.isEmpty())
                        expected_decl = typespec.explicit_type;
                    else if (actuals.isEmpty())
                        expected_decl = "${type}";
                    else {
                        if (rel->stereotype() != "bind") {
                            rel->set_Stereotype("bind");
                            set_updated();
                        }

                        Q3ValueList<UmlActualParameter> current_actuals = uml->actuals();
                        Q3ValueList<UmlActualParameter>::ConstIterator iter_current;

                        // search for first corresponding actual
                        for (iter_current = current_actuals.begin();
                             iter_current != current_actuals.end();
                             iter_current++)
                            if ((*iter_current).superClass() == typespec.type)
                                break;

                        if (iter_current == current_actuals.end())
                            // something wrong
                            expected_decl = "${type}" + str_actuals;
                        else  {
                            expected_decl = "${type}";

                            Q3ValueList<UmlTypeSpec>::ConstIterator iter = actuals.begin();

                            do {
                                if (!(*iter).equal((*iter_current).value())) {
                                    uml->replaceActual((*iter_current).rank(), *iter);
                                    set_updated();
                                }

                                iter++;
                                iter_current++;
                            }
                            while ((iter != actuals.end()) &&
                                   (iter_current != current_actuals.end()) &&
                                   ((*iter_current).superClass() == typespec.type));
                        }
                    }

                    if (neq(rel->javaDecl(), expected_decl)) {
                        rel->set_JavaDecl(expected_decl);
                        set_updated();
                    }

                    expected_order.append(rel);

                    return TRUE;
                }
            }
        }
    }

#endif

    rel = UmlRelation::create(k, uml, typespec.type);

    if (rel == 0) {
#ifdef TRACE
        QLOG_INFO() << "cannot create <|---\n";
#endif
        return FALSE;
    }

#ifdef ROUNDTRIP
    set_updated();
    expected_order.append(rel);
#elif defined(REVERSE)
    Statistic::one_relation_more();
#endif

    if (!typespec.explicit_type.isEmpty())
        rel->set_JavaDecl(typespec.explicit_type);
    else if (actuals.count() > typespec.type->formals().count())
        // something wrong
        rel->set_JavaDecl("${type}" + str_actuals);
    else {
        rel->set_JavaDecl("${type}");

        if (! actuals.isEmpty()) {
            Q3ValueList<UmlTypeSpec>::ConstIterator iter;
            unsigned actual_rank = (typespec.explicit_type.isEmpty())
                                   ? uml->actuals().count()
                                   : 0;

            for (iter = actuals.begin(); iter != actuals.end(); iter++)
                uml->replaceActual(actual_rank++, *iter);

            rel->set_Stereotype("bind");
        }
    }

    return TRUE;
}

// this inherits cl => it knowns cl's sub-classes
void Class::inherit(Class * cl)
{
    if (! cl->reversed()) {
        QString f = cl->get_path();

        if (!f.isEmpty()) {
            while (!((BrowserNode *) cl->parent())->isa_package())
                cl = (Class *) cl->parent();

#ifdef REVERSE
            UmlArtifact * cp = CurrentArtifact;
#endif

            Lex::push_context();
            Package::push_context();
#ifdef ROUNDTRIP

            if ((cl->uml != 0) && !cl->uml->is_created()) {
                if (cl->uml->is_roundtrip_expected())
                    ((Package *) cl->parent())->reverse_file(WrapperStr(f.toAscii().constData()),
                            cl->uml->associatedArtifact());
            }
            else
#endif
                ((Package *) cl->parent())->reverse_file(WrapperStr(f.toAscii().constData()));

            Lex::pop_context();
            Package::pop_context();
#ifdef REVERSE
            CurrentArtifact = cp;
#endif
        }
    }

    Q3DictIterator<Class> it(cl->Defined);

    while (it.current()) {
        Defined.replace(it.currentKey(), it.current());
        ++it;
    }

    Q3DictIterator<UmlClass> it_uml(cl->user_defined);

    while (it_uml.current()) {
        user_defined.replace(it_uml.currentKey(), it_uml.current());
        ++it_uml;
    }
}

// this inherits uml_cl => it knowns uml_cl's sub-classes
void Class::inherit(UmlClass * uml_cl, WrapperStr header)
{
    Q3PtrVector<UmlItem> ch = uml_cl->children();
    UmlItem ** v = ch.data();
    UmlItem ** const vsup = v + ch.size();
    UmlItem * x;

    for (; v != vsup; v += 1) {
        if ((x = *v)->kind() == aClass) {
            WrapperStr s = (header.isEmpty()) ? x->name() : header + x->name();

            user_defined.replace(s, (UmlClass *) x);
            s += ".";
            inherit((UmlClass *) x, s);
        }
    }
}

bool Class::get_formals(FormalParameterList & tmplt, bool name_only,
                        Q3ValueList<FormalParameterList> & tmplts)
{
    // '<' already read
    tmplt.clear();

    WrapperStr s;

    do {
        WrapperStr n = Lex::read_word();

        if (n == ">")
            break;

        if (!Lex::identifierp(n)) {
            if (! Package::scanning())
                Lex::error_near(n, " identifier expected");

            return FALSE;
        }

        s = Lex::read_word();

        if ((s == ",") || (s == ">"))
            ;
        else if (s != "extends") {
            if (! Package::scanning())
                Lex::error_near(n);

            return FALSE;
        }
        else {
            int level = 0;

            if (!name_only)
                Lex::mark();

            for (;;) {
                s = Lex::read_word(TRUE);

                if (s == ",") {
                    if (level == 0)
                        break;
                }
                else if (s == ">") {
                    if (level-- == 0)
                        break;
                }
                else if (s == "<")
                    level += 1;
                else if (s.isEmpty()) {
                    if (! Package::scanning())
                        Lex::premature_eof();

                    return FALSE;
                }
            }
        }

        UmlTypeSpec typespec;

        if (!name_only && !Package::scanning()) {
            WrapperStr e = Lex::region();

            e.resize(e.length()); // remove , or >
            e = e.stripWhiteSpace();

            if (! e.isEmpty())
                compute_type(e, typespec, tmplts);
        }

        tmplt.append(UmlFormalParameter(n, typespec));

    }
    while (s == ",");

    return TRUE;
}

bool Class::manage_member(WrapperStr s, WrapperStr & path
#ifdef ROUNDTRIP
                          , bool roundtrip, QList<UmlItem *> & expected_order
#endif
                         )
{
    aVisibility visibility;

    switch (stereotype) {
    case 'i':
    case '@':
        visibility = PublicVisibility;
        break;

    default:
        visibility = PackageVisibility;
    }

    WrapperStr comment = Lex::get_comments();
    WrapperStr description = Lex::get_description();
    bool m_staticp = FALSE;
    bool m_finalp = FALSE;
    bool m_abstractp = FALSE;
    bool m_nativep = FALSE;
    bool m_strictfp = FALSE;
    bool m_synchronizedp = FALSE;
    bool m_transientp = FALSE;
    bool m_volatilep = FALSE;
    bool first_var = TRUE;
    WrapperStr array_before_name;
    WrapperStr array;
    UmlTypeSpec type;
    bool type_read = FALSE;
    Q3ValueList<FormalParameterList> tmplts;
    Q3ValueList<UmlTypeSpec> actuals;
    WrapperStr str_actuals;
    WrapperStr name;
    WrapperStr value;
    WrapperStr annotation;
    WrapperStr oper_templ;
    UmlClass * first_actual_class = 0;
    WrapperStr type_def;
    WrapperStr genericname;

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
        else if (s == "static") {
            if ((s = Lex::read_word()) == "{") {
#ifdef REVERSE

                if (Package::scanning() || from_libp())
                    UmlOperation::skip_body(1);
                else {
                    Lex::mark();
                    UmlOperation::skip_body(1);

#ifdef ROUNDTRIP
                    UmlExtraClassMember::add_init(get_uml(), "  static {" + Lex::region(),
                                                  roundtrip, expected_order);
#else
                    UmlExtraClassMember::create(get_uml(), "initialization")->
                    set_JavaDecl("  static {" + Lex::region());
#endif
                }

#else
                UmlOperation::skip_body(1);
#endif
                return TRUE;
            }

            m_staticp = TRUE;
            continue;
        }
        else if (s == "abstract")
            m_abstractp = TRUE;
        else if (s == "final")
            m_finalp = TRUE;
        else if (s == "native")
            m_nativep = TRUE;
        else if (s == "strictfp")
            m_strictfp = TRUE;
        else if (s == "synchronized")
            m_synchronizedp = TRUE;
        else if (s == "transient")
            m_transientp = TRUE;
        else if (s == "volatile")
            m_volatilep = TRUE;
        else if (*s == '@') {
            if (annotation.isEmpty())
                annotation = s;
            else
                annotation += "\n" + s;
        }
        else if (s == "{") {
#ifdef REVERSE

            if (Package::scanning() || from_libp())
                UmlOperation::skip_body(1);
            else {
                Lex::mark();
                UmlOperation::skip_body(1);
#ifdef ROUNDTRIP
                UmlExtraClassMember::add_init(get_uml(), "  {" + Lex::region(),
                                              roundtrip, expected_order);
#else
                UmlExtraClassMember::create(get_uml(), "initialization")->
                set_JavaDecl("  {" + Lex::region());
#endif
            }

#else
            UmlOperation::skip_body(1);
#endif
            return TRUE;
        }
        else
            break;

        s = Lex::read_word();
    }

    for (;;) {
        if (s.isEmpty()) {
            if (!Package::scanning())
                Lex::premature_eof();

            return FALSE;
        }
        else if (s == "(") {
            // an operation
            if (Package::scanning()) {
                UmlOperation::skip_body(0);
                return TRUE;
            }

            if (!type_read) {
                Lex::error_near("(", " type missing");
#ifdef TRACE
                QLOG_INFO() << "ERROR '(' and type not yet read\n";
#endif
                return FALSE;
            }
            else if (name.isEmpty()) {
                if (type.toString() == WrapperStr(text(0).toAscii().constData())) {
                    // constructor
                    name = text(0);
                    type.type = 0;
                }
                else {
                    Lex::error_near("(");
#ifdef TRACE
                    QLOG_INFO() << "ERROR '(' no name not constructor\n";
#endif
                    return FALSE;
                }
            }

            return UmlOperation::new_one(this, name, tmplts, oper_templ,
                                         type, str_actuals, first_actual_class, type_def,
                                         visibility, m_finalp, m_abstractp,
                                         m_staticp, m_nativep, m_strictfp, m_synchronizedp,
                                         array, comment, description, annotation
#ifdef ROUNDTRIP
                                         , roundtrip, expected_order
#endif
                                        );
        }
        else if (s == "=") {
            // initialized variable, by pass value
            Lex::mark();
            UmlOperation::skip_expr(0);

            if (Package::scanning())
                return TRUE;

            value = Lex::region();

            if (value.isEmpty())
                Lex::error_near("=", " value is missing");

            char c = ((const char *) value)[value.length() - 1];

            if ((c == ';') || (c == ',')) {
                value.truncate(value.length() - 1); // remove ';' or ','
                s = (c == ';') ? ";" : ",";
            }
            else if (((s = Lex::read_word()) != ";") && (s != ","))
                Lex::error_near(s, " ';' or ',' expected");

            continue;
        }
        else if ((s == ";") || (s == ",")) {
            if (Package::scanning())
                return TRUE;

            if (name.isEmpty() || !type_read || !oper_templ.isEmpty()) {
                Lex::error_near(s);
#ifdef TRACE
                QLOG_INFO() << "ERROR ';' et name.isEmpty() || type not yet read\n";
#endif
                return FALSE;
            }

            // in case of a generic and at least an actual is a known class
            // force the use of a operation to the actual stereotyped by the
            // generic, typically for List ...
            if ((type.type != 0) &&
                ((first_actual_class == 0) ||
                 JavaSettings::umlType(genericname).isEmpty())) {
                if (!UmlRelation::new_one(this, name, type, str_actuals, visibility,
                                          m_staticp, m_finalp, m_transientp, m_volatilep,
                                          array, value, comment, description, annotation
#ifdef ROUNDTRIP
                                          , roundtrip, expected_order
#endif
                                         ))
                    return FALSE;
            }
            else if (first_actual_class != 0) {
                if (!UmlRelation::new_one(this, name, first_actual_class,
                                          type_def, genericname, visibility,
                                          m_staticp, m_finalp, m_transientp, m_volatilep,
                                          array, value, comment, description, annotation
#ifdef ROUNDTRIP
                                          , roundtrip, expected_order
#endif
                                         ))
                    return FALSE;
            }
            else if (!UmlAttribute::new_one(this, name, type, visibility,
                                            m_staticp, m_finalp, m_transientp,
                                            m_volatilep, array, value,
                                            comment, description, annotation
#ifdef ROUNDTRIP
                                            , roundtrip, expected_order
#endif
                                           ))
                return FALSE;

            if (s == ";")
                return TRUE;

            // type var1, var2 ...;
            name = 0;
            array = array_before_name;
            value = 0;
        }
        else if (Lex::identifierp(s)) {
            if ((s == "class") || (s == "enum") ||
                (s == "interface") || (s == "@interface"))
                return reverse(this, s, annotation, m_abstractp, m_finalp,
                               visibility, path, tmplts
#ifdef ROUNDTRIP
                               , roundtrip, expected_order
#endif
                              );

            if (!type_read) {
                if (!((Package::scanning())
                      ? Lex::bypass_type(s)
                      : read_type(type, 0, tmplts, 0, str_actuals, s,
                                  &first_actual_class, type_def, genericname)))
                    return FALSE;

#ifdef TRACE
                QLOG_INFO() << "type = '" << s << "...'\n";
#endif
                type_read = TRUE;
            }
            else if (name.isEmpty()) {
                name = s;

                if (first_var) {
                    first_var = FALSE;
                    array_before_name = array;
                }

#ifdef TRACE
                QLOG_INFO() << "name = '" << name << "'\n";
#endif
            }
            else {
                if (! Package::scanning())
                    Lex::error_near(s);

#ifdef TRACE
                QLOG_INFO() << "ERROR '" << s << "' alors qu a deja le type et le nom '" << name << "'\n";
#endif
                return FALSE;
            }
        }
        else if (*((const char *) s) == '[')
            array += s;
        else if (*((const char *) s) == '<') {
            // <...> behind an operation
            FormalParameterList tmplt;

            tmplts.append(tmplt);
            Lex::mark();

            if (!get_formals(tmplts.last(), TRUE, tmplts))
                return FALSE;

            oper_templ = "<" + Lex::region();
        }
        else {
            if (! Package::scanning())
                Lex::error_near(s);

#ifdef TRACE
            QLOG_INFO() << "ERROR : '" << s << "'\n";
#endif
            UmlOperation::skip_expr(*((const char *) s) == '{');
            return TRUE;
        }

        s = Lex::read_word();
    }
}

bool Class::manage_enum_items(
#ifdef ROUNDTRIP
    bool roundtrip, QList<UmlItem *> & expected_order
#endif
)
{
    UmlClass * cl_uml = (Package::scanning()) ? 0 : get_uml();
    WrapperStr s;

    for (;;) {
        if ((s = Lex::read_word()).isEmpty()) {
            if (! Package::scanning())
                Lex::premature_eof();

            return FALSE;
        }
        else if (s == ";")
            return TRUE;
        else if (s == "}") {
            Lex::unread_word("}");
            return TRUE;
        }
        else if (! Lex::identifierp(s)) {
            if (! Package::scanning())
                Lex::error_near(s, " identifier expected");

            return FALSE;
        }
        else if (! UmlAttribute::manage_enum_item(s, cl_uml
#ifdef ROUNDTRIP
                 , roundtrip, expected_order
#endif
                                                 ))
            return FALSE;
    }
}

Class * Class::define(const WrapperStr & name, char st)
{
    Class * cl = Defined[name];

    if (!Package::scanning() || (cl != 0))
        return cl;

    cl = new Class(this, name, st);

    declare(name, cl);

    return cl;
}

#ifdef ROUNDTRIP
Class * Class::upload_define(UmlClass * ucl)
{
    Class * cl = new Class(this, ucl);

    declare(ucl->name(), cl);

    return cl;
}

Class * Class::localy_defined(QString name) const
{
    return Defined[name];
}
#endif

void Class::compute_type(WrapperStr name, UmlTypeSpec & typespec,
                         const Q3ValueList<FormalParameterList> & tmplts,
                         Class ** need_object)
{
    const Q3ValueList<UmlFormalParameter> formals = get_uml()->formals();
    Q3ValueList<UmlFormalParameter>::ConstIterator it;

    for (it = formals.begin(); it != formals.end(); it++) {
        if ((*it).name() == name) {
            // a formal
            if (need_object == 0) {
                typespec.type = 0;
                typespec.explicit_type = name;

                return;
            }

            BrowserNode * p = (BrowserNode *) this;

            do {
                p = (BrowserNode *) parent();
            }
            while (! p->isa_package());

            ((Package *) p)->force_class(name, typespec, need_object);
            return;
        }
    }

    WrapperStr s = name;
    Class * cl = this;

    for (;;) {
        Class * cl2 = cl->Defined[s];

        if (cl2 != 0) {
            // a sub class
            typespec.type = cl2->get_uml();
            typespec.explicit_type = 0;

            if (need_object != 0)
                *need_object = cl2;

            return;
        }

        UmlClass * uml_cl2 = cl->user_defined[s];

        if (uml_cl2 != 0) {
            // a sub class
            typespec.type = uml_cl2;
            typespec.explicit_type = 0;

            return;
        }

        int index = s.find('.');

        if ((index != -1) && (s.left(index) == (const char *) cl->text(0)))
            s = s.mid(index + 1);
        else
            break;
    }

    // not defined in the current class scope, goes up
    BrowserNode * p = (BrowserNode *) parent();

    if (! p->isa_package())
        ((Class *) p)->compute_type(name, typespec, tmplts, need_object);
    else
        ((Package *) p)->compute_type(name, typespec, tmplts, need_object);
}

void Class::declare(const WrapperStr & name, Class * cl)
{
    Defined.insert(name, cl);

    WrapperStr s = WrapperStr(text(0).toAscii().constData()) + '.' + name;
    ((BrowserNode *) parent())->declare(s, cl);
}

bool Class::isa_package() const
{
    return FALSE;
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
    Historic.appendClass(this);
}

// called when the node is activated for any reason
// for instance because the user clink on it, or
// select it using keyboard arrows

void Class::activated()
{
    JavaCatWindow::clear_trace();

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

    JavaCatWindow::trace(description);
}

void Class::historic_back()
{
    if (Historic.hasPrev())
        BrowserView::select(Historic.prev());
}

void Class::historic_forward()
{
    if (Historic.hasNext())
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
        Class * cl = this;
        BrowserNode * b;

        while (!(b = (BrowserNode *) cl->parent())->isa_package())
            if (b == 0) {
                Q_ASSERT(0);
            }

        cl = (Class *) b;

        JavaSettings::set_UseDefaults(TRUE);
        UmlCom::message("reverse " + cl->filename);
        JavaCatWindow::clear_trace();
        ((Package *) b)->reverse_file(filename);
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

    // sub classes

    Q3ListViewItem * child;

    for (child = firstChild(); child != 0; child = child->nextSibling())
        ((BrowserNode *) child)->backup(dt);

    dt << ((Q_INT8) '@');
}

#endif

void Class::restore(QDataStream  & dt, char c, BrowserNode * parent)
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

    Class * cl;

#ifdef ROUNDTRIP

    if ((cl = parent->localy_defined(n)) != 0)
        cl->from_lib = TRUE;
    else {
#endif
        cl = new Class(parent, n, ((c == 'c') || (c == 'i')) ? c : (c - 'A' + 'a'));

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

#ifdef ROUNDTRIP
    }

#endif

    delete [] n;
    delete [] fn;

    if (d) delete [] d;

    // sub classes

    Q_INT8 k;

    while (dt >> k, k != '@')
        restore(dt, (char) k, cl);
}
