#include "rules.h"
#include "ui_rules.h"

Rules::Rules(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Rules)
{
    ui->setupUi(this);
}

void Rules::updateRulesDialog(RuleCategory category)
{
    // Set the title for the lbl_Permission
    switch (category) {
    case Permissions:
        ui->lbl_Permission->setText("Permissions Rules");
        ui->lbl_Permission->clear();
        ui->lw_Permissions->addItems({"Permission A", "Permission B", "Permission C"});
        break;
    case Reports:
        ui->lbl_Permission->setText("Reports Rules");
        ui->lbl_Permission->clear();
        ui->lw_Permissions->addItems({"Report X", "Report Y", "Report Z"});
        break;
    case Confessions:
        ui->lbl_Permission->setText("Confessions Rules");
        ui->lbl_Permission->clear();
        ui->lw_Permissions->addItems({"Confession 1", "Confession 2"});
        break;
    case Clothing:
        ui->lbl_Permission->setText("Clothing Rules");
        ui->lbl_Permission->clear();
        ui->lw_Permissions->addItems({"Clothing Rule 1", "Clothing Rule 2"});
        break;
    case Instructions:
        ui->lbl_Permission->setText("Instructions Rules");
        ui->lbl_Permission->clear();
        ui->lw_Permissions->addItems({"Instruction A", "Instruction B"});
        break;
    case OtherRules:
        ui->lbl_Permission->setText("Other Rules");
        ui->lbl_Permission->clear();
        ui->lw_Permissions->addItems({"Other Rule 1", "Other Rule 2"});
        break;
    }

    this->show(); // Display the dialog
}

Rules::~Rules()
{
    delete ui;
}
