#ifndef DB_H
#define DB_H

#include "dto/category.h"
#include "dto/knowledge_unit.h"

#include <QSqlDatabase>
#include <QString>

#include <filesystem>

class DB
{
public:
    DB();
    DB(const std::filesystem::path &dbPath);
    int getDueKuCountByCategory(const QString &category);
    void updateTotalKuCount(Category &category);
    void updateSnapshot(Snapshot &snap);
    struct KnowledgeUnit getUrgentKu(const QString &category);
    struct KnowledgeUnit getNewKu(const QString &category);
    struct KnowledgeUnit getRandomOldKu(const Category &cat);
    void updateKu(const struct KnowledgeUnit &ku);
    void deleteKu(const struct KnowledgeUnit &ku);
    void openConnection();
    QSqlQuery openConnThenPrepareQuery(const QString &stmt);
    void openConnThenPrepareQuery(const QString &stmt, QSqlQuery &query);
    void execQuery(QSqlQuery &query);
    std::string getDatabasePath();
    std::vector<Category> getAllCategories();
    QSqlQuery getQueryForKuTableView(const bool isWidthScreen);
    std::vector<std::tuple<QString, QString>> getSearchOptions();
    QSqlDatabase conn;

private:
    std::string databaseName;
    struct KnowledgeUnit fillinKu(QSqlQuery &query);
    QString kuColumns;
};

#endif // DB_H
