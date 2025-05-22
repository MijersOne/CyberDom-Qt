#include "addclothing.h"
#include "ui_addclothing.h"
#include "cyberdom.h"
#include "scriptparser.h"
#include <QMessageBox>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QDebug>

AddClothing::AddClothing(QWidget *parent, const QString &selectedType)
    : QDialog(parent)
    , ui(new Ui::AddClothing)
    , clothingType(selectedType)
    , isEditMode(false)
    , nameEdit(nullptr)
    , hasProvidedAttributes(false)
{
    ui->setupUi(this);
    initializeUI();
    loadAttributes();
}

AddClothing::AddClothing(QWidget *parent, const QString &selectedType, const QStringList &attributes)
    : QDialog(parent)
    , ui(new Ui::AddClothing)
    , clothingType(selectedType)
    , isEditMode(false)
    , nameEdit(nullptr)
    , providedAttributes(attributes)
    , hasProvidedAttributes(true)
{
    ui->setupUi(this);
    initializeUI();
    loadAttributes();
}

AddClothing::AddClothing(QWidget *parent, const QString &selectedType, const ClothingItem &item)
    : QDialog(parent)
    , ui(new Ui::AddClothing)
    , clothingType(selectedType)
    , isEditMode(true)
    , existingItem(item)
    , nameEdit(nullptr)
    , hasProvidedAttributes(false)
{
    ui->setupUi(this);
    initializeUI();
    
    // Set the name in nameEdit before loading attributes
    // This is important because loadAttributes will use this value
    nameEdit->setText(item.getName());
    
    loadAttributes();
    
    // Update title to reflect edit mode
    setWindowTitle("Edit Clothing Item - " + item.getName());
}

AddClothing::AddClothing(QWidget *parent, const QString &selectedType, const ClothingItem &item, const QStringList &attributes)
    : QDialog(parent)
    , ui(new Ui::AddClothing)
    , clothingType(selectedType)
    , isEditMode(true)
    , existingItem(item)
    , nameEdit(nullptr)
    , providedAttributes(attributes)
    , hasProvidedAttributes(true)
{
    ui->setupUi(this);
    initializeUI();
    
    // Set the name in nameEdit before loading attributes
    nameEdit->setText(item.getName());
    
    loadAttributes();
    
    // Update title to reflect edit mode
    setWindowTitle("Edit Clothing Item - " + item.getName());
}

void AddClothing::initializeUI()
{
    setWindowTitle(isEditMode ? "Edit Clothing Item" : "Add Clothing Item");
    
    // Create a label for the clothing type at the top
    QLabel *typeLabel = new QLabel("Clothing Type: " + clothingType, this);
    typeLabel->setAlignment(Qt::AlignCenter);
    typeLabel->setStyleSheet("font-weight: bold;");
    
    // Create a hidden line edit for name (we'll show it in the table instead)
    nameEdit = new QLineEdit(this);
    nameEdit->setVisible(false);
    
    // Insert the type label at the top of the layout
    QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(this->layout());
    if (layout) {
        layout->insertWidget(0, typeLabel);
    }
    
    // Set up table widget
    ui->tw_addcloth->setColumnCount(2);
    ui->tw_addcloth->setHorizontalHeaderLabels(QStringList() << "Attribute" << "Value");
    ui->tw_addcloth->horizontalHeader()->setStretchLastSection(true);
    ui->tw_addcloth->verticalHeader()->setVisible(false);
    
    // Connect button box signals
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &AddClothing::on_buttonBox_accepted);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void AddClothing::loadAttributes()
{
    ui->tw_addcloth->setRowCount(1);
    QTableWidgetItem *nameItem = new QTableWidgetItem("Name");
    nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
    ui->tw_addcloth->setItem(0, 0, nameItem);

    QTableWidgetItem *nameValueItem = new QTableWidgetItem(nameEdit->text());
    ui->tw_addcloth->setItem(0, 1, nameValueItem);

    connect(ui->tw_addcloth, &QTableWidget::cellChanged, this, [this](int row, int column) {
        if (row == 0 && column ==1) {
            QString newName = ui->tw_addcloth->item(row, column)->text();
            nameEdit->setText(newName);
        }
    });

    CyberDom *mainWindow = qobject_cast<CyberDom*>(parent());
    if (!mainWindow && parent() && parent()->parent()) {
        mainWindow = qobject_cast<CyberDom*>(parent()->parent());
    }

    QMap<QString, QStringList> attributesMap;
    if (mainWindow) {
        ScriptParser* parser = mainWindow->getScriptParser();
        if (parser) {
            QMap<QString, ClothTypeSection> clothMap = parser->getClothTypeSectionMap();
            if (clothMap.contains(clothingType)) {
                attributesMap = clothMap[clothingType].attributes;
            }
        }
    }

    if (attributesMap.isEmpty()) {
        qWarning() << "[WARNING] No attributes found for clothing type: " << clothingType;
    }

    attributesMap.insert("Type", QStringList());

    int attributeCount = attributesMap.size();
    ui->tw_addcloth->setRowCount(attributeCount + 1);

    int currentRow = 1;
    for (auto it = attributesMap.constBegin(); it != attributesMap.constEnd(); ++it) {
        QString attrName = it.key();
        QStringList values = it.value();

        QTableWidgetItem *attrItem = new QTableWidgetItem(attrName);
        attrItem->setFlags(attrItem->flags() & ~Qt::ItemIsEditable);
        ui->tw_addcloth->setItem(currentRow, 0, attrItem);

        if (!values.isEmpty()) {
            QComboBox *comboBox = new QComboBox(ui->tw_addcloth);
            comboBox->addItems(values);
            comboBox->setEditable(true);
            ui->tw_addcloth->setCellWidget(currentRow, 1, comboBox);

            if (isEditMode) {
                QString existingValue = existingItem.getAttribute(attrName);
                if (!existingValue.isEmpty()) {
                    comboBox->setCurrentText(existingValue);
                }
            }

            if (attrName.compare("Type", Qt::CaseInsensitive) == 0) {
                comboBox->setCurrentText(clothingType);
                comboBox->setEnabled(false);
            }
        } else {
            QLineEdit *lineEdit = new QLineEdit(ui->tw_addcloth);

            if (attrName.compare("Type", Qt::CaseInsensitive) == 0) {
                lineEdit->setText(clothingType);
                lineEdit->setReadOnly(true);
            } else if (isEditMode) {
                QString existingValue = existingItem.getAttribute(attrName);
                if (!existingValue.isEmpty()) {
                    lineEdit->setText(existingValue);
                }
            }

            ui->tw_addcloth->setCellWidget(currentRow, 1, lineEdit);
        }

        currentRow++;
    }

    ui->tw_addcloth->resizeColumnsToContents();
    ui->tw_addcloth->horizontalHeader()->setStretchLastSection(true);
}

void AddClothing::on_buttonBox_accepted()
{
    QString itemName;
    if (ui->tw_addcloth->item(0, 1)) {
        itemName = ui->tw_addcloth->item(0, 1)->text().trimmed();
    } else {
        itemName = nameEdit->text().trimmed();
    }

    if (itemName.isEmpty()) {
        QMessageBox::warning(this, "Missing Information", "Please enter a name for the clothing item.");
        return;
    }

    ClothingItem item;
    if (isEditMode) {
        item = existingItem;
    } else {
        item = ClothingItem(itemName, clothingType);
    }

    item.setName(itemName);

    for (int row = 1; row < ui->tw_addcloth->rowCount(); ++row) {
        QString attrName = ui->tw_addcloth->item(row, 0)->text();
        QString attrValue;
        QWidget *widget = ui->tw_addcloth->cellWidget(row, 1);
        if (QComboBox *comboBox = qobject_cast<QComboBox*>(widget)) {
            attrValue = comboBox->currentText();
        } else if (QLineEdit *lineEdit = qobject_cast<QLineEdit*>(widget)) {
            attrValue = lineEdit->text();
        } else if (ui->tw_addcloth->item(row, 1)) {
            attrValue = ui->tw_addcloth->item(row, 1)->text();
        }
        if (!attrValue.isEmpty()) {
            item.addAttribute(attrName, attrValue);
        }
    }

    if (isEditMode) {
        emit clothingItemEdited(item);
    } else {
        emit clothingItemAdded(item); // <-- Now using correct signal for ReportClothing
    }

    accept();
}

AddClothing::~AddClothing()
{
    delete ui;
}
