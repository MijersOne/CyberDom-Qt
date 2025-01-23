#include "askpunishment.h"
#include "ui_askpunishment.h"

AskPunishment::AskPunishment(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AskPunishment)
{
    ui->setupUi(this);
}

AskPunishment::~AskPunishment()
{
    delete ui;
}
