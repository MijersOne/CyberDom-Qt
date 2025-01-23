#include "joblaunch.h"
#include "ui_joblaunch.h"

JobLaunch::JobLaunch(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::JobLaunch)
{
    ui->setupUi(this);
}

JobLaunch::~JobLaunch()
{
    delete ui;
}
