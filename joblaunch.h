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

    void populateJobDropdown();

private slots:
    void on_btnLaunchJob_clicked();

signals:
    void jobStatusChanged(QString newStatus);

private:
    Ui::JobLaunch *ui;
};

#endif // JOBLAUNCH_H
