#ifndef DB_H
#define DB_H

#include "./src/qrammer/knowledge_unit.h"

#include <QSqlDatabase>
#include <QString>

#include <filesystem>

class DB
{
public:
    DB();
    DB(const std::filesystem::path &dbPath);
    int getDueKuCountByCategory(const QString &category);
    int getTotalKUNumByCategory(const QString &category);
    struct knowledge_unit getUrgentKu(const QString &category);
    struct knowledge_unit getNewKu(const QString &category);
    struct knowledge_unit getRandomKu(const QString &category);
    void openConnection();
    QSqlQuery prepareQuery(const QString &stmt);
    std::string getDatabasePath();
    QSqlDatabase conn;
    // TODO: move kuColumns to private after refactoring is done
    QString kuColumns;

private:
    std::string databaseName;
    struct knowledge_unit fillinKu(QSqlQuery &query);
};

#endif // DB_H
