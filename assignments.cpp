#include "assignments.h"
#include "ui_assignments.h"
#include "cyberdom.h"

#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>

// extern CyberDom *mainApp;

Assignments::Assignments(QWidget *parent, CyberDom *app)
    : QMainWindow(parent)
    , ui(new Ui::Assignments)
    , mainApp(app)
{
    ui->setupUi(this);
    
    // Initialize settingsFile variable
    settingsFile = mainApp ? mainApp->getSettingsFilePath() : QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/settings.ini";
    
    populateJobList(); // Call the function to populate the list when the window is opened.

    // Button connections are handled automatically via Qt's auto-connection mechanism
    // because the slot names follow the on_objectName_signalName pattern

    if (!mainApp) {
        qDebug() << "[ERROR] mainApp is NULL in Assignments!";
    } else {
        connect(mainApp, &CyberDom::jobListUpdated, this, &Assignments::populateJobList);
        qDebug() << "[DEBUG] Connected Assignments to CyberDom's jobListUpdated Signal!";
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

    QSet<QString> jobs = mainApp->getActiveJobs();
    QMap<QString, QDateTime> deadlines = mainApp->getJobDeadlines();

    int row = 0;
    for (const QString &assignmentName : jobs) {
        if (mainApp->iniData.contains("punishment-" + assignmentName)) {
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
        ui->table_Assignments->setItem(row, 1, new QTableWidgetItem(assignmentName));
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
        if (mainApp->iniData.contains("punishment-" + assignmentName)) {
            QString deadlineStr;
            if (deadlines.contains(assignmentName)) {
                deadlineStr = deadlines[assignmentName].toString("MM-dd-yyyy h:mm AP");
            } else {
                deadlineStr = "No Deadline";
            }

            ui->table_Assignments->insertRow(row);
            ui->table_Assignments->setItem(row, 0, new QTableWidgetItem(deadlineStr));
            ui->table_Assignments->setItem(row, 1, new QTableWidgetItem(assignmentName));
            ui->table_Assignments->setItem(row, 2, new QTableWidgetItem("Punishment"));

            ui->table_Assignments->item(row, 2)->setBackground(QColor(255, 230, 230));
            ui->table_Assignments->item(row, 2)->setForeground(QColor(180, 0, 0));

            row++;
            qDebug() << "[DEBUG] Added Punishment to List: " << assignmentName << " with deadline: " << deadlineStr;
        }
    }

    qDebug() << "[DEBUG] Displayed Active Jobs and Punishments in UI.";
}

void Assignments::on_btn_Start_clicked()
{
    int selectedRow = ui->table_Assignments->currentRow();
    if (selectedRow < 0) {
        QMessageBox::warning(this, "No Selection", "Please select an assignment to start.");
        return;
    }

    // Get the assignment details
    QString assignmentName = ui->table_Assignments->item(selectedRow, 1)->text();
    QString assignmentType = ui->table_Assignments->item(selectedRow, 2)->text();
    bool isPunishment = (assignmentType.toLower() == "punishment");

    if (!mainApp) {
        QMessageBox::warning(this, "Error", "Application reference lost.");
        return;
    }

    // Check if the assignment exists in mainApp's data
    QString sectionPrefix = isPunishment ? "punishment-" : "job-";
    QString sectionName = sectionPrefix + assignmentName;
    
    // Debug iniData keys
    QStringList iniKeys = mainApp->iniData.keys();
    qDebug() << "[DEBUG] Checking if job exists in iniData: " << sectionName;
    qDebug() << "[DEBUG] Available sections in iniData: " << iniKeys.join(", ");

    if (!mainApp->iniData.contains(sectionName)) {
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
    QMap<QString, QString> details = mainApp->iniData[sectionName];
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
    QMessageBox::information(this, "Starting " + assignmentType, assignmentType + " " + assignmentName + " has been started.\n\n" + "Instructions: " + instructions);

    // Reselect the same assignment in the refreshed table
    for (int i = 0; i < ui->table_Assignments->rowCount(); i++) {
        if (ui->table_Assignments->item(i, 1)->text() == savedAssignmentName) {
            ui->table_Assignments->selectRow(i);
            break;
        }
    }
}

void Assignments::on_btn_Done_clicked()
{
    int selectedRow = ui->table_Assignments->currentRow();
    if (selectedRow < 0) {
        QMessageBox::warning(this, "No Selection", "Please select an assignment to mark as done.");
        return;
    }

    // Get assignment details
    QString assignmentName = ui->table_Assignments->item(selectedRow, 1)->text();
    QString assignmentType = ui->table_Assignments->item(selectedRow, 2)->text();
    bool isPunishment = (assignmentType.toLower() == "punishment");

    if (!mainApp) {
        QMessageBox::warning(this, "Error", "Application reference lost.");
        return;
    }

    // Check if the assignment exists
    QString sectionPrefix = isPunishment ? "punishment-" : "job-";
    QString sectionName = sectionPrefix + assignmentName;
    
    // Debug iniData keys
    QStringList iniKeys = mainApp->iniData.keys();
    qDebug() << "[DEBUG] Checking if job exists in iniData for 'done': " << sectionName;
    
    if (!mainApp->iniData.contains(sectionName)) {
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

    QMap<QString, QString> details = mainApp->iniData[sectionName];
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
    mainApp->markAssignmentDone(assignmentName, isPunishment);

    // Select a new row if available after the refresh
    if (ui->table_Assignments->rowCount() > 0) {
        int newRow = qMin(selectedRow, ui->table_Assignments->rowCount() - 1);
        ui->table_Assignments->selectRow(newRow);
    }

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
    QString assignmentName = ui->table_Assignments->item(selectedRow, 1)->text();
    QString assignmentType = ui->table_Assignments->item(selectedRow, 2)->text();
    bool isPunishment = (assignmentType.toLower() == "punishment");

    if (!mainApp) {
        QMessageBox::warning(this, "Error", "Application reference lost.");
        return;
    }

    // Check if the assignment exists
    QString sectionPrefix = isPunishment ? "punishment-" : "job-";
    QString sectionName = sectionPrefix + assignmentName;
    
    // Debug iniData keys
    QStringList iniKeys = mainApp->iniData.keys();
    qDebug() << "[DEBUG] Checking if job exists in iniData for 'abort': " << sectionName;
    
    if (!mainApp->iniData.contains(sectionName)) {
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
        if (ui->table_Assignments->item(i, 1)->text() == savedAssignmentName) {
            ui->table_Assignments->selectRow(i);
            break;
        }
    }
}

void Assignments::on_btn_Delete_clicked()
{
    int selectedRow = ui->table_Assignments->currentRow();
    if (selectedRow < 0) {
        QMessageBox::warning(this, "No Selection", "Please select an assignment to delete.");
        return;
    }

    QString assignmentName = ui->table_Assignments->item(selectedRow, 1)->text();
    QString assignmentType = ui->table_Assignments->item(selectedRow, 2)->text();
    bool isPunishment = (assignmentType.toLower() == "punishment");

    if (!mainApp) {
        QMessageBox::warning(this, "Error", "Application reference lost.");
        return;
    }

    QString sectionPrefix = isPunishment ? "punishment-" : "job-";
    QString sectionName = sectionPrefix + assignmentName;
    
    // Debug iniData keys
    QStringList iniKeys = mainApp->iniData.keys();
    qDebug() << "[DEBUG] Checking if job exists in iniData for 'delete': " << sectionName;
    
    if (!mainApp->iniData.contains(sectionName)) {
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

    QMap<QString, QString> details = mainApp->iniData[sectionName];

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

    // Remove the assignment from active assignments
    mainApp->activeAssignments.remove(assignmentName);
    mainApp->removeJobDeadline(assignmentName);

    // Clear any related flags
    QString startFlagName = (isPunishment ? "punishment_" : "job_") + assignmentName + "_started";
    mainApp->removeFlag(startFlagName);

    // Reset status if needed
    QString statusFlagName = (isPunishment ? "punishment_" : "job_") + assignmentName + "_prev_status";
    QSettings settings(settingsFile, QSettings::IniFormat);
    QString prevStatus = settings.value("Assignments/" + statusFlagName, "").toString();

    if (!prevStatus.isEmpty()) {
        mainApp->updateStatus(prevStatus);
        settings.remove("Assignments/" + statusFlagName);
    }

    // Execute delete procedure if specified
    if (details.contains("DeleteProcedure")) {
        // Call the procedure handling if implemented
        // mainApp->executeProcedure(details["DeleteProcedure"]);
    }

    QMessageBox::information(this, assignmentType + " Deleted", assignmentType + " " + assignmentName + " has been deleted.");

    // Select a new row if available after the refresh
    if (ui->table_Assignments->rowCount() > 0) {
        int newRow = qMin(selectedRow, ui->table_Assignments->rowCount() - 1);
        ui->table_Assignments->selectRow(newRow);
    }
}
