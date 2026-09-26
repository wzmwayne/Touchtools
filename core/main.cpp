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
#include <QUuid>
#include <cstdio>
#include <cstdarg>
#include <windows.h>
#include <tlhelp32.h>

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
    QString ts = QTime::currentTime().toString("hh:mm:ss"); \
    QString msg = QString("[%1] %2").arg(ts, tfmt(fmt, ##__VA_ARGS__)); \
    s_log.append(msg); \
    if (s_log.size() > MAX_LOG) s_log.removeFirst(); \
    fprintf(stderr, "[TTcore %s] " fmt "\n", ts.toLocal8Bit().constData(), ##__VA_ARGS__); \
    fflush(stderr); \
} while(0)

static const wchar_t *PIPE_NAME = L"TTCorePipe";

// ── Trusted client check ──
static QString clientExeName(HANDLE pipe) {
    ULONG pid = 0;
    if (!GetNamedPipeClientProcessId(pipe, &pid)) return {};
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return {};
    PROCESSENTRY32W pe = { sizeof(pe) };
    QString name;
    if (Process32FirstW(snap, &pe)) do {
        if (pe.th32ProcessID == pid) {
            name = QString::fromWCharArray(pe.szExeFile);
            break;
        }
    } while (Process32NextW(snap, &pe));
    CloseHandle(snap);
    return name;
}
static bool isTrustedClient(QLocalSocket *client) {
    QString name = clientExeName(
        reinterpret_cast<HANDLE>(client->socketDescriptor()));
    return name.contains("TouchtoolsPanel", Qt::CaseInsensitive)
        || name.contains("WPPTouchHelper", Qt::CaseInsensitive)
        || name.contains("USBAssistant", Qt::CaseInsensitive)
        || name.contains("TTcore", Qt::CaseInsensitive);
}

// ── Confirmation popup for external IPC ──
static LRESULT CALLBACK ConfirmWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_CREATE) {
        SetTimer(hwnd, 1, 1000, nullptr);
        return 0;
    }
    if (msg == WM_TIMER && wp == 1) {
        int *sec = reinterpret_cast<int*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (sec && *sec > 0) {
            --*sec;
            HWND txt = GetDlgItem(hwnd, 100);
            if (txt) {
                wchar_t buf[128];
                wsprintfW(buf, L"\u5916\u90e8\u7a0b\u5e8f\u8bf7\u6c42\u6267\u884c\u547d\u4ee4\uff0c\u5c06\u5728 %d \u79d2\u540e\u81ea\u52a8\u6267\u884c\u2026", *sec);
                SetWindowTextW(txt, buf);
            }
        } else {
            KillTimer(hwnd, 1);
            // confirmed → post quit with IDOK
            DestroyWindow(hwnd);
        }
        return 0;
    }
    if (msg == WM_COMMAND && LOWORD(wp) == 101) {
        // Cancel button
        KillTimer(hwnd, 1);
        DestroyWindow(hwnd);
        return 0;
    }
    if (msg == WM_DESTROY) {
        // store result: GWLP_USERDATA bit 31 = cancelled
        LONG_PTR data = GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        data |= (1L << 31);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, data);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

static bool showConfirmPopup(const QString &command, const QString &clientName) {
    static bool registered = false;
    static const wchar_t CLASS_NAME[] = L"TTcoreConfirmPopup";
    if (!registered) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc   = ConfirmWndProc;
        wc.hInstance     = GetModuleHandleW(nullptr);
        wc.lpszClassName = CLASS_NAME;
        RegisterClassW(&wc);
        registered = true;
    }

    RECT work;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
    int w = 420, h = 120;
    int x = work.right - w - 8;
    int y = work.top + 8;

    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        CLASS_NAME, L"TTcore - \u5916\u90e8\u8bf7\u6c42\u786e\u8ba4",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        x, y, w, h,
        nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (!hwnd) return true; // fallback: allow

    // static text
    wchar_t msg[512];
    wsprintfW(msg, L"\u5916\u90e8\u7a0b\u5e8f [%s] \u8bf7\u6c42\u6267\u884c\u547d\u4ee4\uff1a%s\n\u5c06\u5728 5 \u79d2\u540e\u81ea\u52a8\u6267\u884c\u2026",
              reinterpret_cast<const wchar_t*>(clientName.utf16()),
              reinterpret_cast<const wchar_t*>(command.utf16()));
    HWND txt = CreateWindowExW(0, L"STATIC", msg,
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        12, 10, w - 24, 44, hwnd, (HMENU)100, nullptr, nullptr);

    // Cancel button
    HWND btn = CreateWindowExW(0, L"BUTTON", L"\u53d6\u6d88",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        w - 80 - 12, h - 34, 80, 26, hwnd, (HMENU)101, nullptr, nullptr);

    SetWindowLongPtrW(hwnd, GWLP_USERDATA, 5); // start countdown at 5

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    // Run nested message loop for this popup
    MSG msgLoop;
    bool cancelled = false;
    while (GetMessageW(&msgLoop, nullptr, 0, 0)) {
        if (msgLoop.message == WM_QUIT) break;
        if (!IsDialogMessageW(hwnd, &msgLoop))
            TranslateMessage(&msgLoop), DispatchMessageW(&msgLoop);

        LONG_PTR data = GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        if (data & (1L << 31)) { // destroyed
            cancelled = true;
            break;
        }
        if ((data & 0x7FFFFFFF) == 0) { // timer expired
            cancelled = false;
            break;
        }
    }
    DestroyWindow(hwnd);
    return !cancelled;
}

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

    SetCurrentDirectoryW(
        reinterpret_cast<const wchar_t*>(
            QDir::toNativeSeparators(
                QCoreApplication::applicationDirPath()).utf16()));

    QSharedMemory lock("TTCoreSingleInstance");
    if (!lock.create(1)) {
        TT_LOG("\u5df2\u6709\u5b9e\u4f8b\u5728\u8fd0\u884c");
        return 0;
    }
    TT_LOG("\u542f\u52a8\u5b8c\u6210");

    // API key (generate on first run, persist in config)
    QString apiKey;
    {
        QFile f(configPath());
        if (f.open(QIODevice::ReadWrite)) {
            QJsonObject cfg = QJsonDocument::fromJson(f.readAll()).object();
            if (!cfg.contains("api_key") || cfg["api_key"].toString().isEmpty()) {
                cfg["api_key"] = QUuid::createUuid().toString(QUuid::Id128);
                f.resize(0);
                f.write(QJsonDocument(cfg).toJson());
            }
            apiKey = cfg["api_key"].toString();
        }
    }
    TT_LOG("API \u5bc6\u94a5\u5df2\u51c6\u5907");

    // IPC server
    QLocalServer::removeServer(QString::fromWCharArray(PIPE_NAME));
    QLocalServer server;
    if (!server.listen(QString::fromWCharArray(PIPE_NAME))) {
        TT_LOG("IPC \u76d1\u542c\u5931\u8d25");
        return 1;
    }
    TT_LOG("IPC \u76d1\u542c\u4e2d %s", "TTCorePipe");

    // Child process management
    QProcess *wppProc = nullptr;
    QProcess *usbProc = nullptr;

    auto cleanup = [&]() {
        if (wppProc && wppProc->state() != QProcess::NotRunning) {
            TT_LOG("\u505c\u6b62 WPPTouchHelper");
            wppProc->kill();
            if (!wppProc->waitForFinished(2000))
                TT_LOG("WPPTouchHelper \u672a\u80fd\u6b63\u5e38\u505c\u6b62");
        }
        delete wppProc; wppProc = nullptr;

        if (usbProc && usbProc->state() != QProcess::NotRunning) {
            TT_LOG("\u505c\u6b62 USBAssistant");
            usbProc->kill();
            if (!usbProc->waitForFinished(2000))
                TT_LOG("USBAssistant \u672a\u80fd\u6b63\u5e38\u505c\u6b62");
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

        TT_LOG("\u914d\u7f6e\uff1aWPPTouchHelper=%d USBAssistant=%d", wppOn, usbOn);

        if (wppOn) {
            TT_LOG("\u542f\u52a8 WPPTouchHelper");
            wppProc = new QProcess;
            wppProc->setProcessChannelMode(QProcess::ForwardedChannels);
            wppProc->start(wppPath());
        }
        if (usbOn) {
            TT_LOG("\u542f\u52a8 USBAssistant");
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
        TT_LOG("\u914d\u7f6e\u6587\u4ef6\u53d8\u66f4\uff0c\u91cd\u65b0\u52a0\u8f7d\u7ec4\u4ef6");
        launchComponents();
        if (QFile::exists(configPath()))
            watcher.addPath(configPath());
    });

    // IPC command handler
    QObject::connect(&server, &QLocalServer::newConnection, [&]() {
        QLocalSocket *client = server.nextPendingConnection();
        if (client->waitForReadyRead(500)) {
            QByteArray raw = client->readAll().trimmed();
            TT_LOG("IPC \u547d\u4ee4: %s", raw.constData());

            // --- API key authentication ---
            int colon = raw.indexOf(':');
            if (colon < 0) {
                client->write("err:missing_key");
                client->waitForBytesWritten(500);
                client->deleteLater();
                return;
            }
            QString key = QString::fromUtf8(raw.left(colon));
            if (key != apiKey) {
                client->write("err:invalid_key");
                client->waitForBytesWritten(500);
                client->deleteLater();
                return;
            }
            QString cmd = QString::fromUtf8(raw.mid(colon + 1));

            // External program confirmation for control commands
            static const QStringList sensitive = {
                "start ", "stop ", "reload", "exit"
            };
            if (!isTrustedClient(client)) {
                bool sensitiveCmd = false;
                for (const auto &s : sensitive)
                    if (cmd.startsWith(s) || cmd == s) { sensitiveCmd = true; break; }
                if (sensitiveCmd) {
                    QString exeName = clientExeName(
                        reinterpret_cast<HANDLE>(client->socketDescriptor()));
                    if (!showConfirmPopup(cmd, exeName)) {
                        client->write("cancelled");
                        client->waitForBytesWritten(500);
                        client->deleteLater();
                        return;
                    }
                }
            }

            if (cmd == "status") {
                QJsonObject s;
                QJsonObject cfg;
                QFile f(configPath());
                if (f.open(QIODevice::ReadOnly))
                    cfg = QJsonDocument::fromJson(f.readAll()).object();
                s["wpptouchhelper_enabled"] = cfg.value("wpptouchhelper_enabled").toBool(true);
                s["usb_enabled"]            = cfg.value("usb_enabled").toBool(true);
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

            } else if (cmd.startsWith("start ")) {
                QString component = cmd.mid(6);
                if (component == "wpptouchhelper") {
                    if (!wppProc || wppProc->state() != QProcess::Running) {
                        delete wppProc; wppProc = nullptr;
                        TT_LOG("\u53d1\u8d77\u542f\u52a8 WPPTouchHelper");
                        wppProc = new QProcess;
                        wppProc->setProcessChannelMode(QProcess::ForwardedChannels);
                        wppProc->start(wppPath());
                    }
                } else if (component == "usb_assistant") {
                    if (!usbProc || usbProc->state() != QProcess::Running) {
                        delete usbProc; usbProc = nullptr;
                        TT_LOG("\u53d1\u8d77\u542f\u52a8 USBAssistant");
                        usbProc = new QProcess;
                        usbProc->setProcessChannelMode(QProcess::ForwardedChannels);
                        usbProc->start(usbPath());
                    }
                }
                client->write("ok");
                client->waitForBytesWritten(500);

            } else if (cmd.startsWith("stop ")) {
                QString component = cmd.mid(5);
                if (component == "wpptouchhelper" && wppProc) {
                    TT_LOG("\u53d1\u8d77\u505c\u6b62 WPPTouchHelper");
                    wppProc->kill();
                } else if (component == "usb_assistant" && usbProc) {
                    TT_LOG("\u53d1\u8d77\u505c\u6b62 USBAssistant");
                    usbProc->kill();
                }
                client->write("ok");
                client->waitForBytesWritten(500);

            } else if (cmd == "exit") {
                TT_LOG("\u6b63\u5728\u5173\u95ed");
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
        TT_LOG("\u521d\u59cb\u5316\u7ec4\u4ef6\u542f\u52a8");
        launchComponents();
    });

    TT_LOG("\u5c31\u7eea");
    int ret = app.exec();
    cleanup();
    TT_LOG("\u5df2\u9000\u51fa");
    return ret;
}
