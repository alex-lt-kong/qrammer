#include "snapshot.h"

Snapshot::Snapshot(QSqlDatabase mySQL, QString category)
{
    this->mySQL = mySQL;
    this->category = category;
}

QString Snapshot::getSnapshot()
{
    QString result;    

    if (mySQL.open()) {

        QSqlQuery *query = new QSqlQuery(mySQL);
        query->prepare("SELECT COUNT(*) FROM knowledge_units WHERE category = :category AND is_shelved = 0");
        query->bindValue(":category", category);
        if (query->exec() && query->first())
            total = query->value(0).toInt();
        else
            total = 0;
        query->finish();

        query->prepare("SELECT COUNT(*) FROM knowledge_units WHERE category = :category AND times_practiced > 0 AND is_shelved = 0");
        query->bindValue(":category", category);
        if (query->exec() && query->first())
            practiced = query->value(0).toInt();
        else
            practiced = 0;
        query->finish();


        query->prepare("SELECT COUNT(*) FROM knowledge_units WHERE category = :category AND times_practiced > 0 AND previous_score >= passing_score AND is_shelved = 0");
        query->bindValue(":category", category);
        if (query->exec() && query->first())
            learned = query->value(0).toInt();
        else
            learned = 0;
        query->finish();

        query->prepare("SELECT COUNT(*) FROM knowledge_units WHERE category = :category AND deadline < DATETIME('now', 'localtime') AND is_shelved = 0");
        query->bindValue(":category", category);
        if (query->exec() && query->first())
            ddlPassed = query->value(0).toInt();
        else
            ddlPassed = 0;
        query->finish();

        query->prepare("SELECT COUNT(*) FROM knowledge_units WHERE category = :category AND times_practiced > 0 AND is_shelved = 0");
        query->bindValue(":category", category);
       if (query->exec() && query->first())
            cvrg = query->value(0).toDouble() / total;
        else
           cvrg = 0;
       query->finish();

        query->prepare("SELECT COUNT(*) FROM knowledge_units WHERE category = :category AND times_practiced > 0  AND last_practice_time > DATETIME('now', 'localtime', '-180 day') AND is_shelved = 0");
        query->bindValue(":category", category);
        if (query->exec() && query->first())
            sixMonthCvrg = query->value(0).toDouble() / total;
        else
            sixMonthCvrg = 0;
        query->finish();


        query->prepare("SELECT COUNT(*) FROM knowledge_units WHERE category = :category AND times_practiced > 0  AND last_practice_time > DATETIME('now', 'localtime', '-14 day') AND is_shelved = 0");
        query->bindValue(":category", category);
       if (query->exec() && query->first())
            biWeeklyCvrg = query->value(0).toDouble() / total;
       else
           biWeeklyCvrg = 0;
       query->finish();


        query->prepare("SELECT AVG(times_practiced) FROM knowledge_units WHERE category = :category AND times_practiced > 0 AND is_shelved = 0");
        query->bindValue(":category", category);
       if (query->exec() && query->first())
           timesPracticed = query->value(0).toDouble();
       else
           timesPracticed = 0;
       query->finish();



        query->prepare("SELECT AVG(JULIANDAY(first_practice_time) - JULIANDAY(insert_time)) FROM knowledge_units WHERE category = :category AND times_practiced > 0 AND is_shelved = 0");
        query->bindValue(":category", category);
       if (query->exec() && query->first())
           daysWaited = query->value(0).toDouble();
       else
           daysWaited = 0;
       query->finish();

       query->prepare("SELECT SUM(time_used) / 3600.0 FROM knowledge_units WHERE category = :category AND is_shelved = 0");
       query->bindValue(":category", category);
       if (query->exec() && query->first())
           timeSpent = query->value(0).toDouble();
       else
           timeSpent = 0;
       query->finish();

        query->prepare("SELECT last_practice_time FROM knowledge_units WHERE category = :category AND times_practiced > 0 AND is_shelved = 0 ORDER BY last_practice_time DESC LIMIT 1 ");
        query->bindValue(":category", category);
        if (query->exec() && query->first())
            lastStudied = query->value(0).toDateTime();
        else
            lastStudied = QDateTime::fromString("1970-01-01 00:00:00", "yyyy-MM-dd hh:mm:ss");
        query->finish();

        result = "[" + category + "]\n";
        result += "• Number\n";
        result += QString("%1%2").arg("Total", -16).arg(total) + "\n";
        result += QString("%1%2").arg("Practiced", -16).arg(practiced) + "\n";
        result += QString("%1%2").arg("Learned", -16).arg(learned) + "\n";
        result += QString("%1%2").arg("DDL Passed", -16).arg(ddlPassed) + "\n";

        result += "• Coverage\n";
        result += QString("%1%2").arg("Total", -16).arg(QString::number(cvrg * 100, 'f', 1)) + "%\n";
        result += QString("%1%2").arg("Six-Monthly", -16).arg(QString::number(sixMonthCvrg * 100, 'f', 1)) + "%\n";
        result += QString("%1%2").arg("Biweekly", -16).arg(QString::number(biWeeklyCvrg * 100, 'f', 1)) + "%\n";

        result += "• Miscellaneous\n";
        result += QString("%1%2").arg("Times Practiced", -16).arg(QString::number(timesPracticed, 'f', 1)) + "\n";
        result += QString("%1%2").arg("Days Waited", -16).arg(QString::number(daysWaited, 'f', 1)) + "\n";
        result += QString("%1%2").arg("Hours Spent", -16).arg(QString::number(timeSpent, 'f', 1)) + "\n";
        result += "• Last Study at\n";
        result += lastStudied.toString("yyyy-MM-dd HH:mm:ss\n");

        mySQL.close();

        return result;
    } else {
        return "Failed to open database: " + mySQL.lastError().text();
    }
}

QString Snapshot::getComparison(bool detailed)
{
    QString result;
    double t_practiced = 0;
    double t_learned = 0;
    double t_ddlPassed = 0;

    QSqlQuery *query = new QSqlQuery(mySQL);

    if (mySQL.open()) {

        query->prepare("SELECT COUNT(*) FROM knowledge_units WHERE category = :category AND times_practiced > 0 AND is_shelved = 0");
        query->bindValue(":category", category);
        if (query->exec() && query->first())
            t_practiced = query->value(0).toInt();
        query->finish();

        query->prepare("SELECT COUNT(*) FROM knowledge_units WHERE category = :category AND times_practiced > 0 AND previous_score >= passing_score AND is_shelved = 0");
        query->bindValue(":category", category);
        if (query->exec() && query->first())
            t_learned = query->value(0).toInt();
        query->finish();

        query->prepare("SELECT COUNT(*) FROM knowledge_units WHERE category = :category AND deadline < DATETIME('now', 'localtime') AND is_shelved = 0");
        query->bindValue(":category", category);
        if (query->exec() && query->first())
            t_ddlPassed = query->value(0).toInt();
        query->finish();

        result = "[" + category + "]\n";
        result += QString("%1%2").arg("Practiced", -16).arg(t_practiced - practiced) + "\n";
        if (detailed) result += QString("%1%2").arg("Learned", -16).arg(t_learned - learned) + "\n";
        result += QString("%1%2").arg("DDL Passed", -16).arg(t_ddlPassed - ddlPassed) + "\n";

        mySQL.close();

        return result;
    } else {
        return "Failed to open database: " + mySQL.lastError().text();
    }
}

