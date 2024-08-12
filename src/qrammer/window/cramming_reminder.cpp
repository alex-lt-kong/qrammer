#include "cramming_reminder.h"
#include "src/qrammer/window/ui_cramming_reminder.h"

using namespace Qrammer::Window;

CrammingReminder::CrammingReminder(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::CrammingReminder)
{
    ui->setupUi(this);

    ui->pushButton_No->setDefault(true);

    timerDelay = new QTimer(this);
    connect(timerDelay, SIGNAL(timeout()), this, SLOT(tmrDelay()));
    timerDelay->start(1000);
}

CrammingReminder::~CrammingReminder()
{
    delete ui;
}

bool CrammingReminder::isAccepted()
{
    return IsAccepted;
}

void CrammingReminder::on_pushButton_Yes_clicked()
{
    IsAccepted = true;
    this->close();
}

void CrammingReminder::on_pushButton_No_clicked()
{
    IsAccepted = false;
    this->close();
}

void CrammingReminder::tmrDelay()
{
    delayedSec++;

    if (delayedSec < 3) {
        ui->pushButton_Yes->setText("Yes [" + QString::number(3 - delayedSec - 1)  + "]");
        return;
    }

    timerDelay->stop();

    ui->pushButton_No->setEnabled(true);
    ui->pushButton_Yes->setEnabled(true);
}

void CrammingReminder::keyPressEvent(QKeyEvent *event)
{
    if(event->key() != Qt::Key_Escape)
        QWidget::keyPressEvent(event);
}

void CrammingReminder::on_pushButton_Yes_pressed()
{
    on_pushButton_Yes_clicked();
}

void CrammingReminder::on_pushButton_No_pressed()
{
    CrammingReminder::on_pushButton_No_clicked();
}
