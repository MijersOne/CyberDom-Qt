#include "askclothing.h"
#include "ui_askclothing.h"
#include "scriptparser.h"
#include "cyberdom.h" // Needed to call resolveInstruction

AskClothing::AskClothing(QWidget *parent, ScriptParser *parser, const QString &target)
    : QDialog(parent)
    , ui(new Ui::AskClothing)
    , parser(parser)
    , targetInstruction(target)
{
    ui->setupUi(this);

    // 1. Populate FIRST
    populateCombo();

    // 2. Connect SECOND (Prevents triggering the slot while populating)
    connect(ui->comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AskClothing::onInstructionSelected);
}

AskClothing::~AskClothing()
{
    delete ui;
}

void AskClothing::setInstructionText(const QString &text)
{
    if (ui->textBrowser) {
        ui->textBrowser->setText(text);
    }
}

void AskClothing::onInstructionSelected(int index)
{
    Q_UNUSED(index);

    if (!ui->comboBox) return;

    // Get the instruction name from the combobox data
    QString name = ui->comboBox->currentData().toString();
    if (name.isEmpty()) return;

    // Get access to the main application to use its resolver
    CyberDom *mainApp = qobject_cast<CyberDom*>(parentWidget());
    if (mainApp) {
        // Resolve the text (this handles logic, random choices, variables)
        QString text = mainApp->resolveInstruction(name);

        // Update the UI
        setInstructionText(text);
    }
}

void AskClothing::populateCombo()
{
    if (!parser || !ui->comboBox)
        return;

    qDebug() << "[AskClothing] Populating combo. Target:" << targetInstruction;

    ui->comboBox->clear();
    const auto &instructions = parser->getScriptData().instructions;

    // --- MODE 1: Forced View ---
    if (!targetInstruction.isEmpty()) {
        QString lowerTarget = targetInstruction.toLower();

        if (instructions.contains(lowerTarget)) {
            const InstructionDefinition &def = instructions.value(lowerTarget);
            QString label = def.title.isEmpty() ? def.name : def.title;

            ui->comboBox->addItem(label, def.name);
            ui->comboBox->setEnabled(false);
        } else {
            qDebug() << "[AskClothing] Target instruction not found:" << targetInstruction;
            ui->comboBox->addItem(targetInstruction, targetInstruction);
            ui->comboBox->setEnabled(false);
        }
        return;
    }

    // --- MODE 2: User Selection ---
    // Use the filtered list helper
    const auto instrList = parser->getInstructionSections();
    for (const auto &instr : instrList) {
        if (instr.isClothing && instr.askable) {
            QString label = instr.title.isEmpty() ? instr.name : instr.title;
            if (!label.isEmpty()) {
                ui->comboBox->addItem(label, instr.name);
            }
        }
    }
}

QString AskClothing::getSelectedInstruction() const
{
    if (!ui->comboBox) return QString();
    return ui->comboBox->currentData().toString();
}
