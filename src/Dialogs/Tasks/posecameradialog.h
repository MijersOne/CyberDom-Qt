#ifndef POSECAMERADIALOG_H
#define POSECAMERADIALOG_H

#include <QDialog>
#include <QCamera>
#include <QMediaCaptureSession>
#include <QImageCapture>
#include <QMediaDevices>
#include <QTimer>
#include <QDir>

namespace Ui {
class PoseCameraDialog;
}

class PoseCameraDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PoseCameraDialog(QWidget *parent = nullptr, const QString &instructionText = "", const QString &saveFolder = "");
    ~PoseCameraDialog();

    QString getSavedFilePath() const { return savedFilePath; }

private slots:
    void updateCountdown();
    void captureImage();
    void onImageCaptured(int id, const QImage &preview);
    void onRetakeClicked();
    void onOkayClicked();

private:
    Ui::PoseCameraDialog *ui;
    QCamera *camera;
    QMediaCaptureSession *captureSession;
    QImageCapture *imageCapture;
    QTimer *countdownTimer;

    QString instructionText;
    QString saveFolder;
    QString savedFilePath;
    QImage currentImage;

    int countdownValue = 6;

    void startCountdown();
    void showReviewUI();
    void showPreviewUI();
};

#endif // POSECAMERADIALOG_H
