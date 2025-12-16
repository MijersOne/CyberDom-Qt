#include "about.h"
#include "ui_about.h"
#include "cyberdom.h"

#include <QSettings>
#include <QFileInfo>
#include <QDebug>

// --- VERSION HELPER ---
// This grabs the raw version number (0.0.2) passed by the .pro file
// and converts it into a string ("0.0.2") usable by the code.
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#ifdef BUILD_VER
    #define APP_VERSION_STR TOSTRING(BUILD_VER)
#else
    #define APP_VERSION_STR "Unknown"
#endif

About::About(QWidget *parent, const QString &iniFilePath, const QString &version)
    : QDialog(parent)
    , ui(new Ui::About)
{
    ui->setupUi(this);

    QString appVerStr = QString("Version %1 Alpha").arg(APP_VERSION_STR);
    ui->lbl_Version->setText(appVerStr);

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

    qDebug() << "App Version:" << APP_VERSION_STR;
    qDebug() << "Version read from INI:" << scriptVersion; // Debugging
}

About::~About()
{
    delete ui;
}
