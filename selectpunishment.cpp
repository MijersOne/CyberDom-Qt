#include "selectpunishment.h"
#include "ui_selectpunishment.h"

SelectPunishment::SelectPunishment(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SelectPunishment)
{
    ui->setupUi(this);
}

SelectPunishment::~SelectPunishment()
{
    delete ui;
}
