// Net.h - 跨平台网络层：TCP / SOCKS5 代理 / TLS(OpenSSL) / WebSocket(RFC6455)
// 不依赖 Boost，Windows(Winsock2) 与 Linux(BSD sockets) 双端支持
#pragma once
#include <string>
#include <functional>
#include <mutex>
#include <atomic>
#include <cstdint>

namespace net {

struct ProxyConfig {
    bool enabled = false;
    std::string host;         // 例如 192.168.0.170
    int port = 0;             // 例如 9200
    std::string username;     // 可空 = 无认证
    std::string password;
};

// 平台初始化（Windows 下 WSAStartup），main 开头调用一次
bool init();
void cleanup();

// ---------- 一条 TCP(+可选TLS) 连接 ----------
// 说明：
//  - 连接建立后，允许"一个线程读 + 另一个线程写"并发（要求 OpenSSL >= 1.1.0）。
//  - recvSome 返回 -2 表示暂时无数据（TLS WANT_READ），调用方配合 waitReadable 使用。
class Connection {
public:
    Connection() = default;
    ~Connection();
    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    bool connect(const std::string& host, int port,
                 const ProxyConfig& proxy,
                 bool useTls, const std::string& tlsHost,
                 bool verifySsl, const std::string& caFile,
                 int timeoutMs, std::string& err);

    bool sendAll(const void* data, size_t len);
    // >0 数据; 0 对端关闭; -1 错误; -2 无数据(WANT_READ)
    int  recvSome(void* buf, size_t len);
    // 等待可读，超时返回 false（不消耗数据）
    bool waitReadable(int timeoutMs);

    bool isOpen() const { return sock_ != kInvalid; }
    void close();
    // 只 shutdown 底层 socket（唤醒阻塞中的 recv），不释放 SSL。
    // 用于跨线程安全停止：由读线程自己调用 close() 收尾。
    void shutdownLink();

    static const unsigned kInvalid = ~0u;

private:
    unsigned sock_ = kInvalid;
    void* sslCtx_ = nullptr;   // SSL_CTX*
    void* ssl_ = nullptr;      // SSL*
    bool tls_ = false;

    void closeSocket();
    bool socks5Handshake(const std::string& host, int port, const ProxyConfig& proxy, std::string& err);
};

// ---------- WebSocket 客户端（wss://，客户端帧强制掩码） ----------
class WsClient {
public:
    std::function<void(const std::string& msg)> onMessage;
    std::function<void()> onClosed;

    std::string logTag = "ws";

    bool connect(const std::string& host, int port, const std::string& path,
                 const ProxyConfig& proxy, bool verifySsl, const std::string& caFile,
                 int timeoutMs, std::string& err);

    bool sendText(const std::string& text);
    // 阻塞接收循环：自动处理 ping/pong、分片、close。
    // 20 秒无数据自动发送 "ping"(OKX 应用层心跳)。连接断开时返回。
    void runLoop();
    void stop();   // 线程安全，主动断开

    bool running() const { return runFlag_.load(); }

private:
    Connection conn_;
    std::mutex sendMtx_;
    std::atomic<bool> runFlag_{false};
    std::string pending_;      // 握手响应中多读出来的帧数据
    std::string frag_;         // 分片消息累积

    bool sendFrame(uint8_t opcode, const uint8_t* data, size_t len);
    bool sendRaw(const void* data, size_t len);
};

} // namespace net
