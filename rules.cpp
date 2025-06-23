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
        ui->lbl_Permission->setText("Confess when:");
        if (mainWin) {
            ScriptParser *parser = mainWin->getScriptParser();
            if (parser) {
                const auto &confs = parser->getScriptData().confessions;
                QStringList items;
                for (auto it = confs.constBegin(); it != confs.constEnd(); ++it) {
                    const ConfessionDefinition &conf = it.value();
                    if (!conf.title.isEmpty())
                        items << conf.title;
                    else
                        items << conf.name;
                }
                ui->lw_Permissions->addItems(items);
            }
        }
        break;
    case Clothing:
        ui->lbl_Permission->setText("ask Clothing Instructions for these situations:");
        if (mainWin) {
            ScriptParser *parser = mainWin->getScriptParser();
            if (parser) {
                const auto instructions = parser->getInstructionSections();
                QStringList items;
                for (const auto &instr : instructions) {
                    if (!instr.isClothing)
                        continue;
                    if (!instr.title.isEmpty())
                        items << instr.title;
                    else
                        items << instr.name;
                }
                ui->lw_Permissions->addItems(items);
            }
        }
        break;
    case Instructions:
        ui->lbl_Permission->setText("always ask Instructions on how to:");
        if (mainWin) {
            ScriptParser *parser = mainWin->getScriptParser();
            if (parser) {
                const auto instructions = parser->getInstructionSections();
                QStringList items;
                for (const auto &instr : instructions) {
                    if (instr.isClothing)
                        continue;
                    if (!instr.title.isEmpty())
                        items << instr.title;
                    else
                        items << instr.name;
                }
                ui->lw_Permissions->addItems(items);
            }
        }
        break;
    case OtherRules:
        ui->lbl_Permission->setText("always obey these Rules:");
        if (mainWin) {
            ScriptParser *parser = mainWin->getScriptParser();
            if (parser) {
                QStringList sections = parser->getRawSectionNames();
                QStringList items;
                for (const QString &sec : sections) {
                    if (!sec.startsWith("rule-", Qt::CaseInsensitive))
                        continue;

                    QString title = parser->getIniValue(sec, "Title");
                    if (title.isEmpty())
                        title = sec.mid(QStringLiteral("rule-").length());
                    items << title;
                }
                ui->lw_Permissions->addItems(items);
            }
        }
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
