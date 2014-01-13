#ifndef PREPROCESS_H
#define PREPROCESS_H

#include <QStringList>
#include <QTextStream>

int j_preprocess(const QString fin, QString& fout,const QStringList include_args, const QString working_dir, QString* errtext=NULL);

#endif // PREPROCESS_H
