#include "global_variables.h"

#include <QCoreApplication>

QSettings settings = QSettings(ORGANIZATION_NAME, APPLICATION_NAME);
DB db;
std::vector<Category> availableCategory;
size_t progressLookBackDays = 60;
