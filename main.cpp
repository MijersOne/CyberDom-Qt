#include "cyberdom.h"

#include <QApplication>
#include <QDebug>
#include <QTextStream>
#include <QFile>
#include <csignal>
#include <execinfo.h>

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

static void crashHandler(int signum)
{
    void *array[50];
    int size = backtrace(array, 50);
    QFile file("crash.log");
    if (file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream out(&file);
        out << "Signal " << signum << " received\n";
        char **symbols = backtrace_symbols(array, size);
        for (int i = 0; i < size; ++i) {
            out << symbols[i] << '\n';
        }
        free(symbols);
        file.close();
    }

    // Re-raise default handler to terminate
    signal(signum, SIG_DFL);
    raise(signum);
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    std::signal(SIGSEGV, crashHandler);
    std::signal(SIGABRT, crashHandler);
    qInstallMessageHandler(customMessageHandler);
    CyberDom w;
    mainApp = &w;
    w.show();
    return a.exec();
}
