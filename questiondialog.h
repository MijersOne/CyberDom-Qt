#ifndef QUESTIONDIALOG_H
#define QUESTIONDIALOG_H

#include "qlistwidget.h"
#include "scriptparser.h"

#include <QDialog>

namespace Ui {
class QuestionDialog;
}

class QuestionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QuestionDialog(const QuestionSection &question, QWidget *parent = nullptr);
    QString getSelectedAnswer() const;
    ~QuestionDialog();

private:
    QuestionSection question;
    QString selectedAnswer;
    QListWidget *optionList;
    Ui::QuestionDialog *ui;
};

#endif // QUESTIONDIALOG_H
