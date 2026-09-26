#pragma once
#include <QString>
#include <QJsonObject>
#include <QJsonArray>

// Send authenticated command to TTcore via TTCorePipe.
// Reads api_key from config and prepends it to the command.
// Returns true on success (server responded "ok" or JSON).
bool ttIpcSend(const QString &cmd, int timeoutMs = 1000);

// Fetch status JSON from TTcore.
QJsonObject ttIpcStatus(int timeoutMs = 1000);

// Fetch log array from TTcore.
QJsonArray ttIpcLog(int timeoutMs = 1000);
