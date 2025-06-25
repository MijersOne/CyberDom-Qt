#include "askclothing.h"
#include "ui_askclothing.h"
#include "scriptparser.h"

AskClothing::AskClothing(QWidget *parent, ScriptParser *parser)
    : QDialog(parent)
    , ui(new Ui::AskClothing)
    , parser(parser)
{
    ui->setupUi(this);
    populateCombo();
}

AskClothing::~AskClothing()
{
    delete ui;
}

void AskClothing::populateCombo()
{
    if (!parser)
        return;

    const auto instructions = parser->getInstructionSections();
    for (const auto &instr : instructions) {
        if (!instr.isClothing)
            continue;
        QString label = instr.title.isEmpty() ? instr.name : instr.title;
        if (!label.isEmpty())
            ui->comboBox->addItem(label);
    }
}
