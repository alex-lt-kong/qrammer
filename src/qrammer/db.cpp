#include "db.h"
#include "src/qrammer/global_variables.h"

#include <QApplication>
#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <spdlog/spdlog.h>

using namespace std;
using namespace Qrammer;

DB::DB() {}

DB::DB(const std::filesystem::path &dbPath)
{
    // qDebug() << "DB::DB(const std::filesystem::path &dbPath)";
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
answer_image,
question_image)***");
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
    auto query = openConnThenPrepareQuery(stmt);
    query.bindValue(":category", category);
    execPreparedQuery(query);
    if (query.next()) {
        dueNumByCat = query.value(0).toInt();
        SPDLOG_INFO("dueNum of category [{}]: {}", category.toStdString(), dueNumByCat);
        query.finish();
    } else {
    }
    return dueNumByCat;
}

void DB::updateTotalKuCount(Qrammer::Dto::Category &category)
{
    int totalKUCount = -1;
    auto stmt = R"***(
SELECT COUNT(*)
FROM knowledge_units
WHERE category = :category AND is_shelved = 0
)***";
    auto query = openConnThenPrepareQuery(stmt);
    query.bindValue(":category", category.name);
    execPreparedQuery(query);
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

struct Dto::KnowledgeUnit DB::getUrgentKu(const QString &category)
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

Dto::KnowledgeUnit DB::getNewKu(const QString &category)
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

struct Dto::KnowledgeUnit DB::selectKuById(const int kuid)
{
    auto stmt = QString(R"***(
SELECT %1
FROM knowledge_units
WHERE id = :id
)***")
                    .arg(kuColumns);
    auto query = execSelectQuery(stmt, std::vector<std::pair<QString, QVariant>>{{":id", kuid}});
    if (query.first()) {
        return fillinKu(query);
    }
    struct Dto::KnowledgeUnit ku;
    ku.ID = -1;
    return ku;
}

struct Dto::KnowledgeUnit DB::getRandomOldKu(const Qrammer::Dto::Category &cat)
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
    execPreparedQuery(query);
    if (query.first()) {
        return fillinKu(query);
    } else {
        throw runtime_error("Failed to SELECT a knowledge unit from getRandomKu();");
    }
}

void DB::openConnection()
{
    if (!conn.isOpen()) {
        conn.setConnectOptions("QSQLITE_BUSY_TIMEOUT=10000");
        SPDLOG_INFO("opening");
        if (!conn.open()) {
            throw runtime_error("Failed to open database: " + conn.lastError().text().toStdString());
        }
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

void DB::openConnThenPrepareQuery(const QString &stmt, QSqlQuery &query)
{
    openConnection();
    query = QSqlQuery(conn);
    if (!query.prepare(stmt)) {
        throw runtime_error("Failed to query.prepare(" + stmt.toStdString()
                            + "): " + query.lastError().text().toStdString());
    }
}

int DB::execPreparedQuery(QSqlQuery &query)
{
    if (!query.exec())
        throw runtime_error("Failed to query.exec(): " + query.lastError().text().toStdString());
    return query.numRowsAffected();
}

struct Dto::KnowledgeUnit DB::fillinKu(QSqlQuery &query)
{
    int idx = 0;
    struct Dto::KnowledgeUnit ku;
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
    ku.QuestionImageBytes = query.value(idx++).toByteArray();
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

    return execSelectQuery(stmt);
}

std::vector<Qrammer::Dto::Category> DB::getAllCategories()
{
    auto stmt = R"(
SELECT DISTINCT(category)
FROM knowledge_units
WHERE is_shelved = 0
ORDER BY category DESC
)";
    auto query = execSelectQuery(stmt);
    auto allCats = std::vector<Dto::Category>();

    while (query.next()) {
        Dto::Category t;
        t.name = query.value(0).toString();
        t.KuToCramCount = 0;
        t.snapshot = Snapshot(t.name);
        updateSnapshot(t.snapshot);
        auto catBinding = std::vector<std::pair<QString, QVariant>>{{":category", t.name}};
        for (int i = 1; i <= PROGRESS_LOOKBACK_PERIODS; ++i) {
            {
                auto stmt = QString(R"***(
SELECT COUNT(*)
FROM knowledge_units
WHERE
    DATE(first_practice_time) > DATE('now', '%1 day') AND
    DATE(first_practice_time) <= DATE('now', '%2 day') AND
    category = :category
)***")
                                .arg(i * -1 * PROGRESS_LOOKBACK_DAYS_PER_PERIOD)
                                .arg((i - 1) * -1 * PROGRESS_LOOKBACK_DAYS_PER_PERIOD);
                auto q = db.execSelectQuery(stmt, catBinding);
                t.histFirstAppearKuCount[i - 1] = q.first() ? q.value(0).toInt() : -1;
            }
            {
                auto stmt = QString(R"***(
SELECT COUNT(*)
FROM knowledge_units
WHERE
    DATE(insert_time) > DATE('now', '%1 day') AND
    DATE(insert_time) <= DATE('now', '%2 day') AND
    category = :category
)***")
                                .arg(i * -1 * PROGRESS_LOOKBACK_DAYS_PER_PERIOD)
                                .arg((i - 1) * -1 * PROGRESS_LOOKBACK_DAYS_PER_PERIOD);
                auto q = db.execSelectQuery(stmt, catBinding);
                t.histNewlyAddedKuCount[i - 1] = q.first() ? q.value(0).toInt() : -1;
            }
        }
        allCats.emplace_back(std::move(t));
    }
    return allCats;
}

void DB::updateSnapshot(Snapshot &snap)
{
    auto catBinding = std::vector<std::pair<QString, QVariant>>{{":category", snap.category}};

    {
        auto stmt = R"***(
SELECT COUNT(*)
FROM knowledge_units
WHERE
    category = :category AND
    is_shelved = 0
)***";
        auto query = execSelectQuery(stmt, catBinding);
        snap.total = query.first() ? query.value(0).toInt() : 0;
    }

    {
        auto stmt = R"***(
SELECT COUNT(*)
FROM knowledge_units
WHERE
    category = :category AND
    times_practiced > 0 AND
    is_shelved = 0
)***";
        auto query = execSelectQuery(stmt, catBinding);
        snap.practiced = query.first() ? query.value(0).toInt() : 0;
    }

    {
        auto stmt = R"***(
SELECT COUNT(*)
FROM knowledge_units
WHERE
    category = :category AND times_practiced > 0 AND
    previous_score >= passing_score AND
    is_shelved = 0
)***";
        auto query = execSelectQuery(stmt, catBinding);
        snap.learned = query.first() ? query.value(0).toInt() : 0;
    }

    {
        auto stmt = R"***(
SELECT COUNT(*)
FROM knowledge_units
WHERE
    category = :category AND
    deadline < DATETIME('now', 'localtime') AND
    is_shelved = 0
)***";
        auto query = execSelectQuery(stmt, catBinding);
        snap.ddlPassed = query.first() ? query.value(0).toInt() : 0;
    }

    {
        auto stmt = R"***(
SELECT COUNT(*)
FROM knowledge_units
WHERE
    category = :category AND
    times_practiced > 0 AND
    is_shelved = 0
)***";
        auto query = execSelectQuery(stmt, catBinding);
        snap.cvrg = query.first() ? query.value(0).toDouble() / snap.total : 0;
    }

    {
        auto stmt = R"***(
SELECT COUNT(*)
FROM knowledge_units
WHERE
    category = :category AND
    times_practiced > 0  AND
    last_practice_time > DATETIME('now', 'localtime', '-180 day') AND
    is_shelved = 0
)***";
        auto query = execSelectQuery(stmt, catBinding);
        snap.sixMonthCvrg = query.first() ?  query.value(0).toDouble() / snap.total : 0;
    }

    {
        auto stmt = R"***(
SELECT COUNT(*)
FROM knowledge_units
WHERE
    category = :category AND
    times_practiced > 0  AND
    last_practice_time > DATETIME('now', 'localtime', '-14 day') AND
    LENGTH(last_practice_time) > 0 AND
    is_shelved = 0
)***";
        auto query = execSelectQuery(stmt, catBinding);
        snap.biWeeklyCvrg = query.first() ? query.value(0).toDouble() / snap.total : 0;
    }

    {
        auto stmt = R"***(
SELECT AVG(times_practiced)
FROM knowledge_units
WHERE
    category = :category AND
    times_practiced > 0 AND
    is_shelved = 0
)***";
        auto query = execSelectQuery(stmt, catBinding);
        snap.timesPracticed = query.first() ? query.value(0).toDouble() : 0;
    }

    {
        auto stmt = R"***(
SELECT AVG(JULIANDAY(first_practice_time) - JULIANDAY(insert_time))
FROM knowledge_units
WHERE
    category = :category AND
    times_practiced > 0 AND
    is_shelved = 0
)***";
        auto query = execSelectQuery(stmt, catBinding);
        snap.daysWaited = query.first() ? query.value(0).toDouble() : 0;
    }

    {
        auto stmt = R"***(
SELECT SUM(time_used) / 3600.0
FROM knowledge_units
WHERE
    category = :category AND
    is_shelved = 0
)***";
        auto query = execSelectQuery(stmt, catBinding);
        snap.timeSpent = query.first() ? query.value(0).toDouble() : 0;
    }

    {
        auto stmt = R"***(
SELECT last_practice_time
FROM knowledge_units
WHERE
    category = :category AND
    times_practiced > 0 AND
    is_shelved = 0
ORDER BY last_practice_time
DESC LIMIT 1
)***";
        auto query = execSelectQuery(stmt, catBinding);
        snap.lastStudied = query.first() ? query.value(0).toDateTime()
                                         : QDateTime::fromString("1970-01-01 00:00:00",
                                                                 "yyyy-MM-dd hh:mm:ss");
    }
}

int DB::updateKu(const struct Dto::KnowledgeUnit &ku)
{
    auto stmt = QString(R"**(
UPDATE knowledge_units
SET
    last_practice_time = :last_practice_time,
    previous_score = :previous_score,
    question = :question,
    answer = :answer,
    times_practiced = :times_practiced,
    passing_score = :passing_score,
    deadline = :deadline,
    first_practice_time = :first_practice_time,
    client_name = :client_name,
    time_used = :time_used,
    answer_image = :answer_image,
    question_image = :question_image
WHERE id = :id
)**");
    auto query = openConnThenPrepareQuery(stmt);
    query.bindValue(":last_practice_time", ku.LastPracticeTime);
    query.bindValue(":previous_score", ku.PreviousScore);
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
    query.bindValue(":question_image", ku.QuestionImageBytes);
    return execPreparedQuery(query);
}

int DB::insertKu(const struct Dto::KnowledgeUnit &ku)
{
    auto stmt = QString(R"***(
INSERT INTO knowledge_units (
    question,
    answer,
    times_practiced,
    previous_score,
    category,
    passing_score,
    deadline,
    insert_time,
    last_practice_time,
    client_name,
    question_image,
    answer_image
)
VALUES (
    :question,
    :answer,
    :times_practiced,
    :previous_score,
    :category,
    :passing_score,
    :deadline,
    DATETIME('Now', 'localtime'),
    :last_practice_time,
    :client_name,
    :question_image,
    :answer_image
)
)***");
    auto query = openConnThenPrepareQuery(stmt);
    query.bindValue(":question", ku.Question);
    query.bindValue(":answer", ku.Answer);
    query.bindValue(":times_practiced", ku.TimesPracticed);
    query.bindValue(":previous_score", ku.PreviousScore);
    query.bindValue(":category", ku.Category);
    query.bindValue(":passing_score", ku.PassingScore);
    query.bindValue(":deadline", ku.Deadline);
    query.bindValue(":last_practice_time", ku.LastPracticeTime);
    query.bindValue(":client_name", ku.ClientName);
    query.bindValue(":question_image", ku.QuestionImageBytes);
    query.bindValue(":answer_image", ku.AnswerImageBytes);
    return execPreparedQuery(query);
}

std::vector<std::tuple<QString, QString>> DB::getSearchOptions()
{
    auto stmt = R"(
SELECT name, url
FROM search_options
ORDER BY id ASC
)";
    auto query = execSelectQuery(stmt);
    auto options = std::vector<std::tuple<QString, QString>>();
    while (query.next()) {
        options.emplace_back(query.value(0).toString(), query.value(1).toString());
    }
    return options;
}

void DB::deleteKu(const struct Dto::KnowledgeUnit &ku)
{
    deleteKu(ku.ID);
}

void DB::deleteKu(const int kuId)
{
    auto stmt = QString(R"**(
DELETE FROM knowledge_units
WHERE id = :id
)**");
    auto query = openConnThenPrepareQuery(stmt);
    query.bindValue(":id", kuId);
    execPreparedQuery(query);
}

QSqlQuery DB::execSelectQuery(const QString &stmt,
                              const std::vector<std::pair<QString, QVariant>> &bindKvs)
{
    auto query = openConnThenPrepareQuery(stmt);
    for (const auto &kv : bindKvs) {
        query.bindValue(kv.first, kv.second);
    }
    execPreparedQuery(query);
    return query;
}
