#ifndef SCRIPTUTILS_H
#define SCRIPTUTILS_H

#include <QString>
#include <QMap>
#include <QDateTime>

namespace ScriptUtils {

bool evaluateCondition(const QString& expr, const QMap<QString, QString>& stringVars, const QMap<QString, int>& counters, const QMap<QString, QDateTime>& timeVars);
int randomInRange(int min, int max, bool centerRandom);

}

#endif // SCRIPTUTILS_H
