
#include "UmlCom.h"
#include "UmlBaseActivityAction.h"
#include "UmlDiagram.h"
//Added by qt3to4:
#include <QByteArray>

const QByteArray & UmlBaseActivityAction::preCondition()
{
    read_if_needed_();
    return _pre_condition;
}

bool UmlBaseActivityAction::set_PreCondition(const char * v)
{
    return set_it_(_pre_condition, v, setUmlEntryBehaviorCmd);
}

const QByteArray & UmlBaseActivityAction::postCondition()
{
    read_if_needed_();
    return _post_condition;
}

bool UmlBaseActivityAction::set_PostCondition(const char * v)
{
    return set_it_(_post_condition, v, setUmlExitBehaviorCmd);
}

#ifdef WITHCPP
const QByteArray & UmlBaseActivityAction::cppPreCondition()
{
    read_if_needed_();
    return _cpp_pre_condition;
}

bool UmlBaseActivityAction::set_CppPreCondition(const char * v)
{
    return set_it_(_cpp_pre_condition, v, setCppEntryBehaviorCmd);
}

const QByteArray & UmlBaseActivityAction::cppPostCondition()
{
    read_if_needed_();
    return _cpp_post_condition;
}

bool UmlBaseActivityAction::set_CppPostCondition(const char * v)
{
    return set_it_(_cpp_post_condition, v, setCppExitBehaviorCmd);
}
#endif

#ifdef WITHJAVA
const QByteArray & UmlBaseActivityAction::javaPreCondition()
{
    read_if_needed_();
    return _java_pre_condition;
}

bool UmlBaseActivityAction::set_JavaPreCondition(const char * v)
{
    return set_it_(_java_pre_condition, v, setJavaEntryBehaviorCmd);
}

const QByteArray & UmlBaseActivityAction::javaPostCondition()
{
    read_if_needed_();
    return _java_post_condition;
}

bool UmlBaseActivityAction::set_JavaPostCondition(const char * v)
{
    return set_it_(_java_post_condition, v, setJavaExitBehaviorCmd);
}
#endif

const QByteArray & UmlBaseActivityAction::constraint()
{
    read_if_needed_();
    return _constraint;
}

bool UmlBaseActivityAction::set_Constraint(const char * v)
{
    return set_it_(_constraint, v, setConstraintCmd);
}

UmlDiagram * UmlBaseActivityAction::associatedDiagram()
{
    read_if_needed_();

    return _assoc_diagram;
}

bool UmlBaseActivityAction::set_AssociatedDiagram(UmlDiagram * d)
{
    UmlCom::send_cmd(_identifier, setAssocDiagramCmd, (d == 0) ? (void *) 0 : ((UmlBaseItem *) d)->_identifier);

    if (UmlCom::read_bool()) {
        _assoc_diagram = d;
        return TRUE;
    }
    else
        return FALSE;
}

void UmlBaseActivityAction::unload(bool rec, bool del)
{
    _pre_condition = 0;
    _post_condition = 0;
#ifdef WITHCPP
    _cpp_pre_condition = 0;
    _cpp_post_condition = 0;
#endif
#ifdef WITHJAVA
    _java_pre_condition = 0;
    _java_post_condition = 0;
#endif
    UmlBaseItem::unload(rec, del);
    _constraint = 0;
}

void UmlBaseActivityAction::read_uml_()
{
    _assoc_diagram = (UmlDiagram *) UmlBaseItem::read_();
    UmlBaseItem::read_uml_();
    _pre_condition = UmlCom::read_string();
    _post_condition = UmlCom::read_string();
    _constraint = UmlCom::read_string();
}

#ifdef WITHCPP
void UmlBaseActivityAction::read_cpp_()
{
    _cpp_pre_condition = UmlCom::read_string();
    _cpp_post_condition = UmlCom::read_string();
}
#endif

#ifdef WITHJAVA
void UmlBaseActivityAction::read_java_()
{
    _java_pre_condition = UmlCom::read_string();
    _java_post_condition = UmlCom::read_string();
}
#endif

