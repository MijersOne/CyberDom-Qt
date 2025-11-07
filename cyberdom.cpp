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

            if (!timer.message.isEmpty()) {
                QMessageBox::information(this, "Timer", replaceVariables(timer.message));
            }

            if (!timer.sound.isEmpty()) {
                playSoundSafe(timer.sound);
            }

            if (!timer.procedure.isEmpty()) {
                runProcedure(timer.procedure);
            }

            if (scriptParser) {
                const auto &defs = scriptParser->getScriptData().timers;
                if (defs.contains(timer.name)) {
                    const TimerDefinition &td = defs.value(timer.name);
                    for (const QString &procName : td.procedures) {
                        runProcedure(procName);
                    }
                }
            }

            timer.triggered = true;
        }
    }
}

void CyberDom::openAboutDialog()
{
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
    AskClothing askclothingDialog(this, scriptParser); // Create the AskClothing dialog with parser
    askclothingDialog.exec(); // Show the dialog modally
}

void CyberDom::openAskInstructionsDialog()
{
    AskInstructions askinstructionsDialog(this); // Create the AskInstructions dialog, passing the parent
    askinstructionsDialog.exec(); // Show the dialog modally
}

void CyberDom::openReportClothingDialog()
{
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
    AddClothing dlg(this, tr("Generic"));
    dlg.exec();
}

void CyberDom::openReport(const QString &name)
{
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
    if (!scriptParser)
        return;

    const auto &perms = scriptParser->getScriptData().permissions;
    if (!perms.contains(name)) {
        qWarning() << "[CyberDom] Permission definition not found:" << name;
        return;
    }

    const PermissionDefinition &perm = perms.value(name);
    if (!isTimeAllowed(perm.notBeforeTimes, perm.notAfterTimes, perm.notBetweenTimes)) {
        QMessageBox::information(this, tr("Permission"),
                                 tr("This permission is not available at this time."));
        return;
    }
    incrementUsageCount(QString("Permission/%1").arg(name));

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

    if (!perm.beforeProcedure.isEmpty())
        runProcedure(perm.beforeProcedure);

    bool granted = QMessageBox::question(this, tr("Permission"),
                                         tr("Grant permission '%1'?")
                                             .arg(perm.title.isEmpty() ? perm.name : perm.title),
                                         QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;

    if (granted) {
        if (!perm.permitMessage.isEmpty())
            QMessageBox::information(this, tr("Permission"), replaceVariables(perm.permitMessage));
        QString eventProc = scriptParser->getScriptData().eventHandlers.permissionGiven;
        if (!eventProc.isEmpty())
            runProcedure(eventProc);
    } else {
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
    if (!scriptParser)
        return;

    const auto &confs = scriptParser->getScriptData().confessions;
    if (!confs.contains(name)) {
        qWarning() << "[CyberDom] Confession definition not found:" << name;
        return;
    }

    const ConfessionDefinition &conf = confs.value(name);
    if (!isTimeAllowed(conf.notBeforeTimes, conf.notAfterTimes, conf.notBetweenTimes)) {
        QMessageBox::information(this, tr("Confession"),
                                 tr("This confession is not available at this time."));
        return;
    }
    incrementUsageCount(QString("Confession/%1").arg(name));

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

    QList<QuestionDefinition> questions;
    for (const QString &qname : conf.inputQuestions) {
        QuestionSection q = scriptParser->getQuestion(qname);
        if (!q.name.isEmpty())
            questions.append(q);
    }
    for (const QString &qname : conf.advancedQuestions) {
        QuestionSection q = scriptParser->getQuestion(qname);
        if (!q.name.isEmpty())
            questions.append(q);
    }

    if (!questions.isEmpty()) {
        for (QuestionDefinition &q : questions)
            q.phrase = replaceVariables(q.phrase);
        QuestionDialog dlg(questions, this);
        dlg.exec();
    } else if (!conf.noInputProcedure.isEmpty()) {
        runProcedure(conf.noInputProcedure);
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
    int minPunishment = getAskPunishmentMin();
    int maxPunishment = getAskPunishmentMax();

    AskPunishment askPunishmentDialog(this, minPunishment, maxPunishment); // Create the AskPunishment dialog, passing the parent
    askPunishmentDialog.exec(); // Show the dialog modally
}


void CyberDom::openChangeMeritsDialog()
{
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
    CalendarView view(this);
    view.exec();
}

void CyberDom::openSelectPunishmentDialog()
{
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
    SelectPopup selectPopupDialog(this); // Create the SelectPopup dialog, passing the parent
    selectPopupDialog.exec(); // Show the dialog modally
}

void CyberDom::openListFlagsDialog()
{
    ListFlags listFlagsDialog(this, &flags);    // Create the ListFlags dialog, passing the parent
    connect(&listFlagsDialog, &ListFlags::flagRemoveRequested, this, &CyberDom::removeFlag);
    listFlagsDialog.exec(); // Show the dialog modally
}

void CyberDom::openSetFlagsDialog()
{
    SetFlags setFlagsDialog(this); // Create the SetFlags dialog, passing the parent
    connect(&setFlagsDialog,&SetFlags::flagSetRequested, this, &CyberDom::setFlag);
    setFlagsDialog.exec(); // Show the dialog modally
}

void CyberDom::openDeleteAssignmentsDialog()
{
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
        bool isPun = punishmentDefs.contains(lowerName);
        QString startFlagName = (isPun ? "punishment_" : "job_") + assignmentName + "_started";

        if (isFlagSet(startFlagName)) {
            QStringList rawLines;
            QString minTimeStr;

            if (isPun) {
                const PunishmentDefinition &punDef = punishmentDefs.value(lowerName);
                rawLines.append(punDef.statusTexts);
                minTimeStr = punDef.duration.minTimeStart;
            } else if (jobDefs.contains(lowerName)) {
                const JobDefinition &jobDef = jobDefs.value(lowerName);
                if (!jobDef.text.isEmpty()) rawLines.append(jobDef.text);
                rawLines.append(jobDef.statusTexts);
                minTimeStr = jobDef.duration.minTimeStart;
            }

            QDateTime start = settings.value("Assignments/" + assignmentName + "_start_time").toDateTime();
            QString runTimeStr = "00:00:00";
            if (start.isValid()) {
                qint64 elapsedSecs = start.secsTo(internalClock);
                if (elapsedSecs < 0) elapsedSecs = 0;
                runTimeStr = QTime(0, 0).addSecs(elapsedSecs).toString("HH:mm:ss");
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

// void CyberDom::updateStatusText() {
//     ui->textBrowser->clear();

//     // Use string lists to build the three main parts of the display
//     QStringList topLines;
//     QStringList middleLines;
//     QStringList bottomLines;

//     if (!scriptParser) {
//         return; // Safety check
//     }

//     // Get the parsed data struct once
//     const ScriptData &data = scriptParser->getScriptData();

//     // --- 1. Populate TopText ---
//     // (This part is correct)
//     for (const QString &line : data.general.topText) {
//         topLines.append(replaceVariables(line));
//     }

//     // --- 2. Populate BottomText ---
//     // (This part is correct)
//     for (const QString &line : data.general.bottomText) {
//         bottomLines.append(replaceVariables(line));
//     }

//     // --- 3. Populate Middle Content ---

//     // A. Get Status Text
//     // (This part is correct)
//     if (data.statuses.contains(currentStatus)) {
//         const StatusDefinition &statusDef = data.statuses.value(currentStatus);
//         middleLines.append(statusDef.statusTexts);
//     }

//     // B. Add Flag Text
//     // (This part is correct)
//     QMapIterator<QString, FlagData> flagIter(flags);
//     while (flagIter.hasNext()) {
//         flagIter.next();
//         const FlagData &flagData = flagIter.value();
//         if (!flagData.text.isEmpty()) {
//             middleLines.append(flagData.text);
//         }
//     }

//     // --- C. Add *Started* Assignment Text (Jobs & Punishments) ---
//     QStringList assignmentTexts;
//     const auto &punishmentDefs = data.punishments;
//     const auto &jobDefs = data.jobs;
//     QSettings settings(settingsFile, QSettings::IniFormat); // Define settings once

//     // Iterate all *active* assignments
//     for (const QString &assignmentName : activeAssignments) {
//         QString lowerName = assignmentName.toLower();
//         bool isPun = punishmentDefs.contains(lowerName);

//         // 1. Check if the assignment has been STARTED
//         QString startFlagName = (isPun ? "punishment_" : "job_") + assignmentName + "_started";
//         if (isFlagSet(startFlagName)) {

//             QStringList rawLines;
//             QString minTimeStr;

//             // 2. Get the raw text lines and MinTime
//             if (isPun) {
//                 const PunishmentDefinition &punDef = punishmentDefs.value(lowerName);
//                 rawLines.append(punDef.statusTexts);
//                 minTimeStr = punDef.duration.minTimeStart; // Get min time
//             } else if (jobDefs.contains(lowerName)) {
//                 const JobDefinition &jobDef = jobDefs.value(lowerName);
//                 if (!jobDef.text.isEmpty()) rawLines.append(jobDef.text); // Get Text=
//                 rawLines.append(jobDef.statusTexts); // Get StatusTexts=
//                 minTimeStr = jobDef.duration.minTimeStart; // Get min time
//             }

//             // 3. Get the live {!zzRunTime}
//             QDateTime start = settings.value("Assignments/" + assignmentName + "_start_time").toDateTime();
//             QString runTimeStr = "00:00:00";
//             if (start.isValid()) {
//                 // Calculate elapsed seconds against the live internal clock
//                 qint64 elapsedSecs = start.secsTo(internalClock);
//                 if (elapsedSecs < 0) elapsedSecs = 0;
//                 // Format as HH:mm:ss
//                 runTimeStr = QTime(0, 0).addSecs(elapsedSecs).toString("HH:mm:ss");
//             }

//             // 4. Process lines and replace variables
//             for (QString line : rawLines) {
//                 if (line.isEmpty()) continue;

//                 // Perform our job-specific replacements
//                 line.replace("{!zzMinTime}", minTimeStr);
//                 line.replace("{!zzRunTime}", runTimeStr);

//                 // Add the processed line to the list
//                 assignmentTexts.append(line);
//             }
//         }
//     }

//     if (!assignmentTexts.isEmpty()) {
//         middleLines.append(""); // Add a spacer line
//         middleLines.append("Assignments"); // A more generic title
//         middleLines.append(assignmentTexts);
//     }


//     // --- 4. Assemble and Set Text ---

//     // Join individual parts
//     QString finalTop = topLines.join("\n");
//     QString finalMiddle = middleLines.join("\n");
//     QString finalBottom = bottomLines.join("\n");

//     // Apply global placeholders (%instruction%, %clothing%)
//     finalMiddle.replace("%instructions", lastInstructions);
//     finalMiddle.replace("%clothing", lastClothingInstructions);

//     // Now apply variable replacement to the entire middle block
//     finalMiddle = replaceVariables(finalMiddle);

//     // Build the final text string with proper spacing
//     QString finalText = "";

//     if (!finalTop.isEmpty()) {
//         finalText += finalTop;
//     }

//     if (!finalMiddle.isEmpty()) {
//         if (!finalText.isEmpty()) {
//             finalText += "\n\n";
//         }
//         finalText += finalMiddle;
//     }

//     if (!finalBottom.isEmpty()) {
//         if (!finalText.isEmpty()) {
//             finalText += "\n\n";
//         }
//         finalText += finalBottom;
//     }

//     // Check if the text has changed. If not, do nothing.
//     if (finalText == lastDisplayedStatusText) {
//         return;
//     }

//     // The text has changed. Save scrollbar's relative position.
//     QScrollBar *scrollbar = ui->textBrowser->verticalScrollBar();
//     int oldScrollValue = scrollbar->value();
//     int oldScrollMax = scrollbar->maximum();

//     // Check if we are scrolled to the bottom (or very close to it)
//     bool isAtBottom = (oldScrollMax > 0 && oldScrollValue >= (oldScrollMax -10));

//     // Save relative position
//     double relativePosition = 0.0;
//     if (oldScrollMax > 0) {
//         relativePosition = static_cast<double>(oldScrollValue) / oldScrollMax;
//     }

//     // Set the new text
//     ui->textBrowser->setText(finalText);

//     // Use a zero-delay timer to restore the position
//     QTimer::singleShot(0, this, [scrollbar, relativePosition, isAtBottom]() {
//         if (isAtBottom) {
//             // If we were at the bottom, stay at the bottom
//             scrollbar->setValue(scrollbar->maximum());
//         } else {
//             // Otherwise, restore the relative position
//             int newPosition = static_cast<int>(relativePosition * scrollbar->maximum());
//             scrollbar->setValue(newPosition);
//         }
//     });
// }

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
    testMenuEnabled = scriptParser->isTestMenuEnabled();

    // Setup UI from general settings
    initializeProgressBarRange();
    setWindowTitle(QString("%1's Virtual Master").arg(masterVariable));
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
    ui->progressBar->setValue(newMerits); // Update progressBar
    ui->lbl_Merits->setText("Merits: " + QString::number(newMerits)); // Update lbl_Merits

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

    const auto& jobs = scriptParser->getScriptData().jobs;
    for (const JobDefinition &job : jobs) {
        bool shouldRunToday = false;

        // Check the Run rules
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

        // Check the NoRun rules
        for (const QString &noRunDay : job.noRunDays) {
            if (currentDayName.compare(noRunDay, Qt::CaseInsensitive) == 0) {
                shouldRunToday = false;
                break;
            }
        }

        // If the job should run today and is not already assigned, add it.
        if (shouldRunToday && !activeAssignments.contains(job.name)) {
            addJobToAssignments(job.name);
            qDebug() << "[DEBUG] Job Auto-Assigned (" << currentDayName << "): " << job.name;
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

void CyberDom::addJobToAssignments(QString jobName)
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
            QStringList respiteParts = jobDef.respite.split(":");
            if (respiteParts.size() >= 2) {
                int hours = respiteParts[0].toInt();
                int minutes = respiteParts[1].toInt();
                deadline = internalClock.addSecs(hours * 3600 + minutes * 60);
                deadlineSet = true;
                qDebug() << "[DEBUG] Job deadline set from Respite: " << deadline.toString("MM-dd-yyyy hh:mm AP");
            }
        }

        // Check for EndTime
        if (!deadlineSet && !jobDef.endTimeMin.isEmpty()) {
            // Reconstruct the range string for the parser function
            QString endTimeRange = jobDef.endTimeMin;
            if (!jobDef.endTimeMax.isEmpty()) {
                endTimeRange += "," + jobDef.endTimeMax;
            }

            // Use parseTimeRangeToSeconds to get a random time in the range
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

        if (!deadlineSet) {
            deadline = QDateTime(internalClock.date(), QTime(23, 59, 59));
        }

        jobDeadlines[jobName] = deadline;
        qDebug() << "[DEBUG] Job deadline set: " << jobName << " - "
                 << deadline.toString("MM-dd-yyyy hh:mm AP");

        // Check for ExpireAfter
        if (!jobDef.expireAfterMin.isEmpty() || !jobDef.expireAfterMax.isEmpty()) {
            QString range = jobDef.expireAfterMin.isEmpty() ? jobDef.expireAfterMax : jobDef.expireAfterMin;
            if (!jobDef.expireAfterMin.isEmpty() && !jobDef.expireAfterMax.isEmpty()) {
                range = jobDef.expireAfterMin + "," + jobDef.expireAfterMax;
            }
            int expireSec = parseTimeRangeToSeconds(range);
            if (expireSec > 0)
                jobExpiryTimes[jobName] = deadline.addSecs(expireSec);
        }

        // Check for RemindInterval
        if (!jobDef.remindIntervalMin.isEmpty() || !jobDef.remindIntervalMax.isEmpty()) {
            QString range = jobDef.remindIntervalMin.isEmpty() ? jobDef.remindIntervalMax : jobDef.remindIntervalMin;
            if (!jobDef.remindIntervalMin.isEmpty() && !jobDef.remindIntervalMax.isEmpty()) {
                range = jobDef.remindIntervalMin + "," + jobDef.remindIntervalMax;
            }
            int intervalSec = parseTimeRangeToSeconds(range);
            if (intervalSec > 0)
                jobRemindIntervals[jobName] = intervalSec;
        }

        // Check for LateMerits
        if (jobDef.lateMeritsMin != 0 || jobDef.lateMeritsMax != 0) {
            QString range = QString::number(jobDef.lateMeritsMin);
            if(jobDef.lateMeritsMax > jobDef.lateMeritsMin) {
                range += "," + QString::number(jobDef.lateMeritsMax);
            }
            jobLateMerits[jobName] = randomIntFromRange(range);
        }
    } else {
        // Fallback if parser or jobDef doesn't exist
        setDefaultDeadlineForJob(jobName);
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
        bool isPun = data.punishments.contains(name.toLower());
        bool isJob = data.jobs.contains(name.toLower());

        if (!isPun && !isJob) {
            // This is an assignment that doesn't exist in the script data
            continue;
        }

        // --- Get properties (if they exist)
        QString remindPenaltyRange;
        QString remindPenaltyGroup;
        QString expirePenaltyRange;
        QString expirePenaltyGroup;
        QString expireProcedure;

        if (isJob) {
            const JobDefinition &jobDef = data.jobs.value(name.toLower());

            if (jobDef.remindPenaltyMin > 0 || jobDef.remindPenaltyMax > 0) {
                remindPenaltyRange = QString::number(jobDef.remindPenaltyMin);
                if (jobDef.remindPenaltyMax > jobDef.remindPenaltyMin) {
                    remindPenaltyRange += "," + QString::number(jobDef.remindPenaltyMax);
                }
            }
            expirePenaltyGroup = jobDef.expirePenaltyGroup;
            expireProcedure = jobDef.expireProcedure;
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
            // The assignment is already expired, and it's time for another reminder
            QMessageBox::information(this, "Assignment Late", QString("You are late completing %1").arg(name));

            // Apply reminder penalty (uses new variables)
            if (!remindPenaltyRange.isEmpty()) {
                int sev = randomIntFromRange(remindPenaltyRange);
                if (sev > 0)
                    applyPunishment(sev, remindPenaltyGroup);
            }

            jobNextReminds[name] = now.addSecs(jobRemindIntervals[name]);
        }

        // Check if the assignment has permanently expired
        if (jobExpiryTimes.contains(name) && now >= jobExpiryTimes[name]) {

            // Apply expiry penalty
            if (!expirePenaltyRange.isEmpty()) {
                int sev = randomIntFromRange(expirePenaltyRange);
                if (sev > 0)
                    applyPunishment(sev, expirePenaltyGroup);
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
    if (amount <= 0)
        return;

    bool alreadyActive = activeAssignments.contains(punishmentName);
    punishmentAmounts[punishmentName] += amount;

    // Get the punishment definition from the script parser
    const PunishmentDefinition *punDef = nullptr;
    if (scriptParser) {
        const auto &punishments = scriptParser->getScriptData().punishments;
        QString lowerName = punishmentName.toLower();

        // Use constFind() to get an iterator to the item
        auto it = punishments.constFind(lowerName);

        // Check if the iterator is valid
        if (it != punishments.constEnd()) {
            // Get the address of the item the iterator is pointing to
            punDef = &(*it);
        }
    }

    if (!punDef) {
        qDebug() << "[WARNING] Punishment section not found in ScriptData:" << punishmentName;
        // If it's not in the script, we can't process deadlines
        if (!alreadyActive) {
            emit jobListUpdated();
            qDebug() << "[DEBUG] jobListUpdated Signal Emitted!";
        }
        return;
    }

    if (!alreadyActive) {
        activeAssignments.insert(punishmentName);
        qDebug() << "[DEBUG] Punishment added to active assignments: " << punishmentName;

        if (scriptParser) {
            QString proc = scriptParser->getScriptData().eventHandlers.punishmentGiven;
            if (!proc.isEmpty())
                runProcedure(proc);
        }

        // Calculate deadline based on punishment parameters
        QDateTime deadline = internalClock;

        // Check if punishment has Respite value
        if (!punDef->respite.isEmpty()) {
            QStringList respiteParts = punDef->respite.split(":");

            if (respiteParts.size() >= 2) {
                int hours = respiteParts[0].toInt();
                int minutes = respiteParts[1].toInt();

                deadline = deadline.addSecs(hours * 3600 + minutes * 60);
                qDebug() << "[DEBUG] Punishment deadline set from Respite: " << deadline.toString("MM-dd-yyyy hh:mm AP");
            }
        }
        // Check if punishment has a ValueUnit of time
        else if (!punDef->valueUnit.isEmpty()) {
            double value = punDef->value;
            int total = qRound(value * amount);

            if (punDef->valueUnit.compare("minute", Qt::CaseInsensitive) == 0) {
                deadline = deadline.addSecs(total * 60);
            } else if (punDef->valueUnit.compare("hour", Qt::CaseInsensitive) == 0) {
                deadline = deadline.addSecs(total * 3600);
            } else if (punDef->valueUnit.compare("day", Qt::CaseInsensitive) == 0) {
                deadline = deadline.addDays(total);
            }

            qDebug() << "[DEBUG] Punishment deadline set from ValueUnit: " << deadline.toString("MM-dd-yyyy hh:mm AP")
                     << " amount:" << amount;
        }

        jobDeadlines[punishmentName] = deadline;

        emit jobListUpdated();
        qDebug() << "[DEBUG] jobListUpdated Signal Emitted!";
    } else {
        qDebug() << "[DEBUG] Punishment already exists in active assignments: " << punishmentName;

        QDateTime deadline = jobDeadlines.value(punishmentName, internalClock);
        bool extended = false;

        // Check if punishment has a ValueUnit of time to extend the deadline
        if (!punDef->valueUnit.isEmpty()) {
            double value = punDef->value;
            int total = qRound(value * amount);

            if (punDef->valueUnit.compare("minute", Qt::CaseInsensitive) == 0) {
                deadline = deadline.addSecs(total * 60);
                if (jobExpiryTimes.contains(punishmentName))
                    jobExpiryTimes[punishmentName] = jobExpiryTimes[punishmentName].addSecs(total * 60);
            } else if (punDef->valueUnit.compare("hour", Qt::CaseInsensitive) == 0) {
                deadline = deadline.addSecs(total * 3600);
                if (jobExpiryTimes.contains(punishmentName))
                    jobExpiryTimes[punishmentName] = jobExpiryTimes[punishmentName].addSecs(total * 3600);
            } else if (punDef->valueUnit.compare("day", Qt::CaseInsensitive) == 0) {
                deadline = deadline.addDays(total);
                if (jobExpiryTimes.contains(punishmentName))
                    jobExpiryTimes[punishmentName] = jobExpiryTimes[punishmentName].addDays(total);
            }
            extended = true;
        }

        if (extended) {
            jobDeadlines[punishmentName] = deadline;
            emit jobListUpdated();
            qDebug() << "[DEBUG] jobListUpdated Signal Emitted!";
        }
    }
}

void CyberDom::applyPunishment(int severity, const QString &group)
{
    // Choose a punishment based on severity and group
    QString punishmentName = selectPunishmentFromGroup(severity, group);
    if (punishmentName.isEmpty())
        return;

    if (!scriptParser) return; // Safety check

    // Get the structed definition for the punishment
    const auto &punishments = scriptParser->getScriptData().punishments;
    QString lowerName = punishmentName.toLower();

    if (!punishments.contains(lowerName)) {
        qDebug() << "[ERROR] applyPunishment: Could not find definition for" << punishmentName;
        return;
    }
    const PunishmentDefinition &punDef = punishments.value(lowerName);

    // Get the raw data to check if "value" was set.
    QMap<QString, QStringList> rawData = scriptParser->getRawSectionData("punishment-" + punishmentName);
    bool hasValue = rawData.contains("value");

    // Fetch parameters from the struct
    double value = punDef.value;
    QString valueUnit = punDef.valueUnit.toLower();

    // Safety checks (same as your old code)
    if (!hasValue && valueUnit == "once")
        value = 1.0;
    if (value <= 0)
        value = 1.0;

    int min = punDef.min;
    int max = punDef.max;
    if (max <= 0)
        max = 20;

    // Calculate totalUnits
    int totalUnits;
    if (valueUnit == "once" && !hasValue) {
        // With ValueUnit=Once and no value specified, punish once
        totalUnits = qMax(min, 1);
    } else {
        // Otherwise, calculate based on severity
        double amt = static_cast<double>(severity) / value;
        totalUnits = qRound(amt);
        if (totalUnits < min)
            totalUnits = min;
    }

    // Add the assignments
    if (valueUnit == "once") {
        // max is ignored when ValueUnit=Once
        addPunishmentToAssignments(punishmentName, totalUnits);
    } else {
        while (totalUnits > 0) {
            int amount = qMin(max, totalUnits);
            addPunishmentToAssignments(punishmentName,amount);
            totalUnits -= amount;
        }
    }
}

QString CyberDom::selectPunishmentFromGroup(int severity, const QString &group)
{
    QList<QString> eligiblePunishments;

    // Safety check
    if (!scriptParser) {
        qDebug() << "[ERROR] selectPunishmentFromGroup: scriptParser is null";
        return QString();
    }

    // Iterate through the structured punishment definitions
    const auto &allPunishments = scriptParser->getScriptData().punishments;

    for (const PunishmentDefinition &punDef : allPunishments.values()) {

        // Check if punishment belongs to the specified group
        if (group.isEmpty() || punDef.groups.contains(group)) {

            // Check if punishment is suitable for the severity
            bool suitable = true;

            // punDef.minSeverity is already an int, no conversion needed
            if (severity < punDef.minSeverity) {
                suitable = false;
            }

            // punDef.maxSeverity is already an int, no conversion needed
            if (punDef.maxSeverity > 0 && severity > punDef.maxSeverity) {
                suitable = false;
            }

            if (suitable) {
                eligiblePunishments.append(punDef.name);
            }
        }
    }

    // If there are eligible punishments, choose one randomly
    if (!eligiblePunishments.isEmpty()) {
        int index;

#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
        // Use QRandomGenerator for Qt 5.10+
        index = QRandomGenerator::global()->bounded(eligiblePunishments.size());
#else
        static bool seeded = false;
        if (!seeded) {
            std::srand(static_cast<unsigned int>(std::time(nullptr)));
            seeded = true;
        }
        index = std::rand() % eligiblePunishments.size();
#endif

        return eligiblePunishments.at(index);
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
        updateStatus(newStatus);
    }

    QString startProcedure; // Store the procedure to run

    if (!scriptParser) {
        qDebug() << "[ERROR] startAssignment: scriptParser is null!";
    } else {
        const ScriptData &data = scriptParser->getScriptData();
        QString lowerName = assignmentName.toLower();

        if (isPunishment) {
            // It's a punishment
            if (data.punishments.contains(lowerName)) {
                startProcedure = data.punishments.value(lowerName).startProcedure;
            } else {
                qDebug() << "[WARNING] Section not found for punishment: " << assignmentName;
            }
        } else {
            // It's a job
            if (data.jobs.contains(lowerName)) {
                startProcedure = data.jobs.value(lowerName).startProcedure;
            } else {
                qDebug() << "[WARNING] Section not found for job: " << assignmentName;
            }
        }
    }

    // Set a flag to track this assignment as started
    QString startFlagName = (isPunishment ? "punishment_" : "job_") + assignmentName + "_started";
    setFlag(startFlagName);

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

    // 1. Get the assignment's definition
    if (isPunishment) {
        if (!data.punishments.contains(lowerName)) {
            qDebug() << "[WARNING] Section not found for completed assignment: punishment-" << assignmentName;
            return false;
        }
        const PunishmentDefinition &def = data.punishments.value(lowerName);

        // Get properties from the PunishmentDefinition & its DurationControl
        const DurationControl& dur = def.duration;
        minTime = dur.minTimeStart; // Assuming MinTime is minTimeStart
        quickPenalty1 = (dur.quickPenalty1 != 0) ? QString::number(dur.quickPenalty1) : "";
        quickPenalty2 = (dur.quickPenalty2 != 0.0) ? QString::number(dur.quickPenalty2) : "";
        quickPenaltyGroup = dur.quickPenaltyGroup;
        quickMessage = dur.quickMessage;
        quickProcedure = dur.quickProcedure;
        mustStart = def.mustStart;
        addMerit = (def.merits.add != 0) ? QString::number(def.merits.add) : "";
        doneProcedure = def.doneProcedure;

    } else { // It's a Job
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
        mustStart = def.mustStart;
        addMerit = (def.merits.add != 0) ? QString::number(def.merits.add) : "";
        doneProcedure = def.doneProcedure;
    }
    // --- END REFACTOR ---


    // 2. Check MinTime (This logic is now based on our new variables)
    // Note: Your original code had 'if (!isPunishment && ...)'
    // I am preserving this, even though punishments also have duration.
    if (!isPunishment && !minTime.isEmpty()) {
        // We removed the duplicate QSettings settings(...) line here
        QDateTime start = settings.value("Assignments/" + assignmentName + "_start_time").toDateTime();
        int elapsed = start.isValid() ? static_cast<int>(start.secsTo(internalClock)) : 0;
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

    // 5. Cleanup flags (This is correct)
    QString startFlagName = (isPunishment ? "punishment_" : "job_") + assignmentName + "_started";
    removeFlag(startFlagName);

    // 6. Return to previous status (This logic is correct, it already uses ScriptData)
    QString nextSubStatus;
    if (scriptParser) {
        // const ScriptData &data = scriptParser->getScriptData(); // Already defined
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
    } else {
        returntoLastStatus();
    }

    // 7. Add merits (This logic is now based on our new 'addMerit' variable)
    if (!addMerit.isEmpty()) {
        bool ok;
        int meritPoints = addMerit.toInt(&ok);
        if (ok && meritPoints != 0) {
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
    return true;
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

void CyberDom::deleteAssignment(const QString &assignmentName, bool isPunishment)
{
    QString sectionPrefix = isPunishment ? "punishment-" : "job-";
    QString sectionName = sectionPrefix + assignmentName;

    bool sectionFound = false;
    QMap<QString, QString> details;
    for (auto it = iniData.constBegin(); it != iniData.constEnd(); ++it) {
        if (it.key().toLower() == sectionName.toLower()) {
            sectionFound = true;
            details = it.value();
            break;
        }
    }

    if (!sectionFound) {
        qDebug() << "[WARNING] Section not found for deleted assignment: " << sectionName;
        return;
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
        updateStatus(prevStatus);
        settings.remove("Assignments/" + statusFlagName);
    }

    if (details.contains("DeleteProcedure")) {
        // Placeholder for procedure execution
        // runProcedure(details["DeleteProcedure"]);
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
                    applyPunishment(severity);
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
    if (!isTimeAllowed(rep.notBeforeTimes, rep.notAfterTimes, rep.notBetweenTimes))
        return;

    int merits = getMeritsFromIni();
    if (rep.merits.set != 0)
        merits = rep.merits.set;
    merits += rep.merits.add;
    merits -= rep.merits.subtract;
    merits = std::max(getMinMerits(), std::min(getMaxMerits(), merits));
    updateMerits(merits);

    auto showMessage = [this](const QString &m) {
        if (!m.trimmed().isEmpty())
            QMessageBox::information(this, tr("Report"), replaceVariables(m.trimmed()));
    };

    for (const QString &txt : rep.statusTexts)
        showMessage(txt);

    for (const MessageGroup &grp : rep.messages) {
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

    for (const QString &flag : rep.setFlags)
        setFlag(flag);
    for (const QString &flag : rep.removeFlags)
        removeFlag(flag);

    if (!rep.clothingInstruction.isEmpty())
        updateClothingInstructions(rep.clothingInstruction);
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
    if (!scriptParser)
        return false;

    const auto &data = scriptParser->getScriptData();
    QString key = name.toLower();
    if (isPunishment) {
        for (auto it = data.punishments.constBegin(); it != data.punishments.constEnd(); ++it) {
            if (it.key().toLower() == key) {
                bool lr = it.value().longRunning;
                if (it.value().valueUnit.compare("day", Qt::CaseInsensitive) == 0)
                    lr = true;
                return lr;
            }
        }
    } else {
        for (auto it = data.jobs.constBegin(); it != data.jobs.constEnd(); ++it) {
            if (it.key().toLower() == key)
                return it.value().longRunning;
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
        QString section = "punishment-" + name;
        if (!iniData.contains(section))
            continue;

        QString flag = "punishment_" + name + "_started";
        if (!isFlagSet(flag))
            continue;

        QString key = name.toLower();
        for (auto it = data.punishments.constBegin(); it != data.punishments.constEnd(); ++it) {
            if (it.key().toLower() == key) {
                bool lr = it.value().longRunning;
                if (it.value().valueUnit.compare("day", Qt::CaseInsensitive) == 0)
                    lr = true;
                if (!lr)
                    return true;
                break;
            }
        }
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
