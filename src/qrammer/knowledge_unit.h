#ifndef KNOWLEDGE_UNIT_H
#define KNOWLEDGE_UNIT_H

#include <QDateTime>

struct knowledge_unit
{
    //0ID, 1Question, 2Answer, 3PassingScore, 4PreviousScore, 5TimesPracticed, 6InsertTime, 7FirstPracticeTime, 8LastPracticeTime, 9Deadline, 10ClientType, 11Maintype"
    int ID;
    QString Question;
    QString Answer;
    double PassingScore;
    // PreviousScore needs to be set to zero or on Android platform there will be a flash of incorrect new score (such as .1617e+242)due to the uninitialized cku_PreviousScore.
    double PreviousScore;
    int TimesPracticed;
    QDateTime InsertTime;
    QDateTime FirstPracticeTime;
    QDateTime LastPracticeTime;
    QDateTime Deadline;
    QString ClientName;
    QString Category;
    int SecSpent;
    QByteArray AnswerImageBytes;
};

#endif // KNOWLEDGE_UNIT_H
