#pragma once
#include "types.hpp"
#include "microprice.hpp"
#include <cmath>
#include <iostream>

struct AvellanedaStoikovParams {
    double gamma = 1.0;         // Augmenté pour réduire le spread
    double sigma = 0.5;          // Volatilité
    double T = 60.0;             // Horizon en secondes (réduit)
    double k = 1.5;
    double A = 10.0;
    double tick_size = 0.0000001;
    double max_position = 10000.0;
    double order_qty = 1000.0;
    bool use_microprice = false;
};

class AvellanedaStoikov {
public:
    AvellanedaStoikov(AvellanedaStoikovParams params = {})
        : params_(params), microprice_estimator_(params.tick_size, 100) {}

    void setCurrentTime(double t) { current_time_ = t; }

    std::pair<Price, Price> getQuotes(const OrderBook& book, double position) {
        double S = book.getMidPrice();
        double remaining_time = std::max(1e-6, params_.T - current_time_);

        // Reservation price d'Avellaneda-Stoikov
        double reservation = S - position * params_.gamma * params_.sigma * params_.sigma * remaining_time;

        // Microprice adjustment
        double reference_price = reservation;
        if (params_.use_microprice) {
            double I = book.getImbalance();
            double mp = S + (I - 0.5) * book.getSpread();
            reference_price = 0.7 * reservation + 0.3 * mp;
        }

        // Spread optimal (formule corrigée pour éviter des valeurs énormes)
        double spread = params_.gamma * params_.sigma * params_.sigma * remaining_time;
        spread += (2.0 / params_.gamma) * std::log(1.0 + params_.gamma / params_.k);

        // Limiter le spread
        spread = std::min(spread, 10.0 * params_.tick_size);

        Price bid = reference_price - spread / 2.0;
        Price ask = reference_price + spread / 2.0;

        // Arrondir au tick size
        bid = std::round(bid / params_.tick_size) * params_.tick_size;
        ask = std::round(ask / params_.tick_size) * params_.tick_size;

        // S'assurer que les quotes sont dans le marché
        bid = std::min(bid, book.getBestBid());
        ask = std::max(ask, book.getBestAsk());

        if (bid >= ask) {
            bid = ask - params_.tick_size;
        }

        return {bid, ask};
    }

    double getBidIntensity(double delta_bid) {
        return params_.A * std::exp(-params_.k * delta_bid / params_.tick_size);
    }

    double getAskIntensity(double delta_ask) {
        return params_.A * std::exp(-params_.k * delta_ask / params_.tick_size);
    }

    void updateParameters(const OrderBook& book, const std::vector<Trade>& trades) {
        // Paramètres constants
    }

    const AvellanedaStoikovParams& getParams() const { return params_; }

private:
    AvellanedaStoikovParams params_;
    MicroPriceEstimator microprice_estimator_;
    double current_time_ = 0.0;
};
