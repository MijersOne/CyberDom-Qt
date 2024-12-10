#include "timeadd.h"
#include "ui_timeadd.h"

TimeAdd::TimeAdd(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::TimeAdd)
{
    ui->setupUi(this);

    // Connect to the buttonBox
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &TimeAdd::onOkClicked);
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
