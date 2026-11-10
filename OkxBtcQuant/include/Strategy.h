// Strategy.h - EMA 交叉趋势策略（基于逐笔最新价，含手续费感知的防抖过滤）
#pragma once
#include <cstdint>

class EmaStrategy {
public:
    // 参数
    int   fastTicks = 60;    // 快线"等效周期"（按 tick 数计）
    int   slowTicks = 600;   // 慢线等效周期
    double orderSz = 1;      // 每次开仓张数（BTC-USDT-SWAP 1张=0.01 BTC）
    bool  tradeEnabled = false; // 允许真实下单（false 只打印信号）
    bool  allowShort = true;    // 允许做空（signal 为 -1）

    // ---- 手续费感知 / 防过度交易 ----
    // 交叉确认时要求 |fast-slow|/price >= minEdgeBps/10000 才算有效信号，
    // 过滤"贴着零轴抖一下"的假交叉。参考：往返一趟 taker 手续费占比
    // = 2*takerFeeRate（0.05% taker 时约 10bps），minEdgeBps 应明显高于它。
    double minEdgeBps = 15;
    // 候选交叉需连续 confirmTicks 个报价方向不变才确认，大幅降低噪声反复开平。
    int   confirmTicks = 20;
    double takerFeeRate = 0.0005;

    // 每来一个最新价调用一次
    // 返回目标仓位方向: +1 做多, -1 做空, 0 空仓
    int onPrice(double px);

    int  target() const { return target_; }
    double emaFast() const { return emaFast_; }
    double emaSlow() const { return emaSlow_; }
    int64_t ticks() const { return ticks_; }
    int64_t trades() const { return trades_; }      // 已确认的方向切换次数
    int64_t rejected() const { return rejected_; }  // 被过滤掉的候选交叉次数

private:
    double emaFast_ = 0, emaSlow_ = 0;
    bool   inited_ = false;
    int    target_ = 0;
    double prevDiff_ = 0;
    int64_t ticks_ = 0;
    int64_t trades_ = 0;
    int64_t rejected_ = 0;
    int    pendingDir_ = 0;     // 候选方向（0=无候选）
    int    pendingCount_ = 0;   // 候选已连续出现的报价数
};
