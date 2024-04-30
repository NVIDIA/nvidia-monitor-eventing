/**
 * Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/timer.hpp>

#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

using namespace std::chrono_literals;

static bool silent;

void syncGetGpuMgrData(unsigned devId, const std::string& property)
{
    auto bus = sdbusplus::bus::new_default_system();
    auto method = bus.new_method_call(
        "xyz.openbmc_project.GpuMgr", "/xyz/openbmc_project/GpuMgr",
        "xyz.openbmc_project.GpuMgr.Server", "DeviceGetData");
    method.append((int)devId);
    method.append(property);
    method.append(1);
    if (!silent)
    {

        std::cout << "\"busctl call xyz.openbmc_project.GpuMgr "
                  << "/xyz/openbmc_project/GpuMgr "
                  << "xyz.openbmc_project.GpuMgr.Server "
                  << "DeviceGetData isi " << devId << " '" << property
                  << "' 1\""
                  << "... ";
    }
    try
    {
        auto reply = method.call();
        if (!silent)
        {
            std::cout << "OK" << std::endl;
        }
    }
    catch (const std::exception& e)
    {
        if (!silent)
        {
            std::cout << "ERROR: " << e.what() << std::endl;
        }
    }
}

static boost::asio::io_context io;
static volatile unsigned responsesReceived = 0;

void asyncGetGpuMgrData(unsigned responsesLimit, boost::asio::io_context& io,
                        std::shared_ptr<sdbusplus::asio::connection> conn,
                        unsigned devId, const std::string& property)
{
    auto method = conn->new_method_call(
        "xyz.openbmc_project.GpuMgr", "/xyz/openbmc_project/GpuMgr",
        "xyz.openbmc_project.GpuMgr.Server", "DeviceGetData");
    method.append((int)devId);
    method.append(property);
    method.append(1);

    std::string cmd;
    if (!silent)
    {
        std::stringstream ss;
        ss << "\"busctl call xyz.openbmc_project.GpuMgr "
           << "/xyz/openbmc_project/GpuMgr "
           << "xyz.openbmc_project.GpuMgr.Server "
           << "DeviceGetData isi " << devId << " '" << property << "' 1 &\"";
        cmd = ss.str();
        std::cout << "Registering " << cmd << std::endl;
    }

    conn->async_send(
        method, [responsesLimit, cmd, &io](boost::system::error_code ec,
                                           sdbusplus::message::message& ret) {
            if (!silent)
            {
                std::cout << cmd << ": ";
                if (ec || ret.is_method_error())
                {
                    std::cout << "ERROR: " << ec.message() << std::endl;
                }
                else
                {
                    std::cout << "OK" << std::endl;
                }
            }
            unsigned x = responsesReceived;
            x++;
            responsesReceived = x;
            if (x >= responsesLimit)
            {
                io.stop();
            }
        });
}

void usage()
{
    std::cout << "Usage:" << std::endl;
    std::cout << "    dbus_tester (sync|async) [iterations [--silent]]"
              << std::endl;
    std::cout
        << "Perform series of calls: " << std::endl
        << "    busctl call xyz.openbmc_project.GpuMgr "
        << "/xyz/openbmc_project/GpuMgr "
        << "xyz.openbmc_project.GpuMgr.Server "
        << "DeviceGetData isi <i> 'gpu.xid.event' 1" << std::endl
        << "    busctl call xyz.openbmc_project.GpuMgr "
        << "/xyz/openbmc_project/GpuMgr "
        << "xyz.openbmc_project.GpuMgr.Server "
        << "DeviceGetData isi <i> 'gpu.interrupt.PresenceInfo' 1" << std::endl
        << "    busctl call xyz.openbmc_project.GpuMgr "
        << "/xyz/openbmc_project/GpuMgr "
        << "xyz.openbmc_project.GpuMgr.Server "
        << "DeviceGetData isi <i> 'gpu.thermal.temperature.overTemperatureInfo' 1"
        << std::endl
        << "    busctl call xyz.openbmc_project.GpuMgr "
        << "/xyz/openbmc_project/GpuMgr "
        << "xyz.openbmc_project.GpuMgr.Server "
        << "DeviceGetData isi <i> 'gpu.interrupt.powerGoodAbnormalChange' 1"
        << std::endl
        << "for <i> = 0 ... 7." << std::endl;
    std::cout << "Default iterations: 1" << std::endl;
}

int main(int argc, char** argv)
{
    bool syncMode = true;
    if (argc < 2)
    {
        usage();
        return 0;
    }

    if (argc >= 2)
    {
        if (std::string(argv[1]) == "sync")
        {
            syncMode = true;
        }
        else if (std::string(argv[1]) == "async")
        {
            syncMode = false;
        }
        else // ! argv[1]
        {
            std::cerr << "Unrecognized mode: " << argv[1] << std::endl;
            usage();
            return 1;
        }
    }

    unsigned iterationsCount = 1;
    if (argc >= 3)
    {
        try
        {
            int a = std::stoi(argv[2]);
            if (a >= 0)
            {
                iterationsCount = a;
            }
            else // ! a >= 0
            {
                std::cerr << "Number of iterations is less than 0" << std::endl;
                usage();
                return 3;
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "Could not parse number of iterations \"" << argv[2]
                      << "\"" << std::endl;
            usage();
            return 2;
        }
    }

    silent = false;
    if (argc >= 4)
    {
        if (std::string(argv[3]) == "--silent")
        {
            silent = true;
        }
        else // ! argv[3]
        {
            std::cerr << "Unrecognized option: \"" << argv[3] << "\""
                      << std::endl;
            usage();
            return 5;
        }
    }
    else
    {
        silent = false;
    }

    boost::asio::io_context io;
    unsigned devsCount = 8;

    if (syncMode)
    {
        for (unsigned j = 0; j < iterationsCount; ++j)
        {
            if (!silent)
            {
                std::cout << "Iteration: '" << j << "'" << std::endl;
            }
            for (unsigned devId = 0; devId < devsCount; ++devId)
            {
                syncGetGpuMgrData(devId, "gpu.xid.event");
                syncGetGpuMgrData(devId, "gpu.interrupt.PresenceInfo");
                syncGetGpuMgrData(
                    devId, "gpu.thermal.temperature.overTemperatureInfo");
                syncGetGpuMgrData(devId,
                                  "gpu.interrupt.powerGoodAbnormalChange");
            }
        }
    }
    else // ! syncMode
    {
        auto conn = std::make_shared<sdbusplus::asio::connection>(io);
        auto totalResponsesExpected = iterationsCount * devsCount * 4;

        for (unsigned j = 0; j < iterationsCount; ++j)
        {
            if (!silent)
            {
                std::cout << "Iteration: '" << j << "'" << std::endl;
            }
            for (unsigned devId = 0; devId < devsCount; ++devId)
            {
                asyncGetGpuMgrData(totalResponsesExpected, io, conn, devId,
                                   "gpu.xid.event");
                asyncGetGpuMgrData(totalResponsesExpected, io, conn, devId,
                                   "gpu.interrupt.PresenceInfo");
                asyncGetGpuMgrData(
                    totalResponsesExpected, io, conn, devId,
                    "gpu.thermal.temperature.overTemperatureInfo");
                asyncGetGpuMgrData(totalResponsesExpected, io, conn, devId,
                                   "gpu.interrupt.powerGoodAbnormalChange");
            }
        }
        try
        {
            io.run();
        }
        catch (const std::exception& e)
        {
            return 4;
        }
    }

    return 0;
}
