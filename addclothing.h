#ifndef ADDCLOTHING_H
#define ADDCLOTHING_H

#include <QDialog>
#include <QMap>
#include <QStringList>
#include <QLineEdit>
#include <QLabel>
#include "clothingitem.h"

namespace Ui {
class AddClothing;
}

class AddClothing : public QDialog
{
    Q_OBJECT

public:
    // Constructor for creating a new clothing item
    explicit AddClothing(const QString &clothingType, QWidget *parent = nullptr);
    
    // Constructor with attributes list
    AddClothing(QWidget *parent,
                const QString &selectedType,
                const QStringList &attributes);

    // Constructor for editing an existing clothing item with attributes
    AddClothing(QWidget *parent,
                const QString &selectedType,
                const ClothingItem &item,
                const QStringList &attributes);
    
    ~AddClothing();

signals:
    void clothingItemAddedItem(const ClothingItem &item); // New signal with unique name
    void clothingItemEdited(const ClothingItem &item); // Signal to emit when an item is edited
    void clothingItemAddedName(const QString &itemName); // Renamed for uniqueness
    void clothingItemAdded(const ClothingItem &item);

private slots:
    void onTypeChanged(int index);
    void on_buttonBox_accepted(); // Handle OK button

private:
    Ui::AddClothing *ui;
    QMap<QString, QStringList> clothingTypes;
    // QString clothingType;
    // bool isEditMode;
    // ClothingItem existingItem;
    // QLineEdit *nameEdit; // Line edit for clothing item name
    // QStringList providedAttributes; // Attributes provided from outside
    // bool hasProvidedAttributes; // Flag indicating if attributes were provided
    
    void initializeUI(); // Helper to set up the UI
    void loadAttributes();
};

#endif // ADDCLOTHING_H
