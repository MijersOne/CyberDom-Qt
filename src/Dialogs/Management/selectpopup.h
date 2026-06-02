#ifndef SELECTPOPUP_H
#define SELECTPOPUP_H

#include <QDialog>

// Forward declaration
class ScriptParser;

namespace Ui {
class SelectPopup;
}

class SelectPopup : public QDialog
{
    Q_OBJECT

public:
    // Update constructor to accept ScriptParser
    explicit SelectPopup(QWidget *parent = nullptr, ScriptParser *parser = nullptr);
    ~SelectPopup();

    // Helper to get the chosen popup's internal name
    QString getSelectedPopup() const;

private:
    Ui::SelectPopup *ui;
};

#endif // SELECTPOPUP_H
