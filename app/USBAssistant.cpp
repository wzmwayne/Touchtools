#include "USBAssistant.h"
#include "Config.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScreen>
#include <QGuiApplication>
#include <QGraphicsDropShadowEffect>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QUrl>
#include <windows.h>
#include <dbt.h>

USBAssistant::USBAssistant(QWidget *parent)
    : QWidget(parent) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(340, 80);

    auto *shadow = new QGraphicsDropShadowEffect;
    shadow->setBlurRadius(20);
    shadow->setOffset(0, 4);
    shadow->setColor(QColor(0, 0, 0, 120));
    setGraphicsEffect(shadow);

    auto *container = new QWidget(this);
    container->setObjectName("container");
    container->setStyleSheet(
        "#container {"
            "background: rgba(40, 40, 40, 235);"
            "border: 1px solid rgba(255, 255, 255, 25);"
            "border-radius: 8px;"
        "}"
    );

    auto *mainLayout = new QHBoxLayout(container);
    mainLayout->setContentsMargins(14, 10, 14, 10);
    mainLayout->setSpacing(12);

    auto *iconLb = new QLabel(QString::fromUtf8("\xf0\x9f\x92\xbe"));
    iconLb->setStyleSheet("font-size: 24px; background: transparent;");

    auto *textLayout = new QVBoxLayout;
    textLayout->setSpacing(1);
    m_titleLabel = new QLabel(QString::fromUtf8("U\u76d8\u5df2\u63d2\u5165"));
    m_titleLabel->setStyleSheet("color: white; font-size: 13px; font-weight: bold; background: transparent;");
    m_infoLabel = new QLabel;
    m_infoLabel->setStyleSheet("color: rgba(255,255,255,170); font-size: 11px; background: transparent;");
    textLayout->addWidget(m_titleLabel);
    textLayout->addWidget(m_infoLabel);

    m_openBtn = new QPushButton(QString::fromUtf8("\u6253\u5f00U\u76d8"));
    m_openBtn->setFixedSize(76, 30);
    m_openBtn->setCursor(Qt::PointingHandCursor);
    m_openBtn->setStyleSheet(
        "QPushButton {"
            "background: #4CAF50; color: white; font-size: 12px;"
            "font-weight: bold; border: none; border-radius: 4px;"
        "}"
        "QPushButton:hover { background: #45a049; }"
        "QPushButton:pressed { background: #3d8b40; }"
    );

    mainLayout->addWidget(iconLb);
    mainLayout->addLayout(textLayout, 1);
    mainLayout->addWidget(m_openBtn);

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(6, 6, 6, 6);
    outer->addWidget(container);

    connect(m_openBtn, &QPushButton::clicked, this, &USBAssistant::onOpenDrive);

    m_hideTimer.setSingleShot(true);
    connect(&m_hideTimer, &QTimer::timeout, this, [this]() { hide(); });

    QCoreApplication::instance()->installNativeEventFilter(this);
}

USBAssistant::~USBAssistant() {
    QCoreApplication::instance()->removeNativeEventFilter(this);
}

bool USBAssistant::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) {
    if (eventType != "windows_generic_MSG" && eventType != "windows_dispatcher_MSG")
        return false;

    MSG *msg = static_cast<MSG*>(message);
    if (msg->message == WM_DEVICECHANGE && msg->wParam == DBT_DEVICEARRIVAL) {
        auto *hdr = reinterpret_cast<DEV_BROADCAST_HDR*>(msg->lParam);
        if (hdr && hdr->dbch_devicetype == DBT_DEVTYP_VOLUME) {
            auto *vol = reinterpret_cast<DEV_BROADCAST_VOLUME*>(hdr);
            char drive = driveFromMask(vol->dbcv_unitmask);
            if (drive) {
                QString drivePath = QString("%1:\\").arg(drive);
                QTimer::singleShot(600, this, [this, drivePath]() {
                    deliverPopup(drivePath);
                });
            }
        }
    }
    return false;
}

void USBAssistant::deliverPopup(const QString &drivePath) {
    if (!Config::instance().usbEnabled()) return;

    m_drivePath = drivePath;
    QString name = volumeNameForDrive(drivePath);
    m_infoLabel->setText(QString("%1 (%2)").arg(name, drivePath));

    QRect screenRect = QGuiApplication::primaryScreen()->availableGeometry();
    move(screenRect.right() - width() - 12, screenRect.bottom() - height() - 12);

    show();
    raise();
    m_hideTimer.start(Config::instance().usbPopupDuration());
}

void USBAssistant::onOpenDrive() {
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_drivePath));
    hide();
}

QString USBAssistant::volumeNameForDrive(const QString &drivePath) {
    WCHAR buf[MAX_PATH] = {};
    if (GetVolumeInformationW(reinterpret_cast<const WCHAR*>(drivePath.utf16()),
                              buf, MAX_PATH, nullptr, nullptr, nullptr, nullptr, 0)) {
        return QString::fromWCharArray(buf);
    }
    return QString::fromUtf8("\u53ef\u79fb\u52a8\u78c1\u76d8");
}

char USBAssistant::driveFromMask(DWORD unitmask) {
    for (char d = 'A'; d <= 'Z'; ++d) {
        if (unitmask & 1) return d;
        unitmask >>= 1;
    }
    return 0;
}
