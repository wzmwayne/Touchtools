#include <QCoreApplication>
#include <QSharedMemory>
#include <QLocalServer>
#include <QLocalSocket>
#include <QProcess>
#include <QFileSystemWatcher>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QTimer>
#include <QTime>
#include <cstdio>
#include <cstdarg>
#include <windows.h>

static const int MAX_LOG = 200;
static QStringList s_log;

static QString tfmt(const char *fmt, ...) {
    char buf[2048];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    return QString::fromUtf8(buf);
}

#define TT_LOG(fmt, ...) do { \
    QString msg = QString("[%1] %2").arg( \
        QTime::currentTime().toString("hh:mm:ss"), \
        tfmt(fmt, ##__VA_ARGS__)); \
    s_log.append(msg); \
    if (s_log.size() > MAX_LOG) s_log.removeFirst(); \
    fprintf(stderr, "[TTcore] " fmt "\n", ##__VA_ARGS__); \
    fflush(stderr); \
} while(0)

static const wchar_t *PIPE_NAME = L"TTCorePipe";

static QString configPath() {
    return QCoreApplication::applicationDirPath() + "/touchtools.json";
}

static QString wppPath() {
    return QCoreApplication::applicationDirPath() + "/WPPTouchHelper.exe";
}

static QString usbPath() {
    return QCoreApplication::applicationDirPath() + "/USBAssistant.exe";
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    bool autoStart = false;
    for (int i = 1; i < argc; ++i)
        if (qstrcmp(argv[i], "-autostart") == 0) autoStart = true;

    SetCurrentDirectoryW(
        reinterpret_cast<const wchar_t*>(
            QDir::toNativeSeparators(
                QCoreApplication::applicationDirPath()).utf16()));

    QSharedMemory lock("TTCoreSingleInstance");
    if (!lock.create(1)) {
        TT_LOG("already running");
        return 0;
    }
    TT_LOG("started (autostart=%d)", autoStart);

    // IPC server
    QLocalServer::removeServer(QString::fromWCharArray(PIPE_NAME));
    QLocalServer server;
    if (!server.listen(QString::fromWCharArray(PIPE_NAME))) {
        TT_LOG("IPC listen failed");
        return 1;
    }
    TT_LOG("IPC listening on %s", "TTCorePipe");

    // Child process management
    QProcess *wppProc = nullptr;
    QProcess *usbProc = nullptr;

    auto cleanup = [&]() {
        if (wppProc && wppProc->state() != QProcess::NotRunning) {
            TT_LOG("stopping WPPTouchHelper");
            wppProc->kill();
            if (!wppProc->waitForFinished(2000))
                TT_LOG("WPPTouchHelper did not stop gracefully");
        }
        delete wppProc; wppProc = nullptr;

        if (usbProc && usbProc->state() != QProcess::NotRunning) {
            TT_LOG("stopping USBAssistant");
            usbProc->kill();
            if (!usbProc->waitForFinished(2000))
                TT_LOG("USBAssistant did not stop gracefully");
        }
        delete usbProc; usbProc = nullptr;
    };

    auto launchComponents = [&]() {
        cleanup();

        QFile f(configPath());
        QJsonObject cfg;
        if (f.open(QIODevice::ReadOnly))
            cfg = QJsonDocument::fromJson(f.readAll()).object();

        bool wppOn = cfg.value("wpptouchhelper_enabled").toBool(true);
        bool usbOn = cfg.value("usb_enabled").toBool(true);

        TT_LOG("config: wpptouchhelper=%d usb_assistant=%d", wppOn, usbOn);

        if (wppOn) {
            TT_LOG("launching WPPTouchHelper");
            wppProc = new QProcess;
            wppProc->setProcessChannelMode(QProcess::ForwardedChannels);
            wppProc->start(wppPath());
        }
        if (usbOn) {
            TT_LOG("launching USBAssistant");
            usbProc = new QProcess;
            usbProc->setProcessChannelMode(QProcess::ForwardedChannels);
            usbProc->start(usbPath());
        }
    };

    // Config file watcher
    QFileSystemWatcher watcher;
    if (QFile::exists(configPath()))
        watcher.addPath(configPath());

    QObject::connect(&watcher, &QFileSystemWatcher::fileChanged, [&]() {
        TT_LOG("config file changed, reloading components");
        launchComponents();
        if (QFile::exists(configPath()))
            watcher.addPath(configPath());
    });

    // IPC command handler
    QObject::connect(&server, &QLocalServer::newConnection, [&]() {
        QLocalSocket *client = server.nextPendingConnection();
        if (client->waitForReadyRead(500)) {
            QByteArray cmd = client->readAll().trimmed();
            TT_LOG("IPC command: %s", cmd.constData());

            if (cmd == "status") {
                QJsonObject s;
                s["wpptouchhelper"] = wppProc && wppProc->state() == QProcess::Running ? "running" : "stopped";
                s["usb_assistant"]  = usbProc && usbProc->state() == QProcess::Running ? "running" : "stopped";
                client->write(QJsonDocument(s).toJson(QJsonDocument::Compact));
                client->waitForBytesWritten(500);

            } else if (cmd == "log") {
                QJsonArray arr;
                for (const auto &l : s_log)
                    arr.append(l);
                client->write(QJsonDocument(arr).toJson(QJsonDocument::Compact));
                client->waitForBytesWritten(500);

            } else if (cmd == "reload") {
                launchComponents();
                client->write("ok");
                client->waitForBytesWritten(500);

            } else if (cmd == "exit") {
                TT_LOG("shutting down");
                client->write("ok");
                client->waitForBytesWritten(500);
                client->flush();
                QTimer::singleShot(200, [&]() {
                    cleanup();
                    app.quit();
                });
            }
        }
        client->deleteLater();
    });

    // Initial launch
    QTimer::singleShot(300, [&]() {
        TT_LOG("initial component launch");
        launchComponents();
    });

    TT_LOG("ready");
    int ret = app.exec();
    cleanup();
    TT_LOG("exited");
    return ret;
}
