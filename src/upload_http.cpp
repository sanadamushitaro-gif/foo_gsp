#include "stdafx.h"
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

static void post_json_to_gas(const char* jsonUtf8) {
    // ÅöÇ±Ç±ÇèëÇ´ä∑Ç¶
    const wchar_t* kUrl = L"https://script.google.com/macros/s/AKfycbzbDEzBWjLrQRLeT1G6kZNuZHZkAaaklqsQoBPKf6piWKhQxRXAYOSfqupeWJGGi08-/exec";

    URL_COMPONENTS uc = { 0 };
    uc.dwStructSize = sizeof(uc);
    wchar_t host[256] = {};
    wchar_t path[2048] = {};
    uc.lpszHostName = host; uc.dwHostNameLength = _countof(host);
    uc.lpszUrlPath = path; uc.dwUrlPathLength = _countof(path);

    if (!WinHttpCrackUrl(kUrl, 0, 0, &uc)) {
        console::print("WinHttpCrackUrl failed");
        return;
    }

    HINTERNET hSession = WinHttpOpen(L"foo_test/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) { console::print("WinHttpOpen failed"); return; }

    HINTERNET hConnect = WinHttpConnect(hSession, uc.lpszHostName, uc.nPort, 0);
    if (!hConnect) { console::print("WinHttpConnect failed"); WinHttpCloseHandle(hSession); return; }

    DWORD flags = (uc.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", uc.lpszUrlPath,
        NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest) { console::print("WinHttpOpenRequest failed"); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return; }

    // UTF-8 -> UTF-16 ïœä∑Åibody ÇÕ UTF-8 ÇÃÇ‹Ç‹ëóÇÈÅj
    const char* body = jsonUtf8;
    DWORD bodyBytes = (DWORD)strlen(body);

    const wchar_t* headers = L"Content-Type: application/json; charset=utf-8\r\n";

    BOOL ok = WinHttpSendRequest(hRequest, headers, (DWORD)-1L,
        (LPVOID)body, bodyBytes, bodyBytes, 0);

    if (!ok) {
        console::print("WinHttpSendRequest failed");
    }
    else if (!WinHttpReceiveResponse(hRequest, NULL)) {
        console::print("WinHttpReceiveResponse failed");
    }
    else {
        console::print("POST ok");
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
}
