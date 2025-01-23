#include "setflags.h"
#include "ui_setflags.h"

SetFlags::SetFlags(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SetFlags)
{
    ui->setupUi(this);
}

SetFlags::~SetFlags()
{
    delete ui;
}
