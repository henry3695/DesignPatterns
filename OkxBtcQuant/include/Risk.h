// Risk.h - 风控：单笔限额、总仓位上限、回撤熔断
#pragma once
#include <atomic>
#include <string>
#include <mutex>
#include "Util.h"

class RiskManager {
public:
    // 参数
    double maxOrderSize = 2;        // 单笔最大张数
    double maxPosition = 10;        // 最大净持仓张数（绝对值）
    double maxDrawdownPct = 10.0;   // 权益最大回撤百分比，超过则熔断

    // 校验一笔交易：当前净持仓 pos，调仓后 newPos
    // 通过返回 true，否则 why 给出原因
    bool checkTrade(double pos, double newPos, std::string& why) const;

    // 权益追踪（用于回撤熔断）
    void updateEquity(double eq);
    bool drawdownBreached(std::string& why) const;
    void resetHalt() { halted_.store(false); }
    void halt(const std::string& reason) { LOG_E("风控熔断: %s", reason.c_str()); halted_.store(true); }
    bool halted() const { return halted_.load(); }

    double startEquity() const { double v; { std::lock_guard<std::mutex> lk(m_); v = startEq_; } return v; }
    double lastEquity()  const { double v; { std::lock_guard<std::mutex> lk(m_); v = lastEq_; } return v; }

private:
    mutable std::mutex m_;
    double startEq_ = 0;
    double lastEq_ = 0;
    mutable std::atomic<bool> halted_{false};
};
