#include "manage_db.h"
#include "src/qrammer/global_variables.h"
#include "src/qrammer/utils.h"
#include "src/qrammer/window/ui_manage_db.h"

#include <QFileDialog>
#include <spdlog/spdlog.h>

using namespace Qrammer::Window;

ManageDB::ManageDB(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ManageDB)
{
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

ManageDB::~ManageDB()
{
    delete ui;
}

void ManageDB::initCategory()
{
    ui->comboBox_Maintype_Search->clear();
    QStringList t;
    try {
        auto cats = db.getAllCategories();
        for (const auto &cat : cats) {
            t.append(cat.name);
        }
    } catch (const std::runtime_error &e) {
        QMessageBox::warning(this, "Qrammer", e.what());
        this->close();
        return;
    }
    ui->comboBox_Maintype_Search->addItems(t);
    ui->comboBox_Maintype_Meta->addItems(t);
}

void ManageDB::conductDatabaseSearch(QString field, QString keyword, QString category)
{
    SPDLOG_INFO("Searching [{}] in category [{}]", keyword.toStdString(), category.toStdString());
    ui->listWidget_SearchResults->clear();

    auto stmt = QString(R"***(
SELECT id, question
FROM knowledge_units
WHERE
    category = :category AND
    %1 LIKE :keyword
LIMIT 50)***")
                    .arg(field);
    QSqlQuery query;
    try {
        query = db.execSelectQuery(stmt,
                                   std::vector<std::pair<QString, QVariant>>{{":category", category},
                                                                             {":keyword", keyword}});
    } catch (const std::runtime_error &e) {
        auto errMsg
            = QString("Error conductDatabaseSearch()ing, reason: %2").arg(currKUID).arg(e.what());
        SPDLOG_INFO(errMsg.toStdString());
        QMessageBox::warning(this, "Qrammer", errMsg);
        return;
    }
    searchResults = QMap<QString, int>();

    while (query.next()) {
        searchResults.insert(query.value(1).toString(), query.value(0).toInt());
        ui->listWidget_SearchResults->addItem(query.value(1).toString());
    }

    if (ui->listWidget_SearchResults->count() > 0)
        ui->listWidget_SearchResults->setCurrentRow(0);
    else
        showSingleKU(-1);
    SPDLOG_INFO("Found {} matches", ui->listWidget_SearchResults->count());
}

void ManageDB::showSingleKU(int kuID)
{
    currKUID = kuID;
    try {
        cku = db.selectKuById(currKUID);
    } catch (const std::runtime_error &e) {
        auto errMsg = QString("Error loading knowledge unit (ID: %1), reason: %2")
                          .arg(currKUID)
                          .arg(e.what());
        SPDLOG_INFO(errMsg.toStdString());
        QMessageBox::warning(this, "Qrammer", errMsg);
        return;
    } catch (const std::invalid_argument &e) {
        (void) e;
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
        ui->label_QuestionImage->setText("[Empty]");
        ui->label_AnswerImage->setText("[Empty]");

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
        return;
    }

    ui->lineEdit_KUID->setText(QString::number(cku.ID));
    ui->plainTextEdit_Question->setPlainText(cku.Question);
    ui->plainTextEdit_Answer->setPlainText(cku.Answer);
    ui->lineEdit_PassingScore->setText(QString::number(cku.PassingScore));
    ui->lineEdit_TimesPracticed->setText(QString::number(cku.TimesPracticed));
    ui->comboBox_Maintype_Meta->setEditText(cku.Category);
    ui->lineEdit_Deadline->setText(cku.Deadline.toString("yyyy-MM-dd HH:mm:ss"));

    ui->lineEdit_InsertDate->setText(cku.InsertTime.toString("yyyy-MM-dd"));
    ui->lineEdit_1stPracticeDate->setText(cku.FirstPracticeTime.toString("yyyy-MM-dd"));
    ui->lineEdit_LastPracticeDate->setText(cku.LastPracticeTime.toString("yyyy-MM-dd"));
    ui->lineEdit_PreviousScore->setText(QString::number(cku.PreviousScore));

    if (cku.QuestionImageBytes.size() > 0) {
        QPixmap questionImage;
        if (questionImage.loadFromData(cku.QuestionImageBytes)) {
            ui->label_QuestionImage->setPixmap(questionImage);
        } else {
            ui->label_QuestionImage->setText("[ERROR]");
            SPDLOG_WARN("KnowledgeUnit ID={} appears to have image data, but it is invalid",
                        ui->lineEdit_KUID->text().toStdString());
        }
    } else {
        ui->label_QuestionImage->setText("[Empty]");
    }

    if (cku.AnswerImageBytes.size() > 0) {
        QPixmap answerImage;
        if (answerImage.loadFromData(cku.AnswerImageBytes)) {
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
}

bool ManageDB::inputValidityCheck()
{
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

void ManageDB::on_comboBox_Field_currentTextChanged(const QString &)
{
    conductDatabaseSearch(ui->comboBox_Field->currentText(),
                          ui->lineEdit_Keyword_Prefix->text() + ui->lineEdit_Keyword->text() + ui->lineEdit_Keyword_Suffix->text(), ui->comboBox_Maintype_Search->currentText());

    setWindowTitle("Qrammer - DB Utility [" + ui->comboBox_Field->currentText() + "]");
}

void ManageDB::on_comboBox_Maintype_Search_currentTextChanged(const QString &)
{
    ui->comboBox_Maintype_Meta->setCurrentText(ui->comboBox_Maintype_Search->currentText());
    conductDatabaseSearch(ui->comboBox_Field->currentText(),
                          ui->lineEdit_Keyword_Prefix->text() + ui->lineEdit_Keyword->text() + ui->lineEdit_Keyword_Suffix->text(), ui->comboBox_Maintype_Search->currentText());
}

void ManageDB::on_listWidget_SearchResults_currentTextChanged(const QString &currentText)
{
    showSingleKU(searchResults.value(currentText, -1));
}

void ManageDB::on_pushButton_NewKU_clicked()
{
    showSingleKU(-1);
}

void ManageDB::on_lineEdit_Keyword_textChanged(const QString &)
{
    conductDatabaseSearch(ui->comboBox_Field->currentText(),
                          ui->lineEdit_Keyword_Prefix->text()
                          + ui->lineEdit_Keyword->text()
                          + ui->lineEdit_Keyword_Suffix->text(), ui->comboBox_Maintype_Search->currentText());
}

void ManageDB::on_pushButton_WriteDB_clicked()
{
    if (!inputValidityCheck())
        return;

    if (!db.conn.isOpen() && !db.conn.open()) {
        SPDLOG_ERROR(db.conn.lastError().text().toStdString());
        QMessageBox::critical(this, "Error", db.conn.lastError().text());
        return;
    }

    QSqlQuery query = QSqlQuery(db.conn);
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
    question_image,
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
    :question_image,
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
        if (!ui->label_QuestionImage->pixmap().isNull()) {
            QByteArray bArray;
            QBuffer buffer(&bArray);
            buffer.open(QIODevice::WriteOnly);
            ui->label_QuestionImage->pixmap().save(&buffer, "PNG");
            query.bindValue(":question_image", bArray);
        } else {
            query.bindValue(":answer_image", QByteArray());
        }
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
    question_image = :question_image,
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
        if (!ui->label_QuestionImage->pixmap().isNull()) {
            QByteArray bArray;
            QBuffer buffer(&bArray);
            buffer.open(QIODevice::WriteOnly);
            ui->label_QuestionImage->pixmap().save(&buffer, "PNG");
            query.bindValue(":question_image", bArray);
        } else {
            query.bindValue(":question_image", QByteArray());
        }
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

void ManageDB::on_lineEdit_Keyword_Suffix_textChanged(const QString &)
{
    conductDatabaseSearch(ui->comboBox_Field->currentText(),
                          ui->lineEdit_Keyword_Prefix->text() + ui->lineEdit_Keyword->text() + ui->lineEdit_Keyword_Suffix->text(), ui->comboBox_Maintype_Search->currentText());
}

void ManageDB::on_lineEdit_Keyword_Prefix_textChanged(const QString &)
{
    conductDatabaseSearch(ui->comboBox_Field->currentText(),
                          ui->lineEdit_Keyword_Prefix->text() + ui->lineEdit_Keyword->text()  + ui->lineEdit_Keyword_Suffix->text(), ui->comboBox_Maintype_Search->currentText());
}

void ManageDB::keyPressEvent(QKeyEvent *event)
{
    if (event->modifiers()&Qt::ControlModifier && (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return))
        on_pushButton_WriteDB_clicked();
}

void ManageDB::on_listWidget_SearchResults_doubleClicked(const QModelIndex &)
{
    showSingleKU(searchResults.value(ui->listWidget_SearchResults->currentItem()->text(), -1));
}

void ManageDB::on_pushButton_Delete_clicked()
{
    if (QMessageBox::question(this,
                              "Qrammer",
                              QString("Sure to remove the KU [%1]?").arg(currKUID),
                              QMessageBox::Yes | QMessageBox::No)
        != QMessageBox::Yes)
        return;

    try {
        db.deleteKu(currKUID);
    } catch (const std::runtime_error &e) {
        auto errMsg = QString("Error deleteing knowledge unit (ID: %1), reason: %2")
                          .arg(currKUID)
                          .arg(e.what());
        SPDLOG_INFO(errMsg.toStdString());
        QMessageBox::warning(this, "Qrammer", errMsg);
        return;
    }

    showSingleKU(-1);
    on_lineEdit_Keyword_textChanged(nullptr);
}

void ManageDB::on_pushButton_ChooseImage_clicked()
{
    ui->label_AnswerImage->setPixmap(selectImageFromFileSystem());
}

void ManageDB::on_pushButton_ChooseQuestionImage_clicked()
{
    qDebug("WindowManageDB::on_pushButton_ChooseQuestionImage_clicked()");
    ui->label_QuestionImage->setPixmap(selectImageFromFileSystem());
}
