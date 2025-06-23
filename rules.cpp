#include "rules.h"
#include "ui_rules.h"
#include "cyberdom.h"

Rules::Rules(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Rules)
{
    ui->setupUi(this);
    connect(this, &QDialog::finished, this, &Rules::resetDialog);
}

void Rules::updateRulesDialog(RuleCategory category)
{
    CyberDom *mainWin = qobject_cast<CyberDom *>(parent());

    ui->lw_Permissions->clear();
    ui->lbl_Permission->clear();

    switch (category) {
    case Permissions:
        ui->lbl_Permission->setText("ask Permission to:");
        if (mainWin) {
            ScriptParser *parser = mainWin->getScriptParser();
            if (parser) {
                const auto &perms = parser->getScriptData().permissions;
                QStringList items;
                for (const auto &perm : perms) {
                    if (!perm.title.isEmpty())
                        items << perm.title;
                    else
                        items << perm.name;
                }
                ui->lw_Permissions->addItems(items);
            }
        }
        break;
    case Reports:
        ui->lbl_Permission->setText("Report when:");
        if (mainWin) {
            ScriptParser *parser = mainWin->getScriptParser();
            if (parser) {
                const auto &reports = parser->getScriptData().reports;
                QStringList items;
                for (auto it = reports.constBegin(); it != reports.constEnd(); ++it) {
                    const ReportDefinition &rep = it.value();
                    if (!rep.title.isEmpty())
                        items << rep.title;
                    else
                        items << rep.name;
                }
                ui->lw_Permissions->addItems(items);
            }
        }
        break;
    case Confessions:
        ui->lbl_Permission->setText("Confessions Rules");
        ui->lw_Permissions->addItems({"Confession 1", "Confession 2"});
        break;
    case Clothing:
        ui->lbl_Permission->setText("Clothing Rules");
        ui->lw_Permissions->addItems({"Clothing Rule 1", "Clothing Rule 2"});
        break;
    case Instructions:
        ui->lbl_Permission->setText("Instructions Rules");
        ui->lw_Permissions->addItems({"Instruction A", "Instruction B"});
        break;
    case OtherRules:
        ui->lbl_Permission->setText("Other Rules");
        ui->lw_Permissions->addItems({"Other Rule 1", "Other Rule 2"});
        break;
    }

    this->show(); // Display the dialog
}

Rules::~Rules()
{
    delete ui;
}

void Rules::resetDialog()
{
    ui->lw_Permissions->clear();
    ui->lbl_Permission->clear();
}
