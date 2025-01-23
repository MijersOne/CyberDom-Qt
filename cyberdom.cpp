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
}

CyberDom::~CyberDom()
{
    delete ui;
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
    AskPunishment askPunishmentDialog(this); // Create the AskPunishment dialog, passing the parent
    askPunishmentDialog.exec(); // Show the dialog modally
}


void CyberDom::openChangeMeritsDialog()
{
    QSettings settings(currentIniFile, QSettings::IniFormat);

    // Fetch MinMerits, MaxMerits, and current Merits
    int minMerits = settings.value("General/MinMerits", 0).toInt();
    int maxMerits = settings.value("General/MaxMerits", 100).toInt();
    int currentMerits = ui->progressBar->value(); // Use the current value of progressBar

    qDebug() << "Opening ChangeMerits Dialog with:";
    qDebug() << "  MinMerits:" << minMerits;
    qDebug() << "  MaxMerits:" << maxMerits;
    qDebug() << "  CurrentMerits:" << currentMerits;

    ChangeMerits changeMeritsDialog(this, minMerits, maxMerits, currentMerits); // Create the ChangeMerits dialog, passing the parent
    connect(&changeMeritsDialog, &ChangeMerits::meritsUpdated, this, &CyberDom::updateMerits);

    changeMeritsDialog.exec(); // Show the dialog modally
}

void CyberDom::openChangeStatusDialog()
{
    ChangeStatus changeStatusDialog(this); // Create the ChangeStatus dialog, passing the parent
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
    QSettings settings("CyberDom", "Desire_Games"); // Replace with you app/company name
    settings.setValue("SelectedIniFile", filePath);
}

QString CyberDom::loadIniFilePath() {
    QSettings settings("CyberDom", "Desire_Games");
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
    qDebug() << "INI File Path set to:" << currentIniFile;

    parseIniFile(); // Parse and store .ini data

    // Apply the visibility of the Test Menu
    if (testMenuEnabled) {
        ui->menuTest->menuAction()->setVisible(true); // Show the Test Menu
        qDebug() << "Test Menu is visible.";
    } else {
        ui->menuTest->menuAction()->setVisible(false); // Hide the Test Menu
        qDebug() << "Test Menu is hidden.";
    }

    // Parse and load any include files
    parseIncludeFiles(currentIniFile);

    // Check if the file exists
    QFile file(iniFilePath);
    if (!file.exists()) {
        qDebug() << "Error: INI file does not exist at the specified path.";
    }

    QSettings settings(iniFilePath, QSettings::IniFormat);

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

    if (!settings.contains(key)) {
        qDebug() << "Key not found in INI File:" << key;
        return;
    }

    QString value = settings.value(key, "").toString();
    value = replaceVariables(value);

    qDebug() << "Processed value:" << value;
}

void CyberDom::parseIniFile() {
    QFile file(currentIniFile);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        qDebug() << "INI File Contents:";
        while (!in.atEnd()) {
            qDebug() << in.readLine();
        }
        file.close();
    }

    QSettings settings(currentIniFile, QSettings::IniFormat);
    iniData.clear(); // Clear existing data

    // Special case for [General]
    QStringList generalKeys = settings.childKeys(); // Global keys (outside any group)
    if (!generalKeys.isEmpty()) {
        QMap<QString, QString> generalData;
        for (const QString &key : generalKeys) {
            QString value = settings.value(key).toString();

            if(key == "AskPunishment") {
                // Parse AskPunishment value
                QStringList values = value.split(",");
                if (values.size() == 2) {
                    bool minOk, maxOk;
                    askPunishmentMin = values[0].trimmed().toInt(&minOk);
                    askPunishmentMax = values[1].trimmed().toInt(&maxOk);

                    if(!minOk || !maxOk) {
                        qDebug() << "Error: Non-numeric AskPunishment values found.";
                        askPunishmentMin = 0; // Fallback default
                        askPunishmentMax = 0; // Fallback default
                    }
                } else {
                    qDebug() << "Error: Invalid AskPunishment format. Expected two comma-separated values.";
                    askPunishmentMin = 0; // Default value
                    askPunishmentMax = 0; // Default value
                }
            }

            if (key == "TestMenu") {
                bool ok;
                int intValue = value.trimmed().toInt(&ok); // Explicit conversion to integer
                if (ok) {
                    testMenuEnabled = (intValue == 1);
                    qDebug() << "Parsed TestMenu as Integer:" << intValue << ", Enabled:" << testMenuEnabled;
                } else {
                    testMenuEnabled = false;
                    qDebug() << "Failed to parse TestMenu value as integer. Defaulting to false.";
                }
            }

            // Debug Output
            qDebug() << "Final TestMenu Enabled Value:" << testMenuEnabled;
            qDebug() << "MaxMerits:" << iniData["General"]["MaxMerits"];
            qDebug() << "MinMerits:" << iniData["General"]["MinMerits"];

            generalData.insert(key, value);
        }
        iniData.insert("General", generalData); // Insert [General] section manually
    }

    QStringList groups = settings.childGroups(); // Get all sections
    for (const QString &group : groups) {
        settings.beginGroup(group); // Enter the section
        QStringList keys = settings.childKeys(); // Get keys in the section
        qDebug() << "Section:" << group;
        for (const QString &key : keys) {
            qDebug() << "  Key:" << key << "Value:" << settings.value(key).toString();
        }

        QMap<QString, QString> keyValues;
        for (const QString &key : keys) {
            QString value = settings.value(key).toString();
            keyValues.insert(key, value); // Store key-value pairs
        }
        iniData.insert(group, keyValues); // Store the section
        settings.endGroup(); // Exit the section
    }

    // Debug: Print AskPunishment values
    qDebug() << "AskPunishment Min:" << askPunishmentMin << "Max:" << askPunishmentMax;

    // Debug: Print all data
    for (const QString &section : iniData.keys()) {
        qDebug() << "Section:" << section;
        for (const QString &key : iniData[section].keys()) {
            qDebug() << "  Key:" << key << "Value:" << iniData[section][key];
        }
    }
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

QString CyberDom::getIniValue(const QString &section, const QString &key) const {
    if (iniData.contains(section) && iniData[section].contains(key)) {
        return iniData[section][key];
    }
    return QString(); // Return empty string if not found
}

int CyberDom::getMeritsFromIni() {
    QSettings settings(currentIniFile, QSettings::IniFormat);
    return settings.value("Init/Merits", 0).toInt(); // Default to 0 if not found
}

void CyberDom::initializeUiWithIniFile() {
    if (iniData.contains("init")) {
        int initialMerits = iniData["init"]["Merits"].toInt();
        ui->progressBar->setValue("Merits: " + int initialMerits);
        ui->lbl_Merits->setText(QString::number(initialMerits));
        qDebug() << "Initial Merits:" << initialMerits;
    }
}

void CyberDom::initializeProgressBarRange() {
    int maxMerits = 100; // Default Value
    int minMerits = 0; // Default Value

    if (iniData.contains("General")) {
        int maxMerits = iniData["General"].value("MaxMerits", "100").toInt();
        int minMerits = iniData["General"].value("MinMerits", "0").toInt();

        qDebug() << "MaxMerits:" << maxMerits << "MinMerits:" << minMerits;
    }

    ui->progressBar->setMinimum(minMerits);
    ui->progressBar->setMaximum(maxMerits);

    qDebug() << "ProgressBar Range:" << minMerits << "to" << maxMerits;
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
