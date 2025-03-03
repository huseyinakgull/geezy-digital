#include "logger.hpp"
#include <Windows.h>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace utils {

    bool Logger::InitializeFileOutput(const std::string& filename) {
        std::lock_guard<std::mutex> lock(m_logMutex);

        if (m_fileLoggingEnabled) {
            CloseFileOutput();
        }

        m_logFile.open(filename);
        if (!m_logFile.is_open()) {
            return false;
        }

        m_fileLoggingEnabled = true;
        return true;
    }

    void Logger::CloseFileOutput() {
        std::lock_guard<std::mutex> lock(m_logMutex);

        if (m_fileLoggingEnabled && m_logFile.is_open()) {
            m_logFile.close();
            m_fileLoggingEnabled = false;
        }
    }

    std::string Logger::GetLevelPrefix(LogLevel level) {
        switch (level) {
        case LogLevel::INFO:    return "[INFO]";
        case LogLevel::SUCCESS: return "[SUCCESS]";
        case LogLevel::WARNING: return "[WARNING]";
        case LogLevel::ERROR:   return "[ERROR]";
        default:                return "[UNKNOWN]";
        }
    }

    int Logger::GetLevelColor(LogLevel level) {
        // Windows konsolu ile uyumlu renkler
        switch (level) {
        case LogLevel::INFO:    return 7;  // Beyaz
        case LogLevel::SUCCESS: return 10; // Ye�il
        case LogLevel::WARNING: return 14; // Sar�
        case LogLevel::ERROR:   return 12; // K�rm�z�
        default:                return 7;  // Beyaz
        }
    }

    void Logger::Log(LogLevel level, const std::string& message) {
        std::lock_guard<std::mutex> lock(m_logMutex);

        // �u anki zaman� al
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm tm_buf;
        localtime_s(&tm_buf, &in_time_t);

        // Zaman damgas� format
        std::stringstream timestamp;
        timestamp << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");

        // Renkli ��k�� i�in konsol tutamac�
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
        GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
        WORD originalAttrs = consoleInfo.wAttributes;

        // Log seviyesine g�re renk ayarla
        SetConsoleTextAttribute(hConsole, GetLevelColor(level));

        // Konsola yazd�r
        std::cout << GetLevelPrefix(level) << " [" << timestamp.str() << "] " << message << std::endl;

        // Konsol rengini s�f�rla
        SetConsoleTextAttribute(hConsole, originalAttrs);

        // Dosyaya log
        if (m_fileLoggingEnabled && m_logFile.is_open()) {
            m_logFile << GetLevelPrefix(level) << " [" << timestamp.str() << "] " << message << std::endl;
        }
    }

    void Logger::LogToFile(LogLevel level, const std::string& message) {
        std::lock_guard<std::mutex> lock(m_logMutex);

        if (!m_fileLoggingEnabled || !m_logFile.is_open()) {
            return;
        }

        // �u anki zaman� al
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm tm_buf;
        localtime_s(&tm_buf, &in_time_t);

        // Zaman damgas� format
        std::stringstream timestamp;
        timestamp << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");

        // Dosyaya log
        m_logFile << GetLevelPrefix(level) << " [" << timestamp.str() << "] " << message << std::endl;
    }

} // namespace utils