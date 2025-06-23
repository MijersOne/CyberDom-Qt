#ifndef SCRIPTDATA_H
#define SCRIPTDATA_H

#include <QString>
#include <QStringList>
#include <QMap>
#include <QDateTime>
#include <QStack>
#include <QSet>

enum class CaseMode {
    All,
    First,
    Random
};

enum class WhenType {
    When,
    WhenNot,
    WhenRandom,
    WhenAll,
    WhenNotAll,
    WhenAny,
    WhenNone
};

struct CaseBranch {
    WhenType type;
    QString condition;
    QStringList statements;
};

struct CaseBlock {
    CaseMode mode;
    QList<CaseBranch> branches;
};

struct ListLoopContext {
    QString listName;
    int currentIndex = 0;
    bool shouldLeave = false;
};

struct VariableState {
    QSet<QString> flags;
    QMap<QString, QString> strings;
    QMap<QString, int> counters;
    QMap<QString, QDateTime> times;
};

struct FTPSettings {
    QString url;
    QString user;
    QString password;
    QString serverType;
    QString directory;

    QString updateScript;
    QString updateProgram;

    bool sendReports = false;
    bool ftpLog = false;
    bool testFtp = true;

    bool sendPictures = false;
};

struct FlagDefinition {
    QString name;
    QStringList textLines;
    QString group;
    QString durationMin;
    QString durationMax;
    QString expireMessage;
    QString expireProcedure;
    QString setProcedure;
    QString removeProcedure;
    bool reportFlag = true;
};

struct EventHandlers {
    // Program lifecycle
    QString firstRun;
    QString openProgram;
    QString closeProgram;
    QString startFromPause;
    QString deleteStatus;
    QString minimize;
    QString restore;

    // Report lifecycle
    QString beforeNewReport;
    QString afterNewReport;
    QString afterReport;
    QString forgetConfession;
    QString ignoreConfession;

    // Permission
    QString permissionGiven;
    QString permissionDenied;

    // Punishments
    QString punishmentGiven;
    QString punishmentAsked;
    QString punishmentDone;

    // Jobs
    QString jobAnnounced;
    QString jobDone;

    // Assignments
    QString beforeDelete;

    // Clothing
    QString beforeClothReport;
    QString afterClothReport;
    QString checkOn;
    QString checkOff;
    QString checkAll;

    // Other
    QString signIn;
    QString meritsChanged;
    QString autoAssignEnd;
    QString autoAssignNone;
    QString mailFailure;
    QString newDay;

    // Status changes
    QString beforeStatusChange;
    QString afterStatusChange;
};

struct TimeWindowControl {
    QString earliestMin;
    QString earliestMax;
    QString latestMin;
    QString latestMax;

    int earlyPenalty1 = 0;
    double earlyPenalty2 = 0.0;
    QString earlyPenaltyGroup;
    QString earlyProcedure;

    int latePenalty1 = 0;
    double latePenalty2 = 0.0;
    QString latePenaltyGroup;
    QString lateProcedure;
};

struct DurationControl {
    QString minTimeStart;
    QString minTimeEnd;
    QString maxTimeStart;
    QString maxTimeEnd;

    int quickPenalty1 = 0;
    double quickPenalty2 = 0.0;
    QString quickPenaltyGroup;
    QString quickMessage;
    QString quickProcedure;

    int slowPenalty1 = 0;
    double slowPenalty2 = 0.0;
    QString slowPenaltyGroup;
    QString slowMessage;
    QString slowProcedure;

    QString minTimeProcedure;
    QString maxTimeProcedure;
};

struct TimerDefinition {
    QString name;
    QString startTimeMin;
    QString startTimeMax;
    QString endTime;

    QStringList preStatuses;
    QStringList ifFlags;
    QStringList notIfFlags;
    QStringList notBefore;
    QStringList notAfter;
    QList<QPair<QString, QString>> notBetween;

    DurationControl duration;

    QStringList notBeforeTimes;
    QStringList notAfterTimes;
    QList<QPair<QString, QString>> notBetweenTimes;

    QString masterMailSubject;
    QStringList masterAttachments;
    QStringList subMailLines;

    QList<CaseBlock> cases;
};

enum class MessageSelectMode {
    All,
    Random
};

struct MessageGroup {
    MessageSelectMode mode = MessageSelectMode::All;
    QStringList messages;
};

struct ProcedureCall {
    QString name;
    int weight = 1;
};

enum class ProcedureSelectMode {
    All,
    First,
    Random
};

struct ProcedureDefinition {
    QString name;
    QString title;

    ProcedureSelectMode selectMode = ProcedureSelectMode::All;
    QList<ProcedureCall> calls;

    QStringList preStatuses;
    QStringList notBefore;
    QStringList notAfter;
    QList<QPair<QString, QString>> notBetween;

    QStringList statusTexts;
    QList<MessageGroup> messages;

    QStringList inputQuestions;
    QString noInputProcedure;
    QStringList advancedQuestions;

    TimeWindowControl timeWindow;

    QStringList notBeforeTimes;
    QStringList notAfterTimes;
    QList<QPair<QString, QString>> notBetweenTimes;

    QString autoAssignMode;
    QString autoAssignValue;
    bool stopAutoAssign = false;

    QString clothingInstruction;
    bool clearClothingCheck = false;

    QString masterMailSubject;
    QStringList masterAttachments;
    QStringList subMailLines;

    QStringList clearFlags;
    QStringList setStringVars;
    QStringList setCounterVars;
    QStringList incrementCounters;
    QStringList decrementCounters;
    QStringList randomCounters;
    QStringList randomStrings;
    QStringList setCounters;
    QStringList addCounters;
    QStringList subtractCounters;
    QStringList multiplyCounters;
    QStringList divideCounters;
    QStringList dropCounters;
    QStringList inputCounters;
    QStringList inputNegCounters;

    QStringList setFlags;
    QStringList removeFlags;
    QStringList setFlagGroups;
    QStringList removeFlagGroups;
    QStringList ifFlags;
    QStringList notIfFlags;
    QStringList denyIfFlags;
    QStringList permitIfFlags;

    QStringList setStrings;
    QStringList inputStrings;
    QStringList inputLongStrings;
    QStringList dropStrings;

    QStringList setTimeVars;
    QStringList addTimeVars;
    QStringList subtractTimeVars;
    QStringList multiplyTimeVars;
    QStringList divideTimeVars;
    QStringList roundTimeVars;
    QStringList dropTimeVars;
    QStringList inputDateVars;
    QStringList inputTimeVars;
    QStringList inputIntervalVars;
    QStringList inputDateDefVars;
    QStringList inputTimeDefVars;
    QStringList randomTimeVars;
    QStringList addDaysVars;
    QStringList subtractDaysVars;
    QStringList extractToCounter; // e.g. Days#=#counter,!timevar
    QStringList convertFromCounter; // e.g. Days!=!timevar,#counter

    QStringList ifConditions;
    QStringList notIfConditions;
    QStringList denyIfConditions;
    QStringList permitIfConditions;

    QList<CaseBlock> cases;
};

struct QuestionAnswerBlock {
    QString answerText;
    QString procedureName;
    bool hasInlineActions = false;
    QString variableValue;

    // Inline actions
    QList<MessageGroup> messages;
    QList<ProcedureCall> procedureCalls;
    int punishMin = 0;
    int punishMax = 0;
};

struct QuestionDefinition {
    QString name;
    QString phrase;
    QList<QuestionAnswerBlock> answers;
    QString variable;
    QString text;
};

struct AssignmentBehavior {
    int addMerit = 0;
    bool longRunning = false;
    bool mustStart = false;
    bool interruptable = true;

    QString startFlag;
    QString newStatus;

    QString announceProcedure;
    QString remindProcedure;
    QString beforeProcedure;
    QString startProcedure;
    QString abortProcedure;
    QString beforeDoneProcedure;
    QString doneProcedure;
    QString deleteProcedure;
    QString beforeDeleteProcedure;

    bool deleteAllowed = false;

    QList<MessageGroup> messages;
    QStringList statusTexts;
};

struct MeritAdjustment {
    int add = 0;
    int subtract = 0;
    int set = -1;
};

struct GeneralSettings {
    QString masterName;
    QStringList subNames;
    QString minVersion;
    QString version;
    int reportTimeFormat = 24;
    bool forgetConfessionEnabled = true;
    bool ignoreConfessionEnabled = true;
    int forgetPenaltyMin = 0;
    int forgetPenaltyMax = 0;
    QString forgetPenaltyGroup;
    int ignorePenaltyMin = 0;
    int ignorePenaltyMax = 0;
    QString ignorePenaltyGroup;
    QStringList interruptStatuses;
    int minMerits = 0;
    int maxMerits = 1000;
    int yellowMerits = -1;
    int redMerits = -1;
    int dayMerits = 0;
    bool hideMerits = false;
    QString reportPassword;
    bool restrictMode = false;
    bool autoEncrypt = false;
    bool centerRandom = false;
    bool allowClothExport = false;
    bool allowClothReport = true;
    QString popupAlarm;
    QStringList topText;
    QStringList bottomText;

    QString popupMessage;

    int signinPenalty1 = 0;
    int signinPenalty2 = 0;
    QString signinPenaltyGroup;

    bool autoClothFlags = false;

    // SMTP Settings
    QString smtpServer;
    QString smtpUser;
    QString smtpPassword;
    QString senderEmail;
    QString subEmail;
    QString masterEmail;
    QString masterEmail2;
    int smtpPort = 25;
    int tlsPort = 0;

    // Email Behavior
    bool autoMailReport = false;
    bool showMailWindow = true;
    bool testMail = false;
    bool mailLog = false;

    QString maxLineBreak;

    bool directShow = false;
    QString cameraFolder;
    QString savePictures;

    // Media
    QStringList quickReportLabels;
    QString openPassword;
    QString quickReport;
    QString quickLabel;
    QString doneLabel;

    // Font settings
    int textSize = 8;
    int buttonSize = 8;
    int border = 0;

    // Icon/Tray
    bool useIcon = false;
    bool startMinimized = false;
    QString minimizePopup;

    // Rules
    bool rulesEnabled = true;

    // Misc
    QString programAction;

    // Flags
    QString flagOnText;
    QString flagOffText;

    bool hideTimeGlobal = false;
};

struct InitSettings {
    QString newStatus;
    int merits = -1;
};

struct EventSettings {
    QString firstRunProcedure;
    QString signIn;
};

struct StatusDefinition {
    QString name;
    QString title;
    QString group;
    bool isSubStatus = false;
    bool reportsOnly = false;
    bool assignmentsEnabled = true;
    bool permissionsEnabled = true;
    bool confessionsEnabled = true;
    bool reportsEnabled = true;
    bool rulesEnabled = true;
    QString endReport;
    bool allowClothReport = true;
    QString popupIntervalMin;
    QString popupIntervalMax;
    QStringList popupIfFlags;
    QStringList noPopupIfFlags;
    QString popupGroup;
    QString popupAlarm;
    QStringList statusTexts;
    QList<MessageGroup> messages;

    QStringList inputQuestions;
    QString noInputProcedure;
    QStringList advancedQuestions;

    DurationControl duration;

    QString signinIntervalMin;
    QString signinIntervalMax;

    int signinPenalty1 = 0;
    int signinPenalty2 = 0;
    QString signinPenaltyGroup;

    bool allowAutoAssign = false;

    QString cameraPrompt;
    QString cameraIntervalMin;
    QString cameraIntervalMax;

    QStringList pictureList;
    QStringList localPictureList;
    QString statisticsLabel;
    bool trackStatistics = false;
    bool isAway = false;
    bool rulesOverride = true;
    QString quickReportName;

    QStringList clearFlags;
    QStringList setStringVars;
    QStringList setCounterVars;
    QStringList incrementCounters;
    QStringList decrementCounters;
    QStringList randomCounters;
    QStringList randomStrings;
    QStringList setCounters;
    QStringList addCounters;
    QStringList subtractCounters;
    QStringList multiplyCounters;
    QStringList divideCounters;
    QStringList dropCounters;
    QStringList inputCounters;
    QStringList inputNegCounters;

    QStringList setFlags;
    QStringList removeFlags;
    QStringList setFlagGroups;
    QStringList removeFlagGroups;
    QStringList ifFlags;
    QStringList notIfFlags;
    QStringList denyIfFlags;
    QStringList permitIfFlags;

    QStringList setStrings;
    QStringList inputStrings;
    QStringList inputLongStrings;
    QStringList dropStrings;

    QStringList setTimeVars;
    QStringList addTimeVars;
    QStringList subtractTimeVars;
    QStringList multiplyTimeVars;
    QStringList divideTimeVars;
    QStringList roundTimeVars;
    QStringList dropTimeVars;
    QStringList inputDateVars;
    QStringList inputTimeVars;
    QStringList inputIntervalVars;
    QStringList inputDateDefVars;
    QStringList inputTimeDefVars;
    QStringList randomTimeVars;
    QStringList addDaysVars;
    QStringList subtractDaysVars;
    QStringList extractToCounter; // e.g. Days#=#counter,!timevar
    QStringList convertFromCounter; // e.g. Days!=!timevar,#counter

    QStringList ifConditions;
    QStringList notIfConditions;
    QStringList denyIfConditions;
    QStringList permitIfConditions;

    QString poseCameraText;

    bool hideTime = false;
    bool hasHideTime = false;
};

struct ReportDefinition {
    QString name;
    QString title;
    bool onTop = false;
    bool showInMenu = true;
    QStringList preStatuses;

    MeritAdjustment merits;

    bool centerRandom = false;

    QString group;

    QStringList statusTexts;
    QList<MessageGroup> messages;

    QStringList inputQuestions;
    QString noInputProcedure;
    QStringList advancedQuestions;

    TimeWindowControl timeWindow;

    QStringList notBeforeTimes;
    QStringList notAfterTimes;
    QList<QPair<QString, QString>> notBetweenTimes;

    QString autoAssignMode;
    QString autoAssignValue;
    bool stopAutoAssign = false;

    QString clothingInstruction;
    bool clearClothingCheck = false;

    QString masterMailSubject;
    QStringList masterAttachments;
    QStringList subMailLines;

    QStringList soundFiles;
    QStringList localSoundFiles;
    QStringList writeReportLines;
    QStringList showPictures;
    QStringList showLocalPictures;
    QStringList removePictures;
    QStringList removeLocalPictures;
    QString launchCommand;
    QString programAction;
    bool makeNewReport = false;
    bool makeNewReportSilent = false;

    QStringList clearFlags;
    QStringList setStringVars;
    QStringList setCounterVars;
    QStringList incrementCounters;
    QStringList decrementCounters;
    QStringList randomCounters;
    QStringList randomStrings;
    QStringList setCounters;
    QStringList addCounters;
    QStringList subtractCounters;
    QStringList multiplyCounters;
    QStringList divideCounters;
    QStringList dropCounters;
    QStringList inputCounters;
    QStringList inputNegCounters;

    QStringList setFlags;
    QStringList removeFlags;
    QStringList setFlagGroups;
    QStringList removeFlagGroups;
    QStringList ifFlags;
    QStringList notIfFlags;
    QStringList denyIfFlags;
    QStringList permitIfFlags;

    QStringList setStrings;
    QStringList inputStrings;
    QStringList inputLongStrings;
    QStringList dropStrings;

    QStringList setTimeVars;
    QStringList addTimeVars;
    QStringList subtractTimeVars;
    QStringList multiplyTimeVars;
    QStringList divideTimeVars;
    QStringList roundTimeVars;
    QStringList dropTimeVars;
    QStringList inputDateVars;
    QStringList inputTimeVars;
    QStringList inputIntervalVars;
    QStringList inputDateDefVars;
    QStringList inputTimeDefVars;
    QStringList randomTimeVars;
    QStringList addDaysVars;
    QStringList subtractDaysVars;
    QStringList extractToCounter; // e.g. Days#=#counter,!timevar
    QStringList convertFromCounter; // e.g. Days!=!timevar,#counter

    QMap<QString, QStringList> lists;
    QMultiMap<QString, QStringList> listCommands;

    QList<CaseBlock> cases;

    QStringList listSets;
    QStringList listSetSplits;
    QStringList listAdds;
    QStringList listAddNoDubs;
    QStringList listAddSplits;
    QStringList listPushes;
    QStringList listRemoves;
    QStringList listRemoveAlls;
    QStringList listPulls;
    QStringList listIntersects;
    QStringList listClears;
    QStringList listDrops;
    QStringList listSorts;
};

struct ConfessionDefinition {
    QString name;
    QString title;
    bool onTop = false;
    bool showInMenu = true;
    QStringList preStatuses;

    MeritAdjustment merits;

    bool centerRandom  = false;

    QString group;

    QStringList statusTexts;
    QList<MessageGroup> messages;

    QStringList inputQuestions;
    QString noInputProcedure;
    QStringList advancedQuestions;

    TimeWindowControl timeWindow;

    QStringList notBeforeTimes;
    QStringList notAfterTimes;
    QList<QPair<QString, QString>> notBetweenTimes;

    QString masterMailSubject;
    QStringList masterAttachments;
    QStringList subMailLines;

    QStringList setFlags;
    QStringList clearFlags;
    QStringList setStringVars;
    QStringList setCounterVars;
    QStringList incrementCounters;
    QStringList decrementCounters;
    QStringList randomCounters;
    QStringList randomStrings;
    QStringList setCounters;
    QStringList addCounters;
    QStringList subtractCounters;
    QStringList multiplyCounters;
    QStringList divideCounters;
    QStringList dropCounters;
    QStringList inputCounters;
    QStringList inputNegCounters;

    QStringList setTimeVars;
    QStringList addTimeVars;
    QStringList subtractTimeVars;
    QStringList multiplyTimeVars;
    QStringList divideTimeVars;
    QStringList roundTimeVars;
    QStringList dropTimeVars;
    QStringList inputDateVars;
    QStringList inputTimeVars;
    QStringList inputIntervalVars;
    QStringList inputDateDefVars;
    QStringList inputTimeDefVars;
    QStringList randomTimeVars;
    QStringList addDaysVars;
    QStringList subtractDaysVars;
    QStringList extractToCounter; // e.g. Days#=#counter,!timevar
    QStringList convertFromCounter; // e.g. Days!=!timevar,#counter

    QStringList ifConditions;
    QStringList notIfConditions;
    QStringList denyIfConditions;
    QStringList permitIfConditions;
};

struct TimeRange {
    QString min;
    QString max;
};

struct PermissionDefinition {
    QString name;
    QString title;
    int pct = 0;
    TimeRange minInterval;
    TimeRange delay;
    TimeRange maxWait;

    int notify = 0;

    MeritAdjustment merits;

    bool centerRandom = false;

    QString group;

    QStringList statusTexts;
    QList<MessageGroup> messages;

    QStringList inputQuestions;
    QString noInputProcedure;
    QStringList advancedQuestions;

    // Time Control
    QString denyBeforeStart;
    QString denyBeforeEnd;
    QString denyAfterStart;
    QString denyAfterEnd;
    QList<QPair<QString, QString>> denyBetweenRanges;

    // Merit Control
    int denyBelowMin = -1;
    int denyBelowMax = -1;
    int denyAboveMin = -1;
    int denyAboveMax = -1;

    // Advanced Randomization
    bool pctIsVariable = false;
    int pctMin = -1;
    int pctMax = -1;
    int highMeritsMin = -1;
    int highMeritsMax = -1;
    int highPctMin = -1;
    int highPctMax = -1;
    int lowMeritsMin = -1;
    int lowMeritsMax = -1;
    int lowPctMin = -1;
    int lowPctMax = -1;

    // Permission Logic Hooks
    QString beforeProcedure;
    QString permitMessage;
    QString denyProcedure;
    QStringList denyFlags;
    QString denyMessage;
    QString denyLaunch;
    QString denyStatus;

    TimeWindowControl timeWindow;

    QStringList notBeforeTimes;
    QStringList notAfterTimes;
    QList<QPair<QString, QString>> notBetweenTimes;

    QString masterMailSubject;
    QStringList masterAttachments;
    QStringList subMailLines;

    QString poseCameraText;
    QString pointCameraText;

    QStringList clearFlags;
    QStringList setStringVars;
    QStringList setCounterVars;
    QStringList incrementCounters;
    QStringList decrementCounters;
    QStringList randomCounters;
    QStringList randomStrings;
    QStringList setCounters;
    QStringList addCounters;
    QStringList subtractCounters;
    QStringList multiplyCounters;
    QStringList divideCounters;
    QStringList dropCounters;
    QStringList inputCounters;
    QStringList inputNegCounters;

    QStringList setFlags;
    QStringList removeFlags;
    QStringList setFlagGroups;
    QStringList removeFlagGroups;
    QStringList ifFlags;
    QStringList notIfFlags;
    QStringList denyIfFlags;
    QStringList permitIfFlags;

    QStringList setStrings;
    QStringList inputStrings;
    QStringList inputLongStrings;
    QStringList dropStrings;

    QStringList setTimeVars;
    QStringList addTimeVars;
    QStringList subtractTimeVars;
    QStringList multiplyTimeVars;
    QStringList divideTimeVars;
    QStringList roundTimeVars;
    QStringList dropTimeVars;
    QStringList inputDateVars;
    QStringList inputTimeVars;
    QStringList inputIntervalVars;
    QStringList inputDateDefVars;
    QStringList inputTimeDefVars;
    QStringList randomTimeVars;
    QStringList addDaysVars;
    QStringList subtractDaysVars;
    QStringList extractToCounter; // e.g. Days#=#counter,!timevar
    QStringList convertFromCounter; // e.g. Days!=!timevar,#counter

    QStringList ifConditions;
    QStringList notIfConditions;
    QStringList denyIfConditions;
    QStringList permitIfConditions;

    QMap<QString, QStringList> lists;
    QMultiMap<QString, QStringList> listCommands;
};

struct PunishmentDefinition : public AssignmentBehavior {
    QString name;
    QString title;
    QString valueUnit = "once";
    double value = 1.0;
    int min = 1;
    int max = 20;
    int minSeverity = 0;
    int maxSeverity = 0;
    int weightMin = 1;
    int weightMax = 1;
    QStringList groups;
    bool groupOnly = false;
    bool longRunning = false;
    bool mustStart = false;
    bool accumulative = false;
    QString respite;
    QString estimate;
    QStringList forbids;

    MeritAdjustment merits;

    bool centerRandom = false;

    QStringList statusTexts;
    QList<MessageGroup> messages;

    QStringList inputQuestions;
    QString noInputProcedure;
    QStringList advancedQuestions;

    DurationControl duration;

    QStringList resources;

    int autoAssignMin = 0;
    int autoAssignMax = 0;

    QString clothingInstruction;
    bool clearClothingCheck = false;

    bool isLineWriting = false;
    QStringList lines;
    QString lineSelectMode;
    int lineCount = 0;
    QString pageSizeMin;
    QString pageSizeMax;

    bool isDetention = false;
    QStringList detentionText;

    QString poseCameraText;
    QString pointCameraText;

    QStringList clearFlags;
    QStringList setStringVars;
    QStringList setCounterVars;
    QStringList incrementCounters;
    QStringList decrementCounters;
    QStringList randomCounters;
    QStringList randomStrings;
    QStringList setCounters;
    QStringList addCounters;
    QStringList subtractCounters;
    QStringList multiplyCounters;
    QStringList divideCounters;
    QStringList dropCounters;
    QStringList inputCounters;
    QStringList inputNegCounters;

    QStringList setFlags;
    QStringList removeFlags;
    QStringList setFlagGroups;
    QStringList removeFlagGroups;
    QStringList ifFlags;
    QStringList notIfFlags;
    QStringList denyIfFlags;
    QStringList permitIfFlags;

    QStringList setStrings;
    QStringList inputStrings;
    QStringList inputLongStrings;
    QStringList dropStrings;

    QStringList setTimeVars;
    QStringList addTimeVars;
    QStringList subtractTimeVars;
    QStringList multiplyTimeVars;
    QStringList divideTimeVars;
    QStringList roundTimeVars;
    QStringList dropTimeVars;
    QStringList inputDateVars;
    QStringList inputTimeVars;
    QStringList inputIntervalVars;
    QStringList inputDateDefVars;
    QStringList inputTimeDefVars;
    QStringList randomTimeVars;
    QStringList addDaysVars;
    QStringList subtractDaysVars;
    QStringList extractToCounter; // e.g. Days#=#counter,!timevar
    QStringList convertFromCounter; // e.g. Days!=!timevar,#counter

    QStringList ifConditions;
    QStringList notIfConditions;
    QStringList denyIfConditions;
    QStringList permitIfConditions;
};

struct JobDefinition : public AssignmentBehavior {
    QString name;
    QString title;
    QString text;

    int intervalMin = 0;
    int intervalMax = 0;
    int firstIntervalMin = 0;
    int firstIntervalMax = 0;

    QStringList runDays;
    QStringList noRunDays;

    QString endTimeMin;
    QString endTimeMax;

    QString respite;
    QString estimate;

    QString remindIntervalMin;
    QString remindIntervalMax;

    int remindPenaltyMin = 0;
    int remindPenaltyMax = 0;
    QString remindPenaltyGroup;

    int lateMeritsMin = 0;
    int lateMeritsMax = 0;

    QString expireAfterMin;
    QString expireAfterMax;
    int expirePenaltyMin = 0;
    int expirePenaltyMax = 0;
    QString expirePenaltyGroup;
    QString expireProcedure;

    bool oneTime = false;
    bool announce = true;

    MeritAdjustment merits;

    bool centerRandom = false;

    QStringList statusTexts;
    QList<MessageGroup> messages;

    DurationControl duration;

    QStringList resources;

    int autoAssignMin = 0;
    int autoAssignMax = 0;

    QString clothingInstruction;
    bool clearClothingCheck = false;

    bool isLineWriting = false;
    QStringList lines;
    QString lineSelectMode;
    int lineCount = 0;
    QString pageSizeMin;
    QString pageSizeMax;

    QString poseCameraText;
    QString pointCameraText;

    QStringList clearFlags;
    QStringList setStringVars;
    QStringList setCounterVars;
    QStringList incrementCounters;
    QStringList decrementCounters;
    QStringList randomCounters;
    QStringList randomStrings;
    QStringList setCounters;
    QStringList addCounters;
    QStringList subtractCounters;
    QStringList multiplyCounters;
    QStringList divideCounters;
    QStringList dropCounters;
    QStringList inputCounters;
    QStringList inputNegCounters;

    QStringList setFlags;
    QStringList removeFlags;
    QStringList setFlagGroups;
    QStringList removeFlagGroups;
    QStringList ifFlags;
    QStringList notIfFlags;
    QStringList denyIfFlags;
    QStringList permitIfFlags;

    QStringList setStrings;
    QStringList inputStrings;
    QStringList inputLongStrings;
    QStringList dropStrings;

    QStringList setTimeVars;
    QStringList addTimeVars;
    QStringList subtractTimeVars;
    QStringList multiplyTimeVars;
    QStringList divideTimeVars;
    QStringList roundTimeVars;
    QStringList dropTimeVars;
    QStringList inputDateVars;
    QStringList inputTimeVars;
    QStringList inputIntervalVars;
    QStringList inputDateDefVars;
    QStringList inputTimeDefVars;
    QStringList randomTimeVars;
    QStringList addDaysVars;
    QStringList subtractDaysVars;
    QStringList extractToCounter; // e.g. Days#=#counter,!timevar
    QStringList convertFromCounter; // e.g. Days!=!timevar,#counter

    QStringList ifConditions;
    QStringList notIfConditions;
    QStringList denyIfConditions;
    QStringList permitIfConditions;

    QStringList listSets;
    QStringList listSetSplits;
    QStringList listAdds;
    QStringList listAddNoDubs;
    QStringList listAddSplits;
    QStringList listPushes;
    QStringList listRemoves;
    QStringList listRemoveAlls;
    QStringList listPulls;
    QStringList listIntersects;
    QStringList listClears;
    QStringList listDrops;
    QStringList listSorts;
};

struct AssignmentAction {
    QStringList markDone;
    QStringList abort;
    QStringList deleteAssignments;
};

struct InstructionOption {
    QString text;
    bool hidden = false;
    bool skip = false;
    int weight = 1;
    QString optionSet;
    QStringList optionFlags;
    QStringList check;
    QStringList checkOff;
};

struct InstructionChoice {
    QString name;
    QList<InstructionOption> options;
};

enum class InstructionChangeMode {
    Daily,
    Program,
    Always
};

enum class InstructionSelectMode {
    All,
    First,
    Random
};

struct InstructionSet {
    QString name;
    QList<InstructionChoice> choices;
    QList<QString> includedSets;
    int weight = 1;
    QStringList ifFlags;
    QStringList notIfFlags;
    QStringList ifChosen;
    QStringList ifNotChosen;
    InstructionSelectMode selectMode = InstructionSelectMode::All;
    QStringList setFlags;
};

struct InstructionDefinition {
    QString name;
    QString title;
    bool isClothing = false;
    bool askable = true;
    InstructionChangeMode changeMode = InstructionChangeMode::Daily;
    InstructionSelectMode selectMode = InstructionSelectMode::All;
    QString noneText;

    QList<InstructionSet> sets;

    QStringList statusTexts;

    QList<MessageGroup> messages;

    QString clothingInstruction;
    bool clearClothingCheck = false;

    QList<CaseBlock> cases;
};

struct ClothingAttribute {
    QString name;
    QStringList values;
    bool allowCustom = false;
};

struct ClothingTypeDefinition {
    QString name;
    QString title;
    QList<ClothingAttribute> attributes;
    QStringList checks;
    QMap<QString, QStringList> valueChecks;
    QMap<QString, QStringList> valueFlags;
    QStringList topLevelFlags;
};

struct PopupDefinition {
    QString name;
    QString title;
    QStringList preStatuses;
    QStringList ifFlags;
    QStringList notIfFlags;
    int weightMin = 1;
    int weightMax = 1;
    QString alarm;
    QStringList groups;
    bool groupOnly = false;

    DurationControl duration;

    QString popupMessage;

    QStringList notBeforeTimes;
    QStringList notAfterTimes;
    QList<QPair<QString, QString>> notBetweenTimes;

    QString masterMailSubject;
    QStringList masterAttachments;
    QStringList subMailLines;

    QList<CaseBlock> cases;
};

struct PopupGroupDefinition {
    QString name;
    QString popupIntervalMin;
    QString popupIntervalMax;

    QStringList popupIfFlags;
    QStringList noPopupIfFlags;
    QString popupAlarm;
    bool groupOnly = false;

    QString popupMinTime;
    int quickPenalty1 = 0;
    int quickPenalty2 = 0;
    QString quickMessage;
    int slowPenalty1 = 0;
    int slowPenalty2 = 0;
    QString slowMessage;
    QString quickProcedure;
    QString slowProcedure;

    QStringList statusTexts;

    DurationControl duration;

    QString popupMessage;
};

struct ScriptData {
    GeneralSettings general;
    QMap<QString, QString> generalSettings;
    QMap<QString, QMap<QString, QStringList>> rawSections;
    InitSettings init;
    EventSettings events;
    QMap<QString, StatusDefinition> statuses;
    QMap<QString, ReportDefinition> reports;
    QMap<QString, ConfessionDefinition> confessions;
    QMap<QString, PermissionDefinition> permissions;
    QMap<QString, PunishmentDefinition> punishments;
    QMap<QString, JobDefinition> jobs;
    QMap<QString, InstructionDefinition> instructions;
    QMap<QString, ClothingTypeDefinition> clothingTypes;
    bool allowClothExport = false;
    bool allowClothImport = true;
    QMap<QString, ProcedureDefinition> procedures;
    QMap<QString, PopupDefinition> popups;
    QMap<QString, PopupGroupDefinition> popupGroups;
    QMap<QString, TimerDefinition> timers;
    QMap<QString, QuestionDefinition> questions;
    EventHandlers eventHandlers;
    FTPSettings ftpSettings;
    VariableState variables;
    QMap<QString, FlagDefinition> flagDefinitions;
    QMap<QString, QString> stringVariables;
    QMap<QString, QDateTime> timeVariables;
    QStringList ifConditions;
    QStringList notIfConditions;
    QStringList denyIfConditions;
    QStringList permitIfConditions;
    QStack<ListLoopContext> activeLoops;
    QMap<QString, QStringList> lists;
};

#endif // SCRIPTDATA_H
