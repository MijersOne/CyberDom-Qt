#include "reportclothing.h"
#include "ui_reportclothing.h"
#include "addclothtype.h"
#include "addclothing.h"
#include "cyberdom.h"
#include "scriptparser.h"

#include <QMessageBox>
#include <QSettings>
#include <QDir>
#include <QFileInfo>
#include <QListWidget>
#include <QTextBrowser>
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
    connect(ui->lst_Wearing, &QListWidget::itemSelectionChanged, this, &ReportClothing::onWearingItemSelected);
    connect(ui->lst_Available, &QListWidget::clicked, this, &ReportClothing::onAvailableItemSelected);
    
    // Initialize data from script if available
    loadClothingTypes();
    loadClothingItems();
    updateWearingItems();

    onTypeSelected(ui->cb_Type->currentText());
    
    // If no types loaded, add some defaults for testing
    if (ui->cb_Type->count() == 0) {
        ui->cb_Type->addItems({"Top", "Bottom", "Underwear", "Shoes", "Accessories"});
        ui->cb_Type->setCurrentIndex(0);
    }
}

void ReportClothing::loadClothingTypes()
{
    ui->cb_Type->clear();
    clothTypeAttributes.clear();

    if (!parser) {
        qWarning() << "[ERROR] ScriptParser pointer is null!";
        return;
    }

    const auto &types = parser->getScriptData().clothingTypes;
    QStringList typeNames;

    for (auto it = types.constBegin(); it != types.constEnd(); ++it) {
        QString name = it.key();
        QString display = name;
        if (!display.isEmpty())
            display[0] = display[0].toUpper();
        typeNames.append(display);

        QStringList attrs;
        for (const ClothingAttribute &attr : it.value().attributes)
            attrs.append(attr.name);
        clothTypeAttributes[display.toLower()] = attrs;
    }

    typeNames.sort();
    for (const QString &t : typeNames)
        ui->cb_Type->addItem(t);

    if (ui->cb_Type->count() > 0)
        ui->cb_Type->setCurrentIndex(0);
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

    // Load wearing items
    int wearingCount = settings.beginReadArray("wearing");
    for (int i = 0; i < wearingCount; ++i) {
        settings.setArrayIndex(i);
        QString serializedItem = settings.value("data").toString();
        ClothingItem item = ClothingItem::fromString(serializedItem);
        wearingItems.append(item);
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

    // Save wearing items
    settings.beginWriteArray("wearing", wearingItems.size());
    for (int i = 0; i < wearingItems.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("data", wearingItems[i].toString());
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
    ui->lst_Wearing->clear();
    for (const ClothingItem &item : wearingItems) {
        ui->lst_Wearing->addItem(QString("%1 (%2)").arg(item.getName()).arg(item.getType()));
    }
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
    ui->btn_Remove->setEnabled(ui->lst_Wearing->currentRow() >= 0);
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
    int row = ui->lst_Wearing->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Selection Required", "Please select an item from the wearing list.");
        return;
    }
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

    CyberDom *mainWindow = qobject_cast<CyberDom*>(parent());
    if (!mainWindow && parent())
        mainWindow = qobject_cast<CyberDom*>(parent()->parent());
    if (mainWindow) {
        if (checked)
            mainWindow->setFlag("zzNaked");
        else
            mainWindow->removeFlag("zzNaked");
    } else {
        qWarning() << "[ERROR] Failed to get main window for setting zzNaked flag";
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
    if (!parser) {
        qDebug() << "[ERROR] ScriptParser pointer is null!";
        return;
    }

    const auto &types = parser->getScriptData().clothingTypes;
    for (auto it = types.constBegin(); it != types.constEnd(); ++it) {
        QString name = it.key();
        if (!name.isEmpty())
            name[0] = name[0].toUpper();
        if (ui->cb_Type->findText(name) == -1)
            ui->cb_Type->addItem(name);
    }
}

ReportClothing::~ReportClothing()
{
    delete ui;
}
