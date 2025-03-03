#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <mutex>

namespace utils {

    // Log seviyelerini tanýmlayan enum
    enum class LogLevel {
        INFO,
        SUCCESS,
        WARNING,
        ERROR
    };

    class Logger {
    public:
        // Singleton örneði almak için
        static Logger& GetInstance() {
            static Logger instance;
            return instance;
        }

        // Konsola log yazma
        void Log(LogLevel level, const std::string& message);

        // Dosyaya log yazma
        void LogToFile(LogLevel level, const std::string& message);

        // Dosya çýkýþýný baþlatma
        bool InitializeFileOutput(const std::string& filename);

        // Dosya çýkýþýný kapatma
        void CloseFileOutput();

        // Kopyalama ve taþýma engelleyicileri
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

        // Log seviyesi için yazý öneki alma
        std::string GetLevelPrefix(LogLevel level);

        // Log seviyesi için renk kodu alma
        int GetLevelColor(LogLevel level);
    };

    // Kolaylýk fonksiyonlarý
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