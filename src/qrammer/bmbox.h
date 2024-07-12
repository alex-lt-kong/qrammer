#ifndef BMBOX_H
#define BMBOX_H

#include <QDialog>

namespace Ui {
class bmbox;
}

class bmbox : public QDialog
{
    Q_OBJECT

public:
    explicit bmbox(QWidget *parent = nullptr);
    ~bmbox();
    bool isVerified();

private slots:
    void on_pushButton_Deactivate_clicked();



private:
    Ui::bmbox *ui;
    bool verified = false;
};

#endif // BMBOX_H
