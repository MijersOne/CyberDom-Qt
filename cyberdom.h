#ifndef CYBERDOM_H
#define CYBERDOM_H

#include <QMainWindow>
#include "ui_assignments.h"
#include <QTimer>
#include <QDateTime>
#include <QMap>
#include <QString>
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
    QString replaceVariables(const QString &input) const;

    int getMinMerits() const;
    int getMaxMerits() const;

public slots:
    void openAssignmentsWindow(); // Slot function to open the Assignments Window
    void openTimeAddDialog(); // Slot function to open the TimeAdd Dialog
    void updateInternalClock(); // Slot to update the internal clock display
    void updateStatus(const QString &newStatus); // Slot function to update the Status

private:
    QString promptForIniFile();
    void saveIniFilePath(const QString &filePath);
    QString loadIniFilePath();
    void initializeIniFile(); // Add this declaration
    void processIniValue(const QString &key);
    void parseIniFile(); // Parses and stores the .ini file sections and keys
    void parseIncludeFiles(const QString &filePath); // Parses and stores the .inc files
    void loadIncludeFile(const QString &fileName); // Load the .inc files
    void initializeUiWithIniFile();
    void initializeProgressBarRange();
    void updateProgressBarValue();
    void loadIniFile();
    void parseAskPunishment();
    QString settingsFile; // Path to the separate user settings file
    QString currentStatus; // Store the current status

    QMap<QString, QMap<QString, QString>> iniData;

    Ui::CyberDom *ui;
    QMainWindow *assignmentsWindow = nullptr; // Pointer to hold the Assignments Window instance
    Ui::Assignments assignmentsUi; // Instance of the Assignments UI class
    QTimer *clockTimer; // Timer to manage internal clock updates
    QDateTime internalClock; // Holds the current value of the internal clock
    QString currentIniFile; // Stores the path to the current loaded .ini file

    Rules *rulesDialog; // Pointer to hold the Rules dialog instance

    QString getIniValue(const QString &section, const QString &key, const QString &defaultValue = "") const;
    QString masterVariable; // Stores the value of "Master" from the .ini file
    QString subNameVariable; // Stores the value of "subName" from the .ini file

    int askPunishmentMin; // Minimum punishment value
    int askPunishmentMax; // Maximum punishment value
    int getMeritsFromIni();
    int minMerits;
    int maxMerits;
    int minPunishment;
    int maxPunishment;
    int getAskPunishmentMin() const;
    int getAskPunishmentMax() const;

    bool testMenuEnabled = false; // Tracks if the Test Menu should be shown

private slots:
    void applyTimeToClock(int days, int hours, int minutes, int seconds);
    void openAboutDialog(); // Slot to open the About dialog
    void openAskClothingDialog(); // Slot to open the AskClothing dialog
    void openAskInstructionsDialog(); // Slot to open the AskInstructions dialog
    void openReportClothingDialog(); // Slot to open the ReportClothing dialog
    void setupMenuConnections(); // Slot to open the Rules dialog
    void openAskPunishmentDialog(); // Slot to open the AskPunishment dialog
    void openChangeMeritsDialog(); // Slot to open the ChangeMerits dialog
    void openChangeStatusDialog(); // Slot to open the ChangeStatus dialog
    void openLaunchJobDialog(); // Slot to open the SelectJob dialog
    void openSelectPunishmentDialog(); // Slot to open the SelectPunishment dialog
    void openSelectPopupDialog(); // Slot to open the SelectPopup dialog
    void openListFlagsDialog(); // Slot to open the ListFlags dialog
    void openSetFlagsDialog(); // Slot to open the SetFlags dialog
    void openDeleteAssignmentsDialog(); // Slot to open the DeleteAssignments dialog
    void resetApplication(); // Slot to clear stored settings, notify the user of the reset, and exit the application so it can restart fresh
    void updateMerits(int newMerits);
};

#endif // CYBERDOM_H
