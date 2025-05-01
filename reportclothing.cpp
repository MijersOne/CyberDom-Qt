#include "reportclothing.h"
#include "ui_reportclothing.h"
#include "addclothtype.h"
#include "addclothing.h"
#include "cyberdom.h"
#include "scriptparser.h"

#include <QMessageBox>
#include <QRegularExpression>
#include <QSettings>
#include <QDir>
#include <QFileInfo>
#include <QListWidget>
#include <QTextBrowser>
#include <QStringListModel>
#include <QDebug>

ReportClothing::ReportClothing(QWidget *parent, ScriptParser* parser)
    : QDialog(parent)
    , ui(new Ui::ReportClothing)
    , parser(parser)
{
    ui->setupUi(this);
    setWindowTitle("Report Clothing");

    populateClothTypes();

    // Connect signals and slots for buttons
    connect(ui->btn_AddType, &QPushButton::clicked, this, &ReportClothing::openAddClothTypeDialog);
    connect(ui->btn_Cancel, &QPushButton::clicked, this, &ReportClothing::cancelDialog);
    connect(ui->btn_Remove, &QPushButton::clicked, this, &ReportClothing::removeItemFromWearing);
    connect(ui->btn_Report, &QPushButton::clicked, this, &ReportClothing::submitReport);
    connect(ui->btn_Select, &QPushButton::clicked, this, &ReportClothing::addItemToWearing);
    connect(ui->btn_Add, &QPushButton::clicked, this, &ReportClothing::openAddClothingDialog);
    connect(ui->btn_Edit, &QPushButton::clicked, this, &ReportClothing::editSelectedItem);
    connect(ui->btn_Delete, &QPushButton::clicked, this, &ReportClothing::deleteSelectedItem);
    
    // Connect type selection
    connect(ui->cb_Type, &QComboBox::currentTextChanged, this, &ReportClothing::onTypeSelected);
    
    // Connect naked checkbox
    connect(ui->chk_Naked, &QCheckBox::toggled, this, &ReportClothing::onNakedCheckboxToggled);
    
    // Connect list widgets for selection
    connect(ui->lst_Wearing, &QListView::clicked, this, &ReportClothing::onWearingItemSelected);
    connect(ui->lst_Available, &QListWidget::clicked, this, &ReportClothing::onAvailableItemSelected);
    
    // Debug: Add default clothing type for testing
    qDebug() << "[DEBUG] Adding default Top clothing type to combobox";
    ui->cb_Type->addItem("Top");
    ui->cb_Type->addItem("Bottom");
    ui->cb_Type->addItem("Underwear");
    ui->cb_Type->addItem("Shoes");
    ui->cb_Type->addItem("Accessories");
    
    // Initialize data from script if available
    loadClothingTypes();
    loadClothingItems();
    updateWearingItems();

    onTypeSelected(ui->cb_Type->currentText());
    
    // If combobox is still empty after loading, use debug items
    if (ui->cb_Type->count() == 0) {
        qDebug() << "[DEBUG] No clothing types loaded from script, using defaults";
        ui->cb_Type->addItem("Top");
        ui->cb_Type->addItem("Bottom");
        ui->cb_Type->addItem("Underwear");
        ui->cb_Type->addItem("Shoes");
        ui->cb_Type->addItem("Accessories");
        ui->cb_Type->setCurrentIndex(0);
    }
}

void ReportClothing::loadClothingTypes()
{
    ui->cb_Type->clear();
    
    // Get main window to access script data
    qDebug() << "\n[DEBUG] ========== LOADING CLOTH TYPES ==========";
    qDebug() << "[DEBUG] Parent class name: " << (parent() ? parent()->metaObject()->className() : "NULL");
    qDebug() << "[DEBUG] Parent's parent: " << (parent() && parent()->parent() ? parent()->parent()->metaObject()->className() : "NULL");
    
    CyberDom *mainWindow = nullptr;
    
    // First try direct parent
    mainWindow = qobject_cast<CyberDom*>(parent());
    if (!mainWindow) {
        // Try parent of parent
        if (parent() && parent()->parent()) {
            mainWindow = qobject_cast<CyberDom*>(parent()->parent());
        }
    }
    
    if (!mainWindow) {
        qWarning("[ERROR] Failed to get main window - parent cast to CyberDom failed");
        return;
    }
    
    // Get ini data from main window
    const QMap<QString, QMap<QString, QString>> iniData = mainWindow->getIniData();
    
    qDebug() << "[DEBUG] Total ini sections: " << iniData.size();
    
    // Debug: List all section names
    QStringList allSections = iniData.keys();
    qDebug() << "[DEBUG] All section names: " << allSections.join(", ");
    
    // Count clothtype sections
    int clothtypeSectionCount = 0;
    for (const QString &section : allSections) {
        if (section.toLower().startsWith("clothtype-")) {
            clothtypeSectionCount++;
            qDebug() << "[DEBUG] Found clothtype section: " << section;
        }
    }
    qDebug() << "[DEBUG] Total clothtype sections found: " << clothtypeSectionCount;
    
    // If no clothtype sections found, try to see if we can directly access ClothTypeSection objects from scriptParser
    if (clothtypeSectionCount == 0) {
        qDebug() << "[DEBUG] No clothtype sections found in iniData, checking if we can directly access scriptParser";
        
        // Manually add some default types if no clothtype sections found
        qDebug() << "[DEBUG] Adding fallback clothing types";
        
        // These types will remain if no clothtype sections are found
        if (ui->cb_Type->count() == 0) {
            ui->cb_Type->addItem("Top");
            ui->cb_Type->addItem("Bottom");
            ui->cb_Type->addItem("Underwear");
            ui->cb_Type->addItem("Shoes");
            ui->cb_Type->addItem("Accessories");
        }
        
        return;
    }
    
    // Initialize our attribute map before processing clothtypes
    clothTypeAttributes.clear();
    
    // Find all clothtype sections
    QRegularExpression clothTypeRegex("^clothtype-(.+)$");
    QStringList typeNames;
    
    for (auto it = iniData.constBegin(); it != iniData.constEnd(); ++it) {
        QRegularExpressionMatch match = clothTypeRegex.match(it.key());
        if (match.hasMatch()) {
            QString typeName = match.captured(1);
            qDebug() << "[DEBUG] Found cloth type: " << typeName << " from section: " << it.key();
            
            // Capitalize the first letter of the type name
            if (!typeName.isEmpty()) {
                typeName[0] = typeName[0].toUpper();
            }
            
            // Add to our list if not already present
            if (!typeNames.contains(typeName)) {
                typeNames.append(typeName);
            }
            
            // Parse the attributes for this type
            parseClothTypeSection(it.key(), mainWindow->replaceVariables(it.key()));
        }
    }
    
    qDebug() << "[DEBUG] Found " << typeNames.size() << " cloth types";
    
    // Sort the type names alphabetically
    typeNames.sort();
    
    // Add all types to the combobox
    for (const QString &typeName : typeNames) {
        qDebug() << "[DEBUG] Adding to dropdown: " << typeName;
        ui->cb_Type->addItem(typeName);
    }
    
    // If there are items, select the first one
    if (ui->cb_Type->count() > 0) {
        ui->cb_Type->setCurrentIndex(0);
        onTypeSelected(ui->cb_Type->currentText());
        qDebug() << "[DEBUG] Selected first type: " << ui->cb_Type->currentText();
    } else {
        qDebug() << "[DEBUG] No types found to select!";
    }
    
    qDebug() << "[DEBUG] ========== FINISHED LOADING CLOTH TYPES ==========\n";
}

void ReportClothing::parseClothTypeSection(const QString &section, const QString &content)
{
    QRegularExpression clothTypeRegex("^clothtype-(.+)$");
    QRegularExpressionMatch match = clothTypeRegex.match(section);
    if (!match.hasMatch()) return;
    
    QString typeName = match.captured(1);
    
    // Get main window to access script data
    CyberDom *mainWindow = nullptr;
    
    // First try direct parent
    mainWindow = qobject_cast<CyberDom*>(parent());
    if (!mainWindow) {
        // Try parent of parent
        if (parent() && parent()->parent()) {
            mainWindow = qobject_cast<CyberDom*>(parent()->parent());
        }
    }
    
    if (!mainWindow) {
        qWarning("[ERROR] Failed to get main window - parent cast to CyberDom failed");
        return;
    }
    
    // Get ini data from main window
    const QMap<QString, QMap<QString, QString>> iniData = mainWindow->getIniData();
    
    if (!iniData.contains(section)) {
        qDebug() << "[DEBUG] Section not found in iniData:" << section;
        return;
    }
    
    // Get the section data
    const QMap<QString, QString> sectionData = iniData[section];
    
    // Extract attributes
    QStringList attributes;
    
    // Debug output
    qDebug() << "[DEBUG] Parsing cloth type section:" << section;
    qDebug() << "[DEBUG] Section data keys:";
    for (auto it = sectionData.constBegin(); it != sectionData.constEnd(); ++it) {
        qDebug() << "[DEBUG]   " << it.key() << " = " << it.value();
    }
    
    // Look for keys that match the pattern "attr=X"
    for (auto it = sectionData.constBegin(); it != sectionData.constEnd(); ++it) {
        // The key from the INI file might be in various formats due to case sensitivity
        // We need to check if it's an attribute key in a case-insensitive way
        QString keyStr = it.key();
        
        // Try different common formats for attr= entries
        if (keyStr.startsWith("attr=", Qt::CaseInsensitive)) {
            QString attrName = keyStr.mid(5); // Remove "attr=" prefix
            attributes.append(attrName);
            qDebug() << "[DEBUG] Found attribute in section" << section << ":" << attrName;
        }
    }
    
    // If we couldn't find any attributes using the above method, try alternative approaches
    if (attributes.isEmpty()) {
        qDebug() << "[DEBUG] No attributes found with standard format, trying alternative parsing";
        
        // Many INI files use exact format "attr" (without =)
        for (auto it = sectionData.constBegin(); it != sectionData.constEnd(); ++it) {
            QString keyStr = it.key();
            QString valueStr = it.value();
            
            if (keyStr.toLower() == "attr") {
                // In this case, the value contains the attribute name
                attributes.append(valueStr);
                qDebug() << "[DEBUG] Found attribute using alt method:" << valueStr;
            }
        }
    }
    
    // Store the attributes for this type
    if (!attributes.isEmpty()) {
        clothTypeAttributes[typeName.toLower()] = attributes;
        qDebug() << "[DEBUG] Stored" << attributes.size() << "attributes for cloth type:" << typeName;
    } else {
        qDebug() << "[DEBUG] No attributes found for cloth type:" << typeName;
    }
}

QStringList ReportClothing::getClothTypeAttributes(const QString &typeName)
{
    // Get main window to access script data
    CyberDom *mainWindow = qobject_cast<CyberDom*>(parent()->parent());
    if (!mainWindow) {
        return QStringList();
    }
    
    // Get ini data from main window
    const QMap<QString, QMap<QString, QString>> iniData = mainWindow->getIniData();
    
    QString sectionName = "clothtype-" + typeName;
    QStringList attributes;
    
    if (iniData.contains(sectionName)) {
        const auto &section = iniData[sectionName];
        
        // Extract attribute names (keys starting with "attr=")
        QRegularExpression attrRegex("attr=(.+)");
        
        for (auto it = section.constBegin(); it != section.constEnd(); ++it) {
            QRegularExpressionMatch match = attrRegex.match(it.key());
            if (match.hasMatch()) {
                attributes << match.captured(1);
            }
        }
    }
    
    return attributes;
}

void ReportClothing::loadClothingItems()
{
    // Get data directory
    QDir dataDir(QDir::homePath() + "/.cyberdom");
    if (!dataDir.exists()) {
        dataDir.mkpath(".");
    }
    
    // Load clothing items from settings
    QSettings settings(dataDir.absoluteFilePath("clothing.ini"), QSettings::IniFormat);
    
    // Clear existing data
    clothingByType.clear();
    wearingItems.clear();
    
    // Load item count
    int itemCount = settings.beginReadArray("items");
    for (int i = 0; i < itemCount; i++) {
        settings.setArrayIndex(i);
        QString serializedItem = settings.value("data").toString();
        
        ClothingItem item = ClothingItem::fromString(serializedItem);
        clothingByType[item.getType()].append(item);
    }
    settings.endArray();
    
    // Update the available items based on the currently selected type
    if (ui->cb_Type->count() > 0) {
        onTypeSelected(ui->cb_Type->currentText());
    }
}

void ReportClothing::saveClothingItems()
{
    // Get data directory
    QDir dataDir(QDir::homePath() + "/.cyberdom");
    if (!dataDir.exists()) {
        dataDir.mkpath(".");
    }
    
    // Save clothing items to settings
    QSettings settings(dataDir.absoluteFilePath("clothing.ini"), QSettings::IniFormat);
    
    // Prepare a flat list of all clothing items
    QList<ClothingItem> allItems;
    for (const auto &typeItems : clothingByType) {
        allItems.append(typeItems);
    }
    
    // Save items
    settings.beginWriteArray("items", allItems.size());
    for (int i = 0; i < allItems.size(); i++) {
        settings.setArrayIndex(i);
        settings.setValue("data", allItems[i].toString());
    }
    settings.endArray();
    settings.sync();
}

void ReportClothing::updateAvailableItems()
{
    QString currentType = ui->cb_Type->currentText();
    if (currentType.isEmpty()) {
        ui->lst_Available->clear();
        return;
    }
    
    // Get items for this type
    const QList<ClothingItem> &items = clothingByType.value(currentType);
    
    ui->lst_Available->clear();
    
    for (const ClothingItem &item : items) {
        // Check if this item is already in the wearing list to avoid duplicates
        bool isWearing = false;
        for (const ClothingItem &wearingItem : wearingItems) {
            if (wearingItem.getName() == item.getName() && 
                wearingItem.getType() == item.getType()) {
                isWearing = true;
                break;
            }
        }
        
        if (!isWearing) {
            ui->lst_Available->addItem(item.getName());
        }
    }
}

void ReportClothing::updateWearingItems()
{
    // Create a model for displaying the wearing items
    QStringListModel *model = new QStringListModel(this);
    QStringList wearingList;
    
    for (const ClothingItem &item : wearingItems) {
        wearingList << QString("%1 (%2)").arg(item.getName()).arg(item.getType());
    }
    
    model->setStringList(wearingList);
    ui->lst_Wearing->setModel(model);
}

void ReportClothing::onTypeSelected(const QString &type)
{
    updateAvailableItems();
    
    // Get attributes for selected type to display them
    QStringList attributes = clothTypeAttributes.value(type.toLower(), QStringList());
    
    // Log the attributes
    if (!attributes.isEmpty()) {
        qDebug() << "[DEBUG] Attributes for type" << type << ":" << attributes.join(", ");
    } else {
        qDebug() << "[DEBUG] No attributes found for type" << type;
    }
}

void ReportClothing::onAvailableItemSelected()
{
    // Enable select button if an item is selected
    ui->btn_Select->setEnabled(ui->lst_Available->currentItem() != nullptr);
}

void ReportClothing::onWearingItemSelected()
{
    // Enable remove button if an item is selected
    ui->btn_Remove->setEnabled(ui->lst_Wearing->currentIndex().isValid());
}

void ReportClothing::addItemToWearing()
{
    QString selectedType = ui->cb_Type->currentText();
    if (selectedType.isEmpty()) {
        QMessageBox::warning(this, "Selection Required", "Please select a clothing type.");
        return;
    }
    
    // Get the selected item
    QListWidgetItem *selectedItem = ui->lst_Available->currentItem();
    if (!selectedItem) {
        QMessageBox::warning(this, "Selection Required", "Please select an item from the available list.");
        return;
    }
    
    QString selectedItemName = selectedItem->text();
    
    // Find the item in the available items
    for (const ClothingItem &item : clothingByType.value(selectedType)) {
        if (item.getName() == selectedItemName) {
            // Add to wearing list
            wearingItems.append(item);
            updateWearingItems();
            updateAvailableItems(); // Refresh to remove the added item
            
            // If an item is added, uncheck the naked checkbox
            ui->chk_Naked->setChecked(false);
            break;
        }
    }
}

void ReportClothing::removeItemFromWearing()
{
    // Get the selected index
    QModelIndex selectedIndex = ui->lst_Wearing->currentIndex();
    if (!selectedIndex.isValid()) {
        QMessageBox::warning(this, "Selection Required", "Please select an item from the wearing list.");
        return;
    }
    
    int row = selectedIndex.row();
    if (row >= 0 && row < wearingItems.size()) {
        // Remove the item from the wearing list
        wearingItems.removeAt(row);
        updateWearingItems();
        updateAvailableItems(); // Refresh to show the removed item
    }
}

void ReportClothing::onNakedCheckboxToggled(bool checked)
{
    if (checked) {
        // If naked is checked, clear the wearing list
        wearingItems.clear();
        updateWearingItems();
    }
}

void ReportClothing::cancelDialog()
{
    reject();
}

void ReportClothing::submitReport()
{
    // Check if no items selected and naked not checked
    if (wearingItems.isEmpty() && !ui->chk_Naked->isChecked()) {
        QMessageBox::warning(this, "Selection Required", 
                           "Please select clothing items or check 'I Am Naked'.");
        return;
    }
    
    // Save current clothing items
    saveClothingItems();
    
    // Accept the dialog
    accept();
}

void ReportClothing::openAddClothTypeDialog()
{
    AddClothType addClothTypeDialog(this);
    connect(&addClothTypeDialog, &AddClothType::clothTypeAdded, this, &ReportClothing::addClothTypeToComboBox);
    
    addClothTypeDialog.exec();
}

void ReportClothing::addClothTypeToComboBox(const QString &clothType)
{
    // Capitalize the first letter of the cloth type
    QString capitalizedClothType = clothType;
    if (!capitalizedClothType.isEmpty()) {
        capitalizedClothType[0] = capitalizedClothType[0].toUpper();
    }
    
    // Check if the type already exists
    for (int i = 0; i < ui->cb_Type->count(); i++) {
        if (ui->cb_Type->itemText(i).toLower() == capitalizedClothType.toLower()) {
            ui->cb_Type->setCurrentIndex(i);
            return;
        }
    }
    
    // Add the new type
    ui->cb_Type->addItem(capitalizedClothType);
    ui->cb_Type->setCurrentText(capitalizedClothType);
}

void ReportClothing::openAddClothingDialog()
{
    QString selectedType = ui->cb_Type->currentText();
    if (selectedType.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select a clothing type.");
        return;
    }

    QStringList attributes = clothTypeAttributes.value(selectedType.toLower(), QStringList());
    AddClothing addClothingDialog(this, selectedType, attributes);

    connect(&addClothingDialog, &AddClothing::clothingItemAdded, this, static_cast<void (ReportClothing::*)(const ClothingItem &)>(&ReportClothing::addClothingItemToList));

    addClothingDialog.exec();
}

// Dedicated slot for handling clothing item addition from dialog
void ReportClothing::onClothingItemAddedFromDialog(const ClothingItem &item)
{
    // Reuse the existing method
    addClothingItemToList(item);
}

void ReportClothing::addClothingItemToList(const ClothingItem &item)
{
    if (item.getName().trimmed().isEmpty() || item.getType().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Invalid Input", "Clothing name and type must not be empty.");
        return;
    }

    QList<ClothingItem> &items = clothingByType[item.getType()];

    // Prevent duplicates by checking name+type
    for (const ClothingItem &existing : items) {
        if (existing.getName().compare(item.getName(), Qt::CaseInsensitive) == 0 &&
            existing.getType().compare(item.getType(), Qt::CaseInsensitive) == 0) {

            qDebug() << "[DEBUG] Duplicate item not added: " << item.getName() << " (" << item.getType() << ")";

            QMessageBox::warning(this, "Duplicate Item",
                                 QString("An item named '%1' already exists in '%2'. Please choose a different name.")
                                     .arg(item.getName(), item.getType()));
            return;
        }
    }

    // Add to the map
    items.append(item);

    // Save and refresh
    saveClothingItems();
    updateAvailableItems();
}

void ReportClothing::editSelectedItem()
{
    // Get the selected item from available items
    QListWidgetItem *selectedItem = ui->lst_Available->currentItem();
    QString selectedType = ui->cb_Type->currentText();
    
    if (!selectedItem || selectedType.isEmpty()) {
        QMessageBox::warning(this, "Selection Required", "Please select an item to edit.");
        return;
    }
    
    QString selectedItemName = selectedItem->text();
    
    // Find the item
    for (int i = 0; i < clothingByType[selectedType].size(); i++) {
        if (clothingByType[selectedType][i].getName() == selectedItemName) {
            // Get attributes for the selected type
            QStringList attributes = clothTypeAttributes.value(selectedType.toLower(), QStringList());
            
            // Open edit dialog with this item and its attributes
            AddClothing editDialog(this, selectedType, clothingByType[selectedType][i], attributes);
            
            // Connect the edited signal
            connect(&editDialog, &AddClothing::clothingItemEdited, this, [this, i, selectedType](const ClothingItem &editedItem) {
                const QString editedName = editedItem.getName();
                const QString editedType = editedItem.getType();

                // Check if edited name would result in duplicate (excluding the item being edited)
                for (int j = 0; j < clothingByType[editedType].size(); ++j) {
                    if (j == i) continue; // Skip the item being edited

                    const ClothingItem &existing = clothingByType[editedType][j];
                    if (existing.getName().compare(editedName, Qt::CaseInsensitive) == 0 &&
                        existing.getType().compare(editedType, Qt::CaseInsensitive) == 0) {

                        QMessageBox::warning(this, "Duplicate Name", QString("An item named '%1' already exists in '%2'. Please choose a different name.").arg(editedName, editedType));
                        return;
                    }
                }

                // Safe to apply the edit
                clothingByType[editedType][i] = editedItem;

                // Save changes and update UI
                saveClothingItems();
                updateAvailableItems();
            });
            
            editDialog.exec();
            break;
        }
    }
}

void ReportClothing::deleteSelectedItem()
{
    // Get the selected item from available items
    QListWidgetItem *selectedItem = ui->lst_Available->currentItem();
    QString selectedType = ui->cb_Type->currentText();
    
    if (!selectedItem || selectedType.isEmpty()) {
        QMessageBox::warning(this, "Selection Required", "Please select an item to delete.");
        return;
    }
    
    QString selectedItemName = selectedItem->text();
    
    // Confirm deletion
    if (QMessageBox::question(this, "Confirm Deletion",
                           QString("Are you sure you want to delete '%1'?").arg(selectedItemName),
                           QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
        return;
    }
    
    // Find and remove the item
    for (int i = 0; i < clothingByType[selectedType].size(); i++) {
        if (clothingByType[selectedType][i].getName() == selectedItemName) {
            clothingByType[selectedType].removeAt(i);
            
            // Save changes
            saveClothingItems();
            
            // Update the available items list
            updateAvailableItems();
            break;
        }
    }
}

QList<ClothingItem> ReportClothing::getWearingItems() const
{
    return wearingItems;
}

bool ReportClothing::isNaked() const
{
    return ui->chk_Naked->isChecked();
}

QString ReportClothing::getSelectedType() const
{
    return ui->cb_Type->currentText();
}

void ReportClothing::populateClothTypes()
{
    if (parser) {
        for (const QString &clothType : parser->clothTypes) {
            ui->cb_Type->addItem(clothType);
        }
    } else {
        qDebug() << "[ERROR] ScriptParser pointer is null!";
    }
}

ReportClothing::~ReportClothing()
{
    delete ui;
}
