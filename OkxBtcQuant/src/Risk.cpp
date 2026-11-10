// Risk.cpp
#include "Risk.h"
#include "Lang.h"
#include "Util.h"
#include <cmath>

bool RiskManager::checkTrade(double pos, double newPos, std::string& why) const {
    if (std::fabs(newPos - pos) < 1e-9) {
        why = TR("无仓位变化", "no position change");
        return false;
    }
    if (std::fabs(newPos - pos) > maxOrderSize + 1e-9) {
        why = TR("单笔张数超过 max_order_size", "order size exceeds max_order_size");
        return false;
    }
    if (std::fabs(newPos) > maxPosition + 1e-9) {
        why = TR("目标净持仓超过 max_position", "target net position exceeds max_position");
        return false;
    }
    if (halted_.load()) {
        why = TR("风控已熔断", "risk already halted");
        return false;
    }
    return true;
}

void RiskManager::updateEquity(double eq) {
    std::lock_guard<std::mutex> lk(m_);
    if (eq <= 0) return;
    if (startEq_ <= 0) startEq_ = eq;
    lastEq_ = eq;
}

bool RiskManager::drawdownBreached(std::string& why) const {
    if (halted_.load()) { why = TR("已熔断", "already halted"); return true; }
    double se, le;
    {
        std::lock_guard<std::mutex> lk(m_);
        se = startEq_; le = lastEq_;
    }
    if (se > 0 && le > 0) {
        double dd = (se - le) / se * 100.0;
        if (dd >= maxDrawdownPct) {
            why = TR("权益回撤 ", "equity drawdown ") + fmtDouble(dd, 2) + "% " + TR("超过限额 ", "exceeds limit ") + fmtDouble(maxDrawdownPct, 2) + "%";
            halted_.store(true);
            return true;
        }
    }
    return false;
}
