#ifndef LISTMANAGER_H
#define LISTMANAGER_H

#include <QObject>
#include <QMap>
#include <QStringList>
#include <QSettings>
#include <functional>
#include <qtmetamacros.h>

class ListManager : public QObject {
    Q_OBJECT

public:
    explicit ListManager(const QString &settingsFile, QObject *parent = nullptr);

    // Loads [Lists] from user_settings.ini
    void loadLists();

    // Saves m_lists back to user_settings.ini
    void saveLists();

    // The Set* command
    // Syntax: *listname,value,value
    void performSet(const QString &rawLine, std::function<QString(QString)> varResolver);
    void performSetSplit(const QString &rawLine, std::function<QString(QString)> varResolver);
    void performAdd(const QString &rawline, std::function<QString(QString)> varResolver);
    void performAddNoDub(const QString &rawLine, std::function<QString(QString)> varResolver);
    void performAddSplit(const QString &rawLine, std::function<QString(QString)> varResolver);
    void performPush(const QString &rawLine, std::function<QString(QString)> varResolver);
    void performRemove(const QString &rawLine, std::function<QString(QString)> varResolver);
    void performRemoveAll(const QString &rawLine, std::function<QString(QString)> varResolver);
    void performPull(const QString &rawLine, std::function<QString(QString)> varResolver, std::function<void(QString, QString)> varSetter);
    void performIntersect(const QString &rawLine, std::function<QString(QString)> varResolver);
    void performClear(const QString &rawLine);
    void performDrop(const QString &rawLine);
    void performSort(const QString &rawLine);

    // Helper to get a list
    QStringList getList(const QString &name) const;

private:
    QString m_settingsFile;

    // Key: List Name (Lowercased, No asterisk)
    QMap<QString, QStringList> m_lists;

    // Helper: checks if a string is "*somelist"
    bool isListReference(const QString &val) const;

    // Helper: converts "*someList" -> "somelist"
    QString getListNameFromReference(const QString &ref) const;
};

#endif // LISTMANAGER_H