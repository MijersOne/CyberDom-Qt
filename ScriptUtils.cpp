#include "ScriptUtils.h"
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QStringList>
#include <QDateTime>
#include <QDebug>

int randomInRange(int min, int max, bool centerRandom) {
    if (!centerRandom) {
        return QRandomGenerator::global()->bounded(min, max + 1);
    } else {
        int a = QRandomGenerator::global()->bounded(min, max + 1);
        int b = QRandomGenerator::global()->bounded(min, max + 1);
        return (a + b) / 2;
    }
}

bool ScriptUtils::evaluateCondition(const QString& expr, const QMap<QString, QString>& stringVars, const QMap<QString, int>& counters, const QMap<QString, QDateTime>& timeVars)
{
    static QStringList operators = {"==", "<=", ">=", "<>", "<", ">", "=", "[[", "["};
    QString opUsed;
    QString lhs, rhs;

    for (const QString& op : operators) {
        int idx = expr.indexOf(op);
        if (idx != -1) {
            opUsed = op;
            lhs = expr.left(idx).trimmed();
            rhs = expr.mid(idx + op.length()).trimmed();
            break;
        }
    }

    if (opUsed.isEmpty() || lhs.isEmpty() || rhs.isEmpty())
        return false;

    auto resolveString = [&](const QString& val) {
        if (val.startsWith("$")) return stringVars.value(val, "");
        return val;
    };

    auto resolveInt = [&](const QString& val) -> int {
        if (val.startsWith("#")) return counters.value(val, 0);
        bool ok = false;
        int result = val.toInt(&ok);
        return ok ? result : 0; // fallback to 0 if conversion fails
    };

    auto resolveTime = [&](const QString& val) -> QDateTime {
        if (val.startsWith("!")) return timeVars.value(val, QDateTime());
        return QDateTime::fromString(val, Qt::ISODate);  // Try parsing literal
    };

    if (lhs.startsWith("#") || rhs.startsWith("#")) {
        int l = resolveInt(lhs);
        int r = resolveInt(rhs);
        if (opUsed == "=") return l == r;
        if (opUsed == "<>") return l != r;
        if (opUsed == "<") return l < r;
        if (opUsed == "<=") return l <= r;
        if (opUsed == ">") return l > r;
        if (opUsed == ">=") return l >= r;
    } else if (lhs.startsWith("!")) {
        QDateTime l = resolveTime(lhs);
        QDateTime r = resolveTime(rhs);
        if (opUsed == "=") return l == r;
        if (opUsed == "<>") return l != r;
        if (opUsed == "<") return l < r;
        if (opUsed == "<=") return l <= r;
        if (opUsed == ">") return l > r;
        if (opUsed == ">=") return l >= r;
    } else {
        QString l = resolveString(lhs);
        QString r = resolveString(rhs);
        if (opUsed == "=") return l.compare(r, Qt::CaseInsensitive) == 0;
        if (opUsed == "==") return l == r;
        if (opUsed == "<>") return l.compare(r, Qt::CaseInsensitive) != 0;
        if (opUsed == "[") return l.toLower().contains(r.toLower());
        if (opUsed == "[[") return l.contains(r);
    }

    return false;
}
