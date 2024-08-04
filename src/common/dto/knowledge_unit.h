#ifndef KNOWLEDGE_UNIT_H
#define KNOWLEDGE_UNIT_H

#include <QDateTime>

struct KnowledgeUnit
{
    // 0ID, 1Question, 2Answer, 3PassingScore, 4PreviousScore,
    // 5TimesPracticed, 6InsertTime, 7FirstPracticeTime, 8LastPracticeTime, 9Deadline, 10ClientType, 11Maintype"
    int ID;
    QString Question;
    QString Answer;
    double PassingScore;
    double PreviousScore;
    double NewScore;
    int TimesPracticed;
    QDateTime InsertTime;
    QDateTime FirstPracticeTime;
    QDateTime LastPracticeTime;
    QDateTime Deadline;
    QString ClientName;
    QString Category;
    int timeUsedSec;
    QByteArray AnswerImageBytes;
    QByteArray QuestionImageBytes;
};

#endif // KNOWLEDGE_UNIT_H
