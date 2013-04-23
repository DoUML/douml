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





#include <Q3TextEdit>
#include <qlabel.h>
#include <qlayout.h>
#include <qfile.h>
#include <QTextStream>
#include <qapplication.h>
#include <qdir.h>
//Added by qt3to4:
#include <Q3VBoxLayout>
#include "misc/mystr.h"
#include <QDesktopWidget>
#include <QSettings>
#include "ShowFileDialog.h"

ShowFileDialog::ShowFileDialog(const WrapperStr & filename)
    : QDialog(0, filename)
{
    Q3VBoxLayout * vbox = new Q3VBoxLayout(this);

    vbox->addWidget(new QLabel("You can specify the editor through the environment dialog",
                               this));

    e = new Q3TextEdit(this);

    QFile f(filename);

    if (f.open(QIODevice::ReadOnly)) {
        QTextStream t(&f);

        e->setText(t.read());
    }

    QFont font = e->font();

    font.setFamily("Courier");
    font.setFixedPitch(TRUE);
    e->setFont(font);

    vbox->addWidget(e);
}

void ShowFileDialog::polish()
{
    QDialog::polish();

    int w = QApplication::desktop()->width();
    int h = QApplication::desktop()->height();
    int cx = w / 2;
    int cy = h / 2;
    bool virtual_desktop = FALSE;

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
        w = r - l + 1;
        h = b - t + 1;
        cx = (r + l) / 2;
        cy = (t + b) / 2;
        virtual_desktop = TRUE;
      }
    }

    resize(w / 2, h / 2);

    if (virtual_desktop)
        move(x() + cx - (x() + width() / 2),
             y() + cy - (y() + height() / 2));
}

ShowFileDialog::~ShowFileDialog()
{
}
