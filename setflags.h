#ifndef SETFLAGS_H
#define SETFLAGS_H

#include <QDialog>

namespace Ui {
class SetFlags;
}

class SetFlags : public QDialog
{
    Q_OBJECT

public:
    explicit SetFlags(QWidget *parent = nullptr);
    ~SetFlags();

private:
    Ui::SetFlags *ui;
};

#endif // SETFLAGS_H
