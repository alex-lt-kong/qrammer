#include "msgbox.h"
#include "ui_msgbox.h"

msgBox::msgBox(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::msgBox)
{
    ui->setupUi(this);

    ui->pushButton_No->setDefault(true);

    timerDelay = new QTimer(this);
    connect(timerDelay, SIGNAL(timeout()), this, SLOT(tmrDelay()));
    timerDelay->start(1000);


  //  if (QGuiApplication::platformName() == "windows") { // It appears that if a program is first compiled on a non-Windows environment, Windows's font needs to be manually set.
  //      QFont font = this->font();              // This is still an ugly hack for Windows platform.
  //      font.setFamily("Microsoft Yahei");
  //      this->setFont(font);
  //  }
}

msgBox::~msgBox()
{
    delete ui;
}

bool msgBox::isAccepted()
{
    return IsAccepted;
}

void msgBox::on_pushButton_Yes_clicked()
{
    IsAccepted = true;
    this->close();
}

void msgBox::on_pushButton_No_clicked()
{
    IsAccepted = false;
    this->close();
}

void msgBox::tmrDelay()
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

void msgBox::keyPressEvent(QKeyEvent* event)
{
    if(event->key() != Qt::Key_Escape)
        QWidget::keyPressEvent(event);
}

void msgBox::on_pushButton_Yes_pressed()
{
    on_pushButton_Yes_clicked();
}

void msgBox::on_pushButton_No_pressed()
{
    msgBox::on_pushButton_No_clicked();
}
