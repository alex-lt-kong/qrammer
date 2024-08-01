#ifndef CATEGORY_H
#define CATEGORY_H

#include "src/qrammer/snapshot.h"

struct Category
{
    QString name;
    size_t KuToCramCount;
    size_t totalKuCount;
    size_t dueKuCount;
    Snapshot snapshot;
};

#endif // CATEGORY_H
