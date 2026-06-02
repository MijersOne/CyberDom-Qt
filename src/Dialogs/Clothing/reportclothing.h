#ifndef REPORTCLOTHING_H
#define REPORTCLOTHING_H

#include <QDialog>
#include <QList>
#include <QMap>
#include <QString>
#include <QListWidgetItem>
#include "clothingitem.h"
#include "scriptparser.h"

namespace Ui {
class ReportClothing;
}

class ReportClothing : public QDialog
{
    Q_OBJECT

public:
    explicit ReportClothing(QWidget *parent = nullptr, ScriptParser* parser = nullptr,
                            bool forced = false, const QString &customTitle = "");
    ~ReportClothing();

    QString getSelectedType() const;
    QList<ClothingItem> getWearingItems() const;
    bool isNaked() const;

private:
    Ui::ReportClothing *ui;
    QMap<QString, QList<ClothingItem>> clothingByType;
    QList<ClothingItem> wearingItems;
    QMap<QString, QList<ClothingAttribute>> clothTypeAttributes;

    void loadClothingTypes();
    void loadClothingItems();
    void saveClothingItems();
    void updateAvailableItems();
    void updateWearingItems();

    ScriptParser* parser;

    void populateClothTypes();

    void addClothingItemToList(const ClothingItem &item);

    void resolveItemChecks(ClothingItem &item);

    bool isForced = false;

private slots:
    void onTypeSelected(const QString &type);
    void onAvailableItemSelected();
    void onWearingItemSelected();

    void addItemToWearing();
    void removeItemFromWearing();

    void onNakedCheckboxToggled(bool checked);

    void cancelDialog();
    void submitReport();

    void openAddClothTypeDialog();
    void addClothTypeToComboBox(const QString &clothType);
    void openAddClothingDialog();

    void onClothingItemAddedFromDialog(const ClothingItem &item);

    void editSelectedItem();
    void deleteSelectedItem();

protected:
    void reject() override;
};

#endif // REPORTCLOTHING_H
