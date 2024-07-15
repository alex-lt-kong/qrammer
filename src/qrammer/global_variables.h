#ifndef GLOBAL_VARIABLES_H
#define GLOBAL_VARIABLES_H

#include <QSettings>
#include <QSqlDatabase>
#include <QString>

#define DATABASE_DRIVER "QSQLITE"

extern QSqlDatabase db;
extern QString databaseName;
extern QSettings settings;

#endif // GLOBAL_VARIABLES_H
