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

// extern CyberDom *mainApp;

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
    
    populateJobList(); // Call the function to populate the list when the window is opened.

    // Button connections are handled automatically via Qt's auto-connection mechanism
    // because the slot names follow the on_objectName_signalName pattern

    if (!mainApp) {
        qDebug() << "[ERROR] mainApp is NULL in Assignments!";
    } else {
        // The connection to jobListUpdated is made in CyberDom::openAssignmentsWindow
        qDebug() << "[DEBUG] Assignments constructed";
    }
}

Assignments::~Assignments()
{
    delete ui;
}

void Assignments::populateJobList() {
    ui->table_Assignments->clearContents(); // Clear existing jobs
    ui->table_Assignments->setRowCount(0);

    // Configure columns (do this once, perhaps in constructor
    if (ui->table_Assignments->columnCount() == 0) {
        ui->table_Assignments->setColumnCount(3);
        ui->table_Assignments->setHorizontalHeaderLabels({"Deadline", "Assignment", "Type"});
        ui->table_Assignments->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        ui->table_Assignments->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        ui->table_Assignments->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui->table_Assignments->setEditTriggers(QAbstractItemView::NoEditTriggers); // Make the table non-editable
        ui->table_Assignments->setAlternatingRowColors(true); // Alternate row background colors
        ui->table_Assignments->setSelectionMode(QAbstractItemView::SingleSelection); // Only one row can be selected
        ui->table_Assignments->setFocusPolicy(Qt::StrongFocus); // Better keyboard navigation
    }

    if (!mainApp) {
        qDebug() << "[Error] Main app reference is null!";
        return;
    }

    const QSet<QString> jobs = mainApp->getActiveJobs();
    const QMap<QString, QDateTime> deadlines = mainApp->getJobDeadlines();
    const auto &iniData = mainApp->getIniData();

    int row = 0;
    for (const QString &assignmentName : jobs) {
        if (iniData.contains("punishment-" + assignmentName)) {
            continue;
        }

        QString deadlineStr;
        if (deadlines.contains(assignmentName)) {
            deadlineStr = deadlines[assignmentName].toString("MM-dd-yyyy h:mm AP");
        } else {
            deadlineStr = "No Deadline";
        }

        ui->table_Assignments->insertRow(row);
        ui->table_Assignments->setItem(row, 0, new QTableWidgetItem(deadlineStr));
        QTableWidgetItem *jobItem = new QTableWidgetItem(assignmentName);
        jobItem->setData(Qt::UserRole, assignmentName);
        ui->table_Assignments->setItem(row, 1, jobItem);
        ui->table_Assignments->setItem(row, 2, new QTableWidgetItem("Job"));

        // Style jobs differently from punishments
        ui->table_Assignments->item(row, 2)->setBackground(QColor(230, 255, 230)); // Light green for jobs
        ui->table_Assignments->item(row, 2)->setForeground(QColor(0, 100, 0));

        row++;

        QString displayText = deadlineStr + " - " + assignmentName;
        qDebug() << "[DEBUG] Adding Job to List: " << displayText;
    }

    // Add Punishments
    for (const QString &assignmentName : jobs) {
        // Check if this is a punishment (not a job)
        if (iniData.contains("punishment-" + assignmentName)) {
            QString deadlineStr;
            if (deadlines.contains(assignmentName)) {
                deadlineStr = deadlines[assignmentName].toString("MM-dd-yyyy h:mm AP");
            } else {
                deadlineStr = "No Deadline";
            }

            QString displayName = assignmentName;
            int amt = mainApp->getPunishmentAmount(assignmentName);
            QMap<QString, QString> details = iniData.value("punishment-" + assignmentName);
            if (displayName.contains('#')) {
                // Use the punishment amount directly when replacing '#'
                // The old logic multiplied the base value by 'amt'
                displayName.replace('#', QString::number(amt));
            }
            if (!displayName.isEmpty()) {
                displayName[0] = displayName[0].toUpper();
            }

            ui->table_Assignments->insertRow(row);
            ui->table_Assignments->setItem(row, 0, new QTableWidgetItem(deadlineStr));
            QTableWidgetItem *punItem = new QTableWidgetItem(displayName);
            punItem->setData(Qt::UserRole, assignmentName);
            ui->table_Assignments->setItem(row, 1, punItem);
            ui->table_Assignments->setItem(row, 2, new QTableWidgetItem("Punishment"));

            ui->table_Assignments->item(row, 2)->setBackground(QColor(255, 230, 230));
            ui->table_Assignments->item(row, 2)->setForeground(QColor(180, 0, 0));

            row++;
            qDebug() << "[DEBUG] Added Punishment to List: " << assignmentName << " with deadline: " << deadlineStr;
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

    // Get the assignment details
    QTableWidgetItem *startItem = ui->table_Assignments->item(selectedRow, 1);
    QString assignmentName = startItem->data(Qt::UserRole).toString();
    if (assignmentName.isEmpty())
        assignmentName = startItem->text();
    QString assignmentType = ui->table_Assignments->item(selectedRow, 2)->text();
    bool isPunishment = (assignmentType.toLower() == "punishment");

    if (!mainApp) {
        QMessageBox::warning(this, "Error", "Application reference lost.");
        return;
    }

    const auto &iniData = mainApp->getIniData();

    // Check if the assignment exists in mainApp's data
    QString sectionPrefix = isPunishment ? "punishment-" : "job-";
    QString sectionName = sectionPrefix + assignmentName;
    
    // Debug iniData keys
    const QStringList iniKeys = iniData.keys();
    qDebug() << "[DEBUG] Checking if job exists in iniData: " << sectionName;
    qDebug() << "[DEBUG] Available sections in iniData: " << iniKeys.join(", ");

    if (!iniData.contains(sectionName)) {
        // Try lowercase check to handle case sensitivity issues
        bool found = false;
        for (const QString &key : iniKeys) {
            if (key.toLower() == sectionName.toLower()) {
                sectionName = key; // Use the actual case from the map
                found = true;
                break;
            }
        }
        
        if (!found) {
            QMessageBox::warning(this, "Error", QString("%1 not found in the script.").arg(assignmentType));
            return;
        }
    }

    // Get assignment details
    QMap<QString, QString> details = iniData.value(sectionName);
    QString instructions = details.value("Text", "No specific instructions available.");

    // Update the assignment notes text box
    ui->text_AssignmentNotes->setText(instructions);

    // Check if the assignment has a NewStatus
    QString newStatus;
    if (details.contains("NewStatus")) {
        newStatus = details["NewStatus"];
    }

    // Save the assignment name for reselection after refresh
    QString savedAssignmentName = assignmentName;

    // Actually start the assignment
    mainApp->startAssignment(assignmentName, isPunishment, newStatus);

    // Show the success message
    QMessageBox::information(this,
                             "Starting " + assignmentType,
                             assignmentType + " " + assignmentName +
                                 " has been started.\n\n" + "Instructions: " + instructions);

    // Launch line writing UI for line punishments
    if (isPunishment) {
        ScriptParser *parser = mainApp->getScriptParser();
        if (parser) {
            for (const PunishmentSection &p : parser->getPunishmentSections()) {
                if (p.name.compare(assignmentName, Qt::CaseInsensitive) == 0 &&
                    p.isLineWriting) {
                    int count = mainApp->getPunishmentAmount(assignmentName);
                    LineWriter dlg(p.lines, count, this);
                    if (dlg.exec() == QDialog::Accepted)
                        mainApp->markAssignmentDone(assignmentName, true);
                    break;
                }
            }
        }
    }

    // Reselect the same assignment in the refreshed table
    for (int i = 0; i < ui->table_Assignments->rowCount(); i++) {
        QTableWidgetItem *item = ui->table_Assignments->item(i,1);
        QString base = item->data(Qt::UserRole).toString();
        if (base.isEmpty())
            base = item->text();
        if (base == savedAssignmentName) {
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
    if (assignmentName.isEmpty())
        assignmentName = doneItem->text();
    QString assignmentType = ui->table_Assignments->item(selectedRow, 2)->text();
    bool isPunishment = (assignmentType.toLower() == "punishment");

    if (!mainApp) {
        QMessageBox::warning(this, "Error", "Application reference lost.");
        return;
    }
    const auto &iniData = mainApp->getIniData();

    // Check if the assignment exists
    QString sectionPrefix = isPunishment ? "punishment-" : "job-";
    QString sectionName = sectionPrefix + assignmentName;
    
    // Debug iniData keys
    const QStringList iniKeys = iniData.keys();
    qDebug() << "[DEBUG] Checking if job exists in iniData for 'done': " << sectionName;
    
    if (!iniData.contains(sectionName)) {
        // Try lowercase check to handle case sensitivity issues
        bool found = false;
        for (const QString &key : iniKeys) {
            if (key.toLower() == sectionName.toLower()) {
                sectionName = key; // Use the actual case from the map
                found = true;
                break;
            }
        }
        
        if (!found) {
            QMessageBox::warning(this, "Error", QString("%1 not found in the script.").arg(assignmentType));
            return;
        }
    }

    QMap<QString, QString> details = iniData.value(sectionName);
    bool mustStart = details.contains("muststart") && details["muststart"] == "1";

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
        return; // user finished too quickly
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
    if (assignmentName.isEmpty())
        assignmentName = abortItem->text();
    QString assignmentType = ui->table_Assignments->item(selectedRow, 2)->text();
    bool isPunishment = (assignmentType.toLower() == "punishment");

    if (!mainApp) {
        QMessageBox::warning(this, "Error", "Application reference lost.");
        return;
    }
    const auto &iniData = mainApp->getIniData();

    // Check if the assignment exists
    QString sectionPrefix = isPunishment ? "punishment-" : "job-";
    QString sectionName = sectionPrefix + assignmentName;
    
    // Debug iniData keys
    const QStringList iniKeys = iniData.keys();
    qDebug() << "[DEBUG] Checking if job exists in iniData for 'abort': " << sectionName;
    
    if (!iniData.contains(sectionName)) {
        // Try lowercase check to handle case sensitivity issues
        bool found = false;
        for (const QString &key : iniKeys) {
            if (key.toLower() == sectionName.toLower()) {
                sectionName = key; // Use the actual case from the map
                found = true;
                break;
            }
        }
        
        if (!found) {
            QMessageBox::warning(this, "Error", QString("%1 not found in the script.").arg(assignmentType));
            return;
        }
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

    // Save the assignment name for reselection after refresh
    QString savedAssignmentName = assignmentName;

    // Placeholder to call the abortAssignment method
    // mainApp->abortAssignment(assignmentName, isPunishment);

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

    // Reselect the same assignment in the refreshed table
    for (int i = 0; i < ui->table_Assignments->rowCount(); i++) {
        QTableWidgetItem *item = ui->table_Assignments->item(i,1);
        QString base = item->data(Qt::UserRole).toString();
        if (base.isEmpty())
            base = item->text();
        if (base == savedAssignmentName) {
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
    if (assignmentName.isEmpty())
        assignmentName = delItem->text();
    QString assignmentType = ui->table_Assignments->item(selectedRow, 2)->text();
    bool isPunishment = (assignmentType.toLower() == "punishment");

    if (!mainApp) {
        QMessageBox::warning(this, "Error", "Application reference lost.");
        return;
    }
    const auto &iniData = mainApp->getIniData();

    QString sectionPrefix = isPunishment ? "punishment-" : "job-";
    QString sectionName = sectionPrefix + assignmentName;
    
    // Debug iniData keys
    const QStringList iniKeys = iniData.keys();
    qDebug() << "[DEBUG] Checking if job exists in iniData for 'delete': " << sectionName;
    
    if (!iniData.contains(sectionName)) {
        // Try lowercase check to handle case sensitivity issues
        bool found = false;
        for (const QString &key : iniKeys) {
            if (key.toLower() == sectionName.toLower()) {
                sectionName = key; // Use the actual case from the map
                found = true;
                break;
            }
        }
        
        if (!found) {
            QMessageBox::warning(this, "Error", QString("%1 not found in script.").arg(assignmentType));
            return;
        }
    }

    QMap<QString, QString> details = iniData.value(sectionName);

    // Check if the assignment allows deletion
    bool deleteAllowed = details.contains("DeleteAllowed") && details["DeleteAllowed"] == "1";
    
    // If not explicitly allowed, we'll still allow deletion if it's not explicitly disallowed
    if (!deleteAllowed && (!details.contains("DeleteAllowed") || details["DeleteAllowed"] != "0")) {
        deleteAllowed = true;
    }

    if (!deleteAllowed) {
        QMessageBox::warning(this, "Delete Not Allowed", "This " + assignmentType.toLower() + " cannot be deleted.");
        return;
    }

    int result = QMessageBox::question(this, "Confirm", "Are you sure you want to delete this " + assignmentType.toLower() + "?", QMessageBox::Yes | QMessageBox::No);

    if (result == QMessageBox::No) {
        return;
    }

    // Delegate the removal logic to CyberDom so the jobListUpdated signal is emitted
    mainApp->deleteAssignment(assignmentName, isPunishment);

    QMessageBox::information(this, assignmentType + " Deleted", assignmentType + " " + assignmentName + " has been deleted.");

    // Select a new row if available after the refresh
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
    if (assignmentName.isEmpty())
        assignmentName = item->text();

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
