#!/usr/bin/env bash
cd "$(dirname "$0")/test"
g++ -std=c++17 -I. -o test_web_reader test_web_reader.cpp && ./test_web_reader
rm -f test_web_reader
