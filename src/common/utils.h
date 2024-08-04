#ifndef UTILS_H
#define UTILS_H

#include <QPixmap>

#include <string>

#define ANSWER_IMAGE_DIMENSION 640
#define QUESTION_IMAGE_DIMENSION 640

void execExternalProgramAsync(const std::string cmd);
QPixmap selectImageFromFileSystem();

#endif // UTILS_H
