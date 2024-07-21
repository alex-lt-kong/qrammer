#include "global_variables.h"

#include <QCoreApplication>

QString databaseName;
QSqlDatabase db;
QSettings settings = QSettings(ORGANIZATION_NAME, APPLICATION_NAME);
