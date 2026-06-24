#include "versionsmanager.h"
#include "libs.h"


VersionsManager::VersionsManager(DownloadManager *DOWNLOAD_MANAGER, const QString &minecraftDirectory, QObject *parent) 
: QObject(parent), 
DOWNLOAD_MANAGER(DOWNLOAD_MANAGER) {
    this->minecraftDirectory = minecraftDirectory;
    connect(DOWNLOAD_MANAGER, &DownloadManager::updatedVersions, this, &VersionsManager::readVersions);
    connect(this, &VersionsManager::updateVersions, DOWNLOAD_MANAGER, &DownloadManager::updateVersions);
}
void VersionsManager::setMinecraftDirectory(const QString minecraftDirectory){
    this->minecraftDirectory = minecraftDirectory;
}


void VersionsManager::loadVersions(const bool isUpdateRequired) {
    if (isUpdateRequired || !readVersions()){
        emit updateVersions();
        return;
    }
}
bool VersionsManager::readVersions() {
    QFile file_versions("versions.llv");
    if (!file_versions.exists()) {
        qWarning() << "File versions.llv not found";
        return false;
    }
    if (!file_versions.open(QIODevice::ReadOnly)){
        qWarning() << "Failed to open file versions.llv:" << file_versions.errorString();
        return false;
    }
    
    QByteArray file_data = file_versions.readAll();
    file_versions.close();

    QJsonDocument doc = QJsonDocument::fromJson(file_data);
    if (doc.isNull()) {
        qWarning() << "Error parsing versions.llv";
        return false;
    }

    const QList<QString> installedVersions = getDownloadedVersions();
    const QList<QString> favoriteVersions = getFavoriteVersions();
    const QJsonObject root_obj = doc.object();
    QList<VersionData> versions_data;
    QList<LatestVersionData> latest_versions_data;

    if (root_obj.contains("latest_versions")) {
        QJsonObject latest = root_obj["latest_versions"].toObject();
        
        if (!latest.contains("release") || !latest.contains("snapshot")){
            qWarning() << "The data in versions.llv is incorrect.";
            return false;
        }

        QJsonValue release_val = latest["release"];
        QJsonValue snapshot_val = latest["snapshot"];
        
        if (!release_val.isArray() || !snapshot_val.isArray()){
            qWarning() << "The data in versions.llv is incorrect.";
            return false;
        }

        QJsonArray release = release_val.toArray();
        QJsonArray snapshot = snapshot_val.toArray();

        if (release.size() < 4 || snapshot.size() < 4){
            qWarning() << "The data in versions.llv is incorrect.";
            return false;
        }
        QString release_name = release.at(0).toString();
        QString snapshot_name = snapshot.at(0).toString();
        latest_versions_data.append({release_name,
                            "release",
                            release.at(2).toString(),
                            release.at(3).toString(), 
                            installedVersions.contains(release_name),
                            favoriteVersions.contains(release_name),
                            release.at(1).toString(),  
                            0});
        latest_versions_data.append({snapshot_name,
                            "snapshot",
                            snapshot.at(2).toString(),
                            snapshot.at(3).toString(),
                            installedVersions.contains(snapshot_name),
                            favoriteVersions.contains(snapshot_name),
                            snapshot.at(1).toString(),
                            1});
    }
    if (root_obj.contains("versions")){
        QJsonArray versions = root_obj["versions"].toArray();
        for(const QJsonValue &v : versions){
            if (!v.isArray()) {
                qWarning() << "The data in versions.llv is incorrect.";
                return false;
            }
            QJsonArray version = v.toArray();
            if (version.size() < 4) {
                qWarning() << "The data in versions.llv is incorrect.";
                return false;
            }
            QString version_name = version.at(0).toString(); 
            versions_data.append({version_name,
                        version.at(1).toString(),
                        version.at(2).toString(),
                        version.at(3).toString(),
                        installedVersions.contains(version_name),
                        favoriteVersions.contains(version_name)
            });
        }
    }
    emit renderVersions(versions_data, latest_versions_data);
    emit showWindow();
    return true;
}
const QList<QString> VersionsManager::getDownloadedVersions() {
    QList<QString> installedVersions;

    QDir rootDir(minecraftDirectory + "/versions");

    if (!rootDir.exists()) return installedVersions;
    QStringList versionDirs = rootDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    for (const QString &vName : versionDirs) {
        QDir vDir(rootDir.filePath(vName));
        
        bool hasClient = vDir.exists("client.jar");
        bool hasManifest = vDir.exists(vName + ".json");

        if (hasClient && hasManifest) {
            installedVersions << vName;
        }
    }
    return installedVersions;
}
const QList<QString> VersionsManager::getFavoriteVersions() {
    QList<QString> favoriteVersions;
    QSettings settings;

    favoriteVersions = settings.value("favoriteVersions").toStringList();
    
    return favoriteVersions;
}
