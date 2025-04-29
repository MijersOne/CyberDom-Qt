#ifndef SCRIPTPARSER_H
#define SCRIPTPARSER_H

#include <QString>
#include <QStringList>
#include <QMap>
#include <QVariant>
#include <QDateTime>
#include <QDebug>

// Forward Declarations
class ScriptParser;
class CyberDom;

// Base struct for all section types
struct ScriptSection {
    QString name;
    QString type;
    QMap<QString, QStringList> keyValues; // Some keys can have multiple values

    // Helper to get a single value (first one if multiple exist)
    QString getValue(const QString &key, const QString &defaultValue = "") const {
        if (!keyValues.contains(key) || keyValues[key].isEmpty())
            return defaultValue;
        return keyValues[key].first();
    }

    // Helper to get all values for a key
    QStringList getValues(const QString &key) const {
        return keyValues.value(key, QStringList());
    }

    // Helper to check if a key exists
    bool hasKey(const QString &key) const {
        return keyValues.contains(key);
    }
};

// Specialized section for Status
struct StatusSection : public ScriptSection {
    QStringList groups;
    bool isSubStatus;
    QStringList quickReports;
    bool reportsOnly;
    bool assignmentsAllowed;
    bool permissionsAllowed;
    bool confessionsAllowed;
    bool reportsAllowed;
    bool rulesAllowed;

    // Initialize with defaults
    StatusSection() :
        isSubStatus(false),
        reportsOnly(false),
        assignmentsAllowed(true),
        permissionsAllowed(true),
        confessionsAllowed(true),
        reportsAllowed(true),
        rulesAllowed(true) {}
};

// Specialized section for permissions
struct PermissionSection : public ScriptSection {
    int percentChance;
    QStringList preStatus;
    QString delayTime;
    QString minInterval;
    QString maxWait;
    int notifyLevel;
    QString newStatus;
    QString denyMessage;
    QString permitMessage;
    QStringList denyFlags;
    QStringList permitFlags;
    int denyBelow;
    int denyAbove;

    // Initialize with defaults
    PermissionSection() :
        percentChance(0),
        notifyLevel(0),
        denyBelow(0),
        denyAbove(INT_MAX) {}
};

// Specialized section for reports
struct ReportSection : public ScriptSection {
    QStringList preStatus;
    QString newStatus;
    bool onTop;
    bool menuVisible;
    QString endReport;

    // Initialize with defaults
    ReportSection() :
        onTop(false),
        menuVisible(true) {}
};

// Specialized section for confessions
struct ConfessionSection : public ScriptSection {
    QStringList preStatus;
    QString newStatus;
    bool onTop;
    int punishSeverity;

    // Initialize with defaults
    ConfessionSection() :
        onTop(false),
        punishSeverity(0) {}
};

// Specialized section for jobs
struct JobSection : public ScriptSection {
    QString interval;
    QString firstInterval;
    QStringList runDays;
    QStringList noRunDays;
    QString endTime;
    QString remindInterval;
    int remindPenalty;
    QString remindPenaltyGroup;
    int lateMerits;
    QString expireAfter;
    int expirePenalty;
    QString expirePenaltyGroup;
    QString respite;
    QString estimate;
    bool oneTime;
    bool announce;

    // Initialize with defaults
    JobSection() :
        remindPenalty(0),
        lateMerits(0),
        expirePenalty(0),
        oneTime(false),
        announce(true) {}
};

// Specialized section for punishments
struct PunishmentSection : public ScriptSection {
    double value;
    QString valueUnit;
    int max;
    int min;
    int minSeverity;
    int maxSeverity;
    int weight;
    QStringList groups;
    bool groupOnly;
    bool longRunning;
    bool mustStart;
    bool accumulative;
    QString respite;
    QString estimate;
    QStringList forbidPermissions;

    // Initialize with defaults
    PunishmentSection() :
        value(1.0),
        max(20),
        min(1),
        minSeverity(0),
        maxSeverity(INT_MAX),
        weight(1),
        groupOnly(false),
        longRunning(false),
        mustStart(false),
        accumulative(false) {}
};

// Specialized section for flags
struct FlagSection : public ScriptSection {
    QStringList groups;
    QString duration;
    QString expireMessage;
    QString expireProcedure;
    QString setProcedure;
    QString removeProcedure;
    bool reportFlag;

    // Initialize with defaults
    FlagSection() :
        reportFlag(true) {}
};

// Specialized section for procedures
struct ProcedureSection : public ScriptSection {
    QStringList preStatus;
};

// Specialized section for popups
struct PopupSection : public ScriptSection {
    QStringList preStatus;
    QStringList groups;
    bool groupOnly;
    int weight;

    // Initialize with defaults
    PopupSection() :
        groupOnly(false),
        weight(1) {}
};

// Specialized section for timers
struct TimerSection : public ScriptSection {
    QString startTime;
    QString endTime;
    QStringList preStatus;
};

// Specialized section for instructions and clothing
struct InstructionSection : public ScriptSection {
    bool askable;
    QString changeFrequency;
    QString noneMessage;

    // Initialize with defaults
    InstructionSection() :
        askable(true),
        changeFrequency("daily") {}
};

// Specialized section for cloth types
struct ClothTypeSection : public ScriptSection {
    QMap<QString, QStringList> attributes;
    QMap<QString, QString> checks;
};

// Class to parse and provide access to the script
class ScriptParser {
public:
    ScriptParser(CyberDom *parent);
    ~ScriptParser();

    // Parse the script file
    bool parseScript(const QString &filePath);

    // Access parsed sections
    QList<StatusSection> getStatusSections() const;
    QList<PermissionSection> getPermissionSections() const;
    QList<ReportSection> getReportSections() const;
    QList<ConfessionSection> getConfessionSections() const;
    QList<JobSection> getJobSections() const;
    QList<PunishmentSection> getPunishmentSections() const;
    QList<FlagSection> getFlagSections() const;
    QList<ProcedureSection> getProcedureSections() const;
    QList<PopupSection> getPopupSections() const;
    QList<TimerSection> getTimerSections() const;
    QList<InstructionSection> getInstructionSections() const;
    QList<InstructionSection> getClothingSections() const;
    QList<ClothTypeSection> getClothTypeSections() const;
    
    // Access raw section data
    QMap<QString, QStringList> getRawSectionData(const QString &sectionName) const;
    
    // Get all section names
    QStringList getRawSectionNames() const;

    // Get specific sections by name
    StatusSection getStatus(const QString &name) const;
    PermissionSection getPermission(const QString &name) const;
    ReportSection getReport(const QString &name) const;
    ConfessionSection getConfession(const QString &name) const;
    JobSection getJob(const QString &name) const;
    PunishmentSection getPunishment(const QString &name) const;
    FlagSection getFlag(const QString &name) const;
    ProcedureSection getProcedure(const QString &name) const;
    PopupSection getPopup(const QString &name) const;
    TimerSection getTimer(const QString &name) const;
    InstructionSection getInstruction(const QString &name) const;
    InstructionSection getClothing(const QString &name) const;
    ClothTypeSection getClothType(const QString &name) const;

    // Get general script info
    QString getMaster() const;
    QString getSubName() const;
    int getMinMerits() const;
    int getMaxMerits() const;
    int getYellowMerits() const;
    int getRedMerits() const;
    bool isTestMenuEnabled() const;

    // Status group management
    QStringList getStatusesInGroup(const QString &groupName) const;
    bool isStatusInGroup(const QString &statusName, const QString &groupName) const;

    QString getIniValue(const QString &section, const QString &key, const QString &defaultValue = "") const;

private:
    CyberDom *mainApp;

    // Storage for parsed sections
    QMap<QString, StatusSection> statusSections;
    QMap<QString, PermissionSection> permissionSections;
    QMap<QString, ReportSection> reportSections;
    QMap<QString, ConfessionSection> confessionSections;
    QMap<QString, JobSection> jobSections;
    QMap<QString, PunishmentSection> punishmentSections;
    QMap<QString, FlagSection> flagSections;
    QMap<QString, ProcedureSection> procedureSections;
    QMap<QString, PopupSection> popupSections;
    QMap<QString, TimerSection> timerSections;
    QMap<QString, InstructionSection> instructionSections;
    QMap<QString, InstructionSection> clothingSections;
    QMap<QString, ClothTypeSection> clothTypeSections;

    // General script info
    QMap<QString, QString> generalSection;
    QMap<QString, QString> initSection;
    QMap<QString, QString> eventsSection;

    // Status groups mapping
    QMap<QString, QStringList> statusGroups;

    // Helper methods for parsing
    void parseSectionLine(const QString &line, QString &currentSection, QString &currentType);
    void parseKeyValueLine(const QString &line, const QString &currentSection, const QString &currentType);
    void processGeneralSection();
    void processStatusSections();
    void processPermissionSections();
    void processReportSections();
    void processConfessionSections();
    void processJobSections();
    void processPunishmentSections();
    void processFlagSections();
    void processProcedureSections();
    void processPopupSections();
    void processTimerSections();
    void processInstructionSections();
    void processClothingSections();
    void processClothTypeSections();
    void buildStatusGroups();

    // Variable expansion
    QString expandVariables(const QString &text);

    // Include file handling
    void parseIncludeFiles(const QString &mainFilePath);
    void parseIncludeFile(const QString &includePath);

    // Raw section data before processing
    QMap<QString, QMap<QString, QStringList>> rawSections;
};

#endif // SCRIPTPARSER_H
