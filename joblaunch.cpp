#include "joblaunch.h"
#include "ui_joblaunch.h"
#include "cyberdom.h"
#include <QMessageBox>

extern CyberDom *mainApp; // Reference main application

JobLaunch::JobLaunch(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::JobLaunch)
{
    ui->setupUi(this);
    populateJobDropdown(); // Load jobs into dropdown
}

JobLaunch::~JobLaunch()
{
    delete ui;
}

void JobLaunch::populateJobDropdown()
{
    if (!mainApp) {
        qDebug() << "[ERROR] Main app reference is null!";
        return;
    }

    QStringList jobList = mainApp->getAvailableJobs(); // Get jobs from CyberDom

    if (jobList.isEmpty()) {
        qDebug() << "[WARNING] No jobs found in the script. Check if the script was loaded correctly.";
    } else {
        qDebug() << "[INFO] Found " << jobList.size() << " jobs to populate in dropdown.";
    }

    ui->jobComboBox->clear(); // Clear the existing items
    ui->jobComboBox->addItems(jobList); // Populate dropdown

    qDebug() << "[DEBUG] Job dropdown populated with available jobs.";
}

void JobLaunch::on_btnLaunchJob_clicked()
{
    QString selectedJob = ui->jobComboBox->currentText();

    if (selectedJob.isEmpty()) {
        QMessageBox::warning(this, "No Job Selected", "Please select a job to launch.");
        return;
    }

    qDebug() << "[DEBUG] Job Launched: " << selectedJob;

    if (!mainApp) {
        qDebug() << "[ERROR] Main app reference is null!";
        return;
    }

    mainApp->addJobToAssignments(selectedJob);

    emit mainApp->jobListUpdated();

    qDebug() << "[DEBUG] Emitted jobListUpdated Signal!";

    // Use getIniData() to access iniData safely
    QMap<QString, QMap<QString, QString>> iniData = mainApp->getIniData();

    if (!iniData.contains("job-" + selectedJob)){
        QMessageBox::warning(this, "Error", "Job details not found in script.");
        return;
    }

    QMap<QString, QString> jobDetails = iniData["job-" + selectedJob];

    // Check if job exists in the INI
    if (!mainApp->iniData.contains("job-" + selectedJob)) {
        QMessageBox::warning(this, "Error", "Job details not found in script.");
        return;
    }

    // Display Job Information
    QString jobInfo = "Starting Job: " + selectedJob + "\n";
    for (const auto &key : jobDetails.keys()) {
        jobInfo += key + ": " + jobDetails[key] + "\n";
    }
    QMessageBox::information(this, "Job Details", jobInfo);

    // Change Status if the job defines it
    if (jobDetails.contains("NewStatus")) {
        emit jobStatusChanged(jobDetails["NewStatus"]);
    }

    accept(); // Close the dialog after launching the job
}
