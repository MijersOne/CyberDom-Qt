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
    explicit AskPunishment(QWidget *parent = nullptr, int minPunishment = 25, int maxPunishment = 75);
    ~AskPunishment();

    int getSelectedSeverity() const;

private:
    Ui::AskPunishment *ui;
    int minPunishment;
    int maxPunishment;
};

#endif // ASKPUNISHMENT_H
