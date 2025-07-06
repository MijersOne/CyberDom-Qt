#include "eventcalendarwidget.h"
#include <QPainter>

EventCalendarWidget::EventCalendarWidget(QWidget *parent)
    : QCalendarWidget(parent)
{
}

void EventCalendarWidget::setEvents(const QList<CalendarEvent> &events)
{
    m_events = events;
    updateCells();
}

void EventCalendarWidget::paintCell(QPainter *painter, const QRect &rect, QDate date) const
{
    QCalendarWidget::paintCell(painter, rect, date);

    if (date == QDate::currentDate()) {
        painter->save();
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(13, 110, 253, 50));
        painter->drawRect(rect.adjusted(1, 1, -1, -1));
        painter->restore();
    }

    QVector<const CalendarEvent*> eventsForDay;
    for (const CalendarEvent &ev : m_events) {
        if (ev.start.date() <= date && date <= ev.end.date())
            eventsForDay.append(&ev);
    }

    if (eventsForDay.isEmpty())
        return;

    int barHeight = 5;
    int offset = rect.bottom() - barHeight * eventsForDay.size();
    int index = 0;
    for (const CalendarEvent *ev : eventsForDay) {
        if (index >= 3)
            break;
        QRect markRect(rect.left() + 2, offset + index * barHeight, rect.width() - 4, barHeight - 1);
        painter->fillRect(markRect, colorForType(ev->type));
        ++index;
    }

    const CalendarEvent *ev = eventsForDay.first();
    QFont f = painter->font();
    f.setPointSizeF(f.pointSizeF() * 0.6);
    painter->setFont(f);
    painter->setPen(Qt::black);
    painter->drawText(rect.adjusted(2, 2, -2, -8 - barHeight * (eventsForDay.size() - 1)),
                      Qt::AlignLeft | Qt::TextWordWrap, ev->title);
}

QColor EventCalendarWidget::colorForType(const QString &type) const
{
    if (type.compare(QStringLiteral("Punishment"), Qt::CaseInsensitive) == 0)
        return QColor(220, 53, 69);
    if (type.compare(QStringLiteral("Job"), Qt::CaseInsensitive) == 0)
        return QColor(13, 110, 253);
    if (type.compare(QStringLiteral("Birthday"), Qt::CaseInsensitive) == 0)
        return QColor(253, 126, 20);
    return QColor(25, 135, 84);
}

