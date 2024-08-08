#ifndef CATEGORY_H
#define CATEGORY_H

#include "src/qrammer/snapshot.h"

#include <QBarSet>

struct Category
{
    QString name;
    size_t KuToCramCount;
    size_t totalKuCount;
    size_t dueKuCount;
    Snapshot snapshot;
    std::unique_ptr<QBarSet> set;
};

#endif // CATEGORY_H
