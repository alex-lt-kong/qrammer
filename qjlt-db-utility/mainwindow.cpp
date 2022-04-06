#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QDir tmpDir = QApplication::applicationFilePath();
    tmpDir.cdUp();
    tmpDir.cdUp();
    mySQL.setDatabaseName(tmpDir.path() + "/db/database.sqlite");

    ui->lineEdit_Keyword->setFocus();
    ui->comboBox_Field->addItems({ "Question", "Answer", "ID" });

    ui->lineEdit_PassingScore->setValidator(new QIntValidator(0, 99, this));

    const int tabStop = 4;  // 4 characters
    QFontMetrics metrics(ui->plainTextEdit_Question->font());
    ui->plainTextEdit_Question->setTabStopWidth(tabStop * metrics.width(' '));
    ui->plainTextEdit_Answer->setTabStopWidth(tabStop * metrics.width(' '));

    initCategory();

    conductDatabaseSearch(ui->comboBox_Field->currentText(),
                          ui->lineEdit_Keyword_Prefix->text()
                          + ui->lineEdit_Keyword->text()
                          + ui->lineEdit_Keyword_Suffix->text(),
                          ui->comboBox_Maintype_Search->currentText());
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initCategory()
{
    if (mySQL.open()) {
        ui->comboBox_Maintype_Search->clear();

        QSqlQuery *query = new QSqlQuery(mySQL);
        query->prepare("SELECT DISTINCT(category) FROM knowledge_units ORDER BY category ASC");
        query->exec();

        QStringList t;
        while (query->next())
            t.append(query->value(0).toString());
        // A very weird workaround: if not receving all maintypes in t first and then add them to combox, only the first item would be added.
        ui->comboBox_Maintype_Search->addItems(t);
        ui->comboBox_Maintype_Meta->addItems(t);
        mySQL.close();
    } else {
        QMessageBox::warning(this, "Warning", "Cannot open the database:\n" + mySQL.lastError().text());
        QApplication::quit();
    }
}

void MainWindow::conductDatabaseSearch(QString field, QString keyword, QString category)
{
    ui->listWidget_SearchResults->clear();
    if (mySQL.open()) {
        QSqlQuery *query = new QSqlQuery(mySQL);

        query->prepare("SELECT id, question FROM knowledge_units WHERE category = :category AND " + field + " LIKE :keyword LIMIT 50");
        query->bindValue(":category", category);
        query->bindValue(":keyword", keyword);
        query->exec();
        searchResults = new QMap<QString, int>;

        while (query->next()) {
            searchResults->insert(query->value(1).toString(), query->value(0).toInt());
            ui->listWidget_SearchResults->addItem(query->value(1).toString());
            if (QGuiApplication::platformName() == "windows") {
                QFont font;
                font.setFamily("Microsoft Yahei");
                ui->listWidget_SearchResults->setFont(font);
            }
        }

        if (ui->listWidget_SearchResults->count() > 0)
            ui->listWidget_SearchResults->setCurrentRow(0);
        else
            showSingleKU(-1);

        qDebug() << "COUNT(conductDatabaseSearch): " << searchResults->count();
    } else {
        QMessageBox::warning(this, "Warning", "Cannot open the database:\n" + mySQL.lastError().text());
        QApplication::quit();
    }
}

void MainWindow::showSingleKU(int kuID)
{
    currKUID = kuID;
    if (mySQL.open())
    {
        QSqlQuery *query = new QSqlQuery(mySQL);
        QString columns = "id, question, answer, passing_score, previous_score, times_practiced, insert_time, first_practice_time, last_practice_time, deadline, category";
        query->prepare("SELECT " + columns + " FROM knowledge_units WHERE id = :id");
        query->bindValue(":id", kuID);
        query->exec();
        if (query->next())
        {
            ui->lineEdit_KUID->setText(query->value(0).toString());
            ui->plainTextEdit_Question->setPlainText(query->value(1).toString());
            ui->plainTextEdit_Answer->setPlainText(query->value(2).toString());
            ui->lineEdit_PassingScore->setText(query->value(3).toString());
            ui->lineEdit_TimesPracticed->setText(query->value(5).toString());
            ui->comboBox_Maintype_Meta->setEditText(query->value(10).toString());
            ui->lineEdit_Deadline->setText(query->value(9).toDateTime().toString("yyyy-MM-dd HH:mm:ss"));

            ui->lineEdit_InsertDate->setText(query->value(6).toDateTime().toString("yyyy-MM-dd"));
            ui->lineEdit_1stPracticeDate->setText(query->value(7).toDateTime().toString("yyyy-MM-dd"));
            ui->lineEdit_LastPracticeDate->setText(query->value(8).toDateTime().toString("yyyy-MM-dd"));
            ui->lineEdit_PreviousScore->setText(QString::number(query->value(4).toInt()));

            ui->pushButton_WriteDB->setText("Update");
            ui->pushButton_Delete->setEnabled(true);
        } else {
            ui->lineEdit_KUID->setText("");

            if (ui->comboBox_Field->currentText() == "Question") {
                ui->plainTextEdit_Question->setPlainText(ui->lineEdit_Keyword->text());
                ui->plainTextEdit_Answer->setPlainText("");
            } else if (ui->comboBox_Field->currentText() == "Answer") {
                ui->plainTextEdit_Question->setPlainText("");
                ui->plainTextEdit_Answer->setPlainText(ui->lineEdit_Keyword->text());
            }
            else {
                ui->plainTextEdit_Question->setPlainText("");
                ui->plainTextEdit_Answer->setPlainText("");
            }

            ui->lineEdit_TimesPracticed->setText("");

            ui->lineEdit_Deadline->setText("");

            ui->lineEdit_InsertDate->setText("");
            ui->lineEdit_1stPracticeDate->setText("");
            ui->lineEdit_LastPracticeDate->setText("");
            ui->lineEdit_PreviousScore->setText("");

            ui->pushButton_WriteDB->setText("Insert");
            ui->pushButton_Delete->setEnabled(false);

            ui->listWidget_SearchResults->clearSelection();

            currKUID = -1;
        }
        mySQL.close();
    }
    else
    {
        QMessageBox::warning(this, "Warning", "Cannot open the database:\n" + mySQL.lastError().text());
        QApplication::quit();
    }

}

bool MainWindow::inputAvailabilityCheck()
{
    if (ui->plainTextEdit_Question->toPlainText().size() <= 0) {
        QMessageBox::information(this, "Information missing", "Field [Question] must be filled");
        return false;
    }
    if (ui->plainTextEdit_Answer->toPlainText().size() <= 0) {
        QMessageBox::information(this, "Information missing", "Field [Answer] must be filled");
        return false;
    }
    if (ui->comboBox_Maintype_Meta->currentText().size() <= 0) {
        QMessageBox::information(this, "Information missing", "Field [Category] must be filled");
        return false;
    }
    if (ui->lineEdit_PassingScore->text().size() <= 0) {
        QMessageBox::information(this, "Information missing", "Field [PassingScore] must be filled");
        return false;
    }

    bool ok;
    ui->lineEdit_PassingScore->text().toInt(&ok);
    if (!ok) {
        QMessageBox::information(this, "Convert from QString to int failed", "Field [PassingScore] should be a number");
        return false;
    }

    if (ui->lineEdit_Deadline->text().size() > 0 &&
            !(QDateTime::fromString(ui->lineEdit_Deadline->text(), "yyyy-MM-dd HH:mm:ss").isValid() || QDateTime::fromString(ui->lineEdit_Deadline->text(), "yyyy-MM-dd").isValid())) {
        QMessageBox::information(this, "Convert from QString to QDatetime failed", "Field [Deadline] must be null or a datetime string in the format of yyyy-MM-dd( HH:mm:ss)");
        return false;
    }
    return true;
}

void MainWindow::on_pushButton_Quit_clicked()
{
    QApplication::exit();
}

void MainWindow::on_comboBox_Field_currentTextChanged(const QString &)
{
    conductDatabaseSearch(ui->comboBox_Field->currentText(),
                          ui->lineEdit_Keyword_Prefix->text() + ui->lineEdit_Keyword->text() + ui->lineEdit_Keyword_Suffix->text(), ui->comboBox_Maintype_Search->currentText());

    setWindowTitle("Mamsds QJLT - DB Utility [" + ui->comboBox_Field->currentText()  + "]");
}

void MainWindow::on_comboBox_Maintype_Search_currentTextChanged(const QString &)
{
    ui->comboBox_Maintype_Meta->setCurrentText(ui->comboBox_Maintype_Search->currentText());
    conductDatabaseSearch(ui->comboBox_Field->currentText(),
                          ui->lineEdit_Keyword_Prefix->text() + ui->lineEdit_Keyword->text() + ui->lineEdit_Keyword_Suffix->text(), ui->comboBox_Maintype_Search->currentText());
}

void MainWindow::on_listWidget_SearchResults_currentTextChanged(const QString &currentText)
{
    showSingleKU(searchResults->value(currentText, -1));
}

void MainWindow::on_pushButton_NewKU_clicked()
{
    showSingleKU(-1);
}

void MainWindow::on_lineEdit_Keyword_textChanged(const QString &)
{

    conductDatabaseSearch(ui->comboBox_Field->currentText(),
                          ui->lineEdit_Keyword_Prefix->text()
                          + ui->lineEdit_Keyword->text()
                          + ui->lineEdit_Keyword_Suffix->text(), ui->comboBox_Maintype_Search->currentText());
}

void MainWindow::on_pushButton_WriteDB_clicked()
{
    if (!inputAvailabilityCheck())
        return;

    if (mySQL.open()) {
        QSqlQuery *query = new QSqlQuery(mySQL);
        if (currKUID == -1) {
            query->prepare(QString("INSERT INTO knowledge_units (question, answer, times_practiced, previous_score, category, passing_score, deadline, insert_time, last_practice_time, client_name) ")
                    + QString("VALUES (:question, :answer, :times_practiced, :previous_score, :category, :passing_score, :deadline, DATETIME('Now', 'localtime'), :last_practice_time, :client_name)"));
                    query->bindValue(":question", ui->plainTextEdit_Question->toPlainText());

            query->bindValue(":answer", ui->plainTextEdit_Answer->toPlainText());
            query->bindValue(":times_practiced", 0);
            query->bindValue(":previous_score", 0);
            query->bindValue(":category", ui->comboBox_Maintype_Meta->currentText());
            query->bindValue(":passing_score", ui->lineEdit_PassingScore->text());
            query->bindValue(":deadline", (ui->lineEdit_Deadline->text().size() > 0 ? (ui->lineEdit_Deadline->text()) : QVariant(QVariant::String)));
            query->bindValue(":last_practice_time", QVariant(QVariant::String));
            query->bindValue(":client_name", "");
        } else {
            query->prepare("UPDATE knowledge_units SET question = :question, answer = :answer, passing_score = :passing_score, category = :category, deadline = :deadline WHERE id = :id");

            query->bindValue(":question", ui->plainTextEdit_Question->toPlainText());
            query->bindValue(":answer", ui->plainTextEdit_Answer->toPlainText());
            query->bindValue(":passing_score", ui->lineEdit_PassingScore->text());
            query->bindValue(":category", ui->comboBox_Maintype_Meta->currentText());
            query->bindValue(":deadline", (ui->lineEdit_Deadline->text().size() > 0 ? (ui->lineEdit_Deadline->text()) : QVariant(QVariant::String)));
            query->bindValue(":id", currKUID);
        }
        query->exec();
        showSingleKU(-1);

        mySQL.close();
    } else {
        QMessageBox::warning(this, "Warning", "Cannot open the database:\n" + mySQL.lastError().text());
        QApplication::quit();
    }
}

void MainWindow::on_lineEdit_Keyword_Suffix_textChanged(const QString &)
{
    conductDatabaseSearch(ui->comboBox_Field->currentText(),
                          ui->lineEdit_Keyword_Prefix->text() + ui->lineEdit_Keyword->text() + ui->lineEdit_Keyword_Suffix->text(), ui->comboBox_Maintype_Search->currentText());
}

void MainWindow::on_lineEdit_Keyword_Prefix_textChanged(const QString &)
{
    conductDatabaseSearch(ui->comboBox_Field->currentText(),
                          ui->lineEdit_Keyword_Prefix->text() + ui->lineEdit_Keyword->text()  + ui->lineEdit_Keyword_Suffix->text(), ui->comboBox_Maintype_Search->currentText());
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->modifiers()&Qt::ControlModifier && (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return))
        on_pushButton_WriteDB_clicked();
}

void MainWindow::on_listWidget_SearchResults_doubleClicked(const QModelIndex &)
{
    showSingleKU(searchResults->value(ui->listWidget_SearchResults->currentItem()->text(), -1));
}

void MainWindow::on_pushButton_Delete_clicked()
{
    if (QMessageBox::question(this, "QJLT - KU Deletion", "Sure to remove the KU [" + QString::number(currKUID) + "]?", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        if (mySQL.open()) {

            QSqlQuery *query = new QSqlQuery(mySQL);
            if (currKUID > 0) {
                query->prepare("DELETE FROM knowledge_units WHERE id = :id");
                query->bindValue(":id", currKUID);
                query->exec();
            }

            showSingleKU(-1);
            mySQL.close();

            on_lineEdit_Keyword_textChanged(nullptr);
        } else {
            QMessageBox::warning(this, "Warning", "Cannot open the database:\n" + mySQL.lastError().text());
        }
    }
}

