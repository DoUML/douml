TEMPLATE      = app
CONFIG(Debug, Debug|Release) {
    CONFIG -= Debug Release
    CONFIG += qt warn_on Debug
    QMAKE_POST_LINK = " "
}
CONFIG(Release, Debug|Release) {
    CONFIG -= Debug Release
    CONFIG += qt Release
    QMAKE_POST_LINK = " "
}
SOURCES          = main.cpp Statistic.cpp \
        ../CppReverse/UmlClassItem.cpp \
        ../CppReverse/UmlAttribute.cpp \
        ../CppReverse/UmlArtifact.cpp \
        ../CppReverse/UmlClass.cpp \
        ../CppReverse/UmlClassDiagram.cpp \
        ../CppReverse/UmlClassMember.cpp \
        ../CppReverse/UmlExtraClassMember.cpp \
        ../CppReverse/UmlClassView.cpp \
        ../CppReverse/UmlCollaborationDiagram.cpp \
        ../CppReverse/UmlComponent.cpp \
        ../CppReverse/UmlComponentDiagram.cpp \
        ../CppReverse/UmlComponentView.cpp \
        ../CppReverse/UmlDeploymentDiagram.cpp \
        ../CppReverse/UmlDeploymentView.cpp \
        ../CppReverse/UmlDiagram.cpp \
        ../CppReverse/UmlItem.cpp \
        ../CppReverse/UmlNode.cpp \
        ../CppReverse/UmlOperation.cpp \
        ../CppReverse/UmlPackage.cpp \
        ../CppReverse/UmlRelation.cpp \
        ../CppReverse/UmlNcRelation.cpp \
        ../CppReverse/UmlSequenceDiagram.cpp \
        ../CppReverse/UmlUseCase.cpp \
        ../CppReverse/UmlUseCaseDiagram.cpp \
        ../CppReverse/UmlUseCaseView.cpp \
        ../CppReverse/UmlChoicePseudoState.cpp \
        ../CppReverse/UmlDeepHistoryPseudoState.cpp \
        ../CppReverse/UmlEntryPointPseudoState.cpp \
        ../CppReverse/UmlExitPointPseudoState.cpp \
        ../CppReverse/UmlFinalState.cpp \
        ../CppReverse/UmlForkPseudoState.cpp \
        ../CppReverse/UmlInitialPseudoState.cpp \
        ../CppReverse/UmlJoinPseudoState.cpp \
        ../CppReverse/UmlJunctionPseudoState.cpp \
        ../CppReverse/UmlShallowHistoryPseudoState.cpp \
        ../CppReverse/UmlStateAction.cpp \
        ../CppReverse/UmlState.cpp \
        ../CppReverse/UmlStateDiagram.cpp \
        ../CppReverse/UmlTerminatePseudoState.cpp \
        ../CppReverse/UmlRegion.cpp \
        ../CppReverse/UmlAccessVariableValueAction.cpp \
        ../CppReverse/UmlOnSignalAction.cpp \
        ../CppReverse/UmlPinParameter.cpp \
        ../CppReverse/UmlActivityObject.cpp \
        ../CppReverse/UmlActivityAction.cpp \
        ../CppReverse/UmlActivityPin.cpp \
        ../CppReverse/UmlActivityRegion.cpp \
        ../CppReverse/UmlActivityControlNodeClasses.cpp \
        ../CppReverse/UmlActivityActionClasses.cpp \
        ../CppReverse/UmlExpansionNode.cpp \
        ../CppReverse/UmlActivityDiagram.cpp \
        ../CppReverse/UmlFlow.cpp \
        ../CppReverse/UmlParameterSet.cpp \
        ../CppReverse/UmlActivityParameter.cpp \
        ../CppReverse/UmlObjectDiagram.cpp \
        ../CppReverse/UmlActualParameter.cpp \
        ../CppReverse/UmlFormalParameter.cpp \
        ../CppReverse/UmlExpansionRegion.cpp \
        ../CppReverse/UmlActivity.cpp \
        ../CppReverse/UmlInterruptibleActivityRegion.cpp \
        ../CppReverse/UmlTypeSpec.cpp \
        ../CppReverse/UmlClassInstance.cpp \
        ../CppReverse/Class.cpp \
        ../CppReverse/Package.cpp \
        ../CppReverse/BrowserNode.cpp \
        ../CppReverse/Namespace.cpp \
        ../CppReverse/Lex.cpp \
        ../CppReverse/ClassContainer.cpp \
        ../CppReverse/Progress.cpp \
        ../Tools/UmlBaseClassItem.cpp \
        ../Tools/CppSettings.cpp \
        ../Tools/UmlBaseArtifact.cpp \
        ../Tools/UmlBaseAttribute.cpp \
        ../Tools/UmlBaseClass.cpp \
        ../Tools/UmlBaseClassDiagram.cpp \
        ../Tools/UmlBaseClassMember.cpp \
        ../Tools/UmlBaseExtraClassMember.cpp \
        ../Tools/UmlBaseClassView.cpp \
        ../Tools/UmlBaseCollaborationDiagram.cpp \
        ../Tools/UmlBaseComponent.cpp \
        ../Tools/UmlBaseComponentDiagram.cpp \
        ../Tools/UmlBaseComponentView.cpp \
        ../Tools/UmlBaseDeploymentDiagram.cpp \
        ../Tools/UmlBaseDeploymentView.cpp \
        ../Tools/UmlBaseDiagram.cpp \
        ../Tools/UmlBaseItem.cpp \
        ../Tools/UmlBaseNode.cpp \
        ../Tools/UmlBaseOperation.cpp \
        ../Tools/UmlBasePackage.cpp \
        ../Tools/UmlBaseRelation.cpp \
        ../Tools/UmlBaseNcRelation.cpp \
        ../Tools/UmlBaseSequenceDiagram.cpp \
        ../Tools/UmlBaseUseCase.cpp \
        ../Tools/UmlBaseUseCaseDiagram.cpp \
        ../Tools/UmlBaseUseCaseView.cpp \
        ../Tools/UmlCom.cpp \
        ../Tools/UmlSettings.cpp \
        ../Tools/UmlBaseChoicePseudoState.cpp \
        ../Tools/UmlBaseDeepHistoryPseudoState.cpp \
        ../Tools/UmlBaseEntryPointPseudoState.cpp \
        ../Tools/UmlBaseExitPointPseudoState.cpp \
        ../Tools/UmlBaseFinalState.cpp \
        ../Tools/UmlBaseForkPseudoState.cpp \
        ../Tools/UmlBaseInitialPseudoState.cpp \
        ../Tools/UmlBaseJoinPseudoState.cpp \
        ../Tools/UmlBaseJunctionPseudoState.cpp \
        ../Tools/UmlBaseShallowHistoryPseudoState.cpp \
        ../Tools/UmlBaseStateAction.cpp \
        ../Tools/UmlBaseState.cpp \
        ../Tools/UmlBaseStateDiagram.cpp \
        ../Tools/UmlBaseTerminatePseudoState.cpp \
        ../Tools/UmlBaseRegion.cpp \
        ../Tools/StateBehavior.cpp \
        ../Tools/UmlBaseTransition.cpp \
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
    ../Logging/QsDebugOutput.cpp



TARGET          = cpp_roundtrip
DEFINES          = WITHCPP REVERSE ROUNDTRIP BooL=bool
INCLUDEPATH   = ../Tools ../CppRoundtrip ../CppReverse

#The following line was inserted by qt3to4
QT += network  qt3support 

INCLUDEPATH += ../../src
CONFIG += qtestlib
DESTDIR = ../../bin
Release {
    MOC_DIR = $${DESTDIR}/moc_release/cpp_roundtrip
    OBJECTS_DIR = $${DESTDIR}/obj_release/cpp_roundtrip
}

Debug {
    MOC_DIR = $${DESTDIR}/moc_debug/cpp_roundtrip
    OBJECTS_DIR = $${DESTDIR}/obj_debug/cpp_roundtrip
}
UI_DIR = src/ui

QMAKE_CXXFLAGS += -std=gnu++11
