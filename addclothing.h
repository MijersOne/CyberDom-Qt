#ifndef ADDCLOTHING_H
#define ADDCLOTHING_H

#include <QDialog>

namespace Ui {
class AddClothing;
}

class AddClothing : public QDialog
{
    Q_OBJECT

public:
    explicit AddClothing(QWidget *parent, const QString &selectedType);
    ~AddClothing();

signals:
    void clothingItemAdded(const QString &itemName); // Signal to emit when an item is added

private slots:
    void on_buttonBox_accepted(); // Handle OK button

private:
    Ui::AddClothing *ui;
};

#endif // ADDCLOTHING_H
