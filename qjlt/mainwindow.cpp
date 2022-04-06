#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "practicewindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    initPlatformSpecificSettings();
    if (!initCategoryStructure()) return;
      //To make sure that the program is exited if database cannot be opened
    initUI();
    initStatistics();
    initSettings();

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->modifiers()&Qt::ControlModifier && (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return))
        on_pushButton_Start_clicked();
}

void MainWindow::closeEvent (QCloseEvent *event)
{
    QMessageBox::StandardButton resBtn = QMessageBox::question( this, "Mamsds QJLT",
                                                                tr("Are you sure to quit?\n"),
                                                                QMessageBox::No | QMessageBox::Yes,
                                                                QMessageBox::Yes);
    if (resBtn != QMessageBox::Yes) {
            event->ignore();
    } else {
        event->accept();
        QApplication::quit();
    }
}

bool MainWindow::initCategoryStructure()
{
    if (mySQL.open()) {
        QSqlQuery *query = new QSqlQuery(mySQL);
        query->prepare("SELECT DISTINCT(category) FROM knowledge_units WHERE is_shelved = 0 ORDER BY category DESC");
        if (!query->exec()) {
            QMessageBox::critical(this, "Mamsds QJLT - Fatal Error", "Cannot read the databse [" + mySQL.databaseName()
                                  + "]. The database could be locked, empty or corrupt.\nInteral error info:\n" + query->lastError().text());
            return false;
        }
        allCats = new QList<CategoryMetaData*>;
        QSqlQuery *query1 = new QSqlQuery(mySQL);
        while (query->next()) {
            CategoryMetaData *t = new CategoryMetaData();
            t->name = query->value(0).toString();
            t->number = 0;

            query1->prepare("SELECT tts_option FROM category_info WHERE name = :name");
            query1->bindValue(":name", t->name);
            query1->exec();

            if (query1->next()) {
                if (query1->value(0).toString() == "question")
                    t->ttsOption = 1;
                else if (query1->value(0).toString() == "answer")
                     t->ttsOption = 2;
                else
                     t->ttsOption = 0;
            } else {
                t->ttsOption = 0;
            }

            allCats->append(t);
        }
        for (int i = 0; i < allCats->count(); i++)
            ui->lineEdit_NumberstoLearn->setPlaceholderText(ui->lineEdit_NumberstoLearn->placeholderText() + allCats->at(i)->name + (i < allCats->count() - 1 ? ", " : ""));
        return true;
    } else {
        QMessageBox::critical(this, "Mamsds QJLT - Fatal Error", "Cannot open the databse file [" + mySQL.databaseName() + "]\nInternal error message:\n"
                              + mySQL.lastError().text());
        QApplication::quit();       // This command is in fact  useless.
        ui->pushButton_Start->setEnabled(false);
        return false;
    }
}

bool MainWindow::initUI()
{
    if (mySQL.open()) {
        QSqlQueryModel *model = new QSqlQueryModel();
        QSqlQuery *query = new QSqlQuery(mySQL);

        if (QGuiApplication::primaryScreen()->geometry().width() >= 1080)
            query->prepare(QString("SELECT id, SUBSTR(REPLACE(REPLACE(question, CHAR(10), ''), CHAR(9), ''), 0, 35) || '...' AS 'Question', SUBSTR(REPLACE(REPLACE(answer, CHAR(10), ''), CHAR(9), ''), 0, 40) || '...' AS 'Answer', ")
                       + QString("ROUND(previous_score, 1) AS 'Score' , STRFTIME('%Y-%m-%d', insert_time) AS 'Insert', STRFTIME('%Y-%m-%d', first_practice_time) AS 'First Practice', ")
                       + QString("last_practice_time AS 'Last Practice', STRFTIME('%Y-%m-%d', deadline) AS 'Deadline' FROM knowledge_units WHERE is_shelved = 0 ORDER BY last_practice_time DESC"));
        else {
            query->prepare(QString("SELECT SUBSTR(REPLACE(REPLACE(question, CHAR(10), ''), CHAR(9), ''), 0, 35) || '...' AS 'Question', ")
                       + QString("STRFTIME('%m-%d %H:%M', last_practice_time) AS 'Last Practice' FROM knowledge_units WHERE is_shelved = 0 ORDER BY last_practice_time DESC"));
        }
        query->exec();

        model->setQuery(*query);
        ui->tableView_KU->setModel(model);
        ui->tableView_KU->setSortingEnabled(true);
        ui->tableView_KU->horizontalHeader()->setSectionsClickable(1);

        ui->tableView_KU->setSelectionBehavior(QAbstractItemView::SelectRows);

        ui->tableView_KU->resizeColumnsToContents();
        ui->tableView_KU->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

        ui->tableView_KU->show();
//        QFont font = QFont("Noto Sans CJK SC Medium", 9);
 /*       if (QGuiApplication::platformName() == "windows")
        {
         //   QFont font = QFont("Microsoft Yahei", 9);
         //   font.setStyleStrategy(QFont::PreferAntialias);
         //   ui->tableView_KU->setFont(font);    // Don't know why it is special. Just needed...
        }*/
        /* Enable the sort function */
        QSortFilterProxyModel *m=new QSortFilterProxyModel(this);
        m->setDynamicSortFilter(true);
        m->setSourceModel(model);
        ui->tableView_KU->setModel(m);
        ui->tableView_KU->setSortingEnabled(true);

    } else {
        QMessageBox::critical(this, "Mamsds QJLT - Fatal Error", "Cannot open the databse [" + mySQL.databaseName() + "].\n\nInternal error message:\n" + mySQL.lastError().text());
        return false;
    }

    ui->tableView_KU->grabGesture(Qt::SwipeGesture);

    ui->lineEdit_NumberstoLearn->setFocus();
    return true;
}

void MainWindow::initSettings()
{

    QSettings settings("MamsdsStudio", "MamsdsQJointLearningTools");

    ui->lineEdit_FontSize->setValidator( new QIntValidator(0, 50, this));
    ui->lineEdit_NKI->setValidator( new QIntValidator(0, 99, this));
    ui->lineEdit_ClientName->setText(settings.value("ClientName", "QJLT-Notset").toString());
    ui->lineEdit_FontSize->setText(QString::number(settings.value("FontSize", 10).toInt()));
    ui->lineEdit_NKI->setText(QString::number(settings.value("NKI", 50).toInt()));
    ui->lineEdit_IntervalNum->setText(settings.value("Interval", "0, 0").toString());
    ui->lineEdit_WindowStyle->setText(settings.value("WindowStyle", "1111").toString());
}

void MainWindow::initStatistics()
{
    QMap<QString, int>::iterator i;
    for (int i = 0; i < allCats->count(); i++) {
        allCats->at(i)->snapshot = new Snapshot(mySQL, allCats->at(i)->name);
        ui->plainTextEdit_Statistics->appendPlainText(allCats->at(i)->snapshot->getSnapshot());
    }

    if (QGuiApplication::platformName() == "windows" || QGuiApplication::platformName() == "xcb") {
        // XCB is the X11 plugin used on regular desktop Linux platforms.
        ui->plainTextEdit_Statistics->setFont(QFont("Noto Sans Mono CJK SC Regular"));
    } else if (QGuiApplication::platformName() == "android") {
        ui->plainTextEdit_Statistics->setFont(QFont("Droid Sans Mono"));
    }
}

void MainWindow::on_pushButton_Start_clicked()
{
    ui->pushButton_Start->setEnabled(false); // To avoid this event from being triggered twice (which would initiate two practice windows)

    for (int i = 0; i < allCats->count(); i++)
        qDebug() << allCats->at(i)->name + ": " + QString::number(allCats->at(i)->ttsOption);
    QRegExp rx("(\\,)"); //RegEx for ' ' or ',' or '.' or ':' or '\t'
    QList<QString> number = ui->lineEdit_NumberstoLearn->text().split(rx);

    int t = 0;
    for (int i = 0; i < number.count() && i < allCats->count(); i++) {
        allCats->at(i)->number = number.at(i).toInt();
        t += allCats->at(i)->number;
    }

    if (t == 0) {
        QMessageBox::warning(this, "Warning", "Category input is invalid, it should be in the format of:\nA [, B, C, D ...]");
        ui->pushButton_Start->setEnabled(true);
        return;
    }

    PracticeWindow *pW = new PracticeWindow(nullptr, mySQL);

    QList<QString> tt = ui->lineEdit_IntervalNum->text().split(rx);
    int t1 = 0, t2 = 0;
    if (tt.count() == 2) {
        t1 = tt.at(0).toInt();
        t2 = tt.at(1).toInt();
    }

    QSettings settings("MamsdsStudio", "MamsdsQJointLearningTools");

    pW->init(allCats, settings.value("NKI", 50).toInt(), t1, t2, settings.value("WindowStyle", "1111").toInt());
 //   pW->showMaximized();
    pW->show();
    pW->initNextKU();

    this->hide();
    ui->pushButton_Start->setEnabled(false);
}

void MainWindow::on_lineEdit_WindowStyle_textChanged(const QString &)
{
    QSettings settings("MamsdsStudio", "MamsdsQJointLearningTools");
    if (ui->lineEdit_WindowStyle->text() == "1000" || ui->lineEdit_WindowStyle->text() == "0001" || ui->lineEdit_WindowStyle->text() == "1111") {
        settings.setValue("WindowStyle", ui->lineEdit_WindowStyle->text());
        initSettings();
    }
}

void MainWindow::on_lineEdit_NKI_textChanged(const QString &)
{
    QSettings settings("MamsdsStudio", "MamsdsQJointLearningTools");
    settings.setValue("NKI", ui->lineEdit_NKI->text());
    initSettings();
}

void MainWindow::on_lineEdit_FontSize_textChanged(const QString &)
{
    QSettings settings("MamsdsStudio", "MamsdsQJointLearningTools");
    settings.setValue("FontSize", ui->lineEdit_FontSize->text());
    initSettings();
}

void MainWindow::on_pushButton_Quit_clicked()
{
    QApplication::quit();
}

void MainWindow::on_lineEdit_IntervalNum_textChanged(const QString &)
{
    QSettings settings("MamsdsStudio", "MamsdsQJointLearningTools");

    QRegExp rx("(\\,)"); //RegEx for ' ' or ',' or '.' or ':' or '\t'
    QList<QString> number = ui->lineEdit_IntervalNum->text().split(rx);
    if (number.count() == 2) {
        settings.setValue("Interval", QString::number(number.at(0).toInt()) + ", " + QString::number(number.at(1).toInt()));
   //     initSettings();
    }
}

void MainWindow::initPlatformSpecificSettings()
{    
    if (QGuiApplication::platformName() == "android") {
        mySQL.setDatabaseName("/sdcard/jlt/db/database.sqlite");

        ui->label_NKI->setVisible(false);
        ui->lineEdit_NKI->setVisible(false);
        ui->label_Fontsize->setVisible(false);
        ui->lineEdit_FontSize->setVisible(false);
        ui->label_IntervalNum->setVisible(false);
        ui->lineEdit_IntervalNum->setVisible(false);
        ui->label_WindowStyle->setVisible(false);
        ui->lineEdit_WindowStyle->setVisible(false);

        ui->groupBox_DBContent->setVisible(false);

        QSettings settings("MamsdsStudio", "MamsdsQJointLearningTools");
   //     settings.setValue("ClientType", "QJLT-Android");
        settings.setValue("FontSize", 12);
        settings.setValue("Interval", "0,0");
        settings.setValue("NKI", 10);
        settings.setValue("WindowStyle", 1111);
        settings.sync();

    } else {
        QDir tmpDir = QApplication::applicationFilePath();
        tmpDir.cdUp();


  /*      if (QGuiApplication::platformName() == "windows")
        {
            // It is conformed that:
            // As at 2019-03-29, wenquanyi series don't work well on Windows.
            QFontDatabase::addApplicationFont(tmpDir.path() +  "/NotoSansMonoCJKsc-Regular.otf");
            QFontDatabase::addApplicationFont(tmpDir.path() + "/NotoSansCJKsc-Medium.otf");

            QSettings settings("MamsdsStudio", "MamsdsQJointLearningTools");

            QFont font = QFont();
            font.setFamily("Noto Sans Mono CJK SC Regular");
            font.setPixelSize(15);
     //       font.setStyleHint(QFont::Monospace);
         //   font.setLetterSpacing(QFont::PercentageSpacing, 102);
            QApplication::setFont(font);
    //        QString styleSheet = QString("font-size:%1px;").arg(font.pixelSize());
      //      this->setStyleSheet(styleSheet);

        }*/

        tmpDir.cdUp();
        mySQL.setDatabaseName(tmpDir.path() + "/db/database.sqlite");
    }
}


void MainWindow::on_lineEdit_ClientName_textChanged()
{
    QSettings settings("MamsdsStudio", "MamsdsQJointLearningTools");
    settings.setValue("ClientName", ui->lineEdit_ClientName->text());
    initSettings();
}

void MainWindow::on_pushButton_Start_pressed()
{
    MainWindow::on_pushButton_Start_clicked();
}

void MainWindow::on_pushButton_Quit_pressed()
{
    MainWindow::on_pushButton_Quit_clicked();
}
