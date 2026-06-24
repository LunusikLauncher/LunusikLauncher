#include "mainwindow.h"
#include "filescontrol.h"

MainWindow::MainWindow(QSplashScreen *splash, QWidget *parent) 
    : QMainWindow(parent){

    DOWNLOAD_MANAGER = new DownloadManager();
    VERSIONS_MANAGER = new VersionsManager(DOWNLOAD_MANAGER);

    
    threadDownloadManager = new QThread();
    threadVersionsManager = new QThread();
    
    DOWNLOAD_MANAGER->moveToThread(threadDownloadManager);
    VERSIONS_MANAGER->moveToThread(threadVersionsManager);

    connect(VERSIONS_MANAGER, &VersionsManager::renderVersions, this, &MainWindow::renderVersions);
    connect(this, &MainWindow::setMinecraftDirectoryVM, VERSIONS_MANAGER, &VersionsManager::setMinecraftDirectory);
    connect(this, &MainWindow::loadVersions, VERSIONS_MANAGER, &VersionsManager::loadVersions);

    connect(threadDownloadManager, &QThread::started, DOWNLOAD_MANAGER, &DownloadManager::init);
    connect(DOWNLOAD_MANAGER, &DownloadManager::statusTextChanged, this, &MainWindow::updateProgressBarTitle);
    connect(DOWNLOAD_MANAGER, &DownloadManager::progressUpdated, this, &MainWindow::updateProgress);
    connect(DOWNLOAD_MANAGER, &DownloadManager::showOrHideProgress, this, &MainWindow::showOrHideProgress);
    connect(this, &MainWindow::downloadMinecraft, DOWNLOAD_MANAGER, &DownloadManager::downloadMinecraft);
    connect(this, &MainWindow::setMinecraftDirectoryDM, DOWNLOAD_MANAGER, &DownloadManager::setMinecraftDirectory);
    threadDownloadManager->start();
    threadVersionsManager->start();

    setupStyles();
    readSettings();
    
    auto *centralWidget = new QWidget(this);
    auto *WindowLayout = new QVBoxLayout(centralWidget);
    WindowLayout->setSpacing(0);
    WindowLayout->setContentsMargins(0, 0, 0, 0);
    setCentralWidget(centralWidget);
    auto *mainContent = new QWidget();
    mainContent->setObjectName("mainContainer"); 
    auto *centerWrapper = new QHBoxLayout();
    centerWrapper->addStretch(1);
    centerWrapper->addWidget(mainContent, 8);
    centerWrapper->addStretch(1);
    WindowLayout->addLayout(centerWrapper);
    auto *mainLayout = new QVBoxLayout(mainContent);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(25);

    // --- HEADER ---
    auto *headerWidget = new QWidget();
    headerWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    headerWidget->setObjectName("header");

    auto *header = new QHBoxLayout(headerWidget);
    header->setContentsMargins(0, 20, 10, 20);

    auto *iconBox = new QWidget();
    iconBox->setFixedSize(40, 40);
    iconBox->setObjectName("logoIconContainer");
    iconBox->setGraphicsEffect(createGlow(15, QColor(138, 43, 226, 200)));


    auto *iconLabel = new QLabel(iconBox);
    iconLabel->setPixmap(QIcon(":/icons/logo.svg").pixmap(scalingIcon(24), scalingIcon(24)));
    iconLabel->setAlignment(Qt::AlignCenter);


    auto *iconLayout = new QVBoxLayout(iconBox);
    iconLayout->addWidget(iconLabel, 0, Qt::AlignCenter);
    iconLayout->setContentsMargins(0, 0, 0, 0);


    logoText = new GradientLabel("", QColor("#b967ff"), QColor("#8a2be2"));
    logoText->setStyleSheet("font-size: 24px; font-weight: bold; color: white; margin-left: 10px;");
    logoText->setGraphicsEffect(createGlow(15, QColor(138, 43, 226, 200)));

    auto *logoContainer = new QHBoxLayout(); 
    logoContainer->setContentsMargins(0, 0, 0, 0);
    logoContainer->setSpacing(10);
    logoContainer->addWidget(iconBox);
    logoContainer->addWidget(logoText);
    logoContainer->addStretch();
    
    iconBox->setStyleSheet(R"(
        #logoIconContainer {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, 
                                        stop:0 #8a2be2, stop:1 #6a0dad);
            border-radius: 10px;
        }
    )");


    auto *userPanel = new QHBoxLayout();
    
    usernameLabel = new QLabel();
    usernameLabel->setStyleSheet(R"(
        QLabel { 
            color: #ece2ff;
            font-size: 14px; 
            font-weight: 600;
        }
    )");
    usernameEL = new EditableLabel();
    usernameEL->setStyleSheet(R"(
        QLabel, QLineEdit{
            color: #e0d6f2;
            margin-right: 10px; 
            font-size: 15px; 
            font-weight: 500;
        } 
        QLabel:hover, QLineEdit:hover{ 
            color: #c2acec; 
            text-decoration: underline; 
        }
    )");
    usernameEL->setText(username);
    connect(usernameEL, &EditableLabel::textUpdate, this, &MainWindow::changeUsername);

    auto *settingsBtn = new QPushButton(); 
    connect(settingsBtn, &QPushButton::clicked, this, [=]() {
        SettingsOverlay->show();
        SettingsOverlay->raise();
    });
    settingsBtn->setObjectName("settingsBtn");
    settingsBtn->setStyleSheet(R"(
        QPushButton#settingsBtn{
            background: transparent;
            background-color: transparent;
            border: none;
            background-image: url(:/icons/settings.svg);
            background-repeat: no-repeat;
            background-position: center;
        }
        QPushButton#settingsBtn:hover{
            background-image: url(:/icons/settings_hover.svg);
        }
    )");
    
    settingsBtn->setFixedSize(30, 30);
    settingsBtn->setCursor(Qt::PointingHandCursor);
    
    auto *openFolderBtn = new QPushButton(); 
    connect(openFolderBtn, &QPushButton::clicked, this, [=]() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(minecraftDirectory));
    });
    openFolderBtn->setObjectName("openFolderBtn");
    openFolderBtn->setStyleSheet(R"(
        QPushButton {
            border: none;
            background-color: transparent;
            background-image: url(:/icons/folder.svg);
            background-repeat: no-repeat;
            background-position: center;
        }
        QPushButton:hover {
            background-image: url(:/icons/folder_hover.svg);
        }
    )");
    openFolderBtn->setCursor(Qt::PointingHandCursor);
    openFolderBtn->setFixedSize(30, 30);

    auto *showInfoBtn = new QPushButton(); 
    connect(showInfoBtn, &QPushButton::clicked, this, &MainWindow::showInfo);
    showInfoBtn->setStyleSheet(R"(
        QPushButton {
            border: none;
            background-color: transparent;
            background-image: url(:/icons/info.svg);
            background-repeat: no-repeat;
            background-position: center;
        }
        QPushButton:hover {
            background-image: url(:/icons/info_hover.svg);
        }
    )");
    showInfoBtn->setCursor(Qt::PointingHandCursor);
    showInfoBtn->setFixedSize(30, 30);

    userPanel->addWidget(usernameLabel);
    userPanel->addWidget(usernameEL);
    userPanel->addWidget(settingsBtn);
    userPanel->addWidget(openFolderBtn);
    userPanel->addWidget(showInfoBtn);
    header->addLayout(logoContainer);
    header->addStretch();
    header->addLayout(userPanel);
    mainLayout->addWidget(headerWidget);

    // --- MAIN CONTENT ---
    auto *contentLayout = new QHBoxLayout();

    auto *versionsContainer = new QWidget();;
    versionsContainer->setObjectName("versions-section");

    auto *versionsSection = new QVBoxLayout(versionsContainer);
    versionsSection->setContentsMargins(20, 20, 20, 20);

    auto *versionsSectionHead = new QHBoxLayout();
    versionsSectionHead->setSpacing(10);
    versionsSectionHead->setContentsMargins(0, 0, 0, 0);

    auto *iconVersions = new QLabel();
    iconVersions->setPixmap(QIcon(":/icons/versions.svg").pixmap(20, 20));
    iconVersions->setAlignment(Qt::AlignCenter);
    versionsTitleText = new QLabel();
    versionsTitleText->setObjectName("section-title");

    auto* btnUpdateVersions = new QPushButton();

    connect(btnUpdateVersions, &QPushButton::clicked, this, [this]() {
        emit loadVersions(true); 
    });

    btnUpdateVersions->setObjectName("btnUpdateVersions");
    btnUpdateVersions->setStyleSheet(R"(
        QPushButton#btnUpdateVersions{
            background: transparent;
            background-color: transparent;
            border: none;
            background-image: url(:/icons/update_versions.svg);
            background-repeat: no-repeat;
            background-position: center;
        }
    )");
    btnUpdateVersions->setFixedSize(30, 30);
    btnUpdateVersions->setCursor(Qt::PointingHandCursor);
    
    versionsSectionHead->addWidget(iconVersions);
    versionsSectionHead->addWidget(versionsTitleText);
    versionsSectionHead->addStretch();
    versionsSectionHead->addWidget(btnUpdateVersions);
    versionsSectionHead->setObjectName("versionsSectionHead");
    versionsSection->addLayout(versionsSectionHead);

    auto *latestVersionsBox = new QFrame();
    latestVersionsBox->setAttribute(Qt::WA_StyledBackground);
    latestVersionsBox->setObjectName("latestVersionsBox");
    latestVersionsBox->setStyleSheet(R"(
        #latestVersionsBox {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, 
                                        stop:0 #4c1d95, stop:1 #6d28d9);
            border-radius: 12px;
            margin-bottom: 20px;
        }
    )");
    latestVersionsLayout = new QGridLayout(latestVersionsBox);
    latestVersionsLayout->setContentsMargins(20, 20, 20, 20);
    latestVersionsLayout->setSpacing(15);
    latestVersionsLayout->setColumnStretch(0, 1);
    latestVersionsLayout->setColumnStretch(1, 1);
    versionsSection->addWidget(latestVersionsBox);

    auto *versionList = new QScrollArea();
    versionList->setStyleSheet(R"(        
        QScrollBar:vertical {
            border: none;
            background: #2d1b3b;
            width: 10px;
            margin: 0px;
            border-radius: 5px;
        }
    )");

    auto *VC = new QWidget();
    VC->setStyleSheet(R"(
        QWidget#versionItem {
            background: rgba(53, 28, 90, 0.5); 
            padding: 12px 15px;
            border-radius: 8px;
            border: 1px solid transparent;
        }

        QWidget#versionItem:hover {
            background: rgba(74, 44, 126, 0.6);
            border: 1px solid #6a0dad;
        }

        QLabel#versionType {
            font-size: 12px;
            padding: 3px 8px;
            border-radius: 4px;
            background: #4a2c7e;
            color: #c9a8ff;
        }
        QLabel#versionName {
            font-size: 16px; 
            font-weight: 500; 
            color: #e0d6f2;
        }

        QLabel#versionType[type="release"] {
            background: #4caf50;
            color: white;
        }

        QPushButton#favoriteBtn{
            background: transparent;
            background-color: transparent;
            border: none;
            background-image: url(:/icons/favorite.svg);
            background-repeat: no-repeat;
            background-position: center;
        }
        QPushButton#favoriteBtn:checked{
            background-image: url(:/icons/favorite_checked.svg);
        }
        QPushButton#favoriteBtn:hover{
            background-image: url(:/icons/favorite_hover.svg);
        }

        QPushButton#versionButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #8a2be2, stop:1 #6a0dad);
            color: white;
            border: none;
            padding: 8px 15px;
            border-radius: 6px;
            font-weight: 500;
            font-size: 14px;
        }

        QPushButton#versionButton:hover, QPushButton#serverButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #7223bd, stop:1 #520a85)
    })");
    
    VL = new QVBoxLayout(VC);
    VL->setSpacing(10);
    VL->setContentsMargins(0, 5, 5, 0);
    versionList->setWidget(VC);
    versionList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    versionList->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    versionList->setWidgetResizable(true);
    versionsSection->addWidget(versionList);

    auto *rightSidebar = new QVBoxLayout();
    auto *filtersBox = new QGroupBox();
    filtersBox->setStyleSheet(R"(
        QPushButton {
            padding: 8px 16px;
            font-size: 14px;
            border: 1px solid #3a2a4a;
            border-radius: 6px;
            background: #2a1b3b;
            color: #a78bfa;
        }

        QPushButton:checked {
            background: #6d28d9;
            color: white;
            border-color: #8b5cf6;
        }
        QPushButton:hover {
            background: #4c1d95;
            color: #ddd6fe;
        }
    )");
    filtersBox->setMaximumHeight(250);
    filtersBox->setMinimumHeight(160);
    filtersBox->setObjectName("filters-section");

    auto *filterLayout = new QVBoxLayout(filtersBox);
    filterLayout->setContentsMargins(0, 0, 0, 10);
    auto *filterTitle = new QHBoxLayout();
    filterTitle->setSpacing(10);
    filterTitle->setContentsMargins(0, 0, 0, 20);
    auto *iconFilter = new QLabel();
    iconFilter->setPixmap(QIcon(":/icons/filter.svg").pixmap(16, 16));
    iconFilter->setAlignment(Qt::AlignCenter);
    filterTitleText = new QLabel();
    filterTitleText->setObjectName("section-title");
    
    filterTitle->addWidget(iconFilter);
    filterTitle->addWidget(filterTitleText);
    filterTitle->addStretch();
    filterLayout->addLayout(filterTitle);
    filterLayout->addStretch();
    searchInp = new QLineEdit();
    searchInp->setObjectName("searchInp");
    searchInp->setFixedWidth(230);
    searchInp->setStyleSheet(R"(
        QLineEdit#searchInp {
            selection-background-color: #6d28d9;
        }
    )");
    searchInp->setText(searchQuery);
    connect(searchInp, &QLineEdit::textChanged, this, &MainWindow::searchInpChanged);

    typeFlow = new FlowLayout(nullptr, 0, 8, 8);
    QStringList types = {"filter_all", "filter_release", "filter_snapshot", "filter_old_alpha", "filter_old_beta"};
    if(filters.contains("all")){
        for(const QString &t : types) {
            auto *btn = new QPushButton();
            btn->setCheckable(true);
            btn->setCursor(Qt::PointingHandCursor);
            btn->setObjectName(t.sliced(7));
            btn->setChecked(true);
            btn->setMinimumHeight(30); 
            filterBtns.append(btn);
            connect(btn, &QPushButton::toggled, this, &MainWindow::clickFilterBtn);

            typeFlow->addWidget(btn);
        }
    }else{
        for(const QString &t : types) {
            auto *btn = new QPushButton();
            btn->setCheckable(true);
            btn->setCursor(Qt::PointingHandCursor);
            btn->setObjectName(t.sliced(7));
            if(filters.contains(btn->objectName())){
                btn->setChecked(true);
            }
            btn->setMinimumHeight(30); 
            filterBtns.append(btn);
            connect(btn, &QPushButton::toggled, this, &MainWindow::clickFilterBtn);

            typeFlow->addWidget(btn);
        }
    }

    filterLayout->addWidget(searchInp);
    filterLayout->addLayout(typeFlow);


    auto *serverContainer = new QWidget();
    auto *serverLayout = new QVBoxLayout(serverContainer);
    serverContainer->setObjectName("server-section");

    serverLayout->setContentsMargins(20, 20, 1, 20);
    auto *serverTitle = new QHBoxLayout();
    serverTitle->setSpacing(10);
    serverTitle->setContentsMargins(0, 0, 0, 20);
    auto *windowServerBtn = new QPushButton();
    windowServerBtn->setObjectName("window-server-btn");
    windowServerBtn->setFixedSize(30, 30);
    windowServerBtn->setStyleSheet(R"(
        QPushButton#window-server-btn {
            background-color: transparent;
            border: none;
            image: url(:/icons/arrow_right.svg);
        }
        QPushButton#window-server-btn:hover {
            image: url(:/icons/arrow_right_hover.svg);
        }
    )");

    auto *iconServer = new QLabel();
    iconServer->setPixmap(QIcon(":/icons/create_server.svg").pixmap(22, 22));
    iconServer->setAlignment(Qt::AlignCenter);
    iconServer->setFixedHeight(30);

    serverTitleText = new QLabel();
    serverTitleText->setObjectName("section-title");
    serverTitleText->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    serverTitleText->setFixedHeight(30);
    
    serverTitle->addWidget(iconServer, 0, Qt::AlignVCenter);
    serverTitle->addWidget(serverTitleText, 0, Qt::AlignVCenter);
    serverTitle->addWidget(windowServerBtn, 0, Qt::AlignVCenter);
    serverTitle->addStretch(1);
    serverLayout->addLayout(serverTitle);

    versionLabel = new QLabel();
    versionLabel->setObjectName("name-version-server");
    versionLabel->setStyleSheet(R"(
        QLabel{
            color: #c9a8ff;
            font-weight: 600;
            font-size: 14px;
        }
    )");
    serverLayout->addWidget(versionLabel);

    auto *scrollSettings = new QScrollArea();
    scrollSettings->setWidgetResizable(true);
    scrollSettings->setMinimumHeight(60);
    scrollSettings->setStyleSheet(R"(        
        QScrollBar:vertical {
            border: none;
            background: #2d1b3b;
            width: 10px;
            margin: 0px;
            border-radius: 5px;
        }
    )"); 

    auto *formWidget = new QWidget();
    formWidget->setStyleSheet(R"(
        QLabel{
            color: #c9a8ff;
            font-weight: 600;
            font-size: 14px;
        }
    )");
    auto *form = new QFormLayout(formWidget);
    form->setLabelAlignment(Qt::AlignLeft);
    form->setFormAlignment(Qt::AlignLeft);

    auto addSelect = [&](QString label, QStringList options) {
        auto *cb = new QComboBox();
        cb->addItems(options);
        cb->setFixedWidth(120);
        cb->setObjectName(label);
        auto *fieldContainer = new QWidget();
        auto *fieldLayout = new QHBoxLayout(fieldContainer);
        
        fieldLayout->addStretch();
        fieldLayout->addWidget(cb);
        fieldLayout->setContentsMargins(0, 0, 0, 0);

        form->addRow(label, fieldContainer);
    };

    auto *levelPathContainer = new QWidget();
    auto *levelPathLayout = new QHBoxLayout(levelPathContainer);
    levelPathBtn = new QPushButton();
    levelPathBtn->setObjectName("get-path");
    levelPathBtn->setCursor(Qt::PointingHandCursor);
    levelPathLayout->addStretch(); 
    levelPathLayout->addWidget(levelPathBtn);
    levelPathLayout->setContentsMargins(0, 0, 0, 0);
    form->addRow("level-name", levelPathContainer);

    addSelect("accepts-transfers", {"false", "true"});
    addSelect("allow-flight", {"false", "true"});
    addSelect("allow-nether", {"true", "false"});
    addSelect("difficulty", {"peaceful", "easy", "normal", "hard"});
    addSelect("gamemode", {"survival", "creative", "adventure", "spectator"});

    auto *maxPlayers = new QSpinBox();
    maxPlayers->setRange(1, 100); maxPlayers->setValue(20);
    maxPlayers->setObjectName("max-players");
    auto *maxPlayersContainer = new QWidget();
    auto *maxPlayersLayout = new QHBoxLayout(maxPlayersContainer);
    maxPlayersLayout->addStretch(); 
    maxPlayersLayout->addWidget(maxPlayers);
    maxPlayersLayout->setContentsMargins(0, 0, 0, 0);
    form->addRow("max-players", maxPlayersContainer);

    auto *serverPort = new QSpinBox();
    serverPort->setRange(1024, 65535); serverPort->setValue(25565);
    serverPort->setObjectName("server-port");
    auto *serverPortContainer = new QWidget();
    auto *serverPortLayout = new QHBoxLayout(serverPortContainer);
    serverPortLayout->addStretch(); 
    serverPortLayout->addWidget(serverPort);
    serverPortLayout->setContentsMargins(0, 0, 0, 0);
    form->addRow("server-port", serverPortContainer);

    auto *motdInput = new QLineEdit("A Minecraft Server");
    motdInput->setObjectName("motd");
    auto *motdInputContainer = new QWidget();
    auto *motdInputLayout = new QHBoxLayout(motdInputContainer);
    motdInputLayout->addStretch(); 
    motdInputLayout->addWidget(motdInput);
    motdInputLayout->setContentsMargins(0, 0, 0, 0);
    form->addRow("motd", motdInputContainer);

    scrollSettings->setWidget(formWidget);
    serverLayout->addWidget(scrollSettings);

    saveBtn = new QPushButton();
    saveBtn->setObjectName("serverButton");
    saveBtn->setGraphicsEffect(createGlow(15, QColor(138, 43, 226, 255)));
    saveBtn->setCursor(Qt::PointingHandCursor);
    startBtn = new QPushButton();
    startBtn->setObjectName("serverButton");
    startBtn->setGraphicsEffect(createGlow(15, QColor(138, 43, 226, 255)));
    startBtn->setCursor(Qt::PointingHandCursor);
    serverLayout->addSpacing(10);
    serverLayout->addWidget(saveBtn, 0,Qt::AlignLeft);
    serverLayout->addWidget(startBtn, 0, Qt::AlignLeft);


    rightSidebar->addWidget(filtersBox);
    rightSidebar->addStretch(1);
    rightSidebar->addWidget(serverContainer);

    contentLayout->addWidget(versionsContainer);
    contentLayout->addLayout(rightSidebar);
    mainLayout->addLayout(contentLayout);

    // --- PROGRESS BAR ---
    widgetProgressSection = new QWidget();
    widgetProgressSection->setObjectName("progress-section");
    widgetProgressSection->setMaximumHeight(140);
    auto *progressSection = new QVBoxLayout(widgetProgressSection);
    progressSection->setContentsMargins(20, 20, 20, 20);

    auto *progressTitle = new QHBoxLayout();
    progressTitle->setSpacing(10);
    progressTitle->setContentsMargins(0, 0, 0, 20);
    auto *iconProgress = new QLabel();
    iconProgress->setPixmap(QIcon(":/icons/install.svg").pixmap(22, 22));
    iconProgress->setAlignment(Qt::AlignCenter);
    progressTitleText = new QLabel();
    progressTitleText->setObjectName("section-title");
    
    progressBarPrecent = new QLabel("    ");
    progressBarPrecent->setObjectName("section-title");
    auto *progressCancel = new QHBoxLayout();
    QPushButton *btnCancel = new QPushButton("Cancel");
    btnCancel->setStyleSheet(R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #8a2be2, stop:1 #6a0dad);
            color: white;
            border: none;
            padding: 8px 15px;
            border-radius: 6px;
            font-weight: 500;
            font-size: 14px;
        }

        QPushButton:hover{
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #7223bd, stop:1 #520a85);
        }
    )");
    btnCancel->installEventFilter(this);
    btnCancel->setCursor(Qt::PointingHandCursor);
    connect(btnCancel, QPushButton::clicked, DOWNLOAD_MANAGER, &DownloadManager::cancelDownload);
    progressCancel->addWidget(btnCancel);
    progressCancel->addStretch();

    progressTitle->addWidget(iconProgress);
    progressTitle->addWidget(progressTitleText);
    progressTitle->addStretch();
    progressTitle->addWidget(progressBarPrecent);

    progressBar = new QProgressBar();
    progressBar->setFixedHeight(8);
    progressBar->setTextVisible(false);
    progressBar->setStyleSheet("QProgressBar { background: #2d1b4e; border-radius: 4px; } "
                        "QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #8a2be2, stop:1 #6a0dad); border-radius: 4px;}");
    progressBar->setRange(0, 1000);
    progressBar->setValue(0);

    progressSection->addLayout(progressTitle);
    progressSection->addWidget(progressBar);
    progressSection->addLayout(progressCancel);

    widgetProgressSection->hide();
    mainLayout->addWidget(widgetProgressSection);

    // --- FOOTER ---
    footer = new QLabel();
    footer->setAlignment(Qt::AlignCenter);
    footer->setStyleSheet("border-top: 1px solid #4a2c7e; color: #555; padding: 10px; font-size: 10px;");
    WindowLayout->addWidget(footer);
    
    // -- SETTINGS ---
    SettingsOverlay = new QWidget(centralWidget);
    SettingsOverlay->setObjectName("settingsOverlay");
    SettingsOverlay->hide();
    SettingsOverlay->setStyleSheet(R"(
        QWidget#settingsOverlay{
            background-color: rgba(13, 8, 22, 217);
        }
    )");
    auto *SettingsOverlayLayout = new QVBoxLayout(SettingsOverlay);
    SettingsOverlayLayout->setAlignment(Qt::AlignCenter);
    auto *settingsModal = new QWidget();
    settingsModal->setObjectName("settingsModal");
    settingsModal->setFixedWidth(456);
    settingsModal->setMaximumHeight(760);
    settingsModal->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    settingsModal->setStyleSheet(R"(
        QWidget#settingsModal {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, 
                                        stop:0 #2a1546, stop:1 #1a102b);
            border-radius: 15px;
            border: 1px solid #4a2c7e;
        }
    )");
    SettingsOverlayLayout->addWidget(settingsModal);
    auto *settingsModalLayout = new QVBoxLayout(settingsModal);
    SettingsOverlayLayout->addWidget(settingsModal);
    settingsModalLayout->setContentsMargins(20, 20, 20, 20);
    auto *settingsModalHeader = new QWidget();
    settingsModalHeader->setStyleSheet(R"(
        QWidget#settingsModalHeader {
            border-bottom: 1px solid #4a2c7e;
        }
    )");
    settingsModalHeader->setObjectName("settingsModalHeader");
    auto *settingsModalHeaderLayout = new QHBoxLayout(settingsModalHeader);
    settingsModalHeaderLayout->setContentsMargins(0, 0, 0, 12);
    auto *iconModalSettings = new QLabel();
    iconModalSettings->setPixmap(QIcon(":/icons/settings_hover.svg").pixmap(22, 22));
    iconModalSettings->setAlignment(Qt::AlignCenter);
    settingsHeaderTitle = new QLabel();
    settingsHeaderTitle->setObjectName("section-title");
    
    auto *settingsCloseBtn = new QPushButton("×");
    connect(settingsCloseBtn, &QPushButton::clicked, this, [=]() {
        SettingsOverlay->hide();
        SettingsOverlay->raise();
    });
    settingsCloseBtn->setObjectName("closeBtn");
    settingsCloseBtn->setCursor(Qt::PointingHandCursor);
    settingsCloseBtn->setStyleSheet(R"(
        QPushButton#closeBtn {
            background: none;
            border: none;
            color: #c9a8ff;
            font-size: 24px;
            font-weight: bold;
        }

        QPushButton#closeBtn:hover {
            color: #ff4545;
        }
    )");
    settingsModalHeaderLayout->addWidget(iconModalSettings);
    settingsModalHeaderLayout->addSpacing(10);
    settingsModalHeaderLayout->addWidget(settingsHeaderTitle);
    settingsModalHeaderLayout->addStretch();
    settingsModalHeaderLayout->addWidget(settingsCloseBtn);
    settingsModalLayout->addWidget(settingsModalHeader);

    auto *settingsModalBody = new QWidget();
    settingsModalBody->setStyleSheet(R"(
        QLabel{
            color: #c9a8ff;
            font-weight: 600;
            font-size: 14px;
        }
    )");
    auto *settingsModalBodyLayout = new QVBoxLayout(settingsModalBody);
    auto *FLSLP = new QHBoxLayout();
    FLSLPL = new QLabel();
    MinecraftPathBtn = new QPushButton();
    MinecraftPathBtn->setObjectName("get-path");
    MinecraftPathBtn->setCursor(Qt::PointingHandCursor);
    MinecraftPathBtn->setToolTip(minecraftDirectory);
    connect(MinecraftPathBtn, &QPushButton::clicked, this, &MainWindow::selectMinecraftDirectory);
    FLSLP->setAlignment(Qt::AlignVCenter);
    FLSLP->addWidget(FLSLPL);
    FLSLP->addStretch();
    FLSLP->addWidget(MinecraftPathBtn);
    FLSLP->setContentsMargins(0, 0, 0, 0);

    settingsLanguage = new QComboBox();
    settingsLanguage->addItem("English", "en-en");
    settingsLanguage->addItem("Русский", "ru-ru");
    settingsLanguage->addItem("日本語", "ja-jp");
    settingsLanguage->addItem("简体中文", "zh-cn");
    settingsLanguage->addItem("Italiano", "it-it");
    settingsLanguage->addItem("Español", "es-es");
    settingsLanguage->addItem("Deutsch", "de-de");
    settingsLanguage->addItem("Français", "fr-fr");
    settingsLanguage->setFixedWidth(120);
    settingsLanguage->setObjectName("language");
    settingsLanguage->installEventFilter(this); 
    auto *FLSL = new QHBoxLayout();
    FLSLL = new QLabel();
    FLSL->setAlignment(Qt::AlignVCenter);
    FLSL->addWidget(FLSLL);
    FLSL->addStretch();
    FLSL->addWidget(settingsLanguage);
    FLSL->setContentsMargins(0, 0, 0, 0);

    auto *settingsRAM = new QSpinBox();
    settingsRAM->setFixedWidth(120);
    settingsRAM->setMinimum(1024);
    settingsRAM->setMaximum(getTotalRAM());
    settingsRAM->setSingleStep(512);
    settingsRAM->setValue(RAM);
    settingsRAM->setSuffix(" MB");
    settingsRAM->setObjectName("RAM");
    
    connect(settingsRAM, &QSpinBox::valueChanged, this, &MainWindow::changeRAM);

    auto *FLSR = new QHBoxLayout();
    FLSRL = new QLabel();
    FLSR->setAlignment(Qt::AlignVCenter);
    FLSR->addWidget(FLSRL);
    FLSR->addStretch();
    FLSR->addWidget(settingsRAM);
    FLSR->setContentsMargins(0, 0, 0, 0);
    
    settingsModalBodyLayout->addLayout(FLSLP);
    settingsModalBodyLayout->addLayout(FLSL);
    settingsModalBodyLayout->addLayout(FLSR);
    settingsModalBody->setObjectName("modalBody");
    settingsModalLayout->addWidget(settingsModalBody);
    settingsModalLayout->addStretch();
    connect(settingsLanguage, &QComboBox::currentIndexChanged, this, &MainWindow::changeLanguage);
    
    QSettings settings;
    QString lang = settings.value("lang", "en-en").toString();
    int i = settingsLanguage->findData(lang);
    if(i != -1) settingsLanguage->setCurrentIndex(i);
    setLanguage(lang);
    
    auto connection = std::make_shared<QMetaObject::Connection>();
    *connection = connect(VERSIONS_MANAGER, &VersionsManager::showWindow, this, [this, splash, connection]() {
        this->show();
        splash->finish(this);
        splash->deleteLater();
        QObject::disconnect(*connection);
    }, Qt::QueuedConnection);

    serverContainer->hide();
    emit loadVersions();

    resize(1180, 920);

}
MainWindow::~MainWindow() {
    threadDownloadManager->quit();
    threadVersionsManager->quit();

    if (!threadDownloadManager->wait(3000)) {
        threadDownloadManager->terminate();
    }
    
    if (!threadVersionsManager->wait(3000)) {
        threadVersionsManager->terminate();
    }
}

void MainWindow::showInfo() {
    QMessageBox::aboutQt(this, "About Qt");
}
int MainWindow::getTotalRAM() {
    quint64 totalMemory = 0;

#if defined(Q_OS_WIN)
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        totalMemory = memInfo.ullTotalPhys;
    }
#elif defined(Q_OS_LINUX)
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    if (pages > 0 && page_size > 0) {
        totalMemory = (quint64)pages * (quint64)page_size;
    }
#elif defined(Q_OS_MAC)
    int mib[2] = { CTL_HW, HW_MEMSIZE };
    size_t length = sizeof(totalMemory);
    sysctl(mib, 2, &totalMemory, &length, NULL, 0);
#endif

    if (totalMemory > 0) {
        return static_cast<int>(totalMemory / (1024 * 1024));
    }
    
    return 4096;
}

// --- UI & STYLING ---
void MainWindow::setupStyles() {
    this->setStyleSheet(R"(
        QLabel#section-title {
            font-size: 18px; 
            color: #caa8ff;
        }
        QWidget#header{
            border-bottom: 1px solid #4a2c7e;
            background: transparent;
        }
        QScrollArea, QScrollArea > QWidget > QWidget {
            background: transparent;
            border: none;
        }
        QWidget#progress-section,    
        QWidget#versions-section,
        QGroupBox#filters-section,
        QWidget#server-section {
            background: rgba(42, 21, 70, 0.6);
            border-radius: 24px;
            padding: 20px;
            border: 1px solid #4a2c7e;
        }
        QGroupBox#filters-section,
        QWidget#server-section{
            min-width: 255px;
            max-width: 255px;
        }

        QScrollBar::handle:vertical {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, 
                                        stop:0 #6a3093, stop:1 #a044ff);
            border: 2px solid #2d1b3b;
            min-height: 20px;
            border-radius: 5px;
        }

        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            border: none;
            background: none;
            height: 0px;
        }
        
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
            background: none;
        }

        QLineEdit, QSpinBox, QComboBox {
            padding: 6px 7px;
            font-size: 14px;
            border: 1px solid #3a2a4a;
            border-radius: 6px;
            background: #2a1b3b;
            color: white;
            margin-bottom: 5px;
        }
        QSpinBox, QComboBox, QLineEdit#motd {
            font-size: 13px;
            padding: 6px 5px;
            color: #e0d6f2;
            min-width: 80px;
        }
        QPushButton#get-path {
            padding: 7px 12px;
            font-size: 14px;
            border: 1px solid #3a2a4a;
            border-radius: 8px;
            background: #2a1b3b;
            color: #a78bfa;
        }
        QPushButton#get-path:hover {
            border-color: #8a2be2;
        }
        QPushButton#get-path:pressed {
            background: #6d28d9;
            color: white;
            border-color: #8a2be2;
        }
        QLineEdit:focus, QSpinBox:focus, QComboBox:focus, 
        QLineEdit:hover, QSpinBox:hover, QComboBox:hover {
            border: 1px solid #8a2be2;
            outline: none;
        }
        QSpinBox::up-button, QSpinBox::down-button {
            width: 0px;
            border: none;
            background: none;
        }
        QComboBox::drop-down {
            border: none;
            width: 25px; 
        }

        QComboBox::down-arrow {
            image: url(:/icons/arrow_down.svg);
            width: 20px;
            height: 20px;
        }
        QComboBox::down-arrow:on {
            image: url(:/icons/arrow_up.svg);
            width: 20px;
            height: 20px;
        }

        QComboBox QAbstractItemView {
            background-color: #2a1b3b;
            color: #a78bfa;
            border: 1px solid #4a2c7e;
            selection-background-color: #4a2c7e;
            selection-color: white;
            outline: none; 
            border-radius: 6px; 
        }

        QComboBox QAbstractItemView::item {
            min-height: 30px;
            padding-left: 10px;
        }

        QComboBox QAbstractItemView::item:hover {
            background-color: #4c1d95;
            color: white;
        }

        QPushButton#serverButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #8a2be2, stop:1 #6a0dad);
            color: white;
            border: none;
            padding: 5px 15px;
            border-radius: 7px;
            font-weight: 500;
            font-size: 13px;
        }
        QMainWindow { 
        background: qlineargradient(x1:0, y1:0, x2:1, y2:1, 
                                    stop:0 #1a102b, 
                                    stop:1 #2d1b4e);
        font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;}
    )");
}
QGraphicsDropShadowEffect* MainWindow::createGlow(const int &blur, const QColor &color) {
        auto *effect = new QGraphicsDropShadowEffect();
        effect->setBlurRadius(blur);
        effect->setOffset(0, 0);
        effect->setColor(color);
        return effect;
    }
int MainWindow::scalingIcon(int size) {
    return size * QWidget::devicePixelRatioF(); 
}
void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    if (SettingsOverlay) {
        SettingsOverlay->resize(this->size());
    }
}
bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    QString objName = obj->objectName();
    if(objName == "versionButton" || objName == "latest-version-card-button"){
        if (event->type() == QEvent::Enter || event->type() == QEvent::Leave) {
            QWidget *item = qobject_cast<QWidget*>(obj);

            QAbstractAnimation *oldAnim = item->property("currentAnim").value<QAbstractAnimation*>();
            if (oldAnim) {
                oldAnim->stop();
                delete oldAnim;
            }

            QGraphicsDropShadowEffect *shadow = qobject_cast<QGraphicsDropShadowEffect*>(item->graphicsEffect());
            if (shadow) {
                QPropertyAnimation *blurAnim = new QPropertyAnimation(shadow, "blurRadius");
                QPropertyAnimation *offsetAnim = new QPropertyAnimation(shadow, "offset");

                blurAnim->setDuration(150);
                offsetAnim->setDuration(150);

                if (event->type() == QEvent::Enter) {
                    blurAnim->setEndValue(15);
                    offsetAnim->setEndValue(QPointF(0, 5));
                } else {
                    blurAnim->setEndValue(0);
                    offsetAnim->setEndValue(QPointF(0, 0));
                }

                QParallelAnimationGroup *group = new QParallelAnimationGroup(item);
                group->addAnimation(blurAnim);
                group->addAnimation(offsetAnim);
                item->setProperty("currentAnim", QVariant::fromValue<QAbstractAnimation*>(group));

                blurAnim->start(QAbstractAnimation::DeleteWhenStopped);
                offsetAnim->start(QAbstractAnimation::DeleteWhenStopped);
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}
void MainWindow::updateProgressBarTitle(const QString text){
    progressTitleText->setText(text);
}
void MainWindow::updateProgress(const qint64 precent){
    progressBar->setValue(precent);
    progressBarPrecent->setText(QString("%1%").arg(precent / 10.0f));
}
void MainWindow::showOrHideProgress(const bool show){
    if (show){
        widgetProgressSection->show();
    }
    else{
        widgetProgressSection->hide();
    }
}
// --- SETTINGS & LOCALIZATION ---
void MainWindow::readSettings(){
    QSettings settings;

    searchQuery = settings.value("searchQuery").toString();
    filters = settings.value("filters", {"all"}).toStringList();
    
    minecraftDirectory = settings.value("minecraftDirectory", QDir::cleanPath(QProcessEnvironment::systemEnvironment().value("APPDATA") + "/.minecraft")).toString();
    emit setMinecraftDirectoryDM(minecraftDirectory);
    emit setMinecraftDirectoryVM(minecraftDirectory);
    
    RAM = settings.value("RAM", 1024).toInt();
    if (RAM < 1024){
        RAM = 1024;
    }
    username = settings.value("username", "Lunusik").toString();

    QStringList defaultFilters = {"all"};
    filters = settings.value("filters", defaultFilters).toStringList();
    searchQuery = settings.value("searchQuery").toString();
}
void MainWindow::changeLanguage(int index) {
    QSettings settings;
    QString lang = settingsLanguage->itemData(index).toString();

    settings.setValue("lang", lang);
    setLanguage(lang);
}
void MainWindow::changeRAM(const int val) {
    QSettings settings;

    RAM = val;
    settings.setValue("RAM", val);
}
void MainWindow::changeUsername(const QString newUsername){
    QSettings settings;

    username = newUsername;
    settings.setValue("username", username);
}
void MainWindow::setLanguage(QString langCode) {
    static QTranslator* translator = nullptr;
    auto* newTranslator = new QTranslator();

    if (newTranslator->load(":/i18n/translations/" + langCode)) {
        if (translator) {
            qApp->removeTranslator(translator);
            delete translator;
        }

        translator = newTranslator;
        qApp->installTranslator(translator);
        updateStrings();

    } else {
        delete newTranslator;
    }
}
void MainWindow::updateStrings() {
    this->setWindowTitle(tr("launcher_title"));
    logoText->setText(tr("launcher_name"));
    usernameLabel->setText(tr("username") + ": ");
    versionsTitleText->setText(tr("versions"));
    filterTitleText->setText(tr("filters"));
    searchInp->setPlaceholderText(tr("search_version"));
    serverTitleText->setText(tr("create_server"));
    versionLabel->setText(tr("server_version_prefix") + " - " + tr("none"));
    levelPathBtn->setText(tr("select_path"));
    saveBtn->setText(tr("save_properties"));
    startBtn->setText(tr("start_server"));
    progressTitleText->setText(tr("install_disable"));
    footer->setText(tr("launcher_version"));
    settingsHeaderTitle->setText(tr("settings"));
    MinecraftPathBtn->setText(tr("select_path"));
    FLSLPL->setText(tr("minecraft_path"));
    FLSLL->setText(tr("language"));
    FLSRL->setText(tr("RAM"));

    
    for(QPushButton* b : filterBtns) {
        QString tag_tr = "filter_" + b->objectName();
        b->setText(tr(tag_tr.toUtf8().constData()));
    } 
    for (QLabel* tl : typeLabelList) {
        QString type = tl->property("type").toString();
        tl->setText(tr(type.toUtf8().constData()));
    }
    for (QPushButton* btnD : btnDownloadList) {
        if (btnD->property("isInstalled").toBool()){
            btnD->setText(tr("play"));
        }else{
            btnD->setText(tr("download"));
        }
    }
    for (QLabel* ltl : latestTypeLabelList) {
        QString type = ltl->property("type").toString();
        ltl->setText(tr(type.toUtf8().constData()));
    }
    for (QPushButton* lBtnD : latestBtnDownloadList) {
        lBtnD->setText(tr("download"));
    }
}
void MainWindow::selectMinecraftDirectory(){
    QString base = !minecraftDirectory.isEmpty() ? minecraftDirectory : QProcessEnvironment::systemEnvironment().value("APPDATA");
    QString dirPath = QFileDialog::getExistingDirectory(
        this,
        tr("select_minecraftDir"),
        base,
        QFileDialog::ShowDirsOnly
    );

    QSettings settings;

    if (dirPath.isEmpty()) {
        dirPath = base;
    }
    if(!dirPath.endsWith(".minecraft", Qt::CaseInsensitive)){
        dirPath += "/.minecraft";
    }

    dirPath = QDir::cleanPath(dirPath);
    minecraftDirectory = dirPath;
    emit setMinecraftDirectoryDM(minecraftDirectory);
    emit setMinecraftDirectoryVM(minecraftDirectory);
    settings.setValue("minecraftDirectory", minecraftDirectory);
    MinecraftPathBtn->setToolTip(minecraftDirectory);
}


// --- VERSION MANAGEMENT ---
void MainWindow::renderVersions(const QList<VersionData> versions, const QList<LatestVersionData> latestVersions){
    allVersions.clear();
    clearVersionsList();
    clearLatestVersionsList();
    for (const VersionData &vd : versions){
        allVersions.append(addVersionItem(vd));
    }
    for (const LatestVersionData &lvd : latestVersions){
        addLatestVersionItem(lvd);
    }
    applyFilter();
}
QWidget* MainWindow::addVersionItem(const VersionData &versionData) {
    auto *item = new QWidget();
    item->setFixedHeight(59);
    item->setObjectName("versionItem");
    item->setAttribute(Qt::WA_StyledBackground);
    item->setCursor(Qt::PointingHandCursor);
    item->setProperty("type", versionData.type);
    item->setProperty("name", versionData.name);
    item->setProperty("isInstalled", versionData.isInstalled);
    item->setProperty("isFavorite", versionData.isFavorite);

    auto *hLayout = new QHBoxLayout(item);
    hLayout->setContentsMargins(15, 10, 15, 10);
    hLayout->setSpacing(15);

    auto *infoLayout = new QHBoxLayout();
    auto *nameLabel = new QLabel(versionData.name);
    nameLabel->setObjectName("versionName");

    auto *typeLabel = new QLabel(tr(versionData.type.toUtf8().constData()));
    typeLabelList.append(typeLabel);
    typeLabel->setObjectName("versionType");
    typeLabel->setProperty("type", versionData.type.toLower());

    infoLayout->addWidget(nameLabel, 0, Qt::AlignVCenter);
    infoLayout->addWidget(typeLabel, 0, Qt::AlignVCenter);

    auto *favoriteBtn = new QPushButton(); 
    favoriteBtn->setObjectName("favoriteBtn");
    favoriteBtn->setCheckable(true);
    favoriteBtn->setFixedSize(30, 30);
    favoriteBtn->setCursor(Qt::PointingHandCursor);
    favoriteBtn->setChecked(versionData.isFavorite);
    connect(favoriteBtn, &QPushButton::toggled, [this, item, versionData](bool checked) {
        QSettings settings;
        QStringList favoriteVersions = settings.value("favoriteVersions").toStringList();

        item->setProperty("isFavorite", checked);

        if (checked) {
            if (!favoriteVersions.contains(versionData.name)) {
                favoriteVersions.append(versionData.name);
            }
        } else {
            favoriteVersions.removeAll(versionData.name);
        }
        settings.setValue("favoriteVersions", favoriteVersions);
        applyFilter();
    });

    auto *btn = new QPushButton(versionData.isInstalled ? tr("play") : tr("download"));
    connect(btn, &QPushButton::clicked, this, [this, item, btn] {
        if(btn->property("isInstalled").toBool()){
            clickPlayMinecraft();
            return;
        }
        clickDownloadMinecraft(item);
    });
    btnDownloadList.append(btn);
    btn->setObjectName("versionButton");
    btn->installEventFilter(this);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setFixedHeight(32);
    btn->setProperty("url", versionData.url);
    btn->setProperty("name", versionData.name);
    btn->setProperty("hash", versionData.hash);
    btn->setProperty("type", versionData.type);
    btn->setProperty("isInstalled", versionData.isInstalled);
    btn->setGraphicsEffect(createGlow(0, QColor(0, 0, 0, 110)));
    

    hLayout->addLayout(infoLayout);
    hLayout->addStretch();
    hLayout->addWidget(favoriteBtn, 0, Qt::AlignVCenter | Qt::AlignRight);
    hLayout->addWidget(btn, 0, Qt::AlignVCenter | Qt::AlignRight);

    VL->insertWidget(VL->count() - 1, item);
    return item;
}
void MainWindow::addLatestVersionItem(const LatestVersionData &latestVersionData) {
    auto *item = new QWidget();
    item->setCursor(Qt::PointingHandCursor);
    item->setObjectName("latest-version-card");
    item->setStyleSheet(R"(
        QWidget#latest-version-card{
            background: hsla(0, 0%, 100%, 0.1);
            border-radius: 8px;
            padding: 15;
            border: 1 solid rgba(255, 255, 255, 0.2);
        }
        QWidget#latest-version-card:hover{
            background: hsla(0, 0%, 100%, 0.2);
        }
    )");
    item->setProperty("isInstalled", latestVersionData.isInstalled);
    auto *layout = new QHBoxLayout(item);
    auto *infoLayout = new QVBoxLayout();

    auto *name_version = new QLabel(latestVersionData.name);
    name_version->setObjectName("latest-version-card-title");
    name_version->setStyleSheet(R"(
        QWidget#latest-version-card-title{
            font-size: 16px;
            font-weight: 700;
            margin-bottom: 5px;
            color: white;
        }
    )");
    auto *type_version = new QLabel(tr(latestVersionData.type.toUtf8().constData()));
    latestTypeLabelList.append(type_version);
    type_version->setProperty("type", latestVersionData.type.toLower());
    type_version->setObjectName("latest-version-card-type");
    type_version->setStyleSheet(R"(
        QWidget#latest-version-card-type{
            font-size: 12px;
            margin-bottom: 10px;
            text-transform: uppercase;
            color: #d8b4fe;
        }
    )");
    auto *date_version = new QLabel(latestVersionData.date);
    date_version->setObjectName("latest-version-card-date");
    date_version->setStyleSheet(R"(
        QWidget#latest-version-card-date{
            font-size: 12px;
            color: #a78bfa;
        }
    )");
    auto *btn = new QPushButton(latestVersionData.isInstalled ? tr("play") : tr("download"));
    btn->setGraphicsEffect(createGlow(0, QColor(0, 0, 0, 80)));
    btn->setProperty("isInstalled", latestVersionData.isInstalled);
    connect(btn, &QPushButton::clicked, this, [this, item, btn] {
        if(btn->property("isInstalled").toBool()){
            clickPlayMinecraft();
            return;
        }
        clickDownloadMinecraft(item);
    });
    btn->setProperty("url", latestVersionData.url);
    btn->setProperty("name", latestVersionData.name);
    btn->setProperty("hash", latestVersionData.hash); 
    latestBtnDownloadList.append(btn);
    btn->installEventFilter(this);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setObjectName("latest-version-card-button");
    btn->setStyleSheet(R"(
        QPushButton#latest-version-card-button {
            background: #10b981;
            font-size: 14px;
            color: white;
            border: none;
            padding: 8px 16px;
            border-radius: 6px;
            font-weight: 800;
        }
        QPushButton#latest-version-card-button:hover {
            background: #059669;
        }
    )");
    infoLayout->addWidget(name_version);
    infoLayout->addWidget(type_version);
    infoLayout->addWidget(date_version);

    layout->addLayout(infoLayout);
    layout->addWidget(btn);
    
    latestVersionsLayout->addWidget(item, 0, latestVersionData.col);
}
void MainWindow::clearVersionsList() {
    for (QLabel* t : typeLabelList) {
        t->deleteLater();
    }
    typeLabelList.clear();

    for (QPushButton* btn : btnDownloadList) {
        btn->deleteLater();
    }
    btnDownloadList.clear();

    while (QLayoutItem* item = VL->takeAt(0)) {
        if (QWidget* widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }
    
}
void MainWindow::clearLatestVersionsList() {
    for (QLabel* t : latestTypeLabelList) {
        t->deleteLater();
    }
    latestTypeLabelList.clear();

    for (QPushButton* btn : latestBtnDownloadList) {
        btn->deleteLater();
    }
    latestBtnDownloadList.clear();

    while (QLayoutItem* item = latestVersionsLayout->takeAt(0)) {
        if (QWidget* widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }
}

// --- FILTERING & SEARCH ---
void MainWindow::clickFilterBtn() {
    auto *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    filters.clear();
    QString name = btn->objectName();
    if (name == "all") {
        bool ischeckedBtn = btn->isChecked();
        for (QPushButton *b : filterBtns) {
            if (b != btn) {
                b->blockSignals(true);
                b->setChecked(ischeckedBtn);
                b->blockSignals(false);
            }
        }
        if(ischeckedBtn){
            filters.append("all");
        }
    } else {
        int checkedCount = 0;
        int totalBtns = 0;
        QPushButton *allBtn = nullptr;

        for (QPushButton *b : filterBtns) {
            if (b->objectName() == "all") {
                allBtn = b;
            } else {
                totalBtns++;
                if (b->isChecked()) {
                    filters.append(b->objectName());
                    checkedCount++;
                }
            }
        }

        allBtn->blockSignals(true);
        
        if (checkedCount == totalBtns) {
            filters.clear();
            filters.append("all");
            allBtn->setChecked(true);
        } else {
            allBtn->setChecked(false);
        }
        
        allBtn->blockSignals(false);
    }
    QSettings settings;
    settings.setValue("filters", filters);
    applyFilter();
}
void MainWindow::searchInpChanged(const QString &text) {
    QSettings settings;

    searchQuery = text.trimmed().toLower();
    settings.setValue("searchQuery", searchQuery);
    
    applyFilter();
}
void MainWindow::applyFilter() {
    applyFilter(filters, searchQuery);
}
void MainWindow::applyFilter(const QStringList &allowedTypes, const QString &searchText) {
    if (filterTimer) {
        filterTimer->stop();
        filterTimer->disconnect();
        filterTimer->deleteLater();
        filterTimer = nullptr;
    }
    
    this->setUpdatesEnabled(false);
    
    QList<QWidget*> widgetsToShow;
    
    QList<QWidget*> others;
    QList<QWidget*> favorites;
    QList<QWidget*> installed;

    bool isAll = allowedTypes.contains("all");
    bool noSearch = searchText.isEmpty();

    if (isAll && noSearch) {
        for (QWidget* version : allVersions) {
            if(version->property("isFavorite").toBool()){
                favorites.append(version);
            }
            else if(version->property("isInstalled").toBool()){
                installed.append(version);    
            }else {
                others.append(version);
            }
        }
    } 
    else{
        for (QWidget* version : allVersions) {
            bool typeMatch = isAll || allowedTypes.contains(version->property("type").toString());
            bool textMatch = noSearch || version->property("name").toString().contains(searchText);

            if (typeMatch && textMatch) {
                if(version->property("isFavorite").toBool()){
                    favorites.append(version);
                }
                else if(version->property("isInstalled").toBool()){
                    installed.append(version);    
                }else {
                    others.append(version);
                }
            }
            else{
                version->hide();
            }
        }
    }

    widgetsToShow << favorites << installed << others;

    if (!widgetsToShow.isEmpty()) {
        int batchSize = 15;
        int currentIndex = 0;
        
        filterTimer = new QTimer(this);
        QTimer *currentT = filterTimer;
        for (int i = 0; i < VL->count(); ++i) {
            if (VL->itemAt(i)->spacerItem()) {
                delete VL->takeAt(i);
                i--; 
            }
        }
        for(QWidget* w : widgetsToShow){
            VL->addWidget(w);
        }
        VL->addStretch(1);

        connect(filterTimer, &QTimer::timeout, this, [this, currentT, widgetsToShow, currentIndex, batchSize]() mutable {
            if (this->filterTimer != currentT) return;
            for (int i = 0; i < batchSize && currentIndex < widgetsToShow.size(); ++i) {
                QWidget* v = widgetsToShow.at(currentIndex);
                v->show();
                currentIndex++;
            }
            
            if (currentIndex >= widgetsToShow.size()) {
                currentT->stop();
                currentT->deleteLater();
                if (this->filterTimer == currentT) {
                    this->filterTimer = nullptr;
                }
            }
        });
        
        currentT->start(1);
    }
    this->setUpdatesEnabled(true);
    VL->invalidate();
    VL->activate();
}


// --- INSTALLATION & DOWNLOADS ---
void MainWindow::clickDownloadMinecraft(QWidget *item){
    auto *btn = qobject_cast<QPushButton*>(sender());
    if(!btn) return;

    const QString name = btn->property("name").toString();
    const QString uuid = QUuid::createUuid().toString();

    auto connection = std::make_shared<QMetaObject::Connection>();
    *connection = connect(DOWNLOAD_MANAGER, &DownloadManager::finished, this, [this, connection, btn, item, uuid](bool success, const QString id) {
        if(id == uuid){
            if(success){
                updateProgress(0);
                btn->setText(tr("play"));
                btn->setProperty("isInstalled", true);
                item->setProperty("isInstalled", true);
                updateProgressBarTitle(tr("install_disable"));
                applyFilter();
            }
            QObject::disconnect(*connection); 
        }
    });
    emit downloadMinecraft((tr("install_active") + " " + name), uuid, name, btn->property("hash").toString(), btn->property("url").toString());
}
void MainWindow::clickPlayMinecraft(){
    auto *btn = qobject_cast<QPushButton*>(sender());

    if(!btn) return;
    QThread *thread = new QThread();
    QString name = btn->property("name").toString();
    gameParams params = {minecraftDirectory,
                        username, 
                        name, 
                        minecraftDirectory + "/versions/" + name,
                        minecraftDirectory + "/versions/" + name + "/natives",
                        minecraftDirectory + "/assets",
                        name,
                        "00000000-0000-0000-0000-000000000000",
                        "00000000000000000000000000000000",
                        "mojang",
                        btn->property("type").toString(),
                        "LunusikLauncher",
                        "v0.3.2 beta",
                        "200",
                        "200",
                    
                        btn->property("hash").toString(),
                        btn->property("url").toString(),
                        QString::number(RAM)};
    MinecraftManager *mm = new MinecraftManager(DOWNLOAD_MANAGER, params, this);
    mm->moveToThread(thread);
    connect(thread, &QThread::started, mm, &MinecraftManager::startMinecraft);
    connect(mm, &MinecraftManager::exit, thread, &QThread::quit);
    connect(mm, &MinecraftManager::exit, mm, &QObject::deleteLater);
    connect(mm, &MinecraftManager::exit, this, &MainWindow::show);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);

    thread->start();
}