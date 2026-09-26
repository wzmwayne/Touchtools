#include "ConfigPanel.h"
#include "WppConfigPage.h"
#include "UsbConfigPage.h"
#include "TtIpc.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QTextBrowser>
#include <QScrollArea>
#include <QScrollBar>
#include <QApplication>
#include <QMouseEvent>
#include <QTimer>
#include <QProcess>
#include <QLocalSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QGroupBox>
#include <QCheckBox>
#include <QFile>
#include <QDir>
#include <QMessageBox>
#include <windows.h>
#include <tlhelp32.h>
#include <shellapi.h>

ConfigPanel::ConfigPanel(QWidget *parent) : QWidget(parent) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    resize(620, 520);

    qApp->setStyleSheet(QString::fromUtf8(
        "QWidget { background: #111; color: #ddd; font-size: 13px; }"
        "QGroupBox {"
            "border: 1px solid #333; border-radius: 6px;"
            "margin-top: 14px; padding: 14px 10px 10px; font-weight: bold;"
        "}"
        "QGroupBox::title {"
            "subcontrol-origin: margin; left: 10px; padding: 0 4px;"
        "}"
        "QPushButton {"
            "background: #333; border: none; border-radius: 4px;"
            "padding: 7px 18px;"
        "}"
        "QPushButton:hover { background: #444; }"
        "QPushButton:pressed { background: #555; }"
        "QPushButton:disabled { background: #222; color: #555; }"
        "QSlider::groove:horizontal {"
            "background: #333; height: 6px; border-radius: 3px;"
        "}"
        "QSlider::handle:horizontal {"
            "background: #aaa; width: 14px; height: 14px;"
            "margin: -4px 0; border-radius: 7px;"
        "}"
        "QSpinBox { background: #222; border: 1px solid #333;"
            "border-radius: 4px; padding: 3px 6px; min-width: 56px; color: #fff; }"
        "QLabel { background: transparent; }"
    ));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // title bar
    m_titleBar = new QWidget;
    m_titleBar->setFixedHeight(32);
    m_titleBar->setStyleSheet("QWidget { background: #1a1a1a; border-bottom: 1px solid #333; }");
    auto *tbLayout = new QHBoxLayout(m_titleBar);
    tbLayout->setContentsMargins(10, 0, 4, 0);
    auto *titleLbl = new QLabel("TouchTools");
    titleLbl->setStyleSheet("font-weight: bold; color: #eee; background: transparent;");
    auto *closeBtn = new QPushButton(QString::fromUtf8("\u00d7"));
    closeBtn->setFixedSize(28, 28);
    closeBtn->setStyleSheet(
        "QPushButton { background: transparent; color: #fff;"
        "font-size: 15px; border: none; border-radius: 4px; }"
        "QPushButton:hover { background: #c33; color: #fff; }"
    );
    tbLayout->addWidget(titleLbl);
    tbLayout->addStretch();
    tbLayout->addWidget(closeBtn);
    root->addWidget(m_titleBar);
    connect(closeBtn, &QPushButton::clicked, this, &QWidget::close);

    // body: nav + content
    auto *body = new QHBoxLayout;
    body->setContentsMargins(0, 0, 0, 0);
    body->setSpacing(0);

    // nav list
    m_navList = new QListWidget;
    m_navList->setFixedWidth(150);
    m_navList->setFrameShape(QFrame::NoFrame);
    m_navList->setStyleSheet(
        "QListWidget { background: #181818; border-right: 1px solid #333; color: #999; }"
        "QListWidget::item { padding: 12px 14px; }"
        "QListWidget::item:selected { background: #333; color: #fff; }"
        "QListWidget::item:hover { background: #282828; color: #ddd; }"
    );
    m_navList->insertItem(0, "WPPTouchHelper");
    m_navList->insertItem(1, QString::fromUtf8("U\u76d8\u52a9\u624b"));
    m_navList->insertItem(2, QString::fromUtf8("\u5173\u4e8e TouchTools ..."));
    m_navList->setCurrentRow(0);

    // stacked content
    m_stack = new QStackedWidget;

    // page 0: WPPTouchHelper config
    auto *wppScroll = new QScrollArea;
    wppScroll->setWidgetResizable(true);
    wppScroll->setFrameShape(QFrame::NoFrame);
    wppScroll->setWidget(new WppConfigPage);
    m_stack->addWidget(wppScroll);        // 0: WPPTouchHelper

    // page 1: USBAssistant config
    auto *usbScroll = new QScrollArea;
    usbScroll->setWidgetResizable(true);
    usbScroll->setFrameShape(QFrame::NoFrame);
    usbScroll->setWidget(new UsbConfigPage);
    m_stack->addWidget(usbScroll);        // 1: USBAssistant

    // page 2: about
    auto *aboutPage = new QWidget;
    auto *aboutLayout = new QVBoxLayout(aboutPage);
    aboutLayout->setContentsMargins(24, 24, 24, 24);
    auto *aboutTitle = new QLabel(QString::fromUtf8(
        "<h2 style='color:#eee;margin:0 0 8px 0;'>TouchTools</h2>"
        "<p style='color:#999;margin:0 0 16px 0;'>Windows \u7efc\u5408\u5de5\u5177\u96c6</p>"));
    aboutTitle->setTextFormat(Qt::RichText);
    aboutTitle->setStyleSheet("background: transparent;");
    aboutLayout->addWidget(aboutTitle);

    auto *aboutText = new QTextBrowser;
    aboutText->setOpenExternalLinks(true);
    aboutText->setHtml(QString::fromUtf8(
        "<hr style='border-color:#333;'>"
        "<p>\u7248\u6743 (C) 2026 Wayne</p>"
        "<p>\u672c\u7a0b\u5e8f\u662f\u81ea\u7531\u8f6f\u4ef6\uff1a"
        "\u60a8\u53ef\u4ee5\u91cd\u65b0\u53d1\u5e03\u5b83\u6216\u4fee\u6539\u5b83\uff0c"
        "\u524d\u63d0\u662f\u9075\u5b88 "
        "<a href=\"https://www.gnu.org/licenses/gpl-3.0.html\">GNU General Public License v3</a>\u3002</p>"
        "<p>\u672c\u7a0b\u5e8f\u53d1\u5e03\u65f6\u5e0c\u671b\u5b83\u6709\u7528\uff0c"
        "\u4f46\u4e0d\u63d0\u4f9b\u4efb\u4f55\u4fdd\u8bc1\uff1b"
        "\u65e0\u4e8b\u5148\u7684\u5546\u4e1a\u6027\u6216\u7279\u5b9a\u76ee\u7684\u9002\u7528\u6027\u4fdd\u8bc1\u3002</p>"
        "<hr style='border-color:#333;'>"
        "<p>\u6b64\u7a0b\u5e8f\u4f7f\u7528 Qt \u6846\u67b6\u5f00\u53d1\u3002"
        "Qt \u662f The Qt Company \u7684\u5546\u6807\u3002</p>"
        "<p>\u60a8\u53ef\u4ee5\u4ece\u4ee5\u4e0b\u94fe\u63a5\u83b7\u53d6 Qt \u6e90\u4ee3\u7801\uff1a"
        "<br><a href=\"https://download.qt.io/\">https://download.qt.io/</a></p>"
        "<p>\u6216\u4f7f\u7528\u672c\u5730 Qt \u5b89\u88c5\u76ee\u5f55\u4e2d\u7684\u6e90\u4ee3\u7801\uff1a"
        "<br>E:\\Qt\\6.11.1\\Src</p>"
    ));
    aboutText->setStyleSheet(
        "QTextBrowser { background: transparent; border: none; color: #bbb; }"
        "a { color: #6af; }"
    );
    aboutLayout->addWidget(aboutText, 1);
    m_stack->addWidget(aboutPage);        // 2: About

    body->addWidget(m_navList);
    body->addWidget(m_stack, 1);
    root->addLayout(body, 1);

    connect(m_navList, &QListWidget::currentRowChanged,
            m_stack, &QStackedWidget::setCurrentIndex);
    connect(m_stack, &QStackedWidget::currentChanged,
            this, &ConfigPanel::onTabChanged);

    // activate initial tab
    onTabChanged(0);

    // deferred TTcore check
    QTimer::singleShot(200, this, [this]() {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        bool running = false;
        if (snap != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32W pe = { sizeof(pe) };
            if (Process32FirstW(snap, &pe)) {
                do {
                    if (_wcsicmp(pe.szExeFile, L"TTcore.exe") == 0) {
                        running = true;
                        break;
                    }
                } while (Process32NextW(snap, &pe));
            }
            CloseHandle(snap);
        }
        if (!running) {
            auto ret = QMessageBox::question(this,
                QString::fromUtf8("TTcore"),
                QString::fromUtf8("TTcore \u5b88\u62a4\u8fdb\u7a0b\u672a\u8fd0\u884c\u3002\n\u662f\u5426\u542f\u52a8\uff1f"),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
            if (ret == QMessageBox::Yes) {
                QString exe = QDir::toNativeSeparators(
                    QCoreApplication::applicationDirPath() + "/TTcore.exe");
                ShellExecuteW(NULL, L"runas",
                    reinterpret_cast<const wchar_t*>(exe.utf16()),
                    NULL, NULL, SW_SHOW);
            }
        }
    });
}

void ConfigPanel::onTabChanged(int index) {
    auto *wpp = m_stack->widget(0)->findChild<WppConfigPage*>();
    if (wpp) wpp->setActive(index == 0);
    auto *usb = m_stack->widget(1)->findChild<UsbConfigPage*>();
    if (usb) usb->setActive(index == 1);
}

void ConfigPanel::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && event->position().y() <= m_titleBar->height()) {
        m_dragging = true;
        m_dragPos = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}

void ConfigPanel::mouseMoveEvent(QMouseEvent *event) {
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - m_dragPos);
        event->accept();
    }
}

void ConfigPanel::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        event->accept();
    }
}
