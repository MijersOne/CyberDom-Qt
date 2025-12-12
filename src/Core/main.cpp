#include "cyberdom.h"

#include <QApplication>
#include <QDebug>
#include <QTextStream>
#include <QFile>
#include <QDir>
#include <QDateTime>
#include <csignal>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QStringConverter>
#endif

#ifndef _WIN32
#include <execinfo.h>
#else
#include <windows.h>
#endif

static QString g_logFilePath;

void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context);

    // Determine the message type tag
    QString typeStr;
    switch (type) {
    case QtDebugMsg:    typeStr = "[DEBUG]"; break;
    case QtInfoMsg:     typeStr = "[INFO]"; break;
    case QtWarningMsg:  typeStr = "[WARN]"; break;
    case QtCriticalMsg: typeStr = "[CRIT]"; break;
    case QtFatalMsg:    typeStr = "[FATAL]"; break;
}

// Get System Time (formatted)
QString timeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

// Open the file (using the global path)
QFile logFile(g_logFilePath);
if (logFile.open(QIODevice::WriteOnly | QIODevice::Append))
{
    QTextStream out(&logFile);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    out.setEncoding(QStringConverter::Utf8);
#else
    out.setCodec("UTF-8");
#endif
        // Write the formatted line
        // Format: [Date Time] [Type] Message
        out << QString("[%1] %2 %3").arg(timeStr, typeStr, msg) << Qt::endl;
    }
}

CyberDom *mainApp = nullptr;

void crashHandler(int sig) {
    void *array[10];
    size_t size;

    // Get void*'s for all entries on the stack
    size = backtrace(array, 10);

    // Print out all the frames to stderr
    fprintf(stderr, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);

    // Also try to write to our log file
    QFile file(g_logFilePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream out(&file);
        out << "\n[CRITICAL] APP CRASHED with Signal " << sig << "\nStack Trace:\n";

        char **messages = backtrace_symbols(array, size);
        for (size_t i = 0; i < size; ++i) {
            out << messages[i] << "\n";
        }
        free(messages);
    }

    exit(1);
}

int main(int argc, char *argv[])
{
    // Instantiate QApplication
    QApplication a(argc, argv);

    // --- LOGGING SETUP ---

    // Ensure "Logs" directory exists
    QString logDir = QCoreApplication::applicationDirPath() + "/Logs";
    QDir dir(logDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // Log Cleanup (Keep Only the latest 10)
    // Filter for log files
    dir.setNameFilters(QStringList() << "Log_*.txt");
    dir.setFilter(QDir::Files);

    // Sort by Name (ISO timestamps mean alphabetical = chronological)
    // Oldest files will be at the start of the list
    dir.setSorting(QDir::Name);

    QFileInfoList logFiles = dir.entryInfoList();

    // Delete oldest logs until we have 9 or fewer
    // (So when we create a new one, we have exactly 10)
    while (logFiles.size() >= 10) {
        QString oldestLog = logFiles.first().absoluteFilePath();
        QFile::remove(oldestLog);
        qDebug() << "Deleted old log file:" << oldestLog;
        logFiles.removeFirst(); // Remove from list to update count
    }

    // Generate Filename based on System Date/Time
    // Format: Log_YYYY-MM-DD_HH-mm-ss.txt
    QString timeStamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
    g_logFilePath = logDir + "/Log_" + timeStamp + ".txt";

    // Install the handler
    qInstallMessageHandler(customMessageHandler);

    signal(SIGSEGV, crashHandler);
    signal(SIGABRT, crashHandler);

    // --- END LOGGING SETUP ---

#ifndef _WIN32
    std::signal(SIGSEGV, crashHandler);
    std::signal(SIGABRT, crashHandler);
#else
    SetUnhandledExceptionFilter(winCrashHandler);
#endif
    qInstallMessageHandler(customMessageHandler);

    // Log that the app has started
    qDebug() << "==========================================";
    qDebug() << "   CyberDom Application Started";
    qDebug() << "   Session Log:" << g_logFilePath;
    qDebug() << "==========================================";

    // Create and show the main window
    CyberDom w;
    mainApp = &w;
    w.show();

    return a.exec();
}
