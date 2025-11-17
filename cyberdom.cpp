#include "cyberdom.h"
#include "scriptparser.h"
#include "askpunishment.h" // Include the header for the AskPunishments UI
#include "changemerits.h" // Include the header for the ChangeMerits UI
#include "changestatus.h" // Include the header for the ChangeStatus UI
#include "joblaunch.h" // Include the header for the JobLaunch UI
#include "selectpopup.h" // Include the header for the SelectPopups UI
#include "selectpunishment.h" // Include the header for the SelectPunishments UI
#include "ui_cyberdom.h"
// #include "ui_assignments.h" // Include the header for Assignments UI
#include "timeadd.h" // Include the header for Time_Add UI
#include "about.h" // Include the header for About UI
#include "addclothing.h" // Include the header for AddClothing UI
#include "askclothing.h" // Include the header for AskClothing UI
#include "askinstructions.h" // Include the header for AskInstructions UI
#include "reportclothing.h" // Include the header for ReportClothing UI
#include "rules.h" // Include the header for Rules UI
#include "listflags.h" // Include the header for the ListFlags UI
#include "setflags.h" // Include the header for the SetFlags UI
#include "deleteassignments.h" // Include the header for DeleteAssignments UI
#include "calendarview.h"
#include "clothingitem.h"
#include "questiondialog.h"
#include "ScriptUtils.h"
#include <QtMath>

#include <QFileDialog>
#include <QAction>
#include <QMessageBox>
#include <QSettings>
#include <QDebug>
#include <QProcess>
#include <QFile>
#include <QTextStream>
#include <QRandomGenerator>
#include <cstdlib>
#include <ctime>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QInputDialog>
#include <QRegularExpression>
#include <QDir>
#include <QStandardPaths>
#include <QUrl>
#include <QCoreApplication>
#include <QScrollBar>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QStringConverter>
#endif

static QDate nthWeekdayOfMonth(int year, int month, Qt::DayOfWeek weekday, int n)
{
    QDate date(year, month, 1);
    if (n > 0) {
        int delta = (weekday - date.dayOfWeek() + 7) % 7;
        return date.addDays(delta + 7 * (n - 1));
    }

    QDate last(year, month, date.daysInMonth());
    int delta = (last.dayOfWeek() - weekday + 7) % 7;
    return last.addDays(-delta);
}

static QList<CalendarEvent> generateUSHolidays(int year)
{
    QList<CalendarEvent> list;
    auto add = [&](const QDate &d, const QString &name) {
        CalendarEvent ev;
        ev.start = QDateTime(d, QTime(0, 0));
        ev.end = QDateTime(d, QTime(23, 59, 59));
        ev.title = name;
        ev.type = QStringLiteral("Holiday");
        list.append(ev);
    };

    add(QDate(year, 1, 1), QObject::tr("New Year's Day"));
    add(nthWeekdayOfMonth(year, 1, Qt::Monday, 3), QObject::tr("Martin Luther King Jr. Day"));
    add(nthWeekdayOfMonth(year, 2, Qt::Monday, 3), QObject::tr("Washington's Birthday"));
    add(nthWeekdayOfMonth(year, 5, Qt::Monday, -1), QObject::tr("Memorial Day"));
    add(QDate(year, 6, 19), QObject::tr("Juneteenth"));
    add(QDate(year, 7, 4), QObject::tr("Independence Day"));
    add(nthWeekdayOfMonth(year, 9, Qt::Monday, 1), QObject::tr("Labor Day"));
    add(nthWeekdayOfMonth(year, 10, Qt::Monday, 2), QObject::tr("Columbus Day"));
    add(QDate(year, 11, 11), QObject::tr("Veterans Day"));
    add(nthWeekdayOfMonth(year, 11, Qt::Thursday, 4), QObject::tr("Thanksgiving Day"));
    add(QDate(year, 12, 25), QObject::tr("Christmas Day"));

    return list;
}
CyberDom::CyberDom(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::CyberDom)
{

    ui->setupUi(this);

    // Setup signin timer early so updateStatus can use it
    signinTimer = new QTimer(this);
    connect(signinTimer, &QTimer::timeout, this, &CyberDom::updateSigninTimer);
    connect(ui->resetTimer, &QPushButton::clicked, this, &CyberDom::onResetSigninTimer);
    ui->timerLabel->hide();
    ui->resetTimer->hide();
    ui->lbl_Timer->hide();

    // Ensure ScriptParser is available early
    if (!scriptParser) {
        scriptParser = new ScriptParser();
        qDebug() << "[DEBUG] Created ScriptParser in constructor";
    }

    reportMenu = ui->menuReport;
    if (reportMenu)
        connect(this, &CyberDom::destroyed, reportMenu, &QMenu::clear);

    confessMenu = ui->menuConfess;
    if (confessMenu)
        connect(this, &CyberDom::destroyed, confessMenu, &QMenu::clear);

    permissionMenu = ui->menuAskPermission;
    if (permissionMenu)
        connect(this, &CyberDom::destroyed, permissionMenu, &QMenu::clear);

    // Connect the menuAssignments action to the slot function
    connect(ui->actionAssignments, &QAction::triggered, this, &CyberDom::openAssignmentsWindow);

    // Connect the Time_Add action to a slot function
    connect(ui->actionTimeAdd, &QAction::triggered, this, &CyberDom::openTimeAddDialog);

    //Connect the About action to a slot function
    connect(ui->actionAbout, &QAction::triggered, this, &CyberDom::openAboutDialog);

    // Connect the AskClothing action to a slot function
    connect(ui->actionAsk_for_Clothing_Instructions, &QAction::triggered, this, &CyberDom::openAskClothingDialog);

    // Connect the AskInstructions action to a slot function
    connect(ui->actionAsk_for_Other_Instructions, &QAction::triggered, this, &CyberDom::openAskInstructionsDialog);

    // Connect the ReportClothing action to a slot function
    connect(ui->actionReport_Clothing, &QAction::triggered, this, &CyberDom::openReportClothingDialog);

    // Connect the AskPunishment action to a slot function
    connect(ui->actionAsk_for_Punishment, &QAction::triggered, this, &CyberDom::openAskPunishmentDialog);

    // Connect the Change_Merit_Points action to a slot function
    connect(ui->actionChange_Merit_Points, &QAction::triggered, this, &CyberDom::openChangeMeritsDialog);

    // Connect the Change_Status action to a slot function
    connect(ui->actionStatus_Change, &QAction::triggered, this, &CyberDom::openChangeStatusDialog);

    // Connect the Select_Job action to a slot function
    connect(ui->actionJob_Launch, &QAction::triggered, this, &CyberDom::openLaunchJobDialog);
    connect(ui->actionCalendar, &QAction::triggered, this, &CyberDom::openCalendarView);

    if (ui->menuAssignments) {
        ui->menuAssignments->removeAction(ui->actionCalendar);
        QList<QAction*> acts = ui->menuAssignments->actions();
        int idx = acts.indexOf(ui->actionAssignments);
        QAction *pos = nullptr;
        if (idx >= 0 && idx + 1 < acts.size())
            pos = acts.at(idx + 1);
        ui->menuAssignments->insertAction(pos, ui->actionCalendar);
    }

    // Connect the Select_Punishment action to a slot function
    connect(ui->actionPunish, &QAction::triggered, this, &CyberDom::openSelectPunishmentDialog);

    // Connect the Select_Popup action to a slot function
    connect(ui->actionPop_Up_Launch, &QAction::triggered, this, &CyberDom::openSelectPopupDialog);

    // Connect the List_Flags action to a slot function
    connect(ui->actionList_and_Remove_Flags, &QAction::triggered, this, &CyberDom::openListFlagsDialog);

    // Connect the SetFlags action to a slot function
    connect(ui->actionSet_Flags, &QAction::triggered, this, &CyberDom::openSetFlagsDialog);

    // Connect the DeleteAssignments action to a slot function
    connect(ui->actionList_and_Delete_Assignments, &QAction::triggered, this, &CyberDom::openDeleteAssignmentsDialog);


    // Connect the Delete_Status_File action to a slot function
    connect(ui->actionDelete_Status_File, &QAction::triggered, this, &CyberDom::resetApplication);

    // Connect the viewScriptData action to a slot function
    connect(ui->actionViewScriptData, &QAction::triggered, this, &CyberDom::openDataInspector);

    // Load settings on startup
    //configManager.loadSettings();

    // Update UI with loaded settings

    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(appDataDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    QString newSessionPath = dir.filePath("session.cdt");
    QString oldSessionPath = QCoreApplication::applicationDirPath() + "/session.cdt";
    if (QFile::exists(oldSessionPath) && !QFile::exists(newSessionPath)) {
        QFile::rename(oldSessionPath, newSessionPath);
    }

    sessionFilePath = newSessionPath;
    bool sessionLoaded = loadSessionData(sessionFilePath);

    // Initialize the .ini file if no session was loaded
    if (!sessionLoaded) {
        initializeIniFile();
    }


    // Load saved variables from .cds file if possible
    if (scriptParser && !currentIniFile.isEmpty()) {
        QString cdsPath = currentIniFile;
        QRegularExpression rx("\\.ini$", QRegularExpression::CaseInsensitiveOption);
        cdsPath.replace(rx, ".cds");
        if (scriptParser->loadFromCDS(cdsPath)) {
            qDebug() << "[INFO] Loaded saved variables from" << cdsPath;
        } else {
            qDebug() << "[INFO] No .cds found (or failed to load) at" << cdsPath;
        }
    }

    // Initialize the internal clock
    QSettings settings(settingsFile, QSettings::IniFormat);
    QString savedDate = settings.value("System/CurrentDate", "").toString();
    QString savedTime = settings.value("System/CurrentTime", "").toString();

    if (!sessionLoaded) {
        if (savedDate.isEmpty()) {
            // First run - use system date and time
            internalClock = QDateTime::currentDateTime();
            settings.setValue("System/CurrentDate",
                              internalClock.date().toString("MM-dd-yyyy"));
            settings.setValue("System/CurrentTime",
                              internalClock.time().toString("hh:mm:ss"));
        } else {
            QDate date = QDate::fromString(savedDate, "MM-dd-yyyy");
            QTime time = QTime::fromString(savedTime, "hh:mm:ss");
            if (!time.isValid())
                time = QTime::currentTime();
            internalClock = QDateTime(date, time);
        }
    }

    // Setup the timer
    clockTimer = new QTimer(this);
    connect(clockTimer, &QTimer::timeout, this, &CyberDom::updateInternalClock);
    clockTimer->start(1000); // Update every second

    // Display the initial time in the QLabel (assuming it's named "clockLabel" in the UI)
    ui->clockLabel->setText(internalClock.toString("hh:mm:ss AP"));

    // Setup menu connections for Rules submenu
    setupMenuConnections();

    // Override progress bar values from the loaded INI file
    initializeProgressBarRange();

    // Intialize PunishmentTimer
    punishmentTimer = new QTimer(this);
    connect(punishmentTimer, &QTimer::timeout, this, &CyberDom::checkPunishments);
    punishmentTimer->start(60000); // Check every minute

    // Intialize Flag Timer
    flagTimer = new QTimer(this);
    connect(flagTimer, &QTimer::timeout, this, &CyberDom::checkFlagExpiry);
    flagTimer->start(30000); // Check every 30 seconds

    // Debugging values to confirm override
    qDebug() << "CyberDom initialized with Min Merits:" << minMerits << "Max Merits:" << maxMerits;
    qDebug() << "Final ProgressBar Min:" << ui->progressBar->minimum();
    qDebug() << "Final ProgressBar Max:" << ui->progressBar->maximum();

    QString settingsPath = QCoreApplication::applicationDirPath() + "/cyberdom_settings.ini";
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, QCoreApplication::applicationDirPath());

    // Trigger openProgram event handler if defined
    if (scriptParser) {
        QString proc = scriptParser->getScriptData().eventHandlers.openProgram;
        if (!proc.isEmpty())
            runProcedure(proc);
    }

    // Check if we missed a scheduled job run
    QDate today = internalClock.date();
    if (!lastScheduledJobsRun.isValid() || lastScheduledJobsRun < today) {
        qDebug() << "[Scheduler] Running missed daily job check on startup.";
        assignScheduledJobs();
        lastScheduledJobsRun = today; // Mark as run for today
    }

    applyDailyMerits(); // Apply daily merits on startup if needed
}

CyberDom::~CyberDom()
{
    // Trigger closeProgram event handler if defined
    if (scriptParser) {
        QString proc = scriptParser->getScriptData().eventHandlers.closeProgram;
        if (!proc.isEmpty())
            runProcedure(proc);
    }

    // Save variables to .cds on exit
    if (scriptParser) {
        QString cdsPath = currentIniFile;
        cdsPath.replace(".ini", ".cds");
        saveVariablesToCDS(cdsPath);
    }

    if (!sessionFilePath.isEmpty()) {
        saveSessionData(sessionFilePath);
    }

    // Persist the current internal clock time to settings
    if (!settingsFile.isEmpty()) {
        QSettings settings(settingsFile, QSettings::IniFormat);
        settings.setValue("System/CurrentDate",
                           internalClock.date().toString("MM-dd-yyyy"));
        settings.setValue("System/CurrentTime",
                           internalClock.time().toString("hh:mm:ss"));
    }

    if (reportMenu)
        reportMenu->clear();
    if (confessMenu)
        confessMenu->clear();

    delete ui;
    delete scriptParser;
}

int CyberDom::getMinMerits() const
{
    return minMerits;
}

int CyberDom::getMaxMerits() const
{
    return maxMerits;
}

int CyberDom::getAskPunishmentMin() const {
    return askPunishmentMin;
}

int CyberDom::getAskPunishmentMax() const {
    return askPunishmentMax;
}

void CyberDom::openAssignmentsWindow()
{
    if (checkInterruptableAssignments()) return;

    if (!assignmentsWindow) {
        assignmentsWindow = new Assignments(this, this);
        connect(this, &CyberDom::jobListUpdated, assignmentsWindow, &Assignments::populateJobList);
    }
    assignmentsWindow->show();
    assignmentsWindow->raise();
    assignmentsWindow->activateWindow();
}

// void CyberDom::openAssignmentsWindow()
// {
//     if (!assignmentsWindow) {
//         assignmentsWindow = new QMainWindow(this); // Create the window if not already created
//         assignmentsUi.setupUi(assignmentsWindow); // Setup the Assignments UI
//     }
//     assignmentsWindow->show(); // Show the Assignments Window
//     assignmentsWindow->raise(); // Bring the window to the front (optional)
//     assignmentsWindow->activateWindow(); // Activate the window (optional)
// }

void CyberDom::openTimeAddDialog()
{
    if (checkInterruptableAssignments()) return;

    TimeAdd timeAddDialog(this); // Pass the parent to ensure proper memory management
    connect(&timeAddDialog, &TimeAdd::timeAdded, this, &CyberDom::applyTimeToClock); // Connect the timeAdded signal to the applyTimeToClock slot
    timeAddDialog.exec(); // Show the dialog modally
}

void CyberDom::applyTimeToClock(int days, int hours, int minutes, int seconds)
{
    // Update the internal clock with the specified values
    internalClock = internalClock.addDays(days)
                                 .addSecs(hours * 3600 + minutes * 60 + seconds);

    // Update the displayed clock
    ui->clockLabel->setText(internalClock.toString("hh:mm:ss AP"));
}

void CyberDom::updateInternalClock()
{
    // Store previous time to check if we crossed midnight
    QDateTime previousTime = internalClock;

    // Increment the internal clock by 1 second
    internalClock = internalClock.addSecs(1);

    // Update the QLabel to display the new time
    ui->clockLabel->setText(internalClock.toString("hh:mm:ss AP"));
    updateStatusText();

    // Check if we crossed midnight (new day)
    if (previousTime.date() != internalClock.date()) {
        qDebug() << "Date changed from: " << previousTime.toString("MM-dd-yyyy") << "to: " << internalClock.toString("MM-dd-yyyy");

        // Reset Timers
        qDebug() << "[Timer] Resetting all timer triggers for new day.";
        for (TimerInstance &timer : activeTimers) {
            timer.triggered = false;
        }

        applyDailyMerits();

        // Trigger the NewDay event
        if (scriptParser) {
            QString newDayProcedure = scriptParser->getScriptData().eventHandlers.newDay;
            if (!newDayProcedure.isEmpty()) {
                runProcedure(newDayProcedure);
            }
        }

        // Save the current date and time to settings
        QSettings settings(settingsFile, QSettings::IniFormat);
        settings.setValue("System/CurrentDate",
                          internalClock.date().toString("MM-dd-yyyy"));
        settings.setValue("System/CurrentTime",
                          internalClock.time().toString("hh:mm:ss"));
    }

    QTime now = internalClock.time();
    QDate today = internalClock.date();

    // Run at 2:00:00 AM, but only if we haven't already run today
    if (now.hour() == 2 && now.minute() == 0 && now.second() == 0 && lastScheduledJobsRun != today) {
        qDebug() << "[Scheduler] Running daily job check at 2 AM.";
        assignScheduledJobs();
        lastScheduledJobsRun = today; // Mark as run for today
    }

    for (TimerInstance &timer : activeTimers) {
        QTime now = internalClock.time();
        if (!timer.triggered && now >= timer.start && now <= timer.end) {
            if (scriptParser) {
                const auto &defs = scriptParser->getScriptData().timers;
                if (defs.contains(timer.name)) {
                    const TimerDefinition &td = defs.value(timer.name);

                    // Check PreStatus requirements for the timer
                    if (!td.preStatuses.isEmpty()) {
                        bool statusMatch = false;
                        QString lowerCurrentStatus = currentStatus.toLower();
                        for (const QString &preStatus : td.preStatuses) {
                            if (preStatus.toLower() == lowerCurrentStatus) {
                                statusMatch = true;
                                break;
                            }
                        }
                        // If no match, skip this timer trigger
                        if (!statusMatch) {
                            continue;
                        }
                    }

                    if (!isTimeAllowed(td.notBeforeTimes, td.notAfterTimes, td.notBetweenTimes))
                        continue;
                }
            }
            qDebug() << "[Timer Triggered]:" << timer.name;
            timer.triggered = true;

            if (scriptParser) {
                const auto &defs = scriptParser->getScriptData().timers;
                if (defs.contains(timer.name)) {
                    const TimerDefinition &td = defs.value(timer.name);

                    // --- Executor Loop ---
                    bool skipNextAction = false;
                    for (const ScriptAction &action : td.actions) {
                        if (skipNextAction) {
                            skipNextAction = false;
                            if (action.type != ScriptActionType::If && action.type != ScriptActionType::NotIf) {
                                continue;
                            }
                        }

                        switch (action.type) {
                        case ScriptActionType::If:
                            if (!evaluateCondition(action.value)) skipNextAction = true;
                            break;
                        case ScriptActionType::NotIf:
                            if (evaluateCondition(action.value)) skipNextAction = true;
                            break;
                        case ScriptActionType::ProcedureCall:
                            runProcedure(action.value.split(",").first().trimmed().toLower());
                            break;
                        case ScriptActionType::SetFlag:
                            setFlag(action.value);
                            break;
                        case ScriptActionType::RemoveFlag:
                        case ScriptActionType::ClearFlag:
                            removeFlag(action.value);
                            break;
                        case ScriptActionType::SetCounterVar: {
                            QStringList parts = action.value.split(",", Qt::SkipEmptyParts);
                            if (parts.size() == 2) {
                                QString varName = parts[0].trimmed();
                                if (varName.startsWith("#")) varName = varName.mid(1);
                                QString valToSet = parts[1].trimmed();
                                if (valToSet.startsWith("#")) {
                                    valToSet = scriptParser->getVariable(valToSet.mid(1));
                                }
                                scriptParser->setVariable(varName, valToSet);
                                qDebug() << "[DEBUG] Variable set from permission:" << varName << "=" << valToSet;
                            }
                            break;
                        }
                        case ScriptActionType::AddCounter:
                            performCounterOperation(action.value, "add");
                            break;
                        case ScriptActionType::SubtractCounter:
                            performCounterOperation(action.value, "subtract");
                            break;
                        case ScriptActionType::MultiplyCounter:
                            performCounterOperation(action.value, "multiply");
                            break;
                        case ScriptActionType::DivideCounter:
                            performCounterOperation(action.value, "divide");
                            break;
                        case ScriptActionType::Message:
                            QMessageBox::information(this, tr("Timer"), replaceVariables(action.value));
                            break;
                        case ScriptActionType::NewStatus:
                            changeStatus(action.value, false);
                            break;
                        case ScriptActionType::MarkDone: {
                            QString name = action.value;
                            bool isPun = name.startsWith("punishment-", Qt::CaseInsensitive);
                            markAssignmentDone(isPun ? name.mid(11) : name.mid(4), isPun);
                            break;
                        }
                        case ScriptActionType::Abort: {
                            QString name = action.value;
                            bool isPun = name.startsWith("punishment-", Qt::CaseInsensitive);
                            abortAssignment(isPun ? name.mid(11) : name.mid(4), isPun);
                            break;
                        }
                        case ScriptActionType::Delete: {
                            QString name = action.value;
                            bool isPun = name.startsWith("punishment-", Qt::CaseInsensitive);
                            deleteAssignment(isPun ? name.mid(11) : name.mid(4), isPun);
                            break;
                        }
                        case ScriptActionType::AddMerit:
                            modifyMerits(action.value, "add");
                            break;
                        case ScriptActionType::SubtractMerit:
                            modifyMerits(action.value, "subtract");
                            break;
                        case ScriptActionType::SetMerit:
                            modifyMerits(action.value, "set");
                            break;

                        case ScriptActionType::Punish:
                            executePunish(action.value, td.punishMessage, td.punishGroup);
                            break;

                        default:
                            qDebug() << "[WARN] Unhandled action type in permission:" << static_cast<int>(action.type);
                            break;
                        }
                    }
                }
            }
        }
    }
}

void CyberDom::openAboutDialog()
{
    if (checkInterruptableAssignments()) return;

    if (currentIniFile.isEmpty()) {
        QMessageBox::warning(this, "Error", "No script file is loaded.");
        return;
    }

    QString version = "Unknown";
    if (scriptParser) {
        // Get the version from the parsed GeneralSettings struct
        version = scriptParser->getScriptData().general.version;
        if (version.isEmpty()) {
            version = "Unknown";
        }
    }

    About aboutDialog(this, currentIniFile, version);
    aboutDialog.exec();
}

void CyberDom::openAskClothingDialog()
{
    if (checkInterruptableAssignments()) return;

    AskClothing askclothingDialog(this, scriptParser); // Create the AskClothing dialog with parser
    askclothingDialog.exec(); // Show the dialog modally
}

void CyberDom::openAskInstructionsDialog()
{
    if (checkInterruptableAssignments()) return;

    AskInstructions askinstructionsDialog(this); // Create the AskInstructions dialog, passing the parent
    askinstructionsDialog.exec(); // Show the dialog modally
}

void CyberDom::openReportClothingDialog()
{
    if (checkInterruptableAssignments()) return;

    ReportClothing *reportClothingDialog = new ReportClothing(this, scriptParser);
    
    // Display the dialog
    if (reportClothingDialog->exec() == QDialog::Accepted) {
        // Process the clothing report
        processClothingReport(reportClothingDialog->getWearingItems(), reportClothingDialog->isNaked());
    }
    
    delete reportClothingDialog;
}

void CyberDom::openAddClothingDialog()
{
    if (checkInterruptableAssignments()) return;

    AddClothing dlg(this, tr("Generic"));
    dlg.exec();
}

void CyberDom::openReport(const QString &name)
{
    if (checkInterruptableAssignments()) return;

    if (!scriptParser) {
        qWarning() << "[CyberDom] No script loaded for report:" << name;
        return;
    }

    const auto &reports = scriptParser->getScriptData().reports;
    if (reports.contains(name)) {
        const ReportDefinition &rep = reports.value(name);
        if (!isTimeAllowed(rep.notBeforeTimes, rep.notAfterTimes, rep.notBetweenTimes)) {
            QMessageBox::information(this, tr("Report"),
                                     tr("This report is not available at this time."));
            return;
        }
    }

    qDebug() << "[CyberDom] Opening report:" << name;
    executeReport(name);
}

void CyberDom::openAskPermissionDialog()
{
    if (checkInterruptableAssignments()) return;

    if (!scriptParser)
        return;

    QStringList labels;
    const auto perms = scriptParser->getPermissionSections();
    for (const auto &perm : perms) {
        QString label = perm.title.isEmpty() ? perm.name : perm.title;
        labels << label;
    }

    bool ok = false;
    QString choice = QInputDialog::getItem(this, tr("Ask Permission"),
                                           tr("Select permission:"), labels, 0, false, &ok);
    if (!ok || choice.isEmpty())
        return;

    QString selectedName;
    for (const auto &perm : perms) {
        QString label = perm.title.isEmpty() ? perm.name : perm.title;
        if (label == choice) {
            selectedName = perm.name;
            break;
        }
    }

    if (!selectedName.isEmpty())
        openPermission(selectedName);
}

void CyberDom::openPermission(const QString &name)
{
    if (checkInterruptableAssignments()) return;

    if (!scriptParser)
        return;

    const auto &perms = scriptParser->getScriptData().permissions;
    if (!perms.contains(name)) {
        qWarning() << "[CyberDom] Permission definition not found:" << name;
        return;
    }

    const PermissionDefinition &perm = perms.value(name);

    // --- 1. Check PreStatus (Consistent with other menus) ---
    if (!perm.preStatuses.isEmpty()) {
        QString lowerCurrentStatus = currentStatus.toLower();
        bool foundMatch = false;
        for (const QString &preStatus : perm.preStatuses) {
            if (preStatus.toLower() == lowerCurrentStatus) {
                foundMatch = true;
                break;
            }
        }
        if (!foundMatch) {
            QMessageBox::information(this, tr("Permission"),
                                     tr("This permission is not available in your current status."));
            return;
        }
    }

    // --- 2. Check Time (Existing logic) ---
    if (!isTimeAllowed(perm.notBeforeTimes, perm.notAfterTimes, perm.notBetweenTimes)) {
        QMessageBox::information(this, tr("Permission"),
                                 tr("This permission is not available at this time."));
        return;
    }
    incrementUsageCount(QString("Permission/%1").arg(name));

    // --- 3. Check if Forbidden (Existing logic) ---
    if (isPermissionForbidden(name)) {
        QString title = perm.title.isEmpty() ? perm.name : perm.title;
        QMessageBox::information(this, tr("Permission"),
                                 tr("No %1 you may not %2")
                                     .arg(subNameVariable, title));
        if (!perm.denyProcedure.isEmpty())
            runProcedure(perm.denyProcedure);
        QString eventProc = scriptParser->getScriptData().eventHandlers.permissionDenied;
        if (!eventProc.isEmpty())
            runProcedure(eventProc);
        return;
    }

    // --- 4. Run BeforeProcedure (Existing logic) ---
    if (!perm.beforeProcedure.isEmpty())
        runProcedure(perm.beforeProcedure);

    // --- 5. Ask User (Existing logic) ---
    bool granted = QMessageBox::question(this, tr("Permission"),
                                         tr("Grant permission '%1'?")
                                             .arg(perm.title.isEmpty() ? perm.name : perm.title),
                                         QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;

    if (granted) {
        // --- 6. Handle "GRANTED" (NEW LOGIC) ---

        // A. Show the permit message
        if (!perm.permitMessage.isEmpty())
            QMessageBox::information(this, tr("Permission"), replaceVariables(perm.permitMessage));

        // B. Run the global event handler
        QString eventProc = scriptParser->getScriptData().eventHandlers.permissionGiven;
        if (!eventProc.isEmpty())
            runProcedure(eventProc);

        int merits = getMeritsFromIni();
        auto getMeritValue = [&](const QString &s) -> int {
            if (s.isEmpty()) return 0;
            if (s.startsWith("#")) {
                return scriptParser->getVariable(s.mid(1)).toInt();
            }
            return s.toInt();
        };

        if (!perm.merits.set.isEmpty()) {
            merits = getMeritValue(perm.merits.set);
        } else {
            merits += getMeritValue(perm.merits.add);
            merits -= getMeritValue(perm.merits.subtract);
        }
        merits = std::max(getMinMerits(), std::min(getMaxMerits(), merits));
        updateMerits(merits);

        // C. Execute all actions associated with this permission
        bool skipNextAction = false;
        for (const ScriptAction &action : perm.actions) {
            if (skipNextAction) {
                skipNextAction = false;
                if (action.type != ScriptActionType::If && action.type != ScriptActionType::NotIf) {
                    qDebug() << "[Permission] Action skipped due to condition.";
                    continue;
                }
            }

            // This is the same execution loop from runProcedure
            switch (action.type) {
            case ScriptActionType::If:
                if (!evaluateCondition(action.value)) skipNextAction = true;
                break;
            case ScriptActionType::NotIf:
                if (evaluateCondition(action.value)) skipNextAction = true;
                break;
            case ScriptActionType::ProcedureCall:
                runProcedure(action.value.split(",").first().trimmed().toLower());
                break;
            case ScriptActionType::SetFlag:
                setFlag(action.value);
                break;
            case ScriptActionType::RemoveFlag:
            case ScriptActionType::ClearFlag:
                removeFlag(action.value);
                break;
            case ScriptActionType::SetCounterVar: {
                QStringList parts = action.value.split(",", Qt::SkipEmptyParts);
                if (parts.size() == 2) {
                    QString varName = parts[0].trimmed();
                    if (varName.startsWith("#")) varName = varName.mid(1);
                    QString valToSet = parts[1].trimmed();
                    if (valToSet.startsWith("#")) {
                        valToSet = scriptParser->getVariable(valToSet.mid(1));
                    }
                    scriptParser->setVariable(varName, valToSet);
                    qDebug() << "[DEBUG] Variable set from permission:" << varName << "=" << valToSet;
                }
                break;
            }
            case ScriptActionType::AddCounter:
                performCounterOperation(action.value, "add");
                break;
            case ScriptActionType::SubtractCounter:
                performCounterOperation(action.value, "subtract");
                break;
            case ScriptActionType::MultiplyCounter:
                performCounterOperation(action.value, "multiply");
                break;
            case ScriptActionType::DivideCounter:
                performCounterOperation(action.value, "divide");
                break;
            case ScriptActionType::Message:
                QMessageBox::information(this, tr("Permission"), replaceVariables(action.value));
                break;
            case ScriptActionType::NewStatus:
                changeStatus(action.value, false);
                break;
            case ScriptActionType::NewSubStatus:
                changeStatus(action.value, true);
                break;
            case ScriptActionType::MarkDone: {
                QString name = action.value;
                bool isPun = name.startsWith("punishment-", Qt::CaseInsensitive);
                markAssignmentDone(isPun ? name.mid(11) : name.mid(4), isPun);
                break;
            }
            case ScriptActionType::Abort: {
                QString name = action.value;
                bool isPun = name.startsWith("punishment-", Qt::CaseInsensitive);
                abortAssignment(isPun ? name.mid(11) : name.mid(4), isPun);
                break;
            }
            case ScriptActionType::Delete: {
                QString name = action.value;
                bool isPun = name.startsWith("punishment-", Qt::CaseInsensitive);
                deleteAssignment(isPun ? name.mid(11) : name.mid(4), isPun);
                break;
            }

            case ScriptActionType::Punish:
                executePunish(action.value, perm.punishMessage, perm.punishGroup);
                break;
            // Add other cases here as needed (Set$, Set!, etc.)
            default:
                qDebug() << "[WARN] Unhandled action type in permission:" << static_cast<int>(action.type);
                break;
            }
        }

    } else {
        // --- 7. Handle "DENIED" (Existing logic) ---
        if (!perm.denyMessage.isEmpty())
            QMessageBox::information(this, tr("Permission"), replaceVariables(perm.denyMessage));
        if (!perm.denyProcedure.isEmpty())
            runProcedure(perm.denyProcedure);
        QString eventProc = scriptParser->getScriptData().eventHandlers.permissionDenied;
        if (!eventProc.isEmpty())
            runProcedure(eventProc);
    }
}

bool CyberDom::isPermissionForbidden(const QString &name) const {
    if (!scriptParser)
        return false;

    const auto &puns = scriptParser->getScriptData().punishments;
    for (auto it = puns.constBegin(); it != puns.constEnd(); ++it) {
        const PunishmentDefinition &p = it.value();
        if (!activeAssignments.contains(p.name))
            continue;
        for (const QString &forbid : p.forbids) {
            if (forbid.trimmed().compare(name, Qt::CaseInsensitive) == 0)
                return true;
        }
    }
    return false;
}

void CyberDom::openConfession(const QString &name)
{
    if (checkInterruptableAssignments()) return;

    if (!scriptParser)
        return;

    const auto &confs = scriptParser->getScriptData().confessions;
    if (!confs.contains(name)) {
        qWarning() << "[CyberDom] Confession definition not found:" << name;
        return;
    }

    const ConfessionDefinition &conf = confs.value(name);

    // --- 1. Check PreStatus ---
    if (!conf.preStatuses.isEmpty()) {
        QString lowerCurrentStatus = currentStatus.toLower();
        bool foundMatch = false;
        for (const QString &preStatus : conf.preStatuses) {
            if (preStatus.toLower() == lowerCurrentStatus) {
                foundMatch = true;
                break;
            }
        }
        if (!foundMatch) {
            QMessageBox::information(this, tr("Confession"),
                                     tr("This confession is not available in your current status."));
            return;
        }
    }

    // --- 2. Check Time ---
    if (!isTimeAllowed(conf.notBeforeTimes, conf.notAfterTimes, conf.notBetweenTimes)) {
        QMessageBox::information(this, tr("Confession"),
                                 tr("This confession is not available at this time."));
        return;
    }
    incrementUsageCount(QString("Confession/%1").arg(name));

    // --- 3. Handle Merits ---
    int merits = getMeritsFromIni();

    // Helper lambda to get the value
    auto getMeritValue = [&](const QString &s) -> int {
        if (s.isEmpty()) return 0;
        if (s.startsWith("#")) {
            return scriptParser->getVariable(s.mid(1)).toInt();
        }
        return s.toInt();
    };

    if (!conf.merits.set.isEmpty()) {
        merits = getMeritValue(conf.merits.set);
    } else {
        merits += getMeritValue(conf.merits.add);
        merits -= getMeritValue(conf.merits.subtract);
    }

    merits = std::max(getMinMerits(), std::min(getMaxMerits(), merits));
    updateMerits(merits);

    // --- 4. Show Messages (These are properties) ---
    auto showMessage = [this](const QString &m) {
        if (!m.trimmed().isEmpty())
            QMessageBox::information(this, tr("Confession"), replaceVariables(m.trimmed()));
    };

    for (const QString &txt : conf.statusTexts)
        showMessage(txt);

    for (const MessageGroup &grp : conf.messages) {
        if (grp.messages.isEmpty())
            continue;
        if (grp.mode == MessageSelectMode::Random) {
            int idx = QRandomGenerator::global()->bounded(grp.messages.size());
            showMessage(grp.messages.at(idx));
        } else {
            for (const QString &msg : grp.messages)
                showMessage(msg);
        }
    }

    // --- 5. Ask Questions (These are properties) ---
    QList<QuestionDefinition> questions;
    for (const QString &qname : conf.inputQuestions) {
        QuestionDefinition q = scriptParser->getQuestion(qname.toLower());
        if (!q.name.isEmpty())
            questions.append(q);
    }
    for (const QString &qname : conf.advancedQuestions) {
        QuestionDefinition q = scriptParser->getQuestion(qname.toLower());
        if (!q.name.isEmpty())
            questions.append(q);
    }

    if (!questions.isEmpty()) {
        for (QuestionDefinition &q : questions)
            q.phrase = replaceVariables(q.phrase);
        QuestionDialog dlg(questions, this);
        dlg.exec();
    } else if (!conf.noInputProcedure.isEmpty()) {
        runProcedure(conf.noInputProcedure.toLower());
    }

    // --- 6. Execute Actions (NEW) ---
    bool skipNextAction = false;
    for (const ScriptAction &action : conf.actions) {
        if (skipNextAction) {
            skipNextAction = false;
            if (action.type != ScriptActionType::If && action.type != ScriptActionType::NotIf) {
                continue;
            }
        }

        switch (action.type) {
        case ScriptActionType::If:
            if (!evaluateCondition(action.value)) skipNextAction = true;
            break;
        case ScriptActionType::NotIf:
            if (evaluateCondition(action.value)) skipNextAction = true;
            break;
        case ScriptActionType::ProcedureCall:
            runProcedure(action.value.split(",").first().trimmed().toLower());
            break;
        case ScriptActionType::SetFlag:
            setFlag(action.value);
            break;
        case ScriptActionType::RemoveFlag:
        case ScriptActionType::ClearFlag:
            removeFlag(action.value);
            break;
        case ScriptActionType::SetCounterVar: {
            QStringList parts = action.value.split(",", Qt::SkipEmptyParts);
            if (parts.size() == 2) {
                QString varName = parts[0].trimmed();
                if (varName.startsWith("#")) varName = varName.mid(1);
                QString valToSet = parts[1].trimmed();
                if (valToSet.startsWith("#")) {
                    valToSet = scriptParser->getVariable(valToSet.mid(1));
                }
                scriptParser->setVariable(varName, valToSet);
                qDebug() << "[DEBUG] Variable set from confession:" << varName << "=" << valToSet;
            }
            break;
        }
        case ScriptActionType::AddCounter:
            performCounterOperation(action.value, "add");
            break;
        case ScriptActionType::SubtractCounter:
            performCounterOperation(action.value, "subtract");
            break;
        case ScriptActionType::MultiplyCounter:
            performCounterOperation(action.value, "multiply");
            break;
        case ScriptActionType::DivideCounter:
            performCounterOperation(action.value, "divide");
            break;
        case ScriptActionType::Message:
            QMessageBox::information(this, tr("Confession"), replaceVariables(action.value));
            break;
        case ScriptActionType::NewStatus:
            changeStatus(action.value, false);
            break;
        case ScriptActionType::NewSubStatus:
            changeStatus(action.value, true);
            break;
        case ScriptActionType::MarkDone: {
            QString name = action.value;
            bool isPun = name.startsWith("punishment-", Qt::CaseInsensitive);
            markAssignmentDone(isPun ? name.mid(11) : name.mid(4), isPun);
            break;
        }
        case ScriptActionType::Abort: {
            QString name = action.value;
            bool isPun = name.startsWith("punishment-", Qt::CaseInsensitive);
            markAssignmentDone(isPun ? name.mid(11) : name.mid(4), isPun);
            break;
        }
        case ScriptActionType::Delete: {
            QString name = action.value;
            bool isPun = name.startsWith("punishment-", Qt::CaseInsensitive);
            deleteAssignment(isPun ? name.mid(11) : name.mid(4), isPun);
            break;
        }

        case ScriptActionType::Punish:
            executePunish(action.value, conf.punishMessage, conf.punishGroup);
            break;

        default:
            qDebug() << "[WARN] Unhandled action type in confession:" << static_cast<int>(action.type);
            break;
        }
    }
}

void CyberDom::populateReportMenu()
{
    // Refresh the pointer each time in case the UI was recreated
    reportMenu = ui->menuReport;
    if (!reportMenu)
        return;

    reportMenu->clear();

    // Optional action always shown at the top of the report list
    QAction *addClothing = new QAction(tr("Add Clothing"), reportMenu);
    reportMenu->addAction(addClothing);
    connect(addClothing, &QAction::triggered, this, &CyberDom::openAddClothingDialog);

    if (!scriptParser)
        return;

    // Insert a separator between the static and dynamic items
    reportMenu->addSeparator();

    // Get the current status (lowercase) for comparison
    QString lowerCurrentStatus = currentStatus.toLower();

    const auto &reports = scriptParser->getScriptData().reports;
    for (auto it = reports.constBegin(); it != reports.constEnd(); ++it) {
        const ReportDefinition &rep = it.value();
        if (!rep.showInMenu)
            continue;

        // Check the PreStatus requirement
        if (!rep.preStatuses.isEmpty()) {
            // This report has status requirements.
            // We must find a match.
            bool foundMatch = false;
            for (const QString &preStatus : rep.preStatuses) {
                if (preStatus.toLower() == lowerCurrentStatus) {
                    foundMatch = true;
                    break;
                }
            }

            // If no match was found, skip this report
            if (!foundMatch) {
                continue;
            }
        }

        QString label = rep.title.isEmpty() ? rep.name : rep.title;
        qDebug() << "[ReportMenu] Adding" << rep.name;
        QAction *act = new QAction(label, reportMenu);
        reportMenu->addAction(act);
        connect(act, &QAction::triggered, this, [this, name = rep.name]() {
            openReport(name);
        });
    }
}

void CyberDom::populateConfessMenu()
{
    confessMenu = ui->menuConfess;
    if (!confessMenu)
        return;

    confessMenu->clear();

    if (!scriptParser)
        return;

    QString lowerCurrentStatus = currentStatus.toLower();
    const auto confessions = scriptParser->getConfessionSections();

    for (const auto &conf : confessions) {
        if (!conf.showInMenu)
            continue;

        // Check the PreStatus requirement
        if (!conf.preStatuses.isEmpty()) {
            bool foundMatch = false;
            for (const QString &preStatus : conf.preStatuses) {
                if (preStatus.toLower() == lowerCurrentStatus) {
                    foundMatch = true;
                    break;
                }
            }
            if (!foundMatch) {
                continue;
            }
        }

        QString label = conf.title.isEmpty() ? conf.name : conf.title;
        QAction *act = new QAction(label, confessMenu);
        confessMenu->addAction(act);
        connect(act, &QAction::triggered, this, [this, name = conf.name]() {
            openConfession(name);
        });
    }
}

void CyberDom::populatePermissionMenu()
{
    permissionMenu = ui->menuAskPermission;
    if (!permissionMenu)
        return;

    permissionMenu->clear();

    if (!scriptParser)
        return;

    QString lowerCurrentStatus = currentStatus.toLower();
    const auto perms = scriptParser->getPermissionSections();

    for (const auto &perm : perms) {

        // Check the PreStatus requirement
        if (!perm.preStatuses.isEmpty()) {
            bool foundMatch = false;
            for (const QString &preStatus : perm.preStatuses) {
                if (preStatus.toLower() == lowerCurrentStatus) {
                    foundMatch = true;
                    break;
                }
            }
            if (!foundMatch) {
                continue;
            }
        }

        QString label = perm.title.isEmpty() ? perm.name : perm.title;
        label = label.trimmed();
        if (!label.isEmpty())
            label[0] = label[0].toUpper();
        QAction *act = new QAction(label, permissionMenu);
        permissionMenu->addAction(act);
        connect(act, &QAction::triggered, this, [this, name = perm.name]() {
            openPermission(name);
        });
    }
}

void CyberDom::setupMenuConnections()
{
    rulesDialog = new Rules(this); // Initialize the rules dialog

    connect(ui->actionPermissions, &QAction::triggered, this, [this]() {
        rulesDialog->updateRulesDialog(Permissions);
    });

    connect(ui->actionReports, &QAction::triggered, this, [this]() {
        rulesDialog->updateRulesDialog(Reports);
    });

    connect(ui->actionConfessions, &QAction::triggered, this, [this]() {
        rulesDialog->updateRulesDialog(Confessions);
    });

    connect(ui->actionClothing, &QAction::triggered, this, [this]() {
        rulesDialog->updateRulesDialog(Clothing);
    });

    connect(ui->actionInstructions, &QAction::triggered, this, [this]() {
        rulesDialog->updateRulesDialog(Instructions);
    });

    connect(ui->actionOtherRules, &QAction::triggered, this, [this]() {
        rulesDialog->updateRulesDialog(OtherRules);
    });
}

void CyberDom::openAskPunishmentDialog()
{
    if (checkInterruptableAssignments()) return;

    int minPunishment = getAskPunishmentMin();
    int maxPunishment = getAskPunishmentMax();

    AskPunishment askPunishmentDialog(this, minPunishment, maxPunishment); // Create the AskPunishment dialog, passing the parent
    askPunishmentDialog.exec(); // Show the dialog modally
}


void CyberDom::openChangeMeritsDialog()
{
    if (checkInterruptableAssignments()) return;

    // Fetch MinMerits and MaxMerits from the class member variables.

    qDebug() << "Opening ChangeMerits Dialog with:";
    qDebug() << " MinMerits:" << minMerits;
    qDebug() << " MaxMerits:" << maxMerits;

    // Pass the member variables directly to the dialog
    ChangeMerits changeMeritsDialog(this, minMerits, maxMerits);
    connect(&changeMeritsDialog, &ChangeMerits::meritsUpdated, this, &CyberDom::updateMerits);

    changeMeritsDialog.exec();
}

void CyberDom::openChangeStatusDialog()
{
    if (checkInterruptableAssignments()) return;

    qDebug() << "\n[DEBUG] Starting openChangeStatusDialog()";

    QStringList availableStatuses;

    if (scriptParser) {
        // Get the map of StatusDefinition objects from the parsed data
        const auto &statuses = scriptParser->getScriptData().statuses;

        // The keys of this map are the status names
        availableStatuses = statuses.keys();

        qDebug() << "[DEBUG] Retrieved " << availableStatuses.size() << " statuses directly from ScriptParser";
    } else {
        qDebug() << "[ERROR] ScriptParser is NULL!";
    }

    // If no statuses found, show error message
    if (availableStatuses.isEmpty()) {
        qDebug() << "[ERROR] No statuses found in the script (or scriptParser is null)";
        QMessageBox::critical(this, "Error", "No statuses could be loaded from the script.");
        return;
    }

    // Debugging to check all found statuses
    qDebug() << "Available Statuses Loaded: " << availableStatuses;

    ChangeStatus changeStatusDialog(this, currentStatus, availableStatuses);
    connect(&changeStatusDialog, &ChangeStatus::statusUpdated, this, &CyberDom::updateStatus);
    changeStatusDialog.exec();
}

void CyberDom::openLaunchJobDialog()
{
    if (checkInterruptableAssignments()) return;

    // Debug: Check if jobs are available
    if (scriptParser) {
        QList<JobSection> jobs = scriptParser->getJobSections();
        qDebug() << "[DEBUG] Number of jobs in ScriptParser: " << jobs.size();
        for (const JobSection &job : jobs) {
            qDebug() << "[DEBUG] Job found in ScriptParser: " << job.name;
        }
    } else {
        qDebug() << "[ERROR] ScriptParser is not initialized!";
    }
    
    JobLaunch launchJobDialog(this); // Create the SelectJob dialog, passing the parent
    launchJobDialog.exec(); // Show the dialog modally
}

void CyberDom::openCalendarView()
{
    if (checkInterruptableAssignments()) return;

    CalendarView view(this);
    view.exec();
}

void CyberDom::openSelectPunishmentDialog()
{
    if (checkInterruptableAssignments()) return;

    qDebug() << "\n[DEBUG] Opening SelectPunishment dialog.";

    QMap<QString, QMap<QString, QString>> dialogData;

    if (!scriptParser) {
        qDebug() << "[ERROR] ScriptParser is not initialized.";
        QMessageBox::critical(this, "Error", "Script parser is not loaded.");
        return;
    }

    // Get the structured punishment data
    const auto& punishments = scriptParser->getScriptData().punishments;
    qDebug() << "[DEBUG] Found" << punishments.size() << "punishments in ScriptData.";

    // Convert new struct data into raw map format
    for (const PunishmentDefinition &pun : punishments.values()) {
        // The section name
        QString sectionName = "punishment-" + pun.name;

        // The inner map of key/value strings
        QMap<QString, QString> sectionData;

        // --- Manually re-create the raw key/value pairs ---
        sectionData["Title"] = pun.title;
        sectionData["ValueUnit"] = pun.valueUnit;
        sectionData["value"] = QString::number(pun.value);
        sectionData["min"] = QString::number(pun.min);
        sectionData["max"] = QString::number(pun.max);
        sectionData["MinSeverity"] = QString::number(pun.minSeverity);
        sectionData["MaxSeverity"] = QString::number(pun.maxSeverity);
        sectionData["Group"] = pun.groups.join(",");
        sectionData["Respite"] = pun.respite;
        sectionData["Estimate"] = pun.estimate;
        sectionData["Forbids"] = pun.forbids.join(",");
        sectionData["Resources"] = pun.resources.join(",");
        sectionData["Clothing"] = pun.clothingInstruction;

        // Add booleans back as "1" or "0"
        sectionData["LongRunning"] = pun.longRunning ? "1" : "0";
        sectionData["MustStart"] = pun.mustStart ? "1" : "0";
        sectionData["Accumulative"] = pun.accumulative ? "1" : "0";

        dialogData[sectionName] = sectionData;
    }

    // Create SelectPunishment dialog
    qDebug() << "[DEBUG] Opening SelectPunishment dialog with" << dialogData.size() << "punishment sections";
    SelectPunishment selectPunishmentDialog(this, dialogData);
    selectPunishmentDialog.exec();
}

void CyberDom::openSelectPopupDialog()
{
    if (checkInterruptableAssignments()) return;

    SelectPopup selectPopupDialog(this); // Create the SelectPopup dialog, passing the parent
    selectPopupDialog.exec(); // Show the dialog modally
}

void CyberDom::openListFlagsDialog()
{
    if (checkInterruptableAssignments()) return;

    ListFlags listFlagsDialog(this, &flags);    // Create the ListFlags dialog, passing the parent
    connect(&listFlagsDialog, &ListFlags::flagRemoveRequested, this, &CyberDom::removeFlag);
    listFlagsDialog.exec(); // Show the dialog modally
}

void CyberDom::openSetFlagsDialog()
{
    if (checkInterruptableAssignments()) return;

    SetFlags setFlagsDialog(this); // Create the SetFlags dialog, passing the parent
    connect(&setFlagsDialog,&SetFlags::flagSetRequested, this, &CyberDom::setFlag);
    setFlagsDialog.exec(); // Show the dialog modally
}

void CyberDom::openDeleteAssignmentsDialog()
{
    if (checkInterruptableAssignments()) return;

    DeleteAssignments deleteAssignmentsDialog(this); // Create the DeleteAssignments dialog, passing the parent
    deleteAssignmentsDialog.exec(); // Show the dialog modally
}

QString CyberDom::promptForIniFile() {
    // Determine a good starting directory for the file dialog
    QString startDir = QDir::currentPath();
    QString scriptsDir = startDir + "/scripts";
    
    // If scripts directory exists, start there
    if (QDir(scriptsDir).exists()) {
        startDir = scriptsDir;
    }
    
    // Show file selection dialog with a clear title
    QString filePath = QFileDialog::getOpenFileName(this,
                                                    tr("Select CyberDom Script File"),
                                                    startDir,
                                                    tr("Script Files (*.ini);;All Files (*)"));

    if (filePath.isEmpty()) {
        QMessageBox::warning(this, "No File Selected", 
                            "You need to select a valid script file (.ini) to proceed with CyberDom.");
    }

    return filePath;
}

void CyberDom::saveIniFilePath(const QString &filePath) {
    QSettings settings(QCoreApplication::applicationDirPath() + "/cyberdom_settings.ini", QSettings::IniFormat);
    settings.setValue("SelectedIniFile", filePath);
}

QString CyberDom::loadIniFilePath() {
    QSettings settings(QCoreApplication::applicationDirPath() + "/cyberdom_settings.ini", QSettings::IniFormat);
    return settings.value("SelectedIniFile", "").toString();
}

void CyberDom::initializeIniFile() {
    // Check if this is a fresh start after reset
    QSettings appSettings("Desire_Games", "CyberDom");
    bool freshStart = appSettings.value("FreshStart", false).toBool();
    
    // Check for flag file (additional method to detect fresh start)
    QString flagFilePath = QDir::currentPath() + "/.fresh_start";
    QFile flagFile(flagFilePath);
    bool flagFileExists = flagFile.exists();
    
    // If flag file exists, remove it after detection
    if (flagFileExists) {
        flagFile.remove();
        freshStart = true;
        qDebug() << "[DEBUG] Fresh start flag file detected and removed";
    }
    
    // Clear the fresh start flag in settings
    if (freshStart) {
        appSettings.setValue("FreshStart", false);
        appSettings.sync();
        qDebug() << "[DEBUG] Fresh start flag cleared from settings";
    }
    
    QString iniFilePath = loadIniFilePath();

    // Validate that the stored ini file path actually exists
    if (!iniFilePath.isEmpty() && !QFile::exists(iniFilePath)) {
        QMessageBox::warning(this,
                             tr("Script Not Found"),
                             tr("The previously selected script could not be found. Please locate it again."));
        iniFilePath = promptForIniFile();
        if (!iniFilePath.isEmpty()) {
            saveIniFilePath(iniFilePath);
        }
    }

    qDebug() << "\n[DEBUG] ============ INI FILE DEBUG ============";
    qDebug() << "[DEBUG] Initial iniFilePath from settings: " << iniFilePath;
    qDebug() << "[DEBUG] Fresh start detected: " << (freshStart ? "Yes" : "No");

    // If this is a fresh start or no ini file path is stored, prompt for a new file
    if (freshStart || iniFilePath.isEmpty()) {
        iniFilePath = promptForIniFile();

        // Save the path for future launches if the user selected a file
        if (!iniFilePath.isEmpty()) {
            saveIniFilePath(iniFilePath);
        }
    }

    qDebug() << "[DEBUG] Loading script file: " << iniFilePath;
    qDebug() << "[DEBUG] File exists: " << QFile::exists(iniFilePath);
    qDebug() << "[DEBUG] File size: " << QFileInfo(iniFilePath).size() << " bytes";

    qDebug() << "[DEBUG] ============ END INI FILE DEBUG ============\n";

    // Use new parsing method
    loadAndParseScript(iniFilePath);
}

void CyberDom::updateStatus(const QString &newStatus) {
    if (newStatus.isEmpty()) {
        qDebug() << "Warning: Attempted to set an empty status.";
        return;
    }

    if (scriptParser) {
        QString proc = scriptParser->getScriptData().eventHandlers.beforeStatusChange;
        if (!proc.isEmpty())
            runProcedure(proc);
    }

    // Store previous status for events (if needed)
    QString previousStatus = currentStatus;

    currentStatus = newStatus;

    // Convert underscores to spaces
    QString formattedStatus = newStatus;
    formattedStatus.replace("_", " ");

    QStringList words = formattedStatus.split(" ");
    for (QString &word : words) {
        if (!word.isEmpty()) {
            word[0] = word[0].toUpper(); // Capitalize first letter
        }
    }
    formattedStatus = words.join(" ");

    // Update the label on the main window
    ui->lbl_Status->setText("Status: " + formattedStatus);

    // Update the text box with the Status text
    updateStatusText();

    // Save the status in the user settings
    QSettings settings(settingsFile, QSettings::IniFormat);
    settings.setValue("User/CurrentStatus", currentStatus);

    qDebug() << "Updated Status Label: " << formattedStatus;

    StatusSection status = scriptParser->getStatus(currentStatus);
    updateSigninWidgetsVisibility(status);

    if (scriptParser) {
        QString proc = scriptParser->getScriptData().eventHandlers.afterStatusChange;
        if (!proc.isEmpty())
            runProcedure(proc);
    }
}

void CyberDom::updateStatusText() {
    QStringList topLines;
    QStringList middleLines;
    QStringList bottomLines;

    if (!scriptParser) {
        ui->statusLabel->setText("");
        return;
    }

    const ScriptData &data = scriptParser->getScriptData();

    // --- Populate TopText ---
    for (const QString &line : data.general.topText) {
        topLines.append(replaceVariables(line));
    }

    // --- Populate BottomText ---
    for (const QString &line : data.general.bottomText) {
        bottomLines.append(replaceVariables(line));
    }

    // --- Populate Middle Content ---

    // Get Status Text
    if (data.statuses.contains(currentStatus)) {
        middleLines.append(data.statuses.value(currentStatus).statusTexts);
    }

    // Add Flag Text
    QMapIterator<QString, FlagData> flagIter(flags);
    while (flagIter.hasNext()) {
        flagIter.next();
        if (!flagIter.value().text.isEmpty()) {
            middleLines.append(flagIter.value().text);
        }
    }

    // Add Started Assignment Text (Jobs & Punishments)
    QStringList assignmentTexts;
    const auto &punishmentDefs = data.punishments;
    const auto &jobDefs = data.jobs;
    QSettings settings(settingsFile, QSettings::IniFormat);

    for (const QString &assignmentName : activeAssignments) {
        QString lowerName = assignmentName.toLower();
        const PunishmentDefinition *ptrDef = getPunishmentDefinition(assignmentName);
        bool isPun = (ptrDef != nullptr);
        QString startFlagName = (isPun ? "punishment_" : "job_") + assignmentName + "_started";

        if (isFlagSet(startFlagName)) {
            QStringList rawLines;
            QString minTimeStr;
            QString estimateStr;

            if (isPun) {
                const PunishmentDefinition &punDef = *ptrDef;
                rawLines.append(punDef.statusTexts);
                minTimeStr = punDef.duration.minTimeStart;
                estimateStr = punDef.estimate;
            } else if (jobDefs.contains(lowerName)) {
                const JobDefinition &jobDef = jobDefs.value(lowerName);
                if (!jobDef.text.isEmpty()) rawLines.append(jobDef.text);
                rawLines.append(jobDef.statusTexts);
                minTimeStr = jobDef.duration.minTimeStart;
                estimateStr = jobDef.estimate;
            }

            QDateTime start = settings.value("Assignments/" + assignmentName + "_start_time").toDateTime();
            QString runTimeStr = "00:00:00";
            if (start.isValid()) {
                qint64 elapsedSecs = start.secsTo(internalClock);
                if (elapsedSecs < 0) elapsedSecs = 0;
                runTimeStr = QTime(0, 0).addSecs(elapsedSecs).toString("HH:mm:ss");
            }

            // Resolve the estimate string if it's a variable
            if (estimateStr.startsWith("!") || estimateStr.startsWith("#")) {
                estimateStr = scriptParser->getVariable(estimateStr.mid(1));
            }

            for (QString line : rawLines) {
                if (line.isEmpty()) continue;
                line.replace("{!zzMinTime}", minTimeStr);
                line.replace("{!zzRunTime}", runTimeStr);
                assignmentTexts.append(line);
            }
        }
    }

    if (!assignmentTexts.isEmpty()) {
        middleLines.append("");
        middleLines.append("Assignments");
        middleLines.append(assignmentTexts);
    }

    // --- Assemble and Set Text ---

    QString finalTop = topLines.join("\n");
    QString finalMiddle = middleLines.join("\n");
    QString finalBottom = bottomLines.join("\n");

    finalMiddle.replace("%instructions", lastInstructions);
    finalMiddle.replace("%clothing", lastClothingInstructions);
    finalMiddle = replaceVariables(finalMiddle);

    finalTop.replace("\n", "<br>");
    finalMiddle.replace("\n", "<br>");
    finalBottom.replace("\n", "<br>");

    QString finalText = "";
    if (!finalTop.isEmpty()) {
        finalText += finalTop;
    }
    if (!finalMiddle.isEmpty()) {
        if (!finalText.isEmpty()) finalText += "<br><br>";
        finalText += finalMiddle;
    }
    if (!finalBottom.isEmpty()) {
        if (!finalText.isEmpty()) finalText += "<br><br>";
        finalText += finalBottom;
    }

    ui->statusLabel->setText(finalText);
}

void CyberDom::updateInstructions(const QString &instructions) {
    lastInstructions = instructions;
    updateStatusText(); // Refresh the status text if needed
}

void CyberDom::updateClothingInstructions(const QString &instructions) {
    lastClothingInstructions = instructions;
    updateStatusText(); // Refresh status text if needed
}

void CyberDom::setFlag(const QString &flagName, int durationMinutes) {
    // Create new flag data structure
    FlagData flagData;
    flagData.name = flagName;
    flagData.setTime = internalClock;

    QString setProcedure;

    // Get flag text and metadata from parsed script data
    if (scriptParser) {
        // Use lowercase name for lookup
        QString lowerFlagName = flagName.toLower();
        const auto &flagDefs = scriptParser->getScriptData().flagDefinitions;

        if (flagDefs.contains(lowerFlagName)) {
            const FlagDefinition &flagDef = flagDefs.value(lowerFlagName);

            // Set Text
            if (!flagDef.textLines.isEmpty()) {
                flagData.text = flagDef.textLines.join("\n");
            }

            // Set expiry from definition's DurationMin/Max
            if (durationMinutes == 0 && (!flagDef.durationMin.isEmpty() || !flagDef.durationMax.isEmpty())) {
                // Reconstruct the duration range string for parseTimeRangeToSeconds
                QString durationRange = flagDef.durationMin;
                if (!flagDef.durationMax.isEmpty() && !flagDef.durationMax.isEmpty()) {
                    durationRange += "," + flagDef.durationMax;
                }

                int secs = parseTimeRangeToSeconds(durationRange);
                if (secs > 0)
                    flagData.expiryTime = internalClock.addSecs(secs);
            }

            // Handle groups (splits the group string from struct)
            if (!flagDef.group.isEmpty()) {
                const QStringList rawGroups = flagDef.group.split(',', Qt::SkipEmptyParts);
                flagData.groups.clear();
                for (const QString &grp : rawGroups)
                    flagData.groups.append(grp.trimmed());
            }

            // Get SetProcedure name
            setProcedure = flagDef.setProcedure;
        }
    }

    // Explicit duration (from function argument) overrides definition
    if (durationMinutes > 0)
        flagData.expiryTime = internalClock.addSecs(durationMinutes * 60);

    // Store the flag
    flags[flagName] = flagData;

    // Run SetProcedure if specified
    if (!setProcedure.isEmpty())
        runProcedure(setProcedure);

    // Update UI
    updateStatusText();
    qDebug() << "Flag set: " << flagName;
}

void CyberDom::removeFlag(const QString &flagName) {
    // Check if the flag is currently active
    if (flags.contains(flagName)) {

        // Get the RemoveProcedure from the parsed script data
        if (scriptParser) {
            QString lowerFlagName = flagName.toLower();
            const auto &flagDefs = scriptParser->getScriptData().flagDefinitions;

            // Check if a definition for this flag exists
            if (flagDefs.contains(lowerFlagName)) {
                const FlagDefinition &flagDef = flagDefs.value(lowerFlagName);

                // If a remove procedure is defined, run it
                if (!flagDef.removeProcedure.isEmpty()) {
                    runProcedure(flagDef.removeProcedure);
                }
            }
        }

        // Remove the flag from the active list
        flags.remove(flagName);
        updateStatusText();
        qDebug() << "Flag removed: " << flagName;
    }
}

void CyberDom::resetApplication() {
    // Confirm reset with the user
    int response = QMessageBox::warning(this, "Reset Application",
        "Are you sure you want to reset the application? This will delete all saved settings and restart the application.",
                                        QMessageBox::Yes | QMessageBox::No);

    if (response == QMessageBox::Yes) {
        // Clear organization/application settings
        QSettings appSettings("Desire_Games", "CyberDom");
        appSettings.clear();
        appSettings.sync();

        // Check if user settings file exists and delete it
        if (!settingsFile.isEmpty()) {
            QFile settingsFileObj(settingsFile);
            if (settingsFileObj.exists()) {
                if (!settingsFileObj.remove()) {
                    QMessageBox::warning(this, "Reset Warning", 
                        "Could not delete the settings file: " + settingsFile + 
                        "\nThe application will restart, but settings may persist.");
                } else {
                    qDebug() << "[INFO] Successfully deleted settings file: " << settingsFile;
                }
            }
        } else {
            qDebug() << "[WARNING] Settings file path is empty";
        }

        // Also try to find and delete any user_settings.ini in the script directory
        if (!currentIniFile.isEmpty()) {
            QFileInfo iniFileInfo(currentIniFile);
            QString userSettingsPath = iniFileInfo.absolutePath() + "/user_settings.ini";
            
            QFile userSettingsFile(userSettingsPath);
            if (userSettingsFile.exists()) {
                if (!userSettingsFile.remove()) {
                    QMessageBox::warning(this, "Reset Warning", 
                        "Could not delete user settings file: " + userSettingsPath + 
                        "\nThe application will restart, but some settings may persist.");
                } else {
                    qDebug() << "[INFO] Successfully deleted user settings file: " << userSettingsPath;
                }
            }
        }

        // Delete the stored session file if it exists
        if (!sessionFilePath.isEmpty()) {
            QFile sessionFile(sessionFilePath);
            if (sessionFile.exists()) {
                if (!sessionFile.remove()) {
                    QMessageBox::warning(this, "Reset Warning",
                                         "Could not delete the session file: " + sessionFilePath +
                                             "\nThe application will restart, but some session data may persist.");
                } else {
                    qDebug() << "[INFO] Successfully deleted session file: " << sessionFilePath;
                }
            }
        }

        // Clear paths so the destructor doesn't recreate the files
        settingsFile.clear();
        sessionFilePath.clear();

        // Explicitly clear the INI file path selection
        appSettings.setValue("SelectedIniFile", "");
        
        // Create a flag file to indicate a fresh start is needed
        QString flagFilePath = QDir::currentPath() + "/.fresh_start";
        QFile flagFile(flagFilePath);
        if (flagFile.open(QIODevice::WriteOnly)) {
            flagFile.close();
            qDebug() << "[INFO] Created fresh start flag file";
        }
        
        appSettings.setValue("FreshStart", true);
        appSettings.sync();
        qDebug() << "[INFO] Cleared selected INI file path setting and set fresh start flag";

        // Notify the user
        QMessageBox::information(this, "Application Successfully Reset", 
                                "The application will now restart and prompt you to select a new script file.");

        // Restart the application
        QCoreApplication::quit();
        QProcess::startDetached(QCoreApplication::applicationFilePath(), QStringList());
    }
}

QString CyberDom::replaceVariables(const QString &input) const {
    QString result = input;

    // Safety check. No parser, no replacements.
    if (!scriptParser) {
        return result;
    }

    const ScriptData &data = scriptParser->getScriptData();

    // 1. Replace "zz" variables
    result.replace("{$zzMaster}", masterVariable);

    // 2. Replace {$zzSubname} with a RANDOM name
    if (result.contains("{$zzSubname}")) {
        const QStringList& subNames = data.general.subNames;
        if (!subNames.isEmpty()) {
            int index;
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
            index = QRandomGenerator::global()->bounded(subNames.size());
#else
            static bool seeded = false;
            if (!seeded) {
                std::srand(static_cast<unsigned int>(std::time(nullptr)));
                seeded = true;
            }
            index = std::rand() % subNames.size();
#endif
            result.replace("{$zzSubname}", subNames.at(index));
        } else {
            result.replace("{$zzSubname}", "");
        }
    }

    // 3. Replace {$questionKey} variables
    for (auto it = questionAnswers.begin(); it != questionAnswers.end(); ++it) {
        QString var = "{$" + it.key() + "}";
        result.replace(var, it.value());
    }

    // --- 4. Replace {#variable} ---
    const auto& variables = data.stringVariables;
    QRegularExpression hashVarRx("\\{#([^}]+)\\}"); // Fixed regex
    QRegularExpressionMatchIterator hashIter = hashVarRx.globalMatch(result);
    QString tempResult;
    int lastIndex = 0;

    while (hashIter.hasNext()) {
        QRegularExpressionMatch match = hashIter.next();
        tempResult += result.mid(lastIndex, match.capturedStart() - lastIndex);
        QString varName = match.captured(1);
        QString value = variables.value(varName, "0");
        tempResult += value;
        lastIndex = match.capturedEnd();
    }
    tempResult += result.mid(lastIndex);
    result = tempResult; // 'result' now contains the {#var} replacements

    // --- 5. Replace {FlagName} ---
    // This pass now runs *on the new result*
    QString onText = data.general.flagOnText;
    QString offText = data.general.flagOffText;

    if (!onText.isEmpty() && !offText.isEmpty()) {
        // Your regex was also broken. It was missing a '['.
        QRegularExpression flagRx("\\{([^\\$#][^}]*)\\}"); // CORRECTED REGEX
        QRegularExpressionMatchIterator flagIter = flagRx.globalMatch(result);
        tempResult.clear(); // Clear the temp string
        lastIndex = 0;

        while (flagIter.hasNext()) {
            QRegularExpressionMatch match = flagIter.next();
            tempResult += result.mid(lastIndex, match.capturedStart() - lastIndex);
            QString name = match.captured(1).trimmed();
            QString replacement = isFlagSet(name) ? onText : offText;
            tempResult += replacement;
            lastIndex = match.capturedEnd();
        }
        tempResult += result.mid(lastIndex);
        result = tempResult; // 'result' now contains *both* replacements
    }

    return result;
}


void CyberDom::loadAndParseScript(const QString &filePath) {
    if (filePath.isEmpty()) {
        qDebug() << "[ERROR] No script file path provided.";
        return;
    }

    if (!QFile::exists(filePath)) {
        qDebug() << "[ERROR] Script file not found:" << filePath;
        QMessageBox::critical(this, "Script Not Found",
                              "The script file could not be located at:\n" + filePath);
        return;
    }

    qDebug() << "\n[DEBUG] Starting loadAndParseScript with file: " << filePath;

    // Create new script parser if needed
    if (!scriptParser) {
        scriptParser = new ScriptParser();
        qDebug() << "[DEBUG] Created new ScriptParser instance";
    }

    // Parse the script file
    if (!scriptParser->parseScript(filePath)) {
        qDebug() << "[ERROR] Failed to parse script file.";
        QMessageBox::critical(this, "[Script Error", "Failed to parse the script file. Please check that it's a valid script.");
        return;
    }

    // Debug information about parsed status sections
    QList<StatusSection> statuses = scriptParser->getStatusSections();
    qDebug() << "[DEBUG] Parsed " << statuses.size() << " status sections:";
    for (const StatusSection &status : statuses) {
        qDebug() << "[DEBUG] Status: " << status.name << " (Group: " << status.group << ")";
    }

    // Store the path to the current ini file
    this->currentIniFile = filePath;

    // Setup a separate settings file for user data
    QFileInfo iniFileInfo(currentIniFile);
    settingsFile = iniFileInfo.absolutePath() + "/user_settings.ini";

    // Apply the script settings to the application
    applyScriptSettings();

    // Initialize UI with script data
    initializeUiWithIniFile();

    // Update the status display
    setupInitialStatus();

    qDebug() << "Script successfully loaded and parsed from: " << filePath;
}

void CyberDom::applyScriptSettings() {
    if (!scriptParser) return;

    const ScriptData &data = scriptParser->getScriptData();

    // Apply general settings from the struct
    masterVariable = data.general.masterName;
    subNameVariable = data.general.subNames.join(", ");
    minMerits = data.general.minMerits;
    maxMerits = data.general.maxMerits;
    yellowMerits = data.general.yellowMerits;
    redMerits = data.general.redMerits;
    testMenuEnabled = scriptParser->isTestMenuEnabled();

    // Setup UI from general settings
    initializeProgressBarRange();
    setWindowTitle(QString("%1's Virtual Master").arg(masterVariable));

    // Handle Merits
    if (data.general.hideMerits) {
        ui->lbl_Merits->setVisible(false);
        ui->progressBar->setVisible(false);
    } else {
        ui->lbl_Merits->setVisible(true);
        ui->progressBar->setVisible(true);
    }

    ui->menuTest->menuAction()->setVisible(testMenuEnabled);
    qDebug() << "TestMenu Enabled:" << testMenuEnabled;

    // Handle AskPunishment
    QString askPunishmentStr = data.generalSettings.value("AskPunishment");
    if (!askPunishmentStr.isEmpty()) {
        QStringList values = askPunishmentStr.split(", ");
        if (values.size() == 2) {
            bool minOk, maxOk;
            askPunishmentMin = values[0].trimmed().toInt(&minOk);
            askPunishmentMax = values[1].trimmed().toInt(&maxOk);

            if (!minOk || !maxOk) {
                askPunishmentMin = 25;
                askPunishmentMax = 75;
            }
        }
    } else {
        askPunishmentMin = 25;
        askPunishmentMax = 75;
    }

    // Build Status Groups
    statusGroups.clear();
    for (const StatusDefinition &status : data.statuses.values()) {
        QString group = status.group.trimmed();
        if (group.isEmpty())
            continue;
        if (!statusGroups.contains(group)) {
            statusGroups[group] = QStringList();
        }
        statusGroups[group].append(status.name);
    }

    // Populate menus
    populateReportMenu();
    populateConfessMenu();
    populatePermissionMenu();

    // Timers
    qDebug() << "[Timer] Registering all stand-alone timers...";
    activeTimers.clear();
    const auto &timerDefs = data.timers;

    for (const TimerDefinition &def : timerDefs.values()) {
        // Convert string times to QTime objects

        // Start Time
        QString startRange = def.startTimeMin;
        if (!def.startTimeMax.isEmpty() && def.startTimeMax != def.startTimeMin) {
            startRange += "," + def.startTimeMax;
        }
        int startSecs = parseTimeRangeToSeconds(startRange);
        QTime startTime = QTime(0,0).addSecs(startSecs);

        // End time
        int endSecs = parseTimeRangeToSeconds(def.endTime);
        QTime endTime = QTime(0,0).addSecs(endSecs);

        // Create the TimerInstance that updateInternalClock uses
        TimerInstance timer;
        timer.name = def.name;
        timer.start = startTime;
        timer.end = endTime;
        timer.triggered = false;

        activeTimers.append(timer);
    }
    qDebug() << "[Timer] Registered" << activeTimers.size() << "timers.";
}

void CyberDom::setupInitialStatus() {
    // Get settings from the user settings file if it exists
    QSettings settings(settingsFile, QSettings::IniFormat);

    QString savedStatus = settings.value("User/CurrentStatus").toString();

    // If no saved status, check the init section for NewStatus
    if (savedStatus.isEmpty()) {
        QString iniStatus = scriptParser->getIniValue("init", "NewStatus");
        if (!iniStatus.isEmpty()) {
            savedStatus = iniStatus;
        } else {
            // Default to "Normal" if no status is defined
            savedStatus = "Normal";
        }
    }

    // Check if we have any valid status sections
    if (scriptParser && scriptParser->getStatusSections().isEmpty()) {
        qDebug() << "[ERROR] No status sections found in the script";
        QMessageBox::critical(this, "Error", "No status sections could be loaded from the script. Please check your script file.");
        
        // Use savedStatus as a fallback, but warn the user
        QMessageBox::warning(this, "Warning", "Using '" + savedStatus + "' as the default status, but functionality may be limited.");
    }

    // Update the current status
    currentStatus = savedStatus;

    // Update the UI
    updateStatusDisplay();
    updateAvailableActions();

    qDebug() << "Initial status set to: " << currentStatus;
}

void CyberDom::changeStatus(const QString &statusName, bool forceAsSubStatus)
{
    if (statusName.isEmpty()) {
        qDebug() << "[Status] Warning: Attempted to set an empty status.";
        return;
    }

    // --- Check for &LastStatus ---
    if (statusName == "&LastStatus") {
        returntoLastStatus();
        return;
    }

    if (!scriptParser) return;

    // --- Get Status Definition ---
    const auto &statuses = scriptParser->getScriptData().statuses;
    if (!statuses.contains(statusName.toLower())) {
        qDebug() << "[Status] ERROR: Status '" << statusName << "' not found in script data.";
        return;
    }
    const StatusDefinition &statusDef = statuses.value(statusName.toLower());

    // Run BeforeStatusChange Event ---
    QString proc = scriptParser->getScriptData().eventHandlers.beforeStatusChange;
    if (!proc.isEmpty())
        runProcedure(proc);

    // --- Handle History Stack ---
    bool isSubStatus = forceAsSubStatus || statusDef.isSubStatus;
    if (isSubStatus) {
        const auto &currentStatuses = scriptParser->getScriptData().statuses;
        if (currentStatuses.contains(currentStatus.toLower())) {
            if (!currentStatuses.value(currentStatus.toLower()).isSubStatus) {
                statusHistory.push(currentStatus);
                qDebug() << "[Status] Pushing" << currentStatus << "to history. New sub-status is" << statusName;
            }
        }
    } else {
        // This is a Primary Status. Clear the History.
        if (!statusHistory.isEmpty()) {
            qDebug() << "[Status] Clearing status history. New primary status is" << statusName;
            statusHistory.clear();
        }
    }

    // --- Set New Status ---
    QString oldStatus = currentStatus;
    currentStatus = statusDef.name;
    qDebug() << "Status changed from" << oldStatus << "to" << currentStatus;

    // --- Update UI and Save ---
    updateStatusDisplay();
    updateAvailableActions();

    // Save the new status to user settings
    QSettings settings(settingsFile, QSettings::IniFormat);
    settings.setValue("User/CurrentStatus", currentStatus);

    // --- Run AfterStatusChange Event ---
    proc = scriptParser->getScriptData().eventHandlers.afterStatusChange;
    if (!proc.isEmpty())
        runProcedure(proc);
}

void CyberDom::returntoLastStatus() {
    if (!statusHistory.isEmpty()) {
        // We have a status in history, pop it and change
        QString lastStatus = statusHistory.pop();
        qDebug() << "[Status] Returning to last status:" << lastStatus;
        changeStatus(lastStatus, false);
    } else {
        // No history, use DefaultStatus from [General]
        qDebug() << "[Status] Status history empty. Finding default status.";
        QString defaultStatus = "Normal"; // Hard fallback
        if (scriptParser) {
            QString scriptDefault = scriptParser->getScriptData().general.defaultStatus;
            if (!scriptDefault.isEmpty()) {
                defaultStatus = scriptDefault;
            }
        }
        changeStatus(defaultStatus, false);
    }
}

bool CyberDom::isInStatusGroup(const QString &groupName) {
    // Check if current status belongs to the given group
    return statusGroups.value(groupName).contains(currentStatus);
}

void CyberDom::updateStatusDisplay() {
    // Format the status name for display (replace underscores with spaces, capitalize words)
    QString formattedStatus = currentStatus;
    formattedStatus.replace("_", " ");

    QStringList words = formattedStatus.split(" ");
    for (QString &word : words) {
        if (!word.isEmpty()) {
            word[0] = word[0].toUpper(); // Capitalize the first letter
        }
    }
    formattedStatus = words.join(" ");

    // Update the label on the main window
    ui->lbl_Status->setText("Status: " + formattedStatus);

    // Update the text box with the Status text
    updateStatusText();

    StatusSection status = scriptParser->getStatus(currentStatus);
    updateSigninWidgetsVisibility(status);
}

void CyberDom::updateSigninWidgetsVisibility(const StatusSection &status) {
    if (!status.signinIntervalMin.isEmpty()) {
        signinRemainingSecs = parseTimeRangeToSeconds(status.signinIntervalMin + "," + status.signinIntervalMax);
        ui->timerLabel->setStyleSheet("");
        QTime t(0, 0);
        t = t.addSecs(signinRemainingSecs);
        ui->timerLabel->setText(t.toString("hh:mm:ss"));
        if (signinTimer)
            signinTimer->start(1000);
        ui->timerLabel->show();
        ui->resetTimer->show();
        ui->lbl_Timer->show();
    } else {
        if (signinTimer)
            signinTimer->stop();
        ui->timerLabel->hide();
        ui->resetTimer->hide();
        ui->lbl_Timer->hide();
    }
}

void CyberDom::updateAvailableActions() {
    if (!scriptParser) {
        qDebug() << "[ERROR] updateAvailableActions called, but scriptParser is null.";
        return;
    }

    // --- Get the current status definition ---
    const auto &statuses = scriptParser->getScriptData().statuses;
    QString lowerStatus = currentStatus.toLower();

    // Define default permissions in case status isn't found
    bool permsEnabled = true;
    bool confsEnabled = true;
    bool reportsEnabled = true;
    bool rulesEnabled = true;
    bool assignEnabled = true;
    bool reportsOnly = false;
    bool allowClothReport = true;

    if (statuses.contains(lowerStatus)) {
        // Status was found, use its specific properties
        const StatusDefinition &status = statuses.value(lowerStatus);

        permsEnabled = status.permissionsEnabled;
        confsEnabled = status.confessionsEnabled;
        reportsEnabled = status.reportsEnabled;
        rulesEnabled = status.rulesEnabled;
        assignEnabled = status.assignmentsEnabled;
        reportsOnly = status.reportsOnly;
        allowClothReport = status.allowClothReport;
    } else {
        qDebug() << "[WARNING] updateAvailableActions: Status '" << currentStatus << "' not found. Using default permissions.";
    }

    // --- Update UI based on status permissions ---
    ui->actionPermissions->setEnabled(permsEnabled);
    ui->actionAsk_for_Punishment->setEnabled(permsEnabled);
    ui->actionConfessions->setEnabled(confsEnabled);
    ui->actionReports->setEnabled(reportsEnabled);
    ui->menuRules->setEnabled(rulesEnabled);
    ui->menuAssignments->setEnabled(assignEnabled);
    ui->actionReport_Clothing->setEnabled(allowClothReport);

    // --- If ReportsOnly is true, disable everything except reports ---
    if (reportsOnly) {
        ui->actionPermissions->setEnabled(false);
        ui->actionAsk_for_Punishment->setEnabled(false);
        ui->actionConfessions->setEnabled(false);
        ui->menuRules->setEnabled(false);
        ui->menuAssignments->setEnabled(false);

        // Reports and Report_Clothing must be enabled if reportsOnly is true
        ui->actionReports->setEnabled(true);
        ui->actionReport_Clothing->setEnabled(true);
    }

    // --- Rebuild the dynamic menus to reflect the new status ---
    populateReportMenu();
    populateConfessMenu();
    populatePermissionMenu();
}

// void CyberDom::updateAvailableActions() {
//     // Get the current status data
//     StatusSection status = scriptParser->getStatus(currentStatus);

//     // Update UI based on status permissions
//     ui->actionPermissions->setEnabled(status.permissionsEnabled);
//     ui->actionAsk_for_Punishment->setEnabled(status.permissionsEnabled);
//     ui->actionConfessions->setEnabled(status.confessionsEnabled);
//     ui->actionReports->setEnabled(status.reportsEnabled);
//     ui->menuRules->setEnabled(status.rulesEnabled);
//     ui->menuAssignments->setEnabled(status.assignmentsEnabled);

//     // If ReportsOnly is true, disable everything except reports
//     if (status.reportsOnly) {
//         ui->actionPermissions->setEnabled(false);
//         ui->actionAsk_for_Punishment->setEnabled(false);
//         ui->actionConfessions->setEnabled(false);
//         ui->menuRules->setEnabled(false);
//         ui->menuAssignments->setEnabled(false);
//     }
// }

void CyberDom::executeStatusEntryProcedures(const QString &statusName) {
    // This would execute any procedures that should run when entering a status
    // For now, this is a placeholder that will be implemented later
    qDebug() << "Executing entry procedures for status: " << statusName;
}

int CyberDom::getMeritsFromIni() {
    return ui->progressBar->value();
}

void CyberDom::initializeUiWithIniFile() {
    bool isFirstRun = false;

    QSettings settings(settingsFile, QSettings::IniFormat);
    if (!settings.contains("User/Initialized")) {
        isFirstRun = true;
        settings.setValue("User/Initialized", true);
        settings.sync();
    }

    // This block handles setting initial merits/status from the script's [init] section
    if (scriptParser) {
        const ScriptData &data = scriptParser->getScriptData();

        int initialMerits = 0;

        // Check if merits are defined in the init section (merits = -1 is the "not set" default)
        if (data.init.merits != -1) {
            initialMerits = data.init.merits;
        } else {
            // Fallback to half of the max merits from the general section
            initialMerits = data.general.maxMerits / 2;
        }

        // Set the initial status if defined in [init]
        if (!data.init.newStatus.isEmpty()) {
            currentStatus = data.init.newStatus;
        }

        // Apply these initial values
        updateMerits(initialMerits);
        updateStatus(currentStatus);
    }

    if (isFirstRun) {
        if (scriptParser) {
            QString proc = scriptParser->getScriptData().eventHandlers.firstRun;
            if (!proc.isEmpty()) {
                qDebug() << "[FirstRun] Executing startup procedure:" << proc;
                runProcedure(proc);
            }
        }
    }

    updateStatusText();
}

void CyberDom::initializeProgressBarRange() {
    ui->progressBar->setMinimum(minMerits);
    ui->progressBar->setMaximum(maxMerits);

    qDebug() << "ProgressBar Range Set: Min=" << minMerits << "Max=" << maxMerits;
}

void CyberDom::updateProgressBarValue() {
    int merits = getMeritsFromIni(); // Reuse the previously defined method
    ui->progressBar->setValue(merits);
}

void CyberDom::updateMerits(int newMerits) {
    ui->progressBar->setValue(newMerits);
    ui->lbl_Merits->setText("Merits: " + QString::number(newMerits));

    // Check for Red first, as it has the highest priority
    if (redMerits != -1 && newMerits < redMerits) {
        // Score is below red threshold
        ui->progressBar->setStyleSheet(
                    "QProgressBar::chunk { background-color: #f44336; }"
        );
    } else if (yellowMerits != -1 && newMerits < yellowMerits) {
        // Score is below yellow threshold
        ui->progressBar->setStyleSheet(
                    "QProgressBar::chunk { background-color: #FFC107; }"
        );
    } else {
        // Score is normal
        ui->progressBar->setStyleSheet(
                    "QProgressBar::chunk { background-color: #4CAF50; }"
        );
    }

    qDebug() << "Merits updated to:" << newMerits;
}


QStringList CyberDom::getAvailableJobs() {
    QStringList jobList;

    // Safely check at the top
    if (!scriptParser) {
        qDebug() << "[ERROR] getAvailableJobs called, but scriptParser is not initialized.";
        return jobList;
    }

    // Get the 'jobs' map directly from the parsed ScriptData
    const auto &jobsMap = scriptParser->getScriptData().jobs;

    jobList = jobsMap.keys();

    qDebug() << "[DEBUG] Loaded Job List: " << jobList;
    return jobList;
}

void CyberDom::assignScheduledJobs() {
    if (!scriptParser) return;

    QDate today = internalClock.date();
    QString currentDayName = today.toString("dddd");
    QSettings settings(settingsFile, QSettings::IniFormat);

    const auto& jobs = scriptParser->getScriptData().jobs;
    for (const JobDefinition &job : jobs) {

        // Don't assign a job that's already active
        if (activeAssignments.contains(job.name)) {
            continue;
        }

        if (job.oneTime) {
            QString oneTimeKey = QString("JobCompletion/%1_oneTimeDone").arg(job.name);
        }

        bool shouldRunToday = false;

        // --- 1. Check Run= logic (Days of the week) ---
        if (job.runDays.contains("Daily", Qt::CaseInsensitive)) {
            shouldRunToday = true;
        } else {
            for (const QString &runDay : job.runDays) {
                if (currentDayName.compare(runDay, Qt::CaseInsensitive) == 0) {
                    shouldRunToday = true;
                    break;
                }
            }
        }
        for (const QString &noRunDay : job.noRunDays) {
            if (currentDayName.compare(noRunDay, Qt::CaseInsensitive) == 0) {
                shouldRunToday = false;
                break;
            }
        }

        // --- 2. Check Interval= / FirstInterval= logic ---
        // We only do this if the Run= logic didn't already trigger it
        if (!shouldRunToday && (job.intervalMin > 0 || job.firstIntervalMin > 0)) {

            QString lastDoneKey = QString("JobCompletion/%1_lastDone").arg(job.name);
            QDate lastDoneDate = QDate::fromString(settings.value(lastDoneKey).toString(), Qt::ISODate);

            if (!lastDoneDate.isValid()) {
                // --- A: JOB HAS NEVER BEEN COMPLETED ---

                // Check if we have a FirstInterval defined
                if (job.firstIntervalMin > 0) {

                    // Check if we've already *set* the first due date
                    QString firstDueKey = QString("JobCompletion/%1_firstDue").arg(job.name);
                    QDate firstDueDate = QDate::fromString(settings.value(firstDueKey).toString(), Qt::ISODate);

                    if (!firstDueDate.isValid()) {
                        // This is the first time we're seeing this job. Set its initial due date.
                        int intervalDays;
                        if (job.firstIntervalMax > job.firstIntervalMin) {
                            intervalDays = QRandomGenerator::global()->bounded(job.firstIntervalMin, job.firstIntervalMax + 1);
                        } else {
                            intervalDays = job.firstIntervalMin;
                        }

                        firstDueDate = today.addDays(intervalDays);
                        settings.setValue(firstDueKey, firstDueDate.toString(Qt::ISODate));
                        qDebug() << "[Scheduler] FirstInterval set for" << job.name << ". Due on:" << firstDueDate.toString(Qt::ISODate);
                    }

                    // Now check if we are on or after that due date
                    if (today >= firstDueDate) {
                        shouldRunToday = true;
                    }

                } else if (job.intervalMin > 0) {
                    // No FirstInterval, but has a regular Interval. Assign it now.
                    shouldRunToday = true;
                }

            } else {
                // --- B: JOB HAS BEEN COMPLETED BEFORE ---
                // Ignore FirstInterval forever. Use regular Interval.

                if (job.intervalMin > 0) {
                    int intervalDays;
                    if (job.intervalMax > job.intervalMin) {
                        intervalDays = QRandomGenerator::global()->bounded(job.intervalMin, job.intervalMax + 1);
                    } else {
                        intervalDays = job.intervalMin;
                    }

                    QDate dueDate = lastDoneDate.addDays(intervalDays);

                    if (today >= dueDate) {
                        shouldRunToday = true;
                    }
                }
            }
        }

        // --- 3. Assign the job if needed ---
        if (shouldRunToday) {
            addJobToAssignments(job.name, true);
            qDebug() << "[Scheduler] Job Auto-Assigned (" << currentDayName << "): " << job.name;
        }
    }
}

void CyberDom::assignJobFromTrigger(QString section) {
    // Safety check
    if (!scriptParser) {
        qDebug() << "[ERROR] assignJobFromTrigger: scriptParser is null";
        return;
    }

    // Get raw data for specified section
    QMap<QString, QStringList> sectionData = scriptParser->getRawSectionData(section);

    // Check if the section was found and has data
    if (sectionData.isEmpty()) {
        qDebug() << "[assignJobFromTrigger] Section not found or empty:" << section;
        return;
    }

    // Check if the section contains a "Job" key
    if (sectionData.contains("Job")) {
        // Get a list of jobs
        QStringList jobs = sectionData.value("Job");

        if (!jobs.isEmpty()) {
            // Get the first job name from the list
            QString jobName = jobs.first().trimmed();

            if (!jobName.isEmpty() && !activeAssignments.contains(jobName)) {
                addJobToAssignments(jobName);
                qDebug() << "[DEBUG] Assigned Job:" << jobName;
            }
        }
    }
}

void CyberDom::addJobToAssignments(QString jobName, bool isAutoAssign)
{
    if (activeAssignments.contains(jobName)) {
        qDebug() << "[DEBUG] Job already exists in active assignments: " << jobName;
        return;
    }

    activeAssignments.insert(jobName);
    qDebug() << "[DEBUG] Job added to active assignments: " << jobName;

    QDateTime deadline;
    bool deadlineSet = false;

    // Check if the script parser and the job definition exist
    if (scriptParser && scriptParser->getScriptData().jobs.contains(jobName.toLower())) {

        // Get the structured job definition
        const JobDefinition &jobDef = scriptParser->getScriptData().jobs.value(jobName.toLower());

        // Check for Respite
        if (!jobDef.respite.isEmpty()) {
            QString respiteStr = jobDef.respite;

            // Check if it's a variable
            if (respiteStr.startsWith("!") || respiteStr.startsWith("#")) {
                // Get the variable's value, which should be "hh:mm"
                respiteStr = scriptParser->getVariable(respiteStr.mid(1));
            }

            // Now, parse the hh:mm string
            QStringList respiteParts = respiteStr.split(":");
            if (respiteParts.size() >= 2) {
                int hours = respiteParts[0].toInt();
                int minutes = respiteParts[1].toInt();
                deadline = internalClock.addSecs(hours * 3600 + minutes * 60);
                deadlineSet = true;
                qDebug() << "[DEBUG] Job deadline set from Respite: " << deadline.toString("MM-dd-yyyy hh:mm AP");
            }
        }

        // --- Check for EndTime ---
        if (!deadlineSet && !jobDef.endTimeMin.isEmpty()) {
            QString endTimeRange = jobDef.endTimeMin;
            if (!jobDef.endTimeMax.isEmpty()) {
                endTimeRange += "," + jobDef.endTimeMax;
            }

            int endTimeSecs = parseTimeRangeToSeconds(endTimeRange);
            QTime endTime = QTime(0,0).addSecs(endTimeSecs);

            if (endTime.isValid()) {
                deadline = QDateTime(internalClock.date(), endTime);
                if (deadline <= internalClock)
                    deadline = deadline.addDays(1);
                deadlineSet = true;
            } else {
                qDebug() << "[DEBUG] Invalid EndTime format for job: " << jobName;
            }
        }

        // --- Apply Default Deadline (48 hours) ---
        if (!deadlineSet) {
            // Neither Respite nor EndTime was set, apply the 48-hour default
            deadline = internalClock.addDays(2);
            deadlineSet = true;
            qDebug() << "[DEBUG] No Respite or EndTime found. Defaulting to 48-hour deadline.";
        }

        jobDeadlines[jobName] = deadline;

        qDebug() << "[DEBUG] Job deadline set: " << jobName << " - "
                 << deadline.toString("MM-dd-yyyy hh:mm AP");

        // Check for ExpireAfter
        if (!jobDef.expireAfterMin.isEmpty()) {
            QString minStr = jobDef.expireAfterMin;
            QString maxStr = jobDef.expireAfterMax;

            // Helper lambda to resolve variables (handles hh:mm or !myVar)
            auto getRange = [&](QString s) -> QString {
                if (s.startsWith("!") || s.startsWith("#")) {
                    // It's a variable, get its value
                    // The variable itself should store an "hh:mm" string
                    return scriptParser->getVariable(s.mid(1));
                }
                return s;
            };

            QString minRange = getRange(minStr);
            QString maxRange = getRange(maxStr);

            QString finalRange = minRange;
            if (maxRange != minRange) {
                finalRange += "," + maxRange;
            }

            int expireSec = parseTimeRangeToSeconds(finalRange);
            if (expireSec > 0) {
                jobExpiryTimes[jobName] = deadline.addSecs(expireSec);
            }
        }

        // Check for RemindInterval
        QString remindRange;
        if (!jobDef.remindIntervalMin.isEmpty() || !jobDef.remindIntervalMax.isEmpty()) {
            // Use Job's specific interval
            remindRange = jobDef.remindIntervalMin.isEmpty() ? jobDef.remindIntervalMax : jobDef.remindIntervalMin;
            if (!jobDef.remindIntervalMin.isEmpty() && !jobDef.remindIntervalMax.isEmpty()) {
                remindRange = jobDef.remindIntervalMin + "," + jobDef.remindIntervalMax;
            }
        } else if (!scriptParser->getScriptData().general.globalRemindInterval.isEmpty()) {
            // Fallback to [General] interval
            remindRange = scriptParser->getScriptData().general.globalRemindInterval;
        } else {
            // Fallback to 24-hour default
            remindRange = "24:00";
        }

        int intervalSec = parseTimeRangeToSeconds(remindRange);
        if (intervalSec > 0)
            jobRemindIntervals[jobName] = intervalSec;

        // Check for LateMerits
        if (!jobDef.lateMeritsMin.isEmpty()) {
            QString minStr = jobDef.lateMeritsMin;
            QString maxStr = jobDef.lateMeritsMax;

            // Helper lambda to resolve counters
            auto getValue = [&](QString s) -> int {
                if (s.startsWith("#")) {
                    return scriptParser->getVariable(s.mid(1)).toInt();
                }
                return s.toInt();
            };

            int minVal = getValue(minStr);
            int maxVal = getValue(maxStr);

            QString range = QString::number(minVal);
            if (maxVal > minVal) {
                range += "," + QString::number(maxVal);
            }

            jobLateMerits[jobName] = randomIntFromRange(range);
        }
    } else {
        // Fallback if parser or jobDef doesn't exist
        setDefaultDeadlineForJob(jobName);
    }

    // --- Announcement Logic ---
    if (scriptParser && scriptParser->getScriptData().jobs.contains(jobName.toLower())) {
        const ScriptData &data = scriptParser->getScriptData();
        const JobDefinition &jobDef = data.jobs.value(jobName.toLower());
        const GeneralSettings &general = data.general;

        // Run the AnnounceProcedure, regardless of message settings
        if (!jobDef.announceProcedure.isEmpty()) {
            runProcedure(jobDef.announceProcedure);
        }

        bool showMessage = false;

        // Check Status-level silence
        if (isAutoAssign) {
            if (data.statuses.contains(currentStatus.toLower())) {
                const StatusDefinition &statusDef = data.statuses.value(currentStatus.toLower());
                if (!statusDef.autoAssignMessage) {
                    // Status explicitly blocks auto-assign messages, so we stop here.
                    emit jobListUpdated();
                    qDebug() << "[DEBUG] jobListUpdated Signal Emitted!";
                    return;
                }
            }
        }

        // Check Job-level settings
        if (jobDef.announce == 1) {
            // Announce=1 forces the message, overriding global silence
            showMessage = true;
        } else if (jobDef.announce == 0) {
            // Announce=0 forces silence, overriding global 'on'
            showMessage = false;
        }

        // Check General-level settings (if job is unset)
        else if (general.announceJobs) {
            // Announce is unset (-1), so check global. Global is ON.
            showMessage = true;
        }

        // (If jobDef.announce is -1 AND general.announceJobs is false, showMessage remains (false)

        if (showMessage) {
            if (isInterruptAllowed()) {
                QString title = jobDef.title.isEmpty() ? jobDef.name : jobDef.title;
                QMessageBox::information(this, "New Assignment", "A new job has been added to your assignments: " + title);
            }
        }
    }

    emit jobListUpdated();
    qDebug() << "[DEBUG] jobListUpdated Signal Emitted!";
}

void CyberDom::checkPunishments() {
    QDateTime now = internalClock;

    if (!scriptParser) return; // Safety Check

    const ScriptData &data = scriptParser->getScriptData();

    for (const QString &name : activeAssignments) {
        if (!jobDeadlines.contains(name))
            continue;

        QDateTime deadline = jobDeadlines[name];
        if (now < deadline)
            continue;

        // Determine the type of assignment and get its properties
        const PunishmentDefinition* punDef = getPunishmentDefinition(name);
        bool isPun = (punDef != nullptr);

        // Jobs don't use suffixes, simple lookup is fin
        bool isJob = data.jobs.contains(name.toLower());

        if (!isPun && !isJob) { continue; }

        // --- Get properties (if they exist)
        QString remindPenaltyRange;
        QString remindPenaltyGroup;
        QString expirePenaltyRange;
        QString expirePenaltyGroup;
        QString expireProcedure;
        QString remindProcedure;

        if (isJob) {
            const JobDefinition &jobDef = data.jobs.value(name.toLower());
            remindProcedure = jobDef.remindProcedure;

            // --- Remind Penalty ---
            if (jobDef.remindPenaltyMin > 0 || jobDef.remindPenaltyMax > 0) {
                // Use Job's specific penalty
                remindPenaltyRange = QString::number(jobDef.remindPenaltyMin);
                if (jobDef.remindPenaltyMax > jobDef.remindPenaltyMin) {
                    remindPenaltyRange += "," + QString::number(jobDef.remindPenaltyMax);
                }
                remindPenaltyGroup = jobDef.remindPenaltyGroup;
            } else if (!data.general.globalRemindPenalty.isEmpty()) {
                // Fallback to [General] penalty
                remindPenaltyRange = data.general.globalRemindPenalty;
                remindPenaltyGroup = data.general.globalRemindPenaltyGroup;
            }
            // (If both are empty, range empty and no punishment is applied)

            // --- Expiry ---
            if (jobDef.expirePenaltyMin > 0 || jobDef.expirePenaltyMax > 0) {
                expirePenaltyRange = QString::number(jobDef.expirePenaltyMin);
                if (jobDef.expirePenaltyMax > jobDef.expirePenaltyMin) {
                    expirePenaltyRange += "," + QString::number(jobDef.expirePenaltyMax);
                }
            }

            if (!jobDef.expirePenaltyGroup.isEmpty()) {
                // Use the Job's specific group
                expirePenaltyGroup = jobDef.expirePenaltyGroup;
            } else {
                // Fallback to the [General] group
                expirePenaltyGroup = data.general.globalExpirePenaltyGroup;
            }

            expireProcedure = jobDef.expireProcedure;

        } else if (isPun) {
            const PunishmentDefinition &punDef = data.punishments.value(name.toLower());
            remindProcedure = punDef.remindProcedure;

            // --- Remind Penalty ---
            if (punDef.remindPenaltyMin > 0 || punDef.remindPenaltyMax > 0) {
                // Use Punishment's specific penalty
                remindPenaltyRange = QString::number(punDef.remindPenaltyMin);
                if (punDef.remindPenaltyMax > punDef.remindPenaltyMin) {
                    remindPenaltyRange += "," + QString::number(punDef.remindPenaltyMax);
                }
                remindPenaltyGroup = punDef.remindPenaltyGroup;
            } else if (!data.general.globalRemindPenalty.isEmpty()) {
                // Fallback to [General] penalty
                remindPenaltyRange = data.general.globalRemindPenalty;
                remindPenaltyGroup = data.general.globalRemindPenaltyGroup;
            }
        }

        QSettings settings(settingsFile, QSettings::IniFormat);

        if (!expiredAssignments.contains(name)) {
            expiredAssignments.insert(name);

            // Apply late merits penalty
            if (jobLateMerits.contains(name)) {
                int current = settings.value("User/Merits", maxMerits / 2).toInt();
                current -= jobLateMerits[name];
                if (current < minMerits) current = minMerits;
                settings.setValue("User/Merits", current);
                updateMerits(current);
            }
            // Set up the next reminder
            if (jobRemindIntervals.contains(name))
                jobNextReminds[name] = now.addSecs(jobRemindIntervals[name]);
        } else if (jobRemindIntervals.contains(name) && now >= jobNextReminds.value(name)) {
            if (isInterruptAllowed()) {
                // The assignment is already expired, and it's time for another reminder
                QMessageBox::information(this, "Assignment Late", QString("You are late completing %1").arg(name));
            }

            // Run the RemindProcedure
            if (!remindProcedure.isEmpty()) {
                runProcedure(remindProcedure);
            }

            // Apply reminder penalty (uses new variables)
            if (!remindPenaltyRange.isEmpty()) {
                int sev = randomIntFromRange(remindPenaltyRange);
                if (sev > 0)
                    applyPunishment(sev, remindPenaltyGroup, QString());
            }

            jobNextReminds[name] = now.addSecs(jobRemindIntervals[name]);
        }

        // Check if the assignment has permanently expired
        if (jobExpiryTimes.contains(name) && now >= jobExpiryTimes[name]) {

            // Apply expiry penalty
            if (!expirePenaltyRange.isEmpty()) {
                int sev = randomIntFromRange(expirePenaltyRange);
                if (sev > 0)
                    applyPunishment(sev, expirePenaltyGroup, QString());
            }
            // Run expiry procedure
            if (!expireProcedure.isEmpty())
                runProcedure(expireProcedure);

            // Clean up the assignment
            activeAssignments.remove(name);
            jobDeadlines.remove(name);
            jobExpiryTimes.remove(name);
            jobRemindIntervals.remove(name);
            jobNextReminds.remove(name);
            jobLateMerits.remove(name);
            expiredAssignments.remove(name);
            emit jobListUpdated();
        }
    }
}

void CyberDom::addPunishmentToAssignments(const QString &punishmentName, int amount)
{
    if (amount <= 0) return;
    if (!scriptParser) return;

    // 1. Get the base definition
    const PunishmentDefinition *punDef = getPunishmentDefinition(punishmentName);
    if (!punDef) {
        qDebug() << "[WARNING] Punishment section not found in ScriptData:" << punishmentName;
        return;
    }

    QString targetInstance;
    QString baseName = punDef->name.toLower(); // Normalize base name

    // 2. Check for an existing instance we can add to (Accumulative Logic)
    if (punDef->accumulative) {
        for (const QString &activeName : activeAssignments) {
            // Check if this active assignment is an instance of our punishment
            const PunishmentDefinition *activeDef = getPunishmentDefinition(activeName);
            if (activeDef && activeDef->name.toLower() == baseName) {

                // Check if it has room
                int currentAmt = punishmentAmounts.value(activeName);
                if (currentAmt + amount <= punDef->max) {
                    targetInstance = activeName;
                    break; // Found a slot!
                }
            }
        }
    }

    // 3. If no suitable existing instance, create a new one
    bool isNewInstance = false;
    if (targetInstance.isEmpty()) {
        isNewInstance = true;
        int counter = 1;
        targetInstance = punDef->name; // Start with base name

        // Find the next available name (spanking, spanking_2, spanking_3...)
        while (activeAssignments.contains(targetInstance)) {
            counter++;
            targetInstance = QString("%1_%2").arg(punDef->name).arg(counter);
        }

        activeAssignments.insert(targetInstance);
        qDebug() << "[DEBUG] Created new punishment instance:" << targetInstance;
    } else {
        qDebug() << "[DEBUG] Accumulating" << amount << "into existing:" << targetInstance;
    }

    // 4. Add the amount
    punishmentAmounts[targetInstance] += amount;

    // 5. Handle Deadline (Only for new instances or extending time-based ones)
    if (isNewInstance) {
        if (scriptParser) {
            QString proc = scriptParser->getScriptData().eventHandlers.punishmentGiven;
            if (!proc.isEmpty()) runProcedure(proc);
            if (!punDef->announceProcedure.isEmpty()) runProcedure(punDef->announceProcedure);
        }

        QDateTime deadline = internalClock;
        bool deadlineSet = false;

        // Respite
        if (!punDef->respite.isEmpty()) {
            QStringList respiteParts = punDef->respite.split(":");
            if (respiteParts.size() >= 2) {
                deadline = deadline.addSecs(respiteParts[0].toInt() * 3600 + respiteParts[1].toInt() * 60);
                deadlineSet = true;
            }
        }

        // ValueUnit Logic (Initial Calculation)
        if (!deadlineSet && !punDef->valueUnit.isEmpty()) {
            double val = punDef->value > 0 ? punDef->value : 1.0;
            int total = qRound(val * amount);

            if (punDef->valueUnit == "minute") deadline = deadline.addSecs(total * 60);
            else if (punDef->valueUnit == "hour") deadline = deadline.addSecs(total * 3600);
            else if (punDef->valueUnit == "day") deadline = deadline.addDays(total);
        }

        jobDeadlines[targetInstance] = deadline;

        // Remind Interval
        QString remindRange;
        if (!punDef->remindIntervalMin.isEmpty()) {
             remindRange = punDef->remindIntervalMin;
             if (!punDef->remindIntervalMax.isEmpty()) remindRange += "," + punDef->remindIntervalMax;
        } else if (!scriptParser->getScriptData().general.globalRemindInterval.isEmpty()) {
             remindRange = scriptParser->getScriptData().general.globalRemindInterval;
        } else {
             remindRange = "24:00";
        }
        int intervalSec = parseTimeRangeToSeconds(remindRange);
        if (intervalSec > 0) jobRemindIntervals[targetInstance] = intervalSec;

    } else {
        // Extending an existing TIME-BASED punishment
        // (We don't extend deadlines for repetitions, only for time units)
        if (!punDef->valueUnit.isEmpty() && punDef->valueUnit != "once") {
            QDateTime deadline = jobDeadlines.value(targetInstance, internalClock);
            double val = punDef->value > 0 ? punDef->value : 1.0;
            int total = qRound(val * amount);

            if (punDef->valueUnit == "minute") deadline = deadline.addSecs(total * 60);
            else if (punDef->valueUnit == "hour") deadline = deadline.addSecs(total * 3600);
            else if (punDef->valueUnit == "day") deadline = deadline.addDays(total);

            jobDeadlines[targetInstance] = deadline;
            qDebug() << "[DEBUG] Extended deadline for" << targetInstance;
        }
    }

    emit jobListUpdated();
}

void CyberDom::applyPunishment(int severity, const QString &group, const QString &message)
{
    // Choose a punishment based on severity and group
    QString punishmentName = selectPunishmentFromGroup(severity, group);
    if (punishmentName.isEmpty()) {
        qDebug() << "[Punish] No punishment found for severity" << severity << "in group" << group;
        return;
    }

    if (!scriptParser) return;

    const auto &punishments = scriptParser->getScriptData().punishments;
    QString lowerName = punishmentName.toLower();

    if (!punishments.contains(lowerName)) {
        qDebug() << "[ERROR] applyPunishment: Could not find definition for" << punishmentName;
        return;
    }
    const PunishmentDefinition &punDef = punishments.value(lowerName);

    QMap<QString, QStringList> rawData = scriptParser->getRawSectionData("punishment-" + punishmentName);
    bool hasValue = rawData.contains("value");

    double value = punDef.value;
    QString valueUnit = punDef.valueUnit.toLower();

    if (!hasValue && valueUnit == "once") value = 1.0;
    if (value <= 0) value = 1.0;

    int min = punDef.min;
    int max = punDef.max;
    if (max <= 0) max = 20;

    int totalUnits;
    if (valueUnit == "once" && !hasValue) {
        totalUnits = qMax(min, 1);
    } else {
        double amt = static_cast<double>(severity) / value;
        totalUnits = qRound(amt);
        if (totalUnits < min) totalUnits = min;
    }

    // --- Show the message ---
    if (!message.isEmpty()) {
        // A custom message was provided
        QMessageBox::information(this, "Punishment", replaceVariables(message));
    } else {
        // No custom message, generate a default one
        QString defaultMsg = QString("You have been punished with %1 %2 of %3.")
                .arg(totalUnits)
                .arg(valueUnit)
                .arg(punDef.title.isEmpty() ? punDef.name : punDef.title);
        QMessageBox::information(this, "Punishment", defaultMsg);
    }

    // Add the assignments
    if (valueUnit == "once") {
        addPunishmentToAssignments(punishmentName, totalUnits);
    } else {
        while (totalUnits > 0) {
            int amount = qMin(max, totalUnits);
            addPunishmentToAssignments(punishmentName, amount);
            totalUnits -= amount;
        }
    }
}

void CyberDom::executePunish(const QString &severityValue, const QString &message, const QString &group)
{
    if (!scriptParser) return;

    int severity = 0;
    QString valueToParse = severityValue;

    // Resolve the severity
    if (valueToParse.startsWith("#")) {
        // It's a counter. Get the counter's value
        valueToParse = scriptParser->getVariable(valueToParse.mid(1));
    }

    // Parse the resulting string
    severity = randomIntFromRange(valueToParse);

    // Apply the punishment
    if (severity > 0) {
        // Pass the message and group to the main punishment function
        applyPunishment(severity, group, message);
    }
}

QString CyberDom::selectPunishmentFromGroup(int severity, const QString &group)
{
    // This list will hold weighted entries
    QStringList weightedEligiblePunishments;

    if (!scriptParser) {
        qDebug() << "[ERROR] selectPunishmentFromGroup: scriptParser is null";
        return QString();
    }

    const auto &allPunishments = scriptParser->getScriptData().punishments;

    // Helper lambda to resolve counter strings to an integer
    auto getWeightValue = [&](QString s) -> int {
        if (s.isEmpty()) return 1;
        if (s.startsWith("#")) {
            return scriptParser->getVariable(s.mid(1)).toInt();
        }
        return s.toInt();
    };

    // --- Prepare Target Groups ---
    QStringList targetGroups;
    if (!group.isEmpty()) {
        // Split by comma and trim whitespace
        QStringList rawGroups = group.split(',', Qt::SkipEmptyParts);
        for (const QString &g : rawGroups) {
            targetGroups.append(g.trimmed().toLower());
        }
    }

    for (const PunishmentDefinition &punDef : allPunishments.values()) {

        bool suitable = true;

        // --- Check GroupOnly ---
        if (targetGroups.isEmpty() && punDef.groupOnly) {
            suitable = false;
        }

        // --- Check Group Match ---
        if (suitable && !targetGroups.isEmpty()) {
            bool groupMatch = false;
            for (const QString &punGroup : punDef.groups) {
                // Check if this punishment's group is in the requested target list
                if (targetGroups.contains(punGroup.trimmed().toLower())) {
                    groupMatch = true;
                    break;
                }
            }
            if (!groupMatch) {
                suitable = false;
            }
        }

        // --- Check Severity ---
        if (suitable) {
            if (severity < punDef.minSeverity) {
                suitable = false;
            }

            if (punDef.maxSeverity > 0 && severity > punDef.maxSeverity) {
                suitable = false;
            }
        }

        // --- Add to weighted list ---
        // Add the punishment 'finalWeight' number of times
        if (suitable) {
            int minWeight = getWeightValue(punDef.weightMin);
            int maxWeight = getWeightValue(punDef.weightMax);

            if (minWeight <= 0) minWeight = 1;
            if (maxWeight < minWeight) maxWeight = minWeight;

            int finalWeight = 1;
            if (minWeight == maxWeight) {
                finalWeight = minWeight;
            } else {
                finalWeight = QRandomGenerator::global()->bounded(minWeight, maxWeight + 1);
            }

            for (int i = 0; i < finalWeight; ++i) {
                weightedEligiblePunishments.append(punDef.name);
            }
        }
    }

    // --- Pick one from the weighted list ---
    if (!weightedEligiblePunishments.isEmpty()) {
        int index = QRandomGenerator::global()->bounded(weightedEligiblePunishments.size());
        return weightedEligiblePunishments.at(index);
    }

    return QString();
}

void CyberDom::checkFlagExpiry() {
    QDateTime now = internalClock;
    QMutableMapIterator<QString, FlagData> i(flags);

    while (i.hasNext()) {
        i.next();

        // Check if the flag is expired
        if (i.value().expiryTime.isValid() && i.value().expiryTime <= now) {
            QString flagName = i.key();

            // Get the flag definition from the script parser
            if (scriptParser) {
                QString lowerFlagName = flagName.toLower();
                const auto &flagDefs = scriptParser->getScriptData().flagDefinitions;

                if (flagDefs.contains(lowerFlagName)) {
                    const FlagDefinition &flagDef = flagDefs.value(lowerFlagName);

                    // Run the expire procedure if one is defined
                    if (!flagDef.expireProcedure.isEmpty()) {
                        runProcedure(flagDef.expireProcedure);
                    }

                    // Show expiry message if one is defined
                    if (!flagDef.expireMessage.isEmpty()) {
                        // Apply variable replacement to the message
                        QString message = replaceVariables(flagDef.expireMessage);
                        QMessageBox::information(this, "Flag Expired", message);
                    }
                }
            }

            // Directly remove the flag from the active map
            i.remove();
            updateStatusText();
            qDebug() << "Flag expired and removed: " << flagName;
        }
    }
}

void CyberDom::startAssignment(const QString &assignmentName, bool isPunishment, const QString &newStatus)
{
    if (hasActiveBlockingPunishment()) {
        QMessageBox::warning(this,
                             tr("Punishment Running"),
                             tr("Finish the current punishment before starting another task"));
        return;
    }

    // Remember the previous status if newStatus is specified
    QString previousStatus;
    if (!newStatus.isEmpty()) {
        previousStatus = currentStatus;
        changeStatus(newStatus, true);
    }

    QString startProcedure; // Store the procedure to run
    QString customStartFlag;
    QString beforeProcedure;

    if (!scriptParser) {
        qDebug() << "[ERROR] startAssignment: scriptParser is null!";
    } else {
        const ScriptData &data = scriptParser->getScriptData();
        QString lowerName = assignmentName.toLower();

        if (isPunishment) {
            if (data.punishments.contains(lowerName)) {
                const PunishmentDefinition &def = data.punishments.value(lowerName);
                startProcedure = def.startProcedure;
                customStartFlag = def.startFlag;
                beforeProcedure = def.beforeProcedure;
            } else {
                qDebug() << "[WARNING] Section not found for punishment: " << assignmentName;
            }
        } else {
            if (data.jobs.contains(lowerName)) {
                const JobDefinition &def = data.jobs.value(lowerName);
                startProcedure = def.startProcedure;
                customStartFlag = def.startFlag;
                beforeProcedure = def.beforeProcedure;
            } else {
                qDebug() << "[WARNING] Section not found for job: " << assignmentName;
            }
        }
    }

    // --- Run BeforeProcedure and check for zzDeny ---
    if (!beforeProcedure.isEmpty()) {
        runProcedure(beforeProcedure);

        // Check if the procedure set the zzDeny flag
        if (isFlagSet("zzDeny")) {
            qDebug() << "[StartAssignment] Start of" << assignmentName << "was denied by BeforeProcedure.";
            removeFlag("zzDeny");
            return;
        }
    }

    // Set the built-in "_started" flag
    QString startFlagName = (isPunishment ? "punishment_" : "job_") + assignmentName;
    setFlag(startFlagName);

    // Set the custom StartFlag from the .ini
    if (!customStartFlag.isEmpty()) {
        setFlag(customStartFlag);
    }

    // Run the global event handler
    if (scriptParser) {
        const auto &handlers = scriptParser->getScriptData().eventHandlers;
        QString proc = isPunishment ? handlers.punishmentGiven : handlers.jobAnnounced;
        if (!proc.isEmpty())
            runProcedure(proc);
    }

    // Store the previous status if needed for returning later
    if (!newStatus.isEmpty()) {
        QString statusFlagName = (isPunishment ? "punishment_" : "job_") + assignmentName + "_prev_status";
        QSettings settings(settingsFile, QSettings::IniFormat);
        settings.setValue("Assignments/" + statusFlagName, previousStatus);
    }

    // Record the start time
    QDateTime startTime = internalClock;
    QSettings settings(settingsFile, QSettings::IniFormat);
    settings.setValue("Assignments/" + assignmentName + "_start_time", startTime);

    // Execute StartProcedure if specified
    if (!startProcedure.isEmpty()) {
        runProcedure(startProcedure);
    }

    updateStatusText();
    emit jobListUpdated();
}

bool CyberDom::markAssignmentDone(const QString &assignmentName, bool isPunishment)
{
    // Settings is defined ONCE at the top.
    QSettings settings(settingsFile, QSettings::IniFormat);

    // --- START REFACTOR ---
    // We no longer use iniData. We get all "details" from the parser.
    if (!scriptParser) {
        qDebug() << "[ERROR] markAssignmentDone: scriptParser is null!";
        return false;
    }

    const ScriptData &data = scriptParser->getScriptData();
    QString lowerName = assignmentName.toLower();

    // These variables will replace the old 'details' map
    QString minTime;
    QString quickPenalty1;
    QString quickPenalty2;
    QString quickPenaltyGroup;
    QString quickMessage;
    QString quickProcedure;
    bool mustStart = false;
    QString addMerit;
    QString doneProcedure;
    QString beforeDoneProcedure;
    QString valueUnit;
    double value = 0;
    int punAmount = 0;

    // 1. Get the assignment's definition
    if (isPunishment) {
        const PunishmentDefinition *ptrDef = getPunishmentDefinition(assignmentName);
        if (!ptrDef) {
            qDebug() << "[WARNING] Definition not found for:" << assignmentName;
            return false;
        }
        const PunishmentDefinition &def = *ptrDef;

        // Get properties from the PunishmentDefinition & its DurationControl
        const DurationControl& dur = def.duration;
        minTime = dur.minTimeStart;
        quickPenalty1 = (dur.quickPenalty1 != 0) ? QString::number(dur.quickPenalty1) : "";
        quickPenalty2 = (dur.quickPenalty2 != 0.0) ? QString::number(dur.quickPenalty2) : "";
        quickPenaltyGroup = dur.quickPenaltyGroup;
        quickMessage = dur.quickMessage;
        quickProcedure = dur.quickProcedure;
        valueUnit = def.valueUnit.toLower();
        bool isTimeBased = (valueUnit == "day" || valueUnit == "hour" || valueUnit == "minute");
        mustStart = def.mustStart || def.longRunning || isTimeBased;
        mustStart = def.mustStart;
        addMerit = def.merits.add;
        doneProcedure = def.doneProcedure;
        beforeDoneProcedure = def.beforeDoneProcedure;

        valueUnit = def.valueUnit.toLower();
        value = def.value;
        punAmount = punishmentAmounts.value(assignmentName, 0);

    } else {
        if (!data.jobs.contains(lowerName)) {
            qDebug() << "[WARNING] Section not found for completed assignment: job-" << assignmentName;
            return false;
        }
        const JobDefinition &def = data.jobs.value(lowerName);

        // Get properties from the JobDefinition & its DurationControl
        const DurationControl& dur = def.duration;
        minTime = dur.minTimeStart;
        quickPenalty1 = (dur.quickPenalty1 != 0) ? QString::number(dur.quickPenalty1) : "";
        quickPenalty2 = (dur.quickPenalty2 != 0.0) ? QString::number(dur.quickPenalty2) : "";
        quickPenaltyGroup = dur.quickPenaltyGroup;
        quickMessage = dur.quickMessage;
        quickProcedure = dur.quickProcedure;
        mustStart = def.mustStart || def.longRunning;
        addMerit = def.merits.add;
        doneProcedure = def.doneProcedure;
        beforeDoneProcedure = def.beforeDoneProcedure;
    }

    // --- Run BeforeDoneProcedure and check for zzDeny ---
    if (!beforeDoneProcedure.isEmpty()) {
        runProcedure(beforeDoneProcedure);

        // Check if the procedure set the zzDeny flag
        if (isFlagSet("zzDeny")) {
            qDebug() << "[markAssignmentDone] 'Done' for" << assignmentName << "was denied by BeforeDoneProcedure.";
        }
    }

    QDateTime start = settings.value("Assignments/" + assignmentName + "_start_time").toDateTime();
    int elapsed = start.isValid() ? static_cast<int>(start.secsTo(internalClock)) : 0;

    // Check Timed Punishment
    if (isPunishment && (valueUnit == "minute" || valueUnit == "hour" || valueUnit == "day")) {
        int requiredSecs = 0;
        int total = qRound(value * punAmount);

        if (valueUnit == "minute") {
            requiredSecs = total * 60;
        } else if (valueUnit == "hour") {
            requiredSecs = total * 3600;
        } else if (valueUnit == "day") {
            requiredSecs = total * 86400;
        }

        if (requiredSecs > 0 && elapsed < requiredSecs) {
            int remaining = requiredSecs - elapsed;
            QMessageBox::warning(this, "Too Quick",
                                 QString("This punishment is not complete. You must continue for another %1.")
                                 .arg(formatDuration(remaining)));
            return false;
        }
    }

    // 2. Check MinTime
    if (!isPunishment && !minTime.isEmpty()) {
        // We removed the duplicate QSettings settings(...) line here
        int required = parseTimeRangeToSeconds(minTime);

        if (required > 0 && elapsed < required) {
            int penalty = 0;
            if (!quickPenalty1.isEmpty())
                penalty += quickPenalty1.toInt();
            if (!quickPenalty2.isEmpty()) {
                double ratio = quickPenalty2.toDouble();
                int minsEarly = static_cast<int>(std::ceil((required - elapsed) / 60.0));
                penalty += qRound(ratio * minsEarly);
            }

            if (penalty > 0)
                applyPunishment(penalty, quickPenaltyGroup);

            bool showMsg = !quickMessage.isEmpty() || !quickPenalty1.isEmpty() || !quickPenalty2.isEmpty();
            if (showMsg) {
                QString msg = quickMessage;
                if (msg.isEmpty())
                    msg = assignmentName + ": How can you finish in %?";
                msg.replace("#", formatDuration(required - elapsed));
                msg.replace("%", formatDuration(elapsed));
                QMessageBox::information(this, "Too Quick", replaceVariables(msg));
            }

            if (!quickProcedure.isEmpty())
                runProcedure(quickProcedure);

            return false;
        }
    }

    // 3. Check MustStart
    if (mustStart) {
        QString startFlagName = (isPunishment ? "punishment_" : "job_") + assignmentName + "_started";
        if (!isFlagSet(startFlagName)) {
            QMessageBox::warning(this, "Not Started", QString("This ") + (isPunishment ? "punishment" : "job") + " must be started before it can be marked done.");
            return false;
        }
    }


    // 4. Remove from active lists (This logic is all correct)
    activeAssignments.remove(assignmentName);
    if (isPunishment)
        punishmentAmounts.remove(assignmentName);

    jobDeadlines.remove(assignmentName);
    jobExpiryTimes.remove(assignmentName);
    jobRemindIntervals.remove(assignmentName);
    jobNextReminds.remove(assignmentName);
    expiredAssignments.remove(assignmentName);

    // 5. Cleanup flags
    QString startFlagName = (isPunishment ? "punishment_" : "job_") + assignmentName + "_started";
    removeFlag(startFlagName);

    if (isPunishment) {
        const PunishmentDefinition &def = data.punishments.value(lowerName);
        if (!def.startFlag.isEmpty()) {
            removeFlag(def.startFlag);
        }
    } else {
        const JobDefinition &def = data.jobs.value(lowerName);
        if (!def.startFlag.isEmpty()) {
            removeFlag(def.startFlag);
        }
    }

    // 6. Return to previous status
    QString statusFlagName = (isPunishment ? "punishment_" : "job_") + assignmentName + "_prev_status";
    QString savedPrevStatus = settings.value("Assignments/" + statusFlagName, "").toString();

    QString nextSubStatus;
    if (scriptParser) {
        QString lowerName = assignmentName.toLower();
        if (isPunishment && data.punishments.contains(lowerName)) {
            nextSubStatus = data.punishments.value(lowerName).nextSubStatus;
        } else if (!isPunishment && data.jobs.contains(lowerName)) {
            nextSubStatus = data.jobs.value(lowerName).nextSubStatus;
        }
    }

    if (!nextSubStatus.isEmpty()) {
        qDebug() << "[Status] Job complete, chaining to NextSubStatus:" << nextSubStatus;
        changeStatus(nextSubStatus, true);

        // Cleanup the saved status setting since we moved on
        if (!savedPrevStatus.isEmpty()) settings.remove("Assignments/" + statusFlagName);

    } else if (!savedPrevStatus.isEmpty()) {
        qDebug() << "[Status] Job complete, returning to saved previous status:" << savedPrevStatus;
        changeStatus(savedPrevStatus, false);
        settings.remove("Assignments/" + statusFlagName);

    } else {
        returntoLastStatus();
    }

    // 7. Add merits (This logic is now based on our new 'addMerit' variable)
    if (!addMerit.isEmpty()) {
        int meritPoints = 0;

        if (addMerit.startsWith("#")) {
            meritPoints = scriptParser->getVariable(addMerit.mid(1)).toInt();
        } else {
            bool ok;
            meritPoints = addMerit.toInt(&ok);
            if (!ok) meritPoints = 0;
        }

        if (meritPoints != 0) {
            int currentMerits = settings.value("User/Merits", maxMerits / 2).toInt();
            currentMerits += meritPoints;

            if (currentMerits > maxMerits) currentMerits = maxMerits;
            if (currentMerits < minMerits) currentMerits = minMerits;

            settings.setValue("User/Merits", currentMerits);
            updateMerits(currentMerits);
        }
    }

    // 8. Restore late merits (This logic is correct)
    if (jobLateMerits.contains(assignmentName)) {
        int restore = jobLateMerits.take(assignmentName);
        int currentMerits = settings.value("User/Merits", maxMerits / 2).toInt();
        currentMerits += restore;
        if (currentMerits > maxMerits) currentMerits = maxMerits;
        if (currentMerits < minMerits) currentMerits = minMerits;
        settings.setValue("User/Merits", currentMerits);
        updateMerits(currentMerits);
    }

    // 9. Execute DoneProcedure (uses new 'doneProcedure' variable)
    if (!doneProcedure.isEmpty()) {
        runProcedure(doneProcedure);
    }

    // 10. Run event handler (This logic is correct)
    if (scriptParser) {
        const auto &handlers = scriptParser->getScriptData().eventHandlers;
        QString proc = isPunishment ? handlers.punishmentDone : handlers.jobDone;
        if (!proc.isEmpty())
            runProcedure(proc);
    }

    // 11. Update UI (This is correct)
    updateStatusText();
    emit jobListUpdated();

    // If this was a job, save its completion date for interval tracking
    if (!isPunishment) {
        QString settingsKey = QString("JobCompletion/%1_lastDone").arg(assignmentName);
        settings.setValue(settingsKey, internalClock.date().toString(Qt::ISODate));
        qDebug() << "[Scheduler] Job" << assignmentName << "completed. Saving lastDone date:" << internalClock.date().toString(Qt::ISODate);

        // Check if this was a OneTime job
        const JobDefinition &jobDef = data.jobs.value(assignmentName.toLower());
        if (jobDef.oneTime) {
            QString oneTimeKey = QString("JobCompletion/$1_oneTimeDone").arg(assignmentName);
            settings.setValue(oneTimeKey, true);
            qDebug() << "[Scheduler] OneTime job" << assignmentName << "completed. Flagging as done forever.";
        }
    }

    return true;
}

void CyberDom::abortAssignment(const QString &assignmentName, bool isPunishment)
{
    // --- Get the AbortProcedure name ---
    QString abortProcedure;
    if (scriptParser) {
        const ScriptData &data = scriptParser->getScriptData();
        QString lowerName = assignmentName.toLower();
        if (isPunishment && data.punishments.contains(lowerName)) {
            abortProcedure = data.punishments.value(lowerName).abortProcedure;
        } else if (data.jobs.contains(lowerName)) {
            abortProcedure = data.jobs.value(lowerName).abortProcedure;
        }
    }

    // --- Check if the assignment was started ---
    QString startFlagName = (isPunishment ? "punishment_" : "job_") + assignmentName + "_started";
    if (!isFlagSet(startFlagName)) {
        qDebug() << "[Abort] Assignment" << assignmentName << "was not started.";
        return;
    }

    // --- Remove flags ---
    removeFlag(startFlagName);

    if (scriptParser) {
        const ScriptData &data = scriptParser->getScriptData();
        QString lowerName = assignmentName.toLower();
        if (isPunishment && data.punishments.contains(lowerName)) {
            if (!data.punishments.value(lowerName).startFlag.isEmpty()) {
                removeFlag(data.punishments.value(lowerName).startFlag);
            }
        } else if (data.jobs.contains(lowerName)) {
            if (!data.jobs.value(lowerName).startFlag.isEmpty()) {
                removeFlag(data.jobs.value(lowerName).startFlag);
            }
        }
    }

    // --- Return to previous status (Existing logic) ---
    QString statusFlagName = (isPunishment ? "punishment_" : "job_") + assignmentName + "_prev_status";
    QSettings settings(settingsFile, QSettings::IniFormat);
    QString prevStatus = settings.value("Assignments/" + statusFlagName, "").toString();

    if (!prevStatus.isEmpty()) {
        changeStatus(prevStatus, false);
        settings.remove("Assignments/" + statusFlagName);
    }

    // --- Run the AbortProcedure ---
    if (!abortProcedure.isEmpty()) {
        runProcedure(abortProcedure);
    }

    qDebug() << "[Abort] Assignment" << assignmentName << "has been aborted.";
    emit jobListUpdated();
}

void CyberDom::playSoundSafe(const QString &filePath) {
    auto player = new QMediaPlayer(this);
    connect(player,
            &QMediaPlayer::errorOccurred,
            this,
            [player, filePath](QMediaPlayer::Error error, const QString &errorString) {
                if (error != QMediaPlayer::NoError) {
                    qWarning() << "[Audio] Failed to play" << filePath << ":" << errorString;
                }
            });
    player->setSource(QUrl::fromLocalFile(filePath));
    player->play();
    if (player->error() != QMediaPlayer::NoError) {
        qWarning() << "[Audio] Failed to start playback for" << filePath << ":"
                   << player->errorString();
    }
}

bool CyberDom::checkInterruptableAssignments()
{
    if (!scriptParser) return false;

    const ScriptData &data = scriptParser->getScriptData();
    bool abortedAssignment = false;

    // Iterate over a *copy* of the list, because
    // abortAssignment() will modify the 'activeAssignments' list
    QSet<QString> active = activeAssignments;

    for (const QString &name : active) {
        QString lowerName = name.toLower();
        bool isPun = data.punishments.contains(lowerName);
        bool isJob = data.jobs.contains(lowerName);

        // Check if this assignment is started
        QString startFlagName = (isPun ? "punishment_" : "job_") + name + "_started";
        if (!isFlagSet(startFlagName)) {
            continue;
        }

        // Check if it's non-interruptable
        bool isInterruptable = true;
        if (isPun) {
            isInterruptable = data.punishments.value(lowerName).interruptable;
        } else if (isJob) {
            isInterruptable = data.jobs.value(lowerName).interruptable;
        }

        if (!isInterruptable) {
            // This is a non-interruptable task that is running. Abort it.
            qDebug() << "[Interrupt] Non-interruptable assignment" << name << "was active. Aborting.";
            QMessageBox::warning(this, "Assignment Aborted",
                                 QString("Your non-interruptable assignment '%1' was aborted because you used the application.").arg(name));
            abortAssignment(name, isPun);
            abortedAssignment = true;
        }
    }
    return abortedAssignment;
}

void CyberDom::deleteAssignment(const QString &assignmentName, bool isPunishment)
{
    QString deleteProcedure;
    if (scriptParser) {
        const ScriptData &data = scriptParser->getScriptData();
        QString lowerName = assignmentName.toLower();

        if (isPunishment) {
            if (data.punishments.contains(lowerName)) {
                deleteProcedure = data.punishments.value(lowerName).deleteProcedure;
            } else {
                qDebug() << "[WARNING] Section not found for deleted assignment: punishment-" << assignmentName;
            }
        } else {
            if (data.jobs.contains(lowerName)) {
                deleteProcedure = data.jobs.value(lowerName).deleteProcedure;
            } else {
                qDebug() << "[WARNING] Section not found for deleted assignment: job-" << assignmentName;
            }
        }
    }

    activeAssignments.remove(assignmentName);
    if (isPunishment)
        punishmentAmounts.remove(assignmentName);
    jobDeadlines.remove(assignmentName);
    jobExpiryTimes.remove(assignmentName);
    jobRemindIntervals.remove(assignmentName);
    jobNextReminds.remove(assignmentName);
    jobLateMerits.remove(assignmentName);
    expiredAssignments.remove(assignmentName);

    QString startFlagName = (isPunishment ? "punishment_" : "job_") + assignmentName + "_started";
    removeFlag(startFlagName);

    QString statusFlagName = (isPunishment ? "punishment_" : "job_") + assignmentName + "_prev_status";
    QSettings settings(settingsFile, QSettings::IniFormat);
    QString prevStatus = settings.value("Assignments/" + statusFlagName, "").toString();
    if (!prevStatus.isEmpty()) {
        changeStatus(prevStatus, false);
        settings.remove("Assignments/" + statusFlagName);
    }

    // Run the DeleteProcedure
    if (!deleteProcedure.isEmpty()) {
        runProcedure(deleteProcedure);
    }

    emit jobListUpdated();
}

bool CyberDom::isFlagSet(const QString &flagName) const {
    return flags.contains(flagName);
}

QStringList CyberDom::getFlagsByGroup(const QString &groupName) const {
    QStringList result;
    QString target = groupName.trimmed().toLower();

    for (auto it = flags.constBegin(); it != flags.constEnd(); ++it) {
        const QString &flagName = it.key();
        const FlagData &data = it.value();

        for (const QString &grp : data.groups) {
            if (grp.trimmed().toLower() == target) {
                result.append(flagName);
                break;
            }
        }
    }

    return result;
}

void CyberDom::setFlagGroup(const QString &groupName) {
    QString target = groupName.trimmed().toLower();

    for (auto it = iniData.constBegin(); it != iniData.constEnd(); ++it) {
        const QString &section = it.key();
        if (!section.startsWith("flag-", Qt::CaseInsensitive))
            continue;

        QString groups = it.value().value("Group");
        if (groups.isEmpty())
            continue;

        for (const QString &grp : groups.split(',')) {
            if (grp.trimmed().toLower() == target) {
                setFlag(section.mid(5));
                break;
            }
        }
    }
}

void CyberDom::removeFlagGroup(const QString &groupName) {
    const QStringList activeFlags = getFlagsByGroup(groupName);
    for (const QString &flag : activeFlags)
        removeFlag(flag);
}

QString CyberDom::getClothingReportPrompt() const
{
    QString prompt = "";

    if (scriptParser) {
        QMap<QString, QStringList> rawStatusData = scriptParser->getRawSectionData("status-" + currentStatus);

        // Find the "ClothReport" key (case-insensitive)
        for (auto it = rawStatusData.constBegin(); it != rawStatusData.constEnd(); ++it) {
            if (it.key().compare("ClothReport", Qt::CaseInsensitive) == 0) {
                if (!it.value().isEmpty()) {
                    prompt = it.value().first();
                    break;
                }
            }
        }
    }

    if (prompt.isEmpty()) {
        prompt = "What are you wearing?";
    }

    return replaceVariables(prompt);
}

void CyberDom::processClothingReport(const QList<ClothingItem> &wearingItems, bool isNaked)
{
    // Build the report text
    QString reportText = "";
    
    if (isNaked) {
        reportText = "I am naked.";
    } else {
        reportText = "I am wearing:";
        
        // Group items by type
        QMap<QString, QStringList> itemsByType;
        
        for (const ClothingItem &item : wearingItems) {
            itemsByType[item.getType()].append(item.getName());
        }
        
        // Generate report text
        for (auto it = itemsByType.constBegin(); it != itemsByType.constEnd(); ++it) {
            reportText += "\n- " + it.key() + ": " + it.value().join(", ");
        }
    }
    
    // Store the clothing report in settings
    storeClothingReport(reportText);
    
    // Show confirmation to the user
    QMessageBox::information(this, "Report Submitted", "Your clothing report has been submitted.");
}

void CyberDom::storeClothingReport(const QString &reportText)
{
    // Store the clothing report in settings
    QSettings settings(settingsFile, QSettings::IniFormat);
    
    // Store report with timestamp
    QDateTime timestamp = internalClock;
    QString reportKey = QString("ClothingReport_%1").arg(timestamp.toString("yyyyMMdd_hhmmss"));
    
    settings.setValue(reportKey, reportText);
    
    // Also store latest report for easy access
    settings.setValue("LatestClothingReport", reportText);
    settings.setValue("LatestClothingReportTime", timestamp.toString("yyyy-MM-dd hh:mm:ss"));
    
    settings.sync();
}

QStringList CyberDom::getClothingSets(const QString &setPrefix)
{
    QStringList result;

    // Safety Check
    if (!scriptParser) {
        qDebug() << "[ERROR] getClothingSets: scriptParser is null";
        return result;
    }

    QRegularExpression setRegex("^" + QRegularExpression::escape(setPrefix) + "-(.+)$");

    // Get the list of all section names from the parser
    QStringList allSectionNames = scriptParser->getRawSectionNames();

    // Iterate the list of names
    for (const QString &sectionName : allSectionNames) {
        QRegularExpressionMatch match = setRegex.match(sectionName);
        if (match.hasMatch()) {
            result << match.captured(1);
        }
    }

    return result;
}

QStringList CyberDom::getClothingOptions(const QString &setName)
{
    QStringList result;
    
    // Get options for the given set
    if (iniData.contains("set-" + setName)) {
        const auto &setData = iniData["set-" + setName];
        
        for (auto it = setData.constBegin(); it != setData.constEnd(); ++it) {
            if (it.key().startsWith("option=")) {
                result << it.value();
            }
        }
    }
    
    return result;
}

void CyberDom::addClothingItem(const ClothingItem& item) {
    clothingInventory.append(item);
}

QMap<QDate, QStringList> CyberDom::getHolidays() const {
    QMap<QDate, QStringList> result;

    // Safety Check
    if (!scriptParser) {
        qDebug() << "[ERROR] getHolidays: scriptParser is null";
        return result;
    }

    // Get all section names from the parser
    QStringList allSectionNames = scriptParser->getRawSectionNames();

    // Iterate through the section names
    for (const QString &sectionName : allSectionNames) {
        if (!sectionName.toLower().startsWith("holiday-"))
            continue;

        // Get the name of the holiday from the section key
        QString name = sectionName.mid(QString("holiday-").length());

        // Get the raw data for this specific holiday
        QMap<QString, QStringList> rawData = scriptParser->getRawSectionData(sectionName);

        // Helper to get the first value for a key, or an empty string
        auto getFirstValue = [&](const QString &key) -> QString {
            if (rawData.contains(key) && !rawData.value(key).isEmpty()) {
                return rawData.value(key).first();
            }
            return QString();
        };

        // Check for a single "Date"
        QString dateStr = getFirstValue("Date");
        if (!dateStr.isEmpty()) {
            QDate d = QDate::fromString(dateStr, "MM-dd-yyyy");
            if (d.isValid())
                result[d].append(name);
            continue;
        }

        // Check for "StartDate" and "EndDate"
        QString startStr = getFirstValue("StartDate");
        QString endStr = getFirstValue("EndDate");
        QDate s = QDate::fromString(startStr, "MM-dd-yyyy");
        QDate e = QDate::fromString(endStr, "MM-dd-yyyy");
        if (s.isValid() && e.isValid()) {
            for (QDate d = s; d <= e; d = d.addDays(1))
                result[d].append(name);
        }
    }

    return result;
}

void CyberDom::executeQuestion(const QString &questionKey, const QString &title)
{
    qDebug() << "[DEBUG] Executing question:" << questionKey;

    QuestionDefinition questionData = scriptParser->getQuestion(questionKey);

    if (questionData.name.isEmpty() || questionData.text.isEmpty()) {
        qDebug() << "[ERROR] Could not find question data for:" << questionKey;
        return;
    }

    // --- Check Question Conditions ---
    for (const QString &condition : questionData.ifConditions) {
        if (!evaluateCondition(condition)) {
            qDebug() << "[Question Skipped] 'If' condition not met:" << condition;
            return;
        }
    }
    for (const QString &condition : questionData.notIfConditions) {
        if (evaluateCondition(condition)) {
            qDebug() << "[Question Skipped] 'NotIf' condition not met:" << condition;
            return;
        }
    }

    questionData.text = replaceVariables(questionData.text);

    QList<QuestionDefinition> questions{questionData};

    QuestionDialog dialog(questions, this);
    dialog.setWindowTitle(title);

    if (dialog.exec() == QDialog::Accepted) {
        QString selectedAnswer;
        QMap<QString, QString> answers = dialog.getAnswers();

        if (!questionData.variable.isEmpty()) {
            selectedAnswer = answers.value(questionData.variable);
        } else if (!answers.isEmpty()) {
            selectedAnswer = answers.begin().value();
        }
        qDebug() << "[DEBUG] Question answered:" << selectedAnswer;

        questionAnswers[questionKey] = selectedAnswer;
        saveQuestionAnswers();

        for (const auto &answerBlock : questionData.answers) {
            QString answerValue = answerBlock.variableValue.isEmpty()
                                  ? answerBlock.answerText
                                      : answerBlock.variableValue;

            if (answerValue == selectedAnswer) {
                QString procedureName = answerBlock.procedureName;

                if (!procedureName.isEmpty() && procedureName != "*") {
                    qDebug() << "[DEBUG] Running selected procedure:" << procedureName;
                    runProcedure(procedureName);
                } else if (procedureName == "*") {
                    qDebug() << "[DEBUG] Inline procedure selected - continuing";
                }

                if (answerBlock.punishMin != 0 || answerBlock.punishMax != 0) {
                    int minSeverity = answerBlock.punishMin;
                    int maxSeverity = answerBlock.punishMax ? answerBlock.punishMax : minSeverity;
                    if (maxSeverity < minSeverity)
                        maxSeverity = minSeverity;

                    int severity;
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
                    severity = QRandomGenerator::global()->bounded(minSeverity, maxSeverity + 1);
#else
                    static bool seeded = false;
                    if (!seeded) {
                        std::srand(static_cast<unsigned int>(std::time(nullptr)));
                        seeded = true;
                    }
                    severity = minSeverity + std::rand() % (maxSeverity - minSeverity + 1);
#endif
                    applyPunishment(severity, QString(), QString());
                }
                break;
            }
        }
    }
}

void CyberDom::performCounterOperation(const QString &opString, const QString &opType)
{
    if (!scriptParser) return;

    QStringList parts = opString.split(",", Qt::SkipEmptyParts);
    if (parts.size() != 2) {
        qDebug() << "[WARN] Malformed counter operation:" << opType << opString;
        return;
    }

    QString varName = parts[0].trimmed();
    QString rawValue = parts[1].trimmed();

    if (varName.startsWith("#")) {
        varName = varName.mid(1);
    }

    // --- Get Current Value ---
    QString currentValStr = scriptParser->getVariable(varName);
    bool ok1;
    int currentVal = currentValStr.toInt(&ok1);
    if (!ok1) currentVal = 0;

    // -- Get Value to Apply ---
    int valToApply = 0;
    if (rawValue.startsWith("#")) {
        // Value is another variable
        QString valStr = scriptParser->getVariable(rawValue.mid(1));
        bool ok2;
        valToApply = valStr.toInt(&ok2);
        if (!ok2) valToApply = 0;
    } else {
        // Value is a literal number
        bool ok2;
        valToApply = rawValue.toInt(&ok2);
        if (!ok2) valToApply = 0;
    }

    // --- Perform Operation ---
    int result = 0;
    if (opType == "add") {
        result = currentVal + valToApply;
    } else if (opType == "subtract") {
        result = currentVal - valToApply;
    } else if (opType == "multiply") {
        result = currentVal * valToApply;
    } else if (opType == "divide") {
        if (valToApply == 0) {
            qDebug() << "[WARN] Division by zero attempted:" << opString;
            result = currentVal;
        } else {
            result = currentVal / valToApply;
        }
    }

    // --- Save New Value ---
    scriptParser->setVariable(varName, QString::number(result));
    qDebug() << "[DEBUG] Counter Op:" << varName << opType << valToApply << "=" << result;
}

void CyberDom::modifyMerits(const QString &value, const QString &opType)
{
    if (!scriptParser) return;

    int points = 0;

    // Resolve the value
    if (value.startsWith("#")) {
        points = scriptParser->getVariable(value.mid(1)).toInt();
    } else {
        bool ok;
        points = value.toInt(&ok);
        if (!ok) points = 0;
    }

    if (points == 0 && opType != "set") {
        return;
    }

    // Get current merits
    int currentMerits = getMeritsFromIni();

    // Perform operation
    if (opType == "add") {
        currentMerits += points;
    } else if (opType == "subtract") {
        currentMerits -= points;
    } else if (opType == "set") {
        currentMerits = points;
    }

    // Clamp and update
    currentMerits = std::max(minMerits, std::min(maxMerits, currentMerits));
    updateMerits(currentMerits);
    qDebug() << "[Merits]" << opType << "points:" << points << "New Total:" << currentMerits;
}

void CyberDom::applyDailyMerits()
{
    if (!scriptParser) return;

    int meritsToAdd = scriptParser->getScriptData().general.dayMerits;

    // Check if this feature is even used
    if (meritsToAdd == 0) {
        return;
    }

    QDate today = internalClock.date();

    // Check if we have already applied merits for today.
    if (lastDayMeritsGiven.isValid() && lastDayMeritsGiven == today) {
        return;
    }

    // Apply the merits and update the tracker
    qDebug() << "[Merits] Applying daily merits:" << meritsToAdd;
    modifyMerits(QString::number(meritsToAdd), "add");

    // Update the tracker to today's date
    lastDayMeritsGiven = today;
}

void CyberDom::runProcedure(const QString &procedureName) {
    if (!scriptParser) {
        qDebug() << "[ERROR] ScriptParser not initialized.";
        return;
    }

    QString lowerProcName = procedureName.trimmed().toLower();

    const auto &procs = scriptParser->getScriptData().procedures;

    if (!procs.contains(lowerProcName)) {
        qDebug() << "[WARNING] Procedure section not found:" << procedureName;
        return;
    }

    const ProcedureDefinition &proc = procs.value(lowerProcName);
    qDebug() << "\n[DEBUG] Running procedure:" << proc.name;

    // Check time restrictions (from struct)
    if (!isTimeAllowed(proc.notBeforeTimes, proc.notAfterTimes, proc.notBetweenTimes)) {
        qDebug() << "[DEBUG] Procedure skipped due to time restrictions:" << proc.name;
        return;
    }

    // Check PreStatus requirements
    if (!proc.preStatuses.isEmpty()) {
        bool statusMatch = false;
        QString lowerCurrentStatus = currentStatus.toLower();
        for (const QString &preStatus : proc.preStatuses) {
            if (preStatus.toLower() == lowerCurrentStatus) {
                statusMatch = true;
                break;
            }
        }

        // If no match was found, skip this procedure
        if (!statusMatch) {
            qDebug() << "[Procedure Skipped]" << proc.name << "is not allowed in stautus:" << currentStatus;
            return;
        }
    }

    // This flag allows 'If' and 'NotIf' to control *next* action
    bool skipNextAction = false;

    for (const ScriptAction &action : proc.actions) {

        // Check if the previous line was a failed 'If' or 'NotIf'
        if (skipNextAction) {
            skipNextAction = false; // Reset the flag

            // We only skip one action. 'If' and 'NotIf' don't get skipped
            if (action.type != ScriptActionType::If && action.type != ScriptActionType::NotIf) {
                qDebug() << "[Procedure] Action skipped due to condition.";
                continue;
            }
        }

        // Execute the action
        switch (action.type) {

            // --- Conditional Actions ---
        case ScriptActionType::If:
            if (!evaluateCondition(action.value)) {
                skipNextAction = true;
            }
            break;

        case ScriptActionType::NotIf:
            if (evaluateCondition(action.value)) {
                skipNextAction = true;
            }
            break;

        // --- Standard Actions ---
        case ScriptActionType::ProcedureCall: {
            QString procToCall = action.value.split(",").first().trimmed().toLower();
            runProcedure(procToCall);
            break;
        }
        case ScriptActionType::SetFlag:
            setFlag(action.value);
            break;
        case ScriptActionType::RemoveFlag:
        case ScriptActionType::ClearFlag:
            removeFlag(action.value);
            break;

        case ScriptActionType::SetCounterVar: {
            QStringList parts = action.value.split(",", Qt::SkipEmptyParts);
            if (parts.size() == 2) {
                QString varName = parts[0].trimmed();
                if (varName.startsWith("#")) varName = varName.mid(1);

                QString valToSet = parts[1].trimmed();
                if (valToSet.startsWith("#")) {
                    valToSet = scriptParser->getVariable(valToSet.mid(1));
                }
                scriptParser->setVariable(varName, valToSet);
                qDebug() << "[DEBUG] Variable set from procedure:" << varName << "=" << valToSet;
            } else {
                qDebug() << "[WARN] Malformed Set# in procedure:" << action.value;
            }
            break;
        }
        case ScriptActionType::SetTimeVar: {
            QStringList parts = action.value.split(",", Qt::SkipEmptyParts);
            if (parts.size() == 2) {
                QString varName = parts[0].trimmed();
                if (varName.startsWith("!")) varName = varName.mid(1);

                QString valToSet = parts[1].trimmed();
                if (valToSet.startsWith("!")) {
                    valToSet = scriptParser->getVariable(valToSet.mid(1));
                }
                scriptParser->setVariable(varName, valToSet);
                qDebug() << "[DEBUG] Time variable set from procedure:" << varName << "=" << valToSet;
            } else {
                qDebug() << "[WARN] Malformed Set! in procedure:" << action.value;
            }
            break;
        }
        case ScriptActionType::AddCounter:
            performCounterOperation(action.value, "add");
            break;
        case ScriptActionType::SubtractCounter:
            performCounterOperation(action.value, "subtract");
            break;
        case ScriptActionType::MultiplyCounter:
            performCounterOperation(action.value, "multiply");
            break;
        case ScriptActionType::DivideCounter:
            performCounterOperation(action.value, "divide");
            break;
        case ScriptActionType::Message:
            QMessageBox::information(this, "Message", replaceVariables(action.value));
            break;
        case ScriptActionType::Question:
            executeQuestion(action.value.trimmed().toLower(), "Question");
            break;
        case ScriptActionType::Clothing:
            updateClothingInstructions(replaceVariables(action.value));
            break;
        case ScriptActionType::NewStatus:
            changeStatus(action.value, false);
            break;
        case ScriptActionType::NewSubStatus:
            changeStatus(action.value, true);
            break;
        case ScriptActionType::AnnounceJob:
            addJobToAssignments(action.value, false);
            break;

        case ScriptActionType::MarkDone: {
            QString name = action.value;
            bool isPun = name.startsWith("punishment-", Qt::CaseInsensitive);
            QString cleanName = isPun ? name.mid(11) : name.mid(4);

            markAssignmentDone(cleanName, isPun);
            break;
        }

        case ScriptActionType::Abort: {
            QString name = action.value;
            bool isPun = name.startsWith("punishment-", Qt::CaseInsensitive);
            QString cleanName = isPun ? name.mid(11) : name.mid(4);

            abortAssignment(cleanName, isPun);
            break;
        }

        case ScriptActionType::Delete: {
            QString name = action.value;
            bool isPun = name.startsWith("punishment-", Qt::CaseInsensitive);
            QString cleanName = isPun ? name.mid(11) : name.mid(4);

            deleteAssignment(cleanName, isPun);
            break;
        }

        case ScriptActionType::Timer: {
            QString timerName = action.value.trimmed().toLower();
            if (scriptParser && scriptParser->getScriptData().timers.contains(timerName)) {
                const TimerDefinition &def = scriptParser->getScriptData().timers.value(timerName);

                // This logic is copied from applyScriptSettings
                QString startRange = def.startTimeMin;
                if (!def.startTimeMax.isEmpty() && def.startTimeMax != def.startTimeMin) {
                    startRange += "," + def.startTimeMax;
                }
                int startSecs = parseTimeRangeToSeconds(startRange);
                QTime startTime = QTime(0,0).addSecs(startSecs);

                int endSecs = parseTimeRangeToSeconds(def.endTime);
                QTime endTime = QTime(0,0).addSecs(endSecs);

                TimerInstance timer;
                timer.name = def.name;
                timer.start = startTime;
                timer.end = endTime;
                timer.triggered = false;

                activeTimers.append(timer);
                qDebug() << "[Timer] Procedure activated timer:" << def.name;
            } else {
                qDebug() << "[WARN] Procedure tried to activate unknown timer:" << timerName;
            }
            break;
        }

        case ScriptActionType::AddMerit:
            modifyMerits(action.value, "add");
            break;
        case ScriptActionType::SubtractMerit:
            modifyMerits(action.value, "subtract");
            break;
        case ScriptActionType::SetMerit:
            modifyMerits(action.value, "set");
            break;
        case ScriptActionType::Punish:
            executePunish(action.value, proc.punishMessage, proc.punishGroup);
            break;

        default:
            qDebug() << "[WARN] Unhandled action type:" << static_cast<int>(action.type);
            break;
        }
    }
}

// void CyberDom::runProcedure(const QString &procedureName) {
//     QString sectionName = "procedure-" + procedureName;

//     if (!scriptParser) {
//         qDebug() << "[ERROR] ScriptParser not initialized.";
//         return;
//     }

//     const auto &procs = scriptParser->getScriptData().procedures;
//     if (procs.contains(procedureName)) {
//         const ProcedureDefinition &proc = procs.value(procedureName);
//         if (!isTimeAllowed(proc.notBeforeTimes, proc.notAfterTimes, proc.notBetweenTimes))
//             return;
//     }

//     QMap<QString, QStringList> procedureData = scriptParser->getRawSectionData(sectionName);

//     if (procedureData.isEmpty()) {
//         qDebug() << "[WARNING] Procedure section not found or empty:" << sectionName;
//         return;
//     }

//     qDebug() << "\n[DEBUG] Running procedure:" << procedureName;

//     for (auto it = procedureData.begin(); it != procedureData.end(); ++it) {
//         QString key = it.key().trimmed();
//         QStringList values = it.value();

//         for (const QString &rawValue : values) {
//             QString value = rawValue.trimmed();
//             qDebug() << "[DEBUG] Processing key:" << key << "with value:" << value;

//             if (key.compare("SetFlag", Qt::CaseInsensitive) == 0) {
//                 setFlag(value);

//             } else if (key.compare("RemoveFlag", Qt::CaseInsensitive) == 0) {
//                 removeFlag(value);

//             } else if (key.compare("AddMerits", Qt::CaseInsensitive) == 0) {
//                 int currentMerits = getMeritsFromIni();
//                 int added = value.toInt();
//                 updateMerits(currentMerits + added);

//             } else if (key.compare("Procedure", Qt::CaseInsensitive) == 0) {
//                 runProcedure(value);

//             } else if (key.compare("Message", Qt::CaseInsensitive) == 0) {
//                 QMessageBox::information(this, "Message", replaceVariables(value));

//             } else if (key.compare("Sound", Qt::CaseInsensitive) == 0) {
//                 playSoundSafe(value);

//             } else if (key.compare("Input", Qt::CaseInsensitive) == 0) {
//                 bool ok;
//                 QString response = QInputDialog::getText(this, "Input Required",
//                                                          replaceVariables(value),
//                                                          QLineEdit::Normal, "", &ok);
//                 if (ok && !response.isEmpty()) {
//                     qDebug() << "[Input Response]:" << response;
//                 }

//             } else if (key.compare("Instructions", Qt::CaseInsensitive) == 0) {
//                 updateInstructions(value);

//             } else if (key.compare("Clothing", Qt::CaseInsensitive) == 0) {
//                 updateClothingInstructions(value);

//             } else if (key.compare("ShowPicture", Qt::CaseInsensitive) == 0) {
//                 // Placeholder — implement if desired
//                 qDebug() << "[Picture] Show:" << value;

//             } else if (key.compare("RemovePicture", Qt::CaseInsensitive) == 0) {
//                 // Placeholder — implement if desired
//                 qDebug() << "[Picture] Remove:" << value;

//             } else if (key.compare("If", Qt::CaseInsensitive) == 0) {
//                 QString condition = value;
//                 if (condition.startsWith("#")) {
//                     QString flag = condition.mid(1).trimmed();
//                     if (!isFlagSet(flag)) {
//                         qDebug() << "[Procedure Skipped] Condition not met: " << flag;
//                         continue;
//                     }
//                 }

//             } else if (key.compare("Question", Qt::CaseInsensitive) == 0) {
//                 // Get the question name from the value
//                 QString questionKey = value.trimmed();
//                 qDebug() << "[DEBUG] Found Question key: " << questionKey;

//                 // Retrieve the question data from the script parser
//                 QuestionSection questionData = scriptParser->getQuestion(questionKey);

//                 // Check if we found a valid question
//                 if (questionData.name.isEmpty() || questionData.phrase.isEmpty()) {
//                     qDebug() << "[ERROR] Could not find question data for:" << questionKey;
//                     continue;
//                 }

//                 // Apply variable replacement to question text
//                 questionData.phrase = replaceVariables(questionData.phrase);

//                 // Build a list containing this single question
//                 QList<QuestionDefinition> questions{questionData};

//                 // Create and show the question dialog
//                 QuestionDialog dialog(questions, this);
//                 dialog.setWindowTitle("Question");

//                 // Show the dialog and get result
//                 if (dialog.exec() == QDialog::Accepted) {
//                     // Extract the selected answer using dialog.getAnswers()
//                     QString selectedAnswer;
//                     QMap<QString, QString> answers = dialog.getAnswers();
//                     if (!questionData.variable.isEmpty()) {
//                         selectedAnswer = answers.value(questionData.variable);
//                     } else if (!answers.isEmpty()) {
//                         selectedAnswer = answers.begin().value();
//                     }
//                     qDebug() << "[DEBUG] Question answered:" << selectedAnswer;

//                     // Save the answer for later reference
//                     questionAnswers[questionKey] = selectedAnswer;
//                     saveQuestionAnswers();

//                     // Check if the selected answer maps to a procedure
//                     for (const auto &answerBlock : questionData.answers) {
//                         QString answerValue = answerBlock.variableValue.isEmpty()
//                                                ? answerBlock.answerText
//                                                : answerBlock.variableValue;
//                         if (answerValue == selectedAnswer) {
//                             QString procedureName = answerBlock.procedureName;

//                             // Handle inline actions if required in the future
//                             if (!procedureName.isEmpty() && procedureName != "*") {
//                                 qDebug() << "[DEBUG] Running selected procedure:" << procedureName;
//                                 runProcedure(procedureName);
//                             } else if (procedureName == "*") {
//                                 qDebug() << "[DEBUG] Inline procedure selected - continuing";
//                             }

//                             // Apply punishment if configured
//                             if (answerBlock.punishMin != 0 || answerBlock.punishMax != 0) {
//                                 int minSeverity = answerBlock.punishMin;
//                                 int maxSeverity = answerBlock.punishMax ? answerBlock.punishMax : minSeverity;
//                                 if (maxSeverity < minSeverity)
//                                     maxSeverity = minSeverity;

//                                 int severity;
// #if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
//                                 severity = QRandomGenerator::global()->bounded(minSeverity, maxSeverity + 1);
// #else
//                                 static bool seeded = false;
//                                 if (!seeded) {
//                                     std::srand(static_cast<unsigned int>(std::time(nullptr)));
//                                     seeded = true;
//                                 }
//                                 severity = minSeverity + std::rand() % (maxSeverity - minSeverity + 1);
// #endif

//                                 applyPunishment(severity);
//                             }
//                             break;
//                         }
//                     }
//                 }

//             } else if (key.startsWith("set#", Qt::CaseInsensitive)) {
//                 QString setValue = value.trimmed();
//                 QStringList parts = setValue.split(",", Qt::SkipEmptyParts);
//                 if (parts.size() == 2) {
//                     QString varName = parts[0].trimmed();
//                     QString varValue = parts[1].trimmed();
//                     if (varName.startsWith("#")) {
//                         varName = varName.mid(1);
//                     }
//                     scriptParser->setVariable(varName, varValue);
//                     qDebug() << "[DEBUG] Variable set from procedure:" << varName << "=" << varValue;
//                 } else {
//                     qDebug() << "[WARN] Malformed set# directive in procedure:" << value;
//             }

//             } else if (key.compare("Timer", Qt::CaseInsensitive) == 0) {
//                 QString timerKey = "timer-" + value;
//                 if (!iniData.contains(timerKey)) {
//                     qDebug() << "[Timer] Timer section not found:" << timerKey;
//                     continue;
//                 }

//                 QMap<QString, QString> timerData = iniData[timerKey];
//                 QTime startTime = QTime::fromString(timerData.value("Start", "00:00"), "hh:mm");
//                 QTime endTime = QTime::fromString(timerData.value("End", "23:59"), "hh:mm");
//                 QString message = timerData.value("Message");
//                 QString sound = timerData.value("Sound");
//                 QString procedure = timerData.value("Procedure");

//                 TimerInstance timer;
//                 timer.name = value;
//                 timer.start = startTime;
//                 timer.end = endTime;
//                 timer.message = message;
//                 timer.sound = sound;
//                 timer.procedure = procedure;
//                 timer.triggered = false;

//                 activeTimers.append(timer);
//                 qDebug() << "[Timer] Registered timer:" << value << " from "
//                          << startTime.toString() << " to " << endTime.toString();

//             } else {
//                 qDebug() << "[UNHANDLED] Procedure key:" << key << " -> " << value;
//             }
//         }
//     }
// }

void CyberDom::saveQuestionAnswers() {
    QSettings settings(settingsFile, QSettings::IniFormat);
    settings.beginGroup("Answers");
    for (auto it = questionAnswers.begin(); it != questionAnswers.end(); ++it) {
        settings.setValue(it.key(), it.value());
    }
    settings.endGroup();
    settings.sync();
}

void CyberDom::loadQuestionAnswers() {
    QSettings settings(settingsFile, QSettings::IniFormat);
    settings.beginGroup("Answers");
    QStringList keys = settings.childKeys();
    for (const QString &key : keys) {
        questionAnswers[key] = settings.value(key).toString();
    }
    settings.endGroup();
}

void CyberDom::saveVariablesToCDS(const QString &cdsPath) {
    if (!scriptParser)
        return;

    QFile file(cdsPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "[CyberDom] Failed to write .cds:" << cdsPath
                   << "-" << file.errorString();
        return;
    }

    QTextStream out(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    out.setEncoding(QStringConverter::Utf8);
#else
    out.setCodec("UTF-8");
#endif
    const auto &vars = scriptParser->getScriptData().stringVariables;
    for (auto it = vars.constBegin(); it != vars.constEnd(); ++it) {
        out << it.key() << "=" << it.value() << "\n";
    }

    file.close();
}

void CyberDom::executeReport(const QString &name) {
    if (!scriptParser) {
        qWarning() << "[CyberDom] No script loaded for report:" << name;
        return;
    }
    const auto &reports = scriptParser->getScriptData().reports;
    if (!reports.contains(name)) {
        qWarning() << "[CyberDom] Report definition not found:" << name;
        return;
    }
    const ReportDefinition &rep = reports.value(name);

    // Check PreStatus
    if (!rep.preStatuses.isEmpty()) {
        QString lowerCurrentStatus = currentStatus.toLower();
        bool foundMatch = false;
        for (const QString &preStatus : rep.preStatuses) {
            if (preStatus.toLower() == lowerCurrentStatus) {
                foundMatch = true;
                break;
            }
        }
        if (!foundMatch) {
            QMessageBox::information(this, tr("Report"),
                                     tr("This report is not available in your current status."));
            return;
        }
    }

    // Check Time
    if (!isTimeAllowed(rep.notBeforeTimes, rep.notAfterTimes, rep.notBetweenTimes)) {
        QMessageBox::information(this, tr("Report"),
                                 tr("This report is not available at this time."));
        return;
    }

    // Handle Merits
    int merits = getMeritsFromIni();

    // Helper lambda to get the value froma a string
    auto getMeritValue = [&](const QString &s) -> int {
        if (s.isEmpty()) return 0;
        if (s.startsWith("#")) {
            return scriptParser->getVariable(s.mid(1)).toInt();
        }
        return s.toInt();
    };

    // Check for SetMerit first
    if (!rep.merits.set.isEmpty()) {
        merits = getMeritValue(rep.merits.set);
    } else {
        // Only apply add/subtract if SetMerit isn't used
        merits += getMeritValue(rep.merits.add);
        merits -= getMeritValue(rep.merits.subtract);
    }

    merits = std::max(getMinMerits(), std::min(getMaxMerits(), merits));
    updateMerits(merits);

    incrementUsageCount(QString("Report/%1").arg(name));

    // --- Executor Loop ---
    bool skipNextAction = false;
    for (const ScriptAction &action : rep.actions) {

        if (skipNextAction) {
            skipNextAction = false;
            if (action.type != ScriptActionType::If && action.type != ScriptActionType::NotIf) {
                continue;
            }
        }

        switch (action.type) {
        case ScriptActionType::If:
            if (!evaluateCondition(action.value)) skipNextAction = true;
            break;
        case ScriptActionType::NotIf:
            if (evaluateCondition(action.value)) skipNextAction = true;
            break;
        case ScriptActionType::ProcedureCall:
            runProcedure(action.value.split(",").first().trimmed().toLower());
            break;
        case ScriptActionType::SetFlag:
            setFlag(action.value);
            break;
        case ScriptActionType::RemoveFlag:
        case ScriptActionType::ClearFlag:
            removeFlag(action.value);
            break;
        case ScriptActionType::SetCounterVar: {
            QStringList parts = action.value.split(",", Qt::SkipEmptyParts);
            if (parts.size() == 2) {
                QString varName = parts[0].trimmed();
                if (varName.startsWith("#")) varName = varName.mid(1);

                QString valToSet = parts[1].trimmed();
                if (valToSet.startsWith("#")) {
                    valToSet = scriptParser->getVariable(valToSet.mid(1));
                }
                scriptParser->setVariable(varName, valToSet);
                qDebug() << "[DEBUG] Variable set from procedure:" << varName << "=" << valToSet;
            } else {
                qDebug() << "[WARN] Malformed Set# in procedure:" << action.value;
            }
            break;
        }

        case ScriptActionType::AddCounter:
            performCounterOperation(action.value, "add");
            break;
        case ScriptActionType::SubtractCounter:
            performCounterOperation(action.value, "subtract");
            break;
        case ScriptActionType::MultiplyCounter:
            performCounterOperation(action.value, "multiply");
            break;
        case ScriptActionType::DivideCounter:
            performCounterOperation(action.value, "divide");
            break;
        case ScriptActionType::Message:
            QMessageBox::information(this, tr("Report"), replaceVariables(action.value));
            break;
        case ScriptActionType::NewStatus:
            changeStatus(action.value, false);
            break;
        case ScriptActionType::NewSubStatus:
            changeStatus(action.value, true);
            break;
        case ScriptActionType::MarkDone: {
            QString name = action.value;
            bool isPun = name.startsWith("punishment-", Qt::CaseInsensitive);
            markAssignmentDone(isPun ? name.mid(11) : name.mid(4), isPun);
            break;
        }
        case ScriptActionType::Abort: {
            QString name = action.value;
            bool isPun = name.startsWith("punishment-", Qt::CaseInsensitive);
            abortAssignment(isPun ? name.mid(11) : name.mid(4), isPun);
            break;
        }
        case ScriptActionType::Delete: {
            QString name = action.value;
            bool isPun = name.startsWith("punishment-", Qt::CaseInsensitive);
            deleteAssignment(isPun ? name.mid(11) : name.mid(4), isPun);
            break;
        }
        case ScriptActionType::AddMerit:
            modifyMerits(action.value, "add");
            break;
        case ScriptActionType::SubtractMerit:
            modifyMerits(action.value, "subtract");
            break;
        case ScriptActionType::SetMerit:
            modifyMerits(action.value, "set");
            break;
        case ScriptActionType::Punish:
            executePunish(action.value, rep.punishMessage, rep.punishGroup);
            break;

        default:
            qDebug() << "[WARN] Unhandled action type in report:" << static_cast<int>(action.type);
            break;
        }
    }
}

bool CyberDom::loadSessionData(const QString &path) {
    if (!QFile::exists(path))
        return false;

    QSettings session(path, QSettings::IniFormat);
    QString script = session.value("Session/ScriptPath").toString();
    int merits = session.value("Session/Merits", 0).toInt();
    QString status = session.value("Session/Status").toString();
    lastInstructions = session.value("Session/LastInstructions").toString();
    lastClothingInstructions = session.value("Session/LastClothingInstructions").toString();
    QDateTime lastInternal =
        QDateTime::fromString(session.value("Session/InternalClock").toString(),
                              Qt::ISODate);
    QDateTime lastSystem =
        QDateTime::fromString(session.value("Session/LastSystemTime").toString(),
                              Qt::ISODate);
    session.beginGroup("Assignments");
    QStringList assignments = session.value("ActiveJobs").toStringList();
    QSet<QString> loadedAssignments;
    for (const QString &name : assignments) {
        if (name.startsWith("job-"))
            loadedAssignments.insert(name.mid(4));
        else if (name.startsWith("punishment-"))
            loadedAssignments.insert(name.mid(11));
        else
            loadedAssignments.insert(name);
    }
    activeAssignments = loadedAssignments;

    session.beginGroup("Deadlines");
    const QStringList deadlineKeys = session.childKeys();
    for (const QString &key : deadlineKeys) {
        QDateTime dt = QDateTime::fromString(session.value(key).toString(), Qt::ISODate);
        if (dt.isValid()) {
            QString cleanKey = key;
            if (cleanKey.startsWith("job-"))
                cleanKey = cleanKey.mid(4);
            else if (cleanKey.startsWith("punishment-"))
                cleanKey = cleanKey.mid(11);

            jobDeadlines[cleanKey] = dt;
        }
    }
    session.endGroup(); // Deadlines

    session.beginGroup("Expiry");
    const QStringList expKeys = session.childKeys();
    for (const QString &key : expKeys) {
        QDateTime dt = QDateTime::fromString(session.value(key).toString(), Qt::ISODate);
        if (dt.isValid())
            jobExpiryTimes[key] = dt;
    }
    session.endGroup();

    session.beginGroup("RemindIntervals");
    const QStringList riKeys = session.childKeys();
    for (const QString &key : riKeys) {
        jobRemindIntervals[key] = session.value(key).toInt();
    }
    session.endGroup();

    session.beginGroup("NextReminds");
    const QStringList nrKeys = session.childKeys();
    for (const QString &key : nrKeys) {
        QDateTime dt = QDateTime::fromString(session.value(key).toString(), Qt::ISODate);
        if (dt.isValid())
            jobNextReminds[key] = dt;
    }
    session.endGroup();

    session.beginGroup("LateMerits");
    const QStringList lmKeys = session.childKeys();
    for (const QString &key : lmKeys) {
        jobLateMerits[key] = session.value(key).toInt();
    }
    session.endGroup();

    session.beginGroup("Amounts");
    const QStringList amtKeys = session.childKeys();
    for (const QString &key : amtKeys) {
        punishmentAmounts[key] = session.value(key).toInt();
    }
    session.endGroup();
    session.endGroup(); // Assignments

    session.beginGroup("Flags");
    const QStringList flagGroups = session.childGroups();
    for (const QString &fname : flagGroups) {
        session.beginGroup(fname);
        FlagData fd;
        fd.name = fname;
        fd.setTime = QDateTime::fromString(session.value("SetTime").toString(), Qt::ISODate);
        fd.expiryTime = QDateTime::fromString(session.value("ExpiryTime").toString(), Qt::ISODate);
        fd.text = session.value("Text").toString();
        QString groupsStr = session.value("Groups").toString();
        if (!groupsStr.isEmpty())
            fd.groups = groupsStr.split(',', Qt::SkipEmptyParts);
        flags[fname] = fd;
        session.endGroup();
    }
    session.endGroup();

    session.beginGroup("Answers");
    const QStringList ansKeys = session.childKeys();
    for (const QString &key : ansKeys) {
        questionAnswers[key] = session.value(key).toString();
    }
    session.endGroup();

    session.beginGroup("Counters");
    session.beginGroup("Reports");
    const QStringList repKeys = session.childKeys();
    for (const QString &key : repKeys) {
        reportCounts[key] = session.value(key).toInt();
    }
    session.endGroup();
    session.beginGroup("Confessions");
    const QStringList confKeys = session.childKeys();
    for (const QString &key : confKeys) {
        confessionCounts[key] = session.value(key).toInt();
    }
    session.endGroup();
    session.beginGroup("Permissions");
    const QStringList permKeys = session.childKeys();
    for (const QString &key : permKeys) {
        permissionCounts[key] = session.value(key).toInt();
    }
    session.endGroup();
    session.endGroup(); // Counters

    lastScheduledJobsRun = QDate::fromString(
        session.value("Session/LastJobsRun").toString(), Qt::ISODate
    );

    lastDayMeritsGiven = QDate::fromString(
                session.value("Session/LastDayMeritsGiven").toString(), Qt::ISODate
    );

    if (script.isEmpty())
        return false;

    if (!QFile::exists(script)) {
        QMessageBox::warning(this, tr("Script Not Found"),
                             tr("The script referenced in the session could not be found:\n%1")
                                 .arg(script));
        return false;
    }

    loadAndParseScript(script);
    updateMerits(merits);
    if (!status.isEmpty()) {
        currentStatus = status;
        updateStatusDisplay();
    }

    if (lastInternal.isValid() && lastSystem.isValid()) {
        qint64 diffSecs = lastSystem.secsTo(QDateTime::currentDateTime());
        internalClock = lastInternal.addSecs(diffSecs);
    } else {
        internalClock = QDateTime::currentDateTime();
    }

    saveIniFilePath(script);
    emit jobListUpdated();
    return true;
}

void CyberDom::saveSessionData(const QString &path) const {
    QSettings session(path, QSettings::IniFormat);
    session.setValue("Session/ScriptPath", currentIniFile);
    session.setValue("Session/Merits", ui->progressBar->value());
    session.setValue("Session/Status", currentStatus);
    session.setValue("Session/InternalClock",
                     internalClock.toString(Qt::ISODate));
    session.setValue("Session/LastSystemTime",
                     QDateTime::currentDateTime().toString(Qt::ISODate));
    session.setValue("Session/LastInstructions", lastInstructions);
    session.setValue("Session/LastClothingInstructions", lastClothingInstructions);
    session.setValue("Session/LastJobsRun", lastScheduledJobsRun.toString(Qt::ISODate));
    session.setValue("Session/LastDayMeritsGiven", lastDayMeritsGiven.toString(Qt::ISODate));

    // Persist active assignments and their deadlines
    session.beginGroup("Assignments");
    session.setValue("ActiveJobs", QStringList(activeAssignments.values()));
    session.beginGroup("Deadlines");
    for (auto it = jobDeadlines.constBegin(); it != jobDeadlines.constEnd(); ++it) {
        session.setValue(it.key(), it.value().toString(Qt::ISODate));
    }
    session.endGroup(); // Deadlines
    session.beginGroup("Expiry");
    for (auto it = jobExpiryTimes.constBegin(); it != jobExpiryTimes.constEnd(); ++it) {
        session.setValue(it.key(), it.value().toString(Qt::ISODate));
    }
    session.endGroup(); // Expiry
    session.beginGroup("RemindIntervals");
    for (auto it = jobRemindIntervals.constBegin(); it != jobRemindIntervals.constEnd(); ++it) {
        session.setValue(it.key(), it.value());
    }
    session.endGroup();
    session.beginGroup("NextReminds");
    for (auto it = jobNextReminds.constBegin(); it != jobNextReminds.constEnd(); ++it) {
        session.setValue(it.key(), it.value().toString(Qt::ISODate));
    }
    session.endGroup();
    session.beginGroup("LateMerits");
    for (auto it = jobLateMerits.constBegin(); it != jobLateMerits.constEnd(); ++it) {
        session.setValue(it.key(), it.value());
    }
    session.endGroup();
    session.beginGroup("Amounts");
    for (auto it = punishmentAmounts.constBegin(); it != punishmentAmounts.constEnd(); ++it) {
        session.setValue(it.key(), it.value());
    }
    session.endGroup();
    session.endGroup(); // Assignments

    // Persist flags
    session.beginGroup("Flags");
    for (auto it = flags.constBegin(); it != flags.constEnd(); ++it) {
        const FlagData &fd = it.value();
        session.beginGroup(it.key());
        session.setValue("SetTime", fd.setTime.toString(Qt::ISODate));
        session.setValue("ExpiryTime", fd.expiryTime.toString(Qt::ISODate));
        session.setValue("Text", fd.text);
        session.setValue("Groups", fd.groups.join(","));
        session.endGroup();
    }
    session.endGroup();

    // Persist question answers
    session.beginGroup("Answers");
    for (auto it = questionAnswers.constBegin(); it != questionAnswers.constEnd(); ++it) {
        session.setValue(it.key(), it.value());
    }
    session.endGroup();

    // Persist menu usage counters
    session.beginGroup("Counters");
    session.beginGroup("Reports");
    for (auto it = reportCounts.constBegin(); it != reportCounts.constEnd(); ++it) {
        session.setValue(it.key(), it.value());
    }
    session.endGroup();
    session.beginGroup("Confessions");
    for (auto it = confessionCounts.constBegin(); it != confessionCounts.constEnd(); ++it) {
        session.setValue(it.key(), it.value());
    }
    session.endGroup();
    session.beginGroup("Permissions");
    for (auto it = permissionCounts.constBegin(); it != permissionCounts.constEnd(); ++it) {
        session.setValue(it.key(), it.value());
    }
    session.endGroup();
    session.endGroup(); // Counters

    session.sync();

    QSettings::Status stat = session.status();
    if (stat != QSettings::NoError) {
        qWarning() << "[CyberDom] Failed to save session data to" << path
                   << "- status:" << stat;
    } else {
        qDebug() << "[CyberDom] Session data saved to" << path;
    }
}

void CyberDom::updateSigninTimer() {
    signinRemainingSecs--;

    int displaySecs = qAbs(signinRemainingSecs);
    QTime t(0, 0);
    t = t.addSecs(displaySecs);
    ui->timerLabel->setText(t.toString("hh:mm:ss"));

    if (signinRemainingSecs <= 0)
        ui->timerLabel->setStyleSheet("QLabel { color: red; }");
}

void CyberDom::onResetSigninTimer() {
    if (checkInterruptableAssignments()) return;

    StatusSection status = scriptParser->getStatus(currentStatus);
    if (status.signinIntervalMin.isEmpty())
        return;

    if (signinRemainingSecs <= 0) {
        int lateMinutes = qCeil((-signinRemainingSecs) / 60.0);
        int severity = status.signinPenalty1 + lateMinutes * status.signinPenalty2;
        applyPunishment(severity, status.signinPenaltyGroup);
        QString eventProc = scriptParser->getScriptData().events.signIn;
        if (!eventProc.isEmpty())
            runProcedure(eventProc);
    }

    signinRemainingSecs = parseTimeRangeToSeconds(status.signinIntervalMin + "," + status.signinIntervalMax);
    ui->timerLabel->setStyleSheet("");
    signinTimer->start(1000);

    QString proc = scriptParser->getScriptData().eventHandlers.signIn;
    if (!proc.isEmpty())
        runProcedure(proc);
}

int CyberDom::parseTimeToSeconds(const QString &timeStr) const {
    QTime t = QTime::fromString(timeStr.trimmed(), "H:mm:ss");
    if (!t.isValid())
        t = QTime::fromString(timeStr.trimmed(), "HH:mm:ss");
    if (!t.isValid())
        t = QTime::fromString(timeStr.trimmed(), "h:mm:ss");
    if (!t.isValid())
        t = QTime::fromString(timeStr.trimmed(), "H:mm");
    if (!t.isValid())
        t = QTime::fromString(timeStr.trimmed(), "HH:mm");
    if (!t.isValid())
        return 0;
    return t.hour() * 3600 + t.minute() * 60 + t.second();
}

int CyberDom::parseTimeRangeToSeconds(const QString &range) const {
    QStringList parts = range.split(',', Qt::SkipEmptyParts);
    if (parts.size() == 2) {
        int minSec = parseTimeToSeconds(parts[0]);
        int maxSec = parseTimeToSeconds(parts[1]);
        return ScriptUtils::randomInRange(minSec, maxSec, false);
    }
    return parseTimeToSeconds(range);
}

bool CyberDom::isTimeAllowed(const QStringList &notBefore,
                             const QStringList &notAfter,
                             const QList<QPair<QString, QString>> &notBetween) const {
    QTime now = internalClock.time();
    int nowSecs = now.hour() * 3600 + now.minute() * 60 + now.second();

    for (const QString &t : notBefore) {
        int secs = parseTimeToSeconds(t);
        if (nowSecs < secs)
            return false;
    }

    for (const QString &t : notAfter) {
        int secs = parseTimeToSeconds(t);
        if (nowSecs > secs)
            return false;
    }

    for (const auto &pair : notBetween) {
        int start = parseTimeToSeconds(pair.first);
        int end = parseTimeToSeconds(pair.second);
        if (start <= end) {
            if (nowSecs >= start && nowSecs <= end)
                return false;
        } else {
            if (nowSecs >= start || nowSecs <= end)
                return false;
        }
    }

    return true;
}

bool CyberDom::isInterruptAllowed() const
{
    if (!scriptParser) return true; // Failsafe, allow popups

    const auto& interruptStatuses = scriptParser->getScriptData().general.interruptStatuses;

    // If the list is empty, it means no restrictions are set, so popups are allowed
    if (interruptStatuses.isEmpty()) {
        return true;
    }

    // If the list is *not* empty, the current status MUST be in the list
    QString lowerCurrentStatus = currentStatus.toLower();
    for (const QString &allowedStatus : interruptStatuses) {
        if (allowedStatus.toLower() == lowerCurrentStatus) {
            return true;
        }
    }

    // No match was found. Suppress the popup.
    qDebug() << "[Interrupt] Popup suppressed in status:" << currentStatus;
    return false;
}

QString CyberDom::formatDuration(int seconds) const {
    if (seconds < 0)
        seconds = 0;
    if (seconds < 60)
        return QString::number(seconds) + " Second" + (seconds == 1 ? "" : "s");
    if (seconds < 3600) {
        int mins = (seconds + 59) / 60;
        return QString::number(mins) + " Minute" + (mins == 1 ? "" : "s");
    }
    int hours = (seconds + 3599) / 3600;
    return QString::number(hours) + " Hour" + (hours == 1 ? "" : "s");
}

int CyberDom::randomIntFromRange(const QString &range) const {
    QStringList parts = range.split(',', Qt::SkipEmptyParts);
    if (parts.size() == 2) {
        int min = parts[0].toInt();
        int max = parts[1].toInt();
        return ScriptUtils::randomInRange(min, max, false);
    }
    return range.toInt();
}

void CyberDom::incrementUsageCount(const QString &key) {
    QSettings settings(settingsFile, QSettings::IniFormat);
    int count = settings.value(QStringLiteral("Usage/%1").arg(key), 0).toInt();
    settings.setValue(QStringLiteral("Usage/%1").arg(key), count + 1);
    settings.sync();
}

void CyberDom::setDefaultDeadlineForJob(const QString &jobName) {
    QDateTime deadline(internalClock.date(), QTime(23, 59, 59));
    jobDeadlines[jobName] = deadline;
    qDebug() << "[DEBUG] Job default deadline set: " << jobName << " - "
             << deadline.toString("MM-dd-yyyy hh:mm AP");
}

QStringList CyberDom::getAssignmentResources(const QString &name, bool isPunishment) const
{
    if (!scriptParser)
        return {};

    const auto &data = scriptParser->getScriptData();
    QString key = name.toLower();
    if (isPunishment) {
        for (auto it = data.punishments.constBegin(); it != data.punishments.constEnd(); ++it) {
            if (it.key().toLower() == key)
                return it.value().resources;
        }
    } else {
        for (auto it = data.jobs.constBegin(); it != data.jobs.constEnd(); ++it) {
            if (it.key().toLower() == key)
                return it.value().resources;
        }
    }
    return {};
}

bool CyberDom::isAssignmentLongRunning(const QString &name, bool isPunishment) const
{
    if (!scriptParser) return false;

    const auto &data = scriptParser->getScriptData();
    QString lowerName = name.toLower();

    if (isPunishment) {
        if (data.punishments.contains(lowerName)) {
            const PunishmentDefinition &def = data.punishments.value(lowerName);

            // Rule: ValueUnit=day implies LongRunning
            if (def.valueUnit.compare("day", Qt::CaseInsensitive) == 0) {
                return true;
            }

            return def.longRunning;
        }
    } else {
        // Jobs
        if (data.jobs.contains(lowerName)) {
            return data.jobs.value(lowerName).longRunning;
        }
    }
    return false;
}

QList<CalendarEvent> CyberDom::getCalendarEvents()
{
    QList<CalendarEvent> events;

    // Job and punishments stored in jobDeadlines
    for (auto it = jobDeadlines.constBegin(); it != jobDeadlines.constEnd(); ++it) {
        const QString &name = it.key();
        QDateTime deadline = it.value();
        CalendarEvent ev;
        ev.title = name;

        // Check if the name exists in the parsed punishment map
        bool isPun = false;
        if (scriptParser) {
            isPun = scriptParser->getScriptData().punishments.contains(name.toLower());
        }

        ev.type = isPun ? QStringLiteral("Punishment") : QStringLiteral("Job");

        if (isPun && isAssignmentLongRunning(name, true)) {
            QSettings settings(settingsFile, QSettings::IniFormat);
            QDateTime start = settings.value(QStringLiteral("Assignments/%1_start_time").arg(name)).toDateTime();
            if (!start.isValid())
                start = internalClock;
            ev.start = start;
            ev.end = deadline;
        } else {
            ev.start = deadline;
            ev.end = deadline;
        }
        events.append(ev);
    }

    // Add holidays if available in script
    if (scriptParser) {
        QMap<QString, QStringList> holSec = scriptParser->getRawSectionData(QStringLiteral("holidays"));
        for (auto it = holSec.constBegin(); it != holSec.constEnd(); ++it) {
            for (const QString &dateStr : it.value()) {
                QDate date = QDate::fromString(dateStr.trimmed(), Qt::ISODate);
                if (!date.isValid())
                    continue;
                CalendarEvent ev;
                ev.start = QDateTime(date, QTime(0,0));
                ev.end = QDateTime(date, QTime(23,59,59));
                ev.title = it.key();
                ev.type = QStringLiteral("Holiday");
                events.append(ev);
            }
        }

        // Add birthdays if available
        QStringList secs = scriptParser->getRawSectionNames();
        for (const QString &sec : secs) {
            if (!sec.startsWith(QStringLiteral("birthday-")))
                continue;
            QMap<QString, QStringList> data = scriptParser->getRawSectionData(sec);
            QString title = sec.mid(QStringLiteral("birthday-").length());
            if (data.contains(QStringLiteral("Title")))
                title = data[QStringLiteral("Title")].value(0);
            QStringList dates = data.value(QStringLiteral("Date"));
            if (dates.isEmpty())
                dates = data.value(QStringLiteral("date"));
            for (const QString &dateStr : dates) {
                QDate date = QDate::fromString(dateStr.trimmed(), Qt::ISODate);
                if (!date.isValid())
                    continue;
                CalendarEvent ev;
                ev.start = QDateTime(date, QTime(0,0));
                ev.end = QDateTime(date, QTime(23,59,59));
                ev.title = title;
                ev.type = QStringLiteral("Birthday");
                events.append(ev);
            }
        }
    }

    // Add US Holidays
    int year = internalClock.date().year();
    for (int y = year - 1; y <= year + 1; ++y)
        events.append(generateUSHolidays(y));

    return events;
}

QSet<QString> CyberDom::getResourcesInUse() const
{
    QSet<QString> used;
    if (!scriptParser)
        return used;

    for (const QString &name : activeAssignments) {
        // Check if the name exists in the parsed punishments map
        bool isPun = scriptParser->getScriptData().punishments.contains(name.toLower());

        QString flag = (isPun ? "punishment_" : "job_") + name + "_started";
        if (!isFlagSet(flag))
            continue;

        if (!isAssignmentLongRunning(name, isPun))
            continue;
        for (const QString &res : getAssignmentResources(name, isPun))
            used.insert(res);
    }
    return used;
}

bool CyberDom::hasActiveBlockingPunishment() const
{
    if (!scriptParser)
        return false;

    const auto &data = scriptParser->getScriptData();

    for (const QString &name : activeAssignments) {
        QString lowerName = name.toLower();

        // Only punishments can block other tasks
        if (!data.punishments.contains(lowerName)) {
            continue;
        }

        // Check if the punishment has explicitly STARTED
        QString flag = "punishment_" + name + "_started";
        if (!isFlagSet(flag)) {
            continue;
        }

        // Check if it is Long-Running
        if (isAssignmentLongRunning(name, true)) {
            continue;
        }

        // If we are here, then the punishment is not Long-Running, and all other Jobs and Punishments are blocked
        qDebug() << "[Status] Blocking punishment active:" << name;
        return true;
    }

    return false;
}

bool CyberDom::evaluateCondition(const QString &condition)
{
    QString op;
    if (condition.contains("!=")) op = "!=";
    else if (condition.contains("=")) op = "=";
    else if (condition.contains(">")) op = ">";
    else if (condition.contains("<")) op = "<";
    else {
        // Fallback for simple flag check (no operator)
        return isFlagSet(condition);
    }

    QStringList parts = condition.split(op);
    if (parts.size() != 2) {
        qDebug() << "[WARN] Malformed condition:" << condition;
        return false;
    }

    // Helper lambda to resolve a token to its value
    auto getResolvedValue = [&](const QString &token) -> QString {
        if (token.startsWith("#")) {
            QString varName = token.mid(1);
            return scriptParser->getVariable(varName);
        }
        return token;
    };

    QString leftVal = getResolvedValue(parts[0].trimmed());
    QString rightVal = getResolvedValue(parts[1].trimmed());

    // Try numeric comparison first
    bool leftOk, rightOk;
    int leftInt = leftVal.toInt(&leftOk);
    int rightInt = rightVal.toInt(&rightOk);

    if (leftOk && rightOk) {
        if (op == "=") return leftInt == rightInt;
        if (op == "!=") return leftInt != rightInt;
        if (op == ">") return leftInt > rightInt;
        if (op == "<") return leftInt < rightInt;
    }

    // Fallback to string comparison for = and !=
    if (op == "=") return leftVal == rightVal;
    if (op == "!=") return leftVal == rightVal;

    qDebug() << "[WARN] Non-numeric coparison for" << op << "in:" << condition;
    return false;
}

QString CyberDom::getAssignmentEstimate(const QString &assignmentName, bool isPunishment) const
{
    if (!scriptParser) return QString("N/A");

    const ScriptData &data = scriptParser->getScriptData();
    QString lowerName = assignmentName.toLower();
    QString estimateStr;

    // Get the estimate string from the correct struct
    if (isPunishment) {
        if (data.punishments.contains(lowerName)) {
            estimateStr = data.punishments.value(lowerName).estimate;
        }
    } else {
        if (data.jobs.contains(lowerName)) {
            estimateStr = data.jobs.value(lowerName).estimate;
        }
    }

    if (estimateStr.isEmpty()) {
        return "N/A";
    }

    // Resolve the string if it's a variable
    if (estimateStr.startsWith("!") || estimateStr.startsWith("#")) {
        // Get the variable's value, which should be "hh:mm"
        estimateStr = scriptParser->getVariable(estimateStr.mid(1));
    }

    return estimateStr;
}

void CyberDom::openDataInspector()
{
    if (!scriptParser) {
        QMessageBox::warning(this, "Error", "Script is not loaded.");
        return;
    }

    DataInspectorDialog *dialog = new DataInspectorDialog(scriptParser, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

const PunishmentDefinition* CyberDom::getPunishmentDefinition(const QString &instanceName) const
{
    if (!scriptParser) return nullptr;

    const auto &punishments = scriptParser->getScriptData().punishments;
    QString lowerName = instanceName.toLower();

    // Try exact match
    auto it = punishments.constFind(lowerName);
    if (it != punishments.constEnd()) {
        return &(*it);
    }

    // Try stripping suffix (_2, _3, etc.)
    int lastUnderscore = lowerName.lastIndexOf('_');
    if (lastUnderscore != -1) {
        QString baseName = lowerName.left(lastUnderscore);

        // Check if the suffix is a number
        bool isNumber;
        lowerName.mid(lastUnderscore + 1).toInt(&isNumber);

        if (isNumber) {
            auto itBase = punishments.constFind(baseName);
            if (itBase != punishments.constEnd()) {
                return &(*itBase);
            }
        }
    }

    return nullptr;
}
