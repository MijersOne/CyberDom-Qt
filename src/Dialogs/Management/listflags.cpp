#include "listflags.h"
#include "ui_listflags.h"
#include "cyberdom.h"

#include <QMessageBox>

ListFlags::ListFlags(QWidget *parent, const QMap<QString, FlagData> *flagsMap)
    : QDialog(parent)
    , ui(new Ui::ListFlags)
    , flagsData(flagsMap)
{
    ui->setupUi(this);
    populateFlagsList();

    connect(ui->btn_Remove, &QPushButton::clicked, this, &ListFlags::removeSelectedFlag);
    connect(ui->btn_SelectAll, &QPushButton::clicked, this, &ListFlags::selectAllFlags);
    connect(ui->btn_SelectNone, &QPushButton::clicked,this, &ListFlags::deselectAllFlags);
}

void ListFlags::populateFlagsList() {
    ui->lw_Flags->clear();

    if (!flagsData) {
        return;
    }

    QMapIterator<QString, FlagData> i(*flagsData);
    while (i.hasNext()) {
        i.next();
        QListWidgetItem *item = new QListWidgetItem(i.key());

        // Add expiry time info if set
        if (i.value().expiryTime.isValid()) {
            item->setToolTip("Expires: " + i.value().expiryTime.toString("MM-dd-yyyy hh:mm:ss AP"));
        }

        // Make item checkable
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);

        ui->lw_Flags->addItem(item);
    }
}

void ListFlags::removeSelectedFlag() {
    QList<QString> selectedFlags;

    // Collect all checked flags
    for (int i = 0; i < ui->lw_Flags->count(); i++) {
        QListWidgetItem *item = ui->lw_Flags->item(i);
        if (item->checkState() == Qt::Checked) {
            selectedFlags.append(item->text());
        }
    }

    if (selectedFlags.isEmpty()) {
        QMessageBox::information(this, "No Selection", "Please select at least one flag to remove.");
        return;
    }

    // Confirmation dialog when removing multiple flags
    if (selectedFlags.count() > 1) {
        int response = QMessageBox::question(this, "Confirm Multiple Removal", QString("Are you sure you want to remove %1 flags?").arg(selectedFlags.count()), QMessageBox::Yes | QMessageBox::No);

        if (response != QMessageBox::Yes) {
            return;
        }
    }

    // Remove each selected flag
    for (const QString &flagName : selectedFlags) {
        emit flagRemoveRequested(flagName);
    }

    // Refresh the list after removing flags
    populateFlagsList();
}

void ListFlags::selectAllFlags() {
    // Check all items
    for (int i = 0; i < ui->lw_Flags->count(); i++) {
        QListWidgetItem *item = ui->lw_Flags->item(i);
        item->setCheckState(Qt::Checked);
    }
}

void ListFlags::deselectAllFlags() {
    // Uncheck all items
    for (int i = 0; i < ui->lw_Flags->count(); i++) {
        QListWidgetItem *item = ui->lw_Flags->item(i);
        item->setCheckState(Qt::Unchecked);
    }
}

ListFlags::~ListFlags()
{
    delete ui;
}
