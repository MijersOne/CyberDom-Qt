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

ReportClothing::ReportClothing(QWidget *parent, ScriptParser* parser, bool forced, const QString &customTitle)
    : QDialog(parent)
    , ui(new Ui::ReportClothing)
    , parser(parser)
    , isForced(forced)
{
    ui->setupUi(this);

    // --- Handle Window Title ---
    if (!customTitle.isEmpty()) {
        setWindowTitle(customTitle);
        ui->lbl_Title->setText(customTitle);
    } else {
        setWindowTitle("Report Clothing");
    }

    // --- Handle Forced Mode ---
    if (isForced) {
        ui->btn_Cancel->hide();
        setWindowFlags(windowFlags() & ~Qt::WindowCloseButtonHint);
    }

    populateClothTypes();

    connect(ui->btn_AddType, &QPushButton::clicked, this, &ReportClothing::openAddClothTypeDialog);
    connect(ui->btn_Cancel, &QPushButton::clicked, this, &ReportClothing::cancelDialog);
    connect(ui->btn_Remove, &QPushButton::clicked, this, &ReportClothing::removeItemFromWearing);
    connect(ui->btn_Report, &QPushButton::clicked, this, &ReportClothing::submitReport);
    connect(ui->btn_Select, &QPushButton::clicked, this, &ReportClothing::addItemToWearing);
    connect(ui->btn_Add, &QPushButton::clicked, this, &ReportClothing::openAddClothingDialog);
    connect(ui->btn_Edit, &QPushButton::clicked, this, &ReportClothing::editSelectedItem);
    connect(ui->btn_Delete, &QPushButton::clicked, this, &ReportClothing::deleteSelectedItem);

    connect(ui->cb_Type, &QComboBox::currentTextChanged, this, &ReportClothing::onTypeSelected);
    connect(ui->chk_Naked, &QCheckBox::toggled, this, &ReportClothing::onNakedCheckboxToggled);

    connect(ui->lst_Wearing, &QListWidget::itemSelectionChanged, this, &ReportClothing::onWearingItemSelected);
    connect(ui->lst_Available, &QListWidget::clicked, this, &ReportClothing::onAvailableItemSelected);

    loadClothingTypes();
    loadClothingItems();
    updateWearingItems();

    onTypeSelected(ui->cb_Type->currentText());

    if (ui->cb_Type->count() == 0) {
        ui->cb_Type->addItems({"Top", "Bottom", "Underwear", "Shoes", "Accessories"});
        ui->cb_Type->setCurrentIndex(0);
    }
}

void ReportClothing::loadClothingTypes()
{
    ui->cb_Type->clear();
    clothTypeAttributes.clear();

    if (!parser) return;

    const auto &types = parser->getScriptData().clothingTypes;
    QStringList typeNames;

    for (auto it = types.constBegin(); it != types.constEnd(); ++it) {
        QString name = it.key();
        QString display = name;
        if (!display.isEmpty())
            display[0] = display[0].toUpper();
        typeNames.append(display);

        clothTypeAttributes[display.toLower()] = it.value().attributes;
    }

    typeNames.sort();
    for (const QString &t : typeNames)
        ui->cb_Type->addItem(t);

    if (ui->cb_Type->count() > 0)
        ui->cb_Type->setCurrentIndex(0);
}

void ReportClothing::loadClothingItems()
{
    QDir dataDir(QDir::homePath() + "/.cyberdom");
    if (!dataDir.exists()) dataDir.mkpath(".");

    QSettings settings(dataDir.absoluteFilePath("clothing.ini"), QSettings::IniFormat);

    clothingByType.clear();
    wearingItems.clear();

    int itemCount = settings.beginReadArray("items");
    for (int i = 0; i < itemCount; i++) {
        settings.setArrayIndex(i);
        QString serializedItem = settings.value("data").toString();
        ClothingItem item = ClothingItem::fromString(serializedItem);
        clothingByType[item.getType()].append(item);
    }
    settings.endArray();

    int wearingCount = settings.beginReadArray("wearing");
    for (int i = 0; i < wearingCount; ++i) {
        settings.setArrayIndex(i);
        QString serializedItem = settings.value("data").toString();
        ClothingItem item = ClothingItem::fromString(serializedItem);
        wearingItems.append(item);
    }
    settings.endArray();

    if (ui->cb_Type->count() > 0) {
        onTypeSelected(ui->cb_Type->currentText());
    }
}

void ReportClothing::saveClothingItems()
{
    QDir dataDir(QDir::homePath() + "/.cyberdom");
    if (!dataDir.exists()) dataDir.mkpath(".");

    QSettings settings(dataDir.absoluteFilePath("clothing.ini"), QSettings::IniFormat);

    QList<ClothingItem> allItems;
    for (const auto &typeItems : clothingByType) {
        allItems.append(typeItems);
    }

    settings.beginWriteArray("items", allItems.size());
    for (int i = 0; i < allItems.size(); i++) {
        settings.setArrayIndex(i);
        settings.setValue("data", allItems[i].toString());
    }
    settings.endArray();

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

    const QList<ClothingItem> &items = clothingByType.value(currentType);

    ui->lst_Available->clear();

    for (const ClothingItem &item : items) {
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

    QList<ClothingAttribute> attributes = clothTypeAttributes.value(type.toLower());
    if (!attributes.isEmpty()) {
        // Optional debug logging
    }
}

void ReportClothing::onAvailableItemSelected()
{
    ui->btn_Select->setEnabled(ui->lst_Available->currentItem() != nullptr);
}

void ReportClothing::onWearingItemSelected()
{
    ui->btn_Remove->setEnabled(ui->lst_Wearing->currentRow() >= 0);
}

void ReportClothing::addItemToWearing()
{
    QString selectedType = ui->cb_Type->currentText();
    if (selectedType.isEmpty()) {
        QMessageBox::warning(this, "Selection Required", "Please select a clothing type.");
        return;
    }

    QListWidgetItem *selectedItem = ui->lst_Available->currentItem();
    if (!selectedItem) {
        QMessageBox::warning(this, "Selection Required", "Please select an item from the available list.");
        return;
    }

    QString selectedItemName = selectedItem->text();

    for (const ClothingItem &item : clothingByType.value(selectedType)) {
        if (item.getName() == selectedItemName) {
            wearingItems.append(item);
            updateWearingItems();
            updateAvailableItems();
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
        wearingItems.removeAt(row);
        updateWearingItems();
        updateAvailableItems();
    }
}

void ReportClothing::onNakedCheckboxToggled(bool checked)
{
    if (checked) {
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
    }
}

void ReportClothing::cancelDialog()
{
    if (isForced) return;
    reject();
}

void ReportClothing::submitReport()
{
    if (wearingItems.isEmpty() && !ui->chk_Naked->isChecked()) {
        QMessageBox::warning(this, "Selection Required",
                             "Please select clothing items or check 'I Am Naked'.");
        return;
    }
    saveClothingItems();
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
    QString capitalizedClothType = clothType;
    if (!capitalizedClothType.isEmpty()) {
        capitalizedClothType[0] = capitalizedClothType[0].toUpper();
    }

    for (int i = 0; i < ui->cb_Type->count(); i++) {
        if (ui->cb_Type->itemText(i).toLower() == capitalizedClothType.toLower()) {
            ui->cb_Type->setCurrentIndex(i);
            return;
        }
    }

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

    QList<ClothingAttribute> attributes = clothTypeAttributes.value(selectedType.toLower());
    AddClothing addClothingDialog(this, selectedType, attributes);

    // Use UniqueConnection to prevent duplicate signal processing
    connect(&addClothingDialog, &AddClothing::clothingItemAdded,
            this, static_cast<void (ReportClothing::*)(const ClothingItem &)>(&ReportClothing::addClothingItemToList),
            Qt::UniqueConnection);

    addClothingDialog.exec();
}

void ReportClothing::onClothingItemAddedFromDialog(const ClothingItem &item)
{
    addClothingItemToList(item);
}

// Resolve Checks
void ReportClothing::resolveItemChecks(ClothingItem &item)
{
    if (!parser) return;

    QString typeName = item.getType().toLower();
    const auto &clothingTypes = parser->getScriptData().clothingTypes;

    if (clothingTypes.contains(typeName)) {
        const ClothingTypeDefinition &def = clothingTypes.value(typeName);
        QStringList resolvedChecks;

        // Add Base Checks (e.g. "Bra")
        resolvedChecks.append(def.checks);

        // Add Attribute Checks (e.g. Value=Sports Bra -> Check=Sports Bra)
        QMap<QString, QString> attrs = item.getAllAttributes();
        for (auto it = attrs.constBegin(); it != attrs.constEnd(); ++it) {
            QString attrValue = it.value();

            // Look up attribute value in definitions (Case Insensitive scan needed)
            for (auto vcIt = def.valueChecks.constBegin(); vcIt != def.valueChecks.constEnd(); ++vcIt) {
                if (vcIt.key().compare(attrValue, Qt::CaseInsensitive) == 0) {
                    resolvedChecks.append(vcIt.value());
                }
            }
        }

        resolvedChecks.removeDuplicates();
        item.setChecks(resolvedChecks);

        qDebug() << "[ReportClothing] Resolved checks for" << item.getName() << ":" << resolvedChecks;
    }
}

void ReportClothing::addClothingItemToList(const ClothingItem &item)
{
    QString itemName = item.getName().trimmed();
    QString itemType = item.getType().trimmed();

    if (itemName.isEmpty() || itemType.isEmpty()) {
        QMessageBox::warning(this, "Invalid Input", "Clothing name and type must not be empty.");
        return;
    }

    // Normalize type casing (Title Case) to ensure it matches the map keys
    if (!itemType.isEmpty()) itemType[0] = itemType[0].toUpper();

    // --- DEBOUNCE / IDEMPOTENCY CHECK ---
    static QString lastAddedSignature = "";
    QString currentSignature = itemName + "|" + itemType;

    if (currentSignature == lastAddedSignature) {
        return; // Silently skip the duplicate call
    }

    // Access list (auto-creates if new type)
    QList<ClothingItem> &items = clothingByType[itemType];

    // Check for duplicates using trimmed strings
    for (const ClothingItem &existing : items) {
        if (existing.getName().trimmed().compare(itemName, Qt::CaseInsensitive) == 0 &&
            existing.getType().trimmed().compare(itemType, Qt::CaseInsensitive) == 0) {

            QMessageBox::warning(this, "Duplicate Item",
                                 QString("An item named '%1' already exists in '%2'. Please choose a different name.")
                                     .arg(itemName, itemType));
            return;
        }
    }

    // Add to the map
    // We create a new item with the normalized type to ensure consistency
    ClothingItem normalizedItem = item;
    normalizedItem.setType(itemType); // Assuming setter exists, otherwise reuse item logic is fine if type matches

    // Resolve Checks
    resolveItemChecks(normalizedItem);

    items.append(normalizedItem);

    // Update the last added signature on success
    lastAddedSignature = currentSignature;

    saveClothingItems();
    updateAvailableItems();
}

void ReportClothing::editSelectedItem()
{
    QListWidgetItem *selectedItem = ui->lst_Available->currentItem();
    QString selectedType = ui->cb_Type->currentText();

    if (!selectedItem || selectedType.isEmpty()) {
        QMessageBox::warning(this, "Selection Required", "Please select an item to edit.");
        return;
    }

    QString selectedItemName = selectedItem->text();

    for (int i = 0; i < clothingByType[selectedType].size(); i++) {
        if (clothingByType[selectedType][i].getName() == selectedItemName) {
            QList<ClothingAttribute> attributes = clothTypeAttributes.value(selectedType.toLower());

            AddClothing editDialog(this, selectedType, clothingByType[selectedType][i], attributes);

            connect(&editDialog, &AddClothing::clothingItemEdited, this, [this, i, selectedType](const ClothingItem &editedItem) {
                const QString editedName = editedItem.getName();
                const QString editedType = editedItem.getType();

                for (int j = 0; j < clothingByType[editedType].size(); ++j) {
                    if (j == i && editedType == selectedType) continue; // Skip self

                    const ClothingItem &existing = clothingByType[editedType][j];
                    if (existing.getName().compare(editedName, Qt::CaseInsensitive) == 0 &&
                        existing.getType().compare(editedType, Qt::CaseInsensitive) == 0) {

                        QMessageBox::warning(this, "Duplicate Name", QString("An item named '%1' already exists in '%2'.").arg(editedName, editedType));
                        return;
                    }
                }

                // Create modifiable copy to resolve checks
                ClothingItem updatedItem = editedItem;

                // Resolve Checks for Edited Item
                resolveItemChecks(updatedItem);

                clothingByType[editedType][i] = editedItem;
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
    QListWidgetItem *selectedItem = ui->lst_Available->currentItem();
    QString selectedType = ui->cb_Type->currentText();

    if (!selectedItem || selectedType.isEmpty()) {
        QMessageBox::warning(this, "Selection Required", "Please select an item to delete.");
        return;
    }

    QString selectedItemName = selectedItem->text();

    if (QMessageBox::question(this, "Confirm Deletion",
                              QString("Are you sure you want to delete '%1'?").arg(selectedItemName),
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
        return;
    }

    for (int i = 0; i < clothingByType[selectedType].size(); i++) {
        if (clothingByType[selectedType][i].getName() == selectedItemName) {
            clothingByType[selectedType].removeAt(i);
            saveClothingItems();
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
    if (!parser) return;

    const auto &types = parser->getScriptData().clothingTypes;
    for (auto it = types.constBegin(); it != types.constEnd(); ++it) {
        QString name = it.key();
        if (!name.isEmpty())
            name[0] = name[0].toUpper();
        if (ui->cb_Type->findText(name) == -1)
            ui->cb_Type->addItem(name);
    }
}

void ReportClothing::reject() {
    if (isForced) return;
    QDialog::reject();
}

ReportClothing::~ReportClothing()
{
    delete ui;
}
