
#ifdef WIN32
#include <stdlib.h>
#endif

#include "Dialog.h"
#include "UmlArtifact.h"
#include "SmallPushButton.h"

#include <qlineedit.h>
#include <Q3TextEdit.h>
#include <qpushbutton.h>
#include <q3combobox.h>
#include <qlayout.h>
#include <q3filedialog.h>
#include <qfileinfo.h>
#include <qlabel.h>
#include <q3grid.h>
#include <q3hbox.h>
#include <qdir.h>
//Added by qt3to4:
#include <Q3CString>
#include <Q3VBoxLayout>
#include <QSettings>

Dialog::Dialog(UmlArtifact * art, const Q3CString & path_exe, Q3CString & pro, Q3CString & target, Q3CString & tmplt, Q3CString & config, Q3CString & defines, Q3CString & includepath, Q3CString & dependpath, Q3CString & objectsdir, Q3CString & footer)
    : QDialog(0, 0, TRUE), _art(art), _pro(pro), _target(target), _tmplt(tmplt),
      _config(config), _defines(defines), _includepath(includepath), _dependpath(dependpath),
      _objectsdir(objectsdir), _footer(footer)
{
    QDir d(path_exe);
    Q3VBoxLayout * vbox = new Q3VBoxLayout(this);
    Q3Grid * grid = new Q3Grid(2, this);
    Q3HBox * htab;
    int index;

    vbox->addWidget(grid);
    vbox->setMargin(5);

    new QLabel(".pro file : ", grid);
    htab = new Q3HBox(grid);
    edpro = new QLineEdit(htab);
    edpro->setText(d.absFilePath(pro));

    new QLabel(" ", htab);
    browsepro = new SmallPushButton("browse", htab);
    connect(browsepro, SIGNAL(clicked()), this, SLOT(browse_pro()));

    new QLabel("target : ", grid);
    htab = new Q3HBox(grid);
    edtarget = new QLineEdit(htab);
    edtarget->setText(d.absFilePath(target));
    new QLabel(" ", htab);
    browsetarget = new SmallPushButton("browse", htab);
    connect(browsetarget, SIGNAL(clicked()), this, SLOT(browse_target()));

    new QLabel("template : ", grid);
    cbtemplate = new Q3ComboBox(TRUE, grid);

    static const char * templates[] = { "app", "lib", "subdirs" };
    bool find = FALSE;

    for (index = 0; index != sizeof(templates) / sizeof(*templates); index += 1) {
        cbtemplate->insertItem(templates[index]);

        if (tmplt == templates[index]) {
            cbtemplate->setCurrentItem(index);
            find = TRUE;
        }
    }

    if (! find) {
        cbtemplate->insertItem((QString) tmplt);
        cbtemplate->setCurrentItem(index);
    }

    new QLabel("config : ", grid);
    htab = new Q3HBox(grid);
    cbconf[0] = new Q3ComboBox(FALSE, htab);
    cbconf[0]->insertItem("debug");
    cbconf[0]->insertItem("release");

    QStringList lcnf =
        QStringList::split(" ", (const char *) config);
    QStringList::Iterator it = lcnf.begin();

    cbconf[0]->setCurrentItem((*it++ == "debug") ? 0 : 1);

    cbconf[1] = new Q3ComboBox(FALSE, htab);
    cbconf[1]->insertItem("warn_on");
    cbconf[1]->insertItem("warn_off");
    cbconf[1]->setCurrentItem((*it++ == "warn_on") ? 0 : 1);

    QSizePolicy sp = cbconf[0]->sizePolicy();

    sp.setHorData(QSizePolicy::Fixed);
    cbconf[0]->setSizePolicy(sp);
    cbconf[1]->setSizePolicy(sp);

    new QLabel(" qt ", htab);
    it++;	// qt

    const char * configs[] = {
        "", "opengl", "thread", "x11", "windows",
        "console", "dll", "staticlib", 0
    };

    for (index = 2;
         index != sizeof(cbconf) / sizeof(*cbconf) - 1;
         index += 1) {
        cbconf[index] = new Q3ComboBox(TRUE, htab);

        if (it != lcnf.end())
            cbconf[index]->insertItem(*it++);

        cbconf[index]->insertStrList(configs);
        cbconf[index]->setCurrentItem(0);
    }

    cbconf[index] = new Q3ComboBox(TRUE, htab);

    if (it != lcnf.end()) {
        QString s = *it++;

        while (it != lcnf.end())
            s += " " + *it++;

        cbconf[index]->insertItem(s);
    }

    cbconf[index]->insertStrList(configs);
    cbconf[index]->setCurrentItem(0);


    new QLabel("defines : ", grid);
    eddefines = new QLineEdit(grid);
    eddefines->setText(defines);

    ///may be computed
    new QLabel("include paths : ", grid);
    htab = new Q3HBox(grid);
    edincludepath = new QLineEdit(htab);
    edincludepath->setText(includepath);
    new QLabel(" ", htab);
    computeincludepath = new SmallPushButton("compute", htab);
    connect(computeincludepath, SIGNAL(clicked()), this, SLOT(compute_includepath()));

    new QLabel("depend paths : ", grid);
    eddependpath = new QLineEdit(grid);
    eddependpath->setText(dependpath);

    new QLabel("objects dir : ", grid);
    htab = new Q3HBox(grid);
    edobjectsdir = new QLineEdit(htab);
    edobjectsdir->setText(objectsdir);
    new QLabel(" ", htab);
    browseobjectsdir = new SmallPushButton("browse", htab);
    connect(browseobjectsdir, SIGNAL(clicked()), this, SLOT(browse_objectsdir()));

    new QLabel("footer : ", grid);
    edfooter = new Q3TextEdit(grid);
    edfooter->setText(footer);

    new QLabel(grid);
    new QLabel(grid);

    new QLabel(grid);
    htab = new Q3HBox(grid);
    new QLabel(htab);
    QPushButton * ok = new QPushButton("&OK", htab);
    new QLabel(htab);
    QPushButton * cancel = new QPushButton("&Cancel", htab);
    new QLabel(htab);
    QSize bs(cancel->sizeHint());

    ok->setDefault(TRUE);

    if (ok->sizeHint().width() > bs.width())
        bs.setWidth(ok->sizeHint().width());

    ok->setFixedSize(bs);
    cancel->setFixedSize(bs);

    connect(ok, SIGNAL(clicked()), this, SLOT(accept()));
    connect(cancel, SIGNAL(clicked()), this, SLOT(reject()));
}

void Dialog::polish()
{
    QDialog::polish();

    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "DoUML", "settings");
    settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
    int l, t, r, b;
    l = settings.value("Desktop/left", -1).toInt();
    r = settings.value("Desktop/right", -1).toInt();
    t = settings.value("Desktop/top", -1).toInt();
    b = settings.value("Desktop/bottom", -1).toInt();

    if(l != -1 && r != -1 && t != -1 && b != -1)
    {
      if (!((r == 0) && (t == 0) && (r == 0) && (b == 0)) &&
          !((r < 0) || (t < 0) || (r < 0) || (b < 0)) &&
          !((r <= l) || (b <= t)))
      {
        int cx = (r + l) / 2;
        int cy = (t + b) / 2;

        move(x() + cx - (x() + width() / 2), y() + cy - (y() + height() / 2));
      }
    }
}

void Dialog::accept()
{
    _pro = edpro->text();
    _target = edtarget->text();
    _objectsdir = edobjectsdir->text();
    _tmplt = cbtemplate->currentText();

    int index;

    _config = cbconf[0]->currentText() + " " + cbconf[1]->currentText() + " qt";

    for (index = 2; index != sizeof(cbconf) / sizeof(*cbconf); index += 1) {
        QString s = cbconf[index]->currentText();

        if (! s.isEmpty())
            _config += " " + s;
    }

    _defines = eddefines->text();
    _includepath = edincludepath->text();
    _dependpath = eddependpath->text();
    _objectsdir = edobjectsdir->text();
    _footer = edfooter->text();

    QDialog::accept();
}

void Dialog::browse_pro()
{
    QString f = Q3FileDialog::getSaveFileName(edpro->text(), "Pro file (*.pro)", this,
                0, "specify .pro file");

    if (! f.isEmpty())
        edpro->setText(f);
}

void Dialog::browse_target()
{
    QString f = Q3FileDialog::getSaveFileName(edtarget->text(), "", this,
                0, "specify target file");

    if (! f.isEmpty())
        edtarget->setText(f);
}

void Dialog::compute_includepath()
{
    const Q3PtrVector<UmlArtifact> & arts = _art->associatedArtifacts();
    QFileInfo fi(edpro->text());
    QString prodir = fi.dirPath(TRUE);
    unsigned index;
    QStringList l;

    for (index = 0; index != arts.count(); index += 1) {
        QString s = arts[index]->way(prodir, TRUE);

        if ((s != "./") && (l.findIndex(s) == -1))
            l.append(s);
    }

    edincludepath->setText(l.join(" "));

}

void Dialog::browse_objectsdir()
{
    QString d = Q3FileDialog::getExistingDirectory(edobjectsdir->text(), this, 0,
                "select objects dir");

    if (! d.isEmpty()) {
#ifdef WIN32
        QDir dir(d);

        if (! dir.exists()) {
            int index = d.length() - 1;

            if (d.at(index) == QChar('/'))
                index -= 1;

            QString d2 = d;

            d2.remove(index, 1);

            QDir dir2(d2);

            if (dir2.exists())
                d = d2;
        }

#endif

        edobjectsdir->setText(d);
    }
}

