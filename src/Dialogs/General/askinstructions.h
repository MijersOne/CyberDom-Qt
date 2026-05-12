#ifndef ASKINSTRUCTIONS_H
#define ASKINSTRUCTIONS_H

#include <QDialog>

namespace Ui {
class AskInstructions;
}

class ScriptParser;

class AskInstructions : public QDialog
{
    Q_OBJECT

public:
    explicit AskInstructions(QWidget *parent = nullptr, ScriptParser *parser = nullptr, const QString &target = "");
    ~AskInstructions();

    QString getSelectedInstruction() const;

    void setInstructionText(const QString &text);

public slots:
    void onInstructionSelected(int index);

private:
    void populateCombo();

private:
    Ui::AskInstructions *ui;
    ScriptParser *parser = nullptr;
    QString targetInstruction;
};

#endif // ASKINSTRUCTIONS_H
