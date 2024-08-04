#ifndef WINDOW_OVERVIEW_H
#define WINDOW_OVERVIEW_H

#include "./dto/category.h"

#include <QDir>
#include <QList>
#include <QMainWindow>
#include <QMediaPlayer>
#include <QMenu>
#include <QSwipeGesture>
#include <QSystemTrayIcon>
#include <QtDebug>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    bool init();
    ~MainWindow() override;

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void closeEvent (QCloseEvent *event) override;

private slots:
    void on_pushButton_Start_clicked();
    void on_lineEdit_WindowStyle_textChanged(const QString &);
    void on_lineEdit_FontSize_textChanged(const QString &);
    void on_pushButton_Quit_clicked();
    void on_lineEdit_IntervalNum_textChanged(const QString &);
    void on_lineEdit_ClientName_textChanged();
    void on_pushButton_Start_pressed();
    void on_pushButton_Quit_pressed();
    void on_lineEdit_NewKUCoeff_textChanged(const QString &arg1);

    void on_pushButto_Manage_clicked();

private:
    void initUi(QSqlQuery &query);
    void initUi_Stats();
    void initUi_Overview(QSqlQuery &query);
    void initUi_Settings();
    void initUi_CrammingSchedule();
    void swipeTriggered(QSwipeGesture *gesture);

private:
    Ui::MainWindow *ui;
    std::vector<Category> allCats;
};

#endif // WINDOW_OVERVIEW_H
