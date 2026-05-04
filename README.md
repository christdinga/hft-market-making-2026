```markdown
# HFT Backtester — Avellaneda-Stoikov Market Making Strategy

## Architecture

```
hft/
├── main.cpp                       ← Entry point
├── Makefile                       ← Build for macOS/clang++
├── include/
│   ├── types.hpp                  ← Core types (OrderBook, Trade, Fill, PnL)
│   ├── parser.hpp                 ← CSV parsers (LOB + Trades)
│   ├── microprice.hpp             ← Micro-price estimator (Stoikov 2018)
│   ├── avellaneda_stoikov.hpp     ← Avellaneda-Stoikov strategy (2008)
│   └── backtest.hpp               ← Backtest engine + execution simulation
├── data/
│   ├── lob.csv                    ← Order book data (25 levels)
│   └── trades.csv                 ← Trade tape data
└── output/
    ├── equity_classic.csv         ← PnL curve (Classic strategy)
    └── equity_micro.csv           ← PnL curve (Microprice strategy)
```

## Strategies

### 1. Classic Avellaneda-Stoikov (2008)

The market maker posts bid and ask quotes around a **reservation price** that accounts for inventory risk.

| Parameter | Formula | Interpretation |
|-----------|---------|----------------|
| Reservation Price | `r = S - q × γ × σ² × τ` | Price adjusted for inventory |
| Optimal Spread | `δ = γ × σ² × τ + (2/γ) × log(1 + γ/k)` | Half-spread around reservation price |
| Arrival Intensity | `λ(δ) = A × exp(-k × δ)` | Order arrival rate as function of distance |

**Quote placement:**
```
bid = reservation - spread/2
ask = reservation + spread/2
```

### 2. Microprice-Enhanced Avellaneda-Stoikov (2018)

Extends the classic model by incorporating **order book imbalance** to better predict short-term price movements.

| Enhancement | Formula | Interpretation |
|-------------|---------|----------------|
| Micro-price | `P_micro = M + (I - 0.5) × S` | Fair price adjusted for imbalance |
| Combined Price | `P = 0.7 × P_reservation + 0.3 × P_micro` | Blend of inventory-driven and imbalance-driven prices |

### Execution Criterion

Fill occurs when the market price crosses the order level:
- Limit BUY → fill if `best_ask ≤ bid_price`
- Limit SELL → fill if `best_bid ≥ ask_price`

### Risk Management

- `max_position`: Maximum allowed inventory (liquidation if exceeded)
- `order_qty`: Quantity per limit order
- Inventory-based quote adjustment prevents excessive accumulation

## Compilation (macOS / Linux)

### Prerequisites

```bash
# macOS
xcode-select --install   # Xcode Command Line Tools (clang++)

# Linux
sudo apt install build-essential   # g++
```

### Build & Run

```bash
make          # compile → ./hft_backtest
make run      # compile + run with data/lob.csv and data/trades.csv
make clean    # remove artifacts and output files
```

### Manual Execution

```bash
./hft_backtest data/lob.csv data/trades.csv
```

## Configuration Parameters

Parameters can be modified in `main.cpp`:

| Parameter | Default | Description |
|-----------|---------|-------------|
| `gamma` | 1.0 | Risk aversion coefficient |
| `sigma` | 0.5 | Volatility |
| `T` | 60.0 | Time horizon (seconds) |
| `k` | 1.5 | Order arrival decay factor |
| `A` | 10.0 | Base arrival rate |
| `tick_size` | 1e-7 | Minimum price increment |
| `max_position` | 10,000 | Maximum allowed inventory |
| `order_qty` | 1,000 | Quantity per limit order |
| `use_microprice` | false | Enable microprice enhancement |

## Outputs

| File | Description |
|------|-------------|
| `output/equity_classic.csv` | Timestamp + cumulative equity (Classic strategy) |
| `output/equity_micro.csv` | Timestamp + cumulative equity (Microprice strategy) |
| Console | Summary report: PnL, drawdown, Sharpe ratio, win rate, turnover |

### Sample Console Output

```
========================================================================
  AVELIANEDA-STOIKOV BACKTEST REPORT
========================================================================
  Total Fills        : 11752
  Total PnL          : 3.022700
  Max Drawdown       : 0.440200
  Peak Equity        : 3.023950
  Turnover           : 103192.847100
  Sharpe Ratio       : 3.0000
  Win Rate (%)       : 76.3214
  Final Position     : 0.0000
========================================================================
```

## Experimental Results

| Strategy | PnL | Fills | Max Drawdown | Win Rate | Sharpe |
|----------|-----|-------|--------------|----------|--------|
| Classic (2008) | +3.02 | 11,752 | 0.44 | 76.3% | 3.00 |
| Microprice (2018) | +4.49 | 12,572 | 0.21 | 77.0% | 3.00 |

The microprice-enhanced strategy shows:
- **Higher PnL** (+49% improvement)
- **Lower drawdown** (52% reduction)
- **Slightly better win rate**

## References

- Avellaneda, M. and Stoikov, S. (2008). High-frequency trading in a limit order book. *Quantitative Finance*, 8(3), 217-224.
- Stoikov, S. (2018). The micro-price: A high-frequency estimator of future prices. *Available at SSRN 2970694*.

## Suggested Extensions

- Add latency modeling (delay between signal and fill)
- Parameter optimization via grid search or Bayesian methods
- Realistic taker/maker fees
- Multi-asset market making
- Hidden order types (iceberg orders)
- Connection to simulated exchange via ZeroMQ

## License

Academic project — for educational purposes only.
```
