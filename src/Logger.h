#pragma once

#include <fstream>
#include <string>

enum class LogLevel { Info, Warning, Error };

// Writes diagnostic messages to the console and/or a log file.
class Logger {
public:
    ~Logger();

    bool open(const std::string& path);
    void close();

    void log(LogLevel level, const std::string& message);
    void info(const std::string& message)    { log(LogLevel::Info, message); }
    void warning(const std::string& message) { log(LogLevel::Warning, message); }
    void error(const std::string& message)   { log(LogLevel::Error, message); }

    // Logs an error and also shows it to the user in a message box.
    void fatal(const std::string& message);
    static void showMessageBox(const std::string& title, const std::string& message);

    void setMinimumLevel(LogLevel level) { m_minLevel = level; }
    void setConsoleOutput(bool enabled) { m_console = enabled; }
    void setFileOutput(bool enabled)    { m_toFile = enabled; }

private:
    static std::string timestamp();
    static const char* levelName(LogLevel level);

    std::ofstream m_file;
    std::streambuf* m_oldSfmlErr = nullptr;
    LogLevel m_minLevel = LogLevel::Info;
    bool m_console = true;
    bool m_toFile = true;
};
