#ifndef FILESCONTROL_H
#define FILESCONTROL_H

#include "libs.h"


class FilesControl : public QObject {
    Q_OBJECT
public:
    explicit FilesControl(QObject *parent = nullptr);
public slots:
    
signals:
    
private:

protected:
    bool checkRules(const QJsonArray rules, const SystemConfig &system);
    bool isVersionOlderOrEqual(const QString &current, const QString &target);
    bool isExistsAndValid(QFile &file, const QString &hashFile);

};

#endif