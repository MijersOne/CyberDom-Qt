#include "calendarview.h"
#include "cyberdom.h"
#include "ui_calendarview.h"

#include <QHeaderView>
#include <QSet>
#include <QDateTime>

CalendarView::CalendarView(QWidget *parent, CyberDom *app)
    : QMainWindow(parent)
    , ui(new Ui::CalendarView)
    , mainApp(app)
{
    ui->setupUi(this);

    if (mainApp)
        holidays = mainApp->getHolidays();

    ui->tableWidget->setColumnCount(2);
    ui->tableWidget->setHorizontalHeaderLabels({tr("Time"), tr("Event")});
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);

    connect(ui->calendarWidget, &QCalendarWidget::clicked,
            this, &CalendarView::onDateClicked);

    populateEventsForDate(ui->calendarWidget->selectedDate());
}

CalendarView::~CalendarView()
{
    delete ui;
}

void CalendarView::onDateClicked(const QDate &date)
{
    populateEventsForDate(date);
}

void CalendarView::populateEventsForDate(const QDate &date)
{
    if (!mainApp)
        return;

    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount(0);
    int row = 0;

    const QSet<QString> jobs = mainApp->getActiveJobs();
    const QMap<QString, QDateTime> deadlines = mainApp->getJobDeadlines();
    const auto &iniData = mainApp->getIniData();

    for (const QString &name : jobs) {
        bool isPun = iniData.contains("punishment-" + name);
        if (!deadlines.contains(name))
            continue;
        QDateTime deadline = deadlines[name];
        bool longRunning = deadline.date() > QDate::currentDate();
        QDate startDate = longRunning ? QDate::currentDate() : deadline.date();

        if (deadline.date() == date || (date >= startDate && date <= deadline.date())) {
            ui->tableWidget->insertRow(row);
            ui->tableWidget->setItem(row, 0, new QTableWidgetItem(deadline.time().toString("hh:mm AP")));
            QString label = (isPun ? tr("Punishment: ") : tr("Job: ")) + name;
            if (longRunning && deadline.date() != date)
                label += tr(" (ongoing)");
            ui->tableWidget->setItem(row, 1, new QTableWidgetItem(label));
            ++row;
        }
    }

    if (holidays.contains(date)) {
        for (const QString &name : holidays.value(date)) {
            ui->tableWidget->insertRow(row);
            ui->tableWidget->setItem(row, 0, new QTableWidgetItem(tr("All Day")));
            ui->tableWidget->setItem(row, 1, new QTableWidgetItem(tr("Holiday: %1").arg(name)));
            ++row;
        }
    }
}

