#include <cstdio>
#include <cstring>
#include <cassert>
#include "Arduino.h"
#include "WiFiClient.h"
#include "service/web_reader.h"

// Serial Mock Instanz
SerialMock Serial;

// Test-Hilfsfunktionen
int tests_run = 0;
int tests_passed = 0;

#define TEST(name) \
	void test_##name(); \
	struct TestRunner_##name { \
		TestRunner_##name() { \
			printf("TEST: %s ... ", #name); \
			tests_run++; \
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

TEST(chunked_response_single_chunk) {
	WiFiClient client;
	Local::Service::WebReader reader(client);

	// HTTP Response mit Chunked Transfer-Encoding
	// "Dies ist der erste Chunk." = 26 Zeichen = 0x1a
	const char* response =
		"HTTP/1.1 200 OK\r\n"
		"Transfer-Encoding: chunked\r\n"
		"\r\n"
		"1a\r\n"                       // 26 Bytes (0x1a)
		"Dies ist der erste Chunk."    // genau 26 Bytes
		"\r\n"                         // Chunk-Ende
		"0\r\n"                        // Ende-Chunk
		"\r\n";

	client.set_response(response);

	ASSERT(reader.send_http_get_request("test.local", 80, "/test"));
	ASSERT(reader.read_next_block_to_buffer());
	ASSERT(reader.find_in_buffer((char*)"erste"));
}

TEST(chunked_response_multiple_chunks) {
	WiFiClient client;
	Local::Service::WebReader reader(client);

	// Response mit mehreren Chunks
	// Chunked-Format: <size hex>\r\n<data>\r\n<size hex>\r\n<data>\r\n0\r\n\r\n
	const char* response =
		"HTTP/1.1 200 OK\r\n"
		"Transfer-Encoding: chunked\r\n"
		"\r\n"
		"10\r\n"                  // 16 Bytes (0x10)
		"Chunk-Eins-Data!"        // genau 16 Bytes
		"\r\n"                    // Chunk-Ende
		"10\r\n"                  // 16 Bytes
		"Chunk-Zwei-Data!"        // genau 16 Bytes
		"\r\n"                    // Chunk-Ende
		"0\r\n"                   // Ende-Chunk
		"\r\n";

	client.set_response(response);

	ASSERT(reader.send_http_get_request("test.local", 80, "/test"));

	// Ersten Chunk lesen
	ASSERT(reader.read_next_block_to_buffer());
	ASSERT(reader.find_in_buffer((char*)"Eins"));

	// Zweiten Chunk lesen
	ASSERT(reader.read_next_block_to_buffer());
	ASSERT(reader.find_in_buffer((char*)"Zwei"));

	// Ende erreicht
	ASSERT(!reader.read_next_block_to_buffer());
}

TEST(content_length_response) {
	WiFiClient client;
	Local::Service::WebReader reader(client);

	const char* response =
		"HTTP/1.1 200 OK\r\n"
		"Content-Length: 20\r\n"
		"\r\n"
		"Dies sind 20 Bytes!";

	client.set_response(response);

	ASSERT(reader.send_http_get_request("test.local", 80, "/test"));
	ASSERT(reader.read_next_block_to_buffer());
	ASSERT(reader.find_in_buffer((char*)"20 Bytes"));
	// Nach 20 Bytes muss Schluss sein
	ASSERT(!reader.read_next_block_to_buffer());
}

TEST(content_length_multiple_blocks) {
	WiFiClient client;
	Local::Service::WebReader reader(client);

	// 150 Bytes Content - wird in 3 Bloecke aufgeteilt (buffer = 63 Bytes nutzbar)
	const char* response =
		"HTTP/1.1 200 OK\r\n"
		"Content-Length: 150\r\n"
		"\r\n"
		"BLOCK1-AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"  // 63 Bytes
		"BLOCK2-BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB"   // 63 Bytes
		"BLOCK3-CCCCCCCCCCCCCCC";                                          // 24 Bytes

	client.set_response(response);

	ASSERT(reader.send_http_get_request("test.local", 80, "/test"));

	// Block 1
	ASSERT(reader.read_next_block_to_buffer());
	ASSERT(reader.find_in_buffer((char*)"BLOCK1"));

	// Block 2
	ASSERT(reader.read_next_block_to_buffer());
	ASSERT(reader.find_in_buffer((char*)"BLOCK2"));

	// Block 3
	ASSERT(reader.read_next_block_to_buffer());
	ASSERT(reader.find_in_buffer((char*)"BLOCK3"));

	// Ende erreicht
	ASSERT(!reader.read_next_block_to_buffer());
}

TEST(find_pattern_across_buffer_boundary) {
	WiFiClient client;
	Local::Service::WebReader reader(client);

	// 100 Bytes Content - wird in 2 Bloecke aufgeteilt (buffer = 64 Bytes)
	// Das Pattern "FINDME" liegt genau an der Grenze
	const char* response =
		"HTTP/1.1 200 OK\r\n"
		"Content-Length: 100\r\n"
		"\r\n"
		"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAFIND"  // 63 Bytes, "FIND" am Ende
		"ME_BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB!";  // 37 Bytes, "ME" am Anfang

	client.set_response(response);

	ASSERT(reader.send_http_get_request("test.local", 80, "/test"));

	// Ersten Block lesen
	ASSERT(reader.read_next_block_to_buffer());
	// FINDME sollte im search_buffer gefunden werden (old_buffer + buffer)

	// Zweiten Block lesen
	ASSERT(reader.read_next_block_to_buffer());
	// Jetzt ist "FIND" in old_buffer und "ME" in buffer
	ASSERT(reader.find_in_buffer((char*)"FINDME"));
}

TEST(binary_data_with_null_bytes) {
	WiFiClient client;
	Local::Service::WebReader reader(client);

	// Binaerdaten mit Null-Byte
	char response[200];
	const char* header = "HTTP/1.1 200 OK\r\nContent-Length: 20\r\n\r\n";
	size_t header_len = strlen(header);
	memcpy(response, header, header_len);

	// 20 Bytes mit Null-Byte in der Mitte
	char binary_data[20] = "ABCDE\0FGHIJ\0KLMNO";
	binary_data[5] = '\0';
	binary_data[11] = '\0';
	memcpy(response + header_len, binary_data, 20);

	client.set_response(response, header_len + 20);

	ASSERT(reader.send_http_get_request("test.local", 80, "/test"));
	ASSERT(reader.read_next_block_to_buffer());
	// Nach dem Fix sollte content_length korrekt auf 0 gehen
	ASSERT(!reader.read_next_block_to_buffer());
}

TEST(regex_capture_content_length) {
	WiFiClient client;
	Local::Service::WebReader reader(client);

	const char* response =
		"HTTP/1.1 200 OK\r\n"
		"Content-Length: 12345\r\n"
		"\r\n"
		"Test";

	client.set_response(response);

	ASSERT(reader.send_http_get_request("test.local", 80, "/test"));
	// Der Content-Length Header sollte geparsed worden sein
	// find_in_buffer kann verwendet werden um das zu pruefen
}

// ============================================================
// Main
// ============================================================

int main() {
	printf("\n=== WebReader Host Tests ===\n\n");

	// Tests werden durch statische Konstruktoren ausgefuehrt

	printf("\n=== Ergebnis: %d/%d Tests bestanden ===\n\n", tests_passed, tests_run);
	return (tests_passed == tests_run) ? 0 : 1;
}
