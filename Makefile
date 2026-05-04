CXX      := clang++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Wpedantic -Wno-unused-parameter -I./include
TARGET   := hft_backtest
SRC      := main.cpp

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRC) include/types.hpp include/parser.hpp include/microprice.hpp include/avellaneda_stoikov.hpp include/backtest.hpp
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)
	@echo "Build OK -> ./$(TARGET)"

run: all
	@mkdir -p output
	./$(TARGET) data/lob.csv data/trades.csv

clean:
	rm -f $(TARGET)
	rm -rf output
