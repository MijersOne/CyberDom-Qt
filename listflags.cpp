#include "listflags.h"
#include "ui_listflags.h"

ListFlags::ListFlags(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ListFlags)
{
    ui->setupUi(this);
}

ListFlags::~ListFlags()
{
    delete ui;
}
