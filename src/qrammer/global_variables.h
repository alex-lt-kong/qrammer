#ifndef GLOBAL_VARIABLES_H
#define GLOBAL_VARIABLES_H

#include "src/common/db.h"

#include <QSettings>
#include <QSqlDatabase>
#include <QString>

#define DATABASE_DRIVER "QSQLITE"
#define ORGANIZATION_NAME "ak-studio"
#define APPLICATION_NAME "qrammer"

extern QSqlDatabase sqlDb;
extern QString databaseName;
extern QSettings settings;
extern DB db;

#endif // GLOBAL_VARIABLES_H
