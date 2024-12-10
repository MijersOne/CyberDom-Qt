#ifndef TIMEADD_H
#define TIMEADD_H

#include <QDialog>

namespace Ui {
class TimeAdd;
}

class TimeAdd : public QDialog
{
    Q_OBJECT

public:
    explicit TimeAdd(QWidget *parent = nullptr);
    ~TimeAdd();

private:
    Ui::TimeAdd *ui;

signals:
    void timeAdded(int days, int hours, int minutes, int seconds); // Signal to send time input

private slots:
    void onOkClicked();
};

#endif // TIMEADD_H
