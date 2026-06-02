#include "pointcameradialog.h"
#include "ui_pointcameradialog.h"

PointCameraDialog::PointCameraDialog(QWidget *parent, const QString &instructionText)
    : QDialog(parent)
    , ui(new Ui::PointCameraDialog)
{
    ui->setupUi(this);
    ui->lbl_Instructions->setText(instructionText);

    // --- Camera Setup ---
    const QCameraDevice &defaultCamera = QMediaDevices::defaultVideoInput();
    if (defaultCamera.isNull()) {
        ui->lbl_Instructions->setText("Error: No camera found.\n" + instructionText);
    } else {
        camera = new QCamera(defaultCamera, this);
        captureSession = new QMediaCaptureSession(this);

        captureSession->setCamera(camera);
        captureSession->setVideoOutput(ui->videoWidget);

        camera->start();
    }
}

PointCameraDialog::~PointCameraDialog()
{
    if (camera && camera->isActive()) {
        camera->stop();
    }
    delete ui;
}
