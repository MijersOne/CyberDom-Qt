#include "mainwindow.h"
#include "assignments.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->menuAssignments, &QMenu::triggered, this, &MainWindow::openAssignments);
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
