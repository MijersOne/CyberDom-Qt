#include "linewriter.h"
#include "ui_linewriter.h"
#include <QMessageBox>
#include <QKeyEvent>
#include <QDebug>
#include <QScrollBar>
#include <QMediaPlayer>
#include <QAudioOutput>

LineWriter::LineWriter(const QStringList &expectedLines, int maxBreakSeconds, const QString &alarmSoundPath, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LineWriter)
    , expectedLines(expectedLines)
    , maxBreakSeconds(maxBreakSeconds)
    , alarmSoundPath(alarmSoundPath)
{
    ui->setupUi(this);

    ui->text_Submitted->setReadOnly(true);
    ui->line_Input->setContextMenuPolicy(Qt::NoContextMenu);
    ui->line_Input->installEventFilter(this);

    connect(ui->line_Input, &QLineEdit::returnPressed, this, &LineWriter::submitLine);
    connect(ui->line_Input, &QLineEdit::textEdited, this, &LineWriter::onTextEdited); // Track activity

    connect(ui->btn_Submit, &QPushButton::clicked, this, &LineWriter::submitLine);
    connect(ui->btn_Discard, &QPushButton::clicked, this, &LineWriter::onDiscardClicked);
    connect(ui->btn_Abort, &QPushButton::clicked, this, &QDialog::reject);

    currentIndex = 0;
    currentPageStart = 0;

    // --- Inactivity Setup ---
    lastActivityTime = QDateTime::currentDateTime();
    warningShown = false;

    if (maxBreakSeconds > 0) {
        inactivityTimer = new QTimer(this);
        connect(inactivityTimer, &QTimer::timeout, this, &LineWriter::checkInactivity);
        inactivityTimer->start(1000); // Check every second
        qDebug() << "[LineWriter] Inactivity timer started. Max break:" << maxBreakSeconds << "s";
    } else {
        inactivityTimer = nullptr;
    }

    updateCurrentPrompt();
    updateLabels();
}

LineWriter::~LineWriter() {
    delete ui;
}

bool LineWriter::eventFilter(QObject *obj, QEvent *event) {
    if (obj == ui->line_Input && event->type() == QEvent::KeyPress) {
        QKeyEvent *key = static_cast<QKeyEvent*>(event);
        if (key->matches(QKeySequence::Paste) || key->matches(QKeySequence::Copy) ||
            key->matches(QKeySequence::Cut) || key->key() == Qt::Key_Backspace ||
            key->key() == Qt::Key_Delete) {
            return true;
        }
        if (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter) {
            submitLine();
            return true;
        }

        // Any keypress counts as activity
        onTextEdited();
    }
    return QDialog::eventFilter(obj, event);
}

void LineWriter::onTextEdited() {
    lastActivityTime = QDateTime::currentDateTime();
    if (warningShown) {
        warningShown = false; // Reset warning flag if they start typing again
    }
}

void LineWriter::checkInactivity() {
    if (maxBreakSeconds <= 0) return;

    qint64 elapsed = lastActivityTime.secsTo(QDateTime::currentDateTime());

    // 1. Check for Failure (100% time)
    if (elapsed >= maxBreakSeconds) {
        inactivityTimer->stop();
        playAlarm();
        QMessageBox::critical(this, tr("Failed"), tr("You took too long to write the line. The assignment is aborted."));
        reject(); // Abort assignment
        return;
    }

    // 2. Check for Warning (90% time)
    // Only warn once per idle period
    if (!warningShown && elapsed >= (maxBreakSeconds * 0.9)) {
        warningShown = true;
        playAlarm();
        QMessageBox::warning(this, tr("Warning"), tr("You are taking too long! Start typing immediately!"));
        // Note: MessageBox blocks the thread in some environments, but QTimer usually continues.
        // If the user ignores the box, 'elapsed' continues to grow on next tick.
        // Closing the box (interacting) updates 'lastActivityTime' via keypress/mouse events if handled,
        // but here we rely on them typing in the box to clear it.

        // Update time to account for time spent looking at the message box?
        // Strict interpretation says "time without typing", so looking at a message box IS "not typing".
    }
}

void LineWriter::playAlarm() {
    if (alarmSoundPath.isEmpty()) {
        QApplication::beep();
        return;
    }

    QMediaPlayer *player = new QMediaPlayer(this);
    QAudioOutput *audioOutput = new QAudioOutput(this);
    player->setAudioOutput(audioOutput);
    player->setSource(QUrl::fromLocalFile(alarmSoundPath));
    audioOutput->setVolume(1.0);
    player->play();

    // Auto-delete when done
    connect(player, &QMediaPlayer::mediaStatusChanged, [player, audioOutput](QMediaPlayer::MediaStatus status){
        if(status == QMediaPlayer::EndOfMedia) {
            player->deleteLater();
            audioOutput->deleteLater();
        }
    });
}

void LineWriter::updateCurrentPrompt() {
    if (currentIndex < expectedLines.size()) {
        ui->line_Write->setText(expectedLines.at(currentIndex));
    } else {
        ui->line_Write->setText(QString());
    }
}

void LineWriter::updateLabels() {
    int totalPages = (expectedLines.size() + PAGE_SIZE - 1) / PAGE_SIZE;
    int currentPage = (currentIndex / PAGE_SIZE) + 1;
    ui->lbl_PageN->setText(QString("Page %1/%2").arg(currentPage).arg(totalPages));
    ui->lbl_LineN->setText(QString("Line %1/%2").arg(currentIndex + 1).arg(expectedLines.size()));
}

void LineWriter::updateTextBox() {
    ui->text_Submitted->setPlainText(currentPageLines.join("\n"));
    ui->text_Submitted->verticalScrollBar()->setValue(ui->text_Submitted->verticalScrollBar()->maximum());
}

void LineWriter::submitLine() {
    QString typed = ui->line_Input->text();

    if (typed.isEmpty()) return;

    ui->line_Input->clear();

    // Activity detected
    onTextEdited();

    currentPageLines.append(typed);
    currentIndex++;

    updateTextBox();
    updateLabels();

    if (currentPageLines.size() >= PAGE_SIZE || currentIndex >= expectedLines.size()) {
        checkPage();
    } else {
        updateCurrentPrompt();
    }
}

void LineWriter::onDiscardClicked() {
    currentIndex = currentPageStart;
    currentPageLines.clear();

    ui->text_Submitted->clear();
    ui->line_Input->clear();

    // Reset activity timer on discard? Yes, it's an interaction.
    onTextEdited();

    updateCurrentPrompt();
    updateLabels();
}

void LineWriter::checkPage() {
    bool correct = true;
    int errorIndex = -1;

    for (int i = 0; i < currentPageLines.size(); ++i) {
        int globalIndex = currentPageStart + i;

        if (globalIndex >= expectedLines.size()) {
            correct = false;
            break;
        }

        if (currentPageLines[i] != expectedLines[globalIndex]) {
            correct = false;
            errorIndex = i + 1;
            break;
        }
    }

    if (correct) {
        currentPageLines.clear();
        currentPageStart = currentIndex;
        ui->text_Submitted->clear();

        if (currentIndex >= expectedLines.size()) {
            QMessageBox::information(this, tr("Complete"), tr("All lines submitted successfully."));
            accept();
        } else {
            updateCurrentPrompt();
            updateLabels();
        }
    } else {
        QString msg = tr("Page not accepted.");
        if (errorIndex != -1) {
            msg += tr("\nMistake found on line %1 of this page.").arg(errorIndex);
        }
        QMessageBox::warning(this, tr("Incorrect"), msg);
        onDiscardClicked();
    }
}
