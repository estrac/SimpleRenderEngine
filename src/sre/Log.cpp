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
Log::Setup(const bool& verbose) {
    isVerbose = verbose;
    if (isSetup) return;

    Log::logPath = "last_log.txt";
    Log::eventsPath = "last_events.txt";

    const std::string logArchiveDirectory = "log_archive";
    std::filesystem::create_directory(logArchiveDirectory);

    std::ostringstream archiveBaseName;
    const auto now = std::chrono::system_clock::now();
    const auto t_c = std::chrono::system_clock::to_time_t(now);
    archiveBaseName << std::put_time(std::localtime(&t_c), "%F_%Hh-%Mm-%Ss")
                    << ".txt";

    Log::logArchivePath = logArchiveDirectory;
    std::ostringstream logArchiveName;
    logArchiveName << "log_" << archiveBaseName.str();
    Log::logArchivePath.append(logArchiveName.str());

    Log::eventsArchivePath = logArchiveDirectory;
    std::ostringstream eventsArchiveName;
    eventsArchiveName << "events_" << archiveBaseName.str();
    Log::eventsArchivePath.append(eventsArchiveName.str());

    if ((std::filesystem::exists(Log::logPath)
                              && !std::filesystem::remove(Log::logPath))
                || (std::filesystem::exists(Log::logArchivePath)
                              && !std::filesystem::remove(Log::logArchivePath))) {
        std::stringstream errorStream;
        errorStream << "Log file(s) '" << Log::logPath.string() << "' and/or '"
            << Log::logArchivePath.string() << "' could not be removed. Please"
            << " move, delete, or change permissions of the file(s)." << std::endl;
        LOG_ERROR(errorStream.str().c_str());
    }

    isSetup = true;
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
                logStream << file << ":" << line << " in " << function << "()";
                if (msg.size() > 0) logStream << "\n       " << msg;
                break;
            case LogType::Info:
                // No prefix
                logStream << msg;
                std::cout << logStream.str() << std::endl;
                break;
            case LogType::Warning:
                logStream << "Warning: ";
                logStream << file << ":" << line << " in " << function << "()\n";
                logStream << "       " << msg;
                std::cout << logStream.str() << std::endl;
                break;
            case LogType::Error:
                logStream << "ERROR: ";
                logStream << file << ":" << line << " in " << function << "()\n";
                logStream << "       " << msg;
                std::cout << logStream.str() << std::endl;
                break;
            case LogType::Fatal:
                if (SDLRenderer::instance != nullptr) {
                    bool error = true;
                    SDLRenderer::instance->stopRecordingEvents(nullptr, error);
                }
                logStream << "ERROR: ";
                logStream << file << ":" << line << " in " << function << "()\n";
                logStream << "       " << msg;
                std::cout << logStream.str() << std::endl;
                // throw of runtime_error breaks before output at end of function
                logFile << logStream.str() << std::endl;
                logArchiveFile << logStream.str() << std::endl;
                logFile.close();
                logArchiveFile.close();
                Log::AppendLabelToFileStemOrWriteLogIfError(Log::logPath, "_ERROR");
                Log::AppendLabelToFileStemOrWriteLogIfError(Log::logArchivePath, "_ERROR");
                throw std::runtime_error(msg);
                break;
            case LogType::Assert:
                if (SDLRenderer::instance != nullptr) {
                    bool error = true;
                    SDLRenderer::instance->stopRecordingEvents(nullptr, error);
                }
                logStream << "ERROR: ";
                logStream << file << ":" << line << " in " << function << "()\n";
                logStream << "       " << msg;
                std::cout << logStream.str() << std::endl;
                // throw of runtime_error breaks before output at end of function
                logFile << logStream.str() << std::endl;
                logArchiveFile << logStream.str() << std::endl;
                logFile.close();
                logArchiveFile.close();
                Log::AppendLabelToFileStemOrWriteLogIfError(Log::logPath, "_ERROR");
                Log::AppendLabelToFileStemOrWriteLogIfError(Log::logArchivePath, "_ERROR");
                throw std::runtime_error(msg);
                break;
        }
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

    bool Log::CopyFileOrWriteLogIfError(const std::filesystem::path& source, const std::filesystem::path& destination) {
        try {
            const auto replace = std::filesystem::copy_options::overwrite_existing;
            std::filesystem::copy(source, destination, replace);
        } catch (const std::filesystem::filesystem_error& e) {
            std::ostringstream errorMessage;
            errorMessage << "Error copying '" << source << "' to '" << destination << "': " << e.what() << std::endl;
            LOG_ERROR(errorMessage.str().c_str());
            return false;
        }
        return true;
    }

    bool Log::AppendLabelToFileStemOrWriteLogIfError(const std::filesystem::path& filePath, const std::string& label) {
        std::filesystem::path newFilePath;
        std::ostringstream newFileName;
        newFilePath = filePath.parent_path();
        newFileName << filePath.stem().string() << label << filePath.extension().string();
        newFilePath.append(newFileName.str());
        try {
            std::filesystem::rename(filePath, newFilePath);
        } catch (const std::filesystem::filesystem_error& e) {
            std::ostringstream errorMessage;
            errorMessage << "Error renaming '" << filePath << "' to '" <<newFilePath << "': " << e.what() << std::endl;
            LOG_ERROR(errorMessage.str().c_str());
            return false;
        }
        return true;
    }
}
