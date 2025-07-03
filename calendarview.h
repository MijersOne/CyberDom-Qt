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

private slots:
    void onDateSelected(const QDate &date);

private:
    void populateList(const QDate &date);

    Ui::CalendarView *ui;
    CyberDom *mainApp;
    QList<CalendarEvent> events;
};

#endif // CALENDARVIEW_H
