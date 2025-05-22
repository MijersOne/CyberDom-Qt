#include "changestatus.h"
#include "ui_changestatus.h"

ChangeStatus::ChangeStatus(QWidget *parent, const QString &currentStatus, const QStringList &availableStatuses)
    : QDialog(parent)
    , ui(new Ui::ChangeStatus), currentStatus(currentStatus)
{
    ui->setupUi(this);

    // Populate the combobox with available statuses
    ui->statuscomboBox->addItems(availableStatuses);

    // Set the current status as selected
    int index = ui->statuscomboBox->findText(currentStatus);
    if (index != -1) {
        ui->statuscomboBox->setCurrentIndex(index);
    }
}

ChangeStatus::~ChangeStatus()
{
    delete ui;
}

void ChangeStatus::on_buttonBox_accepted()
{
    // Get selected status
    QString newStatus = ui->statuscomboBox->currentText();

    // Emit the statusUpdated signal
    emit statusUpdated(newStatus);

    accept(); // Close the dialog
}
