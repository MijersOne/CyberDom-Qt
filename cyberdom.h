#ifndef CYBERDOM_H
#define CYBERDOM_H

#include <QMainWindow>
//#include "ui_assignments.h"
#include <QTimer>
#include <QDateTime>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QStack>
#include "rules.h"
#include "assignments.h"
#include "scriptparser.h"
#include "clothingitem.h"

// Forward declarations
class Assignments;
class ScriptParser;

// Define FlagData structure outside the class so it can be used by other classes
struct FlagData {
    QString name;
    QDateTime setTime;
    QDateTime expiryTime;
    QStringList groups;
    QString text;
};

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

    QSet<QString> assignedJobs;
    QSet<QString> getActiveJobs() { return activeAssignments; }

    QStringList getAvailableJobs();
    QMap<QString, QMap<QString, QString>> iniData;
    QMap<QString, QMap<QString, QString>> getIniData() const { return iniData; }
    QMap<QString, QDateTime> getJobDeadlines() const { return jobDeadlines; }
    QSet<QString> activeAssignments;

    void assignJobFromTrigger(QString section);
    void assignScheduledJobs();
    void addJobToAssignments(QString assignmentName);
    void addPunishmentToAssignments(QString punishmentName);
    void applyPunishment(int severity, const QString &group = QString());

    QString selectPunishmentFromGroup(int severity, const QString &group);

    void setFlag(const QString &flagName, int durationMinutes = 0);
    void removeFlag(const QString &flagName);
    bool isFlagSet(const QString &flagName) const;
    QStringList getFlagsByGroup(const QString &groupName) const;
    void setFlagGroup(const QString &groupName);
    void removeFlagGroup(const QString &groupName);

    const QMap<QString, FlagData>& getActiveFlags() const { return flags; }

    // Status Management
    void changeStatus(const QString &newStatus, bool isSubStatus = false);
    void returntoLastStatus();
    bool isInStatusGroup(const QString &groupName);

    void startAssignment(const QString &assignmentName, bool isPunishment, const QString &newStatus);
    void markAssignmentDone(const QString &assignmentName, bool isPunishment);
    void abortAssignment(const QString &assignmentName, bool isPunishment);
    void deleteAssignment(const QString &assignmentName, bool isPunishment);

    QString getSettingsFilePath() const { return settingsFile; }
    void removeJobDeadline(const QString &jobName) { jobDeadlines.remove(jobName); }
    
    // Creates a sample script file for testing
    void createSampleScriptFile();
    
    // Checks if the demo script exists and creates it if needed
    void checkDemoScriptExists();

    // Clothing management
    QString getCurrentClothingInstructions() const { return lastClothingInstructions; }
    QString getClothingReportPrompt() const;
    void processClothingReport(const QList<ClothingItem> &wearingItems, bool isNaked);
    void storeClothingReport(const QString &reportText);
    QStringList getClothingSets(const QString &setPrefix);
    QStringList getClothingOptions(const QString &setName);
    ScriptParser* getScriptParser() const { return scriptParser; }
    const QList<ClothingItem>& getClothingInventory() const { return clothingInventory; }
    void addClothingItem(const ClothingItem& item);

signals:
    void jobListUpdated();

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

    // New Methods for enhanced script parsing
    void loadAndParseScript(const QString &filePath);
    void applyScriptSettings();
    void setupInitialStatus();

    // void parseIniFile(); // Parses and stores the .ini file sections and keys
    void parseIncludeFiles(const QString &filePath); // Parses and stores the .inc files
    void loadIncludeFile(const QString &fileName); // Load the .inc files
    void initializeUiWithIniFile();
    void initializeProgressBarRange();
    void updateProgressBarValue();
    void loadIniFile();
    void parseAskPunishment();
    QString settingsFile; // Path to the separate user settings file
    QString currentStatus; // Store the current status

    QMap<QString, QDateTime> jobDeadlines;

    Ui::CyberDom *ui;
    Assignments *assignmentsWindow = nullptr;
    ScriptParser *scriptParser = nullptr; // New script parser
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

    QString lastInstructions; // Stores the last given instructions
    QString lastClothingInstructions; // Stores the last given clothing instructions
    QMap<QString, FlagData> flags; // Stores currently active flags

    // Status tracking for substatus
    QStack<QString> statusHistory; // For tracking status history for substatus
    QMap<QString, QStringList> statusGroups; // For mapping status groups to status names

    // New Methods
    void updateStatusText();
    void updateInstructions(const QString &instructions);
    void updateClothingInstructions(const QString &instructions);
    void updateAvailableActions(); // Updates UI based on the current status permissions
    void executeStatusEntryProcedures(const QString &statusName); // Run any procedures triggered by status change
    void updateStatusDisplay(); // Update status-related UI elements

    QTimer *punishmentTimer;

    bool testMenuEnabled = false; // Tracks if the Test Menu should be shown
    bool isPunishment = false;

    QTimer *flagTimer;

    QList<ClothingItem> clothingInventory;

    void runProcedure(const QString &procedureName);

    struct TimerInstance {
        QString name;
        QTime start;
        QTime end;
        QString message;
        QString sound;
        QString procedure;
        bool triggered = false;
    };
    QList<TimerInstance> activeTimers;

    QMap<QString, QString> questionAnswers;
    void loadQuestionAnswers();
    void saveQuestionAnswers();

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
    void checkPunishments();
    void checkFlagExpiry();
};

#endif // CYBERDOM_H
