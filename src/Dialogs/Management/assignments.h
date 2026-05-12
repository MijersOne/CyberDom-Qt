#ifndef ASSIGNMENTS_H
#define ASSIGNMENTS_H

#include <QMainWindow>
#include <QTableWidgetItem>
#include <QDateTime>
#include <QComboBox>
#include <QDateEdit>
#include <QLabel>

class CyberDom;

namespace Ui {
class Assignments;
}

// --- Helper Class for Correct Date Sorting ---
class DateTableWidgetItem : public QTableWidgetItem {
public:
    DateTableWidgetItem(const QString& display, const QDateTime& val)
        : QTableWidgetItem(display), dt(val) {}

    bool operator<(const QTableWidgetItem& other) const override {
        const DateTableWidgetItem* otherItem = dynamic_cast<const DateTableWidgetItem*>(&other);
        if (otherItem) {
            // Handle "No Deadline" (invalid date).
            // We usually want "No Deadline" to appear at the bottom (treated as infinity).
            if (!dt.isValid() && !otherItem->dt.isValid()) return false;
            if (!dt.isValid()) return false; // I am infinity -> I am greater
            if (!otherItem->dt.isValid()) return true; // They are infinity -> I am smaller
            return dt < otherItem->dt;
        }
        return text() < other.text();
    }
private:
    QDateTime dt;
};
// ---------------------------------------------

class Assignments : public QMainWindow
{
    Q_OBJECT

public:
    explicit Assignments(QWidget *parent = nullptr, CyberDom *app = nullptr);
    ~Assignments();

public slots:
    void populateJobList();

protected:
    // Reset filters when the window is shown
    void showEvent(QShowEvent *event) override;

private slots:
    void on_btn_Start_clicked();
    void on_btn_Done_clicked();
    void on_btn_Abort_clicked();
    void on_btn_Delete_clicked();
    void updateStartButtonState();

    void onFilterModeChanged(int index);
    void applyFilter();
    void resetFilter();

private:
    Ui::Assignments *ui;
    CyberDom *mainApp;
    QString settingsFile;

    // --- Filter UI Elements ---
    QComboBox *filterCombo;
    QDateEdit *dateEditA;
    QDateEdit *dateEditB;
    QLabel *labelTo;
    QWidget *filterContainer; // To toggle visibility if needed

    void setupFilterUi(); // Helper to build the UI programmatically
};

#endif // ASSIGNMENTS_H
