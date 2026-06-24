#include "filescontrol.h"

FilesControl::FilesControl(QObject *parent) : QObject(parent){}

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
        if (osRule["name"].toString().replace("osx", "macos") == system.name) {
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
                archMatches = osRule["arch"].toString().replace("86", "32").replace("x", "") == system.arch;
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
    if (!hashFile.isEmpty()){
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "Cant open file";
            return false;
        }
        QCryptographicHash hashGen(QCryptographicHash::Sha1); 
        if (!hashGen.addData(&file)) { 
            return false;
        }
        if (hashGen.result().toHex().toLower() != hashFile.toLower()) {
            return false;
        }
    }
    return true;
}