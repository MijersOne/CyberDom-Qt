#ifndef NEWASSIGNMENTPOPUP_H
#define NEWASSIGNMENTPOPUP_H

#include <QDialog>

namespace Ui {
class NewAssignmentPopup;
}

class NewAssignmentPopup : public QDialog
{
    Q_OBJECT

public:
    explicit NewAssignmentPopup(QWidget *parent = nullptr);
    ~NewAssignmentPopup();

private:
    Ui::NewAssignmentPopup *ui;
};

#endif // NEWASSIGNMENTPOPUP_H
