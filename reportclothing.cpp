#include "reportclothing.h"
#include "ui_reportclothing.h"
#include "addclothtype.h" // Include the AddClothType header
#include "addclothing.h" // Include the AddClothing header

#include <QMessageBox> // Include for the QMessageBox

ReportClothing::ReportClothing(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ReportClothing)
{
    ui->setupUi(this);

    // Connect the AddType button to the slot function
    connect(ui->btn_AddType, &QPushButton::clicked, this, &ReportClothing::openAddClothTypeDialog);

    // Connect the AddItem button to the slot function
    connect(ui->btn_Add, &QPushButton::clicked, this, &ReportClothing::openAddClothingDialog);
}

void ReportClothing::openAddClothTypeDialog()
{
    AddClothType addclothtypeDialog(this); // Create the AddClothType dialog, passing the parent
    connect(&addclothtypeDialog, &AddClothType::clothTypeAdded, this, &ReportClothing::addClothTypeToComboBox);

    addclothtypeDialog.exec(); // Show the dialog modally
}

void ReportClothing::openAddClothingDialog()
{
    QString selectedType = ui->cb_Type->currentText(); // Get the selected type from the combo box
    if (selectedType.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select a clothing type.");
        return;
    }

    AddClothing addClothingDialog(this, selectedType); // Creates the AddClothing dialog, passing the parent
    connect(&addClothingDialog, &AddClothing::clothingItemAdded, this, &ReportClothing::addClothingItemToList);

    addClothingDialog.exec(); // Show the dialog modally
}

void ReportClothing::addClothingItemToList(const QString &itemName)
{
    ui->lst_Available->addItem(itemName); // Add the new item to the listbox
}

void ReportClothing::addClothTypeToComboBox(const QString &clothType)
{
    ui->cb_Type->addItem(clothType); // Add the new cloth type to the combo box
}

QString ReportClothing::getSelectedType() const
{
    QString selectedText = ui->cb_Type->currentText(); // Return the current text of the combo box
    return selectedText;
}

ReportClothing::~ReportClothing()
{
    delete ui;
}
