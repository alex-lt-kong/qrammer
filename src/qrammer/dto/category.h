#ifndef CATEGORY_H
#define CATEGORY_H

#include "src/qrammer/snapshot.h"

#define PROGRESS_LOOKBACK_PERIODS 16
#define PROGRESS_LOOKBACK_DAYS_PER_PERIOD 7

namespace Qrammer::Dto {

struct Category
{
    QString name;
    size_t KuToCramCount;
    size_t totalKuCount;
    size_t dueKuCount;
    Snapshot snapshot;
    int histogram[PROGRESS_LOOKBACK_PERIODS];
};

} // namespace Qrammer::Dto

#endif // CATEGORY_H
