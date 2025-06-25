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
    explicit AskClothing(QWidget *parent = nullptr, ScriptParser *parser = nullptr);
    ~AskClothing();

private:
    void populateCombo();

private:
    Ui::AskClothing *ui;
    ScriptParser *parser = nullptr;
};

#endif // ASKCLOTHING_H
