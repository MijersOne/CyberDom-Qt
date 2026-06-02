#include "deleteassignments.h"
#include "ui_deleteassignments.h"
#include "cyberdom.h"
#include <QMessageBox>

DeleteAssignments::DeleteAssignments(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DeleteAssignments)
{
    ui->setupUi(this);

    mainApp = qobject_cast<CyberDom*>(parent);

    populateAssignmentsList();

    connect(ui->btn_Delete, &QPushButton::clicked, this, &DeleteAssignments::deleteSelectedAssignments);
    connect(ui->btn_SelectAll, &QPushButton::clicked, this, &DeleteAssignments::selectAllAssignments);
    connect(ui->btn_SelectNone, &QPushButton::clicked, this, &DeleteAssignments::deselectAllAssignments);
    connect(ui->btn_Cancel, &QPushButton::clicked, this, &DeleteAssignments::reject);
}

DeleteAssignments::~DeleteAssignments()
{
    delete ui;
}

void DeleteAssignments::populateAssignmentsList()
{
    ui->lw_Assignments->clear();

    if (!mainApp)
        return;

    const QSet<QString> jobs = mainApp->getActiveJobs();
    const QMap<QString, QDateTime> deadlines = mainApp->getJobDeadlines();
    const auto &iniData = mainApp->getIniData();

    for (const QString &assignmentName : jobs) {
        bool isPunishment = iniData.contains("punishment-" + assignmentName);

        QString displayName = assignmentName;
        if (isPunishment) {
            int amt = mainApp->getPunishmentAmount(assignmentName);
            QMap<QString, QString> details = iniData.value("punishment-" + assignmentName);
            if (displayName.contains('#'))
                displayName.replace('#', QString::number(amt));
            if (!displayName.isEmpty())
                displayName[0] = displayName[0].toUpper();
        }

        QString deadlineStr = deadlines.contains(assignmentName)
                               ? deadlines.value(assignmentName).toString("MM-dd-yyyy h:mm AP")
                               : QStringLiteral("No Deadline");

        QString typeStr = isPunishment ? QStringLiteral("Punishment") : QStringLiteral("Job");
        QString label = QString("%1 - %2 (%3)").arg(deadlineStr, displayName, typeStr);

        QListWidgetItem *item = new QListWidgetItem(label, ui->lw_Assignments);
        item->setData(Qt::UserRole, assignmentName);
        item->setData(Qt::UserRole + 1, isPunishment);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
    }
}

void DeleteAssignments::deleteSelectedAssignments()
{
    if (!mainApp)
        return;

    QList<QPair<QString, bool>> toDelete;

    for (int i = 0; i < ui->lw_Assignments->count(); ++i) {
        QListWidgetItem *item = ui->lw_Assignments->item(i);
        if (item->checkState() == Qt::Checked) {
            QString name = item->data(Qt::UserRole).toString();
            bool isPunishment = item->data(Qt::UserRole + 1).toBool();
            toDelete.append(qMakePair(name, isPunishment));
        }
    }

    if (toDelete.isEmpty()) {
        QMessageBox::information(this, tr("No Selection"), tr("Please select at least one assignment to delete."));
        return;
    }

    if (toDelete.count() > 1) {
        int response = QMessageBox::question(this, tr("Confirm Multiple Deletion"),
                                             tr("Are you sure you want to delete %1 assignments?").arg(toDelete.count()),
                                             QMessageBox::Yes | QMessageBox::No);
        if (response != QMessageBox::Yes)
            return;
    }

    for (const auto &pair : toDelete) {
        mainApp->deleteAssignment(pair.first, pair.second);
    }

    populateAssignmentsList();
}

void DeleteAssignments::selectAllAssignments()
{
    for (int i = 0; i < ui->lw_Assignments->count(); ++i) {
        ui->lw_Assignments->item(i)->setCheckState(Qt::Checked);
    }
}

void DeleteAssignments::deselectAllAssignments()
{
    for (int i = 0; i < ui->lw_Assignments->count(); ++i) {
        ui->lw_Assignments->item(i)->setCheckState(Qt::Unchecked);
    }
}
