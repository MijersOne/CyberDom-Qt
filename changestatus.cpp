#include "changestatus.h"
#include "ui_changestatus.h"

ChangeStatus::ChangeStatus(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ChangeStatus)
{
    ui->setupUi(this);
}

ChangeStatus::~ChangeStatus()
{
    delete ui;
}
