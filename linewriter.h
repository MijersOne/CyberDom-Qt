#ifndef LINEWRITER_H
#define LINEWRITER_H

#include <QDialog>
#include <QStringList>
#include <QTimer>
#include <QDateTime>
#include <QUrl>

namespace Ui {
class LineWriter;
}

class LineWriter : public QDialog {
    Q_OBJECT
public:
    // Constructor takes the exact list of lines to be written
    explicit LineWriter(const QStringList &expectedLines,
                        int maxBreakSeconds,
                        const QString &alarmSoundPath,
                        QWidget *parent = nullptr);
    ~LineWriter();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void submitLine();
    void onDiscardClicked();
    void onTextEdited();
    void checkInactivity();

private:
    void updateCurrentPrompt();
    void updateTextBox();
    void checkPage();
    void updateLabels();
    void playAlarm();

    Ui::LineWriter *ui;
    QStringList expectedLines;      // The full list of lines to write
    QStringList currentPageLines;   // The lines typed so far on this page
    int currentIndex = 0;           // The total index (0 to expectedLines.size())
    int currentPageStart = 0;       // The total index where this page started

    const int PAGE_SIZE = 5;        // The number of lines per page

    // Inactivity Tracking
    int maxBreakSeconds;
    QString alarmSoundPath;
    QDateTime lastActivityTime;
    QTimer *inactivityTimer;
    bool warningShown;
};

#endif // LINEWRITER_H
