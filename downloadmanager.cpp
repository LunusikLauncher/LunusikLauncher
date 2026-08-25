#include "downloadmanager.h"

DownloadManager::DownloadManager(const QString &minecraftDirectory, QObject *parent) 
: FilesControl(parent) {
    this->minecraftDirectory = minecraftDirectory;

    QString name;
    #ifdef Q_OS_WIN
        name = "windows";
    #elif defined(Q_OS_LINUX)
        name = "linux";
    #elif defined(Q_OS_MAC)
        name = "macos";
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
}
void DownloadManager::setMinecraftDirectory(const QString minecraftDirectory){
    this->minecraftDirectory = minecraftDirectory;
}

void DownloadManager::init(){
    manager = new QNetworkAccessManager();
}
void DownloadManager::finish(const bool success, const QString id) {
    activeReplies.remove(id);
    emit showOrHideProgress(false);
    emit statusTextChanged(QCoreApplication::translate("MainWindow", "install_disable"));
    emit finished(success, id);
    emit progressUpdated(0);
}
void DownloadManager::cancelDownload(const QString &id) {
    isBreakDownload = true;

    if (currentLoop && currentLoop->isRunning()) {
        currentLoop->quit();
    }

    QList<QNetworkReply*> repliesToAbort = activeReplies[id];
    for (QNetworkReply* reply : repliesToAbort) {
        if (reply && reply->isRunning()) {
            reply->abort();
        }
        reply->deleteLater();
    }
    
    finish(false, id);
    isBreakDownload = false;

}
void DownloadManager::notificationAndCancel(const QString id, const QString message, const QString type, const int duration) {
    cancelDownload(id);
    emit showNotification(message, type);
}
void DownloadManager::downloadMinecraft(const QString text, const QString id, const QString name, const QString hashManifest, const QString url) {
    if (isBreakDownload) return;

    emit showOrHideProgress(true);
    emit statusTextChanged(text);

    QDir dir;

    if(name.isEmpty() || url.isEmpty()){
        notificationAndCancel(id, QCoreApplication::translate("Error", "invalid_version_data"), "Error");
        qWarning() << "Error: invalid data";
        return;
    }

    QString pathVersion = "/versions/" + name + "/";
    QStringList dirs = { "/assets/indexes", "/assets/objects", "/libraries", pathVersion};
    for (const QString &subDir : dirs) {
        QString fullPath = minecraftDirectory + subDir;
        if (!dir.exists(fullPath)) {
            if (!dir.mkpath(fullPath)) {
                notificationAndCancel(id, QCoreApplication::translate("Error", "cant_create_dir"), "Error");
                qWarning() << "Error: can't create folder" << fullPath;
                return;
            }
        }
    }

    QJsonObject versionManifest;
    if(!installOneFile(url, minecraftDirectory + pathVersion + name + ".json", &versionManifest, hashManifest)){
        notificationAndCancel(id, QCoreApplication::translate("Error", "version_manifest").arg(name), "Error");
        qWarning() << "Loading error: cant install manifest for minecraft " << name;
        return;
    }

    QJsonObject assetsManifest;
    const QJsonObject assetIndex = versionManifest["assetIndex"].toObject();
    if(!installOneFile(assetIndex["url"].toString(), minecraftDirectory + "/assets/indexes/" + name + ".json", &assetsManifest, assetIndex["sha1"].toString())){
        notificationAndCancel(id, QCoreApplication::translate("Error", "assets_manifest").arg(name), "Error");
        qWarning() << "Loading error: cant install assets manifest for minecraft " << name;
        return;
    }

    const QJsonObject objects = assetsManifest["objects"].toObject();
    const QJsonArray libraries = versionManifest["libraries"].toArray();

    const QJsonObject client = versionManifest["downloads"].toObject()["client"].toObject();
    QList<DownloadTask> downloadTasks;
    downloadTasks.append({client["url"].toString(), minecraftDirectory + pathVersion + name + ".jar", client["sha1"].toString(), client["size"].toInt()});

    const QJsonObject server = versionManifest["downloads"].toObject()["server"].toObject();
    downloadTasks.append({server["url"].toString(), minecraftDirectory + pathVersion + "server-" + name + ".jar", server["sha1"].toString(), server["size"].toInt()});

    QString pathToNatives = minecraftDirectory + pathVersion + "natives";

    for (const QJsonValue &lib_val : libraries ){
        const QJsonObject lib = lib_val.toObject();
        const QJsonObject downloads = lib["downloads"].toObject();
        
        if(lib.contains("rules")){
            const QJsonArray rules = lib["rules"].toArray();
            if(!checkRules(rules, system)){
                continue;
            }
        }
        qint64 sizeFile = 0;
        QString pathArtifact = minecraftDirectory;
        QString urlArtifact;
        QString hashArtifact;

        QString pathClassifiers = minecraftDirectory;
        QString urlClassifiers;
        QString hashClassifiers;


        if (downloads.contains("artifact")){
            const QJsonObject artifact = downloads["artifact"].toObject();
            pathArtifact.append("/libraries/").append(artifact["path"].toString());
            urlArtifact = artifact["url"].toString();
            hashArtifact = artifact["sha1"].toString();
            sizeFile = artifact["size"].toInt();
        }
        
        if(downloads.contains("classifiers")){
            const QJsonObject classifiers = downloads["classifiers"].toObject();
            QString classifierKey = lib["natives"].toObject()[system.name].toString();
            if (classifierKey.contains("${arch}")) {
                classifierKey.replace("${arch}", system.arch);
            }
            if(classifiers.contains(classifierKey)){
                const QJsonObject natives = classifiers[classifierKey].toObject();
                pathClassifiers.append("/libraries/").append(natives["path"].toString());
                urlClassifiers = natives["url"].toString();
                hashClassifiers = natives["sha1"].toString();
                sizeFile = natives["size"].toInt();
            }
        }


        if(!urlArtifact.isEmpty()){
            downloadTasks.append({urlArtifact, pathArtifact, hashArtifact, sizeFile});
        }
        if(!urlClassifiers.isEmpty()){
            QStringList excludes;
            if (lib.contains("extract")){
                QJsonObject extract = lib["extract"].toObject();
                if(extract.contains("exclude")){
                    QJsonArray exclude = extract["exclude"].toArray();
                    for(QJsonValue e : exclude){
                        excludes.append(e.toString());
                    }
                }
            }
            downloadTasks.append({urlClassifiers, pathClassifiers, hashClassifiers, sizeFile, true, false, "", pathToNatives, excludes});
        }
    }
    for (auto it = objects.begin(); it != objects.end(); ++it) {
        QString fileName = it.key();
        QJsonObject data = it.value().toObject();
        QString hash = data["hash"].toString();
        qint64 size = data["size"].toVariant().toLongLong();
        QString firstTwo = hash.left(2);
        QString url = "https://resources.download.minecraft.net/" + firstTwo + "/" + hash;
        
        QString path = minecraftDirectory + "/assets/objects/" + firstTwo + "/" + hash; 
        downloadTasks.append({url, path, hash, size});
    }
    downloadTasks << downloadJava(versionManifest["javaVersion"].toObject()["component"].toString());

    downloadFiles(downloadTasks, text, id);
}

QList<DownloadTask> DownloadManager::downloadJava(const QString name){
    QList<DownloadTask> downloadTasks;

    if (isBreakDownload) return downloadTasks;
    const QString urlManifest = "https://launchermeta.mojang.com/v1/products/java-runtime/2ec0cc96c44e5a76b9c8b7c39df7210883d12871/all.json";
    const QString hashManifest = "0f47bc501bbc5009f34bdedb5a232d2ecce640fa";
    QJsonObject manifest;

    if(!installOneFile(urlManifest, minecraftDirectory + "/Java/javaManifest.json", &manifest, hashManifest)) {
        emit showNotification(QCoreApplication::translate("Error", "java_versions"), "Error");
        qWarning() << "Loading error: cant install versions manifest Java";
        return downloadTasks;
    }

    if(name.isEmpty()){
        emit showNotification(QCoreApplication::translate("Error", "invalid_java_data"), "Error");
        qWarning() << "Error: invalid data";
        return downloadTasks;
    }
    QString platformKey;
    
    if (system.name == "windows") {
        platformKey = (system.arch == "64") ? "windows-x64" : "windows-x86";
    }
    else if (system.name == "linux") {
        platformKey = (system.arch == "64") ? "linux" : "linux-i386";
    }
    else if (system.name == "macos") {
        platformKey = (system.arch == "arm64") ? "mac-os-arm64" : "mac-os";
    }
    else{
        platformKey = "gamecore";
        emit showNotification(QCoreApplication::translate("Error", "unknown_system"), "Error");
        qWarning() << "Error: unknown system";
        return downloadTasks;
    }

    const QJsonObject dataJavaManifest = manifest[platformKey].toObject()[name].toArray()[0].toObject()["manifest"].toObject();
    const QString urlJavaManifest = dataJavaManifest["url"].toString();
    const QString hashJavaManifest = dataJavaManifest["sha1"].toString();
    QJsonObject javaManifes;
    const QString javaDir = minecraftDirectory + "/Java/" + name + "/";

    if(!installOneFile(urlJavaManifest, javaDir + "manifest.json", &javaManifes, hashJavaManifest)) { 
        emit showNotification(QCoreApplication::translate("Error", "java_manifest"), "Error");
        qWarning() << "Loading error: cant install manifest for Java" << name;
        return downloadTasks;
    }
    
    const QJsonObject files = javaManifes["files"].toObject(); 
    for (auto it = files.begin(); it != files.end(); ++it) {
        QJsonObject dataFile = it.value().toObject();
        if(dataFile["type"].toString() != "file") continue;

        bool executable = dataFile["executable"].toBool();
        dataFile = dataFile["downloads"].toObject();
        bool isLzma = dataFile.contains("lzma");

        QString hashDecompress;
        if (isLzma){
            hashDecompress = dataFile["raw"].toString();
            dataFile = dataFile["lzma"].toObject();
        }else{
            dataFile = dataFile["raw"].toObject();
        }

        QString hash = dataFile["hash"].toString();
        QString url = dataFile["url"].toString();
        qint64 size = dataFile["size"].toVariant().toLongLong();

        QString path = it.key(); 
        if (isLzma){
            downloadTasks.append({url, javaDir + "lzma/" + path, hash, size, false, true, hashDecompress, javaDir + path, QStringList(), executable});
        }
        else{
            downloadTasks.append({url, javaDir + path, hash, size});
        }
    }
    return downloadTasks;
}
void DownloadManager::downloadFiles(const QList<DownloadTask> files, const QString text, const QString id){
    if(files.isEmpty()){
        finish(true, id);
        return;
    }
    emit showOrHideProgress(true);
    emit statusTextChanged(text);
    filesDownloadedSize = 0;
    filesTotalSize = 0;
    for(const DownloadTask &dt : files){
        filesTotalSize += dt.size;
    }
    for(const DownloadTask &dt : files){
        installMoreFiles(dt, id);
    }
}
bool DownloadManager::extractFile(const QString &path, const QString &outputDir, const QStringList &excludes) {
    QDir dir;
    if(!dir.mkpath(outputDir)){
        emit showNotification(QCoreApplication::translate("Error", "cant_create_dir"), "Error");
        qWarning() << "Error: cant create folder: " << outputDir;
        return false;
    }
    
    QZipReader zip(path);
    if (zip.status() != QZipReader::NoError) {
        emit showNotification(QCoreApplication::translate("Error", "cant_open_jar"), "Error");
        qWarning() << "Cant open this jar: " << path;

        zip.close();
        return false;
    }

    for (const QZipReader::FileInfo &info : zip.fileInfoList()) {
        if (info.isDir) continue;

        bool skip = false;
        for (const QString &exclude : excludes) {
            if (info.filePath.startsWith(exclude)) {
                skip = true;
                break;
            }
        }
        if (skip) continue;


        QString fileNameOnly = QFileInfo(info.filePath).fileName();

        QFile file(outputDir + "/" + fileNameOnly);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(zip.fileData(info.filePath));
            file.close();
        }
        else {
            emit showNotification(QCoreApplication::translate("Error", "cant_write_file"), "Error");
            qWarning() << "Cant write to file: " << info.filePath;

            zip.close();
            return false;
        }
    }
    zip.close();
    return true;
}
bool DownloadManager::decompressFile(const QString &inputPath, const QString &outputPath) {
    QFile inFile(inputPath);
    
    if (!inFile.open(QIODevice::ReadOnly)) {
        emit showNotification(QCoreApplication::translate("Error", "cant_read_file"), "Error");
        qWarning() << "Cant read file: " << inputPath;
        return false;
    }

    QByteArray compressedData = inFile.readAll();
    inFile.close();

    lzma_stream strm = LZMA_STREAM_INIT;
    lzma_ret ret = lzma_alone_decoder(&strm, UINT64_MAX);

    if (ret != LZMA_OK) {
        emit showNotification(QCoreApplication::translate("Error", "lzma_decode_error"), "Error");
        qWarning() << "LZMA not ok: " << inputPath;
        return false;
    }

    strm.next_in = reinterpret_cast<const uint8_t*>(compressedData.constData());
    strm.avail_in = compressedData.size();

    QByteArray decompressedData;
    uint8_t outbuf[8192];

    do {
        strm.next_out = outbuf;
        strm.avail_out = sizeof(outbuf);
        ret = lzma_code(&strm, LZMA_RUN);
        
        if (ret != LZMA_OK && ret != LZMA_STREAM_END) {
            lzma_end(&strm);
            emit showNotification(QCoreApplication::translate("Error", "lzma_decode_error"), "Error");
            qWarning() << "LZMA error: Decompression failed for file: " << inputPath;
            return false;
        }

        size_t writeSize = sizeof(outbuf) - strm.avail_out;
        decompressedData.append(reinterpret_cast<const char*>(outbuf), writeSize);
        
    } while (ret != LZMA_STREAM_END);

    lzma_end(&strm);

    QFile outFile(outputPath);
    QFileInfo outFileInfo(outputPath);
    if(!QDir().mkpath(outFileInfo.absolutePath())){
        emit showNotification(QCoreApplication::translate("Error", "cant_create_dir"), "Error");
        qWarning() << "Error: cant create folders: " << outFileInfo.absolutePath();
        return false;
    }
    if (!outFile.open(QIODevice::WriteOnly)) {
        emit showNotification(QCoreApplication::translate("Error", "cant_write_file"), "Error");
        qWarning() << "Cant write file: " << outputPath;
        return false;
    }
    outFile.write(decompressedData);
    outFile.close();

    return true;
}
bool DownloadManager::setExecutable(const QString &filePath) {
    QFile file(filePath);

    if (!file.exists()) {
        emit showNotification(QCoreApplication::translate("Error", "exec_file_missing"), "Error");
        qWarning() << "File not found: " << filePath;
        return false;
    }
    QFile::Permissions permissions = file.permissions();
    permissions |= QFileDevice::ExeOwner | QFileDevice::ExeGroup | QFileDevice::ExeOther;

    if (!file.setPermissions(permissions)) {
        emit showNotification(QCoreApplication::translate("Error", "cant_set_executable"), "Error");
        qWarning() << "Failed to set permissions: " << filePath;
        return false;
    }
    return true;
}
bool DownloadManager::installOneFile(const QString &url, const QString &path, const QString &hashFile) {
    return installOneFile(url, path, nullptr, hashFile);
}
bool DownloadManager::installOneFile(const QString &url, const QString &path, QJsonObject *outJsonData, const QString &hashFile) {
    if (isBreakDownload) return false;
    QDir dir;
    QFileInfo fileinfo(path);
    QFile file(path);
    file.close();
    if(isExistsAndValid(file, hashFile)){
        if (outJsonData){
            if (file.isOpen()){
                file.seek(0);
            }
            else if(!file.open(QIODevice::ReadOnly)){
                return false;
            }
            *outJsonData = QJsonDocument::fromJson(file.readAll()).object();
        }
        file.close();
        return true;
    }
    file.close();
    if(!dir.mkpath(fileinfo.absolutePath())){
        emit showNotification(QCoreApplication::translate("Error", "cant_create_dir"), "Error");
        qWarning() << "Error: cant create folders: " << fileinfo.absolutePath();
        return false;
    }
    
    QNetworkReply* reply = manager->get(QNetworkRequest(QUrl(url)));
    QEventLoop loop;
    this->currentLoop = &loop; 

    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    this->currentLoop = nullptr;
    if (isBreakDownload) {
        reply->abort();
        reply->deleteLater();
        return false;
    }

    if (reply->error() != QNetworkReply::NoError) {
        emit showNotification(reply->errorString(), "Error");
        qWarning() << "Loading error:" << reply->errorString();
        reply->deleteLater();
        return false;
    }

    QByteArray responseData = reply->readAll();
    reply->deleteLater();
    if (responseData.isNull()) {
        emit showNotification(QCoreApplication::translate("Error", "empty_server_response"), "Error");
        qWarning() << "Error: responseData is null";
        return false;
    }
    if (!file.open(QIODevice::WriteOnly)) {
        emit showNotification(QCoreApplication::translate("Error", "cant_write_file"), "Error");
        qWarning() << "Error: cant open file " << fileinfo.fileName();
        return false;
    }
    if (outJsonData){
        *outJsonData = QJsonDocument::fromJson(responseData).object();
    }
    file.write(responseData);
    file.close();
    return true;
}
void DownloadManager::installMoreFiles(const DownloadTask dt, const QString id){
    if (isBreakDownload) return;
    QDir dir;
    QFileInfo fileinfo(dt.path);
    QFile file(dt.path);

    if(isExistsAndValid(file, dt.hash)) {
        if (dt.isExtract) {
            if(!extractFile(dt.path, dt.outputDir, dt.excludes)) {
                cancelDownload(id);
                return;
            }
        }
        if (dt.isDecompress) {
            QFile fileDecompress(dt.outputDir);
            if (!isExistsAndValid(fileDecompress, dt.hashDecompress)){
                if(!decompressFile(dt.path, dt.outputDir)) {
                    cancelDownload(id);
                    return;
                }
            }
        }
        if (dt.isExecutable) {
            if(!setExecutable(dt.path)) {
                cancelDownload(id);
                return;
            }
        }
        filesDownloadedSize += dt.size;
        updateProgressBar(id);
        file.close();
        return;
    }
    file.close();
    if(!dir.mkpath(fileinfo.absolutePath())) {
        notificationAndCancel(id, QCoreApplication::translate("Error", "cant_create_dir"), "Error");
        qWarning() << "Error: cant create folders: " << fileinfo.absolutePath();
        return;
    }

    QNetworkRequest request((QUrl(dt.url)));
    QNetworkReply *reply = manager->get(request);
    activeReplies[id].append(reply);
    connect(reply, &QNetworkReply::finished, this, [this, reply, dt, id]() {
        activeReplies[id].removeOne(reply);
        if(isBreakDownload){
            reply->deleteLater(); 
            return;
        }
        if (reply->error() == QNetworkReply::NoError) {
            QFile outFile(dt.path);
            if (outFile.open(QIODevice::WriteOnly)) {
                outFile.write(reply->readAll());
                outFile.close();
                if (dt.isExtract) {
                    if(!extractFile(dt.path, dt.outputDir, dt.excludes)) {
                        cancelDownload(id);
                        return;
                    }
                }
                if (dt.isDecompress) {
                    if(!decompressFile(dt.path, dt.outputDir)) {
                        cancelDownload(id);
                        return;
                    }
                }
                if (dt.isExecutable) {
                    if(!setExecutable(dt.path)) {
                        cancelDownload(id);
                        return;
                    }
                }
                filesDownloadedSize += dt.size;
                updateProgressBar(id);
            }else{
                notificationAndCancel(id, QCoreApplication::translate("Error", "cant_write_file"), "Error");
                qWarning() << "Error: cant write data to file";
                return;
            }
        } else {
            if(reply->error() == QNetworkReply::OperationCanceledError) { return; }
            notificationAndCancel(id, reply->errorString(), "Error");
            qWarning() << "Error:" << reply->errorString();

            return;
        }
        reply->deleteLater();
    });
}

void DownloadManager::updateProgressBar(const QString &id){
    quint64 prc = filesTotalSize > 0 ? filesDownloadedSize * 1000/filesTotalSize : 1000;
    if(filesDownloadedSize >= filesTotalSize){
        finish(true, id);
        return;
    }
    emit progressUpdated(prc);
}

void DownloadManager::updateVersions() {
    QString url_versions = "https://launchermeta.mojang.com/mc/game/version_manifest_v2.json";
    QNetworkReply* reply = manager->get(QNetworkRequest(QUrl(url_versions)));

    connect(reply, &QNetworkReply::finished, this, [this, reply](){
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            emit showNotification(reply->errorString(), "Error");
            qWarning() << "Loading error:" << reply->errorString();
            if (!emit updatedVersions()) {emit renderVersions({}, {});}
            return;
        }

        QByteArray responseData = reply->readAll();
        QJsonDocument json_doc = QJsonDocument::fromJson(responseData);
        
        if (json_doc.isNull()) {
            emit showNotification(QCoreApplication::translate("Error", "bad_json_data"), "Error");
            qWarning() << "JSON is null";
            if (!emit updatedVersions()) {emit renderVersions({}, {});}
            return;
        }

        QFile file_versions("versions.llv");
        if (!file_versions.open(QIODevice::WriteOnly)) {
            emit showNotification(QCoreApplication::translate("Error", "cant_save_cache"), "Error");
            qWarning() << "Cant open versions file";
            if (!emit updatedVersions()) {emit renderVersions({}, {});}
            return;
        }
        
        QJsonObject doc;
        const QJsonObject json_data = json_doc.object();
        const QJsonArray versions_array = json_data["versions"].toArray();
        QVariantMap latest_versions;
        QJsonArray versions;
        const QJsonObject latest = json_data["latest"].toObject();
        const QString latest_release_id = latest["release"].toString();
        const QString latest_snapshot_id = latest["snapshot"].toString();

        for (const QJsonValue &value : versions_array) {
            QJsonObject v = value.toObject();
            const QString id = v["id"].toString();
            bool is_latest = false;
            if (id == latest_release_id) {
                latest_versions["release"] = QVariantList{id, 
                    QDateTime::fromString(v["releaseTime"].toString(), Qt::ISODate).toString("yyyy/MM/dd"), 
                    v["url"].toString(),
                    v["sha1"].toString()};
                is_latest = true;
            }
            if (id == latest_snapshot_id) {
                latest_versions["snapshot"] = QVariantList{id, 
                    QDateTime::fromString(v["releaseTime"].toString(), Qt::ISODate).toString("yyyy/MM/dd"),
                    v["url"].toString(),
                    v["sha1"].toString()};
                is_latest = true;
            }
            if (is_latest){
                continue;
            }
            versions.append(QJsonArray{
                id,
                v["type"].toString(),
                v["url"].toString(),
                v["sha1"].toString()
            });
        }
        doc["versions"] = versions;
        doc["latest_versions"] = QJsonObject::fromVariantMap(latest_versions);
        file_versions.write(QJsonDocument(doc).toJson(QJsonDocument::Indented));
        file_versions.close();

        emit updatedVersions();
    });
}

