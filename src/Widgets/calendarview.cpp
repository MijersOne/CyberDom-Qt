#include "calendarview.h"
#include <QVBoxLayout>
#include <QUrl>
#include <QQmlContext>
#include <QVariantMap>
#include <QVariantList>
#include <QDate>
#include <QQuickItem>
#include <QDirIterator>
#include <QQmlError>
#include <QDebug>
#include <qdiriterator.h>

CalendarView::CalendarView(CyberDom *app, QWidget *parent)
    : QDialog(parent), mainApp(app)
{
    resize(800, 600);
    setWindowTitle("CyberDom Calendar");

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    qmlWidget = new QQuickWidget(this);
    qmlWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);

    // DIAGNOSTIC: DUMP ALL PACKED RESOURCES
    qDebug() << "================================";
    qDebug() << "CHECKING PACKED RESOURCES:";
    QDirIterator it(":", QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString resource = it.next();
        // Only print QML files
        if (resource.endsWith(".qml")) {
            qDebug() << "[FOUND IN BACKPACK]" << resource;
        }
    }
    qDebug() << "=================================";
    
    // We load the visual interface right away!
    qmlWidget->setSource(QUrl(QStringLiteral("qrc:/src/Widgets/cybercalendar.qml")));

    if (qmlWidget->status() == QQuickWidget::Error) {
        for (const QQmlError &error : qmlWidget->errors()) {
            qDebug() << "[CALENDAR ERROR]" << error.toString();
        }
    }

    layout->addWidget(qmlWidget);
}

CalendarView::~CalendarView()
{
}

void CalendarView::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    
    // If the app is running AND the QML file successfully loaded onto the screen
    if (mainApp && qmlWidget->rootObject()) {
        
        QDate gameDate = mainApp->getInternalDate(); 
        QList<CalendarEvent> rawEvents = mainApp->getCalendarEvents();
        QVariantList qmlEvents;
        
        for (const CalendarEvent& ev : rawEvents) {
            QVariantMap eventMap;
            eventMap["eventTitle"] = ev.title;
            eventMap["eventType"] = ev.type;
            
            eventMap["startYear"] = ev.start.date().year();
            eventMap["startMonth"] = ev.start.date().month() - 1; // Convert to JS 0-11
            eventMap["startDay"] = ev.start.date().day();
            
            eventMap["endYear"] = ev.end.date().year();
            eventMap["endMonth"] = ev.end.date().month() - 1;
            eventMap["endDay"] = ev.end.date().day();
            
            qmlEvents.append(eventMap);
        }

        // The Fix: Directly inject the data into the QML properties
        QQuickItem *root = qmlWidget->rootObject();
        root->setProperty("gameYear", gameDate.year());
        root->setProperty("gameMonth", gameDate.month() - 1);
        root->setProperty("gameDay", gameDate.day());
        
        // Snap the view to the current game month
        root->setProperty("currentYear", gameDate.year());
        root->setProperty("currentMonth", gameDate.month() - 1);
        
        // Hand over the event list
        root->setProperty("eventList", qmlEvents);
    }
}