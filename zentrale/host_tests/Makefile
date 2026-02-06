CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g -I./mocks -I..

TESTS = tests/web_reader_test.cpp
TARGET = test_runner

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(TESTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
