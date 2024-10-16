#include "cyberdom.h"
#include "assignments.h"



MainWindow::~MainWindow()
    QMainWindow(parent),
    ui(new Ui::CyberDom),
    Assignments(nullptr) // Initialize the pointer to nullptr
{
    ui->setupUi(this);

    // Connect the menu action to open the second window
    connect(ui->Open)
}
{
    delete ui;
}
