#include "./src/common/db.h"

#include <QApplication>
#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <spdlog/spdlog.h>

using namespace std;

DB::DB()
{
    qDebug() << "DB::DB()";
}

DB::DB(const std::filesystem::path &dbPath)
{
    qDebug() << "DB::DB(const QString &dbPath)";
    conn = QSqlDatabase::addDatabase("QSQLITE");
    databaseName = dbPath.string();
    conn.setDatabaseName(QString::fromStdString(dbPath.string()));

    kuColumns = QString(R"***(
id,
question,
answer,
passing_score,
previous_score,
times_practiced,
insert_time,
first_practice_time,
last_practice_time,
deadline,
client_name,
category,
time_used,
answer_image)***");
}

int DB::getDueKuCountByCategory(const QString &category)
{
    int dueNumByCat = -1;
    auto stmt = R"***(
SELECT COUNT(*)
FROM knowledge_units
WHERE
    category = :category AND
    deadline <= DATETIME('now', 'localtime') AND
    LENGTH(deadline) > 0 AND
    is_shelved = 0
)***";
    auto query = prepareQuery(stmt);
    query.bindValue(":category", category);

    if (!query.exec()) {
        throw runtime_error(query.lastError().text().toStdString());
    }
    if (query.next()) {
        dueNumByCat = query.value(0).toInt();
        SPDLOG_INFO("dueNum of category [{}]: {}", category.toStdString(), dueNumByCat);
        query.finish();
    } else {
    }
    return dueNumByCat;
}

int DB::getTotalKUNumByCategory(const QString &category)
{
    int totalKUCount = -1;
    auto stmt = R"***(
SELECT COUNT(*)
FROM knowledge_units
WHERE category = :category AND is_shelved = 0
)***";
    auto query = prepareQuery(stmt);
    query.bindValue(":category", category);

    if (!query.exec()) {
        throw runtime_error(query.lastError().text().toStdString());
    }
    if (query.next()) {
        totalKUCount = query.value(0).toInt();
        SPDLOG_INFO("totalKUCount of category [{}]: {}", category.toStdString(), totalKUCount);
        query.finish();
    } else {
        throw runtime_error(query.lastError().text().toStdString());
    }
    return totalKUCount;
}

std::string DB::getDatabasePath()
{
    return databaseName;
}

struct knowledge_unit DB::getUrgentKu(const QString &category)
{
    auto stmt = QString(R"***(
SELECT %1
FROM knowledge_units
WHERE
    category = :category AND
    deadline <= DATETIME('now', 'localtime') AND
    LENGTH(deadline) > 0 AND
    is_shelved = 0
ORDER BY RANDOM()
LIMIT 1
)***")
                    .arg(kuColumns);
    auto query = prepareQuery(stmt);
    query.bindValue(":category", category);
    if (!query.exec()) {
        throw runtime_error(query.lastError().text().toStdString());
    }
    if (query.first()) {
        return fillinKu(query);
    } else {
        throw runtime_error("Failed to SELECT a knowledge unit from getUrgentKu();");
    }
}

struct knowledge_unit DB::getNewKu(const QString &category)
{
    auto stmt = QString(R"***(
SELECT %1
FROM knowledge_units
WHERE
    category = :category AND
    times_practiced = 0 AND
    is_shelved = 0
ORDER BY RANDOM()
LIMIT 1
)***")
                    .arg(kuColumns);
    auto query = prepareQuery(stmt);
    query.bindValue(":category", category);
    if (!query.exec()) {
        auto errMsg = query.lastError().text().toStdString();
        SPDLOG_ERROR(errMsg);
        throw runtime_error(errMsg);
    }
    if (query.first()) {
        return fillinKu(query);
    } else {
        auto errMsg = "Failed to SELECT a knowledge unit from getNewKu();";
        SPDLOG_ERROR(errMsg);
        throw runtime_error(errMsg);
    }
}

struct knowledge_unit DB::getRandomKu(const QString &category)
{
    auto stmt = QString(R"***(
SELECT %1
FROM knowledge_units
WHERE
    category = :category AND
    is_shelved = 0
ORDER BY RANDOM()
LIMIT 1;
)***")
                    .arg(kuColumns);
    auto query = prepareQuery(stmt);
    query.bindValue(":category", category);
    if (!query.exec()) {
        throw runtime_error(query.lastError().text().toStdString());
    }
    if (query.first()) {
        return fillinKu(query);
    } else {
        throw runtime_error("Failed to SELECT a knowledge unit from getRandomKu();");
    }
}

void DB::openConnection()
{
    if (!conn.isOpen() && !conn.open()) {
        throw runtime_error("Failed to open database: " + conn.lastError().text().toStdString());
    }
}

QSqlQuery DB::prepareQuery(const QString &stmt)
{
    openConnection();
    auto query = QSqlQuery(conn);
    if (!query.prepare(stmt)) {
        throw runtime_error(query.lastError().text().toStdString());
    }
    return query;
}

struct knowledge_unit DB::fillinKu(QSqlQuery &query)
{
    int idx = 0;
    struct knowledge_unit ku;
    ku.ID = query.value(idx++).toInt();
    ku.Question = query.value(idx++).toString();
    ku.Answer = query.value(idx++).toString();
    ku.PassingScore = query.value(idx++).toDouble();
    ku.PreviousScore = query.value(idx++).toDouble();
    ku.TimesPracticed = query.value(idx++).toInt();
    ku.InsertTime = query.value(idx++).toDateTime();
    ku.FirstPracticeTime = query.value(idx++).toDateTime();
    ku.LastPracticeTime = query.value(idx++).toDateTime();
    ku.Deadline = query.value(idx++).toDateTime();
    ku.ClientName = query.value(idx++).toString();
    ku.Category = query.value(idx++).toString();
    ku.SecSpent = query.value(idx++).toInt();
    ku.AnswerImageBytes = query.value(idx++).toByteArray();
    return ku;
}
