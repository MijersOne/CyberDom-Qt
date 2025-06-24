#ifndef DELETEASSIGNMENTS_H
#define DELETEASSIGNMENTS_H

#include <QDialog>

class CyberDom;

namespace Ui {
class DeleteAssignments;
}

class DeleteAssignments : public QDialog
{
    Q_OBJECT

public:
    explicit DeleteAssignments(QWidget *parent = nullptr);
    ~DeleteAssignments();

private slots:
    void populateAssignmentsList();
    void deleteSelectedAssignments();
    void selectAllAssignments();
    void deselectAllAssignments();

private:
    Ui::DeleteAssignments *ui;
    CyberDom *mainApp = nullptr;
};

#endif // DELETEASSIGNMENTS_H
