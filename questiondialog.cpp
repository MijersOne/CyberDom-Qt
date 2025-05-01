#include "questiondialog.h"
#include "ui_questiondialog.h"

QuestionDialog::QuestionDialog(const QuestionSection &q, QWidget *parent)
    : QDialog(parent), ui(new Ui::QuestionDialog), question(q)
{
    ui->setupUi(this);

    // Set the question text
    ui->labelQuestionText->setText(q.phrase);

    // Populate the options
    for (const QString &option : question.options.keys()) {
        ui->listOptions->addItem(option);
    }

    // Connect OK/Cancel buttons
    connect(ui->btnOk, &QPushButton::clicked, this, [=]() {
        QListWidgetItem *item = ui->listOptions->currentItem();
        if (item)
            selectedAnswer = item->text();
        accept();
    });

    connect(ui->btnCancel, &QPushButton::clicked, this, &QDialog::reject);
}

QString QuestionDialog::getSelectedAnswer() const {
    return selectedAnswer;
}

QuestionDialog::~QuestionDialog() {
    delete ui;
}
