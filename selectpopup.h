#ifndef SELECTPOPUP_H
#define SELECTPOPUP_H

#include <QDialog>

namespace Ui {
class SelectPopup;
}

class SelectPopup : public QDialog
{
    Q_OBJECT

public:
    explicit SelectPopup(QWidget *parent = nullptr);
    ~SelectPopup();

private:
    Ui::SelectPopup *ui;
};

#endif // SELECTPOPUP_H
