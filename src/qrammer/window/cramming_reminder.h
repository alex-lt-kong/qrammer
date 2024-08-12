#ifndef CRAMMING_REMINDER_H
#define CRAMMING_REMINDER_H

#include <QDialog>
#include <QThread>
#include <QTimer>
#include <QKeyEvent>

namespace Qrammer::Window {
namespace Ui {
class CrammingReminder;
}

class CrammingReminder : public QDialog
{
    Q_OBJECT

public:
    explicit CrammingReminder(QWidget *parent = nullptr);
    ~CrammingReminder();
    bool isAccepted();

protected:
    void keyPressEvent(QKeyEvent* event);

private slots:
    void on_pushButton_Yes_clicked();

    void on_pushButton_No_clicked();

    void tmrDelay();

    void on_pushButton_Yes_pressed();

    void on_pushButton_No_pressed();

private:
    Ui::CrammingReminder *ui;

    QTimer* timerDelay;

    bool IsAccepted = false;

    int delayedSec = 0;
};
} // namespace Qrammer::Window

#endif // CRAMMING_REMINDER_H
