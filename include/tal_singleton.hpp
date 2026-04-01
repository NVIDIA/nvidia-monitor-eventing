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
#include "data_accessor.hpp"
#include "dbus_accessor.hpp"
#include "log.hpp"

#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus.hpp>
#ifdef EVENTING_SERVICE_DEVICE_STATUS_NVIDIA_SHMEM
#include <tal.hpp>
#endif

#ifdef EVENTING_SERVICE_DEVICE_STATUS_NVIDIA_SHMEM
namespace event_tal
{
// Define the function type
using updateMRDFunction =
    std::function<void(std::string deviceName, std::string healthStatus)>;

void updateChassisHealthOnSHM(std::string deviceName, std::string healthStatus)
{
    std::string inventoryObjPath = "/health/chassis/" + deviceName;
    std::string ifaceName = "xyz.openbmc_project.State.Decorator.Health";
    std::string propName = "Health";
    std::vector<uint8_t> smbusData = {};
    nv::sensor_aggregation::DbusVariantType propValue{healthStatus};

    auto timestamp = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count());
    tal::TelemetryAggregator::updateTelemetry(
        inventoryObjPath, ifaceName, propName, smbusData, timestamp, 0,
        propValue);
}

void updateChassisHealthRollupOnSHM(std::string deviceName,
                                    std::string healthStatus)
{
    std::string inventoryObjPath = "/health/chassis/" + deviceName;
    std::string ifaceName = "xyz.openbmc_project.State.Decorator.HealthRollup";
    std::string propName = "HealthRollup";
    std::vector<uint8_t> smbusData = {};
    nv::sensor_aggregation::DbusVariantType propValue{healthStatus};

    auto timestamp = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count());

    tal::TelemetryAggregator::updateTelemetry(
        inventoryObjPath, ifaceName, propName, smbusData, timestamp, 0,
        propValue);
}

void updateSystemHealthOnSHM(std::string deviceName, std::string healthStatus)
{
    std::string inventoryObjPath = "/health/system/" + deviceName;
    std::string ifaceName = "xyz.openbmc_project.State.Decorator.Health";
    std::string propName = "Health";
    std::vector<uint8_t> smbusData = {};
    nv::sensor_aggregation::DbusVariantType propValue{healthStatus};

    auto timestamp = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count());
    tal::TelemetryAggregator::updateTelemetry(
        inventoryObjPath, ifaceName, propName, smbusData, timestamp, 0,
        propValue);
}
void updateSystemHealthRollupOnSHM(std::string deviceName,
                                   std::string healthStatus)
{
    std::string inventoryObjPath = "/health/system/" + deviceName;
    std::string ifaceName = "xyz.openbmc_project.State.Decorator.HealthRollup";
    std::string propName = "HealthRollup";
    std::vector<uint8_t> smbusData = {};
    nv::sensor_aggregation::DbusVariantType propValue{healthStatus};

    auto timestamp = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count());

    tal::TelemetryAggregator::updateTelemetry(
        inventoryObjPath, ifaceName, propName, smbusData, timestamp, 0,
        propValue);
}

class TalHealthMRD
{
  public:
    // Map that stores devices to its inventory type
    std::map<std::string, updateMRDFunction> deviceToHealthUpdateMap;
    std::map<std::string, updateMRDFunction> deviceToHealthRollupUpdateMap;
    // Static method to get the singleton instance
    static TalHealthMRD& getInstance()
    {
        static TalHealthMRD instance;
        return instance;
    }
    // Delete copy constructor and assignment operator
    TalHealthMRD()
    {
        // initializeHealthandHealthMRD();
    }
    TalHealthMRD(const TalHealthMRD&) = delete;
    TalHealthMRD& operator=(const TalHealthMRD&) = delete;

    void initializeSharedMemory(const nlohmann::json& deviceAssociation)
    {
        // step 1 : find all root chassis/system devices from .dat file
        for (const auto& [key, value] : deviceAssociation.items())
        {
            bool systemResource = false;
            if (key.find("[Other]") != std::string::npos)
            {
                systemResource = true;
            }

            // Iterate over the array
            for (const auto& item : value)
            {
                std::string value = item.get<std::string>();
                if (systemResource)
                {
                    deviceToHealthUpdateMap[value] = updateSystemHealthOnSHM;
                    deviceToHealthRollupUpdateMap[value] =
                        updateSystemHealthRollupOnSHM;
                }
                else
                {
                    deviceToHealthUpdateMap[value] = updateChassisHealthOnSHM;
                    deviceToHealthRollupUpdateMap[value] =
                        updateChassisHealthRollupOnSHM;
                }
            }
        }
        // step 2: initialize shared memory with default health status
        // initialize health for devices
        for (const auto& [device, healthUpdate] : deviceToHealthUpdateMap)
        {
            healthUpdate(device, "OK");
        }
        // initialize healthrollup for devices
        for (const auto& [device, healthRollupUpdate] :
             deviceToHealthRollupUpdateMap)
        {
            healthRollupUpdate(device, "OK");
        }
    }

    // Initialize the MRD health for all supported redfish resources on the
    // platform

    bool deviceToHealthUpdateMapHasKey(const std::string& key) const
    {
        return deviceToHealthUpdateMap.find(key) !=
               deviceToHealthUpdateMap.end();
    }

    bool deviceToHealthRollupUpdateMapHasKey(const std::string& key) const
    {
        return deviceToHealthUpdateMap.find(key) !=
               deviceToHealthUpdateMap.end();
    }

    void updateDeviceHealthAndRollup(std::string deviceName,
                                     std::string healthStatus,
                                     std::string healthRollUpStatus)
    {
        if (deviceToHealthUpdateMapHasKey(deviceName))
        {
            auto& healthUpdate = deviceToHealthUpdateMap[deviceName];
            healthUpdate(deviceName, healthStatus);
        }
        if (deviceToHealthRollupUpdateMapHasKey(deviceName))
        {
            auto& healthRollupUpdate =
                deviceToHealthRollupUpdateMap[deviceName];
            healthRollupUpdate(deviceName, healthRollUpStatus);
        }
    }
};
} // namespace event_tal
#endif
