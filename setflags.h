#ifndef SETFLAGS_H
#define SETFLAGS_H

#include <QDialog>

namespace Ui {
class SetFlags;
}

class SetFlags : public QDialog
{
    Q_OBJECT

public:
    explicit SetFlags(QWidget *parent = nullptr);
    ~SetFlags();

signals:
    void flagSetRequested(const QString &flagName, int durationMinutes);

private slots:
    void setSelectedFlags();

private:
    Ui::SetFlags *ui;
};

#endif // SETFLAGS_H
