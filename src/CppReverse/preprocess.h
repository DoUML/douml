#ifndef PREPROCESS_H
#define PREPROCESS_H

#include <QStringList>
#include <QTextStream>

int j_preprocess(QString fin, QString& fout,const QStringList include_args, QString working_dir, QString* errtext=NULL,bool output_includes=false);

#endif // PREPROCESS_H
