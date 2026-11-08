// ============================================================================
//  【打印语言开关】  1 = 中文（默认）    0 = 英文    -1 = 跟随系统语言
//  改下面这一个数字就能切换（控制台日志和网页界面都会跟着变）。
//  不想改代码也可以：cmake -DOUTPUT_LANG=en，或运行 video_monitor.exe --lang en
// ============================================================================
#ifndef OUTPUT_LANG
#define OUTPUT_LANG 0
#endif
#if OUTPUT_LANG != 1 && OUTPUT_LANG != 0 && OUTPUT_LANG != -1
#error "OUTPUT_LANG 只能是 1(中文) / 0(英文) / -1(跟随系统)"
#endif

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

// 引入 OpenCV
#include <opencv2/opencv.hpp>
#include <opencv2/core/utils/logger.hpp>
#include <opencv2/video/background_segm.hpp>

// 引入 Crow (轻量级 C++ Web 框架)
#include "crow_all.h"

#ifdef _WIN32
// 控制台编码转换要用 Win32 API。
// asio 已经把 winsock2.h 引进来了，这里再包含 windows.h 不会重复引入旧版 winsock；
// NOMINMAX 防止 windows.h 的 min/max 宏污染 <algorithm>。
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace fs = std::filesystem;

// ==================== 输出语言 ====================
// 程序里的日志/提示原本只有中文。搬到英文版 Windows 上会遇到两个问题：
//   1) 控制台默认代码页是 437/1252，中文直接变乱码；
//   2) 很多英文系统没装中文字体，就算切到 UTF-8 也只能显示方块。
// 所以这里做了一层最简双语，控制台和网页都跟着同一个开关走。
//
// 语言取值优先级（由低到高）：
//   1) 编译期宏 OUTPUT_LANG（在文件最上面，改一个数字就能切）
//   2) 环境变量 VIDEO_MONITOR_LANG=zh|en|auto
//   3) 命令行 --lang zh|en|auto

// 判断当前系统是否中文环境（OUTPUT_LANG=-1 或 --lang auto 时用）
bool system_prefers_chinese() {
#ifdef _WIN32
    return PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_CHINESE;
#else
    const char* v = std::getenv("LC_ALL");
    if (!v || !*v) v = std::getenv("LC_MESSAGES");
    if (!v || !*v) v = std::getenv("LANG");
    // 树莓派/Linux 终端基本都是 UTF-8，中文能正常显示，所以没设置 locale 或
    // 用的是 C/POSIX 时保持中文（和以前的行为一致）；显式设成 en_US 等才切英文
    if (!v || !*v) return true;
    const std::string s = v;
    if (s == "C" || s == "POSIX" || s == "C.UTF-8" || s == "C.utf8") return true;
    return s.find("zh") != std::string::npos;
#endif
}

// 当前是否输出英文。初值来自编译期宏 OUTPUT_LANG，运行时可被 --lang 覆盖。
bool g_lang_en = (OUTPUT_LANG == 0) ? true
                : (OUTPUT_LANG == 1) ? false
                                     : !system_prefers_chinese();

// 每个输出点写成 TR("中文", "English")，两种文案挨在一起，改起来不容易漏
#define TR(zh, en) (g_lang_en ? (en) : (zh))

#ifdef _WIN32
// ==================== 控制台中文输出（解决 cmd 乱码） ====================
// 源码是 UTF-8（CMake 已加 /utf-8），所以程序里的中文字面量在内存中是 UTF-8 字节。
// 而中文版 Windows 的 cmd 默认代码页是 936(GBK)，把 UTF-8 字节直接喂过去就是乱码。
// 这里给 cout/cerr 套一层转换缓冲：写出去之前先 UTF-8 → 控制台代码页。
// 如果控制台本来就是 UTF-8(65001)，或者输出被重定向到文件（没有控制台），则原样输出。
class ConsoleStreambuf : public std::streambuf {
public:
    ConsoleStreambuf(std::streambuf* target, UINT cp) : target_(target), cp_(cp) {}

protected:
    int overflow(int c) override {
        if (c != EOF) {
            buf_.push_back(static_cast<char>(c));
            if (buf_.size() >= 1024) flush();
        }
        return c;
    }

    std::streamsize xsputn(const char* s, std::streamsize n) override {
        buf_.append(s, static_cast<size_t>(n));
        if (buf_.size() >= 4096) flush();
        return n;
    }

    int sync() override {
        flush();
        target_->pubsync();
        return 0;
    }

private:
    void flush() {
        if (buf_.empty()) return;
        const int n = static_cast<int>(buf_.size());

        const int wlen = MultiByteToWideChar(CP_UTF8, 0, buf_.data(), n, nullptr, 0);
        if (wlen > 0) {
            std::wstring w(static_cast<size_t>(wlen), L'\0');
            MultiByteToWideChar(CP_UTF8, 0, buf_.data(), n, &w[0], wlen);

            const int alen = WideCharToMultiByte(cp_, 0, w.data(), wlen, nullptr, 0, nullptr, nullptr);
            if (alen > 0) {
                std::string a(static_cast<size_t>(alen), '\0');
                WideCharToMultiByte(cp_, 0, w.data(), wlen, &a[0], alen, nullptr, nullptr);
                target_->sputn(a.data(), alen);
            }
        } else {
            // 非法 UTF-8 就原样输出，至少不丢内容
            target_->sputn(buf_.data(), static_cast<std::streamsize>(buf_.size()));
        }
        buf_.clear();
    }

    std::streambuf* target_;
    UINT cp_;
    std::string buf_;
};

inline void setup_console_encoding() {
    UINT cp = GetConsoleOutputCP();

    // 中文/日文/韩文代码页本身就表示得了这些文字，沿用「UTF-8 按代码页转换」的老办法，
    // 这条路径在中文 Windows 上是验证过的，最稳。
    const bool cjk_cp = (cp == 936 || cp == 950 || cp == 932 || cp == 949);

    // 英文系统的控制台是 437/1252，根本装不下中文（转过去就是 ? 或乱码）。
    // 这种情况下先试着把控制台切成 UTF-8(65001)：现代 conhost / Windows Terminal
    // 支持得很好，配 --lang zh 也能显示中文（前提是系统装了中文字体）。
    if (!cjk_cp && cp != 0 && cp != CP_UTF8 && GetConsoleWindow() != nullptr) {
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
        cp = GetConsoleOutputCP();
    }

    // cp==0：输出被重定向到文件/管道，没有控制台；
    // cp==UTF8：UTF-8 字节直接输出即可
    if (cp == 0 || cp == CP_UTF8) return;

    static ConsoleStreambuf out_buf(std::cout.rdbuf(), cp);
    static ConsoleStreambuf err_buf(std::cerr.rdbuf(), cp);
    std::cout.rdbuf(&out_buf);
    std::cerr.rdbuf(&err_buf);
}
#else
inline void setup_console_encoding() {}
#endif

// 全局共享变量与锁
std::mutex frame_mutex;
cv::Mat global_current_frame;          // 用于网页实时查看最新的帧
std::atomic<bool> is_running{true};    // 多线程共享状态，必须用 atomic

// 网页上的两个开关，默认都是关的：
//   g_capture_on  实时画面 —— 只有点了「开始视频采集」，采集线程才把帧交给 /snapshot
//   g_record_on   自动检测录像 —— 只有点了「开始录制」，才跑运动检测、有异动才录
// 关掉后 /snapshot 不会给出任何画面，避免网页在没人看的时候空拉图。
std::atomic<bool> g_capture_on{false};
std::atomic<bool> g_record_on{false};
// 当前是否真有一段录像正在写（网页用来显示「录制中」红点）
std::atomic<bool> g_is_recording{false};

// 正在录制的文件名（空 = 没在录）。ffmpeg 写 mp4 时 moov 原子在文件尾部，
// 录制过程中打开会报 "moov atom not found"；而刚开始录的几秒 ffmpeg 还在缓冲、
// 根本没往磁盘写，靠 mtime 判断不出来，所以这里由采集线程直接登记。
std::mutex rec_file_mutex;
std::string current_recording_file;

void set_recording_file(const std::string& name) {
    std::lock_guard<std::mutex> lk(rec_file_mutex);
    current_recording_file = name;
}

bool is_recording_file(const std::string& name) {
    std::lock_guard<std::mutex> lk(rec_file_mutex);
    return !current_recording_file.empty() && current_recording_file == name;
}

// 跨平台的 popen/pclose 封装（失败时保留 errno 供调用方打印原因）
FILE* cross_popen(const char* command, const char* mode) {
    errno = 0;
#ifdef _WIN32
    return _popen(command, mode);
#else
    return popen(command, mode);
#endif
}

int cross_pclose(FILE* stream) {
#ifdef _WIN32
    return _pclose(stream);
#else
    return pclose(stream);
#endif
}

// ffmpeg 可执行文件位置，按优先级：
//   1) 环境变量 VIDEO_MONITOR_FFMPEG 指定的完整路径
//   2) Windows: 项目目录下的 ffmpeg\ffmpeg.exe（随项目分发，不用配 PATH）
//   3) 直接用 PATH 里的 ffmpeg
std::string ffmpeg_exe() {
    static const std::string path = [] {
        if (const char* env = std::getenv("VIDEO_MONITOR_FFMPEG")) {
            if (*env) return std::string(env);
        }
#ifdef _WIN32
        std::error_code ec;
        if (fs::exists("./ffmpeg/ffmpeg.exe", ec)) return std::string("ffmpeg\\ffmpeg.exe");
#endif
        return std::string("ffmpeg");
    }();
    return path;
}

// 拼命令行时用：只有路径里含空格（或 cmd 元字符）才需要加引号。
// 注意：反斜杠不需要引号，而 MSVC 里 ".\\ffmpeg\\ffmpeg.exe" 这种相对路径一旦加上引号，
// cmd.exe 会把它当成带引号的命令名，报「不是内部或外部命令」。
std::string ffmpeg_prefix() {
    const std::string exe = ffmpeg_exe();
    if (exe.find(' ') != std::string::npos || exe.find('&') != std::string::npos ||
        exe.find('(') != std::string::npos)
        return "\"" + exe + "\"";
    return exe;
}

// 检查 ffmpeg 是否带 H.264 编码器（精简版 ffmpeg 可能没有 libx264）
bool ffmpeg_has_libx264() {
#ifdef _WIN32
    const std::string cmd = ffmpeg_prefix() + " -hide_banner -encoders 2>NUL | findstr /C:libx264 > NUL";
#else
    const std::string cmd = ffmpeg_prefix() + " -hide_banner -encoders 2>/dev/null | grep -q libx264";
#endif
    return std::system(cmd.c_str()) == 0;
}

// 检查系统里是否有 ffmpeg 可用
bool ffmpeg_available() {
#ifdef _WIN32
    const std::string cmd = ffmpeg_prefix() + " -version > NUL 2>&1";
#else
    const std::string cmd = ffmpeg_prefix() + " -version > /dev/null 2>&1";
#endif
    return std::system(cmd.c_str()) == 0;
}

// 检查 ffmpeg 是否带指定编码器（判断字符串时按完整名字边界匹配，避免误判）
bool ffmpeg_has_encoder(const std::string& name) {
    const std::string cmd = ffmpeg_prefix() + " -hide_banner -loglevel error -encoders 2>&1";
    FILE* p = cross_popen(cmd.c_str(), "r");
    if (!p) return false;
    std::string out;
    char buf[4096];
    size_t n = 0;
    while ((n = fread(buf, 1, sizeof(buf), p)) > 0) out.append(buf, n);
    cross_pclose(p);
    return out.find(" " + name + " ") != std::string::npos;
}

// 执行命令并收集输出（stdout+stderr），用于探测设备
std::string run_capture(const std::string& cmd) {
    FILE* p = cross_popen(cmd.c_str(), "r");
    if (!p) return std::string();
    std::string out;
    char buf[4096];
    size_t n = 0;
    while ((n = fread(buf, 1, sizeof(buf), p)) > 0) out.append(buf, n);
    cross_pclose(p);
    return out;
}

#ifdef _WIN32
// 把 UTF-8 字符串转成当前系统 ANSI 代码页（中文 Windows 就是 GBK）。
// _popen 传给 cmd.exe 的是窄字符串，CRT 按 ANSI 解释；我们的源码/字面量是 UTF-8，
// 不转就会把中文设备名变成乱码，ffmpeg 自然找不到设备。
std::string utf8_to_acp(const std::string& s) {
    const int wlen = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    if (wlen <= 0) return s;
    std::wstring w(static_cast<size_t>(wlen), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], wlen);

    const int alen = WideCharToMultiByte(CP_ACP, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (alen <= 0) return s;
    std::string a(static_cast<size_t>(alen), '\0');
    WideCharToMultiByte(CP_ACP, 0, w.c_str(), -1, &a[0], alen, nullptr, nullptr);
    a.pop_back(); // 去掉结尾的 '\0'
    return a;
}
#endif

bool is_ascii(const std::string& s) {
    for (unsigned char c : s)
        if (c < 32 || c >= 127) return false;
    return !s.empty();
}

// 英文模式下打设备名时用：把非 ASCII 字符（中文设备名等）换成 ?，
// 这样在 437/1252 代码页的英文控制台上也不会出现整行乱码，
// 名字里的英文部分（如 "icspring camera"）仍然可读。
std::string ascii_safe(const std::string& s) {
    if (!g_lang_en) return s;
    std::string out;
    out.reserve(s.size());
    bool prev_replaced = false;
    for (unsigned char c : s) {
        if (c < 0x80) {
            out.push_back(static_cast<char>(c));
            prev_replaced = false;
        } else if (!prev_replaced) {
            out.push_back('?');
            prev_replaced = true;
        }
    }
    return out;
}

// ==================== 音频（麦克风）探测 ====================
// 目标：「录视频的时候把声音一起录进去」。
// 设备探测分平台：
//   Windows: 枚举 DirectShow 音频设备。ffmpeg 对每个设备都会给出一个
//            "Alternative name"（形如 @device_cm_{...}\wave_{...}），是纯 ASCII 的，
//            优先用它，可以彻底避开中文设备名在 popen/cmd 里的编码问题。
//   Linux  : 用 arecord -l 找 USB 摄像头的采集声卡，拼成 plughw:N,M。树莓派上
//            USB 摄像头的麦一般就是唯一的采集设备。
// 环境变量：
//   REC_AUDIO=0         关闭录音（只录视频）
//   AUDIO_DEV=<设备名>  手动指定；Windows 填 ffmpeg -list_devices 显示的名字，
//                       Linux 填 plughw:1,0 这类 ALSA 设备名
struct AudioInput {
    bool available = false;
    std::string input_args; // 直接拼进 ffmpeg 命令的输入段
    std::string describe;   // 日志里显示给人看的名字
};

#ifdef _WIN32
struct DShowDevice {
    std::string name; // 友好名（可能含中文，仅供显示）
    std::string alt;  // Alternative name（纯 ASCII，用于命令行）
};

std::vector<DShowDevice> list_dshow_audio_devices() {
    const std::string out =
        run_capture(ffmpeg_prefix() + " -hide_banner -list_devices true -f dshow -i dummy 2>&1");

    std::vector<DShowDevice> devs;
    std::istringstream ss(out);
    std::string line;
    while (std::getline(ss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const size_t audio_pos = line.find("(audio)");
        if (audio_pos == std::string::npos) continue;

        // 形如：[dshow @ 0000] "麦克风 (icspring camera)" (audio)
        const size_t q2 = line.rfind('"', audio_pos);
        if (q2 == std::string::npos) continue;
        const size_t q1 = line.rfind('"', q2 - 1);
        if (q1 == std::string::npos || q2 <= q1 + 1) continue;

        DShowDevice d;
        d.name = line.substr(q1 + 1, q2 - q1 - 1);

        // 紧跟的下一行是 Alternative name
        std::string next;
        if (std::getline(ss, next)) {
            if (!next.empty() && next.back() == '\r') next.pop_back();
            if (next.find("Alternative name") != std::string::npos) {
                const size_t a1 = next.find('"');
                const size_t a2 = next.rfind('"');
                if (a1 != std::string::npos && a2 != std::string::npos && a2 > a1)
                    d.alt = next.substr(a1 + 1, a2 - a1 - 1);
            }
        }
        devs.push_back(d);
    }
    return devs;
}
#endif

// 设备是否真的能打开（列得出来 ≠ 打得开：可能被其它程序独占）
bool audio_input_works(const std::string& input_args) {
    const std::string cmd = ffmpeg_prefix() + " -hide_banner -loglevel error " + input_args +
                            " -t 0.3 -f null - > " +
#ifdef _WIN32
                            "NUL 2>&1";
#else
                            "/dev/null 2>&1";
#endif
    return std::system(cmd.c_str()) == 0;
}

AudioInput detect_audio_input() {
    AudioInput a;

    // REC_AUDIO=0 / off / false 显式关闭
    if (const char* e = std::getenv("REC_AUDIO")) {
        const std::string v = e;
        if (v == "0" || v == "off" || v == "false") {
            a.describe = TR("已被 REC_AUDIO=0 关闭", "disabled by REC_AUDIO=0");
            return a;
        }
    }

    const char* env_dev = std::getenv("AUDIO_DEV");
    const std::string forced = (env_dev && *env_dev) ? std::string(env_dev) : std::string();

#ifdef _WIN32
    std::string arg_dev; // 实际写进命令行的设备标识
    std::string show_dev;

    if (!forced.empty()) {
        show_dev = forced;
        arg_dev = utf8_to_acp(forced);
    } else {
        const auto devs = list_dshow_audio_devices();
        if (devs.empty()) return a;

        // 优先选 USB 摄像头自带的麦：名字里带 camera/usb/webcam
        const DShowDevice* pick = nullptr;
        for (const auto& d : devs) {
            std::string low = d.name;
            std::transform(low.begin(), low.end(), low.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (low.find("camera") != std::string::npos || low.find("usb") != std::string::npos ||
                low.find("webcam") != std::string::npos) {
                pick = &d;
                break;
            }
        }
        if (!pick) pick = &devs.front();

        show_dev = pick->name;
        // 有 ASCII 别名就用别名，否则把中文名转成 ANSI 再用
        if (is_ascii(pick->alt))
            arg_dev = pick->alt;
        else
            arg_dev = utf8_to_acp(pick->name);
    }

    if (arg_dev.empty() || arg_dev.find('"') != std::string::npos) return a;

    a.input_args = "-f dshow -i audio=\"" + arg_dev + "\"";
    a.describe = "DirectShow: " + ascii_safe(show_dev);

#else
    // Linux/树莓派：找 ALSA 采集设备
    std::string arg_dev;
    std::string show_dev;

    if (!forced.empty()) {
        arg_dev = forced;
        show_dev = forced;
    } else {
        const std::string out = run_capture("arecord -l 2>/dev/null");
        int best_card = -1, best_dev = -1;
        std::string best_desc;
        std::istringstream ss(out);
        std::string line;
        while (std::getline(ss, line)) {
            const size_t cpos = line.find("card ");
            if (cpos == std::string::npos) continue;
            const size_t dpos = line.find("device ");
            if (dpos == std::string::npos) continue;

            const int card = std::atoi(line.c_str() + cpos + 5);
            const int dev = std::atoi(line.c_str() + dpos + 7);
            std::string desc = line;
            std::transform(desc.begin(), desc.end(), desc.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

            const bool looks_usb = desc.find("usb") != std::string::npos ||
                                   desc.find("camera") != std::string::npos ||
                                   desc.find("mic") != std::string::npos;
            if (best_card < 0 || looks_usb) {
                best_card = card;
                best_dev = dev;
                best_desc = line;
                if (looks_usb) break; // 命中摄像机自带的麦，直接用
            }
        }
        if (best_card < 0) {
            // 一个都没解析到就退回 default（多数发行版会指向默认输入）
            arg_dev = "default";
            show_dev = "ALSA: default";
        } else {
            arg_dev = "plughw:" + std::to_string(best_card) + "," + std::to_string(best_dev);
            show_dev = "ALSA: " + arg_dev;
        }
    }

    a.input_args = "-f alsa -i " + arg_dev;
    a.describe = show_dev;
#endif

    a.available = true;
    return a;
}

// 视频源：默认第 0 号摄像头，可用 --camera / --source 或环境变量 VIDEO_MONITOR_SOURCE 覆盖：
//   0 / 1 / 2 …              摄像头序号
//   D:\samples\test.mp4      本地视频文件（循环播放，没有摄像头时用它验证整条链路）
//   rtsp://ip:554/stream1    网络摄像头/流转发
// 优先级：命令行 > 环境变量 > 默认 0
std::string g_source_override; // main() 里解析命令行后填写
int g_web_port = 30000;        // Web 端口，可被 --port / VIDEO_MONITOR_PORT 覆盖

std::string video_source() {
    if (!g_source_override.empty()) return g_source_override;
    if (const char* env = std::getenv("VIDEO_MONITOR_SOURCE")) {
        if (*env) return std::string(env);
    }
    return std::string("0");
}

bool is_camera_index(const std::string& s) {
    return !s.empty() && s.find_first_not_of("0123456789") == std::string::npos;
}

// ==================== 摄像头枚举（多摄像头时选对设备） ====================
// 笔记本插了 USB 摄像头后，系统里就有 2 个视频设备，而 index 0 通常是内置摄像头。
// 光看 0/1 分不清谁是谁，所以这里把设备名一起列出来：
//   Windows: ffmpeg -list_devices 的顺序 = DirectShow 枚举顺序 = cv::CAP_DSHOW 的 index
//   Linux  : /dev/videoN 加上 /sys 里的设备名
struct VideoDeviceInfo {
    int index = -1;
    std::string name;
};

#ifdef _WIN32
std::vector<VideoDeviceInfo> list_video_devices() {
    // ffmpeg 把设备列表打到 stderr，这里合并回来
    const std::string out = run_capture(ffmpeg_prefix() +
                                        " -hide_banner -list_devices true -f dshow -i dummy 2>&1");
    std::vector<VideoDeviceInfo> devs;
    std::istringstream ss(out);
    std::string line;
    int idx = 0;
    while (std::getline(ss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.find("DirectShow audio devices") != std::string::npos) break;
        const size_t pos = line.find("(video)");
        if (pos == std::string::npos) continue;

        // 形如：[dshow @ 0000] "Integrated Camera" (video)
        const size_t q2 = line.rfind('"', pos);
        if (q2 == std::string::npos) continue;
        const size_t q1 = line.rfind('"', q2 - 1);
        if (q1 == std::string::npos || q2 <= q1 + 1) continue;

        VideoDeviceInfo d;
        d.index = idx++;
        d.name = line.substr(q1 + 1, q2 - q1 - 1);
        devs.push_back(d);
    }
    return devs;
}
#else
std::vector<VideoDeviceInfo> list_video_devices() {
    std::vector<VideoDeviceInfo> devs;
    for (int i = 0; i < 12; ++i) {
        const std::string dev = "/dev/video" + std::to_string(i);
        std::error_code ec;
        if (!fs::exists(dev, ec)) continue;
        VideoDeviceInfo d;
        d.index = i;
        std::ifstream in("/sys/class/video4linux/video" + std::to_string(i) + "/name");
        std::string n;
        if (in && std::getline(in, n)) d.name = n;
        if (d.name.empty()) d.name = TR("(未知设备)", "(unknown device)");
        devs.push_back(d);
    }
    return devs;
}
#endif

// 只枚举一次，后面多处（启动提示、列表接口）复用
const std::vector<VideoDeviceInfo>& cached_video_devices() {
    static const std::vector<VideoDeviceInfo> devs = list_video_devices();
    return devs;
}

std::string camera_name_of(int index) {
    for (const auto& d : cached_video_devices())
        if (d.index == index) return d.name;
    return std::string();
}

// 按序号打开摄像头。Windows 上显式指定 DirectShow：
//   1) index 顺序和上面枚举出来的完全一致，--camera 1 就是列表里的第 1 项；
//   2) 比默认的 MSMF 打开更快、兼容性更好；
//   3) 打不开再退回默认后端，避免个别驱动只支持 MSMF 时直接失败。
bool open_camera(cv::VideoCapture& cap, int index) {
#ifdef _WIN32
    if (cap.open(index, cv::CAP_DSHOW)) return true;
    cap.release();
    std::cerr << TR("[警告] DirectShow 打不开摄像头 ", "[WARN] DirectShow failed to open camera ")
              << index << TR("，改用默认后端重试…", ", retrying with the default backend...")
              << std::endl;
    return cap.open(index);
#else
    return cap.open(index, cv::CAP_V4L2);
#endif
}

// 估算字符串在终端里的显示宽度（UTF-8：中文等东亚字符占 2 列，其余 1 列），
// 只为了把设备列表对齐，不影响逻辑。
size_t display_width(const std::string& s) {
    size_t w = 0;
    for (size_t i = 0; i < s.size();) {
        const unsigned char c = s[i];
        size_t len = 1;
        unsigned cp = c;
        if (c >= 0xF0) { len = 4; cp = c & 0x07u; }
        else if (c >= 0xE0) { len = 3; cp = c & 0x0Fu; }
        else if (c >= 0xC0) { len = 2; cp = c & 0x1Fu; }
        for (size_t k = 1; k < len && i + k < s.size(); ++k) cp = (cp << 6) | (s[i + k] & 0x3Fu);
        const bool wide = (cp >= 0x1100 && cp <= 0x115F) || (cp >= 0x2E80 && cp <= 0xA4CF) ||
                          (cp >= 0xAC00 && cp <= 0xD7A3) || (cp >= 0xF900 && cp <= 0xFAFF) ||
                          (cp >= 0xFE30 && cp <= 0xFE6F) || (cp >= 0xFF00 && cp <= 0xFF60);
        w += wide ? 2 : 1;
        i += len;
    }
    return w;
}

// --list-cameras：把每个摄像头真正打开一次，打印能协商到的分辨率和帧率
int list_cameras() {
    std::cout << TR("[系统] 正在枚举摄像头…", "[System] Enumerating cameras...") << std::endl;
    const auto& devs = cached_video_devices();
    if (devs.empty()) {
        std::cout << TR("[警告] 没有找到任何摄像头。", "[WARN] No camera found.") << std::endl;
        return 1;
    }

    for (const auto& d : devs) {
        std::string info = TR("打不开（可能被其它程序占用）", "cannot open (in use by another app?)");
        cv::VideoCapture cap;
        if (open_camera(cap, d.index)) {
            const int w = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
            const int h = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
            const double f = cap.get(cv::CAP_PROP_FPS);
            char buf[96];
            std::snprintf(buf, sizeof(buf), "%dx%d @ %.1f FPS", w, h, f);
            info = buf;
            cv::Mat frame;
            if (!cap.read(frame) || frame.empty())
                info += TR("（读不到画面）", " (no frames)");
            cap.release();
        }
        // 用 cout 而不是 printf：Windows 下 cout 已经挂了 UTF-8→GBK 的转换层，
        // printf 是直写，遇到中文设备名会乱码。
        std::string padded = ascii_safe(d.name);
        while (display_width(padded) < 38) padded.push_back(' ');
        std::cout << "  [" << d.index << "] " << padded << info << std::endl;
    }

    std::cout << TR("\n用法：  video_monitor.exe --camera <编号>     例如 --camera 1",
                    "\nUsage:  video_monitor.exe --camera <n>     e.g. --camera 1")
              << std::endl;
    std::cout << TR("        （编号就是上面方括号里的数字）",
                    "        (the number in brackets above)")
              << std::endl;
    return 0;
}

// ==================== 模块 1 & 2：视频采集、运动检测与录像线程 ====================
void video_monitor_thread() {
    // 1. 打开视频源
    // Linux 上显式指定 V4L2 后端：OpenCV 在树莓派上会先试 GStreamer，
    // 而 v4l2src 往往起不来（Internal data stream error），白白刷一屏警告才回退到 V4L2。
    // 指定后端后警告消失，且下面的分辨率/帧率设置才真正落到 V4L2 上。
    const std::string source = video_source();
    const bool source_is_index = is_camera_index(source);
    const bool source_is_local_file = !source_is_index && source.find("://") == std::string::npos;

    cv::VideoCapture cap;
    if (source_is_index) {
        const int idx = std::atoi(source.c_str());

        // 多摄像头（笔记本内置 + USB）时，先把设备列表打出来，
        // 让人一眼看出当前用的是哪个，以及该怎么切。
        const auto& devs = cached_video_devices();
        if (devs.size() > 1) {
            std::cout << TR("[系统] 检测到 ", "[System] Found ") << devs.size()
                      << TR(" 个摄像头：", " camera(s):") << std::endl;
            for (const auto& d : devs) {
                std::cout << "        [" << d.index << "] " << ascii_safe(d.name)
                          << (d.index == idx ? TR("   ← 当前使用", "   <- in use") : "")
                          << std::endl;
            }
            std::cout << TR("        切换设备：video_monitor.exe --camera <编号>"
                            "（或设置环境变量 VIDEO_MONITOR_SOURCE=<编号>）",
                            "        Switch: video_monitor.exe --camera <n>"
                            " (or set VIDEO_MONITOR_SOURCE=<n>)")
                      << std::endl;
        }
        open_camera(cap, idx);
    } else {
        cap.open(source);
    }

    if (!cap.isOpened()) {
        if (source_is_index) {
            std::cerr << TR("[错误] 无法打开摄像头 ", "[ERROR] Cannot open camera ") << source
                      << TR("！", "!") << std::endl;
            std::cerr << TR("        先运行 video_monitor.exe --list-cameras 看看有哪些设备、"
                            "编号是多少。",
                            "        Run video_monitor.exe --list-cameras to see available devices.")
                      << std::endl;
        } else {
            std::cerr << TR("[错误] 无法打开视频源: ", "[ERROR] Cannot open video source: ") << source
                      << std::endl;
        }
#ifndef _WIN32
        if (source_is_index)
            std::cerr << TR("        Linux 下请确认：摄像头存在(/dev/video0)、当前用户在 video 组内"
                            "(sudo usermod -aG video $USER 后需重新登录)。",
                            "        On Linux check that /dev/video0 exists and your user is in the"
                            " 'video' group (sudo usermod -aG video $USER, then re-login).")
                      << std::endl;
#endif
        if (source_is_index)
            std::cerr << TR("        没有摄像头时可以用视频文件代替，例如（Windows）：\n"
                            "        set VIDEO_MONITOR_SOURCE=D:\\samples\\test.mp4 && video_monitor.exe",
                            "        Without a camera you can use a video file instead:\n"
                            "        set VIDEO_MONITOR_SOURCE=D:\\samples\\test.mp4 && video_monitor.exe")
                      << std::endl;
        return;
    }

    if (source_is_index) {
        const std::string cam_name = camera_name_of(std::atoi(source.c_str()));
        if (!cam_name.empty())
            std::cout << TR("[系统] 使用摄像头 [", "[System] Using camera [") << source << "] "
                      << ascii_safe(cam_name) << std::endl;
    } else if (source != "0") {
        std::cout << TR("[系统] 视频源: ", "[System] Video source: ") << source
                  << (source_is_local_file ? TR("（文件，循环播放）", " (file, looping)") : "")
                  << std::endl;
    }

    // 摄像头格式协商：默认只请求分辨率，不强制像素格式和帧率，用驱动给的原生格式。
    // 之前强制 MJPG 后，这块摄像头/驱动返回的 JPEG 帧被截断，OpenCV 解码时刷
    // "Corrupt JPEG data: premature end of data segment"，画面也不可信，所以默认不启用。
    // 需要时可用环境变量覆盖，例如：
    //   CAM_WIDTH=1280 CAM_HEIGHT=720 CAM_FOURCC=MJPG CAM_FPS=15 ./video_monitor
    int req_w = 640, req_h = 480;
    if (const char* s = std::getenv("CAM_WIDTH")) req_w = std::atoi(s);
    if (const char* s = std::getenv("CAM_HEIGHT")) req_h = std::atoi(s);
    if (req_w > 0 && req_h > 0) {
        cap.set(cv::CAP_PROP_FRAME_WIDTH, req_w);
        cap.set(cv::CAP_PROP_FRAME_HEIGHT, req_h);
    }
    if (const char* s = std::getenv("CAM_FOURCC")) {
        if (std::strlen(s) == 4)
            cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc(s[0], s[1], s[2], s[3]));
    }
    if (const char* s = std::getenv("CAM_FPS")) {
        const double v = std::atof(s);
        if (v > 0) cap.set(cv::CAP_PROP_FPS, v);
    }

    // 获取摄像头的基本信息
    int width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    int height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    double fps = cap.get(cv::CAP_PROP_FPS);
    if (fps <= 0 || fps > 120) fps = 25.0; // 防止获取不到 / 拿到异常 FPS
    if (width <= 0 || height <= 0) {        // 少数摄像头驱动拿不到分辨率，给个兜底
        width = 640;
        height = 480;
    }
    // 录像声明帧率：取 min(标称值, 15)。树莓派的 CPU 跑不动 30fps 的软编码，
    // 如果按标称 30fps 声明，实际只有 ~11fps 的帧被压进 30fps 的时间轴，
    // 播放时会 2~3 倍速快进 + 卡顿，看起来就是画面"晃动"。

    // 打印实际协商到的像素格式（YUYV / MJPG …），排查画面问题时很有用
    const int fourcc_val = static_cast<int>(cap.get(cv::CAP_PROP_FOURCC));
    std::string fourcc_str;
    if (fourcc_val == 0) {
        fourcc_str = TR("未知", "unknown"); // 部分后端（如 MSMF）不上报 FOURCC
    } else {
        bool printable = true;
        for (int i = 0; i < 4; ++i) {
            const int c = (fourcc_val >> (8 * i)) & 0xFF;
            if (c >= 32 && c < 127)
                fourcc_str.push_back(static_cast<char>(c));
            else
                printable = false;
        }
        if (!printable) {
            // 不是可读的四个字符，就直接打十六进制，别再输出一串问号
            char hex[16];
            std::snprintf(hex, sizeof(hex), "0x%08X", static_cast<unsigned>(fourcc_val));
            fourcc_str = hex;
        }
    }
    std::cout << TR("[系统] 摄像头开启成功: ", "[System] Camera opened: ") << width << "x" << height
              << " @ " << fps << TR("FPS, 格式 ", "FPS, format ") << fourcc_str << std::endl;

    const bool has_ffmpeg = ffmpeg_available();
    if (!has_ffmpeg) {
        std::cerr << TR("[警告] 未检测到 ffmpeg，运动检测和网页预览仍可用，但不会录像。"
                       " 安装方式：sudo apt install ffmpeg",
                       "[WARN] ffmpeg not found. Motion detection and web preview still work, but"
                       " nothing will be recorded. Install it (sudo apt install ffmpeg / put"
                       " ffmpeg.exe next to the program).")
                  << std::endl;
    }

    // 没有 libx264 就别走管道了，否则 ffmpeg 一起来就退出，白折腾
    bool use_pipe = has_ffmpeg;
    if (has_ffmpeg && !ffmpeg_has_libx264()) {
        use_pipe = false;
        std::cerr << TR("[警告] 当前 ffmpeg 不含 libx264 编码器，改用 OpenCV 内置编码器录制。"
                       " 想要 H.264（浏览器可直接播放）请安装完整版 ffmpeg。",
                       "[WARN] This ffmpeg has no libx264 encoder; falling back to OpenCV's built-in"
                       " encoder. Install a full ffmpeg build for H.264 (playable in browsers).")
                  << std::endl;
    }

    // Pi 4/5 有 H.264 硬件编码器（v4l2m2m），编码几乎不占 CPU，能轻松喂满录像帧率；
    // 若 ffmpeg 不带该编码器或初始化失败，会在首次录像失败后永久切回 libx264 软编。
#ifndef _WIN32
    bool use_hw_encoder = true;
#endif

    // ---------------- 录音（麦克风） ----------------
    // 只有走 ffmpeg 管道才可能带音频：OpenCV 内置 VideoWriter 只写视频轨。
    const AudioInput audio = (has_ffmpeg && use_pipe) ? detect_audio_input() : AudioInput{};
    bool rec_audio = audio.available;
    if (rec_audio) {
        const bool aac_ok = ffmpeg_has_encoder("aac");
        if (!aac_ok) {
            rec_audio = false;
            std::cerr << TR("[警告] 当前 ffmpeg 不含 aac 音频编码器，只录视频。"
                            " 安装完整版 ffmpeg 即可解决。",
                            "[WARN] This ffmpeg has no aac encoder; recording video only."
                            " Install a full ffmpeg build to fix it.")
                      << std::endl;
        } else if (!audio_input_works(audio.input_args)) {
            rec_audio = false;
            std::cerr << TR("[警告] 麦克风打不开（可能被其它程序占用），本次只录视频。",
                            "[WARN] Microphone cannot be opened (in use by another app?),"
                            " recording video only.")
                      << std::endl;
            std::cerr << TR("        探测命令: ", "        probe command: ") << ffmpeg_prefix()
                      << " -hide_banner -loglevel error " << audio.input_args << " -t 0.3 -f null -"
                      << std::endl;
        } else {
            std::cout << TR("[系统] 录音已启用: ", "[System] Audio enabled: ") << audio.describe
                      << std::endl;
        }
    } else if (has_ffmpeg && use_pipe) {
        if (!audio.describe.empty())
            std::cout << TR("[系统] ", "[System] ") << audio.describe
                      << TR("，只录视频。", " - recording video only.") << std::endl;
        else
            std::cout << TR("[系统] 未找到可用麦克风，只录视频（可用 AUDIO_DEV 手动指定设备名）。",
                            "[System] No microphone found, recording video only"
                            " (set AUDIO_DEV to specify one).")
                      << std::endl;
    }

    // 录像声明帧率：环路实际能力为准（详见上方注释），采集慢于它时用重复帧补齐
    const double rec_fps = std::min(fps, 15.0);
    std::cout << TR("[系统] 录像帧率: ", "[System] Recording at ") << rec_fps
              << TR(" FPS（采集慢于此值时补重复帧，时长仍准确）",
                    " FPS (frames are duplicated if capture is slower; duration stays accurate)")
              << std::endl;

    // 2. 初始化 OpenCV MOG2 运动检测器
    auto pMOG2 = cv::createBackgroundSubtractorMOG2(500, 16, true);
    cv::Mat frame, fgMask;

    // 录像状态控制
    bool is_recording = false;
    bool cur_rec_audio = false;   // 当前这段录像是否带音频轨（失败时据此降级）
    FILE* ffmpeg_pipe = nullptr;
    auto last_motion_time = std::chrono::steady_clock::now();
    const int record_extend_seconds = 5; // 检测到运动后，至少持续录像 5 秒防止断续

    // 墙钟补帧用：录像起始时刻 + 已写入帧数。每个 1/rec_fps 的时隙写一帧，
    // 采集慢于 rec_fps 时重复当前帧补齐，保证 mp4 时长 = 真实时长、帧距均匀。
    auto rec_start_time = std::chrono::steady_clock::now();
    long rec_frames_written = 0;

    // 启动失败时的退避控制：否则画面持续有动作时，每一帧都会重试并刷屏
    int record_fail_count = 0;
    bool record_disabled = false;
    auto next_record_try = std::chrono::steady_clock::time_point{};

    // ffmpeg 管道起不来时的保底方案：直接用 OpenCV 内置的 ffmpeg 后端写文件
    cv::VideoWriter fallback_writer;

    // 创建录像存储文件夹
    std::error_code ec;
    fs::create_directories("./videos", ec);

    // 帧间隔控制：按固定节拍采集，避免录像播放速度越来越快
    const auto frame_interval = std::chrono::microseconds(static_cast<int>(1000000.0 / fps));

    // 点开「开始录制」后先喂几帧背景模型再开始判定：pMOG2 空模型时整幅画面
    // 都会被当成前景，不预热的话一点按钮就会立刻误录一段。
    int bg_warmup_frames = 0;
    bool prev_want_record = false;

    while (is_running) {
        const auto loop_start = std::chrono::steady_clock::now();

        // 按固定节拍采集，防止空转榨干 CPU。循环尾部和「跳过检测」的分支共用
        auto pace = [&loop_start, &frame_interval]() {
            const auto elapsed = std::chrono::steady_clock::now() - loop_start;
            if (elapsed < frame_interval)
                std::this_thread::sleep_for(frame_interval - elapsed);
        };

        cap >> frame;
        if (frame.empty()) {
            // 本地文件放完就回到开头（模拟摄像头持续出帧），摄像头/网络流断开则退出
            if (source_is_local_file) {
                cap.set(cv::CAP_PROP_POS_FRAMES, 0);
                cap >> frame;
            }
            if (frame.empty()) break;
        }

        const bool want_capture = g_capture_on.load(std::memory_order_relaxed);
        const bool want_record = g_record_on.load(std::memory_order_relaxed);

        // 刷新全局帧供 Web 实时预览：只有点了「开始视频采集」才更新。
        // 关掉时立刻释放，否则重新打开会先闪一张很久以前的旧画面。
        {
            std::lock_guard<std::mutex> lock(frame_mutex);
            if (want_capture)
                frame.copyTo(global_current_frame);
            else if (!global_current_frame.empty())
                global_current_frame.release();
        }

        // 录制开关被关掉 → 当前这段立即收尾保存，不丢已经写进去的内容
        if (is_recording && !want_record) {
            if (ffmpeg_pipe) {
                cross_pclose(ffmpeg_pipe);
                ffmpeg_pipe = nullptr;
            }
            if (fallback_writer.isOpened()) fallback_writer.release();
            is_recording = false;
            g_is_recording.store(false);
            set_recording_file("");
            std::cout << TR("[系统] 录制已手动停止，当前片段已保存。",
                            "[System] Recording stopped manually; current clip saved.")
                      << std::endl;
        }

        // 刚点开「开始录制」：清掉上次的失败退避，清空背景模型重新学习
        if (want_record && !prev_want_record) {
            pMOG2->clear();
            bg_warmup_frames = 10;
            record_fail_count = 0;
            record_disabled = false;
            next_record_try = std::chrono::steady_clock::time_point{};
            last_motion_time = std::chrono::steady_clock::now();
            std::cout << TR("[系统] 已开启运动检测，画面出现异动时自动录像。",
                            "[System] Motion detection armed; recording starts on movement.")
                      << std::endl;
        }
        prev_want_record = want_record;

        // 录制没开启 → 不跑检测、不写文件，只保持摄像头出帧（避免缓冲堆积）
        if (!want_record) {
            pace();
            continue;
        }

        // 预热帧：只更新背景模型，不判定（否则第一帧必然全屏「运动」）
        if (bg_warmup_frames > 0) {
            pMOG2->apply(frame, fgMask);
            --bg_warmup_frames;
            pace();
            continue;
        }

        // 3. 运动检测核心逻辑
        pMOG2->apply(frame, fgMask);
        // 消除噪点（阈值二值化，同时过滤掉 MOG2 的阴影灰度值）
        cv::threshold(fgMask, fgMask, 200, 255, cv::THRESH_BINARY);

        // 计算前景（移动物体）的白像素点数量
        int non_zero_pixels = cv::countNonZero(fgMask);
        double motion_percentage =
            (static_cast<double>(non_zero_pixels) / static_cast<double>(width * height)) * 100.0;

        // 当移动物体面积超过画面的 1.5% 时，判定为有人或动物走动
        bool motion_detected = (motion_percentage > 1.5);

        auto now = std::chrono::steady_clock::now();
        if (motion_detected) {
            last_motion_time = now; // 更新最后一次检测到运动的时间
            if (!is_recording && !record_disabled && now >= next_record_try) {
                // 生成基于时间戳的文件名
                auto time_now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                char timestamp[64];
                std::tm tm_buf{};
#ifdef _WIN32
                localtime_s(&tm_buf, &time_now);
#else
                localtime_r(&time_now, &tm_buf);
#endif
                std::strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", &tm_buf);
                std::string video_path = "./videos/cam_" + std::string(timestamp) + ".mp4";

                // 4. 通过管道启动 FFmpeg 录制标准 H.264 MP4 视频
                // -f rawvideo 指定输入为原始字节流，-pix_fmt bgr24 对应 OpenCV 的 Mat 格式
                // -nostdin 防止 ffmpeg 抢占终端输入
                // ffmpeg 的报错不丢进 /dev/null，而是追加到 videos/ffmpeg.log，方便定位问题
#ifdef _WIN32
                // Windows 上 ffmpeg 的报错也留下来（以前是丢进 NUL，出问题完全看不到）
                const std::string redirect = " > NUL 2>> .\\videos\\ffmpeg.log";
#else
                const std::string redirect = " > /dev/null 2>> ./videos/ffmpeg.log";
#endif
#if defined(_WIN32)
                // 编码参数见下方按平台拼接处（Windows 固定 libx264 软编）
#else
                // Linux/Pi：优先硬件编码器（几乎不占 CPU）；失败一次就永久切回软编
#endif
                // 使用 use_wallclock_as_timestamps 让 FFmpeg 基于现实墙钟写入时间戳，彻底解决抖动和快进问题
                std::string ffmpeg_cmd = ffmpeg_prefix()
                                       + " -nostdin -loglevel warning -y "
                                       + "-use_wallclock_as_timestamps 1 " 
                                       + "-f rawvideo -vcodec rawvideo -pix_fmt bgr24 -s "
                                       + std::to_string(width) + "x" + std::to_string(height)
                                       + " -r " + std::to_string(static_cast<int>(rec_fps))
                                       + " -i - ";

                // 音频输入作为第 2 路输入（第 1 路是从管道读的原始视频帧）
                if (rec_audio) ffmpeg_cmd += audio.input_args + " ";

#if defined(_WIN32)
                ffmpeg_cmd += "-vcodec libx264 -pix_fmt yuv420p -preset ultrafast ";
#else
                // 树莓派专用的 h264_v4l2m2m 硬件编码参数优化，加入 num_capture_buffers 破除反压阻塞
                ffmpeg_cmd += use_hw_encoder
                    ? "-vcodec h264_v4l2m2m -num_capture_buffers 16 -pix_fmt yuv420p -b:v 1500k "
                    : "-vcodec libx264 -pix_fmt yuv420p -preset ultrafast ";
#endif

                if (rec_audio) {
                    // 只取视频轨和音频轨（dshow 设备可能还带别的流，不显式 map 可能报错）
                    // aresample=async=1 用重采样补偿「声卡时钟 vs 系统墙钟」的漂移，
                    // 否则长录时声音会慢慢跑偏；first_pts=0 让声音从 0 开始、和画面同起点。
                    ffmpeg_cmd += "-map 0:v -map 1:a -c:a aac -b:a 64k -ar 44100 -ac 1 "
                                  "-af aresample=async=1:first_pts=0 -shortest ";
                }

                ffmpeg_cmd += video_path + redirect;


                bool started = false;
                if (use_pipe) {
                    // POSIX 的 popen 只接受 "r"/"w"，"b" 是 Windows 专有的，按平台分开传
#ifdef _WIN32
                    const char* pipe_mode = "wb";
#else
                    const char* pipe_mode = "w";
#endif
                    ffmpeg_pipe = cross_popen(ffmpeg_cmd.c_str(), pipe_mode);
                    if (ffmpeg_pipe) {
                        started = true;
                        std::cout << TR("[警报] 检测到画面异动！启动自动录像: ",
                                        "[ALERT] Motion detected! Recording started: ")
                                  << video_path << (rec_audio ? TR("（含音频）", " (with audio)") : "")
                                  << std::endl;
                    } else {
                        // popen 返回 NULL 说明系统层面建不出子进程/管道，
                        // 和「ffmpeg 命令不存在」不是一回事：那种情况 popen 会成功，
                        // 失败要等 ffmpeg 自己退出后才能发现。这里把 errno 打出来定位。
                        const int err = errno;
                        std::cerr << TR("[错误] 启动 ffmpeg 失败 (errno=", "[ERROR] popen failed (errno=")
                                  << err << ": " << std::strerror(err) << ")" << std::endl;
                        std::cerr << TR("        命令: ", "        command: ") << ffmpeg_cmd
                                  << std::endl;
                    }
                }

                if (!started) {
                    // 保底：走 OpenCV 自带的 ffmpeg 后端直接写文件，不经过子进程和管道
                    fallback_writer.open(video_path, cv::VideoWriter::fourcc('m', 'p', '4', 'v'),
                                         rec_fps, cv::Size(width, height));
                    if (fallback_writer.isOpened()) {
                        started = true;
                        std::cerr << TR("[警告] 改用 OpenCV 内置编码器录制: ",
                                        "[WARN] Falling back to OpenCV's built-in encoder: ")
                                  << video_path
                                  << TR("（MPEG-4，浏览器可能无法直接播放，建议用 VLC 查看）",
                                        " (MPEG-4, may not play in browsers; use VLC)")
                                  << std::endl;
                    }
                }

                if (started) {
                    is_recording = true;
                    g_is_recording.store(true);
                    record_fail_count = 0;
                    rec_start_time = now;       // 墙钟补帧的计时零点
                    rec_frames_written = 0;
                    cur_rec_audio = use_pipe && rec_audio; // 只有走 ffmpeg 管道才可能带音频
                    // 登记「正在录制」，让 /api/list 跳过这个文件（moov 还没写出来）
                    set_recording_file(fs::path(video_path).filename().string());
                } else {
                    ++record_fail_count;
                    next_record_try = now + std::chrono::seconds(10);
                    std::cerr << TR("[错误] 录像启动失败，10 秒后重试。",
                                    "[ERROR] Failed to start recording, retrying in 10s.")
                              << std::endl;
                    if (record_fail_count >= 3) {
                        record_disabled = true;
                        std::cerr << TR("[错误] 已连续失败 3 次，录像功能停用"
                                        "（运动检测和网页预览仍然正常）。",
                                        "[ERROR] Failed 3 times in a row, recording disabled"
                                        " (motion detection and preview still work).")
                                  << std::endl;
                    }
                }
            }
        }

        // 5. 录像写入与停止逻辑
        if (is_recording) {
            // 如果超过设定时间没有物体移动，则停止录像
            if (std::chrono::duration_cast<std::chrono::seconds>(now - last_motion_time).count() >=
                record_extend_seconds) {
                is_recording = false;
                g_is_recording.store(false);
                if (ffmpeg_pipe) {
                    const int status = cross_pclose(ffmpeg_pipe);
                    ffmpeg_pipe = nullptr;
                    if (status != 0) {
                        // ffmpeg 起来了但自己退出失败，多半是编码器/设备/参数问题
                        std::cerr << TR("[警告] ffmpeg 异常退出 (状态码 ",
                                        "[WARN] ffmpeg exited abnormally (status ")
                                  << status
                                  << TR(")，本次录像不可用，原因见 ./videos/ffmpeg.log",
                                        "), this recording is unusable; see ./videos/ffmpeg.log")
                                  << std::endl;
                        // 优先怀疑音频：麦克风被占用/热拔/驱动异常都会让 ffmpeg 直接退出，
                        // 这时关掉音频重试，别把视频录制一起废掉
                        if (cur_rec_audio) {
                            rec_audio = false;
                            std::cerr << TR("[警告] 本次为带音频录制，怀疑麦克风不可用；"
                                            "后续改为只录视频，2 秒后重试。",
                                            "[WARN] This was an audio recording; the microphone is"
                                            " likely unavailable. Switching to video-only,"
                                            " retrying in 2s.")
                                      << std::endl;
                            next_record_try = now + std::chrono::seconds(2);
                        }
#ifndef _WIN32
                        else if (use_hw_encoder) {
                            // 硬件编码器初始化失败（板子不支持/ffmpeg 未编译该模块），
                            // 永久切回 libx264 软编，2 秒后立即重试而不是等 10 秒
                            use_hw_encoder = false;
                            std::cerr << TR("[警告] 已切换回 libx264 软件编码，稍后自动重试。",
                                            "[WARN] Switched back to libx264 software encoding,"
                                            " retrying shortly.")
                                      << std::endl;
                            next_record_try = now + std::chrono::seconds(2);
                        }
#endif
                        else {
                            ++record_fail_count;
                            next_record_try = now + std::chrono::seconds(10);
                        }
                    } else {
                        record_fail_count = 0;
                        std::cout << TR("[系统] 画面恢复静止，录像已压缩保存。",
                                        "[System] Scene is still again; recording saved.")
                                  << std::endl;
                    }
                }
                if (fallback_writer.isOpened()) {
                    fallback_writer.release();
                    std::cout << TR("[系统] 画面恢复静止，录像已保存（OpenCV 内置编码器）。",
                                    "[System] Scene is still again; recording saved"
                                    " (OpenCV built-in encoder).")
                              << std::endl;
                }
                set_recording_file(""); // 文件已写完，列表接口可以正常取缩略图了
            } else if (ffmpeg_pipe) {
                // 墙钟补帧：按真实时间给每个 1/rec_fps 的时隙喂一帧。
                // 采集快就按节拍写；采集慢（Pi 上编码反压）就重复当前帧补齐。
                // 这样 mp4 的时长严格等于真实时间、帧距完全均匀，不会快进/晃动。
                const double rec_elapsed =
                    std::chrono::duration<double>(now - rec_start_time).count();
                const long target_frames = static_cast<long>(rec_elapsed * rec_fps);
                const size_t frame_bytes = frame.total() * frame.elemSize();
                bool write_failed = false;
                while (rec_frames_written < target_frames) {
                    if (fwrite(frame.data, 1, frame_bytes, ffmpeg_pipe) != frame_bytes) {
                        // 管道写不进去，说明 ffmpeg 已经死了（SIGPIPE 已在 main 里忽略）
                        write_failed = true;
                        break;
                    }
                    ++rec_frames_written;
                }
                if (!write_failed && rec_frames_written > 0)
                    fflush(ffmpeg_pipe);

                if (write_failed) {
                    std::cerr << TR("[错误] 写入录像管道失败，ffmpeg 已退出，原因见 ./videos/ffmpeg.log",
                                    "[ERROR] Writing to the ffmpeg pipe failed; ffmpeg has exited."
                                    " See ./videos/ffmpeg.log")
                              << std::endl;
                    cross_pclose(ffmpeg_pipe);
                    ffmpeg_pipe = nullptr;
                    is_recording = false;
                    g_is_recording.store(false);
                    set_recording_file("");
                    if (cur_rec_audio) {
                        rec_audio = false;
                        std::cerr << TR("[警告] 本次为带音频录制，怀疑麦克风不可用；"
                                        "后续改为只录视频，2 秒后重试。",
                                        "[WARN] This was an audio recording; the microphone is"
                                        " likely unavailable. Switching to video-only,"
                                        " retrying in 2s.")
                                  << std::endl;
                        next_record_try = now + std::chrono::seconds(2);
                    }
#ifndef _WIN32
                    else if (use_hw_encoder) {
                        use_hw_encoder = false;
                        std::cerr << TR("[警告] 已切换回 libx264 软件编码，稍后自动重试。",
                                        "[WARN] Switched back to libx264 software encoding,"
                                        " retrying shortly.")
                                  << std::endl;
                        next_record_try = now + std::chrono::seconds(2);
                    }
#endif
                    else {
                        ++record_fail_count;
                        next_record_try = now + std::chrono::seconds(10);
                    }
                }
            } else if (fallback_writer.isOpened()) {
                fallback_writer.write(frame);
            }
        }

        // 按固定节拍采集，防止空转榨干 CPU
        pace();
    }

    if (ffmpeg_pipe) cross_pclose(ffmpeg_pipe);
    if (fallback_writer.isOpened()) fallback_writer.release();
    set_recording_file("");
    cap.release();
}

// ==================== 录像列表用的元信息（缩略图 / 时长 / 大小 / 时间） ====================
const char* const kVideoDir = "./videos";
const char* const kThumbDir = "./videos/.thumbs";

// 已探测过的时长缓存（key = 文件名）。0 表示探测失败，不再反复重试。
std::mutex meta_mutex;
std::unordered_map<std::string, double> duration_cache;

// 文件是否还在被写入（ffmpeg 录制中 mtime 会一直变），此时不宜去解码它
bool is_recently_modified(const fs::path& p, int seconds) {
    std::error_code ec;
    const auto t = fs::last_write_time(p, ec);
    if (ec) return false;
    return (fs::file_time_type::clock::now() - t) < std::chrono::seconds(seconds);
}

// 从 cam_20261107_000350.mp4 这样的文件名里取出录制开始时间
bool parse_record_start(const std::string& name, std::time_t& out) {
    const std::size_t pos = name.find("cam_");
    if (pos == std::string::npos || name.size() < pos + 19) return false;

    const std::string ts = name.substr(pos + 4, 15); // 20261107_000350
    std::tm tm_buf{};
    if (std::sscanf(ts.c_str(), "%4d%2d%2d_%2d%2d%2d", &tm_buf.tm_year, &tm_buf.tm_mon,
                    &tm_buf.tm_mday, &tm_buf.tm_hour, &tm_buf.tm_min, &tm_buf.tm_sec) != 6)
        return false;
    tm_buf.tm_year -= 1900;
    tm_buf.tm_mon -= 1;
    tm_buf.tm_isdst = -1;
    const std::time_t t = std::mktime(&tm_buf);
    if (t == static_cast<std::time_t>(-1)) return false;
    out = t;
    return true;
}

// 文件名不合规时退回文件修改时间
std::time_t file_mtime(const fs::path& p) {
    std::error_code ec;
    const auto ftime = fs::last_write_time(p, ec);
    if (ec) return 0;
    const auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
    return std::chrono::system_clock::to_time_t(sctp);
}

// 打开视频取一帧做缩略图（320 宽），顺便返回时长（秒）；失败返回 0
double make_thumbnail(const std::string& video_path, const std::string& thumb_path) {
    cv::VideoCapture cap(video_path);
    if (!cap.isOpened()) return 0.0;

    const double fps = cap.get(cv::CAP_PROP_FPS);
    const double frames = cap.get(cv::CAP_PROP_FRAME_COUNT);
    const double duration = (fps > 0 && frames > 0) ? frames / fps : 0.0;

    std::error_code ec;
    if (!fs::exists(thumb_path, ec)) {
        cv::Mat img;
        if (duration > 1.5)
            cap.set(cv::CAP_PROP_POS_MSEC, 1000.0); // 取第 1 秒，避开开头黑屏
        cap >> img;
        if (img.empty()) {
            cap.set(cv::CAP_PROP_POS_FRAMES, 0);
            cap >> img;
        }
        if (!img.empty()) {
            const double scale = 320.0 / static_cast<double>(img.cols);
            cv::Mat small;
            cv::resize(img, small, cv::Size(), scale, scale, cv::INTER_AREA);
            fs::create_directories(fs::path(thumb_path).parent_path(), ec);
            cv::imwrite(thumb_path, small, {cv::IMWRITE_JPEG_QUALITY, 80});
        }
    }
    return duration;
}

// ==================== 模块 3 & 4：Web 服务线程 ====================
// 把首页模板里的 {{XXX}} 占位符换成当前语言的文案
void replace_all(std::string& s, const std::string& from, const std::string& to) {
    if (from.empty()) return;
    std::size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
}

// Crow 的日志处理器：默认实现会把 asio 的每条错误直接打到终端。
// 而浏览器关页面、切换视频、取消图片请求时，socket 会被提前关闭，
// asio 立刻报 "buffer write error ... Writing stopped premature" —— 这是
// 客户端主动断开，不是服务端故障，一秒能刷几十行，纯噪音，这里过滤掉。
class QuietCrowLogHandler : public crow::ILogHandler {
public:
    void log(const std::string& message, crow::LogLevel level) override {
        if (message.find("buffer write error") != std::string::npos)
            return; // 客户端断开连接，正常现象
        if (message.find("Writing stopped premature") != std::string::npos)
            return;
        const char* tag = "INFO";
        switch (level) {
            case crow::LogLevel::Debug: tag = "DEBUG"; break;
            case crow::LogLevel::Info: tag = "INFO"; break;
            case crow::LogLevel::Warning: tag = "WARN"; break;
            case crow::LogLevel::Error: tag = "ERROR"; break;
            case crow::LogLevel::Critical: tag = "CRITICAL"; break;
            default: break;
        }
        std::cerr << "[CROW " << tag << "] " << message << std::endl;
    }
};

void web_server_thread(bool verbose) {
    crow::SimpleApp app;

    // Crow 默认是 Info 级别，会把每个请求/响应都打出来；
    // /snapshot 每秒被拉 10 次，几秒就能刷满终端，所以默认只留 Warning 及以上。
    // 需要排查问题时可执行 ./video_monitor -v 打开完整日志。
    static QuietCrowLogHandler quiet_handler;
    if (!verbose)
        crow::logger::setHandler(&quiet_handler);
    app.loglevel(verbose ? crow::LogLevel::Info : crow::LogLevel::Warning);

    // 1. 首页：一个简单的 HTML 页面，包含实时图和点播列表
    CROW_ROUTE(app, "/")
    ([]() {
        std::string page = R"(
            <!DOCTYPE html>
            <html>
            <head>
                <meta charset="utf-8" />
                <title>{{TITLE}}</title>
                <style>
                    * { box-sizing: border-box; }
                    body { font-family: "Segoe UI", Arial, sans-serif; margin: 20px; background: #f0f2f5; color: #222; }
                    h1 { font-size: 20px; margin: 0 0 14px; }
                    h3 { font-size: 15px; margin: 0 0 10px; }
                    .toolbar { display: flex; gap: 10px; align-items: center; flex-wrap: wrap; margin-bottom: 16px; }
                    .btn { font-family: inherit; font-size: 14px; font-weight: 600; color: #fff; background: #2c7be5;
                           border: 0; border-radius: 6px; padding: 9px 18px; cursor: pointer; transition: filter .15s, background .15s; }
                    .btn:hover { filter: brightness(1.08); }
                    .btn:active { filter: brightness(.94); }
                    .btn.on { background: #4a5160; }
                    .btn.rec { background: #5a6270; }
                    .btn.rec.on { background: #d93b3b; }
                    .state { font-size: 13px; color: #8a8f98; display: inline-flex; align-items: center; gap: 6px; }
                    .state.live { color: #d93b3b; font-weight: 600; }
                    .dot { width: 8px; height: 8px; border-radius: 50%; background: currentColor; display: none; }
                    .state.live .dot { display: inline-block; animation: blink 1s infinite; }
                    @keyframes blink { 0%,100% { opacity: 1; } 50% { opacity: .25; } }
                    .container { display: flex; gap: 20px; flex-wrap: wrap; align-items: flex-start; }
                    .box { background: #fff; padding: 15px; border-radius: 8px; box-shadow: 0 2px 6px rgba(0,0,0,.08); }
                    .live-wrap { position: relative; width: 640px; max-width: 100%; }
                    #liveView { width: 100%; height: 480px; background: #000; border-radius: 6px; object-fit: contain; display: block; }
                    .live-hint { position: absolute; inset: 0; background: #0d1117; border-radius: 6px; color: #9aa4b2;
                                 font-size: 14px; line-height: 1.7; display: flex; align-items: center; justify-content: center;
                                 text-align: center; padding: 24px; }
                    #player { width: 480px; max-width: 100%; background: #000; border-radius: 6px; display: block; margin-bottom: 12px; }
                    #videoList { list-style: none; padding: 0; margin: 0; max-height: 420px; overflow-y: auto; }
                    .card { display: flex; gap: 10px; padding: 8px; margin-bottom: 8px; border-radius: 6px; background: #f7f8fa; cursor: pointer; transition: background .15s; }
                    .card:hover { background: #e8eef7; }
                    .thumb { width: 120px; height: 68px; flex: 0 0 120px; border-radius: 4px; background: #1b1b1b center/cover no-repeat; }
                    .info { min-width: 0; display: flex; flex-direction: column; justify-content: center; gap: 3px; }
                    .name { font-size: 13px; color: #333; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }
                    .meta { font-size: 12px; color: #8a8f98; }
                    .empty { color: #999; font-size: 13px; padding: 10px 2px; }
                </style>
            </head>
            <body>
                <h1>{{H1}}</h1>
                <div class="toolbar">
                    <button id="btnCapture" class="btn" type="button">{{BTN_CAP_ON}}</button>
                    <button id="btnRecord" class="btn rec" type="button">{{BTN_REC_ON}}</button>
                    <span id="recState" class="state"><i class="dot"></i><span id="recStateText"></span></span>
                </div>
                <div class="container">
                    <!-- 左侧：实时画面（服务端逐帧推 JPEG，前端定时刷新） -->
                    <div class="box">
                        <h3>{{LIVE}}</h3>
                        <div class="live-wrap">
                            <img id="liveView" alt="{{LIVEALT}}" />
                            <div id="liveHint" class="live-hint"><span>{{LIVEHINT}}</span></div>
                        </div>
                    </div>
                    <!-- 右侧：录像点播 -->
                    <div class="box">
                        <h3>{{REC}}</h3>
                        <video id="player" controls></video>
                        <ul id="videoList"><li class="empty">{{LOADING}}</li></ul>
                    </div>
                </div>

                <script>
                    // ---- 两个开关的状态（默认都关，和服务器保持一致）----
                    var capturing = false;    // 实时画面
                    var autoRecord = false;   // 运动检测录像
                    var recording = false;    // 当前是否真有一段在写

                    var liveImg = document.getElementById('liveView');
                    var liveHint = document.getElementById('liveHint');
                    var btnCap = document.getElementById('btnCapture');
                    var btnRec = document.getElementById('btnRecord');
                    var recState = document.getElementById('recState');
                    var recStateText = document.getElementById('recStateText');

                    var TXT = {
                        capOn: '{{CAP_ON}}', capOff: '{{CAP_OFF}}',
                        recOn: '{{REC_ON}}', recOff: '{{REC_OFF}}',
                        idle: '{{ST_IDLE}}', watch: '{{ST_WATCH}}', rec: '{{ST_REC}}'
                    };

                    function render() {
                        btnCap.textContent = capturing ? TXT.capOff : TXT.capOn;
                        btnCap.classList.toggle('on', capturing);
                        liveHint.style.display = capturing ? 'none' : 'flex';

                        btnRec.textContent = autoRecord ? TXT.recOff : TXT.recOn;
                        btnRec.classList.toggle('on', autoRecord);

                        recState.classList.toggle('live', recording);
                        recStateText.textContent = autoRecord ? (recording ? TXT.rec : TXT.watch) : TXT.idle;
                    }

                    // ---- 实时画面：串行拉取，上一张加载完（或失败）再发下一张。
                    //      不并发是为了不把请求堆在树莓派线程池里，也避免浏览器取消
                    //      未完成请求时服务端刷 "Writing stopped premature"。 ----
                    function pullSnapshot() {
                        if (!capturing) return;            // 关了就不再发任何请求
                        var next = new Image();
                        next.onload = function () {
                            if (!capturing) return;
                            liveImg.src = next.src;
                            setTimeout(pullSnapshot, 80);
                        };
                        next.onerror = function () {
                            if (!capturing) return;
                            setTimeout(pullSnapshot, 700); // 刚开启时可能还没帧，慢点重试
                        };
                        next.src = '/snapshot?t=' + Date.now();
                    }

                    function setCapture(on) {
                        if (capturing === on) return;
                        capturing = on;
                        if (!on) liveImg.removeAttribute('src');  // 关掉就清掉旧画面
                        render();
                        if (on) pullSnapshot();
                    }

                    var lastAction = 0;   // 点过按钮后短暂忽略轮询结果，免得按钮被翻回去

                    // ---- 两个按钮：先更新界面，再通知服务端 ----
                    btnCap.onclick = function () {
                        var target = !capturing;
                        lastAction = Date.now();
                        setCapture(target);
                        fetch('/api/capture?on=' + (target ? 1 : 0)).catch(function () {});
                    };

                    btnRec.onclick = function () {
                        var target = !autoRecord;
                        lastAction = Date.now();
                        autoRecord = target;
                        if (!target) recording = false;
                        render();
                        fetch('/api/record?on=' + (target ? 1 : 0)).catch(function () {});
                    };

                    // ---- 定时同步服务端状态：刷新/多开页面后状态一致，
                    //      也能反映「有异动自动开录、静止自动停录」----
                    function syncStatus() {
                        if (Date.now() - lastAction < 1500) return;
                        fetch('/api/status').then(function (r) { return r.json(); }).then(function (s) {
                            setCapture(!!s.capture);
                            autoRecord = !!s.auto_record;
                            recording = !!s.recording;
                            render();
                        }).catch(function () {});
                    }

                    render();
                    syncStatus();
                    setInterval(syncStatus, 2000);

                    // ---- 格式化工具 ----
                    function pad2(n) { return (n < 10 ? '0' : '') + n; }

                    function fmtSize(bytes) {
                        if (!bytes) return '0 B';
                        if (bytes >= 1048576) return (bytes / 1048576).toFixed(1) + ' MB';
                        if (bytes >= 1024) return (bytes / 1024).toFixed(1) + ' KB';
                        return bytes + ' B';
                    }

                    function fmtDuration(sec) {
                        if (!sec || sec <= 0) return '--:--';
                        var s = Math.round(sec);
                        var m = Math.floor(s / 60);
                        return m + ':' + pad2(s % 60);
                    }

                    function fmtTime(epoch) {
                        if (!epoch) return '{{UNKNOWN}}';
                        var d = new Date(epoch * 1000);
                        return d.getFullYear() + '-' + pad2(d.getMonth() + 1) + '-' + pad2(d.getDate()) +
                               ' ' + pad2(d.getHours()) + ':' + pad2(d.getMinutes()) + ':' + pad2(d.getSeconds());
                    }

                    // ---- 录像列表：缩略图 + 文件名 + 大小/时长 + 录制时间 ----
                    var lastSig = '';

                    function renderList(list) {
                        var ul = document.getElementById('videoList');
                        ul.innerHTML = '';

                        if (!list.length) {
                            var empty = document.createElement('li');
                            empty.className = 'empty';
                            empty.textContent = '{{NOVIDEO}}';
                            ul.appendChild(empty);
                            return;
                        }

                        list.forEach(function (v) {
                            var li = document.createElement('li');
                            li.className = 'card';

                            var thumb = document.createElement('div');
                            thumb.className = 'thumb';
                            if (v.thumb) {
                                var thumbUrl = '/thumb/' + encodeURIComponent(v.name + '.jpg');
                                thumb.style.backgroundImage = 'url(' + thumbUrl + ')';
                            }

                            var info = document.createElement('div');
                            info.className = 'info';

                            var nameEl = document.createElement('div');
                            nameEl.className = 'name';
                            nameEl.textContent = v.name;

                            var meta1 = document.createElement('div');
                            meta1.className = 'meta';
                            meta1.textContent = '{{SIZE}}' + fmtSize(v.size) + '{{SEP}}' + fmtDuration(v.duration);

                            var meta2 = document.createElement('div');
                            meta2.className = 'meta';
                            meta2.textContent = '{{RECORDED}}' + fmtTime(v.time);

                            info.appendChild(nameEl);
                            info.appendChild(meta1);
                            info.appendChild(meta2);
                            li.appendChild(thumb);
                            li.appendChild(info);

                            li.onclick = function () {
                                var player = document.getElementById('player');
                                player.src = '/videos/' + encodeURIComponent(v.name);
                                var p = player.play();
                                if (p && p.catch) p.catch(function () {});
                            };
                            ul.appendChild(li);
                        });
                    }

                    function loadVideos() {
                        fetch('/api/list').then(function (r) { return r.json(); }).then(function (data) {
                            if (!Array.isArray(data)) return;   // 异常响应直接忽略
                            var sig = JSON.stringify(data);
                            if (sig === lastSig) return;        // 内容没变就不重绘，避免闪烁
                            lastSig = sig;
                            renderList(data);
                        }).catch(function () {});
                    }

                    loadVideos();
                    setInterval(loadVideos, 5000);   // 每 5 秒刷新一次列表
                </script>
            </body>
            </html>
        )";

        // 界面文案跟控制台用同一套语言设置：英文系统上直接是英文，
        // 不依赖系统里有没有中文字体（英文版 Windows 常常没装）
        replace_all(page, "{{TITLE}}", TR("C++ 智能监控系统", "C++ Video Monitor"));
        replace_all(page, "{{H1}}", TR("🎥 边缘端 C++ 视频监控系统", "Edge C++ Video Monitoring"));
        replace_all(page, "{{LIVE}}", TR("🔴 实时画面预览", "Live view"));
        // 两个开关按钮 + 状态文字
        replace_all(page, "{{BTN_CAP_ON}}", TR("▶ 开始视频采集", "▶ Start live view"));
        replace_all(page, "{{BTN_REC_ON}}", TR("● 开始录制", "● Start recording"));
        replace_all(page, "{{CAP_ON}}", TR("▶ 开始视频采集", "▶ Start live view"));
        replace_all(page, "{{CAP_OFF}}", TR("■ 停止视频采集", "■ Stop live view"));
        replace_all(page, "{{REC_ON}}", TR("● 开始录制", "● Start recording"));
        replace_all(page, "{{REC_OFF}}", TR("■ 停止录制", "■ Stop recording"));
        replace_all(page, "{{ST_IDLE}}", TR("自动录像未开启", "Auto recording is off"));
        replace_all(page, "{{ST_WATCH}}", TR("运动检测中，有异动自动录像", "Watching for motion"));
        replace_all(page, "{{ST_REC}}", TR("录制中…", "Recording..."));
        replace_all(page, "{{LIVEHINT}}", TR("点击上方「开始视频采集」查看实时画面",
                                             "Click 'Start live view' above to see the camera"));
        replace_all(page, "{{LIVEALT}}", TR("实时画面", "live view"));
        replace_all(page, "{{REC}}", TR("📂 历史异动录像", "Motion recordings"));
        replace_all(page, "{{LOADING}}", TR("加载中...", "Loading..."));
        replace_all(page, "{{NOVIDEO}}", TR("暂无录像", "No recordings yet"));
        replace_all(page, "{{UNKNOWN}}", TR("未知", "unknown"));
        replace_all(page, "{{SIZE}}", TR("大小 ", "Size "));
        replace_all(page, "{{SEP}}", TR(" ｜ 时长 ", "  |  Length "));
        replace_all(page, "{{RECORDED}}", TR("录制于 ", "Recorded "));

        crow::response res(page);
        res.set_header("Content-Type", "text/html; charset=utf-8");
        return res;
    });

    // 2. 网页开关接口
    //    /api/status            查询两个开关 + 是否正在录制（网页定时轮询）
    //    /api/capture?on=1|0    实时画面开关（不带 on 就是切换）
    //    /api/record?on=1|0     运动检测录像开关（不带 on 就是切换）
    CROW_ROUTE(app, "/api/status")
    ([]() {
        std::string body = "{\"capture\":";
        body += g_capture_on.load() ? "true" : "false";
        body += ",\"auto_record\":";
        body += g_record_on.load() ? "true" : "false";
        body += ",\"recording\":";
        body += g_is_recording.load() ? "true" : "false";
        body += "}";
        crow::response res(body);
        res.set_header("Content-Type", "application/json; charset=utf-8");
        res.set_header("Cache-Control", "no-store");
        return res;
    });

    CROW_ROUTE(app, "/api/capture")
    ([](const crow::request& req) {
        bool on = !g_capture_on.load(); // 不带参数时当作切换
        if (const char* v = req.url_params.get("on")) on = (std::string(v) != "0");
        g_capture_on.store(on);
        std::cout << TR("[网页] 实时画面: ", "[Web] Live view: ")
                  << (on ? TR("开启", "on") : TR("关闭", "off")) << std::endl;
        crow::response res(200, on ? "{\"capture\":true}" : "{\"capture\":false}");
        res.set_header("Content-Type", "application/json; charset=utf-8");
        res.set_header("Cache-Control", "no-store");
        return res;
    });

    CROW_ROUTE(app, "/api/record")
    ([](const crow::request& req) {
        bool on = !g_record_on.load();
        if (const char* v = req.url_params.get("on")) on = (std::string(v) != "0");
        g_record_on.store(on);
        std::cout << TR("[网页] 运动检测录像: ", "[Web] Motion recording: ")
                  << (on ? TR("开启", "on") : TR("关闭", "off")) << std::endl;
        crow::response res(200, on ? "{\"auto_record\":true}" : "{\"auto_record\":false}");
        res.set_header("Content-Type", "application/json; charset=utf-8");
        res.set_header("Cache-Control", "no-store");
        return res;
    });

    // 3. 实时画面接口：返回当前帧的 JPEG 快照
    //    注意：当前版本的 Crow 的 response::write() 只是往 body 里追加字符串，
    //    并不会真正流式发送，所以不能用 MJPEG 那种「一次请求持续推送」的写法。
    CROW_ROUTE(app, "/snapshot")
    ([](const crow::request& /*req*/, crow::response& res) {
        // 没点「开始视频采集」就别给画面：网页关掉后不该还在偷偷拉流
        if (!g_capture_on.load()) {
            res.code = 503;
            res.body = "live view is off";
            res.set_header("Content-Type", "text/plain");
            res.set_header("Cache-Control", "no-store");
            res.end();
            return;
        }

        std::vector<uchar> buf;
        {
            std::lock_guard<std::mutex> lock(frame_mutex);
            if (!global_current_frame.empty()) {
                const std::vector<int> params{cv::IMWRITE_JPEG_QUALITY, 70};
                cv::imencode(".jpg", global_current_frame, buf, params);
            }
        }

        if (buf.empty()) {
            res.code = 503;                       // 摄像头还没出第一帧
            res.body = "camera not ready";
            res.set_header("Content-Type", "text/plain");
        } else {
            res.code = 200;
            res.body.assign(buf.begin(), buf.end());
            res.set_header("Content-Type", "image/jpeg");
        }
        res.set_header("Cache-Control", "no-store");
        res.end();
    });

    // 3. 历史录像点播静态分发接口 (利用 C++17 <filesystem>)
    CROW_ROUTE(app, "/videos/<string>")
    ([](const crow::request& /*req*/, crow::response& res, std::string filename) {
        // 只允许取 videos 目录下的裸文件名，阻断 ../ 之类的目录穿越
        if (filename.find('/') != std::string::npos || filename.find('\\') != std::string::npos ||
            filename.find("..") != std::string::npos) {
            res.code = 400;
            res.end();
            return;
        }
        std::string file_path = "./videos/" + filename;
        if (fs::exists(file_path) && fs::is_regular_file(file_path)) {
            res.set_static_file_info(file_path, "video/mp4");
        } else {
            res.code = 404;
        }
        res.end();
    });

    // 4. 缩略图分发（只认 .thumbs 目录下的裸文件名）
    CROW_ROUTE(app, "/thumb/<string>")
    ([](const crow::request& /*req*/, crow::response& res, std::string filename) {
        if (filename.find('/') != std::string::npos || filename.find('\\') != std::string::npos ||
            filename.find("..") != std::string::npos) {
            res.code = 400;
            res.end();
            return;
        }
        const std::string path = std::string(kThumbDir) + "/" + filename;
        if (fs::exists(path) && fs::is_regular_file(path)) {
            res.set_static_file_info(path, "image/jpeg");
        } else {
            res.code = 404;
        }
        res.end();
    });

    // 5. API 获取录像列表：文件名 / 大小 / 录制时间 / 时长 / 缩略图是否就绪
    CROW_ROUTE(app, "/api/list")
    ([]() {
        std::vector<fs::directory_entry> entries;
        std::error_code ec;
        if (fs::exists(kVideoDir, ec)) {
            for (const auto& entry : fs::directory_iterator(kVideoDir, ec)) {
                if (entry.path().extension() == ".mp4") entries.push_back(entry);
            }
        }
        // 文件名本身就是时间戳，倒序即最新的排最前
        std::sort(entries.begin(), entries.end(),
                  [](const fs::directory_entry& a, const fs::directory_entry& b) {
                      return a.path().filename().string() > b.path().filename().string();
                  });

        crow::json::wvalue::list json_list;
        json_list.reserve(entries.size());

        for (const auto& entry : entries) {
            const std::string name = entry.path().filename().string();
            const std::string thumb_path = std::string(kThumbDir) + "/" + name + ".jpg";

            // 时长只在第一次遇到这个文件时探测，结果缓存起来
            double duration = 0.0;
            bool probed = false;
            {
                std::lock_guard<std::mutex> lk(meta_mutex);
                const auto it = duration_cache.find(name);
                if (it != duration_cache.end()) {
                    duration = it->second;
                    probed = true;
                }
            }
            // 正在录制的文件先不碰：mp4 的 moov 原子在文件尾部，录制中打开必然报
            // "moov atom not found"。用采集线程登记的标记判断，比 mtime 可靠——
            // ffmpeg 刚起步的几秒还在内存缓冲，文件根本没被写过。
            const bool recording_now = is_recording_file(name);
            if (!probed && !recording_now && !is_recently_modified(entry.path(), 3)) {
                duration = make_thumbnail(entry.path().string(), thumb_path);
                std::lock_guard<std::mutex> lk(meta_mutex);
                duration_cache[name] = duration;
            }

            std::time_t recorded = 0;
            if (!parse_record_start(name, recorded)) recorded = file_mtime(entry.path());

            std::error_code size_ec;
            const auto bytes = fs::file_size(entry.path(), size_ec);
            std::error_code thumb_ec;
            const bool has_thumb = fs::exists(thumb_path, thumb_ec);

            crow::json::wvalue item;
            item["name"] = name;
            item["size"] = static_cast<long long>(size_ec ? 0 : bytes);
            item["time"] = static_cast<long long>(recorded);
            item["duration"] = duration;
            item["thumb"] = has_thumb;
            json_list.emplace_back(std::move(item));
        }

        return crow::json::wvalue(json_list);
    });

    // 在本地端口启动 Web 服务器（默认 30000，可用 --port 或 VIDEO_MONITOR_PORT 改）
    app.port(g_web_port).multithreaded().run();
}

// ==================== 主函数 ====================
int main(int argc, char* argv[]) {
    // 中文版 Windows 的 cmd 默认是 GBK(936) 代码页，而程序输出的是 UTF-8，
    // 不转换就会满屏乱码。这一步必须在任何打印之前做。
    setup_console_encoding();

    // 输出语言必须在第一条日志之前定下来，优先级：
    //   命令行 --lang zh|en|auto  >  环境变量 VIDEO_MONITOR_LANG  >  编译期宏 OUTPUT_LANG
    // 什么都没指定时就保持 OUTPUT_LANG 的默认值（默认 1 = 中文）。
    std::string lang;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a.rfind("--lang=", 0) == 0)
            lang = a.substr(7);
        else if (a == "--lang" && i + 1 < argc)
            lang = argv[i + 1];
    }
    if (lang.empty()) {
        if (const char* e = std::getenv("VIDEO_MONITOR_LANG")) lang = e;
    }
    if (lang == "en" || lang == "EN" || lang == "english" || lang == "English")
        g_lang_en = true;
    else if (lang == "zh" || lang == "cn" || lang == "chinese" || lang == "Chinese")
        g_lang_en = false;
    else if (lang == "auto" || lang == "AUTO")
        g_lang_en = !system_prefers_chinese();
    // lang 为空或无法识别：沿用编译期 OUTPUT_LANG 决定的 g_lang_en 初值

    // 默认安静模式（只打印 Warning 及以上）；
    // 传 -v / --verbose 可恢复 Crow 的完整请求日志
    bool verbose = false;
    bool want_list = false;
    bool port_set_cli = false;
    std::string cam_arg;
    std::string source_arg;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        } else if (arg == "-l" || arg == "--list-cameras" || arg == "--list") {
            want_list = true;
        } else if (arg == "--lang") {
            if (i + 1 < argc) ++i; // 值已经在上面预扫描时取过，这里跳过
        } else if (arg == "-h" || arg == "--help") {
            std::cout << TR(
                "用法: video_monitor [选项]\n\n"
                "  -c, --camera <编号>    使用第几号摄像头（默认 0）\n"
                "                         编号见 --list-cameras 的输出\n"
                "  -s, --source <源>      直接指定视频源：摄像头编号 / 本地视频文件 / rtsp 地址\n"
                "  -l, --list-cameras     列出本机所有摄像头（编号 + 名称 + 分辨率）\n"
                "  -p, --port <端口>      Web 服务端口（默认 30000）\n"
                "      --lang zh|en|auto  日志语言（默认取编译期 OUTPUT_LANG）\n"
                "  -v, --verbose          打印 HTTP 请求等详细日志\n"
                "  -h, --help             显示本帮助\n\n"
                "示例:\n"
                "  video_monitor.exe -l              看有哪些摄像头\n"
                "  video_monitor.exe -c 1            用第 1 号（通常是 USB 摄像头）\n"
                "  video_monitor.exe -s D:\\samples\\test.mp4  用视频文件测试\n",
                "Usage: video_monitor [options]\n\n"
                "  -c, --camera <n>       Use camera number n (default 0)\n"
                "                         numbers are shown by --list-cameras\n"
                "  -s, --source <src>     Video source: camera number / local file / rtsp url\n"
                "  -l, --list-cameras     List all cameras (number + name + resolution)\n"
                "  -p, --port <port>      Web server port (default 30000)\n"
                "      --lang zh|en|auto  Log language (default: compile-time OUTPUT_LANG)\n"
                "  -v, --verbose          Print HTTP request logs\n"
                "  -h, --help             Show this help\n\n"
                "Examples:\n"
                "  video_monitor.exe -l              list cameras\n"
                "  video_monitor.exe -c 1            use camera 1 (usually the USB one)\n"
                "  video_monitor.exe -s D:\\samples\\test.mp4  test with a video file\n")
                      << std::endl;
            return 0;
        } else if (arg == "-c" || arg == "--camera" || arg == "-s" || arg == "--source") {
            if (i + 1 >= argc) {
                std::cerr << TR("[错误] ", "[ERROR] ") << arg
                          << TR(" 后面要跟一个值", " requires a value") << std::endl;
                return 2;
            }
            if (arg == "-c" || arg == "--camera")
                cam_arg = argv[++i];
            else
                source_arg = argv[++i];
        } else if (arg.rfind("--camera=", 0) == 0) {
            cam_arg = arg.substr(9);
        } else if (arg.rfind("--source=", 0) == 0) {
            source_arg = arg.substr(9);
        } else if (arg == "-p" || arg == "--port") {
            if (i + 1 >= argc) {
                std::cerr << TR("[错误] ", "[ERROR] ") << arg
                          << TR(" 后面要跟一个端口号", " requires a port number") << std::endl;
                return 2;
            }
            g_web_port = std::atoi(argv[++i]);
            port_set_cli = true;
        } else if (arg.rfind("--port=", 0) == 0) {
            g_web_port = std::atoi(arg.c_str() + 7);
            port_set_cli = true;
        } else if (arg.rfind("--lang=", 0) == 0) {
            // 同上，已处理
        } else {
            std::cerr << TR("[警告] 不认识的参数: ", "[WARN] Unknown argument: ") << arg
                      << TR("（用 --help 看用法）", " (see --help)") << std::endl;
        }
    }

    // 命令行优先级最高：--source 直接当视频源；--camera 转成序号
    if (!source_arg.empty())
        g_source_override = source_arg;
    else if (!cam_arg.empty())
        g_source_override = cam_arg;

    if (g_web_port <= 0 || g_web_port > 65535) g_web_port = 30000;
    if (!port_set_cli) {
        if (const char* s = std::getenv("VIDEO_MONITOR_PORT")) {
            const int p = std::atoi(s);
            if (p > 0 && p <= 65535) g_web_port = p;
        }
    }

    // --list-cameras 只是查询，列完就退出，不启动服务
    if (want_list) return list_cameras();

    std::cout << TR("[系统] 正在启动智能监控软件...", "[System] Starting video monitor...")
              << std::endl;
    std::cout << TR("[系统] Web 访问地址: http://<本机IP>:", "[System] Web UI: http://<this-pc-ip>:")
              << g_web_port << std::endl;
    if (!verbose)
        std::cout << TR("[系统] 日志已静音（不打印 HTTP 请求），需要时用 ./video_monitor -v 启动",
                       "[System] Request logs muted; run with -v to see them.")
                  << std::endl;

    // 非 verbose 模式下把 OpenCV 自带的日志压到 ERROR：
    // 后端尝试失败回退、GStreamer/ffmpeg 的 WARN 都在这一层，属于噪音；
    // 我们自己的诊断信息全部走 std::cout/std::cerr，不受影响。
    if (!verbose)
        cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_ERROR);

#ifndef _WIN32
    // ffmpeg 挂掉后继续往管道写会收到 SIGPIPE，默认动作是直接杀掉整个进程，
    // 这里忽略掉，让 fwrite 返回短写、由录像逻辑自己处理
    std::signal(SIGPIPE, SIG_IGN);
#endif

    // 启动双线程
    std::thread monitor(video_monitor_thread);
    std::thread server(web_server_thread, verbose);

    monitor.join();
    server.join();

    return 0;
}
