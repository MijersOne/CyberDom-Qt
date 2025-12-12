#include "selectpunishment.h"
#include "ui_selectpunishment.h"
#include "cyberdom.h"
#include "scriptparser.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QDebug>

SelectPunishment::SelectPunishment(QWidget *parent, ScriptParser *parser)
    : QDialog(parent)
    , ui(new Ui::SelectPunishment)
    , parser(parser)
{
    ui->setupUi(this);

    // --- FIX: Limit visible items to prevent overflow ---
    ui->punishmentcomboBox->setMaxVisibleItems(15);
    // ---------------------------------------------------

    populatePunishments();
}

SelectPunishment::~SelectPunishment()
{
    delete ui;
}

void SelectPunishment::populatePunishments()
{
    ui->punishmentcomboBox->clear();

    if (!parser) {
        qWarning() << "[SelectPunishment] ScriptParser is null.";
        ui->punishmentcomboBox->addItem("No punishments available");
        ui->punishmentcomboBox->setEnabled(false);
        return;
    }

    const auto &punishments = parser->getScriptData().punishments;
    QStringList names;

    for (auto it = punishments.constBegin(); it != punishments.constEnd(); ++it) {
        names.append(it.key());
    }

    names.sort(Qt::CaseInsensitive);

    if (names.isEmpty()) {
        ui->punishmentcomboBox->addItem("No punishments available");
        ui->punishmentcomboBox->setEnabled(false);
    } else {
        // Add formatted names (Capitalized)
        for (const QString &name : names) {
            QString label = name;
            if (!label.isEmpty()) label[0] = label[0].toUpper();

            // Store the internal key in UserData
            ui->punishmentcomboBox->addItem(label, name);
        }
        ui->punishmentcomboBox->setEnabled(true);
    }
}

void SelectPunishment::on_buttonBox_accepted()
{
    // Get the internal name from UserData
    QString selectedPunishmentName = ui->punishmentcomboBox->currentData().toString();

    if (selectedPunishmentName.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select a punishment.");
        return;
    }

    CyberDom *mainApp = qobject_cast<CyberDom*>(parentWidget());
    if (!mainApp) return;

    const auto &punishments = parser->getScriptData().punishments;
    if (!punishments.contains(selectedPunishmentName.toLower())) {
        QMessageBox::warning(this, "Error", "Punishment definition not found.");
        return;
    }

    const PunishmentDefinition &def = punishments.value(selectedPunishmentName.toLower());

    // --- Calculate Severity / Amount ---
    // This mimics your old logic but uses the clean struct data

    // Calculate Min/Max Severity based on 'value' (severity per unit)
    int minSeverity = qRound(def.value * def.min);
    int maxSeverity = qRound(def.value * def.max);
    if (maxSeverity < minSeverity) maxSeverity = minSeverity;

    bool ok = false;
    QString label = tr("Enter Severity (%1 - %2)").arg(minSeverity).arg(maxSeverity);

    // Default to the minimum severity
    int severity = QInputDialog::getInt(this, tr("Punishment Severity"), label,
                                        minSeverity, minSeverity, maxSeverity, 1, &ok);
    if (!ok) return;

    // Calculate units (repetitions/minutes) based on severity
    int totalUnits;
    QString valueUnit = def.valueUnit.toLower();

    if (valueUnit == "once") {
        totalUnits = qMax(def.min, 1);
    } else {
        double val = (def.value > 0) ? def.value : 1.0;
        double amt = static_cast<double>(severity) / val;
        totalUnits = qRound(amt);
        if (totalUnits < def.min) totalUnits = def.min;
    }

    // Assign the punishment(s)
    if (valueUnit == "once") {
        mainApp->addPunishmentToAssignments(selectedPunishmentName, totalUnits);
    } else {
        int maxPerAssign = (def.max > 0) ? def.max : 20;
        while (totalUnits > 0) {
            int amount = qMin(maxPerAssign, totalUnits);
            mainApp->addPunishmentToAssignments(selectedPunishmentName, amount);
            totalUnits -= amount;
        }
    }

    // Show confirmation
    QString info = QString("Applied: %1\nSeverity: %2\nTotal Units: %3 %4")
                       .arg(def.title.isEmpty() ? def.name : def.title)
                       .arg(severity)
                       .arg(valueUnit == "once" ? 1 : (int)(severity/def.value))
                       .arg(valueUnit);

    QMessageBox::information(this, "Punishment Assigned", info);
}
