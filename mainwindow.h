#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "libs.h"
#include "downloadmanager.h"
#include "versionsmanager.h"
#include "minecraftmanager.h"

class GradientLabel : public QLabel {
public:
    GradientLabel(const QString &text, QColor start, QColor end, QWidget *parent = nullptr)
        : QLabel(text, parent), startColor(start), endColor(end) {}
    void setColors(QColor start, QColor end) {
        startColor = start;
        endColor = end;
        update();
    }
private:
    QColor startColor;
    QColor endColor;
protected:
    void paintEvent(QPaintEvent *event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        QLinearGradient gradient(0, 0, width(), 0);
        gradient.setColorAt(0.0, startColor);
        gradient.setColorAt(1.0, endColor);

        painter.setFont(font());
        
        QPen pen;
        pen.setBrush(gradient);
        painter.setPen(pen);
        
        painter.drawText(rect(), alignment(), text());
    }
};


class EditableLabel : public QStackedWidget {
    Q_OBJECT
public:
    explicit EditableLabel(QWidget *parent = nullptr) : QStackedWidget(parent) {
        label = new QLabel();
        editor = new QLineEdit();

        label->setCursor(Qt::PointingHandCursor);

        this->addWidget(label);
        this->addWidget(editor);

        label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        editor->setAlignment(Qt::AlignRight);
        
        label->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        editor->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        this->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

        connect(editor, &QLineEdit::editingFinished, this, &EditableLabel::finishEditing);
    }
    void setText(const QString &text) {
        QString displayText(text);
        if (displayText.length() > 46) {
            displayText = displayText.left(43) + "...";
        }
        label->setText(displayText);
        editor->setText(text);
    }

    QString text() const {
        return label->text();
    }

protected:
    void mousePressEvent(QMouseEvent *event) override {
        if (this->currentWidget() == label) {
            this->setCurrentWidget(editor);
            editor->setFocus();
        }
    }
signals:
    void textUpdate(QString newText);

private slots:
    void finishEditing() {
        QString newText = editor->text();
        QString displayText(newText);
        if (displayText.length() > 46) {
            displayText = displayText.left(43) + "...";
        }
        label->setText(displayText);
        this->setCurrentWidget(label);
        emit textUpdate(newText);
    }

private:
    QLabel *label;
    QLineEdit *editor;
};


class FlowLayout : public QLayout {
public:
    void clearLayout() {
        QLayoutItem *item;
        while ((item = this->takeAt(0))) {
            if (item->widget()) {
                item->widget()->deleteLater();
            }
            delete item;
        }
    }
    explicit FlowLayout(QWidget *parent = nullptr, int margin = -1, int hSpacing = -1, int vSpacing = -1)
        : QLayout(parent), m_hSpace(hSpacing), m_vSpace(vSpacing) {
        setContentsMargins(margin, margin, margin, margin);
    }
    ~FlowLayout() { QLayoutItem *item; while ((item = takeAt(0))) delete item; }
    
    void addItem(QLayoutItem *item) override { itemList.append(item); }
    int horizontalSpacing() const { return m_hSpace >= 0 ? m_hSpace : smartSpacing(QStyle::PM_LayoutHorizontalSpacing); }
    int verticalSpacing() const { return m_vSpace >= 0 ? m_vSpace : smartSpacing(QStyle::PM_LayoutVerticalSpacing); }
    int count() const override { return itemList.size(); }
    QLayoutItem *itemAt(int index) const override { return itemList.value(index); }
    QLayoutItem *takeAt(int index) override { return (index >= 0 && index < itemList.size()) ? itemList.takeAt(index) : nullptr; }
    Qt::Orientations expandingDirections() const override { return { }; }
    bool hasHeightForWidth() const override { return true; }
    int heightForWidth(int width) const override { return doLayout(QRect(0, 0, width, 0), true); }
    void setGeometry(const QRect &rect) override { QLayout::setGeometry(rect); doLayout(rect, false); }
    QSize sizeHint() const override { return minimumSize(); }
    QSize minimumSize() const override {
        QSize size;
        for (const auto *item : std::as_const(itemList)) size = size.expandedTo(item->minimumSize());
        const auto margins = contentsMargins();
        return size + QSize(margins.left() + margins.right(), margins.top() + margins.bottom());
    }

private:
    int doLayout(const QRect &rect, bool testOnly) const {
        int left, top, right, bottom;
        getContentsMargins(&left, &top, &right, &bottom);
        QRect effectiveRect = rect.adjusted(+left, +top, -right, -bottom);
        int x = effectiveRect.x();
        int y = effectiveRect.y();
        int lineHeight = 0;

        for (auto *item : itemList) {
            int nextX = x + item->sizeHint().width() + horizontalSpacing();
            if (nextX - horizontalSpacing() > effectiveRect.right() && lineHeight > 0) {
                x = effectiveRect.x();
                y = y + lineHeight + verticalSpacing();
                nextX = x + item->sizeHint().width() + horizontalSpacing();
                lineHeight = 0;
            }
            if (!testOnly) item->setGeometry(QRect(QPoint(x, y), item->sizeHint()));
            x = nextX;
            lineHeight = qMax(lineHeight, item->sizeHint().height());
        }
        return y + lineHeight - rect.y() + bottom;
    }
    int smartSpacing(QStyle::PixelMetric pm) const {
        QObject *p = parent();
        if (!p) return -1;
        if (p->isWidgetType()) return static_cast<QWidget *>(p)->style()->pixelMetric(pm, nullptr, static_cast<QWidget *>(p));
        return static_cast<QLayout *>(p)->parentWidget()->style()->pixelMetric(pm, nullptr, nullptr);
    }
    QList<QLayoutItem *> itemList;
    int m_hSpace, m_vSpace;
};


class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QSplashScreen *splash, QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void updateProgressBarTitle(const QString text);
    void updateProgress(const qint64 precent);
    void showOrHideProgress(const bool show);
    void renderVersions(const QList<VersionData> versions, const QList<LatestVersionData> latestVersions);
    
signals:
    void loadVersions(const bool isUpdateRequired = false);
    void downloadMinecraft(const QString text, const QString id, const QString name, const QString hashManifest, const QString url);
    void setMinecraftDirectoryDM(const QString minecraftDirectory);
    void setMinecraftDirectoryVM(const QString minecraftDirectory);
private:
    QThread* threadDownloadManager;
    QThread* threadVersionsManager;
    
    DownloadManager *DOWNLOAD_MANAGER;
    VersionsManager *VERSIONS_MANAGER;

    // --- UI & STYLING ---
    void setupStyles();
    QGraphicsDropShadowEffect* createGlow(const int &blur, const QColor &color);
    int scalingIcon(int size);
    bool eventFilter(QObject *obj, QEvent *event);
    void resizeEvent(QResizeEvent *event);

    // --- SETTINGS & LOCALIZATION ---
    QString minecraftDirectory;
    int RAM;
    QString username;
    QComboBox *settingsLanguage;
    void showInfo();
    int getTotalRAM();
    void readSettings();
    void changeLanguage(int index);
    void changeRAM(const int val);
    void changeUsername(const QString newUsername);
    void setLanguage(QString langCode);
    void updateStrings();
    void selectMinecraftDirectory();


    // --- VERSION MANAGEMENT ---
    QWidget* addVersionItem(const VersionData &versionData);
    void addLatestVersionItem(const LatestVersionData &latestVersionData);
    void clearVersionsList();
    void clearLatestVersionsList();


    // --- FILTERING & SEARCH ---
    QTimer *filterTimer = nullptr;
    QList<QPushButton*> filterBtns;
    QStringList filters;
    QString searchQuery;
    void clickFilterBtn();
    void searchInpChanged(const QString &text);
    void applyFilter();
    void applyFilter(const QStringList &allowedTypes, const QString &searchText);


    
    // --- INSTALLATION & DOWNLOADS ---
    void clickDownloadMinecraft(QWidget *item);
    void clickPlayMinecraft();
    
    QList<QWidget*> allVersions;
    QWidget *SettingsOverlay;
    QJsonObject strings;
    QVBoxLayout *VL;
    QGridLayout *latestVersionsLayout;

    // --- STRINGS ---
    GradientLabel *logoText;
    QLabel *usernameLabel;
    EditableLabel *usernameEL;
    QLabel *versionsTitleText;
    QLabel *filterTitleText;
    FlowLayout *typeFlow;
    QLineEdit *searchInp;
    QLabel *serverTitleText;
    QLabel *versionLabel;
    QPushButton *levelPathBtn;
    QPushButton *saveBtn;
    QPushButton *startBtn;
    QWidget *widgetProgressSection;
    QLabel *progressTitleText;
    QProgressBar *progressBar;
    QLabel *progressBarPrecent;
    QLabel *footer;
    QLabel *settingsHeaderTitle;
    QPushButton *MinecraftPathBtn;
    QLabel *FLSLPL;
    QLabel *FLSLL;
    QLabel *FLSRL;

    // --- ARRAY STRINGS ---
    QList<QLabel*> typeLabelList;
    QList<QPushButton*> btnDownloadList;
    QList<QLabel*> latestTypeLabelList;
    QList<QPushButton*> latestBtnDownloadList;
};
#endif