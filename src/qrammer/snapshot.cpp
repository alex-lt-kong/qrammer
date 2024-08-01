#include "snapshot.h"
#include "src/qrammer/global_variables.h"

Snapshot::Snapshot(const QString &category)
{
    this->category = category;
}

Snapshot::Snapshot()
{
    category = "";
}

QString Snapshot::getSnapshotString()
{
    QString result;
    result = "[" + category + "]\n";
    result += "• Number\n";
    result += QString("%1%2").arg("Total", -16).arg(total) + "\n";
    result += QString("%1%2").arg("Practiced", -16).arg(practiced) + "\n";
    result += QString("%1%2").arg("Learned", -16).arg(learned) + "\n";
    result += QString("%1%2").arg("DDL Passed", -16).arg(ddlPassed) + "\n";

    result += "• Coverage\n";
    result += QString("%1%2").arg("Total", -16).arg(QString::number(cvrg * 100, 'f', 1)) + "%\n";
    result += QString("%1%2").arg("Six-Monthly", -16).arg(QString::number(sixMonthCvrg * 100, 'f', 1))
              + "%\n";
    result += QString("%1%2").arg("Biweekly", -16).arg(QString::number(biWeeklyCvrg * 100, 'f', 1))
              + "%\n";

    result += "• Miscellaneous\n";
    result += QString("%1%2").arg("Times Practiced", -16).arg(QString::number(timesPracticed, 'f', 1))
              + "\n";
    result += QString("%1%2").arg("Days Waited", -16).arg(QString::number(daysWaited, 'f', 1))
              + "\n";
    result += QString("%1%2").arg("Hours Spent", -16).arg(QString::number(timeSpent, 'f', 1))
              + "\n";
    result += "• Last Study at\n";
    result += lastStudied.toString("yyyy-MM-dd HH:mm:ss\n");

    return result;
}

QString Snapshot::operator-(const Snapshot &rhs)
{
    QString result;
    result = "[" + category + "]\n";
    result += QString("%1%2").arg("Practiced", -16).arg(practiced - rhs.practiced) + "\n";
    result += QString("%1%2").arg("Learned", -16).arg(learned - rhs.learned) + "\n";
    result += QString("%1%2").arg("DDL Passed", -16).arg(ddlPassed - rhs.ddlPassed) + "\n";
    return result;
}
