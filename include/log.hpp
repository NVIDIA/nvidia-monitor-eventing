/**
 * Copyright (c) 2022, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#pragma once

/**
 * SIMLOG - A SIMple LOG
 *
 * as simple as just do,
 *   #include "log.hpp"
 * before using its APIs.
 *
 * It supports to set debug log level in both compiling time or runtime.
 **/

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include <cstdarg>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <vector>

namespace logging
{

#if defined(LOG_ELAPSED_TIME)
/**
 * @brief This class prints information about execution/performance times
 *
 * The macro "log_elapsed()" can used inside any code block (between {}).
 * Usually that macro will appear at beginning of functions, then total elapsed
 *  time of this function will be logged. But it is also possible to use that
 *  inside other blocks such as if/else, loops, etc.
 *
 * It is also possible to specify a user message following printf standard
 *   format, example:  log_elapsed("(inside loop) counter=%d", counter)
 *
 * Entry points are defined by the execution flow, the first macro called will
 *  generate the first entry point (level 00), others called during the
 *  execution flow will added into the tree which is a LIFO queue.
 *
 * The entire tree is printed and emptied when first entry point finishes its
 *   execution.
 *
 * The code example below:
 * @code
        void t2()
        {
            log_elapsed();
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }

        void t3()
        {
            log_elapsed();
            std::this_thread::sleep_for(std::chrono::milliseconds(40));
        }

        void t1()
        {
            log_elapsed();
            const int  totalLoop = 10;
            for (int counter=0; counter < totalLoop; ++counter)
            {
                log_elapsed("(inside loop) counter=%d", counter);
                std::this_thread::sleep_for(std::chrono::milliseconds(8));
            }
        }

        void t0()
        {
            log_elapsed();
            std::this_thread::sleep_for(std::chrono::milliseconds(900));
            t1();
            t2();
        }

        int  main()
        {
           t0();
           return 0;
        }
    @endcode

    Produces the following output:
    @code
     1001.016 ms   00 [t0()] main.cpp:71
       80.847 ms    01 [t1()] main.cpp:60
        8.065 ms      02 [t1()][msg: (inside loop) counter=0 ] main.cpp:64
        8.066 ms      02 [t1()][msg: (inside loop) counter=1 ] main.cpp:64
        8.072 ms      02 [t1()][msg: (inside loop) counter=2 ] main.cpp:64
        8.066 ms      02 [t1()][msg: (inside loop) counter=3 ] main.cpp:64
        8.064 ms      02 [t1()][msg: (inside loop) counter=4 ] main.cpp:64
        8.063 ms      02 [t1()][msg: (inside loop) counter=5 ] main.cpp:64
        8.065 ms      02 [t1()][msg: (inside loop) counter=6 ] main.cpp:64
        8.062 ms      02 [t1()][msg: (inside loop) counter=7 ] main.cpp:64
        8.059 ms      02 [t1()][msg: (inside loop) counter=8 ] main.cpp:64
        8.058 ms      02 [t1()][msg: (inside loop) counter=9 ] main.cpp:64
       20.065 ms    01 [t2()] main.cpp:48
    @endcode
 */
class LogElapsedTime
{
  private:  // static variables
    /** @brief stores the tree messages, it is cleaned after printing */
    static std::vector<std::string> _messages;
    /** @brief stores the the level/deep of current entry point */
    static int _deep;
  private:
    /** @brief time stamp when the entry point starts its execution */
    struct timespec _begin;
    /** @brief index in _messages for this entry point */
    size_t _msgIndex;
  private:
    /** @brief calculates elapsed time remaking the string in _messages
     *          pointed by by _msgIndex */
    void calcTimeStamp();
    /** @brief prints the entire tree and clears _messages */
    void printElapsedTree() const;
  public:
    explicit LogElapsedTime(const char* file, int line, const char *label,
                            const char* fmt = nullptr, ...);
    LogElapsedTime(const LogElapsedTime&) = delete;
    LogElapsedTime() = delete;
    virtual ~LogElapsedTime();
};

#define log_elapsed(fmt, ...) LogElapsedTime __func_performance__(\
    __FILE__, __LINE__, __func__, fmt " ",  ##__VA_ARGS__)
#else
# define log_elapsed(...) /* fmt, ##__VA_ARGS__ */
#endif // LOG_ELAPSED_TIME

/**
 * Set debug level controller by,
 * #define DBG_LOG_CTRL x
 * #include "log.hpp"
 */
#ifndef DBG_LOG_CTRL
#define DBG_LOG_CTRL "mon_evt_ctrl"
#endif

/**
 * Debug Level Definition
 * 0 : Log disabled
 * 1 : Error log only (default)
 * 2 : Error & Warning logs
 * 3 : Error & Warning & Debug logs
 * 4 : Error & Warning & Debug & Info logs
 */
enum LogLevel
{
    disabled = 0,
    error = 1,
    warning = 2,
    debug = 3,
    information = 4,

    dataonly = 0x8000 | information,
};

/**
 * Set default debug level by,
 * #define DEF_DBG_LEVEL x
 * #include "log.hpp"
 */
#ifndef DEF_DBG_LEVEL
#define DEF_DBG_LEVEL disabled
#endif

class Log
{
    using CtrlType = int;
#define getLogLevel(logFlag) ((logFlag)&0x00FF)
#define getLogControl(logFlag) ((logFlag)&0xFF00)
  public:
    Log(const std::string& file = "", CtrlType level = DEF_DBG_LEVEL) :
        logFile(file), logCtrlName(DBG_LOG_CTRL), initLevel(level), seq(0)
    {
        openLogFile();
        smInit();
    }

    ~Log()
    {
        smDeinit();
        closeLogFile();
    }

    void setLevel(CtrlType desiredLevel = DEF_DBG_LEVEL)
    {
        setCtrlLevel(desiredLevel);
    }

    CtrlType getLevel(void)
    {
        return getCtrlLevel();
    }

    void setLogFile(const std::string& file)
    {
        std::lock_guard<std::mutex> logGuard(lMutex);

        closeLogFile();
        logFile = file;
        openLogFile();
    }

    void log(int desiredLevel, const char* fmt, ...)
    {
        if (!isReady
            || getLogLevel(getLevel()) < getLogLevel(desiredLevel))
        {
            // Should not print anything as log is not ready anyways
            return;
        }

        std::lock_guard<std::mutex> logGuard(lMutex);
        std::stringstream ss;
        if (!(getLogControl(desiredLevel) & LogLevel::dataonly))
        {
            // Timestamp
            ss << timestampString();

            // Severity
            switch (getLogLevel(desiredLevel))
            {
                case LogLevel::error:
                    ss << "E";
                    break;
                case LogLevel::warning:
                    ss << "W";
                    break;
                case LogLevel::debug:
                    ss << "D";
                    break;
                case LogLevel::information:
                    ss << "I";
                    break;
                default:
                    ss << "O";
                    break;
            }

            // Message
            char msg[1024] = {0};
            va_list args;
            va_start(args, fmt);
            vsnprintf(msg, sizeof(msg), fmt, args);
            va_end(args);

            ss << msg;

            outputLog(ss.str());
        }
    }

    void log_raw(int desiredLevel, const char* msg,
                 const std::vector<uint8_t>& array, size_t size)
    {
        if (!isReady
            || getLogLevel(getLevel()) < getLogLevel(desiredLevel))
        {
            // Should not print anything as log is not ready anyways
            return;
        }

        std::lock_guard<std::mutex> logGuard(lMutex);
        std::stringstream ss;
        // Timestamp
        ss << timestampString();

        // Severity
        ss << "I";

        ss << "[raw]:";

        // Prompt
        ss << msg;

        // Raw data
        ss << "(" << size << ") ";
        for (size_t i = 0; i < size; i++)
        {
            ss << std::setfill('0') << std::setw(2) << std::hex
                << int(array[i]) << " ";
        }
        ss << "\n";

        outputLog(ss.str());
    }

    void log_raw(int desiredLevel, const char* msg,
                 const std::vector<uint32_t>& array, size_t size)
    {
        if (!isReady
            || getLogLevel(getLevel()) < getLogLevel(desiredLevel))
        {
            // Should not print anything as log is not ready anyways
            return;
        }

        std::lock_guard<std::mutex> logGuard(lMutex);
        std::stringstream ss;
        // Timestamp
        ss << timestampString();

        // Severity
        ss << "I";

        ss << "[raw]:";

        // Prompt
        ss << msg;

        // Raw data
        ss << "(" << size << ") ";
        for (size_t i = 0; i < size; i++)
        {
            ss << std::setfill('0') << std::setw(8) << std::hex
                << int(array[i]) << " ";
        }
        ss << "\n";

        outputLog(ss.str());
    }

    void log_raw(int desiredLevel, const char* msg, const uint8_t* array,
                 size_t size)
    {
        std::vector<uint8_t> arr(array, array + size);
        log_raw(desiredLevel, msg, arr, arr.size());
    }

    void log_raw(int desiredLevel, const char* msg, const uint32_t* array,
                 size_t size)
    {
        std::vector<uint32_t> arr(array, array + size);
        log_raw(desiredLevel, msg, arr, arr.size());
    }

  private:
    std::mutex lMutex;

    std::string logFile;
    std::string logCtrlName;
    std::fstream logStream;

    int smfd;
    CtrlType* ctrlLevel;

    CtrlType initLevel;

    bool isReady;

    unsigned long long seq;

    CtrlType getCtrlLevel() const
    {
        return *ctrlLevel - '0';
    }
    void setCtrlLevel(const CtrlType& level)
    {
        *ctrlLevel = level + '0';
    }

    void openLogFile()
    {
        if (logFile.size() == 0)
        {
            return;
        }

        // to file
        logStream.open(logFile, std::ios::out);
        if (!logStream.is_open())
        {
            throw std::runtime_error("Log file (" + logFile + ") open failed!");
        }
    }

    void closeLogFile()
    {
        // to file
        if (logStream.is_open())
        {
            log(LogLevel::information, "=========== End ===========\n");
            logStream.flush();
            logStream.close();
        }
    }

    void outputLog(const std::string& msg)
    {
        if (logStream.is_open())
        {
            logStream << msg << std::flush;
        }
        else
        {
            std::cout << msg << std::flush;
        }
    }

    const std::string timestampString()
    {
        std::stringstream ss;
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        char buf[100] = {0};
        strftime(buf, sizeof(buf), "%D %T", gmtime(&ts.tv_sec));

        ss << "[" << buf << "." << std::setfill('0') << std::setw(9)
           << ts.tv_nsec << "]";

        return ss.str();
    }

    int smInit()
    {
        bool isFirstSMHdl = true;
        smfd = shm_open(logCtrlName.c_str(), O_EXCL | O_CREAT | O_RDWR,
                        S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
        auto er = errno;
        if (-1 == smfd)
        {
            if (EEXIST == er) // Already exists
            {
                smfd = shm_open(logCtrlName.c_str(), O_RDWR,
                                S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
                if (-1 == smfd)
                {
                    throw std::runtime_error("Log Ctrl open failed (" +
                                             std::string(strerror(er)) + ")!");
                }
                isFirstSMHdl = false;
            }
            else
            {
                throw std::runtime_error("Log Ctrl init failed (" +
                                         std::string(strerror(er)) + ")!");
            }
        }

        int rc = ftruncate(smfd, sizeof(CtrlType));
        if (-1 == rc)
        {
            throw std::runtime_error("SMEM truncate failed!");
        }

        ctrlLevel =
            (CtrlType*)mmap(NULL, sizeof(CtrlType), PROT_READ | PROT_WRITE,
                            MAP_SHARED, smfd, 0);
        if (ctrlLevel == MAP_FAILED)
        {
            throw std::runtime_error("Map failed!");
        }

        if (isFirstSMHdl)
        {
            setLevel(initLevel);
        }

        isReady = true;

        return 0;
    }

    int smDeinit()
    {
        int rc = munmap(ctrlLevel, sizeof(*ctrlLevel));
        if (-1 == rc)
        {
            throw std::runtime_error("Unmap failed!");
        }

        isReady = false;

        rc = close(smfd);
        if (-1 == rc)
        {
            throw std::runtime_error("close(smfd) failed!");
        }

        return 0;
    }
};

} // namespace logging

using namespace logging;
extern Log logger;

/**
 * log_init; is necessarily to be called before using any log function in
 *  a process.
 **/
#define log_init Log logger

/**
 * log_set_file() for changing output
 **/
#define log_set_file(file) logger.setLogFile(file)

/**
 * log_set_level() is to change debug log level
 **/
#define log_set_level(dl) logger.setLevel(dl)

/**
 * log_get_level() is to get debug log level
 **/
#define log_get_level() logger.getLevel()

/**
 * Use any following log functions for debug logs.
 * log_* for using in class non-static member functions
 * logs_* for using in static and global functions
 **/
#define log_err(fmt, ...)                                                      \
    logger.log(LogLevel::error, "[%s][%s]: " fmt, typeid(*this).name(),        \
               __func__, ##__VA_ARGS__)
#define log_wrn(fmt, ...)                                                      \
    logger.log(LogLevel::warning, "[%s][%s]: " fmt, typeid(*this).name(),      \
               __func__, ##__VA_ARGS__)
#define log_dbg(fmt, ...)                                                      \
    logger.log(LogLevel::debug, "[%s][%s]: " fmt, typeid(*this).name(),        \
               __func__, ##__VA_ARGS__)
#define log_info(fmt, ...)                                                     \
    logger.log(LogLevel::information, "[%s][%s]: " fmt, typeid(*this).name(),  \
               __func__, ##__VA_ARGS__)
#define log_info_raw(fmt, array, size)                                         \
    logger.log_raw(LogLevel::information, fmt, array, size)

#define logs_err(fmt, ...)                                                     \
    logger.log(LogLevel::error, "[%s:%d][%s]: " fmt, __FILE__, __LINE__,       \
               __func__, ##__VA_ARGS__)
#define logs_wrn(fmt, ...)                                                     \
    logger.log(LogLevel::warning, "[%s:%d][%s]: " fmt, __FILE__, __LINE__,     \
               __func__, ##__VA_ARGS__)
#define logs_dbg(fmt, ...)                                                     \
    logger.log(LogLevel::debug, "[%s:%d][%s]: " fmt, __FILE__, __LINE__,       \
               __func__, ##__VA_ARGS__)
#define logs_info(fmt, ...)                                                    \
    logger.log(LogLevel::information, "[%s:%d][%s]: " fmt, __FILE__, __LINE__, \
               __func__, ##__VA_ARGS__)
#define logs_info_raw(fmt, array, size) log_info_raw(fmt, array, size)
