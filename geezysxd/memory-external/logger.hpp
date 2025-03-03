#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <mutex>

namespace utils {

    // Log seviyelerini tan�mlayan enum
    enum class LogLevel {
        INFO,
        SUCCESS,
        WARNING,
        ERROR
    };

    class Logger {
    public:
        // Singleton �rne�i almak i�in
        static Logger& GetInstance() {
            static Logger instance;
            return instance;
        }

        // Konsola log yazma
        void Log(LogLevel level, const std::string& message);

        // Dosyaya log yazma
        void LogToFile(LogLevel level, const std::string& message);

        // Dosya ��k���n� ba�latma
        bool InitializeFileOutput(const std::string& filename);

        // Dosya ��k���n� kapatma
        void CloseFileOutput();

        // Kopyalama ve ta��ma engelleyicileri
        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;
        Logger(Logger&&) = delete;
        Logger& operator=(Logger&&) = delete;

    private:
        Logger() : m_fileLoggingEnabled(false) {}
        ~Logger() { CloseFileOutput(); }

        std::mutex m_logMutex;
        std::ofstream m_logFile;
        bool m_fileLoggingEnabled;

        // Log seviyesi i�in yaz� �neki alma
        std::string GetLevelPrefix(LogLevel level);

        // Log seviyesi i�in renk kodu alma
        int GetLevelColor(LogLevel level);
    };

    // Kolayl�k fonksiyonlar�
    inline void LogInfo(const std::string& message) {
        Logger::GetInstance().Log(LogLevel::INFO, message);
    }

    inline void LogSuccess(const std::string& message) {
        Logger::GetInstance().Log(LogLevel::SUCCESS, message);
    }

    inline void LogWarning(const std::string& message) {
        Logger::GetInstance().Log(LogLevel::WARNING, message);
    }

    inline void LogError(const std::string& message) {
        Logger::GetInstance().Log(LogLevel::ERROR, message);
    }

} // namespace utils