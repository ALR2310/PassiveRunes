#pragma once
#include <string>
#include <fstream>
#include <mutex>

class Logger {
public:
    static Logger& Instance();

    void Init(const std::string& logDir, bool enabled);
    void Log(const std::string& message);

private:
    Logger() = default;
    std::mutex m_mutex;
    std::ofstream m_textLog;
    bool m_initialized = false;
};
