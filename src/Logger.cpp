#include "Logger.h"
#include <windows.h>
#include <ctime>

Logger& Logger::Instance() {
    static Logger instance;
    return instance;
}

void Logger::Init(const std::string& logDir, bool enabled) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_initialized || !enabled) return;

    CreateDirectoryA(logDir.c_str(), nullptr);
    m_textLog.open(logDir + "\\PassiveRunes.log", std::ios::app);
    m_initialized = true;
}

void Logger::Log(const std::string& message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_textLog.is_open()) return;

    std::time_t now = std::time(nullptr);
    char timeBuf[32];
    ctime_s(timeBuf, sizeof(timeBuf), &now);
    std::string timeStr(timeBuf);
    if (!timeStr.empty() && timeStr.back() == '\n') timeStr.pop_back();

    m_textLog << "[" << timeStr << "] " << message << std::endl;
}
