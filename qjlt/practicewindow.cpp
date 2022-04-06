#include "practicewindow.h"
#include "ui_practicewindow.h"

PracticeWindow::PracticeWindow(QWidget *parent, QSqlDatabase mySQL) :
    QMainWindow(parent),
    ui(new Ui::PracticeWindow)
{
    ui->setupUi(this);

    initTrayMenu();
    initPlatformSpecificSettings();

    this->mySQL = mySQL;

    player = new QMediaPlayer(this);
    player->setVolume(50);

    qsrand(static_cast<uint>(QTime::currentTime().msec()));

    timerDelay = new QTimer(this);
    connect(timerDelay, SIGNAL(timeout()), this, SLOT(tmrInterval()));

    initUI();
    initContextMenu();

    QSettings settings("MamsdsStudio", "MamsdsQJointLearningTools");
    clientName = settings.value("ClientName", "QJLT-Unspecified").toString();

//    initNextKU();

//    setWindowStyle();    
}

PracticeWindow::~PracticeWindow()
{
    delete ui;
}

void PracticeWindow::closeEvent (QCloseEvent *event)
{
    QMessageBox::StandardButton resBtn = QMessageBox::question( this, "Mamsds QJLT",
                                                                tr("Are you sure to quit?\n"),
                                                                QMessageBox::No | QMessageBox::Yes,
                                                                QMessageBox::Yes);
    if (resBtn != QMessageBox::Yes) {
            event->ignore();
    } else {
        on_actionExit_triggered();
        event->accept();
    }
}

void PracticeWindow::showEvent(QShowEvent *event)
{
    if (isAndroid) { // This function is needed for android, otherwise the first KU will not be loaded correctly.
      // The implementation is copied from: https://stackoverflow.com/questions/3752742/how-do-i-create-a-pause-wait-function-using-qt
        QTime dieTime= QTime::currentTime().addMSecs(500);
        while (QTime::currentTime() < dieTime)
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        setWindowStyle();
    }
}

void PracticeWindow::initUI()
{
    ui->lineEdit_PassingScore->setValidator( new QIntValidator(0, 99, this));

    ui->comboBox_Score->addItems({"-30", "-10", "0", "10", "20", "30", "40", "50", "60", "70", "80", "90", "100"});
    ui->comboBox_Score->clearEditText();

    const int tabStop = 4;  // 4 characters
    QFontMetrics metrics(ui->textEdit_Answer->font());
    ui->textEdit_Question->setTabStopDistance(tabStop * metrics.width(' '));
    ui->textEdit_Answer->setTabStopDistance(tabStop * metrics.width(' '));
    ui->textEdit_Draft->setTabStopDistance(tabStop * metrics.width(' '));

    adaptTexteditHeight(ui->textEdit_Info);
}

void PracticeWindow::initPlatformSpecificSettings()
{    
    if (QGuiApplication::platformName() == "android") {
        isAndroid = true;
        parentDir = "/sdcard/JLT";

        ui->pushButton_Switch->setVisible(true);
//        ui->pushButton_Switch->setMinimumWidth(160);
     //   qDebug() << "ui->pushButton_Switch->width()" << ui->pushButton_Switch->width();
  //      ui->pushButton_Skip->setMinimumWidth(140);
    //    ui->pushButton_Check->setMinimumWidth(155);
      //  ui->pushButton_Next->setMinimumWidth(150);

        //ui->comboBox_Score->setMinimumWidth(150);
        // the above width limitation is to ensure all four buttons can be shown on a not-so-big-size Android device.

        ui->textEdit_Draft->setVisible(false);
        ui->lineEdit_PassingScore->setVisible(false);

        ui->progressBar_Learning->setVisible(false);

        ui->label_PassingScore->setVisible(false);
        ui->comboBox_Score->setEditable(false);
        ui->label_Score->setVisible(false);
        ui->Label_NewScore->setVisible(false);

    } else {
        isAndroid = false;
        QDir tmpDir = QApplication::applicationFilePath();
        tmpDir.cdUp();
        tmpDir.cdUp();
        parentDir = tmpDir.path();

        ui->pushButton_Switch->setVisible(false);
        ui->pushButton_Skip->setVisible(false);
    }

    ui->textEdit_DraftAndroid->setVisible(false);   //No matter if program is running on Android, this textEdit should be hide by default.

    if (!isAndroid) {
        QSettings settings("MamsdsStudio", "MamsdsQJointLearningTools");
        QString styleSheet = QString("font-size:%1pt;").arg(settings.value("FontSize", 10).toInt());
        this->setStyleSheet(styleSheet);
    } else {
        QString styleSheet = QString("font-size:%1pt;").arg(13);
        this->setStyleSheet(styleSheet);
    }
}

void PracticeWindow::init(QList<CategoryMetaData *> *availableCat, int NKI, int interval, int number, int windowStyle)
{
    this->availableCategory = availableCat;
    this->NKI = NKI;
    this->interval = interval;
    this->number = number;
    this->windowStyle = windowStyle;

    totalKU = 0;
    for (int i = 0; i < availableCat->count(); i++)
        totalKU += availableCat->at(i)->number;

    outstandingKU = totalKU;
}

void PracticeWindow::on_pushButton_Next_clicked()
{
    if (!ui->pushButton_Next->isEnabled())
        return;    
    ui->pushButton_Next->setEnabled(false); // This line is to ensure that the user will not click the button twice.

    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

    if (!finalizeLastKU()) return;

    if (interval > 0 && (totalKU - outstandingKU) > 0 && (totalKU - outstandingKU) % number == 0) {
        startInterval();
    } else {
        initNextKU();
    }
}

void PracticeWindow::startInterval()
{
    secDelayed = 0;
    hide();
    timerDelay->start(1000);
}

void PracticeWindow::tmrInterval()
{
    if (bossMode) {
        secDelayed = 0;
        trayIcon->setToolTip("BM activated");
    } else {
        secDelayed++;
        trayIcon->setToolTip("Mamsds QJoint Learning Tool\nProgress: "
                             + QString::number(totalKU - outstandingKU ) + "/" + QString::number(totalKU)
                             + "\nWait: " + QString::number(interval * 60 - secDelayed) + " sec");
    }

    if (secDelayed < interval * 60)
        return;

    timerDelay->stop();
    secDelayed = 0;

    msgBox *myMsg = new msgBox();       // This is needed since a timeout before selection is required.
    myMsg->setWindowModality(Qt::ApplicationModal);
    myMsg->exec();

    if (myMsg->isAccepted()) {
        show();
        initNextKU();
    } else {
        timerDelay->start(1000);
    }
}

void PracticeWindow::on_pushButton_Check_clicked()
{
    if (!ui->pushButton_Check->isEnabled())
        return;

    ui->textEdit_Answer->setPlainText(cku_Answer);
   // ui->textEdit_Answer->moveCursor(QTextCursor::Start);
  //  ui->textEdit_Answer->ensureCursorVisible();

    adaptTexteditLineSpacing(ui->textEdit_Answer);
    ui->pushButton_Check->setEnabled(false);
    ui->comboBox_Score->setEnabled(true);
    if (ui->pushButton_Switch->text() == "Answer") {
        on_pushButton_Switch_pressed();
    }
    handleTTS(false);
}

// The return value means "if it is okay to start the next ku"
bool PracticeWindow::finalizeLastKU()
{
    if (availableCategory->at(currCatIndex)->number <= 0) {
        QMessageBox::warning(this, "Mamsds QJLT - Warning", "Logic error, this should not happen at all!");
        return false; // to be considered: return true or false?
    }

    double previousScore = calculateNewPreviousScore(ui->comboBox_Score->currentText().toDouble());
    int timesPracticed = cku_TimesPracticed + 1;

    bool hasDeadline = (previousScore - cku_PassingScore < 0);
    double addedDays = 0.0;

    if (hasDeadline) {
        // f(previousScore) = 1 / 10 * abs(previousScore - 40) * (previousScore - 40)
        addedDays = previousScore;
        addedDays = 0.1 * qAbs(addedDays - 40) * (addedDays - 40);
        addedDays = addedDays > 0 ? addedDays : 0;
    }

    while (true) {

        if (!mySQL.open()) {
            if (handleDatabaseOperationError("open database", mySQL.databaseName(), mySQL.lastError().databaseText() + "\n" + mySQL.lastError().driverText()))
                continue;
            else
                break;
        }

        QSqlQuery *query = new QSqlQuery(mySQL);
        query->prepare(QString("UPDATE knowledge_units SET last_practice_time = DATETIME('Now', 'localtime'), previous_score = :previous_score, question = :question, answer = :answer, times_practiced = :times_practiced, ") +
                       "passing_score = :passing_score, " + "deadline = " + (hasDeadline ? ("DATETIME('Now', 'localtime', '" + QString::number(addedDays) + " day')") : "null") + ", " +
                       "client_name = :client_name, time_used = time_used + :time_used WHERE id = :id");
        query->bindValue(":previous_score", previousScore);
        query->bindValue(":question", ui->textEdit_Question->toPlainText());
        query->bindValue(":answer", ui->textEdit_Answer->toPlainText());
        query->bindValue(":times_practiced", timesPracticed);
        query->bindValue(":passing_score", ui->lineEdit_PassingScore->text());
        query->bindValue(":client_name", clientName);
        query->bindValue(":time_used", (QDateTime::currentSecsSinceEpoch() - kuStartLearningTime > 300) ? 300 : (QDateTime::currentSecsSinceEpoch() - kuStartLearningTime));
        query->bindValue(":id", cku_ID);

        if (!query->exec()){
            if (handleDatabaseOperationError("open database", mySQL.databaseName(), query->lastError().databaseText() + "\n" + query->lastError().driverText()))
                continue;
            else break; // Need to consider whether this break is needed.
        }

        if (timesPracticed <= 1) {
            query->prepare("UPDATE knowledge_units SET first_practice_time = DATETIME('Now', 'localtime') WHERE id = :id");
            query->bindValue(":id", cku_ID);
            if (!query->exec()){
                if (handleDatabaseOperationError("open database", mySQL.databaseName(), query->lastError().databaseText()+ "\n" + query->lastError().driverText()))
                    continue;
                else break; // Need to consider whether this break is needed.
            }
        }
        query->finish();
        mySQL.close();
        break;
    }

    // These two operations should only happen aftere the database is written is successfully.
    availableCategory->at(currCatIndex)->number--;
    outstandingKU--;

    for (int i = 0; i < availableCategory->count(); i++)
        if (availableCategory->at(i)->number > 0)
            return true;

    finishLearning();
    return false;
}

void PracticeWindow::initNextKU()
{
    this->setUpdatesEnabled(false);

    setWindowStyle();   // To be safe, setWindowStyle could be called both at the beginning and the end of this function.

    ui->textEdit_Answer->clear();
    ui->textEdit_Draft->clear();
    ui->textEdit_DraftAndroid->clear();

    if (!isAndroid)
        ui->comboBox_Score->clearEditText();
    else
        ui->comboBox_Score->setCurrentIndex(0);

    ui->comboBox_Score->setEnabled(false);
    ui->Label_NewScore->setText(": <null>");
    ui->Label_NewScore->setStyleSheet("QLabel { color : black; }");

    ui->pushButton_Check->setEnabled(true);
    ui->pushButton_Next->setEnabled(false);

    // Init the above first and then load new ku later.
    loadNewKU(0);

    int winWidth = this->size().width();
    QFont myFont(ui->textEdit_Info->font());
    QFontMetrics fm(myFont);

    QString infos[20];
    QString t, v1, v2;
    int i = 0;

    if (!isAndroid) infos[i++] = "ID: " + QString::number(cku_ID);

    v1 = "Cat: " + cku_Category;
    v2 = "Category: " + cku_Category;
    (isAndroid || fm.width(v2) > winWidth / 9.0) ? infos[i++] = v1 : infos[i++] = v2;

    v1 = "Times: " + QString::number(cku_TimesPracticed);
    v2 = "Times Practiced: " + QString::number(cku_TimesPracticed);
    (fm.width(v2) > winWidth / 9.0) ? infos[i++] = v1 : infos[i++] = v2;

    v1 =  "Score: " + QString::number(cku_PreviousScore, 'f', 0);
    v2 = "Previous Score: " + QString::number(cku_PreviousScore, 'f', 1);
    (isAndroid || fm.width(v2) > winWidth / 9.0) ? infos[i++] = v1 : infos[i++] = v2;

    v1 = "Insert: " + (cku_InsertTime.isNull() ? "nul" : cku_InsertTime.toString("yyyyMMdd"));
    v2 = "Insert: " + (cku_InsertTime.isNull() ? "<null>" : cku_InsertTime.toString("yyyy-MM-dd"));
    (fm.width(v2) < winWidth / 10.0) ? infos[i++] = v2 : infos[i++] = v1;

    v1 = "First: " + (cku_FirstPracticeTime.isNull() ? "nul" : cku_FirstPracticeTime.toString("yyyyMMdd"));
    v2 = "First Practice: " + (cku_FirstPracticeTime.isNull() ? "<null>" : cku_FirstPracticeTime.toString("yyyy-MM-dd"));
    if (fm.width(v2) < winWidth * 1.1 / 9.0) infos[i++] = v2;
    else if (fm.width(v1) < winWidth * 2 / 9.0) infos[i++] = v1;

    v1 = "Min Used: " + QString::number(cku_SecSpent / 60);
    v2 = "Minutes Used: " + QString::number(cku_SecSpent / 60);
    if (fm.width(v2) < winWidth * 1.1 / 9.0) infos[i++] = v2;
    else if (fm.width(v1) < winWidth * 2 / 9.0) infos[i++] = v1;

    v1 =  "Last: " + (cku_LastPracticeTime.isNull() ? "nul" : cku_LastPracticeTime.toString("yyyyMMdd"));
    v2 =  "Last Practice: " + (cku_LastPracticeTime.isNull() ? "<null>" : cku_LastPracticeTime.toString("yyyy-MM-dd"));
    if (fm.width(v2) < winWidth *1.1 / 9.0) infos[i++] = v2;
    else if (fm.width(v1) < winWidth * 2 / 9.0) infos[i++] = v1;

    v1 = "DDL: " + (cku_Deadline.isNull() ? "nul" : cku_Deadline.toString("yyyyMMdd"));
    v2 = "Deadline: " + (cku_Deadline.isNull() ? "<null>" : cku_Deadline.toString("yyyy-MM-dd"));
    (fm.width(v2) < winWidth / 9.0) ? infos[i++] = v2 : infos[i++] = v1;

    v1 = "Client: " + (cku_ClientName.length() == 0 ? "nul" : cku_ClientName);
    v2 = "Client Name: " + (cku_ClientName.length() == 0 ? "<null>" : cku_ClientName);
    if (fm.width(v2) < winWidth / 9.0) infos[i++] = v2;
    else if (fm.width(v1) < winWidth * 2 / 9.0) infos[i++] = v1;

    if (isAndroid || winWidth <= 1200) {
        v1 = "Prog.: " + QString::number(totalKU - outstandingKU + 1) + "/" + QString::number(totalKU);
        v2 = "Progress: " + QString::number(totalKU - outstandingKU + 1) + "/" + QString::number(totalKU);
        (fm.width(v2) < winWidth * 1.9 / 10.0) ? infos[i++] = v2 : infos[i++] = v1;

        ui->progressBar_Learning->setVisible(false);
    } else {
        v1 = "Breakdown: ";
        for (int j = 0; j < availableCategory->count(); j++) {
            if (availableCategory->at(j)->number <= 0) continue;
            v1 += availableCategory->at(j)->name + ": " + QString::number(availableCategory->at(j)->number);
            v1 += ", ";
        }
        infos[i++] = v1.left(v1.length() - 2);
        ui->progressBar_Learning->setValue(static_cast<int>((totalKU - outstandingKU + 1) * 100.0 / (totalKU)));
        ui->progressBar_Learning->setFormat(QString::number(totalKU - outstandingKU + 1) + "/" + QString::number(totalKU));
        ui->progressBar_Learning->setVisible(true);
    }

    for (int i = 0; i < 20; i++) {
        if (infos[i].length() > 0)  t += infos[i];
        if (infos[i + 1].length() > 0 && i < 19)  t +=  "; ";
        else    break;
    }

    ui->lineEdit_PassingScore->setText(QString::number(cku_PassingScore));

    ui->textEdit_Info->setPlainText(t);



    adaptTexteditHeight(ui->textEdit_Question);
    adaptTexteditHeight(ui->textEdit_Info);
    adaptTexteditHeight(ui->textEdit_Draft);
    setWindowStyle();   // To be safe, setWindowStyle could be called both at the beginning and the end of this function.

    ui->textEdit_Draft->setFocus();

    handleTTS(true);

    this->setUpdatesEnabled(true);
    this->repaint();    // Without repaint, the whole window will not be painted at all.

    kuStartLearningTime = QDateTime::currentSecsSinceEpoch();

    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

int PracticeWindow::determineCategoryforNewKU()
{
    int r = qrand() % outstandingKU;
    int s = 0;

    for (int i = 0; i < availableCategory->count(); i++) {
        s += availableCategory->at(i)->number;
        if (r < s) {
            return i;
        }
    }
    return -1;
}

void PracticeWindow::loadNewKU(int depth)
{
    currCatIndex = determineCategoryforNewKU();
    QString stravailableCategory = "";
    for (int i = 0; i < availableCategory->length(); i++) {
       qDebug() << availableCategory->at(i)->name << ":\t" << availableCategory->at(i)->number;
    }
    if (mySQL.open()) {
        QSqlQuery *query = new QSqlQuery(mySQL);

        query->prepare("SELECT COUNT(*) FROM knowledge_units WHERE category = :category AND deadline <= DATETIME('now', 'localtime') AND is_shelved = 0");
        query->bindValue(":category", availableCategory->at(currCatIndex)->name);

        int dueNum = 0;
        if (query->exec() & query->next())
            dueNum = query->value(0).toInt();
        else {
            QMessageBox::warning(this, "Mamsds QJLT - Warning", "Cannot get dueNum from database!\n\nThis error is very rare, report it to mamsds IMMEDIATELY if you see it");
            return;
        }
        query->prepare("SELECT COUNT(*) FROM knowledge_units WHERE category = :category AND is_shelved = 0");
        query->bindValue(":category", availableCategory->at(currCatIndex)->name);
        query->exec();
        query->next();
        int TotalNum = query->value(0).toInt();

        double urgencyIndex = 1 - (qPow(static_cast<double>(TotalNum - dueNum) / TotalNum, 0.0275 * TotalNum + dueNum) * 0.65);
        qDebug() << "urgencyIndex = " << urgencyIndex;
        QString columns = "id, question, answer, passing_score, previous_score, times_practiced, insert_time, first_practice_time, last_practice_time, deadline, client_name, category, time_used";
        qDebug() << "[Random number compared to urgencyIndex] (double)qrand() / RAND_MAX: " << static_cast<double>(qrand()) / RAND_MAX;
        if (static_cast<double>(qrand()) / RAND_MAX < urgencyIndex) {
            query->prepare("SELECT " + columns + " FROM knowledge_units WHERE category = :category AND deadline <= DATETIME('now', 'localtime') AND is_shelved = 0 ORDER BY RANDOM() LIMIT 1");
            query->bindValue(":category", availableCategory->at(currCatIndex)->name);
        } else {
            if (qrand() % 100 <= NKI) {
                query->prepare("SELECT " + columns + " FROM knowledge_units WHERE category = :category AND times_practiced = 0 AND is_shelved = 0 ORDER BY RANDOM() LIMIT 1");
                query->bindValue(":category", availableCategory->at(currCatIndex)->name);
            } else {
                QString temp = "";
                temp += "SELECT *";
                temp += "FROM (SELECT " + columns;
                temp += "      FROM  knowledge_units";
                temp += "      WHERE category = :category";
                temp += "      ORDER BY (previous_score - passing_score) ASC";
                temp += "      LIMIT (SELECT ABS(RANDOM()) % (SELECT COUNT(id) + 1";
                temp += "                                     FROM knowledge_units";
                temp += "                                     WHERE category = :category AND is_shelved = 0)))";
                temp += "ORDER BY RANDOM() LIMIT 1";
                query->prepare(temp);
                query->bindValue(":category", availableCategory->at(currCatIndex)->name);
            }
        }

        if (query->exec() && query->first()) {
            int i = 0;
            cku_ID = query->value(i++).toInt();
            cku_Question = query->value(i++).toString();
            cku_Answer = query->value(i++).toString();
            cku_PassingScore = query->value(i++).toDouble();
            cku_PreviousScore = query->value(i++).toDouble();
            cku_TimesPracticed = query->value(i++).toInt();
            cku_InsertTime = query->value(i++).toDateTime();
            cku_FirstPracticeTime = query->value(i++).toDateTime();
            cku_LastPracticeTime = query->value(i++).toDateTime();
            cku_Deadline = query->value(i++).toDateTime();
            cku_ClientName = query->value(i++).toString();
            cku_Category = query->value(i++).toString();
            cku_SecSpent = query->value(i++).toInt();
            query->finish();
        } else if (depth < 100) {
            loadNewKU(++depth);
        } else
            QMessageBox::warning(this, "Mamsds QJLT - Warning", "Cannot load a new Knowledge Unit!\n\nThis error is very rare, report it to mamsds IMMEDIATELY if you see it");
        ui->textEdit_Question->setPlainText(cku_Question);
        adaptTexteditLineSpacing(ui->textEdit_Question);
        adaptTexteditLineSpacing(ui->textEdit_Draft);
    //    ui->plainTextEdit_Question->setPlainText(cku_Question);

        mySQL.close();

    }  else  {
        QMessageBox::warning(this, "Mamsds QJLT - Warning", "Cannot open the databse:\n" + mySQL.lastError().text() + "\n\nThis error is very rare, report to mamsds IMMEDIATELY if you see it.");
        QApplication::quit();
    }
}

void PracticeWindow::on_comboBox_Score_currentTextChanged(const QString &)
{

    ui->pushButton_Next->setEnabled(true);
    if (ui->comboBox_Score->currentText().length() > 0)
    {
        double t = ui->comboBox_Score->currentText().toDouble();

        double newScore = calculateNewPreviousScore(t);
        ui->Label_NewScore->setText(": " + QString::number(newScore, 'f', 1));

        if (newScore > cku_PreviousScore)
            ui->Label_NewScore->setStyleSheet("QLabel { color : green; }");
        else if (newScore < cku_PreviousScore)
            ui->Label_NewScore->setStyleSheet("QLabel { color : red; }");

    }
}

double PracticeWindow::calculateNewPreviousScore(double newScore)
{
    double timesPracticed =  cku_TimesPracticed <= 9 ? cku_TimesPracticed : 9;
    double previousScore = cku_PreviousScore;

    previousScore = (newScore + previousScore * timesPracticed) / (timesPracticed + 1);

    return previousScore;
}

void PracticeWindow::adaptTexteditLineSpacing(QTextEdit *textedit)
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

    QSettings settings("MamsdsStudio", "MamsdsQJointLearningTools");
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

void PracticeWindow::adaptTexteditHeight(QTextEdit *textedit)
{
    if (!textedit) return;

    while (!textedit->verticalScrollBar()->isVisible() && textedit->height() > 0)
        textedit->setFixedHeight(textedit->height() - 1);

    QRect geo = QApplication::desktop()->availableGeometry(this);

    double heightLimit = this->height() > QGuiApplication::primaryScreen()->availableGeometry().height() ? this->height() : QGuiApplication::primaryScreen()->availableGeometry().height();
    while (textedit->verticalScrollBar()->isVisible() &&
           textedit->height() <= heightLimit * 2 / 5.0 && this->height() <= geo.size().height())
        // this ratio cannot be set to a higher value otherwise the landscape mode on android would be not higher enough for the window.
        // The avoid the abovementioned situation, the landscape view is disabled altogether
        textedit->setFixedHeight(textedit->height() + 1);

    ui->progressBar_Learning->setFixedHeight(ui->textEdit_Info->height());

}

void PracticeWindow::adaptSkipButton()
{
    if (ui->horizontalSpacer->geometry().width() > ui->pushButton_Skip->width() || isAndroid == true)
        ui->pushButton_Skip->setVisible(true);
    else if (ui->pushButton_Skip->isVisible() && ui->horizontalSpacer->geometry().width() <= 0) {
        ui->pushButton_Skip->setVisible(false);
    }
}

void PracticeWindow::finishLearning()
{
    QString temp;
    for (int i = 0; i < availableCategory->count(); i++)
        temp += availableCategory->at(i)->snapshot->getComparison(QGuiApplication::platformName() != "android") + "\n";

    QMessageBox *msg = new QMessageBox(QMessageBox::Information, "QJLT - Result", temp, QMessageBox::Ok, this);
    msg->setFont(QFont("Noto Sans Mono CJK SC Regular", 1));
    msg->exec();

    trayIcon->hide();       // The Icon should be hide here since the program is quitted by the next line.
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

    //QMessageBox::information(this, "Result", temp);
    QApplication::quit();
}

void PracticeWindow::initContextMenu()
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
    if (mySQL.open()) {

        QSqlQuery *query = new QSqlQuery(mySQL);

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
        mySQL.close();
    } else {
        QMessageBox::warning(this, "Warning", "Cannot open the databse:\n" + mySQL.lastError().text());
        QApplication::quit();
    }

    ui->textEdit_Question->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->textEdit_Question, SIGNAL(customContextMenuRequested(const QPoint)), this, SLOT(showContextMenu_Question(const QPoint)));
    ui->textEdit_Answer->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->textEdit_Answer, SIGNAL(customContextMenuRequested(const QPoint)), this, SLOT(showContextMenu_Answer(const QPoint)));
    ui->textEdit_Draft->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->textEdit_Draft, SIGNAL(customContextMenuRequested(const QPoint)), this, SLOT(showContextMenu_Blank(const QPoint)));

}

void PracticeWindow::initTrayMenu()
{
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QIcon(":/Main.ico"));
    trayIcon->setToolTip("Mamsds QJoint Learning Tool");

    trayIcon->show();

    QMenu* menuTray = new QMenu(this);

    QAction* actionStartLearning = menuTray->addAction("Start Learning NOW!");
    connect(actionStartLearning, SIGNAL(triggered()), this, SLOT(on_actionStartLearning_triggered()));
    menuTray->addAction(actionStartLearning);

    QAction* actionResetTimer = menuTray->addAction("Reset Timer");
    connect(actionResetTimer, SIGNAL(triggered()), this, SLOT(on_actionResetTimer_triggered()));
    menuTray->addAction(actionResetTimer);

    QAction* actionBossMode = menuTray->addAction("Activate BM");
    connect(actionBossMode, SIGNAL(triggered()), this, SLOT(on_actionBossMode_triggered()));
    menuTray->addAction(actionBossMode);

    QAction* actionExit = menuTray->addAction("Exit");
    connect(actionExit, SIGNAL(triggered()), this, SLOT(on_actionExit_triggered()));
    menuTray->addAction(actionExit);

    trayIcon->setContextMenu(menuTray);
}

void PracticeWindow::showContextMenu_Question(const QPoint &pt)
{
    showContextMenu_Common(ui->textEdit_Question, menuQuestion, pt);
}

void PracticeWindow::showContextMenu_Answer(const QPoint &pt)
{
    showContextMenu_Common(ui->textEdit_Answer, menuAnswer, pt);
}

void PracticeWindow::showContextMenu_Blank(const QPoint &pt)
{
    showContextMenu_Common(ui->textEdit_Draft, menuBlank, pt);
}

void PracticeWindow::showContextMenu_Common(QTextEdit *edit, QMenu* menu, const QPoint &pt)
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

void PracticeWindow::setWindowStyle()
{
    if (!isVisible())       // This condition is needed since this function can accidentally show the window when it should be hidden.
        return;

 //   QCoreApplication::processEvents(QEventLoop::AllEvents, 50);

    QStyleOptionTitleBar so;
    so.titleBarFlags = Qt::Window;
    int titlebarHeight = this->style()->pixelMetric(QStyle::PM_TitleBarHeight, &so, this);

    if (windowStyle == 1000) {
        QWidget::showNormal();
        QRect geo = QApplication::desktop()->availableGeometry(this);
        this->setGeometry(0, titlebarHeight, geo.width() / 3, geo.height() - titlebarHeight);
    } else if (windowStyle == 0001) {
        QWidget::showNormal();
        QRect geo = QApplication::desktop()->availableGeometry(this);
        setGeometry(geo.width() / 3 * 2, titlebarHeight, geo.width() / 3 , geo.height() - titlebarHeight);
    } else {
        this->showMaximized();        

        if (QGuiApplication::primaryScreen()->size().width() < this->size().width()) {
            // This ugly hack is to ensure that the window is maximized correctly.
            // Sometimes, on Android platform, the width of the window would be much larger then the exact screen.
            QRect geo = QApplication::desktop()->availableGeometry(this);
            geo.setWidth(QGuiApplication::primaryScreen()->size().width());
            this->setGeometry(geo);
        }
    }

    adaptSkipButton();

    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);        // To make sure the resize is implemented before next step.
}

QString PracticeWindow::convertStringToFilename(QString name)
{
    QString output(name.normalized(QString::NormalizationForm_D));
    output.replace(QRegExp("[^a-zA-Z,'.-() \\s]"), "_");
    if (output.length() > 100)
        output = output.mid(0, 45 + (output.length() % 7)) + "..." + output.mid(output.length() - 51 + (output.length() % 3));
    return output;
}

void PracticeWindow::handleTTS(bool isQuestion)
{
    QString sanitizedFilepath, originalText, sanitizedFilename;
    TTSDownloader* ttsDownloader = new TTSDownloader();
    bool isTTSEnabled = false;

    if (isQuestion) {
        originalText = cku_Question;
        sanitizedFilename = convertStringToFilename(originalText);
        for (int i = 0; i < availableCategory->count(); i++) {
            if (availableCategory->at(i)->name == cku_Category && availableCategory->at(i)->ttsOption == 1) {
                isTTSEnabled = true;
                break;
            }
        }
    } else {
        originalText = ui->textEdit_Answer->document()->findBlockByLineNumber(0).text();
        sanitizedFilename = convertStringToFilename(originalText);
        for (int i = 0; i < availableCategory->count(); i++) {
            if (availableCategory->at(i)->name == cku_Category && availableCategory->at(i)->ttsOption == 2) {
                isTTSEnabled = true;
                break;
            }
        }
    }
    sanitizedFilepath = parentDir + "/speeches/" + sanitizedFilename + ".mp3";

    if (isTTSEnabled) {
        QFileInfo check_file(sanitizedFilepath);
        if (check_file.exists() && check_file.isFile()) {
            player->setMedia(QUrl::fromLocalFile(sanitizedFilepath));
            player->play();
        } else {
            ttsDownloader->doDownload("http://dict.youdao.com/dictvoice?audio=" + originalText + "&amp;amp;le=eng%3C", sanitizedFilepath);
            // http, instead of https, is used here.
            // If https is used, the program would encounter a "TLS initialization failed" error on Windows. Not sure what would happen on Linux
        }
    }

//    ttsDownloader->deleteLater(); Since ttsDownloader works in an asynchronous manner, the object cannot be simply deleted here.
}

void PracticeWindow::on_actionResetTimer_triggered()
{
    if (bossMode) {
        return;
    }
    else{
        secDelayed = 0;
    }
}

void PracticeWindow::on_actionBossMode_triggered()
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


void PracticeWindow::on_actionStartLearning_triggered()
{
    if (bossMode) {
        return;
    }
    else{
        secDelayed = interval * 60 - 5;
    }
}

void PracticeWindow::on_actionExit_triggered()
{
    if (bossMode) {
        return;
    } else {
        finishLearning();
        trayIcon->hide();       // This line alone won't work since finishLearning() would  quit the program before this line is executed
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        QApplication::quit();
    }
}

void PracticeWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    adaptTexteditHeight(ui->textEdit_Question);
    adaptTexteditHeight(ui->textEdit_Draft);
    adaptTexteditHeight(ui->textEdit_Info);

    adaptSkipButton();
}

void PracticeWindow::keyPressEvent(QKeyEvent *event)
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

void PracticeWindow::on_pushButton_Skip_clicked()
{
    initNextKU();
}

void PracticeWindow::on_textEdit_Question_textChanged()
{
    adaptTexteditHeight(ui->textEdit_Question);
    setWindowStyle();
}

void PracticeWindow::on_textEdit_Draft_textChanged()
{
  //  return;
   // if (isAndroid && ui->textEdit_Draft->isVisible())
    //    return;
    adaptTexteditHeight(ui->textEdit_Draft);
    setWindowStyle();
}

void PracticeWindow::on_textEdit_Info_textChanged()
{
    adaptTexteditHeight(ui->textEdit_Info);
    setWindowStyle();
}

void PracticeWindow::on_pushButton_Skip_pressed()
{
    PracticeWindow::on_pushButton_Skip_clicked();
}

void PracticeWindow::on_pushButton_Check_pressed()
{
    PracticeWindow::on_pushButton_Check_clicked();
}

void PracticeWindow::on_pushButton_Next_pressed()
{
    PracticeWindow::on_pushButton_Next_clicked();
}

// Return value: If user would like to retry the same database operation
bool PracticeWindow::handleDatabaseOperationError(QString operationName, QString dbPath, QString lastError)
{
    QMessageBox::StandardButton resBtn = QMessageBox::warning(this, "Mamsds QJLT - Database Operation Error", "Operation name: " + operationName + "\n"
                                                              + "Database path: " + dbPath + "\n"
                                                              + "Error message: " + lastError + "\n\n"
                                                              + "Retry the operation?"
                                                              , QMessageBox::No | QMessageBox::Yes, QMessageBox::Yes);
    if (resBtn == QMessageBox::Yes)
        return true;
    else
        return false;
}

void PracticeWindow::on_pushButton_Switch_pressed()
{
    if (ui->textEdit_DraftAndroid->isVisible()) {
        ui->textEdit_Draft->setVisible(false);
        ui->textEdit_DraftAndroid->setVisible(false);
        ui->textEdit_Answer->setVisible(true);
        ui->pushButton_Switch->setText("Draft");
    } else {
        ui->textEdit_Draft->setVisible(false);
        ui->textEdit_DraftAndroid->setVisible(true);
        ui->textEdit_Answer->setVisible(false);
        ui->pushButton_Switch->setText("Answer");
    }
}
