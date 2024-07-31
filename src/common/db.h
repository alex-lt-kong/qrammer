#ifndef DB_H
#define DB_H

#include "./src/qrammer/knowledge_unit.h"

#include <QSqldatabase>
#include <QString>

class DB
{
public:
    DB();
    int getDueKuCountByCategory(const QString &category);
    int getTotalKUNumByCategory(const QString &category);
    struct knowledge_unit getUrgentKu(const QString &category);
    struct knowledge_unit getNewKu(const QString &category);
    struct knowledge_unit getRandomKu(const QString &category);
    void openConnection();
    QSqlQuery prepareQuery(const QString &stmt);
    QString getDatabasePath();
    QSqlDatabase conn;
    // TODO: move kuColumns to private after refactoring is done
    QString kuColumns;

private:
    QString databaseName;
    struct knowledge_unit fillinKu(QSqlQuery &query);
};

#endif // DB_H
