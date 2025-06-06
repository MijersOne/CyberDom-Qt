#include "questiondialog.h"
#include "ui_questiondialog.h"

#include <QPushButton>
#include <QMessageBox>
#include <QRadioButton>

QuestionDialog::QuestionDialog(const QList<QuestionDefinition>& questions, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QuestionDialog)
    , questions(questions)
    , currentIndex(0)
{
    ui->setupUi(this);

    connect(ui->nextButton, &QPushButton::clicked, this, &QuestionDialog::onNextClicked);
    showNextQuestion();
}

QuestionDialog::~QuestionDialog()
{
    delete ui;
}

void QuestionDialog::showNextQuestion()
{
    if (currentIndex >= questions.size()) {
        accept();  // Closes the dialog
        return;
    }

    const QuestionDefinition& question = questions[currentIndex];

    // Set the window title
    this->setWindowTitle(question.name);

    // Set the question text
    ui->questionLabel->setText(question.phrase);

    // Clear existing radio buttons
    QLayoutItem *item;
    while ((item = ui->answersLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
    buttonGroup = new QButtonGroup(this);

    // Add each answer as a radio button
    for (int i = 0; i < question.answers.size(); ++i) {
        const QuestionAnswerBlock& answer = question.answers[i];
        QRadioButton* radio = new QRadioButton(answer.answerText);
        ui->answersLayout->addWidget(radio);
        buttonGroup->addButton(radio, i);
    }
}

void QuestionDialog::onNextClicked()
{
    if (!buttonGroup)
        return;

    QAbstractButton* selected = buttonGroup->checkedButton();
    if (!selected) {
        QMessageBox::warning(this, "No Answer Selected", "Please select an answer before continuing.");
        return;
    }

    int id = buttonGroup->id(selected);
    const QuestionDefinition& question = questions[currentIndex];
    const QuestionAnswerBlock& selectedAnswer = question.answers[id];

    // Store the selected value into the result map
    answers.insert(question.variable, selectedAnswer.variableValue);

    ++currentIndex;
    showNextQuestion();
}

QMap<QString, QString> QuestionDialog::getAnswers() const
{
    return answers;
}
