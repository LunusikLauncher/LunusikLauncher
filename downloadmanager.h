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
    void cancelDownload(const QString &id);
    void updateVersions();
signals:
    void progressUpdated(qint64 percent);
    void statusTextChanged(const QString text);
    void showOrHideProgress(const bool show);
    void finished(const bool success, const QString id);
    void finishedExtractNatives();

    void extactNativesStart();
    bool updatedVersions();
    void renderVersions(const QList<VersionData> versions, const QList<LatestVersionData> latestVersions);
private:
    // --- INSTALLATION & DOWNLOADS ---
    QNetworkAccessManager *manager = nullptr;
    QString minecraftDirectory;
    bool isBreakDownload = false;
    qint64 filesDownloadedSize;
    qint64 filesTotalSize;

    QHash<QString, QList<QNetworkReply*>> activeReplies;
    QEventLoop* currentLoop = nullptr;
    SystemConfig system;
    bool extractFile(const QString &zipPath, const QString &outputDir, const QStringList &excludes);
    bool decompressFile(const QString &inputPath, const QString &outputPath);
    bool setExecutable(const QString &filePath);
    bool installOneFile(const QString &url, const QString &path, const QString &hashFile);
    bool installOneFile(const QString &url, const QString &path, QJsonObject *outJsonData = nullptr, const QString &hashFile = nullptr);
    void installMoreFiles(const DownloadTask dt, const QString id = nullptr);
    void updateProgressBar(const QString &id);
    void notificationAndCancel(const QString id, const QString message, const QString type, const int duration = BASE_NOTIFICATION_DURATION);
    void finish(const bool success, const QString id);
    QList<DownloadTask> downloadJava(const QString name);
};
#endif