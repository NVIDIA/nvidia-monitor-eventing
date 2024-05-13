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

#include "common.hpp"
#include "util.hpp"
#include "data_accessor.hpp"
#include "dbus_accessor.hpp"
#include "event_handler.hpp"
#include "event_info.hpp"

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
    DeviceStatus() {}

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
    if (deviceAssociation.count(deviceId) > 0)
    {
        return deviceAssociation[deviceId];
    }
    else
    {
        // return the device itself as the rollup device if no association
        return nlohmann::json::array();
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
    eventing::RcCode process([[maybe_unused]] event_info::EventNode& event) override
    {
        if (event.configEventNode.count("managed") == 0)
        {
            log_dbg("Event (%s) is unmanaged by default, no health rollup.\n",
                event.errorId);
            return eventing::RcCode::succ;
        }

        if (event.configEventNode["managed"] != "yes")
        {
            log_dbg("Event (%s) is unmanaged, no health rollup.\n",
                event.errorId);
            return eventing::RcCode::succ;
        }

        auto names = lookupRollupDeviceId(eventing::profile::deviceAssociation,
            event.device);
        for(auto& name: names)
        {
            DeviceStatus::Device& dev = deviceStatus.getDevice(name);

            if (dev.Health > event.messageRegistry.message.severity)
            {
                log_dbg("Lower severity event, no need to update status of (%s).",
                    dev.name.c_str());
                return eventing::RcCode::succ;
            }

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

            // Write back to file & update device status cache
            std::lock_guard<std::mutex> guard(_mutex);
            std::filesystem::create_directories(monevtDeviceStatusFSPath);

            std::string filePath = monevtDeviceStatusFSPath + std::string("/")
                 + dev.name;

            log_dbg("DevInfoFS path for device(%s): %s.\n",
                dev.name.c_str(), filePath.c_str());

            int rc = util::file_util::writeJson2File(filePath, j);
            if(rc != 0)
            {
                log_err("Save device (%s) status failed, rc = %d!\n",
                    dev.name.c_str(), rc);
                return eventing::RcCode::error;
            }

            // Update Rollup Device Health/Rollup after the devinfofs file updated successfully.
            dev.Health = dev.HealthRollup = event.messageRegistry.message.severity;
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
