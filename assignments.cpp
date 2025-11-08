#include "assignments.h"
#include "ui_assignments.h"
#include "cyberdom.h"
#include "linewriter.h"
#include "scriptparser.h"

#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>
#include <QDebug>
#include <QSet>
#include <QtMath>

Assignments::Assignments(QWidget *parent, CyberDom *app)
    : QMainWindow(parent)
    , ui(new Ui::Assignments)
    , mainApp(app)
{
    ui->setupUi(this);

    connect(ui->table_Assignments, &QTableWidget::itemSelectionChanged,
            this, &Assignments::updateStartButtonState);

    // Initialize settingsFile variable
    settingsFile = mainApp ? mainApp->getSettingsFilePath() : QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/settings.ini";

    populateJobList();

    if (!mainApp) {
        qDebug() << "[ERROR] mainApp is NULL in Assignments!";
    } else {
        qDebug() << "[DEBUG] Assignments constructed";
    }
}

Assignments::~Assignments()
{
    delete ui;
}

void Assignments::populateJobList() {
    ui->table_Assignments->clearContents();
    ui->table_Assignments->setRowCount(0);

    // Configure Columns
    if (ui->table_Assignments->columnCount() == 0) {
        ui->table_Assignments->setColumnCount(4);
        ui->table_Assignments->setHorizontalHeaderLabels({"Deadline", "Assignment", "Estimate", "Type"});
        ui->table_Assignments->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        ui->table_Assignments->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        ui->table_Assignments->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        ui->table_Assignments->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
        ui->table_Assignments->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui->table_Assignments->setEditTriggers(QAbstractItemView::NoEditTriggers);
        ui->table_Assignments->setAlternatingRowColors(true);
        ui->table_Assignments->setSelectionMode(QAbstractItemView::SingleSelection);
        ui->table_Assignments->setFocusPolicy(Qt::StrongFocus);
    }

    if (!mainApp || !mainApp->getScriptParser()) {
        qDebug() << "[ERROR] Main app or script parser is null!";
        return;
    }

    // Get the new structured data
    const ScriptData &data = mainApp->getScriptParser()->getScriptData();
    const QSet<QString> &activeJobs = mainApp->getActiveJobs();
    const QMap<QString, QDateTime> &deadlines = mainApp->getJobDeadlines();

    int row = 0;

    // --- Add Jobs ---
    for (const QString &assignmentName : activeJobs) {
        QString lowerName = assignmentName.toLower();

        // Check if it's a job
        if (data.jobs.contains(lowerName)) {
            const JobDefinition &jobDef = data.jobs.value(lowerName);

            QString deadlineStr;
            if (deadlines.contains(assignmentName)) {
                deadlineStr = deadlines.value(assignmentName).toString("MM-dd-yyyy h:mm AP");
            } else {
                deadlineStr = "No Deadline";
            }

            ui->table_Assignments->insertRow(row);
            ui->table_Assignments->setItem(row, 0, new QTableWidgetItem(deadlineStr));

            QTableWidgetItem *jobItem = new QTableWidgetItem(jobDef.title.isEmpty() ? jobDef.name : jobDef.title);
            jobItem->setData(Qt::UserRole, jobDef.name);
            ui->table_Assignments->setItem(row, 1, jobItem);
            ui->table_Assignments->setItem(row, 2, new QTableWidgetItem("estimateStr"));
            ui->table_Assignments->setItem(row, 3, new QTableWidgetItem("Job"));

            row++;
            qDebug() << "[DEBUG] Adding Job to List: " << jobDef.name;
        }
    }

    // --- Add Punishments ---
    for (const QString &assignmentName : activeJobs) {
        QString lowerName = assignmentName.toLower();

        // Check if this is a punishment
        if (data.punishments.contains(lowerName)) {
            const PunishmentDefinition &punDef = data.punishments.value(lowerName);

            QString deadlineStr;
            if (deadlines.contains(assignmentName)) {
                deadlineStr = deadlines.value(assignmentName).toString("MM-dd-yyyy h:mm AP");
            } else {
                deadlineStr = "No Deadline";
            }

            // Build display name
            QString displayName = punDef.title.isEmpty() ? punDef.name : punDef.title;
            int amt = mainApp->getPunishmentAmount(assignmentName);
            if (displayName.contains('#')) {
                displayName.replace('#', QString::number(amt));
            }
            if (!displayName.isEmpty()) {
                displayName[0] = displayName[0].toUpper();
            }

            ui->table_Assignments->insertRow(row);
            ui->table_Assignments->setItem(row, 0, new QTableWidgetItem(deadlineStr));

            QTableWidgetItem *punItem = new QTableWidgetItem(displayName);
            punItem->setData(Qt::UserRole, punDef.name);
            ui->table_Assignments->setItem(row, 1, punItem);
            ui->table_Assignments->setItem(row, 2, new QTableWidgetItem("Punishment"));

            // Style Punishments
            ui->table_Assignments->item(row, 2)->setBackground(QColor(255, 230, 230));
            ui->table_Assignments->item(row, 2)->setForeground(QColor(180, 0, 0));

            row++;
            qDebug() << "[DEBUG] Added Punishment to List: " << punDef.name;
        }
    }

    qDebug() << "[DEBUG] Displayed Active Jobs and Punishments in UI.";
    updateStartButtonState();
}

void Assignments::on_btn_Start_clicked()
{
    int selectedRow = ui->table_Assignments->currentRow();
    if (selectedRow < 0) {
        QMessageBox::warning(this, "No Selection", "Please select an assignment to start.");
        return;
    }

    // Get assignment details from table
    QTableWidgetItem *startItem = ui->table_Assignments->item(selectedRow, 1);
    QString assignmentName = startItem->data(Qt::UserRole).toString();
    QString assignmentType = ui->table_Assignments->item(selectedRow, 2)->text();
    bool isPunishment = (assignmentType.toLower() == "punishment");

    if (!mainApp || !mainApp->getScriptParser()) {
        QMessageBox::warning(this, "Error", "Application reference lost.");
        return;
    }

    const ScriptData &data = mainApp->getScriptParser()->getScriptData();
    QString lowerName = assignmentName.toLower();

    // Get assignment details from ScriptData
    QString instructions = "No specific instructions available.";
    QString newStatus;

    if (isPunishment) {
        if (!data.punishments.contains(lowerName)) {
            QMessageBox::warning(this, "Error", "Punishment not found in the script");
            return;
        }
        const PunishmentDefinition &punDef = data.punishments.value(lowerName);
        if (!punDef.statusTexts.isEmpty()) {
            instructions = punDef.statusTexts.join("\n");
        }
        newStatus = punDef.newStatus;
    } else {
        if (!data.jobs.contains(lowerName)) {
            QMessageBox::warning(this, "Error", "Job not found in the script.");
            return;
        }
        const JobDefinition &jobDef = data.jobs.value(lowerName);
        if (!jobDef.text.isEmpty()) {
            instructions = jobDef.text;
        }
        newStatus = jobDef.newStatus;
    }

    // Update the assignment notes text box
    ui->text_AssignmentNotes->setText(instructions);

    // Actually start the assignment
    mainApp->startAssignment(assignmentName, isPunishment, newStatus);

    // Show the success message
    QMessageBox::information(this,
                             "Starting " + assignmentType,
                             assignmentType + " " + assignmentName +
                                 "has been started.\n\n" + "Instructions: " + instructions);

    // Launch line writing UI for line punishments
    if (isPunishment) {
        const PunishmentDefinition &punDef = data.punishments.value(lowerName);
        if (punDef.isLineWriting) {
            int count = mainApp->getPunishmentAmount(assignmentName);
            LineWriter dlg(punDef.lines, count, this);
            if (dlg.exec() == QDialog::Accepted)
                mainApp->markAssignmentDone(assignmentName, true);
        }
    }

    // Reselect the same assignment in the refreshed table
    for (int i = 0; i < ui->table_Assignments->rowCount(); i++) {
        QTableWidgetItem *item = ui->table_Assignments->item(i,1);
        if (item->data(Qt::UserRole).toString() == assignmentName) {
            ui->table_Assignments->selectRow(i);
            break;
        }
    }

    updateStartButtonState();
}

void Assignments::on_btn_Done_clicked()
{
    int selectedRow = ui->table_Assignments->currentRow();
    if (selectedRow < 0) {
        QMessageBox::warning(this, "No Selection", "Please select an assignment to mark as done.");
        return;
    }

    // Get assignment details
    QTableWidgetItem *doneItem = ui->table_Assignments->item(selectedRow, 1);
    QString assignmentName = doneItem->data(Qt::UserRole).toString();
    QString assignmentType = ui->table_Assignments->item(selectedRow, 2)->text();
    bool isPunishment = (assignmentType.toLower() == "punishment");

    if (!mainApp || !mainApp->getScriptParser()) {
        QMessageBox::warning(this, "Error", "Application reference lost.");
        return;
    }

    const ScriptData &data = mainApp->getScriptParser()->getScriptData();
    QString lowerName = assignmentName.toLower();
    bool mustStart = false;

    // Check if the assignment exists and get 'mustStart' property
    if (isPunishment) {
        if (!data.punishments.contains(lowerName)) {
            QMessageBox::warning(this, "Error", "Punishment not found in script.");
            return;
        }
        mustStart = data.punishments.value(lowerName).mustStart;
    } else {
        if (!data.jobs.contains(lowerName)) {
            QMessageBox::warning(this, "Error", "Job not found in script");
            return;
        }
        mustStart = data.jobs.value(lowerName).mustStart;
    }

    // Check if the assignment that requires starting was actually started
    if (mustStart) {
        QString startFlagName = (isPunishment ? "punishment_" : "job_") + assignmentName + "_started";
        if (!mainApp->isFlagSet(startFlagName)) {
            QMessageBox::warning(this, "Not Started", "This " + assignmentType.toLower() + " must be started before it can be marked done.");
            return;
        }
    }

    // Confirm with the user
    int result = QMessageBox::question(this, "Confirm", "Are you sure you want to mark this " + assignmentType.toLower() + " as done?", QMessageBox::Yes | QMessageBox::No);

    if (result == QMessageBox::No) {
        return;
    }

    // Call the markAssignmentDone method in CyberDom
    bool completed = mainApp->markAssignmentDone(assignmentName, isPunishment);

    if (!completed) {
        return;
    }

    // Select a new row if available after the refresh
    if (ui->table_Assignments->rowCount() > 0) {
        int newRow = qMin(selectedRow, ui->table_Assignments->rowCount() - 1);
        ui->table_Assignments->selectRow(newRow);
    }

    updateStartButtonState();

    QMessageBox::information(this, assignmentType + " Completed", assignmentType + " " + assignmentName + " has been marked as completed.");
}

void Assignments::on_btn_Abort_clicked()
{
    int selectedRow = ui->table_Assignments->currentRow();
    if (selectedRow < 0) {
        QMessageBox::warning(this, "No Selection", "Please select an assignment to abort.");
        return;
    }

    // Get assignment details
    QTableWidgetItem *abortItem = ui->table_Assignments->item(selectedRow, 1);
    QString assignmentName = abortItem->data(Qt::UserRole).toString();
    QString assignmentType = ui->table_Assignments->item(selectedRow, 2)->text();
    bool isPunishment = (assignmentType.toLower() == "punishment");

    if (!mainApp) {
        QMessageBox::warning(this, "Error", "Application reference lost.");
        return;
    }

    // Check if the assignment was started
    QString startFlagName = (isPunishment ? "punishment_" : "job_") + assignmentName + "_started";
    if (!mainApp->isFlagSet(startFlagName)) {
        QMessageBox::warning(this, "Not Started", "This " + assignmentType.toLower() + " is not currently started.");
        return;
    }

    // Confirm with the user
    int result = QMessageBox::question(this, "Confirm", "Are you sure you want to abort this " + assignmentType.toLower() + "?\n\n" + "It will remain in your assignments list and you'll need to start over.", QMessageBox::Yes | QMessageBox::No);

    if (result == QMessageBox::No) {
        return;
    }

    // Remove the start flag
    mainApp->removeFlag(startFlagName);

    // Return to previous status if needed
    QString statusFlagName = (isPunishment ? "punishment_" : "job_") + assignmentName + "_prev_status";
    QSettings settings(settingsFile, QSettings::IniFormat);
    QString prevStatus = settings.value("Assignments/" + statusFlagName, "").toString();

    if (!prevStatus.isEmpty()) {
        mainApp->updateStatus(prevStatus);
        settings.remove("Assignments/" + statusFlagName);
    }

    QMessageBox::information(this, assignmentType + " Aborted", assignmentType + " " + assignmentName + " has been aborted.");

    // Reselect the same assignment
    for (int i = 0; i < ui->table_Assignments->rowCount(); i++) {
        QTableWidgetItem *item = ui->table_Assignments->item(i,1);
        if (item->data(Qt::UserRole).toString() == assignmentName) {
            ui->table_Assignments->selectRow(i);
            break;
        }
    }

    updateStartButtonState();
}

void Assignments::on_btn_Delete_clicked()
{
    int selectedRow = ui->table_Assignments->currentRow();
    if (selectedRow < 0) {
        QMessageBox::warning(this, "No Selection", "Please select an assignment to delete.");
        return;
    }

    QTableWidgetItem *delItem = ui->table_Assignments->item(selectedRow, 1);
    QString assignmentName = delItem->data(Qt::UserRole).toString();
    QString assignmentType = ui->table_Assignments->item(selectedRow, 2)->text();
    bool isPunishment = (assignmentType.toLower() == "punishment");

    if (!mainApp || !mainApp->getScriptParser()) {
        QMessageBox::warning(this, "Error", "Application reference lost.");
        return;
    }

    const ScriptData &data = mainApp->getScriptParser()->getScriptData();
    QString lowerName = assignmentName.toLower();
    bool deleteAllowed = true; // Default to allowed

    // Check if the assignment allows deletion
    if (isPunishment) {
        if (!data.punishments.contains(lowerName)) {
            QMessageBox::warning(this, "Error", "Punishment not found in script.");
            return;
        }
    } else {
        if (!data.jobs.contains(lowerName)) {
            QMessageBox::warning(this, "Error", "Job not found in script.");
            return;
        }
        deleteAllowed = data.jobs.value(lowerName).deleteAllowed;
    }

    if (!deleteAllowed) {
        QMessageBox::warning(this, "Delete Not Allowed", "This " + assignmentType.toLower() + " cannot be deleted.");
        return;
    }

    int result = QMessageBox::question(this, "Confirm", "Are you sure that you want to delete this " + assignmentType.toLower() + "?", QMessageBox::Yes | QMessageBox::No);

    if (result == (QMessageBox::No)) {
        return;
    }

    // Delegate the remove logic to CyberDom
    mainApp->deleteAssignment(assignmentName, isPunishment);

    QMessageBox::information(this, assignmentType + " Deleted", assignmentType + " " + assignmentName + " has been deleted.");

    // Select a new row if available
    if (ui->table_Assignments->rowCount() > 0) {
        int newRow = qMin(selectedRow, ui->table_Assignments->rowCount() - 1);
        ui->table_Assignments->selectRow(newRow);
    }

    updateStartButtonState();
}

void Assignments::updateStartButtonState()
{
    if (!mainApp) {
        ui->btn_Start->setEnabled(false);
        return;
    }

    int row = ui->table_Assignments->currentRow();
    if (row < 0) {
        ui->btn_Start->setEnabled(false);
        return;
    }

    QTableWidgetItem *item = ui->table_Assignments->item(row, 1);
    QString assignmentName = item->data(Qt::UserRole).toString();

    QString type = ui->table_Assignments->item(row, 2)->text();
    bool isPunishment = (type.toLower() == "punishment");

    QString flag = (isPunishment ? "punishment_" : "job_") + assignmentName + "_started";
    if (mainApp->isFlagSet(flag)) {
        ui->btn_Start->setEnabled(false);
        return;
    }

    QSet<QString> used = mainApp->getResourcesInUse();
    QStringList needed = mainApp->getAssignmentResources(assignmentName, isPunishment);

    bool conflict = false;
    for (const QString &r : needed) {
        if (used.contains(r)) {
            conflict = true;
            break;
        }
    }

    if (conflict || mainApp->hasActiveBlockingPunishment())
        ui->btn_Start->setEnabled(false);
    else
        ui->btn_Start->setEnabled(true);
}
