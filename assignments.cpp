#include "assignments.h"
#include "ui_assignments.h"
#include "cyberdom.h"
#include "linewriter.h"
#include "scriptparser.h"
#include "ScriptUtils.h"
#include "detention.h"

#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>
#include <QDebug>
#include <QSet>
#include <QtMath>
#include <QRandomGenerator>
#include <QTime>

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

    for (const QString &assignmentName : activeJobs) {
        QString lowerName = assignmentName.toLower();

        // --- 1. Check for Punishment (Handle suffixes like _2) ---
        QString baseName = lowerName;
        int lastUnderscore = baseName.lastIndexOf('_');
        if (lastUnderscore != -1) {
            bool isNumber;
            baseName.mid(lastUnderscore + 1).toInt(&isNumber);
            if (isNumber) baseName = baseName.left(lastUnderscore);
        }

        if (data.punishments.contains(baseName)) {
            // It is a punishment
            const PunishmentDefinition &punDef = data.punishments.value(baseName);

            QString deadlineStr = "No Deadline";
            if (deadlines.contains(assignmentName)) {
                deadlineStr = deadlines.value(assignmentName).toString("MM-dd-yyyy h:mm AP");
            }

            // Build display name
            QString displayText = punDef.title.isEmpty() ? punDef.name : punDef.title;
            int amt = mainApp->getPunishmentAmount(assignmentName);
            if (displayText.contains('#')) {
                displayText.replace('#', QString::number(amt));
            }
            if (!displayText.isEmpty()) displayText[0] = displayText[0].toUpper();

            // Handle Line Writing Select=Random Display
            if (punDef.isLineWriting && punDef.lineSelectMode.compare("Random", Qt::CaseInsensitive) == 0) {
                QSettings settings(mainApp->getSettingsFilePath(), QSettings::IniFormat);
                QString line = settings.value("Assignments/" + assignmentName + "_selected_line").toString();
                if (!line.isEmpty()) {
                    // Format: "Write 5 times: I must obey"
                    displayText = QString("Write %1 times: %2").arg(amt).arg(line);
                }
            }

            ui->table_Assignments->insertRow(row);
            ui->table_Assignments->setItem(row, 0, new QTableWidgetItem(deadlineStr));

            QTableWidgetItem *punItem = new QTableWidgetItem(displayText);
            punItem->setData(Qt::UserRole, assignmentName); // Store the FULL instance name
            ui->table_Assignments->setItem(row, 1, punItem);

            QString estimateStr = mainApp->getAssignmentEstimate(assignmentName, true);
            ui->table_Assignments->setItem(row, 2, new QTableWidgetItem(estimateStr));

            // Type is in column 3
            QTableWidgetItem *typeItem = new QTableWidgetItem("Punishment");
            typeItem->setBackground(QColor(255, 230, 230));
            typeItem->setForeground(QColor(180, 0, 0));
            ui->table_Assignments->setItem(row, 3, typeItem);

            row++;
            continue; // Done with this assignment
        }

        // --- 2. Check for Job ---
        if (data.jobs.contains(lowerName)) {
            const JobDefinition &jobDef = data.jobs.value(lowerName);

            QString deadlineStr = "No Deadline";
            if (deadlines.contains(assignmentName)) {
                deadlineStr = deadlines.value(assignmentName).toString("MM-dd-yyyy h:mm AP");
            }

            QString displayText = jobDef.title.isEmpty() ? jobDef.name : jobDef.title;

            // Handle Line Writing Select=Random Display
            if (jobDef.isLineWriting && jobDef.lineSelectMode.compare("Random", Qt::CaseInsensitive) == 0) {
                QSettings settings(mainApp->getSettingsFilePath(), QSettings::IniFormat);
                QString line = settings.value("Assignments/" + assignmentName + "_selected_line").toString();
                if (!line.isEmpty()) {
                    int amt = jobDef.lineCount > 0 ? jobDef.lineCount : 1;
                    displayText = QString("Write %1 times: %2").arg(amt).arg(line);
                }
            }

            ui->table_Assignments->insertRow(row);
            ui->table_Assignments->setItem(row, 0, new QTableWidgetItem(deadlineStr));

            QTableWidgetItem *jobItem = new QTableWidgetItem(jobDef.title.isEmpty() ? jobDef.name : jobDef.title);
            jobItem->setData(Qt::UserRole, assignmentName); // Store the FULL instance name
            ui->table_Assignments->setItem(row, 1, jobItem);

            QString estimateStr = mainApp->getAssignmentEstimate(assignmentName, false);
            ui->table_Assignments->setItem(row, 2, new QTableWidgetItem(estimateStr));

            // Type is in column 3
            QTableWidgetItem *typeItem = new QTableWidgetItem("Job");
            typeItem->setBackground(QColor(230, 255, 230));
            typeItem->setForeground(QColor(0, 100, 0));
            ui->table_Assignments->setItem(row, 3, typeItem);

            row++;
        }
    }

    updateStartButtonState();
}

// In assignments.cpp (Add this include at the top)
#include <QTime>

// ...

void Assignments::on_btn_Start_clicked()
{
    int selectedRow = ui->table_Assignments->currentRow();
    if (selectedRow < 0) {
        QMessageBox::warning(this, "No Selection", "Please select an assignment to start.");
        return;
    }

    QTableWidgetItem *startItem = ui->table_Assignments->item(selectedRow, 1);
    QString assignmentName = startItem->data(Qt::UserRole).toString();

    QString assignmentType = ui->table_Assignments->item(selectedRow, 3)->text();
    bool isPunishment = (assignmentType.toLower() == "punishment");

    if (!mainApp || !mainApp->getScriptParser()) return;

    const ScriptData &data = mainApp->getScriptParser()->getScriptData();
    QString lowerName = assignmentName.toLower();
    QString baseName = lowerName;

    // Resolve base name (handle suffixes)
    if (isPunishment) {
        int lastUnderscore = baseName.lastIndexOf('_');
        if (lastUnderscore != -1) {
            bool isNumber;
            baseName.mid(lastUnderscore + 1).toInt(&isNumber);
            if (isNumber) baseName = baseName.left(lastUnderscore);
        }
    }

    QString instructions = "No specific instructions available.";
    QString newStatus;
    bool isLineWriting = false;
    QStringList lines;
    int lineCount = 0;
    QString lineSelectMode;

    if (isPunishment) {
        if (!data.punishments.contains(baseName)) {
            QMessageBox::warning(this, "Error", "Punishment definition not found for: " + baseName);
            return;
        }
        const PunishmentDefinition &def = data.punishments.value(baseName);
        if (!def.statusTexts.isEmpty()) instructions = def.statusTexts.join("\n");
        newStatus = def.newStatus;
        isLineWriting = def.isLineWriting;
        lines = def.lines;
        lineSelectMode = def.lineSelectMode;
        lineCount = mainApp->getPunishmentAmount(assignmentName);
    } else {
        if (!data.jobs.contains(lowerName)) {
            QMessageBox::warning(this, "Error", "Job definition not found for: " + lowerName);
            return;
        }
        const JobDefinition &def = data.jobs.value(lowerName);
        if (!def.statusTexts.isEmpty()) instructions = def.statusTexts.join("\n");
        if (!def.text.isEmpty()) instructions = def.text + "\n" + instructions;
        newStatus = def.newStatus;
        isLineWriting = def.isLineWriting;
        lines = def.lines;
        lineSelectMode = def.lineSelectMode;
        lineCount = def.lineCount > 0 ? def.lineCount : 1;
    }

    ui->text_AssignmentNotes->setText(instructions);

    // --- START ASSIGNMENT ---
    if (!mainApp->startAssignment(assignmentName, isPunishment, newStatus)) {
        return;
    }

    // --- LAUNCH LINE WRITER ---
    if (isLineWriting) {
        if (lines.isEmpty()) {
            QMessageBox::warning(this, "Error", "This assignment is Type=Lines but has no 'Line=' entries.");
            mainApp->abortAssignment(assignmentName, isPunishment);
        } else {
            QStringList fullExpectedList;
            QString mode = lineSelectMode.isEmpty() ? "All" : lineSelectMode;

            if (mode.compare("Sequence", Qt::CaseInsensitive) == 0) {
                for (int i = 0; i < lineCount; ++i) fullExpectedList.append(lines);
            }
            else if (mode.compare("Random", Qt::CaseInsensitive) == 0) {
                QSettings settings(mainApp->getSettingsFilePath(), QSettings::IniFormat);
                QString selectedLine = settings.value("Assignments/" + assignmentName + "_selected_line").toString();
                if (selectedLine.isEmpty()) selectedLine = lines[0];
                for (int i = 0; i < lineCount; ++i) fullExpectedList.append(selectedLine);
            }
            else {
                for (int i = 0; i < lineCount; ++i) {
                    int idx = ScriptUtils::randomInRange(0, lines.size() - 1, false);
                    fullExpectedList.append(lines[idx]);
                }
            }

            for (QString &s : fullExpectedList) s = mainApp->replaceVariables(s);

            // Get Settings
            int maxBreakSecs = 0;
            QString alarmSound;
            const GeneralSettings &gen = mainApp->getScriptParser()->getScriptData().general;
            alarmSound = gen.popupAlarm;
            QString breakStr = gen.maxLineBreak;
            if (!breakStr.isEmpty()) {
                QTime t = QTime::fromString(breakStr, "h:mm:ss");
                if (t.isValid()) {
                    maxBreakSecs = t.hour() * 3600 + t.minute() * 60 + t.second();
                } else {
                    t = QTime::fromString(breakStr, "mm:ss");
                    if (t.isValid()) maxBreakSecs = t.minute() * 60 + t.second();
                }
            }

            LineWriter dlg(fullExpectedList, maxBreakSecs, alarmSound, this);
            if (dlg.exec() == QDialog::Accepted) {
                mainApp->markAssignmentDone(assignmentName, isPunishment);
            } else {
                mainApp->abortAssignment(assignmentName, isPunishment);
            }
        }
    }
    // --- LAUNCH DETENTION ---
    else if (isPunishment && data.punishments.contains(baseName) &&
             data.punishments.value(baseName).isDetention) {

        const PunishmentDefinition &def = data.punishments.value(baseName);
        int amount = mainApp->getPunishmentAmount(assignmentName);

        // --- FIX: Always calculate as minutes for Detention ---
        int totalSeconds = amount * 60;
        // ----------------------------------------------------

        QString text = def.detentionText.join("\n");
        if (text.isEmpty()) text = "You are in detention.";
        text = mainApp->replaceVariables(text);

        Detention dlg(totalSeconds, text, this);
        if (dlg.exec() == QDialog::Accepted) {
            mainApp->markAssignmentDone(assignmentName, isPunishment);
        } else {
            mainApp->abortAssignment(assignmentName, isPunishment);
        }
    }

    updateStartButtonState();
}

void Assignments::on_btn_Done_clicked()
{
    int selectedRow = ui->table_Assignments->currentRow();
    if (selectedRow < 0) {
        QMessageBox::warning(this, "No Selection", "Please select an assignment.");
        return;
    }

    QTableWidgetItem *doneItem = ui->table_Assignments->item(selectedRow, 1);
    QString assignmentName = doneItem->data(Qt::UserRole).toString();

    // FIX: Use Column 3 for Type
    QString assignmentType = ui->table_Assignments->item(selectedRow, 3)->text();
    bool isPunishment = (assignmentType.toLower() == "punishment");

    if (!mainApp || !mainApp->getScriptParser()) return;

    const ScriptData &data = mainApp->getScriptParser()->getScriptData();
    QString baseName = assignmentName.toLower();

    if (isPunishment) {
        int lastUnderscore = baseName.lastIndexOf('_');
        if (lastUnderscore != -1) {
            bool isNumber;
            baseName.mid(lastUnderscore + 1).toInt(&isNumber);
            if (isNumber) baseName = baseName.left(lastUnderscore);
        }
    }

    bool mustStart = false;

    if (isPunishment) {
        if (!data.punishments.contains(baseName)) return;
        const PunishmentDefinition &def = data.punishments.value(baseName);
        QString vu = def.valueUnit.toLower();
        bool isTimeBased = (vu == "day" || vu == "hour" || vu == "minute");
        mustStart = def.mustStart || def.longRunning || isTimeBased;
    } else {
        if (!data.jobs.contains(baseName)) return;
        const JobDefinition &def = data.jobs.value(baseName);
        mustStart = def.mustStart || def.longRunning;
    }

    if (mustStart) {
        QString startFlagName = (isPunishment ? "punishment_" : "job_") + assignmentName + "_started";
        if (!mainApp->isFlagSet(startFlagName)) {
            QMessageBox::warning(this, "Not Started", "This " + assignmentType.toLower() + " must be started before it can be marked done.");
            return;
        }
    }

    int result = QMessageBox::question(this, "Confirm", "Are you sure you want to mark this " + assignmentType.toLower() + " as done?", QMessageBox::Yes | QMessageBox::No);
    if (result == QMessageBox::No) return;

    bool completed = mainApp->markAssignmentDone(assignmentName, isPunishment);
    if (!completed) return;

    updateStartButtonState();
    QMessageBox::information(this, assignmentType + " Completed", assignmentType + " " + assignmentName + " has been marked as completed.");
}

void Assignments::on_btn_Abort_clicked()
{
    int selectedRow = ui->table_Assignments->currentRow();
    if (selectedRow < 0) {
        QMessageBox::warning(this, "No Selection", "Please select an assignment.");
        return;
    }

    QTableWidgetItem *abortItem = ui->table_Assignments->item(selectedRow, 1);
    QString assignmentName = abortItem->data(Qt::UserRole).toString();

    // FIX: Use Column 3 for Type
    QString assignmentType = ui->table_Assignments->item(selectedRow, 3)->text();
    bool isPunishment = (assignmentType.toLower() == "punishment");

    QString startFlagName = (isPunishment ? "punishment_" : "job_") + assignmentName + "_started";
    if (!mainApp->isFlagSet(startFlagName)) {
        QMessageBox::warning(this, "Not Started", "This " + assignmentType.toLower() + " is not currently started.");
        return;
    }

    int result = QMessageBox::question(this, "Confirm", "Are you sure you want to abort this " + assignmentType.toLower() + "?", QMessageBox::Yes | QMessageBox::No);
    if (result == QMessageBox::No) return;

    mainApp->abortAssignment(assignmentName, isPunishment);
    QMessageBox::information(this, assignmentType + " Aborted", assignmentType + " " + assignmentName + " has been aborted.");
    updateStartButtonState();
}

void Assignments::on_btn_Delete_clicked()
{
    int selectedRow = ui->table_Assignments->currentRow();
    if (selectedRow < 0) {
        QMessageBox::warning(this, "No Selection", "Please select an assignment.");
        return;
    }

    QTableWidgetItem *delItem = ui->table_Assignments->item(selectedRow, 1);
    QString assignmentName = delItem->data(Qt::UserRole).toString();

    // FIX: Use Column 3 for Type
    QString assignmentType = ui->table_Assignments->item(selectedRow, 3)->text();
    bool isPunishment = (assignmentType.toLower() == "punishment");

    if (!mainApp || !mainApp->getScriptParser()) return;

    const ScriptData &data = mainApp->getScriptParser()->getScriptData();
    QString baseName = assignmentName.toLower();

    if (isPunishment) {
        int lastUnderscore = baseName.lastIndexOf('_');
        if (lastUnderscore != -1) {
            bool isNumber;
            baseName.mid(lastUnderscore + 1).toInt(&isNumber);
            if (isNumber) baseName = baseName.left(lastUnderscore);
        }
    }

    bool deleteAllowed = false;
    QString beforeDeleteProcedure;

    if (isPunishment) {
        if (data.punishments.contains(baseName)) {
            const auto& def = data.punishments.value(baseName);
            deleteAllowed = def.deleteAllowed;
            beforeDeleteProcedure = def.beforeDeleteProcedure;
        }
    } else {
        if (data.jobs.contains(baseName)) {
            const auto& def = data.jobs.value(baseName);
            deleteAllowed = def.deleteAllowed;
            beforeDeleteProcedure = def.beforeDeleteProcedure;
        }
    }

    if (!deleteAllowed) {
        QMessageBox::warning(this, "Delete Not Allowed", "This " + assignmentType.toLower() + " cannot be deleted.");
        return;
    }

    if (!beforeDeleteProcedure.isEmpty()) {
        mainApp->runProcedure(beforeDeleteProcedure);
        if (mainApp->isFlagSet("zzDeny")) {
            mainApp->removeFlag("zzDeny");
            QMessageBox::information(this, "Delete Denied", "The request to delete " + assignmentName + " was denied.");
            return;
        }
    }

    if (QMessageBox::question(this, "Confirm", "Are you sure you want to delete this " + assignmentType.toLower() + "?", QMessageBox::Yes | QMessageBox::No) == QMessageBox::No) {
        return;
    }

    mainApp->deleteAssignment(assignmentName, isPunishment);
    QMessageBox::information(this, assignmentType + " Deleted", assignmentType + " " + assignmentName + " has been deleted.");
    updateStartButtonState();
}

void Assignments::updateStartButtonState()
{
    if (!mainApp) return;

    int row = ui->table_Assignments->currentRow();
    if (row < 0) {
        ui->btn_Start->setEnabled(false);
        ui->btn_Done->setEnabled(false);
        return;
    }

    QTableWidgetItem *item = ui->table_Assignments->item(row, 1);
    QString assignmentName = item->data(Qt::UserRole).toString();

    // FIX: Use Column 3 for Type
    QString type = ui->table_Assignments->item(row, 3)->text();
    bool isPunishment = (type.toLower() == "punishment");

    QString flag = (isPunishment ? "punishment_" : "job_") + assignmentName + "_started";
    bool isStarted = mainApp->isFlagSet(flag);

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
        ui->btn_Start->setEnabled(!isStarted);

    bool mustStart = false;
    const ScriptData &data = mainApp->getScriptParser()->getScriptData();
    QString baseName = assignmentName.toLower();

    if (isPunishment) {
        int lastUnderscore = baseName.lastIndexOf('_');
        if (lastUnderscore != -1) {
            bool isNumber;
            baseName.mid(lastUnderscore + 1).toInt(&isNumber);
            if (isNumber) baseName = baseName.left(lastUnderscore);
        }
        if (data.punishments.contains(baseName)) {
            const PunishmentDefinition &def = data.punishments.value(baseName);
            QString vu = def.valueUnit.toLower();
            bool isTimeBased = (vu == "day" || vu == "hour" || vu == "minute");
            mustStart = def.mustStart || def.longRunning || isTimeBased;
        }
    } else {
        if (data.jobs.contains(baseName)) {
            const JobDefinition &def = data.jobs.value(baseName);
            mustStart = def.mustStart || def.longRunning;
        }
    }

    if (isStarted) {
        ui->btn_Done->setEnabled(true);
    } else {
        ui->btn_Done->setEnabled(!mustStart);
    }
}
