#include "selectpopup.h"
#include "ui_selectpopup.h"

SelectPopup::SelectPopup(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SelectPopup)
{
    ui->setupUi(this);
}

SelectPopup::~SelectPopup()
{
    delete ui;
}
