#include "addclothing.h"
#include "ui_addclothing.h"

AddClothing::AddClothing(const QString &clothingType, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AddClothing)
{
    ui->setupUi(this);

    // Set up the table with 5 rows and 2 columns
    // Set up the table widget which is named "tw_addcloth" in the UI file
    ui->tw_addcloth->setRowCount(5);
    ui->tw_addcloth->setColumnCount(2);
    ui->tw_addcloth->setHorizontalHeaderLabels(QStringList() << "Attribute" << "Value");

    QStringList attributes = {"Type", "Name", "Colour", "Style", "Description"};
    for (int row = 0; row < attributes.size(); ++row) {
        QTableWidgetItem *attrItem = new QTableWidgetItem(attributes[row]);
        attrItem->setFlags(attrItem->flags() & ~Qt::ItemIsEditable);  // Make attribute column read-only
        ui->tw_addcloth->setItem(row, 0, attrItem);

        QTableWidgetItem *valueItem = new QTableWidgetItem();
        if (attributes[row].toLower() == "type") {
            valueItem->setText(clothingType);
            valueItem->setFlags(valueItem->flags() & ~Qt::ItemIsEditable);  // Lock type cell too
        }
        ui->tw_addcloth->setItem(row, 1, valueItem);
    }

    ui->tw_addcloth->horizontalHeader()->setStretchLastSection(true);
    ui->tw_addcloth->verticalHeader()->setVisible(false);
}

AddClothing::AddClothing(QWidget *parent,
                         const QString &selectedType,
                         const QStringList &attributes)
    : AddClothing(selectedType, parent)
{
    Q_UNUSED(attributes);
}

AddClothing::AddClothing(QWidget *parent,
                         const QString &selectedType,
                         const ClothingItem &item,
                         const QStringList &attributes)
    : AddClothing(selectedType, parent)
{
    Q_UNUSED(item);
    Q_UNUSED(attributes);
}

AddClothing::~AddClothing()
{
    delete ui;
}
