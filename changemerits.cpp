#include "changemerits.h"
#include "ui_changemerits.h"

ChangeMerits::ChangeMerits(QWidget *parent, int minMerits, int maxMerits, int currentMerits)
    : QDialog(parent)
    , ui(new Ui::ChangeMerits)
{
    ui->setupUi(this);

    // Set the range and current value for the spin box
    ui->spinBoxMerits->setMinimum(minMerits);
    ui->spinBoxMerits->setMaximum(maxMerits);
    ui->spinBoxMerits->setValue(currentMerits);

    // Connect the OK button to handle the value change
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        emit meritsUpdated(ui->spinBoxMerits->value());
        accept(); // Close the Dialog
    });
}

ChangeMerits::~ChangeMerits()
{
    delete ui;
}
