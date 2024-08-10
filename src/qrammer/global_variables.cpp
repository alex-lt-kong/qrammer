#include "global_variables.h"

#include <QCoreApplication>

using namespace Qrammer;

QSettings settings = QSettings(ORGANIZATION_NAME, APPLICATION_NAME);
DB db;
std::vector<Dto::Category> availableCategory;
