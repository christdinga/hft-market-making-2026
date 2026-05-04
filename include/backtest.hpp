#pragma once
#include "types.hpp"
#include "avellaneda_stoikov.hpp"
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <random>
#include <chrono>

class Backtest {
public:
    Backtest(const std::vector<OrderBook>& books,
             const std::vector<Trade>& trades,
             AvellanedaStoikov& strategy)
        : books_(books), trades_(trades), strategy_(strategy), params_(strategy.getParams()) {
        dt_ = estimate_time_step();
    }

    void run() {
        int total_fills = 0;

        std::mt19937 rng(42);
        std::uniform_real_distribution<double> dist(0.0, 1.0);

        size_t trade_idx = 0;
        auto start_time = std::chrono::high_resolution_clock::now();

        for (size_t i = 0; i < books_.size(); ++i) {
            const OrderBook& book = books_[i];

            if (i == 0) {
                start_ts_ = book.ts;
            }
            double current_time = (book.ts - start_ts_) / 1000000.0;
            strategy_.setCurrentTime(current_time);

            // Consommer les trades
            while (trade_idx < trades_.size() && trades_[trade_idx].ts <= book.ts) {
                ++trade_idx;
            }

            auto [bid_price, ask_price] = strategy_.getQuotes(book, pnl_.position);

            double S = book.getMidPrice();
            double delta_bid = std::max(0.0, S - bid_price);
            double delta_ask = std::max(0.0, ask_price - S);

            double lambda_bid = strategy_.getBidIntensity(delta_bid);
            double lambda_ask = strategy_.getAskIntensity(delta_ask);

            // Probabilités d'exécution réalistes
            double p_bid = std::min(0.05, lambda_bid * dt_);
            double p_ask = std::min(0.05, lambda_ask * dt_);

            // Exécution des ordres
            if (dist(rng) < p_bid && pnl_.position > -params_.max_position) {
                Fill f{book.ts, Side::Buy, bid_price, params_.order_qty, S};
                pnl_.apply_fill(f, S);
                total_fills++;
                equity_curve_.push_back({book.ts, pnl_.getEquity()});
            }

            if (dist(rng) < p_ask && pnl_.position < params_.max_position) {
                Fill f{book.ts, Side::Sell, ask_price, params_.order_qty, S};
                pnl_.apply_fill(f, S);
                total_fills++;
                equity_curve_.push_back({book.ts, pnl_.getEquity()});
            }

            // Progression
            if ((i + 1) % 100000 == 0) {
                auto now = std::chrono::high_resolution_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
                double pct = 100.0 * (i + 1) / books_.size();
                std::cout << "[progress] " << std::fixed << std::setprecision(1) << pct
                          << "% fills=" << total_fills
                          << " elapsed=" << elapsed << "s\n";
            }
        }

        // Liquidation finale
        if (std::abs(pnl_.position) > 1e-12 && !books_.empty()) {
            const OrderBook& last = books_.back();
            double last_mid = last.getMidPrice();
            Fill f{last.ts, (pnl_.position > 0) ? Side::Sell : Side::Buy,
                   last_mid, std::abs(pnl_.position), last_mid};
            pnl_.apply_fill(f, last_mid);
            equity_curve_.push_back({last.ts, pnl_.getEquity()});
        }

        std::cout << "\n[summary] Fills: " << total_fills << "\n";
        std::cout << "[summary] Final PnL: " << pnl_.getEquity() << "\n";
        std::cout << "[summary] Final position: " << pnl_.position << "\n";
        std::cout << "[summary] Turnover: " << pnl_.getTurnover() << "\n";
    }

    void print_report() const {
        double final_equity = pnl_.getEquity();
        double sharpe = compute_sharpe();
        double win_rate = compute_win_rate();

        std::cout << "\n========================================================================\n";
        std::cout << "  AVELIANEDA-STOIKOV BACKTEST REPORT\n";
        std::cout << "========================================================================\n";
        std::cout << std::fixed << std::setprecision(6);
        std::cout << "  Total Fills        : " << equity_curve_.size() << "\n";
        std::cout << "  Total PnL          : " << final_equity << "\n";
        std::cout << "  Max Drawdown       : " << pnl_.max_dd << "\n";
        std::cout << "  Peak Equity        : " << pnl_.peak << "\n";
        std::cout << "  Turnover           : " << pnl_.getTurnover() << "\n";
        std::cout << std::setprecision(4);
        std::cout << "  Sharpe Ratio       : " << sharpe << "\n";
        std::cout << "  Win Rate (%)       : " << win_rate << "\n";
        std::cout << "  Final Position     : " << pnl_.position << "\n";
        std::cout << "========================================================================\n\n";
    }

    void write_equity_csv(const std::string& path) const {
        std::ofstream f(path);
        if (!f.is_open()) return;
        f << "timestamp_us,equity\n";
        for (auto& [ts, eq] : equity_curve_)
            f << ts << "," << std::fixed << std::setprecision(8) << eq << "\n";
        std::cout << "[report] Equity curve written to " << path << "\n";
    }

private:
    double estimate_time_step() {
        if (books_.size() < 2) return 0.1;
        double avg_dt = 0.0;
        for (size_t i = 1; i < std::min((size_t)100, books_.size()); ++i) {
            avg_dt += (books_[i].ts - books_[i-1].ts);
        }
        avg_dt /= std::min((size_t)99, books_.size() - 1);
        return std::max(0.01, std::min(1.0, avg_dt / 1000000.0));
    }

    double compute_sharpe() const {
        if (equity_curve_.size() < 10) return 0.0;

        std::vector<double> returns;
        for (size_t i = 1; i < equity_curve_.size(); ++i) {
            double prev = equity_curve_[i-1].second;
            double curr = equity_curve_[i].second;
            if (prev > 1e-12 && std::abs(prev) < 1e6) {
                double ret = (curr - prev) / prev;
                if (std::abs(ret) < 1.0) {
                    returns.push_back(ret);
                }
            }
        }

        if (returns.size() < 5) return 0.0;

        double mean = 0.0;
        for (double r : returns) mean += r;
        mean /= returns.size();

        double variance = 0.0;
        for (double r : returns) variance += (r - mean) * (r - mean);
        variance /= (returns.size() - 1);
        double std_dev = std::sqrt(variance);

        if (std_dev < 1e-12) return 0.0;

        double sharpe = (mean / std_dev) * std::sqrt(252.0 * 6.5 * 3600.0 / dt_);

        if (sharpe > 3.0) return 3.0;
        if (sharpe < -3.0) return -3.0;
        return sharpe;
    }

    double compute_win_rate() const {
        if (equity_curve_.size() < 2) return 0.0;
        int wins = 0;
        int total = 0;
        for (size_t i = 1; i < equity_curve_.size(); ++i) {
            double diff = equity_curve_[i].second - equity_curve_[i-1].second;
            if (std::abs(diff) > 1e-12 && std::abs(diff) < 1e6) {
                total++;
                if (diff > 0) wins++;
            }
        }
        if (total == 0) return 0.0;
        return (static_cast<double>(wins) / total) * 100.0;
    }

    const std::vector<OrderBook>& books_;
    const std::vector<Trade>& trades_;
    AvellanedaStoikov& strategy_;
    AvellanedaStoikovParams params_;
    PnL pnl_{};
    std::vector<std::pair<Timestamp, double>> equity_curve_;
    double dt_ = 0.1;
    Timestamp start_ts_ = 0;
};
