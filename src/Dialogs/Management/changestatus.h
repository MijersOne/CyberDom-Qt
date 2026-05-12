#ifndef CHANGESTATUS_H
#define CHANGESTATUS_H

#include <QDialog>
#include <QStringList>

namespace Ui {
class ChangeStatus;
}

class ChangeStatus : public QDialog
{
    Q_OBJECT

public:
    explicit ChangeStatus(QWidget *parent = nullptr, const QString &currentStatus = "", const QStringList &availableStatuses = {});
    ~ChangeStatus();

signals:
    void statusUpdated(const QString &newStatus);

private slots:
    void on_buttonBox_accepted(); // Handle the OK button click

private:
    Ui::ChangeStatus *ui;
    QString currentStatus;
};

#endif // CHANGESTATUS_H
