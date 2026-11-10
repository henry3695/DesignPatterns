// main.cpp - OKX BTC 合约量化交易（模拟盘）
// 用法: okxquant [config.json]
#include <cstdio>
#include <cstring>
#include <cmath>
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>

#include "Config.h"
#include "Util.h"
#include "Lang.h"
#include "Net.h"
#include "OkxApi.h"
#include "Strategy.h"
#include "Risk.h"

// ---------------- 全局状态 ----------------
namespace {

struct State {
    std::atomic<double> lastPrice{0};
    std::atomic<int64_t> lastPriceMs{0};
    std::atomic<bool>   running{true};

    std::mutex posMtx;
    double pos = 0;        // 净持仓张数（多正空负）
    double upl = 0;        // 未实现盈亏

    // 交易统计（手续费感知）
    std::atomic<long long> feeTotalCents{0}; // 累计已付手续费（单位: 0.01 USDT）
    std::atomic<int>     orderCount{0};    // 累计成交笔数
    std::atomic<int64_t> lastTradeMs{0};   // 上一笔成交时间（冷却用）

    EmaStrategy strat;
    RiskManager risk;
    OkxRest rest;

    void setPos(double p, double u) { std::lock_guard<std::mutex> lk(posMtx); pos = p; upl = u; }
    void getPos(double& p, double& u) { std::lock_guard<std::mutex> lk(posMtx); p = pos; u = upl; }
};

State g;

// 从 REST 刷新本地持仓缓存
void refreshPosition() {
    if (!g.rest.creds.valid()) return;
    std::string err;
    minij::Json r = g.rest.getPositions(err);
    if (r.isNull()) { LOG_W(TR("刷新持仓失败: %s", "Failed to refresh position: %s"), err.c_str()); return; }
    const minij::Json& d = r.at("data").idx(0);
    if (!d.isObj()) return;
    double pos = d.at("pos").asNum(0);
    double upl = d.at("upl").asNum(0);
    g.setPos(pos, upl);
    LOG_I(TR("持仓同步: %s 张, 未实现盈亏 %.2f USDT",
             "Position synced: %s contracts, uPnL %.2f USDT"),
          fmtDouble(pos, 1).c_str(), upl);
}

// 按目标方向调仓（含风控校验、冷却与手续费统计）
// manual=true 时跳过冷却限制（用于控制台手动 open/close）
void executeTarget(int target, bool manual = false) {
    if (!g.rest.creds.valid()) {
        LOG_I(TR("[模拟] 目标仓位 %d（未配置 API Key，仅演示信号）",
                 "[demo] target %d (no API key configured, signal only)"), target);
        return;
    }
    if (!g.strat.tradeEnabled) {
        LOG_I(TR("[dry-run] 目标仓位 %d（trade_enabled=false，不下单）",
                 "[dry-run] target %d (trade_enabled=false, no order)"), target);
        return;
    }

    // 交易冷却：两次成交间隔过短直接跳过（防连环开平被手续费磨掉）
    int64_t now = nowMs();
    int64_t last = g.lastTradeMs.load();
    if (!manual && last > 0 && now - last < (int64_t)cfg().cooldownSec * 1000) {
        LOG_I(TR("冷却中: 距上次成交 %lld 秒 < %d 秒，本次信号暂不执行",
                 "Cooldown: %lld s since last fill < %d s, signal skipped"),
              (long long)((now - last) / 1000), cfg().cooldownSec);
        return;
    }

    double pos, upl;
    g.getPos(pos, upl);
    double desired = (double)target * g.strat.orderSz;
    double delta = desired - pos;

    std::string why;
    if (!g.risk.checkTrade(pos, desired, why)) {
        LOG_W(TR("风控拦截: %s", "Risk check rejected: %s"), why.c_str());
        return;
    }
    if (g.risk.drawdownBreached(why)) {
        LOG_E(TR("风控熔断: %s", "Risk halt: %s"), why.c_str());
        return;
    }

    std::string side = delta > 0 ? "buy" : "sell";
    double sz = std::fabs(delta);
    std::string err;
    double px = g.lastPrice.load();
    // 本次成交的 taker 手续费预估（BTC-USDT-SWAP 1张=0.01 BTC）
    double feeEst = sz * 0.01 * px * cfg().takerFeeRate;
    LOG_I(TR("下单: %s %g 张 (市价, 现价 %.1f) 手续费预估 %.2f USDT",
             "Order: %s %g contracts (market, px %.1f) est fee %.2f USDT"),
          side.c_str(), sz, px, feeEst);
    minij::Json r = g.rest.placeMarketOrder(side, sz, err);
    if (r.isNull() || !OkxRest::ok(r)) {
        LOG_E(TR("下单失败: %s", "Order failed: %s"),
              r.isNull() ? err.c_str() : OkxRest::firstErr(r).c_str());
    } else {
        const minij::Json& d = r.at("data").idx(0);
        LOG_I(TR("下单成功 ordId=%s", "Order placed ordId=%s"), d.at("ordId").asStr().c_str());
        g.feeTotalCents.fetch_add((long long)std::llround(feeEst * 100));
        g.orderCount.fetch_add(1);
        g.lastTradeMs.store(nowMs());
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        refreshPosition();
    }
}

// ---------------- 事件处理 ----------------
void onPublicEvent(const minij::Json& j) {
    const minij::Json& d = j.at("data").idx(0);
    if (!d.isObj()) return;
    const std::string ch = j.at("arg").at("channel").asStr();

    if (ch == "tickers") {
        double px = d.at("last").asNum(0);
        int64_t ts = d.at("ts").asInt(0);
        if (px <= 0) return;
        g.lastPrice.store(px);
        g.lastPriceMs.store(ts);
        int target = g.strat.onPrice(px);
        static int lastTarget = 999;
        if (target != lastTarget) {
            lastTarget = target;
            executeTarget(target);
        }
    }
}

void onPrivateEvent(const minij::Json& j) {
    const minij::Json& d = j.at("data").idx(0);
    if (!d.isObj()) return;
    const std::string ch = j.at("arg").at("channel").asStr();

    if (ch == "orders") {
        const std::string state = d.at("state").asStr();
        const std::string side = d.at("side").asStr();
        double sz = d.at("sz").asNum(0);
        double acc = d.at("accFillSz").asNum(0);
        double px = d.at("avgPx").asNum(0);
        LOG_I(TR("[订单] %s %s %g/%g 张 %s 均价=%.1f ordId=%s",
                 "[order] %s %s %g/%g contracts %s avgPx=%.1f ordId=%s"),
              side.c_str(), state.c_str(), acc, sz,
              px > 0 ? "" : "", px, d.at("ordId").asStr().c_str());
        if (state == "filled" || state == "partially_filled" || state == "canceled") {
            refreshPosition();
        }
    } else if (ch == "account") {
        double eq = d.at("totalEq").asNum(0);
        if (eq > 0) {
            g.risk.updateEquity(eq);
            std::string why;
            if (g.risk.drawdownBreached(why)) {
                LOG_E(TR("风控熔断: %s", "Risk halt: %s"), why.c_str());
            }
        }
    }
}

void printHelp() {
    printf(TR(
        "命令:\n"
        "  status   显示状态(价格/EMA/持仓/权益)\n"
        "  pos      刷新并显示持仓\n"
        "  bal      查询账户余额\n"
        "  open     手动开多 1 份 order_size\n"
        "  close    手动平掉全部持仓\n"
        "  halt     风控熔断(暂停自动交易)\n"
        "  resume   解除熔断\n"
        "  help     帮助\n"
        "  quit     退出\n",
        "Commands:\n"
        "  status   show status (price/EMA/position/equity)\n"
        "  pos      refresh and show position\n"
        "  bal      query account balance\n"
        "  open     manually open long (1 x order_size)\n"
        "  close    manually close all positions\n"
        "  halt     risk halt (pause auto trading)\n"
        "  resume   resume auto trading\n"
        "  help     help\n"
        "  quit     quit\n"));
}

// 自动寻找 CA 证书包（Windows 上 OpenSSL 没有默认系统证书库，必须显式给）
std::string resolveCaFile() {
    if (!cfg().caFile.empty()) return cfg().caFile;
    const std::string dir = exeDir();
    const char* cand[] = {
        "cacert.pem",
        "third_party/ca/cacert.pem",
    };
    for (const char* c : cand) {
        FILE* f = fopen(c, "rb");
        if (f) { fclose(f); return c; }
    }
    const std::string rel[] = {
        dir + "cacert.pem",
        dir + "..\\third_party\\ca\\cacert.pem",
        dir + "../third_party/ca/cacert.pem",
        dir + "..\\..\\third_party\\ca\\cacert.pem",
        dir + "../../third_party/ca/cacert.pem",
    };
    for (const auto& p : rel) {
        FILE* f = fopen(p.c_str(), "rb");
        if (f) { fclose(f); return p; }
    }
    return "";
}

void printStatus() {    double pos, upl;
    g.getPos(pos, upl);
    printf(TR("价格: %.1f | EMA fast=%.1f slow=%.1f | 策略目标: %d\n",
              "Price: %.1f | EMA fast=%.1f slow=%.1f | target: %d\n"),
           g.lastPrice.load(), g.strat.emaFast(), g.strat.emaSlow(), g.strat.target());
    printf(TR("持仓: %s 张 | 未实现盈亏: %.2f USDT | ticks=%lld\n",
              "Position: %s contracts | uPnL: %.2f USDT | ticks=%lld\n"),
           fmtDouble(pos, 1).c_str(), upl, (long long)g.strat.ticks());
    int64_t now = nowMs();
    int64_t last = g.lastTradeMs.load();
    int remain = 0;
    if (last > 0) {
        int64_t el = (now - last) / 1000;
        if (el < cfg().cooldownSec) remain = cfg().cooldownSec - (int)el;
    }
    printf(TR("交易: 成交%d笔 | 累计手续费约 %.2f USDT | 冷却%s | 信号确认=%d笔 最小优势=%.0fbps\n",
              "Trades: %d fills | fees paid ~%.2f USDT | cooldown %s | confirm=%d ticks min edge=%.0f bps\n"),
           g.orderCount.load(), g.feeTotalCents.load() / 100.0,
           remain > 0 ? (std::to_string(remain) + "s").c_str() : TR("无", "none"),
           cfg().confirmTicks, cfg().minEdgeBps);
    printf(TR("风控: 熔断=%s | 起始权益=%.2f | 当前权益=%.2f | 净变化=%.2f (已扣手续费)\n",
              "Risk: halted=%s | start equity=%.2f | equity=%.2f | net chg=%.2f (fees incl.)\n"),
           g.risk.halted() ? TR("是", "yes") : TR("否", "no"),
           g.risk.startEquity(), g.risk.lastEquity(),
           g.risk.startEquity() > 0 ? g.risk.lastEquity() - g.risk.startEquity() : 0.0);
}

} // namespace

int main(int argc, char** argv) {
    consoleInit();   // Windows: 切 UTF-8 代码页 + 放大窗口 + 中文字体 + 开启 ANSI

#ifdef _MSC_VER
    const char* kBuild = "MSVC";
#else
    const char* kBuild = "GCC";
#endif
    printf("OKX BTC Quant v1.1 [%s %s %s]\n", kBuild, __DATE__, __TIME__);
    printf("%s\n", consoleDiag().c_str());
    if (noCjkFont()) {
        printf("\n%s\n", TR(
            "[提示] 系统未安装中文字体，中文会显示为乱码/方块。解决办法（任选其一）：\n"
            "  1. 设置 -> 应用 -> 可选功能 -> 添加功能 -> 安装\"中文(简体)补充字体\"\n"
            "  2. 从其他中文电脑复制 C:\\Windows\\Fonts\\simsun.ttc，右键\"为所有用户安装\"\n"
            "  3. 使用 Windows Terminal 打开本程序\n",
            "[Hint] No CJK font installed; Chinese text will render as boxes. Fixes (any one):\n"
            "  1. Settings > Apps > Optional features > Add \"Chinese Simplified Supplemental Fonts\"\n"
            "  2. Copy C:\\Windows\\Fonts\\simsun.ttc from another PC, right-click > Install for all users\n"
            "  3. Run this program inside Windows Terminal\n"));
    }
    fflush(stdout);

    // ---- 定位 config.json：命令行参数 > 当前目录 > exe 目录 > 向上最多 3 级 ----
    // （覆盖 build\Debug、build\Release 等两级深的 VS 输出目录）
    std::string err;
    std::vector<std::string> candidates;
    if (argc > 1) {
        candidates.push_back(argv[1]);
    } else {
        const std::string dir = exeDir();
        candidates.push_back("config.json");
        candidates.push_back(dir + "config.json");
        for (int up = 1; up <= 3; ++up) {
            std::string pre;
            for (int k = 0; k < up; ++k) pre += "..\\";
            candidates.push_back(dir + pre + "config.json");
            std::string preU;
            for (int k = 0; k < up; ++k) preU += "../";
            candidates.push_back(dir + preU + "config.json");
        }
    }
    std::string cfgPath;
    bool loaded = false;
    for (const auto& c : candidates) {
        if (cfg().load(c, err)) { cfgPath = c; loaded = true; break; }
    }
    if (!loaded) {
        fprintf(stderr, "\n%s %s\n", TR("[错误] 配置加载失败:", "[ERROR] failed to load config:"), err.c_str());
        fprintf(stderr, "%s\n", TR("找不到 config.json。请把它放到程序所在目录，或用 okxquant.exe <config路径> 指定。",
                                   "config.json not found. Put it next to the executable or run: okxquant.exe <config path>"));
        if (argc == 1) {   // 双击运行：留住窗口看错误
            printf("\n%s", TR("按回车键退出...", "Press Enter to exit..."));
            fflush(stdout);
            (void)getchar();
        }
        return 1;
    }
    logSetFile(cfg().logFile);
    LOG_I(TR("已加载配置: %s", "Config loaded: %s"), cfgPath.c_str());
    if (!net::init()) {
        fprintf(stderr, "%s\n", TR("网络初始化失败", "Network init failed"));
        return 1;
    }

    printf("=============================================\n");
    printf(TR(" OKX BTC 合约量化交易 (C++ / 模拟盘: %s)\n",
              " OKX BTC Quant Trading (C++ / demo: %s)\n"),
           cfg().demo ? TR("是", "yes") : TR("否(实盘!)", "no (LIVE!)"));
    if (cfg().proxy.enabled) {
        printf(TR(" 代理: SOCKS5 %s:%d\n", "Proxy: SOCKS5 %s:%d\n"),
               cfg().proxy.host.c_str(), cfg().proxy.port);
    } else {
        printf("%s\n", TR(" 代理: 直连", " Proxy: direct connection"));
    }
    printf(TR(" 合约: %s | 杠杆: %dx | TLS校验: %s\n",
              "Instrument: %s | leverage: %dx | TLS verify: %s\n"),
           cfg().instId.c_str(), cfg().leverage, cfg().verifySsl ? TR("开", "on") : TR("关", "off"));
    if (!cfg().creds.valid()) {
        printf("%s\n", TR(" API Key 未配置 —— 只读模式（看行情与信号，不会下单）",
                          " API key not configured -- read-only mode (market data & signals only, no orders)"));
    }
    printf("=============================================\n");

    // 组装 REST 客户端
    if (cfg().verifySsl) {
        std::string ca = resolveCaFile();
        if (!ca.empty()) {
            cfg().caFile = ca;
            LOG_I(TR("CA 证书: %s", "CA bundle: %s"), ca.c_str());
        } else {
            LOG_W(TR("未找到 cacert.pem（可放到程序目录），TLS 校验在 Windows 上通常无法通过；"
                     "也可在 config.json 设置 ssl.verify=false 临时跳过",
                     "cacert.pem not found (put it next to the exe); TLS verify will usually fail on Windows. "
                     "Alternatively set ssl.verify=false in config.json to skip"));
        }
    }
    g.rest.creds = cfg().creds;
    g.rest.demo = cfg().demo;
    g.rest.instId = cfg().instId;
    g.rest.tdMode = cfg().tdMode;
    g.rest.proxy = cfg().proxy;
    g.rest.verifySsl = cfg().verifySsl;
    g.rest.caFile = cfg().caFile;

    // 策略与风控参数
    g.strat.fastTicks = cfg().fastTicks;
    g.strat.slowTicks = cfg().slowTicks;
    g.strat.orderSz = cfg().orderSize;
    g.strat.tradeEnabled = cfg().tradeEnabled;
    g.strat.allowShort = cfg().allowShort;
    g.strat.takerFeeRate = cfg().takerFeeRate;
    g.strat.minEdgeBps = cfg().minEdgeBps;
    g.strat.confirmTicks = cfg().confirmTicks;
    g.risk.maxOrderSize = cfg().maxOrderSize;
    g.risk.maxPosition = cfg().maxPosition;
    g.risk.maxDrawdownPct = cfg().maxDrawdownPct;
    LOG_I(TR("策略参数: fast/slow=%d/%d tick | 信号确认=%d笔 | 最小优势=%.0fbps | taker费率=%.4f%% | 冷却=%ds",
             "Strategy: fast/slow=%d/%d ticks | confirm=%d ticks | min edge=%.0f bps | taker fee=%.4f%% | cooldown=%ds"),
          g.strat.fastTicks, g.strat.slowTicks, g.strat.confirmTicks, g.strat.minEdgeBps,
          g.strat.takerFeeRate * 100, cfg().cooldownSec);
    {
        // 提示手续费盈亏平衡点
        double roundTripBps = 2 * g.strat.takerFeeRate * 10000;
        if (g.strat.minEdgeBps < roundTripBps * 1.5)
            LOG_W(TR("min_edge_bps(%.0f) 建议至少为往返手续费的 1.5 倍(约 %.0fbps)，否则容易被手续费磨损",
                     "min_edge_bps(%.0f) should be at least 1.5x round-trip fee (~%.0f bps), otherwise fees will erode profits"),
                  g.strat.minEdgeBps, roundTripBps * 1.5);
    }

    // 启动自检
    minij::Json t = g.rest.getTicker(err);
    if (t.isNull()) {
        LOG_E(TR("REST 自检失败: %s （检查网络/代理/TLS 证书配置）",
                 "REST self-test failed: %s (check network/proxy/TLS config)"), err.c_str());
    } else if (t.at("code").asStr() == "0") {
        double px = t.at("data").idx(0).at("last").asNum(0);
        LOG_I(TR("REST 自检通过, %s 最新价: %.1f",
                 "REST self-test OK, %s last price: %.1f"), cfg().instId.c_str(), px);
        g.lastPrice.store(px);
    } else {
        LOG_E(TR("REST 返回错误: %s", "REST error: %s"), t.dump().c_str());
    }

    if (g.rest.creds.valid()) {
        minij::Json lr = g.rest.setLeverage(cfg().leverage, err);
        if (!lr.isNull() && OkxRest::ok(lr)) {
            LOG_I(TR("杠杆已设为 %dx", "Leverage set to %dx"), cfg().leverage);
        } else {
            LOG_W(TR("设置杠杆失败: %s", "Set leverage failed: %s"),
                  lr.isNull() ? err.c_str() : OkxRest::firstErr(lr).c_str());
        }
        refreshPosition();
        minij::Json bal = g.rest.getBalance(err);
        if (!bal.isNull() && bal.at("code").asStr() == "0") {
            const minij::Json& det = bal.at("data").idx(0).at("details").idx(0);
            double eq = det.at("eq").asNum(0);
            g.risk.updateEquity(eq);
            LOG_I(TR("账户权益: %.2f USDT", "Account equity: %.2f USDT"), eq);
        }
    }

    // 启动 WebSocket
    {
        OkxWsChannel pub;
        pub.demo = cfg().demo;
        pub.proxy = cfg().proxy;
        pub.verifySsl = cfg().verifySsl;
        pub.caFile = cfg().caFile;
        pub.onEvent = onPublicEvent;
        {
            minij::Json a = minij::Json::object();
            a.set("channel", minij::Json::stringVal("tickers"));
            a.set("instId", minij::Json::stringVal(cfg().instId));
            pub.subscriptions.push_back(std::move(a));
        }
        pub.start(err);

        OkxWsChannel priv;
        priv.isPrivate = true;
        priv.demo = cfg().demo;
        priv.creds = cfg().creds;
        priv.proxy = cfg().proxy;
        priv.verifySsl = cfg().verifySsl;
        priv.caFile = cfg().caFile;
        priv.onEvent = onPrivateEvent;
        bool privOn = false;
        if (cfg().creds.valid()) {
            minij::Json a1 = minij::Json::object();
            a1.set("channel", minij::Json::stringVal("orders"));
            a1.set("instType", minij::Json::stringVal("SWAP"));
            a1.set("instId", minij::Json::stringVal(cfg().instId));
            minij::Json a2 = minij::Json::object();
            a2.set("channel", minij::Json::stringVal("account"));
            priv.subscriptions.push_back(std::move(a1));
            priv.subscriptions.push_back(std::move(a2));
            privOn = priv.start(err);
            if (!privOn) LOG_E(TR("私有频道启动失败: %s", "Private WS start failed: %s"), err.c_str());
        }

        printHelp();

        // 控制台命令循环
        std::string line;
        while (g.running.load()) {
            printf("> ");
            fflush(stdout);
            if (!std::getline(std::cin, line)) break;
            line = trim(line);
            if (line.empty()) continue;
            std::string cmd = toLower(line);

            if (cmd == "quit" || cmd == "exit" || cmd == "q") {
                break;
            } else if (cmd == "help" || cmd == "h" || cmd == "?") {
                printHelp();
            } else if (cmd == "status" || cmd == "s") {
                printStatus();
            } else if (cmd == "pos" || cmd == "p") {
                refreshPosition();
                printStatus();
            } else if (cmd == "bal" || cmd == "b") {
                if (!g.rest.creds.valid()) { printf("%s\n", TR("未配置 API Key", "API key not configured")); continue; }
                minij::Json bal = g.rest.getBalance(err);
                if (!bal.isNull() && bal.at("code").asStr() == "0") {
                    const minij::Json& det = bal.at("data").idx(0).at("details").idx(0);
                    printf(TR("权益: %s USDT | 可用: %s USDT\n",
                              "Equity: %s USDT | available: %s USDT\n"),
                           det.at("eq").asStr("0").c_str(),
                           det.at("availEq").asStr("0").c_str());
                } else {
                    printf(TR("查询失败: %s\n", "Query failed: %s\n"),
                           bal.isNull() ? err.c_str() : OkxRest::firstErr(bal).c_str());
                }
            } else if (cmd == "open" || cmd == "buy") {
                g.strat.tradeEnabled = true;  // 手动命令视为允许下单
                executeTarget(1, /*manual=*/true);
            } else if (cmd == "close" || cmd == "flat") {
                g.strat.tradeEnabled = true;
                executeTarget(0, /*manual=*/true);
            } else if (cmd == "halt") {
                if (g.risk.halted()) {
                    printf("%s\n", TR("已处于熔断状态", "Already halted"));
                } else {
                    g.risk.halt(TR("手动暂停自动交易", "Manual halt of auto trading"));
                }
            } else if (cmd == "resume") {
                g.risk.resetHalt();
                LOG_I(TR("熔断已解除", "Risk halt removed"));
            } else {
                printf("%s\n", TR("未知命令，输入 help 查看帮助", "Unknown command, type help"));
            }
        }

        g.running.store(false);
        pub.stop();
        if (privOn) priv.stop();
    }

    net::cleanup();
    LOG_I(TR("程序退出", "Program exited"));
    return 0;
}
