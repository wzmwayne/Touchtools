#pragma once
#include <windows.h>

class KeySimulator {
public:
    static bool isSlideShowActive();
    static void nextSlide();
    static void prevSlide();
    static void mouseMode();
    static void penMode();
    static void eraserMode();
    static void exitSlideShow();

private:
    static BOOL CALLBACK enumWindowsProc(HWND hwnd, LPARAM lParam);
    static DWORD findProcessPID(const wchar_t* processName);
    static bool hasSlideShowInTitle(HWND hwnd);
    static HWND findSlideShowWindow();
    static void sendKey(WORD vk, bool ctrl = false);
    static void exitStage(int stage);
};
