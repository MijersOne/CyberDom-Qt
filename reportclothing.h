#ifndef REPORTCLOTHING_H
#define REPORTCLOTHING_H

#include <QDialog>

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

private:
    Ui::ReportClothing *ui;

private slots:
    void openAddClothTypeDialog(); // Slot to open the AddClothType dialog
    void addClothTypeToComboBox(const QString &clothType); // Slot to update the combo box
    void openAddClothingDialog(); // Slot to open the AddClothing dialog
    void addClothingItemToList(const QString &itemName); // Slot to update the listbox
};

#endif // REPORTCLOTHING_H
