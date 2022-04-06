#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSql>
#include <QMessageBox>
#include <QMap>
#include <QDateTime>
#include <QDir>
#include <QFont>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void on_pushButton_Quit_clicked();

    void on_comboBox_Field_currentTextChanged(const QString &);

    void on_comboBox_Maintype_Search_currentTextChanged(const QString &);

    void on_listWidget_SearchResults_currentTextChanged(const QString &currentText);

    void on_pushButton_NewKU_clicked();

    void on_lineEdit_Keyword_textChanged(const QString &);

    void on_pushButton_WriteDB_clicked();

    void on_lineEdit_Keyword_Suffix_textChanged(const QString &);

    void on_lineEdit_Keyword_Prefix_textChanged(const QString &);

    void on_listWidget_SearchResults_doubleClicked(const QModelIndex &);

    void on_pushButton_Delete_clicked();

private:
    void initCategory();
    void conductDatabaseSearch(QString field, QString keyword, QString maintype);
    void showSingleKU(int kuID);
    bool inputAvailabilityCheck();

    Ui::MainWindow *ui;

    QSqlDatabase mySQL = QSqlDatabase::addDatabase("QSQLITE");

    QMap<QString, int> *searchResults;

    int currKUID;
};

#endif // MAINWINDOW_H
