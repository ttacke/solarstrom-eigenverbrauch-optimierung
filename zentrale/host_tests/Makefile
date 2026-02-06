CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g -I./mocks -I..

TARGETS = test_web_reader test_file_reader

.PHONY: all clean test

all: $(TARGETS)

test_web_reader: tests/web_reader_test.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

test_file_reader: tests/file_reader_test.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

test: $(TARGETS)
	@echo "=== Running all tests ==="
	@./test_web_reader && ./test_file_reader

clean:
	rm -f $(TARGETS)
