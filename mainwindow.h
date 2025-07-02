#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenu>
#include "assignments.h" // Include the Assignments header
#include "addclothing.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void openAssignments();
    void openAddClothingDialog();
    void populateReportMenu();

private:
    Ui::MainWindow *ui;
    Assignments *assignments;
    QMenu *reportMenu = nullptr;
};
#endif // MAINWINDOW_H
