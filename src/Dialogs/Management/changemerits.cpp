#include "changemerits.h"
#include "ui_changemerits.h"
#include "cyberdom.h"

ChangeMerits::ChangeMerits(QWidget *parent, int minMerits, int maxMerits)
    : QDialog(parent)
    , ui(new Ui::ChangeMerits)
{
    ui->setupUi(this);

    // Set the range and current value for the spin box
    CyberDom *mainWindow = qobject_cast<CyberDom*>(parent);
    if (mainWindow) {
        minMerits = mainWindow->getMinMerits();
        maxMerits = mainWindow->getMaxMerits();
    }

    ui->spinBoxMerits->setMinimum(minMerits);
    ui->spinBoxMerits->setMaximum(maxMerits);

    qDebug() << "ChangeMerits initialized with Min:" << minMerits << "Max:" << maxMerits;

    // Connect the OK button to handle the value change
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        emit meritsUpdated(ui->spinBoxMerits->value());
        accept(); // Close the dialog
    });
}

ChangeMerits::~ChangeMerits()
{
    delete ui;
}
