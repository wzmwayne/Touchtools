#include "TtIpc.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QLocalSocket>

static QString readApiKey() {
    QString path = QCoreApplication::applicationDirPath() + "/touchtools.json";
    QFile f(path);
    if (f.open(QIODevice::ReadOnly)) {
        QJsonParseError err;
        auto doc = QJsonDocument::fromJson(f.readAll(), &err);
        if (err.error == QJsonParseError::NoError && doc.isObject())
            return doc.object().value("api_key").toString();
    }
    return {};
}

bool ttIpcSend(const QString &cmd, int timeoutMs) {
    QString key = readApiKey();
    if (key.isEmpty()) return false;

    QLocalSocket sock;
    sock.connectToServer("TTCorePipe");
    if (!sock.waitForConnected(timeoutMs / 2)) return false;

    QByteArray payload = (key + ":" + cmd).toUtf8();
    sock.write(payload);
    if (!sock.waitForBytesWritten(timeoutMs / 2)) { sock.disconnectFromServer(); return false; }

    bool ok = false;
    if (sock.waitForReadyRead(timeoutMs / 2)) {
        QByteArray resp = sock.readAll().trimmed();
        ok = (resp != "err:missing_key" && resp != "err:invalid_key");
    }
    sock.disconnectFromServer();
    return ok;
}

QJsonObject ttIpcStatus(int timeoutMs) {
    QJsonObject empty;
    QString key = readApiKey();
    if (key.isEmpty()) return empty;

    QLocalSocket sock;
    sock.connectToServer("TTCorePipe");
    if (!sock.waitForConnected(300)) return empty;

    sock.write((key + ":status").toUtf8());
    sock.waitForBytesWritten(300);

    QJsonObject obj;
    if (sock.waitForReadyRead(500)) {
        QJsonParseError err;
        auto doc = QJsonDocument::fromJson(sock.readAll(), &err);
        if (err.error == QJsonParseError::NoError && doc.isObject())
            obj = doc.object();
    }
    sock.disconnectFromServer();
    return obj;
}

QJsonArray ttIpcLog(int timeoutMs) {
    QJsonArray empty;
    QString key = readApiKey();
    if (key.isEmpty()) return empty;

    QLocalSocket sock;
    sock.connectToServer("TTCorePipe");
    if (!sock.waitForConnected(300)) return empty;

    sock.write((key + ":log").toUtf8());
    sock.waitForBytesWritten(300);

    QJsonArray arr;
    if (sock.waitForReadyRead(500)) {
        QJsonParseError err;
        auto doc = QJsonDocument::fromJson(sock.readAll(), &err);
        if (err.error == QJsonParseError::NoError && doc.isArray())
            arr = doc.array();
    }
    sock.disconnectFromServer();
    return arr;
}
