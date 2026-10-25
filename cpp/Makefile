CXX = clang++
CXXFLAGS = -std=c++17 -Wall -Wextra -O3 -Iinclude

SRC = src/fcfs.cpp src/sjf.cpp src/round_robin.cpp src/adaptive_rr.cpp src/ml_hybrid.cpp src/simulator.cpp
MAIN_SRC = src/main.cpp
TEST_SRC = tests/test_schedulers.cpp

TARGET = scheduler_engine
TEST_TARGET = test_runner

all: $(TARGET) $(TEST_TARGET)

$(TARGET): $(SRC) $(MAIN_SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC) $(MAIN_SRC)

$(TEST_TARGET): $(SRC) $(TEST_SRC)
	$(CXX) $(CXXFLAGS) -o $(TEST_TARGET) $(SRC) $(TEST_SRC)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

clean:
	rm -f $(TARGET) $(TEST_TARGET) *.o

.PHONY: all test clean
