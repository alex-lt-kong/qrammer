#ifndef WINDOW_MANAGE_DB_H
#define WINDOW_MANAGE_DB_H

#include "./src/qrammer/dto/knowledge_unit.h"

#include <QMainWindow>
#include <QtSql>
#include <QMessageBox>
#include <QMap>
#include <QDateTime>
#include <QDir>
#include <QFont>

namespace Qrammer::Window {
namespace Ui {
class ManageDB;
}

class ManageDB : public QMainWindow
{
    Q_OBJECT

public:
    explicit ManageDB(QWidget *parent = nullptr);
    ~ManageDB();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
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
    void on_pushButton_ChooseImage_clicked();
    void on_pushButton_ChooseQuestionImage_clicked();

private:
    void initCategory();
    void conductDatabaseSearch(QString field, QString keyword, QString maintype);
    void showSingleKU(int kuID);
    bool inputValidityCheck();

    Ui::ManageDB *ui;
    QLabel *statusText;

    QMap<QString, int> searchResults;

    struct Dto::KnowledgeUnit cku;
};
} // namespace Qrammer::Window
#endif // WINDOW_MANAGE_DB_H
