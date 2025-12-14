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
#include <QHBoxLayout>
#include <QPushButton>
#include <QHeaderView>

Assignments::Assignments(QWidget *parent, CyberDom *app)
    : QMainWindow(parent)
    , ui(new Ui::Assignments)
    , mainApp(app)
{
    ui->setupUi(this);

    // Initialize settingsFile variable
    settingsFile = mainApp ? mainApp->getSettingsFilePath() : QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/settings.ini";

    // --- 1. Setup the new Filter UI Programmatically ---
    setupFilterUi();

    // --- 2. Configure Table for Sorting ---
    ui->table_Assignments->setSortingEnabled(true);

    connect(ui->table_Assignments, &QTableWidget::itemSelectionChanged,
            this, &Assignments::updateStartButtonState);

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

// --- NEW: Build the Filter UI ---
void Assignments::setupFilterUi()
{
    // Find the central layout (assuming Vertical Layout in main window)
    // If your .ui structure is complex, we might need to insert this into a specific layout.
    // For a QMainWindow, we can add a ToolBar or insert into the central widget's layout.

    QWidget *central = ui->centralwidget;
    QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout*>(central->layout());

    if (!mainLayout) {
        // If layout isn't vertical, creating a wrapper might be risky without seeing .ui
        // Attempt to insert at top
        mainLayout = new QVBoxLayout(central);
    }

    filterContainer = new QWidget(this);
    QHBoxLayout *filterLayout = new QHBoxLayout(filterContainer);
    filterLayout->setContentsMargins(0, 0, 0, 10);

    QLabel *lbl = new QLabel("Filter Deadlines:", this);
    filterLayout->addWidget(lbl);

    filterCombo = new QComboBox(this);
    filterCombo->addItem("Show All");
    filterCombo->addItem("Before Date...");
    filterCombo->addItem("After Date...");
    filterCombo->addItem("Between Dates...");
    connect(filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &Assignments::onFilterModeChanged);
    filterLayout->addWidget(filterCombo);

    dateEditA = new QDateEdit(QDate::currentDate(), this);
    dateEditA->setCalendarPopup(true);
    dateEditA->setVisible(false); // Hidden by default
    filterLayout->addWidget(dateEditA);

    labelTo = new QLabel("and", this);
    labelTo->setVisible(false);
    filterLayout->addWidget(labelTo);

    dateEditB = new QDateEdit(QDate::currentDate().addDays(7), this);
    dateEditB->setCalendarPopup(true);
    dateEditB->setVisible(false);
    filterLayout->addWidget(dateEditB);

    QPushButton *btnApply = new QPushButton("Apply", this);
    connect(btnApply, &QPushButton::clicked, this, &Assignments::applyFilter);
    filterLayout->addWidget(btnApply);

    QPushButton *btnReset = new QPushButton("Reset", this);
    connect(btnReset, &QPushButton::clicked, this, &Assignments::resetFilter);
    filterLayout->addWidget(btnReset);

    filterLayout->addStretch(); // Push everything to the left

    // Insert at index 0 (Top of window)
    mainLayout->insertWidget(0, filterContainer);
}

void Assignments::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    // Reset filters whenever the window is opened/shown
    resetFilter();
}

void Assignments::onFilterModeChanged(int index)
{
    // index 0: All (Hide dates)
    // index 1: Before (Show A)
    // index 2: After (Show A)
    // index 3: Between (Show A, Label, B)

    dateEditA->setVisible(index > 0);
    dateEditB->setVisible(index == 3);
    labelTo->setVisible(index == 3);
}

void Assignments::resetFilter()
{
    filterCombo->setCurrentIndex(0); // "Show All"
    dateEditA->setDate(QDate::currentDate());
    dateEditB->setDate(QDate::currentDate().addDays(7));
    applyFilter(); // Re-show all rows
}

void Assignments::applyFilter()
{
    int mode = filterCombo->currentIndex();
    QDate dateA = dateEditA->date();
    QDate dateB = dateEditB->date();

    // Iterate all rows
    for(int i = 0; i < ui->table_Assignments->rowCount(); ++i) {
        bool showRow = true;

        if (mode > 0) {
            // Get the custom Item from column 0
            DateTableWidgetItem *item = dynamic_cast<DateTableWidgetItem*>(ui->table_Assignments->item(i, 0));

            // If item is null or deadline is invalid (No Deadline)
            // Strategy: "No Deadline" items usually remain visible unless strict filtering is desired.
            // Let's hide them if filtering by specific dates to be strict.
            if (!item || item->text() == "No Deadline") {
                showRow = false;
            } else {
                // We can't access private 'dt', but we can check the text or rely on the fact
                // that we *know* it's a DateTableWidgetItem.
                // Wait, DateTableWidgetItem members are private.
                // FIX: Let's parse the text since it's formatted standardly, OR make 'dt' public/accessible.
                // Parsing text "MM-dd-yyyy..." is safe here.

                // Actually, simplest is to just parse the text in the cell
                QString dateStr = item->text();
                // Format used in populate: "MM-dd-yyyy h:mm AP"
                QDateTime rowDt = QDateTime::fromString(dateStr, "MM-dd-yyyy h:mm AP");

                if (rowDt.isValid()) {
                    QDate rowDate = rowDt.date();

                    if (mode == 1) { // Before Date A
                        if (rowDate >= dateA) showRow = false;
                    }
                    else if (mode == 2) { // After Date A
                        if (rowDate <= dateA) showRow = false;
                    }
                    else if (mode == 3) { // Between A and B
                        if (rowDate < dateA || rowDate > dateB) showRow = false;
                    }
                }
            }
        }

        ui->table_Assignments->setRowHidden(i, !showRow);
    }
}

void Assignments::populateJobList() {
    // Disable sorting while inserting to prevent jumping
    ui->table_Assignments->setSortingEnabled(false);

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
        ui->table_Assignments->setSortingEnabled(true);
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
            QDateTime deadlineDt;

            if (deadlines.contains(assignmentName)) {
                deadlineDt = deadlines.value(assignmentName);
                deadlineStr = deadlineDt.toString("MM-dd-yyyy h:mm AP");
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
                    displayText = QString("Write %1 times: %2").arg(amt).arg(line);
                }
            }

            // --- Visual Indicator ---
            QString startFlag = "punishment_" + assignmentName + "_started";
            if (mainApp->isFlagSet(startFlag)) {
                displayText = QStringLiteral("💠 ") + displayText;
            }

            ui->table_Assignments->insertRow(row);

            // --- FIX: Use Custom Date Item for Sorting ---
            ui->table_Assignments->setItem(row, 0, new DateTableWidgetItem(deadlineStr, deadlineDt));
            // ---------------------------------------------

            QTableWidgetItem *punItem = new QTableWidgetItem(displayText);
            punItem->setData(Qt::UserRole, assignmentName);
            ui->table_Assignments->setItem(row, 1, punItem);

            QString estimateStr = mainApp->getAssignmentEstimate(assignmentName, true);
            ui->table_Assignments->setItem(row, 2, new QTableWidgetItem(estimateStr));

            QTableWidgetItem *typeItem = new QTableWidgetItem("Punishment");
            typeItem->setBackground(QColor(255, 230, 230));
            typeItem->setForeground(QColor(180, 0, 0));
            ui->table_Assignments->setItem(row, 3, typeItem);

            row++;
            continue;
        }

        // --- 2. Check for Job ---
        if (data.jobs.contains(lowerName)) {
            const JobDefinition &jobDef = data.jobs.value(lowerName);

            QString deadlineStr = "No Deadline";
            QDateTime deadlineDt;

            if (deadlines.contains(assignmentName)) {
                deadlineDt = deadlines.value(assignmentName);
                deadlineStr = deadlineDt.toString("MM-dd-yyyy h:mm AP");
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

            // --- Visual Indicator ---
            QString startFlag = "job_" + assignmentName + "_started";
            if (mainApp->isFlagSet(startFlag)) {
                displayText = QStringLiteral("💠 ") + displayText;
            }

            ui->table_Assignments->insertRow(row);

            // --- FIX: Use Custom Date Item for Sorting ---
            ui->table_Assignments->setItem(row, 0, new DateTableWidgetItem(deadlineStr, deadlineDt));
            // ---------------------------------------------

            QTableWidgetItem *jobItem = new QTableWidgetItem(displayText);
            jobItem->setData(Qt::UserRole, assignmentName);
            ui->table_Assignments->setItem(row, 1, jobItem);

            QString estimateStr = mainApp->getAssignmentEstimate(assignmentName, false);
            ui->table_Assignments->setItem(row, 2, new QTableWidgetItem(estimateStr));

            QTableWidgetItem *typeItem = new QTableWidgetItem("Job");
            typeItem->setBackground(QColor(230, 255, 230));
            typeItem->setForeground(QColor(0, 100, 0));
            ui->table_Assignments->setItem(row, 3, typeItem);

            row++;
        }
    }

    // Re-enable sorting and ensure the current filter is applied to the new data
    ui->table_Assignments->setSortingEnabled(true);
    applyFilter();
    updateStartButtonState();
}

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

    if (!mainApp->startAssignment(assignmentName, isPunishment, newStatus)) {
        return;
    }

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
    else if (isPunishment && data.punishments.contains(baseName) &&
             data.punishments.value(baseName).isDetention) {

        const PunishmentDefinition &def = data.punishments.value(baseName);
        int amount = mainApp->getPunishmentAmount(assignmentName);
        int totalSeconds = amount * 60;

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
