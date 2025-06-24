#include "selectpunishment.h"
#include "ui_selectpunishment.h"
#include "cyberdom.h"
#include <QSettings>
#include <QMessageBox>
#include <QInputDialog>
#include <QDebug>

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

    qDebug() << "\n[DEBUG] Starting loadPunishments in SelectPunishment dialog";
    qDebug() << "[DEBUG] iniData contains " << iniData.size() << " sections:";
    
    // Debug: List all sections for easier debugging
    for (auto it = iniData.begin(); it != iniData.end(); ++it) {
        qDebug() << "[DEBUG] Section: " << it.key() << " with " << it.value().size() << " key-value pairs";
    }

    // Iterate through the INI data and extract punishment names
    int punishmentCount = 0;
    QMapIterator<QString, QMap<QString, QString>> i(iniData);
    while (i.hasNext()) {
        i.next();
        if (i.key().toLower().startsWith("punishment-")) {
            // Extract the name after "punishment-"
            QString rawPunishmentName = i.key().mid(11); 
            
            // Debug the punishment data
            QMap<QString, QString> punishmentData = i.value();
            qDebug() << "[DEBUG] Punishment section: " << i.key() << " contains " << punishmentData.size() << " properties:";
            for (auto dataIt = punishmentData.begin(); dataIt != punishmentData.end(); ++dataIt) {
                qDebug() << "[DEBUG]   - " << dataIt.key() << " = " << dataIt.value();
            }
            
            // Add to combobox
            ui->punishmentcomboBox->addItem(rawPunishmentName);
            punishmentCount++;
            qDebug() << "[DEBUG] Added punishment to comboBox: " << rawPunishmentName;
        }
    }
    
    qDebug() << "[DEBUG] Total punishments added to comboBox: " << punishmentCount;
    
    // If no punishments found in iniData, use a hardcoded list as fallback
    if (punishmentCount == 0) {
        qDebug() << "[WARNING] No punishments found in iniData, using fallback list";
        
        // Hardcoded list of punishments from demo-female.ini
        QStringList fallbackPunishments;
        fallbackPunishments << "Stand in the corner # minutes"
                           << "Detention for # minutes"
                           << "Write # times:"
                           << "Write # lines about obedience"
                           << "Write the slave mantra # times"
                           << "Wear a gag for # hours"
                           << "Give yourself # strokes with a belt"
                           << "Give yourself # strokes with your hand on your thighs"
                           << "No TV for # days"
                           << "No hot showers for # days"
                           << "You are forbidden masturbate for # days";
        
        for (const QString &punishment : fallbackPunishments) {
            ui->punishmentcomboBox->addItem(punishment);
            punishmentCount++;
            qDebug() << "[DEBUG] Added fallback punishment to comboBox: " << punishment;
            
            // Also add to iniData to help with other functions
            QMap<QString, QString> dummyData;
            dummyData["value"] = "10";
            dummyData["max"] = "20";
            iniData["punishment-" + punishment] = dummyData;
        }
        
        qDebug() << "[DEBUG] Added " << fallbackPunishments.size() << " fallback punishments";
    }
    
    if (punishmentCount == 0) {
        qDebug() << "[ERROR] Still no punishments available after fallback!";
        
        // Add a placeholder item to show that the control is working but empty
        ui->punishmentcomboBox->addItem("No punishments available");
        ui->punishmentcomboBox->setEnabled(false);
    } else {
        ui->punishmentcomboBox->setEnabled(true);
    }
}

void SelectPunishment::on_buttonBox_accepted()
{
    QString selectedPunishmentName = ui->punishmentcomboBox->currentText();

    if (selectedPunishmentName.isEmpty() || selectedPunishmentName == "No punishments available") {
        QMessageBox::warning(this, "No Punishment Selected", "Please select a punishment to apply.");
        return;
    }

    QString selectedPunishmentKey = "punishment-" + selectedPunishmentName;

    CyberDom *mainApp = qobject_cast<CyberDom*>(parentWidget());
    if (!mainApp) {
        QMessageBox::warning(this, "Error", "Could not access the main application.");
        return;
    }

    // Use case-insensitive lookup to find the punishment section
    bool found = false;
    QMap<QString, QString> punishmentDetails;
    QMap<QString, QMap<QString, QString>>::const_iterator it;
    for (it = iniData.constBegin(); it != iniData.constEnd(); ++it) {
        if (it.key().toLower() == selectedPunishmentKey.toLower()) {
            found = true;
            punishmentDetails = it.value();
            break;
        }
    }

    if (!found) {
        qDebug() << "[WARNING] Punishment details not found in iniData, using default values";

        // Create default punishment details
        punishmentDetails["value"] = "10";
        punishmentDetails["max"] = "20";

        if (selectedPunishmentName.contains("minute")) {
            punishmentDetails["ValueUnit"] = "minute";
        } else if (selectedPunishmentName.contains("hour")) {
            punishmentDetails["ValueUnit"] = "hour";
        } else if (selectedPunishmentName.contains("day")) {
            punishmentDetails["ValueUnit"] = "day";
        }

        // Add to iniData for future use
        iniData[selectedPunishmentKey] = punishmentDetails;
    }

    // Ask for severity if Value and Max are provided
    int severity = 1;
    if (punishmentDetails.contains("value") && punishmentDetails.contains("max")) {
        double value = punishmentDetails.value("value").toDouble();
        if (value <= 0)
            value = 1.0;
        int minAmt = punishmentDetails.value("min", "1").toInt();
        int maxAmt = punishmentDetails.value("max").toInt();

        int minSeverity = qRound(value * minAmt);
        int maxSeverity = qRound(value * maxAmt);

        bool ok = false;
        QString label = tr("Enter Severity (%1 - %2)").arg(minSeverity).arg(maxSeverity);
        severity = QInputDialog::getInt(this,
                                        tr("Punishment Severity"),
                                        label,
                                        minSeverity,
                                        minSeverity,
                                        maxSeverity,
                                        1,
                                        &ok);
        if (!ok)
            return;
    }

    // Apply punishment according to severity and details
    bool hasValue = punishmentDetails.contains("value");
    double value = punishmentDetails.value("value", "1").toDouble();
    QString valueUnit = punishmentDetails.value("ValueUnit").toLower();
    if (!hasValue && valueUnit == "once")
        value = 1.0;
    if (value <= 0)
        value = 1.0;
    int minAmt = punishmentDetails.value("min", "1").toInt();
    int maxAmt = punishmentDetails.contains("max") ? punishmentDetails.value("max").toInt() : 20;
    if (maxAmt <= 0)
        maxAmt = 20;

    int totalUnits;
    if (valueUnit == "once" && !hasValue) {
        totalUnits = qMax(minAmt, 1);
    } else {
        double amt = static_cast<double>(severity) / value;
        totalUnits = qRound(amt);
        if (totalUnits < minAmt)
            totalUnits = minAmt;
    }

    if (valueUnit == "once") {
        mainApp->addPunishmentToAssignments(selectedPunishmentName, totalUnits);
    } else {
        while (totalUnits > 0) {
            int amount = qMin(maxAmt, totalUnits);
            mainApp->addPunishmentToAssignments(selectedPunishmentName, amount);
            totalUnits -= amount;
        }
    }

    // Display Punishment Information
    QString punishmentInfo = "Starting Punishment: " + selectedPunishmentName + "\n";
    for (const auto &key : punishmentDetails.keys()) {
        punishmentInfo += key + ": " + punishmentDetails[key] + "\n";
    }
    punishmentInfo += tr("Severity: %1").arg(severity);
    QMessageBox::information(this, tr("Punishment Details"), punishmentInfo);
}
