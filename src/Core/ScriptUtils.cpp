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

bool checkHistoryConditions(const QStringList& ifChosen, const QStringList& ifNotChosen, const QStringList& history)
{
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
                             int lastLatenessMinutes,
                             int lastEarlyMinutes,
                             int contextSeverity)
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

    // Handle #zzEarly
    else if (varName.compare("zzEarly", Qt::CaseInsensitive) == 0) {
        return lastEarlyMinutes;
    }

    // Handle #zzSeverity
    else if (varName.compare("zzSeverity", Qt::CaseInsensitive) == 0) {
        return contextSeverity;
    }

    return 0; // Not a known predefined counter
}

QDateTime parseDateString(const QString &str)
{
    QString val = str.trimmed();
    QDateTime dt;

    // yyyy-MM-dd HH:mm:ss
    dt = QDateTime::fromString(val, "yyyy-MM-dd HH:mm:ss");
    if (dt.isValid()) return dt;

    // yyyy-MM-dd HH:mm
    dt = QDateTime::fromString(val, "yyyy-MM-dd HH:mm");
    if (dt.isValid()) return dt;

    // yyyy-MM-dd
    dt = QDateTime::fromString(val, "yyyy-MM-dd");
    if (dt.isValid()) {
        dt.setTime(QTime(0, 0, 0));
        return dt;
    }

    return QDateTime();
}

int parseDurationString(const QString &str)
{
    QString val = str.trimmed();
    if (val.isEmpty()) return 0;

    // Check "nd" (Days) format (e.g., "3d", "14d")
    QRegularExpression dayRx("^(\\d+)d$", QRegularExpression::CaseInsensitiveOption);
    auto match = dayRx.match(val);
    if (match.hasMatch()) {
        int days = match.captured(1).toInt();
        return days * 86400; // 24 * 3600
    }

    // Check "hh:mm:ss" or "hh:mm"
    QStringList parts = val.split(':');
    if (parts.size() == 3) {
        // hh:mm:ss
        int h = parts[0].toInt();
        int m = parts[1].toInt();
        int s = parts[2].toInt();
        return (h * 3600) + (m * 60) + s;
    }
    else if (parts.size() == 2) {
        // hh:mm
        int h = parts[0].toInt();
        int m = parts[1].toInt();
        return (h * 3600) + (m * 60);
    }

    // Fallback: Try treating as raw seconds integer
    bool ok;
    int s = val.toInt(&ok);
    if (ok) return s;

    return 0;
}

QString formatDurationString(int seconds)
{
    if (seconds <= 0) return "00:00:00";

    if (seconds % 86400 == 0) {
        return QString("%1d").arg(seconds / 86400);
    }

    int h = seconds / 3600;
    int m = (seconds % 3600) / 60;
    int s = seconds % 60;

    return QString("%1:%2:%3")
        .arg(h, 2, 10, QChar('0'))
        .arg(m, 2, 10, QChar('0'))
        .arg(s, 2, 10, QChar('0'));
}

QVariant resolvePredefinedTime(const QString &varName,
                               const QDateTime &internalClock,
                               const QDateTime &scriptBeginTime,
                               const QDateTime &sessionStartTime,
                               const QDateTime &lastCloseTime,
                               int lastResponseDuration,
                               const QString &contextMinTime,
                               const QString &contextMaxTime,
                               const QDateTime &assignmentStartTime,
                               const QDateTime &assignmentCreationTime,
                               const QDateTime &assignmentDeadline,
                               const QDateTime &assignmentNextRemindTime,
                               int lastLatenessMinutes,
                               int lastEarlyMinutes,
                               const QDateTime &permissionNextAllowTime,
                               int flagDuration,
                               const QDateTime &flagExpiryTime)
{
    // Handle !zzDate
    if (varName.compare("zzDate", Qt::CaseInsensitive) == 0) {
        return QDateTime(internalClock.date(), QTime(0, 0, 0));
    }

    // Handle !zzTime
    else if (varName.compare("zzTime", Qt::CaseInsensitive) == 0) {
        return internalClock.time();
    }

    // Handle !zzDateTime and !zzNow (Both return full current time)
    else if (varName.compare("zzDateTime", Qt::CaseInsensitive) == 0 ||
             varName.compare("zzNow", Qt::CaseInsensitive) == 0) {
        return internalClock;
    }

    // Handle !zzBeginTime
    else if (varName.compare("zzBeginTime", Qt::CaseInsensitive) == 0) {
        return scriptBeginTime;
    }

    // Handle !zzOpenTime
    else if (varName.compare("zzOpenTime", Qt::CaseInsensitive) == 0) {
        return sessionStartTime;
    }

    // Handle !zzCloseTime
    else if (varName.compare("zzCloseTime", Qt::CaseInsensitive) == 0) {
        // If never closed before (invalid), return current session start time
        if (!lastCloseTime.isValid()) {
            return sessionStartTime;
        }
        return lastCloseTime;
    }

    // Handle !zzResponseTime
    else if (varName.compare("zzResponseTime", Qt::CaseInsensitive) == 0) {
        // Return duration as QTime
        return QTime(0, 0, 0).addSecs(lastResponseDuration);
    }

    // Handle !zzMinTime
    else if (varName.compare("zzMinTime", Qt::CaseInsensitive) == 0) {
        // Parse the string "HH:mm" or "3d" into seconds
        int seconds = ScriptUtils::parseDurationString(contextMinTime);
        return seconds;
    }

    // Handle !zzMaxTime
    else if (varName.compare("zzMaxTime", Qt::CaseInsensitive) == 0) {
        return ScriptUtils::parseDurationString(contextMaxTime);
    }

    // Handle !zzStartTime
    else if (varName.compare("zzStartTime", Qt::CaseInsensitive) == 0) {
        return assignmentStartTime;
    }

    // Handle !zzMinTimeEnd
    else if (varName.compare("zzMinTimeEnd", Qt::CaseInsensitive) == 0) {
        // Formula: StartTime + MinTime
        if (assignmentStartTime.isValid()) {
            int duration = ScriptUtils::parseDurationString(contextMinTime);
            return assignmentStartTime.addSecs(duration);
        }
    }

    // Handle !zzMaxTimeEnd
    else if (varName.compare("zzMaxTimeEnd", Qt::CaseInsensitive) == 0) {
        // Formula: StartTime + MaxTime
        if (assignmentStartTime.isValid()) {
            int duration = ScriptUtils::parseDurationString(contextMaxTime);
            return assignmentStartTime.addSecs(duration);
        }
    }

    // Handle !zzInitTime
    else if (varName.compare("zzInitTime", Qt::CaseInsensitive) == 0) {
        return assignmentCreationTime;
    }

    // Handle !zzRunTime (Duration since start)
    else if (varName.compare("zzRunTime", Qt::CaseInsensitive) == 0) {
        if (assignmentStartTime.isValid()) {
            // Calculate seconds elapsed
            return static_cast<int>(assignmentStartTime.secsTo(internalClock));
        }
        return 0;
    }

    // Handle !zzDeadline
    else if (varName.compare("zzDeadline", Qt::CaseInsensitive) == 0) {
        return assignmentDeadline;
    }

    // Handle !zzRemindTime
    else if (varName.compare("zzRemindTime", Qt::CaseInsensitive) == 0) {
        return assignmentNextRemindTime;
    }

    // Handle !zzLate
    else if (varName.compare("zzLate", Qt::CaseInsensitive) == 0) {
        // Return duration in seconds (minutes * 60)
        return lastLatenessMinutes * 60;
    }

    // Handle !zzEarly
    else if (varName.compare("zzEarly", Qt::CaseInsensitive) == 0) {
        // Return duration in seconds
        return lastEarlyMinutes * 60;
    }

    // Handle !zzNextAllow
    else if (varName.compare("zzNextAllow", Qt::CaseInsensitive) == 0) {
        // If the time is valid, return it.
        // If invalid (not locked), return "Now" (Internal Clock)
        return permissionNextAllowTime.isValid() ? permissionNextAllowTime : internalClock;
    }

    // Handle !zzDuration
    else if (varName.compare("zzDuration", Qt::CaseInsensitive) == 0) {
        return flagDuration;
    }

    // Handle !zzExpireTime
    else if (varName.compare("zzExpireTime", Qt::CaseInsensitive) == 0) {
        if (flagExpiryTime.isValid()) {
            return flagExpiryTime;
        }
    }

    return QVariant();
}

}

