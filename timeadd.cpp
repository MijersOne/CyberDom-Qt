#include "timeadd.h"
#include "ui_timeadd.h"

#include <QMenu>
#include <QAction>
#include <QLayout>
#include <QFile>
#include <QIcon>
#include <QDebug>
#include <QSettings>
#include <QToolButton>

TimeAdd::TimeAdd(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::TimeAdd)
{
    ui->setupUi(this);

    // Create a QToolButton and set its popup mode
   // QToolButton* toolButton = new QToolButton(this);
   // toolButton->setObjectName("toolButtonTimeMenu");
   // toolButton->setIcon(QIcon(":/icons/menu_icon.png")); // Optional: Set an icon for the button
   // toolButton->setPopupMode(QToolButton::MenuButtonPopup);

    // Create a minsmenu and add actions to it
    QMenu* minsmenu = new QMenu(this);

    QAction* action5Mins = new QAction("Add 5 Mins", this);
    QAction* action15Mins = new QAction("Add 15 Mins", this);
    QAction* action30Mins = new QAction("Add 30 Mins", this);
    QAction* action45Mins = new QAction("Add 45 Mins", this);

    minsmenu->addAction(action5Mins);
    minsmenu->addAction(action15Mins);
    minsmenu->addAction(action30Mins);
    minsmenu->addAction(action45Mins);

    // Connect actions to slots
    connect(action5Mins, &QAction::triggered, this, [this]() { addTime(0, 0, 5, 0); });
    connect(action15Mins, &QAction::triggered, this, [this]() { addTime(0, 0, 15, 0); });
    connect(action30Mins, &QAction::triggered, this, [this]() { addTime(0, 0, 30, 0); });
    connect(action45Mins, &QAction::triggered, this, [this]() { addTime(0, 0, 45, 0); });

    ui->minsButton->setMenu(minsmenu);

    // Create a daysmenu and add actions to it
    QMenu* daysmenu = new QMenu(this);

    QAction* action3Days = new QAction("Add 3 Days", this);
    QAction* action5Days = new QAction("Add 5 Days", this);
    QAction* action7Days = new QAction("Add 7 Days", this);
    QAction* action10Days = new QAction("Add 10 Days", this);

    daysmenu->addAction(action3Days);
    daysmenu->addAction(action5Days);
    daysmenu->addAction(action7Days);
    daysmenu->addAction(action10Days);

    // Connect actions to slots
    connect(action3Days, &QAction::triggered, this, [this]() { addTime(3, 0, 0, 0); });
    connect(action5Days, &QAction::triggered, this, [this]() { addTime(5, 0, 0, 0); });
    connect(action7Days, &QAction::triggered, this, [this]() { addTime(7, 0, 0, 0); });
    connect(action10Days, &QAction::triggered, this, [this]() {addTime(10, 0, 0, 0); });

    ui->dayButton->setMenu(daysmenu);

    // Create a hoursmenu and add actions to it
    QMenu* hoursmenu = new QMenu(this);

    QAction* action3Hours = new QAction("Add 3 Hours", this);
    QAction* action6Hours = new QAction("Add 6 Hours", this);
    QAction* action8Hours = new QAction("Add 8 Hours", this);
    QAction* action12Hours = new QAction("Add 12 Hours", this);
    QAction* action16Hours = new QAction("Add 16 Hours", this);

    hoursmenu->addAction(action3Hours);
    hoursmenu->addAction(action6Hours);
    hoursmenu->addAction(action8Hours);
    hoursmenu->addAction(action12Hours);
    hoursmenu->addAction(action16Hours);

    // Connect actions to slots
    connect(action3Hours, &QAction::triggered, this, [this]() { addTime(0, 3, 0, 0); });
    connect(action6Hours, &QAction::triggered, this, [this]() { addTime(0, 6, 0, 0); });
    connect(action8Hours, &QAction::triggered, this, [this]() { addTime(0, 8, 0, 0); });
    connect(action12Hours, &QAction::triggered, this, [this]() { addTime(0, 12, 0, 0); });
    connect(action16Hours, &QAction::triggered, this, [this]() { addTime(0, 16, 0, 0); });

    ui->hoursButton->setMenu(hoursmenu);

    // Create a minsmenu and add actions to it
    QMenu* secsmenu = new QMenu(this);

    QAction* action5Secs = new QAction("Add 5 Seconds", this);
    QAction* action15Secs = new QAction("Add 15 Seconds", this);
    QAction* action30Secs = new QAction("Add 30 Seconds", this);
    QAction* action45Secs = new QAction("Add 45 Seconds", this);

    secsmenu->addAction(action5Secs);
    secsmenu->addAction(action15Secs);
    secsmenu->addAction(action30Secs);
    secsmenu->addAction(action45Secs);

    // Connect actions to slots
    connect(action5Secs, &QAction::triggered, this, [this]() { addTime(0, 0, 0, 5); });
    connect(action15Secs, &QAction::triggered, this, [this]() { addTime(0, 0, 0, 15); });
    connect(action30Secs, &QAction::triggered, this, [this]() { addTime(0, 0, 0, 30); });
    connect(action45Secs, &QAction::triggered, this, [this]() { addTime(0, 0, 0, 45); });

    ui->secsButton->setMenu(secsmenu);

    // Connect to the buttonBox
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &TimeAdd::onOkClicked);
}

void TimeAdd::addTime(int days, int hours, int minutes, int seconds)
{
    // Update the line edits by adding the specified time
    int currentDays = ui->lineEditDays->text().toInt();
    int currentHours = ui->lineEditHours->text().toInt();
    int currentMinutes = ui->lineEditMinutes->text().toInt();
    int currentSeconds = ui->lineEditSeconds->text().toInt();

    // Add the new time values
    currentDays += days;
    currentHours += hours;
    currentMinutes += minutes;
    currentSeconds += seconds;

    // Handle overflow for seconds and minutes
    if (currentSeconds >= 60) {
        currentMinutes += currentSeconds / 60;
        currentSeconds = currentSeconds % 60;
    }

    if (currentMinutes >= 60) {
        currentHours += currentMinutes / 60;
        currentMinutes = currentMinutes % 60;
    }

    // Set the updated values back to the line edits
    ui->lineEditDays->setValue(currentDays);
    ui->lineEditHours->setValue(currentHours);
    ui->lineEditMinutes->setValue(currentMinutes);
    ui->lineEditSeconds->setValue(currentSeconds);
}

void TimeAdd::onOkClicked()
{
    // Get values from input fields
    int days = ui->lineEditDays->text().toInt();
    int hours = ui->lineEditHours->text().toInt();
    int minutes = ui->lineEditMinutes->text().toInt();
    int seconds = ui->lineEditSeconds->text().toInt();

    // Emit the signal
    emit timeAdded(days, hours, minutes, seconds);

    // Close the dialog
    accept();
}

TimeAdd::~TimeAdd()
{
    delete ui;
}

