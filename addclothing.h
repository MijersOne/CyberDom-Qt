#ifndef ADDCLOTHING_H
#define ADDCLOTHING_H

#include <QDialog>
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
    explicit AddClothing(QWidget *parent = nullptr, const QString &selectedType = QString());
    
    // Constructor with attributes list
    explicit AddClothing(QWidget *parent, const QString &selectedType, const QStringList &attributes);
    
    // Constructor for editing an existing clothing item
    AddClothing(QWidget *parent, const QString &selectedType, const ClothingItem &item);
    
    // Constructor for editing with provided attributes
    AddClothing(QWidget *parent, const QString &selectedType, const ClothingItem &item, const QStringList &attributes);
    
    ~AddClothing();

signals:
    void clothingItemAddedItem(const ClothingItem &item); // New signal with unique name
    void clothingItemEdited(const ClothingItem &item); // Signal to emit when an item is edited
    void clothingItemAddedName(const QString &itemName); // Renamed for uniqueness

private slots:
    void on_buttonBox_accepted(); // Handle OK button

private:
    Ui::AddClothing *ui;
    QString clothingType;
    bool isEditMode;
    ClothingItem existingItem;
    QLineEdit *nameEdit; // Line edit for clothing item name
    QStringList providedAttributes; // Attributes provided from outside
    bool hasProvidedAttributes; // Flag indicating if attributes were provided
    
    void initializeUI(); // Helper to set up the UI
    void loadAttributes();
};

#endif // ADDCLOTHING_H
