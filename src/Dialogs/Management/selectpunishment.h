#ifndef SELECTPUNISHMENT_H
#define SELECTPUNISHMENT_H

#include "scriptparser.h"
#include "ScriptData.h"

#include <QDialog>

namespace Ui {
class SelectPunishment;
}

class SelectPunishment : public QDialog
{
    Q_OBJECT

public:
    explicit SelectPunishment(QWidget *parent = nullptr, ScriptParser *parser = nullptr);
    ~SelectPunishment();

private:
    Ui::SelectPunishment *ui;
    ScriptParser *parser;

private slots:
    void populatePunishments();
    void on_buttonBox_accepted();
};

#endif // SELECTPUNISHMENT_H
