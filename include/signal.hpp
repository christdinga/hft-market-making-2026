#pragma once
#include "types.hpp"
#include <cmath>
#include <algorithm>

class SignalEngine {
public:
    Signal compute(const OrderBook& prev, const OrderBook& curr) const {
        Signal sig;
        sig.midpx  = (curr.asks[0].price + curr.bids[0].price) * 0.5;
        sig.spread = curr.asks[0].price - curr.bids[0].price;

        if (prev.ts == 0) { sig.ofi = 0.0; return sig; }

        double dQ_bid = 0.0, dQ_ask = 0.0;
        if (curr.bids[0].price > prev.bids[0].price) dQ_bid = curr.bids[0].qty;
        else if (curr.bids[0].price < prev.bids[0].price) dQ_bid = -prev.bids[0].qty;
        else dQ_bid = curr.bids[0].qty - prev.bids[0].qty;

        if (curr.asks[0].price < prev.asks[0].price) dQ_ask = curr.asks[0].qty;
        else if (curr.asks[0].price > prev.asks[0].price) dQ_ask = -prev.asks[0].qty;
        else dQ_ask = curr.asks[0].qty - prev.asks[0].qty;

        double raw = dQ_bid - dQ_ask;
        double scale = std::max(std::abs(raw), 1.0);
        sig.ofi = std::clamp(raw / scale, -1.0, 1.0);
        return sig;
    }
};
