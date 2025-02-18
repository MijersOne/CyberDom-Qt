#include "cyberdom.h"
#include "askpunishment.h" // Include the header for the AskPunishments UI
#include "changemerits.h" // Include the header for the ChangeMerits UI
#include "changestatus.h" // Include the header for the ChangeStatus UI
#include "joblaunch.h" // Include the header for the JobLaunch UI
#include "selectpopup.h" // Include the header for the SelectPopups UI
#include "selectpunishment.h" // Include the header for the SelectPunishments UI
#include "ui_cyberdom.h"
#include "ui_assignments.h" // Include the header for Assignments UI
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
    internalClock = QDateTime::currentDateTime();

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

    // Debugging values to confirm override
    qDebug() << "CyberDom initialized with Min Merits:" << minMerits << "Max Merits:" << maxMerits;
    qDebug() << "Final ProgressBar Min:" << ui->progressBar->minimum();
    qDebug() << "Final ProgressBar Max:" << ui->progressBar->maximum();
}

CyberDom::~CyberDom()
{
    delete ui;
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
        assignmentsWindow = new QMainWindow(this); // Create the window if not already created
        assignmentsUi.setupUi(assignmentsWindow); // Setup the Assignments UI
    }
    assignmentsWindow->show(); // Show the Assignments Window
    assignmentsWindow->raise(); // Bring the window to the front (optional)
    assignmentsWindow->activateWindow(); // Activate the window (optional)
}

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
    // Increment the internal clock by 1 second
    internalClock = internalClock.addSecs(1);

    // Update the QLabel to display the new time
    ui->clockLabel->setText(internalClock.toString("hh:mm:ss AP"));
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
    SelectPunishment selectPunishmentDialog(this); // Create the SelectPunishment dialog, passing the parent
    selectPunishmentDialog.exec(); // Show the dialog modally
}

void CyberDom::openSelectPopupDialog()
{
    SelectPopup selectPopupDialog(this); // Create the SelectPopup dialog, passing the parent
    selectPopupDialog.exec(); // Show the dialog modally
}

void CyberDom::openListFlagsDialog()
{
    ListFlags listFlagsDialog(this); // Create the ListFlags dialog, passing the parent
    listFlagsDialog.exec(); // Show the dialog modally
}

void CyberDom::openSetFlagsDialog()
{
    SetFlags setFlagsDialog(this); // Create the SetFlags dialog, passing the parent
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

    this->currentIniFile = iniFilePath; // Store the path for later use

    // Set up a separate settings file
    QFileInfo iniFileInfo(currentIniFile);
    settingsFile = iniFileInfo.absolutePath() + "/user_settings.ini";

    parseIniFile(); // Parse and store .ini data

    // Call parseAskPunishment() after parsing the INI
    parseAskPunishment();

    // Parse and load any include files
    parseIncludeFiles(currentIniFile);

    // Override UI values
    initializeProgressBarRange();

    // // Ensure Test Menu Visibility is set AFTER Parsing
    // ui->menuTest->menuAction()->setVisible(testMenuEnabled);
    // qDebug() << "[DEBUG] Final TestMenu Enabled Value: " << testMenuEnabled;
    // qDebug() << "[DEBUG] Test Menu Visibility Set to: " << ui->menuTest->menuAction()->isVisible();

    // Check if the file exists
    QFile file(iniFilePath);
    if (!file.exists()) {
        qDebug() << "Error: INI file does not exist at the specified path.";
    }

    QSettings settings(iniFilePath, QSettings::IniFormat);
    currentStatus = settings.value("User/CurrentStatus", iniData["General"].value("DefaultStatus", "Normal")).toString();
    qDebug() << "Loaded Default Status from INI or User Settings: " << currentStatus;

    // Load settings
    QString master = getIniValue("General", "Master");
    QString subName = getIniValue("General", "SubName");

    // Initialize the UI components
    initializeUiWithIniFile(); // Update lbl_Merits
    initializeProgressBarRange(); // Set progressBar min/max
    updateProgressBarValue(); // Set progressBar current value

    qDebug() << "INI File Path:" << iniFilePath;
    qDebug() << "Master Variable:" << master;
    qDebug() << "SubName Variable:" << subName;
}

void CyberDom::updateStatus(const QString &newStatus) {
    if (newStatus.isEmpty()) {
        qDebug() << "Warning: Attempted to set an empty status.";
        return;
    }

    currentStatus = newStatus;

    // Convert underscores to spaces
    QString formattedStatus = newStatus;
    formattedStatus.replace("_", " ");

    // Capitalize each word
    QStringList words = formattedStatus.split(" ");
    for (QString &word : words) {
        word[0] = word[0].toUpper(); // Capitalize first letter
    }
    formattedStatus = words.join(" ");

    // Update the label on the main window
    ui->lbl_Status->setText("Status: " + formattedStatus);

    // Save the status in the user settings file (not the script's INI)
    QSettings settings(settingsFile, QSettings::IniFormat);
    settings.setValue("User/CurrentStatus", currentStatus);

    qDebug() << "Updated Status Label: " << formattedStatus;
}

void CyberDom::resetApplication() {
    // Confirm reset with the user
    int response = QMessageBox::warning(this, "Reset Application",
        "Are you sure you want to reset the application? This will delete all saved settings and restart the application.",
                                        QMessageBox::Yes | QMessageBox::No);

    if (response == QMessageBox::Yes) {
        // Clear stored settings
        QSettings settings("CyberDom", "Desire_Games");
        settings.clear();

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

void CyberDom::parseIniFile() {
    if (currentIniFile.isEmpty()) {
        qDebug() << "Error: No INI file is selected.";
        return;
    }

    QFile file(currentIniFile);
    if (!file.exists()) {
        qDebug() << "Error: INI file does not exist ->" << currentIniFile;
        return;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Error: Failed to open INI file ->" << currentIniFile;
        return;
    }

    QTextStream in(&file);
    qDebug() << "INI File Contents:";
    while (!in.atEnd()) {
        qDebug() << in.readLine();
    }
    file.close();

    QSettings settings(currentIniFile, QSettings::IniFormat);
    iniData.clear(); // Clear previous data

    // --- [General] Section Handling ---
    settings.beginGroup("General");
    QStringList generalKeys = settings.childKeys();
    if (!generalKeys.isEmpty()) {
        QMap<QString, QString> generalData;
        for (const QString &key : generalKeys) {
            QString value = settings.value(key).toString();

            // Handle AskPunishment
            if (key == "AskPunishment") {
                QStringList values = value.split(",");
                if (values.size() == 2) {
                    bool minOk, maxOk;
                    askPunishmentMin = values[0].trimmed().toInt(&minOk);
                    askPunishmentMax = values[1].trimmed().toInt(&maxOk);

                    if (!minOk || !maxOk) {
                        qDebug() << "Error: Non-numeric AskPunishment values found. Using defaults (0,0).";
                        askPunishmentMin = 0;
                        askPunishmentMax = 0;
                    }
                } else {
                    qDebug() << "Error: Invalid AskPunishment format. Expected two comma-separated values. Using defaults (0,0).";
                    askPunishmentMin = 0;
                    askPunishmentMax = 0;
                }
            }

            qDebug() << "[Debug] Reached Line 562 - Entering Function";
            // Handle TestMenu
            if (key == "TestMenu") {
                bool ok;
                int intValue = value.trimmed().toInt(&ok);
                if (ok) {
                    testMenuEnabled = (intValue == 1);
                }
                qDebug() << "[DEBUG] Parsed TestMenu Value from INI: " << intValue;
                qDebug() << "[DEBUG] TestMenu Enabled Flag Set to: " << testMenuEnabled;
            }

            // Ensure Test Menu Visibility is set AFTER Parsing
            ui->menuTest->menuAction()->setVisible(testMenuEnabled);
            qDebug() << "[DEBUG] Final TestMenu Enabled Value: " << testMenuEnabled;
            qDebug() << "[DEBUG] Test Menu Visibility Set to: " << ui->menuTest->menuAction()->isVisible();
            qDebug() << "[DEBUG] Exiting Function at Line 57";
        }

        iniData.insert("General", generalData);
    } else {
        qDebug() << "Warning: [General] section missing in INI file.";
    }
    settings.endGroup();

    // --- Parse All Other Sections ---
    QStringList groups = settings.childGroups();
    for (const QString &group : groups) {
        if (group == "General") continue; // Already processed

        settings.beginGroup(group);
        QStringList keys = settings.childKeys();
        QMap<QString, QString> keyValues;

        qDebug() << "Processing Section: " << group;
        for (const QString &key : keys) {
            QString value = settings.value(key).toString();
            keyValues.insert(key, value);
            qDebug() << "  Key:" << key << "Value:" << value;
        }

        iniData.insert(group, keyValues);
        settings.endGroup();
    }

    // --- Validate and Log Results ---
    qDebug() << "Final TestMenu Enabled Value:" << testMenuEnabled;

    // Validate and print Merit values
    bool ok;
    int maxMerits = iniData["General"].value("MaxMerits", "1000").toInt(&ok);
    if (!ok) {
        qDebug() << "Error converting MaxMerits to int. Using default 1000.";
        maxMerits = 1000;
    }

    int minMerits = iniData["General"].value("MinMerits", "0").toInt(&ok);
    if (!ok) {
        qDebug() << "Error converting MinMerits to int. Using default 0.";
        minMerits = 0;
    }

    qDebug() << "Final Merit Ranges -> Min:" << minMerits << "Max:" << maxMerits;
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
