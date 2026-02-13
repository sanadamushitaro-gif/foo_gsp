#include "stdafx.h"
#include <foobar2000/SDK/foobar2000.h>

#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

// ====== JSONエスケープ（UTF-8安全版） ======
static void json_escape_append(pfc::string8& out, const char* s) {
    if (!s) return;

    const unsigned char* p = (const unsigned char*)s;

    while (*p) {
        unsigned char c = *p;

        switch (c) {
        case '\"': out.add_string("\\\""); break;
        case '\\': out.add_string("\\\\"); break;
        case '\b': out.add_string("\\b");  break;
        case '\f': out.add_string("\\f");  break;
        case '\n': out.add_string("\\n");  break;
        case '\r': out.add_string("\\r");  break;
        case '\t': out.add_string("\\t");  break;
        default:
            if (c < 0x20) {
                char buf[7];
                sprintf_s(buf, "\\u%04x", (unsigned)c);
                out.add_string(buf);
            }
            else {
                // ★ UTF-8バイトをそのまま1バイトコピー
                out.add_string((const char*)p, 1);
            }
        }

        ++p;
    }
}

// ====== HTTP POST ======
static void post_json_to_gas(const char* jsonText) {

    const wchar_t* kUrl =
        L"https://script.google.com/macros/s/AKfycbwJqmVnxlXBE2wFDWt7FR34Slb368S3d1m3F-ESAsHnWu04V0zfq1pDyc_aVjoITGF3/exec";

    URL_COMPONENTS uc = {};
    uc.dwStructSize = sizeof(uc);

    wchar_t host[256] = {};
    wchar_t path[2048] = {};
    uc.lpszHostName = host; uc.dwHostNameLength = _countof(host);
    uc.lpszUrlPath = path; uc.dwUrlPathLength = _countof(path);

    if (!WinHttpCrackUrl(kUrl, 0, 0, &uc)) {
        console::print("WinHttpCrackUrl failed");
        return;
    }

    HINTERNET hSession = WinHttpOpen(
        L"foo_playlist_uploader/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0
    );
    if (!hSession) { console::print("WinHttpOpen failed"); return; }

    HINTERNET hConnect = WinHttpConnect(hSession, uc.lpszHostName, uc.nPort, 0);
    if (!hConnect) { console::print("WinHttpConnect failed"); WinHttpCloseHandle(hSession); return; }

    DWORD flags = (uc.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect, L"POST", uc.lpszUrlPath,
        NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags
    );
    if (!hRequest) {
        console::print("WinHttpOpenRequest failed");
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return;
    }

    const wchar_t* headers = L"Content-Type: application/json; charset=utf-8\r\n";
    DWORD bodyBytes = (DWORD)strlen(jsonText);

    BOOL ok = WinHttpSendRequest(
        hRequest,
        headers, (DWORD)-1L,
        (LPVOID)jsonText, bodyBytes,
        bodyBytes, 0
    );

    if (!ok) {
        console::print("WinHttpSendRequest failed");
    }
    else if (!WinHttpReceiveResponse(hRequest, NULL)) {
        console::print("WinHttpReceiveResponse failed");
    }
    else {
        console::print("POST ok");

        DWORD size = 0;
        pfc::string8 resp;

        for (;;) {
            DWORD avail = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &avail)) break;
            if (avail == 0) break;

            pfc::array_t<char> buf;
            buf.set_size(avail + 1);
            memset(buf.get_ptr(), 0, avail + 1);

            DWORD read = 0;
            if (!WinHttpReadData(hRequest, buf.get_ptr(), avail, &read)) break;
            if (read == 0) break;

            resp.add_string(buf.get_ptr(), read);
        }

        if (resp.get_length() > 0) console::print(resp.c_str());
        else console::print("No response body");
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
}

// ====== メニュー ======
static const GUID guid_upload_playlist =
{ 0x12345678, 0x1234, 0x1234,{ 0x12,0x34,0x12,0x34,0x12,0x34,0x12,0x34 } };

class upload_menu : public mainmenu_commands {
public:
    t_uint32 get_command_count() override { return 1; }
    GUID get_command(t_uint32) override { return guid_upload_playlist; }

    void get_name(t_uint32, pfc::string_base& out) override {
        out = "Upload playlist";
    }

    bool get_description(t_uint32, pfc::string_base& out) override {
        out = "Send playlist to Google Sheets";
        return true;
    }

    GUID get_parent() override { return mainmenu_groups::file; }

    void execute(t_uint32, service_ptr_t<service_base>) override {

        static_api_ptr_t<playlist_manager> pm;
        const t_size active = pm->get_active_playlist();
        if (active == pfc_infinite) {
            console::print("No active playlist.");
            return;
        }

        metadb_handle_list items;
        pm->playlist_get_items(active, items, bit_array_true());

        service_ptr_t<titleformat_object> tf_title, tf_artist, tf_album, tf_playcount;
        static_api_ptr_t<titleformat_compiler> compiler;

        compiler->compile_safe(tf_title, "$if2(%title%,%filename%)");
        compiler->compile_safe(tf_artist, "$if2(%artist%,$if2(%album artist%,%performer%))");
        compiler->compile_safe(tf_album, "%album%");
        compiler->compile_safe(tf_playcount, "%play_count%");

        pfc::string8 json;
        json.add_string("{\"tracks\":[");

        for (t_size i = 0; i < items.get_count(); ++i) {

            pfc::string8 title, artist, album, playcount;

            items[i]->format_title(NULL, title, tf_title, NULL);
            items[i]->format_title(NULL, artist, tf_artist, NULL);
            items[i]->format_title(NULL, album, tf_album, NULL);
            items[i]->format_title(NULL, playcount, tf_playcount, NULL);

            console::print(pfc::string8("TITLE=") + title);
            console::print(pfc::string8("ARTIST=") + artist);

            if (i > 0) json.add_string(",");

            json.add_string("{\"title\":\"");
            json_escape_append(json, title.c_str());

            json.add_string("\",\"artist\":\"");
            json_escape_append(json, artist.c_str());

            json.add_string("\",\"album\":\"");
            json_escape_append(json, album.c_str());

            json.add_string("\",\"play_count\":\"");
            json_escape_append(json, playcount.c_str());

            json.add_string("\"}");
        }

        json.add_string("]}");

        // ★ 送信前JSONを表示
        console::print("JSON to send:");
        console::print(json.c_str());

        post_json_to_gas(json.c_str());
    }
};

static mainmenu_commands_factory_t<upload_menu> g_upload_menu;
