#ifndef CALENDARVIEW_H
#define CALENDARVIEW_H

#include <QDialog>
#include <QQuickWidget>
#include "cyberdom.h"

class CalendarView : public QDialog
{
    Q_OBJECT
public:
    explicit CalendarView(CyberDom *app, QWidget *parent = nullptr);
    ~CalendarView();

protected:
    void showEvent(QShowEvent *event) override;

private:
    CyberDom *mainApp;
    QQuickWidget *qmlWidget; // Replaces the old 'ui' pointer
};

#endif // CALENDARVIEW_H