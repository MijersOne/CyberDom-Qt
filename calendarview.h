#ifndef CALENDARVIEW_H
#define CALENDARVIEW_H


#include <QDialog>
#include "cyberdom.h"

namespace Ui {
class CalendarView;
}

class CalendarView : public QDialog
{
    Q_OBJECT
public:
    explicit CalendarView(CyberDom *app, QWidget *parent = nullptr);
    ~CalendarView();

private:
    Ui::CalendarView *ui;
    CyberDom *mainApp;
};

#endif // CALENDARVIEW_H
