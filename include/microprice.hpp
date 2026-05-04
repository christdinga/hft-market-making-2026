#pragma once
#include "types.hpp"
#include <deque>
#include <cmath>
#include <map>

class MicroPriceEstimator {
public:
    MicroPriceEstimator(double tick_size = 0.0000001, int window = 100)
        : tick_size_(tick_size), window_(window) {}

    double compute(const OrderBook& book, double mid) {
        double I = book.getImbalance();
        double S = book.getSpread();

        if (window_ == 0) {
            return mid + (I - 0.5) * S;
        }

        history_.push_back({book.ts, mid, I, S});
        while ((int)history_.size() > window_) {
            history_.pop_front();
        }

        if (history_.size() < 10) {
            return mid + (I - 0.5) * S;
        }

        return mid + (I - 0.5) * S * 0.5;
    }

private:
    struct State {
        Timestamp ts;
        double mid;
        double imbalance;
        double spread;
    };

    double tick_size_;
    int window_;
    std::deque<State> history_;
};
