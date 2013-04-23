TEMPLATE    = app
TARGET        = file_control
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
DEFINES        = BooL=bool
HEADERS        = ./aVisibility.h \
          ./UmlBaseExtraClassMember.h \
          ./UmlBaseComponentDiagram.h \
          ./UmlBaseNode.h \
          ./UmlBaseFinalState.h \
          ./UmlFormalParameter.h \
          ./UmlBaseClassItem.h \
          ./UmlOperation.h \
          ./UmlBaseClassDiagram.h \
          ./JavaSettingsCmd.h \
          ./UmlBaseJunctionPseudoState.h \
          ./UmlComponent.h \
          ./UmlStateDiagram.h \
          ./StateBehavior.h \
          ./UmlCom.h \
          ./UmlBaseRelation.h \
          ./UmlUseCaseView.h \
          ./UmlBaseDeepHistoryPseudoState.h \
          ./UmlBaseNcRelation.h \
          ./OnInstanceCmd.h \
          ./UmlPackage.h \
          ./UmlBaseUseCaseView.h \
          ./UmlBaseTerminatePseudoState.h \
          ./JavaSettings.h \
          ./UmlBaseDiagram.h \
          ./UmlDeploymentView.h \
          ./UmlStateAction.h \
          ./UmlBaseClassMember.h \
          ./UmlBasePackage.h \
          ./UmlBaseFormalParameter.h \
          ./UmlBaseComponentView.h \
          ./UmlItem.h \
          ./UmlStateItem.h \
          ./UmlState.h \
          ./UmlBaseChoicePseudoState.h \
          ./anItemKind.h \
          ./UmlBaseJoinPseudoState.h \
          ./UmlExtraClassMember.h \
          ./UmlInitialPseudoState.h \
          ./UmlBaseActualParameter.h \
          ./UmlBaseInitialPseudoState.h \
          ./UmlBaseAttribute.h \
          ./UmlClassItem.h \
          ./UmlBaseComponent.h \
          ./UmlTransition.h \
          ./UmlBaseState.h \
          ./UmlClass.h \
          ./MiscGlobalCmd.h \
          ./IdlSettingsCmd.h \
          ./ClassGlobalCmd.h \
          ./UmlForkPseudoState.h \
          ./UmlCollaborationDiagram.h \
          ./UmlRegion.h \
          ./UmlDeepHistoryPseudoState.h \
          ./TransitionBehavior.h \
          ./UmlUseCaseDiagram.h \
          ./UmlBaseOperation.h \
          ./UmlComponentView.h \
          ./UmlSequenceDiagram.h \
          ./Dialog.h \
          ./UmlArtifact.h \
          ./UmlBaseShallowHistoryPseudoState.h \
          ./UmlClassDiagram.h \
          ./UmlJoinPseudoState.h \
          ./UmlAttribute.h \
          ./UmlNcRelation.h \
          ./UmlBaseCollaborationDiagram.h \
          ./UmlBaseStateDiagram.h \
          ./UmlClassView.h \
          ./PackageGlobalCmd.h \
          ./UmlBaseEntryPointPseudoState.h \
          ./UmlFinalState.h \
          ./UmlClassMember.h \
          ./UmlTerminatePseudoState.h \
          ./UmlBaseSequenceDiagram.h \
          ./UmlEntryPointPseudoState.h \
          ./UmlPseudoState.h \
          ./UmlBaseExitPointPseudoState.h \
          ./UmlBaseDeploymentView.h \
          ./UmlComponentDiagram.h \
          ./UmlDiagram.h \
          ./UmlSettingsCmd.h \
          ./UmlShallowHistoryPseudoState.h \
          ./UmlNode.h \
          ./UmlExitPointPseudoState.h \
          ./aRelationKind.h \
          ./UmlBaseItem.h \
          ./UmlBaseTransition.h \
          ./UmlBaseArtifact.h \
          ./IdlSettings.h \
          ./UmlRelation.h \
          ./aDirection.h \
          ./UmlTypeSpec.h \
          ./UmlJunctionPseudoState.h \
          ./UmlBaseForkPseudoState.h \
          ./CppSettings.h \
          ./UmlSettings.h \
          ./CppSettingsCmd.h \
          ./UmlActualParameter.h \
          ./UmlChoicePseudoState.h \
          ./UmlBaseClassView.h \
          ./UmlBaseUseCase.h \
          ./UmlBaseClass.h \
          ./CmdFamily.h \
          ./UmlBaseDeploymentDiagram.h \
          ./UmlBaseUseCaseDiagram.h \
          ./UmlBaseStateAction.h \
          ./UmlBaseRegion.h \
          ./UmlBaseDeployment.h \
          ./UmlStereotype.h \
          ./UmlDeploymentDiagram.h \
          ./UmlUseCase.h
SOURCES        = ./aVisibility.cpp \
          ./UmlBaseExtraClassMember.cpp \
          ./UmlBaseComponentDiagram.cpp \
          ./UmlBaseNode.cpp \
          ./UmlBaseFinalState.cpp \
          ./UmlFormalParameter.cpp \
          ./UmlBaseClassItem.cpp \
          ./UmlOperation.cpp \
          ./UmlBaseClassDiagram.cpp \
          ./JavaSettingsCmd.cpp \
          ./UmlBaseJunctionPseudoState.cpp \
          ./UmlComponent.cpp \
          ./UmlStateDiagram.cpp \
          ./StateBehavior.cpp \
          ./UmlCom.cpp \
          ./UmlBaseRelation.cpp \
          ./UmlUseCaseView.cpp \
          ./UmlBaseDeepHistoryPseudoState.cpp \
          ./UmlBaseNcRelation.cpp \
          ./OnInstanceCmd.cpp \
          ./UmlPackage.cpp \
          ./UmlBaseUseCaseView.cpp \
          ./UmlBaseTerminatePseudoState.cpp \
          ./JavaSettings.cpp \
          ./UmlBaseDiagram.cpp \
          ./UmlDeploymentView.cpp \
          ./UmlStateAction.cpp \
          ./UmlBaseClassMember.cpp \
          ./UmlBasePackage.cpp \
          ./UmlBaseFormalParameter.cpp \
          ./UmlBaseComponentView.cpp \
          ./UmlItem.cpp \
          ./UmlStateItem.cpp \
          ./UmlState.cpp \
          ./UmlBaseChoicePseudoState.cpp \
          ./anItemKind.cpp \
          ./UmlBaseJoinPseudoState.cpp \
          ./UmlExtraClassMember.cpp \
          ./UmlInitialPseudoState.cpp \
          ./UmlBaseActualParameter.cpp \
          ./UmlBaseInitialPseudoState.cpp \
          ./UmlBaseAttribute.cpp \
          ./UmlClassItem.cpp \
          ./UmlBaseComponent.cpp \
          ./UmlTransition.cpp \
          ./UmlBaseState.cpp \
          ./UmlClass.cpp \
          ./MiscGlobalCmd.cpp \
          ./IdlSettingsCmd.cpp \
          ./ClassGlobalCmd.cpp \
          ./UmlForkPseudoState.cpp \
          ./UmlCollaborationDiagram.cpp \
          ./UmlRegion.cpp \
          ./UmlDeepHistoryPseudoState.cpp \
          ./TransitionBehavior.cpp \
          ./UmlUseCaseDiagram.cpp \
          ./UmlBaseOperation.cpp \
          ./UmlComponentView.cpp \
          ./UmlSequenceDiagram.cpp \
          ./Dialog.cpp \
          ./UmlArtifact.cpp \
          ./UmlBaseShallowHistoryPseudoState.cpp \
          ./UmlClassDiagram.cpp \
          ./UmlJoinPseudoState.cpp \
          ./UmlAttribute.cpp \
          ./UmlNcRelation.cpp \
          ./UmlBaseCollaborationDiagram.cpp \
          ./UmlBaseStateDiagram.cpp \
          ./UmlClassView.cpp \
          ./PackageGlobalCmd.cpp \
          ./UmlBaseEntryPointPseudoState.cpp \
          ./UmlFinalState.cpp \
          ./UmlClassMember.cpp \
          ./UmlTerminatePseudoState.cpp \
          ./UmlBaseSequenceDiagram.cpp \
          ./UmlEntryPointPseudoState.cpp \
          ./Main.cpp \
          ./UmlPseudoState.cpp \
          ./UmlBaseExitPointPseudoState.cpp \
          ./UmlBaseDeploymentView.cpp \
          ./UmlComponentDiagram.cpp \
          ./UmlDiagram.cpp \
          ./UmlSettingsCmd.cpp \
          ./UmlShallowHistoryPseudoState.cpp \
          ./UmlNode.cpp \
          ./UmlExitPointPseudoState.cpp \
          ./aRelationKind.cpp \
          ./UmlBaseItem.cpp \
          ./UmlBaseTransition.cpp \
          ./UmlBaseArtifact.cpp \
          ./IdlSettings.cpp \
          ./UmlRelation.cpp \
          ./aDirection.cpp \
          ./UmlTypeSpec.cpp \
          ./UmlJunctionPseudoState.cpp \
          ./UmlBaseForkPseudoState.cpp \
          ./CppSettings.cpp \
          ./UmlSettings.cpp \
          ./CppSettingsCmd.cpp \
          ./UmlActualParameter.cpp \
          ./UmlChoicePseudoState.cpp \
          ./UmlBaseClassView.cpp \
          ./UmlBaseUseCase.cpp \
          ./UmlBaseClass.cpp \
          ./CmdFamily.cpp \
          ./UmlBaseDeploymentDiagram.cpp \
          ./UmlBaseUseCaseDiagram.cpp \
          ./UmlBaseStateAction.cpp \
          ./UmlBaseRegion.cpp \
          ./UmlBaseDeployment.cpp \
          ./UmlStereotype.cpp \
          ./UmlDeploymentDiagram.cpp \
          ./UmlUseCase.cpp

#The following line was inserted by qt3to4
QT += network  qt3support 
INCLUDEPATH += ../../src
CONFIG += qtestlib
DESTDIR = ../../bin
Release {
    MOC_DIR = $${DESTDIR}/moc_release/file_control
    OBJECTS_DIR = $${DESTDIR}/obj_release/file_control
}

Debug {
    MOC_DIR = $${DESTDIR}/moc_debug/file_control
    OBJECTS_DIR = $${DESTDIR}/obj_debug/file_control
}
UI_DIR = src/ui

QMAKE_CXXFLAGS += -std=gnu++11
