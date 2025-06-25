#include <QtTest/QtTest>
#include "cyberdom.h"
#include "assignments.h"
#include <QTemporaryDir>
#include <QStandardPaths>
#include <QSettings>
#include <QFile>
#include <QTextStream>

CyberDom *mainApp = nullptr;

class PunishmentTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void severityToAmount();
    void assignmentDeadline();
    void jobDefaultDeadline();
    void forbidPermission();

private:
    QString sessionPath;
    QString scriptPath;
    QDateTime baseTime;
    QTemporaryDir tempDir;
};

void PunishmentTest::initTestCase() {
    QVERIFY(tempDir.isValid());
    scriptPath = tempDir.filePath("script.ini");
    QFile script(scriptPath);
    QVERIFY(script.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&script);
    out << "[General]\n";
    out << "MinVersion=1\n";
    out << "[punishment-test]\n";
    out << "ValueUnit=minute\n";
    out << "value=10\n";
    out << "min=1\n";
    out << "max=20\n";
    out << "Group=testgrp\n";
    out << "[status-Normal]\n";
    out << "Text=Ready\n";
    out << "[job-feedfish]\n";
    out << "Text=Feed the fish\n";
    out << "[permission-shower]\n";
    out << "Text=Take a hot bath/shower\n";
    out << "[punishment-nohot]\n";
    out << "ValueUnit=once\n";
    out << "Forbid=shower\n";
    script.close();

    sessionPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/session.cdt";
    QDir().mkpath(QFileInfo(sessionPath).path());

    baseTime = QDateTime(QDate(2024,1,1), QTime(12,0));
    QSettings session(sessionPath, QSettings::IniFormat);
    session.setValue("Session/ScriptPath", scriptPath);
    session.setValue("Session/Status", "Normal");
    session.setValue("Session/InternalClock", baseTime.toString(Qt::ISODate));
    session.setValue("Session/LastSystemTime", baseTime.toString(Qt::ISODate));
    session.sync();
}

void PunishmentTest::severityToAmount() {
    QFile::remove(sessionPath);
    CyberDom app;
    mainApp = &app;
    app.applyPunishment(30, "testgrp");
    QCOMPARE(app.getPunishmentAmount("test"), 3);
}

void PunishmentTest::assignmentDeadline() {
    QFile::remove(sessionPath);
    CyberDom app;
    mainApp = &app;
    app.applyPunishment(30, "testgrp");
    Assignments w(nullptr, &app);
    w.populateJobList();
    auto table = w.findChild<QTableWidget*>("table_Assignments");
    QVERIFY(table);
    QString displayed = table->item(0,0)->text();
    QMap<QString, QDateTime> deadlines = app.getJobDeadlines();
    QVERIFY(deadlines.contains("test"));
    QString expected = deadlines["test"].toString("MM-dd-yyyy h:mm AP");
    QCOMPARE(displayed, expected);
}

void PunishmentTest::jobDefaultDeadline() {
    QFile::remove(sessionPath);
    CyberDom app;
    mainApp = &app;
    app.addJobToAssignments("feedfish");
    QMap<QString, QDateTime> deadlines = app.getJobDeadlines();
    QVERIFY(deadlines.contains("feedfish"));
    QCOMPARE(deadlines["feedfish"].time(), QTime(23, 59, 59));
}

void PunishmentTest::forbidPermission() {
    QFile::remove(sessionPath);
    CyberDom app;
    mainApp = &app;
    app.addPunishmentToAssignments("nohot");
    QVERIFY(app.isPermissionForbidden("shower"));
}

QTEST_MAIN(PunishmentTest)
#include "punishmenttest.moc"
