#include <QApplication>
#include <QSharedMemory>
#include <QDir>
#include <windows.h>
#include "Config.h"
#include "USBAssistant.h"
#include "UIAccess.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    SetCurrentDirectoryW(
        reinterpret_cast<const wchar_t*>(
            QDir::toNativeSeparators(
                QCoreApplication::applicationDirPath()).utf16()));

    PrepareForUIAccess();

    // Give the OS time to clean up shared memory from the previous instance
    // (UIAccess preparation may have spawned a new process and exited the old one)
    Sleep(200);

    QSharedMemory lock("TouchtoolsUSBInstance");
    if (!lock.create(1))
        return 0;

    app.setQuitOnLastWindowClosed(false);

    Config::instance();
    USBAssistant assistant;

    return app.exec();
}
