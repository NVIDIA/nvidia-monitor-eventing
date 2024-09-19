/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
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


#include "device_health_service.hpp"

#include "data_structures.hpp"
#include "dbus.hpp"

#include <boost/container/flat_map.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdeventplus/clock.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/utility/timer.hpp>

#include <memory>
#include <regex>
#include <set>
#include <string>
#include <variant>

std::string getLogEntryIdFromObjectPath(std::string path)
{
    std::regex logEntryIdRegex("/xyz/openbmc_project/logging/entry/([0-9]+)");
    std::smatch base_match;
    if (std::regex_match(path, base_match, logEntryIdRegex))
    {
        // match 0 is the entire string, match 1 is the log entry ID
        if (base_match.size() == 2)
        {
            return base_match[1].str();
        }
    }
    return "";
}

void updateDeviceHealth(const std::string& deviceName)
{
    std::string health = computeHealthForDevice(deviceName);
    lg2::warning("Health update: set device {DEVICE_NAME} Health to {HEALTH}",
                 "DEVICE_NAME", deviceName, "HEALTH", health);
    if (!dbus::setDeviceHealth(deviceName, health))
    {
        lg2::warning("updating device Health failed for device {DEVICE_NAME}, "
                     "setting deferred update",
                     "DEVICE_NAME", deviceName);
        addDeferredDevice(deviceName);
    }
}

void addDeferredDevice(std::string device)
{
    lg2::info("add device {DEVICE} to deferred update set", "DEVICE", device);
    deferredDeviceSet.insert(device);
    if (!deferredDeviceUpdateTimer->isEnabled())
    {
        deferredDeviceUpdateTimer->resetRemaining();
        deferredDeviceUpdateTimer->setEnabled(true);
    }
}

void retryDeferredDevices(Timer& timer)
{
    lg2::info("retryDeferredDevices");
    for (auto it = deferredDeviceSet.begin(); it != deferredDeviceSet.end();)
    {
        const auto& device = *it;
        std::string health = computeHealthForDevice(device);
        lg2::info("perform deferred update: set {DEVICE} health to {HEALTH}",
                  "DEVICE", device, "HEALTH", health);
        if (dbus::setDeviceHealth(device, health))
        {
            lg2::info("deferred update on {DEVICE} succeeded", "DEVICE",
                      device);
            it = deferredDeviceSet.erase(it);
        }
        else
        {
            lg2::info("deferred update on {DEVICE} still failed", "DEVICE",
                      device);
            it++;
        }
    }
    if (deferredDeviceSet.size() == 0)
    {
        lg2::info("disable retryDeferredDevices timer");
        timer.setEnabled(false);
    }
    lg2::info("retryDeferredDevices done");
}

std::string computeHealthForDevice(const std::string& deviceName)
{
    if (fileManager->getActiveCriticalErrorCountForDevice(deviceName) > 0)
    {
        return "Critical";
    }
    else if (fileManager->getActiveWarningErrorCountForDevice(deviceName) > 0)
    {
        return "Warning";
    }
    else
    {
        return "OK";
    }
}

void interfacesAddedCallback(sdbusplus::message_t message)
{
    boost::container::flat_map<std::string, PropertiesChangedMap>
        interfacesAddedMap;
    sdbusplus::message::object_path path;
    try
    {
        message.read(path, interfacesAddedMap);
    }
    catch (const std::exception& e)
    {
        lg2::error("error deserializing InterfacesAdded message: {WHAT}",
                   "WHAT", e.what());
        return;
    }

    auto logEntryId = getLogEntryIdFromObjectPath(path);
    lg2::info("get InterfacesAdded message: {PATH}, log entry ID {ID} with "
              "{NUMIFACE} interfaces",
              "PATH", path.str, "ID", logEntryId, "NUMIFACE",
              interfacesAddedMap.size());
    if (!interfacesAddedMap.contains(LOG_ENTRY_IFACE))
    {
        lg2::warning(
            "InterfacesAdded for {ID} does not contain log Entry interface!",
            "ID", logEntryId);
        return;
    }
    auto& props = interfacesAddedMap.at(LOG_ENTRY_IFACE);

    std::string deviceName;

    auto severityVar = props["Severity"];
    auto eventIdVar = props["EventId"];
    auto additionalDataVar = props["AdditionalData"];
    auto resolvedVar = props["Resolved"];
    auto severityPtr = std::get_if<std::string>(&severityVar);
    auto eventIdPtr = std::get_if<std::string>(&eventIdVar);
    auto additionalDataPtr =
        std::get_if<std::vector<std::string>>(&additionalDataVar);
    auto resolvedPtr = std::get_if<bool>(&resolvedVar);
    if (!severityPtr || severityPtr->length() == 0)
    {
        lg2::info("severity missing from log {ID}", "ID", logEntryId);
        return;
    }
    if (!eventIdPtr || eventIdPtr->length() == 0)
    {
        lg2::info("eventId missing from log {ID}", "ID", logEntryId);
        return;
    }
    if (eventIdPtr->length() > MAX_ERROR_ID_LENGTH)
    {
        lg2::info("eventId length is too long ({ACTUAL} > {MAX}) for log {ID}",
                  "ACTUAL", eventIdPtr->length(), "MAX", MAX_ERROR_ID_LENGTH,
                  "ID", logEntryId);
        return;
    }
    if (!resolvedPtr)
    {
        lg2::info("Resolved field missing from log {ID}", "ID", logEntryId);
        return;
    }
    if (!additionalDataPtr)
    {
        lg2::info("additionalData missing from log {ID}", "ID", logEntryId);
        return;
    }
    for (auto& additionalDataItem : *additionalDataPtr)
    {
        // split on '=', look at lhs
        auto eqPos = additionalDataItem.find('=');
        if (eqPos != std::string::npos &&
            eqPos < additionalDataItem.length() - 1)
        {
            // grab device name
            auto additionalDataKey = additionalDataItem.substr(0, eqPos);
            if (additionalDataKey == "DEVICE_NAME")
            {
                deviceName = additionalDataItem.substr(eqPos + 1);
                lg2::info("got device name {DEVICE_NAME} for log {ID}",
                          "DEVICE_NAME", deviceName, "ID", logEntryId);
                break;
            }
        }
    }
    if (deviceName.length() == 0)
    {
        lg2::info("device name missing from log {ID}", "ID", logEntryId);
        return;
    }

    // core logic
    if (*resolvedPtr)
    {
#if defined(DEASSERTION_PATH_ENABLED) && defined(DEASSERTION_MODE_DEASSERT_LOG)
        lg2::warning(
            "error ID {ERROR_ID} resolved=True, removing from {DEVICE_NAME}",
            "ERROR_ID", *eventIdPtr, "DEVICE_NAME", deviceName);
        fileManager->removeErrorFromDevice(*eventIdPtr, deviceName);
#else
        return;
#endif // defined(DEASSERTION_PATH_ENABLED) &&
       // defined(DEASSERTION_MODE_DEASSERT_LOG)
    }
    else
    {
        if (*severityPtr == "xyz.openbmc_project.Logging.Entry.Level.Critical")
        {
            lg2::warning(
                "add Critical error ID {ERROR_ID} to device {DEVICE_NAME}",
                "ERROR_ID", *eventIdPtr, "DEVICE_NAME", deviceName);
            if (!fileManager->addCriticalErrorToDevice(*eventIdPtr, deviceName))
            {
                lg2::error(
                    "add Critical error ID {ERROR_ID} to device {DEVICE_NAME}"
                    " failed, limit of {LIMIT} active error IDs reached",
                    "ERROR_ID", *eventIdPtr, "DEVICE_NAME", deviceName, "LIMIT",
                    MAX_ASSERTED_ERROR_IDS_PER_DEV_SEV);
                return;
            }
        }
        else if (*severityPtr ==
                 "xyz.openbmc_project.Logging.Entry.Level.Warning")
        {
            lg2::warning(
                "add Warning error ID {ERROR_ID} to device {DEVICE_NAME}",
                "ERROR_ID", *eventIdPtr, "DEVICE_NAME", deviceName);
            if (!fileManager->addWarningErrorToDevice(*eventIdPtr, deviceName))
            {
                lg2::error(
                    "add Warning error ID {ERROR_ID} to device {DEVICE_NAME}"
                    " failed, limit of {LIMIT} active error IDs reached",
                    "ERROR_ID", *eventIdPtr, "DEVICE_NAME", deviceName, "LIMIT",
                    MAX_ASSERTED_ERROR_IDS_PER_DEV_SEV);
                return;
            }
        }
        else
        {
            lg2::info(
                "unsupported severity value on assertion log {ID}, no updates",
                "ID", logEntryId);
            return;
        }
    }
    updateDeviceHealth(deviceName);

    lg2::info("finished InterfacesAdded callback");
}

#if defined(DEASSERTION_PATH_ENABLED) && defined(DEASSERTION_MODE_LOG_RESOLVED)
/*
void logResolvedCallback(sdbusplus::message_t message)
{
    std::string msgInterface;
    PropertiesChangedMap propertiesChanged;

    std::string objectPath = message.get_path();
    auto logEntryId = getLogEntryIdFromObjectPath(objectPath);
    message.read(msgInterface, propertiesChanged);

    for (auto& [prop, val] : propertiesChanged)  // TODO: can be direct lookup
    {
        if (prop == RESOLVED_PROPERTY_NAME)
        {
            if (const bool* resolved = std::get_if<bool>(&val))
            {
                std::cerr << "got PropertiesChanged signal for Resolved: " <<
                    std::boolalpha << *resolved << ", log entry ID " <<
                    logEntryId << std::endl;
            }
        }
    }

    //std::string sender = message.get_sender();
}
*/
#endif // defined(DEASSERTION_PATH_ENABLED) &&
       // defined(DEASSERTION_MODE_LOG_RESOLVED)

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
    fileManager = std::make_unique<TmpFileManager>();

    auto bus = sdbusplus::bus::new_default_system();
    auto event = sdeventplus::Event::get_default();
    bus.attach_event(event.get(), SD_EVENT_PRIORITY_NORMAL);

    deferredDeviceUpdateTimer =
        std::make_unique<Timer>(event, &retryDeferredDevices,
                                std::chrono::seconds{DEFERRED_UPDATE_INTERVAL});

    std::set<std::string> existingData = fileManager->getKnownDeviceSet();
    for (const auto& device : existingData)
    {
        updateDeviceHealth(device);
    }

    std::unique_ptr<match> interfacesAddedMatch;
#if defined(DEASSERTION_PATH_ENABLED) && defined(DEASSERTION_MODE_LOG_RESOLVED)
    // std::unique_ptr<match> logResolvedMatch;
#endif // defined(DEASSERTION_PATH_ENABLED) &&
       // defined(DEASSERTION_MODE_LOG_RESOLVED)
    try
    {
        interfacesAddedMatch = std::make_unique<match>(
            bus, MATCH_RULE_INTERFACES_ADDED, &interfacesAddedCallback);
#if defined(DEASSERTION_PATH_ENABLED) && defined(DEASSERTION_MODE_LOG_RESOLVED)
        // logResolvedMatch = std::make_unique<match>(bus,
        //     MATCH_RULE_LOG_RESOLVED,
        //     &logResolvedCallback);
#endif // defined(DEASSERTION_PATH_ENABLED) &&
       // defined(DEASSERTION_MODE_LOG_RESOLVED)
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        lg2::critical("Unable to subscribe: {WHAT}", "WHAT", e.what());
    }

    try
    {
        bus.request_name(BUSNAME_DEVICE_HEALTH);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        lg2::critical("Unable to request bus name: {WHAT}", "WHAT", e.what());
    }
    event.set_watchdog(true);
    return event.loop();
}
