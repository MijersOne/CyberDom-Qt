#include "addclothing.h"
#include "ui_addclothing.h"

#include <QTableWidgetItem>

AddClothing::AddClothing(QWidget *parent, const QString &clothingType)
    : QDialog(parent)
    , ui(new Ui::AddClothing)
{
    ui->setupUi(this);
    setupTable(clothingType, {"Colour", "Style", "Description"});
}

AddClothing::AddClothing(QWidget *parent, const QString &clothingType,
                         const QStringList &attributes)
    : QDialog(parent)
    , ui(new Ui::AddClothing)
{
    ui->setupUi(this);
    setupTable(clothingType, attributes);
}

AddClothing::AddClothing(QWidget *parent, const QString &clothingType,
                         const ClothingItem &item, const QStringList &attributes)
    : QDialog(parent)
    , ui(new Ui::AddClothing)
{
    ui->setupUi(this);
    setupTable(clothingType, attributes, &item);
}

AddClothing::~AddClothing()
{
    delete ui;
}

void AddClothing::setupTable(const QString &clothingType, const QStringList &attributes,
                             const ClothingItem *existingItem)
{
    QStringList rows;
    rows << "Type" << "Name" << attributes;

    ui->tw_addcloth->setRowCount(rows.size());
    ui->tw_addcloth->setColumnCount(2);
    ui->tw_addcloth->setHorizontalHeaderLabels(QStringList()
                                               << "Attribute" << "Value");

    for (int row = 0; row < rows.size(); ++row) {
        const QString &attr = rows[row];
        QTableWidgetItem *attrItem = new QTableWidgetItem(attr);
        attrItem->setFlags(attrItem->flags() & ~Qt::ItemIsEditable);
        ui->tw_addcloth->setItem(row, 0, attrItem);

        QTableWidgetItem *valueItem = new QTableWidgetItem();
        if (attr.compare("Type", Qt::CaseInsensitive) == 0) {
            valueItem->setText(clothingType);
            valueItem->setFlags(valueItem->flags() & ~Qt::ItemIsEditable);
        } else if (existingItem) {
            if (attr.compare("Name", Qt::CaseInsensitive) == 0)
                valueItem->setText(existingItem->getName());
            else
                valueItem->setText(existingItem->getAttribute(attr));
        }
        ui->tw_addcloth->setItem(row, 1, valueItem);
    }

    ui->tw_addcloth->horizontalHeader()->setStretchLastSection(true);
    ui->tw_addcloth->verticalHeader()->setVisible(false);
}

