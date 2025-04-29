#ifndef CLOTHINGITEM_H
#define CLOTHINGITEM_H

#include <QString>
#include <QMap>
#include <QStringList>

class ClothingItem
{
public:
    ClothingItem();
    ClothingItem(const QString &itemName, const QString &clothingType);
    
    // Getters and setters
    QString getName() const { return name; }
    QString getType() const { return type; }
    void setName(const QString &itemName) { name = itemName; }
    void setType(const QString &clothingType) { type = clothingType; }
    
    // Attribute handling
    void addAttribute(const QString &attributeName, const QString &attributeValue);
    QString getAttribute(const QString &attributeName) const;
    QMap<QString, QString> getAllAttributes() const { return attributes; }
    
    // For serialization/deserialization
    QString toString() const;
    static ClothingItem fromString(const QString &str);
    
private:
    QString name;
    QString type;
    QMap<QString, QString> attributes;
};

#endif // CLOTHINGITEM_H
