#ifndef PRACTICEWINDOW_H
#define PRACTICEWINDOW_H

#include <QDateTime>
#include <QMainWindow>
#include <QtSql>
#include <QLabel>
#include <QtGui>
#include <QtWidgets>
#include <QSpacerItem>
#include <QProgressBar>
#include <QShortcut>
#include <QKeySequence>
#include <QSignalMapper>
#include <QMap>
#include <QDir>
#include "mainwindow.h"
//#include "search.h"
#include "downloader.h"
#include "msgbox.h"
#include "bmbox.h"

namespace Ui {
class PracticeWindow;
}

class PracticeWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit PracticeWindow(QWidget *parent = 0, QSqlDatabase mySQL = QSqlDatabase::addDatabase("QSQLITE"));
    ~PracticeWindow();
    void init(QList<CategoryMetaData*> *availableCategory, int NKI, int nnterval, int number, int windowStyle);
    void initNextKU();

private slots:
    void on_pushButton_Next_clicked();

    void on_pushButton_Check_clicked();

    void on_comboBox_Score_currentTextChanged(const QString &);

    void on_actionResetTimer_triggered();

    void on_actionBossMode_triggered();

    void on_actionStartLearning_triggered();

    void on_actionExit_triggered();

    void showContextMenu_Question(const QPoint &pt);

    void showContextMenu_Answer(const QPoint &pt);

    void showContextMenu_Blank(const QPoint &pt);

    void tmrInterval();

    void on_pushButton_Skip_clicked();

    void on_textEdit_Question_textChanged();

    void on_textEdit_Info_textChanged();

    void on_pushButton_Skip_pressed();

    void on_pushButton_Check_pressed();

    void on_pushButton_Next_pressed();

    void on_pushButton_Switch_pressed();

    void on_textEdit_Draft_textChanged();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void closeEvent (QCloseEvent *event) override;

private:
    Ui::PracticeWindow *ui;
    //0ID, 1Question, 2Answer, 3PassingScore, 4PreviousScore, 5TimesPracticed, 6InsertTime, 7FirstPracticeTime, 8LastPracticeTime, 9Deadline, 10ClientType, 11Maintype"
    int cku_ID;
    QString cku_Question;
    QString cku_Answer;
    double cku_PassingScore;
    double cku_PreviousScore = 0; //This needs to be set to zero or on Android platform there will be a flash of incorrect new score (such as .1617e+242)due to the uninitialized cku_PreviousScore.
    int cku_TimesPracticed;
    QDateTime cku_InsertTime;
    QDateTime cku_FirstPracticeTime;
    QDateTime cku_LastPracticeTime;
    QDateTime cku_Deadline;
    QString cku_ClientName;
    QString cku_Category;
    int cku_SecSpent;

    QSqlDatabase mySQL;
    QList<CategoryMetaData*> *availableCategory;
    QHash<QString, QString> *SearchOptions;

    QMenu *menuQuestion;
    QMenu *menuAnswer;
    QMenu *menuBlank;

    int64_t kuStartLearningTime;
    int NKI;
    int interval;
    int number;
    int windowStyle;
    QString parentDir;
    bool isAndroid;

    int concurrentEventOnQuestionTextChanged = 0;

    void initStatusBar();
    void initUI();
    void initPlatformSpecificSettings();
    void loadNewKU(int depth);
    bool finalizeLastKU();
    double calculateNewPreviousScore(double newScore);
    void adaptTexteditHeight(QTextEdit *plaintextedit);
    void adaptTexteditLineSpacing(QTextEdit *textedit);
    void adaptSkipButton();
    void finishLearning();
    void initTrayMenu();
    void initContextMenu();
    void showContextMenu_Common(QTextEdit *edit, QMenu* menu, const QPoint &pt);
    void setWindowStyle();
    QString convertStringToFilename(QString name);
    void handleTTS(bool isQuestion);
    int determineCategoryforNewKU();
    bool downloadTTSFile(QString text);
    bool handleDatabaseOperationError(QString operationName, QString dbPath, QString lastError);
    void startInterval();

    int currCatIndex;
    int totalKU;
    int currentScore;
    int outstandingKU;

    QMediaPlayer *player;
    int secDelayed;
    QTimer *timerDelay;

    QSystemTrayIcon *trayIcon;    

    QString clientName;

    bool bossMode = false;
};


#endif // PRACTICEWINDOW_H
