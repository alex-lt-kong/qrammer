#ifndef SNAPSHOT_H
#define SNAPSHOT_H

#include <QMainWindow>
#include <QtSql>

class Snapshot
{
public:
    Snapshot(QSqlDatabase mySQL, QString category);
    QString getSnapshot();
    QString getComparison(bool detailed);

private:
    double total;
    double practiced;
    double learned;
    double ddlPassed;
    double cvrg;
    double sixMonthCvrg;
    double biWeeklyCvrg;
    double timesPracticed;
    double daysWaited;
    double timeSpent;
    QDateTime lastStudied;
    QSqlDatabase mySQL;
    QString category;

};

#endif // SNAPSHOT_H

