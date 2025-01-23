#ifndef LISTFLAGS_H
#define LISTFLAGS_H

#include <QDialog>

namespace Ui {
class ListFlags;
}

class ListFlags : public QDialog
{
    Q_OBJECT

public:
    explicit ListFlags(QWidget *parent = nullptr);
    ~ListFlags();

private:
    Ui::ListFlags *ui;
};

#endif // LISTFLAGS_H
