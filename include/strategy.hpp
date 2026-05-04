#pragma once
#include "types.hpp"
#include "signal.hpp"
#include <vector>
#include <algorithm>

class MarketMaker {
public:
    explicit MarketMaker(StrategyParams p) : params_(std::move(p)) {}

    std::vector<Order> on_book(Timestamp ts, const Signal& sig,
                               const OrderBook& book, double position) const {
        std::vector<Order> orders;
        if (sig.spread < params_.tick_size) return orders;

        double skew = params_.ofi_skew_factor * sig.ofi * params_.tick_size;
        double mid  = sig.midpx;

        double our_bid = mid - params_.base_spread + skew;
        double our_ask = mid + params_.base_spread + skew;

        our_bid = std::min(our_bid, book.bids[0].price);
        our_ask = std::max(our_ask, book.asks[0].price);

        double inv_skew = -std::clamp(position / params_.max_position, -1.0, 1.0) * params_.tick_size;
        our_bid += inv_skew;
        our_ask += inv_skew;

        orders.push_back({ts, Side::Buy, our_bid, params_.order_qty});
        orders.push_back({ts, Side::Sell, our_ask, params_.order_qty});
        return orders;
    }

    const StrategyParams& params() const { return params_; }

private:
    StrategyParams params_;
};
