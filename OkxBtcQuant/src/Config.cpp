// Config.cpp
#include "Config.h"
#include "Lang.h"
#include "Json.h"
#include "Util.h"

#include <fstream>
#include <sstream>

static AppConfig g_cfg;

AppConfig& cfg() { return g_cfg; }

bool AppConfig::load(const std::string& path, std::string& err) {
    std::ifstream f(path, std::ios::binary);
    if (!f) { err = TR("无法打开配置文件: ", "Cannot open config file: ") + path; return false; }
    std::stringstream ss;
    ss << f.rdbuf();
    std::string text = ss.str();

    std::string perr;
    minij::Json j = minij::Json::parse(text, &perr);
    if (j.isNull()) { err = TR("config.json 解析失败: ", "config.json parse error: ") + perr; return false; }

    const minij::Json& ex = j.at("exchange");
    creds.apiKey = ex.at("api_key").asStr();
    creds.secretKey = ex.at("secret_key").asStr();
    creds.passphrase = ex.at("passphrase").asStr();
    demo = ex.at("demo").asBool(true);
    instId = ex.at("inst_id").asStr("BTC-USDT-SWAP");
    tdMode = ex.at("td_mode").asStr("cross");
    leverage = (int)ex.at("leverage").asInt(5);

    const minij::Json& px = j.at("proxy");
    proxy.enabled = px.at("enabled").asBool(false);
    proxy.host = px.at("host").asStr();
    proxy.port = (int)px.at("port").asInt(0);
    proxy.username = px.at("username").asStr();
    proxy.password = px.at("password").asStr();
    if (proxy.enabled && (proxy.host.empty() || proxy.port <= 0)) {
        err = TR("proxy.enabled=true 但未正确配置 proxy.host / proxy.port",
                 "proxy.enabled=true but proxy.host / proxy.port not set correctly");
        return false;
    }

    const minij::Json& ssl = j.at("ssl");
    verifySsl = ssl.at("verify").asBool(true);
    caFile = ssl.at("ca_file").asStr();

    const minij::Json& st = j.at("strategy");
    fastTicks = (int)st.at("fast_ticks").asInt(60);
    slowTicks = (int)st.at("slow_ticks").asInt(600);
    orderSize = st.at("order_size").asNum(1);
    tradeEnabled = st.at("trade_enabled").asBool(false);
    allowShort = st.at("allow_short").asBool(true);
    takerFeeRate = st.at("taker_fee_rate").asNum(0.0005);   // OKX 合约 taker 0.05%
    minEdgeBps = st.at("min_edge_bps").asNum(15);           // 信号最小优势(bp)
    confirmTicks = (int)st.at("confirm_ticks").asInt(20);   // 信号确认笔数
    cooldownSec = (int)st.at("cooldown_sec").asInt(60);     // 两次下单最小间隔(秒)

    const minij::Json& rk = j.at("risk");
    maxOrderSize = rk.at("max_order_size").asNum(2);
    maxPosition = rk.at("max_position").asNum(10);
    maxDrawdownPct = rk.at("max_drawdown_pct").asNum(10.0);

    logFile = j.at("log_file").asStr("okxquant.log");

    if (fastTicks <= 0 || slowTicks <= fastTicks) {
        err = TR("strategy.fast_ticks 必须大于 0 且小于 slow_ticks",
                 "strategy.fast_ticks must be > 0 and < slow_ticks");
        return false;
    }
    if (confirmTicks < 0 || cooldownSec < 0 || minEdgeBps < 0 || takerFeeRate < 0) {
        err = TR("strategy.confirm_ticks/cooldown_sec/min_edge_bps/taker_fee_rate 不能为负",
                 "strategy.confirm_ticks/cooldown_sec/min_edge_bps/taker_fee_rate cannot be negative");
        return false;
    }
    return true;
}
