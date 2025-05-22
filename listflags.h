#ifndef LISTFLAGS_H
#define LISTFLAGS_H

#include <QDialog>
#include <QMap>
#include <QString>

// Forward declare the FlagData struct to avoid circular dependencies
struct FlagData;

namespace Ui {
class ListFlags;
}

class ListFlags : public QDialog
{
    Q_OBJECT

public:
    explicit ListFlags(QWidget *parent = nullptr, const QMap<QString, FlagData> *flagsMap = nullptr);
    ~ListFlags();

signals:
    void flagRemoveRequested(const QString &flagName);

private slots:
    void removeSelectedFlag();
    void populateFlagsList();
    void selectAllFlags();
    void deselectAllFlags();

private:
    Ui::ListFlags *ui;
    const QMap<QString, FlagData> *flagsData;
};

#endif // LISTFLAGS_H
