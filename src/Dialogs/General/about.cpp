#include "about.h"
#include "ui_about.h"

#include <QSettings>
#include <QFileInfo>
#include <QDebug>

About::About(QWidget *parent, const QString &iniFilePath, const QString &version)
    : QDialog(parent)
    , ui(new Ui::About)
{
    ui->setupUi(this);

    QSettings settings(iniFilePath, QSettings::IniFormat);
    QFileInfo fileInfo(iniFilePath);

    ui->lbl_PathDir->setText(iniFilePath);
    ui->lbl_ScriptName->setText(fileInfo.fileName());

    // Pull the script version from the [General] section.  If a value was
    // provided via the constructor, use it as a fallback.
    scriptVersion = settings.value("General/Version", version).toString();

    // Update the UI label displaying the script version
    if (ui->lbl_SVersionN)
        ui->lbl_SVersionN->setText(scriptVersion);

    // Connect "About Qt" button
    connect(ui->btn_AboutQt, &QPushButton::clicked, this, []() {
        QApplication::aboutQt();
    });

    qDebug() << "Version read from INI:" << scriptVersion; // Debugging
}

About::~About()
{
    delete ui;
}
