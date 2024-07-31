#ifndef WINDOW_OVERVIEW_H
#define WINDOW_OVERVIEW_H

#include <QMainWindow>
#include <QtDebug>
#include <QList>
#include <QMediaPlayer>
#include <QSystemTrayIcon>
#include <QDir>
#include <QMenu>
#include <QSwipeGesture>
#include "snapshot.h"

namespace Ui {
class MainWindow;
}

class CategoryMetaData {
public:
    QString name;
    int number;
    int ttsOption;
    Snapshot* snapshot;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

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

private:
    bool initCategoryStructure();
    bool initUI();
    void initSettings();
    void initStatistics();
    void swipeTriggered(QSwipeGesture *gesture);

private:
    Ui::MainWindow *ui;
    QList<CategoryMetaData*> *allCats;
};

#endif // WINDOW_OVERVIEW_H
