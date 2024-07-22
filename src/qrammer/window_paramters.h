#ifndef WINDOW_PARAMTERS_H
#define WINDOW_PARAMTERS_H

#include <QPoint>
#include <QSize>

struct window_parameters
{
    //0ID, 1Question, 2Answer, 3PassingScore, 4PreviousScore, 5TimesPracticed, 6InsertTime, 7FirstPracticeTime, 8LastPracticeTime, 9Deadline, 10ClientType, 11Maintype"
    int screenId;
    QByteArray geometry;
    QByteArray state;
    QPoint pos;
};

#endif // WINDOW_PARAMTERS_H
