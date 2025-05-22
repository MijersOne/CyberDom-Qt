#ifndef ASKINSTRUCTIONS_H
#define ASKINSTRUCTIONS_H

#include <QDialog>

namespace Ui {
class AskInstructions;
}

class AskInstructions : public QDialog
{
    Q_OBJECT

public:
    explicit AskInstructions(QWidget *parent = nullptr);
    ~AskInstructions();

private:
    Ui::AskInstructions *ui;
};

#endif // ASKINSTRUCTIONS_H
