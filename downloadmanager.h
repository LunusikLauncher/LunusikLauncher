#ifndef DOWNLOADMANAGER_H
#define DOWNLOADMANAGER_H

#include "libs.h"
#include "filescontrol.h"

struct TarHeader {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag;
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    char pad[12];
};

class DownloadManager : public FilesControl {
    Q_OBJECT
public:
    explicit DownloadManager(const QString &minecraftDirectory = QString(), QObject *parent = nullptr);

public slots:
    void init();

    void setMinecraftDirectory(const QString minecraftDirectory);
    void downloadMinecraft(const QString text, const QString id, const QString name, const QString hashManifest, const QString url);
    void downloadMinecraftRust(const QString text, const QString id, const QString name, const QString hashManifest, const QString url);
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
    QString getHorizonSuffixes();
    QString getHashForFile(const QString &SHA256SUMS, const QString &targetFileName);
    bool extractTarGz(const QString &path, const QString &outputDir, const QStringList &excludes, bool flatExtract);
    bool extractFile(const QString &zipPath, const QString &outputDir, const QStringList &excludes, bool flatExtract = true);
    bool decompressFile(const QString &inputPath, const QString &outputPath);
    bool setExecutable(const QString &filePath);
    bool installOneFile(const QString &url, const QString &path, const QString &hashFile);
    bool installOneFile(const QString &url, const QString &path, QJsonObject *outJsonData = nullptr, const QString &hashFile = nullptr);
    void installMoreFiles(const DownloadTask dt, const QString id = nullptr);
    void updateProgressBar(const QString &id);
    void notificationAndCancel(const QString id, const QString message, const QString type, const int duration = BASE_NOTIFICATION_DURATION);
    void finish(const bool success, const QString id);
    QList<DownloadTask> createDownloadTasksMinecraft(const QString id, const QString name, const QString hashManifest, const QString url);
    QList<DownloadTask> createDownloadTasksJava(const QString name);
};
#endif