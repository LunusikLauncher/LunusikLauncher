#ifndef MINECRAFTMANAGER_H
#define MINECRAFTMANAGER_H

#include "libs.h"
#include "filescontrol.h"
#include "downloadmanager.h"

class MinecraftManager : public FilesControl {
    Q_OBJECT
public:
    explicit MinecraftManager(DownloadManager *DOWNLOAD_MANAGER, const gameParams &data, QWidget *mainwinow, QObject *parent = nullptr);
public slots:
    void startMinecraft();
signals:
    void hideWindow();
    void exit();
    void checkFiles(const QString text, const QString id, const QString name, const QString hashManifest, const QString url);
private:
    DownloadManager* DOWNLOAD_MANAGER;
    gameParams data;
    SystemConfig system;
    QString javaExe;
    bool checkRule(const QJsonObject &rule, const SystemConfig &system);
    QStringList processArguments(const QJsonArray &argsArray, const QMap<QString, QString> &vars, const SystemConfig &system);
    QString replaceVars(QString str, const QMap<QString, QString> &vars);
};
#endif