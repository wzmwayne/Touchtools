#define _WIN32_IE 0x0A00
#include <windows.h>
#include <shellapi.h>

#include <QApplication>
#include <QIcon>
#include <QImage>
#include <QPixmap>
#include <QTimer>
#include <QSharedMemory>
#include <QLocalServer>
#include <QLocalSocket>
#include <QDir>
#include "Config.h"
#include "KeySimulator.h"
#include "FloatingControl.h"
#include "FloatingToolbar.h"

#ifndef SIID_PPTFILE
#define SIID_PPTFILE ((SHSTOCKICONID)131)
#endif

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    SetCurrentDirectoryW(
        reinterpret_cast<const wchar_t*>(
            QDir::toNativeSeparators(
                QCoreApplication::applicationDirPath()).utf16()));

    QSharedMemory lock("WPStoolsSingleInstance");
    if (!lock.create(1)) {
        QLocalSocket sock;
        sock.connectToServer("WPStoolsPipe");
        if (sock.waitForConnected(200)) {
            sock.write("show");
            sock.waitForBytesWritten(200);
        }
        return 0;
    }

    app.setQuitOnLastWindowClosed(false);
    {
        SHSTOCKICONINFO sii = { sizeof(sii) };
        if (SUCCEEDED(SHGetStockIconInfo(SIID_PPTFILE, SHGSI_ICON | SHGSI_LARGEICON, &sii))) {
            app.setWindowIcon(QIcon(QPixmap::fromImage(QImage::fromHICON(sii.hIcon))));
            DestroyIcon(sii.hIcon);
        }
    }

    FloatingControl left(FloatingControl::Left);
    FloatingControl right(FloatingControl::Right);
    FloatingToolbar toolbar;

    auto forceShow = [&]() {
        Config::instance().reload();
        left.applyConfig();
        right.applyConfig();
        toolbar.applyConfig();
        left.show(); left.raise();
        right.show(); right.raise();
        toolbar.show(); toolbar.raise();
        left.setKeepAlive(true);
        right.setKeepAlive(true);
        toolbar.setKeepAlive(true);
        QTimer::singleShot(8000, [&]() {
            left.setKeepAlive(false);
            right.setKeepAlive(false);
            toolbar.setKeepAlive(false);
        });
    };

    QLocalServer server;
    server.listen("WPStoolsPipe");
    QObject::connect(&server, &QLocalServer::newConnection, [&]() {
        QLocalSocket *client = server.nextPendingConnection();
        if (client->waitForReadyRead(500)) {
            QByteArray cmd = client->readAll().trimmed();
            if (cmd == "show")
                forceShow();
            else if (cmd == "exit")
                QApplication::quit();
            else if (cmd == "status") {
                QByteArray resp = QString("active=%1")
                    .arg(KeySimulator::isSlideShowActive() ? "yes" : "no")
                    .toUtf8();
                client->write(resp);
                client->waitForBytesWritten(500);
            }
        }
        client->deleteLater();
    });

    return app.exec();
}
