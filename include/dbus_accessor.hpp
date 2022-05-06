/*
 Copyright (c) 2021, NVIDIA CORPORATION.  All rights reserved.

 NVIDIA CORPORATION and its licensors retain all intellectual property
 and proprietary rights in and to this software, related documentation
 and any modifications thereto.  Any use, reproduction, disclosure or
 distribution of this software and related documentation without an express
 license agreement from NVIDIA CORPORATION is strictly prohibited.
*
*/

#pragma once

#include "property_accessor.hpp"

#include <nlohmann/json.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus.hpp>

#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <regex>
#include <string>
#include <tuple>
#include <vector>

namespace data_accessor
{

namespace dbus
{

constexpr auto freeDesktopInterface = "org.freedesktop.DBus.Properties";
constexpr auto getCall = "Get";

using DbusPropertyChangedHandler = std::unique_ptr<sdbusplus::bus::match_t>;
using CallbackFunction = sdbusplus::bus::match::match::callback_t;
using DbusAsioConnection = std::shared_ptr<sdbusplus::asio::connection>;

DbusPropertyChangedHandler
    registerServicePropertyChanged(DbusAsioConnection conn,
                                   const std::string& service,
                                   CallbackFunction callback);

DbusPropertyChangedHandler
    registerServicePropertyChanged(sdbusplus::bus::bus& bus,
                                   const std::string& service,
                                   CallbackFunction callback);

/**
 *  @brief this is the return type for @sa deviceGetCoreAPI()
 */
using RetCoreApi = std::tuple<int, std::string, uint64_t>; // int = return code

/**
 * @brief returns the service assigned with objectPath and interface
 * @param objectPath
 * @param interface
 * @return service name
 */
std::string getService(const std::string& objectPath,
                       const std::string& interface);

/**
 * @brief Performs a DBus call in GpuMgr service calling DeviceGetData method
 * @param devId
 * @param property  the name of the property which defines which data to get
 * @return RetCoreApi with the information
 */
RetCoreApi deviceGetCoreAPI(const int devId, const std::string& property);

/**
 * @brief clear information present GpuMgr service DeviceGetData method
 * @param devId
 * @param property
 * @return 0 meaning success or other value to indicate an error
 */
int deviceClearCoreAPI(const int devId, const std::string& property);

/**
 * @brief getDbusProperty() gets the value from a property in DBUS
 * @param objPath
 * @param interface
 * @param property
 * @return the value based on std::variant
 */
PropertyVariant readDbusProperty(const std::string& objPath,
                                 const std::string& interface,
                                 const std::string& property);

} // namespace dbus

} // namespace data_accessor
