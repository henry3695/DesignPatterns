// Config.h - config.json 配置加载
#pragma once
#include <string>
#include "Net.h"
#include "OkxApi.h"
#include "Strategy.h"
#include "Risk.h"

struct AppConfig {
    // exchange
    OkxCreds creds;
    bool demo = true;
    std::string instId = "BTC-USDT-SWAP";
    std::string tdMode = "cross";
    int leverage = 5;

    // proxy（不设置则直连）
    net::ProxyConfig proxy;

    // ssl
    bool verifySsl = true;
    std::string caFile;

    // strategy
    int fastTicks = 60;
    int slowTicks = 600;
    double orderSize = 1;
    bool tradeEnabled = false;   // 默认只看信号不下单！
    bool allowShort = true;
    double takerFeeRate = 0.0005; // taker 手续费率（OKX 合约 0.05%）
    double minEdgeBps = 15;       // 交叉信号最小优势（基点），须明显大于 2*taker 费
    int confirmTicks = 20;        // 候选信号需连续确认的报价笔数
    int cooldownSec = 60;         // 两次下单最小间隔秒数（防连环开平）

    // risk
    double maxOrderSize = 2;
    double maxPosition = 10;
    double maxDrawdownPct = 10.0;

    // log
    std::string logFile = "okxquant.log";

    // 加载失败返回 false 并填 err
    bool load(const std::string& path, std::string& err);
};

// 单例
AppConfig& cfg();
