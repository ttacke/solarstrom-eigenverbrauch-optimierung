#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include <map>

// Mock-Dateisystem fuer Tests
class MockFileSystem {
public:
	struct MockFile {
		std::string content;
		size_t pos = 0;
	};
	std::map<std::string, MockFile> files;

	void add_file(const char* name, const char* content) {
		files[name] = {content, 0};
	}

	void add_file(const char* name, const char* content, size_t len) {
		files[name] = {std::string(content, len), 0};
	}

	void clear() {
		files.clear();
	}
};

extern MockFileSystem mock_fs;

// File Klasse
class File {
	std::string name;
	bool valid = false;

public:
	File() = default;
	File(const char* n) : name(n), valid(mock_fs.files.count(n) > 0) {}

	explicit operator bool() const { return valid; }

	size_t size() {
		if (!valid) return 0;
		return mock_fs.files[name].content.size();
	}

	void seek(int pos) {
		if (valid) mock_fs.files[name].pos = pos;
	}

	int read(uint8_t* buf, int len) {
		if (!valid) return 0;
		auto& f = mock_fs.files[name];
		size_t to_read = std::min((size_t)len, f.content.size() - f.pos);
		memcpy(buf, f.content.data() + f.pos, to_read);
		f.pos += to_read;
		return (int)to_read;
	}

	void close() {
		valid = false;
	}
};

// SD Klasse
class SDClass {
	bool initialized = false;

public:
	bool begin(int) {
		initialized = true;
		return true;
	}

	bool exists(const char* name) {
		return mock_fs.files.count(name) > 0;
	}

	File open(const char* name) {
		return File(name);
	}
};

extern SDClass SD;
