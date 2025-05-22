#ifndef SCRIPTPARSER_H
#define SCRIPTPARSER_H

#include "ScriptData.h"
#include <QString>
#include <QMap>

class ScriptParser {
public:
    bool parseScript(const QString& path);

    const ScriptData& getScriptData() const { return scriptData; }

private:
    ScriptData scriptData;

    void parseGeneralSection(const QMap<QString, QStringList>& section);
    void parseInitSection(const QMap<QString, QStringList>& section);
    void parseEventsSection(const QMap<QString, QStringList>& section);
    void parseStatusSections(const QMap<QString, QMap<QString, QStringList>>& sections);
    void parseReportSections(const QMap<QString, QMap<QString, QStringList>>& sections);
    void parseConfessionSections(const QMap<QString, QMap<QString, QStringList>>& sections);
    void parsePermissionSections(const QMap<QString, QMap<QString, QStringList>>& sections);
    void parsePunishmentSections(const QMap<QString, QMap<QString, QStringList>>& sections);
    void parseJobSections(const QMap<QString, QMap<QString, QStringList>>& sections);
    void parseAssignmentBehavior(const QMap<QString, QStringList>& entries, AssignmentBehavior& a);
    void parseInstructionSections(const QMap<QString, QMap<QString, QStringList>>& sections);
    void parseInstructionSets(const QMap<QString, QMap<QString, QStringList>>& sections);
    void parseClothingTypes(const QMap<QString, QMap<QString, QStringList>>& sections);
    void parseProcedureSections(const QMap<QString, QMap<QString, QStringList>>& sections);
    void parsePopupSections(const QMap<QString, QMap<QString, QStringList>>& sections);
    void parsePopupGroupSections(const QMap<QString, QMap<QString, QStringList>>& sections);
    void parseTimerSections(const QMap<QString, QMap<QString, QStringList>>& sections);
    void parseQuestionSections(const QMap<QString, QMap<QString, QStringList>>& sections);
    void parseDurationControl (const QMap<QString, QStringList>& entries, DurationControl& d);
    void parseTimeWindowControl(const QMap<QString, QStringList>& entries, TimeWindowControl& tw);
    void parseTimeRestrictions(const QMap<QString, QStringList>& entries, QStringList& notBefore, QStringList& notAfter, QList<QPair<QString, QString>>& notBetween);
    void parseEventSection(const QMap<QString, QStringList>& entries);
    void parseFtpSection(const QMap<QString, QStringList>& entries);
    void parseFontSection(const QMap<QString, QStringList>& entries);
    void parseFlagSection(const QString& name, const QMap<QString, QStringList>& entries);

    QMap<QString, QMap<QString, QStringList>> parseIniFile(const QString& path);

    MeritAdjustment merits;
};

#endif // SCRIPTPARSER_H
