#include "global_variables.h"

#include <QCoreApplication>

QString databaseName;
QSqlDatabase db;
QSettings settings(QCoreApplication::QCoreApplication::applicationDirPath() + "./../config.ini",
                   QSettings::IniFormat);
