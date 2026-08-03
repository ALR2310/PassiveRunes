#include "Config.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace {
    std::string Trim(const std::string& s) {
        size_t start = s.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    }
}

Config& Config::Instance() {
    static Config instance;
    return instance;
}

void Config::Load(const std::string& iniPath) {
    std::ifstream file(iniPath);
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
        std::string trimmed = Trim(line);
        if (trimmed.empty() || trimmed[0] == '#' || trimmed[0] == ';' || trimmed[0] == '[') {
            continue;
        }
        size_t eq = trimmed.find('=');
        if (eq == std::string::npos) continue;

        std::string key = Trim(trimmed.substr(0, eq));
        std::string value = Trim(trimmed.substr(eq + 1));
        m_values[key] = value;
    }
}

int Config::GetInt(const std::string& key, int defaultValue) const {
    auto it = m_values.find(key);
    if (it == m_values.end()) return defaultValue;
    try {
        return std::stoi(it->second);
    } catch (...) {
        return defaultValue;
    }
}

bool Config::GetBool(const std::string& key, bool defaultValue) const {
    auto it = m_values.find(key);
    if (it == m_values.end()) return defaultValue;
    std::string v = it->second;
    std::transform(v.begin(), v.end(), v.begin(), ::tolower);
    if (v == "1" || v == "true" || v == "yes") return true;
    if (v == "0" || v == "false" || v == "no") return false;
    return defaultValue;
}

std::string Config::GetString(const std::string& key, const std::string& defaultValue) const {
    auto it = m_values.find(key);
    return it == m_values.end() ? defaultValue : it->second;
}
