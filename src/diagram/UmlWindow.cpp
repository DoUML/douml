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




#include "Labeled.h"
#include <qcursor.h>
#include <qsplitter.h>
#include <qstatusbar.h>
#include <qpixmap.h>
#include <QToolBar>
#include <qtoolbutton.h>
#include <qmenubar.h>
#ifndef QT_NO_PRINTER
#include <qprinter.h>
#include <QLineEdit>
#endif
#include <qapplication.h>
#include <qfileinfo.h>
#include <QVBoxLayout>
#include <QCloseEvent>
#include <QTextStream>
#include <QLabel>
#include <QKeyEvent>
#include <QStyle>
#include <QFileDialog>
#include <QStyleFactory>
#include "UmlWindow.h"
#include "UmlDesktop.h"
#include "BrowserView.h"
#include "BrowserPackage.h"
#include "BrowserClass.h"
#include "BrowserUseCase.h"
#include "BrowserState.h"
#include "BrowserActivity.h"
#include "BrowserDiagram.h"
#include "TraceDialog.h"
#include "DiagramView.h"
#include "BrowserArtifact.h"
#include "BrowserComponent.h"
#include "BrowserDeploymentNode.h"
#include "DiagramWindow.h"
#include "GenerationSettings.h"
#include "EnvDialog.h"
#include "HelpDialog.h"
#include "Tool.h"
#include "BrowserSearchDialog.h"
#include "ToolDialog.h"
#include "ShortcutDialog.h"
#include "GreetingsDialog.h"
#include "BasicData.h"
#include "ToolCom.h"
#include "About.h"
#include "UmlGlobal.h"
#include "UmlPixmap.h"
#include "DialogUtil.h"
#include "ui/menufactory.h"
#include "strutil.h"
#include "tool/snippets.h"
#include "CustomWidgets/quickedit.h"
#include "Libs/L_UniversalModels/include/TreeItemInterface.h"
#include "ui/catalogwidget.h"
#include "err.h"
#include "myio.h"
#include "filesave.xpm"
#include "fileopen.xpm"
#include "fileprint.xpm"
#include "browsersearch.xpm"
#include "misc/ClipboardManager.h"
#include "Shortcut.h"
#include "mu.h"
#include "translate.h"
#include <QClipboard>
#include <QApplication>
#include <QShortcut>
#include <QAction>
#include <QMdiSubWindow>
#include <QWhatsThis>
#include <QMdiSubWindow>
#include <QPrintDialog>
static QString TemplateProject;

QString template_project()
{
    return TemplateProject;
}

void set_template_project(QString s)
{
    TemplateProject = s;
}

UmlWindow * UmlWindow::the;
QString projectNewText()
{
    return QObject::tr("To create a <i>new project</i>.<br><br>"
                       "This will ask you to indicate where to create a new directory having the name of the project "
                       "where all the project files will be placed.");
}
QString projectNewFromTemplateText()
{
    return QObject::tr("To create a <i>new project</i> from an already existing one.<br><br>"
                       "This will ask you to indicate where to create a new directory having the name of the project "
                       "where all the project files will be placed.");
}
QString projectOpenText()
{
    return QObject::tr("To open a <i>project</i>.<br><br>"
                       "You can also select the <b>Open command</b> from the Project menu.");
}
QString projectSaveText()
{
    return QObject::tr("To save the project.<br><br>"
                       "You can also select the Save command from the Project menu.");
}
QString projectSaveAsText()
{
    return QObject::tr("To save all in a new project.");
}
QString diagramPrintText()
{
    return QObject::tr("To print the current diagram.");
}
QString cppText()
{
    return QObject::tr("To manage or not C++ and to set or not the C++ definition/declaration "
                       "to the default value when a class/operation/attribute/relation/artefact is created");
}
QString javaText()
{
    return QObject::tr("To manage or not Java and to set or not the Java definition "
                       "to the default value when a class/operation/attribute/relation/artefact is created");
}
QString phpText()
{
    return QObject::tr("To manage or not Php and to set or not the Php definition "
                       "to the default value when a class/operation/attribute/relation/artefact is created");
}
QString pythonText()
{
    return QObject::tr("To manage or not Python and to set or not the Python definition "
                       "to the default value when a class/operation/attribute/relation/artefact is created");
}
QString idlText()
{
    return QObject::tr("To manage or not IDL and to set or not the IDL definition/declaration "
                       "to the default value when a class/operation/attribute/relation/artefact is created");
}
QString verboseText()
{
    return QObject::tr("To ask or not for a verbose code generation");
}
QString preserve_bodiesText()
{
    return QObject::tr("To preserve or not the operations's body");
}
QString add_operation_profileText()
{
    return QObject::tr("To write the operation profile at the beginning of the temporary file when you edit an operation's body");
}
QString viewBrowserStereotypeText()
{
    return QObject::tr("To show or hide the <i>stereotypes</i> "
                       "in the <b>Browser</b>.");
}
QString browserUpText()
{
    return QObject::tr("To select the <i>browser</i> current item's parent.");
}
QString browserSearchText()
{
    return QObject::tr("To search an item in the <i>browser</i>.");
}
QString whatsThisText()
{
    return QObject::tr("To get information about items.");
}

QString fontSizeMenuText()
{
    return QObject::tr("To set the point size of the base font used in the "
                       "<i>browser</i> and the diagrams.");
}
QString formatMenuText()
{
    return QObject::tr("To set the default format of the diagrams.");
}
static QString prevText()
{
    return QObject::tr("To select the previously selected element in the <i>browser</i>.");
}
static QString nextText()
{
    return QObject::tr("To select the next selected element in the <i>browser</i>.");
}
static QString completionText()
{
    return QObject::tr("To ask or not for an auto completion (non case sensitive) in choice list (<i>combo box</i>)");
}

QToolButton *
CreateToolButton(
        QPixmap icon,
        QWidget * receiver,
        const char * boundslot,
        QToolBar * parent,
        QString shown)
{
    QToolButton * newButton = new QToolButton();
    QObject::connect(newButton, SIGNAL(clicked()), receiver, boundslot);
    newButton->setIcon(icon);
    newButton->setText(shown);
    newButton->setToolTip(shown);
    newButton->setMinimumSize(30, 30);
    parent->addWidget(newButton);
    return newButton;
}

UmlWindow::UmlWindow(bool ) : QMainWindow(0)
	, isToolMenuLoaded(false)
    , quitConfirmed(false)
{
    setAttribute(Qt::WA_QuitOnClose);
    setWindowTitle("DoUML");
    the = this;
    commented = 0;
    format = IsoA4;
    //Initializing clipboard
    An<ClipboardManager> clipboard;
    QObject::connect(QApplication::clipboard(), SIGNAL(dataChanged()), clipboard.getData(), SLOT(OnClipboardUpdate()));

    //
    // buttons and menus
    //

    QPixmap openIcon, saveIcon;

    /* This part defines the buttons on the toolbar */
    projectTools = new QToolBar("project operations", this);
    projectTools->setMinimumHeight(20);
    projectTools->setOrientation(Qt::Horizontal);
    addToolBar(Qt::TopToolBarArea, projectTools);

    openIcon = QPixmap(fileopen);
    QToolButton * projectOpen
            = CreateToolButton(openIcon,
                               this, SLOT(load()), projectTools, "open project");

    saveIcon = QPixmap(filesave);
    QToolButton * projectSave
            = CreateToolButton(saveIcon,
                               this, SLOT(save()), projectTools, "save project");

#ifndef QT_NO_PRINTER
    QPixmap printIcon;

    printIcon = QPixmap(fileprint);
    QToolButton * diagramPrint
            = CreateToolButton(printIcon,
                               this, SLOT(print()), projectTools, "print diagram");
    diagramPrint->setToolTip("print diagram");
    diagramPrint->setWhatsThis(diagramPrintText());
#endif


    QPixmap searchIcon = QPixmap(browsersearch);
    QToolButton * browserSearch
            = CreateToolButton(searchIcon,
                               this, SLOT(browser_search()), projectTools, "browser search");
    browserSearch->setWhatsThis(browserSearchText());




    prev = CreateToolButton(*leftPixmap,  this, SLOT(prev_select()), projectTools, "previous selected");
    prev->setWhatsThis(prevText());

    next = CreateToolButton(*rightPixmap, this, SLOT(next_select()), projectTools, "next selected");
    next->setWhatsThis(nextText());



    projectOpen->setWhatsThis(projectOpenText());
    projectSave->setWhatsThis(projectSaveText());

    tbClipboard = CreateToolButton(QIcon(":/root/icons/date_empty.png").pixmap(20,20),
                                   this, SLOT(OnCallClipboardMenu()), projectTools, "Call clipboard");
    tbQuickEdit = CreateToolButton(QIcon(":/root/icons/photo_edit.png").pixmap(100,100),
                                   this, SLOT(OnShowQuickEdit()), projectTools, "Call QuickEdit window(Ctrl-E)");
    tbQuickEdit->setEnabled(false);

    QToolButton * whatsThisButton
            = CreateToolButton(QApplication::style()->standardIcon(QStyle::SP_TitleBarContextHelpButton).pixmap(200,2000),
                               this, SLOT(whats_this()), projectTools, "Whats's this?");


    generateLabel = new QLabel(tr("Generate:"));
    QFont font = generateLabel->font();
    font.setBold(true);
    generateLabel->setFont(font);
    generateLabelAction = projectTools->addWidget(generateLabel);


    whatsThisButton->setWhatsThis(whatsThisText());

    this->setMenuBar(new QMenuBar(this));
    projectMenu = new QMenu(tr("&Project"),this);
    menuBar()->addMenu(projectMenu);
    connect(projectMenu, SIGNAL(aboutToShow()),
            this, SLOT(projectMenuAboutToShow()));
    // Add at least one menu item to each menu right after creation, otherwise the
    // menus are never shown on OS X with Qt built againt Cocoa.
    projectMenu->addAction(QObject::tr("&New"), this, SLOT(newProject()));

    windowsMenu = new QMenu(tr("&Windows"),this);
    //windowsMenu->setCheckable(TRUE);
    connect(windowsMenu, SIGNAL(aboutToShow()),
            this, SLOT(windowsMenuAboutToShow()));
    menuBar()->addMenu(windowsMenu);

    toolMenu = new QMenu(tr("&Tools"),this);
    connect(toolMenu, SIGNAL(aboutToShow()), this, SLOT(toolMenuAboutToShow()));
    menuBar()->addMenu(toolMenu);
    toolMenu->addAction(tr("Show &Trace Window"), this, SLOT(show_trace()));

    langMenu = new QMenu(tr("&Languages"),this);
    menuBar()->addMenu(langMenu);
    //langMenu->setCheckable(TRUE);
    connect(langMenu, SIGNAL(aboutToShow()),
            this, SLOT(langMenuAboutToShow()));

    use_cpp_id = new QAction(tr("C++ management and default declaration/definition"),this);
    use_cpp_id->setCheckable(true);
    connect(use_cpp_id,SIGNAL(triggered()), this, SLOT(use_cpp()));
    langMenu->addAction(use_cpp_id);
    use_cpp_id->setWhatsThis(cppText());
    use_cpp_id->setChecked(GenerationSettings::cpp_get_default_defs());
    use_cpp_id->setEnabled(false);


    use_java_id = new QAction(tr("Java management and default definition"), this);
    use_java_id->setCheckable(true);
    connect(use_java_id, SIGNAL(triggered()), this,SLOT(use_java()));
    langMenu->addAction(use_java_id);
    use_java_id->setChecked(GenerationSettings::java_get_default_defs());
    use_java_id->setWhatsThis(javaText());
    use_java_id->setEnabled(false);

    use_php_id =
            new QAction(tr("Php management and default definition"), this);
    use_php_id->setCheckable(true);
    connect(use_php_id, SIGNAL(triggered()), this,SLOT(use_php()));
    langMenu->addAction(use_php_id);
    use_php_id->setChecked(GenerationSettings::php_get_default_defs());
    use_php_id->setWhatsThis(phpText());
    use_php_id->setEnabled(false);

    use_python_id =
            new QAction(tr("Python management and default definition"), this);
    use_python_id->setCheckable(true);
    connect(use_python_id, SIGNAL(triggered()),this, SLOT(use_python()));
    langMenu->addAction(use_python_id);
    use_python_id->setChecked( GenerationSettings::python_get_default_defs());
    use_python_id->setWhatsThis(pythonText());
    use_python_id->setEnabled(false);

    use_idl_id =
            new QAction(tr("Idl management and default declaration"), this);
    use_idl_id->setCheckable(true);
    connect(use_idl_id, SIGNAL(triggered()),this, SLOT(use_idl()));
    langMenu->addAction(use_idl_id);
    use_idl_id->setChecked( GenerationSettings::idl_get_default_defs());
    use_idl_id->setWhatsThis(idlText());
    use_idl_id->setEnabled(false);

    langMenu->addSeparator();

    verbose_gen_id =
            new QAction(tr("Verbose code generation"), this);
    verbose_gen_id->setCheckable(true);
    connect(verbose_gen_id, SIGNAL(triggered()),this, SLOT(verbose()));
    langMenu->addAction(verbose_gen_id);
    verbose_gen_id->setWhatsThis(verboseText());
    verbose_gen_id->setEnabled(false);

    preserve_bodies_id =
            new QAction(tr("Preserve operations's body"), this);
    preserve_bodies_id->setCheckable(true);
    connect(preserve_bodies_id, SIGNAL(triggered()),this, SLOT(preserve()));
    langMenu->addAction(preserve_bodies_id);
    preserve_bodies_id->setWhatsThis( preserve_bodiesText());
    preserve_bodies_id->setEnabled(false);

    add_operation_profile_id =
            new QAction(tr("Add operation profile on body edition"), this);
    add_operation_profile_id->setCheckable(true);
    connect(add_operation_profile_id, SIGNAL(triggered()),this, SLOT(addoperationprofile()));
    langMenu->addAction(add_operation_profile_id);
    add_operation_profile_id->setWhatsThis(add_operation_profileText());
    add_operation_profile_id->setEnabled(false);

    miscMenu = new QMenu(tr("&Miscellaneous"),this);
    menuBar()->addMenu( miscMenu);
    //miscMenu->setCheckable(TRUE);
    connect(miscMenu, SIGNAL(aboutToShow()),
            this, SLOT(miscMenuAboutToShow()));

    show_browser_stereotypes_id =
            new QAction(tr("Show s&tereotypes in browser"), this);
    show_browser_stereotypes_id->setCheckable(true);
    connect(show_browser_stereotypes_id, SIGNAL(triggered()),this,SLOT(show_stereotypes()));
    show_browser_stereotypes_id->setChecked(TRUE);
    show_browser_stereotypes_id->setWhatsThis(viewBrowserStereotypeText());
    show_browser_stereotypes_id->setEnabled(false);
    miscMenu->addAction(show_browser_stereotypes_id);

    completion_id =
            new QAction(tr("Completion in dialog"), this);
    completion_id->setCheckable(true);
    connect(completion_id,SIGNAL(triggered()),this,SLOT(do_completion()));
    completion_id->setChecked( TRUE);
    completion_id->setWhatsThis( completionText());
    miscMenu->addAction(completion_id);


    QMenu * pmstyle = new QMenu(tr("Style"),this);
    QAction *action;
    QStringList styles = QStyleFactory::keys();
    foreach (QString currentStyle, styles) {
        action = pmstyle->addAction(currentStyle);
        connect(action, SIGNAL(triggered(bool)), this, SLOT(setApplicationStyleSlot()));
    }
    if(styles.count())
        miscMenu->addMenu(pmstyle);
    fontSizeMenu = new QMenu(tr("Font size"),this);
    //fontSizeMenu->setCheckable(TRUE);
    connect(fontSizeMenu, SIGNAL(aboutToShow()),
            this, SLOT(fontSizeMenuAboutToShow()));

#define FONTSIZEMIN 6
#define FONTSIZESUP 16

    for (int i = FONTSIZEMIN; i < FONTSIZESUP; i += 1)
    {
        action = new QAction(QString::number(i), this);
        connect(action, SIGNAL(triggered()), this, SLOT(setFontSize()));
        action->setCheckable(true);
        fontSizeMenu->addAction(action);
    }

    miscMenu->addMenu(fontSizeMenu);
    fontSizeMenu->setWhatsThis(fontSizeMenuText());

    formatMenu = new QMenu(tr("Diagram default format"),this);
    formatLandscapeMenu = new QMenu(tr("Landscape formats"),this);
    connect(formatMenu, SIGNAL(aboutToShow()),
            this, SLOT(formatMenuAboutToShow()));
    init_format_menu(formatMenu, formatLandscapeMenu);

    miscMenu->addMenu(formatMenu);
    formatMenu->setWhatsThis(formatMenuText());

    miscMenu->addSeparator();

    shortcut_id =
            new QAction(tr("Edit shortcuts"), this);
    miscMenu->addAction(shortcut_id);
    connect(shortcut_id, SIGNAL(triggered()), this, SLOT(edit_shortcuts()));
    menuBar()->addSeparator();

    action =
            new QAction(tr("Set environment"), this);
    miscMenu->addAction(action);
    connect(action, SIGNAL(triggered()),this, SLOT(edit_env()));

    img_root_dir_id =
            new QAction(tr("Set images root dir"), this);
    miscMenu->addAction(img_root_dir_id);
    connect(img_root_dir_id, SIGNAL(triggered()),this, SLOT(edit_image_root_dir()));

    QMenu * help = new QMenu(tr("&Help"),this);
    menuBar()->addMenu(help);

    action =
            new QAction(tr("&About"), help);
    connect(action, SIGNAL(triggered()), this,SLOT(about()));
    action->setShortcut(::Qt::Key_F2);
    help->addAction(action);

    action =
            new QAction(tr("About&Qt"), help);
    connect(action, SIGNAL(triggered()), this,SLOT(aboutQt()));
    help->addAction(action);

    help->addSeparator();

    action =
            new QAction(tr("&Help"), this);
    connect(action, SIGNAL(triggered()), this,SLOT(help()));
    action->setShortcut(::Qt::Key_F1);
    help->addAction(action);

    action =
            new QAction(tr("What's This?"), this);
    connect(action, SIGNAL(triggered()), this,SLOT(whats_this()));
    action->setShortcut(::Qt::SHIFT + ::Qt::Key_F1);
    help->addAction(action);

    //
    // sub windows
    //

    //spl1 = new QSplitter(Qt::Horizontal, this, "spl1");
    //spl2 = new QSplitter(Qt::Vertical, spl1, "spl2");
    spl1 = new QSplitter(Qt::Horizontal, this);
    spl2 = new QSplitter(Qt::Vertical, /*spl1*/this);
    wdgCatalog = new CatalogWidget;
    quickEdit = new QuickEdit();
    browser = new BrowserView();
    connect(browser, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(OnChooseQuickEditMode(QTreeWidgetItem*, QTreeWidgetItem*)));

    wdgCatalog->Init(this, browser);
    connect(wdgCatalog, SIGNAL(markedRemove(QString,int)), browser, SLOT(OnUnmarkItem(QString,int)));
    connect(wdgCatalog, SIGNAL(allMarkedRemove()), browser, SLOT(OnUnmarkAll()));


    quickEdit->Init(this, browser);
    quickEdit->setWindowFlags(Qt::WindowStaysOnTopHint);
    sh1 = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_E),this, SLOT(OnShowQuickEdit()),SLOT(OnShowQuickEdit()), Qt::ApplicationShortcut);


    splTreeTab = new QSplitter(Qt::Vertical, spl1);
    splTreeTab->addWidget(browser);
    splTreeTab->addWidget(wdgCatalog);
    cppAction = new QAction(tr("C++"),0);
    connect(cppAction, SIGNAL(triggered()),browser, SLOT(OnGenerateCpp()));
    javaAction = new QAction( tr("Java"),0);
    connect(javaAction, SIGNAL(triggered()),browser, SLOT(OnGenerateJava()));
    phpAction = new QAction(tr("Php"),0);
    connect(phpAction, SIGNAL(triggered()),browser, SLOT(OnGeneratePhp()));
    pythonAction = new QAction(tr("Python"),0);
    connect(pythonAction, SIGNAL(triggered()),browser, SLOT(OnGeneratePython()));
    idlAction = new QAction(tr("Idl"),0);
    connect(idlAction, SIGNAL(triggered()),browser, SLOT(OnGenerateIdl()));
    projectTools->addAction(cppAction);
    projectTools->addAction(javaAction);
    projectTools->addAction(phpAction);
    projectTools->addAction(pythonAction);
    projectTools->addAction(idlAction);
    cppAction->setVisible(false);
    javaAction->setVisible(false);
    phpAction->setVisible(false);
    pythonAction->setVisible(false);
    idlAction->setVisible(false);
    ws = new QMdiArea(spl2);
    //ws->setToolTip("mdi area");
    comment = new MultiLineEdit(spl2);
    comment->setToolTip("coment");

    set_commented(0);
    connect(comment, SIGNAL(textChanged()), this, SLOT(comment_changed()));
    spl2->addWidget(ws);
    spl2->addWidget(comment);
    spl1->addWidget(splTreeTab);
    spl1->addWidget(spl2);
    int w = (UmlDesktop::width() * 15) / 16;

    resize(w, (UmlDesktop::height() * 7) / 8);
    UmlDesktop::tocenter(this);

    // browser width = total width/4
    QList<int> lsz = spl1->sizes();

    lsz.first() = w / 4;
    lsz.last() = w - lsz.first();

    spl1->setSizes(lsz);

    // on resize try to resize only the diagrams
    spl1->setStretchFactor(spl1->indexOf(browser), 1);
    spl2->setStretchFactor(spl2->indexOf(comment),1);

#ifdef _WS_QWS_
    // Qt/Embedded XOR drawing not yet implemented.
    spl1->setOpaqueResize(TRUE);
    spl2->setOpaqueResize(TRUE);
#endif

    setCentralWidget(spl1);

    //
    // read historic
    //
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "DoUML", "settings");
    settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
    settings.beginGroup("Recent_Files");
    for(int i = 1; i <= 10; ++i)
    {
        const QString line = settings.value(tr("File%1").arg(i)).toString();
        if(!line.isEmpty())
        {
            historic.append(line);
        }
        else
        {
            break;
        }
    }
    settings.endGroup();

    clear_select_historic();
    statusBar()->showMessage(tr("Ready"), 20000);
}
UmlWindow::~UmlWindow()
{
    BrowserView::remove_temporary_files();
    set_user_id(-1);
    exit(0);
}
void UmlWindow::init_format_menu(QMenu * m, QMenu * lm)
{
    //m->setCheckable(TRUE);
    //lm->setCheckable(TRUE);
    QAction* action;
    int i;

    for (i = 0; i <= IsoA5; i += 1)
    {
        action = new QAction(QString("Iso ") + stringify((CanvasFormat) i), this);
        action->setCheckable(true);
        connect(action, SIGNAL(triggered()),this, SLOT(setFormat()));
        m->addAction(action);
        action->setData(i);
    }

    for (; i <= UsE; i += 1)
    {
        action = new QAction(QString("Ansi ") + stringify((CanvasFormat) i), this);
        action->setCheckable(true);
        connect(action, SIGNAL(triggered()),this,SLOT(setFormat()));
        m->addAction(action);
        action->setData(i);
    }
    for (; i != IsoA0Landscape; i += 1)
    {
        action = new QAction(stringify((CanvasFormat) i), this);
        action->setCheckable(true);
        connect(action, SIGNAL(triggered()),this,SLOT(setFormat()));
        m->addAction(action);
        action->setData(i);
    }

    m->addSeparator();
    m->addMenu(lm);

    for (; i <= IsoA5Landscape; i += 1)
    {
        action = new QAction(QString("Iso ") + stringify((CanvasFormat) i), this);
        action->setCheckable(true);
        connect(action, SIGNAL(triggered()),this,SLOT(setFormat()));
        lm->addAction(action);
        action->setData(i);
    }

    for (; i <= UsELandscape; i += 1)
    {
        action = new QAction(QString("Ansi ") + stringify((CanvasFormat) i), this);
        action->setData(i);
        action->setCheckable(true);
        connect(action, SIGNAL(triggered()),this,SLOT(setFormat()));
        lm->addAction(action);
    }

    for (; i != CanvasFormatSup; i += 1)
    {
        action = new QAction(stringify((CanvasFormat) i), this);
        action->setCheckable(true);
        connect(action, SIGNAL(triggered()),this,SLOT(setFormat()));
        lm->addAction(action);
        action->setData(i);
    }
}

void UmlWindow::projectMenuAboutToShow()
{
    abort_line_construction();

    projectMenu->clear();

    if (!BrowserNode::edition_active()) {
        (void) user_id(); // force doumlrc read to have TemplateProject

        QAction *id;
        bool enabled = (browser->get_project() != 0);
        QPixmap openIcon = QPixmap(fileopen);
        QPixmap saveIcon = QPixmap(filesave);

        id = projectMenu->addAction(QObject::tr("&New"), this, SLOT(newProject()));
        id->setWhatsThis(projectNewText());


        if (!TemplateProject.isEmpty()) {
            id = projectMenu->addAction(QObject::tr("Create from &Template"), this, SLOT(newFromTemplate()));
            id->setWhatsThis(projectNewFromTemplateText());
        }

        id = projectMenu->addAction(openIcon, QObject::tr("&Open"), this, SLOT(load()),
                                    ::Qt::CTRL +::Qt::Key_O);
        id->setWhatsThis(projectOpenText());

        id = projectMenu->addAction(saveIcon, QObject::tr("&Save"), this, SLOT(save()),
                                    ::Qt::CTRL +::Qt::Key_S);
        id->setWhatsThis(projectSaveText());
        id->setEnabled(enabled);

        id = projectMenu->addAction(QObject::tr("Save &as..."), this, SLOT(saveAs()),
                                    ::Qt::CTRL +::Qt::SHIFT +::Qt::Key_S);
        id->setWhatsThis(projectSaveAsText());
        id->setEnabled(enabled);

        projectMenu->addSeparator();
        projectMenu->addAction(QObject::tr("&Close"), this, SLOT(close()),
                               ::Qt::CTRL +::Qt::Key_W);
        id->setEnabled(enabled);
        projectMenu->addAction(QObject::tr("&Quit"), this, SLOT(quit()),
                               ::Qt::CTRL +::Qt::Key_Q);

        // edit

        if (enabled) {
            bool writable = browser->get_project()->is_writable();
            QMenu * ed; // = new QMenu(this);

            projectMenu->addSeparator();
            ed = projectMenu->addMenu(tr("Edit"));
            //projectMenu->addAction(TR("Edit"), ed);

            id = ed->addAction(tr("Edit generation settings"), this, SLOT(edit_gen_settings()));
            id->setWhatsThis( QObject::tr("to set how an Uml type is compiled in C++ etc...,"
                                          "to set the default parameter passing, to set the default code"
                                          "produced for an attribute etc..., and to set the root directories"));

            id->setEnabled( writable);

            id = ed->addAction(tr("Edit default stereotypes"), this, SLOT(edit_stereotypes()));
            id->setWhatsThis( QObject::tr("to set the default stereotypes list"));
            id->setEnabled( writable);

            ed->addSeparator();

            id = ed->addAction(tr("Edit class settings"), this, SLOT(edit_class_settings()));
            id->setWhatsThis( QObject::tr("to set the sub classes settings"));
            id->setEnabled( writable);

            id = ed->addAction(tr("Edit drawing settings"), this, SLOT(edit_drawing_settings()));
            id->setWhatsThis( QObject::tr("to set how the sub <i>diagrams</i>'s items must be drawn"));
            id->setEnabled( writable);
        }

        // historic

        projectMenu->addSeparator();

        for (int i(0); i < historic.size(); ++i)
        {
            id = projectMenu->addAction(historic.at(i), this, SLOT(historicActivated()));
            //projectMenu->setItemParameter(id, i);
            id->setWhatsThis(tr("to open this project.<br><br>The historic is saved in <i>settings.ini</i>"));
            id->setData(QVariant(i));
        }
    }
    else
        msg_warning("DoUML",
                    QObject::tr("Nothing available while a dialog is opened"));
}

void UmlWindow::historicActivated()
{
    QAction *action = static_cast<QAction *>( sender());
    int i = action->data().toInt();
    bool idExists = (i < historic.count() && i >= 0);

    if (!idExists)
    {
        statusBar()->showMessage(tr("Loading aborted as supplied project id is out of history array bounds"), 2000);
        return;
    }

    close();
    bool projectSuccessfullyClosed = browser->get_project() == 0;

    if (!projectSuccessfullyClosed) {
        statusBar()->showMessage(tr("Loading aborted as currently opened project unloading failed"), 2000);
        return;
    }

    load(historic[i]);
}

void UmlWindow::clear()
{
    while (! the->ws->subWindowList().isEmpty()) {
        DiagramWindow * w = (DiagramWindow *) the->ws->subWindowList().at(0)->widget();
        DiagramWindow * sw = (DiagramWindow *) the->ws->subWindowList().at(0);

        if(w)
        {
            w->dont_save();
            w->setParent(0);
            delete w;
        }
        delete sw;
    }


    if (BrowserSearchDialog::get() != 0)
        BrowserSearchDialog::get()->close();
}

void UmlWindow::toolMenuAboutToShow()
{

    abort_line_construction();

    bool cpp = GenerationSettings::cpp_get_default_defs();
    bool java = GenerationSettings::java_get_default_defs();
    bool php = GenerationSettings::php_get_default_defs();
    bool python = GenerationSettings::python_get_default_defs();
    bool idl = GenerationSettings::idl_get_default_defs();
    bool lang_except_idl = cpp || java || php || python;

    toolMenu->clear();
    toolMenu->addAction(tr("Show &Trace Window"), this, SLOT(show_trace()));

    if (browser->get_project() != 0) {

		isToolMenuLoaded = true;

        if (lang_except_idl | idl) {
            toolMenu->addSeparator();

            if (cpp)
                toolMenu->addAction(tr("Generate C++"), this, SLOT(cpp_generate()), ::Qt::CTRL +::Qt::Key_G);

            if (java)
                toolMenu->addAction(tr("Generate Java"), this, SLOT(java_generate()), ::Qt::CTRL +::Qt::Key_J);

            if (php)
                toolMenu->addAction(tr("Generate Php"), this, SLOT(php_generate()), ::Qt::CTRL +::Qt::Key_H);

            if (python)
                toolMenu->addAction(tr("Generate Python"), this, SLOT(python_generate()), ::Qt::CTRL +::Qt::Key_Y);

            if (idl)
                toolMenu->addAction(tr("Generate Idl"), this, SLOT(idl_generate()), ::Qt::CTRL +::Qt::Key_I);
        }

        if (!BrowserNode::edition_active()) {
            if (lang_except_idl) {
                toolMenu->addSeparator();

                if (cpp) {
                    toolMenu->addAction(tr("Reverse C++"), this, SLOT(cpp_reverse()));
                    toolMenu->addAction(tr("Roundtrip C++"), this, SLOT(cpp_roundtrip()));
                }

                if (java) {
                    toolMenu->addAction(tr("Reverse Java"), this, SLOT(java_reverse()));
                    toolMenu->addAction(tr("Roundtrip Java"), this, SLOT(java_roundtrip()));
                }

                if (php)
                    toolMenu->addAction(tr("Reverse Php"), this, SLOT(php_reverse()));


                if (java) {
                    toolMenu->addSeparator();
                    toolMenu->addAction(tr("Java Catalog"), this, SLOT(java_catalog()));
                }

                if (preserve_bodies()) {
                    toolMenu->addSeparator();

                    if (cpp)
                        toolMenu->addAction(tr("Roundtrip C++ bodies"), this, SLOT(cpp_roundtripbody()));

                    if (java)
                        toolMenu->addAction(tr("Roundtrip Java bodies"), this, SLOT(java_roundtripbody()));

                    if (php)
                        toolMenu->addAction(tr("Roundtrip Php bodies"), this, SLOT(php_roundtripbody()));

                    if (python)
                        toolMenu->addAction(tr("Roundtrip Python bodies"), this, SLOT(python_roundtripbody()));
                }
            }

            if (BrowserClass::find("UmlBaseItem") != 0) {
                toolMenu->addSeparator();
                toolMenu->addAction(tr("Plug-out upgrade"), this, SLOT(plug_out_upgrade()));
            }

            Tool::menu_insert(toolMenu, UmlProject, this, SLOT(run_tool()));
        }

        if (browser->get_project()->is_writable()) {
            toolMenu->addSeparator();
            toolMenu->addAction(tr("Tools settings"), this, SLOT(tool_settings()));
            toolMenu->addAction(tr("Import settings"), this, SLOT(import_tool_settings()));
        }
    }
}

void UmlWindow::set_commented(BrowserNode * bn)
{
    if (the != 0) {
        // not on exit
        UmlWindow * him = the;
        bool same = (him->commented == bn);

        the = 0;    // to do nothing in comment_changed() which is called

        him->commented = bn;

        if (bn != 0) {
            him->comment->setText(bn->get_comment());
            him->comment->setReadOnly(!bn->is_writable());
            him->statusBar()->showMessage(bn->get_data()->definition(TRUE, TRUE));

            if (! same)
                him->is_selected(bn);
        }
        else {
            him->comment->clear();
            him->comment->setReadOnly(TRUE);
            him->statusBar()->clearMessage();
        }

        the = him;
    }
}

void UmlWindow::update_comment_if_needed(BrowserNode * bn)
{
    if ((the != 0) && (the->commented == bn))
        set_commented(bn);
}

void UmlWindow::comment_changed()
{
    if ((the != 0) && (the->commented != 0) &&
            (the->commented->get_comment() != the->comment->text())) {
        the->commented->set_comment(the->comment->text());
        the->commented->modified();
        the->commented->package_modified();
    }
}

void UmlWindow::set_message(const QString & m)
{
    the->statusBar()->showMessage(m);
}

QMdiArea  * UmlWindow::get_workspace()
{
    return the->ws;
}

CanvasFormat UmlWindow::default_format()
{
    return the->format;
}

QString UmlWindow::images_root_dir()
{
    return the->img_root_dir;
}

void UmlWindow::set_images_root_dir(QString s)
{
    the->img_root_dir = s;
}

void UmlWindow::newProject()
{
    abort_line_construction();
    if (BrowserNode::edition_active())
        return;

    close();
    if (browser->get_project() == 0) {
        QString f = QFileDialog::getSaveFileName(
                    this,
                    tr("Enter a folder name, this folder will be created and will name the new project"),
                    last_used_directory());

        if (!f.isEmpty()) {
            set_last_used_directory(f);

            QDir di;

            if (di.mkdir(f)) {
                di.cd(f);
                browser->set_project(di);
                Tool::defaults();
                browser->get_project()->BrowserPackage::save_all(FALSE);

                msg_warning(tr("New project"),
                            QObject::tr("Do not forget to set the target languages list\n"
                                        "through the 'Languages' menu\n"
                                        "\n"
                                        "If you program in Java, the Java Catalog plug-out\n"
                                        "will help you, use it !"));
            }
            else
                msg_critical(tr("New project"), QObject::tr("Cannot create directory ") + f);
        }
    }
}

void UmlWindow::newFromTemplate()
{
    abort_line_construction();

    if (!BrowserNode::edition_active()) {
        close();
        load(TemplateProject, true);
    }
}

void UmlWindow::load()
{
    abort_line_construction();

    if (!BrowserNode::edition_active()) {
        close();

        if (browser->get_project() == 0) {
            QString fn = QFileDialog::getOpenFileName(
                        this, tr("Select DoUML project file"), last_used_directory(), QLatin1String("*.prj"));

            if (!fn.isEmpty()) {
                set_last_used_directory(fn);
                load(fn);
            }

            else
                statusBar()->showMessage(tr("Loading aborted"), 2000);
        }
    }
    if(BrowserNode::marked_nodes().count()!=0)
        set_marked_generation();
    else
        set_selected_generation();
}
void UmlWindow::historic_add(QString fn)
{
    //QStringList::Iterator it = the->historic.indexOf(fn);
    the->historic.removeOne(fn);

    //if (it != the->historic.end())
    //  the->historic.remove(it);

    the->historic.prepend(fn);

    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "DoUML", "settings");
    settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
    settings.beginGroup("Recent_Files");
    for(int i = 0; i < 10; i++)
    {
        QString s;
        if(i < the->historic.size())
        {
            settings.setValue(tr("File%1").arg(i+1), the->historic.at(i));
        }
        else
        {
            break;
        }
    }
    settings.endGroup();
}

void UmlWindow::load_it()
{
    the->load();
}

void UmlWindow::load_it(QString fn)
{
    // plug-out
    close_it();
    the->load(fn);
}

void UmlWindow::reload_it()
{
    close_it();
    the->load(the->historic.first());
}

void UmlWindow::load(QString fn, bool forcesaveas)
{
    clear_select_historic();

    QFileInfo fi(fn);

    if (!fi.exists()) {
        return;
    }

    QDir di(fi.dir());
    QRegExp dotSearch("[.]");
    int pos  = dotSearch.lastIndexIn(fi.fileName());
    int nameLength = fi.fileName().length();
    QString filename = fi.fileName().left(nameLength - (nameLength - pos));

    //s.truncate(s.length() - 4);   // QFileInfo::baseName remove all after first dot

    if (di.dirName() != filename) {
        msg_critical("Uml",
                     QObject::tr("The name of the project and the name of\n"
                                 "the directory containing it must be the same\n\n") +
                     QObject::tr("Project name : ") + filename + "\n" +
                     QObject::tr("Directory name : ") + di.dirName());
        close_it();
        return;
    }

    historic_add(fi.absoluteFilePath());

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    set_in_import(FALSE);
    browser->set_project(di);
    BrowserNode::pre_load();

    unsigned format;

    PRE_TRY;

    try {

        format = browser->get_project()->load(TRUE);

    }
    catch (int) {
        QApplication::restoreOverrideCursor();
        close_it();
        clear_select_historic(); // in case selection are done by error

        return;
    }

    POST_TRY;

    BrowserNode::post_load(FALSE);
    idmax_add_margin();
    browser->get_project()->setOpen(TRUE);
    QApplication::restoreOverrideCursor();
    setWindowTitle("DoUML : " + fn);

    if (format < 20) {
        BrowserClass::plug_out_conversion();

        QString new_st = GenerationSettings::new_java_enums();

        BrowserClass::new_java_enums(new_st);

        browser->get_project()->package_modified();
        msg_warning("DoUML",
                    "Project conversion done.<br><br>"
                    "A <i>save-as</i> is forced now to save the result "
                    "in a new project, then the project will be closed");
        saveAs();
        close_it();     // write access of added items not ok
    }
    else if (format < 22) {
        QString new_st = GenerationSettings::new_java_enums();
        bool must_save_as = BrowserClass::new_java_enums(new_st);

        QString m = ("Class stereotype <i>enum_pattern</i> added.<br><br>"
                     "The classes having the stereotype <i>enum</i> and a non empty "
                     "Java definition have now the stereotype <i>enum_pattern</i>.<br><br>");

        if (!new_st.isEmpty())
            m = "The class stereotype <i>enum_pattern</i> is renamed <i>" + new_st +
                    "</i>and the classes having the stereotype <i>enum_pattern</i> have now the stereotype <i>" +
                    new_st + "</i><br><br>" + m;

        if (must_save_as)
            m += "A <i>save-as</i> is forced now to save the result in a new project";

        msg_warning("DoUML", m);

        if (must_save_as || forcesaveas) {
            if (! saveas_it())
                close_it();
        }
        else
            browser->get_project()->package_modified();
    }
    else if (forcesaveas) {
        if (! saveas_it())
            close_it();
    }
    else if ((user_id() != 0) && (my_baseName(fi) == "empty")) {
        set_user_id(-1);
        saveAs();
    }
    else {
        BrowserView::remove_temporary_files();
        read_session();
    }

    clear_select_historic(); // in case selection are done by error
    setup_generator_action_visibility();

}

void UmlWindow::save()
{
    abort_line_construction();

    if (browser->get_project()) {
        if (!BrowserNode::edition_active()) {
            // hide the window else the user see that
            // all the not-open diagram window will be open
            // to save their content
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            ws->hide();
            BrowserPackage::save_all(TRUE);
            ws->show();
            QApplication::restoreOverrideCursor();
        }
        else
            msg_warning("DoUML",
                        QObject::tr("Saving can't be done while a dialog is opened"));
    }
}

void UmlWindow::save_it()
{
    the->save();
}

void UmlWindow::saveAs()
{
    abort_line_construction();
    saveas_it();
}

bool UmlWindow::saveas_it()
{
    if (!the->browser->get_project() || BrowserNode::edition_active())
        return false;

    for (;;) {
        QString f = QFileDialog::getSaveFileName(
                    the,
                    tr("Enter a folder name, this folder will be created and will name the new project"),
                    last_used_directory());

        if (!f.isEmpty()) {
            set_last_used_directory(f);

            QDir d(f);

            if (d.dirName() == "empty")
                msg_critical("Error", QObject::tr("'empty' is reserved to the empty plug-out"));
            else {
                QDir di;

                while (!di.mkdir(f)) {
                    if (msg_critical("Error", QObject::tr("Cannot create directory\n") + f,
                                     QMessageBox::Retry, QMessageBox::Abort)
                            != QMessageBox::Retry) {
                        if (!strcmp(the->browser->get_project()->get_name().toLatin1().constData(), "empty"))
                            exit(0);

                        return false;
                    }
                }

                QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
                di.cd(f);
                the->ws->hide();

                if (the->browser->save_as(di))
                    BrowserPackage::save_all(FALSE);

                the->ws->show();
                QApplication::restoreOverrideCursor();
                the->setWindowTitle("DoUML : " + f);
                return true;
            }
        }
        else if (!strcmp(the->browser->get_project()->get_name().toLatin1().constData(), "empty"))
            exit(0);
        else
            return false;
    }
    return false;
}


bool UmlWindow::can_close()
{
    BrowserPackage * packagePtr = browser->get_project();
    if(!packagePtr)
        return true;
    bool mustBeSaved = BrowserPackage::must_be_saved();
    if(!mustBeSaved)
        return true;


    if (mustBeSaved)
    {
        int result = msg_warning("DoUML", QObject::tr("The project is modified, save it ?\n"),
                                 QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel);

        switch (result)
        {
        case QMessageBox::Yes:
            ws->hide();
            BrowserPackage::save_all(TRUE);
            ws->show();
            break;

        case QMessageBox::No:
            statusBar()->showMessage(tr("Close aborted"), 2000);
            return true;
        case QMessageBox::Cancel:
        {
            statusBar()->showMessage(tr("Close aborted"), 2000);
            return false;
        }
        }
    }
    return true;
}

void UmlWindow::set_marked_generation()
{
    QPalette pal;
    pal.setColor(QPalette::Button, QColor(255,210,210));
    the->generateLabel->setAutoFillBackground(true);   // <--- this is the important line
    the->generateLabel->setPalette(pal);
}

void UmlWindow::set_selected_generation()
{
    QPalette pal;
    pal.setColor(QPalette::Button, QColor(182,206,252));
    the->generateLabel->setAutoFillBackground(true);   // <--- this is the important line
    the->generateLabel->setPalette(pal);
}

void UmlWindow::close()
{
    abort_line_construction();
    bool editionActive = !BrowserNode::edition_active();
    if (quitConfirmed || // if we already confirmed then it must be because the window
         // is being closed by the quit command, not the close project command,
         // so don't ask again to safe if modified, the user already answered the question
         (can_close() && editionActive))
    {
        close_it();
    }
    else
    {
        // reset the quitConfirmed flag so that if for some reason the project did not
        // close, then the user will, on a future close project command, be appropriately
        // warned to save if modified.
        quitConfirmed = false;
    }
}

void UmlWindow::do_close()
{
    if (the)
        the->close();
}
void UmlWindow::closeEvent(QCloseEvent *e)
{
    quit();
    if(!quitConfirmed)
        e->ignore();
#if 0

    if (can_close()) {
        save_session();
        e->accept();
    }

#endif
}

void UmlWindow::close_it()
{
    if (user_id() != 0)
    {
        clear_select_historic();

        ToolCom::close_all();

        the->save_session();

        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

        // close all diagram windows
        // do not hide ws, else a future diagram window opening will crash !
        // [lgfreitas] browser clearing already delete each window
        the->clear();

        // empty the browser
        set_commented(0);
        the->wdgCatalog->CleanupBeforeNewProject();
        the->browser->clear();

        // remove tools
        if (the->isToolMenuLoaded) {
            // Only clear the menu if it has already been loaded once, otherwise the
            // menu is never shown on OS X if Qt is built againt Cocoa.
            the->toolMenu->clear();
        }

        QApplication::restoreOverrideCursor();
        the->setWindowTitle("DoUML");
    }

    Tool::init();
    GenerationSettings::init();
    BrowserPackage::init();
    BrowserClass::init();
    BrowserState::init();
    BrowserActivity::init();
    BrowserUseCase::init();
    BrowserComponent::init();
    BrowserArtifact::init();
    BrowserDeploymentNode::init();
    DiagramView::init();
}

void UmlWindow::quit()
{
    abort_line_construction();

    bool canClose = true;
    bool editionActive = false;

    Q_ASSERT(!quitConfirmed); // if this happens, then probably it was not initialized by the contructor

    if ((editionActive = BrowserNode::edition_active() == false) && (canClose = can_close()) == true)
    {
        if (browser->get_project() != 0) {
            save_session();
            BrowserView::remove_temporary_files();
            // delete lock
            set_user_id(-1);
        }
        quitConfirmed = true;

        // Call the close project command now to prevent a segmentation fault
        // if the project was not manually closed by the user before quitting douml,
        // i.e. if the user used the quit command with a project currently opened.
        // (note: the segmentation fault was due to the static object 'BasicParent BasicParent::the'
        // which is declared in 'BasicData.cpp' that still had invalid references to child items and
        // tried to delete them in its destructor.
        close();

        QApplication::exit(0);
        return;
    }
    quitConfirmed = canClose;
}

void UmlWindow::do_quit()
{
    if (the)
        the->quit();
}

void UmlWindow::save_session()
{
    if ((browser->get_project() == 0) || (user_id() <= 0))
        return;

    QDir d = BrowserView::get_dir();
    char fn[32];

    sprintf(fn, "%d.session", user_id());

    QFile fp(d.absoluteFilePath(fn));

    if (open_file(fp, QIODevice::WriteOnly, TRUE) != -1) {
        QTextStream st(&fp);

        st.setCodec("UTF-8");

        st << "window_sizes " << width() << " " << height() << " "
           << spl1->sizes().first() << " " << spl1->sizes().last() << " "
           << spl2->sizes().first() << " " << spl2->sizes().last() << '\n';

        if(!style.isEmpty())
            st << style + "_style\n";

        QList<QMdiSubWindow *> windows = ws->subWindowList();

        if (! windows.isEmpty()) {
            st << "diagrams\n";

            for (int i = 0; i != windows.count(); i += 1) {
                if (ws->activeSubWindow() == windows.at(i))
                    st << "  active";
                ((DiagramWindow *) windows.at(i)->widget())->save_session(st);
            }

            st << "end\n";
        }

        browser->get_project()->save_session(st);

        if (verbose_generation())
            st << "verbose\n";

        st.flush();
        fp.close();

    }
}
void UmlWindow::read_session()
{
    if (user_id() <= 0)
        return;

    QDir d = BrowserView::get_dir();
    char fn[32];

    sprintf(fn, "%d.session", user_id());

    QFile fp(d.absoluteFilePath(fn));
    qint64 size;

    if ((size = open_file(fp, QIODevice::ReadOnly, TRUE)) != -1) {
        char * s = new char[size + 1];

        if (fp.read(s, size) != -1) {
            s[size] = 0;

            PRE_TRY;

            try {
                char * st = s;

                read_keyword(st, "window_sizes");

                int w = read_unsigned(st);
                int h = read_unsigned(st);

                resize(w, h);
                {
                    qApp->processEvents(/*500*/);
                }

                QList<int> lsz = spl1->sizes();

                lsz.first() = read_unsigned(st);
                lsz.last() = read_unsigned(st);

                spl1->setSizes(lsz);

                lsz = spl2->sizes();

                lsz.first() = read_unsigned(st);
                lsz.last() = read_unsigned(st);

                spl2->setSizes(lsz);
                {
                    qApp->processEvents(/*500*/);
                }

                const char * k = read_keyword(st);
                QString styleDetected(k);
                if(styleDetected.endsWith("_style"))
                {
                    setApplicationStyle(styleDetected.remove("_style"));
                    k = read_keyword(st);
                }

                // diagrams managed in BrowserPackage::read_session
                browser->get_project()->read_session(st, k);

                bool verb = FALSE;

                while (!at_end(st)) {
                    if (!strcmp(read_keyword(st), "verbose")) {
                        verb = TRUE;
                        break;
                    }

                    // keywork not managed by this (old) release
                }

                if (verb != verbose_generation())
                    toggle_verbose_generation();
            }
            catch (...) {
            }

            POST_TRY;
        }

        delete [] s;
    }
}

void UmlWindow::setup_generator_action_visibility()
{
    bool commandVisible = using_cpp() || using_java() || using_php() || using_python() || using_idl();
    if(commandVisible)
        generateLabelAction->setVisible(true);
    else
        generateLabelAction->setVisible(false);
    if(using_cpp())
        cppAction->setVisible(true);
    else
        cppAction->setVisible(false);

    if(using_java())
        javaAction->setVisible(true);
    else
        javaAction->setVisible(false);
    if(using_php())
        phpAction->setVisible(true);
    else
        phpAction->setVisible(false);

    if(using_python())
        pythonAction->setVisible(true);
    else
        pythonAction->setVisible(false);
    if(using_idl())
        idlAction->setVisible(true);
    else
        idlAction->setVisible(false);
}
void UmlWindow::print_it()
{
    the->print();
}

void UmlWindow::print()
{
    abort_line_construction();
#ifndef QT_NO_PRINTER
    DiagramWindow * dw = NULL;
    if(ws->activeSubWindow())
        dw = ((DiagramWindow *) ws->activeSubWindow()->widget());

    if (dw) {
        QPrinter printer;

        printer.setOrientation((format >= IsoA0Landscape)
                               ? QPrinter::Landscape
                               : QPrinter::Portrait);

        static bool initialized = FALSE;
        static QPrinter::ColorMode cm = QPrinter::Color;
        static QPrinter::PageSize ps = QPrinter::A4;
        static bool fp = FALSE;

        if (initialized) {
            printer.setColorMode(cm);
            printer.setPageSize(ps);
            printer.setFullPage(fp);
        }

        QPrintDialog dialog( &printer, this );
        if (/*printer.setup(this)*/dialog.exec() == QDialog::Accepted) {
            initialized = TRUE;
            cm = printer.colorMode();
            ps = printer.pageSize();
            fp = printer.fullPage();

            QMenu m(0);

            MenuFactory::createTitle(m, QObject::tr("Choose"));
            m.addSeparator();
            MenuFactory::addItem(m, tr("Print on 1 page"), 1);
            MenuFactory::addItem(m, tr("Print on 4 pages"), 2);
            MenuFactory::addItem(m, tr("Print on 9 pages"), 3);

            QAction* retAction = m.exec(QCursor::pos());
            if(retAction)
            {
                int div = retAction->data().toInt();

                if ((div >= 1) && (div <= 3)) {
                    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
                    dw->get_view()->print(printer, div);
                    QApplication::restoreOverrideCursor();
                }
            }
        }
    }
#endif
}

void UmlWindow::edit_gen_settings()
{
    browser->get_project()->edit_gen_settings();
}

void UmlWindow::edit_stereotypes()
{
    browser->get_project()->edit_stereotypes();
}

void UmlWindow::edit_class_settings()
{
    browser->get_project()->edit_class_settings();
}

void UmlWindow::edit_drawing_settings()
{
    browser->get_project()->edit_drawing_settings();
}

void UmlWindow::use_cpp()
{
    GenerationSettings::cpp_set_default_defs(!GenerationSettings::cpp_get_default_defs());
    bool cpp = GenerationSettings::cpp_get_default_defs();
    if(cpp)
        cppAction->setVisible(true);
    else
        cppAction->setVisible(false);
}

void UmlWindow::use_java()
{
    GenerationSettings::java_set_default_defs(!GenerationSettings::java_get_default_defs());
    bool java = GenerationSettings::java_get_default_defs();
    if(java)
        javaAction->setVisible(true);
    else
        javaAction->setVisible(false);
}

void UmlWindow::use_php()
{
    GenerationSettings::php_set_default_defs(!GenerationSettings::php_get_default_defs());
    bool php = GenerationSettings::php_get_default_defs();
    if(php)
        phpAction->setVisible(true);
    else
        phpAction->setVisible(false);

}

void UmlWindow::use_python()
{
    GenerationSettings::python_set_default_defs(!GenerationSettings::python_get_default_defs());
    bool python = GenerationSettings::python_get_default_defs();
    if(python)
        pythonAction->setVisible(true);
    else
        pythonAction->setVisible(false);

}

void UmlWindow::use_idl()
{
    GenerationSettings::idl_set_default_defs(!GenerationSettings::idl_get_default_defs());
    bool idl = GenerationSettings::idl_get_default_defs();
    if(idl)
        idlAction->setVisible(true);
    else
        idlAction->setVisible(false);

}
bool UmlWindow::using_cpp()
{
    return GenerationSettings::cpp_get_default_defs();
}

bool UmlWindow::using_java()
{
    return GenerationSettings::java_get_default_defs();
}

bool UmlWindow::using_php()
{
    return GenerationSettings::php_get_default_defs();
}

bool UmlWindow::using_python()
{
    return GenerationSettings::python_get_default_defs();
}

bool UmlWindow::using_idl()
{
    return GenerationSettings::idl_get_default_defs();
}

void UmlWindow::verbose()
{
    toggle_verbose_generation();
}

void UmlWindow::preserve()
{
    BrowserPackage * prj = browser->get_project();

    if (prj != 0) {
        if (!prj->is_writable())
            msg_critical("DoUML", QObject::tr("Unchanged : project is read-only"));
        else {
            toggle_preserve_bodies();

            if (! preserve_bodies())
                msg_warning("DoUML",
                            QObject::tr("Warning : <i>Preserve operations's body</i> set to false.<br><br>"
                                        "If you had modified body of operation outside DoUML without "
                                        "using <i>roundtrip body</i> after these modifications, you "
                                        "will loose them.<br>"
                                        "If needed, set <i>Preserve operations's body</i> to true, apply "
                                        "<i>roundtrip body</i>, then set <i>Preserve operations's body</i> to false"));
            else if (add_operation_profile())
                toggle_add_operation_profile();

            prj->modified();
        }
    }
}

void UmlWindow::addoperationprofile()
{
    BrowserPackage * prj = browser->get_project();

    if (prj != 0) {
        if (!prj->is_writable())
            msg_critical("DoUML", QObject::tr("Unchanged : project is read-only"));
        else {
            toggle_add_operation_profile();

            if (add_operation_profile() && preserve_bodies()) {
                toggle_preserve_bodies();
                msg_critical("DoUML",
                             QObject::tr("Warning : <i>Preserve operations's body</i> toggle is cleared !<br><br>"
                                         "Next code generations will replace operations's body<br><br>"
                                         "If you had modified body of operation outside DoUML without "
                                         "using <i>roundtrip body</i> after these modifications, you "
                                         "will loose them.<br>"
                                         "If needed, set <i>Preserve operations's body</i> to true, apply "
                                         "<i>roundtrip body</i>, then set <i>Preserve operations's body</i> to false"));
            }

            prj->modified();
        }
    }
}

void UmlWindow::edit_shortcuts()
{
    ShortcutDialog d;

    d.exec();
}

void UmlWindow::edit_env()
{
    EnvDialog::edit(FALSE);
    if (BrowserView::get_project() == 0)
        // id was set by reading doumlrc
        set_user_id(-1);
    else
        read_doumlrc();
}
void UmlWindow::edit_image_root_dir()
{
    QString s = QFileDialog::getExistingDirectory(
                this, tr("Select images root directory"), img_root_dir);

    if (! s.isEmpty()) {
        img_root_dir = s;
        the->browser->get_project()->package_modified();
    }
}

void UmlWindow::langMenuAboutToShow()
{
    abort_line_construction();

    use_cpp_id->setChecked(GenerationSettings::cpp_get_default_defs());
    use_java_id->setChecked(GenerationSettings::java_get_default_defs());
    use_php_id->setChecked(GenerationSettings::php_get_default_defs());
    use_python_id->setChecked(GenerationSettings::python_get_default_defs());
    use_idl_id->setChecked(GenerationSettings::idl_get_default_defs());
    verbose_gen_id->setChecked(verbose_generation());

    BrowserPackage * prj = browser->get_project();
    bool enabled = (prj != 0);

    use_cpp_id->setEnabled(enabled);
    use_java_id->setEnabled(enabled);
    use_php_id->setEnabled(enabled);
    use_python_id->setEnabled(enabled);
    use_idl_id->setEnabled(enabled);
    verbose_gen_id->setEnabled(enabled);

    if (enabled) {
        preserve_bodies_id->setChecked(preserve_bodies());
        add_operation_profile_id->setChecked( add_operation_profile());
    }

    preserve_bodies_id->setEnabled(enabled
                                   && prj->is_writable()
                                   );
    add_operation_profile_id->setEnabled(enabled
                                         && prj->is_writable()
                                         );
}

void UmlWindow::miscMenuAboutToShow()
{
    abort_line_construction();
    BrowserPackage * prj = browser->get_project();
    bool enabled = (prj != 0);
    shortcut_id->setEnabled(enabled);
    img_root_dir_id->setEnabled(enabled
                                && browser->get_project()->is_writable()
                                );
    show_browser_stereotypes_id->setEnabled(enabled);
}

void UmlWindow::fontSizeMenuAboutToShow()
{
    abort_line_construction();

    int ps = NormalFont.pointSize();
    BrowserNode * prj = browser->get_project();
    bool enabled = (prj != 0) && prj->is_writable();
    foreach(QAction* action,fontSizeMenu->actions()){
        action->setEnabled(enabled);
        action->setChecked(action->text().toInt() == ps);
        action->setWhatsThis(fontSizeMenuText());
    }
}

void UmlWindow::setFontSize()
{
    QAction *action = static_cast<QAction *>( sender());
    int i = action->text().toInt();
    if (i == NormalFont.pointSize())
        return;
    BrowserNode * prj = browser->get_project();

    if (prj != 0) {
        if (!prj->is_writable() &&
                (msg_warning("DoUML",
                             QObject::tr("Project file is read-only, new font "
                                         "size will not be saved, continue ?\n"),
                             QMessageBox::Yes, QMessageBox::No)
                 != QMessageBox::Yes))
            return;

        prj->package_modified();


        resize_font(i);

        // force browser redisplay

        bool is_open = prj->isExpanded();

        prj->setOpen(!is_open);
        prj->setOpen(is_open);

        // force diagrams redisplay
        QList<QMdiSubWindow *> windows = ws->subWindowList();

        for (i = 0; i < int(windows.count()); ++i) {
            DiagramView * view = ((DiagramWindow *) windows.at(i)->widget())->get_view();
            double z = view->the_canvas()->zoom();

            view->set_zoom(z + 0.1);
            view->set_zoom(z);
        }

        DrawingSettings::modified();
    }
}

void UmlWindow::formatMenuAboutToShow()
{
    abort_line_construction();

    //int i = IsoA0;

    foreach(QAction* action,formatMenu->actions())
    {
        //for (i = IsoA0; i != IsoA0Landscape; i += 1) {
        action->setChecked( action->data().toInt() == (int) format);
        action->setWhatsThis(formatMenuText());
        //}
        //i++;
    }
    foreach(QAction* action,formatLandscapeMenu->actions())
    {
        //for (; i != CanvasFormatSup; i += 1) {
        action->setChecked(action->data().toInt()== (int) format);
        action->setWhatsThis(formatMenuText());
        //}
    }
}

void UmlWindow::setFormat()
{
    QAction* action = static_cast<QAction*>(sender());
    int i = action->data().toInt();
    BrowserNode * prj = browser->get_project();

    if (prj != 0) {
        if (!prj->is_writable() &&
                (msg_warning("DoUML",
                             QObject::tr("Project file is read-only, default "
                                         "format will not be saved, continue ?\n"),
                             QMessageBox::Yes, QMessageBox::No)
                 != QMessageBox::Yes))
            return;

        format = (CanvasFormat) i;

        prj->package_modified();
    }

}

void UmlWindow::set_default_format(CanvasFormat f)
{
    the->format = f;
}

void UmlWindow::browser_search()
{
    abort_line_construction();
    if (browser->get_project() != 0) {
        if (BrowserSearchDialog::get() == 0)
            (new BrowserSearchDialog())->show();
        else
        {
            BrowserSearchDialog::get()->show();
            BrowserSearchDialog::get()->raise();
        }
    }
}

void UmlWindow::browser_search_it()
{
    the->browser_search();
}

void UmlWindow::show_stereotypes()
{
    //miscMenu->setItemChecked(show_browser_stereotypes_id,
    //                       BrowserNode::toggle_show_stereotypes());
    show_browser_stereotypes_id->setChecked(BrowserNode::toggle_show_stereotypes());
}
void UmlWindow::do_completion()
{
    bool cmpltn = !completion();

    //miscMenu->setItemChecked(completion_id, cmpltn);
    completion_id->setChecked(cmpltn);
    set_completion(cmpltn);
}

void UmlWindow::setApplicationStyleSlot()
{
    QAction *action = static_cast<QAction*>(sender());
    setApplicationStyle(action->text());
}

void UmlWindow::setApplicationStyle(QString s)
{
    QApplication::setStyle(s);
    style = s;
}


void UmlWindow::show_trace()
{
    TraceDialog::show_it();
}

void UmlWindow::cpp_generate()
{
    BrowserPackage * prj = browser->get_project();
    bool preserve = preserve_bodies();

    if (prj != 0)
        ToolCom::run((verbose_generation())
                     ? ((preserve) ? "cpp_generator -v -p" : "cpp_generator -v")
                     : ((preserve) ? "cpp_generator -p" : "cpp_generator"),
                     prj);
}

void UmlWindow::java_generate()
{
    BrowserPackage * prj = browser->get_project();
    bool preserve = preserve_bodies();

    if (prj != 0)
        ToolCom::run((verbose_generation())
                     ? ((preserve) ? "java_generator -v -p" : "java_generator -v")
                     : ((preserve) ? "java_generator -p" : "java_generator"),
                     prj);
}

void UmlWindow::php_generate()
{
    BrowserPackage * prj = browser->get_project();
    bool preserve = preserve_bodies();

    if (prj != 0)
        ToolCom::run((verbose_generation())
                     ? ((preserve) ? "php_generator -v -p" : "php_generator -v")
                     : ((preserve) ? "php_generator -p" : "php_generator"),
                     prj);
}

void UmlWindow::python_generate()
{
    BrowserPackage * prj = browser->get_project();
    bool preserve = preserve_bodies();

    if (prj != 0)
        ToolCom::run((verbose_generation())
                     ? ((preserve) ? "python_generator -v -p" : "python_generator -v")
                     : ((preserve) ? "python_generator -p" : "python_generator"),
                     prj);
}

void UmlWindow::idl_generate()
{
    BrowserPackage * prj = browser->get_project();

    if (prj != 0)
        ToolCom::run((verbose_generation()) ? "idl_generator -v" : "idl_generator",
                     prj);
}

void UmlWindow::java_catalog()
{
    BrowserPackage * prj = browser->get_project();

    if (prj != 0)
        ToolCom::run("java_catalog", prj);
}

#if 0
void UmlWindow::doc_generate()
{
    BrowserPackage * prj = browser->get_project();

    if (prj != 0)
        ToolCom::run("doc_generator", prj);
}
#endif

void UmlWindow::cpp_reverse()
{
    BrowserPackage * prj = browser->get_project();

    if (prj != 0)
        ToolCom::run("cpp_reverse", prj);
}

void UmlWindow::java_reverse()
{
    BrowserPackage * prj = browser->get_project();

    if (prj != 0)
        ToolCom::run("java_reverse", prj);
}

void UmlWindow::cpp_roundtrip()
{
    BrowserPackage * prj = browser->get_project();

    if (prj != 0)
        ToolCom::run("cpp_roundtrip", prj);
}

void UmlWindow::java_roundtrip()
{
    BrowserPackage * prj = browser->get_project();

    if (prj != 0)
        ToolCom::run("java_roundtrip", prj);
}

void UmlWindow::php_reverse()
{
    BrowserPackage * prj = browser->get_project();

    if (prj != 0)
        ToolCom::run("php_reverse", prj);
}

void UmlWindow::python_reverse()
{
    BrowserPackage * prj = browser->get_project();

    if (prj != 0)
        ToolCom::run("python_reverse", prj);
}

void UmlWindow::cpp_roundtripbody()
{
    BrowserPackage * prj = browser->get_project();

    if (prj != 0)
        ToolCom::run((verbose_generation()) ? "roundtrip_body -v c++" : "roundtrip_body c++",
                     prj);
}

void UmlWindow::java_roundtripbody()
{
    BrowserPackage * prj = browser->get_project();

    if (prj != 0)
        ToolCom::run((verbose_generation()) ? "roundtrip_body -v java" : "roundtrip_body java",
                     prj);
}

void UmlWindow::php_roundtripbody()
{
    BrowserPackage * prj = browser->get_project();

    if (prj != 0)
        ToolCom::run((verbose_generation()) ? "roundtrip_body -v php" : "roundtrip_body php",
                     prj);
}

void UmlWindow::python_roundtripbody()
{
    BrowserPackage * prj = browser->get_project();

    if (prj != 0)
        ToolCom::run((verbose_generation()) ? "roundtrip_body -v python" : "roundtrip_body python",
                     prj);
}

void UmlWindow::plug_out_upgrade()
{
    BrowserPackage * prj = browser->get_project();

    if (prj != 0)
        ToolCom::run("plug_out_upgrade", prj);
}

void UmlWindow::run_tool()
{
    QAction *action = static_cast<QAction*>( sender());
    int param = action->data().toInt();
    BrowserPackage * prj = browser->get_project();

    if (prj != 0)
        ToolCom::run(Tool::command(param), prj);
}

void UmlWindow::tool_settings()
{
    ToolDialog dlg;

    if (dlg.exec() == QDialog::Accepted) {
        browser->get_project()->modified();
    }
}

void UmlWindow::import_tool_settings()
{
    if (Tool::import())
        browser->get_project()->modified();
}

void UmlWindow::help()
{
    if (BrowserView::get_project() == 0)
        read_doumlrc();

    const char * topic =
            (browser->get_project() == 0)
            ? "starting"
            : ((BrowserView::selected_item() == 0)
               ? "browseritems"
               : BrowserView::selected_item()->help_topic());

    HelpDialog::show(topic);

    if (BrowserView::get_project() == 0)
        // id was set by reading doumlrc
        set_user_id(-1);
}

void UmlWindow::about()
{
    AboutDialog d;

    d.exec();
}

void UmlWindow::aboutQt()
{
    QMessageBox::aboutQt(this, "DoUML");
}

void UmlWindow::windowsMenuAboutToShow()
{
    abort_line_construction();

    windowsMenu->clear();

    QAction* cascadeId = windowsMenu->addAction(tr("&Cascade"), ws, SLOT(cascadeSubWindows()));
    QAction*  tileId = windowsMenu->addAction(tr("&Tile"), ws, SLOT(tileSubWindows()));
    QAction*  preferredId = windowsMenu->addAction(tr("&Preferred geometry"), this, SLOT(preferred_geometry()));
    QAction*  closeAllId = windowsMenu->addAction(tr("C&lose all"), this, SLOT(close_all_windows()));

    QList<QMdiSubWindow *>  windows = ws->subWindowList();

    if (windows.isEmpty()) {
        /*
        windowsMenu->setItemEnabled(cascadeId, FALSE);
        windowsMenu->setItemEnabled(tileId, FALSE);
        windowsMenu->setItemEnabled(preferredId, FALSE);
        windowsMenu->setItemEnabled(closeAllId, FALSE);
        */
        cascadeId->setEnabled(false);
        tileId->setEnabled(false);
        preferredId->setEnabled(false);
        closeAllId->setEnabled(false);
    }
    else {
        windowsMenu->addSeparator();

        for (int i = 0; i < int(windows.count()); ++i) {
            QAction* id = windowsMenu->addAction(windows.at(i)->windowTitle(),
                                                 this, SLOT(windowsMenuActivated()));
            //windowsMenu->setItemParameter(id, i);
            id->setData(i);
            id->setChecked(ws->activeSubWindow() == windows.at(i));
            //windowsMenu->setItemChecked(id, ws->activeWindow() == windows.at(i));
        }
    }

    QWidgetList widgets = dialogs();

    if (! widgets.isEmpty()) {
        windowsMenu->addSeparator();

        for (int j = 0; j < int(widgets.count()); ++j) {
            QAction* id = windowsMenu->addAction(widgets.at(j)->windowTitle(),
                                                 this, SLOT(dialogsMenuActivated()));
            id->setData(j);
            // windowsMenu->setItemParameter(id, j);
        }
    }
}

void UmlWindow::preferred_geometry()
{
    QList<QMdiSubWindow *> l = the->ws->subWindowList();
    QWidget * w;

    //for (w = l.first(); w != 0; w = l.next())

    for (QList<QMdiSubWindow *>::iterator it = l.begin(); it != l.end(); it++) {
        w = (*it)->widget(); // qlist no longer is a iterator itself
        ((DiagramWindow *) w)->get_view()->preferred_size_zoom();
    }
}

void UmlWindow::close_all_windows()
{
    QList<QMdiSubWindow *> l = the->ws->subWindowList();
    QWidget * w;

    for (QList<QMdiSubWindow *>::iterator it = l.begin(); it != l.end(); it++) {
        w = (*it); // qlist no longer is a iterator itself
        /*((DiagramWindow *))*/ w->close();
    }

    QList<QWidget *> listDialogs = dialogs();

    for (QList<QWidget *>::iterator it = listDialogs.begin(); it != listDialogs.end(); it++) {
        w = (*it); // qlist no longer is a iterator itself
        w->close();
    }
}

void UmlWindow::windowsMenuActivated()
{
    QAction* act = static_cast<QAction*>(sender());
    if(act)
    {
        int id = act->data().toInt();
        QMdiSubWindow * w = ws->subWindowList().at(id);

        if (w) {
            w->showNormal();
            w->setFocus();
        }
    }
}

void UmlWindow::dialogsMenuActivated()
{

    QAction* act = static_cast<QAction*>(sender());
    if(act)
    {
        int id = act->data().toInt();
        QWidget * w = dialogs().at(id);

        if (w) {
            w->showNormal();
            w->raise();
        }
    }

}

void UmlWindow::abort_line_construction()
{
    QList<QMdiSubWindow *> l = the->ws->subWindowList();

    QWidget * w;

    /*for (QWidgetList::iterator it = l.begin(); it != l.end(); it++) {
        w = *it; // qlist no longer is a iterator itself
        ((DiagramWindow *) w)->get_view()->abort_line_construction();
    }*/
    for (QList<QMdiSubWindow *>::iterator it = l.begin(); it != l.end(); it++) {
        w = (*it)->widget(); // qlist no longer is a iterator itself
        if(w)
            ((DiagramWindow *) w)->get_view()->abort_line_construction();
    }

}

//

void UmlWindow::keyPressEvent(QKeyEvent * e)
{
    QString s = Shortcut::shortcut(e->key(), e->modifiers());

    if (!s.isEmpty()) {
        e->accept();

        if (s == "Save")
            UmlWindow::save_it();
        else if (s == "Save as")
            UmlWindow::saveas_it();
        else if (s == "Browser search")
            UmlWindow::browser_search_it();
        else if (s == "Close")
            UmlWindow::do_close();
        else if (s == "Quit")
            UmlWindow::do_quit();
        else if (s == "Open project")
            UmlWindow::load_it();
    }
    else
        QMainWindow::keyPressEvent(e);
}

void UmlWindow::OnPickSelectionFromItem(const QModelIndex & current, const QModelIndex &)
{
    TreeItemInterface *itemAsInterface = static_cast<TreeItemInterface*>(current.internalPointer());
    BrowserNode* itemAsNode = static_cast<BrowserNode*>(itemAsInterface->InternalPointer());
    itemAsNode->select_in_browser();
}

void UmlWindow::OnChooseQuickEditMode(QTreeWidgetItem* item, QTreeWidgetItem* old)
{
    Q_UNUSED(old);

    if (!item)
        return;

    BrowserNode* itemAsNode = static_cast<BrowserNode*>(item);
    if(!itemAsNode)
        return;
    if(quickEdit->ValidType(itemAsNode))
        tbQuickEdit->setEnabled(true);
    else
        tbQuickEdit->setEnabled(false);

}

void UmlWindow::OnShowQuickEdit()
{
    if(quickEdit->isVisible())
        quickEdit->close();
    else
        quickEdit->OnShow();
}

void UmlWindow::OnCallClipboardMenu()
{
    An<ClipboardManager> clipboard;
    clipboard->GetMenuInstance()->popup(QCursor::pos());
}

void UmlWindow::whats_this() const
{
    QWhatsThis::enterWhatsThisMode();
}

//

static QList<BrowserNode *>::Iterator HistoricIterator;

static bool OnHistoric = FALSE;
void UmlWindow::clear_select_historic()
{
    the->select_historic.clear();
    HistoricIterator = the->select_historic.end();

    the->prev->setIcon(QIcon(*leftUnavailablePixmap));
    the->next->setIcon(QIcon(*rightUnavailablePixmap));
}

void UmlWindow::is_selected(BrowserNode * bn)
{
    if (OnHistoric)
        OnHistoric = FALSE;
    else if ((HistoricIterator == select_historic.end()) ||
             (*HistoricIterator != bn)) {
        if ((HistoricIterator != select_historic.end()))
            ++HistoricIterator;

        while (HistoricIterator != select_historic.end())
        {
            HistoricIterator = select_historic.erase(HistoricIterator);
        }

        if (select_historic.count() == 50)
            select_historic.removeOne(select_historic.first());

        select_historic.append(bn);
        HistoricIterator = select_historic.end();
        --HistoricIterator;

        prev->setIcon((HistoricIterator != select_historic.begin())
                      ? *leftPixmap : *leftUnavailablePixmap);
        next->setIcon(*rightUnavailablePixmap);
    }
}

void UmlWindow::next_select()
{
    if(select_historic.count())
    {
        if (++HistoricIterator != select_historic.end()) {
            OnHistoric = TRUE;
            browser->select(*HistoricIterator);

            the->prev->setIcon(*leftPixmap);
            ++HistoricIterator;
            the->next->setIcon((HistoricIterator != select_historic.end())
                               ? *rightPixmap : *rightUnavailablePixmap);
        }

        --HistoricIterator;
    }
}

void UmlWindow::prev_select()
{
    if(select_historic.count())
    {
        if (HistoricIterator != select_historic.begin()) {
            OnHistoric = TRUE;
            browser->select(*--HistoricIterator);

            the->prev->setIcon((HistoricIterator != select_historic.begin())
                               ? *leftPixmap : *leftUnavailablePixmap);
            the->next->setIcon(*rightPixmap);
        }
    }
}
void UmlWindow::historic_forget(BrowserNode * bn)
{
    the->select_historic.removeOne(bn);
}

