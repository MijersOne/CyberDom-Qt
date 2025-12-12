#include "joblaunch.h"
#include "ui_joblaunch.h"
#include "cyberdom.h"
#include "scriptparser.h"
#include <QMessageBox>
#include <QPushButton>
#include <QDebug>

extern CyberDom *mainApp;

JobLaunch::JobLaunch(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::JobLaunch)
{
    ui->setupUi(this);
    populateJobDropdown();

    // Connect the ButtonBox accepted signal (OK button) to our launch logic
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &JobLaunch::launchSelectedJob);

    // Reject is automatically handled by QDialog for Cancel
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
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

    QStringList jobList = mainApp->getAvailableJobs();

    if (jobList.isEmpty()) {
        qDebug() << "[WARNING] No jobs found.";
    }

    ui->jobComboBox->clear();
    ui->jobComboBox->addItems(jobList);
}

void JobLaunch::launchSelectedJob()
{
    QString selectedJob = ui->jobComboBox->currentText();

    if (selectedJob.isEmpty()) {
        QMessageBox::warning(this, "No Job Selected", "Please select a job to launch.");
        return;
    }

    if (!mainApp || !mainApp->getScriptParser()) {
        qDebug() << "[ERROR] Main app or parser is null!"; // FIX: Lowercase 'q'
        return;
    }

    const ScriptData &data = mainApp->getScriptParser()->getScriptData();
    QString lowerName = selectedJob.toLower();

    // Validate job existence using the parser
    if (!data.jobs.contains(lowerName)) {
        QMessageBox::warning(this, "Error", "Job definition not found in script.");
        return;
    }

    const JobDefinition &job = data.jobs.value(lowerName);

    qDebug() << "[DEBUG] Job Launched: " << selectedJob;

    // --- Display Job Information ---
    // Construct a clean info string from the struct
    QString jobInfo = QString("Starting Job: %1\n").arg(job.title.isEmpty() ? job.name : job.title);

    if (!job.text.isEmpty()) jobInfo += QString("Text: %1\n").arg(job.text);

    // Duration
    if (!job.duration.minTimeStart.isEmpty()) {
        jobInfo += QString("Min Time: %1").arg(job.duration.minTimeStart);
        if (!job.duration.minTimeEnd.isEmpty() && job.duration.minTimeEnd != job.duration.minTimeStart) {
            jobInfo += QString(" - %1").arg(job.duration.minTimeEnd);
        }
        jobInfo += "\n";
    }

    // Merits (FIX: Compare strings, not ints)
    if (!job.merits.add.isEmpty() && job.merits.add != "0") {
        jobInfo += QString("Merits Reward: %1\n").arg(job.merits.add);
    }
    if (!job.merits.subtract.isEmpty() && job.merits.subtract != "0") {
        jobInfo += QString("Merits Cost: %1\n").arg(job.merits.subtract);
    }

    // Status Change
    if (!job.newStatus.isEmpty()) {
        jobInfo += QString("Status Change: %1\n").arg(job.newStatus);
    }

    QMessageBox::information(this, "Job Details", jobInfo);

    // --- Launch Logic ---
    mainApp->addJobToAssignments(selectedJob);

    emit mainApp->jobListUpdated();

    // Trigger status change if defined
    if (!job.newStatus.isEmpty()) {
        emit jobStatusChanged(job.newStatus);
    }

    accept();
}
