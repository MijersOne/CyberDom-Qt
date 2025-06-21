#include "mainwindow.h"
#include "assignments.h"
#include "addclothing.h"
#include "ui_mainwindow.h"
#include <QTimer>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    reportMenu = ui->menuReport;
    connect(ui->menuAssignments, &QMenu::triggered, this, &MainWindow::openAssignments);
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

void MainWindow::populateReportMenu()
{
    if (!reportMenu)
        return;

    reportMenu->clear();

    QAction *addAct = reportMenu->addAction(tr("Add Clothing"));
    connect(addAct, &QAction::triggered, this, &MainWindow::openAddClothingDialog);
}
