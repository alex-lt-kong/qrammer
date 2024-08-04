#ifndef GLOBAL_VARIABLES_H
#define GLOBAL_VARIABLES_H

#include "src/qrammer/db.h"

#include <QSettings>
#include <QSqlDatabase>
#include <QString>

#define ORGANIZATION_NAME "ak-studio"
#define APPLICATION_NAME "qrammer"

extern QSettings settings;
extern DB db;

#endif // GLOBAL_VARIABLES_H
