#include "mainwindow.h"
#include "cyberdom.h"
#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>
#include <QDebug>
#include <QStandardPaths>
#include <iostream>
#include <csignal>

// --- Platform Specific Headers ---
#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#else
#include <execinfo.h>
#include <unistd.h>
#include <signal.h>
#endif

// Global pointer for main app instance
CyberDom *mainApp = nullptr;
QString g_logFilePath;

// Custom Message Handler
void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context);

    QString txt;
    switch (type) {
    case QtDebugMsg:    txt = QString("Debug: %1").arg(msg); break;
    case QtInfoMsg:     txt = QString("Info: %1").arg(msg); break;
    case QtWarningMsg:  txt = QString("Warning: %1").arg(msg); break;
    case QtCriticalMsg: txt = QString("Critical: %1").arg(msg); break;
    case QtFatalMsg:    txt = QString("Fatal: %1").arg(msg); break;
    }

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    QString formattedMessage = QString("[%1] %2").arg(timestamp, txt);

    // 1. Print to Console (Standard Output)
    std::cout << formattedMessage.toStdString() << std::endl;

    // 2. Write to Log File
    if (!g_logFilePath.isEmpty()) {
        QFile outFile(g_logFilePath);
        if (outFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
            QTextStream ts(&outFile);
            ts << formattedMessage << Qt::endl;
        }
    }

    if (type == QtFatalMsg) {
        abort();
    }
}

// --- Crash Handler Implementation ---

void logCrash(const QString &stackTrace) {
    QString report = "\n!!! CRASH DETECTED !!!\n";
    report += "Timestamp: " + QDateTime::currentDateTime().toString(Qt::ISODate) + "\n";
    report += "Stack Trace:\n" + stackTrace + "\n";
    report += "--------------------------\n";

    // Print to stderr
    std::cerr << report.toStdString() << std::endl;

    // Write to file
    if (!g_logFilePath.isEmpty()) {
        QFile outFile(g_logFilePath);
        if (outFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
            QTextStream ts(&outFile);
            ts << report << Qt::endl;
        }
    }
}

#ifdef _WIN32
// --- WINDOWS CRASH HANDLER ---
LONG WINAPI winCrashHandler(EXCEPTION_POINTERS *ExceptionInfo)
{
    QString stackTrace;

    // Initialize DbgHelp symbols
    HANDLE process = GetCurrentProcess();
    SymInitialize(process, NULL, TRUE);

    // Setup stack frame
    STACKFRAME64 stackFrame;
    memset(&stackFrame, 0, sizeof(stackFrame));

    DWORD machineType = IMAGE_FILE_MACHINE_AMD64; // Assuming 64-bit build

// Context is platform specific
#ifdef _M_X64
    machineType = IMAGE_FILE_MACHINE_AMD64;
    stackFrame.AddrPC.Offset = ExceptionInfo->ContextRecord->Rip;
    stackFrame.AddrPC.Mode = AddrModeFlat;
    stackFrame.AddrFrame.Offset = ExceptionInfo->ContextRecord->Rbp;
    stackFrame.AddrFrame.Mode = AddrModeFlat;
    stackFrame.AddrStack.Offset = ExceptionInfo->ContextRecord->Rsp;
    stackFrame.AddrStack.Mode = AddrModeFlat;
#else
    machineType = IMAGE_FILE_MACHINE_I386;
    stackFrame.AddrPC.Offset = ExceptionInfo->ContextRecord->Eip;
    stackFrame.AddrPC.Mode = AddrModeFlat;
    stackFrame.AddrFrame.Offset = ExceptionInfo->ContextRecord->Ebp;
    stackFrame.AddrFrame.Mode = AddrModeFlat;
    stackFrame.AddrStack.Offset = ExceptionInfo->ContextRecord->Esp;
    stackFrame.AddrStack.Mode = AddrModeFlat;
#endif

    // Walk the stack
    while (StackWalk64(machineType, process, GetCurrentThread(), &stackFrame,
                       ExceptionInfo->ContextRecord, NULL, SymFunctionTableAccess64,
                       SymGetModuleBase64, NULL)) {

        if (stackFrame.AddrPC.Offset == 0) break;

        // Resolve symbol name
        char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
        PSYMBOL_INFO symbol = (PSYMBOL_INFO)buffer;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_SYM_NAME;

        DWORD64 displacement = 0;
        if (SymFromAddr(process, stackFrame.AddrPC.Offset, &displacement, symbol)) {
            stackTrace += QString("  %1 (0x%2)\n")
            .arg(symbol->Name)
                .arg(QString::number(stackFrame.AddrPC.Offset, 16));
        } else {
            stackTrace += QString("  [Unknown Function] (0x%1)\n")
            .arg(QString::number(stackFrame.AddrPC.Offset, 16));
        }
    }

    SymCleanup(process);

    logCrash(stackTrace);

    // Standard Windows crash dialog
    return EXCEPTION_CONTINUE_SEARCH;
}

#else
// --- LINUX / MAC CRASH HANDLER ---
void crashHandler(int signal)
{
    void *array[20];
    size_t size;

    // get void*'s for all entries on the stack
    size = backtrace(array, 20);

    // print out all the frames to stderr
    char **strings = backtrace_symbols(array, size);

    QString stackTrace;
    if (strings) {
        for (size_t i = 0; i < size; i++) {
            stackTrace += QString::fromLocal8Bit(strings[i]) + "\n";
        }
        free(strings);
    }

    logCrash(stackTrace);

    // Restore default handler and re-raise to crash properly
    std::signal(signal, SIG_DFL);
    std::raise(signal);
}
#endif

int main(int argc, char *argv[])
{
    // 1. Instantiate QApplication FIRST
    QApplication a(argc, argv);

    // --- LOGGING SETUP ---
    QString logDir = QCoreApplication::applicationDirPath() + "/Logs";
    QDir dir(logDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // --- Log Cleanup (Keep only latest 10) ---
    dir.setNameFilters(QStringList() << "Log_*.txt");
    dir.setFilter(QDir::Files);
    dir.setSorting(QDir::Name); // Sort by Name (Oldest first)

    QFileInfoList logFiles = dir.entryInfoList();

    while (logFiles.size() >= 10) {
        QString oldestLog = logFiles.first().absoluteFilePath();
        QFile::remove(oldestLog);
        qDebug() << "Deleted old log file:" << oldestLog;
        logFiles.removeFirst();
    }
    // ----------------------------------------------

    // Generate Filename
    QString timeStamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
    g_logFilePath = logDir + "/Log_" + timeStamp + ".txt";

    qInstallMessageHandler(customMessageHandler);

// --- INSTALL CRASH HANDLERS ---
#ifdef _WIN32
    SetUnhandledExceptionFilter(winCrashHandler);
#else
    std::signal(SIGSEGV, crashHandler);
    std::signal(SIGABRT, crashHandler);
    std::signal(SIGFPE, crashHandler);
#endif

    qDebug() << "==========================================";
    qDebug() << "   CyberDom Application Started";
    qDebug() << "   Session Log:" << g_logFilePath;
    qDebug() << "==========================================";

    CyberDom w;
    mainApp = &w;
    w.show();

    return a.exec();
}
