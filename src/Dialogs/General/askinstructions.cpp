#include "askinstructions.h"
#include "ui_askinstructions.h"
#include "scriptparser.h"
#include "cyberdom.h"
#include <QDebug>

AskInstructions::AskInstructions(QWidget *parent, ScriptParser *parser, const QString &target)
    : QDialog(parent)
    , ui(new Ui::AskInstructions)
    , parser(parser)
    , targetInstruction(target)
{
    ui->setupUi(this);

    // Populate Combobox
    populateCombo();

    // Connect (Prevents triggering the slot while populating
    connect(ui->comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AskInstructions::onInstructionSelected);

    // Trigger initial selection if items exist and no specific target
    if (targetInstruction.isEmpty() && ui->comboBox->count() > 0) {
        onInstructionSelected(0);
    }
}

AskInstructions::~AskInstructions()
{
    delete ui;
}

void AskInstructions::setInstructionText(const QString &text)
{
    if (ui->textBrowser) {
        ui->textBrowser->setText(text);
    }
}

void AskInstructions::onInstructionSelected(int index)
{
    Q_UNUSED(index);

    if (!ui->comboBox) return;

    // Get the instruction name from the combobox data (Hidden ID)
    QString name = ui->comboBox->currentData().toString();
    if (name.isEmpty()) return;

    // Get access to the main application to use its resolver
    CyberDom *mainApp = qobject_cast<CyberDom*>(parentWidget());
    if (mainApp) {
        // Resolve the text (this handles logic, random objects, variables)
        QString text = mainApp->resolveInstruction(name);

        // Update the UI
        setInstructionText(text);
    }
}

void AskInstructions::populateCombo()
{
    if (!parser || !ui->comboBox)
        return;

    ui->comboBox->clear();

    // --- MODE 1: Forced View (Askable=0 or specific trigger) ---
    if (!targetInstruction.isEmpty()) {
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
    const QList<InstructionDefinition> instructions = parser->getInstructionSections();

    for (const InstructionDefinition &instr : instructions) {
        // Filter: Must NOT be Clothing AND Must be Askable AND Must NOT be a Set
        // Sets are stored with a "set:" prefix internally, so we filter those out.
        if (!instr.isClothing && instr.askable && !instr.name.startsWith("set:")) {
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
