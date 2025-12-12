#ifndef ADDCLOTHTYPE_H
#define ADDCLOTHTYPE_H

#include <QDialog>

namespace Ui {
class AddClothType;
}

class AddClothType : public QDialog
{
    Q_OBJECT

public:
    explicit AddClothType(QWidget *parent = nullptr);
    ~AddClothType();

private:
    Ui::AddClothType *ui;

signals:
    void clothTypeAdded(const QString &clothType); // Signal to notify about a new cloth type

private slots:
    void handleAccepted();
};

#endif // ADDCLOTHTYPE_H
