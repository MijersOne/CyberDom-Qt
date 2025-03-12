#include "assignments.h"
#include "ui_assignments.h"
#include "cyberdom.h"

#include <QMessageBox>

// extern CyberDom *mainApp;

Assignments::Assignments(QWidget *parent, CyberDom *app)
    : QMainWindow(parent)
    , ui(new Ui::Assignments)
    , mainApp(app)
{
    ui->setupUi(this);
    populateJobList(); // Call the function to populate the list when the window is opened.

    if (!mainApp) {
        qDebug() << "[ERROR] mainApp is NULL in Assignments!";
    } else {
        connect(mainApp, &CyberDom::jobListUpdated, this, &Assignments::populateJobList);
        qDebug() << "[DEBUG] Connected Assignments to CyberDom's jobListUpdated Signal!";
    }
}

Assignments::~Assignments()
{
    delete ui;
}

void Assignments::populateJobList() {
    ui->table_Assignments->clearContents(); // Clear existing jobs
    ui->table_Assignments->setRowCount(0);

    // Configure columns (do this once, perhaps in constructor
    if (ui->table_Assignments->columnCount() == 0) {
        ui->table_Assignments->setColumnCount(3);
        ui->table_Assignments->setHorizontalHeaderLabels({"Deadline", "Assignment", "Type"});
        ui->table_Assignments->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        ui->table_Assignments->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        ui->table_Assignments->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui->table_Assignments->setEditTriggers(QAbstractItemView::NoEditTriggers); // Make the table non-editable
        ui->table_Assignments->setAlternatingRowColors(true); // Alternate row background colors
        ui->table_Assignments->setSelectionMode(QAbstractItemView::SingleSelection); // Only one row can be selected
        ui->table_Assignments->setFocusPolicy(Qt::StrongFocus); // Better keyboard navigation
    }

    QSet<QString> jobs = mainApp->getActiveJobs();
    QMap<QString, QDateTime> deadlines = mainApp->getJobDeadlines();

    int row = 0;
    for (const QString &assignmentName : jobs) {
        if (mainApp->iniData.contains("punishment-" + assignmentName)) {
            continue;
        }

        QString deadlineStr;
        if (deadlines.contains(assignmentName)) {
            deadlineStr = deadlines[assignmentName].toString("MM-dd-yyyy h:mm AP");
        } else {
            deadlineStr = "No Deadline";
        }

        ui->table_Assignments->insertRow(row);
        ui->table_Assignments->setItem(row, 0, new QTableWidgetItem(deadlineStr));
        ui->table_Assignments->setItem(row, 1, new QTableWidgetItem(assignmentName));
        ui->table_Assignments->setItem(row, 2, new QTableWidgetItem("Job"));

        // Style jobs differently from punishments
        ui->table_Assignments->item(row, 2)->setBackground(QColor(230, 255, 230)); // Light green for jobs
        ui->table_Assignments->item(row, 2)->setForeground(QColor(0, 100, 0));

        row++;

    if (!mainApp) {
        qDebug() << "[Error] Main app reference is null!";
        return;
    }

        QString displayText = deadlineStr + " - " + assignmentName;

        qDebug() << "[DEBUG] Adding Job to List: " << displayText;
        //ui->list_Assignments->addItem(displayText);
    }

    // Add Punishments
    for (const QString &assignmentName : jobs) {
        // Check if this is a punishment (not a job)
        if (mainApp->iniData.contains("punishment-" + assignmentName)) {
            QString deadlineStr;
            if (deadlines.contains(assignmentName)) {
                deadlineStr = deadlines[assignmentName].toString("MM-dd-yyyy h:mm AP");
            } else {
                deadlineStr = "No Deadline";
            }

            ui->table_Assignments->insertRow(row);
            ui->table_Assignments->setItem(row, 0, new QTableWidgetItem(deadlineStr));
            ui->table_Assignments->setItem(row, 1, new QTableWidgetItem(assignmentName));
            ui->table_Assignments->setItem(row, 2, new QTableWidgetItem("Punishment"));

            ui->table_Assignments->item(row, 2)->setBackground(QColor(255, 230, 230));
            ui->table_Assignments->item(row, 2)->setForeground(QColor(180, 0, 0));

            row++;
            qDebug() << "[DEBUG] Added Punishment to List: " << assignmentName << " with deadline: " << deadlineStr;
        }
    }

    qDebug() << "[DEBUG] Displayed Active Jobs and Punishments in UI.";
}

void Assignments::on_btn_Start_clicked()
{
    int selectedRow = ui->table_Assignments->currentRow();
    if (selectedRow >= 0) {
        QString jobName = ui->table_Assignments->item(selectedRow, 1)->text();

        // Safely get the type - handle case where column might not exist
        QString type = "Assignment"; // Default Value
        QTableWidgetItem* typeItem = ui->table_Assignments->item(selectedRow, 2);
        if (typeItem) {
            type = typeItem->text();
        }

        // Process the selected job
        QMessageBox::information(this, "Starting " + type, "Starting " + type.toLower() + ": " + jobName);
        // Add job launching code here
    } else {
        QMessageBox::warning(this, "No Selection", "Please select an assignment to start.");
    }
}
