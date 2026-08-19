#ifndef MANAGER_H
#define MANAGER_H

#include "libs.h"

class Manager : public QObject {
Q_OBJECT
signals:
    void showNotification(const QString message, const QString type, const int duration = BASE_NOTIFICATION_DURATION);
public:
    explicit Manager(QObject *parent);
};

#endif