#include "linewriter.h"
#include "ui_linewriter.h"
#include <QMessageBox>
#include <QKeyEvent>

LineWriter::LineWriter(const QStringList &lines, int totalLines, QWidget *parent)
    : QDialog(parent), ui(new Ui::LineWriter) {
    ui->setupUi(this);
    ui->text_Submitted->setReadOnly(true);
    ui->line_Input->setContextMenuPolicy(Qt::NoContextMenu);
    ui->line_Input->installEventFilter(this);

    for (int i = 0; i < totalLines; ++i)
        expectedLines.append(lines.at(i % lines.size()));

    updateCurrentPrompt();

    connect(ui->btn_Submit, &QPushButton::clicked, this, &LineWriter::submitLine);
}

LineWriter::~LineWriter() {
    delete ui;
}

bool LineWriter::eventFilter(QObject *obj, QEvent *event) {
    if (obj == ui->line_Input && event->type() == QEvent::KeyPress) {
        QKeyEvent *key = static_cast<QKeyEvent*>(event);
        if (key->matches(QKeySequence::Paste) || key->matches(QKeySequence::Copy) ||
            key->matches(QKeySequence::Cut) || key->key() == Qt::Key_Backspace ||
            key->key() == Qt::Key_Delete) {
            return true;
        }
    }
    return QDialog::eventFilter(obj, event);
}

void LineWriter::updateCurrentPrompt() {
    if (currentIndex < expectedLines.size())
        ui->label_Line->setText(expectedLines[currentIndex]);
    else
        ui->label_Line->setText(QString());
}

void LineWriter::updateTextBox() {
    ui->text_Submitted->setPlainText(currentPageLines.join("\n"));
}

void LineWriter::submitLine() {
    QString typed = ui->line_Input->text();
    ui->line_Input->clear();
    currentPageLines.append(typed);
    ++currentIndex;
    updateTextBox();

    if (currentPageLines.size() >= 5 || currentIndex == expectedLines.size()) {
        checkPage();
    } else {
        updateCurrentPrompt();
    }
}

void LineWriter::checkPage() {
    bool correct = true;
    for (int i = 0; i < currentPageLines.size(); ++i) {
        if (currentPageLines[i] != expectedLines[currentPageStart + i]) {
            correct = false;
            break;
        }
    }

    if (correct) {
        currentPageLines.clear();
        currentPageStart = currentIndex;
        ui->text_Submitted->clear();
        if (currentIndex == expectedLines.size())
            accept();
        else
            updateCurrentPrompt();
    } else {
        QMessageBox::warning(this, tr("Incorrect"), tr("Lines did not match. Try again."));
        currentIndex = currentPageStart;
        currentPageLines.clear();
        ui->text_Submitted->clear();
        updateCurrentPrompt();
    }
}

