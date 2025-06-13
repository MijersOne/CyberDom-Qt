#ifndef ASSIGNMENTS_H
#define ASSIGNMENTS_H

#include <QMainWindow>
#include <QListWidget>
#include <QTableWidget>

class CyberDom;

namespace Ui {
class Assignments;
}

class Assignments : public QMainWindow
{
    Q_OBJECT

public:
    explicit Assignments(QWidget *parent = nullptr, CyberDom *app = nullptr);
    ~Assignments();

public slots:
    void populateJobList();
    void on_btn_Start_clicked();
    void on_btn_Done_clicked();
    void on_btn_Abort_clicked();
    void on_btn_Delete_clicked();

private:
    Ui::Assignments *ui;
    CyberDom *mainApp;
    QString settingsFile;
};

#endif // ASSIGNMENTS_H
