#ifndef DB_H
#define DB_H

#include "src/common/dto/category.h"
#include "src/common/dto/knowledge_unit.h"

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
    void openConnection();
    QSqlQuery openConnThenPrepareQuery(const QString &stmt);
    void execQuery(QSqlQuery &query);
    std::string getDatabasePath();
    std::vector<Category> getAllCategories();
    QSqlQuery getQueryForKuTableView(const bool isWidthScreen);
    QSqlDatabase conn;
    // TODO: move kuColumns to private after refactoring is done
    QString kuColumns;

private:
    std::string databaseName;
    struct KnowledgeUnit fillinKu(QSqlQuery &query);
};

#endif // DB_H
