#include "addclothing.h"
#include "ui_addclothing.h"

#include <QTableWidgetItem>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QLineEdit>

AddClothing::AddClothing(QWidget *parent, const QString &clothingType)
    : QDialog(parent)
    , ui(new Ui::AddClothing)
{
    ui->setupUi(this);
    isEditMode = false;
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &AddClothing::on_buttonBox_accepted);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    QList<ClothingAttribute> defaultAttrs;
    defaultAttrs.append(ClothingAttribute{QStringLiteral("Colour"), {}});
    defaultAttrs.append(ClothingAttribute{QStringLiteral("Style"), {}});
    defaultAttrs.append(ClothingAttribute{QStringLiteral("Description"), {}});
    setupTable(clothingType, defaultAttrs);
}

AddClothing::AddClothing(QWidget *parent, const QString &clothingType,
                         const QList<ClothingAttribute> &attributes)
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
                         const ClothingItem &item, const QList<ClothingAttribute> &attributes)
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

void AddClothing::setupTable(const QString &clothingType,
                             const QList<ClothingAttribute> &attributes,
                             const ClothingItem *existingItem)
{
    QStringList attrNames;
    for (const ClothingAttribute &attr : attributes)
        attrNames << attr.name;

    QStringList rows;
    rows << "Type" << "Name" << attrNames;

    ui->tw_addcloth->setRowCount(rows.size());
    ui->tw_addcloth->setColumnCount(2);
    ui->tw_addcloth->setHorizontalHeaderLabels(QStringList()
                                               << "Attribute" << "Value");

    for (int row = 0; row < rows.size(); ++row) {
        const QString &attr = rows[row];
        QTableWidgetItem *attrItem = new QTableWidgetItem(attr);
        attrItem->setFlags(attrItem->flags() & ~Qt::ItemIsEditable);
        ui->tw_addcloth->setItem(row, 0, attrItem);

        if (attr.compare("Type", Qt::CaseInsensitive) == 0) {
            QTableWidgetItem *valueItem = new QTableWidgetItem(clothingType);
            valueItem->setFlags(valueItem->flags() & ~Qt::ItemIsEditable);
            ui->tw_addcloth->setItem(row, 1, valueItem);
        } else if (attr.compare("Name", Qt::CaseInsensitive) == 0) {
            QTableWidgetItem *valueItem = new QTableWidgetItem();
            if (existingItem)
                valueItem->setText(existingItem->getName());
            ui->tw_addcloth->setItem(row, 1, valueItem);
        } else {
            const ClothingAttribute &attribute = attributes[row - 2];
            if (!attribute.values.isEmpty()) {
                QComboBox *combo = new QComboBox();
                combo->addItems(attribute.values);
                if (existingItem)
                    combo->setCurrentText(existingItem->getAttribute(attr));
                ui->tw_addcloth->setCellWidget(row, 1, combo);
            } else {
                QLineEdit *edit = new QLineEdit();
                if (existingItem)
                    edit->setText(existingItem->getAttribute(attr));
                ui->tw_addcloth->setCellWidget(row, 1, edit);
            }
        }
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
        if (!attrItem)
            continue;

        QString attrName = attrItem->text();
        QString attrValue;

        QWidget *w = ui->tw_addcloth->cellWidget(row, 1);
        if (QComboBox *combo = qobject_cast<QComboBox*>(w)) {
            attrValue = combo->currentText().trimmed();
        } else if (QLineEdit *edit = qobject_cast<QLineEdit*>(w)) {
            attrValue = edit->text().trimmed();
        } else if (QTableWidgetItem *valueItem = ui->tw_addcloth->item(row, 1)) {
            attrValue = valueItem->text().trimmed();
        }

        if (!attrValue.isEmpty())
            item.addAttribute(attrName, attrValue);
    }

    if (isEditMode)
        emit clothingItemEdited(item);
    else
        emit clothingItemAdded(item);

    accept();
}
