#include "askinstructions.h"
#include "ui_askinstructions.h"
#include "scriptparser.h"
#include <QDebug>

AskInstructions::AskInstructions(QWidget *parent, ScriptParser *parser, const QString &target)
    : QDialog(parent)
    , ui(new Ui::AskInstructions)
    , parser(parser)
    , targetInstruction(target)
{
    ui->setupUi(this);
    populateCombo();
}

AskInstructions::~AskInstructions()
{
    delete ui;
}

void AskInstructions::populateCombo()
{
    if (!parser || !ui->comboBox)
        return;

    ui->comboBox->clear();

    // --- MODE 1: Forced View (Askable=0 or specific trigger) ---
    if (!targetInstruction.isEmpty()) {
        // For direct lookup, we DO use the full map because the target might be a specific set
        const auto &allInstructions = parser->getScriptData().instructions;
        QString lowerTarget = targetInstruction.toLower();

        if (allInstructions.contains(lowerTarget)) {
            const InstructionDefinition &def = allInstructions.value(lowerTarget);
            QString label = def.title.isEmpty() ? def.name : def.title;

            ui->comboBox->addItem(label, def.name);
            ui->comboBox->setEnabled(false);
        } else {
            // Fallback if not found
            ui->comboBox->addItem(targetInstruction, targetInstruction);
            ui->comboBox->setEnabled(false);
        }
        return;
    }

    // --- MODE 2: User Selection (Askable=1) ---
    // FIX: Use getInstructionSections() to filter out [set-*] sections
    const QList<InstructionDefinition> instructions = parser->getInstructionSections();

    for (const InstructionDefinition &instr : instructions) {
        // Filter: Must NOT be Clothing AND Must be Askable
        if (!instr.isClothing && instr.askable) {
            QString label = instr.title.isEmpty() ? instr.name : instr.title;
            if (!label.isEmpty()) {
                ui->comboBox->addItem(label, instr.name);
            }
        }
    }
}

QString AskInstructions::getSelectedInstruction() const
{
    if (!ui->comboBox) return QString();
    return ui->comboBox->currentData().toString();
}
