#include "askinstructions.h"
#include "ui_askinstructions.h"

AskInstructions::AskInstructions(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AskInstructions)
{
    ui->setupUi(this);
}

AskInstructions::~AskInstructions()
{
    delete ui;
}
