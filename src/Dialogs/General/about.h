#ifndef ABOUT_H
#define ABOUT_H

#include <QDialog>
#include <QString>

namespace Ui {
class About;
}

class About : public QDialog
{
    Q_OBJECT

public:
    explicit About(QWidget *parent = nullptr, const QString &iniFilePath = "", const QString &version = "Unknown");
    ~About();

private:
    Ui::About *ui;
    QString scriptVersion;
};

#endif // ABOUT_H
