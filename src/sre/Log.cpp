/*
 *  SimpleRenderEngine (https://github.com/mortennobel/SimpleRenderEngine)
 *
 *  Created by Morten Nobel-Jørgensen ( http://www.nobel-joergensen.com/ )
 *  License: MIT
 */

#include "sre/Log.hpp"
#include <iostream>
#include <cstdarg>
#include <stdexcept>
#include <fstream>
#include <sre/SDLRenderer.hpp>

namespace sre{

void
Log::SetupFiles() {
    if (areFilesSetup) return;

    Log::logPath = "last_log.txt";

    const std::string logArchiveDirectory = "log_archive";
    std::filesystem::create_directory(logArchiveDirectory);
    Log::logArchivePath = logArchiveDirectory;

    std::ostringstream logArchiveName;
    const auto now = std::chrono::system_clock::now();
    const auto t_c = std::chrono::system_clock::to_time_t(now);
    logArchiveName << "log_"
                   << std::put_time(std::localtime(&t_c), "%F_%Hh-%Mm") << ".txt";
    Log::logArchivePath.append(logArchiveName.str());

    std::ofstream logArchiveFile(Log::logArchivePath.string(), std::ios::out);
    std::ofstream logFile(Log::logPath.string(), std::ios::out);

    areFilesSetup = true;
}

    constexpr int maxErrorSize = 1024;
    char errorMsg[maxErrorSize];
    std::function<void(const char * function,const char * file, int line,LogType, std::string)> Log::logHandler = [](const char * function,const char * file, int line, LogType type, std::string msg){
        std::ostringstream logStream;
        std::ofstream logArchiveFile(Log::logArchivePath.string(), std::ios::app);
        std::ofstream logFile(Log::logPath.string(), std::ios::app);
        switch (type){
            case LogType::Verbose:
                logStream << "Verbose: ";
                logStream << file << ":" << line << " in " << function << "()\n";
                logStream << "       " << msg;
            case LogType::Info:
                // No prefix
                logStream << msg;
                break;
            case LogType::Warning:
                logStream << "Warning: ";
                logStream << file << ":" << line << " in " << function << "()\n";
                logStream << "       " << msg;
                break;
            case LogType::Error:
                logStream << "ERROR: ";
                logStream << file << ":" << line << " in " << function << "()\n";
                logStream << "       " << msg;
                break;
            case LogType::Fatal:
                if (SDLRenderer::instance != nullptr) {
                    SDLRenderer::instance->stopRecordingEvents();
                }
                logStream << "ERROR: ";
                logStream << file << ":" << line << " in " << function << "()\n";
                // Don't repeat msg sent to exception handler for std::cout
                std::cout << logStream.str() << std::endl;
                logStream << "       " << msg;
                // runtime_error prevents output at end of function
                logFile << logStream.str() << std::endl;
                logArchiveFile << logStream.str() << std::endl;
                throw std::runtime_error(msg);
                break;
            case LogType::Assert:
                if (SDLRenderer::instance != nullptr) {
                    SDLRenderer::instance->stopRecordingEvents();
                }
                logStream << "ERROR: ";
                logStream << file << ":" << line << " in " << function << "()\n";
                // Don't repeat msg sent to exception handler for std::cout
                std::cout << logStream.str() << std::endl;
                logStream << "       " << msg;
                // runtime_error prevents output at end of function
                logFile << logStream.str() << std::endl;
                logArchiveFile << logStream.str() << std::endl;
                throw std::runtime_error(msg);
                break;
        }
        std::cout << logStream.str() << std::endl;
        logFile << logStream.str() << std::endl;
        logArchiveFile << logStream.str() << std::endl;
    };

    void Log::verbose(const char * function,const char * file, int line, const char *message, ...) {
        va_list args;
        va_start(args, message);
        vsnprintf(errorMsg,maxErrorSize,message,args);
        logHandler(function,file, line, LogType::Verbose, errorMsg);
        va_end(args);
    }

    void Log::info(const char * function,const char * file, int line, const char *message, ...) {
        va_list args;
        va_start(args, message);
        vsnprintf(errorMsg,maxErrorSize,message,args);
        logHandler(function,file, line, LogType::Info, errorMsg);
        va_end(args);
    }

    void Log::warning(const char * function,const char * file, int line, const char *message, ...) {
        va_list args;
        va_start(args, message);
        vsnprintf(errorMsg,maxErrorSize,message,args);
        logHandler(function,file, line, LogType::Warning, errorMsg);
        va_end(args);
    }

    void Log::error(const char * function,const char * file, int line, const char *message, ...) {
        va_list args;
        va_start(args, message);
        vsnprintf(errorMsg,maxErrorSize,message,args);
        logHandler(function,file, line, LogType::Error, errorMsg);
        va_end(args);
    }

    void Log::fatal(const char * function,const char * file, int line, const char *message, ...) {
        va_list args;
        va_start(args, message);
        vsnprintf(errorMsg,maxErrorSize,message,args);
        logHandler(function,file, line, LogType::Fatal, errorMsg);
        va_end(args);
    }

    void Log::sreAssert(const char * function,const char * file, int line, std::string msg) {
        logHandler(function,file, line, LogType::Assert, msg);
    }
}
