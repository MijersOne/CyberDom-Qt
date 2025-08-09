#include "mainwindow.h"
#include "assignments.h"
#include "addclothing.h"
// #include "calendarview.h"
#include "ui_mainwindow.h"
#include <QTimer>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    reportMenu = ui->menuReport;
    connect(ui->menuAssignments, &QMenu::triggered, this, &MainWindow::openAssignments);
    QAction *calAct = new QAction(tr("Calendar"), this);
    ui->menuAssignments->addAction(calAct);
    connect(calAct, &QAction::triggered, this, &MainWindow::openCalendar);
    QTimer::singleShot(0, this, &MainWindow::populateReportMenu);
}

MainWindow::~MainWindow()
{
    delete ui;
    if (assignments) {
        delete assignments;
    }
}

void MainWindow::openAssignments()
{
    if (!assignments) {
        assignments = new Assignments(this);
    }
    assignments->show();
}

void MainWindow::openAddClothingDialog()
{
    AddClothing dlg(this, tr("Generic"));
    dlg.exec();
}

// void MainWindow::openCalendar()
// {
//     if (!calendarWindow)
//         calendarWindow = new CalendarView(this);
//     calendarWindow->show();
// }

void MainWindow::populateReportMenu()
{
    if (!reportMenu)
        return;

    reportMenu->clear();

    QAction *addAct = new QAction(tr("Add Clothing"), reportMenu);
    reportMenu->addAction(addAct);
    connect(addAct, &QAction::triggered, this, &MainWindow::openAddClothingDialog);
}
