#include "cyberdom.h"

#include <QApplication>
#include <QDebug>
#include <QTextStream>
#include <QFile>

void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(type);
    Q_UNUSED(context);

    QFile logFile("debug_output.log");
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Append))
    {
        QTextStream out(&logFile);
        out << msg << Qt::endl;
    }
}

CyberDom *mainApp = nullptr;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    qInstallMessageHandler(customMessageHandler);
    CyberDom w;
    mainApp = &w;
    w.show();
    return a.exec();
}
