#ifndef WINDOW_CRAMMING_H
#define WINDOW_CRAMMING_H

#include "./dto/category.h"
#include "./dto/knowledge_unit.h"

#include <QDateTime>
#include <QDir>
#include <QKeySequence>
#include <QLabel>
#include <QMainWindow>
#include <QMap>
#include <QProgressBar>
#include <QRandomGenerator>
#include <QShortcut>
#include <QSignalMapper>
#include <QSpacerItem>
#include <QtGui>
#include <QtSql>
#include <QtWidgets>

namespace Ui {
class CrammingWindow;
}

class CrammingWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit CrammingWindow(QWidget *parent = nullptr);
    ~CrammingWindow();
    void init(std::vector<Category> availableCategory,
              uint32_t newKuCoeff,
              int nnterval,
              int number,
              int windowStyle);
    void initNextKU();

private slots:
    void on_pushButton_Next_clicked();
    void on_pushButton_Check_clicked();
    void on_comboBox_Score_currentTextChanged(const QString &);
    void actionResetTimer_triggered_cb();
    void actionBossMode_triggered_cb();
    void actionStartLearning_triggered_cb();
    void actionExit_triggered_cb();
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
    void on_textEdit_Response_textChanged();
    void on_pushButton_Delete_clicked();
    void on_pushButton_ChooseQuestionImage_clicked();
    void on_pushButton_ChooseAnswerImage_clicked();

    void on_pushButton_ManageDB_clicked();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void closeEvent (QCloseEvent *event) override;

private:
    Ui::CrammingWindow *ui;
    // cku: current knowledge unit
    struct KnowledgeUnit cku;

    std::vector<Category> availableCategory;
    QHash<QString, QString> SearchOptions;

    QMenu *menuQuestion;
    QMenu *menuAnswer;
    QMenu *menuBlank;

    int64_t kuStartLearningTime;
    uint32_t newKuCoeff;
    int interval;
    int number;
    int windowStyle;
    QString parentDir;

    int concurrentEventOnQuestionTextChanged = 0;

    QRandomGenerator ranGen;

    void initStatusBar();
    void initUI();
    void initPlatformSpecificSettings();
    void loadNextKu(int depth);
    /**
     * @brief
     * @return whether the program should start loading next KU
    */
    bool finalizeKuJustBeingCrammed();
    double calculateNewPreviousScore(double newScore);
    void adaptTexteditHeight(QTextEdit *plaintextedit);
    void adaptTexteditLineSpacing(QTextEdit *textedit);
    void finalizeCrammingSession();
    void initTrayMenu();
    void initContextMenu();
    void showContextMenu_Common(QTextEdit *edit, QMenu* menu, const QPoint &pt);
    void setWindowStyle();
    QString convertStringToFilename(QString name);
    int pickCategoryforNewKU();
    bool downloadTTSFile(QString text);
    bool promptUserToRetryDBError(QString operationName, QString lastError);
    void startInterval();
    void onAnswerShownCallback();
    void fillinThenExecuteCommand(QString callbackName);
    void onKuLoadCallback();
    void preKuLoadGuiUpdate();
    void postKuLoadGuiUpdate();
    void saveWindowLayout();
    void restoreWindowLayout();
    void updateCkuByGuiElements();

    int currCatIndex;
    size_t totalKuToCram;
    size_t remainingKUsToCram;

    int secDelayed;
    QTimer *timerDelay;

    QSystemTrayIcon *trayIcon;    

    QString clientName;

    bool bossMode = false;
};


#endif // WINDOW_CRAMMING_H
