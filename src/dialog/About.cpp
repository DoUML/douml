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

#include <qlayout.h>
#include <qlabel.h>
#include <q3textview.h>
#include <qpushbutton.h>
//Added by qt3to4:
#include <Q3VBoxLayout>
#include <QPixmap>
#include <Q3HBoxLayout>
#include <QPainter>
#include <QTabWidget>
#include "About.h"
#include "UmlDesktop.h"
#include "translate.h"
#include "version.h"

AboutDialog::AboutDialog() : QDialog(0, "About DoUML", TRUE)
{
    setCaption(TR("About DoUML"));

    Q3VBoxLayout * vbox = new Q3VBoxLayout(this);
    Q3HBoxLayout * hbox;

    vbox->setMargin(5);

    hbox = new Q3HBoxLayout(vbox);
    hbox->setMargin(5);

    QPixmap pix(QPixmap(QString(":/douml.png")));
    QColor backgroundColor = palette().light().color();
    backgroundColor.setAlpha(200);
    QPainter p(&pix);
    p.setFont(QFont("Arial", 14, QFont::Black));
    p.setPen( Qt::black );
    p.drawText( p.window(), Qt::AlignCenter, QString(DOUML_VERSION));
    p.end();

    QLabel * lbp = new QLabel(this);
    QPalette palette;
    QBrush brush(Qt::lightGray);
    palette.setBrush(QPalette::Active, QPalette::Window, brush);
    lbp->setPalette(palette);
    lbp->setAutoFillBackground(true);

    lbp->setPixmap(pix);
    hbox->addWidget(lbp);

    hbox = new Q3HBoxLayout(vbox);
    hbox->setMargin(5);
    lbp = new QLabel(this);
    lbp->setText(QString("Build: " DOUML_BUILD_DATE " - Qt version " QT_VERSION_STR));
    hbox->addWidget(lbp);

    hbox = new Q3HBoxLayout(vbox);
    QTabWidget *tabs = new QTabWidget(this);

    // Description

    QWidget *page_description = new QWidget;

    hbox = new Q3HBoxLayout;
    hbox->setMargin(5);

    const char htmltext_description[] = "<p>DoUML</p>\n"
                            "<p>This project is a fork of\n"
                            "Bruno Pages's work, BoUML:\n"
                            "<i>http://sourceforge.net/projects/bouml/</i></p>\n"
                            "<p>DoUML focus is to port BoUML to Qt4\n"
                            "and to maintain it as a community</p>\n"
                            "<p>Join us at:<br>\n"
                            "https://github.com/DoUML/douml<br>\n"
                            "#bdouml@irc.freenode.net</p>\n\n\n";

    Q3TextView * tx_description = new Q3TextView(htmltext_description, QString(), this);
    QFont fnt_description = tx_description->font();

    fnt_description.setItalic(TRUE);

    QFontMetrics fm_description(fnt_description);

    tx_description->setVScrollBarMode(Q3ScrollView::AlwaysOff);
    tx_description->setHScrollBarMode(Q3ScrollView::AlwaysOff);
    tx_description->setMinimumSize(fm_description.size(0, htmltext_description));

    hbox->addWidget(tx_description);

    page_description->setLayout(hbox);

    // License

    QWidget *page_license = new QWidget;

    hbox = new Q3HBoxLayout;
    hbox->setMargin(5);

    const char htmltext[] = "This program is licensed under the terms of the GNU General Public\n"
                            "License version 3\n\n"
                            "Available online under:\n"
                            "http://www.gnu.org/licenses/gpl-3.0.html";

    Q3TextView * tx = new Q3TextView(htmltext, QString(), this);
    QFont fnt = tx->font();

    fnt.setItalic(TRUE);

    QFontMetrics fm(fnt);

    tx->setVScrollBarMode(Q3ScrollView::AlwaysOff);
    tx->setHScrollBarMode(Q3ScrollView::AlwaysOff);
    tx->setMinimumSize(fm.size(0, htmltext));

    hbox->addWidget(tx);

    page_license->setLayout(hbox);

    // Build tabs

    tabs->addTab(page_description, "Description");
    tabs->addTab(page_license, "License");
    vbox->addWidget(tabs);

    // Build button

    hbox = new Q3HBoxLayout(vbox);
    hbox->setMargin(5);
    QPushButton * ok = new QPushButton(TR("&OK"), this);

    ok->setDefault(TRUE);

    hbox->addWidget(new QLabel(this));
    hbox->addWidget(ok);
    hbox->addWidget(new QLabel(this));

    if (UmlDesktop::fixed())
        UmlDesktop::tocenter(this);

    connect(ok, SIGNAL(clicked()), this, SLOT(accept()));
}

