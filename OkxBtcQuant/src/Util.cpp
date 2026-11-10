// Util.cpp
#include "Util.h"
#include "Lang.h"
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <mutex>
#include <chrono>
#include <ctime>
#include <fstream>

#include <openssl/hmac.h>
#include <openssl/sha.h>

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#  include <io.h>
#  ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#    define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004   // 老版 SDK 头文件没有
#  endif
#else
#  include <unistd.h>
#endif

// ---------- 日志 ----------
namespace {
std::mutex g_logMtx;
FILE* g_logFile = nullptr;
bool  g_color = true;
std::string g_consoleDiag = "n/a";
bool g_noCjkFont = false;   // 系统没有可用中文字体（英文系统常见）

bool stdOutIsTty() {
#ifdef _WIN32
    return _isatty(_fileno(stdout)) != 0;
#else
    return isatty(fileno(stdout)) != 0;
#endif
}

const char* levelColor(const char* level) {
    static const char* kNoColor = "";
    if (!g_color) return kNoColor;
    // 控制台颜色（Windows 10+ 与 Linux 终端都支持 ANSI）
    if (!strcmp(level, "ERROR")) return "\033[31m";
    if (!strcmp(level, "WARN ")) return "\033[33m";
    if (!strcmp(level, "DEBUG")) return "\033[36m";
    return "\033[32m";
}
} // namespace

void consoleInit() {
#ifdef _WIN32
    // 1) 控制台代码页切到 UTF-8(65001)，否则 UTF-8 编译的中文会被按 GBK 解释成乱码
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    SetConsoleTitleA("OKX BTC Quant");

    HANDLE hConsole = CreateFileA("CONOUT$", GENERIC_READ | GENERIC_WRITE,
                                  FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
    HANDLE h = (hConsole != INVALID_HANDLE_VALUE) ? hConsole : GetStdHandle(STD_OUTPUT_HANDLE);

    // 2) 窗口放大到 120x35（否则默认 80x25 又小又容易折行）
    {
        CONSOLE_SCREEN_BUFFER_INFO info;
        if (h != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(h, &info)) {
            COORD bufSize;
            bufSize.X = 120;
            bufSize.Y = 3000;
            SetConsoleScreenBufferSize(h, bufSize);
            SMALL_RECT rc;
            rc.Left = 0; rc.Top = 0; rc.Right = 119; rc.Bottom = 34;
            SetConsoleWindowInfo(h, TRUE, &rc);
        }
    }

    // 3) 字体换成支持中文的 TrueType（18px）；
    //    点阵字体("Terminal"/光栅) 渲染不了中文，会显示成方块/乱码。
    //    英文系统（如 Windows 10 LTSC 英文版）可能没装任何中文字体，
    //    依次尝试常见中文字体，回读验证是否真的生效。
    {
        static const wchar_t* kCjkFonts[] = {
            L"NSimSun",            // 新宋体（中文系统默认）
            L"SimSun",             // 宋体
            L"Microsoft YaHei",    // 微软雅黑
            L"Microsoft YaHei UI",
            L"SimHei",             // 黑体
            L"DengXian",           // 等线
        };
        CONSOLE_FONT_INFOEX cur;
        memset(&cur, 0, sizeof(cur));
        cur.cbSize = sizeof(cur);
        bool fontOk = false;
        char face[128] = {0};

        if (h != INVALID_HANDLE_VALUE) {
            for (const wchar_t* name : kCjkFonts) {
                CONSOLE_FONT_INFOEX fi;
                memset(&fi, 0, sizeof(fi));
                fi.cbSize = sizeof(fi);
                fi.nFont = 0;
                fi.dwFontSize.X = 0;
                fi.dwFontSize.Y = 18;
                fi.FontFamily = FF_DONTCARE;
                fi.FontWeight = FW_NORMAL;
                wcscpy_s(fi.FaceName, name);
                SetCurrentConsoleFontEx(h, FALSE, &fi);
                // 回读验证：字体不存在时 SetCurrentConsoleFontEx 静默失败
                memset(&cur, 0, sizeof(cur));
                cur.cbSize = sizeof(cur);
                if (!GetCurrentConsoleFontEx(h, FALSE, &cur)) continue;
                // 比较实际生效字体名（不区分大小写）
                if (_wcsicmp(cur.FaceName, name) == 0) { fontOk = true; break; }
            }
        }
        if (h != INVALID_HANDLE_VALUE && !fontOk && cur.cbSize) {
            // 全部失败：回读当前字体，判断是不是无中文字形的点阵字体
            char curFace[128] = {0};
            WideCharToMultiByte(CP_UTF8, 0, cur.FaceName, -1, curFace, sizeof(curFace), nullptr, nullptr);
            if (strstr(curFace, "Terminal") || strstr(curFace, "Raster") || !curFace[0]) {
                g_noCjkFont = true;   // 提示用户安装中文字体
            }
        }
        if (h != INVALID_HANDLE_VALUE && cur.cbSize) {
            memset(face, 0, sizeof(face));
            WideCharToMultiByte(CP_UTF8, 0, cur.FaceName, -1, face, sizeof(face), nullptr, nullptr);
        }
        char diag[256];
        snprintf(diag, sizeof(diag), TR("CP=%u 字体=%s %dx%d", "CP=%u font=%s %dx%d"),
                 (unsigned)GetConsoleOutputCP(), face[0] ? face : "?",
                 (int)cur.dwFontSize.X, (int)cur.dwFontSize.Y);
        g_consoleDiag = diag;
    }

    // 4) 开启 ANSI 转义序列（颜色）支持；老系统/不支持则关闭颜色，避免打印 "[32m"
    bool vtOk = false;
    DWORD mode = 0;
    if (h != INVALID_HANDLE_VALUE && GetConsoleMode(h, &mode)) {
        if (SetConsoleMode(h, mode | ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING))
            vtOk = true;
    }
    g_color = stdOutIsTty() && vtOk;
    if (hConsole != INVALID_HANDLE_VALUE) CloseHandle(hConsole);
#else
    // 输出被重定向到文件/管道时不上色，避免转义码混进日志
    g_color = stdOutIsTty();
    g_consoleDiag = "UTF-8 terminal";
#endif
}

std::string consoleDiag() { return g_consoleDiag; }
bool noCjkFont() { return g_noCjkFont; }

std::string exeDir() {
#ifdef _WIN32
    char buf[MAX_PATH] = {0};
    DWORD n = GetModuleFileNameA(nullptr, buf, MAX_PATH);
    if (n == 0) return "";
    std::string p(buf, n);
    size_t pos = p.find_last_of("\\/");
    return pos == std::string::npos ? "" : p.substr(0, pos + 1);
#else
    char buf[4096] = {0};
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n <= 0) return "";
    std::string p(buf, (size_t)n);
    size_t pos = p.find_last_of('/');
    return pos == std::string::npos ? "" : p.substr(0, pos + 1);
#endif
}

void logSetFile(const std::string& path) {
    std::lock_guard<std::mutex> lk(g_logMtx);
    if (g_logFile) { fclose(g_logFile); g_logFile = nullptr; }
    if (!path.empty()) {
#ifdef _WIN32
        fopen_s(&g_logFile, path.c_str(), "a");
#else
        g_logFile = fopen(path.c_str(), "a");
#endif
    }
}

void logWrite(const char* level, const char* fmt, ...) {
    char msg[2048];
    va_list ap;
    va_start(ap, fmt);
#ifdef _WIN32
    vsnprintf_s(msg, sizeof(msg), _TRUNCATE, fmt, ap);
#else
    vsnprintf(msg, sizeof(msg), fmt, ap);
#endif
    va_end(ap);

    int64_t ms = nowMs();
    std::time_t t = (std::time_t)(ms / 1000);
    std::tm tmv;
#ifdef _WIN32
    localtime_s(&tmv, &t);
#else
    localtime_r(&t, &tmv);
#endif
    char ts[32];
    strftime(ts, sizeof(ts), "%m-%d %H:%M:%S", &tmv);

    std::lock_guard<std::mutex> lk(g_logMtx);
    const char* color = levelColor(level);
    const char* reset = (color[0] != '\0') ? "\033[0m" : "";
    printf("%s[%s.%03d %s]%s %s\n", color, ts, (int)(ms % 1000), level, reset, msg);
    fflush(stdout);
    if (g_logFile) {
        fprintf(g_logFile, "[%s.%03d %s] %s\n", ts, (int)(ms % 1000), level, msg);
        fflush(g_logFile);
    }
}

// ---------- 加密 ----------
std::string base64Encode(const unsigned char* data, size_t len) {
    static const char* tbl = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(((len + 2) / 3) * 4);
    size_t i = 0;
    while (i + 2 < len) {
        unsigned v = (data[i] << 16) | (data[i + 1] << 8) | data[i + 2];
        out += tbl[(v >> 18) & 63]; out += tbl[(v >> 12) & 63];
        out += tbl[(v >> 6) & 63];  out += tbl[v & 63];
        i += 3;
    }
    if (i + 1 == len) {
        unsigned v = data[i] << 16;
        out += tbl[(v >> 18) & 63]; out += tbl[(v >> 12) & 63];
        out += "==";
    } else if (i + 2 == len) {
        unsigned v = (data[i] << 16) | (data[i + 1] << 8);
        out += tbl[(v >> 18) & 63]; out += tbl[(v >> 12) & 63];
        out += tbl[(v >> 6) & 63];  out += '=';
    }
    return out;
}

std::string hmacSha256Base64(const std::string& key, const std::string& msg) {
    unsigned char md[EVP_MAX_MD_SIZE];
    unsigned int mdLen = 0;
    HMAC(EVP_sha256(), key.data(), (int)key.size(),
         (const unsigned char*)msg.data(), msg.size(), md, &mdLen);
    return base64Encode(md, mdLen);
}

// ---------- 时间 ----------
int64_t nowMs() {
    using namespace std::chrono;
    return (int64_t)duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

std::string isoTimestamp() {
    int64_t ms = nowMs();
    std::time_t t = (std::time_t)(ms / 1000);
    std::tm tmv;
#ifdef _WIN32
    gmtime_s(&tmv, &t);
#else
    gmtime_r(&t, &tmv);
#endif
    char buf[48];
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tmv);
    char out[56];
#ifdef _WIN32
    sprintf_s(out, sizeof(out), "%s.%03dZ", buf, (int)(ms % 1000));
#else
    snprintf(out, sizeof(out), "%s.%03dZ", buf, (int)(ms % 1000));
#endif
    return out;
}

// ---------- 字符串 ----------
std::string trim(const std::string& s) {
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

std::string toLower(std::string s) {
    for (auto& c : s) if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
    return s;
}

std::string fmtDouble(double v, int maxDecimals) {
    if (v != v) return "0"; // NaN
    char buf[64];
    snprintf(buf, sizeof(buf), "%.*f", maxDecimals, v);
    std::string s = buf;
    // 去尾零
    size_t dot = s.find('.');
    if (dot != std::string::npos) {
        size_t last = s.find_last_not_of("0");
        if (last == dot) last = dot - 1;
        s = s.substr(0, last + 1);
    }
    return s;
}
