#ifndef SCRIPTUTILS_H
#define SCRIPTUTILS_H

#include "ScriptData.h"

#include <QString>
#include <QList>
#include <QStringList>
#include <QMap>
#include <QDateTime>
#include <functional>

namespace ScriptUtils {

enum class SelectMode {
    All,
    Random,
    First
};

// Typedefs for data access callbacks
using FlagCheckFunc = std::function<bool(const QString&)>;
using VarGetFunc = std::function<QString(const QString&)>;

bool evaluateCondition(const QString& expr, const FlagCheckFunc &checkFlag, const VarGetFunc &getVar);

bool checkConditions(const QList<QStringList> &ifGroups,
                     const QList<QStringList> &notIfGroups,
                     const FlagCheckFunc &checkFlag,
                     const VarGetFunc &getVar);

int randomInRange(int min, int max, bool centerRandom);

bool checkHistoryConditions(const QStringList& ifChosen, const QStringList& ifNotChosen, const QStringList& history);

// Select Mode
void processSelector(int itemCount, SelectMode mode, const std::function<bool(int index)> &processor);

QString resolvePredefinedString(const QString &varName,
                                const ScriptData &data,
                                const QDateTime &internalClock,
                                const QString &lastReportFile = QString(),
                                const QString &appVersion = QString(),
                                const QString &contextName = QString(),
                                const QString &contextTitle = QString(),
                                const QString &lastReportName = QString(),
                                const QString &lastPermissionName = QString(),
                                const QString &currentStatus = QString(),
                                const QString &lastQuestionAnswer = QString());

int resolveInt(const QString& val, const VarGetFunc &getVar);

int resolvePredefinedCounter(const QString &varName,
                             int currentMerits,
                             int startOfDayMerits,
                             int maxMerits,
                             int minMerits,
                             int yellowMerits,
                             int redMerits,
                             int activeAssignmentCount,
                             int activeJobCount,
                             int activePunishmentCount,
                             int assignmentsDueTodayCount,
                             int jobsDueTodayCount,
                             int punishmentsDueTodayCount,
                             int unstartedAssignmentCount,
                             int unstartedJobCount,
                             int unstartedPunishCount,
                             int startedAssignmentCount,
                             int startedJobCount,
                             int startedPunishCount,
                             int overdueAssignmentCount,
                             int overdueJobCount,
                             int overduePunishCount,
                             int currentDay,
                             int currentMonth,
                             int currentYear,
                             int isLeapYear,
                             int currentHour,
                             int currentMinute,
                             int currentSeconds,
                             int secondsPassedToday,
                             int currentDayOfWeek,
                             int clothFaultsCount,
                             int lastPunishmentSeverity,
                             int lastLatenessMinutes);
}

#endif // SCRIPTUTILS_H
