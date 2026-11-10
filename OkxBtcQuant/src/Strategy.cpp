// Strategy.cpp
#include "Strategy.h"
#include "Lang.h"
#include "Util.h"
#include <cmath>

int EmaStrategy::onPrice(double px) {
    if (px <= 0 || px != px) return target_;

    double aF = 2.0 / (fastTicks + 1.0);
    double aS = 2.0 / (slowTicks + 1.0);

    if (!inited_) {
        emaFast_ = emaSlow_ = px;
        prevDiff_ = 0;
        inited_ = true;
    } else {
        emaFast_ += aF * (px - emaFast_);
        emaSlow_ += aS * (px - emaSlow_);
    }
    ++ticks_;

    double diff = emaFast_ - emaSlow_;

    if (ticks_ > (int64_t)slowTicks) {   // 慢线稳定后才产生信号
        // 原始方向：diff 超过边缘阈值才算数（过滤零轴附近的毛刺交叉）
        int raw = 0;
        double edge = px * minEdgeBps / 10000.0;
        if (diff > edge)  raw = 1;
        else if (diff < -edge) raw = allowShort ? -1 : 0;

        if (raw != target_) {
            // 候选信号确认机制：连续 confirmTicks 个报价方向一致才切换，
            // 避免手续费被高频假交叉磨掉
            if (raw == pendingDir_) {
                ++pendingCount_;
            } else {
                pendingDir_ = raw;
                pendingCount_ = 1;
            }
            if (pendingCount_ >= confirmTicks) {
                LOG_I(TR("[策略] %s 确认 (连续%d笔) fast=%.1f slow=%.1f px=%.1f 差=%.1f(%.1fbps) -> 目标: %s",
                   "[strategy] %s confirmed (%d ticks) fast=%.1f slow=%.1f px=%.1f diff=%.1f (%.1f bps) -> target: %s"),
                   pendingDir_ > 0 ? TR("金叉", "golden cross") : (pendingDir_ < 0 ? TR("死叉", "death cross") : TR("转空仓", "go flat")),
                   pendingCount_, emaFast_, emaSlow_, px, diff,
                   std::fabs(diff) / px * 10000.0,
                   pendingDir_ > 0 ? TR("多头", "long") : (pendingDir_ < 0 ? TR("空头", "short") : TR("空仓", "flat")));
                // 打印本次切换的往返手续费预估（名义价值 = 张数*面值*价格，1张=0.01 BTC）
                double notional = orderSz * 0.01 * px;
                double roundTrip = notional * 2 * takerFeeRate;
                LOG_I(TR("[策略] 本次调仓 2 笔(开+平) taker 手续费预估: %.2f USDT, 行情需走出 >%.1f USDT 才有净利",
                   "[strategy] this rebalance = 2 fills (open+close), est taker fee: %.2f USDT; market must move >%.1f USDT for net profit"),
                   roundTrip, roundTrip);
                target_ = pendingDir_;
                ++trades_;
                pendingDir_ = 0;
                pendingCount_ = 0;
            }
        } else {
            pendingDir_ = 0;    // 已回到目标方向，取消未完成的候选
            pendingCount_ = 0;
        }
        // 统计被边缘阈值过滤掉的假交叉（方向穿越零轴但幅度不足）
        if (raw == 0 && ((prevDiff_ <= 0 && diff > 0) || (prevDiff_ >= 0 && diff < 0)))
            ++rejected_;
    }
    prevDiff_ = diff;
    return target_;
}
