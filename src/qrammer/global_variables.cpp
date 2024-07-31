#include "global_variables.h"

#include <QCoreApplication>

QString databaseName;
QSqlDatabase sqlDb;
QSettings settings = QSettings(ORGANIZATION_NAME, APPLICATION_NAME);
DB db;
