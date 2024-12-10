#ifndef CYBERDOM_H
#define CYBERDOM_H

#include <QMainWindow>
#include "ui_assignments.h"
#include <QTimer>
#include <QDateTime>
#include "rules.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class CyberDom;
}
QT_END_NAMESPACE

class CyberDom : public QMainWindow
{
    Q_OBJECT

public:
    CyberDom(QWidget *parent = nullptr);
    ~CyberDom();

public slots:
    void openAssignmentsWindow(); // Slot function to open the Assignments Window
    void openTimeAddDialog(); // Slot function to open the TimeAdd Dialog
    void updateInternalClock(); // Slot to update the internal clock display

private:
    Ui::CyberDom *ui;
    QMainWindow *assignmentsWindow = nullptr; // Pointer to hold the Assignments Window instance
    Ui::Assignments assignmentsUi; // Instance of the Assignments UI class
    QTimer *clockTimer; // Timer to manage internal clock updates
    QDateTime internalClock; // Holds the current value of the internal clock

    Rules *rulesDialog; // Pointer to hold the Rules dialog instance

private slots:
    void applyTimeToClock(int days, int hours, int minutes, int seconds);
    void openAboutDialog(); // Slot to open the About dialog
    void openAskClothingDialog(); // Slot to open the AskClothing dialog
    void openAskInstructionsDialog(); // Slot to open the AskInstructions dialog
    void openReportClothingDialog(); // Slot to open the ReportClothing dialog
    void setupMenuConnections(); // Slot to open the Rules dialog
};

#endif // CYBERDOM_H
