#ifndef POINTCAMERADIALOG_H
#define POINTCAMERADIALOG_H

#include <QDialog>
#include <QCamera>
#include <QMediaCaptureSession>
#include <QVideoWidget>
#include <QMediaDevices>

namespace Ui {
class PointCameraDialog;
}

class PointCameraDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PointCameraDialog(QWidget *parent = nullptr, const QString &instructionText = "");
    ~PointCameraDialog();

private:
    Ui::PointCameraDialog *ui;
    QCamera *camera;
    QMediaCaptureSession *captureSession;
};

#endif // POINTCAMERADIALOG_H
