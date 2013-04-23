
#include "Dialog.h"

#include <stdlib.h>

#include <qlineedit.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <q3filedialog.h>
#include <qfileinfo.h>
#include <qlabel.h>
#include <q3hbox.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qradiobutton.h>
#include <q3buttongroup.h>
//Added by qt3to4:
#include "misc/mystr.h"
#include <Q3VBoxLayout>
#include <QSettings>
#include "SmallPushButton.h"

Dialog::Dialog(WrapperStr & path, WrapperStr & encoding, WrapperStr & nomodel, WrapperStr & genview, WrapperStr & uml20, WrapperStr & pk, WrapperStr & vis, WrapperStr & primitivetype, WrapperStr & genextension, WrapperStr & geneclipse, WrapperStr & commentexporter, WrapperStr & linefeed, Language & lang)
    : QDialog(0, 0, TRUE), _path(path), _encoding(encoding), _nomodel(nomodel), _genview(genview), _uml20(uml20), _pk(pk), _vis(vis), _primitivetype(primitivetype), _genextension(genextension), _geneclipse(geneclipse), _commentexporter(commentexporter), _linefeed(linefeed), _lang(lang)
{
    Q3VBoxLayout * vbox = new Q3VBoxLayout(this);
    Q3HBox * htab;

    vbox->setMargin(5);

    // get xmi pathname

    htab = new Q3HBox(this);
    htab->setMargin(5);
    vbox->addWidget(htab);

    new QLabel("generated \nXMI file : ", htab);

    ed = new QLineEdit(htab);
    ed->setText(_path);

    new QLabel(" ", htab);

    SmallPushButton * br = new SmallPushButton("browse", htab);

    connect(br, SIGNAL(clicked()), this, SLOT(browse()));

    // to choose encoding

    htab = new Q3HBox(this);
    htab->setMargin(5);
    vbox->addWidget(htab);

    new QLabel("Encoding : ", htab);

    QString charset = getenv("BOUML_CHARSET");
    int index = 0;

    while ((index = charset.find('_')) != -1) {
        charset[index] = '-';
        index += 1;
    }

    encoding_cb = new QComboBox(TRUE, htab);
    htab->setStretchFactor(encoding_cb, 1000);

    if (_encoding.isEmpty())
        _encoding = "UTF-8";

    encoding_cb->insertItem(_encoding);

    if (!charset.isEmpty() && (_encoding != (const char *) charset))
        encoding_cb->insertItem(charset);

    if (_encoding != "UTF-8")
        encoding_cb->insertItem("UTF-8");

#ifdef WIN32

    if ((_encoding != "windows-1252") && (charset != "windows-1252"))
        encoding_cb->insertItem("windows-1252");

    if ((_encoding != "ISO-8859-1") && (charset != "ISO-8859-1"))
        encoding_cb->insertItem("ISO-8859-1");

#else

    if ((_encoding != "ISO-8859-1") && (charset != "ISO-8859-1"))
        encoding_cb->insertItem("ISO-8859-1");

    if ((_encoding != "windows-1252") && (charset != "windows-1252"))
        encoding_cb->insertItem("windows-1252");

#endif

    // uml 2.0 or uml2.1

    htab = new Q3HBox(this);
    htab->setMargin(5);
    vbox->addWidget(htab);

    Q3ButtonGroup * bg = new Q3ButtonGroup(2, Qt::Horizontal, "Uml", htab);

    uml20_rb = new QRadioButton("uml 2.0", bg);
    uml21_rb = new QRadioButton("uml 2.1", bg);

    if (uml20 == "yes")
        uml20_rb->setChecked(TRUE);
    else
        uml21_rb->setChecked(TRUE);

    // generate model

    htab = new Q3HBox(this);
    htab->setMargin(5);
    vbox->addWidget(htab);

    nomodel_cb = new QCheckBox("Don't generate model level", htab);

    if (_nomodel == "yes")
        nomodel_cb->setChecked(TRUE);

    // generate view checkbox

    htab = new Q3HBox(this);
    htab->setMargin(5);
    vbox->addWidget(htab);

    genview_cb = new QCheckBox("Generate views as package (not compatible with profile generation)", htab);

    if (_genview == "yes")
        genview_cb->setChecked(TRUE);

    // generate pk_ prefix

    htab = new Q3HBox(this);
    htab->setMargin(5);
    vbox->addWidget(htab);

    pk_cb = new QCheckBox("Generate 'pk_' prefix for parameter direction", htab);

    if (_pk == "yes")
        pk_cb->setChecked(TRUE);

    // generate vis_ prefix

    htab = new Q3HBox(this);
    htab->setMargin(5);
    vbox->addWidget(htab);

    vis_cb = new QCheckBox("Generate 'vis_' prefix for visibility", htab);

    if (_vis == "yes")
        vis_cb->setChecked(TRUE);

    // use PrimitiveType rather than DataType

    htab = new Q3HBox(this);
    htab->setMargin(5);
    vbox->addWidget(htab);

    primitivetype_cb = new QCheckBox("Use PrimitiveType rather than DataType", htab);

    if (_primitivetype == "yes")
        primitivetype_cb->setChecked(TRUE);

    // generate extension

    htab = new Q3HBox(this);
    htab->setMargin(5);
    vbox->addWidget(htab);

    genextension_cb = new QCheckBox("Generate extensions", htab);

    if (_genextension == "yes")
        genextension_cb->setChecked(TRUE);

    // generate for Eclipse

    htab = new Q3HBox(this);
    htab->setMargin(5);
    vbox->addWidget(htab);

    geneclipse_cb = new QCheckBox("Generate for Eclipse (aggregation set on other relation side)", htab);

    if (_geneclipse == "yes")
        geneclipse_cb->setChecked(TRUE);

    // comment exporter

    htab = new Q3HBox(this);
    htab->setMargin(5);
    vbox->addWidget(htab);

    commentexporter_cb = new QCheckBox("Comment exporter indication (needed to import in some tools like Rsa)", htab);

    if (_commentexporter == "yes")
        commentexporter_cb->setChecked(TRUE);

    // generate &#10; rather than linefeed

    htab = new Q3HBox(this);
    htab->setMargin(5);
    vbox->addWidget(htab);

    linefeed_cb = new QCheckBox("Generate lf and cr characters in string rather than '&&#10;' and '&&#13;'", htab);

    if (_linefeed == "yes")
        linefeed_cb->setChecked(TRUE);

    // uml , c++, java, cancel buttons

    htab = new Q3HBox(this);
    htab->setMargin(5);
    vbox->addWidget(htab);

    new QLabel(htab);
    QPushButton * uml = new QPushButton("&Uml", htab);
    new QLabel(htab);
    QPushButton * cpp = new QPushButton("&C++", htab);
    new QLabel(htab);
    QPushButton * java = new QPushButton("&Java", htab);
    new QLabel(htab);
    QPushButton * cancel = new QPushButton("&Cancel", htab);
    new QLabel(htab);
    QSize bs(cancel->sizeHint());

    uml->setFixedSize(bs);
    cpp->setFixedSize(bs);
    java->setFixedSize(bs);

    connect(uml, SIGNAL(clicked()), this, SLOT(accept_uml()));
    connect(cpp, SIGNAL(clicked()), this, SLOT(accept_cpp()));
    connect(java, SIGNAL(clicked()), this, SLOT(accept_java()));
    connect(cancel, SIGNAL(clicked()), this, SLOT(reject()));

    // help

    htab = new Q3HBox(this);
    htab->setMargin(5);
    vbox->addWidget(htab);

    new QLabel(htab);
    new QLabel("Remark : to help Eclipse to import the generated file,\n"
               "choose Uml 2.1 and name the file with the extension '.xmi'", htab);
    new QLabel(htab);
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

void Dialog::browse()
{
    QString s = Q3FileDialog::getSaveFileName(_path, "*.xmi", 0);

    if (! s.isEmpty()) {
        if (s.right(4).lower() != ".xmi")
            ed->setText(s + ".xmi");
        else
            ed->setText(s);
    }
}

void Dialog::accept_cpp()
{
    _lang = Cpp;

    accept();
}

void Dialog::accept_uml()
{
    _lang = Uml;
    accept();
}

void Dialog::accept_java()
{
    _lang = Java;
    accept();
}

void Dialog::accept()
{
    _path = ed->text();
    _encoding = encoding_cb->currentText();
    _nomodel = (nomodel_cb->isChecked()) ? "yes" : "no";
    _genview = (genview_cb->isChecked()) ? "yes" : "no";
    _uml20 = (uml20_rb->isChecked()) ? "yes" : "no";
    _pk = (pk_cb->isChecked()) ? "yes" : "no";
    _vis = (vis_cb->isChecked()) ? "yes" : "no";
    _primitivetype = (primitivetype_cb->isChecked()) ? "yes" : "no";
    _genextension = (genextension_cb->isChecked()) ? "yes" : "no";
    _geneclipse = (geneclipse_cb->isChecked()) ? "yes" : "no";
    _commentexporter = (commentexporter_cb->isChecked()) ? "yes" : "no";
    _linefeed = (linefeed_cb->isChecked()) ? "yes" : "no";

    QDialog::accept();
}

