#ifndef JOBLAUNCH_H
#define JOBLAUNCH_H

#include <QDialog>

namespace Ui {
class JobLaunch;
}

class JobLaunch : public QDialog
{
    Q_OBJECT

public:
    explicit JobLaunch(QWidget *parent = nullptr);
    ~JobLaunch();

private:
    Ui::JobLaunch *ui;
};

#endif // JOBLAUNCH_H
