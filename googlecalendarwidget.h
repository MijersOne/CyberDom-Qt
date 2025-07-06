#ifndef GOOGLECALENDARWIDGET_H
#define GOOGLECALENDARWIDGET_H

#include <QWidget>
#include "cyberdom.h"

class GoogleCalendarWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GoogleCalendarWidget(QWidget *parent = nullptr);

    void setMonth(const QDate &month);
    void setEvents(const QList<CalendarEvent> &events);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QColor colorForType(const QString &type) const;

    QDate m_month;
    QList<CalendarEvent> m_events;
};

#endif // GOOGLECALENDARWIDGET_H
