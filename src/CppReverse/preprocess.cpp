#include "preprocess.h"

#include "mcpp_lib.h"
#include "errno.h"
#include <QDir>
#include <QTextStream>
#include <QDebug>

int j_preprocess(QString fin, QString& fout,QStringList include_args, const QString working_dir, QString* errstream) {


    QStringList arguments(include_args);
    arguments << fin;

    std::vector<char *> argv(arguments.count() + 2);
    argv[arguments.count() + 1] = 0;
    argv[0]=(char*)"douml_mpcc_lib";
    for (int i = 0; i < arguments.count(); ++i) {
        QString arg = arguments.at(i);
        argv[i + 1] = ::strdup(arg.toLocal8Bit().constData());
    }
    mcpp_use_mem_buffers( true);
    QString old_dir=QDir::currentPath();
    QDir::setCurrent(working_dir);
    int res=mcpp_lib_main(arguments.count() +1, &argv[0]);
    QDir::setCurrent(old_dir);

    if(errstream)
        *errstream = mcpp_get_mem_buffer(ERR);

    fout=QDir::tempPath()+QDir::separator()+"douml"+QDir::separator()+fin.replace("[^\\w-. ]","_")+".included";

    QFile outputfile(fout);
    if(!outputfile.open(QIODevice::WriteOnly )) {
        fout=fin;
    }

    QTextStream out(&outputfile);
    out<<mcpp_get_mem_buffer(OUT);
    return res;
}
