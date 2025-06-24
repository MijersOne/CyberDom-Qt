#include <QtTest>
#include "../cyberdom.h"

class SessionSaveLoadTest : public QObject {
    Q_OBJECT
private slots:
    void saveLoad();
};

void SessionSaveLoadTest::saveLoad() {
    QString scriptPath = "tests/mixedcase_script.ini";
    QString sessionPath = QDir::temp().absoluteFilePath("cyberdom_session_test.cdt");

    // First instance - set data and save
    CyberDom first;
    first.loadAndParseScript(scriptPath);
    first.lastInstructions = "Do chores";
    first.lastClothingInstructions = "Wear uniform";

    FlagData fd;
    fd.name = "flag1";
    fd.setTime = QDateTime::currentDateTime();
    fd.expiryTime = fd.setTime.addSecs(60);
    fd.text = "flag text";
    fd.groups = {"g1","g2"};
    first.flags.insert("flag1", fd);

    first.questionAnswers["q1"] = "a1";
    first.reportCounts["rep1"] = 2;
    first.confessionCounts["conf1"] = 1;
    first.permissionCounts["perm1"] = 3;

    first.saveSessionData(sessionPath);

    // Second instance - load
    CyberDom second;
    QVERIFY(second.loadSessionData(sessionPath));

    QCOMPARE(second.lastInstructions, first.lastInstructions);
    QCOMPARE(second.lastClothingInstructions, first.lastClothingInstructions);
    QCOMPARE(second.flags.size(), first.flags.size());
    QVERIFY(second.flags.contains("flag1"));
    QCOMPARE(second.flags["flag1"].text, QString("flag text"));
    QCOMPARE(second.flags["flag1"].groups, QStringList({"g1","g2"}));
    QCOMPARE(second.questionAnswers, first.questionAnswers);
    QCOMPARE(second.reportCounts, first.reportCounts);
    QCOMPARE(second.confessionCounts, first.confessionCounts);
    QCOMPARE(second.permissionCounts, first.permissionCounts);
}

QTEST_MAIN(SessionSaveLoadTest)
#include "session_test.moc"
