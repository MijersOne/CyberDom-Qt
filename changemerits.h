#ifndef CHANGEMERITS_H
#define CHANGEMERITS_H

#include <QDialog>

namespace Ui {
class ChangeMerits;
}

class ChangeMerits : public QDialog
{
    Q_OBJECT

public:
    explicit ChangeMerits(QWidget *parent = nullptr, int minMerits = 0, int maxMerits = 100);
    ~ChangeMerits();

signals:
    void meritsUpdated(int newMerits);

private:
    Ui::ChangeMerits *ui;
};

#endif // CHANGEMERITS_H
