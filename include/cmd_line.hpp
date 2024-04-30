/**
 * Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */


#pragma once

#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace cmd_line
{

enum class OptFlag
{
    none,      // No argument option
    overwrite, // Overwrite previous present same argument option
    append,    // Append to previous present same argument options
};
enum class ActFlag
{
    normal, // Normal argument whose action function will be called along with
            // the others. Optional.
    mandatory, // Normal but must present.
    exclusive, // Exclusive argument, only whose action function will be called.
};

bool isArgName(const std::string& str)
{
    return (str[0] == '-');
}

using ArgFuncParamType = std::vector<std::string>;

class CmdLineArg
{
  public:
    /**
     * Support arg instance to be initialized like,
     *  {"-h", "--help", cmd_line::OptFlag::none, "",
     *   cmd_line::ActFlag::exclusive, "Show help message.", show_help}
     */
    CmdLineArg(const std::string& shortArg, const std::string& longArg,
               OptFlag optFlag, const std::string& optFmt, ActFlag actFlag,
               const std::string& helpMsg,
               std::function<int(ArgFuncParamType)> argFunc) :
        shortArg(shortArg),
        longArg(longArg), optFlag(optFlag), optFmt(optFmt), actFlag(actFlag),
        helpMsg(helpMsg), argFunc(argFunc)
    {
        optCount = calcOptCount(optFlag, optFmt);
    }
    CmdLineArg(const CmdLineArg& r) :
        shortArg(r.shortArg), longArg(r.longArg), optFlag(r.optFlag),
        optFmt(r.optFmt), actFlag(r.actFlag), helpMsg(r.helpMsg),
        argFunc(r.argFunc)
    {
        optCount = calcOptCount(optFlag, optFmt);
    }
    ~CmdLineArg()
    {}

    bool needOpt() const
    {
        return ((optFlag == OptFlag::overwrite) ||
                (optFlag == OptFlag::append));
    }

    void addOpt(const std::string& opt)
    {
        if (optFlag == OptFlag::none)
        {
            return;
        }
        if (optFlag == OptFlag::overwrite)
        {
            argOpts.clear();
        }

        argOpts.push_back(opt);
    }

    int action(void)
    {
        return argFunc(argOpts);
    }

    const std::string& getShortArg(void) const
    {
        return shortArg;
    }
    const std::string& getLongArg(void) const
    {
        return longArg;
    }
    const std::string getTag() const
    {
        if (!isArgName(shortArg) && !isArgName(longArg))
        {
            return "cmd";
        }
        else
        {
            return ((shortArg == "") ? longArg : shortArg);
        }
    }
    int getOptCount(void) const
    {
        return optCount;
    }
    bool isMandatory(void) const
    {
        return (actFlag == ActFlag::mandatory);
    }
    bool isExclusive(void) const
    {
        return (actFlag == ActFlag::exclusive);
    }

  private:
    static int calcOptCount(OptFlag flag, const std::string& optFmtStr)
    {
        if (flag == OptFlag::none)
        {
            return 0;
        }
        int count = 0;
        bool wasWS = true;
        for (auto& ch : optFmtStr)
        {
            count += (std::isspace(ch) ? (wasWS ? 0 : !(wasWS = true))
                                       : (wasWS ? (!(wasWS = false)) : 0));
        }
        return count;
    }

  private:
    std::string shortArg; // Short argument name, e.g. "-h"
    std::string longArg;  // Long argument name, e.g. "--help"
    OptFlag optFlag;      // Argument option control flag
    std::string optFmt; // Argument option format message, used for showing help
                        // message and to determine arg options count (by word).
    ActFlag actFlag;    // Argument action control flag
    std::string helpMsg;                          // Argument help message
    std::function<int(ArgFuncParamType)> argFunc; // Argument action function

    std::vector<std::string> argOpts;
    int optCount;

    friend class CmdLine;
};

using CmdLineArgs = std::vector<CmdLineArg>;

class CmdLine
{
  public:
    CmdLine(int argc, char* argv[], CmdLineArgs& cmdLineArgs) :
        argc(argc), curPos(0), cmdLineArgs(cmdLineArgs)
    {
        for (int i = 0; i < argc; i++)
        {
            this->argv.push_back(argv[i]);
        }
    }
    ~CmdLine()
    {
        clearActualArgs();
    }

    int process(void)
    {
        int rc = 0;
        // Show help when no arguments
        if (actualArgs.size() == 0)
        {
            auto it = argIndexes.find("-h");
            if (it == argIndexes.end())
            {
                it = argIndexes.find("--help");
                if (it == argIndexes.end())
                {
                    showHelp(cmdLineArgs);
                    return -1;
                }
                it->second->action();
                return -1;
            }
            it->second->action();
            return -1;
        }

        // Do actions
        for (auto& arg : actualArgs)
        {
            rc = arg.second->action();
            if (rc != 0)
            {
                return rc;
            }
            if (arg.second->isExclusive())
            {
                rc = -1;
            }
        }
        return rc;
    }

    int parse(void)
    {
        buildArgIndexes();
        for (curPos = 1; curPos < argc; curPos++)
        {
            std::string argStr = argv[curPos];

            // Search for the argument
            auto it = argIndexes.find(argStr);
            if (it == argIndexes.end())
            {
                throw std::runtime_error("Uknown argument: " + argStr + "!");
                return -1;
            }

            auto oArg = *(it->second);
            auto argTag = oArg.getTag();

            // Mandatory arg presents, mark it in index.
            if (oArg.isMandatory())
            {
                mandatoryPresenceIndexes[argTag] = true;
            }

            // Check if exclusive argument
            if (oArg.isExclusive())
            {
                clearActualArgs();
                actualArgs[argTag] = new CmdLineArg(oArg);
                return 0;
            }

            // Not exclusive, append into the action list
            auto aIt = actualArgs.find(argTag);
            if (aIt == actualArgs.end())
            {
                actualArgs[argTag] = new CmdLineArg(oArg);
                aIt = actualArgs.find(argTag);
            }

            // Get argument option
            auto arg = aIt->second;
            if (arg->needOpt())
            {
                for (int i = 0; i < arg->getOptCount(); i++)
                {
                    if ((curPos + 1 >= argc) || isArgName(argv[curPos + 1]))
                    {
                        throw std::runtime_error("Argument " + argStr +
                                                 " needs parameter!");
                        return -2;
                    }
                    arg->addOpt(argv[++curPos]);
                }
            }
        }

        // Check mandatory arguments presence
        for (auto& m : mandatoryPresenceIndexes)
        {
            if (m.second == false) // Absent
            {
                throw std::runtime_error("Arg " + m.first + " is needed!");
                return -3;
            }
        }
        return 0;
    }

    int getCurrentArgPosition(void) const
    {
        return curPos;
    }

    static std::string showHelp(const CmdLineArgs& args,
                                uint32_t msgBeginPosition = 14,
                                uint32_t msgEndPosition = 80)
    {
        std::stringstream ss;
        for (auto& arg : args)
        {
            // Output argument name
            std::string str = arg.shortArg;
            if (arg.longArg.size() > 0)
            {
                str += ", " + arg.longArg;
            }
            if (arg.needOpt())
            {
                str += " " + arg.optFmt;
            }
            if (str.size() >= msgBeginPosition)
            {
                str += "\n" + std::string(msgBeginPosition, ' ');
            }
            ss << std::setw(msgBeginPosition) << std::left << str;

            // Output argument help message
            int pos = 0;
            for (auto& ch : arg.helpMsg)
            {
                if (ch == '\n')
                {
                    ss << "\n" << std::string(msgBeginPosition, ' ');
                    pos = 0;
                    continue;
                }
                if (pos && !(pos % (msgEndPosition - msgBeginPosition)))
                {
                    ss << "\n" << std::string(msgBeginPosition, ' ');
                }
                ss << ch;
                pos++;
            }
            ss << "\n";

            // A new line space for the next arg
            ss << "\n";
        }
        return ss.str();
    }

  private:
    void buildArgIndexes(void)
    {
        argIndexes.clear();
        mandatoryPresenceIndexes.clear();
        for (auto& arg : cmdLineArgs)
        {
            auto id = arg.getShortArg();
            if (id != "")
            {
                argIndexes[id] = &arg;
            }
            id = arg.getLongArg();
            if (id != "")
            {
                argIndexes[id] = &arg;
            }
            if (arg.isMandatory())
            {
                mandatoryPresenceIndexes[arg.getTag()] = false;
            }
        }
    }

    void clearActualArgs(void)
    {
        for (auto& arg : actualArgs)
        {
            delete arg.second;
        }
        actualArgs.clear();
    }

  private:
    int argc;
    std::vector<std::string> argv;
    int curPos;

    CmdLineArgs& cmdLineArgs;

    std::map<std::string, CmdLineArg*> argIndexes;
    std::map<std::string, bool> mandatoryPresenceIndexes;
    std::map<std::string, CmdLineArg*> actualArgs;
};

} // namespace cmd_line