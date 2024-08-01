#ifndef SNAPSHOT_H
#define SNAPSHOT_H

#include <QtSql>

class Snapshot
{
public:
    Snapshot(const QString &category);
    Snapshot();
    QString getSnapshotString();
    // TODO: it is not a simplified implementation, see if we need to make it more complete..
    QString operator-(const Snapshot &rhs);
    QString getComparison(bool detailed);
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
    QString category;

};

#endif // SNAPSHOT_H

