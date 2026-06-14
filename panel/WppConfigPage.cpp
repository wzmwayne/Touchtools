#include "WppConfigPage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QProcess>
#include <QFile>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QCoreApplication>
#include <QLocalSocket>
#include <windows.h>
#include <cstdio>

#define LOG(fmt, ...) do { \
    fprintf(stderr, "[TouchtoolsPanel] " fmt "\n", ##__VA_ARGS__); \
    fflush(stderr); \
} while(0)

WppConfigPage::WppConfigPage(QWidget *parent) : QWidget(parent) {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 10, 16, 12);
    root->setSpacing(10);

    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, &WppConfigPage::updateStatus);
    m_timer->start();

    // header
    auto *header = new QHBoxLayout;
    m_statusIcon = new QLabel(QString::fromUtf8("\u25cf"));
    m_statusIcon->setStyleSheet("color: #6a6; font-size: 18px; background: transparent;");
    m_statusIcon->setFixedWidth(20);
    m_statusText = new QLabel(QString::fromUtf8("\u8fde\u63a5\u4e2d..."));
    m_startBtn = new QPushButton(QString::fromUtf8("\u25b6 \u542f\u52a8"));
    m_stopBtn  = new QPushButton(QString::fromUtf8("\u25a0 \u505c\u6b62"));
    m_stopBtn->setEnabled(false);
    header->addWidget(m_statusIcon);
    header->addWidget(m_statusText);
    header->addStretch();
    header->addWidget(m_startBtn);
    header->addWidget(m_stopBtn);
    root->addLayout(header);

    // toolbar group
    auto *tg = new QGroupBox(QString::fromUtf8("\u5de5\u5177\u680f\u8bbe\u7f6e"));
    auto *tl = new QVBoxLayout(tg);
    tl->addWidget(makeSlider(QString::fromUtf8("\u900f\u660e\u5ea6"),      "toolbar_opacity",     50, 255, 140));
    tl->addWidget(makeSlider(QString::fromUtf8("\u56fe\u6807\u5927\u5c0f"),  "toolbar_icon_size",   16, 64, 28));
    tl->addWidget(makeSlider(QString::fromUtf8("\u6309\u94ae\u5927\u5c0f"),  "toolbar_button_size", 28, 80, 44));
    root->addWidget(tg);

    // control group
    auto *cg = new QGroupBox(QString::fromUtf8("\u4fa7\u8fb9\u63a7\u5236\u8bbe\u7f6e"));
    auto *cl = new QVBoxLayout(cg);
    cl->addWidget(makeSlider(QString::fromUtf8("\u900f\u660e\u5ea6"),      "control_opacity",      50, 255, 160));
    cl->addWidget(makeSlider(QString::fromUtf8("\u5b57\u53f7"),            "control_font_size",    16, 64, 22));
    cl->addWidget(makeSlider(QString::fromUtf8("\u6309\u94ae\u5927\u5c0f"), "control_btn_size",     28, 100, 44));
    cl->addWidget(makeSlider(QString::fromUtf8("\u8fb9\u7f18\u8ddd\u79bb"), "control_edge_offset",   0, 50, 4));
    root->addWidget(cg);

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

    connect(m_startBtn, &QPushButton::clicked, this, &WppConfigPage::onStart);
    connect(m_stopBtn,  &QPushButton::clicked, this, &WppConfigPage::onStop);
    connect(m_saveBtn,  &QPushButton::clicked, this, &WppConfigPage::onSave);

    loadCfg();
    updateStatus();
}

void WppConfigPage::loadCfg() {
    QString path = QCoreApplication::applicationDirPath() + "/touchtools.json";
    QFile f(path);
    if (f.open(QIODevice::ReadOnly)) {
        QJsonParseError err;
        m_cfg = QJsonDocument::fromJson(f.readAll(), &err).object();
        if (err.error != QJsonParseError::NoError)
            LOG("loadCfg: JSON parse error: %s", qPrintable(err.errorString()));
        f.close();
    }
    for (auto &e : m_entries) {
        int v = m_cfg.contains(e.key) ? m_cfg[e.key].toInt() : e.sl->value();
        e.sl->setValue(v);
        e.sp->setValue(v);
    }
}

void WppConfigPage::saveCfg() {
    for (auto &e : m_entries)
        m_cfg[e.key] = e.sl->value();
    QString path = QCoreApplication::applicationDirPath() + "/touchtools.json";
    QFile f(path);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(QJsonDocument(m_cfg).toJson());
        f.close();
    }
}

static bool isPipeRunning() {
    QLocalSocket sock;
    sock.connectToServer("WPStoolsPipe");
    if (sock.waitForConnected(200)) {
        sock.disconnectFromServer();
        return true;
    }
    return false;
}

void WppConfigPage::updateStatus() {
    bool running = isPipeRunning();

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

void WppConfigPage::onStart() {
    if (isPipeRunning()) return;
    QString exe = QCoreApplication::applicationDirPath() + "/WPPTouchHelper.exe";
    QProcess::startDetached(exe);
    updateStatus();
}

void WppConfigPage::onStop() {
    QLocalSocket sock;
    sock.connectToServer("WPStoolsPipe");
    if (sock.waitForConnected(500)) {
        sock.write("exit");
        sock.waitForBytesWritten(500);
        sock.disconnectFromServer();
        sock.waitForDisconnected(1000);
    }
    QTimer::singleShot(500, this, &WppConfigPage::updateStatus);
}

void WppConfigPage::onSave() {
    saveCfg();

    QLocalSocket sock;
    sock.connectToServer("WPStoolsPipe");
    if (sock.waitForConnected(500)) {
        sock.write("show");
        sock.waitForBytesWritten(500);
        sock.disconnectFromServer();
    }
}

QWidget* WppConfigPage::makeSlider(const QString &label, const QString &key,
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
