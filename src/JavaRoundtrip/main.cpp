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

#include <qapplication.h>
#include <q3filedialog.h>
//Added by qt3to4:
#include <Q3CString>
//Added by qt3to4:
#include <Q3PtrList>
#include <stdlib.h>
#include <qfile.h>
#include <qdir.h>
#include <qdatastream.h>
#include <QTextStream>
#include <qmessagebox.h>

#include "UmlCom.h"
#include "UmlItem.h"
#include "UmlPackage.h"
#include "Package.h"
#include "UmlArtifact.h"
#include "JavaSettings.h"
#include "Statistic.h"

void remove_crlf(char * s)
{
    int len = strlen(s);

    if (len != 0) {
        if (s[len - 1] == '\n')
            s[--len] = 0;

        if ((len != 0) && (s[len - 1] == '\r'))
            s[len - 1] = 0;
    }
}

int main(int argc, char ** argv)
{
    if (argc != 2)
        return 0;

    if (UmlCom::connect(Q3CString(argv[1]).toUInt())) {
        bool aborted = TRUE;

        try {
            UmlCom::trace("<b>Java roundtrip</b> release 1.5<br><hr>");
            UmlCom::traceAutoRaise(FALSE);

            char * argv = 0;
            int argc = 0;
            QApplication * app = new QApplication(argc, &argv);
            UmlItem * item = UmlCom::targetItem();
            int n;

            switch (item->kind()) {
            default:
                UmlCom::trace("<font face=helvetica><b>must be applied on a <i>package, class view, deployment view, artifact</i> or <i>class</i></b></font><br>");
                aborted = FALSE;
                throw 0;

            case aPackage:
                n = ((UmlPackage *) item)->count_roundtriped();
                break;

            case aClassView:
            case aDeploymentView:
            case anArtifact:
                n = 1;
                break;

            case aClass:
                if (item->parent()->kind() != aClassView) {
                    UmlCom::trace("<font face=helvetica><b>can't be applied on a <i>class</i> nested or out of a <i>class view</i></b></font><br>");
                    aborted = FALSE;
                    throw 0;
                }

                n = 1;
                break;
            }

            UmlCom::trace("<font face=helvetica>Upload project ...</font>");
            Package::init(UmlPackage::getProject(), app);
            UmlCom::trace("<font face=helvetica>...done</font><br>");
            UmlCom::message("");
            Package::set_step(0, n);

            UmlCom::trace("<font face=helvetica>Preparation...</font>");

            if (!item->set_roundtrip_expected() &&
                (QMessageBox::warning(0, "Roundtrip",
                                      "Some elements to roundtrip are read-only and will not be updated\n\n"
                                      "Roundtrip anyway ?",
                                      "Yes", "No", QString(), 1, 1)
                 != 0)) {
                aborted = FALSE;
                throw 0;
            }

            Package::set_step(0, -1);
            UmlCom::trace("<font face=helvetica>...done</font><br>");

            if ((item->kind() == aPackage) && !UmlArtifact::is_roundtrip_usefull()) {
                UmlCom::trace("<font face=helvetica>you don't ask for to roundtrip artifact(s)<br><br>"
                              "probably you want to do a <i>reverse</i> rather than a <i>roundtrip</i></font><br>");
                aborted = FALSE;
                throw 0;
            }

            QString path;
            // note : QFile fp(QDir::home().absFilePath(".doumlcat")) doesn't work
            // if the path contains non latin1 characters, for instance cyrillic !
            QString s = QDir::home().absFilePath(".doumlcat");
            FILE * fp = fopen((const char *) s, "r");
















            if (fp != 0) {
                char line[512];

                if (fgets(line, sizeof(line) - 1, fp) != 0) {
                    remove_crlf(line);
                    path = line;
                }

                fclose(fp);
            }

            while (!(path =
                         Q3FileDialog::getOpenFileName(path, "*.cat",
                                 0, 0,
                                 "select a java catalog file to read it, or cancel"))
                   .isEmpty()) {
                QFile f(path);

                if (f.open(QIODevice::ReadOnly)) {
                    if ((fp = fopen((const char *) s, "w")) != 0) {
                        fwrite((const char *) path, 1, path.length(), fp);
                        fputc('\n', fp);
                        fclose(fp);
                    }

                    QDataStream dt(&f);

                    Package::get_root()->restore_children(dt);
                }
            }

            n = 0;
            item->scan_it(n);
            JavaSettings::set_UseDefaults(TRUE);
            UmlPackage::getProject()->set_childrenVisible(FALSE);
            item->send_it(n);
            Statistic::produce();

            // umark all
            {
                Q3PtrVector<UmlItem> marked = UmlItem::markedItems();
                UmlItem ** v = marked.data();
                UmlItem ** const vsup = v + marked.size();

                for (; v != vsup; v += 1)
                    (*v)->set_isMarked(FALSE);
            }

            QList<UmlItem *> useless;

            item->mark_useless(useless);

            if (!useless.isEmpty() &&
                (QMessageBox::warning(0, "Java roundtrip",
                                      "The marked elements are useless because they don't\n"
                                      "correspond to something in the roundtriped files\n\n\n"
                                      "Delete them ?",
                                      "Yes", "No", QString(), 1, 1)
                 == 0)) {

                foreach (UmlItem *item, useless) {
                    if (item->isMarked())
                        item->deleteIt();
                };
            }

            UmlPackage::getProject()->set_childrenVisible(TRUE);
            item->set_childrenVisible(TRUE); // re select it
            aborted = FALSE;
        }
        catch (...) {
        }

        try {
            // socket may be already closed
            if (aborted)
                UmlCom::trace("<font face=helvetica><br><b>Rountrip aborted!</b></font><br>");

            UmlCom::message("");
            UmlCom::showTrace();
            UmlCom::bye((aborted) ? 1 : 0);	// application must not be deleted
        }
        catch (...) {
        }
    }

    UmlCom::close();	// application must not be deleted
    return 0;
}
