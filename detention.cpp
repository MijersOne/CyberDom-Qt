#include "detention.h"
#include "ui_detention.h"
#include <QRandomGenerator>
#include <QDebug>
#include <QScreen>

Detention::Detention(int durationSeconds, const QString &instructions, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Detention)
    , remainingSeconds(durationSeconds)
{
    ui->setupUi(this);

    // --- 1. Secure the Window ---
    // Remove window frame (no X button) and keep on top
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    // Force fullscreen
    showFullScreen();

    ui->lbl_Instructions->setText(instructions);

    // --- 2. Setup Main Timer (Countdown) ---
    mainTimer = new QTimer(this);
    connect(mainTimer, &QTimer::timeout, this, &Detention::updateTimer);
    mainTimer->start(1000);
    updateTimer(); // Initial display

    // --- 3. Setup Dead Man's Switch Timer ---
    // Randomly show the button every 30-120 seconds
    checkTimer = new QTimer(this);
    checkTimer->setSingleShot(true);
    connect(checkTimer, &QTimer::timeout, this, &Detention::showCheckButton);
    connect(ui->btn_Check, &QPushButton::clicked, this, &Detention::onCheckButtonClicked);

    failTimer = new QTimer(this);
    failTimer->setSingleShot(true);
    connect(failTimer, &QTimer::timeout, this, &Detention::onCheckFailed);

    // Start first check
    int nextCheck = QRandomGenerator::global()->bounded(15000, 60000); // 15-60 seconds (adjust as needed)
    checkTimer->start(nextCheck);
}

Detention::~Detention()
{
    delete ui;
}

void Detention::updateTimer()
{
    remainingSeconds--;

    int hours = remainingSeconds / 3600;
    int minutes = (remainingSeconds % 3600) / 60;
    int seconds = remainingSeconds % 60;

    ui->lbl_Time->setText(QString("%1:%2:%3")
                              .arg(hours, 2, 10, QChar('0'))
                              .arg(minutes, 2, 10, QChar('0'))
                              .arg(seconds, 2, 10, QChar('0')));

    if (remainingSeconds <= 0) {
        mainTimer->stop();
        checkTimer->stop();
        accept(); // Punishment complete
    }
}

void Detention::showCheckButton()
{
    // Ensure we have the button dimensions
    ui->btn_Check->adjustSize();
    int btnW = ui->btn_Check->width();
    int btnH = ui->btn_Check->height();
    int padding = 20; // Space between button and text/screen edge

    // 1. Identify the "Forbidden Zone" (The text area)
    // The top safe zone ends at the top of the Time label
    int topZoneBottom = ui->lbl_Time->geometry().top() - padding;

    // The bottom safe zone starts at the bottom of the Instructions label
    int bottomZoneTop = ui->lbl_Instructions->geometry().bottom() + padding;

    // 2. Determine Valid Y Ranges
    QList<QPair<int, int>> validYRanges;

    // Check Top Zone (Is there enough space above the time?)
    if (topZoneBottom > btnH) {
        validYRanges.append(qMakePair(padding, topZoneBottom - btnH));
    }

    // Check Bottom Zone (Is there enough space below the instructions?)
    if (bottomZoneTop < (height() - btnH)) {
        validYRanges.append(qMakePair(bottomZoneTop, height() - btnH - padding));
    }

    // 3. Select a Position
    int y = 0;

    if (!validYRanges.isEmpty()) {
        // Randomly pick one of the valid zones (Top or Bottom)
        int zoneIndex = QRandomGenerator::global()->bounded(validYRanges.size());
        QPair<int, int> range = validYRanges[zoneIndex];

        // Pick a random Y within that zone
        if (range.second > range.first) {
            y = QRandomGenerator::global()->bounded(range.first, range.second);
        } else {
            y = range.first;
        }
    } else {
        // Fallback: If the screen is somehow too small, just pick anywhere (safe overlap)
        y = QRandomGenerator::global()->bounded(height() - btnH);
    }

    // Pick random X (Width is safe, just avoid edges)
    int x = QRandomGenerator::global()->bounded(padding, width() - btnW - padding);

    // 4. Move and Show
    ui->btn_Check->move(x, y);
    ui->btn_Check->show();
    ui->btn_Check->raise(); // Ensure it's on top of everything

    // Start Fail Timer (e.g., 30 seconds to click)
    failTimer->start(30000);
}

void Detention::onCheckButtonClicked()
{
    ui->btn_Check->hide();
    failTimer->stop();

    // Resume timer if we paused it
    if (!mainTimer->isActive() && remainingSeconds > 0) {
        mainTimer->start(1000);
    }

    // Schedule next check
    int nextCheck = QRandomGenerator::global()->bounded(30000, 120000); // 30s - 2m
    checkTimer->start(nextCheck);
}

void Detention::onCheckFailed()
{
    // User didn't click in time
    ui->btn_Check->hide();
    mainTimer->stop();

    reject();
}

void Detention::closeEvent(QCloseEvent *event)
{
    if (remainingSeconds > 0) {
        event->ignore(); // Prevent closing
    } else {
        event->accept();
    }
}

void Detention::keyPressEvent(QKeyEvent *event)
{
    if (event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier) && event->key() == Qt::Key_D) {
        qDebug() << "[Detention] Secret exit triggered.";
        reject();
        return;
    }

    // Block Escape Key to prevent accidental closing
    if (event->key() == Qt::Key_Escape) {
        event->ignore();
        return;
    }

    QDialog::keyPressEvent(event);
}
