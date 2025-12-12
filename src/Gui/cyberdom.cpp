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
#include "pointcameradialog.h"
#include "posecameradialog.h"

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
#include <QComboBox>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QStringConverter>
#include <QDesktopServices>
#include <QUrl>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QTimeEdit>
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

    // Capture the time the application started running
    sessionStartTime = QDateTime::currentDateTime();

    // Capture last response duration
    lastResponseDuration = 0;

    // Capture last flag duration
    lastFlagDuration = 0;

    // Initialize signin timer state to "not running"
    signinRemainingSecs = -1;
    clothFaultsCount = 0;
    lastPunishmentSeverity = 0;
    lastLatenessMinutes = 0;

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
    connect(ui->actionAsk_for_Clothing_Instructions, &QAction::triggered, this, [this]() {
        openAskClothingDialog();
    });

    // Connect the AskInstructions action to a slot function
    connect(ui->actionAsk_for_Other_Instructions, &QAction::triggered, this, [this]() {
        openAskInstructionsDialog();
    });

    // Connect the ReportClothing action to a slot function
    connect(ui->actionReport_Clothing, &QAction::triggered, this, [this]() {
        openReportClothingDialog();
    });

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

    // Connect the Make Report File action
    connect(ui->actionMake_A_New_Report_File, &QAction::triggered, this, &CyberDom::onMakeReportFile);
    connect(ui->actionView_Report_File, &QAction::triggered, this, &CyberDom::onViewReportFile);

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

    // Check if we missed a scheduled job run
    QDate today = internalClock.date();
    if (!lastScheduledJobsRun.isValid() || lastScheduledJobsRun < today) {
        qDebug() << "[Scheduler] Running missed daily job check on startup.";
        assignScheduledJobs();
        lastScheduledJobsRun = today; // Mark as run for today
    }

    applyDailyMerits(); // Apply daily merits on startup if needed

    // Load persistent question answers
    loadQuestionAnswers();

    QSettings appSettings(settingsFile, QSettings::IniFormat);
    bool isFirstRun = !appSettings.contains("User/Initialized");

    // Trigger openProgram event handler ONLY if NOT first run
    if (!isFirstSessionRun && scriptParser) {
        QString proc = scriptParser->getScriptData().eventHandlers.openProgram;
        if (!proc.isEmpty()) {
            qDebug() << "[Event] Running OpenProgram event.";
            runProcedure(proc);
        }
    } else {
        qDebug() << "[Event] Skipping OpenProgram event (First Run).";
    }

    // Initialize Camera
    setupCamera();
}

void CyberDom::setupCamera()
{
    // Find the default camera
    const QCameraDevice &defaultCamera = QMediaDevices::defaultVideoInput();
    if (defaultCamera.isNull()) {
        qDebug() << "[Camera] No camera device found on this system.";
        return;
    }

    qDebug() << "[Camera] Initializing camera:" << defaultCamera.description();

    // Initialize Components
    camera = new QCamera(defaultCamera, this);
    captureSession = new QMediaCaptureSession(this);
    imageCapture = new QImageCapture(this);

    // Connect Components
    captureSession->setCamera(camera);
    captureSession->setImageCapture(imageCapture);

    // Connect Signals for debugging/logic
    connect(camera, &QCamera::errorOccurred, this, &CyberDom::onCameraError);
    connect(imageCapture, &QImageCapture::imageSaved, this, &CyberDom::onImageSaved);
    connect(imageCapture, &QImageCapture::errorOccurred, this, &CyberDom::onImageCaptureError);

    // Start the Camera
    camera->start();
}

void CyberDom::takePicture(const QString &filenamePrefix)
{
    if (!imageCapture || !imageCapture->isReadyForCapture()) {
        qDebug() << "[Camera] Cannot take picture. Camera is not ready or not initialized.";
        return;
    }

    // Determine the folder path
    QString folder;
    if (scriptParser) {
        // Try to get folder from [Genera] CameraFolder=
        folder = scriptParser->getScriptData().general.cameraFolder;
    }

    // Fallback if script doesn't define it
    if (folder.isEmpty()) {
        folder = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + "/CyberDom";
    }

    QDir dir(folder);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // Generate Filename
    // Format: Prefix_YYYYMMDD_HHmmss.jpg
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString fileName = QString("%1/%2_%3.jpg").arg(folder, filenamePrefix, timestamp);

    // Capture
    imageCapture->captureToFile(fileName);
    qDebug() << "[Camera] Requesting capture to:" << fileName;
}

void CyberDom::onImageSaved(int id, const QString &fileName)
{
    Q_UNUSED(id);
    qDebug() << "[Camera] Image successfully saved to:" << fileName;
}

void CyberDom::onCameraError(QCamera::Error value, const QString &description)
{
    Q_UNUSED(value);
    qDebug() << "[Camera] Device Error:" << description;
}

void CyberDom::onImageCaptureError(int id, QImageCapture::Error error, const QString &errorString)
{
    Q_UNUSED(id);
    Q_UNUSED(error);
    qDebug() << "[Camera] Capture Error:" << errorString;
}

CyberDom::~CyberDom()
{
    // Log application closes
    qDebug() << "[SYSTEM] Application Closing (Destructor called).";

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

    // Mark as initialized on clean exit
    if (!settingsFile.isEmpty()) {
        QSettings settings(settingsFile, QSettings::IniFormat);
        settings.setValue("User/Initialized", true);
        settings.setValue("System/CurrentDate", internalClock.date().toString("MM-dd-yyyy"));
        settings.setValue("User/BeginTime", scriptBeginTime.toString(Qt::ISODate));
        settings.setValue("User/LastCloseTime", QDateTime::currentDateTime().toString(Qt::ISODate));
    }

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

QString CyberDom::getAskPunishmentGroups() const {
    return askPunishmentGroups;
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

    if (previousTime.date() != internalClock.date()) {

        // Generate Report for yesterday
        generateDailyReportFile(true);
        todayStats.reset();
    }

    QTime now = internalClock.time();
    QDate today = internalClock.date();

    // Run at 2:00:00 AM, but only if we haven't already run today
    if (now.hour() == 2 && now.minute() == 0 && now.second() == 0 && lastScheduledJobsRun != today) {
        qDebug() << "[Scheduler] Running daily job check at 2 AM.";
        assignScheduledJobs();
        lastScheduledJobsRun = today; // Mark as run for today
    }

    // --- Check Permission Notifications ---
    if (!permissionNotificationsPending.isEmpty()) {
        // Iterate over a copy so we can remove items safely
        QSet<QString> checkSet = permissionNotificationsPending;
        for (const QString &permName : checkSet) {
            if (permissionNextAvailable.contains(permName)) {
                if (internalClock >= permissionNextAvailable.value(permName)) {
                    // Time is up! Notify the user.

                    // Get title for nice display
                    QString title = permName;
                    if (scriptParser) {
                        const auto &perms = scriptParser->getScriptData().permissions;
                        if (perms.contains(permName)) {
                            QString t = perms.value(permName).title;
                            if (!t.isEmpty()) title = t;
                        }
                    }

                    // Show popup (only if interrupts allowed)
                    if (isInterruptAllowed()) {
                        QMessageBox::information(this, tr("Permission Available"),
                                                 tr("You may now ask for permission: %1").arg(title));

                        // Remove from pending list
                        permissionNotificationsPending.remove(permName);
                    }
                }
            } else {
                // Safety cleanup: if nextAvailable is gone, remove pending note
                permissionNotificationsPending.remove(permName);
            }
        }
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

                        case ScriptActionType::ClothReport: {
                            QString val = action.value.trimmed();
                            QString title = "";
                            // If value is not "1", it's a custom title
                            if (val != "1") {
                                title = replaceVariables(val);
                            }
                            // Open forced dialog
                            openReportClothingDialog(true, title);
                            break;
                        }
                        case ScriptActionType::ClearCloth:
                            clearCurrentClothing();
                            break;

                        case ScriptActionType::Clothing: {
                            QString text = resolveInstruction(action.value);
                            updateClothingInstructions(text);

                            openAskClothingDialog(action.value);
                            break;
                        }

                        case ScriptActionType::Instructions: {
                            QString text = resolveInstruction(action.value);
                            updateInstructions(text);

                            openAskInstructionsDialog(action.value);
                            break;
                        }

                        // String Variables
                        case ScriptActionType::SetString:
                        case ScriptActionType::InputString:
                        case ScriptActionType::ChangeString:
                        case ScriptActionType::InputLongString:
                        case ScriptActionType::ChangeLongString:
                        case ScriptActionType::DropString:
                            executeStringAction(action.type, action.value);
                            break;

                        // Counter Variables
                        case ScriptActionType::SetCounterVar:
                        case ScriptActionType::AddCounter:
                        case ScriptActionType::SubtractCounter:
                        case ScriptActionType::MultiplyCounter:
                        case ScriptActionType::DivideCounter:
                        case ScriptActionType::InputCounter:
                        case ScriptActionType::ChangeCounter:
                        case ScriptActionType::InputNegCounter:
                        case ScriptActionType::DropCounter:
                            executeCounterAction(action.type, action.value);
                            break;
                        case ScriptActionType::RandomCounter:
                            executeRandomCounterAction(action.value);
                            break;

                        case ScriptActionType::SetTimeVar:
                        case ScriptActionType::InputDate:
                        case ScriptActionType::InputDateDef:
                        case ScriptActionType::ChangeDate:
                        case ScriptActionType::InputTime:
                        case ScriptActionType::InputTimeDef:
                        case ScriptActionType::ChangeTime:
                        case ScriptActionType::InputInterval:
                        case ScriptActionType::ChangeInterval:
                        case ScriptActionType::AddDaysTime:
                        case ScriptActionType::SubtractDaysTime:
                        case ScriptActionType::RoundTime:
                        case ScriptActionType::RandomTime:
                        case ScriptActionType::DropTime:
                            executeTimeAction(action.type, action.value);
                            break;

                        case ScriptActionType::ExtractDays:
                        case ScriptActionType::ExtractHours:
                        case ScriptActionType::ExtractMinutes:
                        case ScriptActionType::ExtractSeconds:
                            executeTimeExtraction(action.type, action.value);
                            break;

                        case ScriptActionType::ConvertDays:
                        case ScriptActionType::ConvertHours:
                        case ScriptActionType::ConvertMinutes:
                        case ScriptActionType::ConvertSeconds:
                            executeCounterToTime(action.type, action.value);
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

void CyberDom::openAskClothingDialog(const QString &target)
{
    if (target.isEmpty() && checkInterruptableAssignments()) return;

    // 1. Resolve the text (if we have a target)
    QString preCalculatedText;
    if (!target.isEmpty()) {
        preCalculatedText = resolveInstruction(target);
        // (This logic ensures we don't re-roll random choices and get a different result)
    }

    // 2. Create dialog
    AskClothing askclothingDialog(this, scriptParser, target);

    // 3. Set the Text
    if (!preCalculatedText.isEmpty()) {
        askclothingDialog.setInstructionText(preCalculatedText);
    } else {
        // If no target (user menu), trigger the selection manually to show the first item's text
        if (askclothingDialog.findChild<QComboBox*>()) {
             // Simulate selection of the first item
             askclothingDialog.onInstructionSelected(0);
        }
    }

    askclothingDialog.exec();
}

void CyberDom::openAskInstructionsDialog(const QString &target)
{
    if (target.isEmpty() && checkInterruptableAssignments()) return;

    // Create dialog with the optional target
    AskInstructions askinstructionsDialog(this, scriptParser, target);

    askinstructionsDialog.exec();
}

void CyberDom::openReportClothingDialog(bool forced, const QString &title)
{
    if (!forced && checkInterruptableAssignments()) return;

    // Pass the new parameters to the dialog
    ReportClothing *reportClothingDialog = new ReportClothing(this, scriptParser, forced, title);

    if (reportClothingDialog->exec() == QDialog::Accepted) {
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

    lastPermissionName = name;

    if (!scriptParser) return;

    const auto &perms = scriptParser->getScriptData().permissions;
    if (!perms.contains(name)) {
        qWarning() << "[CyberDom] Permission definition not found:" << name;
        return;
    }

    const PermissionDefinition &perm = perms.value(name);

    // --- 0. Pick a random name ---
    QString subName = scriptParser->getSubName();
    if (subName.isEmpty()) subName = "Sub";

    // --- 1. Check PreStatus ---
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

    // --- 2. Check Time (Lockout) ---
    // Check if currently locked due to MinInterval or Delay
    if (permissionNextAvailable.contains(name)) {
        QDateTime unlockTime = permissionNextAvailable.value(name);
        if (internalClock < unlockTime) {
            // Denied due to timer lock
            int delaySecs = calculateSecondsFromTimeRange(perm.delay);
            if (delaySecs > 0) {
                QDateTime newUnlock = internalClock.addSecs(delaySecs);

                if (!permissionFirstDenied.contains(name)) {
                    permissionFirstDenied[name] = internalClock;
                }

                int maxWaitSecs = calculateSecondsFromTimeRange(perm.maxWait);
                if (maxWaitSecs > 0) {
                    QDateTime absoluteMax = permissionFirstDenied[name].addSecs(maxWaitSecs);
                    if (newUnlock > absoluteMax) newUnlock = absoluteMax;
                }

                if (newUnlock > unlockTime) {
                    permissionNextAvailable[name] = newUnlock;
                }
            }

            QMessageBox::information(this, tr("Permission Denied"),
                                     tr("You must wait longer before asking for this permission."));
            return;
        }
    }

    // Check Global Time Restrictions
    if (!isTimeAllowed(perm.notBeforeTimes, perm.notAfterTimes, perm.notBetweenTimes)) {
        QMessageBox::information(this, tr("Permission"),
                                 tr("This permission is not available at this time."));
        return;
    }

    incrementUsageCount(QString("Permission/%1").arg(name));

    // --- 3. Check Merit Limits ---
    int currentMerits = getMeritsFromIni();
    if ((perm.denyBelowMin != -1 && currentMerits < perm.denyBelowMin) ||
        (perm.denyAboveMin != -1 && currentMerits > perm.denyAboveMin)) {

        QMessageBox::information(this, tr("Permission Denied"),
            tr("You do not have the required merit points for this permission."));
        return;
    }

    // --- 4. Check Forbidden (Punishments) ---
    if (isPermissionForbidden(name)) {
        QString title = perm.title.isEmpty() ? perm.name : perm.title;
        QMessageBox::information(this, tr("Permission"),
                                 tr("No %1 you may not %2").arg(subName, title));

        if (!perm.denyProcedure.isEmpty()) runProcedure(perm.denyProcedure);
        QString eventProc = scriptParser->getScriptData().eventHandlers.permissionDenied;
        if (!eventProc.isEmpty()) runProcedure(eventProc);

        // Trigger Delay
        int delaySecs = calculateSecondsFromTimeRange(perm.delay);
        if (delaySecs > 0) {
            permissionNextAvailable[name] = internalClock.addSecs(delaySecs);
            if (!permissionFirstDenied.contains(name)) permissionFirstDenied[name] = internalClock;
        }
        return;
    }

    // --- 5. Run BeforeProcedure ---
    if (!perm.beforeProcedure.isEmpty())
        runProcedure(perm.beforeProcedure);

    // --- 6. CALCULATE OUTCOME (Automatic) ---
    bool granted = false;
    int chance = perm.pct;

    // Calculate variable chance based on merits
    if (perm.pctIsVariable || (perm.highMeritsMin != -1 && perm.lowMeritsMin != -1)) {
        int highM = (perm.highMeritsMin != -1) ? perm.highMeritsMin : 1000;
        int lowM = (perm.lowMeritsMin != -1) ? perm.lowMeritsMin : 0;
        int highP = (perm.highPctMin != -1) ? perm.highPctMin : 100;
        int lowP = (perm.lowPctMin != -1) ? perm.lowPctMin : 0;

        if (currentMerits >= highM) chance = highP;
        else if (currentMerits <= lowM) chance = lowP;
        else {
            double ratio = (double)(currentMerits - lowM) / (highM - lowM);
            chance = lowP + (int)(ratio * (highP - lowP));
        }
    }

    // Default to 100% if no Pct logic provided (standard behavior)
    if (chance == 0 && !perm.pctIsVariable && perm.highMeritsMin == -1) {
        chance = 100;
    }

    // Roll the dice
    int roll = ScriptUtils::randomInRange(1, 100, perm.centerRandom);
    granted = (roll <= chance);

    qDebug() << "[Permission]" << name << "Chance:" << chance << "Roll:" << roll << "Granted:" << granted;


    // --- 7. Execute Result ---
    if (granted) {
        // --- GRANTED ---

        trackPermissionEvent(name, "Granted");

        // Apply MinInterval lock (can't ask again for X time)
        int intervalSecs = calculateSecondsFromTimeRange(perm.minInterval);
        if (intervalSecs > 0) {
            permissionNextAvailable[name] = internalClock.addSecs(intervalSecs);
        }
        permissionFirstDenied.remove(name); // Reset denial tracker

        // Show Message
        QString msg = perm.permitMessage;
        if (msg.isEmpty()) msg = tr("Yes %1, you may %2.").arg(subName, perm.title.isEmpty() ? name : perm.title);
        QMessageBox::information(this, tr("Permission"), replaceVariables(msg, name));

        // Global Event
        QString eventProc = scriptParser->getScriptData().eventHandlers.permissionGiven;
        if (!eventProc.isEmpty()) runProcedure(eventProc);

        // Merits
        auto getMeritValue = [&](const QString &s) -> int {
            if (s.isEmpty()) return 0;
            if (s.startsWith("#")) return scriptParser->getVariable(s.mid(1)).toInt();
            return s.toInt();
        };

        if (!perm.merits.set.isEmpty()) {
            currentMerits = getMeritValue(perm.merits.set);
        } else {
            currentMerits += getMeritValue(perm.merits.add);
            currentMerits -= getMeritValue(perm.merits.subtract);
        }
        currentMerits = std::max(getMinMerits(), std::min(getMaxMerits(), currentMerits));
        updateMerits(currentMerits);

        // Execute Actions
        bool skipNextAction = false;
        for (const ScriptAction &action : perm.actions) {
            if (skipNextAction) {
                skipNextAction = false;
                if (action.type != ScriptActionType::If && action.type != ScriptActionType::NotIf) continue;
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
            case ScriptActionType::SetCounterVar:
            case ScriptActionType::AddCounter:
            case ScriptActionType::SubtractCounter:
            case ScriptActionType::MultiplyCounter:
            case ScriptActionType::DivideCounter:
            case ScriptActionType::InputCounter:
            case ScriptActionType::ChangeCounter:
            case ScriptActionType::InputNegCounter:
            case ScriptActionType::DropCounter:
                executeCounterAction(action.type, action.value);
                break;
            case ScriptActionType::RandomCounter:
                executeRandomCounterAction(action.value);
                break;

            case ScriptActionType::SetTimeVar:
            case ScriptActionType::InputDate:
            case ScriptActionType::InputDateDef:
            case ScriptActionType::ChangeDate:
            case ScriptActionType::InputTime:
            case ScriptActionType::InputTimeDef:
            case ScriptActionType::ChangeTime:
            case ScriptActionType::InputInterval:
            case ScriptActionType::ChangeInterval:
            case ScriptActionType::AddDaysTime:
            case ScriptActionType::SubtractDaysTime:
            case ScriptActionType::RoundTime:
            case ScriptActionType::RandomTime:
            case ScriptActionType::DropTime:
                executeTimeAction(action.type, action.value);
                break;

            case ScriptActionType::ExtractDays:
            case ScriptActionType::ExtractHours:
            case ScriptActionType::ExtractMinutes:
            case ScriptActionType::ExtractSeconds:
                executeTimeExtraction(action.type, action.value);
                break;

            case ScriptActionType::ConvertDays:
            case ScriptActionType::ConvertHours:
            case ScriptActionType::ConvertMinutes:
            case ScriptActionType::ConvertSeconds:
                executeCounterToTime(action.type, action.value);
                break;

            case ScriptActionType::Message:
                QMessageBox::information(this, tr("Permission"), replaceVariables(action.value));
                break;
            case ScriptActionType::NewStatus: changeStatus(action.value, false); break;
            case ScriptActionType::NewSubStatus: changeStatus(action.value, true); break;
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
            case ScriptActionType::Clothing: {
                QString text = resolveInstruction(action.value);
                updateClothingInstructions(text);

                openAskClothingDialog(action.value);
                break;
            }
            case ScriptActionType::Instructions: {
                 QString text = resolveInstruction(action.value);
                 updateInstructions(text);

                 openAskInstructionsDialog(action.value);
                 break;
            }
            case ScriptActionType::SetString:
            case ScriptActionType::InputString:
            case ScriptActionType::ChangeString:
            case ScriptActionType::InputLongString:
            case ScriptActionType::ChangeLongString:
            case ScriptActionType::DropString:
                executeStringAction(action.type, action.value);
                break;

            default:
                break;
            }
        }

    } else {
        // --- DENIED ---

        trackPermissionEvent(name, "Denied");

        // Apply Delay (Lockout)
        int delaySecs = calculateSecondsFromTimeRange(perm.delay);
        if (delaySecs > 0) {
            QDateTime newUnlock = internalClock.addSecs(delaySecs);

            // Initialize FirstDenied if not set
            if (!permissionFirstDenied.contains(name)) {
                permissionFirstDenied[name] = internalClock;
            }

            // Apply MaxWait cap
            int maxWaitSecs = calculateSecondsFromTimeRange(perm.maxWait);
            if (maxWaitSecs > 0) {
                QDateTime absoluteMax = permissionFirstDenied[name].addSecs(maxWaitSecs);
                if (newUnlock > absoluteMax) {
                    newUnlock = absoluteMax;
                }
            }

            permissionNextAvailable[name] = newUnlock;

            // --- NEW: Handle Notify Logic ---
            if (perm.notify > 0) {
                permissionNotificationsPending.insert(name);
            }
            // --- END NEW ---
        }

        // Build Message
        QString msg = perm.denyMessage;
        if (msg.isEmpty()) msg = tr("No %1, you may not %2.").arg(subName, perm.title.isEmpty() ? name : perm.title);
        msg = replaceVariables(msg, name);

        // --- NEW: Append Notify Info ---
        if (delaySecs > 0) {
            if (perm.notify == 1) {
                // Notify=1: Tell them how long
                msg += "\n\n" + tr("You must wait %1. You will be notified when you can ask again.").arg(formatDuration(delaySecs));
            } else if (perm.notify == 2) {
                // Notify=2: Tell them they will be notified (but not how long)
                msg += "\n\n" + tr("You will be notified when you can ask again.");
            } else {
                // Notify=0 (Default): Just tell them to wait (or say nothing extra if you prefer)
                // Standard behavior is usually to give a hint unless hidden
                msg += "\n\n" + tr("You must wait %1.").arg(formatDuration(delaySecs));
            }
        }
        // --- END NEW ---

        QMessageBox::information(this, tr("Permission Denied"), msg);

        if (!perm.denyProcedure.isEmpty())
            runProcedure(perm.denyProcedure);
        QString eventProc = scriptParser->getScriptData().eventHandlers.permissionDenied;
        if (!eventProc.isEmpty())
            runProcedure(eventProc);
    }
}

bool CyberDom::isPermissionForbidden(const QString &name) const {
    if (!scriptParser) return false;

    // Iterate active assignments (handles _2 suffixes correctly via helper)
    for (const QString &assignmentName : activeAssignments) {

        // Get the definition using the helper
        const PunishmentDefinition* punDef = getPunishmentDefinition(assignmentName);
        if (!punDef) continue;

        // Check if the punishment is started
        // Only started punishments should enforce restrictions
        QString startFlag = "punishment_" + assignmentName + "_started";
        if (!isFlagSet(startFlag)) continue;

        // Check the forbids list
        for (const QString &forbid : punDef->forbids) {
            if (forbid.trimmed().compare(name, Qt::CaseInsensitive) == 0) {
                qDebug() << "[Permission] Denied" << name << "because of active punishment:" << assignmentName;
                return true;
            }
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

    todayStats.confessionsMade.append(name);

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
        case ScriptActionType::SetCounterVar:
        case ScriptActionType::AddCounter:
        case ScriptActionType::SubtractCounter:
        case ScriptActionType::MultiplyCounter:
        case ScriptActionType::DivideCounter:
        case ScriptActionType::InputCounter:
        case ScriptActionType::ChangeCounter:
        case ScriptActionType::InputNegCounter:
        case ScriptActionType::DropCounter:
            executeCounterAction(action.type, action.value);
            break;
        case ScriptActionType::RandomCounter:
            executeRandomCounterAction(action.value);
            break;

        case ScriptActionType::SetTimeVar:
        case ScriptActionType::InputDate:
        case ScriptActionType::InputDateDef:
        case ScriptActionType::ChangeDate:
        case ScriptActionType::InputTime:
        case ScriptActionType::InputTimeDef:
        case ScriptActionType::ChangeTime:
        case ScriptActionType::InputInterval:
        case ScriptActionType::ChangeInterval:
        case ScriptActionType::AddDaysTime:
        case ScriptActionType::SubtractDaysTime:
        case ScriptActionType::RoundTime:
        case ScriptActionType::RandomTime:
        case ScriptActionType::DropTime:
            executeTimeAction(action.type, action.value);
            break;

        case ScriptActionType::ExtractDays:
        case ScriptActionType::ExtractHours:
        case ScriptActionType::ExtractMinutes:
        case ScriptActionType::ExtractSeconds:
            executeTimeExtraction(action.type, action.value);
            break;

        case ScriptActionType::ConvertDays:
        case ScriptActionType::ConvertHours:
        case ScriptActionType::ConvertMinutes:
        case ScriptActionType::ConvertSeconds:
            executeCounterToTime(action.type, action.value);
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
        case ScriptActionType::Clothing: {
            QString text = resolveInstruction(action.value);
            updateClothingInstructions(text);

            openAskClothingDialog(action.value);
            break;
        }
        case ScriptActionType::Instructions: {
            QString text = resolveInstruction(action.value);
            updateInstructions(text);

            openAskInstructionsDialog(action.value);
            break;
        }

        case ScriptActionType::Punish:
            executePunish(action.value, conf.punishMessage, conf.punishGroup);
            break;

        case ScriptActionType::ClothReport: {
            QString val = action.value.trimmed();
            QString title = "";
            // If value is not "1", it's a custom title
            if (val != "1") {
                title = replaceVariables(val);
            }
            // Open forced dialog
            openReportClothingDialog(true, title);
            break;
        }

        case ScriptActionType::ClearCloth:
            clearCurrentClothing();
            break;

        case ScriptActionType::SetString:
        case ScriptActionType::InputString:
        case ScriptActionType::ChangeString:
        case ScriptActionType::InputLongString:
        case ScriptActionType::ChangeLongString:
        case ScriptActionType::DropString:
            executeStringAction(action.type, action.value);
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

    // If feature is not enabled in script (max=0), show error or return
    if (maxPunishment <= 0) {
        QMessageBox::information(this, "Not Available", "You cannot ask for a punishment right now.");
        return;
    }

    // Show the simple confirmation dialog
    AskPunishment askPunishmentDialog(this, minPunishment, maxPunishment);

    if (askPunishmentDialog.exec() == QDialog::Accepted) {

        // Calculate a RANDOM severity between min and max
        int severity = 0;
        if (minPunishment == maxPunishment) {
            severity = minPunishment;
        } else {
            severity = ScriptUtils::randomInRange(minPunishment, maxPunishment, false);
        }

        // Get the target groups (if any)
        QString groups = getAskPunishmentGroups();

        // Trigger the "PunishmentAsked" event handler
        if (scriptParser) {
            QString eventProc = scriptParser->getScriptData().eventHandlers.punishmentAsked;
            if (!eventProc.isEmpty()) {
                runProcedure(eventProc);
            }
        }

        // Apply the punishment
        qDebug() << "[AskPunishment] User requested punishment. Calculated Severity:" << severity << "Groups:" << groups;
        applyPunishment(severity, groups, QString());
    }
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

    if (!scriptParser) {
        QMessageBox::critical(this, "Error", "Script parser is not loaded.");
        return;
    }

    SelectPunishment selectPunishmentDialog(this, scriptParser);
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

    // Pass scriptParser to the dialog
    SetFlags setFlagsDialog(this, scriptParser);

    connect (&setFlagsDialog, &SetFlags::flagSetRequested, this, &CyberDom::setFlag);
    setFlagsDialog.exec();
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

    // Check Camera Status
    QString camText = status.pointCameraText.isEmpty() ? status.pointCameraText : status.pointCameraText;

    if (!camText.isEmpty()) {
        triggerPointCamera(camText, status.cameraIntervalMin, status.cameraIntervalMax, "Status_" + currentStatus);
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

    // Reset context variables
    lastFlagDuration = 0;
    lastFlagExpiryTime = QDateTime();

    // Get flag text and metadata from parsed script data
    if (scriptParser) {
        QString lowerFlagName = flagName.toLower();
        const auto &flagDefs = scriptParser->getScriptData().flagDefinitions;

        if (flagDefs.contains(lowerFlagName)) {
            const FlagDefinition &flagDef = flagDefs.value(lowerFlagName);

            // --- FIX: Calculate Duration Correctly ---
            // Construct the range string from min/max
            QString durationRange = flagDef.durationMin;
            if (!flagDef.durationMax.isEmpty() && flagDef.durationMax != flagDef.durationMin) {
                durationRange += "," + flagDef.durationMax;
            }

            if (!durationRange.isEmpty()) {
                lastFlagDuration = parseTimeRangeToSeconds(durationRange);
            }
            // -----------------------------------------

            // Set Text
            if (!flagDef.textLines.isEmpty()) {
                flagData.text = flagDef.textLines.join("\n");
            }

            // Set expiry based on inputs
            if (durationMinutes > 0) {
                // Manual override
                lastFlagDuration = durationMinutes * 60;
                flagData.expiryTime = internalClock.addSecs(lastFlagDuration);
            }
            else if (lastFlagDuration > 0) {
                // Script definition
                flagData.expiryTime = internalClock.addSecs(lastFlagDuration);
            }

            // Capture expiry for !zzExpireTime
            lastFlagExpiryTime = flagData.expiryTime;

            // Handle groups
            if (!flagDef.group.isEmpty()) {
                const QStringList rawGroups = flagDef.group.split(',', Qt::SkipEmptyParts);
                flagData.groups.clear();
                for (const QString &grp : rawGroups)
                    flagData.groups.append(grp.trimmed());
            }

            // Get SetProcedure name
            setProcedure = flagDef.setProcedure;
        }
        else if (durationMinutes > 0) {
            // No definition, but manual duration
            lastFlagDuration = durationMinutes * 60;
            flagData.expiryTime = internalClock.addSecs(lastFlagDuration);
            lastFlagExpiryTime = flagData.expiryTime;
        }
    }

    // Store the flag
    flags[flagName] = flagData;

    // Run SetProcedure if specified
    if (!setProcedure.isEmpty())
        runProcedure(setProcedure);

    // Update UI
    updateStatusText();
    qDebug() << "Flag set:" << flagName << "Duration:" << lastFlagDuration << "s";
}

void CyberDom::removeFlag(const QString &flagName) {
    QString targetKey = flagName;

    // Try to find the flag (Case-Insensitive Search)
    if (!flags.contains(targetKey)) {
        QString lowerName = flagName.toLower();
        for (auto it = flags.keyBegin(); it != flags.keyEnd(); ++it) {
            if (it->toLower() == lowerName) {
                targetKey = *it;
                break;
            }
        }
    }

    // Check if we found it
    if (flags.contains(targetKey)) {

        // Capture Expiry for !zzExpireTime
        lastFlagExpiryTime = flags[targetKey].expiryTime;
        lastFlagDuration = 0;

        if (scriptParser) {
            QString lowerFlagName = targetKey.toLower();
            const auto &flagDefs = scriptParser->getScriptData().flagDefinitions;

            if (flagDefs.contains(lowerFlagName)) {
                const FlagDefinition &flagDef = flagDefs.value(lowerFlagName);

                // --- FIX: Calculate Duration Correctly ---
                QString durationRange = flagDef.durationMin;
                if (!flagDef.durationMax.isEmpty() && flagDef.durationMax != flagDef.durationMin) {
                    durationRange += "," + flagDef.durationMax;
                }

                if (!durationRange.isEmpty()) {
                    lastFlagDuration = parseTimeRangeToSeconds(durationRange);
                }
                // -----------------------------------------

                if (!flagDef.removeProcedure.isEmpty()) {
                    runProcedure(flagDef.removeProcedure);
                }
            }
        }

        // Remove the flag
        flags.remove(targetKey);
        updateStatusText();
        qDebug() << "Flag removed:" << targetKey << "(requested:" << flagName << ")";
    } else {
        qDebug() << "[Flag] Warning: Attempted to remove non-existent flag:" << flagName;
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
        qDebug() << "[SYSTEM] RESET TRIGGERED. Deleting settings and restarting application.";

        // Notify the user
        QMessageBox::information(this, "Application Successfully Reset", 
                                "The application will now restart and prompt you to select a new script file.");

        // Restart the application
        QCoreApplication::quit();
        QProcess::startDetached(QCoreApplication::applicationFilePath(), QStringList());
    }
}

QString CyberDom::replaceVariables(const QString &input,
                                   const QString &contextName,
                                   const QString &contextTitle,
                                   int contextSeverity,
                                   const QString &contextMinTime,
                                   const QString &contextMaxTime,
                                   const QDateTime &contextStartTime,
                                   const QDateTime &contextCreationTime,
                                   const QDateTime &contextDeadline,
                                   const QDateTime &contextNextRemind,
                                   const QDateTime &contextNextAllow) const {
    QString result = input;

    // Auto-Detect Creation Time
    QDateTime creationTime = contextCreationTime;
    if (!creationTime.isValid() && !contextName.isEmpty()) {
        QSettings settings(settingsFile, QSettings::IniFormat);
        QVariant val = settings.value("Assignments/" + contextName + "_creation_time");
        if (val.isValid()) {
            creationTime = val.toDateTime();
        }
    }

    // Auto-Detect Deadline
    QDateTime deadline = contextDeadline;
    if (!deadline.isValid() && !contextName.isEmpty()) {
        // Lookup the deadline in the active deadlines map
        if (jobDeadlines.contains(contextName)) {
            deadline = jobDeadlines.value(contextName);
        }
    }

    // Auto-Detect Next Remind Time
    QDateTime nextRemind = contextNextRemind;
    if (!nextRemind.isValid() && !contextName.isEmpty()) {
        // Lookup in the active reminders map
        if (jobNextReminds.contains(contextName)) {
            nextRemind = jobNextReminds.value(contextName);
        }
    }

    // Auto-Detect Next Allow Time
    QDateTime nextAllow = contextNextAllow;
    if (!nextAllow.isValid() && !contextName.isEmpty()) {
        // Lookup permission lock
        if (permissionNextAvailable.contains(contextName)) {
            nextAllow = permissionNextAvailable.value(contextName);
        }
    }

    if (!scriptParser) return result;
    const ScriptData &data = scriptParser->getScriptData();

    // --- 1. Predefined Variables (via ScriptUtils) ---

    // Handle $zzName
    if (result.contains("zzName", Qt::CaseInsensitive)) {
        // Pass the contextName to the resolver
        QString val = ScriptUtils::resolvePredefinedString("zzName", data, internalClock, lastReportFilename, APP_VERSION, contextName, contextTitle, lastReportName, lastPermissionName, lastQuestionAnswer);
        QRegularExpression nameRx("\\[!$]zzName\\}|\\$zzName", QRegularExpression::CaseInsensitiveOption);
        result.replace(nameRx, val);
    }

    // Handle $zzTitle
    if (result.contains("zzTitle", Qt::CaseInsensitive)) {
        QString val = ScriptUtils::resolvePredefinedString("zzTitle", data, internalClock, lastReportFilename, APP_VERSION, contextName, contextTitle, lastReportName, lastPermissionName, lastQuestionAnswer);
        QRegularExpression titleRx("\\{[!$]zzTitle\\}|$zzTitle", QRegularExpression::CaseInsensitiveOption);
        result.replace(titleRx, val);
    }

    // Handle $zzSubName (Randomized)
    if (result.contains("zzSubName", Qt::CaseInsensitive)) {
        QString val = ScriptUtils::resolvePredefinedString("zzSubName", data, internalClock, lastReportFilename, APP_VERSION, contextName, contextTitle, lastReportName, lastPermissionName, lastQuestionAnswer);
        // Replace {$zzSubName}, {!zzSubName}, and $zzSubName
        QRegularExpression subRx("\\{[!$]zzSubName\\}|\\$zzSubName", QRegularExpression::CaseInsensitiveOption);
        result.replace(subRx, val);
    }

    // Handle $zzMaster
    if (result.contains("zzMaster", Qt::CaseInsensitive)) {
        QString val = ScriptUtils::resolvePredefinedString("zzMaster", data, internalClock, lastReportFilename, APP_VERSION, contextName, contextTitle, lastReportName, lastPermissionName, lastQuestionAnswer);
        QRegularExpression masterRx("\\{[!$]zzMaster\\}|\\$zzMaster", QRegularExpression::CaseInsensitiveOption);
        result.replace(masterRx, masterVariable);
    }

    // Handle $zzDate
    if (result.contains("zzDate", Qt::CaseInsensitive)) {
        QString val = ScriptUtils::resolvePredefinedString("zzDate", data, internalClock, lastReportFilename, APP_VERSION, contextName, contextTitle, lastReportName, lastPermissionName, lastQuestionAnswer);
        QRegularExpression dateRx("\\{[!$]zzDate\\}|\\$zzDate", QRegularExpression::CaseInsensitiveOption);
        result.replace(dateRx, val);
    }

    // Handle $zzTime
    if (result.contains("zzTime", Qt::CaseInsensitive)) {
        QString val = ScriptUtils::resolvePredefinedString("zzTime", data, internalClock, lastReportFilename, APP_VERSION, contextName, contextTitle, lastReportName, lastPermissionName, lastQuestionAnswer);
        QRegularExpression timeRx("\\{[!$]zzTime\\}|\\$zzTime", QRegularExpression::CaseInsensitiveOption);
        result.replace(timeRx, val);
    }

    // Handle $zzDayOfWeek
    if (result.contains("zzDayOfWeek", Qt::CaseInsensitive)) {
        QString val = ScriptUtils::resolvePredefinedString("zzDayOfWeek", data, internalClock, lastReportFilename, APP_VERSION, contextName, contextTitle, lastReportName, lastPermissionName, lastQuestionAnswer);
        QRegularExpression dayRx("\\{[!$]zzDayOfWeek\\}|\\$zzDayOfWeek", QRegularExpression::CaseInsensitiveOption);
        result.replace(dayRx, val);
    }

    // Handle $zzMonth
    if (result.contains("zzMonth", Qt::CaseInsensitive)) {
        QString val = ScriptUtils::resolvePredefinedString("zzMonth", data, internalClock, lastReportFilename, APP_VERSION, contextName, contextTitle, lastReportName, lastPermissionName, lastQuestionAnswer);
        QRegularExpression monthRx("\\{[!$]zzMonth\\}|\\$zzMonth", QRegularExpression::CaseInsensitiveOption);
        result.replace(monthRx, val);
    }

    // Handle $zzReportFile
    if (result.contains("zzReportFile", Qt::CaseInsensitive)) {
        QString val = ScriptUtils::resolvePredefinedString("zzReportFile", data, internalClock, lastReportFilename, APP_VERSION, contextName, contextTitle, lastReportName, lastPermissionName, lastQuestionAnswer);
        QRegularExpression reportRx("\\{[!$]zzReportFile\\}|\\$zzReportFile", QRegularExpression::CaseInsensitiveOption);
        result.replace(reportRx, val);
    }

    // Handle $zzPVersion
    if (result.contains("zzPVersion", Qt::CaseInsensitive)) {
        QString val = ScriptUtils::resolvePredefinedString("zzPVersion", data, internalClock, lastReportFilename, APP_VERSION, contextName, contextTitle, lastReportName, lastPermissionName, lastQuestionAnswer);
        QRegularExpression verRx("\\{[!$]zzPVersion\\}|\\$zzPVersion", QRegularExpression::CaseInsensitiveOption);
        result.replace(verRx, val);
    }

    // Handle $zzSVersion
    if (result.contains("zzSVersion", Qt::CaseInsensitive)) {
        QString val = ScriptUtils::resolvePredefinedString("zzSVersion", data, internalClock, lastReportFilename, APP_VERSION, contextName, contextTitle, lastReportName, lastPermissionName, lastQuestionAnswer);
        QRegularExpression scriptVerRx("\\{[!$]zzSVersion\\}|\\$zzSVersion", QRegularExpression::CaseInsensitiveOption);
        result.replace(scriptVerRx, val);
    }

    // Handle $zzReport
    if (result.contains("zzReport", Qt::CaseInsensitive)) {
        QString val = ScriptUtils::resolvePredefinedString("zzReport", data, internalClock, lastReportFilename, APP_VERSION, contextName, contextTitle, lastReportName, lastPermissionName, lastQuestionAnswer);

        // Use \\b to prevent matching "zzReportFile"
        QRegularExpression nameRx("\\{[!$]zzReport\\}|\\$zzReport\\b", QRegularExpression::CaseInsensitiveOption);
        result.replace(nameRx, val);
    }

    // Handle $zzPermission
    if (result.contains("zzPermission", Qt::CaseInsensitive)) {
        QString val = ScriptUtils::resolvePredefinedString("zzPermission", data, internalClock, lastReportFilename, APP_VERSION, contextName, contextTitle, lastReportName, lastPermissionName, lastQuestionAnswer);

        // Use \\b to prevent partial matching
        QRegularExpression permRx("\\{[!$]zzPermission\\}|\\$zzPermission\\b", QRegularExpression::CaseInsensitiveOption);
        result.replace(permRx, val);
    }

    // Handle $zzStatus
    if (result.contains("zzStatus", Qt::CaseInsensitive)) {
        QString val = ScriptUtils::resolvePredefinedString("zzPermission", data, internalClock, lastReportFilename, APP_VERSION, contextName, contextTitle, lastReportName, lastPermissionName, currentStatus, lastQuestionAnswer);

        QRegularExpression statusRx("\\{[!$]zzStatus\\}|\\$zzStatus\\b", QRegularExpression::CaseInsensitiveOption);
        result.replace(statusRx, val);
    }

    // Handle $zzAnswer
    if (result.contains("zzAnswer", Qt::CaseInsensitive)) {
        QString val = ScriptUtils::resolvePredefinedString("zzAnswer", data, internalClock, lastReportFilename, APP_VERSION, contextName, contextTitle, lastReportName, lastPermissionName, currentStatus, lastQuestionAnswer);

        QRegularExpression ansRx("\\{[!$]zzAnswer\\}|\\$zzAnswer", QRegularExpression::CaseInsensitiveOption);
        result.replace(ansRx, val);
    }

    // --- Replace {#variable} ---
    QRegularExpression hashVarRx("\\{#([^}]+)\\}");
    QRegularExpressionMatchIterator hashIter = hashVarRx.globalMatch(result);
    QString tempResult;
    int lastIndex = 0;

    while (hashIter.hasNext()) {
        QRegularExpressionMatch match = hashIter.next();
        tempResult += result.mid(lastIndex, match.capturedStart() - lastIndex);

        QString varName = match.captured(1);

        // Pass contextSeverity to getVariableValue
        QString value = getVariableValue(varName, contextSeverity);

        tempResult += value;
        lastIndex = match.capturedEnd();
    }
    tempResult += result.mid(lastIndex);
    result = tempResult;

    // --- Question Answers ---
    // Replace {$QuestionKey} with the stored answer
    if (result.contains("{$")) {
        for (auto it = questionAnswers.begin(); it != questionAnswers.end(); ++it) {
            QString var = "{$" + it.key() + "}";
            result.replace(var, it.value());
        }
    }

    // --- Stored Variables ({$Str}, {#Num}, {!Time}) ---
    // Catches any variable inside braces starting with $, #, or !
    // Example: {#counter}, {$stringVar}
    QRegularExpression varRx("\\{([$#!])([^}]+)\\}");
    QRegularExpressionMatchIterator varIter = varRx.globalMatch(result);

    if (varIter.hasNext()) {
        QString tempResult;
        int lastIndex = 0;

        while (varIter.hasNext()) {
            QRegularExpressionMatch match = varIter.next();
            tempResult += result.mid(lastIndex, match.capturedStart() - lastIndex);

            QString prefix = match.captured(1); // $, #, or !
            QString content = match.captured(2);

            QString varName = content;
            QString format = "";
            QString valueStr;

            // Detect Start Time if not provided
            QDateTime startTime = contextStartTime;
            if (!startTime.isValid() && !contextName.isEmpty()) {
                // Try to find the start time for this context (Assignment Name)
                QSettings settings(settingsFile, QSettings::IniFormat);
                QVariant val = settings.value("Assignments/" + contextName + "_start_time");
                if (val.isValid()) {
                    startTime = val.toDateTime();
                }
            }

            // --- Time Variable Formatting Logic ---
            if (prefix == "!") {
                // Check for format specifier (e.g. "VarName,d")
                int commaIdx = content.indexOf(',');
                if (commaIdx != -1) {
                    varName = content.left(commaIdx).trimmed();
                    format = content.mid(commaIdx + 1).trimmed().toLower();
                }

                // Try to get the raw QVariant
                QVariant val = getTimeVariableValue(varName, contextMinTime, contextMaxTime, contextStartTime, creationTime, deadline, nextRemind);

                if (val.isValid()) {
                    if (format == "d") { // Date (yyyy-MM-dd)
                         if (val.userType() == QMetaType::QDateTime) valueStr = val.toDateTime().toString("yyyy-MM-dd");
                         else if (val.userType() == QMetaType::QDate) valueStr = val.toDate().toString("yyyy-MM-dd");
                         else valueStr = val.toString();
                    }
                    else if (format == "t") { // Time (HH:mm)
                        // Note: Prompt example said (22:51), which is HH:mm.
                        // But standard is often HH:mm:ss. Let's use HH:mm for "t".
                         if (val.userType() == QMetaType::QDateTime) valueStr = val.toDateTime().toString("HH:mm");
                         else if (val.userType() == QMetaType::QTime) valueStr = val.toTime().toString("HH:mm");
                         else valueStr = val.toString();
                    }
                    else if (format == "i") { // Interval (3d or hh:mm:ss)
                        int secs = 0;
                        if (val.canConvert<int>()) secs = val.toInt();
                        else if (val.userType() == QMetaType::QTime) secs = QTime(0,0).secsTo(val.toTime());
                        else if (val.userType() == QMetaType::QDateTime) {
                            // If it's a date, interval might mean "time since epoch"?
                            // Usually not used this way, treat as 0.
                        }
                        valueStr = ScriptUtils::formatDurationString(secs);
                    }
                    else if (format == "dt") { // Date & Time
                        if (val.userType() == QMetaType::QDateTime) valueStr = val.toDateTime().toString("yyyy-MM-dd HH:mm");
                         else valueStr = val.toString();
                    }
                    else {
                        // No specific format, use default string conversion
                        valueStr = getVariableValue(varName, contextSeverity);
                    }
                } else {
                    // Not found as a Time Variable, fallback to generic lookup
                    valueStr = getVariableValue(varName, contextSeverity);
                }
            }
            else {
                // Handle # and $ (No formatting supported yet)
                valueStr = getVariableValue(content, contextSeverity);
            }
            // --------------------------------------

            tempResult += valueStr;
            lastIndex = match.capturedEnd();
        }
        tempResult += result.mid(lastIndex);
        result = tempResult;
    }

    // --- Flags {FlagName} ---
    // Replaces {FlagName} with On/Off text defined in [General]
    QString onText = data.general.flagOnText;
    QString offText = data.general.flagOffText;

    if (!onText.isEmpty() && !offText.isEmpty()) {
        // Matches {Name} where Name DOES NOT start with $, #, or !
        QRegularExpression flagRx("\\{([^\\$#!][^}]*)\\}");
        QRegularExpressionMatchIterator flagIter = flagRx.globalMatch(result);

        if (flagIter.hasNext()) {
            QString tempResult;
            int lastIndex = 0;

            while (flagIter.hasNext()) {
                QRegularExpressionMatch match = flagIter.next();
                tempResult += result.mid(lastIndex, match.capturedStart() - lastIndex);

                QString name = match.captured(1).trimmed();
                QString replacement = isFlagSet(name) ? onText : offText;

                tempResult += replacement;
                lastIndex = match.capturedEnd();
            }
            tempResult += result.mid(lastIndex);
            result = tempResult;
        }
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
    askPunishmentMin = data.general.askPunishmentMin;
    askPunishmentMax = data.general.askPunishmentMax;
    askPunishmentGroups = data.general.askPunishmentGroups.join(",");

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
        if (signinRemainingSecs == -1) {
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
        }
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
    bool canAskPunishment = permsEnabled && (askPunishmentMax > 0);
    ui->actionAsk_for_Punishment->setEnabled(canAskPunishment);
    ui->actionConfessions->setEnabled(confsEnabled);
    ui->actionReports->setEnabled(reportsEnabled);
    ui->menuRules->setEnabled(rulesEnabled);
    ui->menuAssignments->setEnabled(assignEnabled);
    ui->actionReport_Clothing->setVisible(allowClothReport);

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

int CyberDom::getMeritsFromIni() const {
    return ui->progressBar->value();
}

void CyberDom::initializeUiWithIniFile() {

    QSettings settings(settingsFile, QSettings::IniFormat);
    if (!settings.contains("User/Initialized")) {
        isFirstSessionRun = true;
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

        // Handle Script Begin Time
        if (settings.contains("User/BeginTime")) {
            // Load existing start time
            scriptBeginTime = QDateTime::fromString(settings.value("User/BeginTime").toString(), Qt::ISODate);
        } else {
            // First Run! Set to current internal time
            scriptBeginTime = internalClock;
            // We rely on the destructor to save this, matching the "User/Initialized" logic
        }

        // Handle Last Close Time
        if (settings.contains("User/LastCloseTime")) {
            lastAppCloseTime = QDateTime::fromString(settings.value("User/LastCloseTime").toString(), Qt::ISODate);
        }

        // Apply these initial values
        updateMerits(initialMerits);
        updateStatus(currentStatus);
    }

    if (isFirstSessionRun) {
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
    int oldMerits = ui->progressBar->value();
    int delta = newMerits - oldMerits;
    if (delta > 0) todayStats.meritsGained += delta;
    else if (delta < 0) todayStats.meritsLost += qAbs(delta);

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

    // Save Creation Time
    QSettings settings(settingsFile, QSettings::IniFormat);
    settings.setValue("Assignments/" + jobName + "_creation_time", internalClock);

    if (scriptParser && scriptParser->getScriptData().jobs.contains(jobName.toLower())) {
        const JobDefinition &def = scriptParser->getScriptData().jobs.value(jobName.toLower());

        if (def.isLineWriting && def.lineSelectMode.compare("Random", Qt::CaseInsensitive) == 0) {
            if (!def.lines.isEmpty()) {
                // Pick one line randomly
                int idx = ScriptUtils::randomInRange(0, def.lines.size() - 1, false);
                QString selectedLine = def.lines[idx];

                // Save it permanently for this assignment
                QSettings settings(settingsFile, QSettings::IniFormat);
                settings.setValue("Assignments/" + jobName + "_selected_line", selectedLine);
                qDebug() << "[LineWriter] Select=Random picked:" << selectedLine;
            }
        }
    }

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

        if (remindRange.compare("Never", Qt::CaseInsensitive) == 0) {
            // Explicitly disabled. Do not set an interval.
            qDebug() << "[DEBUG] RemindInterval is 'Never' for job:" << jobName;
        } else {
            int intervalSec = parseTimeRangeToSeconds(remindRange);
            if (intervalSec > 0)
                jobRemindIntervals[jobName] = intervalSec;
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

    if (!scriptParser) return;

    const ScriptData &data = scriptParser->getScriptData();

    for (const QString &name : activeAssignments) {
        if (!jobDeadlines.contains(name))
            continue;

        QDateTime deadline = jobDeadlines[name];
        if (now < deadline)
            continue;

        // Determine the type of assignment
        const PunishmentDefinition* punDef = getPunishmentDefinition(name);
        bool isPun = (punDef != nullptr);
        bool isJob = data.jobs.contains(name.toLower());

        if (!isPun && !isJob) { continue; }

        // --- Get properties with Hierarchy ---
        QString remindPenaltyRange;
        QString remindPenaltyGroup;
        QString expirePenaltyRange;
        QString expirePenaltyGroup;
        QString expireProcedure;
        QString remindProcedure;

        if (isJob) {
            const JobDefinition &jobDef = data.jobs.value(name.toLower());
            remindProcedure = jobDef.remindProcedure;

            // 1. Remind Penalty Hierarchy (Job -> General)
            if (jobDef.remindPenaltyMin > 0 || jobDef.remindPenaltyMax > 0) {
                remindPenaltyRange = QString::number(jobDef.remindPenaltyMin);
                if (jobDef.remindPenaltyMax > jobDef.remindPenaltyMin) {
                    remindPenaltyRange += "," + QString::number(jobDef.remindPenaltyMax);
                }
                remindPenaltyGroup = jobDef.remindPenaltyGroup;
            } else if (!data.general.globalRemindPenalty.isEmpty()) {
                remindPenaltyRange = data.general.globalRemindPenalty;
                remindPenaltyGroup = data.general.globalRemindPenaltyGroup;
            }

            // 2. Expire Penalty (Jobs only)
            if (jobDef.expirePenaltyMin > 0 || jobDef.expirePenaltyMax > 0) {
                expirePenaltyRange = QString::number(jobDef.expirePenaltyMin);
                if (jobDef.expirePenaltyMax > jobDef.expirePenaltyMin) {
                    expirePenaltyRange += "," + QString::number(jobDef.expirePenaltyMax);
                }
            }
            expirePenaltyGroup = jobDef.expirePenaltyGroup;
            expireProcedure = jobDef.expireProcedure;

        } else if (isPun) {
            // const PunishmentDefinition &punDef is already available from getPunishmentDefinition
            remindProcedure = punDef->remindProcedure;

            // 1. Remind Penalty Hierarchy (Punishment -> General)
            if (punDef->remindPenaltyMin > 0 || punDef->remindPenaltyMax > 0) {
                remindPenaltyRange = QString::number(punDef->remindPenaltyMin);
                if (punDef->remindPenaltyMax > punDef->remindPenaltyMin) {
                    remindPenaltyRange += "," + QString::number(punDef->remindPenaltyMax);
                }
                remindPenaltyGroup = punDef->remindPenaltyGroup;
            } else if (!data.general.globalRemindPenalty.isEmpty()) {
                remindPenaltyRange = data.general.globalRemindPenalty;
                remindPenaltyGroup = data.general.globalRemindPenaltyGroup;
            }
        }

        QSettings settings(settingsFile, QSettings::IniFormat);

        // --- Logic for Late/Expired Assignments ---
        if (!expiredAssignments.contains(name)) {
            // First time expiration
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
            // Repeated Reminder
            if (isInterruptAllowed()) {
                QMessageBox::information(this, "Assignment Late", QString("You are late completing %1").arg(name));
            }

            // Run Remind Procedure
            if (!remindProcedure.isEmpty()) {
                runProcedure(remindProcedure);
            }

            // Apply Remind Penalty (Calculated above)
            if (!remindPenaltyRange.isEmpty()) {
                int sev = randomIntFromRange(remindPenaltyRange);
                if (sev > 0)
                    applyPunishment(sev, remindPenaltyGroup, QString());
            }

            jobNextReminds[name] = now.addSecs(jobRemindIntervals[name]);
        }

        // Check if the assignment has permanently expired (Jobs only mostly)
        if (jobExpiryTimes.contains(name) && now >= jobExpiryTimes[name]) {
            if (!expirePenaltyRange.isEmpty()) {
                int sev = randomIntFromRange(expirePenaltyRange);
                if (sev > 0)
                    applyPunishment(sev, expirePenaltyGroup, QString());
            }
            if (!expireProcedure.isEmpty())
                runProcedure(expireProcedure);

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

    // 1. Get the base definition using the helper
    const PunishmentDefinition *punDef = getPunishmentDefinition(punishmentName);
    if (!punDef) {
        qDebug() << "[WARNING] Punishment section not found in ScriptData:" << punishmentName;
        return;
    }

    QString targetInstance;
    QString baseName = punDef->name.toLower();

    // 2. Check for an existing instance (Accumulative Logic)
    if (punDef->accumulative) {
        for (const QString &activeName : activeAssignments) {
            const PunishmentDefinition *activeDef = getPunishmentDefinition(activeName);
            if (activeDef && activeDef->name.toLower() == baseName) {
                int currentAmt = punishmentAmounts.value(activeName);
                if (currentAmt + amount <= punDef->max) {
                    targetInstance = activeName;
                    break; // Found a slot!
                }
            }
        }
    }

    // 3. Create new instance if needed
    bool isNewInstance = false;
    if (targetInstance.isEmpty()) {
        isNewInstance = true;
        int counter = 1;
        targetInstance = punDef->name;
        while (activeAssignments.contains(targetInstance)) {
            counter++;
            targetInstance = QString("%1_%2").arg(punDef->name).arg(counter);
        }
        activeAssignments.insert(targetInstance);
        qDebug() << "[DEBUG] Created new punishment instance:" << targetInstance;

        // Save Creation Time
        QSettings settings(settingsFile, QSettings::IniFormat);
        // Only save if new (don't overwrite if we are just adding to an existing accumulative punishment
        if (!settings.contains("Assignments/" + targetInstance + "_creation_time")) {
            settings.setValue("Assignments/" + targetInstance + "_creation_time", internalClock);
        }

        if (isNewInstance && punDef->isLineWriting && punDef->lineSelectMode.compare("Random", Qt::CaseInsensitive) == 0) {
            if (!punDef->lines.isEmpty()) {
                int idx = ScriptUtils::randomInRange(0, punDef->lines.size() - 1, false);
                QString selectedLine = punDef->lines[idx];

                QSettings settings(settingsFile, QSettings::IniFormat);
                settings.setValue("Assignments/" + targetInstance + "_selected_line", selectedLine);
                qDebug() << "[LineWriter] Select=Random picked:" << selectedLine;
            }
        }
    } else {
        qDebug() << "[DEBUG] Accumulating" << amount << "into existing:" << targetInstance;
    }

    // 4. Add the amount
    punishmentAmounts[targetInstance] += amount;

    // 5. Handle Deadline & Properties
    if (isNewInstance) {
        if (scriptParser) {
            QString proc = scriptParser->getScriptData().eventHandlers.punishmentGiven;
            if (!proc.isEmpty()) runProcedure(proc);
            if (!punDef->announceProcedure.isEmpty()) runProcedure(punDef->announceProcedure);
        }

        QDateTime deadline = internalClock;
        bool deadlineSet = false;

        // --- Respite (Variable-Aware) ---
        if (!punDef->respite.isEmpty()) {
            QString respiteStr = punDef->respite;
            if (respiteStr.startsWith("!") || respiteStr.startsWith("#")) {
                respiteStr = scriptParser->getVariable(respiteStr.mid(1));
            }

            QStringList respiteParts = respiteStr.split(":");
            if (respiteParts.size() >= 2) {
                deadline = deadline.addSecs(respiteParts[0].toInt() * 3600 + respiteParts[1].toInt() * 60);
                deadlineSet = true;
                qDebug() << "[DEBUG] Punishment deadline set from Respite:" << deadline.toString("MM-dd-yyyy hh:mm AP");
            }
        }

        // --- ValueUnit Logic (Initial Calculation) ---
        if (!deadlineSet && !punDef->valueUnit.isEmpty()) {
            double val = punDef->value > 0 ? punDef->value : 1.0;
            int total = qRound(val * amount);

            if (punDef->valueUnit.compare("minute", Qt::CaseInsensitive) == 0) {
                deadline = deadline.addSecs(total * 60);
                deadlineSet = true;
            } else if (punDef->valueUnit.compare("hour", Qt::CaseInsensitive) == 0) {
                deadline = deadline.addSecs(total * 3600);
                deadlineSet = true;
            } else if (punDef->valueUnit.compare("day", Qt::CaseInsensitive) == 0) {
                deadline = deadline.addDays(total);
                deadlineSet = true;
            }

            if (deadlineSet) {
                qDebug() << "[DEBUG] Punishment deadline set from ValueUnit:" << deadline.toString("MM-dd-yyyy hh:mm AP");
            }
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

        if (remindRange.compare("Never", Qt::CaseInsensitive) == 0) {
            // Explicitly disabled
            qDebug() << "[DEBUG] RemindInterval is 'Never' for punishment:" << targetInstance;
        } else {
            int intervalSec = parseTimeRangeToSeconds(remindRange);
            if (intervalSec > 0) jobRemindIntervals[targetInstance] = intervalSec;
        }

    } else {
        // Extending an existing TIME-BASED punishment
        // (We don't extend deadlines for repetitions, only for time units)
        if (!punDef->valueUnit.isEmpty() && punDef->valueUnit != "once") {
            QDateTime deadline = jobDeadlines.value(targetInstance, internalClock);
            double val = punDef->value > 0 ? punDef->value : 1.0;
            int total = qRound(val * amount);

            if (punDef->valueUnit.compare("minute", Qt::CaseInsensitive) == 0) {
                 deadline = deadline.addSecs(total * 60);
            } else if (punDef->valueUnit.compare("hour", Qt::CaseInsensitive) == 0) {
                 deadline = deadline.addSecs(total * 3600);
            } else if (punDef->valueUnit.compare("day", Qt::CaseInsensitive) == 0) {
                 deadline = deadline.addDays(total);
            }

            jobDeadlines[targetInstance] = deadline;
            qDebug() << "[DEBUG] Extended deadline for" << targetInstance << "to" << deadline.toString("MM-dd-yyyy hh:mm AP");
        }
    }

    emit jobListUpdated();
}

void CyberDom::applyPunishment(int severity, const QString &group, const QString &message)
{
    if (!scriptParser) return;
    if (severity <= 0) return;

    const GeneralSettings &general = scriptParser->getScriptData().general;

    // Enforce global MinPunishment
    int globalMin = general.minPunishment;
    if (severity < globalMin) {
        qDebug() << "[Punish] Severity" << severity << "increased to global minimum" << globalMin;
        severity = globalMin;
    }

    // Enforce global MaxPunishment
    int globalMax = general.maxPunishment;
    if (globalMax > 0 && severity > globalMax) {
        qDebug() << "[Punish] Severity" << severity << "capped at global maximum" << globalMax;
        severity = globalMax;
    }

    // 1. Try to find a single punishment that fits
    QString punishmentName = selectPunishmentFromGroup(severity, group);

    // 2. SPLIT LOGIC: If no single punishment fits, try to split the severity
    if (punishmentName.isEmpty()) {

        const auto &allPunishments = scriptParser->getScriptData().punishments;
        QStringList targetGroups;
        if (!group.isEmpty()) {
            QStringList rawGroups = group.split(',', Qt::SkipEmptyParts);
            for (const QString &g : rawGroups) targetGroups.append(g.trimmed().toLower());
        }

        int bestChunkSize = 0;

        // Iterate to find the biggest possible "chunk" of severity we can handle
        for (const PunishmentDefinition &def : allPunishments.values()) {

            // Check Group logic
            if (!targetGroups.isEmpty()) {
                bool groupMatch = false;
                for (const QString &pg : def.groups) {
                    if (targetGroups.contains(pg.trimmed().toLower())) { groupMatch = true; break; }
                }
                if (!groupMatch) continue;
            } else {
                 if (def.groupOnly) continue;
            }

            if (severity < def.minSeverity) continue;

            int limit = (def.maxSeverity > 0) ? def.maxSeverity : severity;
            int chunk = qMin(limit, severity);

            if (chunk > bestChunkSize) {
                bestChunkSize = chunk;
            }
        }

        if (bestChunkSize > 0 && bestChunkSize < severity) {
            qDebug() << "[Punish] Splitting severity" << severity << "into chunk" << bestChunkSize << "and remainder" << (severity - bestChunkSize);
            applyPunishment(bestChunkSize, group, message);
            applyPunishment(severity - bestChunkSize, group, message);
            return;
        } else {
            qDebug() << "[Punish] Unable to match or split severity" << severity << "in group" << group;
            QMessageBox::warning(this, "Error", "No suitable punishment (or combination of punishments) could be found for this severity.");
            return;
        }
    }

    // 3. INTERACTIVE LOOP
    int currentDeclineCount = 0;
    int maxDecline = scriptParser->getScriptData().general.maxDecline;

    while (true) {
        if (currentDeclineCount > 0) {
            punishmentName = selectPunishmentFromGroup(severity, group);
            if (punishmentName.isEmpty()) {
                punishmentName = selectPunishmentFromGroup(0, group);
                if (punishmentName.isEmpty()) return;
            }
        }

        const auto &punishments = scriptParser->getScriptData().punishments;
        QString lowerName = punishmentName.toLower();
        if (!punishments.contains(lowerName)) return;
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

        // --- 4. Build Display Message (Improved) ---
        QString displayMsg;

        // Clean up the title first
        QString rawTitle = punDef.title.isEmpty() ? punDef.name : punDef.title;
        QString cleanTitle = rawTitle;
        bool titleHadPlaceholder = false;

        // Replace '#' with the calculated amount
        if (cleanTitle.contains('#')) {
            cleanTitle.replace('#', QString::number(totalUnits));
            titleHadPlaceholder = true;
        }
        // Capitalize first letter
        if (!cleanTitle.isEmpty()) cleanTitle[0] = cleanTitle[0].toUpper();

        if (!message.isEmpty()) {
            // Custom message from script
            displayMsg = replaceVariables(message, punDef.name, punDef.title, severity);
            // Also replace # in the custom message if they used it
            displayMsg.replace('#', QString::number(totalUnits));
        } else {
            // Auto-generated message
            if (titleHadPlaceholder) {
                // Since the title now includes the amount (e.g. "No TV for 3 days"),
                // we don't need to repeat the amount/unit.
                displayMsg = QString("You have been punished: %1.").arg(cleanTitle);
            } else {
                // Standard fallback: "You have been punished with 5 Spanks of Spanking."
                displayMsg = QString("You have been punished with %1 %2 of %3.")
                        .arg(totalUnits)
                        .arg(valueUnit)
                        .arg(cleanTitle);
            }
        }

        if (currentDeclineCount > 0) {
            displayMsg += QString("\n\n(Severity increased by %1%. Declines remaining: %2)")
                          .arg(currentDeclineCount * 20)
                          .arg(maxDecline - currentDeclineCount);
        }

        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Punishment");
        msgBox.setText(displayMsg);
        msgBox.setIcon(QMessageBox::Warning);

        QPushButton *acceptBtn = msgBox.addButton("Accept", QMessageBox::AcceptRole);
        QPushButton *declineBtn = msgBox.addButton("Decline", QMessageBox::RejectRole);

        if (currentDeclineCount >= maxDecline) {
            declineBtn->setEnabled(false);
        }

        msgBox.exec();

        if (msgBox.clickedButton() == acceptBtn) {
            lastPunishmentSeverity = severity;

            if (valueUnit == "once") {
                addPunishmentToAssignments(punishmentName, totalUnits);
            } else {
                while (totalUnits > 0) {
                    int amount = qMin(max, totalUnits);
                    addPunishmentToAssignments(punishmentName, amount);
                    totalUnits -= amount;
                }
            }
            break;
        } else {
            currentDeclineCount++;
            severity = qRound(severity * 1.20);
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
    QList<QString> expiredFlags;

    // Identify expired flags
    for (auto it = flags.begin(); it != flags.end(); ++it) {
        if (it.value().expiryTime.isValid() && internalClock >= it.value().expiryTime) {
            expiredFlags.append(it.key());
        }
    }

    // Process them
    for (const QString &flagName : expiredFlags) {
        // Capture Context for ExpireProcedure
        if (scriptParser) {
            // FIX: Corrected typo 'FlagDefintion' -> 'FlagDefinition'
            const FlagDefinition &def = scriptParser->getScriptData().flagDefinitions.value(flagName.toLower());

            // --- FIX: Calculate Duration Correctly ---
            QString durationRange = def.durationMin;
            if (!def.durationMax.isEmpty() && def.durationMax != def.durationMin) {
                durationRange += "," + def.durationMax;
            }

            lastFlagDuration = 0;
            if (!durationRange.isEmpty()) {
                lastFlagDuration = parseTimeRangeToSeconds(durationRange);
            }
            // -----------------------------------------

            lastFlagExpiryTime = flags[flagName].expiryTime;

            if (!def.expireProcedure.isEmpty()) {
                runProcedure(def.expireProcedure);
            }
        }

        flags.remove(flagName);
        qDebug() << "[Flag] Expired:" << flagName;
    }

    if (!expiredFlags.isEmpty()) updateStatusText();
}

bool CyberDom::startAssignment(const QString &assignmentName, bool isPunishment, const QString &newStatus)
{
    if (hasActiveBlockingPunishment()) {
        QMessageBox::warning(this,
                             tr("Punishment Running"),
                             tr("Finish the current punishment before starting another task"));
        return false;
    }

    // Remember the previous status if newStatus is specified
    QString previousStatus;
    if (!newStatus.isEmpty()) {
        previousStatus = currentStatus;
        changeStatus(newStatus, true);
    }

    QString startProcedure;
    QString customStartFlag;
    QString beforeProcedure;

    QString pointCameraText;
    QString poseCameraText;
    QString assignmentTitle;
    QString cameraIntervalMin;
    QString cameraIntervalMax;
    // ------------------------------

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
                assignmentTitle = def.title;

                // --- 2. GET VALUES FROM PUNISHMENT ---
                pointCameraText = def.pointCameraText;
                cameraIntervalMin = def.cameraIntervalMin;
                cameraIntervalMax = def.cameraIntervalMax;
                // -------------------------------------

            } else {
                qDebug() << "[WARNING] Section not found for punishment: " << assignmentName;
            }
        } else {
            if (data.jobs.contains(lowerName)) {
                const JobDefinition &def = data.jobs.value(lowerName);
                startProcedure = def.startProcedure;
                customStartFlag = def.startFlag;
                beforeProcedure = def.beforeProcedure;
                assignmentTitle = def.title;

                // --- 3. GET VALUES FROM JOB ---
                pointCameraText = def.pointCameraText;
                cameraIntervalMin = def.cameraIntervalMin;
                cameraIntervalMax = def.cameraIntervalMax;
                // ------------------------------

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
            return false;
        }
    }

    // Set the built-in "_started" flag
    QString startFlagName = (isPunishment ? "punishment_" : "job_") + assignmentName + "_started";
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

    // --- 4. TRIGGER THE CAMERA ---
    if (!pointCameraText.isEmpty()) {
         QString sourcePrefix = isPunishment ? "Punishment_" : "Job_";
         QString resolvedPointText = replaceVariables(pointCameraText, assignmentName, assignmentTitle);
         triggerPointCamera(pointCameraText, cameraIntervalMin, cameraIntervalMax, sourcePrefix + assignmentName);
    }

    if (!poseCameraText.isEmpty()) {
        triggerPoseCamera(poseCameraText);
    }
    // -----------------------------

    updateStatusText();
    emit jobListUpdated();

    return true;
}

bool CyberDom::markAssignmentDone(const QString &assignmentName, bool isPunishment)
{
    // Settings is defined ONCE at the top.
    QSettings settings(settingsFile, QSettings::IniFormat);

    // We no longer use iniData. We get all "details" from the parser.
    if (!scriptParser) {
        qDebug() << "[ERROR] markAssignmentDone: scriptParser is null!";
        return false;
    }

    lastLatenessMinutes = 0;
    lastEarlyMinutes = 0;

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

    DurationControl dur;

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
            lastEarlyMinutes = static_cast<int>(std::ceil((required - elapsed) / 60.0));

            int penalty = 0;
            if (!quickPenalty1.isEmpty())
                penalty += quickPenalty1.toInt();
            if (!quickPenalty2.isEmpty()) {
                double ratio = quickPenalty2.toDouble();
                penalty += qRound(ratio * lastEarlyMinutes);
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
                QMessageBox::information(this, "Too Quick", replaceVariables(msg, assignmentName, "", 0, minTime));
            }

            if (!quickProcedure.isEmpty())
                runProcedure(quickProcedure);

            return false;
        }
    }

    // Check MaxTime
    QString maxTime = dur.maxTimeStart;
    if (!maxTime.isEmpty()) {
        int limit = parseTimeRangeToSeconds(maxTime);

        if (limit > 0 && elapsed > limit) {
            // User was too slow!
            int diffSeconds = elapsed - limit;
            lastLatenessMinutes = qCeil(diffSeconds / 60.0);

            qDebug() << "[Assignment] User was slow by" << lastLatenessMinutes << "minutes.";

            // Apply Penalties
            int penalty = 0;
            if (dur.slowPenalty1 != 0) {
                penalty += dur.slowPenalty1;
            }
            if (dur.slowPenalty2 != 0.0) {
                penalty += qRound(dur.slowPenalty2 * lastLatenessMinutes);
            }

            if (penalty > 0) {
                applyPunishment(penalty, dur.slowPenaltyGroup);
            }

            // Show Message
            if (!dur.slowMessage.isEmpty()) {
                QMessageBox::information(this, "Too Slow", replaceVariables(dur.slowMessage, assignmentName, "", 0, minTime, dur.maxTimeStart));
            }

            // Run Procedure
            if (!dur.slowProcedure.isEmpty()) {
                runProcedure(dur.slowProcedure);
            }
        } else {
            // Not Late
            lastLatenessMinutes = 0;
        }
    } else {
        lastLatenessMinutes = 0;
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

    settings.remove("Assignments/" + assignmentName + "_selected_line");
    settings.remove("Assignments/" + assignmentName + "_creation_time");

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
    QString timestamp = QTime::currentTime().toString("hh:mm AP");
    QString displayName = getAssignmentDisplayName(assignmentName, isPunishment);

    if (!isPunishment) {
        todayStats.punishmentsCompleted.append(assignmentName + " (" + QTime::currentTime().toString("hh:mm AP") + ")");
        QString settingsKey = QString("JobCompletion/%1_lastDone").arg(assignmentName);
        settings.setValue(settingsKey, internalClock.date().toString(Qt::ISODate));
        qDebug() << "[Scheduler] Job" << assignmentName << "completed. Saving lastDone date:" << internalClock.date().toString(Qt::ISODate);
    } else {
        todayStats.jobsCompleted.append(assignmentName + " (" + QTime::currentTime().toString("hh:mm AP") + ")");

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

    settings.remove("Assignments/" + assignmentName + "_selected_line");
    settings.remove("Assignments/" + assignmentName = "_creation_time");

    // Run the DeleteProcedure
    if (!deleteProcedure.isEmpty()) {
        runProcedure(deleteProcedure);
    }

    emit jobListUpdated();
}

bool CyberDom::isFlagSet(const QString &flagName) const {
    // Try exact match (fast)
    if (flags.contains(flagName)) return true;

    // Try case-insensitive match (robust)
    QString lowerName = flagName.toLower();
    for (auto it = flags.keyBegin(); it != flags.keyEnd(); ++it) {
        if (it->toLower() == lowerName) return true;
    }
    return false;
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
    // --- VALIDATION LOGIC ---

    // 1. Validate 'Check=' (Must wear)
    for (const QString &req : requiredClothingChecks) {
        bool satisfied = false;
        for (const ClothingItem &item : wearingItems) {
            if (itemMatchesCheck(item, req)) {
                satisfied = true;
                break;
            }
        }

        if (!satisfied) {
            // Failed a requirement
            clothFaultsCount++;
            QMessageBox::warning(this, "Incorrect Clothing",
                                 QString("You are supposed to wear %1.").arg(req));
            return; // Stop report
        }
    }

    // 2. Validate 'CheckOff=' (Must NOT wear)
    for (const QString &forbidden : forbiddenClothingChecks) {
        for (const ClothingItem &item : wearingItems) {
            if (itemMatchesCheck(item, forbidden)) {
                // Failed a prohibition
                QMessageBox::warning(this, "Incorrect Clothing",
                                     QString("You are not supposed to wear %1.").arg(forbidden));
                return; // Stop report
            }
        }
    }

    // --- Existing Reporting Logic ---
    QString reportText = "";

    if (isNaked) {
        reportText = "I am naked.";
    } else {
        reportText = "I am wearing:";
        QMap<QString, QStringList> itemsByType;
        for (const ClothingItem &item : wearingItems) {
            itemsByType[item.getType()].append(item.getName());
        }
        for (auto it = itemsByType.constBegin(); it != itemsByType.constEnd(); ++it) {
            reportText += "\n- " + it.key() + ": " + it.value().join(", ");
        }
    }

    storeClothingReport(reportText);

    QString time = QTime::currentTime().toString("hh:mm AP");
    QString summary = isNaked ? QString("Naked %1)").arg(time) : QString("Dressed (%1)").arg(time);
    todayStats.outfitsWorn.append(summary);

    QMessageBox::information(this, "Report Submitted", "Your clothing report has been submitted.");

    // Run 'AfterClothReport' event
    if (scriptParser) {
        QString proc = scriptParser->getScriptData().eventHandlers.afterClothReport;
        if (!proc.isEmpty()) runProcedure(proc);
    }
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

    QDateTime start = QDateTime::currentDateTime(); // Start Timer
    QuestionDialog dialog(questions, this);
    dialog.setWindowTitle(title);
    lastResponseDuration = start.secsTo(QDateTime::currentDateTime()); // End Timer

    if (dialog.exec() == QDialog::Accepted) {
        QString selectedAnswer;
        QMap<QString, QString> answers = dialog.getAnswers();

        // Use the guaranteed variable name
        if (answers.contains(questionData.variable)) {
            selectedAnswer = answers.value(questionData.variable);
        }

        lastQuestionAnswer = selectedAnswer;

        qDebug() << "[DEBUG] Question answered:" << selectedAnswer;

        questionAnswers[questionKey] = selectedAnswer;
        saveQuestionAnswers();

        bool matchFound = false;

        for (const auto &answerBlock : questionData.answers) {
            // Check: Does it match the Answer Text
            bool textMatch = (answerBlock.answerText.compare(selectedAnswer, Qt::CaseInsensitive) == 0);

            // Check: Does it match the Procedure Name
            bool procMatch = (answerBlock.procedureName.compare(selectedAnswer, Qt::CaseInsensitive) == 0);

            // Check: Does it match the explicit variable value
            bool varMatch = (!answerBlock.variableValue.isEmpty() &&
                             answerBlock.variableValue.compare(selectedAnswer, Qt::CaseInsensitive) == 0);

            if (textMatch || procMatch || varMatch) {
                matchFound = true;

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

        // Check "No Input" logic
        if (!matchFound && !questionData.noInputProcedure.isEmpty()) {
            // Ensure the answer really was empty/skipped
            if (selectedAnswer.isEmpty()) {
                qDebug() << "[Question] No input selected. Running NoInputProcedure:" << questionData.noInputProcedure;
                runProcedure(questionData.noInputProcedure);
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

bool CyberDom::runProcedure(const QString &procedureName) {
    if (!scriptParser) {
        qDebug() << "[ERROR] ScriptParser not initialized.";
        return false;
    }

    QString lowerProcName = procedureName.trimmed().toLower();
    const auto &procs = scriptParser->getScriptData().procedures;

    if (!procs.contains(lowerProcName)) {
        qDebug() << "[WARNING] Procedure section not found:" << procedureName;
        return false;
    }

    const ProcedureDefinition &proc = procs.value(lowerProcName);
    qDebug() << "\n[DEBUG] Running procedure:" << proc.name;

    // --- 1. Check PreStatus ---
    if (!proc.preStatuses.isEmpty()) {
        bool statusMatch = false;
        QString lowerCurrentStatus = currentStatus.toLower();
        for (const QString &preStatus : proc.preStatuses) {
            if (preStatus.toLower() == lowerCurrentStatus) {
                statusMatch = true;
                break;
            }
        }
        if (!statusMatch) {
            qDebug() << "[Procedure Skipped]" << proc.name << "is not allowed in status:" << currentStatus;
            return false; // Signal that we did not run
        }
    }

    // --- 2. Check Time Restrictions ---
    if (!isTimeAllowed(proc.notBeforeTimes, proc.notAfterTimes, proc.notBetweenTimes)) {
        qDebug() << "[DEBUG] Procedure skipped due to time restrictions:" << proc.name;
        return false; // Signal that we did not run
    }

    // --- 3. Prepare Selection Logic (First / Random) ---
    bool selectionSatisfied = false;
    int targetRandomIndex = -1;
    int currentProcCallIndex = 0;

    // If Random, pick the winner index in advance
    if (proc.selectMode == ProcedureSelectMode::Random) {
        int procCallCount = 0;
        for (const ScriptAction &a : proc.actions) {
            if (a.type == ScriptActionType::ProcedureCall) procCallCount++;
        }
        if (procCallCount > 0) {
            targetRandomIndex = ScriptUtils::randomInRange(0, procCallCount - 1, false);
        }
    }

    // --- 4. Execute Actions ---
    bool skipNextAction = false;

    for (const ScriptAction &action : proc.actions) {

        // Condition Check (If/NotIf)
        if (skipNextAction) {
            skipNextAction = false;
            if (action.type != ScriptActionType::If && action.type != ScriptActionType::NotIf) {
                qDebug() << "[Procedure] Action skipped due to condition.";
                continue;
            }
        }

        // Special Handling for Procedure Calls (Select Logic)
        if (action.type == ScriptActionType::ProcedureCall) {

            // Select=First: If we already ran one successfully, skip the rest
            if (proc.selectMode == ProcedureSelectMode::First && selectionSatisfied) {
                continue;
            }

            // Select=Random: Only run the one matching our random index
            if (proc.selectMode == ProcedureSelectMode::Random) {
                if (currentProcCallIndex != targetRandomIndex) {
                    currentProcCallIndex++;
                    continue;
                }
                currentProcCallIndex++;
            }

            // Try to run the sub-procedure
            QString procToCall = action.value.split(",").first().trimmed().toLower();

            // Recursively call runProcedure.
            // It returns true ONLY if it passed its own PreStatus/Time checks.
            if (runProcedure(procToCall)) {
                selectionSatisfied = true;
            }

            continue; // Done handling this action
        }

        // Handle all other actions
        switch (action.type) {
        case ScriptActionType::If:
            if (!evaluateCondition(action.value)) {
                qDebug() << "[Procedure] Aborting due to failed If:" << action.value;
                return true;
            }
            break;
        case ScriptActionType::NotIf:
            if (evaluateCondition(action.value)) {
                qDebug() << "[Procedure] Aborting due to failed NotIf:" << action.value;
                return true;
            }
            break;
        case ScriptActionType::SetFlag:
            setFlag(action.value);
            break;
        case ScriptActionType::RemoveFlag:
        case ScriptActionType::ClearFlag:
            removeFlag(action.value);
            break;
        case ScriptActionType::SetCounterVar:
        case ScriptActionType::AddCounter:
        case ScriptActionType::SubtractCounter:
        case ScriptActionType::MultiplyCounter:
        case ScriptActionType::DivideCounter:
        case ScriptActionType::InputCounter:
        case ScriptActionType::ChangeCounter:
        case ScriptActionType::InputNegCounter:
        case ScriptActionType::DropCounter:
            executeCounterAction(action.type, action.value);
            break;
        case ScriptActionType::RandomCounter:
            executeRandomCounterAction(action.value);
            break;

        case ScriptActionType::SetTimeVar:
        case ScriptActionType::InputDate:
        case ScriptActionType::InputDateDef:
        case ScriptActionType::ChangeDate:
        case ScriptActionType::InputTime:
        case ScriptActionType::InputTimeDef:
        case ScriptActionType::ChangeTime:
        case ScriptActionType::InputInterval:
        case ScriptActionType::ChangeInterval:
        case ScriptActionType::AddDaysTime:
        case ScriptActionType::SubtractDaysTime:
        case ScriptActionType::RoundTime:
        case ScriptActionType::RandomTime:
        case ScriptActionType::DropTime:
             executeTimeAction(action.type, action.value);
             break;

        case ScriptActionType::ExtractDays:
        case ScriptActionType::ExtractHours:
        case ScriptActionType::ExtractMinutes:
        case ScriptActionType::ExtractSeconds:
            executeTimeExtraction(action.type, action.value);
            break;

        case ScriptActionType::ConvertDays:
        case ScriptActionType::ConvertHours:
        case ScriptActionType::ConvertMinutes:
        case ScriptActionType::ConvertSeconds:
            executeCounterToTime(action.type, action.value);
            break;

        case ScriptActionType::Message: {
            QDateTime start = QDateTime::currentDateTime(); // Start Timer
            QMessageBox::information(this, "Message", replaceVariables(action.value));
            lastResponseDuration = start.secsTo(QDateTime::currentDateTime()); // End Timer
            break;
        }
        case ScriptActionType::Question:
            executeQuestion(action.value.trimmed().toLower(), "Question");
            break;
        case ScriptActionType::Input:
            executeQuestion(action.value.trimmed().toLower(), "Input Required");
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
        case ScriptActionType::AddMerit: modifyMerits(action.value, "add"); break;
        case ScriptActionType::SubtractMerit: modifyMerits(action.value, "subtract"); break;
        case ScriptActionType::SetMerit: modifyMerits(action.value, "set"); break;
        case ScriptActionType::Punish:
            executePunish(action.value, proc.punishMessage, proc.punishGroup);
            break;
        case ScriptActionType::Clothing: {
            QString text = resolveInstruction(action.value);
            updateClothingInstructions(text);
            openAskClothingDialog(action.value);
            break;
        }
        case ScriptActionType::Instructions: {
             QString text = resolveInstruction(action.value);
             updateInstructions(text);
             openAskInstructionsDialog(action.value);
             break;
        }
        case ScriptActionType::ClothReport: {
            QString val = action.value.trimmed();
            QString title = "";
            if (val != "1") title = replaceVariables(val);
            openReportClothingDialog(true, title);
            break;
        }
        case ScriptActionType::ClearCloth:
            clearCurrentClothing();
            break;
        case ScriptActionType::PointCamera: {
            QString minInt, maxInt;
            triggerPointCamera(action.value, minInt, maxInt, "Procedure_" + proc.name);
            break;
        }
        case ScriptActionType::PoseCamera: {
            triggerPoseCamera(action.value);
            break;
        }
        case ScriptActionType::SetString:
        case ScriptActionType::InputString:
        case ScriptActionType::ChangeString:
        case ScriptActionType::InputLongString:
        case ScriptActionType::ChangeLongString:
        case ScriptActionType::DropString:
            executeStringAction(action.type, action.value);
            break;
        default:
            break;
        }
    }

    return true; // Procedure executed successfully
}

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

    lastReportName = name;

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
    auto getMeritValue = [&](const QString &s) -> int {
        if (s.isEmpty()) return 0;
        if (s.startsWith("#")) {
            return scriptParser->getVariable(s.mid(1)).toInt();
        }
        return s.toInt();
    };

    if (!rep.merits.set.isEmpty()) {
        merits = getMeritValue(rep.merits.set);
    } else {
        merits += getMeritValue(rep.merits.add);
        merits -= getMeritValue(rep.merits.subtract);
    }

    merits = std::max(getMinMerits(), std::min(getMaxMerits(), merits));
    updateMerits(merits);

    incrementUsageCount(QString("Report/%1").arg(name));

    todayStats.reportsMade.append(name);

    // --- NEW: Handle StopAutoAssign ---
    if (rep.stopAutoAssign) {
        // Logic to stop auto-assign (if you have a flag or setting for this)
        // For now, we'll just log it as requested
        qDebug() << "[Report] StopAutoAssign triggered by" << name;
    }

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
        case ScriptActionType::SetCounterVar:
        case ScriptActionType::AddCounter:
        case ScriptActionType::SubtractCounter:
        case ScriptActionType::MultiplyCounter:
        case ScriptActionType::DivideCounter:
        case ScriptActionType::InputCounter:
        case ScriptActionType::ChangeCounter:
        case ScriptActionType::InputNegCounter:
        case ScriptActionType::DropCounter:
            executeCounterAction(action.type, action.value);
            break;
        case ScriptActionType::RandomCounter:
            executeRandomCounterAction(action.value);
            break;

        case ScriptActionType::SetTimeVar:
        case ScriptActionType::InputDate:
        case ScriptActionType::InputDateDef:
        case ScriptActionType::ChangeDate:
        case ScriptActionType::InputTime:
        case ScriptActionType::InputTimeDef:
        case ScriptActionType::ChangeTime:
        case ScriptActionType::InputInterval:
        case ScriptActionType::ChangeInterval:
        case ScriptActionType::AddDaysTime:
        case ScriptActionType::SubtractDaysTime:
        case ScriptActionType::RoundTime:
        case ScriptActionType::RandomTime:
        case ScriptActionType::DropTime:
            executeTimeAction(action.type, action.value);
            break;

        case ScriptActionType::ExtractDays:
        case ScriptActionType::ExtractHours:
        case ScriptActionType::ExtractMinutes:
        case ScriptActionType::ExtractSeconds:
            executeTimeExtraction(action.type, action.value);
            break;

        case ScriptActionType::ConvertDays:
        case ScriptActionType::ConvertHours:
        case ScriptActionType::ConvertMinutes:
        case ScriptActionType::ConvertSeconds:
            executeCounterToTime(action.type, action.value);
            break;

        case ScriptActionType::Message:
            QMessageBox::information(this, tr("Report"), replaceVariables(action.value));
            break;

        // --- ADDED MISSING CASES ---
        case ScriptActionType::Question:
            executeQuestion(action.value.trimmed().toLower(), "Question");
            break;
        case ScriptActionType::Input:
            executeQuestion(action.value.trimmed().toLower(), "Input Required");
            break;
        // ---------------------------

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
        case ScriptActionType::AddMerit: modifyMerits(action.value, "add"); break;
        case ScriptActionType::SubtractMerit: modifyMerits(action.value, "subtract"); break;
        case ScriptActionType::SetMerit: modifyMerits(action.value, "set"); break;

        case ScriptActionType::Punish:
            executePunish(action.value, rep.punishMessage, rep.punishGroup);
            break;

        case ScriptActionType::Clothing: {
            QString text = resolveInstruction(action.value);
            updateClothingInstructions(text);
            openAskClothingDialog(action.value);
            break;
        }
        case ScriptActionType::Instructions: {
            QString text = resolveInstruction(action.value);
            updateInstructions(text);
            openAskInstructionsDialog(action.value);
            break;
        }
        case ScriptActionType::ClothReport: {
            QString val = action.value.trimmed();
            QString title = "";
            if (val != "1") title = replaceVariables(val);
            openReportClothingDialog(true, title);
            break;
        }
        case ScriptActionType::ClearCloth:
            clearCurrentClothing();
            break;

        case ScriptActionType::PointCamera: {
            QString minInt, maxInt;
            triggerPointCamera(action.value, minInt, maxInt, "Procedure_" + rep.name);
            break;
        }

        case ScriptActionType::PoseCamera: {
            triggerPoseCamera(action.value);
            break;
        }

        case ScriptActionType::SetString:
        case ScriptActionType::InputString:
        case ScriptActionType::ChangeString:
        case ScriptActionType::InputLongString:
        case ScriptActionType::ChangeLongString:
        case ScriptActionType::DropString:
            executeStringAction(action.type, action.value);
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
    lastScheduledJobsRun = QDate::fromString(session.value("Session/LastJobsRun").toString(), Qt::ISODate);
    lastDayMeritsGiven = QDate::fromString(session.value("Session/LastDayMeritsGiven").toString(), Qt::ISODate);
    lastReportFilename = session.value("Session/LastReportFile").toString();
    lastReportName = session.value("Session/LastReportName").toString();
    lastPermissionName = session.value("Session/LastPermissionName").toString();
    lastQuestionAnswer = session.value("Session/LastQuestionAnswer").toString();
    clothFaultsCount = session.value("Session/ClothFaultsCount", 0).toInt();
    lastPunishmentSeverity = session.value("Session/LastPunishmentSeverity", 0).toInt();
    lastLatenessMinutes = session.value("Session/LastLatenessMinutes", 0).toInt();
    lastEarlyMinutes = session.value("Session/LastEarlyMinutes", 0).toInt();

    QDateTime lastInternal = QDateTime::fromString(session.value("Session/InternalClock").toString(), Qt::ISODate);
    QDateTime lastSystem = QDateTime::fromString(session.value("Session/LastSystemTime").toString(), Qt::ISODate);

    // --- 1. Load Status History ---
    QStringList historyList = session.value("Session/StatusHistory").toStringList();
    statusHistory.clear();
    for(const QString& s : historyList) statusHistory.push(s);

    // --- 2. Load Assignments ---
    session.beginGroup("Assignments");
    QStringList assignments = session.value("ActiveJobs").toStringList();
    QSet<QString> loadedAssignments;
    for (const QString &name : assignments) {
        // Normalize names (remove prefixes if present in old saves)
        if (name.startsWith("job-")) loadedAssignments.insert(name.mid(4));
        else if (name.startsWith("punishment-")) loadedAssignments.insert(name.mid(11));
        else loadedAssignments.insert(name);
    }
    activeAssignments = loadedAssignments;

    session.beginGroup("Deadlines");
    for (const QString &key : session.childKeys()) {
        QString cleanKey = key;
        if (cleanKey.startsWith("job-")) cleanKey = cleanKey.mid(4);
        else if (cleanKey.startsWith("punishment-")) cleanKey = cleanKey.mid(11);
        jobDeadlines[cleanKey] = QDateTime::fromString(session.value(key).toString(), Qt::ISODate);
    }
    session.endGroup();

    session.beginGroup("Expiry");
    for (const QString &key : session.childKeys()) {
        jobExpiryTimes[key] = QDateTime::fromString(session.value(key).toString(), Qt::ISODate);
    }
    session.endGroup();

    // --- 3. Load Expired Assignments ---
    QStringList expiredList = session.value("ExpiredAssignmentsSet").toStringList();
    expiredAssignments = QSet<QString>(expiredList.begin(), expiredList.end());

    session.beginGroup("RemindIntervals");
    for (const QString &key : session.childKeys()) jobRemindIntervals[key] = session.value(key).toInt();
    session.endGroup();

    session.beginGroup("NextReminds");
    for (const QString &key : session.childKeys()) jobNextReminds[key] = QDateTime::fromString(session.value(key).toString(), Qt::ISODate);
    session.endGroup();

    session.beginGroup("LateMerits");
    for (const QString &key : session.childKeys()) jobLateMerits[key] = session.value(key).toInt();
    session.endGroup();

    session.beginGroup("Amounts");
    for (const QString &key : session.childKeys()) punishmentAmounts[key] = session.value(key).toInt();
    session.endGroup();
    session.endGroup(); // Assignments

    // --- 4. Load Flags ---
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

    // --- 5. Load Counters ---
    session.beginGroup("Counters");

    session.beginGroup("Reports");
    for (const QString &key : session.childKeys()) reportCounts[key] = session.value(key).toInt();
    session.endGroup();

    session.beginGroup("Confessions");
    for (const QString &key : session.childKeys()) confessionCounts[key] = session.value(key).toInt();
    session.endGroup();

    session.beginGroup("Permissions");
    for (const QString &key : session.childKeys()) permissionCounts[key] = session.value(key).toInt();
    session.endGroup();

    session.beginGroup("PermissionTimers");
    for (const QString &key : session.childKeys()) {
        QDateTime dt = QDateTime::fromString(session.value(key).toString(), Qt::ISODate);
        if (dt.isValid()) permissionNextAvailable[key] = dt;
    }
    session.endGroup();

    session.beginGroup("PermissionDeniedStats");
    for (const QString &key : session.childKeys()) {
        QDateTime dt = QDateTime::fromString(session.value(key).toString(), Qt::ISODate);
        if (dt.isValid()) permissionFirstDenied[key] = dt;
    }
    session.endGroup();

    QStringList pendingNotes = session.value("Session/PermissionNotifications").toStringList();
    permissionNotificationsPending = QSet<QString>(pendingNotes.begin(), pendingNotes.end());
    session.endGroup(); // Counters

    // --- 6. Load Daily Stats ---
    session.beginGroup("DailyStats");
    todayStats.jobsCompleted = session.value("Jobs").toStringList();
    todayStats.punishmentsCompleted = session.value("Punishments").toStringList();
    todayStats.outfitsWorn = session.value("Outfits").toStringList();
    todayStats.meritsGained = session.value("MeritsGained", 0).toInt();
    todayStats.meritsLost = session.value("MeritsLost", 0).toInt();
    todayStats.permissionsAsked = session.value("Permissions").toStringList();
    todayStats.reportsMade = session.value("Reports").toStringList();
    todayStats.confessionsMade = session.value("Confessions").toStringList();
    session.endGroup();

    lastScheduledJobsRun = QDate::fromString(session.value("Session/LastJobsRun").toString(), Qt::ISODate);
    lastDayMeritsGiven = QDate::fromString(session.value("Session/LastDayMeritsGiven").toString(), Qt::ISODate);

    if (script.isEmpty() || !QFile::exists(script)) {
        return false;
    }

    loadAndParseScript(script);
    updateMerits(merits);
    if (!status.isEmpty()) {
        currentStatus = status;
        updateStatusDisplay();
    }

    // Calculate time offset
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

    // Clear old session data to prevent "zombie" data
    session.clear();

    session.setValue("Session/ScriptPath", currentIniFile);
    session.setValue("Session/Merits", ui->progressBar->value());
    session.setValue("Session/Status", currentStatus);
    session.setValue("Session/InternalClock", internalClock.toString(Qt::ISODate));
    session.setValue("Session/LastSystemTime", QDateTime::currentDateTime().toString(Qt::ISODate));
    session.setValue("Session/LastInstructions", lastInstructions);
    session.setValue("Session/LastClothingInstructions", lastClothingInstructions);
    session.setValue("Session/LastJobsRun", lastScheduledJobsRun.toString(Qt::ISODate));
    session.setValue("Session/LastDayMeritsGiven", lastDayMeritsGiven.toString(Qt::ISODate));
    session.setValue("Session/LastReportFile", lastReportFilename);
    session.setValue("Session/LastReportName", lastReportName);
    session.setValue("Session/LastPermissionName", lastPermissionName);
    session.setValue("Session/LastQuestionAnswer", lastQuestionAnswer);
    session.setValue("Session/ClothFaultsCount", clothFaultsCount);
    session.setValue("Session/LastPunishmentSeverity", lastPunishmentSeverity);
    session.setValue("Session/LastLatenessMinutes", lastLatenessMinutes);
    session.setValue("Session/LastEarlyMinutes", lastEarlyMinutes);

    // --- 1. Save Status History (New) ---
    // Convert Stack to List for saving
    session.setValue("Session/StatusHistory", statusHistory.toList());

    // --- 2. Save Assignments ---
    session.beginGroup("Assignments");
    session.setValue("ActiveJobs", QStringList(activeAssignments.values()));

    session.beginGroup("Deadlines");
    for (auto it = jobDeadlines.constBegin(); it != jobDeadlines.constEnd(); ++it) {
        session.setValue(it.key(), it.value().toString(Qt::ISODate));
    }
    session.endGroup();

    session.beginGroup("Expiry");
    for (auto it = jobExpiryTimes.constBegin(); it != jobExpiryTimes.constEnd(); ++it) {
        session.setValue(it.key(), it.value().toString(Qt::ISODate));
    }
    session.endGroup();

    // --- 3. Save Expired Assignments Set (New) ---
    session.setValue("ExpiredAssignmentsSet", QStringList(expiredAssignments.values()));

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

    // --- 4. Save Flags ---
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

    // --- REMOVED: Answers Group (Saved in user_settings.ini via saveQuestionAnswers) ---

    // --- 5. Save Counters ---
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

    session.beginGroup("PermissionTimers");
    for (auto it = permissionNextAvailable.constBegin(); it != permissionNextAvailable.constEnd(); ++it) {
        session.setValue(it.key(), it.value().toString(Qt::ISODate));
    }
    session.endGroup();

    session.beginGroup("PermissionDeniedStats");
    for (auto it = permissionFirstDenied.constBegin(); it != permissionFirstDenied.constEnd(); ++it) {
        session.setValue(it.key(), it.value().toString(Qt::ISODate));
    }
    session.endGroup();

    session.setValue("Session/PermissionNotifications", QStringList(permissionNotificationsPending.values()));
    session.endGroup(); // Counters

    // --- 6. Save Daily Stats (New) ---
    session.beginGroup("DailyStats");
    session.setValue("Jobs", todayStats.jobsCompleted);
    session.setValue("Punishments", todayStats.punishmentsCompleted);
    session.setValue("Outfits", todayStats.outfitsWorn);
    session.setValue("MeritsGained", todayStats.meritsGained);
    session.setValue("MeritsLost", todayStats.meritsLost);
    session.setValue("Permissions", todayStats.permissionsAsked);
    session.setValue("Reports", todayStats.reportsMade);
    session.setValue("Confessions", todayStats.confessionsMade);
    session.endGroup();
    // ---------------------------------

    session.sync();
    qDebug() << "[CyberDom] Session data saved to" << path;
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

    if (signinRemainingSecs != -1 && signinRemainingSecs <= 0) {
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
    return ScriptUtils::parseDurationString(timeStr);
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

        // Determine if it is a punishment (robust check handling _2 suffixes)
        bool isPun = (getPunishmentDefinition(name) != nullptr);

        CalendarEvent ev;

        // Use getAssignmentDisplayName to format the title
        ev.title = getAssignmentDisplayName(name, isPun);

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
    if (!scriptParser) return false;

    for (const QString &name : activeAssignments) {

        // 1. Use helper to check if it's a punishment (handles _2 suffixes)
        const PunishmentDefinition* punDef = getPunishmentDefinition(name);

        // If nullptr, it's a Job, so it doesn't block.
        if (!punDef) continue;

        // 2. Check if started
        QString flag = "punishment_" + name + "_started";
        if (!isFlagSet(flag)) continue;

        // 3. Check if Long-Running
        if (punDef->longRunning) continue;
        if (punDef->valueUnit.compare("day", Qt::CaseInsensitive) == 0) continue;

        // Found a started, non-long-running punishment
        qDebug() << "[Status] Blocking punishment active:" << name;
        return true;
    }

    return false;
}

bool CyberDom::evaluateCondition(const QString &condition)
{
    if (!scriptParser) return false;

    return ScriptUtils::evaluateCondition(condition,
        [this](const QString &n){ return isFlagSet(n); },
        [this](const QString &n){ return scriptParser->getVariable(n); }
    );
}

QString CyberDom::getAssignmentEstimate(const QString &assignmentName, bool isPunishment) const
{
    if (!scriptParser) return QString("N/A");

    const ScriptData &data = scriptParser->getScriptData();
    QString lowerName = assignmentName.toLower();
    QString estimateStr;

    if (isPunishment) {
        if (data.punishments.contains(lowerName)) {
            const PunishmentDefinition &def = data.punishments.value(lowerName);

            // --- Check if measured in time ---
            QString vu = def.valueUnit.toLower();
            if (vu == "minute" || vu == "hour" || vu == "day") {
                // It is measured in time, so we ignore def.estimate
                // and calculate the actual duration based on the amount
                int amount = punishmentAmounts.value(assignmentName, 0);
                qint64 totalSecs = 0;

                if (vu == "minute") totalSecs = amount * 60;
                else if (vu == "hour") totalSecs = amount * 3600;
                else if (vu == "day") totalSecs = amount * 86400;

                if (totalSecs > 0) {
                    // Format as HH:mm
                    qint64 hours = totalSecs / 3600;
                    qint64 mins = (totalSecs % 3600) / 60;
                    return QString("%1:%2")
                            .arg(hours, 2, 10, QChar('0'))
                            .arg(mins, 2, 10, QChar('0'));
                }
            }

            // Not time-based, use the script's estimate
            estimateStr = def.estimate;
        }
    } else {
        // Jobs Logic
        if (data.jobs.contains(lowerName)) {
            estimateStr = data.jobs.value(lowerName).estimate;
        }
    }

    if (estimateStr.isEmpty()) {
        return "N/A";
    }

    // Resolve the string if it's a variable (e.g. !myTimeVar)
    if (estimateStr.startsWith("!") || estimateStr.startsWith("#")) {
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

QString CyberDom::resolveInstruction(const QString &name, QStringList *chosenItems, QSet<QString> visited, bool isClothingContext)
{
    if (!scriptParser) return QString();

    QString lowerName = name.toLower();

    // --- 1. RECURSION GUARD ---
    if (visited.contains(lowerName)) {
        qWarning() << "[Instructions] Infinite loop detected! Set" << name << "calls itself.";
        return QString("[Error: Circular Dependency in %1]").arg(name);
    }
    visited.insert(lowerName);

    const auto &instructions = scriptParser->getScriptData().instructions;
    if (!instructions.contains(lowerName)) {
        qDebug() << "[Instructions] Warning: Instruction set not found:" << name;
        return name;
    }

    const InstructionDefinition &def = instructions.value(lowerName);
    QStringList resultLines;

    // Determine if we are in a clothing context
    // (Either passed in, or this definition itself is clothing)
    bool currentContextIsClothing = isClothingContext || def.isClothing;

    // Manage the chosenItems list
    QStringList localChosenItems;
    if (!chosenItems) {
        chosenItems = &localChosenItems;
    }

    // Clear previous checks if this is a ROOT clothing instruction
    if (def.isClothing && def.clearClothingCheck && visited.size() == 1) {
        requiredClothingChecks.clear();
        forbiddenClothingChecks.clear();
    }

    for (const InstructionSet &set : def.sets) {

        // --- 1. Check Flags ---
        if (!checkRequirements(set.ifFlagGroups, set.notIfFlagGroups)) {
            continue;
        }

        // --- 2. Check History ---
        if (!ScriptUtils::checkHistoryConditions(set.ifChosen, set.ifNotChosen, *chosenItems)) {
            continue;
        }

        QList<InstructionStep> eligibleSteps = set.steps;
        if (eligibleSteps.isEmpty()) continue;

        // --- 3. Selection Logic ---
        ScriptUtils::SelectMode mode = ScriptUtils::SelectMode::All;
        if (set.selectMode == InstructionSelectMode::First) mode = ScriptUtils::SelectMode::First;
        else if (set.selectMode == InstructionSelectMode::Random) mode = ScriptUtils::SelectMode::Random;

        ScriptUtils::processSelector(set.steps.size(), mode, [&](int index) -> bool {
            const InstructionStep &step = set.steps[index];

            if (step.type == InstructionStepType::SetReference) {
                // RECURSIVE CALL: Pass the 'currentContextIsClothing' flag down!
                QString refText = resolveInstruction(step.setReference, chosenItems, visited, currentContextIsClothing);
                if (!refText.isEmpty()) {
                    resultLines.append(refText);
                    return true;
                }
            }
            else if (step.type == InstructionStepType::Choice) {
                const InstructionChoice &choice = step.choice;
                if (choice.options.isEmpty()) return false;

                int totalWeight = 0;
                for (const InstructionOption &opt : choice.options) totalWeight += opt.weight;

                if (totalWeight > 0) {
                    int roll = ScriptUtils::randomInRange(1, totalWeight, false);
                    int current = 0;
                    for (const InstructionOption &opt : choice.options) {
                        current += opt.weight;
                        if (roll <= current) {
                            // Selection Success
                            if (!opt.skip && !opt.text.isEmpty()) {
                                QString txt = replaceVariables(opt.text);
                                resultLines.append(txt);
                                chosenItems->append(txt);
                            }

                            // Store Checks (Using the Context Flag)
                            if (currentContextIsClothing) {
                                requiredClothingChecks.append(opt.check);
                                forbiddenClothingChecks.append(opt.checkOff);
                            }
                            return true;
                        }
                    }
                }
            }
            return false;
        });
    }

    QString finalResult = resultLines.join("\n");
    return finalResult;
}

bool CyberDom::itemMatchesCheck(const ClothingItem &item, const QString &checkName) const
{
    if (!scriptParser) return false;

    const auto &clothTypes = scriptParser->getScriptData().clothingTypes;
    QString typeName = item.getType().toLower();

    if (!clothTypes.contains(typeName)) return false;

    const ClothingTypeDefinition &typeDef = clothTypes.value(typeName);
    QString targetCheck = checkName.trimmed();

    // Check the Type itself (e.g., [ClothType-Bra] Check=Bra)
    for (const QString &c : typeDef.checks) {
        if (c.compare(targetCheck, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }

    // Check Attributes
    for (const ClothingAttribute &attrDef : typeDef.attributes) {

        // Get the value the user selected for this attribute (e.g., "Bustier")
        QString itemValue = item.getAttribute(attrDef.name);

        if (itemValue.isEmpty()) continue;

        // Look up this value in the type definition's valueChecks map.
        if (typeDef.valueChecks.contains(itemValue)) {
            const QStringList &checks = typeDef.valueChecks.value(itemValue);

            // See if any of the checks provided by this value match our target
            for (const QString &c : checks) {
                if (c.compare(targetCheck, Qt::CaseInsensitive) == 0) {
                    return true;
                }
            }
        }
    }

    return false;
}

void CyberDom::clearCurrentClothing()
{
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(QDir::homePath() + "/.cyberdom");
    if (!dir.exists()) dir.mkpath(".");

    QSettings settings(dir.absoluteFilePath("clothing.ini"), QSettings::IniFormat);

    // Clear the "wearing" array
    settings.beginWriteArray("wearing", 0);
    settings.endArray();
    settings.sync();

    qDebug() << "[Clothing] User's current clothing cleared.";
}

int CyberDom::calculateSecondsFromTimeRange(const TimeRange &tr)
{
    if (tr.min.isEmpty()) return 0;

    // Helper to resolve a single string (literal "hh:mm" or variable "!var")
    auto resolve = [&](const QString &s) -> int {
        QString val = s;
        if (val.startsWith("!") || val.startsWith("#")) {
            if (scriptParser) val = scriptParser->getVariable(val.mid(1));
        }
        return parseTimeToSeconds(val);
    };

    int minSec = resolve(tr.min);
    // If max is empty, it equals min. If not, resolve it.
    int maxSec = tr.max.isEmpty() ? minSec : resolve(tr.max);

    if (minSec == maxSec) return minSec;

    // Return random value in range
    return ScriptUtils::randomInRange(std::min(minSec, maxSec), std::max(minSec, maxSec), false);
}

bool CyberDom::checkRequirements(const QList<QStringList> &ifGroups, const QList<QStringList> &notIfGroups)
{
    if (!scriptParser) return false;

    return ScriptUtils::checkConditions(ifGroups, notIfGroups,
        [this](const QString &n){ return isFlagSet(n); },
        [this](const QString &n){ return scriptParser->getVariable(n); }
    );
}

void CyberDom::triggerPointCamera(const QString &text, const QString &minInterval, const QString &maxInterval, const QString &sourceName)
{
    if (checkInterruptableAssignments()) return;

    QString resolvedText = replaceVariables(text);

    // Show the Dialog
    PointCameraDialog dlg(this, resolvedText);
    dlg.exec();

    // Start Interval Timer (if defined)
    if (!minInterval.isEmpty()) {
        // Stop existing timer for this source if any
        if (activeCameraTimers.contains(sourceName)) {
            activeCameraTimers[sourceName]->stop();
            delete activeCameraTimers[sourceName];
            activeCameraTimers.remove(sourceName);
        }

        // Store intervals for re-calculation
        cameraIntervals[sourceName] = qMakePair(minInterval, maxInterval);

        // Setup new timer
        QTimer *timer = new QTimer(this);
        timer->setSingleShot(true);
        connect(timer, &QTimer::timeout, this, [this, sourceName]() {
            handleCameraTimer(sourceName);
        });

        // Calculate first interval
        QString range = minInterval;
        if (!maxInterval.isEmpty()) range += "," + maxInterval;
        int secs = parseTimeRangeToSeconds(range);

        if (secs > 0) {
            timer->start(secs * 1000);
            activeCameraTimers[sourceName] = timer;
            qDebug() << "[Camera] Started interval for" << sourceName << "Next shot in" << secs << "s";
        }
    }
}

void CyberDom::handleCameraTimer(const QString &sourceName)
{
    // Take the picture
    takePicture(sourceName);

    // Restart the timer (Randomize next interval)
    if (activeCameraTimers.contains(sourceName) && cameraIntervals.contains(sourceName)) {
        QPair<QString, QString> interval = cameraIntervals[sourceName];
        QString range = interval.first;
        if (!interval.second.isEmpty()) range += "," + interval.second;

        int secs = parseTimeRangeToSeconds(range);
        if (secs > 0) {
            activeCameraTimers[sourceName]->start(secs * 1000);
            qDebug() << "[Camera]" << sourceName << "photo taken. Next in" << secs << "s";
        }
    }
}

void CyberDom::triggerPoseCamera(const QString &text)
{
    if (checkInterruptableAssignments()) return;

    QString resolvedText = replaceVariables(text);

    // Get save folder from settings
    QString folder;
    if (scriptParser) {
        folder = scriptParser->getScriptData().general.cameraFolder;
    }

    PoseCameraDialog dlg(this, resolvedText, folder);
    dlg.exec(); // Blocks until "Okay" is clicked
}

// Helper to track permissions
void CyberDom::trackPermissionEvent(const QString &name, const QString &result) {
    QString time = QTime::currentTime().toString("hh:mm AP");
    todayStats.permissionsAsked.append(QString("%1: %2 (%3)").arg(name, result, time));
}

// Generate the HTML content
QString CyberDom::generateReportHtml(bool isEndOfDay)
{
    QString title = isEndOfDay ? "Daily Activity Report" : "Activity Report (Interim)";
    QString dateStr = internalClock.date().toString("dddd, MMMM d, yyyy");

    QString html = R"(
    <html>
    <head>
        <style>
            body { font-family: sans-serif; color: #333; background-color: #f4f4f4; padding: 20px; }
            .container { max-width: 800px; margin: auto; background: white; padding: 30px; border-radius: 8px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }
            h1 { color: #2c3e50; border-bottom: 2px solid #eee; padding-bottom: 10px; }
            h2 { color: #e67e22; margin-top: 25px; font-size: 1.2em; }
            ul { list-style-type: none; padding: 0; }
            li { background: #f9f9f9; margin: 5px 0; padding: 8px; border-left: 4px solid #3498db; }
            .stats-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 20px; margin-bottom: 20px; }
            .stat-box { background: #ecf0f1; padding: 15px; text-align: center; border-radius: 5px; }
            .stat-num { font-size: 24px; font-weight: bold; color: #2980b9; }
            .stat-label { color: #7f8c8d; font-size: 0.9em; }
        </style>
    </head>
    <body>
    <div class="container">
    )";

    html += QString("<h1>%1</h1>").arg(title);
    html += QString("<p><strong>Date:</strong> %1</p>").arg(dateStr);
    html += QString("<p><strong>Status:</strong> %1</p>").arg(currentStatus);

    // Stats Grid
    html += "<div class='stats-grid'>";
    html += QString("<div class='stat-box'><div class='stat-num'>+%1</div><div class='stat-label'>Merits Gained</div></div>").arg(todayStats.meritsGained);
    html += QString("<div class='stat-box'><div class='stat-num' style='color:#c0392b'>-%1</div><div class='stat-label'>Merits Lost</div></div>").arg(todayStats.meritsLost);
    html += QString("<div class='stat-box'><div class='stat-num'>%1</div><div class='stat-label'>Jobs Done</div></div>").arg(todayStats.jobsCompleted.size());
    html += QString("<div class='stat-box'><div class='stat-num'>%1</div><div class='stat-label'>Punishments</div></div>").arg(todayStats.punishmentsCompleted.size());
    html += "</div>";

    // Helper to add sections
    auto addSection = [&](const QString &header, const QStringList &items) {
        if (items.isEmpty()) return;
        html += QString("<h2>%1</h2><ul>").arg(header);
        for (const QString &item : items) html += QString("<li>%1</li>").arg(item);
        html += "</ul>";
    };

    addSection("Jobs Completed", todayStats.jobsCompleted);
    addSection("Punishments Completed", todayStats.punishmentsCompleted);
    addSection("Outfits Worn", todayStats.outfitsWorn);
    addSection("Permissions", todayStats.permissionsAsked);
    addSection("Reports Submitted", todayStats.reportsMade);
    addSection("Confessions", todayStats.confessionsMade);

    // Currently Active
    html += "<h2>Current Status</h2><ul>";
    if (activeAssignments.isEmpty()) {
        html += "<li>No active assignments.</li>";
    } else {
        for (const QString &name : activeAssignments) {
            QString deadline = jobDeadlines.value(name).toString("MM-dd hh:mm AP");

            bool isPun = false;
            if (getPunishmentDefinition(name)) isPun = true;

            // Use helper to get nice name
            QString displayName = getAssignmentDisplayName(name, isPun);

            QString startFlag = (isPun ? "punishment_" : "job_") + name + "_started";
            bool isStarted = isFlagSet(startFlag);

            QString statusStr = isStarted ? "<strong>(Started)</strong>" : "(Not Started)";
            html += QString("<li>%1 %2 - Due: %3</li>").arg(displayName, statusStr, deadline);
        }
    }
    html += "</ul></div></body></html>";
    return html;
}

void CyberDom::generateDailyReportFile(bool isEndOfDay)
{
    QString html = generateReportHtml(isEndOfDay);
    QString reportDir = QCoreApplication::applicationDirPath() + "/Reports";
    QDir().mkpath(reportDir);

    QString filename = QString("Report_%1.html").arg(internalClock.toString("yyyy-MM-dd"));
    lastReportFilename = filename;
    QFile file(reportDir + "/" + filename);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << html;
        file.close();
        qDebug() << "[Report] Saved daily report to:" << file.fileName();
    }
}

void CyberDom::onMakeReportFile() {
    generateDailyReportFile(false);
    QString path = QCoreApplication::applicationDirPath() + "/Reports/Report_" + internalClock.toString("yyyy-MM-dd") + ".html";
    QMessageBox::information(this, "Report Generated", "Report saved to:\n" + path);
}

void CyberDom::onViewReportFile() {
    QString path = QCoreApplication::applicationDirPath() + "/Reports/Report_" + internalClock.toString("yyyy-MM-dd") + ".html";
    if (QFile::exists(path)) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    } else {
        QMessageBox::warning(this, "No Report", "No report has been generated for today yet.");
    }
}

QString CyberDom::getAssignmentDisplayName(const QString &assignmentName, bool isPunishment) const
{
    if (!scriptParser) return assignmentName;
    const ScriptData &data = scriptParser->getScriptData();
    QString lowerName = assignmentName.toLower();

    if (isPunishment) {
        const PunishmentDefinition *ptrDef = getPunishmentDefinition(assignmentName);
        if (!ptrDef) return assignmentName;
        const PunishmentDefinition &def = *ptrDef;

        QString title = def.title.isEmpty() ? def.name : def.title;

        // 1. Replace # with amount
        if (title.contains('#')) {
            int amt = getPunishmentAmount(assignmentName);
            title.replace('#', QString::number(amt));
        }

        // 2. Handle Line Writing (Select=Random)
        if (def.isLineWriting && def.lineSelectMode.compare("Random", Qt::CaseInsensitive) == 0) {
             QSettings settings(settingsFile, QSettings::IniFormat);
             QString line = settings.value("Assignments/" + assignmentName + "_selected_line").toString();
             if (!line.isEmpty()) {
                 int amt = getPunishmentAmount(assignmentName);
                 title = QString("Write %1 times: %2").arg(amt).arg(line);
             }
        }

        if (!title.isEmpty()) title[0] = title[0].toUpper();
        return title;

    } else {
        // Job
        if (!data.jobs.contains(lowerName)) return assignmentName;
        const JobDefinition &def = data.jobs.value(lowerName);

        QString title = def.title.isEmpty() ? def.name : def.title;

        // Handle Line Writing (Select=Random)
        if (def.isLineWriting && def.lineSelectMode.compare("Random", Qt::CaseInsensitive) == 0) {
             QSettings settings(settingsFile, QSettings::IniFormat);
             QString line = settings.value("Assignments/" + assignmentName + "_selected_line").toString();
             if (!line.isEmpty()) {
                 int amt = def.lineCount > 0 ? def.lineCount : 1;
                 title = QString("Write %1 times: %2").arg(amt).arg(line);
             }
        }

        if (!title.isEmpty()) title[0] = title[0].toUpper();
        return title;
    }
}

void CyberDom::executeStringAction(ScriptActionType type, const QString &value)
{
    // Handle Drop$
    if (type == ScriptActionType::DropString) {
        QString varName = value.trimmed();
        if (varName.startsWith("$")) varName = varName.mid(1);

        scriptParser->removeVariable(varName);
        qDebug() << "[Drop$] Removed variable:" << varName;
        return;
    }

    // Format is usually: $VarName,Value or $VarName,Question
    // Split on the FIRST comma
    int commaIdx = value.indexOf(',');
    if (commaIdx == -1) return;

    QString varName = value.left(commaIdx).trimmed();
    QString content = value.mid(commaIdx + 1).trimmed();

    // Clean variable name (remove leading $)
    if (varName.startsWith("$")) varName = varName.mid(1);

    // Resolve variables in the content (Question or Text)
    QString resolvedContent = replaceVariables(content);

    if (type == ScriptActionType::SetString) {
        // Just set the value
        scriptParser->setVariable(varName, resolvedContent);
        qDebug() << "[Set$] Set" << varName << "to" << resolvedContent;
    }
    else if (type == ScriptActionType::InputString) {
        // Simple Input Dialog
        bool ok;
        QDateTime start = QDateTime::currentDateTime(); // Start Timer
        QString text = QInputDialog::getText(this, "Input Required",
                                           resolvedContent, QLineEdit::Normal, "", &ok);
        lastResponseDuration = start.secsTo(QDateTime::currentDateTime()); // End Timer
        if (ok) {
            scriptParser->setVariable(varName, text);
        }
    }
    else if (type == ScriptActionType::ChangeString) {
        // Input Dialog PRE-FILLED with current value
        QString currentValue = scriptParser->getVariable(varName);
        bool ok;
        QDateTime start = QDateTime::currentDateTime(); // Start Timer
        QString text = QInputDialog::getText(this, "Input Required",
                                           resolvedContent, QLineEdit::Normal, currentValue, &ok);
        lastResponseDuration = start.secsTo(QDateTime::currentDateTime()); // End Timer
        if (ok) {
            scriptParser->setVariable(varName, text);
        }
    }
    else if (type == ScriptActionType::InputLongString) {
        // Multiline Input Dialog
        bool ok;
        QDateTime start = QDateTime::currentDateTime(); // Start Timer
        QString text = QInputDialog::getMultiLineText(this, "Input Required",
                                                    resolvedContent, "", &ok);
        lastResponseDuration = start.secsTo(QDateTime::currentDateTime()); // End Timer
        if (ok) {
            scriptParser->setVariable(varName, text);
        }
    }
    else if (type == ScriptActionType::ChangeLongString) {
        // Multiline Input Dialog PRE-FILLED
        QString currentValue = scriptParser->getVariable(varName);
        bool ok;
        QDateTime start = QDateTime::currentDateTime(); // Start Timer
        QString text = QInputDialog::getMultiLineText(this, "Input Required",
                                                    resolvedContent, currentValue, &ok);
        lastResponseDuration = start.secsTo(QDateTime::currentDateTime()); // End Timer
        if (ok) {
            scriptParser->setVariable(varName, text);
        }
    }
}

void CyberDom::executeCounterAction(ScriptActionType type, const QString &value)
{
    if (!scriptParser) return;

    // Format: "#VarName,Value"
    int commaIdx = value.indexOf(',');
    if (commaIdx == -1) {
        qDebug() << "[Counters] Malformed counter action:" << value;
        return;
    }

    QString varName = value.left(commaIdx).trimmed();
    QString param = value.mid(commaIdx + 1).trimmed();

    if (varName.startsWith("#")) varName = varName.mid(1); // Strip #

    // Handle Input#
    if (type == ScriptActionType::InputCounter) {
        bool ok;
        // Min=0 ensures non-negative
        QDateTime start = QDateTime::currentDateTime(); // Start Timer
        int val = QInputDialog::getInt(this, "Input Required",
                                       replaceVariables(param),
                                       0, 0, 2147483647, 1, &ok);
        lastResponseDuration = start.secsTo(QDateTime::currentDateTime()); // End Timer
        if (ok) {
            scriptParser->setVariable(varName, QString::number(val));
            qDebug() << "[Input#] Set" << varName << "to" << val;
        }
        return;
    }

    // Handle Change#
    if (type == ScriptActionType::ChangeCounter) {
        // Get current value for default
        QString currentStr = scriptParser->getVariable(varName);
        int currentVal = currentStr.toInt(); // Defaults to 0 if empty/invalid

        bool ok;
        QDateTime start = QDateTime::currentDateTime(); // Start Timer
        int val = QInputDialog::getInt(this, "Input Required",
                                       replaceVariables(param),
                                       currentVal, 0, 2147483647, 1, &ok);
        lastResponseDuration = start.secsTo(QDateTime::currentDateTime()); // End Timer
        if (ok) {
            scriptParser->setVariable(varName, QString::number(val));
            qDebug() << "[Change#] Set" << varName << "to" << val;
        }
        return;
    }

    // Handle InputNeg#
    if (type == ScriptActionType::InputNegCounter) {
        bool ok;
        // Allow negative range
        QDateTime start = QDateTime::currentDateTime(); // Start Timer
        int val = QInputDialog::getInt(this, "Input Required",
                                       replaceVariables(param),
                                       0, -2147483647, 2147483647, 1, &ok);
        lastResponseDuration = start.secsTo(QDateTime::currentDateTime()); // End Timer
        if (ok) {
            scriptParser->setVariable(varName, QString::number(val));
            qDebug() << "[InputNeg#] Set" << varName << "to" << val;
        }
        return;
    }

    // Handle Drop#
    if (type == ScriptActionType::DropCounter) {
        QString varName = value.trimmed();
        if (varName.startsWith("#")) varName = varName.mid(1);

        scriptParser->removeVariable(varName);
        qDebug() << "[Drop#] Removed counter:" << varName;
        return;
    }

    // Resolve the value to apply (Right-hand side)
    int valToApply = ScriptUtils::resolveInt(param, [this](const QString &n){
        return scriptParser->getVariable(n);
    });

    // Get current value (Left-hand side) - needed for math ops
    int currentVal = 0;
    if (type != ScriptActionType::SetCounterVar) {
        // We construct a variable reference "#Name" to pass resolveInt
        currentVal = ScriptUtils::resolveInt("#" + varName, [this](const QString &n) {
            return scriptParser->getVariable(n);
        });
    }

    // Perform Operation
    int result = 0;
    switch (type) {
    case ScriptActionType::SetCounterVar:
        result = valToApply;
        break;
    case ScriptActionType::AddCounter:
        result = currentVal + valToApply;
        break;
    case ScriptActionType::SubtractCounter:
        result = currentVal - valToApply;
        break;
    case ScriptActionType::MultiplyCounter:
        result = currentVal * valToApply;
        break;
    case ScriptActionType::DivideCounter:
        if (valToApply == 0) {
            qDebug() << "[Counters] Division by zero prevented for:" << varName;
            result = currentVal;
        } else {
            result = currentVal / valToApply;
        }
        break;
    default: return;
    }

    // Save Result
    scriptParser->setVariable(varName, QString::number(result));
    qDebug() << "[Counters] Action on" << varName << "Result:" << result;
}

void CyberDom::executeRandomCounterAction(const QString &value)
{
    if (!scriptParser) return;

    QStringList parts = value.split(',', Qt::SkipEmptyParts);
    if (parts.size() < 2) {
        qDebug() << "[Random#] Malformed action:" << value;
        return;
    }

    QString varName = parts[0].trimmed();
    if (varName.startsWith("#")) varName = varName.mid(1);

    int min = 1;
    int max = 100;

    // Helper to resolve variables using ScriptUtils
    auto getVar = [this](const QString &n){ return scriptParser->getVariable(n); };

    if (parts.size() == 2) {
        // Format: random#=#counter,max (Min defaults to 1)
        max = ScriptUtils::resolveInt(parts[1], getVar);
    }
    else if (parts.size() >= 3) {
        // Format: random#=#counter,min,max
        min = ScriptUtils::resolveInt(parts[1], getVar);
        max = ScriptUtils::resolveInt(parts[2], getVar);
    }

    // Safety swap
    if (max < min) std::swap(min, max);

    // Generate
    int result = ScriptUtils::randomInRange(min, max, false);

    // Save
    scriptParser->setVariable(varName, QString::number(result));
    qDebug() << "[Random#] Set" << varName << "to" << result << "(Range:" << min << "-" << max << ")";
}

QString CyberDom::getVariableValue(const QString &name, int contextSeverity) const
{
    if (!scriptParser) return "0";

    // Check Predefined Counters
    if (name.compare("zzMerits", Qt::CaseInsensitive) == 0 ||
        name.compare("zzBeginMerits", Qt::CaseInsensitive) == 0 ||
        name.compare("zzMaxMerits", Qt::CaseInsensitive) == 0 ||
        name.compare("zzMinMerits", Qt::CaseInsensitive) == 0 ||
        name.compare("zzBlack", Qt::CaseInsensitive) == 0 ||
        name.compare("zzYellow", Qt::CaseInsensitive) == 0 ||
        name.compare("zzRed", Qt::CaseInsensitive) == 0 ||
        name.compare("zzNoassign", Qt::CaseInsensitive) == 0 ||
        name.compare("zzNoJob", Qt::CaseInsensitive) == 0 ||
        name.compare("zzNoPunish", Qt::CaseInsensitive) == 0 ||
        name.compare("zzNoAssignToday", Qt::CaseInsensitive) == 0 ||
        name.compare("zzNoJobToday", Qt::CaseInsensitive) == 0 ||
        name.compare("zzNoPunishToday", Qt::CaseInsensitive) == 0 ||
        name.contains("zzNoAssignWait", Qt::CaseInsensitive) == 0 ||
        name.contains("zzNoAssignStart", Qt::CaseInsensitive) == 0 ||
        name.contains("zzNoJobStart", Qt::CaseInsensitive) == 0 ||
        name.contains("zzNoPunishStart", Qt::CaseInsensitive) == 0 ||
        name.contains("zzNoAssignLate", Qt::CaseInsensitive) == 0 ||
        name.contains("zzNoJobLate", Qt::CaseInsensitive) == 0 ||
        name.contains("zzNoPunishLate", Qt::CaseInsensitive) == 0 ||
        name.contains("zzDay", Qt::CaseInsensitive) == 0 ||
        name.contains("zzMonth", Qt::CaseInsensitive) == 0 ||
        name.contains("zzYear", Qt::CaseInsensitive) == 0 ||
        name.contains("zzLeapYear", Qt::CaseInsensitive) == 0 ||
        name.contains("zzHour", Qt::CaseInsensitive) == 0 ||
        name.contains("zzMinute", Qt::CaseInsensitive) == 0 ||
        name.contains("zzSecond", Qt::CaseInsensitive) == 0 ||
        name.contains("zzSecondsPassed", Qt::CaseInsensitive) == 0 ||
        name.contains("zzDayOfWeek", Qt::CaseInsensitive) == 0 ||
        name.contains("zzClothFaults", Qt::CaseInsensitive) == 0 ||
        name.contains("zzPunishmentSeverity", Qt::CaseInsensitive) == 0 ||
        name.contains("zzLate", Qt::CaseInsensitive) == 0 ||
        name.contains("zzEarly", Qt::CaseInsensitive) == 0 ||
        name.contains("zzSeverity", Qt::CaseInsensitive) == 0) {

        const auto& general = scriptParser->getScriptData().general;
        const auto& jobs = scriptParser->getScriptData().jobs;

        int currentMerits = getMeritsFromIni();
        int startMerits = currentMerits - todayStats.meritsGained + todayStats.meritsLost;
        int maxMerits = getMaxMerits(); // Get the max value
        int minMerits = getMinMerits();
        int yellowMerits = general.yellowMerits;
        int redMerits = general.redMerits;
        int activeCount = activeAssignments.size();

        // Calculate Active Jobs
        int jobCount = 0;
        int punishmentCount = 0;
        int dueTodayCount = 0;
        int jobsDueTodayCount = 0;
        int punishmentsDueTodayCount = 0;
        int unstartedCount = 0;
        int unstartedJobCount = 0;
        int unstartedPunishCount = 0;
        int startedCount = 0;
        int startedJobCount = 0;
        int startedPunishCount = 0;
        int overdueCount = 0;
        int overdueJobCount = 0;
        int overduePunishCount = 0;

        QDate today = internalClock.date();
        int currentDay = today.day();
        int currentMonth = today.month();
        int currentYear = today.year();

        // Calculate Leap Year
        int isLeapYear = QDate::isLeapYear(today.year()) ? 1: 0;

        int currentHour = internalClock.time().hour();
        int currentMinute = internalClock.time().minute();
        int currentSecond = internalClock.time().second();

        // Calcuate Time Passed
        int secondsPassedToday = QTime(0, 0, 0).secsTo(internalClock.time());

        // Day of Week
        int currentDayOfWeek = today.dayOfWeek();

        for (const QString &assignment : activeAssignments) {
            QString lowerName = assignment.toLower();
            bool isJob = false;
            bool isPun = false;

            // Determine Type
            if (jobs.contains(lowerName)) {
                jobCount++;
                isJob = true;
            } else if (getPunishmentDefinition(assignment)) {
                punishmentCount++;
                isPun = true;
            }

            // Deadline Counting
            if (jobDeadlines.contains(assignment)) {
                QDateTime deadline = jobDeadlines.value(assignment);
                if (jobDeadlines.value(assignment).date() == today) {
                    dueTodayCount++;

                    if (isJob) {
                        jobsDueTodayCount++;
                    } else if (isPun) {
                        punishmentsDueTodayCount++;
                    }

                    if (internalClock > deadline) {
                        overdueCount++;
                    }
                }

                // Overdue Logic
                if (internalClock > deadline) {
                    overdueCount++;
                    if (isJob) overdueJobCount++;
                    else if (isPun) overduePunishCount++;
                }
            }

            // Check if Started
            QString flagName = (isPun ? "punishment_" : "job_") + assignment + "_started";
            if (!isFlagSet(flagName)) {
                unstartedCount++;
                if (isJob) unstartedJobCount++;
                else if (isPun) unstartedPunishCount++;
            } else {
                startedCount++;
                if (isJob) startedJobCount++;
                else if (isPun) startedPunishCount++;
            }
        }

        return QString::number(ScriptUtils::resolvePredefinedCounter(name,
                                                                     currentMerits,
                                                                     startMerits,
                                                                     maxMerits,
                                                                     minMerits,
                                                                     yellowMerits,
                                                                     redMerits,
                                                                     activeCount,
                                                                     jobCount,
                                                                     punishmentCount,
                                                                     dueTodayCount,
                                                                     jobsDueTodayCount,
                                                                     punishmentsDueTodayCount,
                                                                     unstartedCount,
                                                                     unstartedJobCount,
                                                                     unstartedPunishCount,
                                                                     startedCount,
                                                                     startedJobCount,
                                                                     startedPunishCount,
                                                                     overdueCount,
                                                                     overdueJobCount,
                                                                     overduePunishCount,
                                                                     currentDay,
                                                                     currentMonth,
                                                                     currentYear,
                                                                     isLeapYear,
                                                                     currentHour,
                                                                     currentMinute,
                                                                     currentSecond,
                                                                     secondsPassedToday,
                                                                     currentDayOfWeek,
                                                                     clothFaultsCount,
                                                                     lastPunishmentSeverity,
                                                                     lastLatenessMinutes,
                                                                     lastEarlyMinutes,
                                                                     contextSeverity));
    }

    // Check Custom Variables
    return scriptParser->getVariable(name);
}

void CyberDom::executeTimeAction(ScriptActionType type, const QString &value)
{
    if (!scriptParser) return;

    // Format: !VarName,Value
    int commaIdx = value.indexOf(',');
    if (commaIdx == -1) return;

    QString varName = value.left(commaIdx).trimmed();
    QString param = value.mid(commaIdx + 1).trimmed();

    if (varName.startsWith("!")) varName = varName.mid(1);

    // --- 1. HANDLE DATE INPUTS (InputDate, InputDateDef, ChangeDate) ---
    if (type == ScriptActionType::InputDate ||
        type == ScriptActionType::InputDateDef ||
        type == ScriptActionType::ChangeDate) {

        QString question;
        QDate defaultDate = internalClock.date();

        if (type == ScriptActionType::InputDateDef) {
            // Format: InitDate,Question
            int secondComma = param.indexOf(',');
            if (secondComma != -1) {
                QString initStr = param.left(secondComma).trimmed();
                question = replaceVariables(param.mid(secondComma + 1).trimmed());

                // Resolve InitDate
                if (initStr.startsWith("!")) {
                    QVariant val = scriptParser->getTimeVariable(initStr.mid(1));
                    if (val.userType() == QMetaType::QDateTime) {
                        defaultDate = val.toDateTime().date();
                    }
                } else {
                    QDateTime dt = ScriptUtils::parseDateString(replaceVariables(initStr));
                    if (dt.isValid()) defaultDate = dt.date();
                }
            } else {
                question = replaceVariables(param);
            }
        } else if (type == ScriptActionType::ChangeDate) {
            // Format: Question (Default from variable)
            question = replaceVariables(param);
            QVariant currentVal = getTimeVariableValue(varName);
            if (currentVal.userType() == QMetaType::QDateTime) {
                defaultDate = currentVal.toDateTime().date();
            }
        } else {
            // Standard InputDate
            question = replaceVariables(param);
        }

        // Show Dialog
        QDateTime start = QDateTime::currentDateTime();
        QDialog dlg(this);
        dlg.setWindowTitle(tr("Select Date"));
        QVBoxLayout *layout = new QVBoxLayout(&dlg);

        QLabel *lbl = new QLabel(question, &dlg);
        lbl->setWordWrap(true);
        layout->addWidget(lbl);

        QDateEdit *dateEdit = new QDateEdit(&dlg);
        dateEdit->setCalendarPopup(true);
        dateEdit->setDisplayFormat("yyyy-MM-dd");
        dateEdit->setDate(defaultDate);

        // Constraint: Cannot be lower than current internal date
        if (defaultDate < internalClock.date()) {
            dateEdit->setDate(internalClock.date());
        }
        dateEdit->setMinimumDate(internalClock.date());

        layout->addWidget(dateEdit);

        QDialogButtonBox *btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
        layout->addWidget(btns);

        connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

        if (dlg.exec() == QDialog::Accepted) {
            QDateTime dt(dateEdit->date(), QTime(0, 0, 0));
            scriptParser->setTimeVariable(varName, dt);
            qDebug() << "[InputDate] Set" << varName << "to" << dt.toString("yyyy-MM-dd");
        }
        lastResponseDuration = start.secsTo(QDateTime::currentDateTime());
        return;
    }

    // --- 2. HANDLE TIME INPUTS (InputTime, InputTimeDef, ChangeTime) ---
    if (type == ScriptActionType::InputTime ||
        type == ScriptActionType::InputTimeDef ||
        type == ScriptActionType::ChangeTime) {

        QString question;
        QTime defaultTime = QTime::currentTime();

        if (type == ScriptActionType::InputTimeDef) {
            int secondComma = param.indexOf(',');
            if (secondComma != -1) {
                QString initStr = param.left(secondComma).trimmed();
                question = replaceVariables(param.mid(secondComma + 1).trimmed());

                if (initStr.startsWith("!")) {
                    QVariant val = getTimeVariableValue(initStr.mid(1));
                    if (val.userType() == QMetaType::QTime) {
                        defaultTime = val.toTime();
                    } else if (val.userType() == QMetaType::QDateTime) {
                        defaultTime = val.toDateTime().time();
                    }
                } else {
                    QTime t = QTime::fromString(initStr, "HH:mm:ss");
                    if (!t.isValid()) t = QTime::fromString(initStr, "HH:mm");
                    if (t.isValid()) defaultTime = t;
                }
            } else {
                question = replaceVariables(param);
            }
        } else if (type == ScriptActionType::ChangeTime) {
            question = replaceVariables(param);
            QVariant currentVal = getTimeVariableValue(varName);
            if (currentVal.userType() == QMetaType::QTime) {
                defaultTime = currentVal.toTime();
            } else if (currentVal.userType() == QMetaType::QDateTime) {
                defaultTime = currentVal.toDateTime().time();
            }
        } else {
            question = replaceVariables(param);
        }

        // Show Dialog
        QDateTime start = QDateTime::currentDateTime();
        QDialog dlg(this);
        dlg.setWindowTitle(tr("Select Time"));
        QVBoxLayout *layout = new QVBoxLayout(&dlg);

        QLabel *lbl = new QLabel(question, &dlg);
        lbl->setWordWrap(true);
        layout->addWidget(lbl);

        QTimeEdit *timeEdit = new QTimeEdit(&dlg);
        timeEdit->setDisplayFormat("hh:mm AP");
        timeEdit->setTime(defaultTime);
        layout->addWidget(timeEdit);

        QDialogButtonBox *btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
        layout->addWidget(btns);

        connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

        if (dlg.exec() == QDialog::Accepted) {
            scriptParser->setTimeVariable(varName, timeEdit->time());
            qDebug() << "[InputTime] Set" << varName << "to" << timeEdit->time().toString();
        }
        lastResponseDuration = start.secsTo(QDateTime::currentDateTime());
        return;
    }

    // --- 3. HANDLE INTERVAL INPUTS (InputInterval, ChangeInterval) ---
    if (type == ScriptActionType::InputInterval ||
        type == ScriptActionType::ChangeInterval) {

        QString question = replaceVariables(param);
        QTime defaultTime(0, 0, 0);

        if (type == ScriptActionType::ChangeInterval) {
            QVariant currentVal = getTimeVariableValue(varName);
            if (currentVal.userType() == QMetaType::QTime) {
                defaultTime = currentVal.toTime();
            } else if (currentVal.canConvert<int>()) {
                int secs = currentVal.toInt();
                defaultTime = QTime(0, 0, 0).addSecs(secs);
            }
        }

        // Show Dialog
        QDateTime start = QDateTime::currentDateTime();
        QDialog dlg(this);
        dlg.setWindowTitle(tr("Enter Duration"));
        QVBoxLayout *layout = new QVBoxLayout(&dlg);

        QLabel *lbl = new QLabel(question, &dlg);
        lbl->setWordWrap(true);
        layout->addWidget(lbl);

        QTimeEdit *timeEdit = new QTimeEdit(&dlg);
        timeEdit->setDisplayFormat("HH:mm:ss");
        timeEdit->setTime(defaultTime);
        layout->addWidget(timeEdit);

        QDialogButtonBox *btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
        layout->addWidget(btns);

        connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

        if (dlg.exec() == QDialog::Accepted) {
            scriptParser->setTimeVariable(varName, timeEdit->time());
            qDebug() << "[InputInterval] Set" << varName << "to" << timeEdit->time().toString("HH:mm:ss");
        }
        lastResponseDuration = start.secsTo(QDateTime::currentDateTime());
        return;
    }

    // --- 4. HANDLE ADD/SUBTRACT DAYS ---
    if (type == ScriptActionType::AddDaysTime || type == ScriptActionType::SubtractDaysTime) {
        int days = ScriptUtils::resolveInt(param, [this](const QString &n) {
            return scriptParser->getVariable(n);
        });

        if (type == ScriptActionType::SubtractDaysTime) {
            days = -days;
        }

        QVariant currentVal = getTimeVariableValue(varName);
        QVariant newVal;

        if (currentVal.userType() == QMetaType::QDateTime) {
            newVal = currentVal.toDateTime().addDays(days);
            qDebug() << "[AddDays] Adjusted date" << varName << "by" << days << "days";
        } else if (currentVal.canConvert<int>()) {
            int currentSecs = currentVal.toInt();
            int adjustSecs = days * 86400;
            newVal = currentSecs + adjustSecs;
            qDebug() << "[AddDays] Adjusted duration" << varName << "by" << days << "days";
        } else {
            newVal = days * 86400;
        }

        scriptParser->setTimeVariable(varName, newVal);
        return;
    }

    // --- 5. HANDLE ROUNDING ---
    if (type == ScriptActionType::RoundTime) {
        int baseSeconds = 0;
        if (param.startsWith("!")) {
            QVariant val = getTimeVariableValue(param.mid(1));
            if (val.userType() == QMetaType::QTime) {
                baseSeconds = QTime(0, 0, 0).secsTo(val.toTime());
            } else if (val.canConvert<int>()) {
                baseSeconds = val.toInt();
            }
        } else {
            baseSeconds = ScriptUtils::parseDurationString(replaceVariables(param));
        }

        if (baseSeconds <= 0) return;

        QVariant currentVal = getTimeVariableValue(varName);
        QVariant newVal;

        if (currentVal.userType() == QMetaType::QDateTime) {
            qint64 epoch = currentVal.toDateTime().toSecsSinceEpoch();
            qint64 remainder = epoch % baseSeconds;
            if (remainder >= (baseSeconds / 2)) epoch += (baseSeconds - remainder);
            else epoch -= remainder;
            newVal = QDateTime::fromSecsSinceEpoch(epoch);
        } else if (currentVal.canConvert<int>()) {
            int currentSecs = currentVal.toInt();
            int remainder = currentSecs % baseSeconds;
            if (remainder >= (baseSeconds / 2)) currentSecs += (baseSeconds - remainder);
            else currentSecs -= remainder;
            newVal = currentSecs;
        } else if (currentVal.userType() == QMetaType::QTime) {
            int secs = QTime(0, 0, 0).secsTo(currentVal.toTime());
            int remainder = secs % baseSeconds;
            if (remainder >= (baseSeconds / 2)) secs += (baseSeconds - remainder);
            else secs -= remainder;
            newVal = QTime(0, 0, 0).addSecs(secs % 86400);
        }

        scriptParser->setTimeVariable(varName, newVal);
        qDebug() << "[Round] Rounded" << varName << "to nearest" << baseSeconds << "s";
        return;
    }

    // --- 6. HANDLE RANDOM TIME ---
    if (type == ScriptActionType::RandomTime) {
        QStringList parts = param.split(',', Qt::SkipEmptyParts);
        if (parts.size() < 2) return;

        auto resolveSeconds = [&](const QString &s) -> int {
            if (s.startsWith("!")) {
                QVariant v = getTimeVariableValue(s.mid(1));
                if (v.userType() == QMetaType::QTime) return QTime(0, 0, 0).secsTo(v.toTime());
                else if (v.canConvert<int>()) return v.toInt();
            }
            return ScriptUtils::parseDurationString(replaceVariables(s));
        };

        int minSecs = resolveSeconds(parts[0].trimmed());
        int maxSecs = resolveSeconds(parts[1].trimmed());
        if (maxSecs < minSecs) std::swap(minSecs, maxSecs);

        int resultSecs = ScriptUtils::randomInRange(minSecs, maxSecs, false);
        scriptParser->setTimeVariable(varName, QTime(0, 0, 0).addSecs(resultSecs));
        qDebug() << "[RandomTime] Set" << varName << "to random time";
        return;
    }

    // --- 7. HANDLE DROP TIME ---
    if (type == ScriptActionType::DropTime) {
        scriptParser->removeTimeVariable(varName);
        qDebug() << "[DropTime] Removed variable:" << varName;
        return;
    }

    // --- 8. HANDLE SET TIME VAR (Default Action) ---
    if (type == ScriptActionType::SetTimeVar) {
        QVariant valToSet;
        if (param.startsWith("!")) {
            valToSet = getTimeVariableValue(param.mid(1));
        } else {
            QString resolvedParam = replaceVariables(param);
            QDateTime dt = ScriptUtils::parseDateString(resolvedParam);
            if (dt.isValid()) {
                valToSet = dt;
            } else {
                valToSet = ScriptUtils::parseDurationString(resolvedParam);
            }
        }
        scriptParser->setTimeVariable(varName, valToSet);
        qDebug() << "[SetTime] Set" << varName << "to value";
    }
}

void CyberDom::executeTimeExtraction(ScriptActionType type, const QString &value)
{
    if (!scriptParser) return;

    // Format: #Counter,!TimeVar
    QStringList parts = value.split(',', Qt::SkipEmptyParts);
    if (parts.size() != 2) return;

    QString counterName = parts[0].trimmed();
    if (counterName.startsWith("#")) counterName = counterName.mid(1);

    QString timeVarName = parts[1].trimmed();
    if (timeVarName.startsWith("!")) timeVarName = timeVarName.mid(1);

    // Get the Time Variable
    QVariant val = getTimeVariableValue(timeVarName);
    int result = 0;

    // Handle Extract Days
    if (type == ScriptActionType::ExtractDays) {
        if (val.canConvert<int>() && val.userType() != QMetaType::QDateTime && val.userType() != QMetaType::QTime) {
            // It's a Duration (seconds). Return total days.
            // 86400 seconds in a day
            result = val.toInt() / 86400;
        }
        else if (val.userType() == QMetaType::QDateTime) {
            // It's a Date. Return Day of the Month (1-31).
            result = val.toDateTime().date().day();
        }
        // (If it's QTime, days is 0)
    }

    // Handle Extract Hours
    else if (type == ScriptActionType::ExtractHours) {
        if (val.canConvert<int>() && val.userType() != QMetaType::QDateTime && val.userType() != QMetaType::QTime) {
            // Duration: Total Hours (3600) seconds
            result = val.toInt() / 3600;
        }
        else if (val.userType() == QMetaType::QDateTime) {
            // Date: Hour of the Day (0-23)
            result = val.toDateTime().time().hour();
        }
        else if (val.userType() == QMetaType::QTime) {
            // Time: Hour of the Day (0-23)
            result = val.toTime().hour();
        }
    }

    // Handle Extract Minutes
    else if (type == ScriptActionType::ExtractMinutes) {
        if (val.canConvert<int>() && val.userType() != QMetaType::QDateTime && val.userType() != QMetaType::QTime) {
            // Duration: Total minutes (60 seconds)
            result = val.toInt() / 60;
        }
        else if (val.userType() == QMetaType::QDateTime) {
            // Date: Minute of the Hour (0-59)
            result = val.toDateTime().time().minute();
        }
        else if (val.userType() == QMetaType::QTime) {
            // Time: Minute of the Hour (0-59)
            result = val.toTime().minute();
        }
    }

    // Handle Extract Seconds
    else if (type == ScriptActionType::ExtractSeconds) {
        if (val.canConvert<int>() && val.userType() != QMetaType::QDateTime && val.userType() != QMetaType::QTime) {
            // Duration: Total seconds (Raw Value)
            result = val.toInt();
        }
        else if (val.userType() == QMetaType::QDateTime) {
            // Date: Second of the Minute (0-59)
            result = val.toDateTime().time().second();
        }
        else if (val.userType() == QMetaType::QTime) {
            // Time: Second of the Minute (0-59)
            result = val.toTime().second();
        }
    }

    // Save to Counter
    scriptParser->setVariable(counterName, QString::number(result));
    qDebug() << "[Extract] Extracted" << result << "into" << counterName;
}

void CyberDom::executeCounterToTime(ScriptActionType type, const QString &value)
{
    if (!scriptParser) return;

    // Format: !TimeVar,#Counter
    QStringList parts = value.split(',', Qt::SkipEmptyParts);
    if (parts.size() != 2) return;

    QString timeVarName = parts[0].trimmed();
    if (timeVarName.startsWith("!")) timeVarName = timeVarName.mid(1);

    QString counterParam = parts[1].trimmed();
    // (Note: resolveInt handles the # prefix inside the function)

    // Resolve the Counter/Number value
    // We use getVariableValue so we can support Predefined Counters
    int val = ScriptUtils::resolveInt(counterParam, [this](const QString &n){
        return getVariableValue(n);
    });

    // Convert to Seconds based on type
    int resultSeconds = 0;

    switch (type) {
        case ScriptActionType::ConvertDays:
            resultSeconds = val * 86400; // 24 * 60 * 60
            break;
        case ScriptActionType::ConvertHours:
            resultSeconds = val * 3600; // 60 * 60
            break;
        case ScriptActionType::ConvertMinutes:
            resultSeconds = val * 60;
            break;
        case ScriptActionType::ConvertSeconds:
            resultSeconds = val;
            break;
        default: return;
    }

    // Save to Time Variable (as Duration)
    scriptParser->setTimeVariable(timeVarName, resultSeconds);
    qDebug() << "[Convert!] Set" << timeVarName << "to" << resultSeconds << "seconds (from" << val << "units)";
}

QVariant CyberDom::getTimeVariableValue(const QString &name,
                                        const QString &contextMinTime,
                                        const QString &contextMaxTime,
                                        const QDateTime &contextStartTime,
                                        const QDateTime &contextCreationTime,
                                        const QDateTime &contextDeadline,
                                        const QDateTime &contextNextRemind,
                                        const QDateTime &contextNextAllow) const
{
    // Check Predefined Time Variables
    QVariant predefined = ScriptUtils::resolvePredefinedTime(name,
                                                            internalClock,
                                                            scriptBeginTime,
                                                            sessionStartTime,
                                                            lastAppCloseTime,
                                                            lastResponseDuration,
                                                            contextMinTime,
                                                            contextMaxTime,
                                                            contextStartTime,
                                                            contextCreationTime,
                                                            contextDeadline,
                                                            contextNextRemind,
                                                            lastLatenessMinutes,
                                                            lastEarlyMinutes,
                                                            contextNextAllow,
                                                            lastFlagDuration,
                                                            lastFlagExpiryTime);
    if (predefined.isValid()) {
        return predefined;
    }

    // Check Custom Time Variables (from Script)
    if (scriptParser) {
        return scriptParser->getTimeVariable(name);
    }

    return QVariant();
}
