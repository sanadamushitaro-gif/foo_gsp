#include "stdafx.h"
#include <foobar2000/SDK/foobar2000.h>

#include <windows.h>
#include <winhttp.h>
#include <cctype>
#pragma comment(lib, "winhttp.lib")

static const GUID guid_cfg_gsp_token =
{ 0x9d2f0d9d, 0xe7f2, 0x4bc1,{ 0xa4,0xa2,0x97,0x60,0x8e,0xc8,0x8a,0x01 } };

static cfg_string cfg_gsp_token(guid_cfg_gsp_token, "");

namespace {
    constexpr UINT kEditIdToken = 1001;
    constexpr wchar_t kPrefWindowClass[] = L"foo_gsp_pref_page";

    static void trim_ascii_whitespace(pfc::string8& s) {
        const char* p = s.c_str();
        t_size len = s.get_length();
        t_size begin = 0;
        t_size end = len;

        while (begin < end && std::isspace((unsigned char)p[begin])) ++begin;
        while (end > begin && std::isspace((unsigned char)p[end - 1])) --end;

        if (begin == 0 && end == len) return;

        pfc::string8 t;
        t.add_string(p + begin, end - begin);
        s = t;
    }
}

class gsp_preferences_page_instance : public preferences_page_instance {
public:
    gsp_preferences_page_instance(HWND parent, preferences_page_callback::ptr callback)
        : m_callback(callback) {
        register_class_once();

        m_wnd = CreateWindowExW(
            0, kPrefWindowClass, L"",
            WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
            0, 0, 100, 100,
            parent, nullptr, core_api::get_my_instance(), this);

        m_label = CreateWindowExW(
            0, L"Static", L"GSP deployment token",
            WS_CHILD | WS_VISIBLE,
            0, 0, 100, 20,
            m_wnd, nullptr, core_api::get_my_instance(), nullptr);

        m_edit = CreateWindowExW(
            WS_EX_CLIENTEDGE, L"Edit", L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
            0, 0, 100, 24,
            m_wnd, (HMENU)(UINT_PTR)kEditIdToken, core_api::get_my_instance(), nullptr);

        pfc::stringcvt::string_wide_from_utf8 tokenWide(cfg_gsp_token.get());
        ::SetWindowTextW(m_edit, tokenWide.get_ptr());
        layout_controls();
    }

    t_uint32 get_state() override {
        t_uint32 state = preferences_state::resettable;
        if (has_changed()) state |= preferences_state::changed;
        return state;
    }

    fb2k::hwnd_t get_wnd() override {
        return m_wnd;
    }

    void apply() override {
        cfg_gsp_token.set(get_edit_text().c_str());
        notify_state_changed();
    }

    void reset() override {
        ::SetWindowTextW(m_edit, L"");
        notify_state_changed();
    }

private:
    static void register_class_once() {
        static bool registered = false;
        if (registered) return;

        WNDCLASSW wc = {};
        wc.lpfnWndProc = &gsp_preferences_page_instance::s_wndproc;
        wc.hInstance = core_api::get_my_instance();
        wc.lpszClassName = kPrefWindowClass;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

        RegisterClassW(&wc);
        registered = true;
    }

    static LRESULT CALLBACK s_wndproc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp) {
        gsp_preferences_page_instance* self = reinterpret_cast<gsp_preferences_page_instance*>(
            GetWindowLongPtrW(wnd, GWLP_USERDATA));

        if (msg == WM_NCCREATE) {
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
            self = reinterpret_cast<gsp_preferences_page_instance*>(cs->lpCreateParams);
            SetWindowLongPtrW(wnd, GWLP_USERDATA, (LONG_PTR)self);
        }

        if (self) {
            return self->wndproc(wnd, msg, wp, lp);
        }
        return DefWindowProcW(wnd, msg, wp, lp);
    }

    LRESULT wndproc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp) {
        switch (msg) {
        case WM_SIZE:
            layout_controls();
            return 0;
        case WM_COMMAND:
            if (LOWORD(wp) == kEditIdToken && HIWORD(wp) == EN_CHANGE) {
                notify_state_changed();
            }
            return 0;
        case WM_NCDESTROY:
            SetWindowLongPtrW(wnd, GWLP_USERDATA, 0);
            return DefWindowProcW(wnd, msg, wp, lp);
        default:
            return DefWindowProcW(wnd, msg, wp, lp);
        }
    }

    void layout_controls() {
        if (!m_wnd || !m_label || !m_edit) return;

        RECT rc = {};
        GetClientRect(m_wnd, &rc);

        const int margin = 12;
        const int labelHeight = 18;
        const int editHeight = 24;

        SetWindowPos(m_label, nullptr,
            margin, margin,
            (rc.right - rc.left) - margin * 2, labelHeight,
            SWP_NOZORDER);

        SetWindowPos(m_edit, nullptr,
            margin, margin + labelHeight + 6,
            (rc.right - rc.left) - margin * 2, editHeight,
            SWP_NOZORDER);
    }

    pfc::string8 get_edit_text() const {
        int len = GetWindowTextLengthW(m_edit);
        pfc::array_t<wchar_t> buf;
        buf.set_size((t_size)len + 1);
        GetWindowTextW(m_edit, buf.get_ptr(), len + 1);
        pfc::stringcvt::string_utf8_from_wide utf8(buf.get_ptr());
        return pfc::string8(utf8.get_ptr());
    }

    bool has_changed() const {
        pfc::string8 current = get_edit_text();
        trim_ascii_whitespace(current);

        pfc::string8 saved = cfg_gsp_token.get();
        trim_ascii_whitespace(saved);

        return strcmp(current.c_str(), saved.c_str()) != 0;
    }

    void notify_state_changed() {
        if (m_callback.is_valid()) m_callback->on_state_changed();
    }

    preferences_page_callback::ptr m_callback;
    HWND m_wnd = nullptr;
    HWND m_label = nullptr;
    HWND m_edit = nullptr;
};

class gsp_preferences_page : public preferences_page_v3 {
public:
    const char* get_name() override { return "foo_gsp"; }
    GUID get_guid() override {
        static const GUID guid_page =
        { 0x58d5b5f1, 0xc48e, 0x4bd5,{ 0x86,0x6d,0x7d,0xa9,0xa2,0xe2,0x4a,0xd1 } };
        return guid_page;
    }
    GUID get_parent_guid() override { return preferences_page::guid_tools; }
    preferences_page_instance::ptr instantiate(HWND parent, preferences_page_callback::ptr callback) override {
        return fb2k::service_new<gsp_preferences_page_instance>(parent, callback);
    }
};

static preferences_page_factory_t<gsp_preferences_page> g_preferences_page_factory;

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
    pfc::string8 token = cfg_gsp_token.get();
    trim_ascii_whitespace(token);
    if (token.is_empty()) {
        console::print("GSP token is not configured. Open Preferences > Tools > foo_gsp and set 'GSP deployment token'.");
        return;
    }

    pfc::string8 urlUtf8 = "https://script.google.com/macros/s/";
    urlUtf8 += token;
    urlUtf8 += "/exec";
    pfc::stringcvt::string_wide_from_utf8 urlWide(urlUtf8);
    const wchar_t* kUrl = urlWide.get_ptr();

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
