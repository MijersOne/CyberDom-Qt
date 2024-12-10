#include "addclothtype.h"
#include "ui_addclothtype.h"

AddClothType::AddClothType(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AddClothType)
{
    ui->setupUi(this);

    // Manually connect the accepted signal of buttonBox to the custom slot
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &AddClothType::handleAccepted);
}

AddClothType::~AddClothType()
{
    delete ui;
}

void AddClothType::handleAccepted()
{
    QString newClothType = ui->le_ClothType->text(); // Get the text
    if (!newClothType.isEmpty()) {
        emit clothTypeAdded(newClothType); // Emit the signal
    }
    accept(); // Close the dialog
}
