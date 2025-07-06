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

    for (const CalendarEvent &ev : m_events) {
        if (ev.start.date() <= date && date <= ev.end.date()) {
            QColor color;
            if (ev.type.compare(QStringLiteral("Punishment"), Qt::CaseInsensitive) == 0)
                color = Qt::red;
            else if (ev.type.compare(QStringLiteral("Job"), Qt::CaseInsensitive) == 0)
                color = Qt::blue;
            else
                color = Qt::green;

            QRect markRect = QRect(rect.left() + 2, rect.bottom() - 6, rect.width() - 4, 4);
            painter->fillRect(markRect, color);

            QFont f = painter->font();
            f.setPointSizeF(f.pointSizeF() * 0.6);
            painter->setFont(f);
            painter->drawText(rect.adjusted(2, 2, -2, -8), Qt::AlignLeft | Qt::TextWordWrap, ev.title);
            break;
        }
    }
}

