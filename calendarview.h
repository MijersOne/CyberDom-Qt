#ifndef CALENDARVIEW_H
#define CALENDARVIEW_H

#include <QMainWindow>
#include <QCalendarWidget>
#include <QTableWidget>
#include <QMap>
#include <QDate>

class CyberDom;

namespace Ui {
class CalendarView;
}

class CalendarView : public QMainWindow
{
    Q_OBJECT

public:
    explicit CalendarView(QWidget *parent = nullptr, CyberDom *app = nullptr);
    ~CalendarView();

private slots:
    void onDateClicked(const QDate &date);

private:
    void populateEventsForDate(const QDate &date);

    Ui::CalendarView *ui;
    CyberDom *mainApp;
    QMap<QDate, QStringList> holidays;
};

#endif // CALENDARVIEW_H
