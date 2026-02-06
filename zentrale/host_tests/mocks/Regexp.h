#pragma once
#include <regex>
#include <string>
#include <cstring>
#include <vector>

class MatchState {
	std::string target;
	std::vector<std::string> captures;
	std::smatch match_result;

	// Konvertiert Lua-Pattern zu std::regex
	// Unterstützt nur die im Projekt verwendeten Patterns
	std::string lua_to_regex(const char* lua_pattern) {
		std::string result;
		const char* p = lua_pattern;
		while (*p) {
			if (*p == '[' && *(p+1) == '-' && *(p+2) == ']') {
				// Lua [-] = 0 oder mehr '-' (minimal) -> -? fuer 0 oder 1
				result += "-?";
				p += 3;
			} else if (*p == '%' && *(p+1)) {
				// Lua %d, %s etc -> PCRE \d, \s
				char cls = *(p+1);
				if (cls == 'd') result += "\\d";
				else if (cls == 's') result += "\\s";
				else if (cls == 'w') result += "\\w";
				else result += cls; // %%, %(, etc -> literal
				p += 2;
			} else if (*p == ' ' && *(p+1) == '*') {
				// Lua " *" = 0+ Leerzeichen
				result += " *";
				p += 2;
			} else {
				result += *p;
				p++;
			}
		}
		return result;
	}

public:
	void Target(const char* str) {
		target = str;
		captures.clear();
	}

	char Match(const char* lua_pattern) {
		try {
			std::string regex_pattern = lua_to_regex(lua_pattern);
			std::regex re(regex_pattern);
			if (std::regex_search(target, match_result, re)) {
				captures.clear();
				for (size_t i = 1; i < match_result.size(); i++) {
					captures.push_back(match_result[i].str());
				}
				return 1;
			}
		} catch (const std::regex_error& e) {
			// Regex Fehler - kein Match
		}
		return 0;
	}

	int capture_len(int index) {
		if (index < 0 || index >= (int)captures.size()) return 0;
		return (int)captures[index].length();
	}

	void GetCapture(char* buffer, int index) {
		if (index >= 0 && index < (int)captures.size()) {
			strcpy(buffer, captures[index].c_str());
		} else {
			buffer[0] = '\0';
		}
	}

	// Fuer Kompatibilitaet
	int MatchStart = 0;
	int MatchLength = 0;
};
