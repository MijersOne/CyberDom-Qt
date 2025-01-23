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
    explicit SelectPunishment(QWidget *parent = nullptr);
    ~SelectPunishment();

private:
    Ui::SelectPunishment *ui;
};

#endif // SELECTPUNISHMENT_H
