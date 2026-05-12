#ifndef DETENTION_H
#define DETENTION_H

#include <QDialog>
#include <QTimer>
#include <QCloseEvent>
#include <QKeyEvent>

namespace Ui {
class Detention;
}

class Detention : public QDialog
{
    Q_OBJECT

public:
    explicit Detention(int durationSeconds, const QString &instructions, QWidget *parent = nullptr);
    ~Detention();

protected:
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void updateTimer();
    void showCheckButton();
    void onCheckButtonClicked();
    void onCheckFailed();

private:
    Ui::Detention *ui;
    int remainingSeconds;
    QTimer *mainTimer;
    QTimer *checkTimer;
    QTimer *failTimer;
};

#endif // DETENTION_H
