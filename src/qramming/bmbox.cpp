#include "bmbox.h"
#include "ui_bmbox.h"

bmbox::bmbox(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::bmbox)
{
    ui->setupUi(this);
}

bmbox::~bmbox()
{
    delete ui;
}

bool bmbox::isVerified()
{
    return verified;
}

void bmbox::on_pushButton_Deactivate_clicked()
{
    verified = (ui->lineEdit_password->text() == "mimamima");
    this->close();
}
