#include "ConfigPanel.h"
#include "WppConfigPage.h"
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
#include <windows.h>
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
    auto *titleLbl = new QLabel("Touchtools");
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
    m_navList->insertItem(0, "TTcore");
    m_navList->insertItem(1, "WPPTouchHelper");
    m_navList->insertItem(2, QString::fromUtf8("\u5173\u4e8e Touchtools ..."));
    m_navList->setCurrentRow(0);

    // stacked content
    m_stack = new QStackedWidget;

    // ── page 0: TTcore management ──
    auto *ttPage = new QWidget;
    auto *ttLayout = new QVBoxLayout(ttPage);
    ttLayout->setContentsMargins(16, 10, 16, 12);
    ttLayout->setSpacing(10);

    // header: status + control
    auto *ttHeader = new QHBoxLayout;
    auto *ttIcon = new QLabel(QString::fromUtf8("\u25cf"));
    ttIcon->setStyleSheet("color: #666; font-size: 18px;");
    ttIcon->setFixedWidth(20);
    auto *ttStatus = new QLabel(QString::fromUtf8("\u68c0\u67e5\u4e2d..."));
    auto *ttStartBtn = new QPushButton(QString::fromUtf8("\u25b6 \u542f\u52a8"));
    auto *ttStopBtn  = new QPushButton(QString::fromUtf8("\u25a0 \u505c\u6b62"));
    ttStopBtn->setEnabled(false);
    ttHeader->addWidget(ttIcon);
    ttHeader->addWidget(ttStatus);
    ttHeader->addStretch();
    ttHeader->addWidget(ttStartBtn);
    ttHeader->addWidget(ttStopBtn);
    ttLayout->addLayout(ttHeader);

    // component management
    auto *cg = new QGroupBox(QString::fromUtf8("\u7ec4\u4ef6\u7ba1\u7406"));
    auto *cl = new QVBoxLayout(cg);
    auto *wppCb = new QCheckBox(QString::fromUtf8("WPPTouchHelper\uff08\u5e7b\u706f\u7247\u63a7\u4ef6\uff09"));
    auto *usbCb = new QCheckBox(QString::fromUtf8("U\u76d8\u52a9\u624b"));
    wppCb->setStyleSheet("QCheckBox { color: #ddd; font-size: 13px; spacing: 8px; } QCheckBox::indicator { width: 16px; height: 16px; }");
    usbCb->setStyleSheet("QCheckBox { color: #ddd; font-size: 13px; spacing: 8px; } QCheckBox::indicator { width: 16px; height: 16px; }");
    cl->addWidget(wppCb);
    cl->addWidget(usbCb);
    ttLayout->addWidget(cg);

    // auto-start
    auto *asCb = new QCheckBox(QString::fromUtf8("TTcore \u968f\u7cfb\u7edf\u542f\u52a8\uff08\u5efa\u8bae\u5f00\u542f\uff09"));
    asCb->setStyleSheet("QCheckBox { color: #ddd; font-size: 13px; spacing: 8px; } QCheckBox::indicator { width: 16px; height: 16px; }");
    ttLayout->addWidget(asCb);

    // check TTcore scheduled task status
    QProcess asProc;
    asProc.start("powershell", {
        "-NoProfile", "-Command",
        "Get-ScheduledTask -TaskName 'ttcore' -ErrorAction SilentlyContinue"
    });
    asProc.waitForFinished(3000);
    asCb->setChecked(asProc.exitCode() == 0
        && asProc.readAllStandardOutput().contains("ttcore"));

    // log
    auto *lg = new QGroupBox(QString::fromUtf8("\u8fd0\u884c\u65e5\u5fd7"));
    auto *ll = new QVBoxLayout(lg);
    auto *logBrowser = new QTextBrowser;
    logBrowser->setStyleSheet(
        "QTextBrowser { background: #0a0a0a; color: #0f0;"
        "font-family: Consolas, 'Courier New', monospace;"
        "font-size: 11px; border: 1px solid #333; border-radius: 4px; }"
    );
    logBrowser->setMinimumHeight(140);
    ll->addWidget(logBrowser);
    ttLayout->addWidget(lg, 1);

    // save button
    auto *saveBtn = new QPushButton(QString::fromUtf8("\u4fdd\u5b58\u914d\u7f6e"));
    saveBtn->setStyleSheet(
        "QPushButton { background: #555; color: #fff; font-weight: bold;"
        "padding: 9px 0; border-radius: 4px; }"
        "QPushButton:hover { background: #666; }"
        "QPushButton:pressed { background: #777; }"
    );
    ttLayout->addWidget(saveBtn);

    // ── TTcore page timer (poll status + log) ──
    auto *pollTimer = new QTimer(ttPage);
    auto doPoll = [=]() {
        QLocalSocket sock;
        sock.connectToServer("TTCorePipe");
        if (!sock.waitForConnected(200)) {
            ttIcon->setStyleSheet("color: #866; font-size: 18px;");
            ttStatus->setText(QString::fromUtf8("\u5df2\u505c\u6b62"));
            ttStartBtn->setEnabled(true);
            ttStopBtn->setEnabled(false);
            return;
        }

        ttIcon->setStyleSheet("color: #6a6; font-size: 18px;");
        ttStatus->setText(QString::fromUtf8("\u8fd0\u884c\u4e2d"));
        ttStartBtn->setEnabled(false);
        ttStopBtn->setEnabled(true);

        // fetch log
        sock.write("log");
        sock.waitForBytesWritten(200);
        if (sock.waitForReadyRead(500)) {
            auto resp = QJsonDocument::fromJson(sock.readAll());
            if (resp.isArray()) {
                QString html;
                for (const auto &e : resp.array())
                    html += e.toString().toHtmlEscaped() + "<br>";
                logBrowser->setHtml(html);
                logBrowser->verticalScrollBar()->setValue(
                    logBrowser->verticalScrollBar()->maximum());
            }
        }
        sock.disconnectFromServer();
    };
    connect(pollTimer, &QTimer::timeout, doPoll);
    pollTimer->start(2000);

    // ── TTcore page buttons ──
    connect(ttStartBtn, &QPushButton::clicked, [=]() {
        QString exe = QCoreApplication::applicationDirPath() + "/TTcore.exe";
        QProcess::startDetached(exe);
        QTimer::singleShot(500, doPoll);
    });

    connect(ttStopBtn, &QPushButton::clicked, [=]() {
        QLocalSocket sock;
        sock.connectToServer("TTCorePipe");
        if (sock.waitForConnected(300)) {
            sock.write("exit");
            sock.waitForBytesWritten(300);
            sock.disconnectFromServer();
        }
        QTimer::singleShot(500, doPoll);
    });

    // ── Save ──
    auto doSave = [=]() {
        QString path = QCoreApplication::applicationDirPath() + "/touchtools.json";
        QFile f(path);
        QJsonObject cfg;
        if (f.open(QIODevice::ReadOnly)) {
            cfg = QJsonDocument::fromJson(f.readAll()).object();
            f.close();
        }
        cfg["wpptouchhelper_enabled"] = wppCb->isChecked();
        cfg["usb_enabled"] = usbCb->isChecked();
        if (f.open(QIODevice::WriteOnly)) {
            f.write(QJsonDocument(cfg).toJson());
            f.close();
        }

        // TTcore auto-start
        bool wantStart = asCb->isChecked();
        bool hasTask = false;
        {
            QProcess p;
            p.start("powershell", {
                "-NoProfile", "-Command",
                "Get-ScheduledTask -TaskName 'ttcore' -ErrorAction SilentlyContinue"
            });
            p.waitForFinished(3000);
            hasTask = p.exitCode() == 0 && p.readAllStandardOutput().contains("ttcore");
        }

        if (wantStart && !hasTask) {
            QString exe = QDir::toNativeSeparators(
                QCoreApplication::applicationDirPath() + "/TTcore.exe");
            QString wd  = QDir::toNativeSeparators(
                QCoreApplication::applicationDirPath());
            QString ps = QString(
                "$a=New-ScheduledTaskAction -Execute '%1' -Argument '-autostart' -WorkingDirectory '%2'; "
                "$t=New-ScheduledTaskTrigger -AtLogOn; "
                "$s=New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries; "
                "Register-ScheduledTask -TaskName 'ttcore' -Action $a -Trigger $t -Settings $s -Force"
            ).arg(exe, wd);
            ShellExecuteW(NULL, L"runas", L"powershell",
                (L"-NoProfile -Command \"" + ps.toStdWString() + L"\"").c_str(),
                NULL, SW_HIDE);
        } else if (!wantStart && hasTask) {
            ShellExecuteW(NULL, L"runas", L"powershell",
                L"-NoProfile -Command \"Unregister-ScheduledTask -TaskName 'ttcore' -Confirm:$false\"",
                NULL, SW_HIDE);
        }

        // tell TTcore to reload
        QLocalSocket sock;
        sock.connectToServer("TTCorePipe");
        if (sock.waitForConnected(300)) {
            sock.write("reload");
            sock.waitForBytesWritten(300);
            sock.disconnectFromServer();
        }

        QTimer::singleShot(400, doPoll);
    };

    connect(saveBtn, &QPushButton::clicked, doSave);

    // initial poll
    doPoll();

    auto *ttScroll = new QScrollArea;
    ttScroll->setWidget(ttPage);
    ttScroll->setWidgetResizable(true);
    ttScroll->setFrameShape(QFrame::NoFrame);
    m_stack->addWidget(ttScroll);        // 0: TTcore

    // page 1: WPPTouchHelper config
    auto *wppScroll = new QScrollArea;
    wppScroll->setWidgetResizable(true);
    wppScroll->setFrameShape(QFrame::NoFrame);
    wppScroll->setWidget(new WppConfigPage);
    m_stack->addWidget(wppScroll);        // 1: WPPTouchHelper

    // page 1: about
    auto *aboutPage = new QWidget;
    auto *aboutLayout = new QVBoxLayout(aboutPage);
    aboutLayout->setContentsMargins(24, 24, 24, 24);
    auto *aboutTitle = new QLabel(QString::fromUtf8(
        "<h2 style='color:#eee;margin:0 0 8px 0;'>Touchtools</h2>"
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
    m_stack->addWidget(aboutPage);       // 2: About

    body->addWidget(m_navList);
    body->addWidget(m_stack, 1);
    root->addLayout(body, 1);

    connect(m_navList, &QListWidget::currentRowChanged,
            m_stack, &QStackedWidget::setCurrentIndex);
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
