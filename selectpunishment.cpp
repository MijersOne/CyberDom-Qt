#include "selectpunishment.h"
#include "ui_selectpunishment.h"
#include "cyberdom.h"
#include <QSettings>
#include <QMessageBox>
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
    QMap<QString, QMap<QString, QString>>::const_iterator it;
    for (it = iniData.constBegin(); it != iniData.constEnd(); ++it) {
        if (it.key().toLower() == selectedPunishmentKey.toLower()) {
            found = true;
            
            // Add the punishment to assignments
            mainApp->addPunishmentToAssignments(selectedPunishmentName);

            QMap<QString, QString> punishmentDetails = it.value();

            // Display Punishment Information
            QString punishmentInfo = "Starting Punishment: " + selectedPunishmentName + "\n";
            for (const auto &key : punishmentDetails.keys()) {
                punishmentInfo += key + ": " + punishmentDetails[key] + "\n";
            }
            QMessageBox::information(this, "Punishment Details", punishmentInfo);
            break;
        }
    }
    
    if (!found) {
        qDebug() << "[WARNING] Punishment details not found in iniData, using default values";
        
        // Create default punishment details
        QMap<QString, QString> defaultDetails;
        defaultDetails["value"] = "10";
        defaultDetails["max"] = "20";
        
        if (selectedPunishmentName.contains("minute")) {
            defaultDetails["ValueUnit"] = "minute";
        } else if (selectedPunishmentName.contains("hour")) {
            defaultDetails["ValueUnit"] = "hour";
        } else if (selectedPunishmentName.contains("day")) {
            defaultDetails["ValueUnit"] = "day";
        }
        
        // Add to iniData for future use
        iniData[selectedPunishmentKey] = defaultDetails;
        
        // Add the punishment to assignments
        mainApp->addPunishmentToAssignments(selectedPunishmentName);
        
        // Display Punishment Information with default values
        QString punishmentInfo = "Starting Punishment: " + selectedPunishmentName + "\n";
        for (const auto &key : defaultDetails.keys()) {
            punishmentInfo += key + ": " + defaultDetails[key] + "\n";
        }
        QMessageBox::information(this, "Punishment Details", punishmentInfo);
    }
}
