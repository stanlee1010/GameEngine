#include "Logger.h"

#include <SFML/System/Err.hpp>

#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

Logger::~Logger()
{
    close();
}

bool Logger::open(const std::string& path)
{
    close();
    m_file.open(path, std::ios::out | std::ios::app);
    if (!m_file.is_open())
        return false;

    // Send SFML's own diagnostic messages to the same log file
    m_oldSfmlErr = sf::err().rdbuf(m_file.rdbuf());
    m_file << "\n========== Session started " << timestamp() << " ==========\n";
    return true;
}

void Logger::close()
{
    if (m_oldSfmlErr) {
        sf::err().rdbuf(m_oldSfmlErr);
        m_oldSfmlErr = nullptr;
    }
    if (m_file.is_open())
        m_file.close();
}

void Logger::log(LogLevel level, const std::string& message)
{
    if (static_cast<int>(level) < static_cast<int>(m_minLevel))
        return;

    const std::string line = "[" + timestamp() + "] [" + levelName(level) + "] " + message;

    if (m_console) {
        std::ostream& out = (level == LogLevel::Error) ? std::cerr : std::cout;
        out << line << '\n';
    }
    if (m_toFile && m_file.is_open())
        m_file << line << std::endl;
}

std::string Logger::timestamp()
{
    std::time_t now = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &now);
#else
    localtime_r(&now, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

const char* Logger::levelName(LogLevel level)
{
    switch (level) {
    case LogLevel::Info:    return "INFO";
    case LogLevel::Warning: return "WARNING";
    case LogLevel::Error:   return "ERROR";
    }
    return "?";
}

void Logger::fatal(const std::string& message)
{
    log(LogLevel::Error, message);
    showMessageBox("Engine error", message);
}

void Logger::showMessageBox(const std::string& title, const std::string& message)
{
#ifdef _WIN32
    MessageBoxA(nullptr, message.c_str(), title.c_str(), MB_OK | MB_ICONERROR);
#else
    std::cerr << title << ": " << message << '
';
#endif
}
