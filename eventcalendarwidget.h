#ifndef EVENTCALENDARWIDGET_H
#define EVENTCALENDARWIDGET_H

#include <QCalendarWidget>
#include "cyberdom.h"

class EventCalendarWidget : public QCalendarWidget
{
    Q_OBJECT
public:
    explicit EventCalendarWidget(QWidget *parent = nullptr);

    void setEvents(const QList<CalendarEvent> &events);

protected:
    void paintCell(QPainter *painter, const QRect &rect, QDate date) const override;

private:
    QList<CalendarEvent> m_events;
};

#endif // EVENTCALENDARWIDGET_H
