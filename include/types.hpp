#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>

using Timestamp = int64_t;
using Price = double;
using Qty = double;

constexpr int LOB_DEPTH = 25;

struct Level {
    Price price = 0.0;
    Qty qty = 0.0;
};

struct OrderBook {
    Timestamp ts = 0;
    std::array<Level, LOB_DEPTH> asks{};
    std::array<Level, LOB_DEPTH> bids{};

    Price getSpread() const { return asks[0].price - bids[0].price; }
    Price getMidPrice() const { return (asks[0].price + bids[0].price) * 0.5; }
    Price getBestBid() const { return bids[0].price; }
    Price getBestAsk() const { return asks[0].price; }
    Qty getBidSize() const { return bids[0].qty; }
    Qty getAskSize() const { return asks[0].qty; }
    double getImbalance() const {
        double total = bids[0].qty + asks[0].qty;
        if (total < 1e-12) return 0.5;
        return bids[0].qty / total;
    }
};

struct Trade {
    Timestamp ts = 0;
    int side = 0;
    Price price = 0.0;
    Qty qty = 0.0;
};

enum class Side { Buy, Sell };

struct Order {
    Timestamp ts = 0;
    Side side = Side::Buy;
    Price price = 0.0;
    Qty qty = 0.0;
};

struct Fill {
    Timestamp ts = 0;
    Side side = Side::Buy;
    Price fill_price = 0.0;
    Qty fill_qty = 0.0;
    Price mid_at_fill = 0.0;
};

struct PnL {
    double realised = 0.0;
    double unrealised = 0.0;
    double position = 0.0;
    double avg_cost = 0.0;
    int trade_count = 0;
    double max_dd = 0.0;
    double peak = 0.0;
    double total_turnover = 0.0;

    void apply_fill(const Fill& f, Price mid) {
        double signed_qty = (f.side == Side::Buy) ? f.fill_qty : -f.fill_qty;
        double trade_value = std::abs(signed_qty * f.fill_price);
        total_turnover += trade_value;

        double old_pos = position;

        if (std::abs(old_pos) < 1e-12) {
            avg_cost = f.fill_price;
        }
        else if ((old_pos > 0 && signed_qty > 0) || (old_pos < 0 && signed_qty < 0)) {
            double new_qty = std::abs(old_pos) + f.fill_qty;
            avg_cost = (avg_cost * std::abs(old_pos) + f.fill_price * f.fill_qty) / new_qty;
        }
        else {
            double closing_qty = std::min(std::abs(old_pos), f.fill_qty);
            double pnl = closing_qty * (f.fill_price - avg_cost);
            if (old_pos < 0) pnl = -pnl;
            realised += pnl;
        }

        position += signed_qty;

        if (std::abs(position) > 1e-12) {
            unrealised = position * (mid - avg_cost);
        } else {
            unrealised = 0.0;
            avg_cost = 0.0;
        }

        double equity = realised + unrealised;
        if (equity > peak) peak = equity;
        double dd = peak - equity;
        if (dd > max_dd) max_dd = dd;
        trade_count++;
    }

    double getEquity() const { return realised + unrealised; }
    double getTurnover() const { return total_turnover; }
};
