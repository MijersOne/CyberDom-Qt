#ifndef QUESTIONDIALOG_H
#define QUESTIONDIALOG_H

#include <QDialog>
#include <QList>
#include <QMap>
#include <QButtonGroup>
#include "ScriptData.h" // Make sure this includes QuestionDefinition

namespace Ui {
class QuestionDialog;
}

class QuestionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QuestionDialog(const QList<QuestionDefinition> &questions, QWidget *parent = nullptr);
    ~QuestionDialog();

    QMap<QString, QString> getAnswers() const;

private slots:
    void onNextClicked();
    void showNextQuestion();

private:
    Ui::QuestionDialog *ui;
    QList<QuestionDefinition> questions;
    int currentIndex;
    QMap<QString, QString> answers;
    QButtonGroup* buttonGroup = nullptr;
};

#endif // QUESTIONDIALOG_H
