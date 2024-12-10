#include "askclothing.h"
#include "ui_askclothing.h"

AskClothing::AskClothing(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AskClothing)
{
    ui->setupUi(this);
}

AskClothing::~AskClothing()
{
    delete ui;
}
