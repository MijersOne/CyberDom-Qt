#ifndef SELECTPUNISHMENT_H
#define SELECTPUNISHMENT_H

#include <QDialog>

namespace Ui {
class SelectPunishment;
}

class SelectPunishment : public QDialog
{
    Q_OBJECT

public:
    explicit SelectPunishment(QWidget *parent = nullptr, QMap<QString, QMap<QString, QString>> iniData = QMap<QString, QMap<QString, QString>>());
    ~SelectPunishment();

private:
    Ui::SelectPunishment *ui;
    QMap<QString, QMap<QString, QString>> iniData;

private slots:
    void loadPunishments();
    void on_buttonBox_accepted();
};

#endif // SELECTPUNISHMENT_H
