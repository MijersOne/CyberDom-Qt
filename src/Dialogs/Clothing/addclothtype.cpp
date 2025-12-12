#include "addclothtype.h"
#include "ui_addclothtype.h"
#include <QMessageBox>

AddClothType::AddClothType(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AddClothType)
{
    ui->setupUi(this);
    setWindowTitle("Add New Clothing Type");
    
    // Connect signals and slots
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &AddClothType::handleAccepted);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void AddClothType::handleAccepted()
{
    QString clothTypeName = ui->lineEdit->text().trimmed();
    
    if (clothTypeName.isEmpty()) {
        QMessageBox::warning(this, "Missing Information", "Please enter a name for the clothing type.");
        return;
    }
    
    emit clothTypeAdded(clothTypeName);
    accept();
}

AddClothType::~AddClothType()
{
    delete ui;
}
