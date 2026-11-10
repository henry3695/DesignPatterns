// OkxApi.cpp - OKX REST / WebSocket 实现
#include "OkxApi.h"
#include "Lang.h"
#include "Util.h"

#include <mutex>
#include <cstdio>

namespace {

const char* kRestHost = "www.okx.com";
const int   kRestPort = 443;
const char* kWsPublicHost = "ws.okx.com";
const char* kWsPrivateLiveHost = "ws.okx.com";
const char* kWsPrivateDemoHost = "wspap.okx.com";   // 模拟盘私有频道
const int   kWsPort = 8443;

// 读完整 HTTP 响应（Connection: close，读到对端关闭）
bool readHttpResponse(net::Connection& conn, std::string& out, std::string& err) {
    char buf[16384];
    out.clear();
    while (true) {
        if (!conn.waitReadable(30000)) { err = "http: read timeout"; return false; }
        int n = conn.recvSome(buf, sizeof(buf));
        if (n == -2) continue;
        if (n < 0) { err = "http: read error"; return false; }
        if (n == 0) break; // 对端关闭 = 读完了
        out.append(buf, (size_t)n);
        if (out.size() > 8 * 1024 * 1024) { err = "http: response too large"; return false; }
    }
    return true;
}

// 处理 chunked 编码
std::string dechunk(std::string body) {
    std::string out;
    size_t pos = 0;
    while (pos < body.size()) {
        size_t eol = body.find("\r\n", pos);
        if (eol == std::string::npos) break;
        long len = strtol(body.c_str() + pos, nullptr, 16);
        if (len <= 0) break;
        size_t start = eol + 2;
        if (start + (size_t)len > body.size()) break;
        out.append(body, start, (size_t)len);
        pos = start + (size_t)len + 2; // 跳过尾随 CRLF
    }
    return out;
}

std::string extractBody(const std::string& resp, std::string& err) {
    size_t hdrEnd = resp.find("\r\n\r\n");
    if (hdrEnd == std::string::npos) { err = "http: bad response"; return ""; }
    std::string hdr = resp.substr(0, hdrEnd);
    std::string body = resp.substr(hdrEnd + 4);

    // 状态码检查
    std::string statusLine = hdr.substr(0, hdr.find("\r\n"));
    if (statusLine.find(" 2") == std::string::npos) {
        err = "http: " + statusLine;
        // 仍返回 body 供调试
    }
    std::string hdrLower = toLower(hdr);
    if (hdrLower.find("transfer-encoding: chunked") != std::string::npos) {
        body = dechunk(body);
    }
    return body;
}

} // namespace

// ---------------- REST ----------------
minij::Json OkxRest::request(const std::string& method, const std::string& path,
                             const std::string& body, bool auth, std::string& err) {
    minij::Json nullResp;

    net::Connection conn;
    if (!conn.connect(kRestHost, kRestPort, proxy, true, kRestHost,
                      verifySsl, caFile, 10000, err)) {
        return nullResp;
    }

    std::string reqPath = path;
    std::string reqBody = body;
    std::string headers;

    if (auth) {
        if (!creds.valid()) { err = TR("未配置 API Key（模拟盘也需在 OKX 模拟交易页面单独创建）",
                     "API key not configured (demo keys must be created on OKX demo trading page separately)"); return nullResp; }
        std::string ts = isoTimestamp();
        std::string sign = hmacSha256Base64(creds.secretKey, ts + method + reqPath + reqBody);
        headers += "OK-ACCESS-KEY: " + creds.apiKey + "\r\n";
        headers += "OK-ACCESS-SIGN: " + sign + "\r\n";
        headers += "OK-ACCESS-TIMESTAMP: " + ts + "\r\n";
        headers += "OK-ACCESS-PASSPHRASE: " + creds.passphrase + "\r\n";
    }
    if (demo) headers += "x-simulated-trading: 1\r\n";

    std::string req;
    req += method + " " + reqPath + " HTTP/1.1\r\n";
    req += "Host: " + std::string(kRestHost) + "\r\n";
    req += "Content-Type: application/json\r\n";
    req += headers;
    req += "Content-Length: " + std::to_string(reqBody.size()) + "\r\n";
    req += "Connection: close\r\n";
    req += "\r\n";
    req += reqBody;

#ifdef NET_DEBUG_TRACE
    {
        // 打印请求头（凭证打码），便于排查鉴权问题
        std::string masked = req;
        const std::string k = creds.apiKey;
        if (!k.empty()) {
            size_t p = masked.find(k);
            if (p != std::string::npos) masked.replace(p, k.size(), "***");
        }
        fprintf(stderr, "[HTTP] >>>\n%s\n", masked.c_str());
    }
#endif

    if (!conn.sendAll(req.data(), req.size())) {
        err = "http: send failed";
        return nullResp;
    }

    std::string resp;
    if (!readHttpResponse(conn, resp, err)) return nullResp;

    std::string bodyOut = extractBody(resp, err);
    if (bodyOut.empty() && !err.empty()) return nullResp;
#ifdef NET_DEBUG_TRACE
    fprintf(stderr, "[HTTP] <<< %s\n", bodyOut.c_str());
#endif

    minij::Json j = minij::Json::parse(bodyOut, &err);
    if (j.isNull()) { err = "json parse failed: " + err; return nullResp; }
    return j;
}

bool OkxRest::ok(const minij::Json& resp) {
    if (!resp.isObj() || resp.at("code").asStr("1") != "0") return false;
    const minij::Json& d0 = resp.at("data").idx(0);
    if (d0.isObj() && d0.contains("sCode")) return d0.at("sCode").asStr() == "0";
    return true;
}

std::string OkxRest::firstErr(const minij::Json& resp) {
    std::string msg = resp.at("msg").asStr();
    if (!msg.empty()) return msg;
    const minij::Json& d0 = resp.at("data").idx(0);
    if (d0.isObj()) {
        std::string s = d0.at("sMsg").asStr();
        if (!s.empty()) return s + " (sCode=" + d0.at("sCode").asStr() + ")";
    }
    return "unknown error";
}

minij::Json OkxRest::getTicker(std::string& err) {
    return request("GET", "/api/v5/market/ticker?instId=" + instId, "", false, err);
}

minij::Json OkxRest::getMarkPrice(std::string& err) {
    return request("GET", "/api/v5/public/mark-price?instId=" + instId + "&instType=SWAP", "", false, err);
}

minij::Json OkxRest::getBalance(std::string& err) {
    return request("GET", "/api/v5/account/balance?ccy=USDT", "", true, err);
}

minij::Json OkxRest::getPositions(std::string& err) {
    return request("GET", "/api/v5/account/positions?instType=SWAP&instId=" + instId, "", true, err);
}

minij::Json OkxRest::setLeverage(int lever, std::string& err) {
    minij::Json b = minij::Json::object();
    b.set("instId", minij::Json::stringVal(instId));
    b.set("lever", minij::Json::stringVal(std::to_string(lever)));
    b.set("mgnMode", minij::Json::stringVal(tdMode));
    return request("POST", "/api/v5/account/set-leverage", b.dump(), true, err);
}

minij::Json OkxRest::placeMarketOrder(const std::string& side, double sz, std::string& err) {
    minij::Json b = minij::Json::object();
    b.set("instId", minij::Json::stringVal(instId));
    b.set("tdMode", minij::Json::stringVal(tdMode));
    b.set("side", minij::Json::stringVal(side));
    b.set("posSide", minij::Json::stringVal("net"));   // 净持仓模式
    b.set("ordType", minij::Json::stringVal("market"));
    b.set("sz", minij::Json::stringVal(fmtDouble(sz, 0)));
    return request("POST", "/api/v5/trade/order", b.dump(), true, err);
}

minij::Json OkxRest::placeLimitOrder(const std::string& side, double sz, double px, std::string& err) {
    minij::Json b = minij::Json::object();
    b.set("instId", minij::Json::stringVal(instId));
    b.set("tdMode", minij::Json::stringVal(tdMode));
    b.set("side", minij::Json::stringVal(side));
    b.set("posSide", minij::Json::stringVal("net"));
    b.set("ordType", minij::Json::stringVal("limit"));
    b.set("sz", minij::Json::stringVal(fmtDouble(sz, 0)));
    b.set("px", minij::Json::stringVal(fmtDouble(px, 1)));
    return request("POST", "/api/v5/trade/order", b.dump(), true, err);
}

minij::Json OkxRest::cancelOrder(const std::string& ordId, std::string& err) {
    minij::Json b = minij::Json::object();
    b.set("instId", minij::Json::stringVal(instId));
    b.set("ordId", minij::Json::stringVal(ordId));
    return request("POST", "/api/v5/trade/cancel-order", b.dump(), true, err);
}

minij::Json OkxRest::getOpenOrders(std::string& err) {
    return request("GET", "/api/v5/trade/orders-pending?instType=SWAP&instId=" + instId, "", true, err);
}

// ---------------- WebSocket 频道 ----------------
bool OkxWsChannel::start(std::string& err) {
    if (isPrivate && !creds.valid()) {
        err = TR("私有频道需要配置 API Key", "Private channel requires API key");
        return false;
    }
    stopFlag_.store(false);
    th_ = std::thread([this]() { threadFunc(); });
    return true;
}

void OkxWsChannel::stop() {
    stopFlag_.store(true);
    ws_.stop();
    if (th_.joinable()) th_.join();
}

bool OkxWsChannel::loginAndSubscribe() {
    if (isPrivate) {
        std::string ts = std::to_string(nowMs() / 1000);
        std::string sign = hmacSha256Base64(creds.secretKey, ts + "GET" + "/users/self/verify");
        minij::Json arg = minij::Json::object();
        arg.set("apiKey", minij::Json::stringVal(creds.apiKey));
        arg.set("passphrase", minij::Json::stringVal(creds.passphrase));
        arg.set("timestamp", minij::Json::stringVal(ts));
        arg.set("sign", minij::Json::stringVal(sign));

        minij::Json login = minij::Json::object();
        login.set("op", minij::Json::stringVal("login"));
        minij::Json args = minij::Json::array();
        args.push(std::move(arg));
        login.set("args", std::move(args));
        if (!ws_.sendText(login.dump())) return false;
        // 等 login 成功事件，由 handleMessage 触发订阅
        return true;
    }
    // 公共频道直接订阅
    minij::Json sub = minij::Json::object();
    sub.set("op", minij::Json::stringVal("subscribe"));
    minij::Json args = minij::Json::array();
    for (const auto& a : subscriptions) args.push(a);
    sub.set("args", std::move(args));
    return ws_.sendText(sub.dump());
}

void OkxWsChannel::handleMessage(const std::string& raw) {
    if (raw == "pong") return;
    minij::Json j = minij::Json::parse(raw);
    if (j.isNull()) return;

    const std::string ev = j.at("event").asStr();
    if (!ev.empty()) {
        if (ev == "login") {
            LOG_I(TR("WS: 私有频道登录成功", "WS: private channel login OK"));
            minij::Json sub = minij::Json::object();
            sub.set("op", minij::Json::stringVal("subscribe"));
            minij::Json args = minij::Json::array();
            for (const auto& a : subscriptions) args.push(a);
            sub.set("args", std::move(args));
            ws_.sendText(sub.dump());
            return;
        }
        if (ev == "subscribe") {
            const minij::Json& arg = j.at("arg");
            LOG_I(TR("WS: 订阅成功 %s", "WS: subscribed %s"), arg.dump().c_str());
            return;
        }
        if (ev == "error") {
            LOG_E(TR("WS: 错误 %s", "WS: error %s"), raw.c_str());
            return;
        }
        return;
    }

    if (onEvent) onEvent(j);
}

void OkxWsChannel::threadFunc() {
    running_.store(true);
    while (!stopFlag_.load()) {
        const char* host = isPrivate
            ? (demo ? kWsPrivateDemoHost : kWsPrivateLiveHost)
            : kWsPublicHost;
        const char* path = isPrivate ? "/ws/v5/private" : "/ws/v5/public";

        std::string err;
        ws_.onMessage = [this](const std::string& raw) { handleMessage(raw); };
        ws_.onClosed = [this]() {
            if (!stopFlag_.load()) LOG_W(TR("WS: 连接断开，3 秒后重连...", "WS: disconnected, reconnecting in 3s..."));
        };

        if (!ws_.connect(host, kWsPort, path, proxy, verifySsl, caFile, 10000, err)) {
            LOG_E(TR("WS: 连接 %s:%d 失败: %s", "WS: connect %s:%d failed: %s"), host, kWsPort, err.c_str());
        } else {
            LOG_I(TR("WS: 已连接 %s%s", "WS: connected %s%s"), host, path);
            if (!loginAndSubscribe()) {
                LOG_E(TR("WS: 发送订阅失败", "WS: send subscribe failed"));
            } else {
                ws_.runLoop();   // 阻塞直到断开
            }
        }

        if (stopFlag_.load()) break;
        // 简单退避：3 秒后重连
        for (int i = 0; i < 30 && !stopFlag_.load(); ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    running_.store(false);
}
