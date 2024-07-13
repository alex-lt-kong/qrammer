#include "window_overview.h"
#include "global_variables.h"
#include "src/qrammer/ui_window_overview.h"
#include "window_cram.h"

#include <QRegularExpression>
#include <spdlog/spdlog.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    initPlatformSpecificSettings();
    if (!initCategoryStructure()) return;
     //To make sure that the program quits if database cannot be opened
    if (!initUI())
        return;
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
    QMessageBox::StandardButton resBtn = QMessageBox::question(this,
                                                               "Qrammer",
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
    auto db = QSqlDatabase::addDatabase(DATABASE_DRIVER);
    db.setDatabaseName(databaseName);
    if (!db.open()) {
        ui->pushButton_Start->setEnabled(false);
        auto errMsg = "Cannot open the databse file [" + db.databaseName()
                      + "]\nInternal error message:\n" + db.lastError().text();
        SPDLOG_ERROR(errMsg.toStdString());
        QMessageBox::critical(this, "Qrammer - Fatal Error", errMsg);
        // QApplication::quit();
        return false;
    }

    QSqlQuery query = QSqlQuery(db);
    query.prepare(
        R"(
SELECT DISTINCT(category)
FROM knowledge_units
WHERE is_shelved = 0
ORDER BY category DESC
)");
    if (!query.exec()) {
        QMessageBox::critical(
            this,
            "Qrammer - Fatal Error",
            "Cannot read the databse [" + db.databaseName()
                + "]. The database could be locked, empty or corrupt.\nInteral error info:\n"
                + query.lastError().text());
        return false;
    }
        allCats = new QList<CategoryMetaData*>;
        QSqlQuery query1 = QSqlQuery(db);
        while (query.next()) {
            CategoryMetaData *t = new CategoryMetaData();
            t->name = query.value(0).toString();
            t->number = 0;

            query1.prepare("SELECT tts_option FROM category_info WHERE name = :name");
            query1.bindValue(":name", t->name);
            query1.exec();

            if (query1.next()) {
                if (query1.value(0).toString() == "question")
                    t->ttsOption = 1;
                else if (query1.value(0).toString() == "answer")
                    t->ttsOption = 2;
                else
                     t->ttsOption = 0;
            } else {
                t->ttsOption = 0;
            }

            allCats->append(t);
        }
        for (int i = 0; i < allCats->count(); i++) {
            ui->lineEdit_NumberstoLearn->setPlaceholderText(
                        ui->lineEdit_NumberstoLearn->placeholderText() + allCats->at(i)->name + (i < allCats->count() - 1 ? ", " : "")
            );
        }
        return true;
}

bool MainWindow::initUI()
{
    auto db = QSqlDatabase::addDatabase(DATABASE_DRIVER);
    db.setDatabaseName(databaseName);
    if (!db.open()) {
        QMessageBox::critical(this,
                              "Mamsds Qrammer -Fatal Error",
                              "Cannot open the databse [" + db.databaseName()
                                  + "].\n\nInternal error message:\n" + db.lastError().text());
        return false;
    }

    QSqlQueryModel *model = new QSqlQueryModel();
    QSqlQuery *query = new QSqlQuery(db);

    if (QGuiApplication::primaryScreen()->geometry().width() >= 1080) {
        auto stmt = R"***(
SELECT
    id,
    SUBSTR(REPLACE(REPLACE(question, CHAR(10), ''), CHAR(9), ''), 0, 35) || '...' AS 'Question', SUBSTR(REPLACE(REPLACE(answer, CHAR(10), ''), CHAR(9), ''), 0, 40) || '...' AS 'Answer',
    ROUND(previous_score, 1) AS 'Score' ,
    STRFTIME('%Y-%m-%d', insert_time) AS 'Insert',
    STRFTIME('%Y-%m-%d', first_practice_time) AS 'First Practice',
    last_practice_time AS 'Last Practice',
    STRFTIME('%Y-%m-%d', deadline) AS 'Deadline'
FROM knowledge_units
WHERE is_shelved = 0
ORDER BY last_practice_time DESC
)***";
        query->prepare(stmt);
    } else {
        auto stmt = R"***(
SELECT
    SUBSTR(REPLACE(REPLACE(question, CHAR(10), ''), CHAR(9), ''), 0, 35) || '...' AS 'Question',
    STRFTIME('%m-%d %H:%M', last_practice_time) AS 'Last Practice'
FROM knowledge_units
WHERE is_shelved = 0
ORDER BY last_practice_time DESC
)***";
        query->prepare(stmt);
    }
        query->exec();

        model->setQuery(std::move(*query));
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

        ui->tableView_KU->grabGesture(Qt::SwipeGesture);

        ui->lineEdit_NumberstoLearn->setFocus();
        return true;
}

void MainWindow::initSettings()
{
    QSettings settings("AKStudio", "Qrammer");

    ui->lineEdit_FontSize->setValidator(new QIntValidator(0, 50, this));
    ui->lineEdit_NKI->setValidator(new QIntValidator(0, 99, this));
    ui->lineEdit_ClientName->setText(settings.value("ClientName", "Qrammer-Notset").toString());
    ui->lineEdit_FontSize->setText(QString::number(settings.value("FontSize", 10).toInt()));
    ui->lineEdit_NKI->setText(QString::number(settings.value("NKI", 50).toInt()));
    ui->lineEdit_IntervalNum->setText(settings.value("Interval", "0, 0").toString());
    ui->lineEdit_WindowStyle->setText(settings.value("WindowStyle", "1111").toString());
}

void MainWindow::initStatistics()
{
    QMap<QString, int>::iterator i;
    for (int i = 0; i < allCats->count(); i++) {
        allCats->at(i)->snapshot = new Snapshot(allCats->at(i)->name);
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
    //static QRegularExpression re("SEARCHING...", QRegularExpression::CaseInsensitiveOption);
    static QRegularExpression rx("(\\,)"); //RegEx for ' ' or ',' or '.' or ':' or '\t'
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

    CrammingWindow *pW = new CrammingWindow(nullptr);

    QList<QString> tt = ui->lineEdit_IntervalNum->text().split(rx);
    int t1 = 0, t2 = 0;
    if (tt.count() == 2) {
        t1 = tt.at(0).toInt();
        t2 = tt.at(1).toInt();
    }

    QSettings settings("AKStudio", "Qrammer");

    pW->init(allCats, settings.value("NKI", 50).toInt(), t1, t2, settings.value("WindowStyle", "1111").toInt());
    pW->show();
    pW->initNextKU();

    this->hide();
    ui->pushButton_Start->setEnabled(false);
}

void MainWindow::on_lineEdit_WindowStyle_textChanged(const QString &)
{
    QSettings settings("AKStudio", "Qrammer");
    if (ui->lineEdit_WindowStyle->text() == "1000" || ui->lineEdit_WindowStyle->text() == "0001"
        || ui->lineEdit_WindowStyle->text() == "1111") {
        settings.setValue("WindowStyle", ui->lineEdit_WindowStyle->text());
        initSettings();
    }
}

void MainWindow::on_lineEdit_NKI_textChanged(const QString &)
{
    QSettings settings("AKStudio", "Qrammer");
    settings.setValue("NKI", ui->lineEdit_NKI->text());
    initSettings();
}

void MainWindow::on_lineEdit_FontSize_textChanged(const QString &)
{
    QSettings settings("AKStudio", "Qrammer");
    settings.setValue("FontSize", ui->lineEdit_FontSize->text());
    initSettings();
}

void MainWindow::on_pushButton_Quit_clicked()
{
    QApplication::quit();
}

void MainWindow::on_lineEdit_IntervalNum_textChanged(const QString &)
{
    QSettings settings("AKStudio", "Qrammer");

    static QRegularExpression rx("(\\,)"); //RegEx for ' ' or ',' or '.' or ':' or '\t'
    QList<QString> number = ui->lineEdit_IntervalNum->text().split(rx);
    if (number.count() == 2) {
        settings.setValue("Interval", QString::number(number.at(0).toInt()) + ", " + QString::number(number.at(1).toInt()));
   //     initSettings();
    }
}

void MainWindow::initPlatformSpecificSettings()
{    
    if (QGuiApplication::platformName() == "android") {
        databaseName = "/sdcard/qrammer/db/database.sqlite";

        ui->label_NKI->setVisible(false);
        ui->lineEdit_NKI->setVisible(false);
        ui->label_Fontsize->setVisible(false);
        ui->lineEdit_FontSize->setVisible(false);
        ui->label_IntervalNum->setVisible(false);
        ui->lineEdit_IntervalNum->setVisible(false);
        ui->label_WindowStyle->setVisible(false);
        ui->lineEdit_WindowStyle->setVisible(false);

        ui->groupBox_DBContent->setVisible(false);

        QSettings settings("AKStudio", "Qrammer");
        //     settings.setValue("ClientType", "Qrammer-Android");
        settings.setValue("FontSize", 12);
        settings.setValue("Interval", "0,0");
        settings.setValue("NKI", 10);
        settings.setValue("WindowStyle", 1111);
        settings.sync();

    } else {
        QDir tmpDir = QApplication::applicationFilePath();
        tmpDir.cdUp();
        tmpDir.cdUp();
        databaseName = tmpDir.path() + "/db/database.sqlite";
    }
}


void MainWindow::on_lineEdit_ClientName_textChanged()
{
    QSettings settings("AKStudio", "Qrammer");
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
