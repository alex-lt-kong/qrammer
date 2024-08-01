#include "./src/common/db.h"

#include <QApplication>
#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <spdlog/spdlog.h>

using namespace std;

DB::DB() {}

DB::DB(const std::filesystem::path &dbPath)
{
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
    openConnection();
    auto query = openConnThenPrepareQuery(stmt);
    query.bindValue(":category", category);
    execQuery(query);
    if (query.next()) {
        dueNumByCat = query.value(0).toInt();
        SPDLOG_INFO("dueNum of category [{}]: {}", category.toStdString(), dueNumByCat);
        query.finish();
    } else {
    }
    return dueNumByCat;
}

void DB::updateTotalKuCount(Category &category)
{
    int totalKUCount = -1;
    auto stmt = R"***(
SELECT COUNT(*)
FROM knowledge_units
WHERE category = :category AND is_shelved = 0
)***";
    auto query = openConnThenPrepareQuery(stmt);
    query.bindValue(":category", category.name);
    execQuery(query);
    if (query.next()) {
        totalKUCount = query.value(0).toInt();
        SPDLOG_INFO("totalKUCount of category [{}]: {}", category.name.toStdString(), totalKUCount);
        query.finish();
    } else {
        throw runtime_error(query.lastError().text().toStdString());
    }
    category.totalKuCount = totalKUCount;
}

std::string DB::getDatabasePath()
{
    return databaseName;
}

struct KnowledgeUnit DB::getUrgentKu(const QString &category)
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
    auto query = openConnThenPrepareQuery(stmt);
    query.bindValue(":category", category);
    if (!query.exec()) {
        throw runtime_error("Failed to query.exec() for getUrgentKu(): "
                            + query.lastError().text().toStdString());
    }
    if (query.first()) {
        return fillinKu(query);
    } else {
        throw runtime_error("Failed to SELECT a knowledge unit from getUrgentKu();");
    }
}

struct KnowledgeUnit DB::getNewKu(const QString &category)
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
    openConnection();
    auto query = openConnThenPrepareQuery(stmt);
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

struct KnowledgeUnit DB::getRandomOldKu(const Category &cat)
{
    auto stmt = QString(R"***(
SELECT *
FROM
(
    SELECT %1
    FROM  knowledge_units
    WHERE
        category = :category AND
        times_practiced > 0 AND
        is_shelved = 0        
    ORDER BY (previous_score - passing_score) ASC
    LIMIT (SELECT ABS(RANDOM()) % (%2) + %3 + 1)
)
ORDER BY RANDOM() LIMIT 1
)***")
                    .arg(kuColumns)
                    .arg(cat.totalKuCount)
                    .arg(cat.dueKuCount);
    auto query = openConnThenPrepareQuery(stmt);
    query.bindValue(":category", cat.name);
    execQuery(query);
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

QSqlQuery DB::openConnThenPrepareQuery(const QString &stmt)
{
    openConnection();
    auto query = QSqlQuery(conn);
    if (!query.prepare(stmt)) {
        throw runtime_error("Failed to query.prepare(" + stmt.toStdString()
                            + "): " + query.lastError().text().toStdString());
    }
    return query;
}

void DB::execQuery(QSqlQuery &query)
{
    if (!query.exec())
        throw runtime_error("Failed to query.exec(): " + query.lastError().text().toStdString());
}

struct KnowledgeUnit DB::fillinKu(QSqlQuery &query)
{
    int idx = 0;
    struct KnowledgeUnit ku;
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
    ku.timeUsedSec = query.value(idx++).toInt();
    ku.AnswerImageBytes = query.value(idx++).toByteArray();
    return ku;
}

QSqlQuery DB::getQueryForKuTableView(const bool isWidthScreen)
{
    QString stmt;
    if (isWidthScreen) {
        stmt = R"***(
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
    } else {
        stmt = R"***(
SELECT
    SUBSTR(REPLACE(REPLACE(question, CHAR(10), ''), CHAR(9), ''), 0, 35) || '...' AS 'Question',
    STRFTIME('%m-%d %H:%M', last_practice_time) AS 'Last Practice'
FROM knowledge_units
WHERE is_shelved = 0
ORDER BY last_practice_time DESC
)***";
    }

    auto query = openConnThenPrepareQuery(stmt);
    execQuery(query);
    return query;
}

std::vector<Category> DB::getAllCategories()
{
    auto stmt = R"(
SELECT DISTINCT(category)
FROM knowledge_units
WHERE is_shelved = 0
ORDER BY category DESC
)";
    auto query = openConnThenPrepareQuery(stmt);
    execQuery(query);
    auto allCats = std::vector<Category>();
    while (query.next()) {
        struct Category t;
        t.name = query.value(0).toString();
        t.KuToCramCount = 0;
        t.snapshot = Snapshot(t.name);
        updateSnapshot(t.snapshot);
        allCats.emplace_back(t);
    }
    return allCats;
}

void DB::updateSnapshot(Snapshot &snap)
{
    openConnection();
    auto query = QSqlQuery(conn);
    query.prepare(
        "SELECT COUNT(*) FROM knowledge_units WHERE category = :category AND is_shelved = 0");
    query.bindValue(":category", snap.category);
    if (query.exec() && query.first())
        snap.total = query.value(0).toInt();
    else
        snap.total = 0;
    query.finish();

    query.prepare("SELECT COUNT(*) FROM knowledge_units WHERE category = :category AND "
                  "times_practiced > 0 AND is_shelved = 0");
    query.bindValue(":category", snap.category);
    if (query.exec() && query.first())
        snap.practiced = query.value(0).toInt();
    else
        snap.practiced = 0;
    query.finish();

    query.prepare(
        "SELECT COUNT(*) FROM knowledge_units WHERE category = :category AND times_practiced > "
        "0 AND previous_score >= passing_score AND is_shelved = 0");
    query.bindValue(":category", snap.category);
    if (query.exec() && query.first())
        snap.learned = query.value(0).toInt();
    else
        snap.learned = 0;
    query.finish();

    query.prepare("SELECT COUNT(*) FROM knowledge_units WHERE category = :category AND "
                  "deadline < DATETIME('now', 'localtime') AND is_shelved = 0");
    query.bindValue(":category", snap.category);
    if (query.exec() && query.first())
        snap.ddlPassed = query.value(0).toInt();
    else
        snap.ddlPassed = 0;
    query.finish();

    query.prepare("SELECT COUNT(*) FROM knowledge_units WHERE category = :category AND "
                  "times_practiced > 0 AND is_shelved = 0");
    query.bindValue(":category", snap.category);
    if (query.exec() && query.first())
        snap.cvrg = query.value(0).toDouble() / snap.total;
    else
        snap.cvrg = 0;
    query.finish();

    query.prepare("SELECT COUNT(*) FROM knowledge_units WHERE category = :category AND "
                  "times_practiced > 0  AND last_practice_time > DATETIME('now', 'localtime', "
                  "'-180 day') AND is_shelved = 0");
    query.bindValue(":category", snap.category);
    if (query.exec() && query.first())
        snap.sixMonthCvrg = query.value(0).toDouble() / snap.total;
    else
        snap.sixMonthCvrg = 0;
    query.finish();

    query.prepare("SELECT COUNT(*) FROM knowledge_units WHERE category = :category AND "
                  "times_practiced > 0  AND last_practice_time > DATETIME('now', 'localtime', "
                  "'-14 day') AND is_shelved = 0");
    query.bindValue(":category", snap.category);
    if (query.exec() && query.first())
        snap.biWeeklyCvrg = query.value(0).toDouble() / snap.total;
    else
        snap.biWeeklyCvrg = 0;
    query.finish();

    query.prepare("SELECT AVG(times_practiced) FROM knowledge_units WHERE category = :category "
                  "AND times_practiced > 0 AND is_shelved = 0");
    query.bindValue(":category", snap.category);
    if (query.exec() && query.first())
        snap.timesPracticed = query.value(0).toDouble();
    else
        snap.timesPracticed = 0;
    query.finish();

    query.prepare("SELECT AVG(JULIANDAY(first_practice_time) - JULIANDAY(insert_time)) FROM "
                  "knowledge_units WHERE category = :category AND times_practiced > 0 AND "
                  "is_shelved = 0");
    query.bindValue(":category", snap.category);
    if (query.exec() && query.first())
        snap.daysWaited = query.value(0).toDouble();
    else
        snap.daysWaited = 0;
    query.finish();

    query.prepare("SELECT SUM(time_used) / 3600.0 FROM knowledge_units WHERE category = "
                  ":category AND is_shelved = 0");
    query.bindValue(":category", snap.category);
    if (query.exec() && query.first())
        snap.timeSpent = query.value(0).toDouble();
    else
        snap.timeSpent = 0;
    query.finish();

    query.prepare(
        "SELECT last_practice_time FROM knowledge_units WHERE category = :category AND "
        "times_practiced > 0 AND is_shelved = 0 ORDER BY last_practice_time DESC LIMIT 1 ");
    query.bindValue(":category", snap.category);
    if (query.exec() && query.first())
        snap.lastStudied = query.value(0).toDateTime();
    else
        snap.lastStudied = QDateTime::fromString("1970-01-01 00:00:00", "yyyy-MM-dd hh:mm:ss");
    query.finish();
}

void DB::updateKu(const struct KnowledgeUnit &ku)
{
    auto stmt = QString(R"**(
UPDATE knowledge_units
SET
    last_practice_time = DATETIME('Now', 'localtime'),
    previous_score = :previous_score,
    question = :question,
    answer = :answer,
    times_practiced = :times_practiced,
    passing_score = :passing_score,
    deadline = :deadline,
    first_practice_time = :first_practice_time,
    client_name = :client_name,
    time_used = :time_used,
    answer_image = :answer_image
WHERE id = :id
)**");
    auto query = openConnThenPrepareQuery(stmt);
    query.bindValue(":previous_score", ku.NewScore);
    query.bindValue(":question", ku.Question);
    query.bindValue(":answer", ku.Answer);
    query.bindValue(":times_practiced", ku.TimesPracticed);
    query.bindValue(":passing_score", ku.PassingScore);
    query.bindValue(":client_name", ku.ClientName);
    query.bindValue(":time_used", ku.timeUsedSec);
    query.bindValue(":first_practice_time", ku.FirstPracticeTime);
    query.bindValue(":deadline", ku.Deadline);
    query.bindValue(":id", ku.ID);
    query.bindValue(":answer_image", ku.AnswerImageBytes);
    execQuery(query);
}
