#include "UIAccess.h"
#include <tlhelp32.h>

static DWORD DuplicateWinloginToken(DWORD dwSessionId, DWORD dwDesiredAccess, PHANDLE phToken) {
    PRIVILEGE_SET ps;
    ps.PrivilegeCount = 1;
    ps.Control = PRIVILEGE_SET_ALL_NECESSARY;

    if (!LookupPrivilegeValueW(NULL, SE_TCB_NAME, &ps.Privilege[0].Luid))
        return GetLastError();

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
        return GetLastError();

    DWORD dwErr = ERROR_NOT_FOUND;
    PROCESSENTRY32W pe = { sizeof(pe) };
    BOOL bFound = FALSE;

    for (BOOL bCont = Process32FirstW(hSnapshot, &pe); bCont; bCont = Process32NextW(hSnapshot, &pe)) {
        if (_wcsicmp(pe.szExeFile, L"winlogon.exe") != 0)
            continue;

        HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe.th32ProcessID);
        if (!hProcess)
            continue;

        HANDLE hToken;
        if (OpenProcessToken(hProcess, TOKEN_QUERY | TOKEN_DUPLICATE, &hToken)) {
            BOOL fTcb;
            if (PrivilegeCheck(hToken, &ps, &fTcb) && fTcb) {
                DWORD sid;
                DWORD dwRetLen;
                if (GetTokenInformation(hToken, TokenSessionId, &sid, sizeof(sid), &dwRetLen) && sid == dwSessionId) {
                    bFound = TRUE;
                    if (DuplicateTokenEx(hToken, dwDesiredAccess, NULL, SecurityImpersonation, TokenImpersonation, phToken))
                        dwErr = ERROR_SUCCESS;
                    else
                        dwErr = GetLastError();
                }
            }
            CloseHandle(hToken);
        }
        CloseHandle(hProcess);

        if (bFound) break;
    }

    CloseHandle(hSnapshot);
    return dwErr;
}

static DWORD CreateUIAccessToken(PHANDLE phToken) {
    HANDLE hTokenSelf;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_DUPLICATE, &hTokenSelf))
        return GetLastError();

    DWORD dwSessionId, dwRetLen;
    if (!GetTokenInformation(hTokenSelf, TokenSessionId, &dwSessionId, sizeof(dwSessionId), &dwRetLen)) {
        DWORD dwErr = GetLastError();
        CloseHandle(hTokenSelf);
        return dwErr;
    }

    HANDLE hTokenSystem;
    DWORD dwErr = DuplicateWinloginToken(dwSessionId, TOKEN_IMPERSONATE, &hTokenSystem);
    if (dwErr != ERROR_SUCCESS) {
        CloseHandle(hTokenSelf);
        return dwErr;
    }

    if (!SetThreadToken(NULL, hTokenSystem)) {
        dwErr = GetLastError();
        CloseHandle(hTokenSystem);
        CloseHandle(hTokenSelf);
        return dwErr;
    }

    HANDLE hNewToken;
    if (DuplicateTokenEx(hTokenSelf, TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY | TOKEN_ADJUST_DEFAULT,
                         NULL, SecurityAnonymous, TokenPrimary, &hNewToken))
    {
        BOOL bUIAccess = TRUE;
        if (!SetTokenInformation(hNewToken, TokenUIAccess, &bUIAccess, sizeof(bUIAccess))) {
            dwErr = GetLastError();
            CloseHandle(hNewToken);
        } else {
            *phToken = hNewToken;
            dwErr = ERROR_SUCCESS;
        }
    } else {
        dwErr = GetLastError();
    }

    RevertToSelf();
    CloseHandle(hTokenSystem);
    CloseHandle(hTokenSelf);
    return dwErr;
}

static BOOL CheckForUIAccess(DWORD *pdwErr, DWORD *pfUIAccess) {
    HANDLE hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        *pdwErr = GetLastError();
        return FALSE;
    }

    DWORD dwRetLen;
    BOOL result = GetTokenInformation(hToken, TokenUIAccess, pfUIAccess, sizeof(*pfUIAccess), &dwRetLen);
    if (!result)
        *pdwErr = GetLastError();

    CloseHandle(hToken);
    return result;
}

DWORD PrepareForUIAccess() {
    DWORD dwErr;
    DWORD fUIAccess;

    if (!CheckForUIAccess(&dwErr, &fUIAccess))
        return dwErr;

    if (fUIAccess == TRUE)
        return ERROR_SUCCESS;

    HANDLE hTokenUIAccess;
    dwErr = CreateUIAccessToken(&hTokenUIAccess);
    if (dwErr != ERROR_SUCCESS)
        return dwErr;

    STARTUPINFOW si;
    GetStartupInfoW(&si);
    PROCESS_INFORMATION pi;

    if (CreateProcessAsUserW(hTokenUIAccess, NULL, GetCommandLineW(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(hTokenUIAccess);
        ExitProcess(0);
    }

    dwErr = GetLastError();
    CloseHandle(hTokenUIAccess);
    return dwErr;
}
