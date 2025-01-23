#ifndef DELETEASSIGNMENTS_H
#define DELETEASSIGNMENTS_H

#include <QDialog>

namespace Ui {
class DeleteAssignments;
}

class DeleteAssignments : public QDialog
{
    Q_OBJECT

public:
    explicit DeleteAssignments(QWidget *parent = nullptr);
    ~DeleteAssignments();

private:
    Ui::DeleteAssignments *ui;
};

#endif // DELETEASSIGNMENTS_H
