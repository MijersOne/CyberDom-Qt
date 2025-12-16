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
#include <QSysInfo>
#include <QDirIterator>
#include <QImageReader>

// --- Platform Specific Headers ---
#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#else
#include <execinfo.h>
#include <unistd.h>
#include <signal.h>
#endif

// --- VERSION HELPER ---
// We use the BUILD_VER passed from the .pro file (e.g. 0.0.2)
// and turn it into a string ("0.0.2") right here.
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#ifdef BUILD_VER
#define APP_VERSION_STR TOSTRING(BUILD_VER)
#else
#define APP_VERSION_STR "Unknown"
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

    // --- DESKTOP INTEGRATION ---
    // Set Organization and App Name (Used for WM_CLASS on Linux)
    a.setOrganizationName("Desire_Games");
    a.setApplicationName("CyberDom");

    // Set the Desktop File Name
    a.setDesktopFileName("cyberdom.desktop");

    // Icon Loader
    QString iconPath = ":/images/images/cyberdom_256x256.png";

    QIcon icon(iconPath);

    if (icon.isNull()) {
        qDebug() << "ERROR: Failed to load icon from" << iconPath;

        // Debugging: Try to see why
        QImageReader reader(iconPath);
        if (!reader.canRead()) {
            qDebug() << "Image Reader Error:" << reader.errorString();
        }

        // Fallback: Try the alias as a last resort
        QIcon fallback(":/images/cyberdom.png");
        if (!fallback.isNull()) {
            qDebug() << "Fallback to alias worked.";
            a.setWindowIcon(fallback);
        }
    } else {
        qDebug() << "SUCCESS: Loaded window icon from" << iconPath;
        a.setWindowIcon(icon);
    }

    // --- LOGGING SETUP ---
    QString docsLocation = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString logDir = docsLocation + "/CyberDom/Logs";
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
        std::cout << "Deleted old log file: " << oldestLog.toStdString() << std::endl;
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

    // --- DETECT DESKTOP ENVIRONMENT ---
    QString desktopEnv;
#ifdef Q_OS_LINUX
    // Try XDG_CURRENT_DESKTOP first (Standard)
    desktopEnv = qgetenv("XDG_CURRENT_DESKTOP");
    if (desktopEnv.isEmpty()) {
        // Fallback to DESKTOP_SESSION
        desktopEnv = qgetenv("DESKTOP_SESSION");
    }
    if (desktopEnv.isEmpty()) {
        // Fallback to GDMSESSION
        desktopEnv = qgetenv("GDMSESSION");
    }
    if (desktopEnv.isEmpty()) desktopEnv = "Unknown Linux DE";
#else
    desktopEnv = "N/A"; // Not relevant on Windows/MacOS
#endif

    qDebug() << "===================================================================================";
    qDebug() << "   CyberDom Application Started";
    // FIX: Use APP_VERSION_STR instead of APP_VERSION
    qDebug() << "   Version:        " << APP_VERSION_STR;
    qDebug() << "   Build Time:     " << __DATE__ << __TIME__;
    qDebug() << "   Session Log:    " << g_logFilePath;
    qDebug() << "   OS:             " << QSysInfo::prettyProductName();
    qDebug() << "   Kernel:         " << QSysInfo::kernelType() << QSysInfo::kernelVersion();
    qDebug() << "   Architecture:   " << QSysInfo::currentCpuArchitecture();
    qDebug() << "   Desktop Env:    " << desktopEnv;
    qDebug() << "===================================================================================";

    CyberDom w;
    mainApp = &w;
    w.show();

    return a.exec();
}
