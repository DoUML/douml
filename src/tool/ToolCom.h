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

#ifndef TOOLCOM_H
#define TOOLCOM_H

#include <qobject.h>
#include <q3ptrlist.h>
#include <QProcess>
//Added by qt3to4:
#include <Q3PtrList>

#include "BrowserNode.h"
#include <QSettings>
#include "../Tools/ApiCmd.h"

class Socket;
class AType;
class QRect;
class QTimer;

class ToolCom  : public QObject
{
    Q_OBJECT

protected:
    static Q3PtrList<ToolCom> used;
    static Q3PtrList<ToolCom> unused;
    static int exitvalue;

    bool start;
    //! hols the value that processFinished slot should received
    //! originally this value was a parameter passed to a fucntion
    //! but after it got split into func and slot this now needs to be here
    bool exitStaged;
    //bool with_ack;
    bool exit_bouml;
    BrowserNode * target;
    void (*cont)();
    Socket * listen_sock;
    Socket * sock;
    QTimer * timer;
    char * cmd;
    unsigned api_version;
    int id;		// for UmlBaseItem::isToolRunning
    unsigned wanted;
    int already_read;
    char * buffer_in;
    unsigned buffer_in_size;
    char * buffer_out;
    char * p_buffer_out;
    unsigned buffer_out_size;
    QProcess * externalProcess;
    ToolCom();


    void DisconnectExternalProcess();
    void check_size_out(unsigned n);
    const char * read_buffer();

    unsigned bind(unsigned port);
    void close();

    static bool is_running(int id);

public:
    unsigned api_format(bool trueFormat = false) const {
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "DoUML", "settings");
        settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
        if((settings.value("Main/compatibility_save") .toInt() != 1) || trueFormat)
            return api_version;
        else
            return 75;
    }
    virtual ~ToolCom();

    static int run(const char * cmd, BrowserNode *,
                   bool exit = FALSE, bool clr = TRUE,
                   void (*pf)() = 0);
    friend int exit_value();

    void data_received(Socket * who);

    unsigned get_unsigned(const char *& p);
    bool get_bool(const char *& p);
    void * get_id(const char *& p);
    UmlCode get_kind(const char *& p);
    const char * get_string(const char *& p);
    bool get_relation_kind(UmlCode & v, const char *& p);
    bool get_visibility(UmlVisibility & v, const char *& p);
    bool get_extended_visibility(UmlVisibility & v, const char *& p);
    bool get_direction(UmlParamDirection & v, const char *& p);
    void get_type(AType & t, const char *& p);
    void skip_type(const char *& p);

    void write_unsigned(unsigned u);
    void write_bool(bool);
    void write_id(BrowserNode * bn, char kind, const char * name);
    void write_id(void * id);
    void write_string(QString str);
    void write_string(WrapperStr str);
    void write_string(const char * p);
    void write_char(char c);
    void write(QRect r);
    void write_ack(bool);
    void fatal_error(const char * msg);

    static void close_all();

signals:
    void closed();

public slots:
    void connexion_timeout();
    void processFinished();
};

#endif
