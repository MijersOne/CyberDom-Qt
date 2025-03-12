#include "selectpunishment.h"
#include "ui_selectpunishment.h"
#include "cyberdom.h"
#include <QSettings>
#include <QMessageBox>

SelectPunishment::SelectPunishment(QWidget *parent, QMap<QString, QMap<QString, QString>> iniData)
    : QDialog(parent)
    , ui(new Ui::SelectPunishment)
    , iniData(iniData) // Pass the iniData to the constructor
{
    ui->setupUi(this);
    loadPunishments(); // Load punishments when the dialog is created
}

SelectPunishment::~SelectPunishment()
{
    delete ui;
}

void SelectPunishment::loadPunishments()
{
    ui->punishmentcomboBox->clear(); // Clear existing items

    // Iterate through the INI data and extract punishment names
    QMapIterator<QString, QMap<QString, QString>> i(iniData);
    while (i.hasNext()) {
        i.next();
        if (i.key().startsWith("punishment-")) {
            QString punishmentName = i.key().mid(11); // Extract the name after "punishment-"
            ui->punishmentcomboBox->addItem(punishmentName);
        }
    }
}

void SelectPunishment::on_buttonBox_accepted()
{
    QString selectedPunishmentName = ui->punishmentcomboBox->currentText();

    if (selectedPunishmentName.isEmpty()) {
        QMessageBox::warning(this, "No Punishment Selected", "Please select a punishment to apply.");
        return;
    }

    QString selectedPunishmentKey = "punishment-" + selectedPunishmentName;

    CyberDom *mainApp = qobject_cast<CyberDom*>(parentWidget());
    if (!mainApp) {
        QMessageBox::warning(this, "Error", "Could not access the main application.");
        return;
    }

    if (iniData.contains(selectedPunishmentKey)) {
        // Add the punishment to assignments
        mainApp->addPunishmentToAssignments(selectedPunishmentName);

        QMap<QString, QString> punishmentDetails = iniData[selectedPunishmentKey];

        // Display Punishment Information
        QString punishmentInfo = "Starting Punishment: " + selectedPunishmentName;
        for (const auto &key : punishmentDetails.keys()) {
            punishmentInfo += key + ": " + punishmentDetails[key] + "\n";
        }
        QMessageBox::information(this, "Punishment Details", punishmentInfo);
    } else {
        QMessageBox::warning(this, "Error", "Punishment details not found in script.");
    }
}
