#include "timeadd.h"
#include "ui_timeadd.h"
#include <QPushButton>

TimeAdd::TimeAdd(QWidget *parent)
: QDialog(parent)
, ui(new Ui::TimeAdd)
{
    ui->setupUi(this);

    // --- Connect Day Buttons ---
    connect(ui->btn_1Day, &QPushButton::clicked, this, [this]() { addTime(1, 0, 0, 0); });
    connect(ui->btn_7Days, &QPushButton::clicked, this, [this]() { addTime(7, 0, 0, 0); });

    // --- Connect Hour Buttons ---
    connect(ui->btn_1Hour, &QPushButton::clicked, this, [this]() { addTime(0, 1, 0, 0); });
    connect(ui->btn_6Hours, &QPushButton::clicked, this, [this]() { addTime(0, 6, 0, 0); });
    connect(ui->btn_12Hours, &QPushButton::clicked, this, [this]() { addTime(0, 12, 0, 0); });

    // --- Connect Minute Buttons ---
    connect(ui->btn_5Mins, &QPushButton::clicked, this, [this]() { addTime(0, 0, 5, 0); });
    connect(ui->btn_15Mins, &QPushButton::clicked, this, [this]() { addTime(0, 0, 15, 0); });
    connect(ui->btn_30Mins, &QPushButton::clicked, this, [this]() { addTime(0, 0, 30, 0); });

    // --- Connect Second Buttons ---
    connect(ui->btn_30Secs, &QPushButton::clicked, this, [this]() { addTime(0, 0, 0, 30); });

    // --- Connect Button Box ---
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &TimeAdd::onOkClicked);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void TimeAdd::addTime(int days, int hours, int minutes, int seconds)
{
    // Get current values from SpinBoxes
    int currentDays = ui->lineEditDays->value();
    int currentHours = ui->lineEditHours->value();
    int currentMinutes = ui->lineEditMinutes->value();
    int currentSeconds = ui->lineEditSeconds->value();

    // Add the new time values
    currentDays += days;
    currentHours += hours;
    currentMinutes += minutes;
    currentSeconds += seconds;

    // Handle overflow logic (e.g. 65 seconds -> 1 minute 5 seconds)
    if (currentSeconds >= 60) {
        currentMinutes += currentSeconds / 60;
        currentSeconds = currentSeconds % 60;
    }

    if (currentMinutes >= 60) {
        currentHours += currentMinutes / 60;
        currentMinutes = currentMinutes % 60;
    }

    // Note: We generally don't roll hours into days automatically for this specific tool,
    // as users might want "36 hours" specifically, but if you want that, uncomment below:
    if (currentHours >= 24) {
        currentDays += currentHours / 24;
        currentHours = currentHours % 24;
    }

    // Update the SpinBoxes
    ui->lineEditDays->setValue(currentDays);
    ui->lineEditHours->setValue(currentHours);
    ui->lineEditMinutes->setValue(currentMinutes);
    ui->lineEditSeconds->setValue(currentSeconds);
}

void TimeAdd::onOkClicked()
{
    // Get final values
    int days = ui->lineEditDays->value();
    int hours = ui->lineEditHours->value();
    int minutes = ui->lineEditMinutes->value();
    int seconds = ui->lineEditSeconds->value();

    // Emit the signal to update the main clock
    emit timeAdded(days, hours, minutes, seconds);

    accept();
}

TimeAdd::~TimeAdd()
{
    delete ui;
}
