#ifndef VERSIONSMANAGER_H
#define VERSIONSMANAGER_H

#include "libs.h"
#include "downloadmanager.h"

class VersionsManager : public QObject {
    Q_OBJECT
public:
    explicit VersionsManager(DownloadManager *DOWNLOAD_MANAGER, const QString &minecraftDirectory = QString(), QObject *parent = nullptr);

public slots:
    void loadVersions(const bool isUpdateRequired = false);
    void setMinecraftDirectory(const QString minecraftDirectory);
signals:
    void updateVersions();
    void showWindow();
    void renderVersions(const QList<VersionData> versions, const QList<LatestVersionData> latestVersions);

private:
    QString minecraftDirectory;
    DownloadManager* DOWNLOAD_MANAGER;

    bool readVersions();
    const QList<QString> getDownloadedVersions();
    const QList<QString> getFavoriteVersions();
};

#endif