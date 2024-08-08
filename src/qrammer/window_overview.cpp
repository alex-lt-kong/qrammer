#include "window_overview.h"
#include "db.h"
#include "global_variables.h"
#include "src/qrammer/ui_window_overview.h"
#include "window_cramming.h"
#include "window_manage_db.h"

#include <QBarCategoryAxis>
#include <QBarSeries>
#include <QChart>
#include <QChartView>
#include <QDir>
#include <QRegularExpression>
#include <QValueAxis>
#include <spdlog/spdlog.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , winDb(new WindowManageDB(this))
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::init()
{
    QSqlQuery q;
    try {
        availableCategory = db.getAllCategories();
        q = db.getQueryForKuTableView(QGuiApplication::primaryScreen()->geometry().width() >= 1080);
    } catch (const std::runtime_error &e) {
        auto errMsg = QString("db.getAllCategories() failed: %1").arg(e.what());
        SPDLOG_ERROR(errMsg.toStdString());
        QMessageBox::critical(this, "Qrammer - Fatal Error", errMsg);
        return false;
    }
    initUi(q);
    return true;
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    // TODO: for some reasons this does not work yet...
    if (event->modifiers() & Qt::ControlModifier
        && (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return))
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

void MainWindow::initUi_Overview(QSqlQuery &query)
{
    QSqlQueryModel *model = new QSqlQueryModel(this);
    model->setQuery(std::move(query));
    ui->tableView_KU->setModel(model);
    ui->tableView_KU->setSortingEnabled(true);
    ui->tableView_KU->horizontalHeader()->setSectionsClickable(1);
    ui->tableView_KU->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView_KU->resizeColumnsToContents();
    ui->tableView_KU->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

    ui->tableView_KU->show();

    QSortFilterProxyModel *m = new QSortFilterProxyModel(this);
    m->setDynamicSortFilter(true);
    m->setSourceModel(model);
    ui->tableView_KU->setModel(m);
    ui->tableView_KU->setSortingEnabled(true);

    ui->tableView_KU->grabGesture(Qt::SwipeGesture);
}

void MainWindow::initUi_PrograssBarChart()
{
    // TODO: does it leak memory?
    QBarSeries *series = new QBarSeries(this);
    for (auto const &cat : availableCategory) {
        series->append(cat.set.get());
    }
    auto chart = new QChart();
    chart->addSeries(series);
    // chart->setTitle("Simple Bar Chart");
    chart->setAnimationOptions(QChart::SeriesAnimations);
    QStringList dates;
    for (int i = progressLookBackDays; i > 0; --i) {
        dates.append(QString::number(i * -1));
    }
    auto axisX = new QBarCategoryAxis;
    axisX->append(dates);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    auto axisY = new QValueAxis;
    axisY->setRange(0, 15);
    chart->addAxis(axisY, Qt::AlignRight);
    series->attachAxis(axisY);
    auto cv = new QChartView(chart, this);
    cv->setRenderHint(QPainter::Antialiasing);

    ui->gridLayout_PrograssBarChat->addWidget(cv);
}

void MainWindow::initUi_CrammingSchedule()
{
    for (size_t i = 0; i < availableCategory.size(); i++) {
        ui->lineEdit_KusToCramByCategory->setPlaceholderText(
            ui->lineEdit_KusToCramByCategory->placeholderText() + availableCategory[i].name
            + (i < availableCategory.size() - 1 ? ", " : ""));
    }

    ui->lineEdit_KusToCramByCategory->setFocus();
}

void MainWindow::initUi_Stats()
{
    for (size_t i = 0; i < availableCategory.size(); i++) {
        ui->plainTextEdit_Statistics->appendPlainText(
            availableCategory[i].snapshot.getSnapshotString());
    }
    QFont font("Monospace");
    font.setStyleHint(QFont::TypeWriter);
    ui->plainTextEdit_Statistics->setFont(font);
}

void MainWindow::initUi(QSqlQuery &query)
{
    setWindowIcon(QIcon(":/qrammer.ico"));
    setWindowTitle(QString("%1 (git commit: %2)").arg(windowTitle(), GIT_COMMIT_HASH));
    initUi_Stats();
    initUi_Overview(query);
    initUi_Settings();
    initUi_PrograssBarChart();
    initUi_CrammingSchedule();
}

void MainWindow::initUi_Settings()
{
    ui->lineEdit_FontSize->setValidator(new QIntValidator(0, 50, this));
    ui->lineEdit_NewKUCoeff->setValidator(new QIntValidator(0, 99, this));
    ui->lineEdit_ClientName->setText(settings.value("ClientName", "Qrammer-Notset").toString());
    ui->lineEdit_FontSize->setText(QString::number(settings.value("FontSize", 10).toInt()));
    ui->lineEdit_NewKUCoeff->setText(QString::number(settings.value("NewKUCoeff", 50).toInt()));
    ui->lineEdit_IntervalNum->setText(settings.value("Interval", "0, 0").toString());
    ui->lineEdit_WindowStyle->setText(settings.value("WindowStyle", "1111").toString());
}

void MainWindow::on_pushButton_Start_clicked()
{
    SPDLOG_INFO("Cramming session about to start");
    ui->pushButton_Start->setEnabled(false); // To avoid this event from being triggered twice (which would initiate two practice windows)
    static QRegularExpression rx("(\\,)"); //RegEx for ' ' or ',' or '.' or ':' or '\t'
    QList<QString> number = ui->lineEdit_KusToCramByCategory->text().split(rx);

    size_t t = 0;
    for (size_t i = 0; i < number.count() && i < availableCategory.size(); i++) {
        availableCategory[i].KuToCramCount = number.at(i).toInt();
        t += availableCategory[i].KuToCramCount;
    }

    if (t == 0) {
        QMessageBox::warning(this, "Warning", "Category input is invalid, it should be in the format of:\nA [, B, C, D ...]");
        ui->pushButton_Start->setEnabled(true);
        return;
    }

    CrammingWindow *cW = new CrammingWindow(nullptr);

    QList<QString> tt = ui->lineEdit_IntervalNum->text().split(rx);
    int t1 = 0, t2 = 0;
    if (tt.count() == 2) {
        t1 = tt[0].toInt();
        t2 = tt[1].toInt();
    }

    ui->pushButton_Start->setEnabled(false);
    cW->init(settings.value("NewKUCoeff", 50).toUInt(),
             t1,
             t2,
             settings.value("WindowStyle", "1111").toInt());
    cW->show();
    cW->initNextKU();
    this->hide();
}

void MainWindow::on_lineEdit_WindowStyle_textChanged(const QString &)
{
    if (ui->lineEdit_WindowStyle->text() == "1100" || ui->lineEdit_WindowStyle->text() == "1000"
        || ui->lineEdit_WindowStyle->text() == "0001" || ui->lineEdit_WindowStyle->text() == "0011"
        || ui->lineEdit_WindowStyle->text() == "1111") {
        settings.setValue("WindowStyle", ui->lineEdit_WindowStyle->text());
        initUi_Settings();
    }
}

void MainWindow::on_lineEdit_FontSize_textChanged(const QString &)
{
    settings.setValue("FontSize", ui->lineEdit_FontSize->text());
    initUi_Settings();
}

void MainWindow::on_pushButton_Quit_clicked()
{
    QApplication::quit();
}

void MainWindow::on_lineEdit_IntervalNum_textChanged(const QString &)
{
    static QRegularExpression rx("(\\,)"); //RegEx for ' ' or ',' or '.' or ':' or '\t'
    QList<QString> number = ui->lineEdit_IntervalNum->text().split(rx);
    if (number.count() == 2) {
        settings.setValue("Interval", QString::number(number.at(0).toInt()) + ", " + QString::number(number.at(1).toInt()));
    }
}

void MainWindow::on_lineEdit_ClientName_textChanged()
{
    settings.setValue("ClientName", ui->lineEdit_ClientName->text());
    initUi_Settings();
}

void MainWindow::on_pushButton_Start_pressed()
{
    MainWindow::on_pushButton_Start_clicked();
}

void MainWindow::on_lineEdit_NewKUCoeff_textChanged(const QString &arg1)
{
    (void)arg1;
    settings.setValue("NewKUCoeff", ui->lineEdit_NewKUCoeff->text());
    initUi_Settings();
}

void MainWindow::on_pushButto_Manage_clicked()
{
    winDb->show();
}
