#ifndef GLOBAL_VARIABLES_H
#define GLOBAL_VARIABLES_H

#include <QSqlDatabase>
#include <QString>

#define DATABASE_DRIVER "QSQLITE"

extern QSqlDatabase db;
extern QString databaseName;

#endif // GLOBAL_VARIABLES_H
