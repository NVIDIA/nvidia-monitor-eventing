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


#include "dbus.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>

#include <string>

// copied from develop, commit d139e40b0cc0c65dd17875aff9167de4ff903d47
namespace dbus
{

bool setDeviceHealth(const std::string& device, const std::string& health)
{
    try
    {
        const std::string healthInterface(
            "xyz.openbmc_project.State.Decorator.Health");
        dbus::DirectObjectMapper om;
        std::vector<std::string> objPathsToAlter =
            om.getAllDevIdObjPaths(device, healthInterface);
        if (!objPathsToAlter.empty())
        {
            bool allSuccess = true;
            for (const auto& objPath : objPathsToAlter)
            {
                // create the HealthType enumeration value
                std::string healthState =
                    "xyz.openbmc_project.State.Decorator.Health.HealthType." +
                    health;

                lg2::info(
                    "Setting Health Property for: {OBJPATH} to: {HEALTHSTATE}",
                    "OBJPATH", objPath, "HEALTHSTATE", healthState);
                bool ok = dbus::setDbusProperty(
                    objPath, "xyz.openbmc_project.State.Decorator.Health",
                    "Health", PropertyVariant(healthState));
                if (ok == true)
                {
                    lg2::info("Changed health property for {OBJPATH}",
                              "OBJPATH", objPath);
                }
                else
                {
                    lg2::warning("Did not change health property for {OBJPATH}",
                                 "OBJPATH", objPath);
                    allSuccess = false;
                }
            }
            return allSuccess;
        }
        else // ! objPathsToAlter.empty()
        {
            // The object path does not exist (yet). This could be due to
            // the hosting service not started yet (hosting service could also
            // be an eventing service, e.g. pldmd for PLDM T2 events), and
            // the device Health service must start before all eventing
            // services. This is not fatal; the code calling this method will
            // implement retries.
            lg2::warning("No object paths found in the subtree of "
                         "'xyz.openbmc_project.ObjectMapper' "
                         "corresponding to device ID {DEVICE} "
                         "and implementing the {INTERFACE} interface",
                         "DEVICE", device, "INTERFACE", healthInterface);
            return false;
        }
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        lg2::error("ERROR WITH SDBUSPLUS BUS: {WHAT}", "WHAT", e.what());
        return false;
    }
}

std::string getService(const std::string& objectPath,
                       const std::string& interface)
{
    constexpr auto mapperBusBame = "xyz.openbmc_project.ObjectMapper";
    constexpr auto mapperObjectPath = "/xyz/openbmc_project/object_mapper";
    constexpr auto mapperInterface = "xyz.openbmc_project.ObjectMapper";

    std::string ret{""};
    std::vector<std::pair<std::string, std::vector<std::string>>> response;
    auto bus = sdbusplus::bus::new_default_system();
    try
    {
        auto method = bus.new_method_call(mapperBusBame, mapperObjectPath,
                                          mapperInterface, "GetObject");
        method.append(std::string(objectPath));
        method.append(std::vector<std::string>({interface}));
        auto reply = method.call();

        reply.read(response);
        if (response.empty() == false)
        {
            ret = response.begin()->first;
            lg2::info("object path {OBJPATH}, interface {INTERFACE} is hosted"
                      " by service {SERVICE}",
                      "OBJPATH", objectPath, "INTERFACE", interface, "SERVICE",
                      ret);
        }
        else
        {
            lg2::warning(
                "getService(): Service not found for {OBJPATH}, {INTERFACE}",
                "OBJPATH", objectPath, "INTERFACE", interface);
        }
    }
    catch (const sdbusplus::exception::exception& e)
    {
        lg2::error(
            "getService(): DBus error for {OBJPATH}, {INTERFACE}: {WHAT}",
            "OBJPATH", objectPath, "INTERFACE", interface, "WHAT", e.what());
    }
    return ret;
}

bool setDbusProperty(const std::string& objPath, const std::string& interface,
                     const std::string& property, const PropertyVariant& val)
{
    return setDbusProperty(getService(objPath, interface), objPath, interface,
                           property, val);
}

bool setDbusProperty(const std::string& service, const std::string& objPath,
                     const std::string& interface, const std::string& property,
                     const PropertyVariant& val)
{
    auto bus = sdbusplus::bus::new_default_system();
    bool ret = false;
    try
    {
        auto method = bus.new_method_call(service.c_str(), objPath.c_str(),
                                          freeDesktopInterface, setCall);
        method.append(interface);
        method.append(property);
        method.append(val);

        ret = true;
        if (!method.call())
        {
            ret = false;
        }
    }
    catch (const sdbusplus::exception::exception& e)
    {
        lg2::error("setDbusProperty() Failed to set property {OBJPATH}, "
                   "{INTERFACE}, {PROPERTY}: {WHAT}",
                   "OBJPATH", objPath, "INTERFACE", interface, "PROPERTY",
                   property, "WHAT", e.what());
    }
    return ret;
}

// DirectObjectMapper /////////////////////////////////////////////////////////

DirectObjectMapper::ValueType DirectObjectMapper::getObjectImpl(
    sdbusplus::bus::bus& bus, const std::string& objectPath,
    const std::vector<std::string>& interfaces) const
{
    ValueType result;
    auto method =
        bus.new_method_call("xyz.openbmc_project.ObjectMapper",
                            "/xyz/openbmc_project/object_mapper",
                            "xyz.openbmc_project.ObjectMapper", "GetObject");
    method.append(objectPath);
    method.append(interfaces);
    auto reply = method.call();
    reply.read(result);
    return result;
}

std::vector<std::string> DirectObjectMapper::getSubTreePathsImpl(
    sdbusplus::bus::bus& bus, const std::string& subtree, int depth,
    const std::vector<std::string>& interfaces) const
{

    std::vector<std::string> result;
    auto method = bus.new_method_call("xyz.openbmc_project.ObjectMapper",
                                      "/xyz/openbmc_project/object_mapper",
                                      "xyz.openbmc_project.ObjectMapper",
                                      "GetSubTreePaths");
    method.append(subtree);
    method.append(depth);
    method.append(interfaces);
    auto reply = method.call();
    reply.read(result);
    return result;
}

DirectObjectMapper::FullTreeType DirectObjectMapper::getSubtreeImpl(
    sdbusplus::bus::bus& bus, const std::string& subtree, int depth,
    const std::vector<std::string>& interfaces) const
{
    FullTreeType result;
    auto method =
        bus.new_method_call("xyz.openbmc_project.ObjectMapper",
                            "/xyz/openbmc_project/object_mapper",
                            "xyz.openbmc_project.ObjectMapper", "GetSubTree");
    method.append(subtree);
    method.append(depth);
    method.append(interfaces);
    auto reply = method.call();
    reply.read(result);
    return result;
}

} // namespace dbus
