#include "cyberdom.h"
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

#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QProcess>
#include <QFile>
#include <QRandomGenerator>
#include <cstdlib>
#include <ctime>

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

    // Load the INI file and set minMerits and maxMerits
    loadIniFile();

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
}

CyberDom::~CyberDom()
{
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

void CyberDom::loadIniFile()
{
    QSettings settings("", QSettings::IniFormat);

    bool ok;
    minMerits = getIniValue("General", "MinMerits").toInt(&ok);
    if (!ok) {
        qDebug() << "Error converting MinMerits to int, using default value of 0";
        minMerits = 0; // Default value
    }

    maxMerits = getIniValue("General", "MaxMerits").toInt(&ok);
    if (!ok) {
        qDebug() << "Error converting MaxMerits to int, using default value of 100";
        maxMerits = 100; // Default value
    }
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
    ReportClothing reportclothingDialog(this); // Create the ReportClothing dialog, passing the parent
    reportclothingDialog.exec(); // Show the dialog modally
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
    QSettings settings(currentIniFile, QSettings::IniFormat);

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
    // Retrieve available statuses from INI file
    QStringList availableStatuses;

    // Loop through all sections and extract status names
    for (const QString &section : iniData.keys()) {
        if (section.toLower().startsWith("status-")) {
            availableStatuses.append(section.mid(7)); // Extract the name after "Status-"
        }
    }

    if (availableStatuses.isEmpty()) {
            QMessageBox::warning(this, "No Statuses Found", "No statuses are defined in the script.");
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
    JobLaunch launchJobDialog(this); // Create the SelectJob dialog, passing the parent
    launchJobDialog.exec(); // Show the dialog modally
}

void CyberDom::openSelectPunishmentDialog()
{
    SelectPunishment selectPunishmentDialog(this, iniData); // Create the SelectPunishment dialog, passing the parent
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
    QString filePath = QFileDialog::getOpenFileName(this,
                                                    tr("Select Script File"),
                                                    "",
                                                    tr("INI Files (*.ini);;All Files (*)"));

    if (filePath.isEmpty()) {
        QMessageBox::warning(this, "No File Selected", "You must select a valid .ini file to proceed.");
    }

    return filePath;
}

void CyberDom::saveIniFilePath(const QString &filePath) {
    QSettings settings("Desire_Games", "CyberDom"); // Replace with you app/company name
    settings.setValue("SelectedIniFile", filePath);
}

QString CyberDom::loadIniFilePath() {
    QSettings settings("Desire_Games", "CyberDom");
    return settings.value("SelectedIniFile", "").toString();
}

void CyberDom::initializeIniFile() {
    QString iniFilePath = loadIniFilePath();

    if (iniFilePath.isEmpty()) {
        iniFilePath = promptForIniFile();
        if (!iniFilePath.isEmpty()) {
            saveIniFilePath(iniFilePath);
        } else {
            qApp->exit(); // Exit if no file is selected
            return;
        }
    }

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
        // Clear stored settings
        QSettings settings("Desire_Games", "CyberDom");
        settings.clear();

        // Delete the physical User_Settings.ini file
        QFile settingsFileObj(settingsFile);
        if (settingsFileObj.exists()) {
            if (!settingsFileObj.remove()) {
                QMessageBox::warning(this, "Reset Warning", "Could not delete the settings file. The application will restart, but settings may persist.");
            }
        }

        // Notify the user
        QMessageBox::information(this, "Application Successfully Reset", "The application will now restart.");

        // Restart the application
        QCoreApplication::quit();
        QProcess::startDetached(QCoreApplication::applicationFilePath(), QStringList());
    }
}

QString CyberDom::replaceVariables(const QString &input) const {
    QString result = input;
    result.replace("{$zzMaster}", masterVariable);
    result.replace("{$zzSubName}", subNameVariable); // Add other variables as needed

    return result;
}

void CyberDom::processIniValue(const QString &key) {
    QSettings settings(currentIniFile, QSettings::IniFormat);

    QStringList globalKeys = settings.childKeys();
    if (!globalKeys.isEmpty()) {
        QMap<QString, QString> globalData;
        for (const QString &key : globalKeys) {
            globalData.insert(key, settings.value(key).toString());
        }
        iniData.insert("Global", globalData);
    }

    if (!settings.contains(key)) {
        qDebug() << "Key not found in INI File:" << key;
        return;
    }

    QString value = settings.value(key, "").toString();
    value = replaceVariables(value);

    qDebug() << "Processed value:" << value;
}

void CyberDom::loadAndParseScript(const QString &filePath) {
    if (filePath.isEmpty()) {
        qDebug() << "[ERROR] No script file path provided.";
        return;
    }

    // Create new script parser if needed
    if (!scriptParser) {
        scriptParser = new ScriptParser(this);
    }

    // Parse the script file
    if (!scriptParser->parseScript(filePath)) {
        qDebug() << "[ERROR] Failed to parse script file.";
        QMessageBox::critical(this, "[Script Error", "Failed to parse the script file. Please check that it's a valid script.");
        return;
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

    // Parse and load job assignments
    QList<JobSection> jobs = scriptParser->getJobSections();
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
        for (const QString &group : status.groups) {
            if (!statusGroups.contains(group)) {
                statusGroups[group] = QStringList ();
            }
            statusGroups[group].append(status.name);
        }
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

    // Update the current status
    currentStatus = savedStatus;

    // Update the UI
    updateStatusDisplay();
    updateAvailableActions();

    qDebug() << "Initial status set to: " << currentStatus;
}

void CyberDom::changeStatus(const QString &newStatus, bool isSubStatus) {
    // Validate the new status exists
    if (!scriptParser->getStatus(newStatus).name.isEmpty()) {
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
    ui->actionPermissions->setEnabled(status.permissionsAllowed);
    ui->actionAsk_for_Punishment->setEnabled(status.permissionsAllowed);
    ui->actionConfessions->setEnabled(status.confessionsAllowed);
    ui->actionReports->setEnabled(status.reportsAllowed);
    ui->menuRules->setEnabled(status.rulesAllowed);
    ui->menuAssignments->setEnabled(status.assignmentsAllowed);

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

void CyberDom::parseIncludeFiles(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Error: Unable to open file" << filePath;
        return;
    }

    QTextStream in(&file);
    QStringList includeFiles; // List to store included file names

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.startsWith("%include=")) {
            QString includedFile = line.section('=', 1).trimmed(); // Extract the file name
            includeFiles.append(includedFile);
        }
    }

    file.close();

    // Debug: List all included files
    for (const QString &includeFile : includeFiles) {
        qDebug() << "Including file:" << includeFile;
        loadIncludeFile(includeFile); // Load each included file
    }
}

void CyberDom::loadIncludeFile(const QString &fileName) {
    QString filePath = QFileInfo(currentIniFile).absolutePath() + "/" + fileName; // Build full path

    QFile file(filePath);
    if (!file.exists()) {
        qDebug() << "Error: Include file not found:" << filePath;
        return;
    }

    QSettings settings(filePath, QSettings::IniFormat);

    // Parse the file and add its contents to iniData
    QStringList groups = settings.childGroups();
    for (const QString &group : groups) {
        settings.beginGroup(group);
        QStringList keys = settings.childKeys();

        QMap<QString, QString> keyValues;
        for (const QString &key : keys) {
            QString value = settings.value(key).toString();
            keyValues.insert(key, value);
        }
        iniData.insert(group, keyValues);
        settings.endGroup();
    }

    qDebug() << "Loaded include file:" << filePath;
}

QString CyberDom::getIniValue(const QString &section, const QString &key, const QString &defaultValue) const {
    if (iniData.contains(section)) {
        qDebug() << "Warning: Section not found in INI -" << section;
        return defaultValue;
    }

    if (!iniData[section].contains(key)) {
        qDebug() << "Warning: Key not found in section -" << section << "/" << key;
        return defaultValue;
    }

    QString value = iniData[section][key];
    qDebug() << "getIniValue Retrieved -> Section: " << section << ", Key: " << key << ", Value: " << value;
    return value;
}

int CyberDom::getMeritsFromIni() {
    QSettings settings(currentIniFile, QSettings::IniFormat);
    return getIniValue("Init", "Merits").toInt(); // Default to 0 if not found
}

void CyberDom::initializeUiWithIniFile() {
    if (iniData.contains("init")) {
        int initialMerits = iniData["init"]["Merits"].toInt();
        ui->progressBar->setValue(initialMerits);
        ui->lbl_Merits->setText("Merits: " + QString::number(initialMerits));
        ui->lbl_Status->setText("Status: " + currentStatus);
        qDebug() << "Initial Merits:" << initialMerits;
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

void CyberDom::parseAskPunishment() {
    QString askPunishmentValue = getIniValue("General", "AskPunishment", "25,75");

    if (askPunishmentValue.isEmpty()) {
        qDebug() << "Error: AskPunishment key not found. Using default values.";
        askPunishmentMin = 25;
        askPunishmentMax = 75;
        return;
}

    qDebug() << "Raw AskPunishment Value from INI:" << askPunishmentValue;

    QStringList values = askPunishmentValue.split(",");
    if (values.size() != 2) {
        qDebug() << "Error: Invalid AskPunishment format. Expected two comma-separated values.";
        askPunishmentMin = 25;
        askPunishmentMax = 75;
        return;
    }

    bool okMin, okMax;
    int minVal = values[0].toInt(&okMin);
    int maxVal = values[1].toInt(&okMax);

    qDebug() << "Parsed AskPunishment Values -> Min:" << minVal << ", Max:" << maxVal;

    // If parsing failed, set defaults
    if (!okMin || !okMax) {
        qDebug() << "Error: Failed to convert AskPunishment values to integers.";
        askPunishmentMin = 25;
        askPunishmentMax = 75;
    } else {
        askPunishmentMin = minVal;
        askPunishmentMax = maxVal;
    }

    qDebug() << "Final AskPunishment Values -> Min:" << askPunishmentMin << ", Max:" << askPunishmentMax;
}

QStringList CyberDom::getAvailableJobs() {
    QStringList jobList;

    for (const QString &section : iniData.keys()) {
        if (section.startsWith("job-")) {
            jobList.append(section.mid(4)); // Remove "job-" prefix
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
        if (iniData.contains("punishment-" + punishmentName)) {
            QMap<QString, QString> punishmentDetails = iniData["punishment-" + punishmentName];

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

        if (i.key().startsWith("punishment-")) {
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

    if (!iniData.contains(sectionName)) {
        return;
    }

    QMap<QString, QString> details = iniData[sectionName];

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

    if (!iniData.contains(sectionName)) {
        return;
    }

    QMap<QString, QString> details = iniData[sectionName];

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

    // For repeating jobs, schedule the next occurrence
    if (!isPunishment && details.contains("Interval")) {
        // Placeholder until JobOccurence is implemented
        // scheduleNextJobOccurrence(assignmentName, details);
    }

    updateStatusText();
    emit jobListUpdated();
}

bool CyberDom::isFlagSet(const QString &flagName) const {
    return flags.contains(flagName);
}
