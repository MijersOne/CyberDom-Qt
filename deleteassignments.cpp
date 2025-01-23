#include "deleteassignments.h"
#include "ui_deleteassignments.h"

DeleteAssignments::DeleteAssignments(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DeleteAssignments)
{
    ui->setupUi(this);
}

DeleteAssignments::~DeleteAssignments()
{
    delete ui;
}
