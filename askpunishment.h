#ifndef ASKPUNISHMENT_H
#define ASKPUNISHMENT_H

#include <QDialog>

namespace Ui {
class AskPunishment;
}

class AskPunishment : public QDialog
{
    Q_OBJECT

public:
    explicit AskPunishment(QWidget *parent = nullptr);
    ~AskPunishment();

private:
    Ui::AskPunishment *ui;
};

#endif // ASKPUNISHMENT_H
