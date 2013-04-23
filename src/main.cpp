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
//
// *************************************************************************





#include <qapplication.h>
#include <QTextCodec>
//Added by qt3to4:
#include "misc/mystr.h"
#include <QTextCodec>

#include "UmlWindow.h"
#include "UmlPixmap.h"
#include "UmlGlobal.h"
#include "UmlDesktop.h"
#include "ToolCom.h"
#include "BrowserView.h"
#include "BrowserPackage.h"
#include "Shortcut.h"
#include "DialogUtil.h"
#include "mu.h"
#include "err.h"
#include "EnvDialog.h"
#include "Logging/QsLogDest.h"
#include "Logging/QsLog.h"
#include "Factories/EdgeMenuFactory.h"
#include "Factories/DialogConnections.h"
#include "Factories/EdgeToolBarCreation.h"
#include "dialog/ClassDialog.h"
#include "dialog/ArtifactDialog.h"
#include "dialog/OperationDialog.h"
#include "ui/constructorinitializerdialog.h"
#include "misc/TypeIdentifier.h"
#include <QSettings>

#include "translate.h"
#include "settings_ini.h"

bool ExitOnError = FALSE;
int main(int argc, char ** argv)
{

    ExitOnError = FALSE;

    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(QString(":/douml.64.png")));

    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));

//#ifdef DEBUG
    QsLogging::Logger & logger = QsLogging::Logger::instance();
    logger.setLoggingLevel(QsLogging::TraceLevel);
    QDir dir;
    dir.setPath(qApp->applicationDirPath());
    dir.remove(QString("douml") + QString(".log"));
    const QString sLogPath(QDir(qApp->applicationDirPath()).filePath(QString("douml") + QString(".log")));
    QsLogging::DestinationPtr fileDestination(QsLogging::DestinationFactory::MakeFileDestination(sLogPath));
    QsLogging::DestinationPtr debugDestination(QsLogging::DestinationFactory::MakeDebugOutputDestination());
    logger.addDestination(debugDestination.get());
    logger.addDestination(fileDestination.get());
    QLOG_INFO() << "Starting the log";
//#endif /* DEBUG */


    An<EdgeMenuFactory> factory;
    factory->AddFactory(TypeIdentifier<ClassDialog>::id(), CreateClassDialogMenu);
    factory->AddConnectionFunctor(TypeIdentifier<ClassDialog>::id(), ConnectToClassDialog<EdgeMenuDialog>);
    factory->AddFactory(TypeIdentifier<OperationDialog>::id(), CreateClassDialogMenu);
    factory->AddConnectionFunctor(TypeIdentifier<OperationDialog>::id(), ConnectToClassDialog<EdgeMenuDialog>);
    factory->AddFactory(TypeIdentifier<ArtifactDialog>::id(), CreateLimitedDialogMenu);
    factory->AddConnectionFunctor(TypeIdentifier<ArtifactDialog>::id(), ConnectToLimitedDialog<EdgeMenuDialog>);
    factory->AddFactory(TypeIdentifier<ConstructorInitializerDialog>::id(), CreateLimitedDialogMenu);
    factory->AddConnectionFunctorQt4(TypeIdentifier<ConstructorInitializerDialog>::id(), ConnectToLimitedDialog<EdgeMenuDialogQt4>);


    UmlDesktop::init();

    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "DoUML", "settings");
    settings.setIniCodec(QTextCodec::codecForName("UTF-8"));

    QFileInfo settings_info(settings.fileName());
    if(!settings_info.exists())
    {
      QDir settings_path(settings_info.absolutePath());
      if(!settings_path.exists())
      {
        settings_path.mkpath(settings_path.absolutePath());
      }
      QFile settings_file(settings_info.absoluteFilePath());
      settings_file.open(QIODevice::WriteOnly);
      QByteArray data((const char *)settings_ini, settings_ini_len);
      settings_file.write(data);
      settings_file.close();
      EnvDialog::edit(TRUE);
      settings.sync();
    }

    bool overridePresent = QFileInfo("override_transition.txt").exists();
    if(settings.value("Main/compatibility_save") .toInt() == 1 && !overridePresent)
    {
      QMessageBox::warning(0, QObject::tr("Warning"),
         QObject::tr("Douml is working in transitional mode.\n All UI improvements are yours to use,  "
                     "but saving is done in the format of Bouml 4.22 "
                     "which loses all new c++11 and hierarchy specifiers\n\n"
                     "To suppress this warning place empty file override_transition.txt into the application folder\n"
                     "To disable the mode - change compatibility_save parameter to 0 in settings.ini\n"));
    }

    read_doumlrc(); // for virtual desktop
    init_pixmaps();
    init_font();
    Shortcut::init(true);

    bool exec = FALSE;
    bool no_gui = FALSE;

    if (argc > 3) {
        if (!strcmp(argv[2], "-execnogui"))
            exec = no_gui = TRUE;
        else
            exec = !strcmp(argv[2], "-exec");
    }

    UmlWindow * uw = new UmlWindow(exec);

    if (no_gui)
        UmlDesktop::set_nogui();
    else
    {
        uw->showMaximized();
    }

    if (argc > 1) {
        try {
            if ((argc == 3) &&
                !strcmp(argv[2], "-root") &&
                (msg_critical(TR("DO NOT CONFIRM"),
                              TR("Root mode protection\n\n"
                                 "This mode allows me to develop BOUML\n\n"
                                 "do NOT confirm to avoid a disaster !!!\n\n"
                                 "confirm ?"),
                              QMessageBox::Yes, QMessageBox::No)
                 == QMessageBox::Yes)) {
                set_user_id(0);
                set_editor(getenv("BOUML_EDITOR")); // no environment file
                argc = 1;
            }

            uw->load_it(argv[1]);
        }
        catch (...) {
            // cannot read a file
            return -1;

        }
    }

    QObject::connect(&a, SIGNAL(lastWindowClosed()), &a, SLOT(quit()));

    try {
        if (argc > 2) {
            if (exec) {
                bool with_exit = FALSE;

                if (!strcmp(argv[argc - 1], "-exit")) {
                    with_exit = TRUE;
                    argc -= 1;
                }

                WrapperStr cmd = argv[3];
                WrapperStr space = " ";
                int index;

                for (index = 4; index != argc; index += 1)
                    cmd += space + WrapperStr(argv[index]);

                ToolCom::run((const char *) cmd, BrowserView::get_project(), with_exit);
            }
            else
                msg_warning(TR("Error"), TR("Bouml was called with wrong parameters, ignore them"));
        }


        ExitOnError = TRUE;
        a.exec();
    }
    catch (...) {
        ;
    }


    return exit_value();
}
