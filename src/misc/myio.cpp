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





#include <QTextStream>
#include <qdir.h>
#include <qfile.h>
#include <qstring.h>
#include <q3canvas.h>
#include <Q3TextEdit>
#include <q3textview.h>
#include <qlayout.h>
#include <qdialog.h>
#include <qlabel.h>
#include <qfont.h>
#include <qworkspace.h>
#include <qstringlist.h>
#include <qapplication.h>
#include <qbuffer.h>
//Added by qt3to4:
#include <Q3VBoxLayout>

#include <Q3PointArray>
#include <QTextCodec>
#include <QFileInfo>

#include "myio.h"
#include "UmlWindow.h"
#include "BrowserView.h"
#include "Labeled.h"
#include "strutil.h"
#include "DialogUtil.h"
#include "mu.h"
#include "UmlGlobal.h"
#include "err.h"
#include "translate.h"
#include "Logging/QsLog.h"

// QFile::baseName() return string before the first '.' rather than the last

QString my_baseName(QFileInfo & fi)
{
    QString fn = fi.fileName();
    int index = fn.findRev('.');

    return (index == -1)
           ? fn
           : fn.left(index);
}

static ReadContext Context;
static bool ReadOnlyForced;

static int Indent;

ReadContext current_context()
{
    return Context;
}

void restore_context(ReadContext & c)
{
    Context = c;
}

bool read_only_file()
{
    return Context.read_only;
}

void set_read_only_file()
{
    Context.read_only = TRUE;
}

void force_read_only(bool y)
{
    ReadOnlyForced = y;
}

static bool OnLoadDiagram;

bool on_load_diagram()
{
    return OnLoadDiagram;
}

void set_on_load_diagram(bool y)
{
    OnLoadDiagram = y;
}


const char * stringify(UmlVisibility v)
{
    //QLOG_INFO() << "Call to stringigy detected";
    switch (v) {
    case UmlPublic:
        return "public";

    case UmlProtected:
        return "protected";

    case UmlPrivate:
        return "private";

    case UmlPackageVisibility:
        return "package";

    case UmlDefaultVisibility:
        return "default";

    default:
        return "WRONG VISIBILITY";
    }
}

const char * stringify(UmlParamDirection d)
{
    switch (d) {
    case UmlInOut:
        return "inout";

    case UmlIn:
        return "in";

    case UmlOut:
        return "out";

    default:
        return "return";
    }
}

const char * stringify(UmlParamEffect e)
{
    switch (e) {
    case UmlNoEffect:
        return "none";

    case UmlCreate:
        return "create";

    case UmlRead:
        return "read";

    case UmlUpdate:
        return "update";

    case UmlDelete:
        return "delete";

    default:
        return "WRONG CODE";
    }
}

const char * stringify(UmlActionKind a)
{
    switch (a) {
    case UmlOpaqueAction:
        return "opaque_action";

    case UmlAcceptEventAction:
        return "accept_event_action";

    case UmlReadVariableValueAction:
        return "read_variable_value_action";

    case UmlClearVariableValueAction:
        return "clear_variable_value_action";

    case UmlWriteVariableValueAction:
        return "write_variable_value_action";

    case UmlAddVariableValueAction:
        return "add_variable_value_action";

    case UmlRemoveVariableValueAction:
        return "remove_variable_value_action";

    case UmlCallBehaviorAction:
        return "call_behavior_action";

    case UmlCallOperationAction:
        return "call_operation_action";

    case UmlSendObjectAction:
        return "send_object_action";

    case UmlSendSignalAction:
        return "send_signal_action";

    case UmlBroadcastSignalAction:
        return "broadcast_signal_action";

    case UmlUnmarshallAction:
        return "unmarshall_action";

    case UmlValueSpecificationAction:
        return "value_specification_action";

    case UmlAcceptCallAction:
        return "accept_call_action";

    case UmlReplyAction:
        return "reply_action";

    case UmlCreateObjectAction:
        return "create_object_action";

    case UmlDestroyObjectAction:
        return "destroy_object_action";

    case UmlTestIdentityAction:
        return "test_identity_action";

    case UmlRaiseExceptionAction:
        return "raise_exception_action";

    case UmlReduceAction:
        return "reduce_action";

    default:
        return "WRONG CODE";
    }
}

const char * stringify(UmlExpansionKind k)
{
    switch (k) {
    case UmlParallel:
        return "parallel";

    case UmlIterative:
        return "iterative";

    case UmlStream:
        return "stream";

    default:
        return "WRONG CODE";
    }
}

const char * stringify(UmlOrderingKind k)
{
    switch (k) {
    case UmlUnordered:
        return "unordered";

    case UmlOrdered:
        return "ordered";

    case UmlLifo:
        return "lifo";

    case UmlFifo:
        return "fifo";

    default:
        return "WRONG CODE";
    }
}


const char * stringify(UmlCode c)
{
    switch (c)
    {
    case UmlAssociation:
        return "----";

    case UmlDirectionalAssociation:
        return "--->";

    case UmlGeneralisation:
    case UmlInherit:
        return "---|>";

    case UmlAggregation:
        return "o---";

    case UmlAggregationByValue:
        return "*---";

    case UmlDirectionalAggregation:
        return "o-->";

    case UmlDirectionalAggregationByValue:
        return "*-->";

    case UmlDependency:
    case UmlDependOn:
        return "-_->";

    case UmlRealize:
        return "-_-|>";

    case UmlAnchor:
        return "-_-_";

    case UmlProvided:
        return "---O";

    case UmlRequired:
        return "---(";

    case UmlInner:
        return "---+";

    case InitialPS:
        return "initial";

    case EntryPointPS:
        return "entry_point";

    case FinalPS:
        return "final";

    case TerminatePS:
        return "terminate";

    case ExitPointPS:
        return "exit_point";

    case DeepHistoryPS:
        return "deep_history";

    case ShallowHistoryPS:
        return "shallow_history";

    case JunctionPS:
        return "junction";

    case ChoicePS:
        return "choice";

    case ForkPS:
        return "fork";

    case JoinPS:
        return "join";

    case InitialAN:
        return "initial_node";

    case FlowFinalAN:
        return "flow_final";

    case ActivityFinalAN:
        return "activity_final";

    case DecisionAN:
        return "decision";

    case MergeAN:
        return "merge";

    case ForkAN:
        return "fork";

    case JoinAN:
        return "join";

    case UmlActivityAction:     // for InfoCanvas
        return "activity action";

    case UmlFlow:		      // for InfoCanvas
        return "flow";

    default:
        return "WRONG CODE";
    }
}

const char * stringify(Uml3States s)
{
    switch (s) {
    case UmlYes:
        return "yes";

    case UmlNo:
        return "no";

    case UmlDefaultState:
        return "default";

    default:
        return "WRONG 3STATES";
    }
}

const char * stringify(UmlColor c)
{
    switch (c) {
    case UmlTransparent:
        return "transparent";

    case UmlWhite:
        return "white";

    case UmlLightYellow:
        return "lightyellow";

    case UmlYellow:
        return "yellow";

    case UmlMediumYellow:
        return "mediumyellow";

    case UmlDarkYellow:
        return "gold";

    case UmlLightBlue:
        return "lightblue";

    case UmlBlue:
        return "blue";

    case UmlLightMediumBlue:
        return "lightmediumblue";

    case UmlMediumBlue:
        return "mediumblue";

    case UmlDarkBlue:
        return "darkblue";

    case UmlLightGreen:
        return "lightgreen";

    case UmlGreen:
        return "green";

    case UmlLightMediumGreen:
        return "lightmediumgreen";

    case UmlMediumGreen:
        return "mediumgreen";

    case UmlDarkGreen:
        return "darkgreen";

    case UmlRed:
        return "red";

    case UmlVeryLightGray:
        return "verylightgray";

    case UmlLightGray:
        return "lightgray";

    case UmlGray:
        return "gray";

    case UmlDarkGray:
        return "darkgray";

    case UmlBlack:
        return "black";

    case UmlLightRed:
        return "lightred";

    case UmlMidRed:
        return "midred";

    case UmlVeryLightOrange:
        return "verylightorange";

    case UmlLightOrange:
        return "lightorange";

    case UmlOrange:
        return "orange";

    case UmlDarkOrange:
        return "darkorange";

    case UmlLightMagenta:
        return "lightmagenta";

    case UmlMagenta:
        return "magenta";

    case UmlMidMagenta:
        return "midmagenta";

    case UmlDarkMagenta:
        return "darkmagenta";

    case UmlDefaultColor:
        return "default";

    default:
        return "WRONG COLOR";
    }
}

const char * stringify(UmlFont f)
{
    switch (f) {
    case UmlNormalFont:
        return "normal";

    case UmlNormalBoldFont:
        return "bold";

    case UmlNormalItalicFont:
        return "italic";

    case UmlNormalBoldItalicFont:
        return "bold_italic";

    case UmlSmallFont:
        return "small";

    case UmlSmallBoldFont:
        return "small_bold";

    case UmlSmallItalicFont:
        return "small_italic";

    case UmlSmallBoldItalicFont:
        return "small_bold_italic";

    case UmlLargeFont:
        return "large";

    case UmlLargeBoldFont:
        return "large_bold";

    case UmlLargeItalicFont:
        return "large_italic";

    case UmlLargeBoldItalicFont:
        return "large_bold_italic";

    case UmlNormalUnderlinedFont:
        return "underlined";

    case UmlNormalStrikeOutFont:
        return "strikeout";

    default:
        return "WRONG FONT";
    }
}

const char * stringify(ClassDrawingMode v)
{
    switch (v) {
    case asClass:
        return "class";

    case asInterface:
        return "interface";

    case asControl:
        return "control";

    case asBoundary:
        return "boundary";

    case asEntity:
        return "entity";

    case asActor:
        return "actor";

    case Natural:
        return "natural";

    case DefaultClassDrawingMode:
        return "default";

    default:
        return "WRONG CLASSDRAWINGMODE";
    }
}

const char * stringify(DrawingLanguage l)
{
    switch (l) {
    case UmlView:
        return "uml";

    case CppView:
        return "c++";

    case JavaView:
        return "java";

    case PhpView:
        return "php";

    case PythonView:
        return "python";

    case IdlView:
        return "idl";

    case DefaultDrawingLanguage:
        return "default";

    default:
        return "WRONG DRAWINGLANGUAGE";
    }
}

const char * stringify(ShowContextMode v)
{
    switch (v) {
    case noContext:
        return "no";

    case umlContext:
        return "uml";

    case namespaceContext:
        return "namespace";

    case javaPackageContext:
        return "java_package";

    case pythonPackageContext:
        return "python_package";

    case moduleContext:
        return "module";

    case DefaultShowContextMode:
        return "default";

    default:
        return "WRONG CLASSVIEWMODE";
    }
}

const char * stringify(CanvasFormat v)
{
    switch (v) {
    case IsoA0:
        return "A0";

    case IsoA1:
        return "A1";

    case IsoA2:
        return "A2";

    case IsoA3:
        return "A3";

    case IsoA4:
        return "A4";

    case IsoA5:
        return "A5";

    case UsA:
        return "A";

    case UsB:
        return "B";

    case UsC:
        return "C";

    case UsD:
        return "D";

    case UsE:
        return "E";

    case UsLetter:
        return "Letter";

    case UsLegal:
        return "Legal";

    case UsTabloid:
        return "Tabloid";

    case IsoA0Landscape:
        return "A0-Landscape";

    case IsoA1Landscape:
        return "A1-Landscape";

    case IsoA2Landscape:
        return "A2-Landscape";

    case IsoA3Landscape:
        return "A3-Landscape";

    case IsoA4Landscape:
        return "A4-Landscape";

    case IsoA5Landscape:
        return "A5-Landscape";

    case UsALandscape:
        return "A-Landscape";

    case UsBLandscape:
        return "B-Landscape";

    case UsCLandscape:
        return "C-Landscape";

    case UsDLandscape:
        return "D-Landscape";

    case UsELandscape:
        return "E-Landscape";

    case UsLetterLandscape:
        return "Letter-Landscape";

    case UsLegalLandscape:
        return "Legal-Landscape";

    case UsLedger:
        return "Ledger";

    default:
        return "WRONG CanvasFormat";
    }
}

const char * stringify(LineGeometry v)
{
    switch (v) {
    case HVGeometry:
        return "HV";

    case HVrGeometry:
        return "HVr";

    case VHGeometry:
        return "VH";

    case VHrGeometry:
        return "VHr";

    case HVHGeometry:
        return "HVH";

    case VHVGeometry:
        return "VHV";

    default:
        return "WRONG LineGeometry";
    }
}

UmlCode arrow_type(const char * s)
{
    if (!strcmp(s, "---|>"))
        return UmlGeneralisation;

    if (!strcmp(s, "----"))
        return UmlAssociation;

    if (!strcmp(s, "--->"))
        return UmlDirectionalAssociation;

    if (!strcmp(s, "-_->"))
        return UmlDependency;

    if (!strcmp(s, "-_-_"))
        return UmlAnchor;

    if (!strcmp(s, "---("))
        return UmlRequired;

    if (!strcmp(s, "---O"))
        return UmlProvided;

    if (!strcmp(s, "---+"))
        return UmlInner;

    msg_critical("Error",
                 Context.filename + " : " + s + " is not a line kind");
    THROW_ERROR 0;
}

UmlCode relation_type(const char * s, bool simple_relation)
{
    if (!strcmp(s, "----"))
        return UmlAssociation;

    if (!strcmp(s, "--->"))
        return UmlDirectionalAssociation;

    if (!strcmp(s, "---|>"))
        return (simple_relation)
               ? UmlInherit
               : UmlGeneralisation;

    if (!strcmp(s, "o---"))
        return UmlAggregation;

    if (!strcmp(s, "*---"))
        return UmlAggregationByValue;

    if (!strcmp(s, "<>-->") || !strcmp(s, "o-->"))
        return UmlDirectionalAggregation;

    if (!strcmp(s, "<#>-->") || !strcmp(s, "*-->"))
        return UmlDirectionalAggregationByValue;

    if (!strcmp(s, "-_->"))
        return (simple_relation)
               ? UmlDependOn
               : UmlDependency;

    if (!strcmp(s, "-_-|>"))
        return UmlRealize;

    msg_critical("Error",
                 Context.filename + " : " + s + " is not a relation kind");
    THROW_ERROR 0;
}

bool IsaSimpleRelation(UmlCode c)
{
    switch (c) {
    case UmlDependOn:
    case UmlInherit:
        return TRUE;

    default:
        return FALSE;
    }
}

UmlVisibility visibility(const char * s)
{
    if (!strcmp(s, "public"))
        return UmlPublic;
    else if (!strcmp(s, "protected"))
        return UmlProtected;
    else if (!strcmp(s, "private"))
        return UmlPrivate;
    else if (!strcmp(s, "package"))
        return UmlPackageVisibility;
    else if (!strcmp(s, "default"))
        return UmlDefaultVisibility;

    msg_critical(TR("Error"),
                 Context.filename + " : " + s + TR(" is not a visibility"));
    THROW_ERROR 0;
}

UmlParamDirection direction(const char * s)
{
    if (!strcmp(s, "inout"))
        return UmlInOut;

    if (!strcmp(s, "in"))
        return UmlIn;

    if (!strcmp(s, "out"))
        return UmlOut;

    if (!strcmp(s, "return"))
        return UmlReturn;

    msg_critical(TR("Error"),
                 Context.filename + " : " + s + TR(" is not a direction"));
    THROW_ERROR 0;
}

UmlParamEffect effect(const char * s)
{
    if (!strcmp(s, "none"))
        return UmlNoEffect;

    if (!strcmp(s, "create"))
        return UmlCreate;

    if (!strcmp(s, "read"))
        return UmlRead;

    if (!strcmp(s, "update"))
        return UmlUpdate;

    if (!strcmp(s, "delete"))
        return UmlDelete;

    msg_critical(TR("Error"),
                 Context.filename + " : " + s + TR(" is not a parameter effect"));
    THROW_ERROR 0;
}

UmlActionKind activity_action_kind(const char * s)
{
    if (!strcmp(s, "opaque_action"))
        return UmlOpaqueAction;

    if (!strcmp(s, "accept_event_action"))
        return UmlAcceptEventAction;

    if (!strcmp(s, "read_variable_value_action"))
        return UmlReadVariableValueAction;

    if (!strcmp(s, "clear_variable_value_action"))
        return UmlClearVariableValueAction;

    if (!strcmp(s, "write_variable_value_action"))
        return UmlWriteVariableValueAction;

    if (!strcmp(s, "add_variable_value_action"))
        return UmlAddVariableValueAction;

    if (!strcmp(s, "remove_variable_value_action"))
        return UmlRemoveVariableValueAction;

    if (!strcmp(s, "call_behavior_action"))
        return UmlCallBehaviorAction;

    if (!strcmp(s, "call_operation_action"))
        return UmlCallOperationAction;

    if (!strcmp(s, "send_object_action"))
        return UmlSendObjectAction;

    if (!strcmp(s, "send_signal_action"))
        return UmlSendSignalAction;

    if (!strcmp(s, "broadcast_signal_action"))
        return UmlBroadcastSignalAction;

    if (!strcmp(s, "unmarshall_action"))
        return UmlUnmarshallAction;

    if (!strcmp(s, "value_specification_action"))
        return UmlValueSpecificationAction;

    if (!strcmp(s, "accept_call_action"))
        return UmlAcceptCallAction;

    if (!strcmp(s, "reply_action"))
        return UmlReplyAction;

    if (!strcmp(s, "create_object_action"))
        return UmlCreateObjectAction;

    if (!strcmp(s, "destroy_object_action"))
        return UmlDestroyObjectAction;

    if (!strcmp(s, "test_identity_action"))
        return UmlTestIdentityAction;

    if (!strcmp(s, "raise_exception_action"))
        return UmlRaiseExceptionAction;

    if (!strcmp(s, "reduce_action"))
        return UmlReduceAction;

    msg_critical(TR("Error"),
                 Context.filename + " : " + s + TR(" is not an activity action kind"));
    THROW_ERROR 0;
}

UmlExpansionKind expansion_mode_kind(const char * s)
{
    if (! strcmp(s, "parallel"))
        return UmlParallel;

    if (! strcmp(s, "iterative"))
        return UmlIterative;

    if (! strcmp(s, "stream"))
        return UmlStream;

    msg_critical(TR("Error"),
                 Context.filename + " : " + s + TR(" is not an expansion node kind"));
    THROW_ERROR 0;
}

UmlOrderingKind ordering(const char * s)
{
    if (! strcmp(s, "unordered"))
        return UmlUnordered;

    if (! strcmp(s, "ordered"))
        return UmlOrdered;

    if (! strcmp(s, "lifo"))
        return UmlLifo;

    if (! strcmp(s, "fifo"))
        return UmlFifo;

    msg_critical(TR("Error"),
                 Context.filename + " : " + s + TR(" is not an ordering"));
    THROW_ERROR 0;
}

Uml3States state(const char * s)
{
    if (!strcmp(s, "yes"))
        return UmlYes;

    if (!strcmp(s, "no"))
        return UmlNo;

    if (!strcmp(s, "default"))
        return UmlDefaultState;

    msg_critical(TR("Error"),
                 Context.filename + " : " + s + TR(" is not a state"));
    THROW_ERROR 0;
}

ClassDrawingMode drawing_mode(const char * s)
{
    if (!strcmp(s, "class"))
        return asClass;

    if (!strcmp(s, "interface"))
        return asInterface;

    if (!strcmp(s, "control"))
        return asControl;

    if (!strcmp(s, "boundary"))
        return asBoundary;

    if (!strcmp(s, "entity"))
        return asEntity;

    if (!strcmp(s, "actor"))
        return asActor;

    if (!strcmp(s, "natural"))
        return Natural;

    if (!strcmp(s, "default"))
        return DefaultClassDrawingMode;

    msg_critical(TR("Error"),
                 Context.filename + " : " + s + TR(" is not a class drawing mode"));
    THROW_ERROR 0;
}

DrawingLanguage drawing_language(const char * s)
{
    if (!strcmp(s, "uml"))
        return UmlView;

    if (!strcmp(s, "c++"))
        return CppView;

    if (!strcmp(s, "java"))
        return JavaView;

    if (!strcmp(s, "php"))
        return PhpView;

    if (!strcmp(s, "python"))
        return PythonView;

    if (!strcmp(s, "idl"))
        return IdlView;

    if (!strcmp(s, "default"))
        return DefaultDrawingLanguage;

    msg_critical(TR("Error"),
                 Context.filename + " : " + s + TR(" is not a drawing language"));
    THROW_ERROR 0;
}

ShowContextMode context_mode(const char * s)
{
    if (!strcmp(s, "no"))
        return noContext;

    if (!strcmp(s, "uml"))
        return umlContext;

    if (!strcmp(s, "namespace"))
        return namespaceContext;

    if (!strcmp(s, "package") || !strcmp(s, "java_package"))
        return javaPackageContext;

    if (!strcmp(s, "python_package"))
        return pythonPackageContext;

    if (!strcmp(s, "module"))
        return moduleContext;

    if (!strcmp(s, "default"))
        return DefaultShowContextMode;

    msg_critical(TR("Error"),
                 Context.filename + " : " + s + TR(" is not a context mode"));
    THROW_ERROR 0;
}

UmlColor color(const char * s)
{
    if (!strcmp(s, "transparent"))
        return UmlTransparent;

    if (!strcmp(s, "white"))
        return UmlWhite;

    if (!strcmp(s, "lightyellow"))
        return UmlLightYellow;

    if (!strcmp(s, "yellow"))
        return UmlYellow;

    if (!strcmp(s, "mediumyellow"))
        return UmlMediumYellow;

    if (!strcmp(s, "gold"))
        return UmlDarkYellow;

    if (!strcmp(s, "lightblue"))
        return UmlLightBlue;

    if (!strcmp(s, "lightmediumblue"))
        return UmlLightMediumBlue;

    if (!strcmp(s, "mediumblue"))
        return UmlMediumBlue;

    if (!strcmp(s, "blue"))
        return UmlBlue;

    if (!strcmp(s, "darkblue"))
        return UmlDarkBlue;

    if (!strcmp(s, "lightgreen"))
        return UmlLightGreen;

    if (!strcmp(s, "green"))
        return UmlGreen;

    if (!strcmp(s, "lightmediumgreen"))
        return UmlLightMediumGreen;

    if (!strcmp(s, "mediumgreen"))
        return UmlMediumGreen;

    if (!strcmp(s, "darkgreen"))
        return UmlDarkGreen;

    if (! strcmp(s, "lightred"))
        return UmlLightRed;

    if (! strcmp(s, "midred"))
        return UmlMidRed;

    if (!strcmp(s, "red"))
        return UmlRed;

    if (!strcmp(s, "verylightgray"))
        return UmlVeryLightGray;

    if (!strcmp(s, "lightgray"))
        return UmlLightGray;

    if (!strcmp(s, "gray"))
        return UmlGray;

    if (!strcmp(s, "darkgray"))
        return UmlDarkGray;

    if (!strcmp(s, "black"))
        return UmlBlack;

    if (! strcmp(s, "verylightorange"))
        return UmlVeryLightOrange;

    if (! strcmp(s, "lightorange"))
        return UmlLightOrange;

    if (! strcmp(s, "orange"))
        return UmlOrange;

    if (! strcmp(s, "darkorange"))
        return UmlDarkOrange;

    if (! strcmp(s, "lightmagenta"))
        return UmlLightMagenta;

    if (! strcmp(s, "magenta"))
        return UmlMagenta;

    if (! strcmp(s, "midmagenta"))
        return UmlMidMagenta;

    if (! strcmp(s, "darkmagenta"))
        return UmlDarkMagenta;

    msg_critical(TR("Error"),
                 Context.filename + " : " + s + TR(" is not a color"));
    THROW_ERROR 0;
}

UmlFont font(const char * s)
{
    if (!strcmp(s, "normal"))
        return UmlNormalFont;

    if (!strcmp(s, "bold"))
        return UmlNormalBoldFont;

    if (!strcmp(s, "italic"))
        return UmlNormalItalicFont;

    if (!strcmp(s, "bold_italic"))
        return UmlNormalBoldItalicFont;

    if (!strcmp(s, "small"))
        return UmlSmallFont;

    if (!strcmp(s, "small_bold"))
        return UmlSmallBoldFont;

    if (!strcmp(s, "small_italic"))
        return UmlSmallItalicFont;

    if (!strcmp(s, "small_bold_italic"))
        return UmlSmallBoldItalicFont;

    if (!strcmp(s, "large"))
        return UmlLargeFont;

    if (!strcmp(s, "large_bold"))
        return UmlLargeBoldFont;

    if (!strcmp(s, "large_italic"))
        return UmlLargeItalicFont;

    if (!strcmp(s, "large_bold_italic"))
        return UmlLargeBoldItalicFont;

    if (!strcmp(s, "underlined"))
        return UmlNormalUnderlinedFont;

    if (!strcmp(s, "strikeout"))
        return UmlNormalStrikeOutFont;

    msg_critical(TR("Error"),
                 Context.filename + " : " + s + TR(" is not a font"));
    THROW_ERROR 0;
}

CanvasFormat canvas_format(const char * s)
{
    if (! strcmp(s, "A0"))
        return IsoA0;

    if (! strcmp(s, "A1"))
        return IsoA1;

    if (! strcmp(s, "A2"))
        return IsoA2;

    if (! strcmp(s, "A3"))
        return IsoA3;

    if (! strcmp(s, "A4"))
        return IsoA4;

    if (! strcmp(s, "A5"))
        return IsoA5;

    if (! strcmp(s, "A"))
        return UsA;

    if (! strcmp(s, "B"))
        return UsB;

    if (! strcmp(s, "C"))
        return UsC;

    if (! strcmp(s, "D"))
        return UsD;

    if (! strcmp(s, "E"))
        return UsE;

    if (! strcmp(s, "Letter"))
        return UsLetter;

    if (! strcmp(s, "Legal"))
        return UsLegal;

    if (! strcmp(s, "Tabloid"))
        return UsTabloid;

    if (! strcmp(s, "A0-Landscape"))
        return IsoA0Landscape;

    if (! strcmp(s, "A1-Landscape"))
        return IsoA1Landscape;

    if (! strcmp(s, "A2-Landscape"))
        return IsoA2Landscape;

    if (! strcmp(s, "A3-Landscape"))
        return IsoA3Landscape;

    if (! strcmp(s, "A4-Landscape"))
        return IsoA4Landscape;

    if (! strcmp(s, "A5-Landscape"))
        return IsoA5Landscape;

    if (! strcmp(s, "A-Landscape"))
        return UsALandscape;

    if (! strcmp(s, "B-Landscape"))
        return UsBLandscape;

    if (! strcmp(s, "C-Landscape"))
        return UsCLandscape;

    if (! strcmp(s, "D-Landscape"))
        return UsDLandscape;

    if (! strcmp(s, "E-Landscape"))
        return UsELandscape;

    if (! strcmp(s, "Letter-Landscape"))
        return UsLetterLandscape;

    if (! strcmp(s, "Legal-Landscape"))
        return UsLegalLandscape;

    if (! strcmp(s, "Ledger"))
        return UsLedger;

    msg_critical(TR("Error"),
                 Context.filename + " : " + s + TR(" is not a canvas format"));
    THROW_ERROR 0;
}

UmlCode pseudo_state_kind(const char * s)
{
    if (! strcmp(s, "initial"))
        return InitialPS;

    if (! strcmp(s, "entry_point"))
        return EntryPointPS;

    if (! strcmp(s, "final"))
        return FinalPS;

    if (! strcmp(s, "terminate"))
        return TerminatePS;

    if (! strcmp(s, "exit_point"))
        return ExitPointPS;

    if (! strcmp(s, "deep_history"))
        return DeepHistoryPS;

    if (! strcmp(s, "shallow_history"))
        return ShallowHistoryPS;

    if (! strcmp(s, "junction"))
        return JunctionPS;

    if (! strcmp(s, "choice"))
        return ChoicePS;

    if (! strcmp(s, "fork"))
        return ForkPS;

    if (! strcmp(s, "join"))
        return JoinPS;

    msg_critical(TR("Error"),
                 Context.filename + " : " + s + TR(" is not a pseudo state kind"));
    THROW_ERROR 0;
}

UmlCode activity_node_kind(const char * s)
{
    if (! strcmp(s, "initial"))
        // old releases
        return InitialAN;

    if (! strcmp(s, "initial_node"))
        return InitialAN;

    if (! strcmp(s, "flow_final"))
        return FlowFinalAN;

    if (! strcmp(s, "activity_final"))
        return ActivityFinalAN;

    if (! strcmp(s, "decision"))
        return DecisionAN;

    if (! strcmp(s, "merge"))
        return MergeAN;

    if (! strcmp(s, "fork"))
        return ForkAN;

    if (! strcmp(s, "join"))
        return JoinAN;

    msg_critical(TR("Error"),
                 Context.filename + " : " + s + TR(" is not an activity node kind"));
    THROW_ERROR 0;
}

LineGeometry line_geometry(const char * s)
{
    if (! strcmp(s, "HV"))
        return HVGeometry;

    if (! strcmp(s, "HVr"))
        return HVrGeometry;

    if (! strcmp(s, "VH"))
        return VHGeometry;

    if (! strcmp(s, "VHr"))
        return VHrGeometry;

    if (! strcmp(s, "HVH"))
        return HVHGeometry;

    if (! strcmp(s, "VHV"))
        return VHVGeometry;

    msg_critical(TR("Error"),
                 Context.filename + " : " + s + TR(" is not a line geometry"));
    THROW_ERROR 0;
}


//

void set_read_file_format(unsigned u)
{
    if(api_format(true) == 75)
        u = 75;
    Context.read_file_format = u;
}

unsigned read_file_format()
{
    return Context.read_file_format;
}

//

static char EmptyString[] = { 0 };
static char UnvalidString[] = { '?', '?', '?', 0 };

static void set_previous_word(char * s)
{
    if (Context.previous_word_allocated) {
        Context.previous_word_allocated = FALSE;
        delete [] Context.previous_word;
    }

    Context.previous_word = s;
}

void backup(QDir & d, QString fn)
{
    QString s;

    s.sprintf("_%u.bak", user_id());

    while (d.exists(fn) &&
           !d.rename(fn, fn + s) &&
           (msg_warning("Uml",
                        d.absFilePath(fn) +
                        TR("\ncannot be renamed ") + fn + s +
                        TR(", retry ?\n"),
                        QMessageBox::Yes, QMessageBox::No)
            == QMessageBox::Yes))
        ;
}

void delete_backup(QDir & d)
{
    QString s;

    s.sprintf("*_%u.bak", user_id());

    //[lgfreitas] entryInfoList des not return a pointer anymore
    QFileInfoList l = d.entryInfoList(s);

    if (!l.empty()) {
        //Q3PtrListIterator<QFileInfo> it(l);
        QFileInfoList::iterator it = l.begin(); //[lgfreitas] above iterator was not working.
        //QFileInfo fi;

        while (it != l.end())  {
            QFile::remove((*it).absFilePath());
            it++;
        }
    }
}

int open_file(QFile & fp, int mode, bool silent)
{
    Context.filename = fp.name();

    while (! fp.open(QFlag(mode))) { //[lgfreitas] it does not accept a int anymore, but a QFlag. Hopefully, this does the trick
        if (silent ||
            (msg_critical("Uml",
                          Context.filename +
                          ((mode != QIODevice::ReadOnly)
                           ? TR("\ncannot be opened for write, retry ?\n")
                           : TR("\ncannot be opened for read, retry ?\n")),
                          QMessageBox::Yes, QMessageBox::No)
             != QMessageBox::Yes))
            return -1;
    }

    if (mode == QIODevice::ReadOnly) {
        set_previous_word(EmptyString);
        Context.linenum = 1;
        Context.removed_char = ' ';
    }

    QFileInfo fi(fp);

    Context.read_only = !fi.isWritable() || ReadOnlyForced;
    return fi.size();
}

bool copy_file(QFileInfo * src, const QDir & dest)
{
    QString src_path = src->absFilePath();
    FILE * fp_src;

    if ((fp_src = fopen((const char *) src_path, "rb")) == 0) {
        msg_critical("Error", TR("cannot open %1 to read it", src_path));
        return FALSE;
    }

    QString dest_path = dest.absFilePath(src->fileName());
    FILE * fp_dest;

    if ((fp_dest = fopen((const char *) dest_path, "wb")) == 0) {
        msg_critical("Error", TR("cannot create %1", dest_path));
        fclose(fp_src);
        return FALSE;
    }

    char b[4096];
    size_t n;

    while ((n = fread(b, 1, sizeof(b) - 1, fp_src)) > 0) {
        if (fwrite(b, 1, n, fp_dest) != n) {
            msg_critical("Error", TR("error when writing %1", dest_path));
            fclose(fp_src);
            fclose(fp_dest);
            return FALSE;
        }
    }

    if (! feof(fp_src)) {
        msg_critical("Error", TR("error when reading %1", src_path));
        fclose(fp_src);
        fclose(fp_dest);
        return FALSE;
    }

    fclose(fp_src);
    fclose(fp_dest);
    return TRUE;
}

void save_if_needed(const char * filename, QSharedPointer<QByteArray> newdef)
{
    QDir d = BrowserView::get_dir();
    QString path = d.absFilePath(filename);
    FILE * fp = fopen((const char *) path, "rb");
    bool needed;

    if (fp == 0)
        needed = TRUE;
    else {
        int c;
        const char * new_contains = newdef->constData();

        needed = FALSE;

        while ((c = fgetc(fp)) != EOF) {
            if (c != *new_contains++) {
                needed = TRUE;
                break;
            }
        }

        fclose(fp);

        needed |= (*new_contains != 0);
    }

    if (needed) {
        backup(d, filename);

        for (;;) {
            while ((fp = fopen((const char *) path, "wb")) == 0)
                (void) msg_critical("Error", TR("Cannot create file\n") + path,
                                    QMessageBox::Retry);

            QLOG_INFO() << newdef.data();

            if (!newdef->isEmpty()) {
                if (fputs(newdef->constData(), fp) < 0) {
                    fclose(fp);
                    (void) msg_critical(TR("Error"),
                                        TR("Error while writting in\n%1\nmay be your disk is full", path),
                                        QMessageBox::Retry);
                }
                else
                    // ok
                    break;
            }
            else
                // ok
                break;
        }

        fclose(fp);
    }
}

char * read_file(QString filename)
{
    filename = (in_import())
               ? BrowserView::get_import_dir().absFilePath(filename)
               : BrowserView::get_dir().absFilePath(filename);

    QFile fp(filename);
    int size;

    if ((size = open_file(fp, QIODevice::ReadOnly, TRUE)) != -1) {
        char * s = new char[size + 1];

        if (fp.readBlock(s, size) == -1) {
            msg_critical(TR("Error"), TR("%1 cannot be read", filename));
            delete [] s;
            return 0;
        }
        else {
            s[size] = 0;

            return s;
        }
    }
    else
        return 0;
}

QString abs_file_path(int id, const char * ext)
{
    QString s;

    if (ext[1] == 0)
        s.sprintf("%d_%d.%c", id,
                  root_permission() ? root_permission() : user_id(),
                  *ext);
    else
        s.sprintf("%d.%s", id, ext);

    return BrowserView::get_dir().absFilePath(s);
}

char * read_definition(int id, const char * ext)
{
    QString s;
    char * r;

    // try to read modified definition

    s.sprintf("%d_%d.%c", id,
              root_permission() ? root_permission() : user_id(),
              *ext);

    if ((r = read_file(s)) != 0)
        return r;

    if (ext[1] == 0)
        // want the modified definition
        return 0;

    // try to read saved definition
    s.sprintf("%d.%s", id, ext);

    return read_file(s);
}

char * read_file(QString filename, int offset, int len)
{
    filename = BrowserView::get_dir().absFilePath(filename);

    QFile fp(filename);

    if (open_file(fp, QIODevice::ReadOnly, TRUE) != -1) {
        char * s = new char[len + 1];

        fp.at(offset);

        if (fp.readBlock(s, len) == -1) {
            msg_critical(TR("Error"), TR("%1 cannot be read", filename));
            delete [] s;
            return 0;
        }
        else {
            s[len] = 0;

            return s;
        }
    }
    else
        return 0;
}

char * read_definition(int id, const char * ext, int offset, int len)
{
    if (len == 0)
        return 0;

    QString s;
    char * r;

    // try to read modified definition

    s.sprintf("%d_%d.%c", id,
              root_permission() ? root_permission() : user_id(),
              *ext);

    if ((r = read_file(s, offset, len)) != 0)
        return r;

    if (ext[1] == 0)
        // want the modified definition
        return 0;

    // try to read saved definition
    s.sprintf("%d.%s", id, ext);

    return read_file(s, offset, len);
}

void save_definition(int id, const char * ext, const char * def,
                     BooL & is_new)
{
    QString s;
    FILE * fp;
    QDir d = BrowserView::get_dir();

    if (ext[1] != 0) {
        // saved definition
        s.sprintf("%d.%s", id, ext);

        if (is_new)
            is_new = FALSE;
        else
            backup(d, s);
    }
    else
        // modified definition
        s.sprintf("%d_%d.%s", id,
                  root_permission() ? root_permission() : user_id(),
                  ext);

    s = d.absFilePath(s);

    for (;;) {
        while ((fp = fopen((const char *) s, "wb")) == 0)
            (void) msg_critical(TR("Error"),
                                TR("Cannot create file\n") + s,
                                QMessageBox::Retry);

        if ((def != 0) && *def) {
            if (fputs(def, fp) < 0) {
                fclose(fp);
                (void) msg_critical(TR("Error"),
                                    TR("Cannot save definition in\n%1\nmay be your disk is full",
                                       s),
                                    QMessageBox::Retry);
            }
            else
                // ok
                break;
        }
        else
            // ok
            break;
    }

    fclose(fp);
}

void delete_definition(int id, const char * ext)
{
    QDir d = BrowserView::get_dir();
    QString s;

    if (ext[1] != 0) {
        // saved definition
        s.sprintf("%d.%s", id, ext);
        backup(d, s);
    }
    else {
        // modified definition
        s.sprintf("%d_%d.%s", id, user_id(), ext);
        QFile::remove(d.absFilePath(s));
    }
}

void read_in(const QString & s)
{
    set_previous_word(EmptyString);
    Context.linenum = 1;
    Context.removed_char = ' ';
    Context.filename = s;
}

static QString where()
{
    return Context.filename + TR(" line ") + QString::number(Context.linenum) + "\n";
}

//

void save_string(const char * p, QTextStream & st)
{
    if ((p == 0) || (*p == 0))
        st << "\"\"";
    else {
        st << '"';

        for (;;) {
            const char * p2 = strchr(p, '"');
            const char * p3 = strchr(p, '\\');

            if ((p2 != 0) || (p3 != 0)) {
                if ((p2 == 0) || ((p3 != 0) && (p3 < p2)))
                    p2 = p3;

                // do NOT use writeRawBytes !
                while (p != p2)
                {

                    QString temp1 = QTextCodec::codecForLocale()->toUnicode(p).left(1);
                    int size = QTextCodec::codecForLocale()->fromUnicode(temp1).size();
                    st << temp1;
                    p+=size;


                }

                st << '\\';
                QString temp1 = QTextCodec::codecForLocale()->toUnicode(p).left(1);
                int size =  QTextCodec::codecForLocale()->fromUnicode(temp1).size();
                p+=size;
                st << temp1;


                if (*p == 0)
                    break;
            }
            else {
                st << p;
                break;
            }
        }

        st << '"';
    }
}

void save_string_list(QStringList & list, QTextStream & st)
{
    st << " " << list.count();

    for (QStringList::Iterator it = list.begin();
         it != list.end();
         ++it) {
        st << " ";
        save_string(*it, st);
    }
}

void save_unicode_string_list(QStringList & list, QTextStream & st)
{
    st << " " << list.count();

    for (QStringList::Iterator it = list.begin();
         it != list.end();
         ++it) {
        st << " ";
        save_string(fromUnicode(*it), st);
    }
}

void nl_indent(QTextStream & st)
{
    int i = Indent;

    st << '\n';

    while (i >= 4) {
        st << '\t';
        i -= 4;
    }

    static const char * const ss[] = { "", "  ", "    ", "      " };

    st << ss[i];
}

void indent(int d)
{
    Indent += d;
}

int indent()
{
    return Indent;
}

void indent0()
{
    Indent = 0;
}

//

static void skip_whitespace_comment(char *& s)
{
    if (Context.removed_char != ' ')
        *--s = Context.removed_char;

    for (;;) {
        switch (*s) {
        case '\n':
            Context.linenum += 1;

        case ' ' :
        case '\r':
        case '\t':
            s += 1;
            break;

        case '/':
            for (s += 2; *s != '\n'; s += 1)
                if (*s == 0)
                    return;

            Context.linenum += 1;
            s += 1;
            break;

        default:
            return;
        }
    }
}

bool at_end(char *& st)
{
    skip_whitespace_comment(st);

    return (*st == 0);
}

char * read_string(char *& st)
{
    skip_whitespace_comment(st);

    if (*st == 0) {
        msg_critical(TR("Error"),
                     where() + TR("premature end of file, string expected"));
        THROW_ERROR 0;
    }

    if (*st != '"') {
        msg_critical(TR("Error"),
                     where() + TR("string expected after '%1'",
                                  Context.previous_word));
        THROW_ERROR 0;
    }

    char * r = st + 1;
    char * p = r;

    for (;;) {
        switch (*++st) {
        case 0:
            msg_critical(TR("Error"),
                         where() + TR("premature end of file reading a string"));
            THROW_ERROR 0;

        case '"':
            Context.removed_char = ' ';
            *st++ = 0;
            *p = 0;

            if (Context.previous_word_allocated)
                delete [] Context.previous_word;
            else
                Context.previous_word_allocated = TRUE;

            Context.previous_word = new char[p - r + 1];
            memcpy(Context.previous_word, r, p - r + 1);
            return r;

        case '\\':
            if (*++st  == 0) {
                msg_critical(TR("Error"),
                             where() + TR("premature end of file reading a string"));
                THROW_ERROR 0;
            }

            *p++ = *st;
            break;

        case '\r':
            break;

        case '\n':
            Context.linenum += 1;

            // no break
        default:
            *p++ = *st;
        }
    }
}

char * read_keyword(char *& st)
{
    skip_whitespace_comment(st);

    if (*st == 0) {
        msg_critical(TR("Error"),
                     where() + TR("premature end of file, keyword expected"));
        THROW_ERROR 0;
    }

    if (*st == '"') {
        msg_critical(TR("Error"),
                     where() +
                     TR("keyword expected after '%1'",
                        Context.previous_word));
        THROW_ERROR 0;
    }

    char * r = st;
    char c;

    do
        c = *++st;

    while ((c > ' ') && (c != '"'));

    Context.removed_char = c;
    *st++ = 0;
    set_previous_word(r);
    return r;
}

char * read_keyword(char *& st, const char * expected)
{
    skip_whitespace_comment(st);

    if (*st == 0) {
        msg_critical(TR("Error"),
                     where() +
                     TR("premature end of file, '%1' expected",
                        expected));
        THROW_ERROR 0;
    }

    if (*st == '"') {
        msg_critical("Error",
                     where() +
                     TR("'%1' expected rather than a string after '%2'",
                        expected, Context.previous_word));
        THROW_ERROR 0;
    }

    char * r = st;
    char c;

    do
        c = *++st;

    while ((c > ' ') && (c != '"'));

    *st++ = 0;

    if (strcmp(r, expected)) {
        msg_critical("Error",
                     where() +
                     TR("'%1' expected rather than '%2' after '%3'",
                        expected, r, + Context.previous_word));
        THROW_ERROR 0;
    }

    set_previous_word(r);
    Context.removed_char = c;

    return r;
}

// for old versions
char * read_keyword(char *& st, const char * expected1,
                    const char * expected2)
{
    skip_whitespace_comment(st);

    if (*st == 0) {
        msg_critical("Error",
                     where() +
                     TR("premature end of file, '%1' expected" ,
                        expected1));
        THROW_ERROR 0;
    }

    if (*st == '"') {
        msg_critical("Error",
                     where() +
                     TR("'%1' expected rather than a string after '%2'",
                        expected1, Context.previous_word));
        THROW_ERROR 0;
    }

    char * r = st;
    char c;

    do
        c = *++st;

    while ((c > ' ') && (c != '"'));

    *st++ = 0;

    if (strcmp(r, expected1) && strcmp(r, expected2)) {
        msg_critical("Error",
                     where() +
                     TR("'%1' expected rather than '%2' after '%3'",
                        expected1, r, Context.previous_word));
        THROW_ERROR 0;
    }

    set_previous_word(r);
    Context.removed_char = c;

    return r;
}

void unread_keyword(char * keyword, char *& st)
{
    // not at the beginning !
    st[-1] = Context.removed_char;
    Context.removed_char = ' ';

    st = keyword;
    set_previous_word(UnvalidString);	// to free Context.previous_word if necessary
}


int read_id(char *& st)
{
    skip_whitespace_comment(st);

    if (*st == 0) {
        msg_critical("Error",
                     where() + TR("premature end of file, id expected"));
        THROW_ERROR 0;
    }

    if (*st == '"') {
        msg_critical("Error",
                     where() + TR("id expected after '%1'", Context.previous_word));
        THROW_ERROR 0;
    }

    char * r = st;
    int result = (int) strtol(r, &st, 10);

    if (st == r) {
        msg_critical(TR("Error"),
                     where() + TR("id expected after '%1'", Context.previous_word));
        THROW_ERROR 0;
    }

    Context.removed_char = *st;
    *st++ = 0;
    set_previous_word(r);

    return result;
}

unsigned read_unsigned(char *& st)
{
    skip_whitespace_comment(st);

    if (*st == 0) {
        msg_critical(TR("Error"),
                     where() + TR("premature end of file, unsigned expected"));
        THROW_ERROR 0;
    }

    if (*st == '"') {
        msg_critical(TR("Error"),
                     where() + TR("unsigned expected after '%1'", Context.previous_word));
        THROW_ERROR 0;
    }

    int result = 0;
    char * r = st;
    char c;

    while (((c = *st++) >= '0') && (c <= '9'))
        result = result * 10 + c - '0';

    if (st == r + 1) {
        msg_critical(TR("Error"),
                     where() + TR("unsigned expected after '%1'", Context.previous_word));
        THROW_ERROR 0;
    }

    st[-1] = 0;
    set_previous_word(r);
    Context.removed_char = c;

    return result;
}

double read_double(char *& st)
{
    skip_whitespace_comment(st);

    if (*st == 0) {
        msg_critical(TR("Error"),
                     where() + TR("premature end of file, float expected"));
        THROW_ERROR 0;
    }

    if (*st == '"') {
        msg_critical(TR("Error"),
                     where() + TR("float expected after '%1'", Context.previous_word));
        THROW_ERROR 0;
    }

    char * r = st;
    QString s;
    char c;

    while (((c = *st++) > ' ') && (c != '"'))
        s += c;

    bool ok;
    double result = s.toDouble(&ok);

    if (!ok) {
        // '.' or ',' problem ?
        int index = s.find(',');

        if (index != -1) {
            s.replace(index, 1, ".");
            result = s.toDouble(&ok);
        }
        else if ((index = s.find('.')) != -1) {
            s.replace(index, 1, ",");
            result = s.toDouble(&ok);
        }

        if (!ok) {
            msg_critical(TR("Error"),
                         where() +
                         TR("float expected rather than '%1' after '%2'",
                            s, Context.previous_word));
            THROW_ERROR 0;
        }
    }

    Context.removed_char = c;
    st[-1] = 0;
    set_previous_word(r);

    return result;
}

void read_string_list(QStringList & list, char *& st)
{
    int n = read_unsigned(st);

    list.clear();

    while (n--)
        list.append(read_string(st));
}

void read_unicode_string_list(QStringList & list, char *& st)
{
    int n = read_unsigned(st);

    list.clear();

    while (n--)
        list.append(toUnicode(read_string(st)));
}

char * skip_until(char *& st, const char * expected)
{
    char * k;

    for (;;) {
        skip_whitespace_comment(st);

        if (*st == 0) {
            msg_critical(TR("Error"),
                         where() + TR("premature end of file"));
            THROW_ERROR 0;
        }

        if (*st == '"')
            read_string(st);
        else if (strcmp((k = read_keyword(st)), expected) == 0)
            return k;
    }
}

void wrong_keyword(const char * k, const char * expected)
{
    msg_critical(TR("Error"),
                 where() + TR("'%1' expected rather than '%2'", expected, k));
    THROW_ERROR 0;
}

void unknown_keyword(const char * k)
{
    msg_critical(TR("Error"),
                 where() + TR("unknown or unexpected keyword '%1'", k));
    THROW_ERROR 0;
}

void unknown_ref(const char * kind, int id)
{
    msg_critical(TR("Error"),
                 where() +
                 TR("%1 unknown %2 identifier",
                    QString::number(id), QString(kind)));
    THROW_ERROR 0;
}

//

void save_xy(QTextStream & st, const Q3CanvasItem * c, const char * s)
{
#ifdef FORCE_INT_COORD
    st << s << " " << (int) c->x() << " " << (int) c->y();
#else
    QString xs, ys;

    st << s << " " << xs.setNum(c->x()) << " " << ys.setNum(c->y());
#endif
}

void save_xyz(QTextStream & st, const Q3CanvasItem * c, const char * s)
{
#ifdef FORCE_INT_COORD
    st << s << " " << (int) c->x() << " " << (int) c->y() << " " << (int) c->z();
#else
    QString xs, ys, zs;

    st << s << " " << xs.setNum(c->x()) << " " << ys.setNum(c->y()) << " " << zs.setNum(c->z());
#endif
}

void save_xyzwh(QTextStream & st, const Q3CanvasRectangle * c, const char * s)
{
#ifdef FORCE_INT_COORD
    st << s << " " << (int) c->x() << " " << (int) c->y() << " " << (int) c->z()
       << " " << (int) c->width() << " " << (int) c->height();
#else
    QString xs, ys, zs;

    st << s << " " << xs.setNum(c->x()) << " " << ys.setNum(c->y()) << " " << zs.setNum(c->z())
       << " " << (int) c->width() << " " << (int) c->height();
#endif
}

void read_xy(char *& st, Q3CanvasItem * c)
{
    double px = read_double(st);
    double py = read_double(st);

    c->moveBy(px - c->x(), py - c->y());
}

void bypass_xy(char *& st)
{
    read_double(st);
    read_double(st);
}

void read_xyz(char *& st, Q3CanvasItem * c)
{
    double px = read_double(st);
    double py = read_double(st);

    c->moveBy(px - c->x(), py - c->y());
    c->setZ(read_double(st));
}

void read_xyzwh(char *& st, Q3CanvasRectangle * c)
{
    double px = read_double(st);
    double py = read_double(st);

    c->moveBy(px - c->x(), py - c->y());
    c->setZ(read_double(st));

    double w = read_double(st);

    c->setSize((int) w, (int) read_double(st));
}

void read_zwh(char *& st, Q3CanvasRectangle * c)
{
    c->setZ(read_double(st));

    double w = read_double(st);

    c->setSize((int) w, (int) read_double(st));
}

void save_color(QTextStream & st, const char * s, UmlColor c, BooL & nl)
{
    if (c != UmlDefaultColor) {
        if (!nl) {
            nl_indent(st);
            nl = TRUE;
        }

        st << s << " " << stringify(c) << " ";
    }
}

void read_color(char *& st, const char * key, UmlColor & c, char *& k)
{
    if (!strcmp(k, key)) {
        c = color(read_keyword(st));

        if ((Context.read_file_format < 23) && (c == UmlWhite))
            c = UmlTransparent;

        k = read_keyword(st);
    }
}

void read_font(char *& st, const char * key, UmlFont & f, char *& k)
{
    if (!strcmp(k, key)) {
        f = font(read_keyword(st));
        k = read_keyword(st);
    }
    else
        f = UmlNormalFont;
}

//

void warn(const QString & s)
{
    // not showed dialog to compute needed size

    QDialog d_aux;
    Q3VBoxLayout * vbox_aux = new Q3VBoxLayout(&d_aux);

    vbox_aux->setMargin(5);
    Q3TextEdit * e = new Q3TextEdit(&d_aux);
    e->setText(s);

    // showed dialog

    QDialog * d = new QDialog;

    d->setCaption("Uml");

    Q3VBoxLayout * vbox = new Q3VBoxLayout(d);

    vbox->setMargin(5);

    Q3TextView * t = new Q3TextView(d);
    QFontMetrics fm(QApplication::font());
    //int w = e->maxLineWidth() + e->minimumSizeHint().width();
    int maxw = (UmlWindow::get_workspace()->width() * 4) / 5;
    int maxh = (UmlWindow::get_workspace()->height() * 4) / 5;
    int he = (e->lines() + 5) * fm.height();

    t->setText(s);
    t->setMinimumSize(maxw, //(w > maxw) ? maxw : w,
                      (he > maxh) ? maxh : he);

    vbox->addWidget(t);

    d->show();
}

// history save/load

void save(const DiagramItem * di, QBuffer & b)
{
    b.writeBlock((char *) &di, sizeof(di));
}

DiagramItem * load_item(QBuffer & b)
{
    DiagramItem * di;

    b.readBlock((char *) &di, sizeof(di));
    return di;
}

void save_ptr(const void * ptr, QBuffer & b)
{
    b.writeBlock((char *) &ptr, sizeof(ptr));
}

void * load_ptr(QBuffer & b)
{
    void * ptr;

    b.readBlock((char *) &ptr, sizeof(ptr));
    return ptr;
}

void save(const QPoint & p, QBuffer & b)
{
    int i;

    i = p.x();
    b.writeBlock((char *) &i, sizeof(i));
    i = p.y();
    b.writeBlock((char *) &i, sizeof(i));
}

void load(QPoint & p, QBuffer & b)
{
    int i;

    b.readBlock((char *) &i, sizeof(i));
    p.setX(i);

    b.readBlock((char *) &i, sizeof(i));
    p.setY(i);
}

void save(const Q3PointArray & a, QBuffer & b)
{
    const QPoint * p = a.data();

    for (int i = 0; i != a.size(); i += 1)
        save(*p++, b);
}

void load(Q3PointArray & a, QBuffer & b)
{
    QPoint * p = a.data();

    for (int i = 0; i != a.size(); i += 1)
        load(*p++, b);
}

void save(double d, QBuffer & b)
{
    b.writeBlock((char *) &d, sizeof(d));
}

void load(double & d, QBuffer & b)
{
    b.readBlock((char *) &d, sizeof(d));
}

double load_double(QBuffer & b)
{
    double d;

    b.readBlock((char *) &d, sizeof(d));
    return d;
}

void save(float f, QBuffer & b)
{
    b.writeBlock((char *) &f, sizeof(f));
}

void load(float & f, QBuffer & b)
{
    b.readBlock((char *) &f, sizeof(f));
}

double load_float(QBuffer & b)
{
    float f;

    b.readBlock((char *) &f, sizeof(f));
    return f;
}

void save(int i, QBuffer & b)
{
    b.writeBlock((char *) &i, sizeof(i));
}

void load(int & i, QBuffer & b)
{
    b.readBlock((char *) &i, sizeof(i));
}

// svg

static FILE * svg_fp;
static int pict_height;

bool start_svg(const char * f, int w, int h)
{
    svg_fp = fopen(f, "w");

    if (svg_fp == 0)
        return FALSE;

    (void) fprintf(svg_fp,
                   "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n"
                   "<!-- Created with Douml (http://sourceforge.net/projects/douml/) -->\n"
                   "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n"
                   "<svg width=\"%d\" height=\"%d\" version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\">\n",
                   w, h);

    pict_height = h;

    return TRUE;
}

void end_svg()
{
    (void) fputs("</svg>\n", svg_fp);

    fclose(svg_fp);
    svg_fp = 0;
}

FILE * svg()
{
    return svg_fp;
}

int svg_height()
{
    return pict_height;
}

const char * svg_color(UmlColor c)
{
    static const char * svg[UmlDefaultColor];
    const char * r = svg[c];

    if (r == 0) {
        if (c == UmlTransparent)
            r = svg[c] = "none";
        else {
            QColor col = color(c);

            r = svg[c] = new char[16];
            sprintf((char *) r, "#%06x", col.rgb() & 0xffffff);
        }
    }

    return r;
}

void draw_poly(FILE * fp, Q3PointArray & poly, UmlColor color, bool stroke)
{
    (void) fprintf(fp, (stroke) ? "\t<polygon fill=\"%s\" stroke=\"black\" stroke-opacity=\"1\""
                   : "\t<polygon fill=\"%s\" stroke=\"none\"",
                   svg_color(color));

    const char * sep = " points=\"";
    int n = poly.size();
    int i;

    for (i = 0; i != n; i += 1) {
        (void) fprintf(fp, "%s%d,%d", sep, poly.point(i).x(), poly.point(i).y());
        sep = " ";
    }

    (void) fputs("\" />\n", fp);
}

void draw_shadow(FILE * fp, Q3PointArray & poly)
{
    (void) fprintf(fp, "\t<polygon fill=\"#%06x\" stroke=\"none\"",
                   QColor(::Qt::darkGray).rgb() & 0xffffff);

    const char * sep = " points=\"";
    int n = poly.size();
    int i;

    for (i = 0; i != n; i += 1) {
        (void) fprintf(fp, "%s%d,%d", sep, poly.point(i).x(), poly.point(i).y());
        sep = " ";
    }

    (void) fputs("\" />\n", fp);
}

void draw_text(const QRect & r, int align, QString s, const QFont & fn, FILE * fp)
{
    draw_text(r.left(), r.top(), r.width(), r.height(),
              align, s, fn, fp);
}

static void xml_text(FILE * fp, QString s)
{
    WrapperStr cs = s.utf8();
    const char * p = cs;

    for (;;) {
        char c = *p;

        switch (c) {
        case 0:
            return;

        case '<':
            (void) fputs("&lt;", fp);
            break;

        case '>':
            (void) fputs("&gt;", fp);
            break;

        case '"':
            (void) fputs("&quot;", fp);
            break;

        case '&':
            (void) fputs("&amp;", fp);
            break;

        default:
            (void) fputc(c, fp);
        }

        p += 1;
    }
}

void draw_text(int x, int y, int w, int h, int align,
               QString s, const QFont & fn, FILE * fp,
               UmlColor fg, UmlColor bg)
{
    if (s.isEmpty())
        return;

    if (bg != UmlTransparent)
        (void) fprintf(fp, "\t<rect fill=\"%s\" stroke=\"none\" stroke-opacity=\"1\""
                       " x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" />\n",
                       svg_color(bg), x, y, w, h);

    if (fg == UmlTransparent)
        fg = UmlBlack;

    int ps = fn.pixelSize();
    int pts = fn.pointSize();
    char header[256];

    sprintf(header, "\t<text font-family=\"%s\" font-size=\"%d\" fill=\"%s\" xml:space=\"preserve\"",
            (const char *) fn.family(),
            // decrease size to help to have enough area
            (ps == -1) ? /* Qt3 */ pts : ps - 1,
            svg_color(fg));

    if (ps == -1)
        /* Qt3 */
        ps = pts;

    if (fn.bold())
        strcat(header, " font-weight=\"bold\"");

    if (fn.italic())
        strcat(header, " font-style=\"italic\"");

    if (fn.underline())
        strcat(header, " text-decoration=\"underline\"");

    bool wb = align & ::Qt::TextWordWrap;

    if ((s.find('\n') == -1) && !wb) {
        // one line
        (void) fputs(header, fp);

        if ((align & ::Qt::AlignHCenter) != 0) {
            (void) fputs(" text-anchor=\"middle\"", fp);
            x += w / 2;
        }

        if ((align & ::Qt::AlignVCenter) != 0)
            y += (h + ps) / 2;
        else
            y += ps;

        (void) fprintf(fp, " x=\"%d\" y=\"%d\">", x, y);
        xml_text(fp, s);
        (void) fputs("</text>\n", fp);
    }
    else {
        // several lines
        // remove all \r, remove last \n and extract lines
        int index = s.length();
        QChar lf('\n');

        while ((index = s.find('\r')) != -1)
            s.remove(index, 1);

        index = s.length();

        while ((index != 0) && s.at(index - 1) == lf)
            index -= 1;

        if (index == 0)
            return;

        s.truncate(index);

        QStringList l = QStringList::split(lf, s, TRUE);

        // cut lines too large
        QFontMetrics fm(fn);
        QStringList::Iterator iter = l.begin();
        QChar space(' ');
        QChar tab('\t');

        while (iter != l.end()) {
            s = *iter;

            if (fm.width(s) <= w)
                ++iter;
            else {
                QString left;
                int lastsep = 0;

                index = 0;

                do {
                    QChar c = s.at(index);

                    if ((c == space) || (c == tab))
                        lastsep = index;

                    left += c;
                    index += 1;
                }
                while (fm.width(left) <= w);

                if ((lastsep == 0) || !wb)
                    // must cut word
                    lastsep = index - 1;
                else
                    s.remove(lastsep, 1);

                left.truncate(lastsep);
                *iter = left;
                ++iter;
                s = s.mid(lastsep);

                if (! s.isEmpty())
                    // last char is != a space/tab
                    iter = l.insert(iter, s);
            }
        }

        // write lines
        ps = fm.lineSpacing();

        if ((align & ::Qt::AlignVCenter) != 0) {
            int dy = h - l.count() * ps;

            if (dy > 0)
                y += dy / 2;
        }

        y -= fm.leading();

        if ((align & ::Qt::AlignHCenter) != 0) {
            x += w / 2;

            for (iter = l.begin(); iter != l.end(); ++iter) {
                y += ps;
                (void) fputs(header, fp);
                (void) fputs(" text-anchor=\"middle\"", fp);
                (void) fprintf(fp, " x=\"%d\" y=\"%d\">", x, y);
                xml_text(fp, *iter);
                (void) fputs("</text>\n", fp);
            }
        }
        else {
            for (iter = l.begin(); iter != l.end(); ++iter) {
                y += ps;
                (void) fputs(header, fp);
                (void) fprintf(fp, " x=\"%d\" y=\"%d\">", x, y);
                xml_text(fp, *iter);
                (void) fputs("</text>\n", fp);
            }
        }
    }
}

void draw_rotate_text(int cx, int cy, int angle, QString s,
                      const QFont & fn, FILE * fp, UmlColor fg)
{
    if (s.isEmpty())
        return;

    int ps = fn.pixelSize();
    int pts = fn.pointSize();

    (void) fprintf(fp, "\t<text font-family=\"%s\" font-size=\"%d\" fill=\"%s\" xml:space=\"preserve\"",
                   (const char *) fn.family(),
                   // decrease size to help to have enough area
                   (ps == -1) ? /* Qt3 */ pts : ps - 1,
                   svg_color(fg));

    if (ps == -1)
        /* Qt3 */
        ps = pts;

    cx += ps / 2;

    if (fn.bold())
        (void) fputs(" font-weight=\"bold\"", fp);

    if (fn.italic())
        (void) fputs(" font-style=\"italic\"", fp);

    if (fn.underline())
        (void) fputs(" text-decoration=\"underline\"", fp);

    (void) fputs(" text-anchor=\"middle\"", fp);

    (void) fprintf(fp, " x=\"%d\" y=\"%d\" transform=\"rotate(%d %d %d)\">",
                   cx, cy, angle, cx, cy);

    xml_text(fp, s);

    (void) fputs("</text>\n", fp);
}

//

static QString Last_Used_Directory;

QString last_used_directory()
{
    return Last_Used_Directory;
}

void set_last_used_directory(QString s)
{
    QFileInfo fi(s);

    Last_Used_Directory = fi.dirPath();
}

unsigned api_format(bool useTrueFormat)
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "DoUML", "settings");
    settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
    int compat = settings.value("Main/compatibility_save").toInt();
    if((compat != 1) || useTrueFormat)
    {
        int fileFormat = settings.value("Main/fileformat").toInt();
        return fileFormat;
    }
    else
        return 75;
}
