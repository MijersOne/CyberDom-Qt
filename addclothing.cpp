#include "addclothing.h"
#include "ui_addclothing.h"

AddClothing::AddClothing(QWidget *parent, const QString &selectedType)
    : QDialog(parent)
    , ui(new Ui::AddClothing)
{
    ui->setupUi(this);

    // Set the label text to the selected type
    ui->lbl_SelectedType->setText(selectedType);
}

AddClothing::~AddClothing()
{
    delete ui;
}

void AddClothing::on_buttonBox_accepted()
{
    QString itemName = ui->le_Name->text(); // Get the entered item name from the input field
    if (!itemName.isEmpty()) {
        emit clothingItemAdded(itemName); // Emit the signal with the new item name
    }
    accept(); // Close the dialog
}
