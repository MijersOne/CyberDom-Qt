#include "calendarview.h"
#include "ui_calendarview.h"
#include <QCalendarWidget>
#include <QListWidget>

CalendarView::CalendarView(CyberDom *app, QWidget *parent)
    : QDialog(parent), ui(new Ui::CalendarView), mainApp(app)
{
    ui->setupUi(this);
    if (mainApp)
        events = mainApp->getCalendarEvents();
    connect(ui->calendarWidget, &QCalendarWidget::selectionChanged, this, [this]() {
        onDateSelected(ui->calendarWidget->selectedDate());
    });
    onDateSelected(ui->calendarWidget->selectedDate());
}

CalendarView::~CalendarView()
{
    delete ui;
}

void CalendarView::populateList(const QDate &date)
{
    ui->listWidget->clear();
    for (const CalendarEvent &ev : events) {
        if (ev.start.date() <= date && date <= ev.end.date()) {
            QString timeStr = ev.start.time().toString("hh:mm");
            if (ev.start != ev.end)
                timeStr += QString("-%1").arg(ev.end.time().toString("hh:mm"));
            ui->listWidget->addItem(timeStr + " " + ev.title + " (" + ev.type + ")");
        }
    }
}

void CalendarView::onDateSelected(const QDate &date)
{
    populateList(date);
}

