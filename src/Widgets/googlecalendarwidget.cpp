#include "googlecalendarwidget.h"
#include <QPainter>
#include <QLocale>
#include <QStaticText>
#include <QMouseEvent>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QScrollArea>

GoogleCalendarWidget::GoogleCalendarWidget(QWidget *parent)
: QWidget(parent), m_month(QDate::currentDate())
{
    // Set a minimum size to ensure cells are readable
    setMinimumSize(800, 600);
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

    // --- Configuration ---
    const int cols = 7;
    const int headerHeight = 30;
    const int cellWidth = width() / cols;
    const int cellHeight = (height() - headerHeight) / 6;

    const int chipMarginX = 4;
    const int chipMarginY = 2;

    QFont dayFont = font();
    dayFont.setBold(true);
    dayFont.setPointSize(dayFont.pointSize() + 2);

    QFont eventFont = font();
    eventFont.setPointSize(eventFont.pointSize() - 1);
    QFontMetrics eventFm(eventFont);

    // --- Draw Header ---
    painter.setPen(palette().text().color());
    for (int i = 0; i < cols; ++i) {
        QRect rect(i * cellWidth, 0, cellWidth, headerHeight);
        QString text = QLocale().dayName(((i + 1) % 7) + 1, QLocale::ShortFormat);
        painter.drawText(rect, Qt::AlignCenter, text);
    }

    // --- Draw Grid & Cells ---
    QDate first(m_month.year(), m_month.month(), 1);
    int startIdx = (first.dayOfWeek() + 6) % 7;
    QDate day = first.addDays(-startIdx);

    QColor gridColor = palette().color(QPalette::Mid);

    for (int row = 0; row < 6; ++row) {
        for (int col = 0; col < cols; ++col) {
            QRect cellRect(col * cellWidth, headerHeight + row * cellHeight, cellWidth, cellHeight);

            // Reset Brush
            painter.setBrush(Qt::NoBrush);

            // Background
            if (day.month() != m_month.month()) {
                painter.fillRect(cellRect, palette().color(QPalette::AlternateBase));
            } else {
                painter.fillRect(cellRect, palette().base());
            }

            // Grid Lines
            painter.setPen(gridColor);
            painter.drawRect(cellRect.adjusted(0, 0, -1, -1));

            // Today Highlight
            if (day == QDate::currentDate()) {
                painter.save();
                QColor highlight = palette().color(QPalette::Highlight);
                highlight.setAlpha(40);
                painter.fillRect(cellRect.adjusted(1, 1, -1, -1), highlight);
                painter.restore();
            }

            // Day Number
            painter.setFont(dayFont);
            painter.setPen(palette().text().color());
            if (day == QDate::currentDate()) {
                painter.setPen(palette().color(QPalette::Highlight));
            }
            painter.drawText(cellRect.adjusted(5, 5, -5, -5), Qt::AlignLeft | Qt::AlignTop, QString::number(day.day()));

            // --- Draw Events ---
            QVector<const CalendarEvent*> eventsForDay;
            for (const CalendarEvent &ev : m_events) {
                if (ev.start.date() <= day && day <= ev.end.date()) {
                    eventsForDay.append(&ev);
                }
            }

            if (!eventsForDay.isEmpty()) {
                int eventY = cellRect.top() + 30;
                int eventH = eventFm.height() + 6;
                int availableHeight = cellRect.bottom() - eventY;
                int maxEvents = availableHeight / (eventH + chipMarginY);

                painter.setFont(eventFont);

                for (int i = 0; i < eventsForDay.size(); ++i) {
                    if (i >= maxEvents && i > 0) {
                        painter.setPen(palette().color(QPalette::Text));
                        QString moreText = QString("+%1 more").arg(eventsForDay.size() - i);
                        painter.drawText(cellRect.left() + 5, eventY + eventFm.ascent(), moreText);
                        break;
                    }

                    const CalendarEvent *ev = eventsForDay[i];
                    QRect eventRect(cellRect.left() + chipMarginX, eventY, cellWidth - (chipMarginX * 2), eventH);

                    QColor bg = colorForType(ev->type);
                    painter.setBrush(bg);
                    painter.setPen(Qt::NoPen);
                    painter.drawRoundedRect(eventRect, 4, 4);

                    painter.setPen(Qt::white);
                    QString displayText = ev->title;
                    QString elidedText = eventFm.elidedText(displayText, Qt::ElideRight, eventRect.width() - 8);
                    painter.drawText(eventRect.adjusted(4, 0, -4, 0), Qt::AlignVCenter | Qt::AlignLeft, elidedText);

                    eventY += eventH + chipMarginY;
                }
            }
            day = day.addDays(1);
        }
    }
}

void GoogleCalendarWidget::mousePressEvent(QMouseEvent *event)
{
    const int cols = 7;
    const int headerHeight = 30;
    const int cellWidth = width() / cols;
    const int cellHeight = (height() - headerHeight) / 6;

    if (event->pos().y() < headerHeight) {
        QWidget::mousePressEvent(event);
        return;
    }

    int col = event->pos().x() / cellWidth;
    int row = (event->pos().y() - headerHeight) / cellHeight;

    if (col < 0 || col >= cols || row < 0 || row >= 6) return;

    QDate first(m_month.year(), m_month.month(), 1);
    int startIdx = (first.dayOfWeek() + 6) % 7;
    QDate startDate = first.addDays(-startIdx);
    int dayIndex = (row * 7) + col;
    QDate clickedDate = startDate.addDays(dayIndex);

    QList<const CalendarEvent*> dayEvents;
    for (const CalendarEvent &ev : m_events) {
        if (ev.start.date() <= clickedDate && clickedDate <= ev.end.date()) {
            dayEvents.append(&ev);
        }
    }

    if (dayEvents.isEmpty()) return;

    // --- Show Custom Styled Dialog ---
    QDialog dlg(this);
    dlg.setWindowTitle(clickedDate.toString("dddd, MMMM d, yyyy"));
    dlg.resize(400, 500);

    QVBoxLayout *dlgLayout = new QVBoxLayout(&dlg);

    // Header
    QLabel *lbl = new QLabel(QString("Events (%1):").arg(dayEvents.size()), &dlg);
    QFont headerFont = font();
    headerFont.setBold(true);
    headerFont.setPointSize(12);
    lbl->setFont(headerFont);
    dlgLayout->addWidget(lbl);

    // --- Scroll Area for Events ---
    QScrollArea *scroll = new QScrollArea(&dlg);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame); // Clean look

    QWidget *scrollContent = new QWidget();
    QVBoxLayout *scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setSpacing(8); // Gap between events
    scrollLayout->setContentsMargins(10, 10, 15, 10); // Padding around the list (Right has extra for scrollbar)

    for (const CalendarEvent *ev : dayEvents) {
        QLabel *itemLbl = new QLabel(ev->title);
        itemLbl->setWordWrap(true);

        QColor bg = colorForType(ev->type);

        // CSS Styling for the "Chip" look
        QString css = QString(
            "QLabel {"
            "  background-color: %1;"
            "  color: white;"
            "  border-radius: 8px;"       // Rounded corners
            "  padding: 10px 15px;"       // Internal padding
            "  font-size: 14px;"
            "}"
        ).arg(bg.name());

        itemLbl->setStyleSheet(css);

        // Add tooltip info
        itemLbl->setToolTip(QString("Type: %1\nEnds: %2").arg(ev->type, ev->end.toString()));

        scrollLayout->addWidget(itemLbl);
    }

    scrollLayout->addStretch(); // Push items to top
    scroll->setWidget(scrollContent);
    dlgLayout->addWidget(scroll);

    // Close Button
    QDialogButtonBox *btns = new QDialogButtonBox(QDialogButtonBox::Close, &dlg);
    connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::accept);
    dlgLayout->addWidget(btns);

    dlg.exec();
}

QColor GoogleCalendarWidget::colorForType(const QString &type) const
{
    if (type.compare(QStringLiteral("Punishment"), Qt::CaseInsensitive) == 0)
        return QColor(220, 53, 69);
    if (type.compare(QStringLiteral("Job"), Qt::CaseInsensitive) == 0)
        return QColor(25, 135, 84);
    if (type.compare(QStringLiteral("Holiday"), Qt::CaseInsensitive) == 0)
        return QColor(13, 110, 253);
    if (type.compare(QStringLiteral("Birthday"), Qt::CaseInsensitive) == 0)
        return QColor(253, 126, 20);
    return QColor(108, 117, 125);
}
