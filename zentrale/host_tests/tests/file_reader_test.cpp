#include <cstdio>
#include <cstring>
#include <cassert>
#include <string>
#include "Arduino.h"
#include "SD.h"
#include "service/file_reader.h"

// Mock-Instanzen
SerialMock Serial;
SDClass SD;
MockFileSystem mock_fs;

// Test-Hilfsfunktionen
int tests_run = 0;
int tests_passed = 0;

#define TEST(name) \
	void test_##name(); \
	struct TestRunner_##name { \
		TestRunner_##name() { \
			printf("TEST: %s ... ", #name); \
			tests_run++; \
			mock_fs.clear(); \
			test_##name(); \
			tests_passed++; \
			printf("OK\n"); \
		} \
	} test_runner_##name; \
	void test_##name()

#define ASSERT(cond) \
	do { if (!(cond)) { printf("FAILED\n  Assertion failed: %s\n  at %s:%d\n", #cond, __FILE__, __LINE__); exit(1); } } while(0)

#define ASSERT_EQ(a, b) \
	do { if ((a) != (b)) { printf("FAILED\n  Expected %d, got %d\n  at %s:%d\n", (int)(b), (int)(a), __FILE__, __LINE__); exit(1); } } while(0)

#define ASSERT_STREQ(a, b) \
	do { if (strcmp((a), (b)) != 0) { printf("FAILED\n  Expected \"%s\", got \"%s\"\n  at %s:%d\n", (b), (a), __FILE__, __LINE__); exit(1); } } while(0)

// ============================================================
// Tests
// ============================================================

TEST(open_nonexistent_file) {
	Local::Service::FileReader reader;
	ASSERT(!reader.open_file_to_read("/nonexistent.txt"));
}

TEST(open_and_read_small_file) {
	mock_fs.add_file("/test.txt", "Hello World!");

	Local::Service::FileReader reader;
	ASSERT(reader.open_file_to_read("/test.txt"));
	ASSERT_EQ(reader.get_file_size(), 12);
	ASSERT(reader.read_next_block_to_buffer());
	ASSERT(reader.find_in_buffer((char*)"(Hello)"));
	ASSERT_STREQ(reader.finding_buffer, "Hello");
	reader.close_file();
}

TEST(read_multiple_blocks) {
	// 300 Bytes - wird in 3 Bloecke aufgeteilt (buffer = 127 Bytes nutzbar)
	std::string content;
	content += "BLOCK1-";
	content += std::string(120, 'A');  // 127 Bytes fuer Block 1
	content += "BLOCK2-";
	content += std::string(120, 'B');  // 127 Bytes fuer Block 2
	content += "BLOCK3-";
	content += std::string(40, 'C');   // Rest fuer Block 3

	mock_fs.add_file("/big.txt", content.c_str(), content.size());

	Local::Service::FileReader reader;
	ASSERT(reader.open_file_to_read("/big.txt"));

	// Block 1
	ASSERT(reader.read_next_block_to_buffer());
	ASSERT(reader.find_in_buffer((char*)"(BLOCK1)"));

	// Block 2
	ASSERT(reader.read_next_block_to_buffer());
	ASSERT(reader.find_in_buffer((char*)"(BLOCK2)"));

	// Block 3
	ASSERT(reader.read_next_block_to_buffer());
	ASSERT(reader.find_in_buffer((char*)"(BLOCK3)"));

	// Ende
	ASSERT(!reader.read_next_block_to_buffer());
	reader.close_file();
}

TEST(read_lines) {
	mock_fs.add_file("/lines.txt", "Zeile Eins\nZeile Zwei\nZeile Drei\n");

	Local::Service::FileReader reader;
	ASSERT(reader.open_file_to_read("/lines.txt"));

	ASSERT(reader.read_next_line_to_buffer());
	ASSERT(reader.find_in_buffer((char*)"(Eins)"));

	ASSERT(reader.read_next_line_to_buffer());
	ASSERT(reader.find_in_buffer((char*)"(Zwei)"));

	ASSERT(reader.read_next_line_to_buffer());
	ASSERT(reader.find_in_buffer((char*)"(Drei)"));

	ASSERT(!reader.read_next_line_to_buffer());
	reader.close_file();
}

TEST(find_pattern_across_block_boundary) {
	// FINDME liegt an der Grenze zwischen Block 1 und 2
	char content[200];
	memset(content, 'X', 124);      // 124 x X
	memcpy(content + 124, "FIND", 4); // Block 1 endet mit "FIND"
	memcpy(content + 128, "ME", 2);   // Block 2 startet mit "ME"
	memset(content + 130, 'Y', 50);   // Rest
	content[180] = '\0';

	mock_fs.add_file("/boundary.txt", content, 180);

	Local::Service::FileReader reader;
	ASSERT(reader.open_file_to_read("/boundary.txt"));

	ASSERT(reader.read_next_block_to_buffer());  // Liest 127 Bytes
	ASSERT(reader.read_next_block_to_buffer());  // Liest Rest
	// search_buffer = old_buffer (127) + buffer (53) = 180 Bytes
	// "FINDME" sollte gefunden werden
	ASSERT(reader.find_in_buffer((char*)"(FINDME)"));
	reader.close_file();
}

TEST(multiple_capture_groups) {
	mock_fs.add_file("/data.txt", "name=Alice age=30 city=Berlin");

	Local::Service::FileReader reader;
	ASSERT(reader.open_file_to_read("/data.txt"));
	ASSERT(reader.read_next_block_to_buffer());

	// Regex mit 3 Capture-Gruppen
	ASSERT(reader.find_in_buffer((char*)"name=(%w+) age=(%d+) city=(%w+)"));
	ASSERT_STREQ(reader.finding_buffer, "Alice");

	ASSERT(reader.fetch_next_finding());
	ASSERT_STREQ(reader.finding_buffer, "30");

	ASSERT(reader.fetch_next_finding());
	ASSERT_STREQ(reader.finding_buffer, "Berlin");

	ASSERT(!reader.fetch_next_finding());
	reader.close_file();
}

TEST(empty_file) {
	mock_fs.add_file("/empty.txt", "");

	Local::Service::FileReader reader;
	ASSERT(reader.open_file_to_read("/empty.txt"));
	ASSERT_EQ(reader.get_file_size(), 0);
	ASSERT(!reader.read_next_block_to_buffer());
	reader.close_file();
}

// ============================================================
// Main
// ============================================================

int main() {
	printf("\n=== FileReader Host Tests ===\n\n");
	printf("\n=== Ergebnis: %d/%d Tests bestanden ===\n\n", tests_passed, tests_run);
	return (tests_passed == tests_run) ? 0 : 1;
}
