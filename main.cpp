#include "include/types.hpp"
#include "include/parser.hpp"
#include "include/avellaneda_stoikov.hpp"
#include "include/backtest.hpp"
#include <iostream>
#include <filesystem>
#include <chrono>

int main(int argc, char* argv[]) {
    std::string lob_path    = (argc > 1) ? argv[1] : "data/lob.csv";
    std::string trades_path = (argc > 2) ? argv[2] : "data/trades.csv";

    std::cout << "========================================\n";
    std::cout << " Avellaneda-Stoikov HFT Backtester\n";
    std::cout << "========================================\n";
    std::cout << " LOB    : " << lob_path << "\n";
    std::cout << " Trades : " << trades_path << "\n\n";

    if (!std::filesystem::exists(lob_path)) {
        std::cerr << "[ERROR] LOB file not found: " << lob_path << "\n";
        return 1;
    }
    if (!std::filesystem::exists(trades_path)) {
        std::cerr << "[ERROR] Trades file not found: " << trades_path << "\n";
        return 1;
    }

    std::filesystem::create_directory("output");

    std::vector<OrderBook> books;
    std::vector<Trade> trades;

    auto start_time = std::chrono::high_resolution_clock::now();

    try {
        books = parse_lob(lob_path);
        trades = parse_trades(trades_path);
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << "\n";
        return 1;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto load_duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
    std::cout << "[main] Data loaded in " << load_duration.count() << " seconds\n";

    if (books.empty()) {
        std::cerr << "[ERROR] No order book data loaded.\n";
        return 1;
    }

    // Run Classic Avellaneda-Stoikov
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "Running Classic Avellaneda-Stoikov (2008)\n";
    std::cout << std::string(60, '=') << "\n";

    AvellanedaStoikovParams classic_params;
    classic_params.gamma = 0.1;
    classic_params.sigma = 0.5;
    classic_params.T = 3600.0;
    classic_params.k = 1.5;
    classic_params.A = 10.0;
    classic_params.tick_size = 0.0000001;
    classic_params.max_position = 10000.0;
    classic_params.order_qty = 1000.0;
    classic_params.use_microprice = false;

    AvellanedaStoikov classic_strategy(classic_params);
    Backtest classic_backtest(books, trades, classic_strategy);
    classic_backtest.run();
    classic_backtest.print_report();
    classic_backtest.write_equity_csv("output/equity_classic.csv");

    // Run Microprice-enhanced Avellaneda-Stoikov
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "Running Microprice-Enhanced Avellaneda-Stoikov (2018)\n";
    std::cout << std::string(60, '=') << "\n";

    AvellanedaStoikovParams micro_params = classic_params;
    micro_params.use_microprice = true;

    AvellanedaStoikov micro_strategy(micro_params);
    Backtest micro_backtest(books, trades, micro_strategy);
    micro_backtest.run();
    micro_backtest.print_report();
    micro_backtest.write_equity_csv("output/equity_micro.csv");

    std::cout << "\n========================================\n";
    std::cout << " Backtest finished successfully\n";
    std::cout << " Reports written to output/\n";
    std::cout << "   - equity_classic.csv (Classic AS 2008)\n";
    std::cout << "   - equity_micro.csv (Microprice AS 2018)\n";
    std::cout << "========================================\n";

    return 0;
}
