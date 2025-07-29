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

#pragma once

#include "common.hpp"
#include "data_accessor.hpp"
#include "dbus_accessor.hpp"
#include "event_handler.hpp"
#include "event_info.hpp"
#include "tal_singleton.hpp"
#include "util.hpp"

#include <boost/algorithm/string.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>

#include <chrono>
#include <string>

namespace event_handler
{

class DeviceStatus
{
  public:
    struct Device
    {
        std::string name;
        util::Severity Health;
        util::Severity HealthRollup;
    };

  public:
    DeviceStatus()
    {}

    /**
     * @brief Get Device object from cache by device name. If not exist, create
     * one.
     *
     * @param name
     * @return Device&
     */
    Device& getDevice(const std::string& name)
    {
        for (auto& dev : _device)
        {
            if (dev.name == name)
            {
                return dev;
            }
        }

        std::lock_guard<std::mutex> guard(_mutex);
        Device dev{name, "OK", "OK"};
        _device.push_back(dev);
        return _device.back();
    }

  private:
    std::vector<Device> _device;
    std::mutex _mutex;
};

/**
 * @brief Global cache hold all rollup device status.
 */
DeviceStatus deviceStatus;

nlohmann::json lookupRollupDeviceId(const nlohmann::json& deviceAssociation,
                                    const std::string& deviceId)
{
    const char* const device = deviceId.c_str();
    logs_dbg("Entry lookupRollupDeviceId for deviceId(%s).\n", device);
    auto j = deviceAssociation.find(deviceId);
    if (j != deviceAssociation.end())
    {
        // Found rollup device for deviceId
        logs_dbg("dev(%s)'s rollup dev(%s) found in dat.\n", device,
                 j->dump().c_str());
        return *j;
    }
    else
    {
        logs_dbg("dev(%s) not found in dat, trying [Other] field.\n", device);
        // If deviceId not found in the list, use "[Other]" field info.
        auto jOther = deviceAssociation.find("[Other]");
        if (jOther != deviceAssociation.end())
        {
            // Found rollup device in [Other]
            logs_dbg("[Other] rollup dev: (%s).\n", jOther->dump().c_str());
            return *jOther;
        }
        else
        {
            logs_err("No [Other] field either, it's a problem!\n");
            // If no "[Other]" either, return nothing.
            return nlohmann::json::array();
        }
    }
}

/**
 * @brief A class for update device status into devinfofs.
 *
 */
class DeviceStatusHandler : public EventHandler
{
  public:
    DeviceStatusHandler(const std::string& name = __PRETTY_FUNCTION__) :
        EventHandler(name)
    {}

    ~DeviceStatusHandler() {};

  public:
    /**
     * @brief Read current device status and update into devinfofs,
     * based on first & highest-severity event.
     *
     * @param event
     * @return eventing::RcCode
     */
    eventing::RcCode
        process([[maybe_unused]] event_info::EventNode& event) override
    {
        const char* const device = event.device.c_str();
        const char* const errorId = event.errorId.c_str();

        log_dbg("Entry process (%s:%s)\n", device, errorId);

        if (event.configEventNode.count("managed") == 0)
        {
            log_dbg("Event (%s) is unmanaged by default, no health rollup.\n",
                    event.errorId.c_str());
            return eventing::RcCode::succ;
        }

        if (event.configEventNode["managed"] != "yes")
        {
            log_dbg("Event (%s) is unmanaged, no health rollup.\n",
                    event.errorId.c_str());
            return eventing::RcCode::succ;
        }

        log_dbg("(%s)Needs DeviceStatus evaluation.\n", errorId);
        auto names = lookupRollupDeviceId(eventing::profile::deviceAssociation,
                                          event.device);
        // names is an array of strings
        for (auto& jName : names)
        {
            std::string name = "";
            try
            {
                name = jName.get<std::string>();
            }
            catch (nlohmann::json::exception& e)
            {
                name = "";
                log_err("(%s)Incorrect format of rollup device name in list!\n",
                        errorId);
                log_err("nlohmann exception: %s.\n", e.what());
            }

            if (name.empty())
            {
                log_wrn("This rollup device name is invalid (%s).\n",
                        jName.dump().c_str());
                continue;
            }

            log_dbg("(%s)Checking rollup device(%s) status cache.\n", errorId,
                    name.c_str());
            DeviceStatus::Device& dev = deviceStatus.getDevice(name);
            if (dev.name.empty())
            {
                log_err("(%s)Failed to get DeviceStatus for device(%s)!\n",
                        errorId, device);
                continue;
            }

            log_dbg("(%s)DeviceStatus is valid(%s).", errorId,
                    dev.name.c_str());

            if (dev.Health > event.messageRegistry.message.severity)
            {
                log_dbg(
                    "Lower severity event(%s), no need to update status of (%s).\n",
                    errorId, dev.name.c_str());
                return eventing::RcCode::succ;
            }

            log_err("Event(%s) severity >= cache, updating the cache.\n",
                    errorId);

            nlohmann::json j;

            j["Status"]["Health"] = event.messageRegistry.message.severity;
            j["Status"]["HealthRollup"] = j["Status"]["Health"];
            j["Status"]["Conditions"] = json::array();

            nlohmann::json jCond;

            std::vector<std::string> tokens;
            boost::algorithm::split(tokens, event.getStringMessageArgs(),
                                    boost::is_any_of(","));
            for (auto& token : tokens)
            {
                boost::algorithm::trim(token);
            }
            jCond["MessageArgs"] = nlohmann::json(tokens);

            jCond["MessageId"] = event.getMessageId();
            jCond["OriginOfCondition"] = *event.getOriginOfCondition();
            jCond["Resolution"] = event.messageRegistry.message.resolution;
            jCond["Severity"] = event.messageRegistry.message.severity;
            jCond["Timestamp"] = getTimestamp();
            jCond["Device"] = event.device;
            jCond["ErrorId"] = event.errorId;

            j["Status"]["Conditions"].push_back(jCond);

            log_dbg("json(%s): %s.\n", errorId, j.dump().c_str());

            // Write back to file & update device status cache
            std::lock_guard<std::mutex> guard(_mutex);
            std::filesystem::create_directories(monevtDeviceStatusFSPath);

            std::string filePath =
                monevtDeviceStatusFSPath + std::string("/") + dev.name;

            log_dbg("DevInfoFS path for device(%s): %s.\n", dev.name.c_str(),
                    filePath.c_str());

            int rc = util::file_util::writeJson2File(filePath, j);
#ifdef EVENTING_SERVICE_DEVICE_STATUS_NVIDIA_SHMEM
            auto& mrdInstance = event_tal::TalHealthMRD::getInstance();
            mrdInstance.updateDeviceHealthAndRollup(
                dev.name, j["Status"]["Health"], j["Status"]["HealthRollup"]);
#endif

            if (rc != 0)
            {
                log_err("Save device (%s) status failed, rc = %d!\n",
                        dev.name.c_str(), rc);
                return eventing::RcCode::error;
            }

            log_dbg("Save device (%s) status done.\n", dev.name.c_str());

            // Update Rollup Device Health/Rollup after the devinfofs file
            // updated successfully.
            dev.Health = dev.HealthRollup =
                event.messageRegistry.message.severity;
        }
        return eventing::RcCode::succ;
    }

    /**
     * @brief Get current time and output as Redfish time
     *
     * @return std::string
     */
    std::string getTimestamp()
    {
        auto now = std::chrono::system_clock::now();
        std::time_t now_t = std::chrono::system_clock::to_time_t(now);
        std::tm* now_tm = std::gmtime(&now_t);

        std::stringstream ss;
        ss << std::put_time(now_tm, "%Y-%m-%dT%H:%M:%SZ");

        return ss.str();
    }

  private:
    std::mutex _mutex;
};

} // namespace event_handler
