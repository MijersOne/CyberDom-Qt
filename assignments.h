#ifndef ASSIGNMENTS_H
#define ASSIGNMENTS_H

#include <QMainWindow>

namespace Ui {
class Assignments;
}

class Assignments : public QMainWindow
{
    Q_OBJECT

public:
    explicit Assignments(QWidget *parent = nullptr);
    ~Assignments();

private:
    Ui::Assignments *ui;
};

#endif // ASSIGNMENTS_H
