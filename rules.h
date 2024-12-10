#ifndef RULES_H
#define RULES_H

#include <QDialog>

namespace Ui {
class Rules;
}


enum RuleCategory {
    Permissions,
    Reports,
    Confessions,
    Clothing,
    Instructions,
    OtherRules
};

class Rules : public QDialog
{
    Q_OBJECT

public:
    explicit Rules(QWidget *parent = nullptr);
    ~Rules();

private:
    Ui::Rules *ui;

public slots:
    void updateRulesDialog(RuleCategory category);
};

#endif // RULES_H
