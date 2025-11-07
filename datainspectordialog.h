#ifndef DATAINSPECTORDIALOG_H
#define DATAINSPECTORDIALOG_H

#include <QDialog>
#include "scriptparser.h"
#include "ScriptData.h"

namespace Ui {
class DataInspectorDialog;
}

class DataInspectorDialog : public QDialog
{
    Q_OBJECT

public:
    // Modify the constructor to accept the parser
    explicit DataInspectorDialog(ScriptParser* parser, QWidget *parent = nullptr);
    ~DataInspectorDialog();

private:
    void populateTree();

    Ui::DataInspectorDialog *ui;
    ScriptParser* scriptParser;
};

#endif // DATAINSPECTORDIALOG_H
