#pragma once
#include <string>
#include <map>

// Minimal flat INI reader: "key=value" per line, "#" or ";" starts a comment.
// Section headers ("[Section]") are accepted but ignored - all keys share one namespace.
class Config {
public:
    static Config& Instance();

    void Load(const std::string& iniPath);

    int GetInt(const std::string& key, int defaultValue) const;
    bool GetBool(const std::string& key, bool defaultValue) const;
    std::string GetString(const std::string& key, const std::string& defaultValue) const;

private:
    Config() = default;
    std::map<std::string, std::string> m_values;
};
