#include "askpunishment.h"
#include "ui_askpunishment.h"

AskPunishment::AskPunishment(QWidget *parent, int minPunishment, int maxPunishment)
    : QDialog(parent)
    , ui(new Ui::AskPunishment), minPunishment(minPunishment), maxPunishment(maxPunishment)
{
    ui->setupUi(this);

    // Debugging line to confirm values
    qDebug() << "AskPunishment Dialog Created with Min:" << minPunishment << ", Max:" << maxPunishment;
}

AskPunishment::~AskPunishment()
{
    delete ui;
}
