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





#include <q3popupmenu.h>
#include <QTextStream>
#include <q3filedialog.h>
//Added by qt3to4:
#include <QTextStream>

#include "Tool.h"
#include "myio.h"
#include "BrowserView.h"
#include "err.h"

unsigned Tool::ntools;
ATool * Tool::tools;
bool Tool::already_read;

ATool::ATool()
{
    for (int i = 0; i != UmlCodeSup; i += 1)
        applicable[i] = FALSE;
}

void Tool::init()
{
    already_read = FALSE;
    set_ntools(0);
}

void Tool::defaults()
{
    unsigned index;

    already_read = FALSE;
    set_ntools(17);

    tools[0].display = "HTML documentation";
    tools[0].cmd = "ghtml";

    for (index = 0; index != UmlCodeSup; index += 1)
        tools[0].applicable[index] = TRUE;

    tools[1].display = "HTML doc. (flat)";
    tools[1].cmd = "ghtml -flat";

    for (index = 0; index != UmlCodeSup; index += 1)
        tools[1].applicable[index] = TRUE;

    tools[2].display = "HTML doc. (svg)";
    tools[2].cmd = "ghtml -svg";

    for (index = 0; index != UmlCodeSup; index += 1)
        tools[2].applicable[index] = TRUE;

    tools[3].display = "HTML doc. (flat, svg)";
    tools[3].cmd = "ghtml -flat -svg";

    for (index = 0; index != UmlCodeSup; index += 1)
        tools[3].applicable[index] = TRUE;

    tools[4].display = "Generate .pro";
    tools[4].cmd = "gpro";
    tools[4].applicable[UmlArtifact] = TRUE;

    tools[5].display = "Import Rose";
    tools[5].cmd = "irose";
    tools[5].applicable[UmlProject] = TRUE;
    tools[5].applicable[UmlPackage] = TRUE;

    tools[6].display = "C++ utilities";
    tools[6].cmd = "cpp_util";
    tools[6].applicable[UmlClass] = TRUE;

    tools[7].display = "Generate XMI 1.2";
    tools[7].cmd = "gxmi";
    tools[7].applicable[UmlProject] = TRUE;

    tools[8].display = "Generate XMI 2.1";
    tools[8].cmd = "gxmi2";
    tools[8].applicable[UmlProject] = TRUE;

    tools[9].display = "Import XMI 2.1";
    tools[9].cmd = "ixmi2";
    tools[9].applicable[UmlProject] = TRUE;
    tools[9].applicable[UmlPackage] = TRUE;

    tools[10].display = "C++ state machine";
    tools[10].cmd = "stmgen";
    tools[10].applicable[UmlState] = TRUE;

    tools[11].display = "Use case wizard";
    tools[11].cmd = "usecasewizard";
    tools[11].applicable[UmlUseCase] = TRUE;

    tools[12].display = "Check-in";
    tools[12].cmd = "file_control ci";
    tools[12].applicable[UmlProject] = TRUE;
    tools[12].applicable[UmlPackage] = TRUE;

    tools[13].display = "Check-out";
    tools[13].cmd = "file_control co";
    tools[13].applicable[UmlProject] = TRUE;
    tools[13].applicable[UmlPackage] = TRUE;

    tools[14].display = "Deploy classes";
    tools[14].cmd = "deplcl";
    tools[14].applicable[UmlClassView] = TRUE;

    tools[15].display = "Global Change";
    tools[15].cmd = "global_change";
    tools[15].applicable[UmlProject] = TRUE;
    tools[15].applicable[UmlPackage] = TRUE;
    tools[15].applicable[UmlClassView] = TRUE;
    tools[15].applicable[UmlDeploymentView] = TRUE;
    tools[15].applicable[UmlClass] = TRUE;

    tools[16].display = "Uml projection";
    tools[16].cmd = "uml_proj";
    tools[16].applicable[UmlProject] = TRUE;
    tools[16].applicable[UmlPackage] = TRUE;
    tools[16].applicable[UmlClassView] = TRUE;
    tools[16].applicable[UmlClass] = TRUE;
    tools[16].applicable[UmlAttribute] = TRUE;
    tools[16].applicable[UmlOperation] = TRUE;

    for (index = UmlAssociation; index != UmlRelations; index += 1)
        tools[16].applicable[index] = TRUE;
}

QList<QString> SetupFailingTools()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "DoUML", "settings");
    settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
    return settings.value("Failing_Tools/Tools").toString().split(",");
}

bool Tool::menu_insert(Q3PopupMenu * tool, UmlCode target,
                       const QObject * receiver, const char * member)
{
    unsigned index;
    bool first = TRUE;

    static QList<QString> failingTools = SetupFailingTools();

    for (index = 0; index != ntools; index += 1) {
        if (tools[index].applicable[target] && !failingTools.contains(tools[index].display)) {
            if (first) {
                tool->insertSeparator();
                first = FALSE;
            }

            tool->setItemParameter(tool->insertItem(tools[index].display,
                                                    receiver, member),
                                   index);
        }
    }

    return !first;
}

bool Tool::menu_insert(Q3PopupMenu * tool, UmlCode target, int first_id)
{
    unsigned index;
    bool have = FALSE;

    static QList<QString> failingTools = SetupFailingTools();

    for (index = 0; index != ntools; index += 1) {
        if (tools[index].applicable[target] && !failingTools.contains(tools[index].display)) {
            have = TRUE;
            tool->insertItem(tools[index].display, first_id + index);
        }
    }

    return have;
}

void Tool::shortcut(QString s, int & choice, UmlCode target,
                    int first_id)
{
    unsigned index;

    for (index = 0; index != ntools; index += 1) {
        if (tools[index].applicable[target] &&
            (tools[index].display == s)) {
            choice = first_id + index;
            return;
        }
    }
}

QStringList Tool::all_display()
{
    QStringList r;
    unsigned index;

    for (index = 0; index != ntools; index += 1) {
        QString s = tools[index].display;

        if (r.findIndex(s) == -1)
            r.append(s);
    }

    r.sort();
    return r;
}

const char * Tool::command(int rank)
{
    return tools[rank].cmd;
}

const char * Tool::command(const char * d)
{
    unsigned index;

    for (index = 0; index != ntools; index += 1)
        if (strcmp(d, tools[index].display) == 0)
            return tools[index].cmd;

    return 0;
}

void Tool::set_ntools(unsigned n)
{
    if (tools)
        delete [] tools;

    tools = ((ntools = n) == 0) ? 0 : new ATool[n];
}

static const struct {
    UmlCode kind;
    const char * key;
} ToolCase[] = {
    { UmlClass, "Class" },
    { UmlOperation, "Operation" },
    { UmlAttribute, "Attribute" },
    { UmlGeneralisation, "Generalisation" },
    { UmlRealize, "Realize" },
    { UmlDependency, "Dependency" },
    { UmlAssociation, "Association" },
    { UmlDirectionalAssociation, "DirectionalAssociation" },
    { UmlAggregation, "Aggregation" },
    { UmlAggregationByValue, "AggregationByValue" },
    { UmlDirectionalAggregation, "DirectionalAggregation" },
    { UmlDirectionalAggregationByValue, "DirectionalAggregationByValue" },
    { UmlExtraMember, "ExtraMember" },
    { UmlClassInstance, "ClassInstance" },

    { UmlState, "State" },
    { UmlRegion, "Region" },
    { UmlStateAction, "StateAction" },
    { InitialPS, "Initial" },
    { EntryPointPS, "EntryPoint" },
    { FinalPS, "Final" },
    { TerminatePS, "Terminate" },
    { ExitPointPS, "ExitPoint" },
    { DeepHistoryPS, "DeepHistory" },
    { ShallowHistoryPS, "ShallowHistory" },
    { JunctionPS, "Junction" },
    { ChoicePS, "Choice" },
    { ForkPS, "Fork" },
    { JoinPS, "Join" },
    { UmlTransition, "Transition" },

    { UmlActivity, "Activity" },
    { UmlInterruptibleActivityRegion, "InterruptibleActivityRegion" },
    { UmlExpansionRegion, "ExpansionRegion" },
    { UmlActivityObject, "ActivityObject" },
    { UmlActivityAction, "ActivityAction" },
    { UmlParameter, "Parameter" },
    { UmlParameterSet, "ParameterSet" },
    { UmlActivityPin, "Pin" },
    { UmlExpansionNode, "ExpansionNode" },
    { InitialAN, "InitialActivityNode" },
    { ActivityFinalAN, "FinalActivityNode" },
    { FlowFinalAN, "ExitPointActivityNode" },
    { DecisionAN, "DecisionActivityNode" },
    { MergeAN, "MergeActivityNode" },
    { ForkAN, "ForkActivityNode" },
    { JoinAN, "JoinActivityNode" },
    { UmlFlow, "Flow" },

    { UmlProject , "Project" },
    { UmlPackage, "Package" },
    { UmlUseCaseView, "UseCaseView" },
    { UmlClassView, "ClassView" },
    { UmlComponentView, "ComponentView" },
    { UmlDeploymentView, "DeploymentView" },
    { UmlUseCaseDiagram, "UseCaseDiagram" },
    { UmlSeqDiagram, "SeqDiagram" },
    { UmlColDiagram, "ColDiagram" },
    { UmlClassDiagram, "ClassDiagram" },
    { UmlObjectDiagram, "ObjectDiagram" },
    { UmlStateDiagram, "StateDiagram" },
    { UmlActivityDiagram, "ActivityDiagram" },
    { UmlComponentDiagram, "ComponentDiagram" },
    { UmlDeploymentDiagram, "DeploymentDiagram" },
    { UmlUseCase, "UseCase" },
    { UmlComponent, "Component" },
    { UmlDeploymentNode, "Node" },
    { UmlArtifact, "Artifact" },
    { UmlInherit, "Inherit" },
    { UmlDependOn, "DependOn" },
};

void Tool::save()
{
    QSharedPointer<QByteArray> newdef(new QByteArray());
    QTextStream st(newdef.data(), QIODevice::WriteOnly);

    st.setEncoding(QTextStream::Latin1);

    st << "// 'tool' \"the executable\" \"displayed string\" {target}+";

    for (unsigned rank = 0; rank != ntools; rank += 1) {
        ATool & tool = tools[rank];

        st << "\ntool ";
        save_string(tools[rank].display, st);
        st << " ";
        save_string(tools[rank].cmd, st);

        for (int index = 0; index != sizeof(ToolCase) / sizeof(*ToolCase); index += 1) {
            if (tool.applicable[ToolCase[index].kind]) {
                st << " " << ToolCase[index].key;
            }
        }
    }

    st << '\n';

    st << '\000';
    st.flush();
    save_if_needed("tools", newdef);
}

// try to read tool settings in the prj file

void Tool::read(char *& st, char *& k)
{
    if (!strcmp(k, "tools_number")) {
        // old format
        set_ntools(read_unsigned(st));
        read(st, k, FALSE);
    }
    else
        set_ntools(0);
}

// try to read tool settings

bool Tool::read(const char * f)
{
    if (!already_read) {
        char * s = read_file((f == 0) ? "tools" : f);

        if (s != 0) {
            PRE_TRY;

            try {
                char * st = s;
                char * k;

                read(st, k, TRUE);
            }
            catch (int) {
                ;
            }

            POST_TRY;
            delete [] s;
            return TRUE;
        }
    }

    return FALSE;
}

// try to read tool settings of an other project

bool Tool::import()
{
    QString fn = Q3FileDialog::getOpenFileName(last_used_directory(), "tools");

    if (!fn.isEmpty()) {
        set_last_used_directory(fn);
        already_read = FALSE;
        set_ntools(0);
        return read((const char *) fn);
    }

    return FALSE;
}

// try to add tools from an other project

void Tool::add()
{
    char * s = read_file("tools");

    if (s != 0) {
        unsigned oldn = ntools;
        ATool * olds = tools;

        ntools = 0;
        tools = 0;

        PRE_TRY;

        try {
            char * st = s;
            char * k;

            read(st, k, TRUE);
        }
        catch (int) {
            ;
        }

        POST_TRY;
        delete [] s;

        if (olds != 0) {
            // reset to old tools
            ATool * newt = new ATool[ntools + oldn /*worst case*/];
            unsigned newn = ntools;
            unsigned index;

            for (index = 0; index != oldn; index += 1)
                newt[index] = olds[index];

            delete [] olds;
            ntools = oldn;

            // add imported tools if missing
            for (index = 0; index != newn; index += 1) {
                ATool & t = tools[index];

                unsigned index2;

                for (index2 = 0; index2 != oldn; index2 += 1) {
                    ATool & t2 = newt[index2];

                    if (t2.display == t.display) {
                        // add cases
                        unsigned index3;

                        // warning : do NOT use bool * to access inside t2.applicable
                        // and t.applicable because generated code is wrong under Windows
                        // (may be bool[128] use 128 nits rather than 128bytes ?)
                        for (index3 = 0; index3 != UmlCodeSup; index3 += 1)
                            t2.applicable[index3] |= t.applicable[index3];

                        break;
                    }
                }

                if (index2 == oldn)
                    // add tool
                    newt[ntools++] = t;
            }

            delete [] tools;
            tools = newt;
        }
    }
}

// for all cases

void Tool::read(char *& st, char *& k, bool new_format)
{
    if (new_format && at_end(st)) {
        ntools = 0;
        return;
    }

    unsigned rank = 0;

    k = read_keyword(st);
    already_read = TRUE;

    for (;;) {
        if (strcmp(k, "tool")) {
            if (new_format) {
                ntools = rank;
                wrong_keyword(k, "tool");
            }
            else if (strcmp(k, "end"))
                wrong_keyword(k, "end");
            else
                k = read_keyword(st);

            return;
        }

        if (rank == ntools) {
            // too small table
            ATool * t = new ATool[ntools + 16];

            for (rank = 0; rank != ntools; rank += 1)
                t[rank] = tools[rank];

            if (tools)
                delete [] tools;

            tools = t;
            ntools += 16;
        }

        ATool & tool = tools[rank++];

        tool.display = read_string(st);
        tool.cmd = read_string(st);

        // fixe ghtml default conf bug
        int index = tool.cmd.find("ghtml - flat");

        if (index != -1)
            tool.cmd.remove(index + 7, 1);

        if (new_format && at_end(st)) {
            ntools = rank;
            return;
        }

        k = read_keyword(st);

        if (!strcmp(k, "Project")) {
            tool.applicable[UmlProject] = TRUE;

            if (new_format && at_end(st)) {
                ntools = rank;
                return;
            }

            k = read_keyword(st);
        }

        for (;;) {
            // old format
            const char * kc;

            if (!strcmp(k, "Attribut"))
                kc = "Attribute";
            else if (((index = strlen(k)) > 4) &&
                     !strcmp(k + index - 4, "Icon")) {
                k[index - 4] = 0;
                kc = k;
            }
            else
                kc = k;

            for (index = 0; index != sizeof(ToolCase) / sizeof(*ToolCase); index += 1) {
                if (!strcmp(kc, ToolCase[index].key))
                    break;
            }

            if (index != sizeof(ToolCase) / sizeof(*ToolCase))
                tool.applicable[ToolCase[index].kind] = TRUE;
            else
                break;

            if (new_format && at_end(st)) {
                ntools = rank;
                return;
            }

            k = read_keyword(st);
        }
    }
}

//
//
//

static int Verbose;

void toggle_verbose_generation()
{
    Verbose ^= 1;
}

bool verbose_generation()
{
    return (Verbose != 0);
}

//
//
//

static int PreserveBodies;

void toggle_preserve_bodies()
{
    PreserveBodies ^= 1;
}

bool preserve_bodies()
{
    return (PreserveBodies != 0);
}

//

static int AddOperationProfile;

void toggle_add_operation_profile()
{
    AddOperationProfile ^= 1;
}

bool add_operation_profile()
{
    return (AddOperationProfile != 0);
}
