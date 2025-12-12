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
                    QString label = perm.title.isEmpty() ? perm.name : perm.title;
                    if (!label.isEmpty()) label[0] = label[0].toUpper();
                    items << label;
                }
                items.sort(Qt::CaseInsensitive);
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
                    QString label = rep.title.isEmpty() ? rep.name : rep.title;
                    if (!label.isEmpty()) label[0] = label[0].toUpper();
                    items << label;
                }
                items.sort(Qt::CaseInsensitive);
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
                    QString label = conf.title.isEmpty() ? conf.name : conf.title;
                    if (!label.isEmpty()) label[0] = label[0].toUpper();
                    items << label;
                }
                items.sort(Qt::CaseInsensitive);
                ui->lw_Permissions->addItems(items);
            }
        }
        break;
    case Clothing:
        ui->lbl_Permission->setText("ask Clothing Instructions for these situations:");
        if (mainWin) {
            ScriptParser *parser = mainWin->getScriptParser();
            if (parser) {
                // FIX: Access raw map directly to avoid 'Set' filtering
                const auto &instructions = parser->getScriptData().instructions;
                QStringList items;

                for (auto it = instructions.constBegin(); it != instructions.constEnd(); ++it) {
                    const InstructionDefinition &instr = it.value();

                    // Filter: Must be Clothing AND Must be Askable
                    if (instr.isClothing && instr.askable) {
                        QString label = instr.title.isEmpty() ? instr.name : instr.title;
                        if (!label.isEmpty()) label[0] = label[0].toUpper();
                        items << label;
                    }
                }
                items.sort(Qt::CaseInsensitive);
                ui->lw_Permissions->addItems(items);
            }
        }
        break;
    case Instructions:
        ui->lbl_Permission->setText("always ask Instructions on how to:");
        if (mainWin) {
            ScriptParser *parser = mainWin->getScriptParser();
            if (parser) {
                // FIX: Access raw map directly
                const auto &instructions = parser->getScriptData().instructions;
                QStringList items;

                for (auto it = instructions.constBegin(); it != instructions.constEnd(); ++it) {
                    const InstructionDefinition &instr = it.value();

                    // Filter: Must NOT be Clothing AND Must be Askable AND Must not be a Set
                    if (!instr.isClothing && instr.askable && !instr.name.startsWith("set:")) {
                        QString label = instr.title.isEmpty() ? instr.name : instr.title;
                        if (!label.isEmpty()) label[0] = label[0].toUpper();
                        items << label;
                    }
                }
                items.sort(Qt::CaseInsensitive);
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

                    if (!title.isEmpty()) title[0] = title[0].toUpper();
                    items << title;
                }
                items.sort(Qt::CaseInsensitive);
                ui->lw_Permissions->addItems(items);
            }
        }
        break;
    }

    this->show();
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
