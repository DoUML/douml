#ifndef _UMLBASECLASSITEM_H
#define _UMLBASECLASSITEM_H


#include "UmlItem.h"
#include <QByteArray>

//  Mother class of the all the class's items including the class themself
class UmlBaseClassItem : public UmlItem
{
public:
#ifdef WITHCPP
    //  return the C++ declaration

    const QByteArray & cppDecl();

    //  to set the C++ declaration
    //
    // On error return FALSE in C++, produce a RuntimeException in Java
    bool set_CppDecl(const char * s);
#endif

#ifdef WITHJAVA
    //  return the Java defininition

    const QByteArray & javaDecl();

    //  to set the Java definition
    //
    // On error return FALSE in C++, produce a RuntimeException in Java
    bool set_JavaDecl(const char * s);
#endif

#ifdef WITHIDL
    //  return the IDL declaration

    const QByteArray & idlDecl();

    //  set the IDL declaration
    //
    // On error return FALSE in C++, produce a RuntimeException in Java
    bool set_IdlDecl(const char * s);
#endif

    virtual void unload(bool = FALSE, bool = FALSE);

    friend class UmlBaseAttribute;
    friend class UmlBaseOperation;
    friend class UmlBaseRelation;

private:
#ifdef WITHCPP
    QByteArray _cpp_decl;
#endif

#ifdef WITHJAVA
    QByteArray _java_decl;
#endif

#ifdef WITHIDL
    QByteArray _idl_decl;
#endif


protected:
    UmlBaseClassItem(void * id, const QByteArray & n) : UmlItem(id, n) {};

#ifdef WITHCPP
    //internal, do NOT use it

    virtual void read_cpp_();
#endif

#ifdef WITHJAVA
    //internal, do NOT use it

    virtual void read_java_();
#endif

#ifdef WITHIDL
    //internal, do NOT use it

    virtual void read_idl_();
#endif

    friend class UmlBaseClassMember;
    friend class UmlBaseExtraClassMember;
    friend class UmlBaseClass;
};

#endif
