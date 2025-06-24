#include <QtTest>
#include "../scriptparser.h"

class SigninIntervalTest : public QObject {
    Q_OBJECT
private slots:
    void parseMixedCase();
};

void SigninIntervalTest::parseMixedCase() {
    ScriptParser parser;
    QVERIFY(parser.parseScript("tests/mixedcase_script.ini"));

    auto s1 = parser.getStatus("test1");
    QCOMPARE(s1.signinIntervalMin, QString("1"));
    QCOMPARE(s1.signinIntervalMax, QString("2"));

    auto s2 = parser.getStatus("test2");
    QCOMPARE(s2.signinIntervalMin, QString("3"));
    QCOMPARE(s2.signinIntervalMax, QString("4"));

    auto s3 = parser.getStatus("test3");
    QCOMPARE(s3.signinIntervalMin, QString("5"));
    QCOMPARE(s3.signinIntervalMax, QString("6"));
}

QTEST_MAIN(SigninIntervalTest)
#include "signininterval_test.moc"
