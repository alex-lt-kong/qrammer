#include "src/qrammer/window_cramming.h"
#include "bmbox.h"
#include "msgbox.h"
#include "src/common/utils.h"
#include "src/qrammer/global_variables.h"
#include "ui_window_cramming.h"

#include <QAudioOutput>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <spdlog/spdlog.h>

CrammingWindow::CrammingWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::CrammingWindow)
{
    ui->setupUi(this);

    initTrayMenu();
    initPlatformSpecificSettings();

    player = new QMediaPlayer(this);
    // player->setVolume(50);
    auto audioOutput = new QAudioOutput;
    player->setAudioOutput(audioOutput);
    audioOutput->setVolume(50);

    ranGen = QRandomGenerator(static_cast<uint>(QTime::currentTime().msec()));

    timerDelay = new QTimer(this);
    connect(timerDelay, SIGNAL(timeout()), this, SLOT(tmrInterval()));

    initUI();
    initContextMenu();

    clientName = settings.value("ClientName", "Qrammer-Unspecified").toString();

    cku.PreviousScore = 0;
}

void CrammingWindow::saveWindowLayout()
{
    /*
    wp.geometry = this->saveGeometry();
    wp.state = this->saveState();
    wp.pos = this->pos();*/
}

void CrammingWindow::restoreWindowLayout()
{
    /*
    this->restoreGeometry(wp.geometry);
    this->restoreState(wp.state);
    this->move(wp.pos);
*/
}

CrammingWindow::~CrammingWindow()
{
    delete ui;
}

void CrammingWindow::closeEvent(QCloseEvent *event)
{
    if (remainingKUsToCram == 0) {
        actionExit_triggered_cb();
        event->accept();
        return;
    }
    auto resBtn = QMessageBox::question(this,
                                        "Qrammer",
                                        "Are you sure to quit?\n",
                                        QMessageBox::No | QMessageBox::Yes,
                                        QMessageBox::Yes);
    if (resBtn != QMessageBox::Yes) {
        event->ignore();
    } else {
        actionExit_triggered_cb();
        event->accept();
    }
}

void CrammingWindow::initUI()
{
    ui->lineEdit_PassingScore->setValidator( new QIntValidator(0, 99, this));

    ui->comboBox_Score->addItems({"-30", "-10", "0", "10", "20", "30", "40", "50", "60", "70", "80", "90", "100"});
    ui->comboBox_Score->clearEditText();

    const int tabStop = 4;  // 4 characters
    QFontMetrics metrics(ui->textEdit_Answer->font());
    ui->textEdit_Question->setTabStopDistance(tabStop * metrics.horizontalAdvance(' '));
    ui->textEdit_Answer->setTabStopDistance(tabStop * metrics.horizontalAdvance(' '));
    ui->textEdit_Response->setTabStopDistance(tabStop * metrics.horizontalAdvance(' '));

    adaptTexteditHeight(ui->textEdit_Info);
}

void CrammingWindow::initPlatformSpecificSettings()
{    
    if (QGuiApplication::platformName() == "android") {
        isAndroid = true;
        parentDir = "/sdcard/Qrammer";

        ui->textEdit_Response->setVisible(false);
        ui->lineEdit_PassingScore->setVisible(false);

        ui->progressBar_Learning->setVisible(false);

        ui->label_PassingScore->setVisible(false);
        ui->comboBox_Score->setEditable(false);
        ui->label_Score->setVisible(false);
        ui->label_NewScore->setVisible(false);

    } else {
        isAndroid = false;
        QDir tmpDir = QApplication::applicationFilePath();
        tmpDir.cdUp();
        tmpDir.cdUp();
        parentDir = tmpDir.path();
        ui->pushButton_Skip->setVisible(false);
    }

    if (!isAndroid) {
        QString styleSheet = QString("font-size:%1pt;").arg(settings.value("FontSize", 10).toInt());
        this->setStyleSheet(styleSheet);
    } else {
        QString styleSheet = QString("font-size:%1pt;").arg(13);
        this->setStyleSheet(styleSheet);
    }
}

void CrammingWindow::init(QList<CategoryMetaData *> *availableCat,
                          uint32_t newKuCoeff,
                          int interval,
                          int number,
                          int windowStyle)
{
    this->availableCategory = availableCat;
    this->newKuCoeff = newKuCoeff;
    this->interval = interval;
    this->number = number;
    this->windowStyle = windowStyle;

    totalKU = 0;
    for (int i = 0; i < availableCat->count(); i++)
        totalKU += availableCat->at(i)->number;

    remainingKUsToCram = totalKU;

    fillinThenExecuteCommand("Init");
}

void CrammingWindow::on_pushButton_Next_clicked()
{
    if (!ui->pushButton_Next->isEnabled())
        return;
    // This is to ensure that the user will not click the button twice.
    ui->pushButton_Next->setEnabled(false);
    ui->pushButton_ChooseImage->setEnabled(false);

    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

    if (!finalizeTheKUJustBeingCrammed())
        return;

    if (interval > 0 && (totalKU - remainingKUsToCram) > 0
        && (totalKU - remainingKUsToCram) % number == 0) {
        startInterval();
    } else {
        initNextKU();
    }
}

void CrammingWindow::startInterval()
{
    secDelayed = 0;
    // saveWindowLayout();
    hide();
    timerDelay->start(1000);
}

void CrammingWindow::tmrInterval()
{
    if (bossMode) {
        secDelayed = 0;
        trayIcon->setToolTip("BM activated");
    } else {
        secDelayed++;
        auto toolTip = QString("Qrammer\nProgress: %1/%2\nWait: %3 sec")
                           .arg(QString::number(totalKU - remainingKUsToCram),
                                QString::number(totalKU),
                                QString::number(interval * 60 - secDelayed));
        trayIcon->setToolTip(toolTip);
    }

    if (secDelayed < interval * 60)
        return;

    timerDelay->stop();
    secDelayed = 0;

    msgBox *myMsg = new msgBox(); // This is needed since a timeout before selection is required.
    myMsg->setWindowModality(Qt::ApplicationModal);
    myMsg->exec();

    if (myMsg->isAccepted()) {
        // restoreWindowLayout();
        show();
        initNextKU();
    } else {
        timerDelay->start(1000);
    }
}

void CrammingWindow::onAnswerShownCallback()
{
    fillinThenExecuteCommand("OnAnswerShown");
}

void CrammingWindow::on_pushButton_Check_clicked()
{
    if (!ui->pushButton_Check->isEnabled())
        return;

    ui->textEdit_Answer->setPlainText(cku.Answer);
    QPixmap answerImage;
    if (cku.AnswerImageBytes.size() > 0 && answerImage.loadFromData(cku.AnswerImageBytes)) {
        auto w = std::min(answerImage.width(), ANSWER_IMAGE_DIMENSION);
        auto h = std::min(answerImage.height(), ANSWER_IMAGE_DIMENSION);
        answerImage = answerImage.scaled(QSize(w, h), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        ui->label_AnswerImage->setPixmap(answerImage);
    } else {
        ui->label_AnswerImage->setText("[Empty]");
    }
    ui->pushButton_ChooseImage->setEnabled(true);
    adaptTexteditLineSpacing(ui->textEdit_Answer);
    ui->pushButton_Check->setEnabled(false);
    ui->comboBox_Score->setEnabled(true);

    onAnswerShownCallback();
}

bool CrammingWindow::finalizeTheKUJustBeingCrammed()
{
    if (availableCategory->at(currCatIndex)->number <= 0) {
        QMessageBox::warning(this,
                             "Qrammer - Warning",
                             "Logic error, this should not happen at all!");
        return false; // to be considered: return true or false?
    }

    double previousScore = calculateNewPreviousScore(ui->comboBox_Score->currentText().toDouble());
    int timesPracticed = cku.TimesPracticed + 1;

    bool hasDeadline = (previousScore - cku.PassingScore < 0);
    double addedDays = 0.0;

    if (hasDeadline) {
        // f(previousScore) = 1 / 10 * abs(previousScore - 40) * (previousScore - 40)
        addedDays = previousScore;
        addedDays = 0.1 * qAbs(addedDays - 40) * (addedDays - 40);
        addedDays = addedDays > 0 ? addedDays : 0;
    }

    //auto db = QSqlDatabase::addDatabase(DATABASE_DRIVER);
    //db.setDatabaseName(databaseName);
    while (true) {
        if (!db.isOpen() && !db.open()) {
            if (promptUserToRetryDBError("db.open()", db.databaseName(), db.lastError().text()))
                continue;
            else
                break;
        }

        QSqlQuery query = QSqlQuery(db);
        auto stmt = QString(R"**(
UPDATE knowledge_units
SET
    last_practice_time = DATETIME('Now', 'localtime'),
    previous_score = :previous_score,
    question = :question,
    answer = :answer,
    times_practiced = :times_practiced,
    passing_score = :passing_score,
    deadline = %1,
    client_name = :client_name,
    time_used = time_used + :time_used,
    answer_image = :answer_image
WHERE id = :id
)**")
                        .arg(hasDeadline ? ("DATETIME('Now', 'localtime', '"
                                            + QString::number(addedDays) + " day')")
                                         : "null");
        if (!query.prepare(stmt)) {
            if (promptUserToRetryDBError(QString("query->prepare(%1)").arg(stmt),
                                         db.databaseName(),
                                         query.lastError().text()))
                continue;
            else
                break;
        }
        query.bindValue(":previous_score", previousScore);
        query.bindValue(":question", ui->textEdit_Question->toPlainText());
        query.bindValue(":answer", ui->textEdit_Answer->toPlainText());
        query.bindValue(":times_practiced", timesPracticed);
        query.bindValue(":passing_score", ui->lineEdit_PassingScore->text());
        query.bindValue(":client_name", clientName);
        query.bindValue(":time_used",
                        (QDateTime::currentSecsSinceEpoch() - kuStartLearningTime > 300)
                            ? 300
                            : (QDateTime::currentSecsSinceEpoch() - kuStartLearningTime));
        query.bindValue(":id", cku.ID);
        if (!ui->label_AnswerImage->pixmap().isNull()) {
            QByteArray bArray;
            QBuffer buffer(&bArray);
            buffer.open(QIODevice::WriteOnly);
            ui->label_AnswerImage->pixmap().save(&buffer, "PNG");
            query.bindValue(":answer_image", bArray);
        } else {
            query.bindValue(":answer_image", QByteArray());
        }

        if (!query.exec()) {
            if (promptUserToRetryDBError(QString("query.exec() the following statement: 1%")
                                             .arg(stmt),
                                         db.databaseName(),
                                         query.lastError().text()))
                continue;
            else break; // Need to consider whether this break is needed.
        }

        if (timesPracticed <= 1) {
            query.prepare("UPDATE knowledge_units SET first_practice_time = DATETIME('Now', "
                          "'localtime') WHERE id = :id");
            query.bindValue(":id", cku.ID);
            if (!query.exec()) {
                if (promptUserToRetryDBError("open database",
                                             db.databaseName(),
                                             query.lastError().text()))
                    continue;
                else break; // Need to consider whether this break is needed.
            }
        }
        query.finish();
        break;
    }

    availableCategory->at(currCatIndex)->number--;
    remainingKUsToCram--;

    for (int i = 0; i < availableCategory->count(); i++)
        if (availableCategory->at(i)->number > 0)
            return true;

    QApplication::quit();
    // finishLearning();
    return false;
}

void CrammingWindow::preKuLoadGuiUpdate()
{
    this->setUpdatesEnabled(false);

    setWindowStyle(); // To be safe, setWindowStyle could be called both at the beginning and the end of this function.

    ui->textEdit_Answer->clear();
    ui->textEdit_Response->clear();
    ui->label_AnswerImage->setText("[Empty]");
    if (!isAndroid)
        ui->comboBox_Score->clearEditText();
    else
        ui->comboBox_Score->setCurrentIndex(0);

    ui->comboBox_Score->setEnabled(false);
    ui->label_NewScore->setText(": <null>");
    ui->label_NewScore->setStyleSheet("QLabel { color : black; }");

    ui->pushButton_Check->setEnabled(true);
    ui->pushButton_Next->setEnabled(false);
}

void CrammingWindow::postKuLoadGuiUpdate()
{
    int winWidth = this->size().width();
    QFont myFont(ui->textEdit_Info->font());
    QFontMetrics fm(myFont);

    QString infos[20];
    QString t, v1, v2;
    int i = 0;

    if (!isAndroid)
        infos[i++] = "ID: " + QString::number(cku.ID);

    v1 = "Cat: " + cku.Category;
    v2 = "Category: " + cku.Category;
    (isAndroid || fm.horizontalAdvance(v2) > winWidth / 9.0) ? infos[i++] = v1 : infos[i++] = v2;

    v1 = "Times: " + QString::number(cku.TimesPracticed);
    v2 = "Times Practiced: " + QString::number(cku.TimesPracticed);
    (fm.horizontalAdvance(v2) > winWidth / 9.0) ? infos[i++] = v1 : infos[i++] = v2;

    v1 = "Score: " + QString::number(cku.PreviousScore, 'f', 0);
    v2 = "Previous Score: " + QString::number(cku.PreviousScore, 'f', 1);
    (isAndroid || fm.horizontalAdvance(v2) > winWidth / 9.0) ? infos[i++] = v1 : infos[i++] = v2;

    v1 = "Insert: " + (cku.InsertTime.isNull() ? "nul" : cku.InsertTime.toString("yyyyMMdd"));
    v2 = "Insert: " + (cku.InsertTime.isNull() ? "<null>" : cku.InsertTime.toString("yyyy-MM-dd"));
    (fm.horizontalAdvance(v2) < winWidth / 10.0) ? infos[i++] = v2 : infos[i++] = v1;

    v1 = "First: "
         + (cku.FirstPracticeTime.isNull() ? "nul" : cku.FirstPracticeTime.toString("yyyyMMdd"));
    v2 = "First Practice: "
         + (cku.FirstPracticeTime.isNull() ? "<null>"
                                           : cku.FirstPracticeTime.toString("yyyy-MM-dd"));
    if (fm.horizontalAdvance(v2) < winWidth * 1.1 / 9.0)
        infos[i++] = v2;
    else if (fm.horizontalAdvance(v1) < winWidth * 2 / 9.0)
        infos[i++] = v1;

    v1 = "Min Used: " + QString::number(cku.SecSpent / 60);
    v2 = "Minutes Used: " + QString::number(cku.SecSpent / 60);
    if (fm.horizontalAdvance(v2) < winWidth * 1.1 / 9.0)
        infos[i++] = v2;
    else if (fm.horizontalAdvance(v1) < winWidth * 2 / 9.0)
        infos[i++] = v1;

    v1 = "Last: "
         + (cku.LastPracticeTime.isNull() ? "nul" : cku.LastPracticeTime.toString("yyyyMMdd"));
    v2 = "Last Practice: "
         + (cku.LastPracticeTime.isNull() ? "<null>" : cku.LastPracticeTime.toString("yyyy-MM-dd"));
    if (fm.horizontalAdvance(v2) < winWidth * 1.1 / 9.0)
        infos[i++] = v2;
    else if (fm.horizontalAdvance(v1) < winWidth * 2 / 9.0)
        infos[i++] = v1;

    v1 = "DDL: " + (cku.Deadline.isNull() ? "nul" : cku.Deadline.toString("yyyyMMdd"));
    v2 = "Deadline: " + (cku.Deadline.isNull() ? "<null>" : cku.Deadline.toString("yyyy-MM-dd"));
    (fm.horizontalAdvance(v2) < winWidth / 9.0) ? infos[i++] = v2 : infos[i++] = v1;

    v1 = "Client: " + (cku.ClientName.length() == 0 ? "nul" : cku.ClientName);
    v2 = "Client Name: " + (cku.ClientName.length() == 0 ? "<null>" : cku.ClientName);
    if (fm.horizontalAdvance(v2) < winWidth / 9.0)
        infos[i++] = v2;
    else if (fm.horizontalAdvance(v1) < winWidth * 2 / 9.0)
        infos[i++] = v1;

    if (isAndroid || winWidth <= 1200) {
        v1 = "Prog.: " + QString::number(totalKU - remainingKUsToCram + 1) + "/"
             + QString::number(totalKU);
        v2 = "Progress: " + QString::number(totalKU - remainingKUsToCram + 1) + "/"
             + QString::number(totalKU);
        (fm.horizontalAdvance(v2) < winWidth * 1.9 / 10.0) ? infos[i++] = v2 : infos[i++] = v1;

        ui->progressBar_Learning->setVisible(false);
    } else {
        v1 = "Breakdown: ";
        for (int j = 0; j < availableCategory->count(); j++) {
            if (availableCategory->at(j)->number <= 0)
                continue;
            v1 += availableCategory->at(j)->name + ": "
                  + QString::number(availableCategory->at(j)->number);
            v1 += ", ";
        }
        infos[i++] = v1.left(v1.length() - 2);
        ui->progressBar_Learning->setValue(
            static_cast<int>((totalKU - remainingKUsToCram + 1) * 100.0 / (totalKU)));
        ui->progressBar_Learning->setFormat(QString::number(totalKU - remainingKUsToCram + 1) + "/"
                                            + QString::number(totalKU));
        ui->progressBar_Learning->setVisible(true);
    }

    for (int i = 0; i < 20; i++) {
        if (infos[i].length() > 0)
            t += infos[i];
        if (infos[i + 1].length() > 0 && i < 19)
            t += "; ";
        else
            break;
    }

    ui->lineEdit_PassingScore->setText(QString::number(cku.PassingScore));

    ui->textEdit_Info->setPlainText(t);

    adaptTexteditHeight(ui->textEdit_Question);
    adaptTexteditHeight(ui->textEdit_Info);
    adaptTexteditHeight(ui->textEdit_Response);
    setWindowStyle(); // To be safe, setWindowStyle could be called both at the beginning and the end of this function.

    ui->textEdit_Response->setFocus();

    /*
    QPixmap answerImage;
    if (answerImage.loadFromData(cku.AnswerImageBytes)) {
        auto w = std::min(answerImage.width(), ANSWER_IMAGE_DIMENSION);
        auto h = std::min(answerImage.height(), ANSWER_IMAGE_DIMENSION);
        answerImage = answerImage.scaled(QSize(w, h), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        ui->label_AnswerImage->setPixmap(answerImage);
    }
    */
    ui->label_AnswerImage->setText("[Hidden]");
    this->setUpdatesEnabled(true);
    this->repaint(); // Without repaint, the whole window will not be painted at all.

    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

void CrammingWindow::fillinThenExecuteCommand(QString callbackName)
{
    QString cmdTemplate = settings.value(QString("Callbacks/%1").arg(callbackName), "").toString();
    if (cmdTemplate.length() > 0) {
        auto cmd = cmdTemplate.replace("{callbackName}", callbackName)
                       .replace("{Question}", cku.Question)
                       .replace("{Answer}", cku.Answer)
                       .replace("{Category}", cku.Category)
                       .replace("{ClientName}", cku.ClientName)
                       .replace("{ID}", QString::number(cku.ID))
                       .replace("{TimesPracticed}", QString::number(cku.TimesPracticed));
        SPDLOG_INFO("Callback {} triggered, command to execute: {}",
                    callbackName.toStdString(),
                    cmd.toStdString());
        execExternalProgramAsync(cmd.toStdString());
        // system(cmd.toStdString().c_str());
    } else {
        SPDLOG_INFO("Callback {}'s command is empty", callbackName.toStdString());
    }
}

void CrammingWindow::onKuLoadCallback()
{
    fillinThenExecuteCommand("OnKULoad");
}

void CrammingWindow::initNextKU()
{
    preKuLoadGuiUpdate();
    loadNewKU(0);
    postKuLoadGuiUpdate();
    onKuLoadCallback();
    kuStartLearningTime = QDateTime::currentSecsSinceEpoch();
}

int CrammingWindow::pickCategoryforNewKU()
{
    int r = ranGen.generate() % remainingKUsToCram;
    int s = 0;

    // It tries to draw a category probabilistically using remaining KUs' distribution
    for (int i = 0; i < availableCategory->count(); i++) {
        s += availableCategory->at(i)->number;
        if (r < s) {
            return i;
        }
    }
    throw std::runtime_error("pickCategoryforNewKU() failed");
}

void CrammingWindow::loadNewKU(int recursion_depth)
{
    auto max_retry = 100;
    SPDLOG_INFO("Loading new knowledge unit (recursion_depth: {}, max_depth: {}) ",
                recursion_depth,
                max_retry);
    if (recursion_depth > max_retry) {
        auto errMsg = QString("max_retry (%1) reached, the program must exit").arg(max_retry);
        SPDLOG_ERROR(errMsg.toStdString());
        QMessageBox::critical(this, "Qrammer - Critical", errMsg);
        QApplication::exit();
        return;
    }

    try {
        currCatIndex = pickCategoryforNewKU();
    } catch (std::runtime_error &e) {
        SPDLOG_ERROR(e.what());
        QMessageBox::critical(this, "Qrammer - Critical", e.what());
        // let someone higher up the call stack handle it if they want
        throw;
    }
    //
    auto currCat = availableCategory->at(currCatIndex)->name;

    auto msg = QString("Remaining KUs to be crammed by category: ");
    for (int i = 0; i < availableCategory->length(); i++) {
        msg += QString("%1: %2, ")
                   .arg(availableCategory->at(i)->name,
                        QString::number(availableCategory->at(i)->number));
    }
    SPDLOG_INFO(msg.toStdString());

    //auto db = QSqlDatabase::addDatabase(DATABASE_DRIVER);
    //db.setDatabaseName(databaseName);
    if (!db.isOpen() && !db.open()) {
        if (promptUserToRetryDBError("db.open()", db.databaseName(), db.lastError().text())) {
            loadNewKU(++recursion_depth);
            return;
        }
        QApplication::exit();
        return;
    }
    QSqlQuery query = QSqlQuery(db);
    QString stmt = R"***(
SELECT COUNT(*)
FROM knowledge_units
WHERE
    category = :category AND
    deadline <= DATETIME('now', 'localtime') AND
    is_shelved = 0
)***";
    if (!query.prepare(stmt)) {
        if (promptUserToRetryDBError(QString("query.prepare(%1)").arg(stmt),
                                     db.databaseName(),
                                     db.lastError().text())) {
            loadNewKU(++recursion_depth);
            return;
        }
        QApplication::exit();
        return;
    }
    query.bindValue(":category", currCat);

    int dueNumByCat = 0;
    if (!query.exec()) {
        if (promptUserToRetryDBError(
                QString("query.exec() the following statement to extract dueNumByCat:\n%1").arg(stmt),
                db.databaseName(),
                query.lastError().text())) {
            loadNewKU(++recursion_depth);
            return;
        }
        QApplication::exit();
        return;
    }
    if (query.next()) {
        dueNumByCat = query.value(0).toInt();
        SPDLOG_INFO("dueNum of category [{}]: {}", currCat.toStdString(), dueNumByCat);
        query.finish();
    } else {
        if (promptUserToRetryDBError("Extracting dueNumByCat from query.next()",
                                     db.databaseName(),
                                     query.lastError().text())) {
            loadNewKU(++recursion_depth);
            return;
        }
        QApplication::exit();
        return;
    }

    stmt = R"***(
SELECT COUNT(*)
FROM knowledge_units
WHERE category = :category AND is_shelved = 0
)***";
    if (!query.prepare(stmt)) {
        if (promptUserToRetryDBError(QString("query.prepare(%1)").arg(stmt),
                                     db.databaseName(),
                                     query.lastError().text())) {
            loadNewKU(++recursion_depth);
            return;
        }
        QApplication::exit();
        return;
    }
    query.bindValue(":category", currCat);

    if (!query.exec()) {
        if (promptUserToRetryDBError(
                QString("query.exec() the following statement to extract TotalNum:\n%1").arg(stmt),
                db.databaseName(),
                query.lastError().text())) {
            loadNewKU(++recursion_depth);
            return;
        }
        QApplication::exit();
        return;
    }
    int TotalNum;
    if (query.next()) {
        TotalNum = query.value(0).toInt();
        query.finish();
    } else {
        if (promptUserToRetryDBError("Extracting TotalNum from query.next()",
                                     db.databaseName(),
                                     query.lastError().text())) {
            loadNewKU(++recursion_depth);
            return;
        }
        QApplication::exit();
        return;
    }

    double urgencyCoeff = 1
                          - (qPow(static_cast<double>(TotalNum - dueNumByCat) / TotalNum,
                                  0.0275 * TotalNum + dueNumByCat)
                             * 0.65);
    SPDLOG_INFO("TotalNum: {}, dueNumByCat: {}, urgencyCoef: {}",
                TotalNum,
                dueNumByCat,
                urgencyCoeff);
    const auto columns = QString(R"***(
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
client_name,
category,
time_used,
answer_image)***");
    double r = ranGen.generateDouble();
    SPDLOG_INFO("A random number in [0, 1): {}", r);
    if (r < urgencyCoeff) {
        SPDLOG_INFO("SELECTing an urgent unit");
        stmt = QString(R"***(
SELECT %1
FROM knowledge_units
WHERE
    category = :category AND
    deadline <= DATETIME('now', 'localtime') AND
    is_shelved = 0
ORDER BY RANDOM()
LIMIT 1
)***")
                   .arg(columns);
        query.prepare(stmt);
        query.bindValue(":category", currCat);
    } else {
        if (ranGen.generate() % 100 <= newKuCoeff) {
            SPDLOG_INFO("Not SELECTing an urgent unit, SELECTing a new unit");
            stmt = QString(R"***(
SELECT %1
FROM knowledge_units
WHERE
    category = :category AND
    times_practiced = 0 AND
    is_shelved = 0
ORDER BY RANDOM()
LIMIT 1
)***")
                       .arg(columns);
            query.prepare(stmt);
            query.bindValue(":category", currCat);
        } else {
            SPDLOG_INFO("Randomly SELECTing a unit");
            stmt = QString(R"***(
SELECT %1
FROM knowledge_units
WHERE
    category = :category AND
    is_shelved = 0
ORDER BY RANDOM()
LIMIT 1;
)***")
                       .arg(columns);
            if (!query.prepare(stmt)) {
                if (promptUserToRetryDBError(
                        QString(
                            "query.prepare() the following stmt to load a new KnowledgeUnit:\n%1")
                            .arg(stmt),
                        db.databaseName(),
                        query.lastError().text())) {
                    loadNewKU(++recursion_depth);
                    return;
                }
                QApplication::exit();
                return;
            }
            query.bindValue(":category", currCat);
        }
    }

    if (!query.exec()) {
        if (promptUserToRetryDBError(
                QString("query.exec() the following query to load a new KnowledgeUnit:\n%1")
                    .arg(query.lastQuery()),
                db.databaseName(),
                query.lastError().text())) {
            loadNewKU(++recursion_depth);
            return;
        }
        QApplication::exit();
        return;
    }
    if (query.first()) {
        int idx = 0;
        cku.ID = query.value(idx++).toInt();
        cku.Question = query.value(idx++).toString();
        cku.Answer = query.value(idx++).toString();
        cku.PassingScore = query.value(idx++).toDouble();
        cku.PreviousScore = query.value(idx++).toDouble();
        cku.TimesPracticed = query.value(idx++).toInt();
        cku.InsertTime = query.value(idx++).toDateTime();
        cku.FirstPracticeTime = query.value(idx++).toDateTime();
        cku.LastPracticeTime = query.value(idx++).toDateTime();
        cku.Deadline = query.value(idx++).toDateTime();
        cku.ClientName = query.value(idx++).toString();
        cku.Category = query.value(idx++).toString();
        cku.SecSpent = query.value(idx++).toInt();
        cku.AnswerImageBytes = query.value(idx++).toByteArray();
        query.finish();
    } else {
        // if (promptUserToRetryDBError("Extracting next KU from query.first()",
        //                             db.databaseName(),
        //                            query.lastError().text())) {
        loadNewKU(++recursion_depth);
        //   return;
        // }
        //QApplication::exit();
        //return;
    }

    ui->textEdit_Question->setPlainText(cku.Question);
    adaptTexteditLineSpacing(ui->textEdit_Question);
    adaptTexteditLineSpacing(ui->textEdit_Response);
}

void CrammingWindow::on_comboBox_Score_currentTextChanged(const QString &)
{

    ui->pushButton_Next->setEnabled(true);
    if (ui->comboBox_Score->currentText().length() > 0)
    {
        double t = ui->comboBox_Score->currentText().toDouble();

        double newScore = calculateNewPreviousScore(t);
        ui->label_NewScore->setText(": " + QString::number(newScore, 'f', 1));

        if (newScore > cku.PreviousScore)
            ui->label_NewScore->setStyleSheet("QLabel { color : green; }");
        else if (newScore < cku.PreviousScore)
            ui->label_NewScore->setStyleSheet("QLabel { color : red; }");

    }
}

double CrammingWindow::calculateNewPreviousScore(double newScore)
{
    double timesPracticed = cku.TimesPracticed <= 9 ? cku.TimesPracticed : 9;
    double previousScore = cku.PreviousScore;

    previousScore = (newScore + previousScore * timesPracticed) / (timesPracticed + 1);

    return previousScore;
}

void CrammingWindow::adaptTexteditLineSpacing(QTextEdit *textedit)
{
    if (textedit == nullptr) return;

    QTextDocument *doc =  textedit->document();
    QTextCursor textcursor = textedit->textCursor();

    for(QTextBlock it = doc->begin(); it !=doc->end();it = it.next()) {
      QTextBlockFormat tbf = it.blockFormat();
      tbf.setLineHeight(textedit->fontMetrics().lineSpacing() * 0.3, QTextBlockFormat::LineDistanceHeight);
      textcursor.setPosition(it.position());
      textcursor.setBlockFormat(tbf);
      textedit->setTextCursor(textcursor);
    }

    QFont font = this->font();          // This is a simple hack to ensure that the format of pasted text would not impact the TextEdit.
  //  if (QGuiApplication::platformName() == "windows") // This is still an ugly hack for Windows platform.
  //      font.setFamily("Microsoft Yahei");
    textedit->setFont(font);    
    textedit->setFontPointSize(settings.value("FontSize", 10).toInt());
    textedit->setFontWeight(font.weight());
    textedit->setFontUnderline(false);
    textedit->setFontItalic(false);    

    textedit -> moveCursor(QTextCursor::Start);
    textedit -> ensureCursorVisible();
}

void CrammingWindow::adaptTexteditHeight(QTextEdit *textedit)
{
    if (!textedit) return;

    while (!textedit->verticalScrollBar()->isVisible() && textedit->height() > 0)
        textedit->setFixedHeight(textedit->height() - 1);

    QRect geo = QApplication::primaryScreen()->availableGeometry();

    double heightLimit = this->height() > QGuiApplication::primaryScreen()->availableGeometry().height() ? this->height() : QGuiApplication::primaryScreen()->availableGeometry().height();
    while (textedit->verticalScrollBar()->isVisible() &&
           textedit->height() <= heightLimit * 2 / 5.0 && this->height() <= geo.size().height())
        // this ratio cannot be set to a higher value otherwise the landscape mode on android would be not higher enough for the window.
        // The avoid the abovementioned situation, the landscape view is disabled altogether
        textedit->setFixedHeight(textedit->height() + 1);

    ui->progressBar_Learning->setFixedHeight(ui->textEdit_Info->height());

}

void CrammingWindow::adaptSkipButton()
{
    if (ui->horizontalSpacer->geometry().width() > ui->pushButton_Skip->width() || isAndroid == true)
        ui->pushButton_Skip->setVisible(true);
    else if (ui->pushButton_Skip->isVisible() && ui->horizontalSpacer->geometry().width() <= 0) {
        ui->pushButton_Skip->setVisible(false);
    }
}

void CrammingWindow::finalizeCrammingSession()
{
    QString temp;
    for (int i = 0; i < availableCategory->count(); i++)
        temp += availableCategory->at(i)->snapshot->getComparison(QGuiApplication::platformName() != "android") + "\n";

    QMessageBox *msg = new QMessageBox(QMessageBox::Information,
                                       "Qrammer - Result",
                                       temp,
                                       QMessageBox::Ok,
                                       this);

    QFont font("Monospace");
    font.setStyleHint(QFont::TypeWriter);
    msg->setFont(font);
    msg->exec();

    trayIcon->hide(); // The Icon should be hide here since the program is quitted by the next line.
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

    fillinThenExecuteCommand("OnCrammingFinished");
    QApplication::quit();
}

void CrammingWindow::initContextMenu()
{    
    menuQuestion = new QMenu(this);
    menuAnswer = new QMenu(this);
    menuBlank = new QMenu(this);

    QAction* copy = new QAction();
    copy->setText("Copy");
    menuQuestion->addAction(copy);
    menuAnswer->addAction(copy);
    menuBlank->addAction(copy);

    QAction* paste = new QAction();
    paste->setText("Paste Plaintext");
    menuQuestion->addAction(paste);
    menuAnswer->addAction(paste);
    menuBlank->addAction(paste);

    QAction* skip = new QAction();
    skip->setText("Skip this KU");
    menuQuestion->addAction(skip);
    menuAnswer->addAction(skip);
    menuBlank->addAction(skip);


    menuQuestion->addSeparator();
    menuAnswer->addSeparator();
    menuBlank->addSeparator();

    SearchOptions = new QHash<QString, QString>;
    //auto db = QSqlDatabase::addDatabase(DATABASE_DRIVER);
    // db.setDatabaseName(databaseName);
    if (db.open()) {
        QSqlQuery *query = new QSqlQuery(db);

        query->prepare("SELECT name, url FROM search_options ORDER BY id ASC");
        query->exec();

        while (query->next()) {
            SearchOptions->insert(query->value(0).toString(), query->value(1).toString());

            QAction* actionSearchOption = new QAction(ui->textEdit_Question);
            actionSearchOption->setText(query->value(0).toString());
            menuQuestion->addAction(actionSearchOption);
            menuAnswer->addAction(actionSearchOption);
            menuBlank->addAction(actionSearchOption);
        }

    } else {
        QMessageBox::warning(this, "Warning", "Cannot open the databse:\n" + db.lastError().text());
        QApplication::quit();
    }

    ui->textEdit_Question->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->textEdit_Question,
            SIGNAL(customContextMenuRequested(QPoint)),
            this,
            SLOT(showContextMenu_Question(QPoint)));
    ui->textEdit_Answer->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->textEdit_Answer,
            SIGNAL(customContextMenuRequested(QPoint)),
            this,
            SLOT(showContextMenu_Answer(QPoint)));
    ui->textEdit_Response->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->textEdit_Response,
            SIGNAL(customContextMenuRequested(QPoint)),
            this,
            SLOT(showContextMenu_Blank(QPoint)));
}

void CrammingWindow::initTrayMenu()
{
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QIcon(":/qrammer.ico"));
    trayIcon->setToolTip("Qrammer");
    trayIcon->show();

    QMenu* menuTray = new QMenu(this);

    QAction* actionStartLearning = menuTray->addAction("Start Learning NOW!");
    connect(actionStartLearning,
            SIGNAL(triggered()),
            this,
            SLOT(actionStartLearning_triggered_cb()));
    menuTray->addAction(actionStartLearning);

    QAction* actionResetTimer = menuTray->addAction("Reset Timer");
    connect(actionResetTimer, SIGNAL(triggered()), this, SLOT(actionResetTimer_triggered_cb()));
    menuTray->addAction(actionResetTimer);

    QAction* actionBossMode = menuTray->addAction("Activate BM");
    connect(actionBossMode, SIGNAL(triggered()), this, SLOT(actionBossMode_triggered_cb()));
    menuTray->addAction(actionBossMode);

    QAction* actionExit = menuTray->addAction("Exit");
    connect(actionExit, SIGNAL(triggered()), this, SLOT(actionExit_triggered_cb()));
    menuTray->addAction(actionExit);

    trayIcon->setContextMenu(menuTray);
}

void CrammingWindow::showContextMenu_Question(const QPoint &pt)
{
    showContextMenu_Common(ui->textEdit_Question, menuQuestion, pt);
}

void CrammingWindow::showContextMenu_Answer(const QPoint &pt)
{
    showContextMenu_Common(ui->textEdit_Answer, menuAnswer, pt);
}

void CrammingWindow::showContextMenu_Blank(const QPoint &pt)
{
    showContextMenu_Common(ui->textEdit_Response, menuBlank, pt);
}

void CrammingWindow::showContextMenu_Common(QTextEdit *edit, QMenu *menu, const QPoint &pt)
{
    QPoint globalPos = edit->mapToGlobal(pt);

    QAction* selectedItem = menu->exec(globalPos);
    if (selectedItem)
    {
        if (selectedItem->text() == "Copy")
            QApplication::clipboard()->setText(edit->textCursor().selectedText());

        if (selectedItem->text() == "Paste Plaintext")
            edit->textCursor().insertText(QApplication::clipboard()->text());

        if (selectedItem->text() == "Skip this KU")
            initNextKU();

        if (SearchOptions->contains(selectedItem->text()))
        {
            QString link = SearchOptions->value(selectedItem->text());
            link.replace(QString("SEARCHTEXT"), QString(edit->textCursor().selectedText()));
            QDesktopServices::openUrl(QUrl(link));
            QApplication::clipboard()->setText(edit->textCursor().selectedText());
        }
    }
}

void CrammingWindow::setWindowStyle()
{
    if (!isVisible()) // This condition is needed since this function can accidentally show the window when it should be hidden.
        return;

    QStyleOptionTitleBar so;
    so.titleBarFlags = Qt::Window;
    int titlebarHeight = this->style()->pixelMetric(QStyle::PM_TitleBarHeight, &so, this);

    QRect geo = QApplication::primaryScreen()->availableGeometry();
    QWidget::showNormal();
    switch (windowStyle) {
    case 1000:
        this->setGeometry(0, titlebarHeight, geo.width() / 3, geo.height() - titlebarHeight);
        break;
    case 1100:
        this->setGeometry(0, titlebarHeight, geo.width() / 2, geo.height() - titlebarHeight);
        break;
    // You cant write 0011 here: an integer constant that starts with 0 is an octal number
    case 11:
        setGeometry(geo.width() / 2, titlebarHeight, geo.width() / 2, geo.height() - titlebarHeight);
        break;
    case 1:
        setGeometry(geo.width() / 3 * 2,
                    titlebarHeight,
                    geo.width() / 3,
                    geo.height() - titlebarHeight);
        break;
    default:
        this->showMaximized();
    }
    adaptSkipButton();

    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);        // To make sure the resize is implemented before next step.
}

QString CrammingWindow::convertStringToFilename(QString name)
{
    QString output;
    for (const auto c : name) {
        if (c.isLetterOrNumber() || c == '.' || c == '_' || c == '-') {
            output += c;
        } else
            output += '_';
    }
    return output;
}

/*
void CrammingWindow::handleTTS(bool isQuestion)
{
    QString sanitizedFilepath, originalText, sanitizedFilename;
    TTSDownloader *ttsDownloader = new TTSDownloader(this);
    bool isTTSEnabled = false;

    if (isQuestion) {
        originalText = cku.Question;
        sanitizedFilename = convertStringToFilename(originalText);
        for (int i = 0; i < availableCategory->count(); i++) {
            if (availableCategory->at(i)->name == cku.Category
                && availableCategory->at(i)->ttsOption == 1) {
                isTTSEnabled = true;
                break;
            }
        }
    } else {
        originalText = ui->textEdit_Answer->document()->findBlockByLineNumber(0).text();
        sanitizedFilename = convertStringToFilename(originalText);
        for (int i = 0; i < availableCategory->count(); i++) {
            if (availableCategory->at(i)->name == cku.Category
                && availableCategory->at(i)->ttsOption == 2) {
                isTTSEnabled = true;
                break;
            }
        }
    }
    sanitizedFilepath = parentDir + "/speeches/" + sanitizedFilename + ".mp3";

    if (!isTTSEnabled) {
        delete ttsDownloader;
        return;
    }

    QFileInfo check_file(sanitizedFilepath);
    if (check_file.exists() && check_file.isFile()) {
        SPDLOG_INFO("Start playing TTS file at {}", sanitizedFilepath.toStdString());
        player->setSource(QUrl::fromLocalFile(sanitizedFilepath));
        player->play();
    } else {
        auto url = "http://dict.youdao.com/dictvoice?audio=" + originalText + "&amp;amp;le=eng%3C";
        SPDLOG_INFO("Start downloading TTS file from {} to {}",
                    url.toStdString(),
                    sanitizedFilepath.toStdString());
        ttsDownloader->doDownload(url, sanitizedFilepath);
        // http, instead of https, is used here.
        // If https is used, the program would encounter a "TLS initialization failed" error on Windows. Not sure what would happen on Linux
    }

    // ttsDownloader->deleteLater();
    //Since ttsDownloader works in an asynchronous manner, the object cannot be simply deleted here.
}
*/

void CrammingWindow::actionResetTimer_triggered_cb()
{
    if (bossMode) {
        return;
    }
    else{
        secDelayed = 0;
    }
}

void CrammingWindow::actionBossMode_triggered_cb()
{
    bossMode = !bossMode;
    if (bossMode){
        QAction *action = trayIcon->contextMenu()->actions().at(2);
        action->setText("Deactivate BM");
        startInterval();    // No matter if the user is learning or waiting, just start the waiting cycle should be fine.
    } else {
        bmbox *myBm = new bmbox();
        myBm->setWindowModality(Qt::ApplicationModal);
        myBm->exec();

        if (myBm->isVerified()) {
            QAction *action = trayIcon->contextMenu()->actions().at(2);
            action->setText("Activate BM");
        } else {
            bossMode = true;
        }
    }
}

void CrammingWindow::actionStartLearning_triggered_cb()
{
    if (bossMode) {
        return;
    }
    else{
        secDelayed = interval * 60 - 5;
    }
}

void CrammingWindow::actionExit_triggered_cb()
{
    if (bossMode) {
        return;
    } else {
        finalizeCrammingSession();
        trayIcon->hide();       // This line alone won't work since finishLearning() would  quit the program before this line is executed
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        QApplication::quit();
    }
}

void CrammingWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    adaptTexteditHeight(ui->textEdit_Question);
    adaptTexteditHeight(ui->textEdit_Response);
    adaptTexteditHeight(ui->textEdit_Info);

    adaptSkipButton();
}

void CrammingWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->modifiers()&Qt::ControlModifier && (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return))
        on_pushButton_Check_clicked();

    if (event->key() == Qt::Key_PageDown)
        on_pushButton_Next_clicked();

    if (ui->comboBox_Score->isEnabled()) {
        if (event->key() == Qt::Key_Escape)
            ui->comboBox_Score->setCurrentText("0");
        if (event->key() == Qt::Key_F1)
            ui->comboBox_Score->setCurrentText("10");
        if (event->key() == Qt::Key_F2)
            ui->comboBox_Score->setCurrentText("20");
        if (event->key() == Qt::Key_F3)
            ui->comboBox_Score->setCurrentText("30");
        if (event->key() == Qt::Key_F4)
            ui->comboBox_Score->setCurrentText("40");
        if (event->key() == Qt::Key_F5)
            ui->comboBox_Score->setCurrentText("50");
        if (event->key() == Qt::Key_F6)
            ui->comboBox_Score->setCurrentText("60");
        if (event->key() == Qt::Key_F7)
            ui->comboBox_Score->setCurrentText("70");
        if (event->key() == Qt::Key_F8)
            ui->comboBox_Score->setCurrentText("80");
        if (event->key() == Qt::Key_F9)
            ui->comboBox_Score->setCurrentText("90");
        if (event->key() == Qt::Key_F10)
            ui->comboBox_Score->setCurrentText("100");
    }

/*    if (event->key() == Qt::Key_Back) This condition is not necessary: once you overriden closeEvent, the back key on Android would trigger the closeEvent by default.
        closeEvent(nullptr); */
    QMainWindow::keyPressEvent(event);
}

void CrammingWindow::on_pushButton_Skip_clicked()
{
    initNextKU();
}

void CrammingWindow::on_textEdit_Question_textChanged()
{
    adaptTexteditHeight(ui->textEdit_Question);
    setWindowStyle();
}

void CrammingWindow::on_textEdit_Info_textChanged()
{
    adaptTexteditHeight(ui->textEdit_Info);
    setWindowStyle();
}

void CrammingWindow::on_pushButton_Skip_pressed()
{
    CrammingWindow::on_pushButton_Skip_clicked();
}

void CrammingWindow::on_pushButton_Check_pressed()
{
    CrammingWindow::on_pushButton_Check_clicked();
}

void CrammingWindow::on_pushButton_Next_pressed()
{
    CrammingWindow::on_pushButton_Next_clicked();
}

// Return value: If user would like to retry the same database operation
bool CrammingWindow::promptUserToRetryDBError(QString operationName, QString dbPath, QString errMsg)
{
    auto msg = QString(R"*(Operation: %1
Database path: %2
Error message: %3)*")
                   .arg(operationName, dbPath, errMsg);
    SPDLOG_ERROR(msg.toStdString());
    QMessageBox::StandardButton resBtn = QMessageBox::warning(this,
                                                              "Qrammer - DB Error",
                                                              msg + "\n\n" + "Retry?",
                                                              QMessageBox::No | QMessageBox::Yes,
                                                              QMessageBox::Yes);
    if (resBtn == QMessageBox::Yes) {
        SPDLOG_INFO("User selected Yes");
        return true;
    } else {
        SPDLOG_INFO("User selected No");
        return false;
    }
}

void CrammingWindow::on_pushButton_ChooseImage_clicked()
{
    auto fileContentReady = [this](const QString &fileName, const QByteArray &fileContent) {
        if (fileName.isEmpty()) {
            SPDLOG_INFO("No file is selected");
        } else {
            SPDLOG_INFO("fileName: {}", fileName.toStdString());
            SPDLOG_INFO("fileContent.size(): {} bytes", fileContent.size());
            QPixmap answerImage;
            // cku.AnswerImageBytes = fileContent;
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

void CrammingWindow::on_textEdit_Response_textChanged()
{
    adaptTexteditHeight(ui->textEdit_Response);
    setWindowStyle();
}
