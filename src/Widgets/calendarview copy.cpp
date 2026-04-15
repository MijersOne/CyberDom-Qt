#include "calendarview.h"
#include "ui_calendarview.h"
#include <QSizePolicy>
#include <QDate>

CalendarView::CalendarView(CyberDom *app, QWidget *parent)
    : QDialog(parent), ui(new Ui::CalendarView), mainApp(app)
{
    ui->setupUi(this);
    // Fill the dialog with the calendar widget
    ui->calendarWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

CalendarView::~CalendarView()
{
    delete ui;
}

void CalendarView::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    if (mainApp) {
        ui->calendarWidget->setMonth(QDate::currentDate());
        ui->calendarWidget->setEvents(mainApp->getCalendarEvents());
    }
}


