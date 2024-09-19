/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES.
 * All rights reserved. SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "log.hpp"

namespace logging
{

#if defined(LOG_ELAPSED_TIME)
// initialize static variables
int LogElapsedTime::_deep = -1;
std::vector<std::string> LogElapsedTime::_messages;

void LogElapsedTime::calcTimeStamp()
{
    struct timespec end;
    ::clock_gettime(CLOCK_REALTIME, &end);
    auto seconds = end.tv_sec - _begin.tv_sec;
    auto nanoseconds = end.tv_nsec - _begin.tv_nsec;
    float milli = ((float)nanoseconds / 1000000);
    if (seconds > 0)
    {
        milli += (seconds * 1000);
    }
    char elapsed[128]{0};
    snprintf(elapsed, sizeof(elapsed) - 1, "%9.3f ms %*c %02d ", milli,
             LogElapsedTime::_deep * 2, ' ', LogElapsedTime::_deep);
    auto prevMsg = LogElapsedTime::_messages.at(_msgIndex);
    LogElapsedTime::_messages[_msgIndex] = std::string{elapsed} + prevMsg;
}

void LogElapsedTime::printElapsedTree() const
{
    std::cout << std::endl;
    auto size = LogElapsedTime::_messages.size();
    decltype(size) counter = 0;
    for (; counter < size; ++counter)
    {
        std::cout << LogElapsedTime::_messages.at(counter) << std::endl;
    }
    LogElapsedTime::_messages.clear();
    std::cout << std::endl;
}

LogElapsedTime::LogElapsedTime(const char* file, int line, const char* label,
                               const char* fmt, ...) :
    _msgIndex(LogElapsedTime::_messages.size())
{
    std::string msg{"["};
    msg += label;
    msg += "()]";

    if (fmt != nullptr && *fmt != ' ')
    {
        char usermsg[512] = {'[', 'm', 's', 'g', ':', ' ', 0x00};
        va_list args;
        va_start(args, fmt);
        auto len = ::strlen(usermsg);
        vsnprintf(&usermsg[len], sizeof(usermsg) - len - 2, fmt, args);
        va_end(args);
        msg += usermsg;
        msg += ']';
    }
    msg += ' ';
    auto slash = ::strrchr(file, '/');
    if (slash != nullptr)
    {
        auto last_slash = slash;
        for (slash--; slash > file && *slash != '/'; slash--)
            ;
        file = (*slash == '/') ? slash + 1 : last_slash + 1;
    }
    msg += file;
    msg += ':';
    msg += std::to_string(line);
    LogElapsedTime::_messages.push_back(msg);
    ::clock_gettime(CLOCK_REALTIME, &_begin);
    LogElapsedTime::_deep++;
}

LogElapsedTime::~LogElapsedTime()
{
    calcTimeStamp();
    if (LogElapsedTime::_deep-- == 0)
    {
        printElapsedTree();
    }
}
#endif // LOG_ELAPSED_TIME

} // namespace logging
