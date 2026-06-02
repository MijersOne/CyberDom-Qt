#include "setflags.h"
#include "ui_setflags.h"
#include "cyberdom.h"
#include "scriptparser.h"

#include <QMessageBox>
#include <QListWidgetItem>
#include <QInputDialog>
#include <QDebug>

SetFlags::SetFlags(QWidget *parent, ScriptParser *parser)
    : QDialog(parent)
    , ui(new Ui::SetFlags)
    , parser(parser)
{
    ui->setupUi(this);

    // Get a reference to the main application to get ACTIVE flags
    CyberDom *mainApp = qobject_cast<CyberDom*>(parent);
    QMap<QString, FlagData> activeFlags;
    if (mainApp) {
        activeFlags = mainApp->getActiveFlags();
    }

    QStringList availableFlags;

    // 1. Get flags defined in the script
    if (parser) {
        const auto& flagDefs = parser->getScriptData().flagDefinitions;

        for (auto it = flagDefs.begin(); it != flagDefs.end(); ++it) {
            QString flagName = it.key(); // This is usually lowercase from parser

            // Check if this flag is already active (Case Insensitive)
            bool isActive = false;
            for(const QString& activeKey : activeFlags.keys()) {
                if(activeKey.compare(flagName, Qt::CaseInsensitive) == 0) {
                    isActive = true;
                    break;
                }
            }

            if (!isActive) {
                availableFlags.append(flagName);
            }
        }
    }

    // 2. Add common/built-in flags (if not already present)
    QStringList commonFlags = {"naked", "plugged", "Kneeling", "TV", "NoPhoneCall"};
    for (const QString &flag : commonFlags) {
        // Check if active
        bool isActive = false;
        for(const QString& activeKey : activeFlags.keys()) {
            if(activeKey.compare(flag, Qt::CaseInsensitive) == 0) {
                isActive = true;
                break;
            }
        }

        // Check if already added from script definitions
        bool alreadyInList = false;
        for(const QString& existing : availableFlags) {
            if(existing.compare(flag, Qt::CaseInsensitive) == 0) {
                alreadyInList = true;
                break;
            }
        }

        if (!isActive && !alreadyInList) {
            availableFlags.append(flag);
        }
    }

    // Sort alphabetically for better UX
    availableFlags.sort(Qt::CaseInsensitive);

    // Add items to the list widget
    for (const QString &flagName : availableFlags) {
        // Capitalize first letter for display
        QString displayName = flagName;
        if (!displayName.isEmpty()) displayName[0] = displayName[0].toUpper();

        QListWidgetItem *item = new QListWidgetItem(displayName, ui->lw_AvailableFlags);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
        // Store exact internal name in data
        item->setData(Qt::UserRole, flagName);
    }

    // If no flags are available
    if (availableFlags.isEmpty()) {
        QListWidgetItem *item = new QListWidgetItem("No flags available to set", ui->lw_AvailableFlags);
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
    }

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &SetFlags::setSelectedFlags);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void SetFlags::setSelectedFlags()
{
    QList<QString> selectedFlags;

    for (int i = 0; i < ui->lw_AvailableFlags->count(); i++) {
        QListWidgetItem *item = ui->lw_AvailableFlags->item(i);
        if (item->checkState() == Qt::Checked) {
            // Retrieve the actual flag name we stored
            selectedFlags.append(item->data(Qt::UserRole).toString());
        }
    }

    if (selectedFlags.isEmpty()) {
        QMessageBox::information(this, "No Flags Selected", "No flags were selected.");
        return;
    }

    // Ask for duration
    bool ok;
    int durationMinutes = QInputDialog::getInt(this, "Flag Duration", "Enter duration in minutes (0 for permanent):", 0, 0, 10080, 5, &ok);
    if (!ok) {
        return;
    }

    for (const QString &flagName : selectedFlags) {
        emit flagSetRequested(flagName, durationMinutes);
    }

    accept();
}

SetFlags::~SetFlags()
{
    delete ui;
}
