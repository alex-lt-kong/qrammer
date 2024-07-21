﻿#include "mainwindow.h"
#include "src/common/utils.h"
#include "src/qrammer-db-util/global_variables.h"
#include "src/qrammer-db-util/ui_mainwindow.h"

#include <QFileDialog>
#include <spdlog/spdlog.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    QDir tmpDir = QApplication::applicationFilePath();
    tmpDir.cdUp();
    tmpDir.cdUp();
    db = QSqlDatabase::addDatabase(DATABASE_DRIVER);
    db.setDatabaseName(databaseName);
    db.setDatabaseName(tmpDir.path() + "/db/database.sqlite");

    ui->setupUi(this);

    ui->lineEdit_Keyword->setFocus();
    ui->comboBox_Field->addItems({ "Question", "Answer", "ID" });

    ui->lineEdit_PassingScore->setValidator(new QIntValidator(0, 99, this));

    const int tabStop = 4;  // 4 characters
    QFontMetrics metrics(ui->plainTextEdit_Question->font());
    ui->plainTextEdit_Question->setTabStopDistance(tabStop * metrics.horizontalAdvance(' '));
    ui->plainTextEdit_Answer->setTabStopDistance(tabStop * metrics.horizontalAdvance(' '));

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
    if (!db.isOpen() && !db.open()) {
        QMessageBox::warning(this,
                             "Warning",
                             QString("Cannot open the database %1:\n%2")
                                 .arg(db.databaseName(), db.lastError().text()));
        QApplication::quit();
        return;
    }
    ui->comboBox_Maintype_Search->clear();

    QSqlQuery query = QSqlQuery(db);
    auto stmt = "SELECT DISTINCT(category) FROM knowledge_units ORDER BY category ASC";
    if (!query.prepare(stmt)) {
        SPDLOG_ERROR(query.lastError().text().toStdString());
        QMessageBox::critical(this, "Error", query.lastError().text());
        return;
    }
    if (!query.exec()) {
        SPDLOG_ERROR(query.lastError().text().toStdString());
        QMessageBox::critical(this, "Error", query.lastError().text());
        return;
    }

    QStringList t;
    while (query.next())
        t.append(query.value(0).toString());
    // A very weird workaround: if not receving all maintypes in t first and then add them to combox, only the first item would be added.
    ui->comboBox_Maintype_Search->addItems(t);
    ui->comboBox_Maintype_Meta->addItems(t);
    db.close();
}

void MainWindow::conductDatabaseSearch(QString field, QString keyword, QString category)
{
    SPDLOG_INFO("Searching [{}] in category [{}]", keyword.toStdString(), category.toStdString());
    ui->listWidget_SearchResults->clear();
    if (!db.isOpen() && !db.open()) {
        SPDLOG_ERROR(db.lastError().text().toStdString());
        QMessageBox::critical(this, "Error", db.lastError().text());
        return;
    }
    auto stmt = QString(R"***(
SELECT id, question
FROM knowledge_units
WHERE
    category = :category AND
    %1 LIKE :keyword
LIMIT 50)***")
                    .arg(field);
    auto query = QSqlQuery(db);
    if (!query.prepare(stmt)) {
        SPDLOG_ERROR(query.lastError().text().toStdString());
        QMessageBox::critical(this, "Error", query.lastError().text());
        return;
    }
    query.bindValue(":category", category);
    query.bindValue(":keyword", keyword);
    if (!query.exec()) {
        SPDLOG_ERROR(query.lastError().text().toStdString());
        QMessageBox::critical(this, "Error", query.lastError().text());
        return;
    }
    searchResults = new QMap<QString, int>;

    while (query.next()) {
        searchResults->insert(query.value(1).toString(), query.value(0).toInt());
        ui->listWidget_SearchResults->addItem(query.value(1).toString());
        /*
            if (QGuiApplication::platformName() == "windows") {
                QFont font;
                font.setFamily("Microsoft Yahei");
                ui->listWidget_SearchResults->setFont(font);
            }
            */
    }

    if (ui->listWidget_SearchResults->count() > 0)
        ui->listWidget_SearchResults->setCurrentRow(0);
    else
        showSingleKU(-1);
    SPDLOG_INFO("Found {} matches", ui->listWidget_SearchResults->count());
}

void MainWindow::showSingleKU(int kuID)
{
    currKUID = kuID;
    if (!db.isOpen() && !db.open()) {
        SPDLOG_ERROR(db.lastError().text().toStdString());
        QMessageBox::critical(this, "Error", db.lastError().text());
        return;
    }
    auto query = QSqlQuery(db);
    auto columns = QString(R"***(
id,
question,
answer,
passing_score,
previous_score,
times_practiced,
insert_time,
first_practice_time,
last_practice_time,
deadline,
category,
answer_image)***");
    auto stmt = QString("SELECT %1 FROM knowledge_units WHERE id = :id").arg(columns);
    if (!query.prepare(stmt)) {
        SPDLOG_ERROR(query.lastError().text().toStdString());
        QMessageBox::critical(this, "Error", query.lastError().text());
        return;
    }
    query.bindValue(":id", kuID);
    if (!query.exec()) {
        SPDLOG_ERROR(query.lastError().text().toStdString());
        QMessageBox::critical(this, "Error", query.lastError().text());
        return;
    }
    if (query.next()) {
        ui->lineEdit_KUID->setText(query.value(0).toString());
        ui->plainTextEdit_Question->setPlainText(query.value(1).toString());
        ui->plainTextEdit_Answer->setPlainText(query.value(2).toString());
        ui->lineEdit_PassingScore->setText(query.value(3).toString());
        ui->lineEdit_TimesPracticed->setText(query.value(5).toString());
        ui->comboBox_Maintype_Meta->setEditText(query.value(10).toString());
        ui->lineEdit_Deadline->setText(query.value(9).toDateTime().toString("yyyy-MM-dd HH:mm:ss"));

        ui->lineEdit_InsertDate->setText(query.value(6).toDateTime().toString("yyyy-MM-dd"));
        ui->lineEdit_1stPracticeDate->setText(query.value(7).toDateTime().toString("yyyy-MM-dd"));
        ui->lineEdit_LastPracticeDate->setText(query.value(8).toDateTime().toString("yyyy-MM-dd"));
        ui->lineEdit_PreviousScore->setText(QString::number(query.value(4).toInt()));

        auto imageBytes = query.value(11).toByteArray();
        if (imageBytes.size() > 0) {
            QPixmap answerImage;
            if (answerImage.loadFromData(imageBytes)) {
                ui->label_AnswerImage->setPixmap(answerImage);
            } else {
                ui->label_AnswerImage->setText("[ERROR]");
                SPDLOG_WARN("KnowledgeUnit ID={} appears to have image data, but it is invalid",
                            ui->lineEdit_KUID->text().toStdString());
            }
        } else {
            ui->label_AnswerImage->setText("[Empty]");
        }

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
        } else {
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
    // db.close();
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

    if (ui->lineEdit_Deadline->text().size() > 0
        && !(QDateTime::fromString(ui->lineEdit_Deadline->text(), "yyyy-MM-dd HH:mm:ss").isValid()
             || QDateTime::fromString(ui->lineEdit_Deadline->text(), "yyyy-MM-dd").isValid())) {
        QMessageBox::information(this,
                                 "Convert from QString to QDatetime failed",
                                 "Field [Deadline] must be null or a datetime string in the format "
                                 "of yyyy-MM-dd( HH:mm:ss)");
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

    setWindowTitle("Qrammer - DB Utility [" + ui->comboBox_Field->currentText() + "]");
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

    if (!db.isOpen() && !db.open()) {
        SPDLOG_ERROR(db.lastError().text().toStdString());
        QMessageBox::critical(this, "Error", db.lastError().text());
        return;
    }

    QSqlQuery query = QSqlQuery(db);
    if (currKUID == -1) {
        auto stmt = QString(R"***(
INSERT INTO knowledge_units (
    question,
    answer,
    times_practiced,
    previous_score,
    category,
    passing_score,
    deadline,
    insert_time,
    last_practice_time,
    client_name,
    answer_image
)
VALUES (
    :question,
    :answer,
    :times_practiced,
    :previous_score,
    :category,
    :passing_score,
    :deadline,
    DATETIME('Now', 'localtime'),
    :last_practice_time,
    :client_name,
    :answer_image
)
)***");
        if (!query.prepare(stmt)) {
            SPDLOG_ERROR(query.lastError().text().toStdString());
            QMessageBox::critical(this, "Error", query.lastError().text());
            return;
        }
        query.bindValue(":question", ui->plainTextEdit_Question->toPlainText());

        query.bindValue(":answer", ui->plainTextEdit_Answer->toPlainText());
        query.bindValue(":times_practiced", 0);
        query.bindValue(":previous_score", 0);
        query.bindValue(":category", ui->comboBox_Maintype_Meta->currentText());
        query.bindValue(":passing_score", ui->lineEdit_PassingScore->text());
        query.bindValue(":deadline", ui->lineEdit_Deadline->text());
        query.bindValue(":last_practice_time", "");
        query.bindValue(":client_name", "");
        if (!ui->label_AnswerImage->pixmap().isNull()) {
            QByteArray bArray;
            QBuffer buffer(&bArray);
            buffer.open(QIODevice::WriteOnly);
            ui->label_AnswerImage->pixmap().save(&buffer, "PNG");
            query.bindValue(":answer_image", bArray);
        } else {
            query.bindValue(":answer_image", QByteArray());
        }
    } else {
        auto stmt = R"***(
UPDATE knowledge_units
SET
    question = :question,
    answer = :answer,
    passing_score = :passing_score,
    category = :category,
    deadline = :deadline,
    answer_image = :answer_image
WHERE id = :id)***";
        if (!query.prepare(stmt)) {
            SPDLOG_ERROR(query.lastError().text().toStdString());
            QMessageBox::critical(this, "Error", query.lastError().text());
            return;
        }
        query.bindValue(":question", ui->plainTextEdit_Question->toPlainText());
        query.bindValue(":answer", ui->plainTextEdit_Answer->toPlainText());
        query.bindValue(":passing_score", ui->lineEdit_PassingScore->text());
        query.bindValue(":category", ui->comboBox_Maintype_Meta->currentText());
        query.bindValue(":deadline", ui->lineEdit_Deadline->text());
        query.bindValue(":id", currKUID);
        if (!ui->label_AnswerImage->pixmap().isNull()) {
            QByteArray bArray;
            QBuffer buffer(&bArray);
            buffer.open(QIODevice::WriteOnly);
            ui->label_AnswerImage->pixmap().save(&buffer, "PNG");
            query.bindValue(":answer_image", bArray);
        } else {
            query.bindValue(":answer_image", QByteArray());
        }
    }
    if (!query.exec()) {
        SPDLOG_ERROR(query.lastError().text().toStdString());
        QMessageBox::critical(this, "Error", query.lastError().text());
    }
    showSingleKU(-1);
    // db.close();
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
    if (QMessageBox::question(this,
                              "QJLT - KU Deletion",
                              "Sure to remove the KU [" + QString::number(currKUID) + "]?",
                              QMessageBox::Yes | QMessageBox::No)
        != QMessageBox::Yes)
        return;

    if (!db.isOpen() && !db.open()) {
        SPDLOG_ERROR(db.lastError().text().toStdString());
        QMessageBox::critical(this, "Error", db.lastError().text());
        return;
    }

    QSqlQuery query = QSqlQuery(db);
    if (currKUID > 0) {
        auto stmt = "DELETE FROM knowledge_units WHERE id = :id";
        if (!query.prepare(stmt)) {
            SPDLOG_ERROR(query.lastError().text().toStdString());
            QMessageBox::critical(this, "Error", query.lastError().text());
            return;
        }
        query.bindValue(":id", currKUID);
        if (!query.exec()) {
            SPDLOG_ERROR(query.lastError().text().toStdString());
            QMessageBox::critical(this, "Error", query.lastError().text());
            return;
        }
    }

    showSingleKU(-1);
    // db.close();

    on_lineEdit_Keyword_textChanged(nullptr);
}

void MainWindow::on_pushButton_ChooseImage_clicked()
{
    auto fileContentReady = [this](const QString &fileName, const QByteArray &fileContent) {
        if (fileName.isEmpty()) {
            SPDLOG_INFO("No file is selected");
        } else {
            SPDLOG_INFO("fileName: {}", fileName.toStdString());
            SPDLOG_INFO("fileContent.size(): {} bytes", fileContent.size());
            QPixmap answerImage;
            if (answerImage.loadFromData(fileContent)) {
                auto w = std::min(answerImage.width(), ANSWER_IMAGE_DIMENSION);
                auto h = std::min(answerImage.height(), ANSWER_IMAGE_DIMENSION);
                answerImage = answerImage.scaled(QSize(w, h),
                                                 Qt::KeepAspectRatio,
                                                 Qt::SmoothTransformation);
                ui->label_AnswerImage->setPixmap(answerImage);
            }
        }
    };
    QFileDialog::getOpenFileContent("Images (*.png *.xpm *.jpg)", fileContentReady);
    return;
    auto fileName = QFileDialog::getOpenFileName(this,
                                                 "Select an image...",
                                                 "",
                                                 "Image Files (*.png *.jpg *.bmp)");
    if (fileName.isEmpty()) {
        SPDLOG_INFO("No file is selected");
    } else {
        SPDLOG_INFO("fileName: {}", fileName.toStdString());
    }
}
