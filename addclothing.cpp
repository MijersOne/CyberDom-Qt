#include "addclothing.h"
#include "ui_addclothing.h"

AddClothing::AddClothing(const QString &clothingType, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AddClothing)
{
    ui->setupUi(this);

    // Set up the table with 5 rows and 2 columns
    ui->clothingTable->setRowCount(5);
    ui->clothingTable->setColumnCount(2);
    ui->clothingTable->setHorizontalHeaderLabels(QStringList() << "Attribute" << "Value");

    QStringList attributes = {"Type", "Name", "Colour", "Style", "Description"};
    for (int row = 0; row < attributes.size(); ++row) {
        QTableWidgetItem *attrItem = new QTableWidgetItem(attributes[row]);
        attrItem->setFlags(attrItem->flags() & ~Qt::ItemIsEditable);  // Make attribute column read-only
        ui->clothingTable->setItem(row, 0, attrItem);

        QTableWidgetItem *valueItem = new QTableWidgetItem();
        if (attributes[row].toLower() == "type") {
            valueItem->setText(clothingType);
            valueItem->setFlags(valueItem->flags() & ~Qt::ItemIsEditable);  // Lock type cell too
        }
        ui->clothingTable->setItem(row, 1, valueItem);
    }

    ui->clothingTable->horizontalHeader()->setStretchLastSection(true);
    ui->clothingTable->verticalHeader()->setVisible(false);
}
