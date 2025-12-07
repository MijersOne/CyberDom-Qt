#include "ScriptUtils.h"
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QStringList>
#include <QDateTime>
#include <QDebug>
#include <QtMath>

namespace ScriptUtils {

int randomInRange(int min, int max, bool centerRandom) {
    if (!centerRandom) {
        return QRandomGenerator::global()->bounded(min, max + 1);
    } else {
        int a = QRandomGenerator::global()->bounded(min, max + 1);
        int b = QRandomGenerator::global()->bounded(min, max + 1);
        return (a + b) / 2;
    }
}

void processSelector(int itemCount, SelectMode mode, const std::function<bool(int)> &processor)
{
    if (itemCount <= 0) return;

    switch (mode) {
    case SelectMode::All: {
        for (int i = 0; i < itemCount; ++i) {
            processor(i);
        }
        break;
    }
    case SelectMode::Random: {
        int idx = randomInRange(0, itemCount - 1, false);
        processor(idx);
        break;
    }
    case SelectMode::First: {
        for (int i = 0; i < itemCount; ++i) {
            if (processor(i)) break;
        }
        break;
    }
    }
}

bool evaluateCondition(const QString& expr, const FlagCheckFunc &checkFlag, const VarGetFunc &getVar)
{
    static QStringList operators = {"==", "<=", ">=", "<>", "<", ">", "=", "[[", "["};
    QString opUsed, lhs, rhs;

    for (const QString& op : operators) {
        int idx = expr.indexOf(op);
        if (idx != -1) {
            opUsed = op;
            lhs = expr.left(idx).trimmed();
            rhs = expr.mid(idx + op.length()).trimmed();
            break;
        }
    }

    if (opUsed.isEmpty()) return checkFlag(expr.trimmed());

    auto resolve = [&](const QString& val) -> QString {
        if (val.startsWith("#") || val.startsWith("!") || val.startsWith("$")) {
            return getVar(val.mid(1));
        }
        return val;
    };

    QString lVal = resolve(lhs);
    QString rVal = resolve(rhs);

    bool lOk, rOk;
    double lNum = lVal.toDouble(&lOk);
    double rNum = rVal.toDouble(&rOk);

    if (lOk && rOk) {
        if (opUsed == "=" || opUsed == "==") return qFuzzyCompare(lNum, rNum);
        if (opUsed == "<>" || opUsed == "!=") return !qFuzzyCompare(lNum, rNum);
        if (opUsed == "<") return lNum < rNum;
        if (opUsed == "<=") return lNum <= rNum;
        if (opUsed == ">") return lNum > rNum;
        if (opUsed == ">=") return lNum >= rNum;
    } else {
        if (opUsed == "=" || opUsed == "==") return lVal.compare(rVal, Qt::CaseInsensitive) == 0;
        if (opUsed == "<>" || opUsed == "!=") return lVal.compare(rVal, Qt::CaseInsensitive) != 0;
        if (opUsed == "[") return lVal.toLower().contains(rVal.toLower());
        if (opUsed == "[[") return lVal.contains(rVal);
    }
    return false;
}

bool checkConditions(const QList<QStringList> &ifGroups,
                     const QList<QStringList> &notIfGroups,
                     const FlagCheckFunc &checkFlag,
                     const VarGetFunc &getVar)
{
    if (!ifGroups.isEmpty()) {
        bool anyGroupMet = false;
        for (const QStringList &group : ifGroups) {
            bool allInGroup = true;
            for (const QString &cond : group) {
                if (!evaluateCondition(cond, checkFlag, getVar)) {
                    allInGroup = false;
                    break;
                }
            }
            if (allInGroup) {
                anyGroupMet = true;
                break;
            }
        }
        if (!anyGroupMet) return false;
    }

    if (!notIfGroups.isEmpty()) {
        for (const QStringList &group : notIfGroups) {
            bool allInGroup = true;
            for (const QString &cond : group) {
                if (!evaluateCondition(cond, checkFlag, getVar)) {
                    allInGroup = false;
                    break;
                }
            }
            if (allInGroup) return false;
        }
    }

    return true;
}

// --- IMPLEMENTATION OF MISSING FUNCTION ---
bool checkHistoryConditions(const QStringList& ifChosen, const QStringList& ifNotChosen, const QStringList& history)
{
    // 1. Check IfChosen (At least one item must exist in history)
    if (!ifChosen.isEmpty()) {
        bool matchFound = false;
        for (const QString &req : ifChosen) {
            if (history.contains(req, Qt::CaseInsensitive)) {
                matchFound = true;
                break;
            }
        }
        if (!matchFound) return false;
    }

    // 2. Check IfNotChosen (No items may exist in history)
    if (!ifNotChosen.isEmpty()) {
        for (const QString &req : ifNotChosen) {
            if (history.contains(req, Qt::CaseInsensitive)) {
                return false;
            }
        }
    }

    return true;
}

QString resolvePredefinedString(const QString &varName,
                                const ScriptData &data,
                                const QDateTime &internalClock,
                                const QString &lastReportFile,
                                const QString &appVersion,
                                const QString &contextName,
                                const QString &contextTitle,
                                const QString &lastReportName,
                                const QString &lastPermissionName,
                                const QString &currentStatus,
                                const QString &lastQuestionAnswer)
{
    // Handle $zzSubName
    if (varName.compare("zzSubName", Qt::CaseInsensitive) == 0) {
        const QStringList &names = data.general.subNames;

        if (names.isEmpty()) {
            return "Sub"; // Default fallback
        }

        if (names.size() == 1) {
            return names.first();
        }

        // Pick a random name from the list
        int index = randomInRange(0, names.size() - 1, false);
        return names.at(index);
    }

    // Handle $zzMaster
    else if (varName.compare("zzMaster", Qt::CaseInsensitive) == 0) {
        return data.general.masterName;
    }

    // Handle $zzDate
    else if (varName.compare("zzDate", Qt::CaseInsensitive) == 0) {
        return internalClock.date().toString("MM-dd-yyyy");
    }

    // Handle $zzTime
    else if (varName.compare("zzTime", Qt::CaseInsensitive) == 0) {
        return internalClock.time().toString("hh:mm:ss AP");
    }

    // Handle $zzDayOfWeek
    else if (varName.compare("zzDayOfWeek", Qt::CaseInsensitive) == 0) {
        return internalClock.date().toString("dddd");
    }

    // Handle $zzMonth
    else if (varName.compare("zzMonth", Qt::CaseInsensitive) == 0) {
        return internalClock.date().toString("MMMM");
    }

    // Handle $zzReportFile
    else if (varName.compare("zzReportFile", Qt::CaseInsensitive) == 0) {
        return lastReportFile;
    }

    // Handle $zzPVersion
    else if (varName.compare("zzPVersion", Qt::CaseInsensitive) == 0) {
        return appVersion;
    }

    // Handle $zzSVersion
    else if (varName.compare("zzSVersion", Qt::CaseInsensitive) == 0) {
        return data.general.version;
    }

    // Handle $zzName
    if (varName.compare("zzName", Qt::CaseInsensitive) == 0) {
        return contextName;
    }

    // Handle $zzTitle
    else if (varName.compare("zzTitle", Qt::CaseInsensitive) == 0) {
        return contextTitle;
    }

    // Handle $zzReport
    else if (varName.compare("zzReport", Qt::CaseInsensitive) == 0) {
        return lastReportName;
    }

    // Handle $zzPermission
    else if (varName.compare("zzPermission", Qt::CaseInsensitive) == 0) {
        return lastPermissionName;
    }

    // Handle $zzStatus
    else if (varName.compare("zzStatus", Qt::CaseInsensitive) == 0) {
        return currentStatus;
    }

    // Handle $zzAnswer
    else if (varName.compare("zzAnswer", Qt::CaseInsensitive) == 0) {
        return lastQuestionAnswer;
    }

    return QString();
}

int resolveInt(const QString& val, const VarGetFunc &getVar) {
    QString strVal = val.trimmed();

    // Check for variable reference (#Counter or $String)
    if (strVal.startsWith("#") || strVal.startsWith("$")) {
        // Strip the prefix and lookup the value
        QString resolvedStr = getVar(strVal.mid(1));

        // Convert resolved string to int (default to 0 if failed)
        bool ok;
        int result = resolvedStr.toInt(&ok);
        return ok ? result : 0;
    }

    // It's a literal number
    bool ok;
    int result = strVal.toInt(&ok);
    return ok ? result : 0;
}

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
                             int currentSecond,
                             int secondsPassedToday,
                             int currentDayOfWeek,
                             int clothFaultsCount,
                             int lastPunishmentSeverity,
                             int lastLatenessMinutes)
{
    // Handle #zzMerits
    if (varName.compare("zzMerits", Qt::CaseInsensitive) == 0) {
        return currentMerits;
    }

    // Handle #zzBeginMerits
    if (varName.compare("zzBeginMerits", Qt::CaseInsensitive) == 0) {
        return startOfDayMerits;
    }

    // Handle #zzMaxMerits
    else if (varName.compare("zzMaxMerits", Qt::CaseInsensitive) == 0) {
        return maxMerits;
    }

    // Handle #zzMinMerits and #zzBlack (Alias)
    else if (varName.compare("zzMinMerits", Qt::CaseInsensitive) == 0 ||
             varName.compare("zzBlack", Qt::CaseInsensitive) == 0) {
        return minMerits;
    }

    // Handle #zzYellow
    else if (varName.compare("zzYellow", Qt::CaseInsensitive) == 0) {
        return yellowMerits;
    }

    // Handle #zzRed
    else if (varName.compare("zzRed", Qt::CaseInsensitive) == 0) {
        return redMerits;
    }

    // Handle #zzNoassign
    else if (varName.compare("zzNoassign", Qt::CaseInsensitive) == 0) {
        return activeAssignmentCount;
    }

    // Handle #zzNoJob
    else if (varName.compare("zzNoJob", Qt::CaseInsensitive) == 0) {
        return activeJobCount;
    }

    // Handle #zzNoPunish
    else if (varName.compare("zzNoPunish", Qt::CaseInsensitive) == 0) {
        return activePunishmentCount;
    }

    // Handle #zzNoAssignToday
    else if (varName.compare("zzNoAssignToday", Qt::CaseInsensitive) == 0) {
        return assignmentsDueTodayCount;
    }

    // Handle #zzNoJobToday
    else if (varName.compare("zzNoJobToday", Qt::CaseInsensitive) == 0) {
        return jobsDueTodayCount;
    }

    // Handle #zzNoPunishToday
    else if (varName.compare("zzNoPunishToday", Qt::CaseInsensitive) == 0) {
        return punishmentsDueTodayCount;
    }

    // Handle #zzNoAssignWait
    else if (varName.compare("zzNoAssignWait", Qt::CaseInsensitive) == 0) {
        return unstartedAssignmentCount;
    }

    // Handle #zzNoJobWait
    else if (varName.compare("zzNoJobWait", Qt::CaseInsensitive) == 0) {
        return unstartedJobCount;
    }

    // Handle #zzNoPunishWait
    else if (varName.compare("zzNoPunishWait", Qt::CaseInsensitive) == 0) {
        return unstartedPunishCount;
    }

    // Handle #zzNoAssignStart
    else if (varName.compare("zzNoAssignStart", Qt::CaseInsensitive) == 0) {
        return startedAssignmentCount;
    }

    // Handle #zzNoJobStart
    else if (varName.compare("zzNoJobStart", Qt::CaseInsensitive) == 0) {
        return startedJobCount;
    }

    // Handle #zzNoPunishStart
    else if (varName.compare("zzNoPunishStart", Qt::CaseInsensitive) == 0) {
        return startedPunishCount;
    }

    // Handle #zzNoAssignLate
    else if (varName.compare("zzNoAssignLate", Qt::CaseInsensitive) == 0) {
        return overdueAssignmentCount;
    }

    // Handle #zzNoJobLate
    else if (varName.compare("zzNoJobLate", Qt::CaseInsensitive) == 0) {
        return overdueJobCount;
    }

    // Handle #zzNoPunishLate
    else if (varName.compare("zzNoPunishLate", Qt::CaseInsensitive) == 0) {
        return overduePunishCount;
    }

    // Handle #zzDay
    else if (varName.compare("zzDay", Qt::CaseInsensitive) == 0) {
        return currentDay;
    }

    // Handle #zzMonth
    else if (varName.compare("zzMonth", Qt::CaseInsensitive) == 0) {
        return currentMonth;
    }

    // Handle #zzYear
    else if (varName.compare("zzYear", Qt::CaseInsensitive) == 0) {
        return currentYear;
    }

    // Handle #zzLeapYear
    else if (varName.compare("zzLeapYear", Qt::CaseInsensitive) == 0) {
        return isLeapYear;
    }

    // Handle #zzHour
    else if (varName.compare("zzHour", Qt::CaseInsensitive) == 0) {
        return currentHour;
    }

    // Handle #zzMinute
    else if (varName.compare("zzMinute", Qt::CaseInsensitive) == 0) {
        return currentMinute;
    }

    // Handle #zzSecond
    else if (varName.compare("zzSecond", Qt::CaseInsensitive) == 0) {
        return currentSecond;
    }

    // Handle #zzSecondsPassed
    else if (varName.compare("zzSecondsPassed", Qt::CaseInsensitive) == 0) {
        return secondsPassedToday;
    }

    // Handle #zzDayOfWeek (1=Mon ... 7=Sun)
    else if (varName.compare("zzDayOfWeek", Qt::CaseInsensitive) == 0) {
        return currentDayOfWeek;
    }

    // Handle #zzClothFaults
    else if (varName.compare("zzClothFaults", Qt::CaseInsensitive) == 0) {
        return clothFaultsCount;
    }

    // Handle #zzPunishmentSeverity
    else if (varName.compare("zzPunishmentSeverity", Qt::CaseInsensitive) == 0) {
        return lastPunishmentSeverity;
    }

    // Handle #zzLate
    else if (varName.compare("zzLate", Qt::CaseInsensitive) == 0) {
        return lastLatenessMinutes;
    }

    return 0; // Not a known predefined counter
}

}

