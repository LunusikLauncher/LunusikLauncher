#include "minecraftmanager.h"
#include "mainwindow.h"

MinecraftManager::MinecraftManager(DownloadManager *DOWNLOAD_MANAGER, const gameParams &data, QWidget *mainwindow,  QObject *parent) : 
FilesControl(parent), 
DOWNLOAD_MANAGER(DOWNLOAD_MANAGER) {
    this->data = data;

    QString name;
    #ifdef Q_OS_WIN
        name = "windows";
        javaExe = "java.exe";
    #elif defined(Q_OS_LINUX)
        name = "linux";
        javaExe = "java";
    #elif defined(Q_OS_MAC)
        name = "macos";
        javaExe = "java";
    #endif

    QString arch;
    #if defined(Q_PROCESSOR_ARM_64)
        arch = "arm64";
    #elif defined(Q_PROCESSOR_X86_64)
        arch = "64";
    #elif defined(Q_PROCESSOR_X86_32)
        arch = "32";
    #endif


    system = {.name   = name,
                .version = QSysInfo::kernelVersion(),
                .arch    = arch};
    connect(this, &MinecraftManager::checkFiles, DOWNLOAD_MANAGER, &DownloadManager::downloadMinecraft);
    connect(this, &MinecraftManager::checkRustFiles, DOWNLOAD_MANAGER, &DownloadManager::downloadMinecraftRust);
    connect(this, &MinecraftManager::hideWindow, mainwindow, &QWidget::hide);
}


void MinecraftManager::startMinecraft(){
    QDir dir;
    QFile vm(data.gameDir + "/" + data.version + ".json");
    if (!vm.open(QIODevice::ReadOnly)){
        emit showNotification(QCoreApplication::translate("Error", "cant_read_file"), "Error");
        qWarning() << "Cant open version manifest";
        return;
    }
    QJsonObject versionManifest = QJsonDocument::fromJson(vm.readAll()).object();
    vm.close();
    const QString javaHome = QString("%1/Java/%2").arg(data.minecraftDir, versionManifest["javaVersion"].toObject()["component"].toString());
    const QString javaPath = javaHome + "/bin/" + javaExe;
    const bool isRust = data.versionType == "rust";
    if (isRust) {
        const QString uuid = QUuid::createUuid().toString();

        QString horizonClient;
        #ifdef Q_OS_WIN
            horizonClient = "horizon-client.exe";
        #else
            horizonClient = "horizon-client";
        #endif
        const QString localClient = data.gameDir + "/" + horizonClient;
        if (QFile::exists(localClient)) horizonClient = localClient;

        QJsonObject session;
        session["name"] = data.username;
        session["uuid"] = QUuid::createUuid().toString().remove("{").remove("}");
        if (!data.userType.isEmpty()) session["user_type"] = data.userType;
        const QByteArray sessionJson = QJsonDocument(session).toJson(QJsonDocument::Compact);

        QStringList horizonArgs;
        const QString preparedResourcesDir = data.minecraftDir + "/prepared_resources";
        horizonArgs << "--resources" << preparedResourcesDir
                    << "--game-dir"  << data.minecraftDir
                    << "--home"      << data.minecraftDir
                    << "--width"     << QString::number(data.width.toInt())
                    << "--height"    << QString::number(data.height.toInt())
                    << "--session-stdin";

        auto connection = std::make_shared<QMetaObject::Connection>();
        *connection = connect(DOWNLOAD_MANAGER, &DownloadManager::finished, this,
            [this, connection, javaHome, preparedResourcesDir, horizonClient, horizonArgs, sessionJson, uuid](bool success, const QString id) {
                if (id != uuid) return;
                if (!success) { disconnect(*connection); return; }

                emit hideWindow();
                prepareHorizonResources(preparedResourcesDir, javaHome);

                QProcess *process = new QProcess();
                QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
                env.insert("SHED_FORCE_NVIDIA", "1");
                env.insert("__NV_PRIME_RENDER_OFFLOAD", "1");
                env.insert("__GLX_VENDOR_LIBRARY_NAME", "nvidia");
                process->setProcessEnvironment(env);
                process->setWorkingDirectory(data.minecraftDir);
                process->start(horizonClient, horizonArgs);

                if (process->waitForStarted()) {
                    process->write(sessionJson);
                    process->closeWriteChannel();
                    process->waitForFinished(-1);
                    qDebug() << "Horizon STDOUT:" << process->readAllStandardOutput();
                    qDebug() << "Horizon STDERR:" << process->readAllStandardError();
                    if (process->exitCode() != 0)
                        qWarning() << "Horizon exited with code" << process->exitCode();
                } else {
                    emit showNotification(QCoreApplication::translate("Error", "cant_start_minecraft"), "Error");
                    qWarning() << "Cant start horizon-client:" << process->errorString();
                }

                process->deleteLater();
                emit exit();
                disconnect(*connection);
            });

        emit checkRustFiles(QCoreApplication::translate("MainWindow", "check_files") + " " + data.version, uuid, data.version, data.hashManifest,  data.urlManifest);
        return;
    }
    QString assetsIndex = versionManifest["assets"].toString();
    QFile am(data.minecraftDir + "/assets/indexes/" + assetsIndex + ".json");
    if (!am.open(QIODevice::ReadOnly)){
        emit showNotification(QCoreApplication::translate("Error", "cant_read_file"), "Error");
        qWarning() << "Cant open assets manifest";
        return;
    }
    QJsonObject assetsManifest = QJsonDocument::fromJson(am.readAll()).object();
    am.close();

    if(assetsManifest.contains("map_to_resources") && assetsManifest["map_to_resources"].toBool()){
        QJsonObject objects = assetsManifest["objects"].toObject();

        for (auto it = objects.begin(); it != objects.end(); ++it) {
            QString hash = it.value().toObject()["hash"].toString();
            QString firstTwo = hash.left(2);
            QString path = data.gameDir + "/resources/" + it.key();
            QFile file(path);

            if(isExistsAndValid(file, hash)){
                file.close();
                continue;
            }
            file.close();
            QFileInfo fileInfo(path);

            if(!dir.mkpath(fileInfo.absolutePath())){
                emit showNotification(QCoreApplication::translate("Error", "cant_create_dir"), "Error");
                qWarning() << "Error: cant create folders: " << fileInfo.absolutePath();
                continue;
            }


            if (!QFile::copy(data.minecraftDir + "/assets/objects/" + firstTwo + "/" + hash, path)) {
                emit showNotification(QCoreApplication::translate("Error", "cant_copy_file"), "Error");
                qWarning() << "Cant copy file";
            }
        }
    }
    
    const QJsonArray libraries = versionManifest["libraries"].toArray();


    QStringList librariesList;
    for (const QJsonValue &lib_val : libraries ){
        const QJsonObject lib = lib_val.toObject();
        const QJsonObject downloads = lib["downloads"].toObject();
        
        if(lib.contains("rules")){
            const QJsonArray rules = lib["rules"].toArray();
            if(!checkRules(rules, system)){
                continue;
            }
        }

        QString pathArtifact = data.minecraftDir;
        QString urlArtifact;

        if (downloads.contains("artifact")){
            const QJsonObject artifact = downloads["artifact"].toObject();
            pathArtifact.append("/libraries/").append(artifact["path"].toString());
            urlArtifact = artifact["url"].toString();
        }


        if(!urlArtifact.isEmpty()){
            if (!librariesList.contains(pathArtifact)) {
                librariesList.append(pathArtifact);
            }
        }
    }
    QString separator = QDir::listSeparator();
    QString librariesStr = librariesList.join(separator) + separator;
    const QString uuid = QUuid::createUuid().toString();
    librariesStr += data.gameDir + "/" + data.version + ".jar";
    QMap<QString, QString> vars = {
        {"auth_player_name", data.username},
        {"version_name", data.version},
        {"game_directory", data.minecraftDir},
        {"natives_directory", data.nativesDir},
        {"assets_root", data.assetsDir},
        {"game_assets", data.minecraftDir + "/resources"},
        {"assets_index_name", assetsIndex},
        {"auth_uuid", data.uuid},
        {"auth_access_token", data.accessToken},
        {"auth_session", data.accessToken},
        {"user_type", data.userType},
        {"version_type", data.versionType},
        {"user_properties", "{}"},

        {"launcher_name", data.launcherName},
        {"launcher_version", data.launcherVersion},

        {"classpath", librariesStr},

        {"resolution_width", data.width},
        {"resolution_height", data.height}
    };
    QStringList baseJvm = {
        QString("-Djava.library.path=%1").arg(data.nativesDir),
        "-Dfile.encoding=UTF-8",
        "-cp",
        librariesStr};
    QString mainClass = versionManifest["mainClass"].toString();
    QStringList command;
    command << "-Xmx" + data.RAM + "M" << "-Xms1024M" << "-Dminecraft.api.auth.host=http://localhost:0/"
 << "-Dminecraft.api.account.host=http://localhost:0/"
 << "-Dminecraft.api.session.host=http://localhost:0/"
 << "-Dminecraft.api.services.host=http://localhost:0/";
    if(versionManifest.contains("arguments")){
        QJsonObject args = versionManifest["arguments"].toObject();
        if (args.contains("jvm")){
            command << processArguments(args["jvm"].toArray(), vars, system);
        }else{
            command << baseJvm;
        }

        command << mainClass;

        if (args.contains("game")){
            command << processArguments(args["game"].toArray(), vars, system);
        }
    }else if(versionManifest.contains("minecraftArguments")){
        command << baseJvm;
        command << mainClass;
        command << replaceVars(versionManifest["minecraftArguments"].toString(), vars).split(" ");
    }
    auto connection = std::make_shared<QMetaObject::Connection>();
    *connection = connect(DOWNLOAD_MANAGER, &DownloadManager::finished, this, [this, connection, javaPath, command, uuid](bool success, const QString id) {
        if(id == uuid){
            if(success){
                emit hideWindow();
                QProcess *process = new QProcess();

                QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
                env.insert("SHED_FORCE_NVIDIA", "1");
                env.insert("__NV_PRIME_RENDER_OFFLOAD", "1"); 
                env.insert("__GLX_VENDOR_LIBRARY_NAME", "nvidia");

                process->setProcessEnvironment(env);
                process->setWorkingDirectory(data.minecraftDir);

                QFile::setPermissions(javaPath, QFileDevice::Permission(0x755));
                process->start(javaPath, command);
                
                if (process->waitForStarted()) {
                    process->waitForFinished(-1); 
                    QByteArray output = process->readAllStandardOutput();
                    QByteArray errors = process->readAllStandardError();
                    qDebug() << command;
                    qDebug() << "STDOUT:" << output;
                    qDebug() << "STDERR:" << errors; 
                }

                emit exit();
            }
            disconnect(*connection); 
        }
    });

    emit checkFiles(QCoreApplication::translate("MainWindow", "check_files") + " " + data.version, uuid, data.version, data.hashManifest, data.urlManifest);
}

QStringList MinecraftManager::processArguments(const QJsonArray &argsArray, const QMap<QString, QString> &vars, const SystemConfig &system) {
    QStringList result;

    for (const QJsonValue &val : argsArray) {
        if (val.isString()) {
            result << replaceVars(val.toString(), vars);
        } 
        else if (val.isObject()) {
            QJsonObject obj = val.toObject();
            bool allow = false;
            
            const QJsonArray rules = obj["rules"].toArray();
            for (const QJsonValue &r : rules) {
                if (r.toObject()["action"].toString() == "allow" && checkRule(r.toObject(), system)) {
                    allow = true;
                    break;
                }
            }

            if (allow) {
                QJsonValue value = obj["value"];
                if (value.isString()) {
                    result << replaceVars(value.toString(), vars);
                } else if (value.isArray()) {
                    for (const QJsonValue &v : value.toArray()) {
                        result << replaceVars(v.toString(), vars);
                    }
                }
            }
        }
    }
    return result;
}
void MinecraftManager::prepareHorizonResources(const QString &outputDir, const QString &javaHome) {
    QString prepareExe = "horizon-prepare";
    #ifdef Q_OS_WIN
        prepareExe += ".exe";
    #endif
    const QString localPrep = QDir(data.gameDir).filePath(prepareExe);
    if (QFile::exists(localPrep)) prepareExe = localPrep;

    QProcess checkProcess;
    QStringList checkArgs;
    checkArgs << "--check" << "--output" << outputDir;

    checkProcess.start(prepareExe, checkArgs);
    checkProcess.waitForFinished(-1);

    if (checkProcess.exitStatus() == QProcess::NormalExit && checkProcess.exitCode() == 0) {
        return;
    }
    
    QStringList args;
    args << "--prepare"
         << "--output" << outputDir
         << "--minecraft-dir" << data.minecraftDir
         << "--server-jar" << QDir(data.gameDir.chopped(5)).filePath("server-26.3.jar")
         << "--complete";

    if (!javaHome.isEmpty()) {
        args << "--java-home" << javaHome;
    }

    QProcess prepareProcess;
    prepareProcess.start(prepareExe, args);
    prepareProcess.waitForFinished(-1);
    qDebug() << "Prepare STDOUT:" << prepareProcess.readAllStandardOutput();
    qDebug() << "Prepare STDERR:" << prepareProcess.readAllStandardError();
    if (prepareProcess.exitCode() != 0) {
        qWarning() << "Horizon prepare failed:" << prepareProcess.readAllStandardError();
    }
}
bool MinecraftManager::checkRule(const QJsonObject &rule, const SystemConfig &system) {
    if (!rule.contains("os") && !rule.contains("features")) return true;

    if (rule.contains("os")) {
        const QJsonObject osRule = rule["os"].toObject();

        if (osRule.contains("name") && osRule["name"].toString().replace("osx", "macos") != system.version) return false;
        
        if (osRule.contains("arch") && osRule["arch"].toString().replace("86", "32").replace("x", "") != system.arch) return false;

        if(osRule.contains("version")){
            QRegularExpression re(osRule["version"].toString());
            return re.match(system.version).hasMatch();
        }
        if (osRule.contains("versionRange")) {
            QJsonObject range = osRule["versionRange"].toObject();
            if (range.contains("max")) {
                return isVersionOlderOrEqual(system.version, range["max"].toString());
            }
            if (range.contains("min")) {
                return isVersionOlderOrEqual(range["min"].toString(), system.version);
            }
        }
    }

    if (rule.contains("features") && !rule["features"].toObject().contains("has_custom_resolution")) {
        return false;
    }

    return true;
}

QString MinecraftManager::replaceVars(QString str, const QMap<QString, QString> &vars) {
    for (auto it = vars.begin(); it != vars.end(); ++it) {
        str.replace("${" + it.key() + "}", it.value());
    }
    return str;
}

