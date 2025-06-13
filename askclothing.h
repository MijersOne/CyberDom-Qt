#ifndef ASKCLOTHING_H
#define ASKCLOTHING_H

#include <QDialog>

namespace Ui {
class AskClothing;
}

class AskClothing : public QDialog
{
    Q_OBJECT

public:
    explicit AskClothing(QWidget *parent = nullptr);
    ~AskClothing();

private:
    Ui::AskClothing *ui;
};

#endif // ASKCLOTHING_H
