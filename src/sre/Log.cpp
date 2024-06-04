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
#include <sre/SDLRenderer.hpp>

namespace sre{
    constexpr int maxErrorSize = 1024;
    char errorMsg[maxErrorSize];
    std::function<void(const char * function,const char * file, int line,LogType, std::string)> Log::logHandler = [](const char * function,const char * file, int line, LogType type, std::string msg){
        switch (type){
            case LogType::Verbose:
                std::cout <<"SRE Verbose: ";
                std::cout <<file<<":"<<line<<" in "<<function<<"()\n";
            case LogType::Info:
                // No prefix
                break;
            case LogType::Warning:
                std::cout <<"SRE Warning: ";
                std::cout <<file<<":"<<line<<" in "<<function<<"()\n";
                break;
            case LogType::Error:
                std::cout <<"SRE Error: ";
                std::cout <<file<<":"<<line<<" in "<<function<<"()\n";
                break;
            case LogType::Fatal:
                if (SDLRenderer::instance != nullptr) {
                    SDLRenderer::instance->stopRecordingEvents();
                }
                std::cout <<"SRE Error: ";
                std::cout <<file<<":"<<line<<" in "<<function<<"()\n";
                throw std::runtime_error(msg);
                break;
        }
        std::cout <<msg<<std::endl;
    };

    std::function<void()> Log::assertHandler = [](){
        if (SDLRenderer::instance != nullptr) {
            SDLRenderer::instance->stopRecordingEvents();
        }
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

    void Log::sreAssert() {
        assertHandler();
    }
}
