#ifndef CYBERDOM_H
#define CYBERDOM_H

#include "ui_cyberdom.h"
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class CyberDom;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::CyberDom *ui;
    Ui::Assignments *Assignments;

};

inline MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::CyberDom)
{
    ui->setupUi(this);
}
#endif // CYBERDOM_H
