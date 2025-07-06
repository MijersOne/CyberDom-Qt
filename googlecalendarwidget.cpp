#include "googlecalendarwidget.h"
#include <QPainter>
#include <QLocale>

GoogleCalendarWidget::GoogleCalendarWidget(QWidget *parent)
    : QWidget(parent), m_month(QDate::currentDate())
{
}

void GoogleCalendarWidget::setMonth(const QDate &month)
{
    if (month.isValid())
        m_month = QDate(month.year(), month.month(), 1);
    update();
}

void GoogleCalendarWidget::setEvents(const QList<CalendarEvent> &events)
{
    m_events = events;
    update();
}

void GoogleCalendarWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const int cols = 7;
    const int headerHeight = 20;
    const int cellWidth = width() / cols;
    const int cellHeight = (height() - headerHeight) / 6;

    // Day of week headers
    QDate weekRef(2023,1,2); // Monday
    for (int i = 0; i < cols; ++i) {
        QRect rect(i * cellWidth, 0, cellWidth, headerHeight);
        QString text = QLocale().dayName(((i + 1) % 7) + 1, QLocale::ShortFormat);
        painter.drawText(rect, Qt::AlignCenter, text);
    }

    QDate first(m_month.year(), m_month.month(), 1);
    int startIdx = (first.dayOfWeek() + 6) % 7; // Monday=0
    QDate day = first.addDays(-startIdx);

    for (int row = 0; row < 6; ++row) {
        for (int col = 0; col < cols; ++col) {
            QRect rect(col * cellWidth, headerHeight + row * cellHeight, cellWidth, cellHeight);
            if (day.month() != m_month.month())
                painter.fillRect(rect, palette().alternateBase());
            else
                painter.fillRect(rect, palette().base());
            painter.drawRect(rect.adjusted(0,0,-1,-1));

            if (day == QDate::currentDate()) {
                painter.save();
                painter.setPen(Qt::NoPen);
                painter.setBrush(QColor(13,110,253,50));
                painter.drawRect(rect.adjusted(1,1,-1,-1));
                painter.restore();
            }

            painter.drawText(rect.adjusted(2,2,-2,-2), Qt::AlignLeft | Qt::AlignTop, QString::number(day.day()));

            QVector<const CalendarEvent*> eventsForDay;
            for (const CalendarEvent &ev : m_events) {
                if (ev.start.date() <= day && day <= ev.end.date())
                    eventsForDay.append(&ev);
            }

            int barHeight = 5;
            int offset = rect.bottom() - barHeight * eventsForDay.size();
            int index = 0;
            for (const CalendarEvent *ev : eventsForDay) {
                if (index >= 3)
                    break;
                QRect markRect(rect.left() + 2, offset + index * barHeight, rect.width() - 4, barHeight - 1);
                painter.fillRect(markRect, colorForType(ev->type));
                ++index;
            }

            if (!eventsForDay.isEmpty()) {
                const CalendarEvent *ev = eventsForDay.first();
                QFont f = painter.font();
                f.setPointSizeF(f.pointSizeF() * 0.6);
                painter.setFont(f);
                painter.setPen(Qt::black);
                painter.drawText(rect.adjusted(2, 20, -2, -8 - barHeight * (eventsForDay.size() - 1)),
                                 Qt::AlignLeft | Qt::TextWordWrap, ev->title);
            }

            day = day.addDays(1);
        }
    }
}

QColor GoogleCalendarWidget::colorForType(const QString &type) const
{
    if (type.compare(QStringLiteral("Punishment"), Qt::CaseInsensitive) == 0)
        return QColor(220, 53, 69);
    if (type.compare(QStringLiteral("Job"), Qt::CaseInsensitive) == 0)
        return QColor(13, 110, 253);
    if (type.compare(QStringLiteral("Birthday"), Qt::CaseInsensitive) == 0)
        return QColor(253, 126, 20);
    return QColor(25, 135, 84);
}
