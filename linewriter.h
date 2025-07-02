#ifndef LINEWRITER_H
#define LINEWRITER_H

#include <QDialog>
#include <QStringList>

namespace Ui {
class LineWriter;
}

class LineWriter : public QDialog {
    Q_OBJECT
public:
    explicit LineWriter(const QStringList &lines, int totalLines, QWidget *parent = nullptr);
    ~LineWriter();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void submitLine();

private:
    void updateCurrentPrompt();
    void updateTextBox();
    void checkPage();

    Ui::LineWriter *ui;
    QStringList expectedLines;
    QStringList currentPageLines;
    int currentIndex = 0;
    int currentPageStart = 0;
};

#endif // LINEWRITER_H
