#include "cyberdom.h"

#include <QApplication>
#include <QDebug>
#include <QTextStream>
#include <QFile>
#include <csignal>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QStringConverter>
#endif

#ifndef _WIN32
#include <execinfo.h>
#else
#include <windows.h>
#endif

void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(type);
    Q_UNUSED(context);

    QFile logFile("debug_output.log");
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Append))
    {
        QTextStream out(&logFile);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        out.setEncoding(QStringConverter::Utf8);
#else
        out.setCodec("UTF-8");
#endif
        out << msg << Qt::endl;
    }
}

CyberDom *mainApp = nullptr;

#ifndef _WIN32
static void crashHandler(int signum)
{
    void *array[50];
    int size = backtrace(array, 50);
    QFile file("crash.log");
    if (file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream out(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        out.setEncoding(QStringConverter::Utf8);
#else
        out.setCodec("UTF-8");
#endif
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
#else
LONG WINAPI winCrashHandler(struct _EXCEPTION_POINTERS* ExceptionInfo)
{
    QFile file("crash.log");
    if (file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream out(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        out.setEncoding(QStringConverter::Utf8);
#else
        out.setCodec("UTF-8");
#endif
        out << "Unhandled exception caught! Code: 0x"
            << QString::number(ExceptionInfo->ExceptionRecord->ExceptionCode, 16).toUpper()
            <<"\n";
        file.close();
    }
    // Optional: call abort() or exit() if you want immediate termination
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
#ifndef _WIN32
    std::signal(SIGSEGV, crashHandler);
    std::signal(SIGABRT, crashHandler);
#else
    SetUnhandledExceptionFilter(winCrashHandler);
#endif
    qInstallMessageHandler(customMessageHandler);
    CyberDom w;
    mainApp = &w;
    w.show();
    return a.exec();
}
