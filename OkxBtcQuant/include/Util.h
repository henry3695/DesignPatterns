// Util.h - 日志 / 加密签名 / 时间 / 字符串工具
#pragma once
#include <string>
#include <cstdint>

// ---------- 日志 ----------
// 控制台初始化：Windows 下切换代码页到 UTF-8 并开启 ANSI 颜色。
// 必须在任何输出（含 LOG_*）之前调用，否则中文会乱码。
void consoleInit();

// 可执行文件所在目录（带结尾斜杠），用于双击运行时定位 config.json
std::string exeDir();

// 控制台诊断信息（代码页 / 字体），用于确认中文显示环境
std::string consoleDiag();

// true = 系统未安装任何中文字体（英文系统常见），控制台无法正常显示中文
bool noCjkFont();

void logSetFile(const std::string& path);
void logWrite(const char* level, const char* fmt, ...);

#define LOG_I(...) logWrite("INFO ", __VA_ARGS__)
#define LOG_W(...) logWrite("WARN ", __VA_ARGS__)
#define LOG_E(...) logWrite("ERROR", __VA_ARGS__)
#ifdef _DEBUG
#define LOG_D(...) logWrite("DEBUG", __VA_ARGS__)
#else
#define LOG_D(...) ((void)0)
#endif

// ---------- 加密 ----------
// Base64(HMAC-SHA256(key, msg)) —— OKX 签名用
std::string hmacSha256Base64(const std::string& key, const std::string& msg);
std::string base64Encode(const unsigned char* data, size_t len);

// ---------- 时间 ----------
int64_t  nowMs();                 // 当前 Unix 毫秒
std::string isoTimestamp();       // 2026-01-01T00:00:00.123Z（OKX 签名头用）

// ---------- 字符串 ----------
std::string trim(const std::string& s);
std::string toLower(std::string s);
// 双转字符串（下单价格/数量用），去掉多余的尾零
std::string fmtDouble(double v, int maxDecimals = 8);
