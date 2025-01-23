#include "about.h"
#include "ui_about.h"

#include <QSettings>
#include <QFileInfo>

About::About(QWidget *parent, const QString &iniFilePath, const QString &version)
    : QDialog(parent)
    , ui(new Ui::About)
{
    ui->setupUi(this);

    QSettings settings(iniFilePath, QSettings::IniFormat);
    QFileInfo fileInfo(iniFilePath);

    ui->lbl_PathDir->setText(iniFilePath);
    ui->lbl_ScriptName->setText(fileInfo.fileName());

    // Read the version from the .ini file and update lbl_SVersionN
    ui->lbl_SVersionN->setText(version);

    qDebug() << "Version read from INI:" << version; // Debugging
}

About::~About()
{
    delete ui;
}
