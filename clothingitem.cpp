#include "clothingitem.h"
#include <QStringList>
#include <QJsonObject>
#include <QJsonDocument>

ClothingItem::ClothingItem() : name(""), type("") {}

ClothingItem::ClothingItem(const QString &itemName, const QString &clothingType)
    : name(itemName), type(clothingType) {}

void ClothingItem::addAttribute(const QString &attributeName, const QString &attributeValue) {
    attributes[attributeName] = attributeValue;
}

QString ClothingItem::getAttribute(const QString &attributeName) const {
    return attributes.value(attributeName, "");
}

QString ClothingItem::toString() const {
    QJsonObject jsonObj;
    jsonObj["name"] = name;
    jsonObj["type"] = type;
    
    QJsonObject attrsObj;
    for (auto it = attributes.constBegin(); it != attributes.constEnd(); ++it) {
        attrsObj[it.key()] = it.value();
    }
    
    jsonObj["attributes"] = attrsObj;
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
    
    return item;
}
