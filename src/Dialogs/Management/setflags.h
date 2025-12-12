#ifndef SETFLAGS_H
#define SETFLAGS_H

#include <QDialog>

namespace Ui {
class SetFlags;
}

class ScriptParser; // Forward declaration

class SetFlags : public QDialog
{
    Q_OBJECT

public:
    // Updated constructor to accept ScriptParser
    explicit SetFlags(QWidget *parent = nullptr, ScriptParser *parser = nullptr);
    ~SetFlags();

signals:
    void flagSetRequested(const QString &flagName, int durationMinutes);

private slots:
    void setSelectedFlags();

private:
    Ui::SetFlags *ui;
    ScriptParser *parser;
};

#endif // SETFLAGS_H
