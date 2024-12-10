#include "cyberdom.h"
#include "ui_cyberdom.h"
#include "ui_assignments.h" // Include the header for Assignments UI
#include "timeadd.h" // Include the header for Time_Add UI
#include "about.h" // Include the header for About UI
#include "askclothing.h" // Include the header for AskClothing UI
#include "askinstructions.h" // Include the header for AskInstructions UI
#include "reportclothing.h" // Include the header for ReportClothing UI
#include "rules.h" // Include the header for Rules UI

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
    About aboutDialog(this); // Create the About dialog, passing the parent
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
