
#ifdef WIN32
#include <stdlib.h>
#endif

#include "Dialog.h"

#include <qlayout.h>
#include <qlineedit.h>
#include <qlabel.h>
#include <q3hbox.h>
#include <qcheckbox.h>
#include <qpushbutton.h>
#include <qmessagebox.h>
#include <qdir.h>
//Added by qt3to4:
#include <Q3CString>
#include <Q3VBoxLayout>
#include <QSettings>
#include <QTextCodec>

Dialog::Dialog(bool ci, Q3CString & cmd, BooL & rec, BooL & reload)
    : QDialog(0, 0, TRUE), _cmd(cmd), _rec(rec), _reload(reload)
{
    Q3VBoxLayout * vbox = new Q3VBoxLayout(this);
    Q3HBox * htab;

    vbox->setMargin(5);

    // add recursive check box

    htab = new Q3HBox(this);
    htab->setMargin(5);
    vbox->addWidget(htab);

    rec_cb = new QCheckBox((ci) ? "Check-in sub packages" : "Check-out sub-packages",
                           htab);

    // add comment

    htab = new Q3HBox(this);
    htab->setMargin(5);
    vbox->addWidget(htab);
    new QLabel("In the command %file will be replaced by the file's name,"
               " %dir by the directory where is the file\n"
               "%dironly is replaced by the directory without the drive", htab);

    // add command edit

    htab = new Q3HBox(this);
    htab->setMargin(5);
    vbox->addWidget(htab);

    new QLabel("Command : ", htab);
    cmd_ed = new QLineEdit(htab);
    cmd_ed->setText(_cmd);

    // save & load

    htab = new Q3HBox(this);
    htab->setMargin(5);
    vbox->addWidget(htab);

    reload_cb = new QCheckBox("Save then reload the project to see the new write permission of the files",
                              htab);
    reload_cb->setChecked(TRUE);

    // ok & cancel
    htab = new Q3HBox(this);
    htab->setMargin(5);
    vbox->addWidget(htab);

    new QLabel(htab);
    QPushButton * ok = new QPushButton("&Ok", htab);
    new QLabel(htab);
    QPushButton * cancel = new QPushButton("&Cancel", htab);
    new QLabel(htab);
    QSize bs(cancel->sizeHint());

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
    if (cmd_ed->text().find("%file") == -1)
        QMessageBox::critical(this, "Error", "the command must contain %file");
    else {
        _cmd = cmd_ed->text();
        _rec = rec_cb->isChecked();
        _reload = reload_cb->isChecked();
        QDialog::accept();
    }
}

