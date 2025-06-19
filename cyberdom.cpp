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
#include "askclothing.h" // Include the header for AskClothing UI
#include "askinstructions.h" // Include the header for AskInstructions UI
#include "reportclothing.h" // Include the header for ReportClothing UI
#include "rules.h" // Include the header for Rules UI
#include "listflags.h" // Include the header for the ListFlags UI
#include "setflags.h" // Include the header for the SetFlags UI
#include "deleteassignments.h" // Include the header for the DeleteAssignments UI
#include "askpunishment.h"
#include "clothingitem.h"
#include "questiondialog.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
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

CyberDom::CyberDom(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::CyberDom)
{

    ui->setupUi(this);

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

    // Load settings on startup
    //configManager.loadSettings();

    // Update UI with loaded settings

    // Initialize the .ini file
    initializeIniFile();


    // Load saved variables from .cds file
    QString cdsPath = currentIniFile;
    QRegularExpression rx("\\.ini$", QRegularExpression::CaseInsensitiveOption);
    cdsPath.replace(rx, ".cds");
    if (scriptParser->loadFromCDS(cdsPath)) {
        qDebug() << "[INFO] Loaded saved variables from" << cdsPath;
    } else {
        qDebug() << "[INFO] No .cds found (or failed to load) at" << cdsPath;
    }

    // Initialize the internal clock with the current system time
    QSettings settings(settingsFile, QSettings::IniFormat);
    QString savedDate = settings.value("System/CurrentDate", "").toString();
    QString savedTime = settings.value("System/CurrentTime", "").toString();

    if (savedDate.isEmpty()) {
        // First run - use system date and time
        internalClock = QDateTime::currentDateTime();
        settings.setValue("System/CurrentDate", internalClock.date().toString("MM-dd-yyyy"));
    } else {
        // Use saved date but current time
        QDate date = QDate::fromString(savedDate, "MM-dd-yyyy");
        QTime time = QTime::fromString(savedTime, "HH:MM");
        internalClock = QDateTime(date, time);
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
}

CyberDom::~CyberDom()
{
    // Save variables to .cds on exit
    if (scriptParser) {
        QString cdsPath = currentIniFile;
        cdsPath.replace(".ini", ".cds");
        saveVariablesToCDS(cdsPath);
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
                                 .addSecs(hours * 3000 + minutes * 60 + seconds);

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

    // Check if we crossed midnight (new day)
    if (previousTime.date() != internalClock.date()) {
        qDebug() << "Date changed from: " << previousTime.toString("MM-dd-yyyy") << "to: " << internalClock.toString("MM-dd-yyyy");

        // Save the current date to settings
        QSettings settings(settingsFile, QSettings::IniFormat);
        settings.setValue("System/CurrentDate", internalClock.date().toString("MM-dd-yyyy"));
    }

    for (TimerInstance &timer : activeTimers) {
        QTime now = internalClock.time();
        if (!timer.triggered && now >= timer.start && now <= timer.end) {
            qDebug() << "[Timer Triggered]:" << timer.name;

            if (!timer.message.isEmpty()) {
                QMessageBox::information(this, "Timer", replaceVariables(timer.message));
            }

            if (!timer.sound.isEmpty()) {
                auto player = new QMediaPlayer(this);
                auto audioOutput = new QAudioOutput(this);
                player->setAudioOutput(audioOutput);
                player->setSource(QUrl::fromLocalFile(timer.sound));
                player->play();
            }

            if (!timer.procedure.isEmpty()) {
                runProcedure(timer.procedure);
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

    QString version = getIniValue("General", "Version"); // Retrieve version
    About aboutDialog(this, currentIniFile, version); // Create the About dialog, passing the parent
    aboutDialog.exec(); // Show the dialog modally
}

void CyberDom::openAskClothingDialog()
{
    AskClothing askclothingDialog(this); // Create the AskClothing dialog, passing the parent
    askclothingDialog.exec(); // Show the dialog modally
}

void CyberDom::openAskInstructionsDialog()
{
    AskInstructions askinstructionsDialog(this); // Create the AskInstructions dialog, passing the parent
    askinstructionsDialog.exec(); // Show the dialog modally
}

void CyberDom::openReportClothingDialog()
{
    ReportClothing *reportClothingDialog = new ReportClothing(this);
    
    // Display the dialog
    if (reportClothingDialog->exec() == QDialog::Accepted) {
        // Process the clothing report
        processClothingReport(reportClothingDialog->getWearingItems(), reportClothingDialog->isNaked());
    }
    
    delete reportClothingDialog;
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
    // Fetch MinMerits, MaxMerits, and current Merits
    //int minMerits = settings.value("General/MinMerits", 0).toInt();
    int minMerits = getIniValue("General", "MinMerits").toInt();
    //int maxMerits = settings.value("General/MaxMerits", 100).toInt();
    int maxMerits = getIniValue("General", "MaxMerits").toInt();

    qDebug() << "Opening ChangeMerits Dialog with:";
    qDebug() << "  MinMerits:" << minMerits;
    qDebug() << "  MaxMerits:" << maxMerits;

    ChangeMerits changeMeritsDialog(this, minMerits, maxMerits); // Create the ChangeMerits dialog, passing the parent
    connect(&changeMeritsDialog, &ChangeMerits::meritsUpdated, this, &CyberDom::updateMerits);

    changeMeritsDialog.exec(); // Show the dialog modally

    qDebug() << "Opening ChangeMerits with MinMerits:" << minMerits << "MaxMerits:" << maxMerits;
}

void CyberDom::openChangeStatusDialog()
{
    qDebug() << "\n[DEBUG] Starting openChangeStatusDialog()";
    
    // Retrieve available statuses from INI file
    QStringList availableStatuses;

    // First try from iniData
    qDebug() << "[DEBUG] iniData sections: " << iniData.keys();
    for (const QString &section : iniData.keys()) {
        if (section.toLower().startsWith("status-")) {
            availableStatuses.append(section.mid(7)); // Extract the name after "Status-"
            qDebug() << "[DEBUG] Found status in iniData: " << section.mid(7);
        }
    }

    // If empty, try directly from scriptParser
    if (availableStatuses.isEmpty() && scriptParser) {
        QList<StatusSection> statuses = scriptParser->getStatusSections();
        qDebug() << "[DEBUG] ScriptParser has " << statuses.size() << " status sections";
        
        for (const StatusSection &status : statuses) {
            availableStatuses.append(status.name);
            qDebug() << "[DEBUG] Found status in ScriptParser: " << status.name;
        }
        qDebug() << "Retrieved " << availableStatuses.size() << " statuses directly from ScriptParser";
    } else if (!scriptParser) {
        qDebug() << "[ERROR] ScriptParser is NULL!";
    }

    // If still empty, try directly reading the script file
    if (availableStatuses.isEmpty() && !currentIniFile.isEmpty()) {
        qDebug() << "[DEBUG] No statuses found, trying direct file reading from: " << currentIniFile;
        QFile file(currentIniFile);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();
                if (line.startsWith("[") && line.endsWith("]")) {
                    QString sectionName = line.mid(1, line.length() - 2);
                    if (sectionName.toLower().startsWith("status-")) {
                        QString statusName = sectionName.mid(7);
                        if (!availableStatuses.contains(statusName)) {
                            availableStatuses.append(statusName);
                            qDebug() << "[DEBUG] Found status via direct file read: " << statusName;
                        }
                    }
                }
            }
            file.close();
        } else {
            qDebug() << "[ERROR] Could not open script file for direct reading: " << file.errorString();
        }
    }

    // If no statuses found, show error message
    if (availableStatuses.isEmpty()) {
        qDebug() << "[ERROR] No statuses found in the script";
        QMessageBox::critical(this, "Error", "No statuses could be loaded from the script.");
        return;
    }

    // Debugging to check all found statuses
    qDebug() << "Available Statuses Loaded: " << availableStatuses;

    ChangeStatus changeStatusDialog(this, currentStatus, availableStatuses); // Create the ChangeStatus dialog, passing the parent
    connect(&changeStatusDialog, &ChangeStatus::statusUpdated, this, &CyberDom::updateStatus);
    changeStatusDialog.exec(); // Show the dialog modally
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

void CyberDom::openSelectPunishmentDialog()
{
    // Debug: Check if punishments are available in iniData
    qDebug() << "\n[DEBUG] Opening SelectPunishment dialog with iniData containing " << iniData.size() << " sections";
    int punishmentCount = 0;
    for (auto it = iniData.begin(); it != iniData.end(); ++it) {
        if (it.key().toLower().startsWith("punishment-")) {
            punishmentCount++;
            qDebug() << "[DEBUG] Punishment found in iniData: " << it.key();
        }
    }
    qDebug() << "[DEBUG] Total punishment sections found in iniData: " << punishmentCount;
    
    // Create a new map to pass to the dialog
    QMap<QString, QMap<QString, QString>> dialogData;
    
    // First try to use any existing punishment sections from iniData
    if (punishmentCount > 0) {
        // Copy punishment sections from existing iniData
        for (auto it = iniData.begin(); it != iniData.end(); ++it) {
            if (it.key().toLower().startsWith("punishment-")) {
                dialogData[it.key()] = it.value();
            }
        }
    } else {
        // Direct approach: Read the script file and extract punishment sections
        qDebug() << "[DEBUG] No punishments found in iniData, reading directly from script file";
        
        // Try the current script file first
        QString scriptPath = currentIniFile;
        if (scriptPath.isEmpty() || !QFile::exists(scriptPath)) {
            // If no script is loaded, try the demo script
            scriptPath = QDir::currentPath() + "/scripts/demo-female.ini";
            qDebug() << "[DEBUG] Using demo script path: " << scriptPath;
        }
        
        QFile file(scriptPath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "[DEBUG] Successfully opened script file for direct reading";
            QTextStream in(&file);
            bool inPunishmentSection = false;
            QString currentSection;
            QMap<QString, QString> currentSectionData;
            
            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();
                
                // Skip empty lines and comments
                if (line.isEmpty() || line.startsWith(";")) {
                    continue;
                }
                
                // Check for section headers
                if (line.startsWith("[") && line.endsWith("]")) {
                    // Save previous section if it was a punishment section
                    if (inPunishmentSection && !currentSection.isEmpty()) {
                        dialogData[currentSection] = currentSectionData;
                        qDebug() << "[DEBUG] Added punishment section from file: " << currentSection;
                    }
                    
                    // Start new section
                    currentSection = line.mid(1, line.length() - 2);
                    currentSectionData.clear();
                    
                    // Check if this is a punishment section
                    inPunishmentSection = currentSection.toLower().startsWith("punishment-");
                    if (inPunishmentSection) {
                        qDebug() << "[DEBUG] Found punishment section in file: " << currentSection;
                    }
                }
                // Process key=value pairs inside punishment sections
                else if (inPunishmentSection && line.contains("=")) {
                    int equalsPos = line.indexOf('=');
                    QString key = line.left(equalsPos).trimmed();
                    QString value = line.mid(equalsPos + 1).trimmed();
                    currentSectionData[key] = value;
                }
            }
            
            // Save the last section if it was a punishment section
            if (inPunishmentSection && !currentSection.isEmpty()) {
                dialogData[currentSection] = currentSectionData;
                qDebug() << "[DEBUG] Added final punishment section from file: " << currentSection;
            }
            
            file.close();
            qDebug() << "[DEBUG] Finished direct reading, found " << dialogData.size() << " punishment sections";
        } else {
            qDebug() << "[ERROR] Could not open script file for direct reading: " << file.errorString();
        }
    }
    
    // Fallback if still no punishments found
    if (dialogData.isEmpty() && scriptParser) {
        qDebug() << "[DEBUG] Trying one more approach: Getting punishments from scriptParser";
        QList<PunishmentSection> punishments = scriptParser->getPunishmentSections();
        
        for (const PunishmentSection &punishment : punishments) {
            QString sectionName = "punishment-" + punishment.name;
            QMap<QString, QStringList> rawData = scriptParser->getRawSectionData(sectionName);
            QMap<QString, QString> punishmentData;
            for (auto it = rawData.begin(); it != rawData.end(); ++it) {
                if (!it.value().isEmpty()) {
                    punishmentData[it.key()] = it.value().first();
                }
            }

            dialogData[sectionName] = punishmentData;

            qDebug() << "[DEBUG] Added punishment from scriptParser: " << sectionName;
        }
    }
    
    // Create the SelectPunishment dialog with our curated data
    qDebug() << "[DEBUG] Opening SelectPunishment dialog with " << dialogData.size() << " punishment sections";
    SelectPunishment selectPunishmentDialog(this, dialogData);
    selectPunishmentDialog.exec(); // Show the dialog modally
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

    qDebug() << "\n[DEBUG] ============ INI FILE DEBUG ============";
    qDebug() << "[DEBUG] Initial iniFilePath from settings: " << iniFilePath;
    qDebug() << "[DEBUG] Fresh start detected: " << (freshStart ? "Yes" : "No");

    // If this is a fresh start or no ini file path is stored, prompt for a new file
    if (freshStart || iniFilePath.isEmpty()) {
        // If this is a fresh start, force selection of a new file
        if (freshStart) {
            iniFilePath = promptForIniFile();
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
}

void CyberDom::updateStatusText() {
    ui->textBrowser->clear();

    QString statusText = "";

    // Add top text if defined in [General] section
    if (iniData.contains("General") && iniData["General"].contains("TopText")) {
        statusText += iniData["General"]["TopText"] + "\n\n";
    }

    // Get text from the current status definition
    QString statusKey = "status-" + currentStatus;
    if (iniData.contains(statusKey)) {
        QMap<QString, QString> statusData = iniData[statusKey];

        // Collect all text= lines
        QStringList textLines;
        for (auto it = statusData.begin(); it != statusData.end(); ++it) {
            if (it.key().toLower() == "text") {
                textLines.append(it.value());
            }
        }

        if (!textLines.isEmpty()) {
            statusText += textLines.join("\n") + "\n\n";
        }
    }

    // Process special placeholders
    statusText.replace("%instructions", lastInstructions);
    statusText.replace("%clothing", lastClothingInstructions);

    // Add text from active flags
    QMapIterator<QString, FlagData> flagIter(flags);
    while (flagIter.hasNext()) {
        flagIter.next();
        const FlagData &flagData = flagIter.value();

        if (!flagData.text.isEmpty()) {
            statusText += flagData.text + "\n";
        } else {
            // Try to get text from the script if not stored in the flag data
            QString flagKey = "flag-" + flagIter.key();
            if (iniData.contains(flagKey)) {
                QMap<QString, QString> flagDefData = iniData[flagKey];
                if (flagDefData.contains("text")) {
                    statusText += flagDefData["text"] + "\n";
                }
            }
        }
    }

    // Add bottom text if defined
    if (iniData.contains("General") && iniData["General"].contains("BottomText")) {
        statusText += "\n" + iniData["General"]["BottomText"];
    }

    // Set the completed text in the UI
    ui->textBrowser->setText(statusText);
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
    // Create a new flag data structure
    FlagData flagData;
    flagData.name = flagName;
    flagData.setTime = internalClock;

    // Set expiry time if duration is specified
    if (durationMinutes > 0) {
        flagData.expiryTime = internalClock.addSecs(durationMinutes * 60);
    }

    // Add flag text if defined in the script
    QString flagKey = "flag-" + flagName;
    if (iniData.contains(flagKey)) {
        QMap<QString, QString> flagDefData = iniData[flagKey];
        if (flagDefData.contains("text")) {
            flagData.text = flagDefData["text"];
        }

        // Handle groups if defined
        if (flagDefData.contains("Group")) {
            flagData.groups = flagDefData["Group"].split(",");
        }
    }

    // Store the flag
    flags[flagName] = flagData;

    // Update UI
    updateStatusText();
    qDebug() << "Flag set: " << flagName;
}

void CyberDom::removeFlag(const QString &flagName) {
    if (flags.contains(flagName)) {
        flags.remove(flagName);
        updateStatusText(); // Update text as flag might have added text
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
    result.replace("{$zzMaster}", masterVariable);
    result.replace("{$zzSubName}", subNameVariable); // Add other variables as needed

    for (auto it = questionAnswers.begin(); it != questionAnswers.end(); ++it) {
        QString var = "{$" + it.key() + "}";
        result.replace(var, it.value());
    }

    return result;
}


void CyberDom::loadAndParseScript(const QString &filePath) {
    if (filePath.isEmpty()) {
        qDebug() << "[ERROR] No script file path provided.";
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
    // Apply general settings
    masterVariable = scriptParser->getMaster();
    subNameVariable = scriptParser->getSubName();
    minMerits = scriptParser->getMinMerits();
    maxMerits = scriptParser->getMaxMerits();
    testMenuEnabled = scriptParser->isTestMenuEnabled();

    // Initialize progress bar range
    initializeProgressBarRange();

    // Update the window title
    setWindowTitle(QString("%1's Virtual Master").arg(masterVariable));

    // Set visibility of Test Menu
    ui->menuTest->menuAction()->setVisible(testMenuEnabled);
    qDebug() << "TestMenu Enabled:" << testMenuEnabled;

    // Clear and rebuild the iniData map from the scriptParser data
    iniData.clear();
    
    // Add all raw sections to ensure no data is missed
    QStringList sectionNames = scriptParser->getRawSectionNames();
    qDebug() << "[DEBUG] Adding all raw sections to iniData. Total sections: " << sectionNames.size();
    
    for (const QString &sectionName : sectionNames) {
        QMap<QString, QStringList> rawData = scriptParser->getRawSectionData(sectionName);
        QMap<QString, QString> sectionData;
        
        for (auto it = rawData.begin(); it != rawData.end(); ++it) {
            if (!it.value().isEmpty()) {
                sectionData[it.key()] = it.value().first();
            }
        }
        
        iniData[sectionName] = sectionData;
        qDebug() << "[DEBUG] Added raw section to iniData: " << sectionName;
    }
    
    // Now add specific section types with proper formatting
    // Add general section
    QMap<QString, QString> generalData;
    QMap<QString, QStringList> generalRawData = scriptParser->getRawSectionData("general");
    for (auto it = generalRawData.begin(); it != generalRawData.end(); ++it) {
        if (!it.value().isEmpty()) {
            generalData[it.key()] = it.value().first();
        }
    }
    iniData["general"] = generalData;
    
    // Add job sections
    QList<JobSection> jobs = scriptParser->getJobSections();
    for (const JobSection &job : jobs) {
        QString sectionName = "job-" + job.name;
        QMap<QString, QStringList> rawData = scriptParser->getRawSectionData(sectionName);
        QMap<QString, QString> jobData;
        for (auto it = rawData.begin(); it != rawData.end(); ++it) {
            if (!it.value().isEmpty()) {
                jobData[it.key()] = it.value().first();
            }
        }

        iniData[sectionName] = jobData;

        qDebug() << "[DEBUG] Added job to iniData: " << sectionName;
    }
    
    // Add punishment sections
    QList<PunishmentSection> punishments = scriptParser->getPunishmentSections();
    for (const PunishmentSection &punishment : punishments) {
        QString sectionName = "punishment-" + punishment.name;
        QMap<QString, QStringList> rawData = scriptParser->getRawSectionData(sectionName);
        QMap<QString, QString> punishmentData;
        for (auto it = rawData.begin(); it != rawData.end(); ++it) {
            if (!it.value().isEmpty()) {
                punishmentData[it.key()] = it.value().first();
            }
        }

        iniData[sectionName] = punishmentData;

        qDebug() << "[DEBUG] Added punishment to iniData: " << sectionName;
    }

    // Add status sections
    QList<StatusSection> statuses = scriptParser->getStatusSections();
    qDebug() << "[DEBUG] Adding " << statuses.size() << " status sections to iniData";
    for (const StatusSection &status : statuses) {
        QString sectionName = "status-" + status.name;
        QMap<QString, QStringList> rawData = scriptParser->getRawSectionData(sectionName);
        QMap<QString, QString> statusData;
        for (auto it = rawData.begin(); it != rawData.end(); ++it) {
            if (!it.value().isEmpty()) {
                statusData[it.key()] = it.value().first();
            }
        }

        iniData[sectionName] = statusData;

        qDebug() << "[DEBUG] Added status to iniData: " << sectionName << " with " << statusData.size() << " key-value pairs";
    }

    // Add cloth type sections 
    QList<ClothTypeSection> clothTypes = scriptParser->getClothTypeSections();
    qDebug() << "[DEBUG] Adding " << clothTypes.size() << " cloth type sections to iniData";
    for (const ClothTypeSection &clothType : clothTypes) {
        QString sectionName = "clothtype-" + clothType.name;
        QMap<QString, QStringList> rawData = scriptParser->getRawSectionData(sectionName);
        QMap<QString, QString> clothTypeData;
        for (auto it = rawData.begin(); it != rawData.end(); ++it) {
            if (!it.value().isEmpty()) {
                clothTypeData[it.key()] = it.value().first();
            }
        }

        iniData[sectionName] = clothTypeData;

        qDebug() << "[DEBUG] Added cloth type to iniData: " << sectionName << " with " << clothTypeData.size() << " attributes";
    }

    // If we don't have any status sections, add a default one
    if (statuses.isEmpty()) {
        QMap<QString, QString> normalStatusData;
        normalStatusData["text"] = "This is a default status.";
        iniData["status-Normal"] = normalStatusData;
        qDebug() << "[WARNING] No status sections found, added a default 'Normal' status";
    }

    // Parse and load job assignments
    for (const JobSection &job : jobs) {
        // Add daily jobs to assignment list
        if (job.runDays.contains("daily", Qt::CaseInsensitive)) {
            assignedJobs.insert(job.name);
            qDebug() << "[DEBUG] Assigned Daily Job: " << job.name;
        }
    }

    // Update the ask punishment values
    QString askPunishmentStr = scriptParser->getIniValue("General", "AskPunishment");
    if (!askPunishmentStr.isEmpty()) {
        QStringList values = askPunishmentStr.split(",");
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

    // Build status groups from the parsed data
    statusGroups.clear();
    for (const StatusSection &status : scriptParser->getStatusSections()) {
        QString group = status.group.trimmed();
        if (group.isEmpty())
            continue;
        if (!statusGroups.contains(group)) {
            statusGroups[group] = QStringList();
        }
        statusGroups[group].append(status.name);
    }
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

void CyberDom::changeStatus(const QString &newStatus, bool isSubStatus) {
    // Validate the new status exists
    if (scriptParser->getStatus(newStatus).name.isEmpty()) {
        qDebug() << "[ERROR] Status: '" << newStatus << "' doesn't exist in the script.";
        return;
    }

    // If this is a substatus, save the current status for later return
    if (isSubStatus) {
        statusHistory.push(currentStatus);
    } else {
        // Clear status history when changing to a primary status
        statusHistory.clear();
    }

    // Update status
    QString oldStatus = currentStatus;
    currentStatus = newStatus;

    // Update UI elements based on new status
    updateStatusDisplay();
    updateAvailableActions();

    // Sae the status to settings
    QSettings settings(settingsFile, QSettings::IniFormat);
    settings.setValue("User/CurrentStatus", currentStatus);

    // Log the status change
    qDebug() << "Status changed from" << oldStatus << "to" << currentStatus;

    // Check for and execute any status-entry procedures
    executeStatusEntryProcedures(newStatus);
}

void CyberDom::returntoLastStatus() {
    if (!statusHistory.isEmpty()) {
        QString lastStatus = statusHistory.pop();
        changeStatus(lastStatus);
    } else {
        // If no history, maybe go to a default status from general section
        QString defaultStatus = scriptParser->getIniValue("General", "DefaultStatus");
        if (!defaultStatus.isEmpty()) {
            changeStatus(defaultStatus);
        } else {
            qDebug() << "No previous status to return to and no default status defined.";
        }
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
}

void CyberDom::updateAvailableActions() {
    // Get the current status data
    StatusSection status = scriptParser->getStatus(currentStatus);

    // Update UI based on status permissions
    ui->actionPermissions->setEnabled(status.permissionsEnabled);
    ui->actionAsk_for_Punishment->setEnabled(status.permissionsEnabled);
    ui->actionConfessions->setEnabled(status.confessionsEnabled);
    ui->actionReports->setEnabled(status.reportsEnabled);
    ui->menuRules->setEnabled(status.rulesEnabled);
    ui->menuAssignments->setEnabled(status.assignmentsEnabled);

    // If ReportsOnly is true, disable everything except reports
    if (status.reportsOnly) {
        ui->actionPermissions->setEnabled(false);
        ui->actionAsk_for_Punishment->setEnabled(false);
        ui->actionConfessions->setEnabled(false);
        ui->menuRules->setEnabled(false);
        ui->menuAssignments->setEnabled(false);
    }
}

void CyberDom::executeStatusEntryProcedures(const QString &statusName) {
    // This would execute any procedures that should run when entering a status
    // For now, this is a placeholder that will be implemented later
    qDebug() << "Executing entry procedures for status: " << statusName;
}

QString CyberDom::getIniValue(const QString &section, const QString &key, const QString &defaultValue) const {
    if (!iniData.contains(section)) {
        qDebug() << "Section not found in INI -" << section;
        return defaultValue;
    }

    if (!iniData[section].contains(key)) {
        qDebug() << "Key not found in section -" << section << "/" << key;
        return defaultValue;
    }

    QString value = iniData[section][key];
    return value;
}

int CyberDom::getMeritsFromIni() {
    QSettings settings(currentIniFile, QSettings::IniFormat);
    return getIniValue("Init", "Merits").toInt(); // Default to 0 if not found
}

void CyberDom::initializeUiWithIniFile() {
    bool isFirstRun = false;

    QSettings settings(settingsFile, QSettings::IniFormat);
    if (!settings.contains("User/Initialized")) {
        isFirstRun = true;
        settings.setValue("User/Initialized", true);
        settings.sync();
    }

    if (iniData.contains("init")) {
        int initialMerits = iniData["init"]["Merits"].toInt();
        currentStatus = iniData["init"]["NewStatus"];
        updateMerits(initialMerits);
        updateStatus(currentStatus);
    }

    if (isFirstRun) {
        QString firstRunProcedure = getIniValue("events", "FirstRun");
        if (!firstRunProcedure.isEmpty()) {
            qDebug() << "[FirstRun] Executing startup procedure:" << firstRunProcedure;
            runProcedure(firstRunProcedure);
        }
    }

    updateStatusText();
}

void CyberDom::initializeProgressBarRange() {
    bool ok = false;

    int maxMerits = getIniValue(QString("General"), QString("MaxMerits"), QString("1000")).toInt(&ok);
    if (!ok) maxMerits = 1000; // Fallback value if conversion fails

    int minMerits = getIniValue("General", "MinMerits", "0").toInt(&ok);
    if (!ok) minMerits = 0;

    // Set the progress bar values correctly
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
    
    if (scriptParser) {
        // Use the scriptParser to get the job sections
        QList<JobSection> jobs = scriptParser->getJobSections();
        for (const JobSection &job : jobs) {
            jobList.append(job.name);
        }
    } else {
        // Fallback to the old way if scriptParser is not available
        for (const QString &section : iniData.keys()) {
            if (section.startsWith("job-")) {
                jobList.append(section.mid(4)); // Remove "job-" prefix
            }
        }
    }

    qDebug() << "[DEBUG] Loaded Job List: " << jobList;
    return jobList;
}

void CyberDom::assignScheduledJobs() {
    QDateTime currentTime = QDateTime::currentDateTime();

    for (const QString &job : iniData.keys()) {
        if (!job.startsWith("job-")) continue;

        QMap<QString, QString> jobDetails = iniData[job];

        // Run Daily Jobs
        if (jobDetails.contains("Run") && jobDetails["Run"] == "Daily") {
            if (!activeAssignments.contains(job)) {
                activeAssignments.insert(job);
                qDebug() << "[DEBUG] Job Auto-Assigned (Daily Run): " << job;
            }
        }

        // Run Jobs at Specific Intervals
        if (jobDetails.contains("FirstInterval")) {
            QTime jobStartTime = QTime::fromString(jobDetails["FirstInterval"], "hh:mm");
            if (currentTime.time() >= jobStartTime && !activeAssignments.contains(job)) {
                activeAssignments.insert(job);
                qDebug() << "[DEBUG] Job Auto-Assigned (First Interval): " << job;
            }
        }
    }
}

void CyberDom::assignJobFromTrigger(QString section) {
    if (!iniData.contains(section)) return; // If section doesn't exist, return

    QMap<QString, QString> sectionData = iniData[section];

    if (sectionData.contains("Job")) {
        QString jobName = sectionData["Job"].trimmed();
        if (!activeAssignments.contains(jobName)) {
            activeAssignments.insert(jobName);


            // Extract EndTime if available
            if (iniData.contains("job-" + jobName)) {
                QMap<QString, QString> jobDetails = iniData["job-" + jobName];

                if (jobDetails.contains("EndTime")) {
                    // Extract EndTime (HH:MM format)
                    QString endTimeStr = jobDetails["EndTime"];
                    QTime endTime = QTime::fromString(endTimeStr, "HH:mm");

                    if (endTime.isValid()) {
                        // Create deadline datetime using current date initially
                        QDateTime deadline = QDateTime(internalClock.date(), endTime);

                        // If the deadline time has already passed today, move to tomorrow
                        if (deadline <= internalClock) {
                            deadline = deadline.addDays(1);
                        }

                        jobDeadlines[jobName] = deadline;
                        qDebug() << "[DEBUG] Job deadline set: " << jobName << " - " << deadline.toString("MM-dd-yyyy hh:mm AP");
                    }
                }
            }

            qDebug() << "[DEBUG] Assigned Job: " << jobName;
        }
    }
}

void CyberDom::addJobToAssignments(QString jobName)
{
    if (!activeAssignments.contains(jobName)) {
        activeAssignments.insert(jobName);
        qDebug() << "[DEBUG] Job added to active assignments: " << jobName;

        // Process the deadline based on EndTime
        if (iniData.contains("job-" + jobName)) {
            QMap<QString, QString> jobDetails = iniData["job-" + jobName];

            if (jobDetails.contains("EndTime")) {
                // Extract EndTime (HH:MM format)
                QString endTimeStr = jobDetails["EndTime"];
                QTime endTime = QTime::fromString(endTimeStr, "HH:mm");

                if (endTime.isValid()) {
                    // Create deadline datetime using the current date initially
                    QDateTime deadline = QDateTime(internalClock.date(), endTime);

                    // If the deadline time has already passed today, move to tomorrow
                    if (deadline <= internalClock) {
                        deadline = deadline.addDays(1);
                    }

                    jobDeadlines[jobName] = deadline;
                    qDebug() << "[DEBUG] Job deadline set: " << jobName << " - " << deadline.toString("MM-dd-yyyy hh:mm AP");
                } else {
                    qDebug() << "[DEBUG] Invalid EndTime format for job: " << jobName;
                }
            }
        }

        emit jobListUpdated();
        qDebug() << "[DEBUG] jobListUpdated Signal Emitted!";
    } else {
        qDebug() << "[DEBUG] Job already exists in active assignments: " << jobName;
    }
}

void CyberDom::checkPunishments() {
    QDateTime now = QDateTime::currentDateTime();

    // Check if any punishments have reached their deadline
    for (const QString &jobName : activeAssignments) {
        if (jobDeadlines.contains(jobName) && jobDeadlines[jobName] <= now) {
            // Handle completed punishments (e.g., remove from activeJobs, show a message)
            qDebug() << "Punishment completed: " << jobName;
            activeAssignments.remove(jobName);
            jobDeadlines.remove(jobName);
            emit jobListUpdated(); // Update the Assignments window
        }
    }
}

void CyberDom::addPunishmentToAssignments(QString punishmentName)
{
    if (!activeAssignments.contains(punishmentName)) {
        activeAssignments.insert(punishmentName);
        qDebug() << "[DEBUG] Punishment added to active assignments: " << punishmentName;

        // Process the deadline based on Respite or other timing parameters
        QString punishmentKey = "punishment-" + punishmentName;
        bool found = false;
        QMap<QString, QMap<QString, QString>>::const_iterator it;
        
        // Case-insensitive lookup for the punishment section
        for (it = iniData.constBegin(); it != iniData.constEnd(); ++it) {
            if (it.key().toLower() == punishmentKey.toLower()) {
                found = true;
                QMap<QString, QString> punishmentDetails = it.value();

                // Calculate deadline based on punishment parameters
                QDateTime deadline = internalClock; // Start with current time

                // Check if punishment has Respite value
                if (punishmentDetails.contains("Respite")) {
                    QString respiteStr = punishmentDetails["Respite"];
                    QStringList respiteParts = respiteStr.split(":");

                    if (respiteParts.size() >= 2) {
                        int hours = respiteParts[0].toInt();
                        int minutes = respiteParts[1].toInt();

                        deadline = deadline.addSecs(hours * 3600 + minutes * 60);
                        qDebug() << "[DEBUG] Punishment deadline set from Respite: " << deadline.toString("MM-dd-yyyy hh:mm AP");
                    }
                }
                // Check if punishment has a ValueUnit of time
                else if (punishmentDetails.contains("ValueUnit")) {
                    QString valueUnit = punishmentDetails["ValueUnit"];
                    int value = 0;

                    // Get the severity/value
                    if (punishmentDetails.contains("value")) {
                        value = punishmentDetails["value"].toInt();
                    }

                    if (valueUnit.toLower() == "minute") {
                        deadline = deadline.addSecs(value * 60);
                    } else if (valueUnit.toLower() == "hour") {
                        deadline = deadline.addSecs(value * 3600);
                    } else if (valueUnit.toLower() == "day") {
                        deadline = deadline.addDays(value);
                    }

                    qDebug() << "[DEBUG] Punishment deadline set from ValueUnit: " << deadline.toString("MM-dd-yyyy hh:mm AP");
                }

                jobDeadlines[punishmentName] = deadline;
                break;
            }
        }

        if (!found) {
            qDebug() << "[WARNING] Punishment section not found in iniData: " << punishmentKey;
        }

        emit jobListUpdated();
        qDebug() << "[DEBUG] jobListUpdated Signal Emitted!";
    } else {
        qDebug() << "[DEBUG] Punishment already exists in active assignments: " << punishmentName;
    }
}

void CyberDom::applyPunishment(int severity, const QString &group)
{
    // Choose a punishment based on severity and group
    QString punishmentName = selectPunishmentFromGroup(severity, group);

    if (!punishmentName.isEmpty()) {
        addPunishmentToAssignments(punishmentName);
    }
}

QString CyberDom::selectPunishmentFromGroup(int severity, const QString &group)
{
    QList<QString> eligiblePunishments;

    // Iterate through all punishments in iniData
    QMapIterator<QString, QMap<QString, QString>> i(iniData);
    while (i.hasNext()) {
        i.next();

        if (i.key().toLower().startsWith("punishment-")) {
            QString punishmentId = i.key();
            QString punishmentName = punishmentId.mid(11);
            QMap<QString, QString> punishmentData = i.value();

            // Check if punishment belongs to the specified group
            if (group.isEmpty() ||
                (punishmentData.contains("Group") && punishmentData["Group"].split(",").contains(group))) {

                // Check if punishment is suitable for the severity
                bool suitable = true;

                if (punishmentData.contains("MinSeverity")) {
                    int minSeverity = punishmentData["MinSeverity"].toInt();
                    if (severity < minSeverity) {
                        suitable = false;
                    }
                }

                if (punishmentData.contains("MaxSeverity")) {
                    int maxSeverity = punishmentData["MaxSeverity"].toInt();
                    if (severity > maxSeverity) {
                        suitable = false;
                    }
                }

                if (suitable) {
                    eligiblePunishments.append(punishmentName);
                }
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

    return QString();  // Returns an empty string if no suitable punishment is found.
}

void CyberDom::checkFlagExpiry() {
    QDateTime now = internalClock;
    QMutableMapIterator<QString, FlagData> i(flags);

    while (i.hasNext()) {
        i.next();
        if (i.value().expiryTime.isValid() && i.value().expiryTime <= now) {
            QString flagName = i.key();

            // Call any expire procedures defined in the script
            QString flagKey = "flag-" + flagName;
            if (iniData.contains(flagKey)) {
                QString expireProcedure = iniData[flagKey].value("ExpireProcedure");
                if (!expireProcedure.isEmpty()) {
                    // Call the procedure (needs to be implemented)
                }

                // Show expiry message if defined
                QString expireMessage = getIniValue("flag=" + flagName, "ExpireMessage");
                if (!expireMessage.isEmpty()) {
                    QMessageBox::information(this, "Flag Expired", expireMessage);
                }
            }

            // Remove the flag
            i.remove();
            updateStatusText(); // Update the UI if needed

            qDebug() << "Flag expired and removed: " << flagName;
        }
    }
}

void CyberDom::startAssignment(const QString &assignmentName, bool isPunishment, const QString &newStatus)
{
    // Remember the previous status if newStatus is specified
    QString previousStatus;
    if (!newStatus.isEmpty()) {
        previousStatus = currentStatus;
        updateStatus(newStatus);
    }

    // Set appropriate flags for the assignment
    QString sectionPrefix = isPunishment ? "punishment-" : "job-";
    QString sectionName = sectionPrefix + assignmentName;

    // Case-insensitive lookup for the section
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
        qDebug() << "[WARNING] Section not found for assignment: " << sectionName;
        return;
    }

    // Set a flag to track this assignment as started
    QString startFlagName = (isPunishment ? "punishment_" : "job_") + assignmentName + "_started";
    setFlag(startFlagName);

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
    if (details.contains("StartProcedure")) {
        // Placeholder for procedure handling system
        // executeProcedure(details["StartProcedure"]);
    }

    updateStatusText();
    emit jobListUpdated();
}

void CyberDom::markAssignmentDone(const QString &assignmentName, bool isPunishment)
{
    // Prefix for section lookup
    QString sectionPrefix = isPunishment ? "punishment-" : "job-";
    QString sectionName = sectionPrefix + assignmentName;

    // Case-insensitive lookup for the section
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
        qDebug() << "[WARNING] Section not found for completed assignment: " << sectionName;
        return;
    }

    // Remove from active assignments
    activeAssignments.remove(assignmentName);

    // Remove from deadlines
    jobDeadlines.remove(assignmentName);

    // Cleanup any flags
    QString startFlagName = (isPunishment ? "punishment_" : "job_") + assignmentName + "_started";
    removeFlag(startFlagName);

    // Return to previous status if needed
    QString statusFlagName = (isPunishment ? "punishment_" : "job_") + assignmentName + "_prev_status";
    QSettings settings(settingsFile, QSettings::IniFormat);
    QString prevStatus = settings.value("Assignments/" + statusFlagName, "").toString();

    if (!prevStatus.isEmpty()) {
        updateStatus(prevStatus);
        settings.remove("Assignments/" + statusFlagName);
    }

    // Add merit points if specified
    if (details.contains("AddMerit")) {
        bool ok;
        int meritPoints = details["AddMerit"].toInt(&ok);
        if (ok && meritPoints != 0) {
            // Add the merit points
            int currentMerits = settings.value("User/Merits", maxMerits / 2).toInt();
            currentMerits += meritPoints;

            // Ensure merits are within limits
            if (currentMerits > maxMerits) currentMerits = maxMerits;
            if (currentMerits < minMerits) currentMerits = minMerits;

            // Save the new merit value
            settings.setValue("User/Merits", currentMerits);

            updateMerits(currentMerits);
        }
    }

    // Execute DoneProcedure if specified
    if (details.contains("DoneProcedure")) {
        // Placeholder until procedure handling is implemented
        // executeProcedure(details["DoneProcedure"]);
    }

    // Update UI
    updateStatusText();
    emit jobListUpdated();
}

bool CyberDom::isFlagSet(const QString &flagName) const {
    return flags.contains(flagName);
}

QString CyberDom::getClothingReportPrompt() const
{
    // Get the clothing report prompt from the current status or script
    QString prompt = getIniValue("status-" + currentStatus, "ClothReport", "");
    
    if (prompt.isEmpty()) {
        // Default prompt
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
    
    // Find all clothing sets in the INI data
    QRegularExpression setRegex("^" + QRegularExpression::escape(setPrefix) + "-(.+)$");
    
    for (auto it = iniData.constBegin(); it != iniData.constEnd(); ++it) {
        QRegularExpressionMatch match = setRegex.match(it.key());
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

void CyberDom::runProcedure(const QString &procedureName) {
    QString sectionName = "procedure-" + procedureName;

    if (!scriptParser) {
        qDebug() << "[ERROR] ScriptParser not initialized.";
        return;
    }

    QMap<QString, QStringList> procedureData = scriptParser->getRawSectionData(sectionName);

    if (procedureData.isEmpty()) {
        qDebug() << "[WARNING] Procedure section not found or empty:" << sectionName;
        return;
    }

    qDebug() << "\n[DEBUG] Running procedure:" << procedureName;

    for (auto it = procedureData.begin(); it != procedureData.end(); ++it) {
        QString key = it.key().trimmed();
        QStringList values = it.value();

        for (const QString &rawValue : values) {
            QString value = rawValue.trimmed();
            qDebug() << "[DEBUG] Processing key:" << key << "with value:" << value;

            if (key.compare("SetFlag", Qt::CaseInsensitive) == 0) {
                setFlag(value);

            } else if (key.compare("RemoveFlag", Qt::CaseInsensitive) == 0) {
                removeFlag(value);

            } else if (key.compare("AddMerits", Qt::CaseInsensitive) == 0) {
                int currentMerits = getMeritsFromIni();
                int added = value.toInt();
                updateMerits(currentMerits + added);

            } else if (key.compare("Procedure", Qt::CaseInsensitive) == 0) {
                runProcedure(value);

            } else if (key.compare("Message", Qt::CaseInsensitive) == 0) {
                QMessageBox::information(this, "Message", replaceVariables(value));

            } else if (key.compare("Sound", Qt::CaseInsensitive) == 0) {
                auto player = new QMediaPlayer(this);
                auto audioOutput = new QAudioOutput(this);
                player->setAudioOutput(audioOutput);
                player->setSource(QUrl::fromLocalFile(value));
                player->play();

            } else if (key.compare("Input", Qt::CaseInsensitive) == 0) {
                bool ok;
                QString response = QInputDialog::getText(this, "Input Required",
                                                         replaceVariables(value),
                                                         QLineEdit::Normal, "", &ok);
                if (ok && !response.isEmpty()) {
                    qDebug() << "[Input Response]:" << response;
                }

            } else if (key.compare("Instructions", Qt::CaseInsensitive) == 0) {
                updateInstructions(value);

            } else if (key.compare("Clothing", Qt::CaseInsensitive) == 0) {
                updateClothingInstructions(value);

            } else if (key.compare("ShowPicture", Qt::CaseInsensitive) == 0) {
                // Placeholder — implement if desired
                qDebug() << "[Picture] Show:" << value;

            } else if (key.compare("RemovePicture", Qt::CaseInsensitive) == 0) {
                // Placeholder — implement if desired
                qDebug() << "[Picture] Remove:" << value;

            } else if (key.compare("If", Qt::CaseInsensitive) == 0) {
                QString condition = value;
                if (condition.startsWith("#")) {
                    QString flag = condition.mid(1).trimmed();
                    if (!isFlagSet(flag)) {
                        qDebug() << "[Procedure Skipped] Condition not met: " << flag;
                        continue;
                    }
                }

            } else if (key.compare("Question", Qt::CaseInsensitive) == 0) {
                // Get the question name from the value
                QString questionKey = value.trimmed();
                qDebug() << "[DEBUG] Found Question key: " << questionKey;

                // Retrieve the question data from the script parser
                QuestionSection questionData = scriptParser->getQuestion(questionKey);

                // Check if we found a valid question
                if (questionData.name.isEmpty() || questionData.phrase.isEmpty()) {
                    qDebug() << "[ERROR] Could not find question data for:" << questionKey;
                    continue;
                }

                // Apply variable replacement to question text
                questionData.phrase = replaceVariables(questionData.phrase);

                // Create and show the question dialog
                QuestionDialog dialog(questionData, this);
                dialog.setWindowTitle("Question");

                // Show the dialog and get result
                if (dialog.exec() == QDialog::Accepted) {
                    // Extract the selected answer using dialog.getAnswers()
                    QString selectedAnswer;
                    QMap<QString, QString> answers = dialog.getAnswers();
                    if (!questionData.variable.isEmpty()) {
                        selectedAnswer = answers.value(questionData.variable);
                    } else if (!answers.isEmpty()) {
                        selectedAnswer = answers.begin().value();
                    }
                    qDebug() << "[DEBUG] Question answered:" << selectedAnswer;

                    // Save the answer for later reference
                    questionAnswers[questionKey] = selectedAnswer;
                    saveQuestionAnswers();

                    // Check if the selected answer has an associated procedure
                    if (questionData.options.contains(selectedAnswer)) {
                        QString procedureName = questionData.options[selectedAnswer];

                        // Check if this is an inline procedure
                        if (procedureName == "*") {
                            qDebug() << "[DEBUG] Inline procedure selected - continuing";
                            // Continue - the procedure is inline with the question
                        } else {
                            // Call the associated procedure
                            qDebug() << "[DEBUG] Running selected procedure:" << procedureName;
                            runProcedure(procedureName);
                        }
                    }
                } else {
                    // User canceled - check if we have a NoInputProcedure
                    if (!questionData.noInputProcedure.isEmpty()) {
                        qDebug() << "[DEBUG] Running NoInputProcedure:" << questionData.noInputProcedure;
                        runProcedure(questionData.noInputProcedure);
                    }
                }

            } else if (key.startsWith("set#", Qt::CaseInsensitive)) {
                QString setValue = value.trimmed();
                QStringList parts = setValue.split(",", Qt::SkipEmptyParts);
                if (parts.size() == 2) {
                    QString varName = parts[0].trimmed();
                    QString varValue = parts[1].trimmed();
                    if (varName.startsWith("#")) {
                        varName = varName.mid(1);
                    }
                    scriptParser->setVariable(varName, varValue);
                    qDebug() << "[DEBUG] Variable set from procedure:" << varName << "=" << varValue;
                } else {
                    qDebug() << "[WARN] Malformed set# directive in procedure:" << value;
            }

            } else if (key.compare("Timer", Qt::CaseInsensitive) == 0) {
                QString timerKey = "timer-" + value;
                if (!iniData.contains(timerKey)) {
                    qDebug() << "[Timer] Timer section not found:" << timerKey;
                    continue;
                }

                QMap<QString, QString> timerData = iniData[timerKey];
                QTime startTime = QTime::fromString(timerData.value("Start", "00:00"), "hh:mm");
                QTime endTime = QTime::fromString(timerData.value("End", "23:59"), "hh:mm");
                QString message = timerData.value("Message");
                QString sound = timerData.value("Sound");
                QString procedure = timerData.value("Procedure");

                TimerInstance timer;
                timer.name = value;
                timer.start = startTime;
                timer.end = endTime;
                timer.message = message;
                timer.sound = sound;
                timer.procedure = procedure;
                timer.triggered = false;

                activeTimers.append(timer);
                qDebug() << "[Timer] Registered timer:" << value << " from "
                         << startTime.toString() << " to " << endTime.toString();

            } else {
                qDebug() << "[UNHANDLED] Procedure key:" << key << " -> " << value;
            }
        }
    }
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
    const auto &vars = scriptParser->getScriptData().stringVariables;
    for (auto it = vars.constBegin(); it != vars.constEnd(); ++it) {
        out << it.key() << "=" << it.value() << "\n";
    }

    file.close();
}
