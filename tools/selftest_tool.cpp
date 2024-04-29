/**
 * Copyright (c) 2022, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#include "common.hpp"
#include "cmd_line.hpp"
#include "dat_traverse.hpp"
#include "dbus_accessor.hpp"
#include "selftest.hpp"

#include <dbus_log_utils.hpp>
#include <dbus_utility.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus.hpp>

#include <fstream>
#include <iostream>
#include <map>
#include <queue>
#include <string>
#include <vector>

using namespace std;
using namespace phosphor::logging;

const auto APPNAME = "selftest_tool";
const auto APPVER = "0.1";

/* TODO: multiple log_init causes issues in prog termination */
// log_init;

const std::string default_report_name = "selftest_report.json";

struct Configuration
{
    /** @brief DAT file path **/
    std::string dat;
    /** @brief device name to test or entire tree if empty **/
    std::string targetDevice;
    /** @brief output report file path **/
    std::string report;
};

Configuration configuration = {
    .dat = "", .targetDevice = "", .report = default_report_name};

int loadDAT(cmd_line::ArgFuncParamType params)
{
    if (params[0].size() == 0)
    {
        cout << "Need a parameter!"
             << "\n";
        return -1;
    }

    ifstream f(params[0]);
    if (!f.is_open())
    {
        throw std::runtime_error("File (" + params[0] + ") not found!");
    }

    configuration.dat = params[0];

    return 0;
}

int setLogLevel(cmd_line::ArgFuncParamType params)
{
    int newLvl = std::stoi(params[0]);

    if (newLvl < 0 || newLvl > 5)
    {
        throw std::runtime_error("Level of our range[0-4]!");
    }

    log_set_level(newLvl);

    return 0;
}

int setLogFile(cmd_line::ArgFuncParamType params)
{
    if (!params[0].size())
    {
        throw std::runtime_error("Need a file name!");
    }

    log_set_file(params[0].c_str());

    return 0;
}

int loadTargetDevice(cmd_line::ArgFuncParamType params)
{
    if (params[0].size() == 0)
    {
        cout << "Need a parameter!"
             << "\n";
        return -1;
    }

    configuration.targetDevice = params[0];

    return 0;
}

int loadReportPath(cmd_line::ArgFuncParamType params)
{
    if (params[0].size() == 0)
    {
        cout << "Need a parameter!"
             << "\n";
        return -1;
    }

    configuration.report = params[0];

    return 0;
}

int show_help([[maybe_unused]] cmd_line::ArgFuncParamType params);

cmd_line::CmdLineArgs cmdLineArgs = {
    {"-h", "--help", cmd_line::OptFlag::none, "", cmd_line::ActFlag::exclusive,
     "This help.", show_help},
    {"-d", "", cmd_line::OptFlag::overwrite, "<file>",
     cmd_line::ActFlag::mandatory, "Device Association Tree filename.",
     loadDAT},
    {"-l", "", cmd_line::OptFlag::overwrite, "<level>",
     cmd_line::ActFlag::normal, "Debug Log Level [0-4].", setLogLevel},
    {"-L", "", cmd_line::OptFlag::overwrite, "<file>",
     cmd_line::ActFlag::normal, "Debug Log file. Use stdout if omitted.",
     setLogFile},
    {"-D", "", cmd_line::OptFlag::overwrite, "<device>",
     cmd_line::ActFlag::normal,
     "Target Device for testing. Default: whole DAT tree", loadTargetDevice},
    {"-r", "", cmd_line::OptFlag::overwrite, "<file>",
     cmd_line::ActFlag::normal,
     "Report file path. Default: " + default_report_name, loadReportPath},
};

int show_help([[maybe_unused]] cmd_line::ArgFuncParamType params)
{
    cout << "NVIDIA Selftest Tool, ver = " << APPVER << "\n";
    cout << "<usage>\n";
    cout << "  ./" << APPNAME << " [options]\n";
    cout << "\n";
    cout << "options:\n";
    cout << cmd_line::CmdLine::showHelp(cmdLineArgs);
    cout << "\n";

    return 0;
}

constexpr const char* badHealth = "Critical";
constexpr const char* goodHealth = "OK";

void updateDevicesHealthBasedOnResults(
    selftest::Selftest& selftest, const selftest::ReportResult& reportResult)
{
    std::cout << "About to update devices health. To update: "
              << reportResult.size() << " devices.\n";

    auto bus = sdbusplus::bus::new_default_system();

    for (auto& dev : reportResult)
    {
        std::string deviceHealth =
            selftest.getDeviceTestResult(dev.second);

        std::cout << "Setting health " << dev.first << " = " << deviceHealth
                  << "\r\n";
        selftest.updateDeviceHealth(dev.first, deviceHealth);
    }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Entry Point
 */
int main(int argc, char* argv[])
{
    PROFILING_SWITCH(selftest::TsLatcher TS("selftest-tool-timing"));

    int rc = 0;
    try
    {
        cmd_line::CmdLine cmdLine(argc, argv, cmdLineArgs);
        rc = cmdLine.parse();
        rc = cmdLine.process();
    }
    catch (const std::exception& e)
    {
        logs_err("%s\n", e.what());
        show_help({});
        return rc;
    }

    // std::cout << "DAT: " << configuration.dat << "\n";
    // std::cout << "Target: " << configuration.targetDevice << "\n";
    // std::cout << "Report path: " << configuration.report << "\n";

    std::map<std::string, dat_traverse::Device> datMap;
    dat_traverse::Device::populateMap(datMap, configuration.dat);

    if (configuration.targetDevice.size())
    {
        if (datMap.find(configuration.targetDevice) == datMap.end())
        {
            throw std::runtime_error("Device [" + configuration.targetDevice +
                                     "] not found in provided DAT file!");
        }
    }

    selftest::Selftest selftest("selfTest", datMap);
    selftest::ReportResult reportResult;
    eventing::RcCode result = eventing::RcCode::succ;

    PROFILING_SWITCH(TS.addTimepoint("initialized"));

    try
    {
        if (configuration.targetDevice.size())
        {
            auto& dev = datMap.at(configuration.targetDevice);
            result = selftest.perform(dev, reportResult);
        }
        else
        {
            result = selftest.performEntireTree(reportResult);
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }

    PROFILING_SWITCH(TS.addTimepoint("selftest_finished"));

    if (result != eventing::RcCode::succ)
    {
        return -1;
    }

    updateDevicesHealthBasedOnResults(selftest, reportResult);

    PROFILING_SWITCH(TS.addTimepoint("health_updated_&_log_resolved"));

    selftest::Report reportGenerator(selftest::Report::getDBusFwVersionString);
    if (!reportGenerator.generateReport(reportResult))
    {
        std::cerr << "Error: failed to generate report!" << std::endl;
        return -1;
    }

    std::ofstream ofile(configuration.report);
    ofile << reportGenerator;

    PROFILING_SWITCH(TS.addTimepoint("report_file_written"));

    return 0;
}
