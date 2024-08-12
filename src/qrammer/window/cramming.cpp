#include "src/qrammer/window/cramming.h"
#include "src/qrammer/db.h"
#include "src/qrammer/global_variables.h"
#include "src/qrammer/utils.h"
#include "src/qrammer/window/cramming_reminder.h"
#include "src/qrammer/window/manage_db.h"
#include "src/qrammer/window/ui_cramming.h"

#include <QRandomGenerator>
#include <QRegularExpression>
#include <spdlog/spdlog.h>

using namespace Qrammer::Window;

Cramming::Cramming(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Cramming)
    , winDb(Qrammer::Window::ManageDB())
{
    ui->setupUi(this);

    initTrayMenu();
    initPlatformSpecificSettings();

    ranGen = QRandomGenerator(static_cast<uint>(QTime::currentTime().msec()));

    timerDelay = new QTimer(this);
    connect(timerDelay, SIGNAL(timeout()), this, SLOT(tmrInterval()));

    initUI();
    initContextMenu();

    clientName = settings.value("ClientName", "Qrammer-Unspecified").toString();

    cku.PreviousScore = 0;
}

Cramming::~Cramming()
{
    delete ui;
}

void Cramming::closeEvent(QCloseEvent *event)
{
    if (remainingKUsToCram == 0) {
        actionExit_triggered_cb();
        event->accept();
        return;
    }
    auto resBtn = QMessageBox::question(this,
                                        "Qrammer",
                                        "Are you sure to quit?",
                                        QMessageBox::No | QMessageBox::Yes,
                                        QMessageBox::Yes);
    if (resBtn != QMessageBox::Yes) {
        event->ignore();
    } else {
        actionExit_triggered_cb();
        event->accept();
    }
}

void Cramming::initUI()
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

void Cramming::initPlatformSpecificSettings()
{
    QString styleSheet = QString("font-size:%1pt;").arg(settings.value("FontSize", 10).toInt());
    this->setStyleSheet(styleSheet);
}

void Cramming::init(uint32_t newKuCoeff, int interval, int number, int windowStyle)
{
    this->newKuCoeff = newKuCoeff;
    this->interval = interval;
    this->number = number;
    this->windowStyle = windowStyle;

    totalKuToCram = 0;
    for (size_t i = 0; i < availableCategory.size(); i++)
        totalKuToCram += availableCategory[i].KuToCramCount;

    remainingKUsToCram = totalKuToCram;

    fillinThenExecuteCommand("Init");
}

void Cramming::on_pushButton_Next_clicked()
{
    if (!ui->pushButton_Next->isEnabled())
        return;
    // This is to ensure that the user will not click the button twice.
    ui->pushButton_Next->setEnabled(false);
    ui->pushButton_ChooseAnswerImage->setEnabled(false);

    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

    updateCkuByGuiElements();
    if (!finalizeKuJustBeingCrammed())
        return;
    spdlog::default_logger()->flush();

    if (interval > 0 && (totalKuToCram - remainingKUsToCram) > 0
        && (totalKuToCram - remainingKUsToCram) % number == 0) {
        startInterval();
    } else {
        initNextKU();
    }
}

void Cramming::startInterval()
{
    secDelayed = 0;
    hide();
    timerDelay->start(1000);
}

void Cramming::tmrInterval()
{
    secDelayed++;
    auto toolTip = QString("Qrammer\nProgress: %1/%2\nWait: %3 sec")
                       .arg(QString::number(totalKuToCram - remainingKUsToCram),
                            QString::number(totalKuToCram),
                            QString::number(interval * 60 - secDelayed));
    trayIcon->setToolTip(toolTip);

    if (secDelayed < interval * 60)
        return;

    timerDelay->stop();
    secDelayed = 0;

    auto myCr = CrammingReminder(); // This is needed since a timeout before selection is required.
    myCr.setWindowModality(Qt::ApplicationModal);
    myCr.exec();

    if (myCr.isAccepted()) {
        show();
        initNextKU();
    } else {
        timerDelay->start(1000);
    }
}

void Cramming::onAnswerShownCallback()
{
    fillinThenExecuteCommand("OnAnswerShown");
}

void Cramming::on_pushButton_Check_clicked()
{
    if (!ui->pushButton_Check->isEnabled())
        return;

    ui->textEdit_Answer->setPlainText(cku.Answer);
    QPixmap answerImage;
    if (cku.AnswerImageBytes.size() > 0 && answerImage.loadFromData(cku.AnswerImageBytes)) {
        ui->label_AnswerImage->setPixmap(answerImage);
    } else {
        ui->label_AnswerImage->setText("[Empty]");
    }
    ui->pushButton_ChooseAnswerImage->setEnabled(true);
    adaptTexteditLineSpacing(ui->textEdit_Answer);
    ui->pushButton_Check->setEnabled(false);
    ui->comboBox_Score->setEnabled(true);

    onAnswerShownCallback();
}

void Cramming::updateCkuByGuiElements()
{
    cku.ClientName = clientName;
    cku.Question = ui->textEdit_Question->toPlainText();
    cku.Answer = ui->textEdit_Answer->toPlainText();
    cku.PassingScore = ui->lineEdit_PassingScore->text().toDouble();
    cku.NewScore = calculateNewPreviousScore(ui->comboBox_Score->currentText().toDouble());
    cku.timeUsedSec += ((QDateTime::currentSecsSinceEpoch() - kuStartLearningTime > 300)
                            ? 300
                            : (QDateTime::currentSecsSinceEpoch() - kuStartLearningTime));
    cku.TimesPracticed += 1;
    if (cku.TimesPracticed <= 1) {
        assert(cku.FirstPracticeTime.isNull());
        cku.FirstPracticeTime = QDateTime::currentDateTime();
    }

    if (cku.NewScore - cku.PassingScore < 0) {
        double addedDays = 30 - (cku.PassingScore - cku.NewScore);
        cku.Deadline = QDateTime::currentDateTime().addDays(addedDays);
    } else {
        cku.Deadline = QDateTime();
        assert(cku.Deadline.isNull());
    }

    if (!ui->label_AnswerImage->pixmap().isNull()) {
        QBuffer buffer(&cku.AnswerImageBytes);
        buffer.open(QIODevice::WriteOnly);
        ui->label_AnswerImage->pixmap().save(&buffer, "PNG");
    } else {
        cku.AnswerImageBytes = QByteArray();
    }
    if (!ui->label_QuestionImage->pixmap().isNull()) {
        QBuffer buffer(&cku.QuestionImageBytes);
        buffer.open(QIODevice::WriteOnly);
        ui->label_QuestionImage->pixmap().save(&buffer, "PNG");
    } else {
        cku.QuestionImageBytes = QByteArray();
    }
}

bool Cramming::finalizeKuJustBeingCrammed()
{
    assert(availableCategory[currCatIndex].KuToCramCount > 0);

    while (true) {
        try {
            int rowsAffected;
            if ((rowsAffected = db.updateKu(cku)) != 1)
                QMessageBox::warning(this,
                                     "Qrammer",
                                     QString("%1 units affected, expecting 1").arg(rowsAffected));
            break;
        } catch (const std::runtime_error &e) {
            auto errMsg = std::string("Error db.updateKu(cku): ") + e.what();
            SPDLOG_ERROR(errMsg);
            if (!promptUserToRetryDBError("finalizeTheKUJustBeingCrammed()",
                                          QString::fromStdString(errMsg))) {
                QApplication::quit();
                return false;
            }
        }
    }

    availableCategory[currCatIndex].KuToCramCount--;
    remainingKUsToCram--;
    size_t sum = 0;
    for (const auto &cat : availableCategory)
        sum += cat.KuToCramCount;
    assert(sum == remainingKUsToCram);

    for (size_t i = 0; i < availableCategory.size(); i++)
        if (availableCategory[i].KuToCramCount > 0)
            return true;

    QApplication::quit();
    return false;
}

void Cramming::preKuLoadGuiUpdate()
{
    this->setUpdatesEnabled(false);

    setWindowStyle(); // To be safe, setWindowStyle could be called both at the beginning and the end of this function.

    ui->textEdit_Answer->clear();
    ui->textEdit_Response->clear();
    ui->label_QuestionImage->setText("[Empty]");
    ui->label_AnswerImage->setText("[Hidden]");
    ui->pushButton_ChooseAnswerImage->setEnabled(false);
    ui->comboBox_Score->clearEditText();

    ui->comboBox_Score->setEnabled(false);
    ui->label_NewScore->setText(": <null>");
    ui->label_NewScore->setStyleSheet("QLabel { color : black; }");

    ui->pushButton_Check->setEnabled(true);
    ui->pushButton_Next->setEnabled(false);
}

void Cramming::postKuLoadGuiUpdate()
{
    ui->textEdit_Question->setPlainText(cku.Question);
    adaptTexteditLineSpacing(ui->textEdit_Question);
    adaptTexteditLineSpacing(ui->textEdit_Response);

    QPixmap questionImage;
    if (cku.QuestionImageBytes.size() > 0 && questionImage.loadFromData(cku.QuestionImageBytes)) {
        ui->label_QuestionImage->setPixmap(questionImage);
    } else {
        ui->label_QuestionImage->setText("[Empty]");
    }

    int winWidth = this->size().width();
    QFont myFont(ui->textEdit_Info->font());
    QFontMetrics fm(myFont);

    QString infos[20];
    QString t, v1, v2;
    int idx = 0;

    infos[idx++] = "ID: " + QString::number(cku.ID);

    v1 = "Cat: " + cku.Category;
    v2 = "Category: " + cku.Category;
    (fm.horizontalAdvance(v2) > winWidth / 9.0) ? infos[idx++] = v1 : infos[idx++] = v2;

    v1 = "Times: " + QString::number(cku.TimesPracticed);
    v2 = "Times Practiced: " + QString::number(cku.TimesPracticed);
    (fm.horizontalAdvance(v2) > winWidth / 9.0) ? infos[idx++] = v1 : infos[idx++] = v2;

    v1 = "Score: " + QString::number(cku.PreviousScore, 'f', 0);
    v2 = "Previous Score: " + QString::number(cku.PreviousScore, 'f', 1);
    (fm.horizontalAdvance(v2) > winWidth / 9.0) ? infos[idx++] = v1 : infos[idx++] = v2;

    v1 = "Insert: " + (cku.InsertTime.isNull() ? "nul" : cku.InsertTime.toString("yyyyMMdd"));
    v2 = "Insert: " + (cku.InsertTime.isNull() ? "<null>" : cku.InsertTime.toString("yyyy-MM-dd"));
    (fm.horizontalAdvance(v2) < winWidth / 10.0) ? infos[idx++] = v2 : infos[idx++] = v1;

    v1 = "First: "
         + (cku.FirstPracticeTime.isNull() ? "nul" : cku.FirstPracticeTime.toString("yyyyMMdd"));
    v2 = "First Practice: "
         + (cku.FirstPracticeTime.isNull() ? "<null>"
                                           : cku.FirstPracticeTime.toString("yyyy-MM-dd"));
    if (fm.horizontalAdvance(v2) < winWidth * 1.1 / 9.0)
        infos[idx++] = v2;
    else if (fm.horizontalAdvance(v1) < winWidth * 2 / 9.0)
        infos[idx++] = v1;

    v1 = "Min Used: " + QString::number(cku.timeUsedSec / 60);
    v2 = "Minutes Used: " + QString::number(cku.timeUsedSec / 60);
    if (fm.horizontalAdvance(v2) < winWidth * 1.1 / 9.0)
        infos[idx++] = v2;
    else if (fm.horizontalAdvance(v1) < winWidth * 2 / 9.0)
        infos[idx++] = v1;

    v1 = "Last: "
         + (cku.LastPracticeTime.isNull() ? "nul" : cku.LastPracticeTime.toString("yyyyMMdd"));
    v2 = "Last Practice: "
         + (cku.LastPracticeTime.isNull() ? "<null>" : cku.LastPracticeTime.toString("yyyy-MM-dd"));
    if (fm.horizontalAdvance(v2) < winWidth * 1.1 / 9.0)
        infos[idx++] = v2;
    else if (fm.horizontalAdvance(v1) < winWidth * 2 / 9.0)
        infos[idx++] = v1;

    v1 = "DDL: " + (cku.Deadline.isNull() ? "nul" : cku.Deadline.toString("yyyyMMdd"));
    v2 = "Deadline: " + (cku.Deadline.isNull() ? "<null>" : cku.Deadline.toString("yyyy-MM-dd"));
    (fm.horizontalAdvance(v2) < winWidth / 9.0) ? infos[idx++] = v2 : infos[idx++] = v1;

    v1 = "Client: " + (cku.ClientName.length() == 0 ? "nul" : cku.ClientName);
    v2 = "Client Name: " + (cku.ClientName.length() == 0 ? "<null>" : cku.ClientName);
    if (fm.horizontalAdvance(v2) < winWidth / 9.0)
        infos[idx++] = v2;
    else if (fm.horizontalAdvance(v1) < winWidth * 2 / 9.0)
        infos[idx++] = v1;

    if (winWidth <= 1200) {
        v1 = "Prog.: " + QString::number(totalKuToCram - remainingKUsToCram + 1) + "/"
             + QString::number(totalKuToCram);
        v2 = "Progress: " + QString::number(totalKuToCram - remainingKUsToCram + 1) + "/"
             + QString::number(totalKuToCram);
        (fm.horizontalAdvance(v2) < winWidth * 1.9 / 10.0) ? infos[idx++] = v2 : infos[idx++] = v1;

        ui->progressBar_Learning->setVisible(false);
    } else {
        v1 = "Breakdown: ";
        for (size_t j = 0; j < availableCategory.size(); j++) {
            if (availableCategory[j].KuToCramCount <= 0)
                continue;
            v1 += availableCategory[j].name + ": "
                  + QString::number(availableCategory[j].KuToCramCount);
            v1 += ", ";
        }
        infos[idx++] = v1.left(v1.length() - 2);
        ui->progressBar_Learning->setValue(
            static_cast<int>((totalKuToCram - remainingKUsToCram + 1) * 100.0 / (totalKuToCram)));
        ui->progressBar_Learning->setFormat(QString::number(totalKuToCram - remainingKUsToCram + 1) + "/"
                                            + QString::number(totalKuToCram));
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

    ui->label_AnswerImage->setText("[Hidden]");
    this->setUpdatesEnabled(true);
    this->repaint(); // Without repaint, the whole window will not be painted at all.

    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

void Cramming::fillinThenExecuteCommand(QString callbackName)
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

void Cramming::onKuLoadCallback()
{
    fillinThenExecuteCommand("OnKULoad");
}

void Cramming::initNextKU()
{
    preKuLoadGuiUpdate();
    loadNextKu(0);
    postKuLoadGuiUpdate();
    onKuLoadCallback();
    kuStartLearningTime = QDateTime::currentSecsSinceEpoch();
    spdlog::default_logger()->flush();
}

size_t Cramming::pickCategoryforNewKU()
{
    size_t r = ranGen.generate() % remainingKUsToCram;
    size_t s = 0;

    // It tries to draw a category probabilistically using remaining KUs' distribution
    for (size_t i = 0; i < availableCategory.size(); i++) {
        s += availableCategory[i].KuToCramCount;
        if (r < s) {
            return i;
        }
    }
    throw std::runtime_error("pickCategoryforNewKU() failed");
}

void Cramming::loadNextKu(int recursion_depth)
{
    auto max_retry = 100;
    SPDLOG_INFO("Loading next knowledge unit (recursion_depth: {}, max_depth: {}) ",
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
    auto &currCat = availableCategory[currCatIndex];

    auto msg = QString("Remaining KUs to be crammed by category: ");
    for (size_t i = 0; i < availableCategory.size(); i++) {
        msg += QString("%1: %2, ")
                   .arg(availableCategory[i].name,
                        QString::number(availableCategory[i].KuToCramCount));
    }
    SPDLOG_INFO(msg.toStdString());

    try {
        currCat.dueKuCount = db.getDueKuCountByCategory(currCat.name);
    } catch (const std::runtime_error &e) {
        QString errMsg = QString("Failed to query dueNumByCat: %1").arg(e.what());
        SPDLOG_ERROR(errMsg.toStdString());
        if (promptUserToRetryDBError("getDueNumByCategory()", errMsg)) {
            loadNextKu(++recursion_depth);
            return;
        }
        QApplication::exit();
        return;
    }

    try {
        db.updateTotalKuCount(currCat);
    } catch (const std::runtime_error &e) {
        QString errMsg = QString("Failed to query TotalNum: %1").arg(e.what());
        SPDLOG_ERROR(errMsg.toStdString());
        if (promptUserToRetryDBError("getTotalKUNumByCategory()", errMsg)) {
            loadNextKu(++recursion_depth);
            return;
        }
        QApplication::exit();
        return;
    }

    double urgencyCoeff = 1
                          - (qPow(static_cast<double>(currCat.totalKuCount - currCat.dueKuCount)
                                      / currCat.totalKuCount,
                                  0.0275 * currCat.totalKuCount + currCat.dueKuCount)
                             * 0.65);
    SPDLOG_INFO("totalKuCount: {}, dueKuCountByCat: {}, urgencyCoef: {:.5f}",
                currCat.totalKuCount,
                currCat.dueKuCount,
                urgencyCoeff);

    double r = ranGen.generateDouble();
    SPDLOG_INFO("A random number r in [0, 1): {:.5f}", r);
    try {
        if (r < urgencyCoeff) {
            SPDLOG_INFO("r < urgencyCoeff, SELECTing an urgent unit");
            cku = db.getUrgentKu(currCat.name);
        } else {
            SPDLOG_INFO("r >= urgencyCoeff, urgent unit SELECTion skipped");
            if (ranGen.generate() % 100 <= newKuCoeff) {
                SPDLOG_INFO("SELECTing a new unit");
                cku = db.getNewKu(currCat.name);
            } else {
                SPDLOG_INFO("Randomly SELECTing a unit");
                cku = db.getRandomOldKu(currCat);
            }
        }
    } catch (const std::runtime_error &e) {
        QString errMsg = QString("Failed to select a knowledge unit: %1").arg(e.what());
        SPDLOG_ERROR(errMsg.toStdString());
        if (promptUserToRetryDBError("Select a knowledge unit from database", errMsg)) {
            loadNextKu(++recursion_depth);
            return;
        }
        QApplication::exit();
        return;
    }
    spdlog::default_logger()->flush();
}

void Cramming::on_comboBox_Score_currentTextChanged(const QString &)
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

double Cramming::calculateNewPreviousScore(double newScore)
{
    double timesPracticed = cku.TimesPracticed <= 9 ? cku.TimesPracticed : 9;
    double previousScore = cku.PreviousScore;

    previousScore = (newScore + previousScore * timesPracticed) / (timesPracticed + 1);

    return previousScore;
}

void Cramming::adaptTexteditLineSpacing(QTextEdit *textedit)
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

void Cramming::adaptTexteditHeight(QTextEdit *textedit)
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

void Cramming::finalizeCrammingSession()
{
    QString snapDiffStr;
    for (size_t i = 0; i < availableCategory.size(); i++) {
        auto snap = Snapshot(availableCategory[i].name);
        snap.category = availableCategory[i].name;
        db.updateSnapshot(snap);
        snapDiffStr += snap - availableCategory[i].snapshot + "\n";
    }
    SPDLOG_INFO("snapDiffStr: {}", snapDiffStr.toStdString());
    QMessageBox *msg = new QMessageBox(QMessageBox::Information,
                                       "Qrammer - Result",
                                       snapDiffStr,
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

void Cramming::initContextMenu()
{    
    menuQuestion = new QMenu(this);
    menuAnswer = new QMenu(this);
    menuBlank = new QMenu(this);

    QAction *copy = new QAction(this);
    copy->setText("Copy");
    menuQuestion->addAction(copy);
    menuAnswer->addAction(copy);
    menuBlank->addAction(copy);

    QAction *paste = new QAction(this);
    paste->setText("Paste Plaintext");
    menuQuestion->addAction(paste);
    menuAnswer->addAction(paste);
    menuBlank->addAction(paste);

    QAction *skip = new QAction(this);
    skip->setText("Skip this KU");
    menuQuestion->addAction(skip);
    menuAnswer->addAction(skip);
    menuBlank->addAction(skip);


    menuQuestion->addSeparator();
    menuAnswer->addSeparator();
    menuBlank->addSeparator();

    SearchOptions = QHash<QString, QString>();
    try {
        auto options = db.getSearchOptions();
        SearchOptions = QHash<QString, QString>();
        for (auto &[name, url] : options) {
            SearchOptions.insert(name, url);
            QAction *actionSearchOption = new QAction(ui->textEdit_Question);
            actionSearchOption->setText(name);
            menuQuestion->addAction(actionSearchOption);
            menuAnswer->addAction(actionSearchOption);
            menuBlank->addAction(actionSearchOption);
        }
    } catch (const std::runtime_error &e) {
        auto errMsg = QString("getSearchOptions failed: %1").arg(e.what());
        SPDLOG_ERROR(errMsg.toStdString());
        QMessageBox::critical(this, "Qrammer - critical", errMsg);
        QApplication::quit();
        return;
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

void Cramming::initTrayMenu()
{
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QIcon(":/qrammer.ico"));
    trayIcon->setToolTip("Qrammer");
    trayIcon->show();

    QMenu* menuTray = new QMenu(this);

    connect(menuTray->addAction("Start cramming NOW!"),
            SIGNAL(triggered()),
            this,
            SLOT(actionStartLearning_triggered_cb()));

    connect(menuTray->addAction("Reset Timer"),
            SIGNAL(triggered()),
            this,
            SLOT(actionResetTimer_triggered_cb()));

    connect(menuTray->addAction("Manage DB"),
            SIGNAL(triggered()),
            this,
            SLOT(actionManageDb_triggered_cb()));

    connect(menuTray->addAction("Exit"), SIGNAL(triggered()), this, SLOT(actionExit_triggered_cb()));

    // QSystemTrayIcon does NOT take the ownership of QMenu
    trayIcon->setContextMenu(menuTray);
}

void Cramming::actionManageDb_triggered_cb()
{
    winDb.show();
}

void Cramming::actionStartLearning_triggered_cb()
{
    secDelayed = interval * 60 - 5;
}

void Cramming::showContextMenu_Question(const QPoint &pt)
{
    showContextMenu_Common(ui->textEdit_Question, menuQuestion, pt);
}
void Cramming::actionResetTimer_triggered_cb()
{
    secDelayed = 0;
}

void Cramming::showContextMenu_Answer(const QPoint &pt)
{
    showContextMenu_Common(ui->textEdit_Answer, menuAnswer, pt);
}

void Cramming::showContextMenu_Blank(const QPoint &pt)
{
    showContextMenu_Common(ui->textEdit_Response, menuBlank, pt);
}

void Cramming::showContextMenu_Common(QTextEdit *edit, QMenu *menu, const QPoint &pt)
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

        if (SearchOptions.contains(selectedItem->text())) {
            QString link = SearchOptions.value(selectedItem->text());
            link.replace(QString("SEARCHTEXT"), QString(edit->textCursor().selectedText()));
            QDesktopServices::openUrl(QUrl(link));
            QApplication::clipboard()->setText(edit->textCursor().selectedText());
        }
    }
}

void Cramming::setWindowStyle()
{
    if (!isVisible()) // This condition is needed since this function can accidentally show the window when it should be hidden.
        return;

    QStyleOptionTitleBar so;
    so.titleBarFlags = Qt::Window;
    int titlebarHeight = this->style()->pixelMetric(QStyle::PM_TitleBarHeight, &so, this);

    QRect geo = QApplication::primaryScreen()->availableGeometry();
    switch (windowStyle) {
    case 1000:
        QWidget::showNormal();
        this->setGeometry(0, titlebarHeight, geo.width() / 3, geo.height() - titlebarHeight);
        break;
    case 1100:
        QWidget::showNormal();
        this->setGeometry(0, titlebarHeight, geo.width() / 2, geo.height() - titlebarHeight);
        break;
    // You cant write 0011 here: an integer constant that starts with 0 is an octal number
    case 11:
        QWidget::showNormal();
        setGeometry(geo.width() / 2, titlebarHeight, geo.width() / 2, geo.height() - titlebarHeight);
        break;
    case 1:
        QWidget::showNormal();
        setGeometry(geo.width() / 3 * 2,
                    titlebarHeight,
                    geo.width() / 3,
                    geo.height() - titlebarHeight);
        break;
    default:
        this->showMaximized();
    }

    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);        // To make sure the resize is implemented before next step.
}

QString Cramming::convertStringToFilename(QString name)
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

void Cramming::actionExit_triggered_cb()
{
    finalizeCrammingSession();
    // This line alone won't work since finishLearning() would  quit the program before this line is executed
    trayIcon->hide();
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    QApplication::quit();
}

void Cramming::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    adaptTexteditHeight(ui->textEdit_Question);
    adaptTexteditHeight(ui->textEdit_Response);
    adaptTexteditHeight(ui->textEdit_Info);
}

void Cramming::keyPressEvent(QKeyEvent *event)
{
    auto em = event->modifiers();
    auto ek = event->key();
    qDebug() << "event->modifiers(): " << em << ", event->key(): " << ek;
    if (em & Qt::ControlModifier && (ek == Qt::Key_Enter || ek == Qt::Key_Return)) {
        on_pushButton_Check_clicked();
    } else if (em & Qt::ControlModifier && ek == Qt::Key_H) {
        startInterval();
    } else if (em & Qt::ControlModifier && (ek == Qt::Key_PageDown || ek == Qt::Key_Down)) {
        on_pushButton_Next_clicked();
    } else if (ui->comboBox_Score->isEnabled()) {
        if (ek == Qt::Key_Escape)
            ui->comboBox_Score->setCurrentText("0");
        if (ek == Qt::Key_F1)
            ui->comboBox_Score->setCurrentText("10");
        if (ek == Qt::Key_F2)
            ui->comboBox_Score->setCurrentText("20");
        if (ek == Qt::Key_F3)
            ui->comboBox_Score->setCurrentText("30");
        if (ek == Qt::Key_F4)
            ui->comboBox_Score->setCurrentText("40");
        if (ek == Qt::Key_F5)
            ui->comboBox_Score->setCurrentText("50");
        if (ek == Qt::Key_F6)
            ui->comboBox_Score->setCurrentText("60");
        if (ek == Qt::Key_F7)
            ui->comboBox_Score->setCurrentText("70");
        if (ek == Qt::Key_F8)
            ui->comboBox_Score->setCurrentText("80");
        if (ek == Qt::Key_F9)
            ui->comboBox_Score->setCurrentText("90");
        if (ek == Qt::Key_F10)
            ui->comboBox_Score->setCurrentText("100");
    }
    QMainWindow::keyPressEvent(event);
}

void Cramming::on_textEdit_Question_textChanged()
{
    adaptTexteditHeight(ui->textEdit_Question);
    setWindowStyle();
}

void Cramming::on_textEdit_Info_textChanged()
{
    adaptTexteditHeight(ui->textEdit_Info);
    setWindowStyle();
}

void Cramming::on_pushButton_Skip_pressed()
{
    initNextKU();
}

void Cramming::on_pushButton_Check_pressed()
{
    Cramming::on_pushButton_Check_clicked();
}

void Cramming::on_pushButton_Next_pressed()
{
    Cramming::on_pushButton_Next_clicked();
}

// Return value: If user would like to retry the same database operation
bool Cramming::promptUserToRetryDBError(QString operationName, QString errMsg)
{
    auto msg = QString(R"*(Operation: %1
Database path: %2
Error message: %3)*")
                   .arg(operationName, QString::fromStdString(db.getDatabasePath()), errMsg);
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

void Cramming::on_textEdit_Response_textChanged()
{
    adaptTexteditHeight(ui->textEdit_Response);
    setWindowStyle();
}

void Cramming::on_pushButton_ChooseQuestionImage_clicked()
{
    qDebug("Cramming::on_pushButton_ChooseQuestionImage_clicked()");
    ui->label_QuestionImage->setPixmap(selectImageFromFileSystem());
}

void Cramming::on_pushButton_ChooseAnswerImage_clicked()
{
    ui->label_AnswerImage->setPixmap(selectImageFromFileSystem());
}

void Cramming::on_pushButton_ManageDB_clicked()
{
    winDb.show();
}

void Cramming::on_pushButton_Delete_clicked()
{
    if (QMessageBox::question(this,
                              "Qrammer",
                              QString("Sure to remove the KU [%1]?").arg(cku.ID),
                              QMessageBox::Yes | QMessageBox::No)
        != QMessageBox::Yes)
        return;
    db.deleteKu(cku.ID);
    initNextKU();
}
