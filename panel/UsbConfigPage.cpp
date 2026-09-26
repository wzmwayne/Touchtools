#include "UsbConfigPage.h"
#include "TtIpc.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFile>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QCoreApplication>
#include <QProcess>
#include <QDir>
#include <QLocalSocket>
#include <windows.h>
#include <tlhelp32.h>
#include <cstdio>

#define LOG(fmt, ...) do { \
    fprintf(stderr, "[TouchtoolsPanel] " fmt "\n", ##__VA_ARGS__); \
    fflush(stderr); \
} while(0)

static bool isUsbAssistantRunning() {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return false;
    PROCESSENTRY32W pe = { sizeof(pe) };
    bool found = false;
    if (Process32FirstW(snapshot, &pe)) {
        do {
            if (QString::fromWCharArray(pe.szExeFile).compare("USBAssistant.exe", Qt::CaseInsensitive) == 0) {
                found = true;
                break;
            }
        } while (Process32NextW(snapshot, &pe));
    }
    CloseHandle(snapshot);
    return found;
}

UsbConfigPage::UsbConfigPage(QWidget *parent) : QWidget(parent) {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 10, 16, 12);
    root->setSpacing(10);

    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, &UsbConfigPage::updateStatus);
    m_timer->start();

    // header
    auto *header = new QHBoxLayout;
    m_statusIcon = new QLabel(QString::fromUtf8("\u25cf"));
    m_statusIcon->setStyleSheet("color: #6a6; font-size: 18px; background: transparent;");
    m_statusIcon->setFixedWidth(20);
    m_statusText = new QLabel(QString::fromUtf8("\u68c0\u67e5\u4e2d..."));
    m_startBtn = new QPushButton(QString::fromUtf8("\u25b6 \u542f\u52a8"));
    m_stopBtn  = new QPushButton(QString::fromUtf8("\u25a0 \u505c\u6b62"));
    m_stopBtn->setEnabled(false);
    header->addWidget(m_statusIcon);
    header->addWidget(m_statusText);
    header->addStretch();
    header->addWidget(m_startBtn);
    header->addWidget(m_stopBtn);
    root->addLayout(header);

    // enable checkbox
    m_enabledCb = new QCheckBox(QString::fromUtf8("\u542f\u7528U\u76d8\u52a9\u624b\uff08\u63d2\u5165U\u76d8\u65f6\u53f3\u4e0b\u89d2\u5f39\u7a97\u63d0\u793a\uff09"));
    m_enabledCb->setStyleSheet("QCheckBox { color: #ddd; font-size: 13px; spacing: 8px; } QCheckBox::indicator { width: 16px; height: 16px; }");
    root->addWidget(m_enabledCb);

    // settings group
    auto *sg = new QGroupBox(QString::fromUtf8("\u5f39\u7a97\u8bbe\u7f6e"));
    auto *sl = new QVBoxLayout(sg);
    sl->addWidget(makeSlider(QString::fromUtf8("\u5f39\u7a97\u6301\u7eed\u65f6\u95f4"), "usb_popup_duration", 2000, 15000, 5000));
    root->addWidget(sg);

    // save
    m_saveBtn = new QPushButton(QString::fromUtf8("\u4fdd\u5b58\u914d\u7f6e"));
    m_saveBtn->setStyleSheet(
        "QPushButton { background: #555; color: #fff; font-weight: bold;"
        "padding: 9px 0; border-radius: 4px; }"
        "QPushButton:hover { background: #666; }"
        "QPushButton:pressed { background: #777; }"
    );
    root->addWidget(m_saveBtn);

    root->addStretch();

    connect(m_startBtn, &QPushButton::clicked, this, &UsbConfigPage::onStart);
    connect(m_stopBtn,  &QPushButton::clicked, this, &UsbConfigPage::onStop);
    connect(m_saveBtn,  &QPushButton::clicked, this, &UsbConfigPage::onSave);

    loadCfg();
    updateStatus();
    m_timer->stop();
}

void UsbConfigPage::setActive(bool active) {
    if (active) {
        updateStatus();
        m_timer->start();
    } else {
        m_timer->stop();
    }
}

void UsbConfigPage::loadCfg() {
    QString path = QCoreApplication::applicationDirPath() + "/touchtools.json";
    QFile f(path);
    if (f.open(QIODevice::ReadOnly)) {
        QJsonParseError err;
        m_cfg = QJsonDocument::fromJson(f.readAll(), &err).object();
        if (err.error != QJsonParseError::NoError)
            LOG("loadCfg: JSON parse error: %s", qPrintable(err.errorString()));
        f.close();
    }
    m_enabledCb->setChecked(m_cfg.value("usb_enabled").toBool(true));
    for (auto &e : m_entries) {
        int v = m_cfg.contains(e.key) ? m_cfg[e.key].toInt() : e.sl->value();
        e.sl->setValue(v);
        e.sp->setValue(v);
    }
}

void UsbConfigPage::saveCfg() {
    QString path = QCoreApplication::applicationDirPath() + "/touchtools.json";
    QFile f(path);
    if (f.open(QIODevice::ReadOnly)) {
        m_cfg = QJsonDocument::fromJson(f.readAll()).object();
        f.close();
    }
    m_cfg["usb_enabled"] = m_enabledCb->isChecked();
    for (auto &e : m_entries)
        m_cfg[e.key] = e.sl->value();
    if (f.open(QIODevice::WriteOnly)) {
        f.write(QJsonDocument(m_cfg).toJson());
        f.close();
    }
}

void UsbConfigPage::updateStatus() {
    bool running = isUsbAssistantRunning();

    if (running) {
        m_statusIcon->setStyleSheet("color: #6a6; font-size: 18px; background: transparent;");
        m_statusText->setText(QString::fromUtf8("\u8fd0\u884c\u4e2d"));
        m_startBtn->setEnabled(false);
        m_stopBtn->setEnabled(true);
    } else {
        m_statusIcon->setStyleSheet("color: #866; font-size: 18px; background: transparent;");
        m_statusText->setText(QString::fromUtf8("\u5df2\u505c\u6b62"));
        m_startBtn->setEnabled(true);
        m_stopBtn->setEnabled(false);
    }
}

void UsbConfigPage::onStart() {
    if (isUsbAssistantRunning()) return;
    QString exe = QCoreApplication::applicationDirPath() + "/USBAssistant.exe";
    ShellExecuteW(NULL, L"runas",
        reinterpret_cast<const wchar_t*>(QDir::toNativeSeparators(exe).utf16()),
        NULL, NULL, SW_SHOW);
    QTimer::singleShot(500, this, &UsbConfigPage::updateStatus);
}

void UsbConfigPage::onStop() {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe = { sizeof(pe) };
        if (Process32FirstW(snapshot, &pe)) {
            do {
                if (QString::fromWCharArray(pe.szExeFile).compare("USBAssistant.exe", Qt::CaseInsensitive) == 0) {
                    HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                    if (hProc) {
                        TerminateProcess(hProc, 0);
                        CloseHandle(hProc);
                    }
                }
            } while (Process32NextW(snapshot, &pe));
        }
        CloseHandle(snapshot);
    }
    QTimer::singleShot(500, this, &UsbConfigPage::updateStatus);
}

void UsbConfigPage::onSave() {
    saveCfg();
    ttIpcSend("reload");
}

QWidget* UsbConfigPage::makeSlider(const QString &label, const QString &key,
                                 int min, int max, int val) {
    auto *row = new QWidget;
    auto *hl = new QHBoxLayout(row);
    hl->setContentsMargins(0, 2, 0, 2);

    auto *lb = new QLabel(label);
    lb->setFixedWidth(64);
    auto *sl = new QSlider(Qt::Horizontal);
    sl->setRange(min, max);
    sl->setValue(val);
    auto *sp = new QSpinBox;
    sp->setRange(min, max);
    sp->setValue(val);

    hl->addWidget(lb);
    hl->addWidget(sl, 1);
    hl->addWidget(sp);

    m_entries.append({key, sl, sp});

    connect(sl, &QSlider::valueChanged, sp, &QSpinBox::setValue);
    connect(sp, QOverload<int>::of(&QSpinBox::valueChanged), sl, &QSlider::setValue);

    return row;
}
