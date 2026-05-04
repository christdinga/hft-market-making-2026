#pragma once
#include "types.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <algorithm>
#include <vector>
#include <string>

inline std::vector<std::string> split_csv(const std::string& line) {
    std::vector<std::string> cols;
    std::istringstream ss(line);
    std::string tok;
    while (std::getline(ss, tok, ',')) {
        cols.push_back(tok);
    }
    return cols;
}

inline std::vector<OrderBook> parse_lob(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        throw std::runtime_error("Cannot open LOB file: " + path);
    }

    std::vector<OrderBook> books;
    std::string line;

    std::getline(f, line);
    auto headers = split_csv(line);
    std::cout << "[parser] LOB columns: " << headers.size() << " cols\n";

    while (std::getline(f, line)) {
        if (line.empty()) continue;

        auto cols = split_csv(line);
        if (cols.size() < 2 + LOB_DEPTH * 4) continue;

        OrderBook ob;
        ob.ts = std::stoll(cols[1]);

        for (int i = 0; i < LOB_DEPTH; ++i) {
            size_t base = 2 + static_cast<size_t>(i) * 4;
            if (base + 3 >= cols.size()) break;
            ob.asks[i].price = std::stod(cols[base]);
            ob.asks[i].qty   = std::stod(cols[base + 1]);
            ob.bids[i].price = std::stod(cols[base + 2]);
            ob.bids[i].qty   = std::stod(cols[base + 3]);
        }

        if (ob.asks[0].price > ob.bids[0].price && ob.asks[0].price > 0) {
            books.push_back(ob);
        }
    }

    std::cout << "[parser] LOB rows loaded: " << books.size() << "\n";
    if (!books.empty()) {
        std::cout << "[parser] LOB time range: " << books.front().ts
                  << " to " << books.back().ts << "\n";
    }
    return books;
}

inline std::vector<Trade> parse_trades(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        throw std::runtime_error("Cannot open trades file: " + path);
    }

    std::vector<Trade> trades;
    std::string line;
    std::getline(f, line);

    while (std::getline(f, line)) {
        if (line.empty()) continue;

        auto cols = split_csv(line);
        if (cols.size() < 5) continue;

        Trade t;
        t.ts = std::stoll(cols[1]);
        std::string side_str = cols[2];
        std::transform(side_str.begin(), side_str.end(), side_str.begin(), ::tolower);
        t.side = (side_str == "buy") ? 1 : -1;
        t.price = std::stod(cols[3]);
        t.qty = std::stod(cols[4]);

        if (t.price > 0 && t.qty > 0) {
            trades.push_back(t);
        }
    }

    std::cout << "[parser] Trade rows loaded: " << trades.size() << "\n";
    if (!trades.empty()) {
        std::cout << "[parser] Trade time range: " << trades.front().ts
                  << " to " << trades.back().ts << "\n";
    }
    return trades;
}
