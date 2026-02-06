#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <algorithm>

// Arduino Typen
typedef uint8_t byte;

// Pin-Konstanten
const int D2 = 2;

// Serial Mock
class SerialMock {
public:
	void begin(int) {}
	void print(const char* s) { printf("%s", s); }
	void print(char c) { printf("%c", c); }
	void print(int i) { printf("%d", i); }
	void println(const char* s) { printf("%s\n", s); }
	void println(int i) { printf("%d\n", i); }
	void println() { printf("\n"); }
	void printf(const char* fmt, ...) {
		va_list args;
		va_start(args, fmt);
		vprintf(fmt, args);
		va_end(args);
	}
};

extern SerialMock Serial;

// Arduino Funktionen
inline void delay(unsigned long) {
}

inline void yield() {
	// No-op im Test
}

inline unsigned long millis() {
	static unsigned long counter = 0;
	return counter += 10;
}

// String Klasse (minimal)
class String {
	char* data;
	size_t len;
public:
	String() : data(nullptr), len(0) {}
	String(const char* s) {
		len = strlen(s);
		data = new char[len + 1];
		strcpy(data, s);
	}
	~String() { delete[] data; }
	const char* c_str() const { return data ? data : ""; }
	size_t length() const { return len; }
};
