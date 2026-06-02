#include "clothingitem.h"
#include <QStringList>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>

ClothingItem::ClothingItem() : name(""), type("") {}

ClothingItem::ClothingItem(const QString &itemName, const QString &clothingType)
    : name(itemName), type(clothingType) {}

void ClothingItem::addAttribute(const QString &attributeName, const QString &attributeValue) {
    attributes[attributeName] = attributeValue;
}

QString ClothingItem::getAttribute(const QString &attributeName) const {
    return attributes.value(attributeName, "");
}

// --- New Methods ---
void ClothingItem::addCheck(const QString &check) {
    if (!checks.contains(check)) {
        checks.append(check);
    }
}

void ClothingItem::setChecks(const QStringList &checkList) {
    checks = checkList;
}
// -------------------

QString ClothingItem::toString() const {
    QJsonObject jsonObj;
    jsonObj["name"] = name;
    jsonObj["type"] = type;

    QJsonObject attrsObj;
    for (auto it = attributes.constBegin(); it != attributes.constEnd(); ++it) {
        attrsObj[it.key()] = it.value();
    }
    jsonObj["attributes"] = attrsObj;

    // Save Checks
    QJsonArray checksArray;
    for (const QString &c : checks) {
        checksArray.append(c);
    }
    jsonObj["checks"] = checksArray;

    QJsonDocument doc(jsonObj);
    return QString(doc.toJson(QJsonDocument::Compact));
}

ClothingItem ClothingItem::fromString(const QString &str) {
    QJsonDocument doc = QJsonDocument::fromJson(str.toUtf8());
    QJsonObject jsonObj = doc.object();

    ClothingItem item;
    item.setName(jsonObj["name"].toString());
    item.setType(jsonObj["type"].toString());

    QJsonObject attrsObj = jsonObj["attributes"].toObject();
    for (auto it = attrsObj.constBegin(); it != attrsObj.constEnd(); ++it) {
        item.addAttribute(it.key(), it.value().toString());
    }

    // Load Checks
    if (jsonObj.contains("checks")) {
        QJsonArray checksArray = jsonObj["checks"].toArray();
        QStringList loadedChecks;
        for (const QJsonValue &val : checksArray) {
            loadedChecks.append(val.toString());
        }
        item.setChecks(loadedChecks);
    }

    return item;
}
