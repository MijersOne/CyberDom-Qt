#include "mainwindow.h"
#include "assignments.h"
#include "addclothing.h"
#include "calendarview.h"
#include "ui_mainwindow.h"
#include "cyberdom.h" // [1] Include CyberDom header
#include <QTimer>

// [2] Declare the external global pointer (defined in main.cpp)
extern CyberDom *mainApp;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    reportMenu = ui->menuReport;

    // Note: Using &QMenu::triggered for assignments might trigger on ANY action in that menu.
    // Usually you want to connect a specific QAction.
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
        // Assignments also likely needs mainApp if its constructor is (QWidget*, CyberDom*)
        // Check assignments.h to be sure. If it's (QWidget*), this is fine.
        // If it matches your other files, it might need: new Assignments(this, mainApp);
        assignments = new Assignments(this);
    }
    assignments->show();
}

void MainWindow::openAddClothingDialog()
{
    AddClothing dlg(this, tr("Generic"));
    dlg.exec();
}

void MainWindow::openCalendar()
{
    if (!calendarWindow)
        // [3] FIX: Pass mainApp as the first argument, and 'this' as the parent
        calendarWindow = new CalendarView(mainApp, this);

    calendarWindow->show();
}

void MainWindow::populateReportMenu()
{
    if (!reportMenu)
        return;

    reportMenu->clear();

    QAction *addAct = new QAction(tr("Add Clothing"), reportMenu);
    reportMenu->addAction(addAct);
    connect(addAct, &QAction::triggered, this, &MainWindow::openAddClothingDialog);
}
