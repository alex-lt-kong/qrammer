#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QDateTime>
#include <QFile>
#include <QDebug>

class TTSDownloader : public QObject
{
    Q_OBJECT
public:
    explicit TTSDownloader(QObject *parent = nullptr);

    void doDownload(QString url, QString fileName);

signals:

public slots:
    void replyFinished (QNetworkReply *reply);

private:
   QNetworkAccessManager *manager;
   QString fileName;
};

#endif // DOWNLOADER_H
