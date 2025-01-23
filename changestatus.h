#ifndef CHANGESTATUS_H
#define CHANGESTATUS_H

#include <QDialog>

namespace Ui {
class ChangeStatus;
}

class ChangeStatus : public QDialog
{
    Q_OBJECT

public:
    explicit ChangeStatus(QWidget *parent = nullptr);
    ~ChangeStatus();

private:
    Ui::ChangeStatus *ui;
};

#endif // CHANGESTATUS_H
