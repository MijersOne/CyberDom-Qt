#include "setflags.h"
#include "ui_setflags.h"
#include "cyberdom.h"

#include <QMessageBox>
#include <QListWidgetItem>
#include <QInputDialog>
#include <QDebug>

SetFlags::SetFlags(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SetFlags)
{
    ui->setupUi(this);

    // Get a reference to the main application
    CyberDom *mainApp = qobject_cast<CyberDom*>(parent);
    if (!mainApp) {
        qDebug() << "[ERROR] Could not get reference to main application";
        return;
    }

    // Get the list of all sections of the INI data
    QMap<QString, QMap<QString, QString>> iniData = mainApp->getIniData();

    // Find all flag definitions in the INI data
    QStringList availableFlags;
    QMap<QString, FlagData> activeFlags;

    // If we can get the activeFlags from the main app, do so
    if (mainApp) {
        activeFlags = mainApp->getActiveFlags();
    }

    // Collect all flag definitions from the INI file
    for (auto it = iniData.begin(); it != iniData.end(); ++it) {
        QString section = it.key();

        // Check if this is a flag definition
        if (section.startsWith("flag-", Qt::CaseInsensitive)) {
            QString flagName = section.mid(5); // Remove "flag-" prefix

            // Only include flags that aren't already active
            if (!activeFlags.contains(flagName)) {
                availableFlags.append(flagName);
            }
        }
    }

    // Also include common flags from the script manual
    QStringList commonFlags = {"naked", "plugged", "Kneeling", "TV", "NoPhoneCall"};
    for (const QString &flag : commonFlags) {
        if (!availableFlags.contains(flag) && !activeFlags.contains(flag)) {
            availableFlags.append(flag);
        }
    }

    // Add items to the list widget
    for (const QString &flagName : availableFlags) {
        QListWidgetItem *item = new QListWidgetItem(flagName, ui->lw_AvailableFlags);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable); // Make item checkable
        item->setCheckState(Qt::Unchecked); // Initially unchecked
    }

    // If no flags are available, show a message
    if (availableFlags.isEmpty()) {
        QListWidgetItem *item = new QListWidgetItem("No flags available to set", ui->lw_AvailableFlags);
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled); // Disable the item
    }

    // Connect the button box accepted signal to our custom slot
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &SetFlags::setSelectedFlags);
}

void SetFlags::setSelectedFlags()
{
    // Get the list of selected items from the list widget
    QList<QString> selectedFlags;

    for (int i = 0; i < ui->lw_AvailableFlags->count(); i++) {
        QListWidgetItem *item = ui->lw_AvailableFlags->item(i);
        if (item->checkState() == Qt::Checked) {
            selectedFlags.append(item->text());
        }
    }

    // If no flags were selected, show a message and return
    if (selectedFlags.isEmpty()) {
        QMessageBox::information(this, "No Flags Selected", "No flags were selected. Please select at least one flag or cancel.");
        return;
    }

    // Ask for duration
    bool ok;
    int durationMinutes = QInputDialog::getInt(this, "Flag Duration", "Enter duration in minutes (0 for permanent):", 0, 0, 10080, 5, &ok); // 7 days max
    if (!ok) {
        // User cancelled the duration dialog
        return;
    }

    // Emit the signal for each selected flag
    for (const QString &flagName : selectedFlags) {
        emit flagSetRequested(flagName, durationMinutes);
    }

    // Accept the dialog (close it)
    accept();
}

SetFlags::~SetFlags()
{
    delete ui;
}
