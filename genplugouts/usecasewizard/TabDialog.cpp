
#ifdef WIN32
#include <stdlib.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <q3vbox.h>
#include <qlabel.h>
#include <Q3TextEdit>
#include <q3groupbox.h>
#include <qtextcodec.h>
#include <qdir.h>
#include <QSettings>
//Added by qt3to4:
#include <Q3CString>

#include "UmlCom.h"

#include "TabDialog.h"
#include "UmlUseCase.h"

const struct {
    const char * lbl;
    const char * key;
    Q3TextEdit * (TabDialog::* a);
} Tabs[] = {
    {
        "The summary of the use case '",
        "Summary", &TabDialog::summary
    },
    {
        "The operational context of the use case '",
        "Context", &TabDialog::context
    },
    {
        "The pre-conditions of the use case '",
        "Pre-Conditions", &TabDialog::precond
    },
    {
        "The detailed description of the use case '",
        "Description", &TabDialog::description
    },
    {
        "The post-conditions of the use case '",
        "Post-Conditions", &TabDialog::postcond
    },
    {
        "The exceptions of the use case '",
        "Exceptions", &TabDialog::exceptions
    }
};

TabDialog::TabDialog(UmlUseCase * u) : Q3TabDialog(0, ""), uc(u)
{
    setCaption(Q3CString("Properties of the use case '") + u->name() + "'");

    setCancelButton();

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
        desktopCenter.setX((r + l) / 2);
        desktopCenter.setY((t + b) / 2);
      }
    }

    QString cs = settings.value("Main/encoding", "UTF-8").toString();

    Codec = 0;

    if (!cs.isEmpty() && ((Codec = QTextCodec::codecForName(cs)) == 0)) {
        Q3VBox * vbox = new Q3VBox(this);

        vbox->setMargin(5);
        (new QLabel("ERROR : No codec for '" + cs + "'", vbox))
        ->setAlignment(Qt::AlignCenter);

        addTab(vbox, "Use case wizard");
        setOkButton(QString::null);
    }
    else {
        for (unsigned i = 0; i != sizeof(Tabs) / sizeof(*Tabs); i += 1) {
            Q3VBox * vbox = new Q3VBox(this);

            vbox->setMargin(5);
            (new QLabel(Q3CString(Tabs[i].lbl) + u->name() + "'",
                        new Q3GroupBox(1, Qt::Horizontal, vbox)))
            ->setAlignment(Qt::AlignCenter);
            this->*(Tabs[i]).a = new Q3TextEdit(vbox);

            Q3CString v;

            if (u->propertyValue(Tabs[i].key, v))
                (this->*(Tabs[i]).a)->setText(toUnicode(v));

            addTab(vbox, Tabs[i].key);
        }
    }
}

void TabDialog::polish()
{
    Q3TabDialog::polish();

    if (! desktopCenter.isNull())
        move(x() + desktopCenter.x() - (x() + width() / 2),
             y() + desktopCenter.y() - (y() + height() / 2));
}

void TabDialog::accept()
{
    for (unsigned i = 0; i != sizeof(Tabs) / sizeof(*Tabs); i += 1)
        uc->set_PropertyValue(Tabs[i].key, fromUnicode((this->*(Tabs[i]).a)->text()));

    UmlCom::bye();
    Q3TabDialog::accept();
}

void TabDialog::reject()
{
    UmlCom::bye();
    Q3TabDialog::reject();
}

QString TabDialog::toUnicode(const char * str)
{
    if (Codec == 0)
        return str;
    else if ((str == 0) || (*str == 0))
        return QString::null;
    else
        return Codec->toUnicode(str);
}

void TabDialog::latinize(QString & s)
{
    unsigned i = s.length();

    while (i != 0) {
        QChar c = s.at(--i);

        if (c.latin1() == 0) {
            if (c.unicode() == 8217)
                // special case for the pseudo ' returned by microsoft editors
                c = '\'';
            else
                c = ' ';

            s.replace(i, 1, &c, 1);
        }
    }
}

Q3CString TabDialog::fromUnicode(const QString & s)
{
    if (Codec == 0) {
        QString str = s;

        latinize(str);
        return Q3CString((const char *)str);
    }
    else if (s.isEmpty())
        return "";
    else
        return Codec->fromUnicode(s);
}

