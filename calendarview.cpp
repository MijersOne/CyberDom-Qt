#include "calendarview.h"
#include "ui_calendarview.h"
#include <QCalendarWidget>
#include <QSizePolicy>

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


