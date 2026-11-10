// Net.cpp - TCP / SOCKS5 / TLS / WebSocket 实现
#include "Net.h"
#include "Lang.h"
#include "Util.h"

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <random>

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  ifdef _MSC_VER
#    pragma comment(lib, "ws2_32.lib")
#  endif
using socklen_t = int;
#  define SHUT_RDWR SD_BOTH
#else
#  include <sys/types.h>
#  include <sys/socket.h>
#  include <sys/select.h>
#  include <netdb.h>
#  include <unistd.h>
#  include <fcntl.h>
#  include <netinet/in.h>
#  include <netinet/tcp.h>
#  include <arpa/inet.h>
#  define closesocket close
#  define SOCKET_ERROR (-1)
#endif

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509v3.h>
#include <openssl/rand.h>

namespace net {

// ---------------- 平台初始化 ----------------
bool init() {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;
#endif
    return true;
}

void cleanup() {
#ifdef _WIN32
    WSACleanup();
#endif
}

// ---------------- 小工具 ----------------
namespace {

#ifdef NET_DEBUG_TRACE
void dumpHex(const char* tag, const void* data, size_t n) {
    const unsigned char* p = (const unsigned char*)data;
    std::string s;
    char b[8];
    for (size_t i = 0; i < n; ++i) { snprintf(b, sizeof(b), "%02x ", p[i]); s += b; }
    fprintf(stderr, "[NET] %s (%zu): %s\n", tag, n, s.c_str());
}
#else
inline void dumpHex(const char*, const void*, size_t) {}
#endif

void setSockTimeouts(unsigned s) {
#ifdef _WIN32
    DWORD tv = 30000; // 收发兜底超时 30s，防止永久卡死
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));
#else
    struct timeval tv { 30, 0 };
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif
}

void setNodelay(unsigned s) {
    int one = 1;
    setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (const char*)&one, sizeof(one));
}

// 非阻塞 connect + select 超时
bool tcpConnect(unsigned* out, const std::string& host, int port, int timeoutMs, std::string& err) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    char portStr[16];
    snprintf(portStr, sizeof(portStr), "%d", port);

    struct addrinfo* res = nullptr;
    if (getaddrinfo(host.c_str(), portStr, &hints, &res) != 0 || !res) {
        err = "DNS resolve failed: " + host;
        return false;
    }

    unsigned s = (unsigned)socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (s == Connection::kInvalid || s == ~0u) {
        freeaddrinfo(res);
        err = "socket() failed";
        return false;
    }

#ifdef _WIN32
    u_long nb = 1;
    ioctlsocket(s, FIONBIO, &nb);
#else
    int flags = fcntl((int)s, F_GETFL, 0);
    fcntl((int)s, F_SETFL, flags | O_NONBLOCK);
#endif

    int rc = ::connect((unsigned)s, res->ai_addr, (socklen_t)res->ai_addrlen);
    bool ok = false;
    if (rc == 0) {
        ok = true;
    } else {
        fd_set wset, eset;
        FD_ZERO(&wset); FD_SET((unsigned)s, &wset);
        FD_ZERO(&eset); FD_SET((unsigned)s, &eset);
        struct timeval tv { timeoutMs / 1000, (timeoutMs % 1000) * 1000 };
        if (select((int)s + 1, nullptr, &wset, &eset, &tv) > 0) {
            int soerr = 0;
            socklen_t len = sizeof(soerr);
            getsockopt((unsigned)s, SOL_SOCKET, SO_ERROR, (char*)&soerr, &len);
            ok = (soerr == 0);
            if (!ok) err = "connect error: " + std::to_string(soerr);
        } else {
            err = "connect timeout";
        }
    }

#ifdef _WIN32
    u_long nb0 = 0;
    ioctlsocket(s, FIONBIO, &nb0);
#else
    fcntl((int)s, F_SETFL, flags & ~O_NONBLOCK);
#endif

    if (!ok) { closesocket((unsigned)s); freeaddrinfo(res); return false; }

    setSockTimeouts(s);
    setNodelay(s);
    freeaddrinfo(res);
    *out = s;
    return true;
}

std::string randomKey(int nBytes) {    unsigned char buf[32];    if (nBytes > 32) nBytes = 32;
    if (RAND_bytes(buf, nBytes) != 1) {
        // 兜底：用随机设备
        std::mt19937_64 rng((uint64_t)std::chrono::steady_clock::now().time_since_epoch().count()
                            ^ (uint64_t)nowMs());
        for (int i = 0; i < nBytes; ++i) buf[i] = (unsigned char)(rng() & 0xFF);
    }
    return std::string((const char*)buf, (size_t)nBytes);
}

// 精确读满 len 字节（处理 WANT_READ 与超时）
bool readExact(Connection& c, void* buf, size_t len, int timeoutMs, std::string& err) {
    char* p = (char*)buf;
    size_t got = 0;
    int64_t deadline = nowMs() + timeoutMs;
    while (got < len) {
        int n = c.recvSome(p + got, len - got);
#ifdef NET_DEBUG_TRACE
        fprintf(stderr, "[NET] readExact want=%zu got=%zu n=%d\n", len, got, n);
#endif
        if (n > 0) { got += (size_t)n; continue; }
        if (n == -2) {
            if (nowMs() > deadline) { err = "read timeout"; return false; }
            c.waitReadable(500);
            continue;
        }
        err = (n == 0) ? "connection closed by peer" : "socket read error";
        return false;
    }
    return true;
}

} // namespace

// ---------------- Connection ----------------
Connection::~Connection() { close(); }

void Connection::closeSocket() {
    if (sock_ != kInvalid) {
        shutdown(sock_, SHUT_RDWR); // 唤醒阻塞中的 recv
        closesocket(sock_);
        sock_ = kInvalid;
    }
}

void Connection::close() {
    if (ssl_) {
        SSL* ssl = (SSL*)ssl_;
        SSL_shutdown(ssl); // best-effort
        SSL_free(ssl);
        ssl_ = nullptr;
    }
    if (sslCtx_) {
        SSL_CTX_free((SSL_CTX*)sslCtx_);
        sslCtx_ = nullptr;
    }
    closeSocket();
    tls_ = false;
}

void Connection::shutdownLink() {
    if (sock_ != kInvalid) shutdown(sock_, SHUT_RDWR);
}

bool Connection::socks5Handshake(const std::string& host, int port, const ProxyConfig& proxy, std::string& err) {
    // 方法协商：支持无认证(0x00)与用户名/密码(0x02)
    bool hasAuth = !proxy.username.empty();
    unsigned char greet[4] = { 0x05, (unsigned char)(hasAuth ? 2 : 1), 0x00, 0x02 };
    if (!sendAll(greet, hasAuth ? 4 : 3)) { err = "socks5: send greeting failed"; return false; }
    dumpHex("socks5 greet ->", greet, hasAuth ? 4 : 3);

    unsigned char resp[2];
    if (!readExact(*this, resp, 2, 10000, err)) { err = "socks5: greeting response failed: " + err; return false; }
    dumpHex("socks5 greet <-", resp, 2);
    if (resp[0] != 0x05) { err = "socks5: bad version"; return false; }
    unsigned char method = resp[1];

    if (method == 0x02) {
        std::string u = proxy.username, p = proxy.password;
        if (u.size() > 255) u.resize(255);
        if (p.size() > 255) p.resize(255);
        std::string auth;
        auth += (char)0x01;
        auth += (char)u.size(); auth += u;
        auth += (char)p.size(); auth += p;
        if (!sendAll(auth.data(), auth.size())) { err = "socks5: send auth failed"; return false; }
        unsigned char ar[2];
        if (!readExact(*this, ar, 2, 10000, err) || ar[1] != 0x00) { err = "socks5: auth failed"; return false; }
    } else if (method != 0x00) {
        err = "socks5: unsupported auth method: " + std::to_string(method);
        return false;
    }

    // CONNECT 请求：域名方式（ATYP=0x03），由代理解析 DNS
    if (host.size() > 255) { err = "socks5: hostname too long"; return false; }
    std::string req;
    req += (char)0x05; req += (char)0x01; req += (char)0x00;
    req += (char)0x03; req += (char)host.size(); req += host;
    // 端口按网络字节序（大端）写入：先高字节，再低字节
    // 注意：不要先 htons 再拆字节，那会双重反转（曾导致连到 20480 端口）
    req += (char)((port >> 8) & 0xFF);
    req += (char)(port & 0xFF);
    if (!sendAll(req.data(), req.size())) { err = "socks5: send connect failed"; return false; }
    dumpHex("socks5 connect ->", req.data(), req.size());

    // 应答: VER REP RSV ATYP BND.ADDR BND.PORT
    // 注意：必须把 BND.ADDR 和**后面的 2 字节 PORT** 全部读干净，
    // 否则残留字节会被当成 TLS 记录的开头，导致握手失败（曾踩过这个坑）。
    unsigned char hdr[4];
    if (!readExact(*this, hdr, 4, 15000, err)) { err = "socks5: connect response failed: " + err; return false; }
    dumpHex("socks5 connect <-", hdr, 4);
    if (hdr[0] != 0x05) { err = "socks5: bad reply version"; return false; }
    if (hdr[1] != 0x00) {
        static const char* kRep[] = { "succeeded", "general failure", "not allowed",
                                      "network unreachable", "host unreachable",
                                      "connection refused", "TTL expired",
                                      "command not supported", "address type not supported" };
        std::string why = (hdr[1] < 9) ? kRep[hdr[1]] : "unknown";
        err = "socks5: connect rejected, rep=" + std::to_string(hdr[1]) + " (" + why + ")";
        return false;
    }

    size_t addrLen = 0;
    unsigned char atyp = hdr[3];
    if (atyp == 0x01) addrLen = 4;                 // IPv4
    else if (atyp == 0x04) addrLen = 16;           // IPv6
    else if (atyp == 0x03) {                       // 域名：先读 1 字节长度
        unsigned char l = 0;
        if (!readExact(*this, &l, 1, 10000, err)) { err = "socks5: bad domain reply"; return false; }
        addrLen = (size_t)l;
    } else {
        err = "socks5: bad ATYP=" + std::to_string((int)atyp);
        return false;
    }

    unsigned char skip[256];
    if (addrLen > 0 && !readExact(*this, skip, addrLen, 10000, err)) {
        err = "socks5: short address reply";
        return false;
    }
    unsigned char portBuf[2];   // BND.PORT —— 必须读掉
    if (!readExact(*this, portBuf, 2, 10000, err)) {
        err = "socks5: short port reply";
        return false;
    }
#ifdef NET_DEBUG_TRACE
    {
        u_long pending = 0;
        ioctlsocket(sock_, FIONREAD, &pending);
        fprintf(stderr, "[NET] socks5 done, pending bytes=%lu\n", pending);
    }
#endif
    return true;
}

bool Connection::connect(const std::string& host, int port,
                         const ProxyConfig& proxy,
                         bool useTls, const std::string& tlsHost,
                         bool verifySsl, const std::string& caFile,
                         int timeoutMs, std::string& err) {
    close();

    if (proxy.enabled) {
        if (!tcpConnect(&sock_, proxy.host, proxy.port, timeoutMs, err)) return false;
        if (!socks5Handshake(host, port, proxy, err)) { close(); return false; }
    } else {
        if (!tcpConnect(&sock_, host, port, timeoutMs, err)) return false;
    }

    tls_ = useTls;
    if (useTls) {
        SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
        if (!ctx) { err = "SSL_CTX_new failed"; close(); return false; }
        sslCtx_ = ctx;

        if (verifySsl) {
            SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr);
            bool loaded = false;
            if (!caFile.empty()) {
                if (SSL_CTX_load_verify_locations(ctx, caFile.c_str(), nullptr) == 1) loaded = true;
                else LOG_W(TR("TLS: 未能加载 CA 文件: %s", "TLS: failed to load CA file: %s"), caFile.c_str());
            }
            if (!loaded) {
                if (SSL_CTX_set_default_verify_paths(ctx) == 1) loaded = true;
            }
            if (!loaded) {
                LOG_W(TR("TLS: 未找到任何 CA 证书，证书校验可能失败。"
                      "可在配置 ssl.ca_file 指向 cacert.pem，或将 ssl.verify 设为 false",
                      "TLS: no CA certificate found, verify may fail. "
                      "Set ssl.ca_file to cacert.pem in config, or set ssl.verify=false"));
            }
        } else {
            SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);
        }

        SSL* ssl = SSL_new(ctx);
        if (!ssl) { err = "SSL_new failed"; close(); return false; }
        ssl_ = ssl;
        SSL_set_fd(ssl, (int)sock_);

        // SNI + 主机名校验
        const std::string& h = tlsHost.empty() ? host : tlsHost;
        SSL_set_tlsext_host_name(ssl, h.c_str());
        if (verifySsl) {
            X509_VERIFY_PARAM* param = SSL_get0_param(ssl);
            X509_VERIFY_PARAM_set1_host(param, h.c_str(), 0);
        }

        ERR_clear_error();
        int rc = SSL_connect(ssl);
        if (rc != 1) {
            int sslErr = SSL_get_error(ssl, rc);
            unsigned long e = ERR_get_error();
            char buf[320] = {0};
            if (e) {
                ERR_error_string_n(e, buf, sizeof(buf));
            } else {
                const char* hint = TR("未知原因", "unknown");
                switch (sslErr) {
                    case SSL_ERROR_WANT_READ:  hint = TR("等待对端数据超时（代理/防火墙未转发或对端无响应）", "timeout waiting for peer (proxy/firewall not forwarding, or peer silent)"); break;
                    case SSL_ERROR_WANT_WRITE: hint = TR("写入阻塞", "write blocked"); break;
                    case SSL_ERROR_SYSCALL:    hint = TR("底层 socket 出错/超时（多为代理未正确转发或网络中断）", "socket error/timeout (usually proxy not forwarding or network dropped)"); break;
                    case SSL_ERROR_ZERO_RETURN:hint = TR("对端在握手中关闭了连接（可能不支持该端口上的 TLS）", "peer closed during handshake (TLS may not be supported on this port)"); break;
                    case SSL_ERROR_SSL:        hint = TR("协议错误", "protocol error"); break;
                    default: break;
                }
                snprintf(buf, sizeof(buf), "ssl_error=%d: %s", sslErr, hint);
            }
            err = std::string("TLS handshake failed: ") + buf;
            close();
            return false;
        }
        if (verifySsl && SSL_get_verify_result(ssl) != X509_V_OK) {
            err = std::string(TR("TLS 证书校验失败: ", "TLS certificate verify failed: ")) + X509_verify_cert_error_string(SSL_get_verify_result(ssl));
            close();
            return false;
        }
    }
    return true;
}

bool Connection::sendAll(const void* data, size_t len) {
    const char* p = (const char*)data;
    while (len > 0) {
        int n;
        if (tls_) {
            SSL* ssl = (SSL*)ssl_;
            n = SSL_write(ssl, p, (int)len);
            if (n <= 0) {
                int e = SSL_get_error(ssl, n);
                if (e == SSL_ERROR_WANT_WRITE || e == SSL_ERROR_WANT_READ) { continue; }
                return false;
            }
        } else {
            n = (int)::send(sock_, p, (int)len, 0);
#ifdef NET_DEBUG_TRACE
            dumpHex("send ->", p, len > 96 ? 96 : len);
            fprintf(stderr, "[NET] send n=%d wsa=%d fd=%u\n", n, WSAGetLastError(), (unsigned)sock_);
#endif
            if (n <= 0) {
                return false;
            }
        }
        p += n;
        len -= (size_t)n;
    }
    return true;
}

int Connection::recvSome(void* buf, size_t len) {
    if (!isOpen()) return -1;
    if (tls_) {
        SSL* ssl = (SSL*)ssl_;
        int n = SSL_read(ssl, buf, (int)len);
        if (n > 0) return n;
        int e = SSL_get_error(ssl, n);
        if (e == SSL_ERROR_WANT_READ || e == SSL_ERROR_WANT_WRITE) return -2;
        if (e == SSL_ERROR_ZERO_RETURN) return 0;
        return -1;
    }
#ifdef _WIN32
    int n = (int)::recv(sock_, (char*)buf, (int)len, 0);
#else
    ssize_t rn = ::recv(sock_, buf, len, 0);
    int n = (int)rn;
#endif
    if (n > 0) return n;
    if (n == 0) return 0;
#ifdef _WIN32
    int se = WSAGetLastError();
#ifdef NET_DEBUG_TRACE
    fprintf(stderr, "[NET] recv n=%d wsa=%d\n", n, se);
#endif
    if (se == WSAEWOULDBLOCK || se == WSAETIMEDOUT) return -2;
#else
    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) return -2;
#endif
    return -1;
}

bool Connection::waitReadable(int timeoutMs) {
    if (!isOpen()) return false;
    fd_set rset;
    FD_ZERO(&rset);
    FD_SET(sock_, &rset);
    struct timeval tv { timeoutMs / 1000, (timeoutMs % 1000) * 1000 };
    int r = select((int)sock_ + 1, &rset, nullptr, nullptr, &tv);
#ifdef NET_DEBUG_TRACE
    if (r <= 0) fprintf(stderr, "[NET] select=%d wsa=%d fd=%u\n", r, WSAGetLastError(), (unsigned)sock_);
#endif
    return r > 0;
}

// ---------------- WebSocket ----------------
bool WsClient::sendRaw(const void* data, size_t len) {
    std::lock_guard<std::mutex> lk(sendMtx_);
    return conn_.sendAll(data, len);
}

bool WsClient::sendFrame(uint8_t opcode, const uint8_t* data, size_t len) {
    std::string mask = randomKey(4);
    std::string frame;
    frame += (char)(0x80 | opcode); // FIN + opcode
    if (len < 126) {
        frame += (char)(0x80 | len); // 客户端必须掩码
    } else if (len <= 0xFFFF) {
        frame += (char)(0x80 | 126);
        frame += (char)((len >> 8) & 0xFF);
        frame += (char)(len & 0xFF);
    } else {
        frame += (char)(0x80 | 127);
        uint64_t l = (uint64_t)len;
        for (int i = 7; i >= 0; --i) frame += (char)((l >> (8 * i)) & 0xFF);
    }
    frame += mask;
    for (size_t i = 0; i < len; ++i) frame += (char)(data[i] ^ (unsigned char)mask[i % 4]);
    return sendRaw(frame.data(), frame.size());
}

bool WsClient::sendText(const std::string& text) {
    return sendFrame(0x1, (const uint8_t*)text.data(), text.size());
}

bool WsClient::connect(const std::string& host, int port, const std::string& path,
                       const ProxyConfig& proxy, bool verifySsl, const std::string& caFile,
                       int timeoutMs, std::string& err) {
    std::lock_guard<std::mutex> lk(sendMtx_);
    if (!conn_.connect(host, port, proxy, true, host, verifySsl, caFile, timeoutMs, err)) {
        return false;
    }

    // HTTP Upgrade 握手
    std::string key = base64Encode((const unsigned char*)randomKey(16).data(), 16);
    std::string req;
    req += "GET " + path + " HTTP/1.1\r\n";
    req += "Host: " + host + ":" + std::to_string(port) + "\r\n";
    req += "Upgrade: websocket\r\n";
    req += "Connection: Upgrade\r\n";
    req += "Sec-WebSocket-Key: " + key + "\r\n";
    req += "Sec-WebSocket-Version: 13\r\n";
    req += "\r\n";
    if (!conn_.sendAll(req.data(), req.size())) { err = "ws: send handshake failed"; return false; }

    // 读到 \r\n\r\n 为止
    std::string resp;
    char buf[4096];
    while (resp.find("\r\n\r\n") == std::string::npos) {
        if (!conn_.waitReadable(timeoutMs)) { err = "ws: handshake timeout"; return false; }
        int n = conn_.recvSome(buf, sizeof(buf));
        if (n <= 0 && n != -2) { err = "ws: closed during handshake"; return false; }
        if (n > 0) resp.append(buf, (size_t)n);
        if (resp.size() > 64 * 1024) { err = "ws: handshake response too large"; return false; }
    }

    size_t hdrEnd = resp.find("\r\n\r\n");
    std::string statusLine = resp.substr(0, resp.find("\r\n"));
    if (statusLine.find("101") == std::string::npos) {
        err = "ws: handshake rejected: " + statusLine;
        return false;
    }
    pending_ = resp.substr(hdrEnd + 4); // 帧起点
    runFlag_.store(true);
    return true;
}

void WsClient::runLoop() {
    std::string buf = pending_;
    pending_.clear();
    int64_t lastRecv = nowMs();

    while (runFlag_.load()) {
        if (!conn_.isOpen()) break;

        // 空闲心跳：OKX 约定 30s 无数据断连，客户端发文本 "ping" 回 "pong"
        if (!conn_.waitReadable(5000)) {
            if (nowMs() - lastRecv > 20000) {
                sendText("ping");
                lastRecv = nowMs();
            }
            continue;
        }

        char tmp[8192];
        int n = conn_.recvSome(tmp, sizeof(tmp));
        if (n == -2) continue;
        if (n <= 0) break;             // 关闭/错误
        lastRecv = nowMs();
        buf.append(tmp, (size_t)n);

        // 解析帧
        size_t off = 0;
        while (true) {
            if (buf.size() - off < 2) break;
            const unsigned char* b = (const unsigned char*)buf.data() + off;
            bool fin = (b[0] & 0x80) != 0;
            uint8_t opcode = b[0] & 0x0F;
            bool masked = (b[1] & 0x80) != 0;
            uint64_t len = b[1] & 0x7F;
            size_t hdr = 2;

            if (len == 126) {
                if (buf.size() - off < hdr + 2) break;
                len = ((uint64_t)b[2] << 8) | b[3];
                hdr += 2;
            } else if (len == 127) {
                if (buf.size() - off < hdr + 8) break;
                len = 0;
                for (int i = 0; i < 8; ++i) len = (len << 8) | b[2 + i];
                hdr += 8;
            }
            if (masked) hdr += 4;
            if (len > (uint64_t)(16 * 1024 * 1024)) { runFlag_.store(false); break; } // 防御
            if (buf.size() - off < hdr + (size_t)len) break;

            const unsigned char* payload = b + hdr;
            if (masked) {
                const unsigned char* mk = b + hdr - 4;
                for (uint64_t i = 0; i < len; ++i)
                    ((unsigned char*)payload)[i] ^= mk[i % 4];
            }

            std::string s((const char*)payload, (size_t)len);
            off += hdr + (size_t)len;

            switch (opcode) {
                case 0x1: // text
                case 0x2: // binary
                    if (fin) {
                        std::string msg = frag_.empty() ? s : (frag_ += s, std::move(frag_));
                        frag_.clear();
                        if (onMessage) onMessage(msg);
                    } else {
                        frag_ += s;
                    }
                    break;
                case 0x0: // continuation
                    frag_ += s;
                    if (fin) {
                        std::string msg = std::move(frag_);
                        frag_.clear();
                        if (onMessage) onMessage(msg);
                    }
                    break;
                case 0x8: // close
                    sendFrame(0x8, nullptr, 0);
                    runFlag_.store(false);
                    break;
                case 0x9: // ping -> pong
                    sendFrame(0xA, (const uint8_t*)s.data(), s.size());
                    break;
                case 0xA: // pong
                default:
                    break;
            }
            if (!runFlag_.load()) break;
        }
        if (off > 0) buf.erase(0, off);
    }

    runFlag_.store(false);
    conn_.close();
    if (onClosed) onClosed();
}

void WsClient::stop() {
    runFlag_.store(false);
    // 只 shutdown 唤醒阻塞的读线程；SSL 对象由读线程在 runLoop 结束时自行释放，
    // 否则会在另一个线程正使用 SSL 时 free，导致退出崩溃。
    std::lock_guard<std::mutex> lk(sendMtx_);
    conn_.shutdownLink();
}

} // namespace net
