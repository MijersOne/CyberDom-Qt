#ifndef CYBERDOM_H
#define CYBERDOM_H

#include <QMainWindow>
#include <QTimer>
#include <QDateTime>
#include <QMenu>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QStack>
#include <QSet>
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
    friend class SessionSaveLoadTest;

public:
    CyberDom(QWidget *parent = nullptr);
    ~CyberDom();
    QString replaceVariables(const QString &input) const;

    int getMinMerits() const;
    int getMaxMerits() const;

    QSet<QString> assignedJobs;
    QSet<QString> getActiveJobs() { return activeAssignments; }

    const QMap<QString, QMap<QString, QString>>& getIniData() const { return iniData; }

    QStringList getAvailableJobs();
    QMap<QString, QDateTime> getJobDeadlines() const { return jobDeadlines; }
    QSet<QString> activeAssignments;

    void assignJobFromTrigger(QString section);
    void assignScheduledJobs();
    void addJobToAssignments(QString assignmentName);
    void addPunishmentToAssignments(const QString &punishmentName, int amount = 1);
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
    void openAssignmentsWindow();
    void openTimeAddDialog();
    void updateInternalClock();
    void updateStatus(const QString &newStatus);

private:
    // File management
    QString promptForIniFile();
    void saveIniFilePath(const QString &filePath);
    QString loadIniFilePath();
    void initializeIniFile();

    // Save variables from the script parser to a .cds file
    void saveVariablesToCDS(const QString &cdsPath);

    // Session management
    bool loadSessionData(const QString &path);
    void saveSessionData(const QString &path) const;

    // Script initialization
    void loadAndParseScript(const QString &filePath);
    void applyScriptSettings();
    void setupInitialStatus();

    // UI initialization and updates
    void initializeUiWithIniFile();
    void initializeProgressBarRange();
    void updateProgressBarValue();

    // Get values from parsed script data
    QString getIniValue(const QString &section, const QString &key, const QString &defaultValue = "") const;
    int getMeritsFromIni();
    int getAskPunishmentMin() const;
    int getAskPunishmentMax() const;

    // Member variables
    Ui::CyberDom *ui;
    Assignments *assignmentsWindow = nullptr;
    ScriptParser *scriptParser = nullptr;
    QTimer *clockTimer;
    QDateTime internalClock;
    QString currentIniFile;
    QString settingsFile;
    QString sessionFilePath;
    QString currentStatus;

    Rules *rulesDialog;

    QString masterVariable;
    QString subNameVariable;

    int askPunishmentMin;
    int askPunishmentMax;
    int minMerits;
    int maxMerits;

    QString lastInstructions;
    QString lastClothingInstructions;

    QMap<QString, FlagData> flags;
    QMap<QString, QDateTime> jobDeadlines;
    QMap<QString, QDateTime> jobExpiryTimes;
    QMap<QString, int> jobRemindIntervals; // seconds
    QMap<QString, QDateTime> jobNextReminds;
    QMap<QString, int> jobLateMerits;
    QSet<QString> expiredAssignments;
    QMap<QString, QMap<QString, QString>> iniData;

    // Status tracking
    QStack<QString> statusHistory;
    QMap<QString, QStringList> statusGroups;

    // Timers
    QTimer *punishmentTimer;
    QTimer *flagTimer;
    QTimer *signinTimer;
    int signinRemainingSecs = 0;

    bool testMenuEnabled = false;
    QMenu *reportMenu = nullptr;
    QMenu *confessMenu = nullptr;

    bool isPunishment = false;

    QList<ClothingItem> clothingInventory;

    // Procedure handling
    void runProcedure(const QString &procedureName);
    void executeReport(const QString &name);

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

    QMap<QString, int> reportCounts;
    QMap<QString, int> confessionCounts;
    QMap<QString, int> permissionCounts;

    // UI update methods
    void updateStatusText();
    void updateInstructions(const QString &instructions);
    void updateClothingInstructions(const QString &instructions);
    void updateAvailableActions();
    void executeStatusEntryProcedures(const QString &statusName);
    void updateStatusDisplay();
    void updateSigninWidgetsVisibility(const StatusSection &status);
    void playSoundSafe(const QString &filePath);

    void populateReportMenu();
    void populateConfessMenu();

    int parseTimeToSeconds(const QString &timeStr) const;
    int parseTimeRangeToSeconds(const QString &range) const;
    int randomIntFromRange(const QString &range) const;
private slots:
    void applyTimeToClock(int days, int hours, int minutes, int seconds);
    void openAboutDialog();
    void openAskClothingDialog();
    void openAskInstructionsDialog();
    void openReportClothingDialog();
    void openAddClothingDialog();
    void setupMenuConnections();
    void openAskPunishmentDialog();
    void openReport(const QString &name);
    void openChangeMeritsDialog();
    void openChangeStatusDialog();
    void openLaunchJobDialog();
    void openSelectPunishmentDialog();
    void openSelectPopupDialog();
    void openListFlagsDialog();
    void openSetFlagsDialog();
    void openDeleteAssignmentsDialog();
    void openConfession(const QString &name);
    void resetApplication();
    void updateMerits(int newMerits);
    void checkPunishments();
    void checkFlagExpiry();
    void updateSigninTimer();
    void onResetSigninTimer();
};

#endif // CYBERDOM_H
