#include "downloader.h"

TTSDownloader::TTSDownloader(QObject *parent) : QObject(parent)
{

}

void TTSDownloader::doDownload(QString url, QString fileName)
{
    this->fileName = fileName;

    manager = new QNetworkAccessManager(this);

    connect(manager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(replyFinished(QNetworkReply*)));

    manager->get(QNetworkRequest(QUrl(url)));
}

void TTSDownloader::replyFinished(QNetworkReply *reply)
{
    if(reply->error()) {
        qDebug() << "replyFinished Error: " << reply->errorString();
    } else {
        QFile *file = new QFile(fileName);
        if(file->open(QFile::Append)) {
            file->write(reply->readAll());
            file->flush();
            file->close();
        }
        delete file;
    }

    reply->deleteLater();
    this->deleteLater();
}
