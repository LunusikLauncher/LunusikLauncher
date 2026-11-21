#include "filescontrol.h"

FilesControl::FilesControl(QObject *parent) : Manager(parent){}

bool FilesControl::isAlreadyExtracted(const QList<QZipReader::FileInfo> &entries, const QString &outputDir, const QString &prefix, bool flatExtract) {
    for (const auto &info : entries) {
        if (info.isDir) continue;

        QString rel = info.filePath;
        if (!prefix.isEmpty() && rel.startsWith(prefix))
            rel = rel.mid(prefix.size());
        if (rel.isEmpty()) continue;

        const QString target = flatExtract
            ? outputDir + "/" + QFileInfo(rel).fileName()
            : outputDir + "/" + rel;

        const QFileInfo fi(target);
        if (!fi.exists() || fi.size() != info.size)
            return false;
    }
    return true;
}
QString FilesControl::detectSingleRootPrefix(const QList<QZipReader::FileInfo> &entries)
{
    QString root;
    bool first = true;

    for (const QZipReader::FileInfo &info : entries) {
        QString p = info.filePath;
        p.replace('\\', '/');
        if (p.isEmpty()) continue;

        int slash = p.indexOf('/');
        QString top;
        if (slash < 0) {
            if (!info.isDir) return QString();
            top = p;
        } else {
            top = p.left(slash);
        }

        if (first) { root = top; first = false; }
        else if (top != root) return QString();
    }

    if (first || root.isEmpty()) return QString();
    return root + '/';
}
bool FilesControl::checkRules(const QJsonArray rules, const SystemConfig &system){
    bool isInstall = false;

    for (const QJsonValue &rule_val : rules) {
        const QJsonObject rule = rule_val.toObject();
        const QString action = rule["action"].toString();
        
        if (!rule.contains("os")) {
            if (action == "allow") isInstall = true;
            else if (action == "disallow") isInstall = false;
            continue;
        }

        const QJsonObject osRule = rule["os"].toObject();
        if(osRule.isEmpty()){
            if (action == "allow") isInstall = true;
            else if (action == "disallow") isInstall = false;
            continue;
        }

        QString osName = osRule["name"].toString();
        if(osName == "osx") osName = "macos";

        if (osName == system.name) {
            bool versionMatches = true;
            bool archMatches = true;

            if (osRule.contains("version")) {
                QRegularExpression re(osRule["version"].toString());
                versionMatches = re.match(system.version).hasMatch();
            }
            if (osRule.contains("versionRange")) {
                QJsonObject range = osRule["versionRange"].toObject();
                if (range.contains("max")) {
                    if (!isVersionOlderOrEqual(system.version, range["max"].toString())) 
                        versionMatches = false;
                }
                if (range.contains("min")) {
                    if (!isVersionOlderOrEqual(range["min"].toString(), system.version)) 
                        versionMatches = false;
                }
            }
            if (osRule.contains("arch")){
                QString osArch = osRule["arch"].toString();
                osArch.remove("x");
                osArch.replace("86", "32");

                archMatches = osArch == system.arch;
            }

            if (versionMatches && archMatches) {
                isInstall = (action == "allow");
            }
        }
    }
    return isInstall;
}
bool FilesControl::isVersionOlderOrEqual(const QString &current, const QString &target) {
    QStringList curParts = current.split('.');
    QStringList tarParts = target.split('.');
    for (int i = 0; i < qMax(curParts.size(), tarParts.size()); ++i) {
        int cur = (i < curParts.size()) ? curParts[i].toInt() : 0;
        int tar = (i < tarParts.size()) ? tarParts[i].toInt() : 0;
        if (cur < tar) return true;
        if (cur > tar) return false;
    }
    return true;
}
bool FilesControl::isExistsAndValid(QFile &file, const QString &hashFile){
    if (!file.exists()) {
        return false;
    }
    if (!hashFile.isNull() && !hashFile.isEmpty()){
        if (!file.open(QIODevice::ReadOnly)) {
            emit showNotification(QCoreApplication::translate("Error", "cant_read_file"), "Error");
            qWarning() << "Cant open file: " << file.fileName();
            return false;
        }
        QCryptographicHash hashGen(QCryptographicHash::Sha1); 
        
        char buffer[65536]; 
        while (!file.atEnd()) {
            qint64 readBytes = file.read(buffer, sizeof(buffer));
            if (readBytes <= 0) {
                if (file.error() != QFile::NoError) {
                    emit showNotification(QCoreApplication::translate("Error", "cant_read_file"), "Error");
                    qWarning() << "Read error:" << file.errorString();
                    return false;
                }
                break;
            }
            hashGen.addData(QByteArrayView(buffer, readBytes));
        }

        if (!hashGen.addData(&file)) { 
            return false;
        }
        if (hashGen.result().toHex().toLower() != hashFile.toLower()) {
            return false;
        }
    }
    return true;
}