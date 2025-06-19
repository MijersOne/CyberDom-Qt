#include "addclothing.h"
#include "ui_addclothing.h"

#include <QTableWidgetItem>
#include <QMessageBox>
#include <QDialogButtonBox>

AddClothing::AddClothing(QWidget *parent, const QString &clothingType)
    : QDialog(parent)
    , ui(new Ui::AddClothing)
{
    ui->setupUi(this);
    isEditMode = false;
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &AddClothing::on_buttonBox_accepted);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    setupTable(clothingType, {"Colour", "Style", "Description"});
}

AddClothing::AddClothing(QWidget *parent, const QString &clothingType,
                         const QStringList &attributes)
    : QDialog(parent)
    , ui(new Ui::AddClothing)
{
    ui->setupUi(this);
    isEditMode = false;
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &AddClothing::on_buttonBox_accepted);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    setupTable(clothingType, attributes);
}

AddClothing::AddClothing(QWidget *parent, const QString &clothingType,
                         const ClothingItem &item, const QStringList &attributes)
    : QDialog(parent)
    , ui(new Ui::AddClothing)
{
    ui->setupUi(this);
    isEditMode = true;
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &AddClothing::on_buttonBox_accepted);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
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
    ui->tw_addcloth->setRowCount(5);
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

void AddClothing::onTypeChanged(int index)
{
    Q_UNUSED(index);
    // Placeholder for future type-based behaviour
}

void AddClothing::on_buttonBox_accepted()
{
    QString type = ui->tw_addcloth->item(0, 1)->text().trimmed();
    QString name = ui->tw_addcloth->item(1, 1)->text().trimmed();

    if (name.isEmpty()) {
        QMessageBox::warning(this, tr("Missing Name"),
                             tr("Please enter a name for the clothing item."));
        return;
    }

    ClothingItem item(name, type);
    for (int row = 2; row < ui->tw_addcloth->rowCount(); ++row) {
        QTableWidgetItem *attrItem = ui->tw_addcloth->item(row, 0);
        QTableWidgetItem *valueItem = ui->tw_addcloth->item(row, 1);
        if (!attrItem || !valueItem)
            continue;
        QString attrName = attrItem->text();
        QString attrValue = valueItem->text().trimmed();
        if (!attrValue.isEmpty())
            item.addAttribute(attrName, attrValue);
    }

    if (isEditMode)
        emit clothingItemEdited(item);
    else
        emit clothingItemAdded(item);

    accept();
}
