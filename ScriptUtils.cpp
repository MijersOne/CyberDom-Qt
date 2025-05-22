#include "ScriptUtils.h"
#include <QRandomGenerator>

int randomInRange(int min, int max, bool centerRandom) {
    if (!centerRandom) {
        return QRandomGenerator::global()->bounded(min, max + 1);
    } else {
        int a = QRandomGenerator::global()->bounded(min, max + 1);
        int b = QRandomGenerator::global()->bounded(min, max + 1);
        return (a + b) / 2;
    }
}
