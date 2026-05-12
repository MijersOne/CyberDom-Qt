#include "ListManager.h"
#include <QDebug>
#include <qnamespace.h>
#include <qobject.h>
#include <qregularexpression.h>

ListManager::ListManager(const QString &settingsFile, QObject *parent)
    : QObject(parent), m_settingsFile(settingsFile) 
{
    loadLists();
}

void ListManager::loadLists() {
    QSettings settings(m_settingsFile, QSettings::IniFormat);
    settings.beginGroup("Lists");
    QStringList keys = settings.childKeys();
    for (const QString &key : keys) {
        // keys in QSettings are stored as written, we store internal keys as lowercase
        m_lists[key.toLower()] = settings.value(key).toStringList();
    }
    settings.endGroup();
    qDebug() << "[ListManager] Loaded" << m_lists.size() << "lists.";
}

void ListManager::saveLists() {
    QSettings settings(m_settingsFile, QSettings::IniFormat);
    settings.beginGroup("Lists");
    settings.remove(""); // Clean up old keys
    
    QMapIterator<QString, QStringList> i(m_lists);
    while (i.hasNext()) {
        i.next();
        settings.setValue(i.key(), i.value());
    }
    settings.endGroup();
    settings.sync();
}

QStringList ListManager::getList(const QString &name) const {
    return m_lists.value(name.toLower());
}

bool ListManager::isListReference(const QString &val) const {
    return val.trimmed().startsWith('*');
}

QString ListManager::getListNameFromReference(const QString &ref) const {
    // Remove the leading '*' and whitespace
    return ref.trimmed().mid(1);
}

// Command: Set*=*listname,val1,val2...
void ListManager::performSet(const QString &rawLine, std::function<QString(QString)> varResolver) {
    if (rawLine.isEmpty()) return;

    // 1. Separate Target List Name from Values
    // The syntax is "*listname,val1,val2"
    int firstComma = rawLine.indexOf(',');
    
    QString targetNameRaw;
    QString valuesPart;

    if (firstComma == -1) {
        // Case: *listname (No values, implies empty list)
        targetNameRaw = rawLine.trimmed();
    } else {
        targetNameRaw = rawLine.left(firstComma).trimmed();
        valuesPart = rawLine.mid(firstComma + 1);
    }

    // Validation: Must start with *
    if (!targetNameRaw.startsWith('*')) {
        qWarning() << "[ListManager] Error: List name in Set* must start with asterisk. Got:" << targetNameRaw;
        return;
    }

    // Store key as "listname" (no asterisk, lowercase)
    QString key = targetNameRaw.mid(1).toLower();

    // 2. Process Elements
    QStringList newContent;
    
    // Split by comma first
    // Note: This assumes variables inside don't break comma logic yet. 
    // If strict CSV parsing is needed later, we can adjust.
    QStringList rawItems = valuesPart.split(',', Qt::SkipEmptyParts);

    for (QString item : rawItems) {
        item = item.trimmed();

        // A. Resolve Variables (e.g. $MyString -> "Hello")
        // We do this BEFORE checking if it's a list, because a variable might hold "*OtherList"
        QString resolved = varResolver(item);

        // B. Check if it is a List Reference (e.g. *OtherList)
        if (isListReference(resolved)) {
            QString refKey = getListNameFromReference(resolved);
            
            // "If the list exists, all elements in that list will be added"
            if (m_lists.contains(refKey.toLower())) {
                newContent.append(m_lists[refKey.toLower()]);
            } else {
                qDebug() << "[ListManager] Warning: Referenced list not found:" << refKey;
                // If it doesn't exist, we generally don't add anything (it expands to nothing)
            }
        } 
        else {
            // C. It is simple text/value
            newContent.append(resolved);
        }
    }

    // 3. Save (Replaces existing list)
    m_lists[key] = newContent;
    saveLists();
    
    qDebug() << "[ListManager] Set list" << key << "with" << newContent.size() << "items.";
}

void ListManager::performSetSplit(const QString &rawLine, std::function<QString(QString)> varResolver) {
    if (rawLine.isEmpty()) return;

    // Separate Target List Name from Variable/Value
    // Syntax: *listname,$var
    int firstComma = rawLine.indexOf(',');

    if (firstComma == -1) {
        qWarning() << "[ListManager] Error: SetSplit* requires a value (e.g. *list,$var). Got" << rawLine;
        return;
    }

    QString targetNameRaw = rawLine.left(firstComma).trimmed();
    QString valuePart = rawLine.mid(firstComma + 1); // Keep raw for resolver

    // Validation
    if (!targetNameRaw.startsWith('*')) {
        qWarning() << "[ListManager] Error: List name in SetSplit* must start with asterisk. Got:" << targetNameRaw;
        return;
    }

    QString key = targetNameRaw.mid(1).toLower();

    // Resolve Variable
    QString resolved = varResolver(valuePart);

    // Split by NewLines
    QStringList newContent = resolved.split(QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts);

    // Save (Overwrites existining list)
    m_lists[key] = newContent;
    saveLists();

    qDebug() << "[ListManager] SetSplit list" << key << "with" << newContent.size() << "items from variable.";
}

void ListManager::performAdd(const QString &rawLine, std::function<QString(QString)> varResolver) {
    if (rawLine.isEmpty()) return;

    // 1. Separate Target List Name from Values
    // Syntax: *listname,val1,val2...
    int firstComma = rawLine.indexOf(',');
    
    QString targetNameRaw;
    QString valuesPart;

    if (firstComma == -1) {
        // Case: *listname (No values, implies we add nothing or empty string?)
        // Usually, Add* without values does nothing, but let's handle it gracefully.
        return; 
    } else {
        targetNameRaw = rawLine.left(firstComma).trimmed();
        valuesPart = rawLine.mid(firstComma + 1);
    }

    // Validation
    if (!targetNameRaw.startsWith('*')) {
        qWarning() << "[ListManager] Error: List name in Add* must start with asterisk. Got:" << targetNameRaw;
        return;
    }

    QString key = targetNameRaw.mid(1).toLower();

    // 2. Process Elements (Identical to performSet logic)
    QStringList itemsToAdd;
    QStringList rawItems = valuesPart.split(',', Qt::SkipEmptyParts);

    for (QString item : rawItems) {
        item = item.trimmed();

        // A. Resolve Variables
        QString resolved = varResolver(item);

        // B. Check List Reference
        if (isListReference(resolved)) {
            QString refKey = getListNameFromReference(resolved);
            if (m_lists.contains(refKey.toLower())) {
                itemsToAdd.append(m_lists[refKey.toLower()]);
            }
        } 
        else {
            // C. Text/Value
            itemsToAdd.append(resolved);
        }
    }

    // 3. Save (Append to existing list, or create new)
    if (m_lists.contains(key)) {
        m_lists[key].append(itemsToAdd);
    } else {
        m_lists[key] = itemsToAdd;
    }
    
    saveLists();
    
    qDebug() << "[ListManager] Add list" << key << "- Added" << itemsToAdd.size() << "items. New total:" << m_lists[key].size();
}

void ListManager::performAddNoDub(const QString &rawLine, std::function<QString(QString)> varResolver) {
    if (rawLine.isEmpty()) return;

    // 1. Separate Target List Name from Values
    // Syntax: *listname,val1,val2...
    int firstComma = rawLine.indexOf(',');
    
    QString targetNameRaw;
    QString valuesPart;

    if (firstComma == -1) {
        // No values to add, nothing to do
        return; 
    } else {
        targetNameRaw = rawLine.left(firstComma).trimmed();
        valuesPart = rawLine.mid(firstComma + 1);
    }

    // Validation
    if (!targetNameRaw.startsWith('*')) {
        qWarning() << "[ListManager] Error: List name in AddNoDub* must start with asterisk. Got:" << targetNameRaw;
        return;
    }

    QString key = targetNameRaw.mid(1).toLower();

    // 2. Prepare Candidates (Resolve variables and expand lists)
    QStringList candidates;
    QStringList rawItems = valuesPart.split(',', Qt::SkipEmptyParts);

    for (QString item : rawItems) {
        item = item.trimmed();

        QString resolved = varResolver(item);

        if (isListReference(resolved)) {
            QString refKey = getListNameFromReference(resolved);
            if (m_lists.contains(refKey.toLower())) {
                candidates.append(m_lists[refKey.toLower()]);
            }
        } 
        else {
            candidates.append(resolved);
        }
    }

    // 3. Append Unique Items Only
    // We create the list if it doesn't exist
    if (!m_lists.contains(key)) {
        m_lists[key] = QStringList();
    }

    QStringList &targetList = m_lists[key];
    int addedCount = 0;

    for (const QString &item : candidates) {
        // Case Sensitivity: Usually strict for lists, change to CaseInsensitive if needed
        if (!targetList.contains(item)) {
            targetList.append(item);
            addedCount++;
        }
    }
    
    // Only save if we actually modified the list
    if (addedCount > 0) {
        saveLists();
    }
    
    qDebug() << "[ListManager] AddNoDub list" << key << "- Added" << addedCount << "unique items. Total:" << targetList.size();
}

void ListManager::performAddSplit(const QString &rawLine, std::function<QString(QString)> varResolver) {
    if (rawLine.isEmpty()) return;

    // 1. Separate Target List Name from Variable
    // Syntax: *listname,$var
    int firstComma = rawLine.indexOf(',');

    if (firstComma == -1) {
        qWarning() << "[ListManager] Error: AddSplit* requires a value (e.g. *list,$var). Got:" << rawLine;
        return; 
    }

    QString targetNameRaw = rawLine.left(firstComma).trimmed();
    QString valuePart = rawLine.mid(firstComma + 1);

    // Validation
    if (!targetNameRaw.startsWith('*')) {
        qWarning() << "[ListManager] Error: List name in AddSplit* must start with asterisk. Got:" << targetNameRaw;
        return;
    }

    QString key = targetNameRaw.mid(1).toLower();

    // 2. Resolve Variable
    QString resolved = varResolver(valuePart);

    // 3. Split by Newlines
    // Uses the same regex as SetSplit to handle \n, \r\n, etc.
    QStringList newItems = resolved.split(QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts);

    // 4. Save (Append to existing list, or create new)
    if (m_lists.contains(key)) {
        m_lists[key].append(newItems);
    } else {
        m_lists[key] = newItems;
    }
    
    saveLists();

    qDebug() << "[ListManager] AddSplit list" << key << "- Added" << newItems.size() << "items from variable.";
}

void ListManager::performPush(const QString &rawLine, std::function<QString(QString)> varResolver) {
    if (rawLine.isEmpty()) return;

    // Separate Target List Name from Values
    int firstComma = rawLine.indexOf(',');

    QString targetNameRaw;
    QString valuesPart;

    if (firstComma == -1) {
        return;
    } else {
        targetNameRaw = rawLine.left(firstComma).trimmed();
        valuesPart = rawLine.mid(firstComma + 1);
    }

    // Validation
    if (!targetNameRaw.startsWith('*')) {
        qWarning() << "[ListManager] Error: List name in Push* must start with asterisk. Got:" << targetNameRaw;
        return;
    }

    QString key = targetNameRaw.mid(1).toLower();

    // Prepare Items to Push
    QStringList itemsToPush;
    QStringList rawItems = valuesPart.split(',', Qt::SkipEmptyParts);

    for (QString item : rawItems) {
        item = item.trimmed();

        QString resolved = varResolver(item);

        if (isListReference(resolved)) {
            QString refKey = getListNameFromReference(resolved);
            if (m_lists.contains(refKey.toLower())) {
                itemsToPush.append(m_lists[refKey.toLower()]);
            }
        }
        else {
            itemsToPush.append(resolved);
        }
    }

    // Prepend to List
    if (m_lists.contains(key)) {
        // Create new list: [New Items] + [Old Items]
        QStringList currentList = m_lists[key];
        itemsToPush.append(currentList);
        m_lists[key] = itemsToPush;
    } else {
        // List didn't exist, just create it with new items
        m_lists[key] = itemsToPush;
    }

    saveLists();

    qDebug() << "[ListManager] Push list" << key << "- Pushed" << itemsToPush.size() - (m_lists.contains(key) ? 0 : 0) << "items to front.";
}

void ListManager::performRemove(const QString &rawLine, std::function<QString(QString)> varResolver) {
    if (rawLine.isEmpty()) return;

    // Separate Target List Name from Values
    int firstComma = rawLine.indexOf(',');

    QString targetNameRaw;
    QString valuesPart;

    if (firstComma == -1) {
        return;
    } else {
        targetNameRaw = rawLine.left(firstComma).trimmed();
        valuesPart = rawLine.mid(firstComma + 1);
    }

    // Validation
    if (!targetNameRaw.startsWith('*')) {
        qWarning() << "[ListManager] Error: List name in Remove* must start with with an asterisk. Got:" << targetNameRaw;
        return;
    }

    QString key = targetNameRaw.mid(1).toLower();

    // If the list doesn't exist, we can't remove anything
    if (!m_lists.contains(key)) {
        return;
    }

    // Prepare Items to Remove
    QStringList itemsToRemove;
    QStringList rawItems = valuesPart.split(',', Qt::SkipEmptyParts);

    for (QString item : rawItems) {
        item = item.trimmed();

        QString resolved = varResolver(item);

        if (isListReference(resolved)) {
            QString refKey = getListNameFromReference(resolved);
            if (m_lists.contains(refKey.toLower())) {
                itemsToRemove.append(m_lists[refKey.toLower()]);
            }
        }
        else {
            itemsToRemove.append(resolved);
        }
    }

    // Remove Items (First Instance Only)
    QStringList &targetList = m_lists[key];
    bool changed = false;

    for (const QString &item : itemsToRemove) {
        if (targetList.removeOne(item)) {
            changed = true;
        }
    }

    if (changed) {
        saveLists();
        qDebug() << "[ListManager] Remove list" << key << "- Removed items. New size:" << targetList.size();
    }
}

void ListManager::performRemoveAll(const QString &rawLine, std::function<QString(QString)> varResolver) {
    if (rawLine.isEmpty()) return;

    // Separate Target List Name from values
    int firstComma = rawLine.indexOf(',');

    QString targetNameRaw;
    QString valuesPart;

    if (firstComma == -1) {
        // No values specified to remove
        return;
    } else {
        targetNameRaw = rawLine.left(firstComma).trimmed();
        valuesPart = rawLine.mid(firstComma + 1);
    }

    QString key = targetNameRaw.mid(1).toLower();

    // If the list doesn't exist, we can't remove anything
    if (!m_lists.contains(key)) {
        return;
    }

    // Prepare Items to Remove
    QStringList itemsToRemove;
    QStringList rawItems = valuesPart.split(',', Qt::SkipEmptyParts);

    for (QString item : rawItems) {
        item = item.trimmed();
        QString resolved = varResolver(item);

        if (isListReference(resolved)) {
            QString refKey = getListNameFromReference(resolved);
            if (m_lists.contains(refKey.toLower())) {
                itemsToRemove.append(m_lists[refKey.toLower()]);
            }
        }
        else {
            itemsToRemove.append(resolved);
        }
    }

    // Remove All Instances
    QStringList &targetList = m_lists[key];
    int totalRemoved = 0;
    for (const QString &item : itemsToRemove) {
        // removeAll returns the number of entries removed
        totalRemoved += targetList.removeAll(item);
    }

    if (totalRemoved > 0) {
        saveLists();
        qDebug() << "[ListManager] RemoveAll list" << key << "- Removed" << totalRemoved << "instances.";
    }
}

void ListManager::performPull(const QString &rawLine, 
                              std::function<QString(QString)> varResolver, 
                              std::function<void(QString, QString)> varSetter) 
{
    if (rawLine.isEmpty()) return;

    // 1. Separate Target List Name from Variable Name
    // Syntax: *listname,$var
    int firstComma = rawLine.indexOf(',');

    if (firstComma == -1) {
        qWarning() << "[ListManager] Error: Pull* requires a variable (e.g. *list,$var). Got:" << rawLine;
        return; 
    }

    QString targetNameRaw = rawLine.left(firstComma).trimmed();
    QString varName = rawLine.mid(firstComma + 1).trimmed();

    // Validation
    if (!targetNameRaw.startsWith('*')) {
        qWarning() << "[ListManager] Error: List name in Pull* must start with asterisk. Got:" << targetNameRaw;
        return;
    }
    // Validation: Variable must start with $ (or # for counters? usually lists imply strings $)
    if (!varName.startsWith('$')) {
        qWarning() << "[ListManager] Error: Variable in Pull* must start with $. Got:" << varName;
        return;
    }

    QString key = targetNameRaw.mid(1).toLower();

    // 2. Check if List Exists and is not Empty
    if (!m_lists.contains(key) || m_lists[key].isEmpty()) {
        qDebug() << "[ListManager] Warning: Pull* on empty/missing list:" << key;
        // Optionally set $var to empty string?
        varSetter(varName.mid(1), ""); 
        return;
    }

    // 3. Get the First Item (Head)
    QString value = m_lists[key].first();

    // 4. Set the Variable (Call back to CyberDom)
    // We strip the '$' because setVariable usually expects just "myVar"
    varSetter(varName.mid(1), value);

    // 5. Remove from List
    m_lists[key].removeFirst();
    saveLists();

    qDebug() << "[ListManager] Pull list" << key << "- Pulled:" << value << "into" << varName;
}

void ListManager::performIntersect(const QString &rawLine, std::function<QString(QString)> varResolver) {
    if (rawLine.isEmpty()) return;

    // 1. Separate Target List Name from Sources
    // Syntax: *NewList,*Source1,*Source2...
    int firstComma = rawLine.indexOf(',');
    
    if (firstComma == -1) {
        qWarning() << "[ListManager] Error: Intersect* requires sources (e.g. *New,*Src1,*Src2). Got:" << rawLine;
        return; 
    }

    QString targetNameRaw = rawLine.left(firstComma).trimmed();
    QString sourcesPart = rawLine.mid(firstComma + 1);

    // Validation
    if (!targetNameRaw.startsWith('*')) {
        qWarning() << "[ListManager] Error: List name in Intersect* must start with asterisk. Got:" << targetNameRaw;
        return;
    }

    QString targetKey = targetNameRaw.mid(1).toLower();

    // 2. Parse Sources
    QStringList rawItems = sourcesPart.split(',', Qt::SkipEmptyParts);
    if (rawItems.isEmpty()) return;

    // We need to establish the "Base" list (the first source)
    // and then filter it against all subsequent sources.
    QStringList resultList;
    bool firstSourceProcessed = false;

    for (QString item : rawItems) {
        item = item.trimmed();
        QString resolved = varResolver(item);
        
        QStringList currentSourceContent;

        // Get content of current source argument
        if (isListReference(resolved)) {
            QString refKey = getListNameFromReference(resolved);
            if (m_lists.contains(refKey.toLower())) {
                currentSourceContent = m_lists[refKey.toLower()];
            }
        } else {
            // Treat a literal value as a list containing just that value
            currentSourceContent.append(resolved);
        }

        if (!firstSourceProcessed) {
            // This is the first source, so it becomes our starting point
            resultList = currentSourceContent;
            firstSourceProcessed = true;
        } else {
            // Intersect: Keep elements in 'resultList' ONLY if they exist in 'currentSourceContent'
            // We iterate backwards or create a new list to avoid iterator invalidation issues while removing
            QStringList nextResult;
            for (const QString &val : resultList) {
                // Case sensitivity: Lists are usually case-sensitive, but let's match case-insensitively if safer?
                // Stick to standard strict matching for now unless requested otherwise.
                if (currentSourceContent.contains(val)) {
                    nextResult.append(val);
                }
            }
            resultList = nextResult;
        }

        // Optimization: If result is empty, we can stop early
        if (resultList.isEmpty()) break;
    }

    // 3. Save Result
    m_lists[targetKey] = resultList;
    saveLists();
    
    qDebug() << "[ListManager] Intersect list" << targetKey << "- Result size:" << resultList.size();
}

void ListManager::performClear(const QString &rawLine) {
    if (rawLine.isEmpty()) return;

    // 1. Get List Name
    // Syntax: *listname
    QString targetNameRaw;
    int firstComma = rawLine.indexOf(',');

    if (firstComma == -1) {
        targetNameRaw = rawLine.trimmed();
    } else {
        targetNameRaw = rawLine.left(firstComma).trimmed();
    }

    // Validation
    if (!targetNameRaw.startsWith('*')) {
        qWarning() << "[ListManager] Error: List name in Clear* must start with asterisk. Got:" << targetNameRaw;
        return;
    }

    QString key = targetNameRaw.mid(1).toLower();

    // 2. Empty the List (But keep the key in the map)
    if (m_lists.contains(key)) {
        m_lists[key].clear(); // Sets count to 0, but key remains
        saveLists();          // Will write "listname=" or similar to .ini
        qDebug() << "[ListManager] Cleared list" << key << "(Container preserved)";
    }
}

void ListManager::performDrop(const QString &rawLine) {
    if (rawLine.isEmpty()) return;

    // 1. Get List Name
    // Syntax: *listname
    QString targetNameRaw;
    int firstComma = rawLine.indexOf(',');

    if (firstComma == -1) {
        targetNameRaw = rawLine.trimmed();
    } else {
        targetNameRaw = rawLine.left(firstComma).trimmed();
    }

    // Validation
    if (!targetNameRaw.startsWith('*')) {
        qWarning() << "[ListManager] Error: List name in Drop* must start with asterisk. Got:" << targetNameRaw;
        return;
    }

    QString key = targetNameRaw.mid(1).toLower();

    // 2. Drop the List
    if (m_lists.contains(key)) {
        m_lists.remove(key); // Completely removes the key from the Map
        saveLists();         // Syncs to file, removing the line from .ini
        qDebug() << "[ListManager] Dropped list" << key;
    } else {
        qDebug() << "[ListManager] Drop* called on non-existent list:" << key << "(Ignored)";
    }
}