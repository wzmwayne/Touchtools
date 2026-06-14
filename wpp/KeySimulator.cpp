#include "KeySimulator.h"
#include <tlhelp32.h>
#include <cstdio>
#include <QTimer>

#define LOG(fmt, ...) do { \
    fprintf(stderr, "[WPPTouchHelper] " fmt "\n", ##__VA_ARGS__); \
    fflush(stderr); \
} while(0)

// ── Process / Window detection ───────────────────────────────────────────

DWORD KeySimulator::findProcessPID(const wchar_t* processName) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32W pe = { sizeof(pe) };
    DWORD pid = 0;
    if (Process32FirstW(snapshot, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, processName) == 0) {
                pid = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(snapshot, &pe));
    }
    CloseHandle(snapshot);
    return pid;
}

struct EnumData { DWORD targetPid; HWND result; };

BOOL CALLBACK KeySimulator::enumWindowsProc(HWND hwnd, LPARAM lParam) {
    EnumData& data = *reinterpret_cast<EnumData*>(lParam);
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == data.targetPid && hasSlideShowInTitle(hwnd)) {
        data.result = hwnd;
        return FALSE;
    }
    return TRUE;
}

bool KeySimulator::hasSlideShowInTitle(HWND hwnd) {
    wchar_t title[256];
    int len = GetWindowTextW(hwnd, title, 256);
    if (len == 0) return false;
    return wcsstr(title, L"Slide Show") != nullptr
        || wcsstr(title, L"\u5e7b\u706f\u7247\u653e\u6620") != nullptr;
}

HWND KeySimulator::findSlideShowWindow() {
    DWORD pid = findProcessPID(L"wpp.exe");
    if (!pid) return nullptr;
    EnumData data = { pid, nullptr };
    EnumWindows(enumWindowsProc, reinterpret_cast<LPARAM>(&data));
    return data.result;
}

bool KeySimulator::isSlideShowActive() {
    return findSlideShowWindow() != nullptr;
}

// ── Bring window to front and SendInput ──────────────────────────────────

void KeySimulator::sendKey(WORD vk, bool ctrl) {
    const char *name = "?";
         if (vk == VK_ESCAPE) name = "Esc";
    else if (vk == VK_RIGHT)  name = "Right";
    else if (vk == VK_LEFT)   name = "Left";
    else if (vk == 'A')       name = "A";
    else if (vk == 'P')       name = "P";
    else if (vk == 'E')       name = "E";
    LOG("sendKey: %s%s", ctrl ? "Ctrl+" : "", name);

    HWND hwnd = findSlideShowWindow();
    if (!hwnd) {
        LOG("slideshow window not found");
        return;
    }

    // Attach input queues so SetForegroundWindow works from our thread
    DWORD targetTid = GetWindowThreadProcessId(hwnd, NULL);
    DWORD ourTid = GetCurrentThreadId();
    AttachThreadInput(ourTid, targetTid, TRUE);
    SetForegroundWindow(hwnd);
    BringWindowToTop(hwnd);
    AttachThreadInput(ourTid, targetTid, FALSE);

    // Build SendInput sequence
    INPUT in[4] = {};
    int n = 0;
    if (ctrl) {
        in[n].type = INPUT_KEYBOARD;
        in[n].ki.wVk = VK_CONTROL;
        n++;
    }
    in[n].type = INPUT_KEYBOARD;
    in[n].ki.wVk = vk;
    n++;
    in[n].type = INPUT_KEYBOARD;
    in[n].ki.wVk = vk;
    in[n].ki.dwFlags = KEYEVENTF_KEYUP;
    n++;
    if (ctrl) {
        in[n].type = INPUT_KEYBOARD;
        in[n].ki.wVk = VK_CONTROL;
        in[n].ki.dwFlags = KEYEVENTF_KEYUP;
        n++;
    }
    SendInput(n, in, sizeof(INPUT));
    LOG("SendInput done");
}

// ── Public API ───────────────────────────────────────────────────────────

void KeySimulator::nextSlide()        { sendKey(VK_RIGHT, false); }
void KeySimulator::prevSlide()        { sendKey(VK_LEFT, false); }
void KeySimulator::mouseMode()        { sendKey('A', true); }
void KeySimulator::penMode()          { sendKey('P', true); }
void KeySimulator::eraserMode()       { sendKey('E', true); }

static void sendAltKey(WORD vk) {
    INPUT in[4] = {};
    in[0].type = INPUT_KEYBOARD; in[0].ki.wVk = VK_MENU;
    in[1].type = INPUT_KEYBOARD; in[1].ki.wVk = vk;
    in[2].type = INPUT_KEYBOARD; in[2].ki.wVk = vk;    in[2].ki.dwFlags = KEYEVENTF_KEYUP;
    in[3].type = INPUT_KEYBOARD; in[3].ki.wVk = VK_MENU; in[3].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(4, in, sizeof(INPUT));
}

void KeySimulator::exitSlideShow() {
    exitStage(0);
}

void KeySimulator::exitStage(int stage) {
    switch (stage) {
    case 0:
        LOG("exitSlideShow: attempt 1 (Esc)");
        sendKey(VK_ESCAPE, false);
        QTimer::singleShot(500, []{ exitStage(1); });
        return;
    case 1:
        if (!isSlideShowActive()) { LOG("exit: closed after 1st Esc"); return; }
        LOG("exitSlideShow: attempt 2 (Alt+D)");
        sendAltKey('D');
        QTimer::singleShot(500, []{ exitStage(2); });
        return;
    case 2:
        LOG("exitSlideShow: attempt 2 (Esc)");
        sendKey(VK_ESCAPE, false);
        QTimer::singleShot(1000, []{ exitStage(3); });
        return;
    case 3:
        if (!isSlideShowActive()) { LOG("exit: closed after 2nd"); return; }
        LOG("exitSlideShow: attempt 3 (Alt+D)");
        sendAltKey('D');
        QTimer::singleShot(500, []{ exitStage(4); });
        return;
    case 4:
        LOG("exitSlideShow: attempt 3 (Esc)");
        sendKey(VK_ESCAPE, false);
        QTimer::singleShot(2000, []{ exitStage(5); });
        return;
    case 5:
        if (!isSlideShowActive()) { LOG("exit: closed after 3rd"); return; }
        LOG("exit: force kill wpp.exe");
        {
            DWORD pid = findProcessPID(L"wpp.exe");
            if (!pid) { LOG("wpp.exe not found, cannot kill"); return; }
            HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
            if (!hProcess) {
                LOG("OpenProcess(PROCESS_TERMINATE) failed: %lu", GetLastError());
                return;
            }
            TerminateProcess(hProcess, 1);
            CloseHandle(hProcess);
            LOG("wpp.exe (pid=%lu) terminated", pid);
        }
        return;
    }
}
