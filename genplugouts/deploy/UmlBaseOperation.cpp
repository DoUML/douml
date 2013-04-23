
#include "UmlBaseOperation.h"
#include "UmlOperation.h"
#include "UmlClass.h"

#include "UmlCom.h"
#include "UmlBaseClass.h"
#include "UmlClassMember.h"
#include "UmlOperation.h"
//#include "misc/myio.h"
//Added by qt3to4:
#include <Q3CString>
#include <Q3ValueList>
#include <QFileInfo>
#include <QSettings>
#include "Logging/QsLog.h"

UmlOperation * UmlBaseOperation::create(UmlClass * parent, const char * s)
{
    return (UmlOperation *) parent->create_(anOperation, s);
}

anItemKind UmlBaseOperation::kind()
{
    return anOperation;
}

bool UmlBaseOperation::isBodyGenerationForced()
{
    read_if_needed_();
    return _force_body_generation;
}

bool UmlBaseOperation::set_isBodyGenerationForced(bool v)
{
    UmlCom::send_cmd(_identifier, setIsForceBodyGenCmd, (char) v);

    if (UmlCom::read_bool())
    {
        _force_body_generation = v;
        return TRUE;
    }
    else
        return FALSE;
}

bool UmlBaseOperation::isAbstract()
{
    read_if_needed_();

    return _abstract;
}

bool UmlBaseOperation::set_isAbstract(bool y)
{
    bool b;

    if (set_it_(b, y, setIsAbstractCmd)) {
        _abstract = y;

        if (y)
            ((UmlBaseClass *) parent())->set_isAbstract(y);

        return TRUE;
    }
    else
        return FALSE;
}

const UmlTypeSpec & UmlBaseOperation::returnType()
{
    read_if_needed_();

    return _return_type;
}

bool UmlBaseOperation::set_ReturnType(const UmlTypeSpec & t)
{
    return set_it_(_return_type, t, setReturnTypeCmd);
}

const Q3ValueList<UmlParameter> UmlBaseOperation::params()
{
    read_if_needed_();

    return _params;
}

bool UmlBaseOperation::addParameter(unsigned rank, const UmlParameter & p)
{
    UmlCom::send_cmd(_identifier, addParameterCmd, rank, p.dir, p.name,
                     p.default_value, p.type);

    if (UmlCom::read_bool()) {
        if (_defined)
            _params.insert(_params.at(rank), p);

        return TRUE;
    }
    else
        return FALSE;
}

bool UmlBaseOperation::removeParameter(unsigned rank)
{
    UmlCom::send_cmd(_identifier, removeParameterCmd, rank);

    if (UmlCom::read_bool()) {
        if (_defined)
            _params.remove(_params.at(rank));

        return TRUE;
    }
    else
        return FALSE;
}

bool UmlBaseOperation::replaceParameter(unsigned rank, const UmlParameter & p)
{
    UmlCom::send_cmd(_identifier, replaceParameterCmd, rank, p.dir, p.name,
                     p.default_value, p.type);

    if (UmlCom::read_bool()) {
        if (_defined)
            _params[rank] = p;

        return TRUE;
    }
    else
        return FALSE;
}

const Q3ValueList<UmlTypeSpec> UmlBaseOperation::exceptions()
{
    read_if_needed_();

    return _exceptions;
}

bool UmlBaseOperation::addException(unsigned rank, const UmlTypeSpec & t)
{
    UmlCom::send_cmd(_identifier, addExceptionCmd, rank, t);

    if (UmlCom::read_bool()) {
        if (_defined)
            _exceptions.insert(_exceptions.at(rank), t);

        return TRUE;
    }
    else
        return FALSE;
}

bool UmlBaseOperation::removeException(unsigned rank)
{
    UmlCom::send_cmd(_identifier, removeExceptionCmd, rank);

    if (UmlCom::read_bool()) {
        if (_defined)
            _exceptions.remove(_exceptions.at(rank));

        return TRUE;
    }
    else
        return FALSE;
}

bool UmlBaseOperation::replaceException(unsigned rank, const UmlTypeSpec & t)
{
    UmlCom::send_cmd(_identifier, replaceExceptionCmd, rank, t);

    if (UmlCom::read_bool()) {
        if (_defined)
            _exceptions[rank] = t;

        return TRUE;
    }
    else
        return FALSE;
}

UmlClassMember * UmlBaseOperation::getOf()
{
    read_if_needed_();

    return _get_of;
}

UmlClassMember * UmlBaseOperation::setOf()
{
    read_if_needed_();

    return _set_of;
}

#ifdef WITHCPP
bool UmlBaseOperation::isCppConst()
{
    read_if_needed_();

    return _cpp_const;
}

bool UmlBaseOperation::set_isCppConst(bool y)
{
    bool b;

    if (set_it_(b, y, setIsCppConstCmd)) {
        _cpp_const = y;
        return TRUE;
    }
    else
        return FALSE;
}

bool UmlBaseOperation::isCppFriend()
{
    read_if_needed_();

    return _cpp_friend;
}

bool UmlBaseOperation::set_isCppFriend(bool y)
{
    bool b;

    if (set_it_(b, y, setIsCppFriendCmd)) {
        _cpp_friend = y;
        return TRUE;
    }
    else
        return FALSE;
}

bool UmlBaseOperation::isCppVirtual()
{
    read_if_needed_();

    return _cpp_virtual;
}

bool UmlBaseOperation::set_isCppVirtual(bool y)
{
    bool b;

    if (set_it_(b, y, setIsCppVirtualCmd)) {
        _cpp_virtual = y;
        return TRUE;
    }
    else
        return FALSE;
}

bool UmlBaseOperation::isCppInline()
{
    read_if_needed_();

    return _cpp_inline;
}

bool UmlBaseOperation::set_isCppInline(bool y)
{
    bool b;

    if (set_it_(b, y, setIsCppInlineCmd)) {
        _cpp_inline = y;
        return TRUE;
    }
    else
        return FALSE;
}

const Q3CString & UmlBaseOperation::cppDef()
{
    read_if_needed_();

    return _cpp_def;
}

bool UmlBaseOperation::set_CppDef(const char * s)
{
    return set_it_(_cpp_def, s, setCppDefCmd);
}

Q3CString UmlBaseOperation::cppBody()
{
    // not memorized in the instance for memory size reason
    UmlCom::send_cmd(_identifier, cppBodyCmd);
    return UmlCom::read_string();
}

bool UmlBaseOperation::set_CppBody(const char * s)
{
    // not memorized in the instance for memory size reason
    UmlCom::send_cmd(_identifier, setCppBodyCmd, s);
    return UmlCom::read_bool();
}

const Q3CString & UmlBaseOperation::cppNameSpec()
{
    read_if_needed_();

    return _cpp_name_spec;
}

bool UmlBaseOperation::set_CppNameSpec(const char * s)
{
    return set_it_(_cpp_name_spec, s, setCppNameSpecCmd);
}
#endif

#ifdef WITHCPP
bool UmlBaseOperation::cppGetSetFrozen()
{
    read_if_needed_();
    return _cpp_get_set_frozen;
}

bool UmlBaseOperation::set_CppGetSetFrozen(bool v)
{
    UmlCom::send_cmd(_identifier, setCppFrozenCmd, (char) v);

    if (UmlCom::read_bool()) {
        _cpp_get_set_frozen = v;
        return TRUE;
    }
    else
        return FALSE;
}
#endif

#ifdef WITHCPP
bool UmlBaseOperation::cppContextualBodyIndent()
{
    read_if_needed_();
    return _cpp_contextual_body_indent;
}

bool UmlBaseOperation::set_CppContextualBodyIndent(bool v)
{
    UmlCom::send_cmd(_identifier, setCppContextualBodyIndentCmd, (char) v);

    if (UmlCom::read_bool()) {
        _cpp_contextual_body_indent = v;
        return TRUE;
    }
    else
        return FALSE;
}
#endif

#ifdef WITHJAVA
bool UmlBaseOperation::isJavaFinal()
{
    read_if_needed_();

    return _java_final;
}

bool UmlBaseOperation::set_isJavaFinal(bool y)
{
    bool b;

    if (set_it_(b, y, setJavaFinalCmd)) {
        _java_final = y;
        return TRUE;
    }
    else
        return FALSE;
}

bool UmlBaseOperation::isJavaSynchronized()
{
    read_if_needed_();

    return _java_synchronized;
}

bool UmlBaseOperation::set_isJavaSynchronized(bool y)
{
    bool b;

    if (set_it_(b, y, setJavaSynchronizedCmd)) {
        _java_synchronized = y;
        return TRUE;
    }
    else
        return FALSE;
}

const Q3CString & UmlBaseOperation::javaDef()
{
    return javaDecl();
}

bool UmlBaseOperation::set_JavaDef(const char * s)
{
    return set_JavaDecl(s);
}

Q3CString UmlBaseOperation::javaBody()
{
    // not memorized in the instance for memory size reason
    UmlCom::send_cmd(_identifier, javaBodyCmd);
    return UmlCom::read_string();
}

bool UmlBaseOperation::set_JavaBody(const char * s)
{
    // not memorized in the instance for memory size reason
    UmlCom::send_cmd(_identifier, setJavaBodyCmd, s);
    return UmlCom::read_bool();
}

const Q3CString & UmlBaseOperation::javaNameSpec()
{
    read_if_needed_();

    return _java_name_spec;
}

bool UmlBaseOperation::set_JavaNameSpec(const char * s)
{
    return set_it_(_java_name_spec, s, setJavaNameSpecCmd);
}
#endif

#ifdef WITHJAVA
bool UmlBaseOperation::javaGetSetFrozen()
{
    read_if_needed_();
    return _java_get_set_frozen;
}

bool UmlBaseOperation::set_JavaGetSetFrozen(bool v)
{
    UmlCom::send_cmd(_identifier, setJavaFrozenCmd, (char) v);

    if (UmlCom::read_bool()) {
        _java_get_set_frozen = v;
        return TRUE;
    }
    else
        return FALSE;
}
#endif

#ifdef WITHJAVA
bool UmlBaseOperation::javaContextualBodyIndent()
{
    read_if_needed_();
    return _java_contextual_body_indent;
}

bool UmlBaseOperation::set_JavaContextualBodyIndent(bool v)
{
    UmlCom::send_cmd(_identifier, setJavaContextualBodyIndentCmd, (char) v);

    if (UmlCom::read_bool()) {
        _java_contextual_body_indent = v;
        return TRUE;
    }
    else
        return FALSE;
}
#endif

#ifdef WITHPHP
bool UmlBaseOperation::isPhpFinal()
{
    read_if_needed_();

    return _php_final;
}

bool UmlBaseOperation::set_isPhpFinal(bool y)
{
    bool b;

    if (set_it_(b, y, setPhpFinalCmd)) {
        _php_final = y;
        return TRUE;
    }
    else
        return FALSE;
}

const Q3CString & UmlBaseOperation::phpDef()
{
    return phpDecl();
}

bool UmlBaseOperation::set_PhpDef(const char * s)
{
    return set_PhpDecl(s);
}

Q3CString UmlBaseOperation::phpBody()
{
    // not memorized in the instance for memory size reason
    UmlCom::send_cmd(_identifier, phpBodyCmd);
    return UmlCom::read_string();
}

bool UmlBaseOperation::set_PhpBody(const char * s)
{
    // not memorized in the instance for memory size reason
    UmlCom::send_cmd(_identifier, setPhpBodyCmd, s);
    return UmlCom::read_bool();
}

const Q3CString & UmlBaseOperation::phpNameSpec()
{
    read_if_needed_();

    return _php_name_spec;
}

bool UmlBaseOperation::set_PhpNameSpec(const char * s)
{
    return set_it_(_php_name_spec, s, setPhpNameSpecCmd);
}
#endif

#ifdef WITHPHP
bool UmlBaseOperation::phpGetSetFrozen()
{
    read_if_needed_();
    return _php_get_set_frozen;
}

bool UmlBaseOperation::set_PhpGetSetFrozen(bool v)
{
    UmlCom::send_cmd(_identifier, setPhpFrozenCmd, (char) v);

    if (UmlCom::read_bool()) {
        _php_get_set_frozen = v;
        return TRUE;
    }
    else
        return FALSE;
}
#endif

#ifdef WITHPHP
bool UmlBaseOperation::phpContextualBodyIndent()
{
    read_if_needed_();
    return _php_contextual_body_indent;
}

bool UmlBaseOperation::set_PhpContextualBodyIndent(bool v)
{
    UmlCom::send_cmd(_identifier, setPhpContextualBodyIndentCmd, (char) v);

    if (UmlCom::read_bool()) {
        _php_contextual_body_indent = v;
        return TRUE;
    }
    else
        return FALSE;
}
#endif

#ifdef WITHPYTHON
const Q3CString & UmlBaseOperation::pythonDef()
{
    return pythonDecl();
}

bool UmlBaseOperation::set_PythonDef(const char * s)
{
    return set_PythonDecl(s);
}

Q3CString UmlBaseOperation::pythonBody()
{
    // not memorized in the instance for memory size reason
    UmlCom::send_cmd(_identifier, pythonBodyCmd);
    return UmlCom::read_string();
}

bool UmlBaseOperation::set_PythonBody(const char * s)
{
    // not memorized in the instance for memory size reason
    UmlCom::send_cmd(_identifier, setPythonBodyCmd, s);
    return UmlCom::read_bool();
}

const Q3CString & UmlBaseOperation::pythonNameSpec()
{
    read_if_needed_();

    return _python_name_spec;
}

bool UmlBaseOperation::set_PythonNameSpec(const char * s)
{
    return set_it_(_python_name_spec, s, setPythonNameSpecCmd);
}
#endif

#ifdef WITHPYTHON
bool UmlBaseOperation::pythonGetSetFrozen()
{
    read_if_needed_();
    return _python_get_set_frozen;
}

bool UmlBaseOperation::set_PythonGetSetFrozen(bool v)
{
    UmlCom::send_cmd(_identifier, setPythonFrozenCmd, (char) v);

    if (UmlCom::read_bool()) {
        _python_get_set_frozen = v;
        return TRUE;
    }
    else
        return FALSE;
}
#endif

#ifdef WITHPYTHON
bool UmlBaseOperation::pythonContextualBodyIndent()
{
    read_if_needed_();
    return _python_contextual_body_indent;
}

bool UmlBaseOperation::set_PythonContextualBodyIndent(bool v)
{
    UmlCom::send_cmd(_identifier, setPythonContextualBodyIndentCmd, (char) v);

    if (UmlCom::read_bool()) {
        _python_contextual_body_indent = v;
        return TRUE;
    }
    else
        return FALSE;
}
#endif

#ifdef WITHPYTHON
const Q3CString & UmlBaseOperation::pythonDecorators()
{
    read_if_needed_();
    return _python_decorators;
}

bool UmlBaseOperation::set_PythonDecorators(const char * v)
{
    return set_it_(_python_decorators, v, setPythonDecoratorsCmd);
}
#endif

#ifdef WITHIDL
bool UmlBaseOperation::isIdlOneway()
{
    read_if_needed_();

    return _idl_oneway;
}

bool UmlBaseOperation::set_isIdlOneway(bool y)
{
    bool b;

    if (set_it_(b, y, setIsIdlOnewayCmd)) {
        _idl_oneway = y;
        return TRUE;
    }
    else
        return FALSE;
}

const Q3CString & UmlBaseOperation::idlNameSpec()
{
    read_if_needed_();

    return _idl_name_spec;
}

bool UmlBaseOperation::set_IdlNameSpec(const char * s)
{
    return set_it_(_idl_name_spec, s, setIdlNameSpecCmd);
}
#endif

#ifdef WITHIDL
bool UmlBaseOperation::idlGetSetFrozen()
{
    read_if_needed_();
    return _idl_get_set_frozen;
}

bool UmlBaseOperation::set_IdlGetSetFrozen(bool v)
{
    UmlCom::send_cmd(_identifier, setIdlFrozenCmd, (char) v);

    if (UmlCom::read_bool()) {
        _idl_get_set_frozen = v;
        return TRUE;
    }
    else
        return FALSE;
}
#endif

void UmlBaseOperation::unload(bool rec, bool del)
{
    _return_type.explicit_type = 0;
    _params.clear();
    _exceptions.clear();
#ifdef WITHCPP
    _cpp_def = 0;
    _cpp_name_spec = 0;
#endif
#ifdef WITHJAVA
    _java_name_spec = 0;
#endif
#ifdef WITHPHP
    _php_name_spec = 0;
#endif
#ifdef WITHPYTHON
    _python_name_spec = 0;
    _python_decorators = 0;
#endif
#ifdef WITHIDL
    _idl_name_spec = 0;
#endif
    UmlBaseClassMember::unload(rec, del);
}

void UmlBaseOperation::read_uml_()
{
    UmlBaseClassMember::read_uml_();
    _return_type.type = (UmlClass *) UmlBaseItem::read_();

    if (_return_type.type == 0)
        _return_type.explicit_type = UmlCom::read_string();

    _abstract = UmlCom::read_bool();

    _force_body_generation = UmlCom::read_bool();

    unsigned n;

    for (n = UmlCom::read_unsigned(); n; n -= 1) {
        UmlParameter param;

        param.dir = (aDirection) UmlCom::read_char();
        param.type.type = (UmlClass *) UmlBaseItem::read_();

        if (param.type.type == 0)
            param.type.explicit_type = UmlCom::read_string();

        param.name = UmlCom::read_string();
        param.default_value = UmlCom::read_string();
        _params.append(param);
    }

    for (n = UmlCom::read_unsigned(); n; n -= 1) {
        UmlTypeSpec exception;

        exception.type = (UmlClass *) UmlBaseItem::read_();

        if (exception.type == 0)
            exception.explicit_type = UmlCom::read_string();

        _exceptions.append(exception);
    }

    _get_of = (UmlClassMember *) UmlBaseItem::read_();
    _set_of = (UmlClassMember *) UmlBaseItem::read_();
}
static unsigned api_format()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "DoUML", "settings");
    settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
    int compat = settings.value("Main/compatibility_save", 0).toInt();
    if(compat != 1)
    {
        int fileFormat = settings.value("Main/fileformat").toInt();
        QLOG_WARN() << "grabbing real format 77";
        return fileFormat;
    }
    else
    {
        QLOG_WARN() << "grabbing real format 75";
        return 75;
    }
}
#ifdef WITHCPP
void UmlBaseOperation::read_cpp_()
{
    UmlBaseClassMember::read_cpp_();
    _cpp_const = UmlCom::read_bool();
    _cpp_friend = UmlCom::read_bool();
    _cpp_virtual = UmlCom::read_bool();
    _cpp_inline = UmlCom::read_bool();
    QLOG_WARN() << "reading cpp";
    if(api_format() > 76)
    {
        _cpp_default = UmlCom::read_bool();
        _cpp_delete = UmlCom::read_bool();
        _cpp_override = UmlCom::read_bool();
        _cpp_final = UmlCom::read_bool();
    }
    _cpp_def = UmlCom::read_string();
    _cpp_name_spec = UmlCom::read_string();
    _cpp_get_set_frozen = UmlCom::read_bool();
    _cpp_contextual_body_indent = UmlCom::read_bool();
}
#endif

#ifdef WITHJAVA
void UmlBaseOperation::read_java_()
{
    UmlBaseClassMember::read_java_();
    _java_final = UmlCom::read_bool();
    _java_synchronized = UmlCom::read_bool();
    _java_name_spec = UmlCom::read_string();
    _java_get_set_frozen = UmlCom::read_bool();
    _java_contextual_body_indent = UmlCom::read_bool();
}
#endif

#ifdef WITHPHP
void UmlBaseOperation::read_php_()
{
    UmlBaseClassMember::read_php_();
    _php_final = UmlCom::read_bool();
    _php_name_spec = UmlCom::read_string();
    _php_get_set_frozen = UmlCom::read_bool();
    _php_contextual_body_indent = UmlCom::read_bool();
}
#endif

#ifdef WITHPYTHON
void UmlBaseOperation::read_python_()
{
    UmlBaseClassMember::read_python_();
    _python_decorators = UmlCom::read_string();
    _python_name_spec = UmlCom::read_string();
    _python_get_set_frozen = UmlCom::read_bool();
    _python_contextual_body_indent = UmlCom::read_bool();
}
#endif

#ifdef WITHIDL
void UmlBaseOperation::read_idl_()
{
    UmlBaseClassMember::read_idl_();
    _idl_oneway = UmlCom::read_bool();
    _idl_name_spec = UmlCom::read_string();
    _idl_get_set_frozen = UmlCom::read_bool();
}
#endif

