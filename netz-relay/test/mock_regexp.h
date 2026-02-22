#pragma once
#include <cstring>
#include <regex>
#include <string>

class MatchState {
    std::string target;
    std::smatch match;
public:
    void Target(const char* t) { target = t; }
    char Match(const char* pattern) {
        try {
            std::string p = pattern;
            // Arduino Regexp -> std::regex Konvertierung (minimal)
            // [-] -> - (Zeichenklasse)
            size_t pos;
            while ((pos = p.find("[-]")) != std::string::npos) {
                p.replace(pos, 3, "-");
            }
            // * am Anfang einer Gruppe -> \\s*
            std::regex re(p);
            if (std::regex_search(target, match, re)) {
                return 1;
            }
        } catch (...) {}
        return 0;
    }
    void GetCapture(char* buf, int index) {
        if (match.size() > (size_t)(index + 1)) {
            strcpy(buf, match[index + 1].str().c_str());
        } else {
            buf[0] = '\0';
        }
    }
};
