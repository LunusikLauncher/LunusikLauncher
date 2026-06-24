#ifndef LIBS_H
#define LIBS_H

#include <QSysInfo>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QComboBox>
#include <QSpinBox>
#include <QGroupBox>
#include <QProgressBar>
#include <QFormLayout>
#include <QLineEdit>
#include <QPainter>
#include <QLayout>
#include <QStyle>
#include <QPropertyAnimation>
#include <QGraphicsDropShadowEffect>
#include <QParallelAnimationGroup>
#include <QEvent>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>
#include <QTranslator>
#include <QCoreApplication>
#include <QTimer>
#include <QMessageBox>
#include <QSettings>
#include <QProcessEnvironment>
#include <QFileDialog>
#include <QThread>
#include <QMovie>
#include <QSplashScreen>
#include <private/qzipreader_p.h>
#include <QDesktopServices>
#include <lzma.h>
#include <QStackedWidget>
#if defined(Q_OS_WIN)
#include <windows.h>
#elif defined(Q_OS_LINUX)
#include <unistd.h>
#elif defined(Q_OS_MAC)
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

struct SystemConfig {
    QString name;
    QString version;
    QString arch;
};

struct VersionData {
    QString name;
    QString type;
    QString url;
    QString hash;

    bool isInstalled;
    bool isFavorite;
};
struct LatestVersionData : VersionData {
    QString date;
    int col;
};

struct DownloadTask { 
    QString url; 
    QString path; 
    QString hash;
    qint64 size;

    bool isExtract = false;
    bool isDecompress = false;
    QString hashDecompress;

    QString outputDir;
    QStringList excludes;

    bool isExecutable = false;
};

struct gameParams {
    QString minecraftDir;
    QString username;
    QString version;
    QString gameDir;
    QString nativesDir;
    QString assetsDir;
    QString assetsIndex;
    QString uuid;
    QString accessToken;
    QString userType;
    QString versionType;

    QString launcherName;
    QString launcherVersion;

    QString height;
    QString width;

    QString hashManifest;
    QString urlManifest;
    QString RAM;
};

#endif