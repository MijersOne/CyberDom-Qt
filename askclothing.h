#ifndef ASKCLOTHING_H
#define ASKCLOTHING_H

#include <QDialog>

namespace Ui {
class AskClothing;
}

class ScriptParser; // Forward declaration

class AskClothing : public QDialog
{
    Q_OBJECT

public:
    explicit AskClothing(QWidget *parent = nullptr, ScriptParser *parser = nullptr, const QString &target = "");
    ~AskClothing();

    QString getSelectedInstruction() const;

    void setInstructionText(const QString &text);

public slots:
    void onInstructionSelected(int index);

private:
    void populateCombo();

private:
    Ui::AskClothing *ui;
    ScriptParser *parser = nullptr;
    QString targetInstruction;

private slots:

};

#endif // ASKCLOTHING_H
