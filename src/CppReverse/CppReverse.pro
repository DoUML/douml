TEMPLATE      = app
CONFIG -= app_bundle
CONFIG += qt warn_on c++11
QMAKE_POST_LINK = " "
DEFINES += QT_DEPRECATED_WARNINGS
SOURCES          = UmlClassItem.cpp UmlAttribute.cpp UmlArtifact.cpp \
        UmlClass.cpp UmlClassDiagram.cpp UmlClassMember.cpp \
        UmlExtraClassMember.cpp \
        UmlClassView.cpp UmlCollaborationDiagram.cpp UmlComponent.cpp \
        UmlComponentDiagram.cpp UmlComponentView.cpp \
        UmlDeploymentDiagram.cpp UmlDeploymentView.cpp UmlDiagram.cpp \
        UmlItem.cpp UmlNode.cpp UmlOperation.cpp UmlPackage.cpp \
        UmlRelation.cpp UmlNcRelation.cpp UmlSequenceDiagram.cpp UmlUseCase.cpp \
        UmlUseCaseDiagram.cpp UmlUseCaseView.cpp main.cpp \
        UmlChoicePseudoState.cpp UmlDeepHistoryPseudoState.cpp \
        UmlEntryPointPseudoState.cpp UmlExitPointPseudoState.cpp \
        UmlFinalState.cpp UmlForkPseudoState.cpp \
        UmlInitialPseudoState.cpp UmlJoinPseudoState.cpp \
        UmlJunctionPseudoState.cpp UmlShallowHistoryPseudoState.cpp \
        UmlStateAction.cpp UmlState.cpp \
        UmlStateDiagram.cpp UmlTerminatePseudoState.cpp \
        UmlRegion.cpp \
        UmlAccessVariableValueAction.cpp \
        UmlOnSignalAction.cpp \
        UmlPinParameter.cpp \
        UmlActivityObject.cpp \
        UmlActivityAction.cpp \
        UmlActivityPin.cpp \
        UmlActivityRegion.cpp \
        UmlActivityControlNodeClasses.cpp \
        UmlActivityActionClasses.cpp \
        UmlExpansionNode.cpp \
        UmlActivityDiagram.cpp \
        UmlFlow.cpp \
        UmlParameterSet.cpp \
        UmlActivityParameter.cpp \
        UmlObjectDiagram.cpp \
        UmlActualParameter.cpp \
        UmlFormalParameter.cpp \
        UmlExpansionRegion.cpp \
        UmlActivity.cpp \
        UmlInterruptibleActivityRegion.cpp \
        UmlTypeSpec.cpp \
        UmlClassInstance.cpp \
        Class.cpp Package.cpp BrowserNode.cpp Namespace.cpp \
        Lex.cpp Statistic.cpp ClassContainer.cpp Progress.cpp \
        ../Tools/UmlBaseClassItem.cpp ../Tools/CppSettings.cpp \
        ../Tools/UmlBaseArtifact.cpp \
        ../Tools/UmlBaseAttribute.cpp ../Tools/UmlBaseClass.cpp \
        ../Tools/UmlBaseClassDiagram.cpp ../Tools/UmlBaseClassMember.cpp \
        ../Tools/UmlBaseExtraClassMember.cpp ../Tools/UmlBaseClassView.cpp \
        ../Tools/UmlBaseCollaborationDiagram.cpp ../Tools/UmlBaseComponent.cpp \
        ../Tools/UmlBaseComponentDiagram.cpp ../Tools/UmlBaseComponentView.cpp \
        ../Tools/UmlBaseDeploymentDiagram.cpp ../Tools/UmlBaseDeploymentView.cpp \
        ../Tools/UmlBaseDiagram.cpp ../Tools/UmlBaseItem.cpp \
        ../Tools/UmlBaseNode.cpp ../Tools/UmlBaseOperation.cpp \
        ../Tools/UmlBasePackage.cpp ../Tools/UmlBaseRelation.cpp \
        ../Tools/UmlBaseNcRelation.cpp \
        ../Tools/UmlBaseSequenceDiagram.cpp ../Tools/UmlBaseUseCase.cpp \
        ../Tools/UmlBaseUseCaseDiagram.cpp ../Tools/UmlBaseUseCaseView.cpp \
        ../Tools/UmlCom.cpp ../Tools/UmlSettings.cpp \
        ../Tools/UmlBaseChoicePseudoState.cpp ../Tools/UmlBaseDeepHistoryPseudoState.cpp \
        ../Tools/UmlBaseEntryPointPseudoState.cpp ../Tools/UmlBaseExitPointPseudoState.cpp \
        ../Tools/UmlBaseFinalState.cpp ../Tools/UmlBaseForkPseudoState.cpp \
        ../Tools/UmlBaseInitialPseudoState.cpp ../Tools/UmlBaseJoinPseudoState.cpp \
        ../Tools/UmlBaseJunctionPseudoState.cpp ../Tools/UmlBaseShallowHistoryPseudoState.cpp \
        ../Tools/UmlBaseStateAction.cpp ../Tools/UmlBaseState.cpp \
        ../Tools/UmlBaseStateDiagram.cpp ../Tools/UmlBaseTerminatePseudoState.cpp \
        ../Tools/UmlBaseRegion.cpp ../Tools/StateBehavior.cpp ../Tools/UmlBaseTransition.cpp \
        ../Tools/TransitionBehavior.cpp \
        ../Tools/UmlBaseAccessVariableValueAction.cpp \
        ../Tools/UmlBaseOnSignalAction.cpp \
        ../Tools/UmlBasePinParameter.cpp \
        ../Tools/UmlBaseActivityObject.cpp \
        ../Tools/UmlBaseActivityAction.cpp \
        ../Tools/UmlBaseActivityPin.cpp \
        ../Tools/UmlBaseActivityRegion.cpp \
        ../Tools/UmlBaseActivityPartition.cpp \
        ../Tools/UmlBaseActivityControlNodeClasses.cpp \
        ../Tools/UmlBaseActivityActionClasses.cpp \
        ../Tools/UmlBaseExpansionNode.cpp \
        ../Tools/UmlBaseActivityDiagram.cpp \
        ../Tools/UmlBaseFlow.cpp \
        ../Tools/UmlBaseParameterSet.cpp \
        ../Tools/UmlBaseActivityParameter.cpp \
        ../Tools/UmlBaseObjectDiagram.cpp \
        ../Tools/UmlBaseActualParameter.cpp \
        ../Tools/UmlBaseFormalParameter.cpp \
        ../Tools/UmlBaseExpansionRegion.cpp \
        ../Tools/UmlBaseActivity.cpp \
        ../Tools/UmlBaseInterruptibleActivityRegion.cpp \
        ../Tools/FlowBehavior.cpp \
        ../Tools/UmlBaseTypeSpec.cpp \
        ../Tools/UmlBaseParameter.cpp \
        ../Tools/UmlBaseView.cpp \
        ../Tools/UmlBaseClassInstance.cpp \
        ../misc/mystr.cpp \
    ../Logging/QsLogDest.cpp \
    ../Logging/QsLog.cpp \
    ../Logging/QsDebugOutput.cpp \
    mcpp/system.cpp \
    mcpp/support.cpp \
    mcpp/preproc.cpp \
    mcpp/mbchar.cpp \
    mcpp/main_mpcc.cpp \
    mcpp/expand.cpp \
    mcpp/eval.cpp \
    mcpp/directive.cpp \
    preprocess.cpp


TARGET          = cpp_reverse
DEFINES          += WITHCPP REVERSE BooL=bool FALSE=false TRUE=true
#DEFINES += _RUN_PLUGOUT_EXTERNAL_

INCLUDEPATH   += ../Tools ../CppReverse ../
QT += testlib


#The following line was inserted by qt3to4
QT += network widgets
#qt3support

DESTDIR = ../../bin
CONFIG(release, debug|release) {
    DEFINES += NDEBUG
    MOC_DIR = $${DESTDIR}/moc_release/cpp_reverse
    OBJECTS_DIR = $${DESTDIR}/obj_release/cpp_reverse
}
CONFIG(debug, debug|release) {
    DEFINES += TRACE DEBUG DEBUG_DOUML
    MOC_DIR = $${DESTDIR}/moc_debug/cpp_reverse
    OBJECTS_DIR = $${DESTDIR}/obj_debug/cpp_reverse
}
UI_DIR = src/ui

HEADERS += \
    ../Logging/QsLogDest.h \
    ../Logging/QsLog.h \
    ../Logging/QsDebugOutput.h \
    mcpp/system.H \
    mcpp/noconfig.H \
    mcpp/mcpp_out.h \
    mcpp/mcpp_lib.h \
    mcpp/internal.H \
    preprocess.h
