#ifndef MSGBOX_H
#define MSGBOX_H

#include <QDialog>
#include <QThread>
#include <QTimer>
#include <QKeyEvent>

namespace Ui {
class msgBox;
}

class msgBox : public QDialog
{
    Q_OBJECT

public:
    explicit msgBox(QWidget *parent = nullptr);
    ~msgBox();
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
    Ui::msgBox *ui;

    QTimer* timerDelay;

    bool IsAccepted = false;

    int delayedSec = 0;
};

#endif // MSGBOX_H
