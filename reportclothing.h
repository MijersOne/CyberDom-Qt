#ifndef REPORTCLOTHING_H
#define REPORTCLOTHING_H

#include <QDialog>
#include <QList>
#include <QMap>
#include <QString>
#include <QListWidgetItem>
#include "clothingitem.h"

namespace Ui {
class ReportClothing;
}

class ReportClothing : public QDialog
{
    Q_OBJECT

public:
    explicit ReportClothing(QWidget *parent = nullptr);
    ~ReportClothing();

    QString getSelectedType() const; // Method to get the selected item
    QList<ClothingItem> getWearingItems() const; // Method to get items being worn
    bool isNaked() const; // Method to check if "I am naked" is checked

private:
    Ui::ReportClothing *ui;
    QMap<QString, QList<ClothingItem>> clothingByType; // Map of clothing items by type
    QList<ClothingItem> wearingItems; // List of items currently being worn
    QMap<QString, QStringList> clothTypeAttributes; // Map of cloth type names to their attributes
    
    void loadClothingTypes(); // Load clothing types from ini file
    void loadClothingItems(); // Load clothing items from settings
    void saveClothingItems(); // Save clothing items to settings
    void updateAvailableItems(); // Update the available items list based on selected type
    void updateWearingItems(); // Update the wearing items list
    
    // Helper methods
    void parseClothTypeSection(const QString &section, const QString &content);
    QStringList getClothTypeAttributes(const QString &typeName);

private slots:
    void onTypeSelected(const QString &type); // When type is selected in dropdown
    void onAvailableItemSelected(); // When item is selected from available list
    void onWearingItemSelected(); // When item is selected from wearing list
    
    void addItemToWearing(); // Add selected item to wearing list
    void removeItemFromWearing(); // Remove selected item from wearing list
    
    void onNakedCheckboxToggled(bool checked); // When "I am naked" is checked
    
    void cancelDialog(); // Cancel button clicked
    void submitReport(); // Report button clicked
    
    void openAddClothTypeDialog(); // Slot to open the AddClothType dialog
    void addClothTypeToComboBox(const QString &clothType); // Slot to update the combo box
    void openAddClothingDialog(); // Slot to open the AddClothing dialog
    
    // Slots to update the listbox
    void addClothingItemToList(const QString &itemName);
    void addClothingItemToList(const ClothingItem &item);
    
    // Dedicated slot for AddClothing dialog
    void onClothingItemAddedFromDialog(const ClothingItem &item);
    
    void editSelectedItem(); // Edit selected item
    void deleteSelectedItem(); // Delete selected item
};

#endif // REPORTCLOTHING_H
