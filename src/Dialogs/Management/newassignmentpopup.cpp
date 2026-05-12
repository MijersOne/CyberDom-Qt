#include "newassignmentpopup.h"
#include "ui_newassignmentpopup.h"

NewAssignmentPopup::NewAssignmentPopup(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::NewAssignmentPopup)
{
    ui->setupUi(this);
}

NewAssignmentPopup::~NewAssignmentPopup()
{
    delete ui;
}
