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

//#define DEBUGCOM





# include <errno.h>
#ifdef DEBUGCOM
#include <iostream>
//Added by qt3to4:
#include <Q3PtrList>
using namespace std;
#endif
#include <QCoreApplication>
#include <QProcess>
#include <qtimer.h>
#include "ToolCom.h"
#include "Socket.h"
#include "BrowserNode.h"
#include "GenerationSettings.h"
#include "TraceDialog.h"
#include "AType.h"
#include "UmlWindow.h"
#include "BrowserView.h"
#include "BrowserClass.h"
#include "BrowserPackage.h"
#include "BrowserOperation.h"
#include "DialogUtil.h"
#include "mu.h"
#include "err.h"
#include "Logging/QsLog.h"

Socket::Socket(ToolCom * c)
    : Q3SocketDevice(Q3SocketDevice::Stream), com(c)
{
    setAddressReusable(TRUE);
    notifier = new QSocketNotifier(socket(), QSocketNotifier::Read);
    QObject::connect(notifier, SIGNAL(activated(int)),
                     this, SLOT(data_received()));
}

Socket::Socket(ToolCom * c, int s)
    : Q3SocketDevice(s, Q3SocketDevice::Stream), com(c)
{
    setAddressReusable(TRUE);
    notifier = new QSocketNotifier(s, QSocketNotifier::Read);
    QObject::connect(notifier, SIGNAL(activated(int)),
                     this, SLOT(data_received()));
}

Socket::~Socket()
{
    delete notifier;
}

bool Socket::write_block(char * data, unsigned int len)
{
    /* the four first bytes of data are free to contains the length
     these are taken into account by len */
    len -= 4;
    data[0] = len >> 24;
    data[1] = len >> 16;
    data[2] = len >> 8;
    data[3] = len;

#ifdef DEBUGCOM
    QLOG_INFO() << "ToolCom write " << len << "bytes\n";
#endif

    QLOG_INFO() << QByteArray(data, len);
    len += 4;

    for (;;) {
        int sent = writeBlock(data, len);

        if (sent == -1)
            return FALSE;
        else if ((len -= sent) == 0) {
            flush();
            return TRUE;
        }
        else {
            waitForMore(100);
            data += sent;
        }
    }
}

void Socket::data_received()
{
    com->data_received(this);
}

//

Q3PtrList<ToolCom> ToolCom::used;
Q3PtrList<ToolCom> ToolCom::unused;
int ToolCom::exitvalue;

ToolCom::ToolCom()
{
    listen_sock = 0;
    sock = 0;
    timer = 0;
    buffer_in_size = 0;
    buffer_in = 0;
    buffer_out_size = 0;
    buffer_out = 0;
    p_buffer_out = 0;
    wanted = 0;
    id = 0;
    start = true;
    exit_bouml = false;
    target = 0;
    cont = 0;
    cmd = 0;
    api_version = -1;
    already_read = -1;
    externalProcess = 0;
    exitStaged = false;
}

ToolCom::~ToolCom()
{
    DisconnectExternalProcess();
}

int ToolCom::run(const char * cmd, BrowserNode * bn,
                 bool exit, bool clr, void (*pf)())
{
    QLOG_TRACE() << Q_FUNC_INFO << " called";

    if (exit)
        exitvalue = -1;

    TraceDialog::trace_auto_raise(TRUE);

    if (clr)
        TraceDialog::clear();

    ToolCom * com = (unused.isEmpty())
            ? new ToolCom
            : unused.take(0);

    com->DisconnectExternalProcess();

    static int rank = 0;

    com->id = ++rank;
    com->wanted = 0;

    used.append(com);

    com->target = bn;
    com->cont = pf;

    com->exit_bouml = exit;
    com->cmd = strdup(cmd);
    com->timer = new QTimer(com);
    connect(com->timer, SIGNAL(timeout()), com, SLOT(connexion_timeout()));
    com->timer->start(30 * 10000, TRUE);

    unsigned port = com->bind(1024);


    QStringList commandList = QString(cmd).split(" ");
    QString command = commandList.at(0);
    commandList.removeAt(0);
    com->externalProcess = new QProcess();
    connect(com->externalProcess, SIGNAL(finished(int)), com, SLOT(processFinished()));

    errno = 0;
    QStringList arguments;
    arguments.append(commandList << QString::number(port));
    com->exitStaged = exit;
    com->externalProcess->start(command, arguments);
    com->start = TRUE;

    return com->id;
}

int exit_value()
{
    return ToolCom::exitvalue;
}

bool ToolCom::is_running(int id)
{
    Q3PtrListIterator<ToolCom> it(used);

    for (; it.current(); ++it)
        if (it.current()->id == id)
            return TRUE;

    return FALSE;
}

unsigned ToolCom::bind(unsigned port)
{
    listen_sock = new Socket(this);

    if (buffer_in_size == 0) {
        buffer_in_size = 1024;
        buffer_in = new char[buffer_in_size];
    }

    if (buffer_out_size == 0) {
        buffer_out_size = 1024;
        buffer_out = new char[buffer_out_size];
    }

    p_buffer_out = buffer_out + 4/*bytes for length*/;

    QHostAddress ha;

    ha.setAddress("127.0.0.1");

    if (listen_sock->bind(ha, 0))
        port = listen_sock->port();
    else
        while (!listen_sock->bind(ha, port))
            port += 1;

    listen_sock->listen(1);

    return port;
}

void ToolCom::close()
{
    if (listen_sock) {
        if (timer != 0) {
            delete timer;
            timer = 0;
        }

        if (cmd != 0) {
            free(cmd);
            cmd = 0;
        }

        delete listen_sock;
        listen_sock = 0;

        if (sock != 0) {
            delete sock;
            sock = 0;
        }

        used.remove(this);
        unused.append(this);
    }
}

void ToolCom::close_all()
{
    while (! used.isEmpty())
        used.at(0)->close();
}

const char * ToolCom::read_buffer()
{
    if (wanted == 0)
    {
        char s[4];

        while ((sock->readBlock(s, 4) != 4));

        const char * p = s;

        if ((wanted = get_unsigned(p)) == 0)
            return "";
        else
            already_read = 0;
    }

    if (buffer_in_size < wanted) {
        delete [] buffer_in;
        buffer_in_size = wanted + 1024;
        buffer_in = new char[buffer_in_size];
    }

    int nread = sock->readBlock(buffer_in + already_read, wanted);

    if (nread == -1) {
        fatal_error("ToolCom read error");
        return 0;
    }
    else if ((wanted -= nread) == 0)
        return buffer_in;
    else {
        already_read += nread;
        return 0;
    }
}

unsigned ToolCom::get_unsigned(const char *& p)
{
    p += 4;

    return (((unsigned char *) p)[-4] << 24) +
            (((unsigned char *) p)[-3] << 16) +
            (((unsigned char *) p)[-2] << 8) +
            ((unsigned char *) p)[-1];
}

bool ToolCom::get_bool(const char *& p)
{
    return (*p++ != 0);
}

void * ToolCom::get_id(const char *& p)
{
    void * a;

    memcpy((char *) &a, p, sizeof(void *));
    p += sizeof(void *);
    return a;
}

const char * ToolCom::get_string(const char *& p)
{
    const char * r = p;

    p += strlen(p) + 1;
    return r;
}

UmlCode ToolCom::get_kind(const char *& p)
{
    int v = *p++;

    if ((api_version < 24) && (v >= (UmlObjectDiagram - UmlRelations)))
        // adds UmlObjectDiagram & UmlActivityDiagram
        v += 2;

    if ((api_version < 44) && (v >= (umlAcceptCallAction - UmlRelations)))
        // adds umlAcceptCallAction .. umlReduceAction
        v += 7;

    return (UmlCode)(v + UmlRelations);
}

bool ToolCom::get_relation_kind(UmlCode & v, const char *& p)
{
    v = (UmlCode) * p++;

    return ((unsigned) v) < UmlRelations;
}

bool ToolCom::get_visibility(UmlVisibility & v, const char *& p)
{
    v = (UmlVisibility) * p++;

    if ((api_version < 23) &&
            (v == UmlPackageVisibility))	// UmlDefaultVisibility for plug-out
        v = UmlDefaultVisibility;

    return ((unsigned) v) != UmlDefaultVisibility;
}

bool ToolCom::get_extended_visibility(UmlVisibility & v, const char *& p)
{
    v = (UmlVisibility) * p++;

    if ((api_version < 23) &&
            (v == UmlPackageVisibility))	// UmlDefaultVisibility for plug-out
        v = UmlDefaultVisibility;

    return ((unsigned) v) <= UmlDefaultVisibility;
}

bool ToolCom::get_direction(UmlParamDirection & v, const char *& p)
{
    v = (UmlParamDirection) * p++;

    return ((unsigned) v) <= UmlOut;
}

void ToolCom::get_type(AType & t, const char *& p)
{
    t.type = (BrowserClass *) get_id(p);
    t.explicit_type = get_string(p);
}

void ToolCom::skip_type(const char *& p)
{
    get_id(p);
    get_string(p);
}

//

void ToolCom::check_size_out(unsigned n)
{
    unsigned used = p_buffer_out - buffer_out;

    if ((used + n) >= buffer_out_size) {
        buffer_out_size = used + n + 1024;

        char * newbuff = new char[buffer_out_size];

        memcpy(newbuff, buffer_out, used);
        delete [] buffer_out;

        p_buffer_out = (buffer_out = newbuff) + used;
    }
}

void ToolCom::write_unsigned(unsigned u)
{
    QLOG_INFO() << "ToolCom::write_unsigned(" << u << ")\n";
    check_size_out(4);

    p_buffer_out[0] = u >> 24;
    p_buffer_out[1] = u >> 16;
    p_buffer_out[2] = u >> 8;
    p_buffer_out[3] = u;

    p_buffer_out += 4;
}

void ToolCom::write_bool(bool b)
{
    check_size_out(1);

    *p_buffer_out++ = (b) ? 1 : 0;
#ifdef DEBUGCOM
    QLOG_INFO() << "ToolCom::write_bool(" << ((b) ? 1 : 0) << ")\n";
#endif
}

void ToolCom::write_id(void * id)
{
    check_size_out(sizeof(void *) + 1);

    *p_buffer_out = sizeof(void *);
    memcpy(p_buffer_out + 1, (char *) &id, sizeof(void *));
    p_buffer_out += sizeof(void *) + 1;
#ifdef DEBUGCOM
    QLOG_INFO() << "ToolCom::write_id(" << (void *) id << ")\n";
#endif
}

void ToolCom::write_id(BrowserNode * bn, char k, const char * s)
{
    unsigned ln = strlen(s) + 1;

    check_size_out(ln + 2 + sizeof(void *));

    *p_buffer_out = sizeof(void *);
    memcpy(p_buffer_out + 1, (char *) &bn, sizeof(void *));

    if ((api_version < 24) && (k >= (UmlObjectDiagram - UmlRelations)))
        // removes UmlObjectDiagram & UmlActivityDiagram
        k -= 2;

    if ((api_version < 44) && (k >= (umlAcceptCallAction - UmlRelations)))
        // remove umlAcceptCallAction .. umlReduceAction
        k -= 7;

    p_buffer_out[sizeof(void *) + 1] = k;
    memcpy(p_buffer_out + sizeof(void *) + 2, s, ln);

    ToolCom::p_buffer_out += ln + 2 + sizeof(void *);
#ifdef DEBUGCOM
    QLOG_INFO() << "ToolCom::write_id(" << (void *) bn << ")\n";
    QLOG_INFO() << "ToolCom::write_char(" << (unsigned) k << ")\n";
    QLOG_INFO() << "ToolCom::write_string(\"" << s << "\")\n";
#endif
}
void ToolCom::write_string(QString str)
{
    QByteArray ba = str.toUtf8();
    const char* pointer = ba.constData();
    write_string(pointer);
}
void ToolCom::write_string(WrapperStr str)
{
    write_string(str.operator QString());
}
void ToolCom::write_string(const char * p)
{
    if (p == 0)
        // from a QString()
        p = "";

    int len = strlen(p) + 1;

    check_size_out(len);
    memcpy(p_buffer_out, p, len);
    p_buffer_out += len;
#ifdef DEBUGCOM
    QLOG_INFO() << "Writing string of length: " << len;
    QLOG_INFO() << "ToolCom::write_string(\"" << p << "\")\n";
#endif
}

void ToolCom::write_char(char c)
{
    check_size_out(1);

    *p_buffer_out++ = c;
#ifdef DEBUGCOM
    QLOG_INFO() << "ToolCom::write_char(" << (unsigned) c << ")\n";
#endif
}

void ToolCom::write(QRect r)
{
    write_unsigned((unsigned) r.x());
    write_unsigned((unsigned) r.y());
    write_unsigned((unsigned) r.width());
    write_unsigned((unsigned) r.height());
}

void ToolCom::write_ack(bool b)
{
    /*if (with_ack)*/ {
        check_size_out(1);

        *p_buffer_out++ = (b) ? 1 : 0;
#ifdef DEBUGCOM
        QLOG_INFO() << "ToolCom::write_ack(" << ((b) ? 1 : 0) << ")\n";
#endif
    }
}

void ToolCom::fatal_error(const char *
                          #ifdef DEBUGCOM
                          msg
                          #endif
                          )
{
#ifdef DEBUGCOM
    QLOG_INFO() << "ToolCom::fatal_error " << msg << '\n';
#endif

    close();
    THROW_ERROR 0;
}

void ToolCom::connexion_timeout()
{
    msg_critical("Douml",
                 QString("connexion timeout for '") + QString(cmd) + QString("'"));
    close();

    if (exit_bouml) {
        BrowserView::remove_temporary_files();
        set_user_id(-1);    // delete lock

        THROW_ERROR 0;
    }
}

void ToolCom::data_received(Socket * who)
{
    bool do_exit = FALSE;
    void (*pf)() = 0;

    if (who == listen_sock) {
        if (sock == 0) {
            int s = listen_sock->accept();

#ifdef DEBUGCOM
            QLOG_INFO() << "ToolCom::connexion() accept ok " << s << '\n';
#endif

            if (s != -1) {
                if (timer != 0) {
                    delete timer;
                    timer = 0;
                }

                sock = new Socket(this, s);
                return;
            }
        }

        // can't connect or comm closed on client side
        close();
        do_exit = exit_bouml;
        pf = cont;
    }
    else if (sock != 0) {
        PRE_TRY;

        try {
            const char * p = read_buffer();

            if (p == 0)
                return;

            if (start) {
                // plug-out start, get API version
                start = FALSE;

                api_version = get_unsigned(p);
                if (api_version < 11) {
                    TraceDialog::add("<font color =\"red\"><b>the plug-out was written for a BDUML release less or equal to 1.5<b></font>");
                    TraceDialog::show_it();
                    close();
                    return;
                }
                /*else if (api_version < ??) {
                TraceDialog::add("<font color =\"red\"><b>the plug-out is too old, update the SYSTEM part and re-compile it<b></font>");
                TraceDialog::show_it();
                close();
                return;
                }*/
                else if (api_version > 55) {
                    TraceDialog::add("<font color =\"red\"><b>the plug-out is incompatible with this too old version of DOUML<b></font>");
                    TraceDialog::show_it();
                    close();
                    return;
                }
            }
            else {
                // cmd sent by the plug-oug
                p += 1;

                switch ((CmdFamily) p[-1]) {
                case onInstanceCmd: {
                    BrowserNode * bn = (BrowserNode *) get_id(p);

#ifdef DEBUGCOM
                    QLOG_INFO() << "ToolCom::data_received() cmd " << (unsigned) p[0]
                                << " for " << bn->get_name() << '\n';
#endif

                    if (!bn->tool_cmd(this, p + 1))
                    {
#ifdef DEBUGCOM
                        QLOG_INFO() << "unknown cmd : " << (unsigned) p[0]
                                    << " for " << bn->get_name() << '\n';
#endif
                        close();
                    }
                }
                    break;

                case classGlobalCmd:
#ifdef DEBUGCOM
                    QLOG_INFO() << "ToolCom::data_received() ClassGlobalCmd "
                                << (unsigned) p[0] << '\n';
#endif

                    if (!BrowserClass::tool_global_cmd(this, p + 1)) {
#ifdef DEBUGCOM
                        QLOG_INFO() << "unknown ClassGlobalCmd : " << p[0] << "\n";
#endif
                        throw 0;
                    }

                    break;

                case packageGlobalCmd:
#ifdef DEBUGCOM
                    QLOG_INFO() << "ToolCom::data_received() PackageGlobalCmd "
                                << (unsigned) p[0] << '\n';
#endif

                    if (!BrowserPackage::tool_global_cmd(this, p + 1)) {
#ifdef DEBUGCOM
                        QLOG_INFO() << "unknown PackageGlobalCmd : <" << p[0] << ">\n";
#endif
                        throw 0;
                    }

                    break;

                case miscGlobalCmd:
#ifdef DEBUGCOM
                    QLOG_INFO() << "ToolCom::data_received() MiscGlobalCmd "
                                << (unsigned) p[0] << '\n';
#endif

                    switch (p[0]) {
                    case byeCmd:
#ifdef DEBUGCOM
                        QLOG_INFO() << "bye\n";
#endif

                        if (exit_bouml) {
                            do_exit = true;
                            p += 1;
                            exitvalue = (api_version > 51) ? (int) get_unsigned(p) : 0;
                        }

                        pf = cont;
                        throw 0;

                    case traceCmd:
                        TraceDialog::add(p + 1);
                        break;

                    case showTraceCmd:
                        TraceDialog::show_it();
                        break;

                    case traceAutoRaiseCmd:
                        TraceDialog::trace_auto_raise(p[1]);
                        break;

                    case messageCmd:
                        UmlWindow::set_message(p + 1);
                        break;

                    case toolRunningCmd:
                        p += 1;
                        write_bool(is_running((int) get_unsigned(p)));
                        break;

                    case targetCmd:
                        if (target->api_compatible(api_format(true)))
                            target->write_id(this);
                        else
                            write_id(0);

                        break;

                    case allMarkedCmd: {
                        Q3PtrList<BrowserNode> marked = BrowserNode::marked_nodes();
                        unsigned n = 0;
                        BrowserNode * bn;

                        for (bn = marked.first(); bn != 0; bn = marked.next())
                            if (bn->api_compatible(api_format(true)))
                                n += 1;

                        write_unsigned(n);

                        for (bn = marked.first(); bn != 0; bn = marked.next())
                            if (bn->api_compatible(api_format(true)))
                                bn->write_id(this);
                    }
                        break;

                    case loadCmd:
                        if (!BrowserNode::edition_active()) {
                            UmlWindow::load_it(p + 1);
                            // com closed
                            return;
                        }

                        break;

                        /*case withAckCmd:
                        with_ack = p[1];
                        break;*/
                    case userIdCmd:
                        write_char(user_id());
                        break;

                    case setUserIdCmd:
                        set_user_id((int)((unsigned char *) p)[1]);
                        break;

                    case setRootPermissionCmd:
                        if (p[1] != 0) {
                            // enter
                            if (BrowserNode::edition_active())
                                write_bool(FALSE);
                            else {
                                BrowserPackage::update_idmax_for_root();
                                set_root_permission(TRUE);
                                write_bool(TRUE);
                            }
                        }
                        else {
                            // exit
                            set_root_permission(FALSE);
                            UmlWindow::saveas_it();
                            UmlWindow::close_it();	// write access of added items not ok
                            // no respons, all com closed by close_it()
                            return;
                        }

                        break;

                    case fromIdCmd: {
                        p += 1;

                        unsigned id = get_unsigned(p);
                        BrowserNode * op;

                        switch (get_kind(p)) {
                        case UmlOperation:
                            op = BrowserOperation::get_it("operation_ref", id);

                            if ((op == 0) || !op->deletedp())
                                break;

                            // no break
                        default:
                            // error
                            op = 0;
                            break;
                        }

                        if (op == 0)
                            write_id(0);
                        else
                            op->write_id(this);
                    }
                        break;

                    default:
#ifdef DEBUGCOM
                        QLOG_INFO() << "unknown MiscGlobalCmd : " << (unsigned) p[0] << "\n";
#endif
                        throw 0;
                    }

                    break;

                case umlSettingsCmd:
#ifdef DEBUGCOM
                    QLOG_INFO() << "ToolCom::data_received() UmlSettingsCmd "
                                << (unsigned) p[0] << '\n';
#endif

                    if (!GenerationSettings::tool_global_uml_cmd(this, p + 1)) {
#ifdef DEBUGCOM
                        QLOG_INFO() << "unknown UmlSettingsCmd : " << (unsigned) p[0] << "\n";
#endif
                        throw 0;
                    }

                    break;

                case cppSettingsCmd:
#ifdef DEBUGCOM
                    QLOG_INFO() << "ToolCom::data_received() CppSettingsCmd "
                                << (unsigned) p[0] << '\n';
#endif

                    if (!GenerationSettings::tool_global_cpp_cmd(this, p + 1)) {
#ifdef DEBUGCOM
                        QLOG_INFO() << "unknown CppSettingsCmd : " << (unsigned) p[0] << "\n";
#endif
                        throw 0;
                    }

                    break;

                case javaSettingsCmd:
#ifdef DEBUGCOM
                    QLOG_INFO() << "ToolCom::data_received() JavaSettingsCmd "
                                << (unsigned) p[0] << '\n';
#endif

                    if (!GenerationSettings::tool_global_java_cmd(this, p + 1)) {
#ifdef DEBUGCOM
                        QLOG_INFO() << "unknown JavaSettingsCmd : " << (unsigned) p[0] << "\n";
#endif
                        throw 0;
                    }

                    break;

                case phpSettingsCmd:
#ifdef DEBUGCOM
                    QLOG_INFO() << "ToolCom::data_received() PhpSettingsCmd "
                                << (unsigned) p[0] << '\n';
#endif

                    if (!GenerationSettings::tool_global_php_cmd(this, p + 1)) {
#ifdef DEBUGCOM
                        QLOG_INFO() << "unknown PhpSettingsCmd : " << (unsigned) p[0] << "\n";
#endif
                        throw 0;
                    }

                    break;

                case idlSettingsCmd:
#ifdef DEBUGCOM
                    QLOG_INFO() << "ToolCom::data_received() IdlSettingsCmd "
                                << (unsigned) p[0] << '\n';
#endif

                    if (!GenerationSettings::tool_global_idl_cmd(this, p + 1)) {
#ifdef DEBUGCOM
                        QLOG_INFO() << "unknown IdlSettingsCmd : " << p[0] << "\n";
#endif
                        throw 0;
                    }

                    break;

                case pythonSettingsCmd:
#ifdef DEBUGCOM
                    QLOG_INFO() << "ToolCom::data_received() PythonSettingsCmd "
                                << (unsigned) p[0] << '\n';
#endif

                    if (!GenerationSettings::tool_global_python_cmd(this, p + 1)) {
#ifdef DEBUGCOM
                        QLOG_INFO() << "unknown PythonSettingsCmd : " << (unsigned) p[0] << "\n";
#endif
                        throw 0;
                    }

                    break;

                default:
#ifdef DEBUGCOM
                    QLOG_INFO() << "unknown CmdFamily : " << (unsigned) p[-1] << "\n";
#endif
                    throw 0;
                }

                if (sock && (p_buffer_out != buffer_out + 4)) {
                    if (!sock->write_block(buffer_out, p_buffer_out - buffer_out))
                        fatal_error("write error");
                    else {
                        p_buffer_out = buffer_out + 4/*bytes for length*/;
                    }
                }
            }
        }
        catch (...) {
            close();
        }

        POST_TRY;
    }

    if (do_exit) {
        BrowserView::remove_temporary_files();
        set_user_id(-1);    // delete lock

        THROW_ERROR 0;
    }

    if (pf != 0)
        pf();
}

void ToolCom::processFinished()
{
    QLOG_TRACE() << "Disconnecting external process signals";

    /*if (errno != 0) {
        msg_critical("Douml",
                     "error while executing '" + QString(cmd) + "'\n"
                     "perhaps you must specify its absolute path"
                     "or set the environment variable PATH ?");
        this->close();

        if (exitStaged) {
            BrowserView::remove_temporary_files();
            set_user_id(-1);    // delete lock

            THROW_ERROR 0;
        }

        //else
    }*/

}

void ToolCom::DisconnectExternalProcess()
{
    if (this->externalProcess) {
        disconnect(this->externalProcess, SIGNAL(finished(int)), this, SLOT(processFinished()));
        this->externalProcess->kill();
        delete this->externalProcess;
        this->externalProcess = 0;
    }
}

