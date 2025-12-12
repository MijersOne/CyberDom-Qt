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
#include <QCamera>
#include <QMediaCaptureSession>
#include <QImageCapture>
#include <QMediaDevices>
#include <QCameraDevice>
#include "rules.h"
#include "assignments.h"
#include "scriptparser.h"
#include "clothingitem.h"
#include "datainspectordialog.h"

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

struct CalendarEvent {
    QDateTime start;
    QDateTime end;
    QString title;
    QString type;
};

struct DailyStats {
    QStringList jobsCompleted;
    QStringList punishmentsCompleted;
    QStringList outfitsWorn;
    int meritsGained = 0;
    int meritsLost = 0;
    QStringList permissionsAsked;
    QStringList reportsMade;
    QStringList confessionsMade;

    void reset() {
        jobsCompleted.clear();
        punishmentsCompleted.clear();
        outfitsWorn.clear();
        meritsGained = 0;
        meritsLost = 0;
        permissionsAsked.clear();
        reportsMade.clear();
        confessionsMade.clear();
    }
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

    // App Version
    static constexpr const char* APP_VERSION = "0.0.1";

    QString replaceVariables(const QString &input,
                             const QString &contextName = "",
                             const QString &contextTitle = "",
                             int contextSeverity = 0,
                             const QString &contextMinTime = "",
                             const QString &contextMaxTime = "",
                             const QDateTime &contextStartTime = QDateTime(),
                             const QDateTime &contextCreationTime = QDateTime(),
                             const QDateTime &contextDeadline = QDateTime(),
                             const QDateTime &contextNextRemind = QDateTime(),
                             const QDateTime &contextNextAllow = QDateTime()) const;

    int getMinMerits() const;
    int getMaxMerits() const;

    QSet<QString> assignedJobs;
    QSet<QString> getActiveJobs() { return activeAssignments; }

    QStringList getAssignmentResources(const QString &name, bool isPunishment) const;
    QSet<QString> getResourcesInUse() const;
    bool hasActiveBlockingPunishment() const;

    const QMap<QString, QMap<QString, QString>>& getIniData() const { return iniData; }

    QStringList getAvailableJobs();
    QMap<QString, QDateTime> getJobDeadlines() const { return jobDeadlines; }
    int getPunishmentAmount(const QString &name) const { return punishmentAmounts.value(name, 0); }
    bool isPermissionForbidden(const QString &name) const;
    QList<CalendarEvent> getCalendarEvents();
    QSet<QString> activeAssignments;

    void assignJobFromTrigger(QString section);
    void assignScheduledJobs();
    void addJobToAssignments(QString assignmentName, bool isAutoAssign = false);
    void addPunishmentToAssignments(const QString &punishmentName, int amount = 1);
    void applyPunishment(int severity, const QString &group = QString(), const QString &message = QString());

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

    bool startAssignment(const QString &assignmentName, bool isPunishment, const QString &newStatus);
    bool markAssignmentDone(const QString &assignmentName, bool isPunishment);
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
    QMap<QDate, QStringList> getHolidays() const;

    QString getAssignmentEstimate(const QString &assignmentName, bool isPunishment) const;

    // Procedure Management
    bool runProcedure(const QString &procedureName);

    // Punishment Management
    QString getAskPunishmentGroups() const;

    // Instructions
    QString resolveInstruction(const QString &name, QStringList *chosenItems = nullptr, QSet<QString> visited = QSet<QString>(), bool isClothingContext = false);

    QString getAssignmentDisplayName(const QString &assignmentName, bool isPunishment) const;

signals:
    void jobListUpdated();

public slots:
    void openAssignmentsWindow();
    void openTimeAddDialog();
    void updateInternalClock();
    void updateStatus(const QString &newStatus);
    void openReportClothingDialog(bool forced = false, const QString &title = "");
    void openAskClothingDialog(const QString &target = "");
    void openAskInstructionsDialog(const QString &target = "");
    void onMakeReportFile();
    void onViewReportFile();

private:
    // File management
    QString promptForIniFile();
    void saveIniFilePath(const QString &filePath);
    QString loadIniFilePath();
    void initializeIniFile();

    // Save variables from the script parser to a .cds file
    void saveVariablesToCDS(const QString &cdsPath);

    bool isAssignmentLongRunning(const QString &name, bool isPunishment) const;

    // Session management
    bool loadSessionData(const QString &path);
    void saveSessionData(const QString &path) const;
    bool isFirstSessionRun = false;

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
    int getMeritsFromIni() const;
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

    QMap<QString, FlagData> flags;
    QMap<QString, QDateTime> jobDeadlines;
    QMap<QString, QDateTime> jobExpiryTimes;
    QMap<QString, int> jobRemindIntervals; // seconds
    QMap<QString, QDateTime> jobNextReminds;
    QMap<QString, int> jobLateMerits;
    QMap<QString, int> punishmentAmounts; // amount units for each punishment
    QSet<QString> expiredAssignments;
    QMap<QString, QMap<QString, QString>> iniData;

    // Status tracking
    QStack<QString> statusHistory;
    QMap<QString, QStringList> statusGroups;

    // Timers
    QTimer *punishmentTimer;
    QTimer *flagTimer;
    QTimer *signinTimer;
    int signinRemainingSecs = -1;

    bool testMenuEnabled = false;
    QMenu *reportMenu = nullptr;
    QMenu *confessMenu = nullptr;
    QMenu *permissionMenu = nullptr;

    bool isPunishment = false;
    const PunishmentDefinition* getPunishmentDefinition(const QString &instanceName) const;

    // Clothing
    QList<ClothingItem> clothingInventory;
    QString lastClothingInstructions;
    QStringList requiredClothingChecks;
    QStringList forbiddenClothingChecks;
    bool itemMatchesCheck(const ClothingItem &item, const QString &checkName) const;
    void clearCurrentClothing();

    // Procedure handling
    void executeReport(const QString &name);
    void executeQuestion(const QString &questionKey, const QString &title);

    // Condition Handling
    bool evaluateCondition(const QString &condition);

    QString lastDisplayedStatusText;

    bool isInterruptAllowed() const;

    void applyDailyMerits();
    QDate lastDayMeritsGiven;

    void executePunish(const QString &severityValue, const QString &message, const QString &group);

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
    void populatePermissionMenu();

    int parseTimeToSeconds(const QString &timeStr) const;
    int parseTimeRangeToSeconds(const QString &range) const;
    QString formatDuration(int seconds) const;
    int randomIntFromRange(const QString &range) const;
    void incrementUsageCount(const QString &key);
    void setDefaultDeadlineForJob(const QString &jobName);

    bool isTimeAllowed(const QStringList &notBefore,
                       const QStringList &notAfter,
                       const QList<QPair<QString, QString>> &notBetween) const;

   // QList<ClothingItem> clothingIventory;

    QDate lastScheduledJobsRun;

    void performCounterOperation(const QString &opString, const QString &opType);

    bool checkInterruptableAssignments();

    int yellowMerits = -1;
    int redMerits = -1;

    void modifyMerits(const QString &value, const QString &opType);

    // Punishments
    QString askPunishmentGroups;

    // Permissions
    QMap<QString, QDateTime> permissionNextAvailable;
    QMap<QString, QDateTime> permissionFirstDenied;
    int calculateSecondsFromTimeRange(const TimeRange &tr);
    QSet<QString> permissionNotificationsPending;

    // Helper to check If/NotIf lists using ScriptUtils
    bool checkRequirements(const QList<QStringList> &ifGroups, const QList<QStringList> &notIfGroups);

    // Camera Components
    QCamera *camera = nullptr;
    QMediaCaptureSession *captureSession = nullptr;
    QImageCapture *imageCapture = nullptr;

    // Camera Helper Functions
    void setupCamera();
    void takePicture(const QString &filenamePrefix = "Auto");
    void triggerPointCamera(const QString &text, const QString &minInterval, const QString &maxInterval, const QString &sourceName);
    void triggerPoseCamera(const QString &text);
    void handleCameraTimer(const QString &sourceName);
    QMap<QString, QTimer*> activeCameraTimers;
    QMap<QString, QPair<QString, QString>> cameraIntervals;

    // Reports File
    DailyStats todayStats;
    void generateDailyReportFile(bool isEndOfDay);
    QString generateReportHtml(bool isEndOfDay);
    void trackPermissionEvent(const QString &name, const QString &result);

    // String Variables
    void executeStringAction(ScriptActionType type, const QString &value);
    QString lastReportFilename;
    QString lastReportName;
    QString lastPermissionName;
    QString lastQuestionAnswer;

    // Counter Variables
    void executeCounterAction(ScriptActionType type, const QString &value);
    void executeRandomCounterAction(const QString &value);
    QString getVariableValue(const QString &name, int contextSeverity = 0) const;
    int clothFaultsCount = 0;
    int lastPunishmentSeverity = 0;
    int lastLatenessMinutes = 0;
    int lastEarlyMinutes = 0;

    // Time Variables
    void executeTimeAction(ScriptActionType type, const QString &value);
    void executeTimeExtraction(ScriptActionType type, const QString &value);
    void executeCounterToTime(ScriptActionType type, const QString &value);
    QVariant getTimeVariableValue (const QString &name,
                                   const QString &contextMinTime = "",
                                   const QString &contextMaxTime = "",
                                   const QDateTime &contextStartTime = QDateTime(),
                                   const QDateTime &contextCreationTime = QDateTime(),
                                   const QDateTime &contextDeadline = QDateTime(),
                                   const QDateTime &contextNextRemind = QDateTime(),
                                   const QDateTime &contextNextAllow = QDateTime()) const;
    QDateTime scriptBeginTime;
    QDateTime sessionStartTime;
    QDateTime lastAppCloseTime;
    int lastResponseDuration;
    int lastFlagDuration = 0;
    QDateTime lastFlagExpiryTime;

private slots:
    void applyTimeToClock(int days, int hours, int minutes, int seconds);
    void openAboutDialog();
    void openAddClothingDialog();
    void setupMenuConnections();
    void openAskPunishmentDialog();
    void openReport(const QString &name);
    void openChangeMeritsDialog();
    void openChangeStatusDialog();
    void openLaunchJobDialog();
    void openCalendarView();
    void openSelectPunishmentDialog();
    void openSelectPopupDialog();
    void openListFlagsDialog();
    void openSetFlagsDialog();
    void openDeleteAssignmentsDialog();
    void openAskPermissionDialog();
    void openPermission(const QString &name);
    void openConfession(const QString &name);
    void resetApplication();
    void updateMerits(int newMerits);
    void checkPunishments();
    void checkFlagExpiry();
    void updateSigninTimer();
    void onResetSigninTimer();
    void openDataInspector();
    void onImageSaved(int id, const QString &fileName);
    void onCameraError(QCamera::Error value, const QString &description);
    void onImageCaptureError(int id, QImageCapture::Error error, const QString &errorString);
};

#endif // CYBERDOM_H
