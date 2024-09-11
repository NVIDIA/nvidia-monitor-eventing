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

#include "data_structures.hpp"
#include "dbus.hpp"

#include <boost/container/flat_map.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdeventplus/clock.hpp>
#include <sdeventplus/utility/timer.hpp>

#include <memory>
#include <set>
#include <string>

const auto APPNAME = "device-health-service";
const auto APPVER = "0.1";
const auto BUSNAME_DEVICE_HEALTH = "xyz.openbmc_project.DeviceHealthService";
const auto MATCH_RULE_INTERFACES_ADDED =
    "type=signal,interface=org.freedesktop.DBus.ObjectManager,member=InterfacesAdded,path=/xyz/openbmc_project/logging";
const auto MATCH_RULE_LOG_RESOLVED =
    "type=signal,interface=org.freedesktop.DBus.Properties,member=PropertiesChanged,path_namespace=/xyz/openbmc_project/logging/entry";
const auto RESOLVED_PROPERTY_NAME = "Resolved";
const auto LOG_ENTRY_IFACE = "xyz.openbmc_project.Logging.Entry";

using PropertiesChangedMap =
    boost::container::flat_map<std::string, dbus::PropertyVariant>;
using match = sdbusplus::bus::match::match;
using Timer = sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic>;

std::unique_ptr<TmpFileManager> fileManager;
std::set<std::string> deferredDeviceSet;
std::unique_ptr<Timer> deferredDeviceUpdateTimer;

#ifndef DEFERRED_UPDATE_INTERVAL
#define DEFERRED_UPDATE_INTERVAL 10
#endif // DEFERRED_UPDATE_INTERVAL

#ifndef MAX_ERROR_ID_LENGTH
#define MAX_ERROR_ID_LENGTH 100
#endif // MAX_ERROR_ID_LENGTH

std::string getLogEntryIdFromObjectPath(std::string path);
void updateDeviceHealth(const std::string& deviceName);
void addDeferredDevice(std::string device);
void retryDeferredDevices(Timer& timer);
std::string computeHealthForDevice(const std::string& deviceName);
void interfacesAddedCallback(sdbusplus::message_t message);
#if defined(DEASSERTION_PATH_ENABLED) && defined(DEASSERTION_MODE_LOG_RESOLVED)
void logResolvedCallback(sdbusplus::message_t message);
#endif // defined(DEASSERTION_PATH_ENABLED) &&
       // defined(DEASSERTION_MODE_LOG_RESOLVED)
int main(int argc, char* argv[]);
