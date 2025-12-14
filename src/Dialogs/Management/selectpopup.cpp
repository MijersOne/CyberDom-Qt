#include "selectpopup.h"
#include "ui_selectpopup.h"
#include "scriptparser.h"

SelectPopup::SelectPopup(QWidget *parent, ScriptParser *parser)
    : QDialog(parent)
    , ui(new Ui::SelectPopup)
{
    ui->setupUi(this);

    if (parser) {
        const auto &popups = parser->getScriptData().popups;

        // Sort names alphabetically for easier finding
        QStringList keys = popups.keys();
        keys.sort(Qt::CaseInsensitive);

        for (const QString &key : keys) {
            const PopupDefinition &def = popups.value(key);

            // Display Title if available, otherwise internal Name
            QString label = def.title.isEmpty() ? def.name : def.title;

            // Store internal name in UserData for retrieval
            ui->comboBox->addItem(label, def.name);
        }
    }
}

SelectPopup::~SelectPopup()
{
    delete ui;
}

QString SelectPopup::getSelectedPopup() const
{
    // Return the internal name stored in the current item's data
    return ui->comboBox->currentData().toString();
}
