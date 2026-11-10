// Lang.h - 界面语言切换
// 编译时定义 UI_ENGLISH（如 cl /DUI_ENGLISH 或 CMake -DUI_ENGLISH=ON）
// 则所有用户可见的日志/提示输出英文（适配海外 VPS / 英文系统）；
// 不定义则保持中文。
#pragma once

#ifdef UI_ENGLISH
#define TR(zh, en) en
#else
#define TR(zh, en) zh
#endif
