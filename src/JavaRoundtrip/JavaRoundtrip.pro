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
HEADERS          = 
SOURCES          = main.cpp BrowserNode.cpp Statistic.cpp \
        ../JavaCat/UmlClassItem.cpp ../JavaCat/UmlArtifact.cpp \
        ../JavaCat/UmlAttribute.cpp ../JavaCat/UmlClass.cpp \
        ../JavaCat/UmlExtraClassMember.cpp \
        ../JavaCat/UmlClassDiagram.cpp ../JavaCat/UmlClassView.cpp \
        ../JavaCat/UmlCollaborationDiagram.cpp ../JavaCat/UmlComponent.cpp \
        ../JavaCat/UmlComponentDiagram.cpp ../JavaCat/UmlComponentView.cpp \
        ../JavaCat/UmlDeploymentDiagram.cpp ../JavaCat/UmlDeploymentView.cpp \
        ../JavaCat/UmlDiagram.cpp ../JavaCat/UmlItem.cpp ../JavaCat/UmlNode.cpp \
        ../JavaCat/UmlOperation.cpp ../JavaCat/UmlPackage.cpp \
        ../JavaCat/UmlRelation.cpp ../JavaCat/UmlNcRelation.cpp ../JavaCat/UmlSequenceDiagram.cpp \
        ../JavaCat/UmlUseCase.cpp ../JavaCat/UmlUseCaseDiagram.cpp \
        ../JavaCat/UmlUseCaseView.cpp ../JavaCat/Lex.cpp \
        ../JavaCat/ClassContainer.cpp ../JavaCat/Package.cpp \
        ../JavaCat/Class.cpp ../JavaCat/UmlClassMember.cpp \
        ../JavaCat/UmlChoicePseudoState.cpp ../JavaCat/UmlDeepHistoryPseudoState.cpp \
        ../JavaCat/UmlEntryPointPseudoState.cpp ../JavaCat/UmlExitPointPseudoState.cpp \
        ../JavaCat/UmlFinalState.cpp ../JavaCat/UmlForkPseudoState.cpp \
        ../JavaCat/UmlInitialPseudoState.cpp ../JavaCat/UmlJoinPseudoState.cpp \
        ../JavaCat/UmlJunctionPseudoState.cpp ../JavaCat/UmlShallowHistoryPseudoState.cpp \
        ../JavaCat/UmlStateAction.cpp ../JavaCat/UmlState.cpp \
        ../JavaCat/UmlStateDiagram.cpp ../JavaCat/UmlTerminatePseudoState.cpp \
        ../JavaCat/UmlRegion.cpp ../JavaCat/Progress.cpp \
        ../JavaCat/UmlAccessVariableValueAction.cpp \
        ../JavaCat/UmlOnSignalAction.cpp \
        ../JavaCat/UmlPinParameter.cpp \
        ../JavaCat/UmlActivityObject.cpp \
        ../JavaCat/UmlActivityAction.cpp \
        ../JavaCat/UmlActivityPin.cpp \
        ../JavaCat/UmlActivityRegion.cpp \
        ../JavaCat/UmlActivityControlNodeClasses.cpp \
        ../JavaCat/UmlActivityActionClasses.cpp \
        ../JavaCat/UmlExpansionNode.cpp \
        ../JavaCat/UmlActivityDiagram.cpp \
        ../JavaCat/UmlFlow.cpp \
        ../JavaCat/UmlParameterSet.cpp \
        ../JavaCat/UmlActivityParameter.cpp \
        ../JavaCat/UmlObjectDiagram.cpp \
        ../JavaCat/UmlActualParameter.cpp \
        ../JavaCat/UmlFormalParameter.cpp \
        ../JavaCat/UmlExpansionRegion.cpp \
        ../JavaCat/UmlActivity.cpp \
        ../JavaCat/UmlInterruptibleActivityRegion.cpp \
        ../JavaCat/UmlTypeSpec.cpp \
        ../JavaCat/UmlClassInstance.cpp \
        ../Tools/UmlBaseClassItem.cpp ../Tools/UmlBaseClassMember.cpp \
        ../Tools/JavaSettings.cpp \
        ../Tools/UmlBaseAttribute.cpp ../Tools/UmlBaseClass.cpp \
        ../Tools/UmlBaseExtraClassMember.cpp ../Tools/UmlBaseArtifact.cpp \
        ../Tools/UmlBaseClassDiagram.cpp ../Tools/UmlBaseClassView.cpp \
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
    ../Logging/QsDebugOutput.cpp

TARGET          = java_roundtrip
DEFINES          = WITHJAVA REVERSE ROUNDTRIP
INCLUDEPATH   = ../Tools ../JavaRoundtrip ../JavaCat ../ ../misc

#The following line was inserted by qt3to4
QT += network  qt3support 


QMAKE_CXXFLAGS += -std=gnu++11
DESTDIR = ../../bin
Release {
    MOC_DIR = $${DESTDIR}/moc_release/java_roundtrip
    OBJECTS_DIR = $${DESTDIR}/obj_release/java_roundtrip
}

Debug {
    MOC_DIR = $${DESTDIR}/moc_debug/java_roundtrip
    OBJECTS_DIR = $${DESTDIR}/obj_debug/java_roundtrip
}

