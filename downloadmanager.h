#ifndef DOWNLOADMANAGER_H
#define DOWNLOADMANAGER_H

#include "libs.h"
#include "filescontrol.h"

class DownloadManager : public FilesControl {
    Q_OBJECT
public:
    explicit DownloadManager(const QString &minecraftDirectory = QString(), QObject *parent = nullptr);

public slots:
    void init();

    void setMinecraftDirectory(const QString minecraftDirectory);
    void downloadMinecraft(const QString text, const QString id, const QString name, const QString hashManifest, const QString url);
    void downloadFiles(const QList<DownloadTask> files, const QString text, const QString id);
    void cancelDownload();
    void updateVersions();
signals:
    void progressUpdated(qint64 percent);
    void statusTextChanged(const QString text);
    void showOrHideProgress(const bool show);
    void finished(bool success, const QString id);
    void finishedExtractNatives();
    void error(QString message);

    void extactNativesStart();
    void updatedVersions();
private:
    // --- INSTALLATION & DOWNLOADS ---
    QNetworkAccessManager *manager = nullptr;
    QString minecraftDirectory;
    bool isStopDownload = false;
    qint64 filesDownloadedSize;
    qint64 filesTotalSize;

    QList<QNetworkReply*> activeReplies;
    QEventLoop* currentLoop = nullptr;
    SystemConfig system;
    void extractFile(const QString &zipPath, const QString &outputDir, const QStringList &excludes);
    bool decompressFile(const QString &inputPath, const QString &outputPath);
    bool installOneFile(const QString &url, const QString &path, const QString &hashFile);
    bool installOneFile(const QString &url, const QString &path, QJsonObject *outJsonData = nullptr, const QString &hashFile = nullptr);
    void installMoreFiles(const DownloadTask dt, const QString id = nullptr);
    void setExecutable(const QString &filePath);
    void updateProgressBar(const QString &id);
    QList<DownloadTask> downloadJava(const QString name);
};
#endif