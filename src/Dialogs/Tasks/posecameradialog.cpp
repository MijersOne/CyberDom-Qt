#include "posecameradialog.h"
#include "ui_posecameradialog.h"

#include <QDateTime>
#include <QStandardPaths>
#include <QDebug>
#include <QMessageBox>

PoseCameraDialog::PoseCameraDialog(QWidget *parent, const QString &instructionText, const QString &saveFolder)
    : QDialog(parent)
    , ui(new Ui::PoseCameraDialog)
    , instructionText(instructionText)
    , saveFolder(saveFolder)
{
    ui->setupUi(this);
    ui->lbl_Instructions->setText(instructionText);

    // Setup Camera
    const QCameraDevice &defaultCamera = QMediaDevices::defaultVideoInput();
    if (defaultCamera.isNull()) {
        ui->lbl_Instructions->setText("Error: No camera found.\n" + instructionText);
    } else {
        camera = new QCamera(defaultCamera, this);
        captureSession = new QMediaCaptureSession(this);
        imageCapture = new QImageCapture(this);

        captureSession->setCamera(camera);
        captureSession->setImageCapture(imageCapture);
        captureSession->setVideoOutput(ui->videoWidget);

        connect(imageCapture, &QImageCapture::imageCaptured, this, &PoseCameraDialog::onImageCaptured);
        connect(ui->btn_Retake, &QPushButton::clicked, this, &PoseCameraDialog::onRetakeClicked);
        connect(ui->btn_Okay, &QPushButton::clicked, this, &PoseCameraDialog::onOkayClicked);

        // Setup Timer
        countdownTimer = new QTimer(this);
        connect(countdownTimer, &QTimer::timeout, this, &PoseCameraDialog::updateCountdown);

        // Start
        showPreviewUI();
    }
}

PoseCameraDialog::~PoseCameraDialog()
{
    if (camera && camera->isActive()) camera->stop();
    delete ui;
}

void PoseCameraDialog::showPreviewUI()
{
    ui->stackedWidget->setCurrentWidget(ui->page_preview);
    ui->btn_Retake->setVisible(false);
    ui->btn_Okay->setVisible(false);
    ui->lbl_Countdown->setVisible(true);

    if (camera) camera->start();
    startCountdown();
}

void PoseCameraDialog::showReviewUI()
{
    ui->stackedWidget->setCurrentWidget(ui->page_review);
    ui->btn_Retake->setVisible(true);
    ui->btn_Okay->setVisible(true);
    if (camera) camera->stop();
}

void PoseCameraDialog::startCountdown()
{
    countdownValue = 6;
    ui->lbl_Countdown->setText(QString::number(countdownValue));
    countdownTimer->start(1000);
}

void PoseCameraDialog::updateCountdown()
{
    countdownValue--;
    if (countdownValue > 0) {
        ui->lbl_Countdown->setText(QString::number(countdownValue));
    } else {
        ui->lbl_Countdown->setText("SNAP!");
        countdownTimer->stop();
        captureImage();
    }
}

void PoseCameraDialog::captureImage()
{
    if (imageCapture && imageCapture->isReadyForCapture()) {
        imageCapture->capture();
    }
}

void PoseCameraDialog::onImageCaptured(int id, const QImage &preview)
{
    Q_UNUSED(id);
    currentImage = preview;

    // Scale image to fit label
    QPixmap pixmap = QPixmap::fromImage(preview);
    ui->lbl_ImagePreview->setPixmap(pixmap.scaled(ui->lbl_ImagePreview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));

    showReviewUI();
}

void PoseCameraDialog::onRetakeClicked()
{
    showPreviewUI();
}

void PoseCameraDialog::onOkayClicked()
{
    // Save the image
    if (saveFolder.isEmpty()) {
        saveFolder = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + "/CyberDom";
    }

    QDir dir(saveFolder);
    if (!dir.exists()) dir.mkpath(".");

    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    savedFilePath = QString("%1/Pose_%2.jpg").arg(saveFolder, timestamp);

    if (currentImage.save(savedFilePath)) {
        qDebug() << "[PoseCamera] Image saved to:" << savedFilePath;
        accept();
    } else {
        QMessageBox::warning(this, "Error", "Failed to save image.");
        // Don't close, let them try again or retake
    }
}
