// OkxApi.h - OKX V5 接口：REST(签名) + WebSocket(公共/私有)
// 模拟盘：REST 加 x-simulated-trading:1 头；私有 WS 使用 wspap.okx.com
#pragma once
#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <thread>

#include "Json.h"
#include "Net.h"

struct OkxCreds {
    std::string apiKey;
    std::string secretKey;
    std::string passphrase;
    bool valid() const { return !apiKey.empty() && !secretKey.empty() && !passphrase.empty(); }
};

// ---------------- REST ----------------
class OkxRest {
public:
    OkxCreds creds;
    bool demo = true;               // true = 模拟盘
    std::string instId = "BTC-USDT-SWAP";
    std::string tdMode = "cross";   // cross / isolated
    net::ProxyConfig proxy;
    bool verifySsl = true;
    std::string caFile;

    // 底层请求（auth=false 时不加签名头）
    minij::Json request(const std::string& method, const std::string& path,
                        const std::string& body, bool auth, std::string& err);

    // ---- 公共 ----
    minij::Json getTicker(std::string& err);                       // 最新成交
    minij::Json getMarkPrice(std::string& err);

    // ---- 私有（需 creds） ----
    minij::Json getBalance(std::string& err);                      // 账户余额
    minij::Json getPositions(std::string& err);                    // 当前持仓
    minij::Json setLeverage(int lever, std::string& err);
    minij::Json placeMarketOrder(const std::string& side, double sz, std::string& err);
    minij::Json placeLimitOrder(const std::string& side, double sz, double px, std::string& err);
    minij::Json cancelOrder(const std::string& ordId, std::string& err);
    minij::Json getOpenOrders(std::string& err);

    // 便捷判断：resp.code=="0" 且 data[0].sCode=="0"
    static bool ok(const minij::Json& resp);
    static std::string firstErr(const minij::Json& resp);
};

// ---------------- WebSocket 频道 ----------------
class OkxWsChannel {
public:
    bool isPrivate = false;
    bool demo = true;
    OkxCreds creds;
    std::vector<minij::Json> subscriptions;   // 订阅参数（args 数组元素）
    net::ProxyConfig proxy;
    bool verifySsl = true;
    std::string caFile;
    std::function<void(const minij::Json&)> onEvent;  // 收到业务事件（已解析 JSON）

    bool start(std::string& err);
    void stop();
    bool running() const { return running_.load(); }

private:
    void threadFunc();
    bool loginAndSubscribe();
    void handleMessage(const std::string& raw);

    net::WsClient ws_;
    std::atomic<bool> running_{false};
    std::atomic<bool> stopFlag_{false};
    std::thread th_;
};
