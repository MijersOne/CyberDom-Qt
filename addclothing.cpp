#include "addclothing.h"
#include "ui_addclothing.h"
#include "cyberdom.h"
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
    // Add "Name" attribute directly at the start
    ui->tw_addcloth->setRowCount(1);
    QTableWidgetItem *nameItem = new QTableWidgetItem("Name");
    nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
    ui->tw_addcloth->setItem(0, 0, nameItem);
    
    QTableWidgetItem *nameValueItem = new QTableWidgetItem(nameEdit->text());
    ui->tw_addcloth->setItem(0, 1, nameValueItem);
    
    // Update Name field when the table item changes
    connect(ui->tw_addcloth, &QTableWidget::cellChanged, this, [this](int row, int column) {
        if (row == 0 && column == 1) {
            QString newName = ui->tw_addcloth->item(row, column)->text();
            nameEdit->setText(newName);
        }
    });
    
    QStringList attributes;
    
    // Use provided attributes if available
    if (hasProvidedAttributes && !providedAttributes.isEmpty()) {
        qDebug() << "Using provided attributes:" << providedAttributes.join(", ");
        attributes = providedAttributes;
        
        // Make sure "Type" is included, as it's essential
        if (!attributes.contains("Type", Qt::CaseInsensitive)) {
            attributes.prepend("Type");
        }
    } else {
        // If no provided attributes, try to get from iniData
        // Get the main window to access script data
        CyberDom *mainWindow = qobject_cast<CyberDom*>(parent());
        if (!mainWindow && parent() && parent()->parent()) {
            mainWindow = qobject_cast<CyberDom*>(parent()->parent());
        }
        
        if (!mainWindow) {
            qWarning("[ERROR] Failed to get main window - cannot load attributes");
            
            // Add default attributes
            attributes = QStringList() << "Type" << "Colour" << "Style" << "Description";
        } else {
            // Get ini data from main window
            const QMap<QString, QMap<QString, QString>> iniData = mainWindow->getIniData();
            
            // Find the section for this clothing type
            QString sectionName = "clothtype-" + clothingType.toLower();
            qDebug() << "Looking for attributes in section:" << sectionName;
            
            if (iniData.contains(sectionName)) {
                // Extract attributes from the section
                QMap<QString, QString> sectionData = iniData[sectionName];
                
                qDebug() << "Section data contains" << sectionData.size() << "entries";
                for (auto it = sectionData.begin(); it != sectionData.end(); ++it) {
                    qDebug() << "Key:" << it.key() << "Value:" << it.value();
                    
                    // Look for keys that match the pattern "attr=X" (case insensitive)
                    QString key = it.key();
                    if (key.startsWith("attr=", Qt::CaseInsensitive)) {
                        QString attrName = it.key().mid(5);  // Remove "attr=" prefix
                        if (!attributes.contains(attrName, Qt::CaseInsensitive)) {
                            attributes.append(attrName);
                            qDebug() << "Found attribute:" << attrName;
                        }
                    }
                }
                
                // If we still don't have attributes, try alternative format
                if (attributes.isEmpty()) {
                    for (auto it = sectionData.begin(); it != sectionData.end(); ++it) {
                        QString key = it.key().toLower();
                        QString value = it.value();
                        
                        if (key == "attr") {
                            if (!attributes.contains(value, Qt::CaseInsensitive)) {
                                attributes.append(value);
                                qDebug() << "Found attribute (alt format):" << value;
                            }
                        }
                    }
                }
            } else {
                // Section not found, use default attributes
                qWarning() << "[ERROR] Clothing type section not found:" << sectionName;
                attributes = QStringList() << "Type" << "Colour" << "Style" << "Description";
            }
        }
    }
    
    // If still empty after all attempts, use default attributes
    if (attributes.isEmpty()) {
        attributes = QStringList() << "Type" << "Colour" << "Style" << "Description";
    }
    
    // Ensure Type is always first in the list (after Name)
    if (attributes.contains("Type", Qt::CaseInsensitive)) {
        attributes.removeAll("Type");
        attributes.prepend("Type");
    } else {
        attributes.prepend("Type");
    }
    
    // Populate the table widget with attributes (first row is already Name)
    ui->tw_addcloth->setRowCount(attributes.size() + 1);
    
    // Add attributes to table
    for (int i = 0; i < attributes.size(); i++) {
        int row = i + 1; // Start at row 1 (after Name)
        
        // Create attribute name item
        QTableWidgetItem *attrItem = new QTableWidgetItem(attributes[i]);
        attrItem->setFlags(attrItem->flags() & ~Qt::ItemIsEditable); // Make it read-only
        ui->tw_addcloth->setItem(row, 0, attrItem);
        
        // Create value widget as line edit
        QLineEdit *lineEdit = new QLineEdit(ui->tw_addcloth);
        ui->tw_addcloth->setCellWidget(row, 1, lineEdit);
        
        // Set Type to the current clothing type and make it read-only
        if (attributes[i].compare("Type", Qt::CaseInsensitive) == 0) {
            lineEdit->setText(clothingType);
            lineEdit->setReadOnly(true);
        }
        
        // Set existing values for editing mode
        if (isEditMode) {
            QString existingValue = existingItem.getAttribute(attributes[i]);
            if (!existingValue.isEmpty()) {
                lineEdit->setText(existingValue);
            }
        }
    }
    
    // Resize the table columns to fit the content
    ui->tw_addcloth->resizeColumnsToContents();
    ui->tw_addcloth->horizontalHeader()->setStretchLastSection(true);
}

void AddClothing::on_buttonBox_accepted()
{
    // Get item name from the table (row 0, column 1)
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
    
    // Create or update the clothing item
    ClothingItem item;
    if (isEditMode) {
        item = existingItem;
    } else {
        item = ClothingItem(itemName, clothingType);
    }
    
    item.setName(itemName);
    
    // Collect attribute values from the table (starting from row 1, as row 0 is name)
    for (int row = 1; row < ui->tw_addcloth->rowCount(); ++row) {
        QString attrName = ui->tw_addcloth->item(row, 0)->text();
        QString attrValue;
        
        QWidget *widget = ui->tw_addcloth->cellWidget(row, 1);
        if (QComboBox *comboBox = qobject_cast<QComboBox*>(widget)) {
            attrValue = comboBox->currentText();
        } else if (QLineEdit *lineEdit = qobject_cast<QLineEdit*>(widget)) {
            attrValue = lineEdit->text();
        } else if (ui->tw_addcloth->item(row, 1)) {
            // Direct table widget item
            attrValue = ui->tw_addcloth->item(row, 1)->text();
        }
        
        if (!attrValue.isEmpty()) {
            item.addAttribute(attrName, attrValue);
        }
    }
    
    if (isEditMode) {
        emit clothingItemEdited(item);
    } else {
        emit clothingItemAddedItem(item);
        // Also emit the legacy signal for backward compatibility
        emit clothingItemAddedName(itemName);
    }
    
    accept();
}

AddClothing::~AddClothing()
{
    delete ui;
}
